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


void map_dcset(PLA)
pPLA PLA;
{
    int var, i;
    pcover Tplus, Tminus, Tplusbar, Tminusbar;
    pcover newf, term1, term2, dcset, dcsetbar;
    pcube cplus, cminus, last, p;

    if (PLA->label == NIL(char *) || PLA->label[0] == NIL(char))
    return;

    /* try to find a binary variable named "DONT_CARE" */
    var = -1;
    for(i = 0; i < cube.num_binary_vars * 2; i++) {
    if (strncmp(PLA->label[i], "DONT_CARE", 9) == 0 ||
      strncmp(PLA->label[i], "DONTCARE", 8) == 0 ||
      strncmp(PLA->label[i], "dont_care", 9) == 0 ||
      strncmp(PLA->label[i], "dontcare", 8) == 0) {
        var = i/2;
        break;
    }
    }
    if (var == -1) {
    return;
    }

    /* form the cofactor cubes for the don't-care variable */
    cplus = set_save(cube.fullset);
    cminus = set_save(cube.fullset);
    set_remove(cplus, var*2);
    set_remove(cminus, var*2 + 1);

    /* form the don't-care set */
    EXEC(simp_comp(cofactor(cube1list(PLA->F), cplus), &Tplus, &Tplusbar),
    "simpcomp+", Tplus);
    EXEC(simp_comp(cofactor(cube1list(PLA->F), cminus), &Tminus, &Tminusbar),
    "simpcomp-", Tminus);
    EXEC(term1 = cv_intersect(Tplus, Tminusbar), "term1    ", term1);
    EXEC(term2 = cv_intersect(Tminus, Tplusbar), "term2    ", term2);
    EXEC(dcset = sf_union(term1, term2), "union     ", dcset);
    EXEC(simp_comp(cube1list(dcset), &PLA->D, &dcsetbar), "simplify", PLA->D);
    EXEC(newf = cv_intersect(PLA->F, dcsetbar), "separate  ", PLA->F);
    free_cover(PLA->F);
    PLA->F = newf;
    free_cover(Tplus);
    free_cover(Tminus);
    free_cover(Tplusbar);
    free_cover(Tminusbar);
    free_cover(dcsetbar);

    /* remove any cubes dependent on the DONT_CARE variable */
    (void) sf_active(PLA->F);
    foreach_set(PLA->F, last, p) {
    if (! is_in_set(p, var*2) || ! is_in_set(p, var*2+1)) {
        RESET(p, ACTIVE);
    }
    }
    PLA->F = sf_inactive(PLA->F);

    /* resize the cube and delete the don't-care variable */
    setdown_cube();
    for(i = 2*var+2; i < cube.size; i++) {
    PLA->label[i-2] = PLA->label[i];
    }
    for(i = var+1; i < cube.num_vars; i++) {
    cube.part_size[i-1] = cube.part_size[i];
    }
    cube.num_binary_vars--;
    cube.num_vars--;
    cube_setup();
    PLA->F = sf_delc(PLA->F, 2*var, 2*var+1);
    PLA->D = sf_delc(PLA->D, 2*var, 2*var+1);
}

