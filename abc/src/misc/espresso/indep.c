/*
 * Revision Control Information
 *
 * $Source$
 * $Author$
 * $Revision$
 * $Date$
 *
 */
#include "mincov_int.h"

ABC_NAMESPACE_IMPL_START


static sm_matrix *build_intersection_matrix();


#if 0
/*
 *  verify that all rows in 'indep' are actually independent !
 */
static int 
verify_indep_set(A, indep)
sm_matrix *A;
sm_row *indep;
{
    register sm_row *prow, *prow1;
    register sm_element *p, *p1;

    for(p = indep->first_col; p != 0; p = p->next_col) {
	prow = sm_get_row(A, p->col_num);
	for(p1 = p->next_col; p1 != 0; p1 = p1->next_col) {
	    prow1 = sm_get_row(A, p1->col_num);
	    if (sm_row_intersects(prow, prow1)) {
		return 0;
	    }
	}
    }
    return 1;
}
#endif

solution_t * 
sm_maximal_independent_set(A, weight)
sm_matrix *A;
int *weight;
{
    register sm_row *best_row, *prow;
    register sm_element *p;
    int least_weight;
    sm_row *save;
    sm_matrix *B;
    solution_t *indep;

    indep = solution_alloc();
    B = build_intersection_matrix(A);

    while (B->nrows > 0) {
	/*  Find the row which is disjoint from a maximum number of rows */
	best_row = B->first_row;
	for(prow = B->first_row->next_row; prow != 0; prow = prow->next_row) {
	    if (prow->length < best_row->length) {
		best_row = prow;
	    }
	}

	/* Find which element in this row has least weight */
	if (weight == NIL(int)) {
	    least_weight = 1;
	} else {
	    prow = sm_get_row(A, best_row->row_num);
	    least_weight = weight[prow->first_col->col_num];
	    for(p = prow->first_col->next_col; p != 0; p = p->next_col) {
		if (weight[p->col_num] < least_weight) {
		    least_weight = weight[p->col_num];
		}
	    }
	}
	indep->cost += least_weight;
	(void) sm_row_insert(indep->row, best_row->row_num);

	/*  Discard the rows which intersect this row */
	save = sm_row_dup(best_row);
	for(p = save->first_col; p != 0; p = p->next_col) {
	    sm_delrow(B, p->col_num);
	    sm_delcol(B, p->col_num);
	}
	sm_row_free(save);
    }

    sm_free(B);

/*
    if (! verify_indep_set(A, indep->row)) {
	fail("sm_maximal_independent_set: row set is not independent");
    }
*/
    return indep;
}

static sm_matrix *
build_intersection_matrix(A)
sm_matrix *A;
{
    register sm_row *prow, *prow1;
    register sm_element *p, *p1;
    register sm_col *pcol;
    sm_matrix *B;

    /* Build row-intersection matrix */
    B = sm_alloc();
    for(prow = A->first_row; prow != 0; prow = prow->next_row) {

	/* Clear flags on all rows we can reach from row 'prow' */
	for(p = prow->first_col; p != 0; p = p->next_col) {
	    pcol = sm_get_col(A, p->col_num);
	    for(p1 = pcol->first_row; p1 != 0; p1 = p1->next_row) {
		prow1 = sm_get_row(A, p1->row_num);
		prow1->flag = 0;
	    }
	}

	/* Now record which rows can be reached */
	for(p = prow->first_col; p != 0; p = p->next_col) {
	    pcol = sm_get_col(A, p->col_num);
	    for(p1 = pcol->first_row; p1 != 0; p1 = p1->next_row) {
		prow1 = sm_get_row(A, p1->row_num);
		if (! prow1->flag) {
		    prow1->flag = 1;
		    (void) sm_insert(B, prow->row_num, prow1->row_num);
		}
	    }
	}
    }

    return B;
}
ABC_NAMESPACE_IMPL_END

