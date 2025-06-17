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


/*
 *   Phase assignment technique (T. Sasao):
 *
 *      1. create a function with 2*m outputs which implements the
 *      original function and its complement for each output
 *
 *      2. minimize this function
 *
 *      3. choose the minimum number of prime implicants from the
 *      result of step 2 which are needed to realize either a function
 *      or its complement for each output
 *
 *  Step 3 is performed in a rather crude way -- by simply multiplying
 *  out a large expression of the form:
 *
 *      I = (ab + cdef)(acd + bgh) ...
 *
 *  which is a product of m expressions where each expression has two
 *  product terms -- one representing which primes are needed for the
 *  function, and one representing which primes are needed for the
 *  complement.  The largest product term resulting shows which primes
 *  to keep to implement one function or the other for each output.
 *  For problems with many outputs, this may grind to a
 *  halt.
 *
 *  Untried: form complement of I and use unate_complement ...
 *
 *  I have unsuccessfully tried several modifications to the basic
 *  algorithm.  The first is quite simple: use Sasao's technique, but
 *  only commit to a single output at a time (rather than all
 *  outputs).  The goal would be that the later minimizations can "take
 *  into account" the partial assignment at each step.  This is
 *  expensive (m+1 minimizations rather than 2), and the results are
 *  discouraging.
 *
 *  The second modification is rather complicated.  The result from the
 *  minimization in step 2 is guaranteed to be minimal.  Hence, for
 *  each output, the set of primes with a 1 in that output are both
 *  necessary and sufficient to implement the function.  Espresso
 *  achieves the minimality using the routine MAKE_SPARSE.  The
 *  modification is to prevent MAKE_SPARSE from running.  Hence, there
 *  are potentially many subsets of the set of primes with a 1 in a
 *  column which can be used to implement that output.  We use
 *  IRREDUNDANT to enumerate all possible subsets and then proceed as
 *  before.
 */

static int opo_no_make_sparse;
static int opo_repeated;
static int opo_exact;
static void minimize();

void phase_assignment(PLA, opo_strategy)
pPLA PLA;
int opo_strategy;
{
    opo_no_make_sparse = opo_strategy % 2;
    skip_make_sparse = opo_no_make_sparse;
    opo_repeated = (opo_strategy / 2) % 2;
    opo_exact = (opo_strategy / 4) % 2;

    /* Determine a phase assignment */
    if (PLA->phase != NULL) {
    FREE(PLA->phase);
    }

    if (opo_repeated) {
    PLA->phase = set_save(cube.fullset);
    repeated_phase_assignment(PLA);
    } else {
    PLA->phase = find_phase(PLA, 0, (pcube) NULL);
    }

    /* Now minimize with this assignment */
    skip_make_sparse = FALSE;
    (void) set_phase(PLA);
    minimize(PLA);
}

/*
 *  repeated_phase_assignment -- an alternate strategy which commits
 *  to a single phase assignment a step at a time.  Performs m + 1
 *  minimizations !
 */
void repeated_phase_assignment(PLA)
pPLA PLA;
{
    int i;
    pcube phase;

    for(i = 0; i < cube.part_size[cube.output]; i++) {

    /* Find best assignment for all undecided outputs */
    phase = find_phase(PLA, i, PLA->phase);

    /* Commit for only a single output ... */
    if (! is_in_set(phase, cube.first_part[cube.output] + i)) {
        set_remove(PLA->phase, cube.first_part[cube.output] + i);
    }

    if (trace || summary) {
        printf("\nOPO loop for output #%d\n", i);
        printf("PLA->phase is %s\n", pc1(PLA->phase));
        printf("phase      is %s\n", pc1(phase));
    }
    set_free(phase);
    }
}


/*
 *  find_phase -- find a phase assignment for the PLA for all outputs starting
 *  with output number first_output.
 */
pcube find_phase(PLA, first_output, phase1)
pPLA PLA;
int first_output;
pcube phase1;
{
    pcube phase;
    pPLA PLA1;

    phase = set_save(cube.fullset);

    /* setup the double-phase characteristic function, resize the cube */
    PLA1 = new_PLA();
    PLA1->F = sf_save(PLA->F);
    PLA1->R = sf_save(PLA->R);
    PLA1->D = sf_save(PLA->D);
    if (phase1 != NULL) {
    PLA1->phase = set_save(phase1);
    (void) set_phase(PLA1);
    }
    EXEC_S(output_phase_setup(PLA1, first_output), "OPO-SETUP ", PLA1->F);

    /* minimize the double-phase function */
    minimize(PLA1);

    /* set the proper phases according to what gives a minimum solution */
    EXEC_S(PLA1->F = opo(phase, PLA1->F, PLA1->D, PLA1->R, first_output),
        "OPO       ", PLA1->F);
    free_PLA(PLA1);

    /* set the cube structure to reflect the old size */
    setdown_cube();
    cube.part_size[cube.output] -=
    (cube.part_size[cube.output] - first_output) / 2;
    cube_setup();

    return phase;
}

