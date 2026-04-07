/** @file x-base.c
 *  @brief Base object construction, error handling, and environment/IO management.
 *  @author Jon Ruttan (jonruttan@gmail.com)
 *  @copyright 2021 Jon Ruttan
 *  @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
/*
 * # Includes
 */
#include "x-base-typesystem.h"
#include "x-type.h"
#include "x-alist.h"
#include "x-eval.h"
#include "x-type/ptr.h"
#include "x-type/str.h"
#include <setjmp.h>

#include "x-token.h"

#define nil			NULL
#define pair(X,Y)	(x_mkspair(p_base, X_OBJ_FLAG_NONE, (X), (Y)))
#define atom(X)		(x_mksatom(p_base, X_OBJ_FLAG_NONE, (X)))

static x_satom_t x_type_prim_type_name_hook =
	x_obj_set(NULL, X_OBJ_FLAG_NONE, { .fn = x_type_prim_type_name });
static x_satom_t x_type_prim_units_hook =
	x_obj_set(NULL, X_OBJ_FLAG_NONE, { .fn = x_type_prim_units });
static x_satom_t x_type_prim_length_hook =
	x_obj_set(NULL, X_OBJ_FLAG_NONE, { .fn = x_type_prim_length });
static x_satom_t x_base_error_hook =
	x_obj_set(NULL, X_OBJ_FLAG_NONE, { .v = (void *)x_base_error });
static x_satom_t x_type_heap_mark_hook =
	x_obj_set(NULL, X_OBJ_FLAG_NONE, { .v = (void *)x_type_heap_mark });
static x_satom_t x_type_heap_free_hook =
	x_obj_set(NULL, X_OBJ_FLAG_NONE, { .v = (void *)x_type_heap_free });

/**
 * Create and initialize a full x-lang base object atop x-expr.
 *
 * Calls x_base_make (x-expr layer) with default file descriptors and
 * hooks, then fills in the type-system-specific slots: env-group
 * (alist, local-boundary, global-tree, shadow-list), ctrl-group
 * (save-stack, error-handler, TCO slots), io-state (line counter,
 * boolean caches), extended profile counters, and project extras
 * (eval-list, token-cache, mark/free hooks, mark-roots).
 *
 * @param p_base  x_obj_t* -- Parent base (or NULL for root)
 * @param p_args  x_obj_t* -- Unused
 * @return x_obj_t* -- Newly constructed base object
 *
 * @details **x-expr vs x-lang layers.**  x_base_make (x-expr) allocates
 *          the base tree skeleton: heap group (pools, GC state), file
 *          descriptors, buffer stack, type-alist slot, profile head
 *          (1 counter for GC cycles), and hook slots.  It leaves env,
 *          ctrl, io-state, and extras as nil.  This function fills all
 *          of those in, giving the base its full evaluator personality.
 *
 * @details **Base tree nodes carry X_OBJ_FLAG_SHARED** (set by x-expr's
 *          x_base_make).  The SHARED flag tells the GC mark phase that
 *          these spine nodes are allocated from the base's own pool and
 *          must be marked but never freed -- they are structurally
 *          permanent for the lifetime of the base.
 *
 * @details **Env-group layout:**
 *          @code
 *          (env-alist . (local-boundary . (global-tree . shadow-list)))
 *          @endcode
 *          - env-alist: linear list of (symbol . value) bindings
 *          - local-boundary: pointer into alist separating locals from globals
 *          - global-tree: BST index over global bindings for O(log n) lookup
 *          - shadow-list: symbols with X_OBJ_FLAG_SHADOW for scope unwinding
 *
 * @details **Ctrl-group layout:**
 *          @code
 *          ((save-stack . (error-handler-slot . nil)) .
 *           ((tco-expr-slot . nil) . (tco-env-slot . nil)))
 *          @endcode
 *
 * @details **Profile counters** (9 additional beyond x-expr's GC counter):
 *          evals, TCO hits, lookups, BST lookups, and internal metrics.
 *
 * @note When @p p_base is non-NULL (child base), boolean caches (#t/#f)
 *       are inherited from the parent so all bases in a tree share the
 *       same singleton boolean objects.
 *
 * @see x_base_error  -- uses the error-handler from ctrl-group
 * @see x_eval        -- uses tco-expr/tco-env from ctrl-group
 */
