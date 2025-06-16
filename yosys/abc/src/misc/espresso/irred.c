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


static void fcube_is_covered();
static void ftautology();
static bool ftaut_special_cases();


static int Rp_current;

/*
 *   irredundant -- Return a minimal subset of F
 */

pcover
irredundant(F, D)
pcover F, D;
{
    mark_irredundant(F, D);
    return sf_inactive(F);
}


/*
 *   mark_irredundant -- find redundant cubes, and mark them "INACTIVE"
 */

void
mark_irredundant(F, D)
pcover F, D;
{
    pcover E, Rt, Rp;
    pset p, p1, last;
    sm_matrix *table;
    sm_row *cover;
    sm_element *pe;

    /* extract a minimum cover */
    irred_split_cover(F, D, &E, &Rt, &Rp);
    table = irred_derive_table(D, E, Rp);
    cover = sm_minimum_cover(table, NIL(int), /* heuristic */ 1, /* debug */ 0);

    /* mark the cubes for the result */
    foreach_set(F, last, p) {
    RESET(p, ACTIVE);
    RESET(p, RELESSEN);
    }
    foreach_set(E, last, p) {
    p1 = GETSET(F, SIZE(p));
    assert(setp_equal(p1, p));
    SET(p1, ACTIVE);
    SET(p1, RELESSEN);        /* for essen(), mark as rel. ess. */
    }
    sm_foreach_row_element(cover, pe) {
    p1 = GETSET(F, pe->col_num);
    SET(p1, ACTIVE);
    }

    if (debug & IRRED) {
    printf("# IRRED: F=%d E=%d R=%d Rt=%d Rp=%d Rc=%d Final=%d Bound=%d\n",
        F->count, E->count, Rt->count+Rp->count, Rt->count, Rp->count,
        cover->length, E->count + cover->length, 0);
    }

    free_cover(E);
    free_cover(Rt);
    free_cover(Rp);
    sm_free(table);
    sm_row_free(cover);
}

/*
 *  irred_split_cover -- find E, Rt, and Rp from the cover F, D
 *
 *    E  -- relatively essential cubes
 *    Rt  -- totally redundant cubes
 *    Rp  -- partially redundant cubes
 */

void
irred_split_cover(F, D, E, Rt, Rp)
pcover F, D;
pcover *E, *Rt, *Rp;
{
    register pcube p, last;
    register int index;
    pcover R;
    pcube *FD, *ED;

    /* number the cubes of F -- these numbers track into E, Rp, Rt, etc. */
    index = 0;
    foreach_set(F, last, p) {
    PUTSIZE(p, index);
    index++;
    }

    *E = new_cover(10);
    *Rt = new_cover(10);
    *Rp = new_cover(10);
    R = new_cover(10);

    /* Split F into E and R */
    FD = cube2list(F, D);
    foreach_set(F, last, p) {
    if (cube_is_covered(FD, p)) {
        R = sf_addset(R, p);
    } else {
        *E = sf_addset(*E, p);
    }
    if (debug & IRRED1) {
        (void) printf("IRRED1: zr=%d ze=%d to-go=%d time=%s\n",
        R->count, (*E)->count, F->count - (R->count + (*E)->count),
        print_time(ptime()));
    }
    }
    free_cubelist(FD);

    /* Split R into Rt and Rp */
    ED = cube2list(*E, D);
    foreach_set(R, last, p) {
    if (cube_is_covered(ED, p)) {
        *Rt = sf_addset(*Rt, p);
    } else {
        *Rp = sf_addset(*Rp, p);
    }
    if (debug & IRRED1) {
        (void) printf("IRRED1: zr=%d zrt=%d to-go=%d time=%s\n",
        (*Rp)->count, (*Rt)->count,
        R->count - ((*Rp)->count +(*Rt)->count), print_time(ptime()));
    }
    }
    free_cubelist(ED);

    free_cover(R);
}

/*
 *  irred_derive_table -- given the covers D, E and the set of
 *  partially redundant primes Rp, build a covering table showing
 *  possible selections of primes to cover Rp.
 */

