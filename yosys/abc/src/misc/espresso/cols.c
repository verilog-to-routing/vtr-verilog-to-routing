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
 *  allocate a new col vector 
 */
sm_col *
sm_col_alloc()
{
    register sm_col *pcol;

#ifdef FAST_AND_LOOSE
    if (sm_col_freelist == NIL(sm_col)) {
    pcol = ALLOC(sm_col, 1);
    } else {
    pcol = sm_col_freelist;
    sm_col_freelist = pcol->next_col;
    }
#else
    pcol = ALLOC(sm_col, 1);
#endif

    pcol->col_num = 0;
    pcol->length = 0;
    pcol->first_row = pcol->last_row = NIL(sm_element);
    pcol->next_col = pcol->prev_col = NIL(sm_col);
    pcol->flag = 0;
    pcol->user_word = NIL(char);        /* for our user ... */
    return pcol;
}


/*
 *  free a col vector -- for FAST_AND_LOOSE, this is real cheap for cols;
 *  however, freeing a rowumn must still walk down the rowumn discarding
 *  the elements one-by-one; that is the only use for the extra '-DCOLS'
 *  compile flag ...
 */
void
sm_col_free(pcol)
register sm_col *pcol;
{
#if defined(FAST_AND_LOOSE) && ! defined(COLS)
    if (pcol->first_row != NIL(sm_element)) {
    /* Add the linked list of col items to the free list */
    pcol->last_row->next_row = sm_element_freelist;
    sm_element_freelist = pcol->first_row;
    }

    /* Add the col to the free list of cols */
    pcol->next_col = sm_col_freelist;
    sm_col_freelist = pcol;
#else
    register sm_element *p, *pnext;

    for(p = pcol->first_row; p != 0; p = pnext) {
    pnext = p->next_row;
    sm_element_free(p);
    }
    FREE(pcol);
#endif
}


/*
 *  duplicate an existing col
 */
sm_col *
sm_col_dup(pcol)
register sm_col *pcol;
{
    register sm_col *pnew;
    register sm_element *p;

    pnew = sm_col_alloc();
    for(p = pcol->first_row; p != 0; p = p->next_row) {
    (void) sm_col_insert(pnew, p->row_num);
    }
    return pnew;
}


/*
 *  insert an element into a col vector 
 */
sm_element *
sm_col_insert(pcol, row)
register sm_col *pcol;
register int row;
{
    register sm_element *test, *element;

    /* get a new item, save its address */
    sm_element_alloc(element);
    test = element;
    sorted_insert(sm_element, pcol->first_row, pcol->last_row, pcol->length, 
            next_row, prev_row, row_num, row, test);

    /* if item was not used, free it */
    if (element != test) {
    sm_element_free(element);
    }

    /* either way, return the current new value */
    return test;
}


/*
 *  remove an element from a col vector 
 */
void
sm_col_remove(pcol, row)
register sm_col *pcol;
register int row;
{
    register sm_element *p;

    for(p = pcol->first_row; p != 0 && p->row_num < row; p = p->next_row)
    ;
    if (p != 0 && p->row_num == row) {
    dll_unlink(p, pcol->first_row, pcol->last_row, 
                next_row, prev_row, pcol->length);
    sm_element_free(p);
    }
}


/*
 *  find an element (if it is in the col vector)
 */
sm_element *
sm_col_find(pcol, row)
sm_col *pcol;
int row;
{
    register sm_element *p;

    for(p = pcol->first_row; p != 0 && p->row_num < row; p = p->next_row)
    ;
    if (p != 0 && p->row_num == row) {
    return p;
    } else {
    return NIL(sm_element);
    }
}

/*
 *  return 1 if col p2 contains col p1; 0 otherwise
 */
int 
sm_col_contains(p1, p2)
sm_col *p1, *p2;
{
    register sm_element *q1, *q2;

    q1 = p1->first_row;
    q2 = p2->first_row;
    while (q1 != 0) {
    if (q2 == 0 || q1->row_num < q2->row_num) {
        return 0;
    } else if (q1->row_num == q2->row_num) {
        q1 = q1->next_row;
        q2 = q2->next_row;
    } else {
        q2 = q2->next_row;
    }
    }
    return 1;
}


/*
 *  return 1 if col p1 and col p2 share an element in common
 */
int 
sm_col_intersects(p1, p2)
sm_col *p1, *p2;
{
    register sm_element *q1, *q2;

    q1 = p1->first_row;
    q2 = p2->first_row;
    if (q1 == 0 || q2 == 0) return 0;
    for(;;) {
    if (q1->row_num < q2->row_num) {
        if ((q1 = q1->next_row) == 0) {
        return 0;
        }
    } else if (q1->row_num > q2->row_num) {
        if ((q2 = q2->next_row) == 0) {
        return 0;
        }
    } else {
        return 1;
    }
    }
}


/*
 *  compare two cols, lexical ordering
 */
int 
sm_col_compare(p1, p2)
sm_col *p1, *p2;
{
    register sm_element *q1, *q2;

    q1 = p1->first_row;
    q2 = p2->first_row;
    while(q1 != 0 && q2 != 0) {
    if (q1->row_num != q2->row_num) {
        return q1->row_num - q2->row_num;
    }
    q1 = q1->next_row;
    q2 = q2->next_row;
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
sm_col *
sm_col_and(p1, p2)
sm_col *p1, *p2;
{
    register sm_element *q1, *q2;
    register sm_col *result;

    result = sm_col_alloc();
    q1 = p1->first_row;
    q2 = p2->first_row;
    if (q1 == 0 || q2 == 0) return result;
    for(;;) {
    if (q1->row_num < q2->row_num) {
        if ((q1 = q1->next_row) == 0) {
        return result;
        }
    } else if (q1->row_num > q2->row_num) {
        if ((q2 = q2->next_row) == 0) {
        return result;
        }
    } else {
        (void) sm_col_insert(result, q1->row_num);
        if ((q1 = q1->next_row) == 0) {
        return result;
        }
        if ((q2 = q2->next_row) == 0) {
        return result;
        }
    }
    }
}

int 
sm_col_hash(pcol, modulus)
sm_col *pcol;
int modulus;
{
    register int sum;
    register sm_element *p;

    sum = 0;
    for(p = pcol->first_row; p != 0; p = p->next_row) {
    sum = (sum*17 + p->row_num) % modulus;
    }
    return sum;
}

/*
 *  remove an element from a col vector (given a pointer to the element) 
 */
void
sm_col_remove_element(pcol, p)
register sm_col *pcol;
register sm_element *p;
{
    dll_unlink(p, pcol->first_row, pcol->last_row, 
            next_row, prev_row, pcol->length);
    sm_element_free(p);
}


void
sm_col_print(fp, pcol)
FILE *fp;
sm_col *pcol;
{
    sm_element *p;

    for(p = pcol->first_row; p != 0; p = p->next_row) {
    (void) fprintf(fp, " %d", p->row_num);
    }
}
ABC_NAMESPACE_IMPL_END

