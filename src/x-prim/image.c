/** @file x-prim/image.c
 *  @brief State-image primitives -- image save!, image rebuild!, image write!.
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
#include "x-type/prim.h"
#include "x-type/ptr.h"

/**
 * @brief The type word of an object record, when it is not an object index.
 *
 * A positive type word is the object index of the type struct; these three
 * are the objects that have no struct in the image.  See
 * docs/state-image-format.md, section 3.3.
 */
enum {
	X_IMAGE_ROLE_SPAIR = -1,    /**< A type-struct node, tagged x_type_pair_obj. */
	X_IMAGE_ROLE_SATOM = -2,    /**< A static atom, tagged x_type_atom_obj. */
	X_IMAGE_ROLE_NIL = -3       /**< An untyped object. */
};

/**
 * @brief The layout of one object record: [type][flags][n][kind word]*n.
 */
enum {
	X_IMAGE_RECORD_TYPE = 0,    /**< Object index of the type, or a role. */
	X_IMAGE_RECORD_FLAGS = 1,   /**< The object's flags as written. */
	X_IMAGE_RECORD_COUNT = 2,   /**< n, the unit count. */
	X_IMAGE_RECORD_UNITS = 3,   /**< First unit; each unit is a kind and a word. */
	X_IMAGE_UNIT_WORDS = 2      /**< Words per unit. */
};

/**
 * @brief The flags a rebuilt object keeps from its record.
 *
 * The attribute bits and RO are meaning; the rest describe the allocation
 * that made the original, which this one does not share.
 */
#define X_IMAGE_FLAGS_KEPT   (X_OBJ_FLAG_ATTR_MASK | X_OBJ_FLAG_RO)

/**
 * @brief Everything a rebuild pass reads.
 */
typedef struct {
	const x_int_t *w;           /**< The image, as words. */
	x_int_t ostart;             /**< Word position of the first record. */
	x_int_t n;                  /**< Object count; objects are 1..n. */
	x_obj_t **ix;               /**< Index: entry i is object i. */
	x_obj_t *p_ext;             /**< Externals vector: an object or an INT per entry. */
	x_int_t nextern;            /**< One past the last external; at or beyond is the sentinel. */
	char *blob;                 /**< The bytes section. */
} x_image_t;


/**
 * @brief Words occupied by a record with @p units units.
 */
static x_int_t x_image_record_words(x_int_t units)
{
	return X_IMAGE_RECORD_UNITS + X_IMAGE_UNIT_WORDS * units;
}

/**
 * @brief The type tag for a role, or NULL for a type set later by index.
 */
static x_obj_t *x_image_role_type(x_int_t type)
{
	if (type == X_IMAGE_ROLE_SPAIR) {
		return (x_obj_t *)x_type_pair_obj;
	}

	if (type == X_IMAGE_ROLE_SATOM) {
		return (x_obj_t *)x_type_atom_obj;
	}

	return NULL;
}

/**
 * @brief External entry @p k as an object, or NULL past the table.
 */
static x_obj_t *x_image_extern_obj(const x_image_t *img, x_int_t k)
{
	if (k <= 0 || k >= img->nextern) {
		return NULL;
	}

	return x_obj(x_obj_data_i(img->p_ext, k));
}

/**
 * @brief External entry @p k as an integer, or 0 past the table.
 *
 * X fills the table through (obj set!), which stores an OBJECT; the word
 * is the INT atom's value, not the slot's.
 */
static x_int_t x_image_extern_int(const x_image_t *img, x_int_t k)
{
	x_obj_t *p_val;

	p_val = x_image_extern_obj(img, k);

	if (p_val == NULL) {
		return 0;
	}

	return x_atomint(p_val);
}

/**
 * @brief A ref unit: an object index, or the negated external index.
 */
static x_obj_t *x_image_ref(const x_image_t *img, x_int_t v)
{
	if (v > 0 && v <= img->n) {
		return img->ix[v];
	}

	return x_image_extern_obj(img, -v);
}

/**
 * @brief Store one unit of @p p_obj from its kind and word.
 */