sm_matrix *
irred_derive_table(D, E, Rp)
pcover D, E, Rp;
{
    register pcube last, p, *list;
    sm_matrix *table;
    int size_last_dominance, i;

    /* Mark each cube in DE as not part of the redundant set */
    foreach_set(D, last, p) {
    RESET(p, REDUND);
    }
    foreach_set(E, last, p) {
    RESET(p, REDUND);
    }

    /* Mark each cube in Rp as partially redundant */
    foreach_set(Rp, last, p) {
    SET(p, REDUND);             /* belongs to redundant set */
    }

    /* For each cube in Rp, find ways to cover its minterms */
    list = cube3list(D, E, Rp);
    table = sm_alloc();
    size_last_dominance = 0;
    i = 0;
    foreach_set(Rp, last, p) {
    Rp_current = SIZE(p);
    fcube_is_covered(list, p, table);
    RESET(p, REDUND);    /* can now consider this cube redundant */
    if (debug & IRRED1) {
        (void) printf("IRRED1: %d of %d to-go=%d, table=%dx%d time=%s\n",
        i, Rp->count, Rp->count - i,
        table->nrows, table->ncols, print_time(ptime()));
    }
    /* try to keep memory limits down by reducing table as we go along */
    if (table->nrows - size_last_dominance > 1000) {
        (void) sm_row_dominance(table);
        size_last_dominance = table->nrows;
        if (debug & IRRED1) {
        (void) printf("IRRED1: delete redundant rows, now %dx%d\n",
            table->nrows, table->ncols);
        }
    }
    i++;
    }
    free_cubelist(list);

    return table;
}

/* cube_is_covered -- determine if a cubelist "covers" a single cube */
bool
cube_is_covered(T, c)
pcube *T, c;
{
    return tautology(cofactor(T,c));
}



/* tautology -- answer the tautology question for T */
bool
tautology(T)
pcube *T;         /* T will be disposed of */
{
    register pcube cl, cr;
    register int best, result;
    static int taut_level = 0;

    if (debug & TAUT) {
    debug_print(T, "TAUTOLOGY", taut_level++);
    }

    if ((result = taut_special_cases(T)) == MAYBE) {
    cl = new_cube();
    cr = new_cube();
    best = binate_split_select(T, cl, cr, TAUT);
    result = tautology(scofactor(T, cl, best)) &&
         tautology(scofactor(T, cr, best));
    free_cubelist(T);
    free_cube(cl);
    free_cube(cr);
    }

    if (debug & TAUT) {
    printf("exit TAUTOLOGY[%d]: %s\n", --taut_level, print_bool(result));
    }
    return result;
}

/*
 *  taut_special_cases -- check special cases for tautology
 */

bool
taut_special_cases(T)
pcube *T;            /* will be disposed if answer is determined */
{
    register pcube *T1, *Tsave, p, ceil=cube.temp[0], temp=cube.temp[1];
    pcube *A, *B;
    int var;

    /* Check for a row of all 1's which implies tautology */
    for(T1 = T+2; (p = *T1++) != NULL; ) {
    if (full_row(p, T[0])) {
        free_cubelist(T);
        return TRUE;
    }
    }

    /* Check for a column of all 0's which implies no tautology */
start:
    INLINEset_copy(ceil, T[0]);
    for(T1 = T+2; (p = *T1++) != NULL; ) {
    INLINEset_or(ceil, ceil, p);
    }
    if (! setp_equal(ceil, cube.fullset)) {
    free_cubelist(T);
    return FALSE;
    }

    /* Collect column counts, determine unate variables, etc. */
    massive_count(T);

    /* If function is unate (and no row of all 1's), then no tautology */
    if (cdata.vars_unate == cdata.vars_active) {
    free_cubelist(T);
    return FALSE;

    /* If active in a single variable (and no column of 0's) then tautology */
    } else if (cdata.vars_active == 1) {
    free_cubelist(T);
    return TRUE;

    /* Check for unate variables, and reduce cover if there are any */
    } else if (cdata.vars_unate != 0) {
    /* Form a cube "ceil" with full variables in the unate variables */
    (void) set_copy(ceil, cube.emptyset);
    for(var = 0; var < cube.num_vars; var++) {
        if (cdata.is_unate[var]) {
        INLINEset_or(ceil, ceil, cube.var_mask[var]);
        }
    }

    /* Save only those cubes that are "full" in all unate variables */
    for(Tsave = T1 = T+2; (p = *T1++) != 0; ) {
        if (setp_implies(ceil, set_or(temp, p, T[0]))) {
        *Tsave++ = p;
        }
    }
    *Tsave++ = NULL;
    T[1] = (pcube) Tsave;

    if (debug & TAUT) {
        printf("UNATE_REDUCTION: %d unate variables, reduced to %d\n",
        (int)cdata.vars_unate, (int)CUBELISTSIZE(T));
    }
    goto start;

    /* Check for component reduction */
    } else if (cdata.var_zeros[cdata.best] < CUBELISTSIZE(T) / 2) {
    if (cubelist_partition(T, &A, &B, debug & TAUT) == 0) {
        return MAYBE;
    } else {
        free_cubelist(T);
        if (tautology(A)) {
        free_cubelist(B);
        return TRUE;
        } else {
        return tautology(B);
        }
    }
    }

    /* We tried as hard as we could, but must recurse from here on */
    return MAYBE;
}

