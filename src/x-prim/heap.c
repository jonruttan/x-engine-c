/** @file x-prim/heap.c
 *  @brief GC primitives -- mark/sweep phases, hooks, roots, alloc limit.
 *  @author Jon Ruttan (jonruttan@gmail.com)
 *  @copyright 2026 Jon Ruttan
 *  @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
#include "x-prim.h"
#include "x-eval.h"
#include "x-heap.h"
#include "x-type.h"
#include "x-type/int.h"
#include "x-type/ptr.h"
#include "x-type/str.h"
#include "x-type/symbol.h"
#include "x-obj/prim.h"

/** Fire each hook on a hook list, draining any deferred tail call.  Shared
 *  by the mark and free hook walks (mark_phase / sweep_phase).  A procedure
 *  hook that returns a non-nil tail leaves the env extended and tco_expr/
 *  tco_env set for an outer trampoline; there is none here, so drain it via
 *  x_eval_tco_trampoline -- otherwise the env frame the hook left live is
 *  freed by the sweep and the next eval dereferences it. */
static void x_heap_run_hooks(x_obj_t *p_base, x_obj_t *p_hooks)
{
	x_spair_t hook_args[1];

	hook_args[0][X_OBJ_META_TYPE].p = NULL;
	hook_args[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;

	while ( ! x_obj_isnil(p_base, p_hooks)) {
		x_firstobj((x_obj_t *)hook_args) = x_firstobj(p_hooks);
		x_restobj((x_obj_t *)hook_args) = NULL;
		x_eval_tco_trampoline(p_base,
			x_obj_prim_call(p_base, (x_obj_t *)hook_args));
		p_hooks = x_restobj(p_hooks);
	}
}

/** Mark phase: trace live objects from every root (GC phase 1).
 *
 *  Four passes: (1) fire mark hooks, (2) tree-mark from the base data
 *  tree, (3) root-chain walk -- the off-chain stack objects frames
 *  registered via x_heap_root_push, (4) tree-mark each registered GC
 *  root.  The hook/root lists live in x-expr's heap-group; x-expr
 *  stores but cannot dispatch (no callable mechanism at that layer), so
 *  the walk + invoke happens here.
 *
 *  Hooks MUST fire before the marking passes: everything a hook
 *  allocates is born unmarked, and the sweep that follows this phase
 *  frees every unmarked object.  Firing hooks first means a hook
 *  allocation that escaped into reachable state -- a (heap-mark-root!)
 *  spine cell, a set! into a global -- is marked by the later passes
 *  and survives, while the hook's transient garbage is correctly
 *  swept.  It also makes a root registered mid-collect count in the
 *  same cycle (pass 4 runs after the hooks push it).  With hooks last,
 *  the sweep freed the escaped cells and left reachable dangling
 *  pointers -- a use-after-free on the next collect (usually silent:
 *  the chunk is recycled and the mark walk traverses a reinterpreted
 *  object; ASan catches it only when the chunk stays unreused).
 *
 *  @note A sweep must run with no allocation between it and this mark:
 *        a transient cell allocated *after* the mark is unmarked, so an
 *        intervening sweep frees it while the evaluator is still
 *        traversing it (see x_prim_heap_collect).
 *  @see x_heap_sweep_phase
 */
static void x_heap_mark_phase(x_obj_t *p_base)
{
	x_obj_t *p_roots;

	if (x_base_isset(p_base)) {
		x_heap_run_hooks(p_base,
			x_firstobj(x_base_field_heap_mark_hooks(p_base)));
	}

	x_heap_tree_mark(p_base, x_atomobj(p_base), X_OBJ_FLAG_MARK);
	x_heap_root_chain_mark(p_base, X_OBJ_FLAG_MARK);

	if (x_base_isset(p_base)) {
		p_roots = x_firstobj(x_base_field_heap_mark_roots(p_base));

		while ( ! x_obj_isnil(p_base, p_roots)) {
			x_heap_tree_mark(p_base, x_firstobj(p_roots),
				X_OBJ_FLAG_MARK);
			p_roots = x_restobj(p_roots);
		}
	}
}

/** Sweep phase: fire free hooks, then reclaim unmarked objects (GC phase
 *  2).  x_heap_sweep also clears the mark flag on retained objects, readying
 *  them for the next cycle.
 *  @see x_heap_mark_phase
 */
static void x_heap_sweep_phase(x_obj_t *p_base)
{
	if (x_base_isset(p_base)) {
		x_heap_run_hooks(p_base,
			x_firstobj(x_base_field_heap_free_hooks(p_base)));
	}

	x_heap_sweep(p_base, x_obj_heap(p_base), X_OBJ_FLAG_MARK);
}

/** Sweep unmarked objects from the heap (GC phase 2, low-level).
 *  x-lang: (heap-sweep)
 *
 *  LOW-LEVEL / UNSAFE on its own.  A sweep frees every object not marked
 *  by a *preceding* mark, so calling (heap-sweep) without an immediately
 *  preceding (heap-mark) -- or with any allocation in between -- frees
 *  live data, including the eval-list cell the evaluator is mid-traversal
 *  on.  Use (heap-collect) for a safe, atomic mark+sweep.  This primitive
 *  is exposed only for instrumentation that controls the phases manually
 *  with no intervening allocation.
 *
 *  @param p_base  Base (execution context).
 *  @param p_args  Unused.
 *  @return NULL.
 *  @see x_prim_heap_collect, x_prim_heap_mark, x_prim_heap_count
 */
static x_obj_t *x_prim_heap_sweep(x_obj_t *p_base, x_obj_t *p_args)
{
	(void)p_args;
#ifdef X_PROFILE
	if (x_base_isset(p_base))
		x_atomint(x_firstobj(x_eval_field_profile_gc_runs(p_base)))++;
#endif

	x_heap_sweep_phase(p_base);

	return NULL;
}

/** Count the number of objects currently on the heap.
 *  x-lang: (heap-count)
 *  @param p_base  Base (execution context).
 *  @param p_args  Unused.
 *  @return Integer with the heap object count.
 *  @see x_prim_heap_collect, x_prim_heap_mark, x_prim_heap_sweep
 */
static x_obj_t *x_prim_heap_count(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p = x_obj_heap(p_base);
	long count = 0;
	(void)p_args;

	while (p) {
		count++;
		p = x_obj_heap(p);
	}

	return x_mkint(p_base, count);
}

/** Set the allocation ceiling (the runaway-memory guard).
 *  x-lang: (alloc-limit! n)  -- n > 0 arms the guard; 0 disables (the
 *  default; negative input is treated as 0).
 *
 *  When armed, x_obj_alloc reports an error through the standard error path
 *  and stops the process rather than allocate past n objects; once tripped
 *  the limit latches, so an intercepting guard handler cannot spin it (see
 *  x_obj_alloc).  The trip-message text is stored here at arm time --
 *  x-expr's mechanism layer holds no prose.  Configuration is in-language
 *  (the interpreter reads no environment): the spec runner feeds
 *  (alloc-limit! n) ahead of each library load, so a runaway ./x stops
 *  itself instead of exhausting system memory.  A development guard against
 *  runaway allocation, not a sandbox -- code can re-set or disable it.
 *
 *  @param p_base  Base (execution context).
 *  @param p_args  Unevaluated (n); x_eargs evaluates it.
 *  @return NULL.
 *  @note Fexpr: args unevaluated; x_eargs evaluates n.
 */
static x_obj_t *x_prim_alloc_limit(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_val;
	x_int_t n;

	x_eargs(p_base, p_args, 2, NULL, &p_val);
	n = x_intval(p_val);
	/* The trip message lives with the policy, not the mechanism: x-expr
	 * holds no prose, so arming supplies what the allocator reports.
	 * Stored BEFORE the limit arms: the message string is itself an
	 * allocation, and when the session's count is already past the new
	 * ceiling, storing it after would trip on it -- reporting whatever
	 * message was in the cell beforehand. */
	x_firstobj(x_base_field_alloc_error(p_base)) =
		x_mkstr(p_base, (x_char_t *)"allocation limit exceeded");
	/* Negative cell values are reserved for the allocator's tripped latch. */
	x_atomint(x_firstobj(x_base_field_alloc_limit(p_base))) = n < 0 ? 0 : n;

	return NULL;
}

/** Mark all reachable objects on the heap (GC phase 1, low-level).
 *  x-lang: (heap-mark)
 *
 *  LOW-LEVEL.  Marking alone is harmless (it frees nothing), but a mark
 *  is only useful paired with a sweep that runs with no allocation in
 *  between.  Use (heap-collect) for a safe, atomic mark+sweep.
 *
 *  @param p_base  Base (execution context).
 *  @param p_args  Unused.
 *  @return NULL.
 *  @see x_prim_heap_collect, x_prim_heap_sweep, x_prim_heap_count
 */
static x_obj_t *x_prim_heap_mark(x_obj_t *p_base, x_obj_t *p_args)
{
	(void)p_args;
	x_heap_mark_phase(p_base);

	return NULL;
}

/** Run a full, atomic garbage collection cycle (mark then sweep).
 *  x-lang: (heap-collect)
 *
 *  This is the safe GC entry point.  Mark and sweep run back-to-back in
 *  one C call with no x-lang-level evaluation -- and therefore no
 *  allocation -- between them.  That matters because eval-list scratch
 *  cells (and the env/ctrl/extras half of the base tree) are allocated
 *  X_OBJ_FLAG_NONE and survive a sweep only by being marked.  The mark
 *  phase here runs while the (heap-collect) call's own eval-list frame is
 *  live, so that frame is marked and survives the immediately following
 *  sweep.  Splitting mark and sweep into two separate evaluations (e.g.
 *  (begin (heap-mark) (heap-sweep))) reintroduces an allocation between
 *  them and frees the in-flight frame -- hence (heap-collect), not the
 *  raw phases, is the supported API.
 *
 *  @param p_base  Base (execution context).
 *  @param p_args  Unused.
 *  @return NULL.
 *  @note Increments the GC run counter when X_PROFILE is defined.
 *  @see x_prim_heap_mark, x_prim_heap_sweep, x_prim_heap_count
 */
static x_obj_t *x_prim_heap_collect(x_obj_t *p_base, x_obj_t *p_args)
{
	(void)p_args;
#ifdef X_PROFILE
	if (x_base_isset(p_base))
		x_atomint(x_firstobj(x_eval_field_profile_gc_runs(p_base)))++;
#endif

	x_heap_mark_phase(p_base);
	x_heap_sweep_phase(p_base);

	return NULL;
}

/** Recursively mark an object and all reachable objects as SYSTEM (GC-immune).
 *  x-lang: (gc-pin! obj)
 *  @param p_base  Base (execution context).
 *  @param p_args  Unevaluated argument list (obj).
 *  @return The marked object.
 *  @note Fexpr: args unevaluated; x_eargs evaluates obj.
 *  @note Uses X_OBJ_FLAG_SHARED to make objects immune to GC sweep.
 *  @see x_prim_heap_mark, x_prim_heap_sweep
 */
static x_obj_t *x_prim_system_mark(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj;
	x_eargs(p_base, p_args, 2, NULL, &p_obj);

	/* Reuse the mark traversal with SYSTEM flag */
	x_heap_tree_mark(p_base, p_obj, X_OBJ_FLAG_SHARED);

	return p_obj;
}

/** Register a callable to run during the GC mark phase.
 *  x-lang: (heap-mark-hook! hook)
 *  @param p_base  Base (execution context).
 *  @param p_args  Unevaluated (hook).
 *  @return NULL.
 *  @note Storage in x-expr's heap-group (one canonical location).
 *  @see x_heap_mark_hook_add
 */
static x_obj_t *x_prim_heap_mark_hook(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_hook;
	x_eargs(p_base, p_args, 2, NULL, &p_hook);
	x_heap_mark_hook_add(p_base, p_hook);
	return NULL;
}

/** Register a callable to run during the GC sweep phase.
 *  x-lang: (heap-free-hook! hook)
 *  @param p_base  Base (execution context).
 *  @param p_args  Unevaluated (hook).
 *  @return NULL.
 *  @see x_heap_free_hook_add
 */
static x_obj_t *x_prim_heap_free_hook(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_hook;
	x_eargs(p_base, p_args, 2, NULL, &p_hook);
	x_heap_free_hook_add(p_base, p_hook);
	return NULL;
}

/** Register an object to mark on every collection (extra GC root).
 *  x-lang: (heap-mark-root! obj)
 *  @param p_base  Base (execution context).
 *  @param p_args  Unevaluated (obj).
 *  @return NULL.
 *  @see x_heap_mark_root_add
 */
static x_obj_t *x_prim_heap_mark_root(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj;
	x_eargs(p_base, p_args, 2, NULL, &p_obj);
	x_heap_mark_root_add(p_base, p_obj);
	return NULL;
}


/**
 * @brief Mark a tree with caller-chosen flags. x-lang: (heap tree-mark! obj flags)
 *
 * x_heap_tree_mark is the collector's own traversal, and it takes the flag it
 * sets as a parameter.  Exposed, it answers a question no walk written in
 * x-lang can: what is reachable from here, counting the base sentinel, the
 * custom mark handlers, the mark hooks and the root chain.
 *
 * WHICH FLAG IS THE CALLER'S PROBLEM, and the caller had better not pick one
 * the collector owns: the flag doubles as the traversal's visited test, so
 * X_OBJ_FLAG_SHARED halts at the first base-tree node, and a leftover
 * X_OBJ_FLAG_MARK makes the next mark phase stop short and its sweep free the
 * children it missed.  The layout descriptor names a bit reserved for this.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self obj flags).
 * @return The object marked from.
 */
static x_obj_t *x_prim_heap_tree_mark(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj, *p_flags;

	x_eargs(p_base, p_args, 3, NULL, &p_obj, &p_flags);
	x_heap_tree_mark(p_base, p_obj, (x_obj_flag_t)x_atomint(p_flags));

	return p_obj;
}

/**
 * @brief Clear flags across the allocation chain. x-lang: (heap chain-clear! flags)
 *
 * The counterpart to the above: sweeping would clear the flag too, but
 * sweeping also frees.  A chain clear reaches every object, including whatever
 * became garbage since the mark, which a tree walk would leave flagged.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self flags).
 * @return nil.
 */
static x_obj_t *x_prim_heap_chain_clear(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_flags;

	x_eargs(p_base, p_args, 2, NULL, &p_flags);

	return x_heap_chain_clear(x_obj_heap(p_base),
		(x_obj_flag_t)x_atomint(p_flags));
}


/**
 * @brief One object's unit count, from its type's declared shape.
 *
 * The count is the type's, not the file's: an image carries no length field,
 * because (type set-shape!) already said how many units an instance has.  The
 * negative form is k leading units plus a slot-0-counted payload, and slot 0
 * is the first unit word of the record.
 */
/** @brief Unbox an INT atom from a caller-supplied table, or @p dflt when the
 *  slot is empty.  X fills these tables through (obj set!), which stores an
 *  OBJECT -- reading such a slot as a raw machine word reads the POINTER,
 *  which is how a unit count once came back as 0x3020004fd9d28 and ASan
 *  caught it at the allocation. */
static x_int_t x_image_int(x_obj_t *p_val, x_int_t dflt)
{
	return (p_val == NULL) ? dflt : x_atomint(p_val);
}

static x_int_t x_image_units(x_obj_t *p_type, x_int_t given,
	const x_int_t *w, x_int_t pos)
{
	x_obj_t *p_units;
	x_int_t c;

	/* The three NON-HEAP tags -- structural PAIR, static ATOM, nil-typed --
	 * have no type to ask, so the caller states their count and this trusts
	 * it.  Everything else asks the type, which is the whole point. */
	if (given >= 0)
		return given;

	p_units = (p_type == NULL) ? NULL : x_type_field_units(p_type);
	if (p_units == NULL)
		return 0;

	c = x_type_units_count(p_units);

	return (c < 0) ? w[pos + 2] - c : c;
}

/** @brief The kind of unit @p j, from the type's mask. Units past the mask
 *  repeat its last entry, which is what makes the -k form describable. */
static x_int_t x_image_kind(x_obj_t *p_units, x_int_t j)
{
	x_int_t d, m;

	if (p_units == NULL)
		return X_TYPE_UNIT_WORD;

	d = x_type_units_described(p_units);
	m = x_type_units_mask(p_units);
	if (d < 1)
		d = 1;
	if (j >= d)
		j = d - 1;

	return (m >> (X_TYPE_UNIT_BITS * j)) & ((1 << X_TYPE_UNIT_BITS) - 1);
}

/**
 * @brief Rebuild an image's object graph.
 * x-lang: (image rebuild! buf ostart nobj types foreign statics blob index)
 *
 * THE ONLY PART OF LOADING AN IMAGE THAT IS C, and it is here because it is
 * the only part that is per-object.  Everything else -- verifying the header,
 * resolving foreign names, walking base paths for the statics -- is per-entry
 * work over a few hundred items, which X does in milliseconds.  This is two
 * passes over ~90k records, which X does in ~30s while allocating garbage that
 * nothing collects.
 *
 * The caller passes tables it has already resolved: @p types (slot i = the
 * live type struct for type index i), @p foreign (slot i = the machine word to
 * store), @p statics (slot i = the object a base path reached), and @p index,
 * a pre-allocated table this fills with the rebuilt objects.  Allocating them
 * here would mean choosing a type in C for a table that is X's business.
 *
 * A reference is an index: positive into @p index, negative into @p statics.
 * Out of range is left nil rather than trusted -- the writer emits one-past
 * each table for "nameable by nothing", and a loader that dereferenced that
 * would build a plausible wrong graph.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self buf ostart nobj types foreign statics blob index counts symti nfor nstat).
 * @return The index table pointer, filled.
 */
static x_obj_t *x_prim_image_rebuild(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buf, *p_ostart, *p_nobj, *p_types, *p_foreign;
	x_obj_t *p_statics, *p_blob, *p_index, *p_counts, *p_symti;
	x_obj_t *p_nfor, *p_nstat;
	x_obj_t *p_type, *p_units, *p_obj;
	const x_int_t *w;
	x_obj_t **ix;
	x_int_t ostart, n, blob, nstat, nfor, pos, i, j, ti, units, v, k, given, symti;

	x_eargs(p_base, p_args, 13, NULL, &p_buf, &p_ostart, &p_nobj,
		&p_types, &p_foreign, &p_statics, &p_blob, &p_index, &p_counts,
		&p_symti, &p_nfor, &p_nstat);

	w = (const x_int_t *)x_firstptr(p_buf);
	ostart = x_atomint(p_ostart);
	n = x_atomint(p_nobj);
	blob = x_atomint(p_blob);
	/* THE CALLER STATES THE TABLE SIZES.  x_obj_units reports the TYPE's
	 * declared unit count, not how many units an object was allocated
	 * with -- these tables are allocated with the pair type, so it answers
	 * 2 however large they are.  Bounds-checking against that silently
	 * rejected every index above 1: all 1,271 callables in a rebuilt image
	 * came back with a null call pointer, and every static reference came
	 * back nil. */
	/* THE INDEX TABLE IS RAW MEMORY, not an object.  Allocating an object
	 * of ~95k units for it cost 612ms of a 1.5s load -- more than every
	 * other phase combined and eighty times the rebuild itself.  Nothing
	 * needs the collector to see it: the objects are on the heap chain
	 * from the moment they are allocated, and nothing collects between
	 * here and the caller installing them. */
	ix = (x_obj_t **)x_ptrval(p_index);
	nstat = x_atomint(p_nstat);
	nfor = x_atomint(p_nfor);
	symti = x_atomint(p_symti);

	for (i = 1, pos = ostart; i <= n; i++) {
		ti = w[pos];
		p_type = x_obj(x_obj_data_i(p_types, ti));
		given = x_image_int(x_obj(x_obj_data_i(p_counts, ti)), -1);
		units = x_image_units(p_type, given, w, pos);

		/* A SYMBOL IS REACQUIRED, NOT REBUILT.  Symbols are interned and
		 * the evaluator resolves them by pointer identity, so a freshly
		 * allocated one is a key nothing in the loading process can
		 * match -- every binding in a restored environment would be
		 * unreachable while looking perfectly well-formed.  Interning by
		 * name is the same rule the foreign table follows. */
		if (ti == symti) {
			ix[i] = x_make_symbol(p_base, 0,
				(x_char_t *)(blob + w[pos + 2] + (x_int_t)sizeof(x_int_t)));
			pos += 2 + units;
			continue;
		}

		if (p_type == NULL)
			p_type = x_obj_type(p_index);
		/* SHARED IS REPLAYED; nothing else is.  Most flags describe an
		 * object the allocator has not laid out -- metadata presence,
		 * heap membership -- so handing them to x_obj_alloc claims a
		 * shape that is not there.  SHARED is different in kind: it is
		 * not a description but a POLICY, "the collector may not sweep
		 * this", and x-alist.c's bst_pair sets it on every node of the
		 * global environment tree precisely because that tree outlives
		 * every sweep.  A tree rebuilt without it is a tree the
		 * collector is entitled to free. */
		/* The ATTRIBUTE bits are semantic and must be replayed: WRAP says
		 * a procedure is a wrapped applicative and is called through its
		 * combiner, FRAME and FNFRAME say a spine cell is a lexical
		 * frame, which is what symbol lookup's first step walks.  SHARED
		 * is policy and RO advisory.  META, MARK and OWN are not
		 * replayed: they describe a layout the allocator has not made,
		 * the collector mid-write, and an allocation this object does
		 * not own. */
		ix[i] = x_obj_alloc(p_base, p_type,
			(x_obj_flag_t)(w[pos + 1] & (X_OBJ_FLAG_1 | X_OBJ_FLAG_2
				| X_OBJ_FLAG_3 | X_OBJ_FLAG_4
				| X_OBJ_FLAG_RO | X_OBJ_FLAG_SHARED)),
			(size_t)(units < 1 ? 1 : units));
		pos += 2 + units;
	}

	for (i = 1, pos = ostart; i <= n; i++) {
		ti = w[pos];
		p_type = x_obj(x_obj_data_i(p_types, ti));
		given = x_image_int(x_obj(x_obj_data_i(p_counts, ti)), -1);
		p_units = (p_type == NULL) ? NULL : x_type_field_units(p_type);
		units = x_image_units(p_type, given, w, pos);
		p_obj = ix[i];

		if (ti == symti) {	/* interned above; its bytes are its own */
			pos += 2 + units;
			continue;
		}

		for (j = 0; j < units; j++) {
			v = w[pos + 2 + j];
			/* A stated count means a tag: two units are a pair's
			 * references, one is a word.  Mirrors %over-tw. */
			k = (given >= 0)
				? ((given == 2) ? X_TYPE_UNIT_REF : X_TYPE_UNIT_WORD)
				: x_image_kind(p_units, j);

			if (k == X_TYPE_UNIT_REF) {
				if (v > 0 && v <= n)
					x_obj(x_obj_data_i(p_obj, j)) = ix[v];
				else if (v < 0 && -v < nstat)
					x_obj(x_obj_data_i(p_obj, j)) =
						x_obj(x_obj_data_i(p_statics, -v));
				else
					/* -nstat is the writer's sentinel for a reference
					 * it could not name -- one past the table, like
					 * the foreign sentinel below -- and reading it AS
					 * an entry is a word past the statics object.
					 * That word held whatever the allocator left,
					 * and it sat unnoticed in every object nothing
					 * traversed until the type stacks were installed
					 * and the collector walked through it. */
					x_obj(x_obj_data_i(p_obj, j)) = NULL;
			} else if (k == X_TYPE_UNIT_BYTES) {
				x_ptr(x_obj_data_i(p_obj, j)) =
					(void *)(blob + v + (x_int_t)sizeof(x_int_t));
			} else if (k == X_TYPE_UNIT_FOREIGN) {
				/* BOUNDS-CHECKED like the other two.  The writer emits
				 * one past the table for an address it could not name,
				 * so that a loader can refuse rather than restore a
				 * silent nil -- and reading that sentinel AS an entry
				 * walks off the end of the table.  The refusal has to
				 * be honoured here or the sentinel is worse than the
				 * nil it replaced. */
				x_int(x_obj_data_i(p_obj, j)) = (v > 0 && v < nfor)
					? x_image_int(x_obj(x_obj_data_i(p_foreign, v)), 0)
					: 0;
			} else {
				x_int(x_obj_data_i(p_obj, j)) = v;
			}
		}
		pos += 2 + units;
	}

	return p_index;
}

/** Register the GC primitives. */
x_obj_t *x_prim_heap_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_prim_entry_t entries[] = {
		{ "heap-collect",    x_prim_heap_collect,      "heap", "collect"        },
		{ "heap-mark",       x_prim_heap_mark,         "heap", "mark"           },
		{ "heap-sweep",      x_prim_heap_sweep,        "heap", "sweep"          },
		{ "heap-count",      x_prim_heap_count,        "heap", "count"          },
		{ "alloc-limit!",    x_prim_alloc_limit,       "alloc", "limit!"        },
		{ "heap-mark-hook!", x_prim_heap_mark_hook,    "heap", "mark-hook!"     },
		{ "heap-free-hook!", x_prim_heap_free_hook,    "heap", "free-hook!"     },
		{ "heap-mark-root!", x_prim_heap_mark_root,    "heap", "mark-root!"     },
		{ "gc-pin!",         x_prim_system_mark,       "heap", "pin!"           },
		{ "heap-tree-mark!", x_prim_heap_tree_mark,    "heap", "tree-mark!"     },
		{ "heap-chain-clear!", x_prim_heap_chain_clear, "heap", "chain-clear!" },
		{ "image-rebuild!",  x_prim_image_rebuild,     "image", "rebuild!"      }
	};

	x_prims_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));

	return p_base;
}