/*
 *  opo -- multiply the expression out to determine a minimum subset of
 *  primes.
 */

/*ARGSUSED*/
pcover opo(phase, T, D, R, first_output)
pcube phase;
pcover T, D, R;
int first_output;
{
    int offset, output, i, last_output, ind;
    pset pdest, select, p, p1, last, last1, not_covered, tmp;
    pset_family temp, T1, T2;

    /* must select all primes for outputs [0 .. first_output-1] */
    select = set_full(T->count);
    for(output = 0; output < first_output; output++) {
    ind = cube.first_part[cube.output] + output;
    foreachi_set(T, i, p) {
        if (is_in_set(p, ind)) {
        set_remove(select, i);
        }
    }
    }

    /* Recursively perform the intersections */
    offset = (cube.part_size[cube.output] - first_output) / 2;
    last_output = first_output + offset - 1;
    temp = opo_recur(T, D, select, offset, first_output, last_output);

    /* largest set is on top -- select primes which are inferred from it */
    pdest = temp->data;
    T1 = new_cover(T->count);
    foreachi_set(T, i, p) {
    if (! is_in_set(pdest, i)) {
        T1 = sf_addset(T1, p);
    }
    }

    set_free(select);
    sf_free(temp);

    /* finding phases is difficult -- see which functions are not covered */
    T2 = complement(cube1list(T1));
    not_covered = new_cube();
    tmp = new_cube();
    foreach_set(T, last, p) {
    foreach_set(T2, last1, p1) {
        if (cdist0(p, p1)) {
        (void) set_or(not_covered, not_covered, set_and(tmp, p, p1));
        }
    }
    }
    free_cover(T);
    free_cover(T2);
    set_free(tmp);

    /* Now reflect the phase choice in a single cube */
    for(output = first_output; output <= last_output; output++) {
    ind = cube.first_part[cube.output] + output;
    if (is_in_set(not_covered, ind)) {
        if (is_in_set(not_covered, ind + offset)) {
        fatal("error in output phase assignment");
        } else {
        set_remove(phase, ind);
        }
    }
    }
    set_free(not_covered);
    return T1;
}

pset_family opo_recur(T, D, select, offset, first, last)
pcover T, D;
pcube select;
int offset, first, last;
{
    static int level = 0;
    int middle;
    pset_family sl, sr, temp;

    level++;
    if (first == last) {
#if 0
    if (opo_no_make_sparse) {
        temp = form_cover_table(T, D, select, first, first + offset);
    } else {
        temp = opo_leaf(T, select, first, first + offset);
    }
#else
    temp = opo_leaf(T, select, first, first + offset);
#endif
    } else {
    middle = (first + last) / 2;
    sl = opo_recur(T, D, select, offset, first, middle);
    sr = opo_recur(T, D, select, offset, middle+1, last);
    temp = unate_intersect(sl, sr, level == 1);
    if (trace) {
        printf("# OPO[%d]: %4d = %4d x %4d, time = %s\n", level - 1,
        temp->count, sl->count, sr->count, print_time(ptime()));
        (void) fflush(stdout);
    }
    free_cover(sl);
    free_cover(sr);
    }
    level--;
    return temp;
}


pset_family opo_leaf(T, select, out1, out2)
register pcover T;
pset select;
int out1, out2;
{
    register pset_family temp;
    register pset p, pdest;
    register int i;

    out1 += cube.first_part[cube.output];
    out2 += cube.first_part[cube.output];

    /* Allocate space for the result */
    temp = sf_new(2, T->count);

    /* Find which primes are needed for the ON-set of this fct */
    pdest = GETSET(temp, temp->count++);
    (void) set_copy(pdest, select);
    foreachi_set(T, i, p) {
    if (is_in_set(p, out1)) {
        set_remove(pdest, i);
    }
    }

    /* Find which primes are needed for the OFF-set of this fct */
    pdest = GETSET(temp, temp->count++);
    (void) set_copy(pdest, select);
    foreachi_set(T, i, p) {
    if (is_in_set(p, out2)) {
        set_remove(pdest, i);
    }
    }

    return temp;
}

#if 0
pset_family form_cover_table(F, D, select, f, fbar)
pcover F, D;
pset select;
int f, fbar;        /* indices of f and fbar in the output part */
{
    register int i;
    register pcube p;
    pset_family f_table, fbar_table;

    /* setup required for fcube_is_covered */
    Rp_size = F->count;
    Rp_start = set_new(Rp_size);
    foreachi_set(F, i, p) {
    PUTSIZE(p, i);
    }
    foreachi_set(D, i, p) {
    RESET(p, REDUND);
    }

    f_table = find_covers(F, D, select, f);
    fbar_table = find_covers(F, D, select, fbar);
    f_table = sf_append(f_table, fbar_table);

    set_free(Rp_start);
    return f_table;
}


