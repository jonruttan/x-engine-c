#ifndef X_OBJ_H
#define X_OBJ_H

/*
 * # Computational Expressions in C
 *
 * ## x-obj.h -- Header - Objects
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
#include "x-lib.h"

#define X_TYPE_NIL_NAME				"NIL"
#define X_TYPE_ATOM_NAME			"ATOM"
#define X_TYPE_PAIR_NAME			"PAIR"


#define X_OBJ_DUMP_BUFFER_SIZE	65536

/*
 * # Data Structures
 */
typedef enum x_obj_flag_enum
{
	X_OBJ_FLAG_NONE=0x0,
	X_OBJ_FLAG_OBJ=0x0,

	X_OBJ_FLAG_1=0x1,
	X_OBJ_FLAG_2=0x2,
	X_OBJ_FLAG_3=0x4,
	X_OBJ_FLAG_4=0x8,

	X_OBJ_FLAG_ATTR_MASK=0xF,

	X_OBJ_FLAG_PRIM=0x10,
	X_OBJ_FLAG_FN,
	X_OBJ_FLAG_INT,
	X_OBJ_FLAG_CHAR,
	X_OBJ_FLAG_STR,
	X_OBJ_FLAG_PTR,

	X_OBJ_FLAG_OWN=0x20,
	X_OBJ_FLAG_RO=0x40,

#ifndef X_GC
	X_OBJ_FLAG_MASK=0x7F
#else /* X_GC */
	X_OBJ_FLAG_GC=0x80,

	X_OBJ_FLAG_MASK=0xFF
#endif /* X_GC */
} x_obj_flag_t;

typedef union x_datum_union x_obj_t;

typedef x_obj_t * (*x_prim_fn)(x_obj_t *p_base, x_obj_t *p_args);

union x_datum_union
{
	x_obj_t *p;
	x_prim_fn fn;
	x_int_t i;
	x_char_t c;
	x_char_t *s;
	void *v;
};


#ifdef X_GC
#define X_OBJ_UNITS_GC				1
#else /* X_GC */
#define X_OBJ_UNITS_GC				0
#endif /* X_GC */

#define X_OBJ_UNITS_TYPE			1
#define X_OBJ_UNITS_FLAGS			1

enum {
#ifdef X_GC
	X_OBJ_META_GC = 0,
#endif /* X_GC */
	X_OBJ_META_TYPE = X_OBJ_UNITS_GC,
	X_OBJ_META_FLAGS = (X_OBJ_UNITS_GC + X_OBJ_UNITS_TYPE),
	X_OBJ_META_LEN = (X_OBJ_UNITS_GC + X_OBJ_UNITS_TYPE + X_OBJ_UNITS_FLAGS)
};

#define X_OBJ_UNITS_BASE			X_OBJ_META_LEN

#define X_OBJ_UNITS_ATOM			1
#define X_OBJ_UNITS_PAIR			2

#define X_OBJ_LENGTH_ATOM			X_OBJ_UNITS_ATOM
#define X_OBJ_LENGTH_PAIR			X_OBJ_UNITS_PAIR


typedef x_obj_t x_satom_t[X_OBJ_META_LEN + X_OBJ_UNITS_ATOM];

typedef x_obj_t x_spair_t[X_OBJ_META_LEN + X_OBJ_UNITS_PAIR];

#ifdef X_GC
#define x_obj_gc(X)					((X)[X_OBJ_META_GC].p)
#endif /* X_GC */
#define x_obj_type(X)				((X)[X_OBJ_META_TYPE].p)
#define x_obj_flags(X)				((X)[X_OBJ_META_FLAGS].i)
#define x_obj_data_ptr(X)			((X) + X_OBJ_META_LEN)
#define x_obj_data_i(X,I)			(x_obj_data_ptr((X))[(I)])
#define x_obj_data(X)				x_obj_data_i((X),0)

