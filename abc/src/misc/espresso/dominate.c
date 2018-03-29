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



int 
sm_row_dominance(A)
sm_matrix *A; 
{
    register sm_row *prow, *prow1;
    register sm_col *pcol, *least_col;
    register sm_element *p, *pnext;
    int rowcnt;

    rowcnt = A->nrows;

    /* Check each row against all other rows */
    for(prow = A->first_row; prow != 0; prow = prow->next_row) {

    /* Among all columns with a 1 in this row, choose smallest */
    least_col = sm_get_col(A, prow->first_col->col_num);
    for(p = prow->first_col->next_col; p != 0; p = p->next_col) {
        pcol = sm_get_col(A, p->col_num);
        if (pcol->length < least_col->length) {
        least_col = pcol;
        }
    }

    /* Only check for containment against rows in this column */
    for(p = least_col->first_row; p != 0; p = pnext) {
        pnext = p->next_row;

        prow1 = sm_get_row(A, p->row_num);
        if ((prow1->length > prow->length) ||
                  (prow1->length == prow->length && 
                  prow1->row_num > prow->row_num)) {
        if (sm_row_contains(prow, prow1)) {
            sm_delrow(A, prow1->row_num);
        }
        }
    }
    }

    return rowcnt - A->nrows;
}

int 
sm_col_dominance(A, weight)
sm_matrix *A;
int *weight;
{
    register sm_row *prow;
    register sm_col *pcol, *pcol1;
    register sm_element *p;
    sm_row *least_row;
    sm_col *next_col;
    int colcnt;

    colcnt = A->ncols;

    /* Check each column against all other columns */
    for(pcol = A->first_col; pcol != 0; pcol = next_col) {
    next_col = pcol->next_col;

    /* Check all rows to find the one with fewest elements */
    least_row = sm_get_row(A, pcol->first_row->row_num);
    for(p = pcol->first_row->next_row; p != 0; p = p->next_row) {
        prow = sm_get_row(A, p->row_num);
        if (prow->length < least_row->length) {
        least_row = prow;
        }
    }

    /* Only check for containment against columns in this row */
    for(p = least_row->first_col; p != 0; p = p->next_col) {
        pcol1 = sm_get_col(A, p->col_num);
        if (weight != 0 && weight[pcol1->col_num] > weight[pcol->col_num])
        continue;
        if ((pcol1->length > pcol->length) ||
           (pcol1->length == pcol->length && 
                   pcol1->col_num > pcol->col_num)) {
        if (sm_col_contains(pcol, pcol1)) {
            sm_delcol(A, pcol->col_num);
            break;
        }
        }
    }
    }

    return colcnt - A->ncols;
}
ABC_NAMESPACE_IMPL_END

