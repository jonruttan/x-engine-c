/*
 * # Computational Expressions in C
 *
 * ## x-eval.c -- Implementation - Evaluator
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
#include "x-eval.h"

/*
 * # Environment Managment Functions
 */

/* NOTE: Defined in header
#define x_extend(BASE, ENV, SYM, VAL) (x_cons((BASE), x_cons((BASE), (SYM), (VAL)), (ENV)))
*/

x_obj_t *x_multiple_extend(x_obj_t *p_base, x_obj_t *env, x_obj_t *syms, x_obj_t *vals)
{
	if ( ! (syms == nil || syms->type == X_PAIR)) {
		return x_extend(p_base, env, syms, vals);
	}

	return syms == nil ? env : x_multiple_extend(p_base, x_extend(p_base, env, x_car(syms),
			x_car(vals)), x_cdr(syms), x_cdr(vals));
}

x_obj_t *x_extend_top(x_obj_t *p_base, x_obj_t *sym, x_obj_t *val)
{
	x_setcdr(x_vectorval(p_base, X_I_EXPR), x_cons(p_base, x_cons(p_base, sym, val), x_cdr(x_vectorval(p_base, X_I_EXPR))));

	return val;
}

x_obj_t *x_val_sym(x_obj_t *p_base, x_obj_t *sym)
{
	return x_cdr(x_assoc(p_base, sym, x_vectorval(p_base, X_I_EXPR)));
}

x_obj_t *x_val_str(x_obj_t *p_base, x_char_t *str)
{
	return x_val_sym(p_base, x_car(x_findsym(p_base, str)));
}


x_obj_t *x_mkproccall(x_obj_t *p_base, x_char_t *proc, x_obj_t *exp)
{
	return x_cons(p_base,
		x_car(x_findsym(p_base, proc)),
		x_cons(p_base,
			x_cons(p_base, x_car(x_findsym(p_base, "quote")), x_cons(p_base, exp, nil)),
			nil));
}


x_obj_t *x_substlis(x_obj_t *p_base, x_obj_t *exps, x_obj_t *env)
{
	if (exps == x_nil) {
		return x_nil;
	}

	return x_cons(p_base, x_car(exps), x_substlis(p_base, x_cdr(exps), env));
}

x_obj_t *x_evlis(x_obj_t *p_base, x_obj_t *exps, x_obj_t *env)
{
	if (exps == x_nil) {
		return x_nil;
	}

	return x_cons(p_base, x_eval(p_base, x_car(exps), env), x_evlis(p_base, x_cdr(exps), env));
}

/**
 * ## Primitives
 */
x_obj_t *x_append(x_obj_t *p_base, x_obj_t *exp)
{
	x_obj_t *tmp, *args;

	for (args = exp; x_is_type(x_cdr(args), _X_PAIR) && x_cdr(args) != x_nil; args = x_cdr(args)) {
		tmp = x_car(args);

		while(x_is_type(x_cdr(tmp), _X_PAIR) && x_cdr(tmp) != x_nil) {
			tmp = x_cdr(tmp);
		}

		x_cdr(tmp) = x_cadr(args);
	}

	return x_car(exp);
}

