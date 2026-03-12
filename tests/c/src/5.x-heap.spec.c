/*
 * # Unit Tests: *x-heap*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"

#ifndef X_HEAP
#define X_HEAP
#endif /* X_HEAP */

#include "ext/x-expr/src/x-sys.c"
#include "ext/x-expr/src/x-lib.c"
#include "src/x-obj.c"
#include "ext/x-expr/src/x.c"
#include "ext/x-expr/src/x-heap.c"

#include "helper-system-functions.c"


/*
 * ## Test Overhead
 */

static void _setup(void)
{
	helper_set_alloc(MEM_GUARANTEED);
}

static void _teardown(void)
{
}

static char *test_gc_mark(void)
{
	x_obj_t *p_obj[3], *p_ret;

	helper_alloc_reset();


	/* Atoms */
	p_obj[0] = x_mkfsatom(NULL, 3, 0);
	_it_should("set the new object's flags to 3", 3 == x_obj_flags(p_obj[0]));

	p_ret = x_heap_mark(NULL, p_obj[0], 3);
	_it_should("return the object", p_ret == p_obj[0]);
	_it_should("mark the object with the flag provided", 3 == x_obj_flags(p_obj[0]));

	x_sys_free(p_obj[0]);

return NULL;
	/* Pairs */
/*

	p_obj[0] = x_mksatom(NULL, 0);
	_it_should("set the pair' first object's flags to 0", x_obj_flags(p_obj[0]) == 0);
	p_obj[1] = x_mksatom(NULL, 1);
	_it_should("set the pair' rest object's flags to 0", x_obj_flags(p_obj[1]) == 0);

	p_obj[2] = x_obj_make(NULL, x_type_pair_obj, 0, 2, p_obj[0], p_obj[1]);
	_it_should("set the pair object's flags to 0", x_obj_flags(p_obj[2]) == 0);

	p_ret = x_heap_mark(NULL, p_obj[2], 1);
	_it_should("return the pair object", p_obj[2] == p_ret);
	_it_should("mark the pair object with the flag provided", 1 == x_obj_flags(p_obj[2]));
	_it_should("mark the pair's first object with the flag provided", 1 == x_obj_flags(p_obj[0]));
	_it_should("mark the pair's rest object with the flag provided", 1 == x_obj_flags(p_obj[1]));

	x_sys_free(p_obj[2]);
	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);
*/

	/* Pairs (Recursive) */
/*
	p_obj[0] = x_mkspair(NULL, NULL, NULL);
	_it_should("set the pair' first object's flags to 0", x_obj_flags(p_obj[0]) == 0);
	p_obj[1] = x_mkspair(NULL, p_obj[0], NULL);
	_it_should("set the pair' rest object's flags to 0", x_obj_flags(p_obj[1]) == 0);

	p_obj[2] = x_obj_make(NULL, x_type_pair_obj, 0, 2, p_obj[0], p_obj[1]);
	_it_should("set the pair object's flags to 0", x_obj_flags(p_obj[2]) == 0);

	x_firstobj(p_obj[0]) = p_obj[2];
	x_restobj(p_obj[0]) = p_obj[2];
	x_restobj(p_obj[1]) = p_obj[2];


	p_ret = x_heap_mark(NULL, p_obj[2], 1);
	_it_should("return the pair object", p_obj[2] == p_ret);
	_it_should("mark the pair object with the flag provided", 1 == x_obj_flags(p_obj[2]));
	_it_should("mark the pair's first object with the flag provided", 1 == x_obj_flags(p_obj[0]));
	_it_should("mark the pair's rest object with the flag provided", 1 == x_obj_flags(p_obj[1]));

	x_sys_free(p_obj[2]);
	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);
*/

	/* Symbols */
/*	p_base = x_obj_alloc(NULL, X_TYPE_SYMBOL, 0, 1);
	_it_should("set the symbol object's flags to 0", x_obj_flags(p_base) == 0);
	p_obj[0] = x_atomobj(p_base) = x_mksatom(p_base, "sym");
	_it_should("set the symbol's res object's flags to 0", x_obj_flags(p_obj[0]) == 0);

	p_ret = x_heap_mark(p_base, p_base, 1);
	_it_should("return the symbol object", p_ret == p_base);
	_it_should("mark the symbol object with the flag provided", x_obj_flags(p_base) == 1);
	_it_should("mark the symbol's res object with the flag provided", x_obj_flags(p_obj[0]) == 1);

	x_sys_free(p_obj[0]);
	x_sys_free(p_base);
*/

	/* Strings */
/*	p_base = x_obj_alloc(NULL, X_TYPE_STRING, 0, 1);
	_it_should("set the string object's flags to 0", x_obj_flags(p_base) == 0);
	p_obj[0] = x_resval(p_base) = mkres(p_base, "string");
	_it_should("set the string's res object's flags to 0", x_obj_flags(p_obj[0]) == 0);

	p_ret = x_heap_mark(p_base, p_base, 1);
	_it_should("return the string object", p_ret == p_base);
	_it_should("mark the string object with the flag provided", x_obj_flags(p_base) == 1);
	_it_should("mark the string's res object with the flag provided", x_obj_flags(p_obj[0]) == 1);

	x_sys_free(p_obj[0]);
	x_sys_free(p_base);
*/

	/* Macros */
/*	p_base = x_obj_alloc(NULL, X_TYPE_MACRO, 0, 2);
	_it_should("set the macro object's flags to 0", x_obj_flags(p_base) == 0);
	p_obj[0] = x_macroargs(p_base) = x_mkint(p_base, 0);
	_it_should("set the macro's args object's flags to 0", x_obj_flags(p_obj[0]) == 0);
	p_obj[1] = x_macrocode(p_base) = x_mkint(p_base, 1);
	_it_should("set the macro's code object's flags to 0", x_obj_flags(p_obj[1]) == 0);

	p_ret = x_heap_mark(p_base, p_base, 1);
	_it_should("return the object", p_ret == p_base);
	_it_should("mark the macro object with the flag provided", x_obj_flags(p_base) == 1);
	_it_should("mark the macro's args object with the flag provided", x_obj_flags(p_obj[0]) == 1);
	_it_should("mark the macro's code object with the flag provided", x_obj_flags(p_obj[1]) == 1);

	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);
	x_sys_free(p_base);
*/

	/* Procs */
/*	p_base = x_obj_alloc(NULL, X_TYPE_PROCEDURE, 0, 3);
	_it_should("set the proc object's flags to 0", x_obj_flags(p_base) == 0);
	p_obj[0] = x_procargs(p_base) = x_mkint(p_base, 0);
	_it_should("set the proc's args object's flags to 0", x_obj_flags(p_obj[0]) == 0);
	p_obj[1] = x_proccode(p_base) = x_mkint(p_base, 1);
	_it_should("set the proc's code object's flags to 0", x_obj_flags(p_obj[1]) == 0);
	p_obj[2] = x_procenv(p_base) = x_mkint(p_base, 2);
	_it_should("set the proc's env object's flags to 0", x_obj_flags(p_obj[2]) == 0);

	p_ret = x_heap_mark(p_base, p_base, 1);
	_it_should("return the object", p_ret == p_base);
	_it_should("mark the proc object with the flag provided", x_obj_flags(p_base) == 1);
	_it_should("mark the proc's args object with the flag provided", x_obj_flags(p_obj[0]) == 1);
	_it_should("mark the proc's code object with the flag provided", x_obj_flags(p_obj[1]) == 1);
	_it_should("mark the proc's env object with the flag provided", x_obj_flags(p_obj[2]) == 1);

	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);
	x_sys_free(p_obj[2]);
	x_sys_free(p_base);
*/

	/* Vectors */
/*	p_base = x_obj_alloc(NULL, X_TYPE_VECTOR, 0, 3);
	_it_should("set the vector object's flags to 0", x_obj_flags(p_base) == 0);
	p_obj[0] = x_vectorlen(p_base) = x_mkint(p_base, 2);
	_it_should("set the vector's 0th object's flags to 0", x_obj_flags(p_obj[0]) == 0);
	p_obj[1] = x_vectorval(p_base, 0) = x_mkint(p_base, 0);
	_it_should("set the vector's 1st object's flags to 0", x_obj_flags(p_obj[1]) == 0);
	p_obj[2] = x_vectorval(p_base, 1) = x_mkint(p_base, 1);
	_it_should("set the vector's 2nd object's flags to 0", x_obj_flags(p_obj[2]) == 0);

	p_ret = x_heap_mark(p_base, p_base, 1);
	_it_should("return the object", p_ret == p_base);
	_it_should("mark the vector object with the flag provided", x_obj_flags(p_base) == 1);
	_it_should("mark the vector's length object with the flag provided", x_obj_flags(p_obj[0]) == 1);
	_it_should("mark the vector's pos 0 object with the flag provided", x_obj_flags(p_obj[1]) == 1);
	_it_should("mark the vector's pos 1 object with the flag provided", x_obj_flags(p_obj[2]) == 1);

	x_sys_free(p_obj[2]);
	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);
	x_sys_free(p_base);
*/
	return NULL;
}

