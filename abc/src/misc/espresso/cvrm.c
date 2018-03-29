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
    module: cvrm.c
    Purpose: miscellaneous cover manipulation
    a) verify two covers are equal, check consistency of a cover
    b) unravel a multiple-valued cover into minterms
    c) sort covers
*/

#include "espresso.h"

ABC_NAMESPACE_IMPL_START



static void cb_unravel(c, start, end, startbase, B1)
IN register pcube c;
IN int start, end;
IN pcube startbase;
INOUT pcover B1;
{
    pcube base = cube.temp[0], p, last;
    int expansion, place, skip, var, size, offset;
    register int i, j, k, n;

    /* Determine how many cubes it will blow up into, and create a mask
    for those parts that have only a single coordinate
    */
    expansion = 1;
    (void) set_copy(base, startbase);
    for(var = start; var <= end; var++) {
    if ((size = set_dist(c, cube.var_mask[var])) < 2) {
        (void) set_or(base, base, cube.var_mask[var]);
    } else {
        expansion *= size;
    }
    }
    (void) set_and(base, c, base);

    /* Add the unravelled sets starting at the last element of B1 */
    offset = B1->count;
    B1->count += expansion;
    foreach_remaining_set(B1, last, GETSET(B1, offset-1), p) {
    INLINEset_copy(p, base);
    }

    place = expansion;
    for(var = start; var <= end; var++) {
    if ((size = set_dist(c, cube.var_mask[var])) > 1) {
        skip = place;
        place = place / size;
        n = 0;
        for(i = cube.first_part[var]; i <= cube.last_part[var]; i++) {
        if (is_in_set(c, i)) {
            for(j = n; j < expansion; j += skip) {
            for(k = 0; k < place; k++) {
                p = GETSET(B1, j+k+offset);
                (void) set_insert(p, i);
            }
            }
            n += place;
        }
        }
    }
    }
}


pcover unravel_range(B, start, end)
IN pcover B;
IN int start, end;
{
    pcover B1;
    int var, total_size, expansion, size;
    register pcube p, last, startbase = cube.temp[1];

    /* Create the starting base for those variables not being unravelled */
    (void) set_copy(startbase, cube.emptyset);
    for(var = 0; var < start; var++)
    (void) set_or(startbase, startbase, cube.var_mask[var]);
    for(var = end+1; var < cube.num_vars; var++)
    (void) set_or(startbase, startbase, cube.var_mask[var]);

    /* Determine how many cubes it will blow up into */
    total_size = 0;
    foreach_set(B, last, p) {
    expansion = 1;
    for(var = start; var <= end; var++)
        if ((size = set_dist(p, cube.var_mask[var])) >= 2)
        if ((expansion *= size) > 1000000)
            fatal("unreasonable expansion in unravel");
    total_size += expansion;
    }

    /* We can now allocate a cover of exactly the correct size */
    B1 = new_cover(total_size);
    foreach_set(B, last, p) {
    cb_unravel(p, start, end, startbase, B1);
    }
    free_cover(B);
    return B1;
}


pcover unravel(B, start)
IN pcover B;
IN int start;
{
    return unravel_range(B, start, cube.num_vars-1);
}

/* lex_sort -- sort cubes in a standard lexical fashion */
pcover lex_sort(T)
pcover T;
{
    pcover T1 = sf_unlist(sf_sort(T, lex_order), T->count, T->sf_size);
    free_cover(T);
    return T1;
}


/* size_sort -- sort cubes by their size */
pcover size_sort(T)
pcover T;
{
    pcover T1 = sf_unlist(sf_sort(T, descend), T->count, T->sf_size);
    free_cover(T);
    return T1;
}


/*  mini_sort -- sort cubes according to the heuristics of mini */
pcover mini_sort(F, compare)
pcover F;
int (*compare)();
{
    register int *count, cnt, n = cube.size, i;
    register pcube p, last;
    pcover F_sorted;
    pcube *F1;

    /* Perform a column sum over the set family */
    count = sf_count(F);

    /* weight is "inner product of the cube and the column sums" */
    foreach_set(F, last, p) {
    cnt = 0;
    for(i = 0; i < n; i++)
        if (is_in_set(p, i))
        cnt += count[i];
    PUTSIZE(p, cnt);
    }
    FREE(count);

    /* use qsort to sort the array */
    qsort((char *) (F1 = sf_list(F)), F->count, sizeof(pcube), compare);
    F_sorted = sf_unlist(F1, F->count, F->sf_size);
    free_cover(F);

    return F_sorted;
}


/* sort_reduce -- Espresso strategy for ordering the cubes before reduction */
pcover sort_reduce(T)
IN pcover T;
{
    register pcube p, last, largest = NULL;
    register int bestsize = -1, size, n = cube.num_vars;
    pcover T_sorted;
    pcube *T1;

    if (T->count == 0)
    return T;

    /* find largest cube */
    foreach_set(T, last, p)
    if ((size = set_ord(p)) > bestsize)
        largest = p, bestsize = size;

    foreach_set(T, last, p)
    PUTSIZE(p, ((n - cdist(largest,p)) << 7) + MIN(set_ord(p),127));

    qsort((char *) (T1 = sf_list(T)), T->count, sizeof(pcube), (int (*)()) descend);
    T_sorted = sf_unlist(T1, T->count, T->sf_size);
    free_cover(T);

    return T_sorted;
}