#ifdef X_GC
#define x_obj_set(T,F,...)			{ { .v = NULL }, { .p = (T) }, { .i = (F) }, __VA_ARGS__ }
#else /* X_GC */
#define x_obj_set(T,F,...)			{ { .p = (T) }, { .i = (F) }, __VA_ARGS__ }
#endif /* X_GC */


#define x_obj_type_isnil(B,X)		x_obj_isnil((B), x_obj_type(X))
#define x_obj_is_type(B,X,T)		(x_lib_strcmp(x_obj_type_name((B), (X)), (T)) == 0)

extern x_satom_t x_type_atom_obj;
extern x_satom_t x_type_pair_obj;
extern x_satom_t x_type_units_atom_obj;
extern x_satom_t x_type_units_pair_obj;

#define x_obj_type_issatom(X)		(x_obj_type((X)) == x_type_atom_obj)
#define x_obj_type_isspair(X)		(x_obj_type((X)) == x_type_pair_obj)


#define x_mkfsatom(B,F,X)			x_obj_make((B), x_type_atom_obj, (F), X_OBJ_LENGTH_ATOM, (X))
#define x_mkfsatomown(B,F,X)		x_obj_make((B), x_type_atom_obj, (F) | X_OBJ_FLAG_OWN, X_OBJ_LENGTH_ATOM, (X))
#define x_mkfspair(B,F,X,Y)			x_obj_make((B), x_type_pair_obj, (F), X_OBJ_LENGTH_PAIR, (X), (Y))

#define x_mksatom(B,X)				x_mkfsatom((B), X_OBJ_FLAG_NONE, (X))
#define x_mksatomown(B,X)			x_mkfsatomown((B), X_OBJ_FLAG_NONE, (X))
#define x_mkspair(B,X,Y)			x_mkfspair((B), X_OBJ_FLAG_NONE, (X), (Y))

#define x_obj(X)					((X).p)
#define x_fn(X)						((X).fn)
#define x_int(X)					((X).i)
#define x_char(X)					((X).c)
#define x_str(X)					((X).s)
#define x_ptr(X)					((X).v)

#define x_first(X)					x_obj_data_i((X),0)
#define x_second(X)					x_obj_data_i((X),1)
#define x_rest(X)					x_second((X))

#define x_firstptr(X)				x_ptr(x_first((X)))
#define x_firstobj(X)				x_obj(x_first((X)))
#define x_firstint(X)				x_int(x_first((X)))
#define x_firstchar(X)				x_char(x_first((X)))
#define x_firststr(X)				x_str(x_first((X)))
#define x_firstfn(X)				x_fn(x_first((X)))

#define x_secondptr(X)				x_ptr(x_rest((X)))
#define x_secondobj(X)				x_obj(x_rest((X)))
#define x_secondint(X)				x_int(x_rest((X)))
#define x_secondchar(X)				x_char(x_rest((X)))
#define x_secondstr(X)				x_str(x_rest((X)))
#define x_secondfn(X)				x_fn(x_rest((X)))

#define x_atomptr(X)				x_firstptr((X))
#define x_atomobj(X)				x_firstobj((X))
#define x_atomint(X)				x_firstint((X))
#define x_atomchar(X)				x_firstchar((X))
#define x_atomstr(X)				x_firststr((X))
#define x_atomfn(X)					x_firstfn((X))

#define x_restptr(X)				x_secondptr((X))
#define x_restobj(X)				x_secondobj((X))
#define x_restint(X)				x_secondint((X))
#define x_restchar(X)				x_secondchar((X))
#define x_reststr(X)				x_secondstr((X))
#define x_restfn(X)					x_secondfn((X))