x_obj_t *x_base_ts_make(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_parent = p_base;
	struct x_base_t base_cfg;

	base_cfg.filein = STDIN_FILENO;
	base_cfg.fileout = STDOUT_FILENO;
	base_cfg.fileerr = STDERR_FILENO;
	base_cfg.p_hook_type_name = (x_obj_t *)x_type_prim_type_name_hook;
	base_cfg.p_hook_units = (x_obj_t *)x_type_prim_units_hook;
	base_cfg.p_hook_length = (x_obj_t *)x_type_prim_length_hook;
	base_cfg.p_hook_error = (x_obj_t *)x_base_error_hook;
	base_cfg.obj_meta_extra = 0;
	base_cfg.p_heap_mark = (x_obj_t *)x_type_heap_mark_hook;
	base_cfg.p_heap_free = (x_obj_t *)x_type_heap_free_hook;
	base_cfg.p_stack_base = nil;

	p_base = x_base_make(p_base, base_cfg);

	/* Set base type (x-expr uses NULL). */
	x_obj_type(p_base) = x_type_base_obj;

	/* Replace x-expr's p_base sentinel in type-alist slot with a real stack cell. */
	x_firstobj(x_firstobj(x_base_field_io_group(p_base))) = pair(nil, nil);

	/* Fill env+ctrl (x-expr leaves nil). */
	x_base_hot(p_base) = pair(
		/* env-group */
		pair(
			/* env-alist */
			pair(nil, nil),
			/* env-aux: (local-boundary . (global-tree . shadow-list)) */
			pair(nil, pair(nil, nil))),
		/* ctrl-group */
		pair(
			/* ctrl-head: (save-stack . error-handler) */
			pair(nil, pair(nil, nil)),
			/* tco: (tco-expr . tco-env) */
			pair(pair(nil, nil), pair(nil, nil))));

	/* Fill io-state (x-expr leaves nil). */
	x_base_io_state(p_base) = pair(
		/* line */
		pair(atom(1), nil),
		/* booleans: (true . false) */
		pair(
			pair(p_parent ? x_firstobj(x_base_field_true(p_parent)) : nil, nil),
			pair(p_parent ? x_firstobj(x_base_field_false(p_parent)) : nil, nil)));

	/* Extend profile (x-expr has 1 counter; we add 9 more). */
	x_restobj(x_base_field_profile(p_base)) =
		pair(pair(atom(0), nil), pair(pair(atom(0), nil),
		pair(pair(atom(0), nil), pair(pair(atom(0), nil),
		pair(pair(atom(0), nil), pair(pair(atom(0), nil),
		pair(pair(atom(0), nil), pair(pair(atom(0), nil),
		pair(pair(atom(0), nil),
		nil)))))))));

	/* Fill x-project-extras (x-expr leaves nil after heap-group). */
	x_base_extras(p_base) = pair(
		/* eval-list */
		pair(nil, nil),
		pair(
			/* token-cache */
			pair(nil, nil),
			pair(
				/* mark-hooks */
				pair(nil, nil),
				pair(
					/* free-hooks */
					pair(nil, nil),
					/* mark-roots */
					pair(nil, nil)))));

	return p_base;
}

#undef nil
#undef pair
#undef atom

/**
 * Signal an error with a message and optional object context.
 *
 * If an error handler is installed (via @c guard), builds a combined
 * error string with line number, restores the saved environment, and
 * longjmps to the handler. Otherwise, writes the error to stderr via
 * the low-level x_error function.
 *
 * @param p_base   x_obj_t* -- Execution context
 * @param message  x_char_t* -- Error message string
 * @param p_obj    x_obj_t* -- Object associated with the error (may be NULL)
 *
 * @details **Zero-allocation error path.**  When a handler is installed,
 *          the message string pointer is stored directly in a static
 *          atom (no malloc, no x_mkstrown).  Message strings from C
 *          callers are always string literals (static storage), so they
 *          survive the longjmp.  The guard handler in x-lang receives
 *          the bare message; x-lang code can add line/symbol context
 *          via (%base) if needed.
 *
 * @details **longjmp protocol.**  The error value is stored in the
 *          handler's error slot, then the env-alist and local-boundary
 *          are restored from the handler's saved copies (captured at
 *          guard installation time).  Finally, longjmp transfers control
 *          to the setjmp site in x_prim_guard.  This unwinds all C
 *          frames between the error site and the guard -- any local
 *          state in those frames is lost.
 *
 * @note When no handler is installed, falls through to x_error which
 *       writes to stderr and is typically fatal.  The interpreter does
 *       NOT abort; the caller may continue if x_error returns.
 *
 * @see x_prim_guard  -- installs the handler and setjmp site
 * @see x_prim_error  -- x-lang (error msg) primitive that calls this
 */
