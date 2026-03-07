/*
 * # Computational Expressions in C
 *
 * ## x-base.c -- Implementation - Base
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
#include "x-base.h"
#include "x-alist.h"

#include "x-sexp.h"

#define nil			p_base
#define pair(X,Y)	(x_mkspair(p_base, (X), (Y)))
#define atom(X)		(x_mksatom(p_base, (X)))

x_obj_t *x_base_make(x_obj_t *p_base, x_obj_t *p_args)
{
	p_base = atom(nil);
	x_atomobj(p_base) = pair(
		nil,
		pair(
			pair(atom(STDIN_FILENO),
			pair(atom(STDOUT_FILENO),
			pair(atom(STDERR_FILENO),
			nil))),
		pair(
			pair(pair(nil, nil),
			pair(pair(nil, nil),
			pair(nil,
			pair(nil,
			pair(nil,
			pair(nil,
			pair(nil,
			nil))))))),
		nil)));

	return p_base;
}

#undef nil
#undef pair
#undef atom

x_obj_t *x_base_type_alist_extend(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_args }, { NULL });

	if ( ! x_base_isset(p_base)) {
		return p_base;
	}

	x_restobj((x_obj_t *)args) = x_base_field_type_alist(p_base);

	return x_base_field_type_alist(p_base) = x_alist_extend(p_base, (x_obj_t *)args);
}

x_obj_t *x_base_type_alist_assoc(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_firstobj(p_args) }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL })
	};

	if ( ! x_base_isset(p_base)) {
		return p_base;
	}

	x_firstobj((x_obj_t *)args[1]) = x_base_field_type_alist(p_base);

	return x_alist_assoc(p_base, (x_obj_t *)args);
}

x_obj_t *x_base_env_alist_extend(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_args }, { NULL });

	if ( ! x_base_isset(p_base)) {
		return p_base;
	}

	x_restobj((x_obj_t *)args) = x_base_field_env_alist(p_base);

	return x_base_field_env_alist(p_base) = x_alist_extend(p_base, (x_obj_t *)args);
}

x_obj_t *x_base_env_alist_assoc(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_firstobj(p_args) }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL })
	};

	if ( ! x_base_isset(p_base)) {
		return p_base;
	}

	x_firstobj((x_obj_t *)args[1]) = x_base_field_env_alist(p_base);

	return x_alist_assoc(p_base, (x_obj_t *)args);
}

x_obj_t *x_base_read(x_obj_t *p_base, x_obj_t *p_args)
{
	int fd = x_base_isset(p_base) ? x_atomint(x_base_field_filein(p_base)) : STDIN_FILENO;
	x_obj_t *p_atom = x_firstobj(p_args);
	x_int_t size = x_atomint(x_firstobj(x_restobj(p_args)));

	if (x_sys_read(fd, &x_atomchar(p_atom), size) == size) {
		return p_atom;
	}

	return p_base;
}

x_obj_t *x_base_write(x_obj_t *p_base, x_obj_t *p_args)
{
	int fd = x_base_isset(p_base) ? x_atomint(x_base_field_fileout(p_base)) : STDOUT_FILENO;
	x_obj_t *p_atom = x_firstobj(p_args);
	x_int_t size = x_atomint(x_firstobj(x_restobj(p_args)));

	if (x_sys_write(fd, x_atomstr(p_atom), size) == size) {
		return p_atom;
	}

	return p_base;
}