/*** Evaluator (Eval/no Apply) ***/
x_obj_t *x_eval(x_obj_t *p_base, x_obj_t *exp, x_obj_t *env)
{
	x_obj_t *tmp, *itr, *proc, *vals;

eval_start:
	if (exp == x_nil) {
		return x_nil;
	}

	if (x_obj_types[exp->type].eval) {
		return x_obj_types[exp->type].eval(p_base, exp, env);
	}

	if (is_type(exp, _X_PAIR)) {
		if (x_car(exp) == x_car(x_findsym(p_base, "quote"))) {
			return x_cadr(exp);
		}

		if (x_car(exp) == x_car(x_findsym(p_base, "quasiquote"))) {
			return x_quasiquote(p_base, x_cadr(exp), env);
		}


		if (x_car(exp) == x_car(x_findsym(p_base, "cond"))) {
			while ((exp = x_cdr(exp)) != x_nil) {
				if((tmp = x_eval(p_base, x_caar(exp), env)) != x_nil) {
					exp = x_cdar(exp) != x_nil ? x_cadar(exp) : tmp;

					goto eval_start;
				}
			}

			return exp;
		}

		if (x_car(exp) == x_car(x_findsym(p_base, _X_MACRO))) {
			return x_macro(p_base, x_cadr(exp), x_cddr(exp));
		}

		if (x_car(exp) == x_car(x_findsym(p_base, "lambda"))) {
			return x_proc(p_base, x_cadr(exp), x_cddr(exp), env);
		}

		if (x_car(exp) == x_car(x_findsym(p_base, "define"))) {
/*			return x_extend_top(p_base, x_cadr(exp), x_eval(p_base, x_caddr(exp), env));*/
			x_exp = x_extend_top(p_base, x_cadr(exp), x_eval(p_base, x_caddr(exp), env));

			return exp;

		}

		if (x_car(exp) == x_car(x_findsym(p_base, "begin"))) {
			exp = x_cdr(exp);

			if (exp == x_nil) {
				return x_nil;
			}

			for (;; exp = x_cdr(exp)) {
				if (x_cdr(exp) == x_nil) {
					exp = x_car(exp);

					goto eval_start;
				}

				x_eval(p_base, x_car(exp), env);
			}
		}

		if (x_car(exp) == x_car(x_findsym(p_base, "let"))) {
			for (itr = x_cadr(exp); itr != nil; itr = x_cdr(itr))
				env = x_extend(p_base, env, x_caar(itr), x_eval(p_base, x_cadar(itr), env));

			exp = x_cons(p_base, x_car(x_findsym(p_base, "begin")), x_cddr(exp));

			goto eval_start;
		}

		if (x_car(exp) == x_car(x_findsym(p_base, "set!"))) {
			x_obj_t *pair = x_assoc(p_base, x_cadr(exp), env);
			exp = x_eval(p_base, x_caddr(exp), env);
			x_setcdr(pair, exp);

			return exp;
		}

		if (x_car(exp) == x_car(x_findsym(p_base, "eval"))) {
			exp = x_eval(p_base, x_cadr(exp), env);
#ifdef DEBUG
			x_eval(p_base, x_mkproccall(p_base, "write", exp), x_vectorval(p_base, I_EXPR));
			puts(":eval");
#endif
			goto eval_start;
		}

		proc = x_eval(p_base, x_car(exp), env);

		if (x_is_type(proc, _X_MACRO)) {
			vals = x_substlis(p_base, x_cdr(exp), env);
			exp = x_eval(p_base, x_cons(p_base, x_car(x_findsym(p_base, "begin")), x_macrocode(proc)), x_multiple_extend(p_base, env, x_macroargs(proc), vals));
#ifdef DEBUG
			x_eval(p_base, x_mkproccall(p_base, "write", exp), x_vectorval(p_base, X_I_EXPR));
			puts(":"_X_MACRO);
#endif
			goto eval_start;
		}

		vals = x_evlis(p_base, x_cdr(exp), env);

		if (x_obj_types[proc->type].proc) {
			return x_obj_types[proc->type].proc(p_base, proc, vals);
		}

		if (x_is_type(proc, _X_PROCEDURE)) {
			/* For dynamic scope, use env instead of procenv(proc) */
			env = x_multiple_extend(p_base, x_procenv(proc), x_procargs(proc), vals);
			exp = x_cons(p_base, x_car(x_findsym(p_base, "begin")), x_proccode(proc));

			goto eval_start;
		}

		x_eval(p_base, x_mkproccall(p_base, "write", exp), x_vectorval(p_base, X_I_EXPR));
		puts("");
		x_sys_error("Bad "_X_PROCEDURE" type", x_obj_types[proc->type].name);
	}

	return exp;
}

/*** Initialization ***/
x_obj_t *x_make_top()
{
	x_obj_t *p_base, *p_nil;
	const int count = 4;

	/* Make a non-gc vector object.
	 *
	 * {
	 * 	0: vector size
	 * 	1: nil
	 * 	2: expression
	 * 	3: symbols
	 * 	4: text buffer
	 * 	5: cache
	 * }
	 */
	p_base = x_obj_alloc(X_VECTOR, 0, count + 1);
	p_nil = x_mksym(p_base, "#f");
/*	x_lib_memcpy(p_base->data.p, (x_obj_t*[]){
		x_mkint(p_base, count),
		p_nil,
		x_cons(p_base, x_cons(p_base, p_nil, p_nil), p_nil),
		x_cons(p_base, p_nil, p_nil),
		x_mkstr(p_base, X_TOKEN_SIZE_MAX),
		NULL
	}, sizeof(x_obj_t*[count + 1]));
*/	p_base->data.p[0] = x_mkint(p_base, count);
	p_base->data.p[X_I_NIL+1] = p_nil;
	p_base->data.p[X_I_EXPR+1] = x_cons(p_base, x_cons(p_base, p_nil, p_nil), p_nil);
	p_base->data.p[X_I_SYMBOLS+1] = x_cons(p_base, p_nil, p_nil);
	p_base->data.p[X_I_BUFFER+1] = x_ownstr(p_base, NULL, X_TOKEN_SIZE_MAX);
/*	p_base->data.p[X_I_CACHE+1] = p_nil;
*/
/*	tmp = x_intern(p_base, "#t");
	x_extend_top(p_base, tmp, tmp);
*/
	return p_base;
}