void map_output_symbolic(PLA)
pPLA PLA;
{
    pset_family newF, newD;
    pset compress;
    symbolic_t *p1;
    symbolic_list_t *p2;
    int i, bit, tot_size, base, old_size;

    /* Remove the DC-set from the ON-set (is this necessary ??) */
    if (PLA->D->count > 0) {
    sf_free(PLA->F);
    PLA->F = complement(cube2list(PLA->D, PLA->R));
    }

    /* tot_size = width added for all symbolic variables */
    tot_size = 0;
    for(p1=PLA->symbolic_output; p1!=NIL(symbolic_t); p1=p1->next) {
    for(p2=p1->symbolic_list; p2!=NIL(symbolic_list_t); p2=p2->next) {
        if (p2->pos<0 || p2->pos>=cube.part_size[cube.output]) {
        fatal("symbolic-output index out of range");
/*        } else if (p2->variable != cube.output) {
        fatal("symbolic-output label must be an output");*/
        }
    }
    tot_size += 1 << p1->symbolic_list_length;
    }

    /* adjust the indices to skip over new outputs */
    for(p1=PLA->symbolic_output; p1!=NIL(symbolic_t); p1=p1->next) {
    for(p2=p1->symbolic_list; p2!=NIL(symbolic_list_t); p2=p2->next) {
        p2->pos += tot_size;
    }
    }

    /* resize the cube structure -- add enough for the one-hot outputs */
    old_size = cube.size;
    cube.part_size[cube.output] += tot_size;
    setdown_cube();
    cube_setup();

    /* insert space in the output part for the one-hot output */
    base = cube.first_part[cube.output];
    PLA->F = sf_addcol(PLA->F, base, tot_size);
    PLA->D = sf_addcol(PLA->D, base, tot_size);
    PLA->R = sf_addcol(PLA->R, base, tot_size);

    /* do the real work */
    for(p1=PLA->symbolic_output; p1!=NIL(symbolic_t); p1=p1->next) {
    newF = new_cover(100);
    newD = new_cover(100);
    find_inputs(NIL(set_family_t), PLA, p1->symbolic_list, base, 0,
                &newF, &newD);
/*
 *  Not sure what this means
    find_dc_inputs(PLA, p1->symbolic_list,
                base, 1 << p1->symbolic_list_length, &newF, &newD);
 */
    free_cover(PLA->F);
    PLA->F = newF;
/*
 *  retain OLD DC-set -- but we've lost the don't-care arc information
 *  (it defaults to branch to the zero state)
    free_cover(PLA->D);
    PLA->D = newD;
 */
    free_cover(newD);
    base += 1 << p1->symbolic_list_length;
    }

    /* delete the old outputs, and resize the cube */
    compress = set_full(newF->sf_size);
    for(p1=PLA->symbolic_output; p1!=NIL(symbolic_t); p1=p1->next) {
    for(p2=p1->symbolic_list; p2!=NIL(symbolic_list_t); p2=p2->next) {
        bit = cube.first_part[cube.output] + p2->pos;
        set_remove(compress, bit);
    }
    }
    cube.part_size[cube.output] -= newF->sf_size - set_ord(compress);
    setdown_cube();
    cube_setup();
    PLA->F = sf_compress(PLA->F, compress);
    PLA->D = sf_compress(PLA->D, compress);
    if (cube.size != PLA->F->sf_size) fatal("error");

    /* Quick minimization */
    PLA->F = sf_contain(PLA->F);
    PLA->D = sf_contain(PLA->D);
    for(i = 0; i < cube.num_vars; i++) {
    PLA->F = d1merge(PLA->F, i);
    PLA->D = d1merge(PLA->D, i);
    }
    PLA->F = sf_contain(PLA->F);
    PLA->D = sf_contain(PLA->D);

    free_cover(PLA->R);
    PLA->R = new_cover(0);

    symbolic_hack_labels(PLA, PLA->symbolic_output,
                compress, cube.size, old_size, tot_size);
    set_free(compress);
}


void find_inputs(A, PLA, list, base, value, newF, newD)
pcover A;
pPLA PLA;
symbolic_list_t *list;
int base, value;
pcover *newF, *newD;
{
    pcover S, S1;
    register pset last, p;

    /*
     *  A represents th 'input' values for which the outputs assume
     *  the integer value 'value
     */
    if (list == NIL(symbolic_list_t)) {
    /*
     *  Simulate these inputs against the on-set; then, insert into the
     *  new on-set a 1 in the proper position
     */
    S = cv_intersect(A, PLA->F);
    foreach_set(S, last, p) {
        set_insert(p, base + value);
    }
    *newF = sf_append(*newF, S);

    /*
     *  'simulate' these inputs against the don't-care set
    S = cv_intersect(A, PLA->D);
    *newD = sf_append(*newD, S);
     */

    } else {
    /* intersect and recur with the OFF-set */
    S = cof_output(PLA->R, cube.first_part[cube.output] + list->pos);
    if (A != NIL(set_family_t)) {
        S1 = cv_intersect(A, S);
        free_cover(S);
        S = S1;
    }
    find_inputs(S, PLA, list->next, base, value*2, newF, newD);
    free_cover(S);

    /* intersect and recur with the ON-set */
    S = cof_output(PLA->F, cube.first_part[cube.output] + list->pos);
    if (A != NIL(set_family_t)) {
        S1 = cv_intersect(A, S);
        free_cover(S);
        S = S1;
    }
    find_inputs(S, PLA, list->next, base, value*2 + 1, newF, newD);
    free_cover(S);
    }
}


