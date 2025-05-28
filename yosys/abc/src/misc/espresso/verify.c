/*
 * Revision Control Information
 *
 * $Source$
 * $Author$
 * $Revision$
 * $Date$
 *
 */
/*
 */

#include "espresso.h"

ABC_NAMESPACE_IMPL_START


/*
 *  verify -- check that all minterms of F are contained in (Fold u Dold)
 *  and that all minterms of Fold are contained in (F u Dold).
 */
bool verify(F, Fold, Dold)
pcover F, Fold, Dold;
{
    pcube p, last, *FD;
    bool verify_error = FALSE;

    /* Make sure the function didn't grow too large */
    FD = cube2list(Fold, Dold);
    foreach_set(F, last, p)
    if (! cube_is_covered(FD, p)) {
        printf("some minterm in F is not covered by Fold u Dold\n");
        verify_error = TRUE;
        if (verbose_debug) printf("%s\n", pc1(p)); else break;
    }
    free_cubelist(FD);

    /* Make sure minimized function covers the original function */
    FD = cube2list(F, Dold);
    foreach_set(Fold, last, p)
    if (! cube_is_covered(FD, p)) {
        printf("some minterm in Fold is not covered by F u Dold\n");
        verify_error = TRUE;
        if (verbose_debug) printf("%s\n", pc1(p)); else break;
    }
    free_cubelist(FD);

    return verify_error;
}



/*
 *  PLA_verify -- verify that two PLA's are identical
 *
 *  If names are given, row and column permutations are done to make
 *  the comparison meaningful.
 *
 */
bool PLA_verify(PLA1, PLA2)
pPLA PLA1, PLA2;
{
    /* Check if both have names given; if so, attempt to permute to
     * match the names
     */
    if (PLA1->label != NULL && PLA1->label[0] != NULL &&
       PLA2->label != NULL && PLA2->label[0] != NULL) {
    PLA_permute(PLA1, PLA2);
    } else {
    (void) fprintf(stderr, "Warning: cannot permute columns without names\n");
    return TRUE;
    }

    if (PLA1->F->sf_size != PLA2->F->sf_size) {
    (void) fprintf(stderr, "PLA_verify: PLA's are not the same size\n");
    return TRUE;
    }

    return verify(PLA2->F, PLA1->F, PLA1->D);
}



/*
 *  Permute the columns of PLA1 so that they match the order of PLA2
 *  Discard any columns of PLA1 which are not in PLA2
 *  Association is strictly by the names of the columns of the cover.
 */
void PLA_permute(PLA1, PLA2)
pPLA PLA1, PLA2;
{
    register int i, j, *permute, npermute;
    register char *labi;
    char **label;

    /* determine which columns of PLA1 to save, and place these in the list
     * "permute"; the order in this list is the final output order
     */
    npermute = 0;
    permute = ALLOC(int, PLA2->F->sf_size);
    for(i = 0; i < PLA2->F->sf_size; i++) {
    labi = PLA2->label[i];
    for(j = 0; j < PLA1->F->sf_size; j++) {
        if (strcmp(labi, PLA1->label[j]) == 0) {
        permute[npermute++] = j;
        break;
        }
    }
    }

    /* permute columns */
    if (PLA1->F != NULL) {
    PLA1->F = sf_permute(PLA1->F, permute, npermute);
    }
    if (PLA1->R != NULL) {
    PLA1->R = sf_permute(PLA1->R, permute, npermute);
    }
    if (PLA1->D != NULL) {
    PLA1->D = sf_permute(PLA1->D, permute, npermute);
    }

    /* permute the labels */
    label = ALLOC(char *, cube.size);
    for(i = 0; i < npermute; i++) {
    label[i] = PLA1->label[permute[i]];
    }
    for(i = npermute; i < cube.size; i++) {
    label[i] = NULL;
    }
    FREE(PLA1->label);
    PLA1->label = label;

    FREE(permute);
}



/*
 *  check_consistency -- test that the ON-set, OFF-set and DC-set form
 *  a partition of the boolean space.
 */
bool check_consistency(PLA)
pPLA PLA;
{
    bool verify_error = FALSE;
    pcover T;

    T = cv_intersect(PLA->F, PLA->D);
    if (T->count == 0)
    printf("ON-SET and DC-SET are disjoint\n");
    else {
    printf("Some minterm(s) belong to both the ON-SET and DC-SET !\n");
    if (verbose_debug)
        cprint(T);
    verify_error = TRUE;
    }
    (void) fflush(stdout);
    free_cover(T);

    T = cv_intersect(PLA->F, PLA->R);
    if (T->count == 0)
    printf("ON-SET and OFF-SET are disjoint\n");
    else {
    printf("Some minterm(s) belong to both the ON-SET and OFF-SET !\n");
    if (verbose_debug)
        cprint(T);
    verify_error = TRUE;
    }
    (void) fflush(stdout);
    free_cover(T);

    T = cv_intersect(PLA->D, PLA->R);
    if (T->count == 0)
    printf("DC-SET and OFF-SET are disjoint\n");
    else {
    printf("Some minterm(s) belong to both the OFF-SET and DC-SET !\n");
    if (verbose_debug)
        cprint(T);
    verify_error = TRUE;
    }
    (void) fflush(stdout);
    free_cover(T);

    if (tautology(cube3list(PLA->F, PLA->D, PLA->R)))
    printf("Union of ON-SET, OFF-SET and DC-SET is the universe\n");
    else {
    T = complement(cube3list(PLA->F, PLA->D, PLA->R));
    printf("There are minterms left unspecified !\n");
    if (verbose_debug)
        cprint(T);
    verify_error = TRUE;
    free_cover(T);
    }
    (void) fflush(stdout);
    return verify_error;
}
ABC_NAMESPACE_IMPL_END