pset_family find_covers(F, D, select, n)
pcover F, D;
register pset select;
int n;
{
    register pset p, last, new;
    pcover F1;
    pcube *Flist;
    pset_family f_table, table;
    int i;

    n += cube.first_part[cube.output];

    /* save cubes in this output, and remove the output variable */
    F1 = new_cover(F->count);
    foreach_set(F, last, p)
    if (is_in_set(p, n)) {
        new = GETSET(F1, F1->count++);
        set_or(new, p, cube.var_mask[cube.output]);
        PUTSIZE(new, SIZE(p));
        SET(new, REDUND);
    }

    /* Find ways (sop form) to fail to cover output indexed by n */
    Flist = cube2list(F1, D);
    table = sf_new(10, Rp_size);
    foreach_set(F1, last, p) {
    set_fill(Rp_start, Rp_size);
    set_remove(Rp_start, SIZE(p));
    table = sf_append(table, fcube_is_covered(Flist, p));
    RESET(p, REDUND);
    }
    set_fill(Rp_start, Rp_size);
    foreach_set(table, last, p) {
    set_diff(p, Rp_start, p);
    }

    /* complement this to get possible ways to cover the function */
    for(i = 0; i < Rp_size; i++) {
    if (! is_in_set(select, i)) {
        p = set_new(Rp_size);
        set_insert(p, i);
        table = sf_addset(table, p);
        set_free(p);
    }
    }
    f_table = unate_compl(table);

    /* what a pain, but we need bitwise complement of this */
    set_fill(Rp_start, Rp_size);
    foreach_set(f_table, last, p) {
    set_diff(p, Rp_start, p);
    }

    free_cubelist(Flist);
    sf_free(F1);
    return f_table;
}
#endif

/*
 *  Take a PLA (ON-set, OFF-set and DC-set) and create the
 *  "double-phase characteristic function" which is merely a new
 *  function which has twice as many outputs and realizes both the
 *  function and the complement.
 *
 *  The cube structure is assumed to represent the PLA upon entering.
 *  It will be modified to represent the double-phase function upon
 *  exit.
 *
 *  Only the outputs numbered starting with "first_output" are
 *  duplicated in the output part
 */

void output_phase_setup(PLA, first_output)
INOUT pPLA PLA;
int first_output;
{
    pcover F, R, D;
    pcube mask, mask1, last;
    int first_part, offset;
    bool save;
    register pcube p, pr, pf;
    register int i, last_part;

    if (cube.output == -1)
    fatal("output_phase_setup: must have an output");

    F = PLA->F;
    D = PLA->D;
    R = PLA->R;
    first_part = cube.first_part[cube.output] + first_output;
    last_part = cube.last_part[cube.output];
    offset = cube.part_size[cube.output] - first_output;

    /* Change the output size, setup the cube structure */
    setdown_cube();
    cube.part_size[cube.output] += offset;
    cube_setup();

    /* Create a mask to select that part of the cube which isn't changing */
    mask = set_save(cube.fullset);
    for(i = first_part; i < cube.size; i++)
    set_remove(mask, i);
    mask1 = set_save(mask);
    for(i = cube.first_part[cube.output]; i < first_part; i++) {
    set_remove(mask1, i);
    }

    PLA->F = new_cover(F->count + R->count);
    PLA->R = new_cover(F->count + R->count);
    PLA->D = new_cover(D->count);

    foreach_set(F, last, p) {
    pf = GETSET(PLA->F, (PLA->F)->count++);
    pr = GETSET(PLA->R, (PLA->R)->count++);
    INLINEset_and(pf, mask, p);
    INLINEset_and(pr, mask1, p);
    for(i = first_part; i <= last_part; i++)
        if (is_in_set(p, i))
        set_insert(pf, i);
    save = FALSE;
    for(i = first_part; i <= last_part; i++)
        if (is_in_set(p, i))
        save = TRUE, set_insert(pr, i+offset);
    if (! save) PLA->R->count--;
    }

    foreach_set(R, last, p) {
    pf = GETSET(PLA->F, (PLA->F)->count++);
    pr = GETSET(PLA->R, (PLA->R)->count++);
    INLINEset_and(pf, mask1, p);
    INLINEset_and(pr, mask, p);
    save = FALSE;
    for(i = first_part; i <= last_part; i++)
        if (is_in_set(p, i))
        save = TRUE, set_insert(pf, i+offset);
    if (! save) PLA->F->count--;
    for(i = first_part; i <= last_part; i++)
        if (is_in_set(p, i))
        set_insert(pr, i);
    }

    foreach_set(D, last, p) {
    pf = GETSET(PLA->D, (PLA->D)->count++);
    INLINEset_and(pf, mask, p);
    for(i = first_part; i <= last_part; i++)
        if (is_in_set(p, i)) {
        set_insert(pf, i);
        set_insert(pf, i+offset);
        }
    }

    free_cover(F);
    free_cover(D);
    free_cover(R);
    set_free(mask);
    set_free(mask1);
}