#if 0
find_dc_inputs(PLA, list, base, maxval, newF, newD)
pPLA PLA;
symbolic_list_t *list;
int base, maxval;
pcover *newF, *newD;
{
    pcover A, S, S1;
    symbolic_list_t *p2;
    register pset p, last;
    register int i;

    /* painfully find the points for which the symbolic output is dc */
    A = NIL(set_family_t);
    for(p2=list; p2!=NIL(symbolic_list_t); p2=p2->next) {
    S = cof_output(PLA->D, cube.first_part[cube.output] + p2->pos);
    if (A == NIL(set_family_t)) {
        A = S;
    } else {
        S1 = cv_intersect(A, S);
        free_cover(S);
        free_cover(A);
        A = S1;
    }
    }

    S = cv_intersect(A, PLA->F);
    *newF = sf_append(*newF, S);

    S = cv_intersect(A, PLA->D);
    foreach_set(S, last, p) {
    for(i = base; i < base + maxval; i++) {
        set_insert(p, i);
    }
    }
    *newD = sf_append(*newD, S);
    free_cover(A);
}
#endif

void map_symbolic(PLA)
pPLA PLA;
{
    symbolic_t *p1;
    symbolic_list_t *p2;
    int var, base, num_vars, num_binary_vars, *new_part_size;
    int new_size, size_added, num_deleted_vars, num_added_vars, newvar;
    pset compress;

    /* Verify legal values are in the symbolic lists */
    for(p1 = PLA->symbolic; p1 != NIL(symbolic_t); p1 = p1->next) {
    for(p2=p1->symbolic_list; p2!=NIL(symbolic_list_t); p2=p2->next) {
        if (p2->variable  < 0 || p2->variable >= cube.num_binary_vars) {
        fatal(".symbolic requires binary variables");
        }
    }
    }

    /*
     *  size_added = width added for all symbolic variables
     *  num_deleted_vars = # binary variables to be deleted
     *  num_added_vars = # new mv variables
     *  compress = a cube which will be used to compress the set families
     */
    size_added = 0;
    num_added_vars = 0;
    for(p1 = PLA->symbolic; p1 != NIL(symbolic_t); p1 = p1->next) {
    size_added += 1 << p1->symbolic_list_length;
    num_added_vars++;
    }
    compress = set_full(PLA->F->sf_size + size_added);
    for(p1 = PLA->symbolic; p1 != NIL(symbolic_t); p1 = p1->next) {
    for(p2=p1->symbolic_list; p2!=NIL(symbolic_list_t); p2=p2->next) {
        set_remove(compress, p2->variable*2);
        set_remove(compress, p2->variable*2+1);
    }
    }
    num_deleted_vars = ((PLA->F->sf_size + size_added) - set_ord(compress))/2;

    /* compute the new cube constants */
    num_vars = cube.num_vars - num_deleted_vars + num_added_vars;
    num_binary_vars = cube.num_binary_vars - num_deleted_vars;
    new_size = cube.size - num_deleted_vars*2 + size_added;
    new_part_size = ALLOC(int, num_vars);
    new_part_size[num_vars-1] = cube.part_size[cube.num_vars-1];
    for(var = cube.num_binary_vars; var < cube.num_vars-1; var++) {
    new_part_size[var-num_deleted_vars] = cube.part_size[var];
    }

    /* re-size the covers, opening room for the new mv variables */
    base = cube.first_part[cube.output];
    PLA->F = sf_addcol(PLA->F, base, size_added);
    PLA->D = sf_addcol(PLA->D, base, size_added);
    PLA->R = sf_addcol(PLA->R, base, size_added);

    /* compute the values for the new mv variables */
    newvar = (cube.num_vars - 1) - num_deleted_vars;
    for(p1 = PLA->symbolic; p1 != NIL(symbolic_t); p1 = p1->next) {
    PLA->F = map_symbolic_cover(PLA->F, p1->symbolic_list, base);
    PLA->D = map_symbolic_cover(PLA->D, p1->symbolic_list, base);
    PLA->R = map_symbolic_cover(PLA->R, p1->symbolic_list, base);
    base += 1 << p1->symbolic_list_length;
    new_part_size[newvar++] = 1 << p1->symbolic_list_length;
    }

    /* delete the binary variables which disappear */
    PLA->F = sf_compress(PLA->F, compress);
    PLA->D = sf_compress(PLA->D, compress);
    PLA->R = sf_compress(PLA->R, compress);

    symbolic_hack_labels(PLA, PLA->symbolic, compress,
        new_size, cube.size, size_added);
    setdown_cube();
    FREE(cube.part_size);
    cube.num_vars = num_vars;
    cube.num_binary_vars = num_binary_vars;
    cube.part_size = new_part_size;
    cube_setup();
    set_free(compress);
}