pcover random_order(F)
register pcover F;
{
    pset temp;
    register int i, k;
#ifdef RANDOM
    long random();
#endif

    temp = set_new(F->sf_size);
    for(i = F->count - 1; i > 0; i--) {
    /* Choose a random number between 0 and i */
#ifdef RANDOM
    k = random() % i;
#else
    /* this is not meant to be really used; just provides an easy
       "out" if random() and srandom() aren't around
    */
    k = (i*23 + 997) % i;
#endif
    /* swap sets i and k */
    (void) set_copy(temp, GETSET(F, k));
    (void) set_copy(GETSET(F, k), GETSET(F, i));
    (void) set_copy(GETSET(F, i), temp);
    }
    set_free(temp);
    return F;
}

/*
 *  cubelist_partition -- take a cubelist T and see if it has any components;
 *  if so, return cubelist's of the two partitions A and B; the return value
 *  is the size of the partition; if not, A and B
 *  are undefined and the return value is 0
 */
int cubelist_partition(T, A, B, comp_debug)
pcube *T;            /* a list of cubes */
pcube **A, **B;            /* cubelist of partition and remainder */
unsigned int comp_debug;
{
    register pcube *T1, p, seed, cof;
    pcube *A1, *B1;
    bool change;
    int count, numcube;

    numcube = CUBELISTSIZE(T);

    /* Mark all cubes -- covered cubes belong to the partition */
    for(T1 = T+2; (p = *T1++) != NULL; ) {
    RESET(p, COVERED);
    }

    /*
     *  Extract a partition from the cubelist T; start with the first cube as a
     *  seed, and then pull in all cubes which share a variable with the seed;
     *  iterate until no new cubes are brought into the partition.
     */
    seed = set_save(T[2]);
    cof = T[0];
    SET(T[2], COVERED);
    count = 1;

    do {
    change = FALSE;
    for(T1 = T+2; (p = *T1++) != NULL; ) {
        if (! TESTP(p, COVERED) && ccommon(p, seed, cof)) {
        INLINEset_and(seed, seed, p);
        SET(p, COVERED);
        change = TRUE;
        count++;
        }
    
    }
    } while (change);

    set_free(seed);

    if (comp_debug) {
    (void) printf("COMPONENT_REDUCTION: split into %d %d\n",
        count, numcube - count);
    }

    if (count != numcube) {
    /* Allocate and setup the cubelist's for the two partitions */
    *A = A1 = ALLOC(pcube, numcube+3);
    *B = B1 = ALLOC(pcube, numcube+3);
    (*A)[0] = set_save(T[0]);
    (*B)[0] = set_save(T[0]);
    A1 = *A + 2;
    B1 = *B + 2;

    /* Loop over the cubes in T and distribute to A and B */
    for(T1 = T+2; (p = *T1++) != NULL; ) {
        if (TESTP(p, COVERED)) {
        *A1++ = p;
        } else {
        *B1++ = p;
        }
    }

    /* Stuff needed at the end of the cubelist's */
    *A1++ = NULL;
    (*A)[1] = (pcube) A1;
    *B1++ = NULL;
    (*B)[1] = (pcube) B1;
    }

    return numcube - count;
}

/*
 *  quick cofactor against a single output function
 */
pcover cof_output(T, i)
pcover T;
register int i;
{
    pcover T1;
    register pcube p, last, pdest, mask;

    mask = cube.var_mask[cube.output];
    T1 = new_cover(T->count);
    foreach_set(T, last, p) {
    if (is_in_set(p, i)) {
        pdest = GETSET(T1, T1->count++);
        INLINEset_or(pdest, p, mask);
        RESET(pdest, PRIME);
    }
    }
    return T1;
}


/*
 *  quick intersection against a single output function
 */
pcover uncof_output(T, i)
pcover T;
int i;
{
    register pcube p, last, mask;

    if (T == NULL) {
    return T;
    }

    mask = cube.var_mask[cube.output];
    foreach_set(T, last, p) {
    INLINEset_diff(p, p, mask);
    set_insert(p, i);
    }
    return T;
}


/*
 *  A generic routine to perform an operation for each output function
 *
 *  func() is called with a PLA for each output function (with the output
 *  part effectively removed).
 *  func1() is called after reforming the equivalent output function
 *
 *  Each function returns TRUE if process is to continue
 */
