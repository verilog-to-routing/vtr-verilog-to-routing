#ifndef __ACE_CUBE_H__
#define __ACE_CUBE_H__

#include "ace.h"
//#include "avl.h"
//#include "espresso.h"

//////////////// Espresso Cube Functions //////////////////
#define fail(why) {\
		(void) fprintf(stderr, "Fatal error: file %s, line %d\n%s\n",\
				__FILE__, __LINE__, why);\
				(void) fflush(stdout);\
				abort();\
				}

#define LOOPINIT(size)		((size <= BPI) ? 1 : WHICH_WORD((size)-1))

#define BPI            32
#define LOGBPI          5

#define WHICH_WORD(element)     (((element) >> LOGBPI) + 1)
#define WHICH_BIT(element)      ((element) & (BPI-1))

#define GETINPUT(c, pos)\
    ((c[WHICH_WORD(2*pos)] >> WHICH_BIT(2*pos)) & 3)

#define node_get_literal(c_, j_) 					\
		((c_) ? GETINPUT((c_), (j_)) :  node_error(2))

#define TWO     3
#define DASH    3
#define ONE     2
#define ZERO    1

typedef unsigned int *pset;

#define ALLOC(type, num)	\
    ((type *) malloc(sizeof(type) * (num)))

#define set_remove(set, e)      (set[WHICH_WORD(e)] &= ~ (1 << WHICH_BIT(e)))
#define set_insert(set, e)      (set[WHICH_WORD(e)] |= 1 << WHICH_BIT(e))
#define set_new(size)			ALLOC(unsigned int, SET_SIZE(size))

/* # of ints needed to allocate a set with "size" elements */
#define SET_SIZE(size)          ((size) <= BPI ? 2 : (WHICH_WORD((size)-1) + 1))

typedef struct {
	pset cube;
	int num_literals;
	double static_prob;
} ace_cube_t;

ace_cube_t * ace_cube_new_dc(int num_literals);
ace_cube_t * ace_cube_dup(ace_cube_t * cube);
void ace_cube_free(ace_cube_t * cube);

#endif