pcover map_symbolic_cover(T, list, base)
pcover T;
symbolic_list_t *list;
int base;
{
    pset last, p;
    foreach_set(T, last, p) {
    form_bitvector(p, base, 0, list);
    }
    return T;
}


void form_bitvector(p, base, value, list)
pset p;            /* old cube, looking at binary variables */
int base;        /* where in mv cube the new variable starts */
int value;        /* current value for this recursion */
symbolic_list_t *list;    /* current place in the symbolic list */
{
    if (list == NIL(symbolic_list_t)) {
    set_insert(p, base + value);
    } else {
    switch(GETINPUT(p, list->variable)) {
        case ZERO:
        form_bitvector(p, base, value*2, list->next);
        break;
        case ONE:
        form_bitvector(p, base, value*2+1, list->next);
        break;
        case TWO:
        form_bitvector(p, base, value*2, list->next);
        form_bitvector(p, base, value*2+1, list->next);
        break;
        default:
        fatal("bad cube in form_bitvector");
    }
    }
}


void symbolic_hack_labels(PLA, list, compress, new_size, old_size, size_added)
pPLA PLA;
symbolic_t *list;
pset compress;
int new_size, old_size, size_added;
{
    int i, base;
    char **oldlabel;
    symbolic_t *p1;
    symbolic_label_t *p3;

    /* hack with the labels */
    if ((oldlabel = PLA->label) == NIL(char *))
    return;
    PLA->label = ALLOC(char *, new_size);
    for(i = 0; i < new_size; i++) {
    PLA->label[i] = NIL(char);
    }

    /* copy the binary variable labels and unchanged mv variable labels */
    base = 0;
    for(i = 0; i < cube.first_part[cube.output]; i++) {
    if (is_in_set(compress, i)) {
        PLA->label[base++] = oldlabel[i];
    } else {
        if (oldlabel[i] != NIL(char)) {
        FREE(oldlabel[i]);
        }
    }
    }

    /* add the user-defined labels for the symbolic outputs */
    for(p1 = list; p1 != NIL(symbolic_t); p1 = p1->next) {
    p3 = p1->symbolic_label;
    for(i = 0; i < (1 << p1->symbolic_list_length); i++) {
        if (p3 == NIL(symbolic_label_t)) {
        PLA->label[base+i] = ALLOC(char, 10);
        (void) sprintf(PLA->label[base+i], "X%d", i);
        } else {
        PLA->label[base+i] = p3->label;
        p3 = p3->next;
        }
    }
    base += 1 << p1->symbolic_list_length;
    }

    /* copy the labels for the binary outputs which remain */
    for(i = cube.first_part[cube.output]; i < old_size; i++) {
    if (is_in_set(compress, i + size_added)) {
        PLA->label[base++] = oldlabel[i];
    } else {
        if (oldlabel[i] != NIL(char)) {
        FREE(oldlabel[i]);
        }
    }
    }
    FREE(oldlabel);
}

static pcover fsm_simplify(F)
pcover F;
{
    pcover D, R;
    D = new_cover(0);
    R = complement(cube1list(F));
    F = espresso(F, D, R);
    free_cover(D);
    free_cover(R);
    return F;
}