void x_base_error(x_obj_t *p_base, x_char_t *message, x_obj_t *p_obj)
{
	static x_satom_t err_str = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = NULL });
	int fd;
	x_char_t *symbol = NULL;

	/* Extract symbol string from object if possible. */
	if (p_obj != NULL && x_obj_type_issatom(p_obj)) {
		symbol = x_atomstr(p_obj);
	}

	/* If an error handler is installed, store message and longjmp. */
	if (x_base_isset(p_base)
		&& ! x_obj_isnil(p_base, x_firstobj(x_base_field_error_handler(p_base)))) {
		x_obj_t *p_handler = x_firstobj(x_base_field_error_handler(p_base));

		/* Store message in static atom — zero allocation */
		x_atomstr(err_str) = message;
		x_error_handler_error(p_handler) = (x_obj_t *)err_str;

		/* Save error line — raw int in rest slot, zero allocation */
		x_error_handler_line(p_handler)
			= (x_obj_t *)(x_int_t)x_atomint(x_firstobj(x_base_field_line(p_base)));

		x_firstobj(x_base_field_env_alist(p_base))
			= x_error_handler_saved_env(p_handler);
		x_base_field_env_local_boundary(p_base)
			= x_error_handler_saved_boundary(p_handler);
		longjmp(*(jmp_buf *)x_error_handler_jmp(p_handler), 1);
	}

	fd = x_base_isset(p_base) ? x_atomint(x_firstobj(x_base_field_fileerr(p_base))) : STDERR_FILENO;

	x_error(fd, message, symbol);
}

/**
 * Add a type struct to the base's type alist.
 *
 * Wraps the type struct as a (name . type_struct) pair for alist
 * keying and prepends it to the type alist.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- Type struct to register
 * @return x_obj_t* -- The new type alist head, or NULL if base is unset
 */
x_obj_t *x_base_type_alist_extend(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_entry;
	x_spair_t args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL })
	};

	if ( ! x_base_isset(p_base)) {
		return NULL;
	}

	/* Wrap type struct as (name . type_struct) for alist keying */
	p_entry = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_type_field_name(p_args), p_args);
	x_firstobj((x_obj_t *)args) = p_entry;
	x_restobj((x_obj_t *)args) = x_firstobj(x_base_field_type_alist(p_base));

	return x_firstobj(x_base_field_type_alist(p_base)) = x_alist_extend(p_base, (x_obj_t *)args);
}

/**
 * Look up a type struct in the base's type alist by name.
 *
 * Searches for a (name . type_struct) entry matching the first element
 * of @p p_args. Returns the bare type struct (unwrapped from the
 * alist entry), or NULL if not found.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- Pair whose first is the type name to look up
 * @return x_obj_t* -- Type struct, or NULL
 */
x_obj_t *x_base_type_alist_assoc(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_result;
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_firstobj(p_args) }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL })
	};

	if ( ! x_base_isset(p_base)) {
		return NULL;
	}

	x_firstobj((x_obj_t *)args[1]) = x_firstobj(x_base_field_type_alist(p_base));

	p_result = x_alist_assoc(p_base, (x_obj_t *)args);

	/* Unwrap (name . type_struct) entry to return bare type struct */
	return x_obj_isnil(p_base, p_result) ? NULL : x_restobj(p_result);
}

/**
 * Prepend a binding pair to the base's environment alist.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- (symbol . value) pair to prepend
 * @return x_obj_t* -- The new env alist head, or NULL if base is unset
 */
x_obj_t *x_base_env_alist_extend(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_args }, { NULL });

	if ( ! x_base_isset(p_base)) {
		return NULL;
	}

	x_restobj((x_obj_t *)args) = x_firstobj(x_base_field_env_alist(p_base));

	return x_firstobj(x_base_field_env_alist(p_base)) = x_alist_extend(p_base, (x_obj_t *)args);
}

/**
 * Push a file descriptor onto the input file stack.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param fd      x_int_t -- File descriptor to push
 * @return x_obj_t* -- The new top-of-stack atom
 */
