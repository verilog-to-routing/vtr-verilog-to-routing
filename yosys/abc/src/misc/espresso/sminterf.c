/*
 * Revision Control Information
 *
 * $Source$
 * $Author$
 * $Revision$
 * $Date$
 *
 */
#include "espresso.h"

ABC_NAMESPACE_IMPL_START



pset
do_sm_minimum_cover(A)
pset_family A;
{
    sm_matrix *M;
    sm_row *sparse_cover;
    sm_element *pe;
    pset cover;
    register int i, base, rownum;
    register unsigned val;
    register pset last, p;

    M = sm_alloc();
    rownum = 0;
    foreach_set(A, last, p) {
    foreach_set_element(p, i, val, base) {
        (void) sm_insert(M, rownum, base);
    }
    rownum++;
    }

    sparse_cover = sm_minimum_cover(M, NIL(int), 1, 0);
    sm_free(M);

    cover = set_new(A->sf_size);
    sm_foreach_row_element(sparse_cover, pe) {
    set_insert(cover, pe->col_num);
    }
    sm_row_free(sparse_cover);

    return cover;
}
ABC_NAMESPACE_IMPL_END