void disassemble_fsm(PLA, verbose_mode)
pPLA PLA;
int verbose_mode;
{
    int nin, nstates, nout;
    int before, after, present_state, next_state, i, j;
    pcube next_state_mask, present_state_mask, state_mask, p, p1, last;
    pcover go_nowhere, F, tF;

    /* We make the DISGUSTING assumption that the first 'n' outputs have
     *  been created by .symbolic-output, and represent a one-hot encoding
     * of the next state.  'n' is the size of the second-to-last multiple-
     * valued variable (i.e., before the outputs
     */

    if (cube.num_vars - cube.num_binary_vars != 2) {
    (void) fprintf(stderr,
    "use .symbolic and .symbolic-output to specify\n");
    (void) fprintf(stderr,
    "the present state and next state field information\n");
    fatal("disassemble_pla: need two multiple-valued variables\n");
    }

    nin = cube.num_binary_vars;
    nstates = cube.part_size[cube.num_binary_vars];
    nout = cube.part_size[cube.num_vars - 1];
    if (nout < nstates) {
    (void) fprintf(stderr,
        "use .symbolic and .symbolic-output to specify\n");
    (void) fprintf(stderr,
        "the present state and next state field information\n");
    fatal("disassemble_pla: # outputs < # states\n");
    }


    present_state = cube.first_part[cube.num_binary_vars];
    present_state_mask = new_cube();
    for(i = 0; i < nstates; i++) {
    set_insert(present_state_mask, i + present_state);
    }

    next_state = cube.first_part[cube.num_binary_vars+1];
    next_state_mask = new_cube();
    for(i = 0; i < nstates; i++) {
    set_insert(next_state_mask, i + next_state);
    }

    state_mask = set_or(new_cube(), next_state_mask, present_state_mask);

    F = new_cover(10);


    /*
     *  check for arcs which go from ANY state to state #i
     */
    for(i = 0; i < nstates; i++) {
    tF = new_cover(10);
    foreach_set(PLA->F, last, p) {
        if (setp_implies(present_state_mask, p)) { /* from any state ! */
        if (is_in_set(p, next_state + i)) {
            tF = sf_addset(tF, p);
        }
        }
    }
    before = tF->count;
    if (before > 0) {
        tF = fsm_simplify(tF);
        /* don't allow the next state to disappear ... */
        foreach_set(tF, last, p) {
        set_insert(p, next_state + i);
        }
        after = tF->count;
        F = sf_append(F, tF);
        if (verbose_mode) {
        printf("# state EVERY to %d, before=%d after=%d\n",
            i, before, after);
        }
    }
    }


    /*
     *  some 'arcs' may NOT have a next state -- handle these
     *  we must unravel the present state part
     */
    go_nowhere = new_cover(10);
    foreach_set(PLA->F, last, p) {
    if (setp_disjoint(p, next_state_mask)) { /* no next state !! */
        go_nowhere = sf_addset(go_nowhere, p);
    }
    }
    before = go_nowhere->count;
    go_nowhere = unravel_range(go_nowhere,
                cube.num_binary_vars, cube.num_binary_vars);
    after = go_nowhere->count;
    F = sf_append(F, go_nowhere);
    if (verbose_mode) {
    printf("# state ANY to NOWHERE, before=%d after=%d\n", before, after);
    }


    /*
     *  minimize cover for all arcs from state #i to state #j
     */
    for(i = 0; i < nstates; i++) {
    for(j = 0; j < nstates; j++) {
        tF = new_cover(10);
        foreach_set(PLA->F, last, p) {
        /* not EVERY state */
        if (! setp_implies(present_state_mask, p)) {
            if (is_in_set(p, present_state + i)) {
            if (is_in_set(p, next_state + j)) {
                p1 = set_save(p);
                set_diff(p1, p1, state_mask);
                set_insert(p1, present_state + i);
                set_insert(p1, next_state + j);
                tF = sf_addset(tF, p1);
                set_free(p1);
            }
            }
        }
        }
        before = tF->count;
        if (before > 0) {
        tF = fsm_simplify(tF);
        /* don't allow the next state to disappear ... */
        foreach_set(tF, last, p) {
            set_insert(p, next_state + j);
        }
        after = tF->count;
        F = sf_append(F, tF);
        if (verbose_mode) {
            printf("# state %d to %d, before=%d after=%d\n",
                i, j, before, after);
        }
        }
    }
    }


    free_cube(state_mask);
    free_cube(present_state_mask);
    free_cube(next_state_mask);

    free_cover(PLA->F);
    PLA->F = F;
    free_cover(PLA->D);
    PLA->D = new_cover(0);

    setdown_cube();
    FREE(cube.part_size);
    cube.num_binary_vars = nin;
    cube.num_vars = nin + 3;
    cube.part_size = ALLOC(int, cube.num_vars);
    cube.part_size[cube.num_binary_vars] = nstates;
    cube.part_size[cube.num_binary_vars+1] = nstates;
    cube.part_size[cube.num_binary_vars+2] = nout - nstates;
    cube_setup();

    foreach_set(PLA->F, last, p) {
    kiss_print_cube(stdout, PLA, p, "~1");
    }
}
ABC_NAMESPACE_IMPL_END