x_obj_t *x_base_filein_push(x_obj_t *p_base, x_int_t fd)
{
	x_base_field_filein(p_base) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mksatom(p_base, X_OBJ_FLAG_NONE, fd), x_base_field_filein(p_base));
	return x_firstobj(x_base_field_filein(p_base));
}

/**
 * Pop the top file descriptor from the input file stack.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @return x_obj_t* -- The popped top-of-stack atom
 */
x_obj_t *x_base_filein_pop(x_obj_t *p_base)
{
	x_obj_t *p_top = x_firstobj(x_base_field_filein(p_base));
	x_base_field_filein(p_base) =
		x_restobj(x_base_field_filein(p_base));
	return p_top;
}

/**
 * Push a buffer onto the buffer stack.
 *
 * @param p_base   x_obj_t* -- Execution context
 * @param p_buffer x_obj_t* -- Buffer object to push
 * @return x_obj_t* -- The pushed buffer
 */
x_obj_t *x_base_buffer_push(x_obj_t *p_base, x_obj_t *p_buffer)
{
	x_base_field_buffer(p_base) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		p_buffer, x_base_field_buffer(p_base));
	return p_buffer;
}

/**
 * Pop the top buffer from the buffer stack.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @return x_obj_t* -- The popped buffer object
 */
x_obj_t *x_base_buffer_pop(x_obj_t *p_base)
{
	x_obj_t *p_top = x_firstobj(x_base_field_buffer(p_base));
	x_base_field_buffer(p_base) =
		x_restobj(x_base_field_buffer(p_base));
	return p_top;
}

/**
 * Read and evaluate all expressions from the current buffer.
 *
 * Loops calling x_token_read until EOF, evaluating each expression
 * via x_eval. Returns the result of the last expression.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- Unused
 * @return x_obj_t* -- Result of the last evaluated expression, or NULL
 *
 * @details Reads from the buffer at the top of the buffer stack
 *          (x_base_field_buffer).  The caller is responsible for
 *          pushing the desired buffer before calling this function
 *          (via x_base_buffer_push) and popping it afterward.  Each
 *          read expression is wrapped in a stack-allocated (atom . nil)
 *          eval-args pair and passed to x_eval, which runs the full
 *          evaluator including the TCO trampoline.  The result of each
 *          expression is discarded except the last.
 *
 * @note This is the primary entry point for loading library files.
 *       The shell driver pipes library source via stdin:
 *       @code
 *       cat lib/x.x - | ./x
 *       @endcode
 *       There is no file I/O in the C interpreter; all loading goes
 *       through the buffer/fd mechanism.
 *
 * @see x_base_buffer_push -- push buffer before calling
 * @see x_base_buffer_pop  -- pop buffer after calling
 * @see x_eval             -- evaluator called for each expression
 */
x_obj_t *x_base_load(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_firstobj(x_base_field_buffer(p_base));
	x_obj_t *p_exp, *p_result = NULL;
	x_satom_t exp_wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL });
	x_spair_t eval_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { exp_wrap }, { NULL })
	};
	x_spair_t read_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_buffer }, { p_base })
	};

	for (;;) {
		p_exp = x_token_read(p_base, (x_obj_t *)read_args);
		if (x_obj_isnil(p_base, p_exp)) break;

		x_firstobj((x_obj_t *)exp_wrap) = p_exp;
		p_result = x_eval(p_base, (x_obj_t *)eval_args);
	}

	return p_result;
}

/**
 * Write a string atom to the base's output file descriptor.
 *
 * Extracts the string pointer and optional length from @p p_args,
 * then delegates to x_base_write. If no length is provided, uses
 * strlen.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- (string-atom . optional-length)
 * @return x_obj_t* -- Result of x_base_write
 */
x_obj_t *x_base_write_str(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_atom = x_firstobj(p_args);
	x_char_t *str = x_atomstr(p_atom);
	x_int_t len = x_obj_isnil(p_base, x_restobj(p_args))
		? (x_int_t)x_lib_strlen(str)
		: x_atomint(x_firstobj(x_restobj(p_args)));
	x_satom_t data = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE,
		{ .s = str }),
		sz = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE,
		{ .i = len });
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE,
			{ data }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { sz }, { NULL })
	};

	return x_base_write(p_base, (x_obj_t *)args);
}