/* fcube_is_covered -- determine exactly how a cubelist "covers" a cube */
static void
fcube_is_covered(T, c, table)
pcube *T, c;
sm_matrix *table;
{
    ftautology(cofactor(T,c), table);
}


/* ftautology -- find ways to make a tautology */
static void
ftautology(T, table)
pcube *T;             /* T will be disposed of */
sm_matrix *table;
{
    register pcube cl, cr;
    register int best;
    static int ftaut_level = 0;

    if (debug & TAUT) {
    debug_print(T, "FIND_TAUTOLOGY", ftaut_level++);
    }

    if (ftaut_special_cases(T, table) == MAYBE) {
    cl = new_cube();
    cr = new_cube();
    best = binate_split_select(T, cl, cr, TAUT);

    ftautology(scofactor(T, cl, best), table);
    ftautology(scofactor(T, cr, best), table);

    free_cubelist(T);
    free_cube(cl);
    free_cube(cr);
    }

    if (debug & TAUT) {
    (void) printf("exit FIND_TAUTOLOGY[%d]: table is %d by %d\n",
        --ftaut_level, table->nrows, table->ncols);
    }
}

static bool
ftaut_special_cases(T, table)
pcube *T;                 /* will be disposed if answer is determined */
sm_matrix *table;
{
    register pcube *T1, *Tsave, p, temp = cube.temp[0], ceil = cube.temp[1];
    int var, rownum;

    /* Check for a row of all 1's in the essential cubes */
    for(T1 = T+2; (p = *T1++) != 0; ) {
    if (! TESTP(p, REDUND)) {
        if (full_row(p, T[0])) {
        /* subspace is covered by essentials -- no new rows for table */
        free_cubelist(T);
        return TRUE;
        }
    }
    }

    /* Collect column counts, determine unate variables, etc. */
start:
    massive_count(T);

    /* If function is unate, find the rows of all 1's */
    if (cdata.vars_unate == cdata.vars_active) {
    /* find which nonessentials cover this subspace */
    rownum = table->last_row ? table->last_row->row_num+1 : 0;
    (void) sm_insert(table, rownum, Rp_current);
    for(T1 = T+2; (p = *T1++) != 0; ) {
        if (TESTP(p, REDUND)) {
        /* See if a redundant cube covers this leaf */
        if (full_row(p, T[0])) {
            (void) sm_insert(table, rownum, (int) SIZE(p));
        }
        }
    }
    free_cubelist(T);
    return TRUE;

    /* Perform unate reduction if there are any unate variables */
    } else if (cdata.vars_unate != 0) {
    /* Form a cube "ceil" with full variables in the unate variables */
    (void) set_copy(ceil, cube.emptyset);
    for(var = 0; var < cube.num_vars; var++) {
        if (cdata.is_unate[var]) {
        INLINEset_or(ceil, ceil, cube.var_mask[var]);
        }
    }

    /* Save only those cubes that are "full" in all unate variables */
    for(Tsave = T1 = T+2; (p = *T1++) != 0; ) {
        if (setp_implies(ceil, set_or(temp, p, T[0]))) {
        *Tsave++ = p;
        }
    }
    *Tsave++ = 0;
    T[1] = (pcube) Tsave;

    if (debug & TAUT) {
        printf("UNATE_REDUCTION: %d unate variables, reduced to %d\n",
        (int)cdata.vars_unate, (int)CUBELISTSIZE(T));
    }
    goto start;
    }

    /* Not much we can do about it */
    return MAYBE;
}
ABC_NAMESPACE_IMPL_END

