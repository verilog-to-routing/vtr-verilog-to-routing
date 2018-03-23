/*
 * Revision Control Information
 *
 * $Source$
 * $Author$
 * $Revision$
 * $Date$
 *
 */
//#include "port.h"
//#include "utility.h"
#include "sparse.h"

#include "util_hack.h" // added



/*
 *  sorted, double-linked list insertion
 *
 *  type: object type
 *
 *  first, last: fields (in header) to head and tail of the list
 *  count: field (in header) of length of the list
 *
 *  next, prev: fields (in object) to link next and previous objects
 *  value: field (in object) which controls the order
 *
 *  newval: value field for new object
 *  e: an object to use if insertion needed (set to actual value used)
 */

#define ABC__misc__espresso__sparse_int_h
    if (last == 0) { \
	e->value = newval; \
	first = e; \
	last = e; \
	e->next = 0; \
	e->prev = 0; \
	count++; \
    } else if (last->value < newval) { \
	e->value = newval; \
	last->next = e; \
	e->prev = last; \
	last = e; \
	e->next = 0; \
	count++; \
    } else if (first->value > newval) { \
	e->value = newval; \
	first->prev = e; \
	e->next = first; \
	first = e; \
	e->prev = 0; \
	count++; \
    } else { \
	type *p; \
	for(p = first; p->value < newval; p = p->next) \
	    ; \
	if (p->value > newval) { \
	    e->value = newval; \
	    p = p->prev; \
	    p->next->prev = e; \
	    e->next = p->next; \
	    p->next = e; \
	    e->prev = p; \
	    count++; \
	} else { \
	    e = p; \
	} \
    }


/*
 *  double linked-list deletion
 */
#define dll_unlink(p, first, last, next, prev, count) { \
    if (p->prev == 0) { \
	first = p->next; \
    } else { \
	p->prev->next = p->next; \
    } \
    if (p->next == 0) { \
	last = p->prev; \
    } else { \
	p->next->prev = p->prev; \
    } \
    count--; \
}


#ifdef FAST_AND_LOOSE
extern sm_element *sm_element_freelist;
extern sm_row *sm_row_freelist;
extern sm_col *sm_col_freelist;

#define sm_element_alloc(newobj) \
    if (sm_element_freelist == NIL(sm_element)) { \
	newobj = ALLOC(sm_element, 1); \
    } else { \
	newobj = sm_element_freelist; \
	sm_element_freelist = sm_element_freelist->next_col; \
    } \
    newobj->user_word = NIL(char); \

#define sm_element_free(e) \
    (e->next_col = sm_element_freelist, sm_element_freelist = e)

#else

#define sm_element_alloc(newobj)	\
    newobj = ALLOC(sm_element, 1);	\
    newobj->user_word = NIL(char);
#define sm_element_free(e)		\
    FREE(e)
#endif


extern void sm_row_remove_element();
extern void sm_col_remove_element();

/* LINTLIBRARY */