#define x_0(X)						x_firstobj(X)
#define x_1(X)						x_restobj(X)
#define x_00(X)						x_0(x_0(X))
#define x_01(X)						x_0(x_1(X))
#define x_10(X)						x_1(x_0(X))
#define x_11(X)						x_1(x_1(X))
#define x_000(X)					x_0(x_00(X))
#define x_001(X)					x_0(x_01(X))
#define x_010(X)					x_0(x_10(X))
#define x_011(X)					x_0(x_11(X))
#define x_100(X)					x_1(x_00(X))
#define x_101(X)					x_1(x_01(X))
#define x_110(X)					x_1(x_10(X))
#define x_111(X)					x_1(x_11(X))
#define x_0000(X)					x_0(x_000(X))
#define x_0001(X)					x_0(x_001(X))
#define x_0010(X)					x_0(x_010(X))
#define x_0011(X)					x_0(x_011(X))
#define x_0100(X)					x_0(x_100(X))
#define x_0101(X)					x_0(x_101(X))
#define x_0110(X)					x_0(x_110(X))
#define x_0111(X)					x_0(x_111(X))
#define x_1000(X)					x_1(x_000(X))
#define x_1001(X)					x_1(x_001(X))
#define x_1010(X)					x_1(x_010(X))
#define x_1011(X)					x_1(x_011(X))
#define x_1100(X)					x_1(x_100(X))
#define x_1101(X)					x_1(x_101(X))
#define x_1110(X)					x_1(x_110(X))
#define x_1111(X)					x_1(x_111(X))

#include "x-lisp.h"

int x_obj_isnil(x_obj_t *p_base, x_obj_t *p_obj);

x_obj_t *x_obj_alloc(x_obj_t *p_base, x_obj_t *p_type, x_obj_flag_t flags, size_t size);
x_obj_t *x_obj_make_va(x_obj_t *p_base, x_obj_t *p_type, x_obj_flag_t flags, size_t size, va_list ap);
x_obj_t *x_obj_make(x_obj_t *p_base, x_obj_t *p_type, x_obj_flag_t flags, size_t size, ...);
void x_obj_free(x_obj_t *p_obj);

x_char_t *x_obj_type_name(x_obj_t *p_base, x_obj_t *p_obj);

x_int_t x_obj_units(x_obj_t *p_base, x_obj_t *p_obj);
x_int_t x_obj_length(x_obj_t *p_base, x_obj_t *p_obj);

x_obj_t *x_obj_call(x_obj_t *p_base, x_obj_t *p_obj);
x_obj_t *x_obj_eval(x_obj_t *p_base, x_obj_t *p_obj);
x_obj_t *x_obj_convert(x_obj_t *p_base, x_obj_t *p_obj);

x_obj_t *x_obj_read(x_obj_t *p_base, x_obj_t *p_obj);
x_obj_t *x_obj_write(x_obj_t *p_base, x_obj_t *p_obj);

void x_obj_error(x_obj_t *p_base, x_char_t *message, x_char_t *symbol);

#ifdef DEBUG
void _x_obj_debug_va(char *file, long unsigned line, x_obj_t *p_base, char *fmt, va_list ap);
#define x_obj_debug_va(p_base, fmt, ap)\
	_x_obj_debug_va(__FILE__, __LINE__, p_base, fmt, ap)

void _x_obj_debug(char *file, long unsigned line, x_obj_t *p_base, char *fmt, ...);
#define x_obj_debug(p_base, fmt, ...)\
	_x_obj_debug(__FILE__, __LINE__, p_base, fmt, __VA_ARGS__)

void _x_obj_dump(char *file, long unsigned line, x_obj_t *p_base, x_obj_t *p_obj, char *msg);
#define x_obj_dump(p_base, p_obj, msg)\
	_x_obj_dump(__FILE__, __LINE__, p_base, p_obj, msg)

#else /* DEBUG */

#define x_obj_debug_va(p_base, fmt, ap)		;
#define x_obj_debug(p_base, fmt, ...)		;
#define x_obj_dump(p_base, p_obj, msg)		;

#endif /* DEBUG */

#endif /* X_OBJ_H */