static char *test_gc_sweep(void)
{
	x_obj_t *p_base, *p_obj[4], *p_ret;
	x_char_t *s;
	int n;


	/* # Single object */
	helper_alloc_reset();

	/* Create the base object */
	n = helper_alloc_count();
	p_base = x_obj_alloc(NULL, NULL, X_OBJ_FLAG_NONE, 0);
	_it_should("allocate memory for the base object", 1 == helper_alloc_count() - n);
	_it_should("set the base object's flags to 0", 0 == x_obj_flags(p_base));
	_it_should("set the base object's gc pointer to NULL", NULL == x_obj_heap(p_base));

	/* Create an object */
	n = helper_alloc_count();
	p_obj[0] = x_obj_alloc(p_base, NULL, X_OBJ_FLAG_RO|X_OBJ_FLAG_HEAP, 0);
	_it_should("allocate memory for the object", 1 == helper_alloc_count() - n);
	_it_should("set the object's flags to GC,RO", (X_OBJ_FLAG_RO|X_OBJ_FLAG_HEAP) == x_obj_flags(p_obj[0]));
	_it_should("set the object's gc pointer to NULL", NULL == x_obj_heap(p_obj[0]));
	_it_should("set the base object's gc pointer to the object", p_obj[0] == x_obj_heap(p_base));

	/* Sweep the RO flag from the object */
	n = helper_free_count();
	p_ret = x_heap_sweep(p_base, p_obj[0], X_OBJ_FLAG_RO);
	_it_should("not have freed the object's memory", 0 == helper_free_count() - n);
	_it_should("return the base object", p_base == p_ret);
	_it_should("cleared RO bit in the object's flags", X_OBJ_FLAG_HEAP == x_obj_flags(p_obj[0]));

	/* Garbage collect everything */
	n = helper_free_count();
	p_ret = x_heap_sweep(p_base, p_base, X_OBJ_FLAG_NONE);
	_it_should("free all of the objects", 2 == helper_free_count() - n);
	_it_should("returned the base object", p_base == p_ret);
	_it_should("have freed all allocated memory", helper_free_count() == helper_alloc_count());


	/* Add an orphaned object */

	/* Create the base object */
	helper_alloc_reset();
	n = helper_alloc_count();
	p_base = x_obj_alloc(NULL, NULL, X_OBJ_FLAG_NONE, 0);
	_it_should("allocate memory for the base object", 1 == helper_alloc_count() - n);
	_it_should("set the base object's flags to 0", 0 == x_obj_flags(p_base));
	_it_should("set the base base object's gc pointer to NULL", NULL == x_obj_heap(p_base));

	/* Create an object */
	n = helper_alloc_count();
	p_obj[0] = x_obj_alloc(p_base, NULL, X_OBJ_FLAG_HEAP, 0);
	_it_should("allocate memory for the object", 1 == helper_alloc_count() - n);
	_it_should("set the object's flags to GC", X_OBJ_FLAG_HEAP == x_obj_flags(p_obj[0]));
	_it_should("set the object's gc pointer to NULL", NULL == x_obj_heap(p_obj[0]));
	_it_should("set the base object's gc pointer to the object", p_obj[0] == x_obj_heap(p_base));

	n = helper_alloc_count();
	p_obj[1] = x_obj_alloc(p_base, NULL, X_OBJ_FLAG_NONE, 0);
	_it_should("allocate memory for the orphaned object", 1 == helper_alloc_count() - n);
	_it_should("set the orphaned object's flags to 0", 0 == x_obj_flags(p_obj[1]));
	_it_should("set the orphaned object's gc pointer to the first object", p_obj[0] == x_obj_heap(p_obj[1]));
	_it_should("set the base object's gc pointer to the orphaned object", p_obj[1] == x_obj_heap(p_base));

	n = helper_free_count();
	p_ret = x_heap_sweep(p_base, x_obj_heap(p_base), X_OBJ_FLAG_HEAP);
	_it_should("have freed the orphaned object's memory", 1 == helper_free_count() - n);
	_it_should("return the base object", p_base == p_ret);
	_it_should("set the base object's gc pointer to the first object", p_obj[0] == x_obj_heap(p_base));

	n = helper_free_count();
	p_ret = x_heap_sweep(p_base, p_base, X_OBJ_FLAG_NONE);
	_it_should("have freed the object's memory", 2 == helper_free_count() - n);
	_it_should("return the base object", p_base == p_ret);
	_it_should("have freed all allocated memory", helper_free_count() == helper_alloc_count());



	/* # Atom */
	helper_alloc_reset();

	/* Create the base object */
	n = helper_alloc_count();
	p_base = x_obj_alloc(NULL, NULL, X_OBJ_FLAG_NONE, 0);
	_it_should("allocate memory for the base object", 1 == helper_alloc_count() - n);
	_it_should("set the base object's flags to 0", 0 == x_obj_flags(p_base));
	_it_should("set the base object's gc pointer to NULL", NULL == x_obj_heap(p_base));

	/* Create an Atom object. */
	n = helper_alloc_count();
	p_obj[0] = x_mkfsatom(p_base, X_OBJ_FLAG_RO|X_OBJ_FLAG_HEAP, 0);
	_it_should("allocate memory for the object", 1 == helper_alloc_count() - n);
	_it_should("set the object's flags to GC,RO", (X_OBJ_FLAG_RO|X_OBJ_FLAG_HEAP) == x_obj_flags(p_obj[0]));
	_it_should("set the object's gc pointer to NULL", NULL == x_obj_heap(p_obj[0]));
	_it_should("set the base object's gc pointer to the object", p_obj[0] == x_obj_heap(p_base));

	/* Sweep the RO flag from the object */
	n = helper_free_count();
	p_ret = x_heap_sweep(p_base, p_obj[0], X_OBJ_FLAG_RO);
	_it_should("not have freed the object's memory", 0 == helper_free_count() - n);
	_it_should("return the base object", p_base == p_ret);
	_it_should("cleared RO bit in the object's flags", X_OBJ_FLAG_HEAP == x_obj_flags(p_obj[0]));

	/* Garbage collect everything */
	n = helper_free_count();
	p_ret = x_heap_sweep(p_base, p_base, X_OBJ_FLAG_NONE);
	_it_should("free the object's memory", 2 == helper_free_count() - n);
	_it_should("return the base object", p_base == p_ret);
	_it_should("have freed all allocated memory", helper_free_count() == helper_alloc_count());


	/* Add an orphaned object */
	helper_alloc_reset();

	/* Create the base object */
	n = helper_alloc_count();
	p_base = x_obj_alloc(NULL, NULL, X_OBJ_FLAG_NONE, 0);
	_it_should("allocate memory for the base object", 1 == helper_alloc_count() - n);
	_it_should("set the base object's flags to 0", 0 == x_obj_flags(p_base));
	_it_should("set the base object's gc pointer to NULL", NULL == x_obj_heap(p_base));

	/* Create an atom object */
	n = helper_alloc_count();
	p_obj[0] = x_mkfsatom(p_base, X_OBJ_FLAG_HEAP, 0);
	_it_should("allocate memory for the object", 1 == helper_alloc_count() - n);
	_it_should("set the object's flags to GC", X_OBJ_FLAG_HEAP == x_obj_flags(p_obj[0]));
	_it_should("set the object's gc pointer to NULL", NULL == x_obj_heap(p_obj[0]));
	_it_should("set the base object's gc pointer to the object", p_obj[0] == x_obj_heap(p_base));

	/* Create another atom object */
	n = helper_alloc_count();
	p_obj[1] = x_mksatom(p_base, 0);
	_it_should("allocate memory for the orphaned object", 1 == helper_alloc_count() - n);
	_it_should("set the orphaned object's flags to 0", 0 == x_obj_flags(p_obj[1]));
	_it_should("set the orphaned object's gc pointer to the previous object", p_obj[0] == x_obj_heap(p_obj[1]));
	_it_should("set the base object's gc pointer to the object", p_obj[1] == x_obj_heap(p_base));

	n = helper_free_count();
	p_ret = x_heap_sweep(p_base, x_obj_heap(p_base), X_OBJ_FLAG_HEAP);
	_it_should("have freed the orphaned object's memory", 1 == helper_free_count() - n);
	_it_should("return the base object", p_base == p_ret);
	_it_should("set the base object's gc pointer to the first object", p_obj[0] == x_obj_heap(p_base));

	n = helper_free_count();
	p_ret = x_heap_sweep(p_base, p_base, X_OBJ_FLAG_NONE);
	_it_should("have freed the object's memory", 2 == helper_free_count() - n);
	_it_should("return the base object", p_base == p_ret);
	_it_should("have freed all allocated memory", helper_alloc_count() == helper_free_count());


	/* Owner Atom */
	helper_alloc_reset();

	/* Create the base object */
	n = helper_alloc_count();
	p_base = x_obj_alloc(NULL, NULL, X_OBJ_FLAG_NONE, 0);
	_it_should("allocate memory for the base object", 1 == helper_alloc_count() - n);
	_it_should("set the base object's flags to 0", 0 == x_obj_flags(p_base));
	_it_should("set the base object's gc pointer to NULL", NULL == x_obj_heap(p_base));

	/* Create an Owner Atom object. */
	n = helper_alloc_count();
	s = (x_char_t *)"ATOMOWN";
	p_obj[0] = x_mkfsatomown(p_base, X_OBJ_FLAG_RO|X_OBJ_FLAG_HEAP, (x_obj_t *)x_lib_memdup(s, strlen((char *)s) + 1));
	_it_should("allocate memory for the object", 2 == helper_alloc_count() - n);
	_it_should("set the object's flags to OWN,RO,GC", (X_OBJ_FLAG_OWN|X_OBJ_FLAG_RO|X_OBJ_FLAG_HEAP) == x_obj_flags(p_obj[0]));
	_it_should("set the object's gc pointer to NULL", NULL == x_obj_heap(p_obj[0]));
	_it_should("set the base object's gc pointer to the object", p_obj[0] == x_obj_heap(p_base));

	/* Sweep the RO flag from the object */
	n = helper_free_count();
	p_ret = x_heap_sweep(p_base, p_obj[0], X_OBJ_FLAG_RO);
	_it_should("not have freed the object's memory and the owned resource", 0 == helper_free_count() - n);
	_it_should("return the base object",  p_base == p_ret);
	_it_should("cleared RO bit in the object's flags", (X_OBJ_FLAG_OWN|X_OBJ_FLAG_HEAP) == x_obj_flags(p_obj[0]));

	/* Garbage collect everything */
	n = helper_free_count();
	p_ret = x_heap_sweep(p_base, p_base, X_OBJ_FLAG_NONE);
	_it_should("free the object's memory and the owned resource", 3 == helper_free_count() - n);
	_it_should("return the base object", p_base == p_ret);
	_it_should("have freed all allocated memory", helper_free_count() == helper_alloc_count());


	/* Add an orphaned object */
	helper_alloc_reset();

	/* Create the base object */
	n = helper_alloc_count();
	p_base = x_obj_alloc(NULL, NULL, X_OBJ_FLAG_NONE, 0);
	_it_should("allocate memory for the base object", 1 == helper_alloc_count() - n);
	_it_should("set the base object's flags to 0", 0 == x_obj_flags(p_base));
	_it_should("set the base object's gc pointer to NULL", NULL == x_obj_heap(p_base));

	/* Create an Owner Atom object. */
	n = helper_alloc_count();
	s = (x_char_t *)"ATOMOWN1";
	p_obj[0] = x_mkfsatomown(p_base, X_OBJ_FLAG_HEAP, (x_obj_t *)x_lib_memdup(s, strlen((char *)s) + 1));
	_it_should("allocate memory for the object and its resource", 2 == helper_alloc_count() - n);
	_it_should("set the object's flags to OWN,GC", (X_OBJ_FLAG_OWN|X_OBJ_FLAG_HEAP) == x_obj_flags(p_obj[0]));
	_it_should("set the object's gc pointer to NULL", NULL == x_obj_heap(p_obj[0]));
	_it_should("set the base object's gc pointer to the object", p_obj[0] == x_obj_heap(p_base));

	/* Create an Owner Atom object. */
	n = helper_alloc_count();
	s = (x_char_t *)"ATOMOWN2";
	p_obj[1] = x_mkfsatomown(p_base, X_OBJ_FLAG_NONE, (x_obj_t *)x_lib_memdup(s, strlen((char *)s) + 1));
	_it_should("allocate memory for the orphaned object and its resource", 2 == helper_alloc_count() - n);
	_it_should("set the orphaned object's flags to OWN", X_OBJ_FLAG_OWN == x_obj_flags(p_obj[1]));
	_it_should("set the orphaned object's gc pointer to the previous object", p_obj[0] == x_obj_heap(p_obj[1]));
	_it_should("set the base object's gc pointer to the object", p_obj[1] == x_obj_heap(p_base));

	n = helper_free_count();
	p_ret = x_heap_sweep(p_base, x_obj_heap(p_base), X_OBJ_FLAG_HEAP);
	_it_should("have freed the orphaned object's memory and the owned resource", 2 ==helper_free_count() - n);
	_it_should("return the base object",  p_base == p_ret);
	_it_should("set the base object's gc pointer to the first object", p_obj[0] == x_obj_heap(p_base));

	n = helper_free_count();
	p_ret = x_heap_sweep(p_base, p_base, X_OBJ_FLAG_NONE);
	_it_should("have freed the object's memory and the owned resource", 3 == helper_free_count() - n);
	_it_should("return the object", p_base == p_ret);
	_it_should("have freed all allocated memory", helper_alloc_count() == helper_free_count());


	/* Pair */
	helper_alloc_reset();

	/* Create the Base object */
	n = helper_alloc_count();
	p_base = x_obj_alloc(NULL, NULL, X_OBJ_FLAG_NONE, 0);
	_it_should("allocate memory for the base object", 1 == helper_alloc_count() - n);
	_it_should("set the base object's flags to 0", 0 == x_obj_flags(p_base));
	_it_should("set the base object's gc pointer to NULL", NULL == x_obj_heap(p_base));

	/* Create an Atom object */
	n = helper_alloc_count();
	p_obj[0] = x_mkfsatom(p_base, X_OBJ_FLAG_RO|X_OBJ_FLAG_HEAP, "ATOM");
	_it_should("allocate memory for the atom object", 1 == helper_alloc_count() - n);
	_it_should("set the object's flags to RO,GC", (X_OBJ_FLAG_RO|X_OBJ_FLAG_HEAP) == x_obj_flags(p_obj[0]));
	_it_should("set the object's gc pointer to NULL", NULL == x_obj_heap(p_obj[0]));
	_it_should("set the base's gc pointer to the object", p_obj[0] == x_obj_heap(p_base));

	/* Create an Owner Atom object */
	n = helper_alloc_count();
	s = (x_char_t *)"ATOMOWN";
	p_obj[1] = x_mkfsatomown(p_base, X_OBJ_FLAG_RO|X_OBJ_FLAG_HEAP, x_lib_memdup(s, strlen((char *)s) + 1));
	_it_should("allocate memory for the atomown object", 2 == helper_alloc_count() - n);
	_it_should("set the object's flags to OWN,RO,GC", (X_OBJ_FLAG_OWN|X_OBJ_FLAG_RO|X_OBJ_FLAG_HEAP) == x_obj_flags(p_obj[1]));
	_it_should("set the object's gc pointer to the previous object", p_obj[0] == x_obj_heap(p_obj[1]));
	_it_should("set the base's gc pointer to the object", p_obj[1] == x_obj_heap(p_base));

	/* Create a Pair containing the two objects */
	n = helper_alloc_count();
	p_obj[2] = x_mkfspair(p_base, X_OBJ_FLAG_RO|X_OBJ_FLAG_HEAP, p_obj[0], p_obj[1]);
	_it_should("allocate memory for the pair object", 1 == helper_alloc_count() -n);
	_it_should("set the object's flags to GC,RO", (X_OBJ_FLAG_RO|X_OBJ_FLAG_HEAP) == x_obj_flags(p_obj[2]));
	_it_should("set the object's gc pointer to the previous object", p_obj[1] == x_obj_heap(p_obj[2]));
	_it_should("set the base's gc pointer to the object", p_obj[2] == x_obj_heap(p_base));

	n = helper_free_count();
	p_ret = x_heap_sweep(p_base, p_obj[2], X_OBJ_FLAG_RO);
	_it_should("not have freed the object's memory", 0 == helper_free_count() - n);
	_it_should("return the base object", p_base == p_ret);
	_it_should("cleared RO bit in the object's flags", X_OBJ_FLAG_HEAP == x_obj_flags(p_obj[2]));

	/* Add an orphaned object */
	n = helper_alloc_count();
	p_obj[3] = x_mksatom(p_base, 0);
	_it_should("allocate memory for the orphaned object", 1 == helper_alloc_count() - n);
	_it_should("set the orphaned object's flags to 0", 0 == x_obj_flags(p_obj[3]));
	_it_should("set the orphaned object's gc pointer to the pair object", p_obj[2] == x_obj_heap(p_obj[3]));
	_it_should("set the object's gc pointer to the orphaned object", p_obj[3] == x_obj_heap(p_base));

	n = helper_free_count();
	p_ret = x_heap_sweep(p_base, x_obj_heap(p_base), X_OBJ_FLAG_HEAP);
	_it_should("have freed the orphaned object's memory", 1 == helper_free_count() - n);
	_it_should("return the base object", p_base == p_ret);
	_it_should("set the object's gc pointer to the pair object", p_obj[2] == x_obj_heap(p_base));

	n = helper_free_count();
	p_ret = x_heap_sweep(p_base, p_base, X_OBJ_FLAG_NONE);
	_it_should("have freed the object's memory", 5 == helper_free_count() - n);
	_it_should("return the object", p_base == p_ret);
	_it_should("have freed all allocated memory", helper_free_count() == helper_alloc_count());

	return NULL;
}

static char *run_tests() {
	_run_test(test_gc_mark);
	_xrun_test(test_gc_sweep);

	return NULL;
}