void foreach_output_function(PLA, func, func1)
pPLA PLA;
int (*func)();
int (*func1)();
{
    pPLA PLA1;
    int i;

    /* Loop for each output function */
    for(i = 0; i < cube.part_size[cube.output]; i++) {

    /* cofactor on the output part */
    PLA1 = new_PLA();
    PLA1->F = cof_output(PLA->F, i + cube.first_part[cube.output]);
    PLA1->R = cof_output(PLA->R, i + cube.first_part[cube.output]);
    PLA1->D = cof_output(PLA->D, i + cube.first_part[cube.output]);

    /* Call a routine to do something with the cover */
    if ((*func)(PLA1, i) == 0) {
        free_PLA(PLA1);
        return;
    }

    /* intersect with the particular output part again */
    PLA1->F = uncof_output(PLA1->F, i + cube.first_part[cube.output]);
    PLA1->R = uncof_output(PLA1->R, i + cube.first_part[cube.output]);
    PLA1->D = uncof_output(PLA1->D, i + cube.first_part[cube.output]);

    /* Call a routine to do something with the final result */
    if ((*func1)(PLA1, i) == 0) {
        free_PLA(PLA1);
        return;
    }

    /* Cleanup for next go-around */
    free_PLA(PLA1);
    

    }
}

static pcover Fmin;
static pcube phase;

/*
 *  minimize each output function individually
 */
void so_espresso(PLA, strategy)
pPLA PLA;
int strategy;
{
    Fmin = new_cover(PLA->F->count);
    if (strategy == 0) {
    foreach_output_function(PLA, so_do_espresso, so_save);
    } else {
    foreach_output_function(PLA, so_do_exact, so_save);
    }
    sf_free(PLA->F);
    PLA->F = Fmin;
}


/*
 *  minimize each output function, choose function or complement based on the
 *  one with the fewer number of terms
 */
void so_both_espresso(PLA, strategy)
pPLA PLA;
int strategy;
{
    phase = set_save(cube.fullset);
    Fmin = new_cover(PLA->F->count);
    if (strategy == 0) {
    foreach_output_function(PLA, so_both_do_espresso, so_both_save);
    } else {
    foreach_output_function(PLA, so_both_do_exact, so_both_save);
    }
    sf_free(PLA->F);
    PLA->F = Fmin;
    PLA->phase = phase;
}


int so_do_espresso(PLA, i)
pPLA PLA;
int i;
{
    char word[32];

    /* minimize the single-output function (on-set) */
    skip_make_sparse = 1;
    (void) sprintf(word, "ESPRESSO-POS(%d)", i);
    EXEC_S(PLA->F = espresso(PLA->F, PLA->D, PLA->R), word, PLA->F);
    return 1;
}


int so_do_exact(PLA, i)
pPLA PLA;
int i;
{
    char word[32];

    /* minimize the single-output function (on-set) */
    skip_make_sparse = 1;
    (void) sprintf(word, "EXACT-POS(%d)", i);
    EXEC_S(PLA->F = minimize_exact(PLA->F, PLA->D, PLA->R, 1), word, PLA->F);
    return 1;
}


/*ARGSUSED*/
int so_save(PLA, i)
pPLA PLA;
int i;
{
    Fmin = sf_append(Fmin, PLA->F);    /* disposes of PLA->F */
    PLA->F = NULL;
    return 1;
}


int so_both_do_espresso(PLA, i)
pPLA PLA;
int i;
{
    char word[32];

    /* minimize the single-output function (on-set) */
    (void) sprintf(word, "ESPRESSO-POS(%d)", i);
    skip_make_sparse = 1;
    EXEC_S(PLA->F = espresso(PLA->F, PLA->D, PLA->R), word, PLA->F);

    /* minimize the single-output function (off-set) */
    (void) sprintf(word, "ESPRESSO-NEG(%d)", i);
    skip_make_sparse = 1;
    EXEC_S(PLA->R = espresso(PLA->R, PLA->D, PLA->F), word, PLA->R);

    return 1;
}


int so_both_do_exact(PLA, i)
pPLA PLA;
int i;
{
    char word[32];

    /* minimize the single-output function (on-set) */
    (void) sprintf(word, "EXACT-POS(%d)", i);
    skip_make_sparse = 1;
    EXEC_S(PLA->F = minimize_exact(PLA->F, PLA->D, PLA->R, 1), word, PLA->F);

    /* minimize the single-output function (off-set) */
    (void) sprintf(word, "EXACT-NEG(%d)", i);
    skip_make_sparse = 1;
    EXEC_S(PLA->R = minimize_exact(PLA->R, PLA->D, PLA->F, 1), word, PLA->R);

    return 1;
}


int so_both_save(PLA, i)
pPLA PLA;
int i;
{
    if (PLA->F->count > PLA->R->count) {
    sf_free(PLA->F);
    PLA->F = PLA->R;
    PLA->R = NULL;
    i += cube.first_part[cube.output];
    set_remove(phase, i);
    } else {
    sf_free(PLA->R);
    PLA->R = NULL;
    }
    Fmin = sf_append(Fmin, PLA->F);
    PLA->F = NULL;
    return 1;
}
ABC_NAMESPACE_IMPL_END

