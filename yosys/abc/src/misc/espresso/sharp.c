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
    sharp.c -- perform sharp, disjoint sharp, and intersection
*/

#include "espresso.h"

ABC_NAMESPACE_IMPL_START


long start_time;


/* cv_sharp -- form the sharp product between two covers */
pcover cv_sharp(A, B)
pcover A, B;
{
    pcube last, p;
    pcover T;

    T = new_cover(0);
    foreach_set(A, last, p)
    T = sf_union(T, cb_sharp(p, B));
    return T;
}


/* cb_sharp -- form the sharp product between a cube and a cover */
pcover cb_sharp(c, T)
pcube c;
pcover T;
{
    if (T->count == 0) {
    T = sf_addset(new_cover(1), c);
    } else {
    start_time = ptime();
    T = cb_recur_sharp(c, T, 0, T->count-1, 0);
    }
    return T;
}


/* recursive formulation to provide balanced merging */
pcover cb_recur_sharp(c, T, first, last, level)
pcube c;
pcover T;
int first, last, level;
{
    pcover temp, left, right;
    int middle;

    if (first == last) {
    temp = sharp(c, GETSET(T, first));
    } else {
    middle = (first + last) / 2;
    left = cb_recur_sharp(c, T, first, middle, level+1);
    right = cb_recur_sharp(c, T, middle+1, last, level+1);
    temp = cv_intersect(left, right);
    if ((debug & SHARP) && level < 4) {
        printf("# SHARP[%d]: %4d = %4d x %4d, time = %s\n",
        level, temp->count, left->count, right->count,
        print_time(ptime() - start_time));
        (void) fflush(stdout);
    }
    free_cover(left);
    free_cover(right);
    }
    return temp;
}


/* sharp -- form the sharp product between two cubes */
pcover sharp(a, b)
pcube a, b;
{
    register int var;
    register pcube d=cube.temp[0], temp=cube.temp[1], temp1=cube.temp[2];
    pcover r = new_cover(cube.num_vars);

    if (cdist0(a, b)) {
    set_diff(d, a, b);
    for(var = 0; var < cube.num_vars; var++) {
        if (! setp_empty(set_and(temp, d, cube.var_mask[var]))) {
        set_diff(temp1, a, cube.var_mask[var]);
        set_or(GETSET(r, r->count++), temp, temp1);
        }
    }
    } else {
    r = sf_addset(r, a);
    }
    return r;
}

pcover make_disjoint(A)
pcover A;
{
    pcover R, new;
    register pset last, p;

    R = new_cover(0);
    foreach_set(A, last, p) {
    new = cb_dsharp(p, R);
    R = sf_append(R, new);
    }
    return R;
}


/* cv_dsharp -- disjoint-sharp product between two covers */
pcover cv_dsharp(A, B)
pcover A, B;
{
    register pcube last, p;
    pcover T;

    T = new_cover(0);
    foreach_set(A, last, p) {
    T = sf_union(T, cb_dsharp(p, B));
    }
    return T;
}


/* cb1_dsharp -- disjoint-sharp product between a cover and a cube */
pcover cb1_dsharp(T, c)
pcover T;
pcube c;
{
    pcube last, p;
    pcover R;

    R = new_cover(T->count);
    foreach_set(T, last, p) {
    R = sf_union(R, dsharp(p, c));
    }
    return R;
}


/* cb_dsharp -- disjoint-sharp product between a cube and a cover */
pcover cb_dsharp(c, T)
pcube c;
pcover T;
{
    pcube last, p;
    pcover Y, Y1;

    if (T->count == 0) {
    Y = sf_addset(new_cover(1), c);
    } else {
    Y = new_cover(T->count);
    set_copy(GETSET(Y,Y->count++), c);
    foreach_set(T, last, p) {
        Y1 = cb1_dsharp(Y, p);
        free_cover(Y);
        Y = Y1;
    }
    }
    return Y;
}


/* dsharp -- form the disjoint-sharp product between two cubes */
pcover dsharp(a, b)
pcube a, b;
{
    register pcube mask, diff, and, temp, temp1 = cube.temp[0];
    int var;
    pcover r;

    r = new_cover(cube.num_vars);

    if (cdist0(a, b)) {
    diff = set_diff(new_cube(), a, b);
    and = set_and(new_cube(), a, b);
    mask = new_cube();
    for(var = 0; var < cube.num_vars; var++) {
        /* check if position var of "a and not b" is not empty */
        if (! setp_disjoint(diff, cube.var_mask[var])) {

        /* coordinate var equals the difference between a and b */
        temp = GETSET(r, r->count++);
        (void) set_and(temp, diff, cube.var_mask[var]);

        /* coordinates 0 ... var-1 equal the intersection */
        INLINEset_and(temp1, and, mask);
        INLINEset_or(temp, temp, temp1);

        /* coordinates var+1 .. cube.num_vars equal a */
        set_or(mask, mask, cube.var_mask[var]);
        INLINEset_diff(temp1, a, mask);
        INLINEset_or(temp, temp, temp1);
        }
    }
    free_cube(diff);
    free_cube(and);
    free_cube(mask);
    } else {
    r = sf_addset(r, a);
    }
    return r;
}

/* cv_intersect -- form the intersection of two covers */

#define MAGIC 500               /* save 500 cubes before containment */

pcover cv_intersect(A, B)
pcover A, B;
{
    register pcube pi, pj, lasti, lastj, pt;
    pcover T, Tsave = NULL;

    /* How large should each temporary result cover be ? */
    T = new_cover(MAGIC);
    pt = T->data;

    /* Form pairwise intersection of each cube of A with each cube of B */
    foreach_set(A, lasti, pi) {
    foreach_set(B, lastj, pj) {
        if (cdist0(pi, pj)) {
        (void) set_and(pt, pi, pj);
        if (++T->count >= T->capacity) {
            if (Tsave == NULL)
            Tsave = sf_contain(T);
            else
            Tsave = sf_union(Tsave, sf_contain(T));
            T = new_cover(MAGIC);
            pt = T->data;
        } else
            pt += T->wsize;
        }
    }
    }


    if (Tsave == NULL)
    Tsave = sf_contain(T);
    else
    Tsave = sf_union(Tsave, sf_contain(T));
    return Tsave;
}
ABC_NAMESPACE_IMPL_END

