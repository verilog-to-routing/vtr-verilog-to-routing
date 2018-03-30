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
    setc.c -- massive bit-hacking for performing special "cube"-type
    operations on a set

    The basic trick used for binary valued variables is the following:

    If a[w] and b[w] contain a full word of binary variables, then:

     1) to get the full word of their intersection, we use

        x = a[w] & b[w];


     2) to see if the intersection is null in any variables, we examine

        x = ~(x | x >> 1) & DISJOINT;

    this will have a single 1 in each binary variable for which
    the intersection is null.  In particular, if this is zero,
    then there are no disjoint variables; or, if this is nonzero,
    then there is at least one disjoint variable.  A "count_ones"
    over x will tell in how many variables they have an null
    intersection.


     3) to get a mask which selects the disjoint variables, we use

        (x | x << 1)

    this provides a selector which can be used to see where
    they have an null intersection


    cdist       return distance between two cubes
    cdist0      return true if two cubes are distance 0 apart
    cdist01     return distance, or 2 if distance exceeds 1
    consensus   compute consensus of two cubes distance 1 apart
    force_lower expand hack (for now), related to consensus
*/

#include "espresso.h"

ABC_NAMESPACE_IMPL_START


/* see if the cube has a full row of 1's (with respect to cof) */
bool full_row(p, cof)
IN register pcube p, cof;
{
    register int i = LOOP(p);
    do if ((p[i] | cof[i]) != cube.fullset[i]) return FALSE; while (--i > 0);
    return TRUE;
}

/*
    cdist0 -- return TRUE if a and b are distance 0 apart
*/

bool cdist0(a, b)
register pcube a, b;
{
 {  /* Check binary variables */
    register int w, last; register unsigned int x;
    if ((last = cube.inword) != -1) {

    /* Check the partial word of binary variables */
    x = a[last] & b[last];
    if (~(x | x >> 1) & cube.inmask)
        return FALSE;               /* disjoint in some variable */

    /* Check the full words of binary variables */
    for(w = 1; w < last; w++) {
        x = a[w] & b[w];
        if (~(x | x >> 1) & DISJOINT)
        return FALSE;           /* disjoint in some variable */
    }
    }
 }

 {  /* Check the multiple-valued variables */
    register int w, var, last; register pcube mask;
    for(var = cube.num_binary_vars; var < cube.num_vars; var++) {
    mask = cube.var_mask[var]; last = cube.last_word[var];
    for(w = cube.first_word[var]; w <= last; w++)
        if (a[w] & b[w] & mask[w])
        goto nextvar;
    return FALSE;           /* disjoint in this variable */
    nextvar: ;
    }
 }
    return TRUE;
}

/*
    cdist01 -- return the "distance" between two cubes (defined as the
    number of null variables in their intersection).  If the distance
    exceeds 1, the value 2 is returned.
*/

int cdist01(a, b)
register pset a, b;
{
    int dist = 0;

 {  /* Check binary variables */
    register int w, last; register unsigned int x;
    if ((last = cube.inword) != -1) {

    /* Check the partial word of binary variables */
    x = a[last] & b[last];
    if ((x = ~ (x | x >> 1) & cube.inmask))
        if ((dist = count_ones(x)) > 1)
        return 2;

    /* Check the full words of binary variables */
    for(w = 1; w < last; w++) {
        x = a[w] & b[w];
        if ((x = ~ (x | x >> 1) & DISJOINT))
        if (dist == 1 || (dist += count_ones(x)) > 1)
            return 2;
    }
    }
 }

 {  /* Check the multiple-valued variables */
    register int w, var, last; register pcube mask;
    for(var = cube.num_binary_vars; var < cube.num_vars; var++) {
    mask = cube.var_mask[var]; last = cube.last_word[var];
    for(w = cube.first_word[var]; w <= last; w++)
        if (a[w] & b[w] & mask[w])
        goto nextvar;
    if (++dist > 1)
        return 2;
    nextvar: ;
    }
 }
    return dist;
}

/*
    cdist -- return the "distance" between two cubes (defined as the
    number of null variables in their intersection).
*/

int cdist(a, b)
register pset a, b;
{
    int dist = 0;

 {  /* Check binary variables */
    register int w, last; register unsigned int x;
    if ((last = cube.inword) != -1) {

    /* Check the partial word of binary variables */
    x = a[last] & b[last];
    if ((x = ~ (x | x >> 1) & cube.inmask))
        dist = count_ones(x);

    /* Check the full words of binary variables */
    for(w = 1; w < last; w++) {
        x = a[w] & b[w];
        if ((x = ~ (x | x >> 1) & DISJOINT))
        dist += count_ones(x);
    }
    }
 }

 {  /* Check the multiple-valued variables */
    register int w, var, last; register pcube mask;
    for(var = cube.num_binary_vars; var < cube.num_vars; var++) {
    mask = cube.var_mask[var]; last = cube.last_word[var];
    for(w = cube.first_word[var]; w <= last; w++)
        if (a[w] & b[w] & mask[w])
        goto nextvar;
    dist++;
    nextvar: ;
    }
 }
    return dist;
}