static void x_image_patch_unit(const x_image_t *img, x_obj_t *p_obj,
	x_int_t j, x_int_t kind, x_int_t v)
{
	switch (kind) {
	case X_TYPE_UNIT_REF:
		x_obj(x_obj_data_i(p_obj, j)) = x_image_ref(img, v);
		break;

	case X_TYPE_UNIT_BYTES:
		x_ptr(x_obj_data_i(p_obj, j)) = img->blob + v + sizeof(x_int_t);
		break;

	case X_TYPE_UNIT_FOREIGN:
		x_int(x_obj_data_i(p_obj, j)) = x_image_extern_int(img, v);
		break;

	default:
		x_int(x_obj_data_i(p_obj, j)) = v;
		break;
	}
}

/**
 * @brief Pass 1: allocate every object on this base's chain.
 *
 * A role is typed now; an indexed type is set once its struct exists.
 * Every object is SHARED: the image lives as long as the process.
 */
static void x_image_alloc_pass(x_obj_t *p_base, x_image_t *img)
{
	x_int_t i, pos, units;
	const x_int_t *rec;
	x_obj_flag_t flags;

	pos = img->ostart;

	for (i = 1; i <= img->n; i++) {
		rec = img->w + pos;
		units = rec[X_IMAGE_RECORD_COUNT];
		flags = (x_obj_flag_t)(rec[X_IMAGE_RECORD_FLAGS] & X_IMAGE_FLAGS_KEPT);

		img->ix[i] = x_obj_alloc(p_base,
			x_image_role_type(rec[X_IMAGE_RECORD_TYPE]),
			flags | X_OBJ_FLAG_SHARED,
			(size_t)(units < 1 ? 1 : units));

		pos += x_image_record_words(units);
	}
}

/**
 * @brief Pass 2: type each object by index and fill its units.
 */
static void x_image_patch_pass(x_image_t *img)
{
	x_int_t i, j, pos, units;
	const x_int_t *rec, *unit;
	x_obj_t *p_obj;

	pos = img->ostart;

	for (i = 1; i <= img->n; i++) {
		rec = img->w + pos;
		units = rec[X_IMAGE_RECORD_COUNT];
		p_obj = img->ix[i];

		if (rec[X_IMAGE_RECORD_TYPE] > 0) {
			x_obj_type(p_obj) = img->ix[rec[X_IMAGE_RECORD_TYPE]];
		}

		for (j = 0; j < units; j++) {
			unit = rec + X_IMAGE_RECORD_UNITS + X_IMAGE_UNIT_WORDS * j;
			x_image_patch_unit(img, p_obj, j, unit[0], unit[1]);
		}

		pos += x_image_record_words(units);
	}
}

/**
 * @brief The type's load handler for @p p_obj, or NULL when it has none.
 */
static x_obj_t *x_image_load_handler(x_obj_t *p_base, x_obj_t *p_obj)
{
	x_obj_t *p_type, *p_load;

	p_type = x_obj_type(p_obj);

	if (p_type == NULL || ! x_obj_type_isspair(p_type)) {
		return NULL;
	}

	p_load = x_type_field_load(p_type);

	if (p_load == NULL || x_obj_isnil(p_base, p_load)) {
		return NULL;
	}

	return p_load;
}

/**
 * @brief Pass 3: each type's own load handler, now that the graph is whole.
 *
 * Applied as (load obj) with an evaluated argument.
 */
