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
#include "sparse_int.h"

ABC_NAMESPACE_IMPL_START



/*
 *  allocate a new row vector 
 */
sm_row *
sm_row_alloc()
{
    register sm_row *prow;

#ifdef FAST_AND_LOOSE
    if (sm_row_freelist == NIL(sm_row)) {
	prow = ALLOC(sm_row, 1);
    } else {
	prow = sm_row_freelist;
	sm_row_freelist = prow->next_row;
    }
#else
    prow = ALLOC(sm_row, 1);
#endif

    prow->row_num = 0;
    prow->length = 0;
    prow->first_col = prow->last_col = NIL(sm_element);
    prow->next_row = prow->prev_row = NIL(sm_row);
    prow->flag = 0;
    prow->user_word = NIL(char);		/* for our user ... */
    return prow;
}


/*
 *  free a row vector -- for FAST_AND_LOOSE, this is real cheap for rows;
 *  however, freeing a column must still walk down the column discarding
 *  the elements one-by-one; that is the only use for the extra '-DCOLS'
 *  compile flag ...
 */
void
sm_row_free(prow)
register sm_row *prow;
{
#if defined(FAST_AND_LOOSE) && ! defined(COLS)
    if (prow->first_col != NIL(sm_element)) {
	/* Add the linked list of row items to the free list */
	prow->last_col->next_col = sm_element_freelist;
	sm_element_freelist = prow->first_col;
    }

    /* Add the row to the free list of rows */
    prow->next_row = sm_row_freelist;
    sm_row_freelist = prow;
#else
    register sm_element *p, *pnext;

    for(p = prow->first_col; p != 0; p = pnext) {
	pnext = p->next_col;
	sm_element_free(p);
    }
    FREE(prow);
#endif
}


/*
 *  duplicate an existing row
 */
sm_row *
sm_row_dup(prow)
register sm_row *prow;
{
    register sm_row *pnew;
    register sm_element *p;

    pnew = sm_row_alloc();
    for(p = prow->first_col; p != 0; p = p->next_col) {
	(void) sm_row_insert(pnew, p->col_num);
    }
    return pnew;
}


/*
 *  insert an element into a row vector 
 */
sm_element *
sm_row_insert(prow, col)
register sm_row *prow;
register int col;
{
    register sm_element *test, *element;

    /* get a new item, save its address */
    sm_element_alloc(element);
    test = element;
    sorted_insert(sm_element, prow->first_col, prow->last_col, prow->length, 
		    next_col, prev_col, col_num, col, test);

    /* if item was not used, free it */
    if (element != test) {
	sm_element_free(element);
    }

    /* either way, return the current new value */
    return test;
}


/*
 *  remove an element from a row vector 
 */
void
sm_row_remove(prow, col)
register sm_row *prow;
register int col;
{
    register sm_element *p;

    for(p = prow->first_col; p != 0 && p->col_num < col; p = p->next_col)
	;
    if (p != 0 && p->col_num == col) {
	dll_unlink(p, prow->first_col, prow->last_col, 
			    next_col, prev_col, prow->length);
	sm_element_free(p);
    }
}


/*
 *  find an element (if it is in the row vector)
 */
sm_element *
sm_row_find(prow, col)
sm_row *prow;
int col;
{
    register sm_element *p;

    for(p = prow->first_col; p != 0 && p->col_num < col; p = p->next_col)
	;
    if (p != 0 && p->col_num == col) {
	return p;
    } else {
	return NIL(sm_element);
    }
}

/*
 *  return 1 if row p2 contains row p1; 0 otherwise
 */
int 
sm_row_contains(p1, p2)
sm_row *p1, *p2;
{
    register sm_element *q1, *q2;

    q1 = p1->first_col;
    q2 = p2->first_col;
    while (q1 != 0) {
	if (q2 == 0 || q1->col_num < q2->col_num) {
	    return 0;
	} else if (q1->col_num == q2->col_num) {
	    q1 = q1->next_col;
	    q2 = q2->next_col;
	} else {
	    q2 = q2->next_col;
	}
    }
    return 1;
}


/*
 *  return 1 if row p1 and row p2 share an element in common
 */
int 
sm_row_intersects(p1, p2)
sm_row *p1, *p2;
{
    register sm_element *q1, *q2;

    q1 = p1->first_col;
    q2 = p2->first_col;
    if (q1 == 0 || q2 == 0) return 0;
    for(;;) {
	if (q1->col_num < q2->col_num) {
	    if ((q1 = q1->next_col) == 0) {
		return 0;
	    }
	} else if (q1->col_num > q2->col_num) {
	    if ((q2 = q2->next_col) == 0) {
		return 0;
	    }
	} else {
	    return 1;
	}
    }
}


/*
 *  compare two rows, lexical ordering
 */
int 
sm_row_compare(p1, p2)
sm_row *p1, *p2;
{
    register sm_element *q1, *q2;

    q1 = p1->first_col;
    q2 = p2->first_col;
    while(q1 != 0 && q2 != 0) {
	if (q1->col_num != q2->col_num) {
	    return q1->col_num - q2->col_num;
	}
	q1 = q1->next_col;
	q2 = q2->next_col;
    }

    if (q1 != 0) {
	return 1;
    } else if (q2 != 0) {
	return -1;
    } else {
	return 0;
    }
}


/*
 *  return the intersection
 */
sm_row *
sm_row_and(p1, p2)
sm_row *p1, *p2;
{
    register sm_element *q1, *q2;
    register sm_row *result;

    result = sm_row_alloc();
    q1 = p1->first_col;
    q2 = p2->first_col;
    if (q1 == 0 || q2 == 0) return result;
    for(;;) {
	if (q1->col_num < q2->col_num) {
	    if ((q1 = q1->next_col) == 0) {
		return result;
	    }
	} else if (q1->col_num > q2->col_num) {
	    if ((q2 = q2->next_col) == 0) {
		return result;
	    }
	} else {
	    (void) sm_row_insert(result, q1->col_num);
	    if ((q1 = q1->next_col) == 0) {
		return result;
	    }
	    if ((q2 = q2->next_col) == 0) {
		return result;
	    }
	}
    }
}

int 
sm_row_hash(prow, modulus)
sm_row *prow;
int modulus;
{
    register int sum;
    register sm_element *p;

    sum = 0;
    for(p = prow->first_col; p != 0; p = p->next_col) {
	sum = (sum*17 + p->col_num) % modulus;
    }
    return sum;
}

/*
 *  remove an element from a row vector (given a pointer to the element) 
 */
void
sm_row_remove_element(prow, p)
register sm_row *prow;
register sm_element *p;
{
    dll_unlink(p, prow->first_col, prow->last_col, 
			next_col, prev_col, prow->length);
    sm_element_free(p);
}


void
sm_row_print(fp, prow)
FILE *fp;
sm_row *prow;
{
    sm_element *p;

    for(p = prow->first_col; p != 0; p = p->next_col) {
	(void) fprintf(fp, " %d", p->col_num);
    }
}
ABC_NAMESPACE_IMPL_END