/*
    force_lower -- Determine which variables of a do not intersect b.
*/

pset force_lower(xlower, a, b)
INOUT pset xlower;
IN register pset a, b;
{

 {  /* Check binary variables (if any) */
    register int w, last; register unsigned int x;
    if ((last = cube.inword) != -1) {

    /* Check the partial word of binary variables */
    x = a[last] & b[last];
    if ((x = ~(x | x >> 1) & cube.inmask))
        xlower[last] |= (x | (x << 1)) & a[last];

    /* Check the full words of binary variables */
    for(w = 1; w < last; w++) {
        x = a[w] & b[w];
        if ((x = ~(x | x >> 1) & DISJOINT))
        xlower[w] |= (x | (x << 1)) & a[w];
    }
    }
 }

 {  /* Check the multiple-valued variables */
    register int w, var, last; register pcube mask;
    for(var = cube.num_binary_vars; var < cube.num_vars; var++) {
    mask = cube.var_mask[var]; last = cube.last_word[var];
    for(w = cube.first_word[var]; w <= last; w++)
        if (a[w] & b[w] & mask[w])
        goto nextvar;
    for(w = cube.first_word[var]; w <= last; w++)
        xlower[w] |= a[w] & mask[w];
    nextvar: ;
    }
 }
    return xlower;
}

/*
    consensus -- multiple-valued consensus

    Although this looks very messy, the idea is to compute for r the
    "and" of the cubes a and b for each variable, unless the "and" is
    null in a variable, in which case the "or" of a and b is computed
    for this variable.

    Because we don't check how many variables are null in the
    intersection of a and b, the returned value for r really only
    represents the consensus when a and b are distance 1 apart.
*/

void consensus(r, a, b)
INOUT pcube r;
IN register pcube a, b;
{
    INLINEset_clear(r, cube.size);

 {  /* Check binary variables (if any) */
    register int w, last; register unsigned int x;
    if ((last = cube.inword) != -1) {

    /* Check the partial word of binary variables */
    r[last] = x = a[last] & b[last];
    if ((x = ~(x | x >> 1) & cube.inmask))
        r[last] |= (x | (x << 1)) & (a[last] | b[last]);

    /* Check the full words of binary variables */
    for(w = 1; w < last; w++) {
        r[w] = x = a[w] & b[w];
        if ((x = ~(x | x >> 1) & DISJOINT))
        r[w] |= (x | (x << 1)) & (a[w] | b[w]);
    }
    }
 }


 {  /* Check the multiple-valued variables */
    bool empty; int var; unsigned int x;
    register int w, last; register pcube mask;
    for(var = cube.num_binary_vars; var < cube.num_vars; var++) {
    mask = cube.var_mask[var];
    last = cube.last_word[var];
    empty = TRUE;
    for(w = cube.first_word[var]; w <= last; w++)
        if ((x = a[w] & b[w] & mask[w]))
        empty = FALSE, r[w] |= x;
    if (empty)
        for(w = cube.first_word[var]; w <= last; w++)
        r[w] |= mask[w] & (a[w] | b[w]);
    }
 }
}

/*
    cactive -- return the index of the single active variable in
    the cube, or return -1 if there are none or more than 2.
*/

int cactive(a)
register pcube a;
{
    int active = -1, dist = 0, bit_index();

 {  /* Check binary variables */
    register int w, last;
    register unsigned int x;
    if ((last = cube.inword) != -1) {

    /* Check the partial word of binary variables */
    x = a[last];
    if ((x = ~ (x & x >> 1) & cube.inmask)) {
        if ((dist = count_ones(x)) > 1)
        return -1;        /* more than 2 active variables */
        active = (last-1)*(BPI/2) + bit_index(x) / 2;
    }

    /* Check the full words of binary variables */
    for(w = 1; w < last; w++) {
        x = a[w];
        if ((x = ~ (x & x >> 1) & DISJOINT)) {
        if ((dist += count_ones(x)) > 1)
            return -1;        /* more than 2 active variables */
        active = (w-1)*(BPI/2) + bit_index(x) / 2;
        }
    }
    }
 }

 {  /* Check the multiple-valued variables */
    register int w, var, last;
    register pcube mask;
    for(var = cube.num_binary_vars; var < cube.num_vars; var++) {
    mask = cube.var_mask[var];
    last = cube.last_word[var];
    for(w = cube.first_word[var]; w <= last; w++)
        if (mask[w] & ~ a[w]) {
        if (++dist > 1)
            return -1;
        active = var;
        break;
        }
    }
 }
 return active;
}

