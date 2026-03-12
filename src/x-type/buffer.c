/*
 * # Computational Expressions in C
 *
 * ## x-type/ptr.c -- Implementation - Type - Buffer
 *
 * @description Computational Expressions in C
 * @author [Jon Ruttan](jonruttan@gmail.com)
 * @copyright 2021 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 *
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
/*
 * # Includes
 */
#include "x-type/buffer.h"
#include "x-type/char.h"
#include "x-base.h"
#include "x-heap.h"

x_satom_t x_type_buffer_name = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = (x_char_t *)X_TYPE_BUFFER_NAME }),
	x_type_buffer_mark_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_buffer_mark }),
	x_type_buffer_make_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_buffer_make }),
	x_type_buffer_write_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_buffer_write }),
	x_type_buffer_struct_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_buffer_struct });

x_obj_t *x_make_buffer(x_obj_t *p_base, x_obj_flag_t flags, void *p)
{
	x_satom_t buffer = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .v = p }),
		flags_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = flags });
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { buffer }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { flags_obj }, { NULL })
	};

	return x_type_buffer_make(p_base, (x_obj_t *)args);
}

x_obj_t *x_type_buffer_mark(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_firstobj(p_args);
	x_obj_flag_t flags = x_firstint(x_firstobj(x_restobj(p_args)));

	/* Buffer data slots contain raw char pointers, not objects.
	 * Mark the inner bookkeeping pair explicitly. */
	x_heap_mark(p_base, x_restobj(p_obj), flags, NULL);

	return NULL;
}

x_obj_t *x_type_buffer_struct(x_obj_t *p_base, x_obj_t *p_args)
{
	struct x_type_t type = {
		.p_name = x_type_buffer_name,
		.p_mark = x_type_buffer_mark_prim,
		.p_make = x_type_buffer_make_prim,
		.p_write = x_type_buffer_write_prim
	};

	return x_type_struct_make(p_base, type);
}

x_obj_t *x_type_buffer_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_buffer_name }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_buffer_struct_prim }, { NULL })
	};

	return x_type_struct_get(p_base, (x_obj_t *)args);
}

x_obj_t *x_type_buffer_make(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_type = x_type_buffer_register(p_base, p_base),
		*p_buffer = x_0(p_args);
	x_obj_flag_t flags = x_obj_isnil(p_base, x_1(p_args))
		? 0 : x_firstint(x_01(p_args));

	return x_obj_make(p_base, p_type, flags, X_OBJ_LENGTH_PAIR,
			x_bufferval(p_buffer),
			x_obj_make(p_base, p_type,
				flags & ~X_OBJ_FLAG_OWN, X_OBJ_LENGTH_PAIR,
				x_bufferval(p_buffer), x_bufferval(p_buffer)));
}

x_obj_t *x_type_buffer_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_satom_t buffer = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = (x_char_t *)X_TYPE_BUFFER_WRITE_STR }),
		size = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .i = X_TYPE_BUFFER_WRITE_LEN });
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { buffer }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { size }, { NULL })
	};

	x_base_write(p_base, (x_obj_t *)args);

	return x_firstobj(p_args);
}

x_obj_t *x_type_buffer_reset(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_firstobj(p_args);

	x_bufferwrite(p_buffer) = x_bufferread(p_buffer) = x_bufferval(p_buffer);

	return p_buffer;
}

x_obj_t *x_type_buffer_retain(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_firstobj(p_args);
	x_int_t n = x_bufferwrite(p_buffer) - x_bufferread(p_buffer);

	x_lib_memcpy(x_bufferval(p_buffer), x_bufferread(p_buffer), n);
	x_bufferread(p_buffer) = x_bufferval(p_buffer);
	x_bufferwrite(p_buffer) = x_bufferread(p_buffer) + n;

	return p_buffer;
}

x_obj_t *x_type_buffer_append(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_firstobj(p_args);

	*x_bufferwrite(p_buffer) = x_charval(x_firstobj(x_restobj(p_args)));
	x_bufferwrite(p_buffer) += 1;

	return p_buffer;
}

x_obj_t *x_type_buffer_read(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_firstobj(p_args), *p_char;
	x_satom_t char_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .c = '\0' }),
		int_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = 1 });
	x_spair_t read_args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { char_obj }, { (x_obj_t *)(read_args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { int_obj }, { NULL })
	};
	x_spair_t append_args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_buffer }, { (x_obj_t *)(append_args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { char_obj }, { NULL })
	};

	if (x_bufferwrite(p_buffer) <= x_bufferread(p_buffer)) {
		/* Readonly buffers don't extend from stdin. */
		if (x_obj_flags(p_buffer) & X_OBJ_FLAG_RO) {
			return NULL;
		}

		p_char = x_base_read(p_base, (x_obj_t *)read_args);

		if (x_obj_isnil(p_base, p_char)) {
			return NULL;
		}

		x_type_buffer_append(p_base, (x_obj_t *)append_args);
	}

	x_bufferread(p_buffer) += 1;

	/* Track line numbers for error reporting. */
	if (x_bufferlastchar(p_buffer) == '\n' && x_base_isset(p_base)) {
		x_atomint(x_base_field_line(p_base)) += 1;
	}

	return p_buffer;
}

x_obj_t *x_type_buffer_read_text(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer;

	p_buffer = x_type_buffer_read(p_base, p_args);

	if (x_obj_isnil(p_base, p_buffer) || '\0' == x_bufferlastchar(p_buffer)) {
		return NULL;
	}

	return p_buffer;
}