x_obj_t *x_init(int ifp, int ofp)
{
	x_obj_t *p_base, *tmp;
	int i;

/*
	p_base = x_obj_alloc(X_PAIR, 0, 2);
	x_vectorval(p_base, X_I_EXPR) = x_cons(p_base, x_cons(p_base, nil, nil), nil);
	x_vectorval(p_base, X_I_SYMBOLS) = x_cons(p_base, nil, nil);
*/
	p_base = x_make_top();
	nil = x_vectorval(p_base, I_NIL);
	buf = x_strval(x_vectorval(p_base, I_BUFFER));

	tmp = x_intern(p_base, "#t");
	x_extend_top(p_base, tmp, tmp);

	x_intern(p_base, "quote");
	x_intern(p_base, "cond");
	x_intern(p_base, "lambda");
	x_intern(p_base, "define");
	x_intern(p_base, _X_MACRO);
	x_intern(p_base, "begin");
	x_intern(p_base, "let");
	x_intern(p_base, "set!");
	x_intern(p_base, "eval");

	tmp = x_mkvector(p_base, X_OBJ_T_ENUM_LEN);
	for(i=0; i < X_OBJ_T_ENUM_LEN; i++)
		x_vectorval(tmp, i) = x_intern(p_base, x_obj_types[i].name);

	x_extend_top(p_base, x_intern(p_base, "types"), tmp);
	x_extend_top(p_base, x_intern(p_base, "not"), x_mkprim(p_base, x_prim_not));
	x_extend_top(p_base, x_intern(p_base, "eq?"), x_mkprim(p_base, x_prim_eq));
	x_extend_top(p_base, x_intern(p_base, "="), x_mkprim(p_base, x_prim_num_eq));
	x_extend_top(p_base, x_intern(p_base, ">"), x_mkprim(p_base, x_prim_num_gt));
	x_extend_top(p_base, x_intern(p_base, "+"), x_mkprim(p_base, x_prim_num_sum));
	x_extend_top(p_base, x_intern(p_base, "-"), x_mkprim(p_base, x_prim_num_sub));
	x_extend_top(p_base, x_intern(p_base, "*"), x_mkprim(p_base, x_prim_num_prod));
	x_extend_top(p_base, x_intern(p_base, "/"), x_mkprim(p_base, x_prim_num_div));
	x_extend_top(p_base, x_intern(p_base, "%"), x_mkprim(p_base, x_prim_num_mod));
	x_extend_top(p_base, x_intern(p_base, "~"), x_mkprim(p_base, x_prim_num_not));
	x_extend_top(p_base, x_intern(p_base, "&"), x_mkprim(p_base, x_prim_num_and));
	x_extend_top(p_base, x_intern(p_base, "|"), x_mkprim(p_base, x_prim_num_or));
	x_extend_top(p_base, x_intern(p_base, "^"), x_mkprim(p_base, x_prim_num_xor));
	x_extend_top(p_base, x_intern(p_base, "<<"), x_mkprim(p_base, x_prim_num_shl));
	x_extend_top(p_base, x_intern(p_base, ">>"), x_mkprim(p_base, x_prim_num_shr));
	x_extend_top(p_base, x_intern(p_base, "cons"), x_mkprim(p_base, x_prim_cons));
	x_extend_top(p_base, x_intern(p_base, "car"), x_mkprim(p_base, x_prim_car));
	x_extend_top(p_base, x_intern(p_base, "cdr"), x_mkprim(p_base, x_prim_cdr));
	x_extend_top(p_base, x_intern(p_base, "findsym"), x_mkprim(p_base, x_prim_findsym));
	x_extend_top(p_base, x_intern(p_base, "assoc"), x_mkprim(p_base, x_prim_assoc));
	x_extend_top(p_base, x_intern(p_base, "append"), x_mkprim(p_base, x_prim_append));
	x_extend_top(p_base, x_intern(p_base, "read-char"), x_mkprim(p_base, x_prim_read_char));
	x_extend_top(p_base, x_intern(p_base, "write"), x_mkprim(p_base, x_prim_write));
	x_extend_top(p_base, x_intern(p_base, "length"), x_mkprim(p_base, x_prim_length));
	x_extend_top(p_base, x_intern(p_base, "gc"), x_mkprim(p_base, x_prim_gc));
	x_extend_top(p_base, x_intern(p_base, "make-string"), x_mkprim(p_base, x_prim_mkstr));
	x_extend_top(p_base, x_intern(p_base, "string"), x_mkprim(p_base, x_prim_str));
	x_extend_top(p_base, x_intern(p_base, "string-compare"), x_mkprim(p_base, x_prim_strcmp));
	x_extend_top(p_base, x_intern(p_base, _VECTOR), x_mkprim(p_base, x_prim_vector));
	x_extend_top(p_base, x_intern(p_base, "p_base"), x_mkprim(p_base, x_prim_top));
	x_extend_top(p_base, x_intern(p_base, "ptr"), x_mkprim(p_base, x_prim_ptr));
	x_extend_top(p_base, x_intern(p_base, "type"), x_mkprim(p_base, x_prim_type));
	x_extend_top(p_base, x_intern(p_base, "->type"), x_mkprim(p_base, x_prim_totype));
	x_extend_top(p_base, x_intern(p_base, "x-version"), x_mkstr(p_base, X_VERSION));
	x_extend_top(p_base, x_intern(p_base, "x-machine"), x_mkstr(p_base, X_MACHINE));
	x_extend_top(p_base, x_intern(p_base, "x-arch"), x_mkstr(p_base, X_ARCH));

	x_gc_mark(p_base, RO);

	return p_base;
}

void x_fini(x_obj_t *p_base)
{
	x_gc_sweep(p_base, 0);
	x_sys_debug("<FINI>");
}