/*
    ccommon -- return TRUE if a and b are share "active" variables
    active variables include variables that are empty;
*/

bool ccommon(a, b, cof)
register pcube a, b, cof;
{
 {  /* Check binary variables */
    int last;
    register int w;
    register unsigned int x, y;
    if ((last = cube.inword) != -1) {

    /* Check the partial word of binary variables */
    x = a[last] | cof[last];
    y = b[last] | cof[last];
    if (~(x & x>>1) & ~(y & y>>1) & cube.inmask)
        return TRUE;

    /* Check the full words of binary variables */
    for(w = 1; w < last; w++) {
        x = a[w] | cof[w];
        y = b[w] | cof[w];
        if (~(x & x>>1) & ~(y & y>>1) & DISJOINT)
        return TRUE;
    }
    }
 }

 {  /* Check the multiple-valued variables */
    int var;
    register int w, last;
    register pcube mask;
    for(var = cube.num_binary_vars; var < cube.num_vars; var++) {
    mask = cube.var_mask[var]; last = cube.last_word[var];
    /* Check for some part missing from a */
    for(w = cube.first_word[var]; w <= last; w++)
        if (mask[w] & ~a[w] & ~cof[w]) {

        /* If so, check for some part missing from b */
        for(w = cube.first_word[var]; w <= last; w++)
            if (mask[w] & ~b[w] & ~cof[w])
            return TRUE;            /* both active */
        break;
        }
    }
 }
    return FALSE;
}

/*
    These routines compare two sets (cubes) for the qsort() routine and
    return:

    -1 if set a is to precede set b
     0 if set a and set b are equal
     1 if set a is to follow set b

    Usually the SIZE field of the set is assumed to contain the size
    of the set (which will save recomputing the set size during the
    sort).  For distance-1 merging, the global variable cube.temp[0] is
    a mask which mask's-out the merging variable.
*/

/* descend -- comparison for descending sort on set size */
int descend(a, b)
pset *a, *b;
{
    register pset a1 = *a, b1 = *b;
    if (SIZE(a1) > SIZE(b1)) return -1;
    else if (SIZE(a1) < SIZE(b1)) return 1;
    else {
    register int i = LOOP(a1);
    do
        if (a1[i] > b1[i]) return -1; else if (a1[i] < b1[i]) return 1;
    while (--i > 0);
    }
    return 0;
}

/* ascend -- comparison for ascending sort on set size */
int ascend(a, b)
pset *a, *b;
{
    register pset a1 = *a, b1 = *b;
    if (SIZE(a1) > SIZE(b1)) return 1;
    else if (SIZE(a1) < SIZE(b1)) return -1;
    else {
    register int i = LOOP(a1);
    do
        if (a1[i] > b1[i]) return 1; else if (a1[i] < b1[i]) return -1;
    while (--i > 0);
    }
    return 0;
}


/* lex_order -- comparison for "lexical" ordering of cubes */
int lex_order(a, b)
pset *a, *b;
{
    register pset a1 = *a, b1 = *b;
    register int i = LOOP(a1);
    do
    if (a1[i] > b1[i]) return -1; else if (a1[i] < b1[i]) return 1;
    while (--i > 0);
    return 0;
}


/* d1_order -- comparison for distance-1 merge routine */
int d1_order(a, b)
pset *a, *b;
{
    register pset a1 = *a, b1 = *b, c1 = cube.temp[0];
    register int i = LOOP(a1);
    register unsigned int x1, x2;
    do
    if ((x1 = a1[i] | c1[i]) > (x2 = b1[i] | c1[i])) return -1;
    else if (x1 < x2) return 1;
    while (--i > 0);
    return 0;
}


/* desc1 -- comparison (without indirection) for descending sort */
/* also has effect of handling NULL pointers,and a NULL pointer has smallest
order */
int desc1(a, b)
register pset a, b;
{
    if (a == (pset) NULL)
    return (b == (pset) NULL) ? 0 : 1;
    else if (b == (pset) NULL)
    return -1;
    if (SIZE(a) > SIZE(b)) return -1;
    else if (SIZE(a) < SIZE(b)) return 1;
    else {
    register int i = LOOP(a);
    do
        if (a[i] > b[i]) return -1; else if (a[i] < b[i]) return 1;
    while (--i > 0);
    }
    return 0;
}
ABC_NAMESPACE_IMPL_END