/*
 *  set_phase -- given a "cube" which describes which phases of the output
 *  are to be implemented, compute the appropriate on-set and off-set
 */
pPLA set_phase(PLA)
INOUT pPLA PLA;
{
    pcover F1, R1;
    register pcube last, p, outmask;
    register pcube temp=cube.temp[0], phase=PLA->phase, phase1=cube.temp[1];

    outmask = cube.var_mask[cube.num_vars - 1];
    set_diff(phase1, outmask, phase);
    set_or(phase1, set_diff(temp, cube.fullset, outmask), phase1);
    F1 = new_cover((PLA->F)->count + (PLA->R)->count);
    R1 = new_cover((PLA->F)->count + (PLA->R)->count);

    foreach_set(PLA->F, last, p) {
    if (! setp_disjoint(set_and(temp, p, phase), outmask))
        set_copy(GETSET(F1, F1->count++), temp);
    if (! setp_disjoint(set_and(temp, p, phase1), outmask))
        set_copy(GETSET(R1, R1->count++), temp);
    }
    foreach_set(PLA->R, last, p) {
    if (! setp_disjoint(set_and(temp, p, phase), outmask))
        set_copy(GETSET(R1, R1->count++), temp);
    if (! setp_disjoint(set_and(temp, p, phase1), outmask))
        set_copy(GETSET(F1, F1->count++), temp);
    }
    free_cover(PLA->F);
    free_cover(PLA->R);
    PLA->F = F1;
    PLA->R = R1;
    return PLA;
}

#define POW2(x)        (1 << (x))

void opoall(PLA, first_output, last_output, opo_strategy)
pPLA PLA;
int first_output, last_output;
int opo_strategy;
{
    pcover F, D, R, best_F, best_D, best_R;
    int i, j, ind, num;
    pcube bestphase;

    opo_exact = opo_strategy;

    if (PLA->phase != NULL) {
    set_free(PLA->phase);
    }

    bestphase = set_save(cube.fullset);
    best_F = sf_save(PLA->F);
    best_D = sf_save(PLA->D);
    best_R = sf_save(PLA->R);

    for(i = 0; i < POW2(last_output - first_output + 1); i++) {

    /* save the initial PLA covers */
    F = sf_save(PLA->F);
    D = sf_save(PLA->D);
    R = sf_save(PLA->R);

    /* compute the phase cube for this iteration */
    PLA->phase = set_save(cube.fullset);
    num = i;
    for(j = last_output; j >= first_output; j--) {
        if (num % 2 == 0) {
        ind = cube.first_part[cube.output] + j;
        set_remove(PLA->phase, ind);
        }
        num /= 2;
    }

    /* set the phase and minimize */
    (void) set_phase(PLA);
    printf("# phase is %s\n", pc1(PLA->phase));
    summary = TRUE;
    minimize(PLA);

    /* see if this is the best so far */
    if (PLA->F->count < best_F->count) {
        /* save new best solution */
        set_copy(bestphase, PLA->phase);
        sf_free(best_F);
        sf_free(best_D);
        sf_free(best_R);
        best_F = PLA->F;
        best_D = PLA->D;
        best_R = PLA->R;
    } else {
        /* throw away the solution */
        free_cover(PLA->F);
        free_cover(PLA->D);
        free_cover(PLA->R);
    }
    set_free(PLA->phase);

    /* restore the initial PLA covers */
    PLA->F = F;
    PLA->D = D;
    PLA->R = R;
    }

    /* one more minimization to restore the best answer */
    PLA->phase = bestphase;
    sf_free(PLA->F);
    sf_free(PLA->D);
    sf_free(PLA->R);
    PLA->F = best_F;
    PLA->D = best_D;
    PLA->R = best_R;
}

static void minimize(PLA)
pPLA PLA;
{
    if (opo_exact) {
    EXEC_S(PLA->F = minimize_exact(PLA->F,PLA->D,PLA->R,1), "EXACT", PLA->F);
    } else {
    EXEC_S(PLA->F = espresso(PLA->F, PLA->D, PLA->R), "ESPRESSO  ",PLA->F);
    }
}
ABC_NAMESPACE_IMPL_END