static void x_image_load_pass(x_obj_t *p_base, x_image_t *img)
{
	x_int_t i;
	x_obj_t *p_obj, *p_load;
	x_spair_t args[2];

	for (i = 1; i <= img->n; i++) {
		p_obj = img->ix[i];
		p_load = x_image_load_handler(p_base, p_obj);

		if (p_load == NULL) {
			continue;
		}

		args[0][X_OBJ_META_TYPE].p = NULL;
		args[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
		args[1][X_OBJ_META_TYPE].p = NULL;
		args[1][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
		x_firstobj((x_obj_t *)args) = p_load;
		x_restobj((x_obj_t *)args) = (x_obj_t *)(args + 1);
		x_firstobj((x_obj_t *)(args + 1)) = p_obj;
		x_restobj((x_obj_t *)(args + 1)) = NULL;

		x_callable_apply(p_base, (x_obj_t *)args);
	}
}

/**
 * @brief Rebuild an image's object graph -- docs/state-image-format.md, section 5.
 *
 * x-lang form: @code (image rebuild! buf ostart nobj externals nextern blob index) @endcode
 *
 * The object table at word @p ostart of @p buf holds @p nobj records with
 * no length word: n is the unit count and each unit carries its kind,
 * exactly as the type's save handler wrote it.  Three passes -- allocate,
 * patch, load -- so that a type struct exists before an instance names it,
 * and every reference is in place before a load handler runs.
 *
 * @param p_base  Base (execution context); the objects join its chain.
 * @param p_args  Unevaluated: (self buf ostart nobj externals nextern blob index).
 * @return @p index, filled so that entry i is object i.
 */
static x_obj_t *x_prim_image_rebuild(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buf, *p_ostart, *p_nobj, *p_ext, *p_nextern, *p_blob, *p_index;
	x_image_t img;

	x_eargs(p_base, p_args, 8, NULL, &p_buf, &p_ostart, &p_nobj,
		&p_ext, &p_nextern, &p_blob, &p_index);

	img.w = (const x_int_t *)x_ptrval(p_buf);
	img.ostart = x_atomint(p_ostart);
	img.n = x_atomint(p_nobj);
	img.ix = (x_obj_t **)x_ptrval(p_index);
	img.p_ext = p_ext;
	img.nextern = x_atomint(p_nextern);
	img.blob = (char *)x_ptrval(p_blob);

	x_image_alloc_pass(p_base, &img);
	x_image_patch_pass(&img);
	x_image_load_pass(p_base, &img);

	return p_index;
}

/**
 * @brief Save a type-struct node: two references.
 */
static void x_image_save_spair(x_obj_t *p_obj, x_int_t *buf)
{
	buf[0] = 2;
	buf[1] = X_TYPE_UNIT_REF;
	buf[2] = x_obj_data_i(p_obj, 0).i;
	buf[3] = X_TYPE_UNIT_REF;
	buf[4] = x_obj_data_i(p_obj, 1).i;
}

/**
 * @brief Save a static-tagged atom or an untyped object: one unit.
 *
 * The word is bytes when the object owns them -- a type handle is its name
 * atom, x_type_atom_obj-tagged and OWN, its word a C string (type.c,
 * make-type) -- and a machine word otherwise.  The rebuild does not keep
 * OWN, so the loaded atom never frees the blob it then points into.
 */
static void x_image_save_word(x_obj_t *p_obj, x_int_t *buf)
{
	buf[0] = 1;
	buf[1] = (x_obj_flags(p_obj) & X_OBJ_FLAG_OWN)
		? X_TYPE_UNIT_BYTES : X_TYPE_UNIT_WORD;
	buf[2] = x_obj_data_i(p_obj, 0).i;
}

/**
 * @brief Save a typed object through its type's save handler.
 *
 * Applied as (save obj buf) with evaluated arguments; a type without a
 * handler of its own gets the default, which walks the units shape.
 */
static void x_image_save_typed(x_obj_t *p_base, x_obj_t *p_obj,
	x_obj_t *p_buf)
{
	x_obj_t *p_save;
	x_spair_t args[3];

	p_save = x_type_field_save(x_obj_type(p_obj));

	if (p_save == NULL || x_obj_isnil(p_base, p_save)) {
		p_save = (x_obj_t *)x_type_save_default_prim;
	}

	args[0][X_OBJ_META_TYPE].p = NULL;
	args[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	args[1][X_OBJ_META_TYPE].p = NULL;
	args[1][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	args[2][X_OBJ_META_TYPE].p = NULL;
	args[2][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	x_firstobj((x_obj_t *)args) = p_save;
	x_restobj((x_obj_t *)args) = (x_obj_t *)(args + 1);
	x_firstobj((x_obj_t *)(args + 1)) = p_obj;
	x_restobj((x_obj_t *)(args + 1)) = (x_obj_t *)(args + 2);
	x_firstobj((x_obj_t *)(args + 2)) = p_buf;
	x_restobj((x_obj_t *)(args + 2)) = NULL;

	x_callable_apply(p_base, (x_obj_t *)args);
}

/**
 * @brief Save one object into @p p_buf by its role or its type; the unit count.
 */
static x_int_t x_image_save(x_obj_t *p_base, x_obj_t *p_obj, x_obj_t *p_buf)
{
	x_obj_t *p_type;
	x_int_t *buf;

	buf = (x_int_t *)x_ptrval(p_buf);
	p_type = x_obj_type(p_obj);

	if (p_type == (x_obj_t *)x_type_pair_obj) {
		x_image_save_spair(p_obj, buf);
	} else if (p_type == NULL || p_type == (x_obj_t *)x_type_atom_obj
			|| ! x_obj_type_isspair(p_type)) {
		x_image_save_word(p_obj, buf);
	} else {
		x_image_save_typed(p_base, p_obj, p_buf);
	}

	return buf[0];
}

/**
 * @brief Save one object's payload through its type -- docs/state-image-format.md, section 4.3.
 *
 * x-lang form: @code (image save! obj buf) @endcode
 *
 * The three roles are structural; everything else is its type's.  @p buf
 * receives [n][kind word]*n.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self obj buf).
 * @return The unit count n, as an integer.
 */
static x_obj_t *x_prim_image_save(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj, *p_buf;

	x_eargs(p_base, p_args, 3, NULL, &p_obj, &p_buf);

	return x_mkint(p_base, x_image_save(p_base, p_obj, p_buf));
}

/**
 * @brief The save buffer's capacity, in units: room for a 200k-slot vector.
 */
#define X_IMAGE_SAVE_UNITS   200000

/**
 * @brief An address-to-integer table: open addressing over a power of two.
 *
 * The object index while an image is written, and the cache of the words
 * the naming callable has already answered.  An empty key is 0, which no
 * address is.
 */
typedef struct {
	x_int_t *keys;              /**< Addresses; 0 marks an empty slot. */
	x_int_t *vals;              /**< The value beside each key. */
	x_int_t mask;               /**< Slot count minus one. */
	x_int_t count;              /**< Keys held. */
} x_image_table_t;

/**
 * @brief Give @p t room for @p n keys at half occupancy or better.
 */
static void x_image_table_init(x_image_table_t *t, x_int_t n)
{
	x_int_t size, i;

	size = 16;

	while (size < 2 * n) {
		size *= 2;
	}

	t->keys = (x_int_t *)x_sys_malloc((size_t)size * sizeof(x_int_t));
	t->vals = (x_int_t *)x_sys_malloc((size_t)size * sizeof(x_int_t));
	t->mask = size - 1;
	t->count = 0;

	for (i = 0; i < size; i++) {
		t->keys[i] = 0;
		t->vals[i] = 0;
	}
}

/**
 * @brief Release @p t's storage.
 */
static void x_image_table_free(x_image_table_t *t)
{
	x_sys_free(t->keys);
	x_sys_free(t->vals);
}

/**
 * @brief The slot holding @p key, or the empty slot it would take.
 */
static x_int_t x_image_table_slot(const x_image_table_t *t, x_int_t key)
{
	x_int_t i;

	i = (key >> 4) & t->mask;

	while (t->keys[i] != 0 && t->keys[i] != key) {
		i = (i + 1) & t->mask;
	}

	return i;
}

/**
 * @brief The value stored for @p key, or 0 when there is none.
 */
static x_int_t x_image_table_get(const x_image_table_t *t, x_int_t key)
{
	x_int_t i;

	i = x_image_table_slot(t, key);

	return (t->keys[i] == key) ? t->vals[i] : 0;
}

/**
 * @brief Store @p val for @p key, doubling the table past half occupancy.
 */
static void x_image_table_put(x_image_table_t *t, x_int_t key, x_int_t val)
{
	x_image_table_t bigger;
	x_int_t i;

	if (2 * (t->count + 1) > t->mask + 1) {
		x_image_table_init(&bigger, 2 * (t->mask + 1));

		for (i = 0; i <= t->mask; i++) {
			if (t->keys[i] != 0) {
				x_image_table_put(&bigger, t->keys[i], t->vals[i]);
			}
		}

		x_image_table_free(t);
		*t = bigger;
	}

	i = x_image_table_slot(t, key);

	if (t->keys[i] == 0) {
		t->count++;
	}

	t->keys[i] = key;
	t->vals[i] = val;
}

/**
 * @brief Everything a write pass reads and fills.
 */
typedef struct {
	x_obj_t *p_base;            /**< The base the write runs on. */
	x_obj_t *p_cursor;          /**< First object of the chain walked. */
	x_int_t flag;               /**< The flag bit marking an imaged object. */
	x_int_t *table;             /**< The object table, as words. */
	x_int_t table_cap;          /**< Its capacity, in words. */
	x_int_t table_pos;          /**< Words written. */
	char *blob;                 /**< The bytes section. */
	x_int_t blob_cap;           /**< Its capacity, in bytes. */
	x_int_t blob_pos;           /**< Bytes written. */
	x_obj_t *p_name;            /**< (name word kind obj): an external index, or nil. */
	x_obj_t *p_buf;             /**< The save buffer, as the PTR the type saves take. */
	x_int_t *buf;               /**< The same buffer, as words. */
	x_image_table_t index;      /**< Object address to object index. */
	x_image_table_t externs;    /**< Word to external index plus one. */
	x_int_t n;                  /**< Objects imaged. */
	x_int_t sent;               /**< Distinct words no name was given for. */
} x_image_writer_t;

/**
 * @brief Whether @p p_obj is in the image: it carries the writer's flag.
 */
static int x_image_imaged(const x_image_writer_t *w, x_obj_t *p_obj)
{
	return (x_obj_flags(p_obj) & w->flag) != 0;
}

/**
 * @brief Pass 1: count the flagged objects and give each its index.
 *
 * Indices follow the chain, most recent allocation first, as the loader
 * will rebuild them.
 */
static void x_image_count_pass(x_image_writer_t *w)
{
	x_obj_t *p_obj;
	x_int_t i;

	w->n = 0;

	for (p_obj = w->p_cursor; p_obj != NULL; p_obj = x_obj_heap(p_obj)) {
		if (x_image_imaged(w, p_obj)) {
			w->n++;
		}
	}

	x_image_table_init(&w->index, w->n);
	i = 0;

	for (p_obj = w->p_cursor; p_obj != NULL; p_obj = x_obj_heap(p_obj)) {
		if (x_image_imaged(w, p_obj)) {
			i++;
			x_image_table_put(&w->index, (x_int_t)p_obj, i);
		}
	}
}

/**
 * @brief The external index for @p word, asking the naming callable once.
 *
 * Applied as (name word kind obj) with evaluated arguments; an integer
 * answer is the index, anything else means the word has no name and is
 * written as the sentinel.  Every answer is cached, so a word is asked
 * about once however often it occurs.
 */
static x_int_t x_image_extern(x_image_writer_t *w, x_int_t word, x_int_t kind,
	x_obj_t *p_obj)
{
	x_spair_t args[4];
	x_obj_t *p_k;
	x_int_t k, cached;

	cached = x_image_table_get(&w->externs, word);

	if (cached != 0) {
		return cached - 1;
	}

	args[0][X_OBJ_META_TYPE].p = NULL;
	args[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	args[1][X_OBJ_META_TYPE].p = NULL;
	args[1][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	args[2][X_OBJ_META_TYPE].p = NULL;
	args[2][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	args[3][X_OBJ_META_TYPE].p = NULL;
	args[3][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	x_firstobj((x_obj_t *)args) = w->p_name;
	x_restobj((x_obj_t *)args) = (x_obj_t *)(args + 1);
	x_firstobj((x_obj_t *)(args + 1)) = x_mkint(w->p_base, word);
	x_restobj((x_obj_t *)(args + 1)) = (x_obj_t *)(args + 2);
	x_firstobj((x_obj_t *)(args + 2)) = x_mkint(w->p_base, kind);
	x_restobj((x_obj_t *)(args + 2)) = (x_obj_t *)(args + 3);
	x_firstobj((x_obj_t *)(args + 3)) = p_obj;
	x_restobj((x_obj_t *)(args + 3)) = NULL;

	p_k = x_callable_apply(w->p_base, (x_obj_t *)args);
	k = (p_k == NULL || x_obj_isnil(w->p_base, p_k)) ? 0 : x_atomint(p_k);

	if (k == 0) {
		w->sent++;
	}

	x_image_table_put(&w->externs, word, k + 1);

	return k;
}

/**
 * @brief A reference unit's word: the object's index, or the negated
 * external index of an object outside the image.
 */
static x_int_t x_image_ref_word(x_image_writer_t *w, x_int_t word,
	x_obj_t *p_obj)
{
	x_int_t i;

	if (word == 0) {
		return 0;
	}

	i = x_image_table_get(&w->index, word);

	if (i != 0) {
		return i;
	}

	return -x_image_extern(w, word, X_TYPE_UNIT_REF, p_obj);
}

/**
 * @brief A bytes unit's word: the blob offset of [len][bytes NUL].
 *
 * A NULL pointer becomes an empty string.  Byte offsets, unaligned: the
 * length is copied in, not stored.
 */
static x_int_t x_image_bytes_word(x_image_writer_t *w, x_int_t word)
{
	x_int_t off, len;

	off = w->blob_pos;
	len = (word == 0) ? 0 : (x_int_t)x_lib_strlen((x_char_t *)word);

	if (off + (x_int_t)sizeof(x_int_t) + len + 1 > w->blob_cap) {
		x_eval_error(w->p_base, (x_char_t *)"image write!: blob full", NULL);
	}

	x_lib_memcpy(w->blob + off, &len, sizeof(x_int_t));

	if (len > 0) {
		x_lib_memcpy(w->blob + off + sizeof(x_int_t), (void *)word, (size_t)len);
	}

	w->blob[off + (x_int_t)sizeof(x_int_t) + len] = '\0';
	w->blob_pos = off + (x_int_t)sizeof(x_int_t) + len + 1;

	return off;
}

/**
 * @brief A foreign unit's word: the external index, or 0.
 */
static x_int_t x_image_foreign_word(x_image_writer_t *w, x_int_t word,
	x_obj_t *p_obj)
{
	if (word == 0) {
		return 0;
	}

	return x_image_extern(w, word, X_TYPE_UNIT_FOREIGN, p_obj);
}

/**
 * @brief A record's type word: a role, or the index of the type struct.
 */
static x_int_t x_image_type_word(x_image_writer_t *w, x_obj_t *p_obj)
{
	x_obj_t *p_type;
	x_int_t i;

	p_type = x_obj_type(p_obj);

	if (p_type == NULL) {
		return X_IMAGE_ROLE_NIL;
	}

	if (p_type == (x_obj_t *)x_type_pair_obj) {
		return X_IMAGE_ROLE_SPAIR;
	}

	if (p_type == (x_obj_t *)x_type_atom_obj || ! x_obj_type_isspair(p_type)) {
		return X_IMAGE_ROLE_SATOM;
	}

	i = x_image_table_get(&w->index, (x_int_t)p_type);

	if (i == 0) {
		x_eval_error(w->p_base, (x_char_t *)"image write!: type not imaged", p_type);
	}

	return i;
}

/**
 * @brief Write one object's record: [type][flags][n][kind word]*n.
 */
static void x_image_emit_object(x_image_writer_t *w, x_obj_t *p_obj)
{
	x_int_t n, j, kind, word;
	x_int_t *rec;

	n = x_image_save(w->p_base, p_obj, w->p_buf);

	if (w->table_pos + x_image_record_words(n) > w->table_cap) {
		x_eval_error(w->p_base, (x_char_t *)"image write!: object table full", NULL);
	}

	rec = w->table + w->table_pos;
	rec[X_IMAGE_RECORD_TYPE] = x_image_type_word(w, p_obj);
	rec[X_IMAGE_RECORD_FLAGS] = x_obj_flags(p_obj) & X_IMAGE_FLAGS_KEPT;
	rec[X_IMAGE_RECORD_COUNT] = n;

	for (j = 0; j < n; j++) {
		kind = w->buf[1 + X_IMAGE_UNIT_WORDS * j];
		word = w->buf[2 + X_IMAGE_UNIT_WORDS * j];

		switch (kind) {
		case X_TYPE_UNIT_REF:
			word = x_image_ref_word(w, word, p_obj);
			break;

		case X_TYPE_UNIT_BYTES:
			word = x_image_bytes_word(w, word);
			break;

		case X_TYPE_UNIT_FOREIGN:
			word = x_image_foreign_word(w, word, p_obj);
			break;

		default:
			break;
		}

		rec[X_IMAGE_RECORD_UNITS + X_IMAGE_UNIT_WORDS * j] = kind;
		rec[X_IMAGE_RECORD_UNITS + X_IMAGE_UNIT_WORDS * j + 1] = word;
	}

	w->table_pos += x_image_record_words(n);
}

/**
 * @brief Pass 2: every flagged object's record, in index order.
 */
static void x_image_emit_pass(x_image_writer_t *w)
{
	x_obj_t *p_obj;

	for (p_obj = w->p_cursor; p_obj != NULL; p_obj = x_obj_heap(p_obj)) {
		if (x_image_imaged(w, p_obj)) {
			x_image_emit_object(w, p_obj);
		}
	}
}

/**
 * @brief Write an image's object table and blob -- docs/state-image-format.md, section 4.
 *
 * x-lang form: @code (image write! cursor flag table blob name roots result) @endcode
 *
 * Walks the allocation chain from @p cursor twice: once to index every
 * object carrying @p flag, once to write each one's record through its
 * type's save.  The loop names nothing: a reference to an object outside
 * the image and every foreign word go to @p name, applied as
 * (name word kind obj), once per distinct word, and its integer answer is
 * the external index -- nil, the sentinel.  @p result holds the table's
 * capacity in words and the blob's in bytes on the way in, and the object
 * count, table words, blob bytes and sentinel count on the way out,
 * followed by the object index of each root in @p roots, a list of the
 * objects the roots table will name -- 0 for one outside the image.
 *
 * @param p_base  Base (execution context); the callable runs here.
 * @param p_args  Unevaluated: (self cursor flag table blob name roots result).
 * @return The object count, as an integer.
 */
static x_obj_t *x_prim_image_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_cursor, *p_flag, *p_table, *p_blob, *p_name, *p_roots, *p_result;
	x_obj_t *p_root;
	x_int_t *result;
	x_int_t i;
	x_image_writer_t w;

	x_eargs(p_base, p_args, 8, NULL, &p_cursor, &p_flag, &p_table, &p_blob,
		&p_name, &p_roots, &p_result);

	result = (x_int_t *)x_ptrval(p_result);

	w.p_base = p_base;
	w.p_cursor = p_cursor;
	w.flag = x_atomint(p_flag);
	w.table = (x_int_t *)x_ptrval(p_table);
	w.table_cap = result[0];
	w.table_pos = 0;
	w.blob = (char *)x_ptrval(p_blob);
	w.blob_cap = result[1];
	w.blob_pos = 0;
	w.p_name = p_name;
	w.buf = (x_int_t *)x_sys_malloc((1 + X_IMAGE_UNIT_WORDS * X_IMAGE_SAVE_UNITS) * sizeof(x_int_t));
	w.p_buf = x_mkptr(p_base, w.buf);
	w.sent = 0;
	x_image_table_init(&w.externs, 256);

	x_image_count_pass(&w);
	x_image_emit_pass(&w);

	result[0] = w.n;
	result[1] = w.table_pos;
	result[2] = w.blob_pos;
	result[3] = w.sent;
	i = 4;

	for (p_root = p_roots; p_root != NULL; p_root = x_restobj(p_root)) {
		result[i] = (x_firstobj(p_root) == NULL)
			? 0 : x_image_table_get(&w.index, (x_int_t)x_firstobj(p_root));
		i++;
	}

	x_image_table_free(&w.index);
	x_image_table_free(&w.externs);
	x_sys_free(w.buf);

	return x_mkint(p_base, w.n);
}

/** Register the state-image primitives. */
x_obj_t *x_prim_image_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_prim_entry_t entries[] = {
		{ "image-save!",     x_prim_image_save,     "image", "save!"    },
		{ "image-rebuild!",  x_prim_image_rebuild,  "image", "rebuild!" },
		{ "image-write!",    x_prim_image_write,    "image", "write!"   }
	};

	x_prims_bind_table(p_base, entries, sizeof(entries) / sizeof(entries[0]));
	return p_base;
}
