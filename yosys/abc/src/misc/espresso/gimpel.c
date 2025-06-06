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



/*
 *  check for:
 *
 *        c1    c2    rest
 *        --      --      ---
 *         1     1    0 0 0 0        <-- primary row
 *         1     0    S1        <-- secondary row
 *         0       1    T1
 *         0       1    T2
 *         0       1    Tn
 *         0       0      R
 */

int
gimpel_reduce(A, select, weight, lb, bound, depth, stats, best)
sm_matrix *A;
solution_t *select;
int *weight;
int lb;
int bound;
int depth;
stats_t *stats;
solution_t **best;
{
    register sm_row *prow, *save_sec;
    register sm_col *c1 = NULL, *c2 = NULL; // Suppress "might be used uninitialized"
    register sm_element *p, *p1;
    int c1_col_num, c2_col_num;
    int primary_row_num = -1, secondary_row_num = -1; // Suppress "might be used uninitialized"
    int reduce_it; 

    reduce_it = 0;
    for(prow = A->first_row; prow != 0; prow = prow->next_row) {
    if (prow->length == 2) {
        c1 = sm_get_col(A, prow->first_col->col_num);
        c2 = sm_get_col(A, prow->last_col->col_num);
        if (c1->length == 2) {
        reduce_it = 1;
        } else if (c2->length == 2) {
        c1 = sm_get_col(A, prow->last_col->col_num);
        c2 = sm_get_col(A, prow->first_col->col_num);
        reduce_it = 1;
        }
        if (reduce_it) {
        primary_row_num = prow->row_num;
        secondary_row_num = c1->first_row->row_num;
        if (secondary_row_num == primary_row_num) {
            secondary_row_num = c1->last_row->row_num;
        }
        break;
        }
    }
    }

    if (reduce_it) {
    c1_col_num = c1->col_num;
    c2_col_num = c2->col_num;
    save_sec = sm_row_dup(sm_get_row(A, secondary_row_num));
    sm_row_remove(save_sec, c1_col_num);

    for(p = c2->first_row; p != 0; p = p->next_row) {
        if (p->row_num != primary_row_num) {
        /* merge rows S1 and T */
        for(p1 = save_sec->first_col; p1 != 0; p1 = p1->next_col) {
            (void) sm_insert(A, p->row_num, p1->col_num);
        }
        }
    }

    sm_delcol(A, c1_col_num);
    sm_delcol(A, c2_col_num);
    sm_delrow(A, primary_row_num);
    sm_delrow(A, secondary_row_num);

    stats->gimpel_count++;
    stats->gimpel++;
    *best = sm_mincov(A, select, weight, lb-1, bound-1, depth, stats);
    stats->gimpel--;

    if (*best != NIL(solution_t)) {
        /* is secondary row covered ? */
        if (sm_row_intersects(save_sec, (*best)->row)) {
        /* yes, actually select c2 */
        solution_add(*best, weight, c2_col_num);
        } else {
        solution_add(*best, weight, c1_col_num);
        }
    }

    sm_row_free(save_sec);
    return 1;
    } else {
    return 0;
    }
}
ABC_NAMESPACE_IMPL_END

