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
    module: reduce.c
    purpose: Perform the Espresso-II reduction step

    Reduction is a technique used to explore larger regions of the
    optimization space.  We replace each cube of F with a smaller
    cube while still maintaining a cover of the same logic function.
*/

#include "espresso.h"

ABC_NAMESPACE_IMPL_START


static bool toggle = TRUE;


/*
    reduce -- replace each cube in F with its reduction

    The reduction of a cube is the smallest cube contained in the cube
    which can replace the cube in the original cover without changing
    the cover.  This is equivalent to the super cube of all of the
    essential points in the cube.  This can be computed directly.

    The problem is that the order in which the cubes are reduced can
    greatly affect the final result.  We alternate between two ordering
    strategies:

	(1) Order the cubes in ascending order of distance from the
	largest cube breaking ties by ordering cubes of equal distance
	in descending order of size (sort_reduce)

	(2) Order the cubes in descending order of the inner-product of
	the cube and the column sums (mini_sort)

    The real workhorse of this section is the routine SCCC which is
    used to find the Smallest Cube Containing the Complement of a cover.
    Reduction as proposed by Espresso-II takes a cube and computes its
    maximal reduction as the intersection between the cube and the
    smallest cube containing the complement of (F u D - {c}) cofactored
    against c.

    As usual, the unate-recursive paradigm is used to compute SCCC.
    The SCCC of a unate cover is trivial to compute, and thus we perform
    Shannon Cofactor expansion attempting to drive the cover to be unate
    as fast as possible.
*/

pcover reduce(F, D)
INOUT pcover F;
IN pcover D;
{
    register pcube last, p, cunder, *FD;

    /* Order the cubes */
    if (use_random_order)
	F = random_order(F);
    else {
	F = toggle ? sort_reduce(F) : mini_sort(F, descend);
	toggle = ! toggle;
    }

    /* Try to reduce each cube */
    FD = cube2list(F, D);
    foreach_set(F, last, p) {
	cunder = reduce_cube(FD, p);		/* reduce the cube */
	if (setp_equal(cunder, p)) {            /* see if it actually did */
	    SET(p, ACTIVE);	/* cube remains active */
	    SET(p, PRIME);	/* cube remains prime ? */
	} else {
	    if (debug & REDUCE) {
		printf("REDUCE: %s to %s %s\n",
		    pc1(p), pc2(cunder), print_time(ptime()));
	    }
	    set_copy(p, cunder);                /* save reduced version */
	    RESET(p, PRIME);                    /* cube is no longer prime */
	    if (setp_empty(cunder))
		RESET(p, ACTIVE);               /* if null, kill the cube */
	    else
		SET(p, ACTIVE);                 /* cube is active */
	}
	free_cube(cunder);
    }
    free_cubelist(FD);

    /* Delete any cubes of F which reduced to the empty cube */
    return sf_inactive(F);
}

/* reduce_cube -- find the maximal reduction of a cube */
pcube reduce_cube(FD, p)
IN pcube *FD, p;
{
    pcube cunder;

    cunder = sccc(cofactor(FD, p));
    return set_and(cunder, cunder, p);
}


/* sccc -- find Smallest Cube Containing the Complement of a cover */
pcube sccc(T)
INOUT pcube *T;         /* T will be disposed of */
{
    pcube r;
    register pcube cl, cr;
    register int best;
    static int sccc_level = 0;

    if (debug & REDUCE1) {
	debug_print(T, "SCCC", sccc_level++);
    }

    if (sccc_special_cases(T, &r) == MAYBE) {
	cl = new_cube();
	cr = new_cube();
	best = binate_split_select(T, cl, cr, REDUCE1);
	r = sccc_merge(sccc(scofactor(T, cl, best)),
		       sccc(scofactor(T, cr, best)), cl, cr);
	free_cubelist(T);
    }

    if (debug & REDUCE1)
	printf("SCCC[%d]: result is %s\n", --sccc_level, pc1(r));
    return r;
}


pcube sccc_merge(left, right, cl, cr)
INOUT register pcube left, right;       /* will be disposed of ... */
INOUT register pcube cl, cr;            /* will be disposed of ... */
{
    INLINEset_and(left, left, cl);
    INLINEset_and(right, right, cr);
    INLINEset_or(left, left, right);
    free_cube(right);
    free_cube(cl);
    free_cube(cr);
    return left;
}


/*
    sccc_cube -- find the smallest cube containing the complement of a cube

    By DeMorgan's law and the fact that the smallest cube containing a
    cover is the "or" of the positional cubes, it is simple to see that
    the SCCC is the universe if the cube has more than two active
    variables.  If there is only a single active variable, then the
    SCCC is merely the bitwise complement of the cube in that
    variable.  A last special case is no active variables, in which
    case the SCCC is empty.

    This is "anded" with the incoming cube result.
*/
pcube sccc_cube(result, p)
register pcube result, p;
{
    register pcube temp=cube.temp[0], mask;
    int var;

    if ((var = cactive(p)) >= 0) {
	mask = cube.var_mask[var];
	INLINEset_xor(temp, p, mask);
	INLINEset_and(result, result, temp);
    }
    return result;
}

/*
 *   sccc_special_cases -- check the special cases for sccc
 */

bool sccc_special_cases(T, result)
INOUT pcube *T;                 /* will be disposed if answer is determined */
OUT pcube *result;              /* returned only if answer determined */
{
    register pcube *T1, p, temp = cube.temp[1], ceil, cof = T[0];
    pcube *A, *B;

    /* empty cover => complement is universe => SCCC is universe */
    if (T[2] == NULL) {
	*result = set_save(cube.fullset);
	free_cubelist(T);
	return TRUE;
    }

    /* row of 1's => complement is empty => SCCC is empty */
    for(T1 = T+2; (p = *T1++) != NULL; ) {
	if (full_row(p, cof)) {
	    *result = new_cube();
	    free_cubelist(T);
	    return TRUE;
	}
    }

    /* Collect column counts, determine unate variables, etc. */
    massive_count(T);

    /* If cover is unate (or single cube), apply simple rules to find SCCCU */
    if (cdata.vars_unate == cdata.vars_active || T[3] == NULL) {
	*result = set_save(cube.fullset);
	for(T1 = T+2; (p = *T1++) != NULL; ) {
	    (void) sccc_cube(*result, set_or(temp, p, cof));
	}
	free_cubelist(T);
	return TRUE;
    }

    /* Check for column of 0's (which can be easily factored( */
    ceil = set_save(cof);
    for(T1 = T+2; (p = *T1++) != NULL; ) {
	INLINEset_or(ceil, ceil, p);
    }
    if (! setp_equal(ceil, cube.fullset)) {
	*result = sccc_cube(set_save(cube.fullset), ceil);
	if (setp_equal(*result, cube.fullset)) {
	    free_cube(ceil);
	} else {
	    *result = sccc_merge(sccc(cofactor(T,ceil)),
			 set_save(cube.fullset), ceil, *result);
	}
	free_cubelist(T);
	return TRUE;
    }
    free_cube(ceil);

    /* Single active column at this point => tautology => SCCC is empty */
    if (cdata.vars_active == 1) {
	*result = new_cube();
	free_cubelist(T);
	return TRUE;
    }

    /* Check for components */
    if (cdata.var_zeros[cdata.best] < CUBELISTSIZE(T)/2) {
	if (cubelist_partition(T, &A, &B, debug & REDUCE1) == 0) {
	    return MAYBE;
	} else {
	    free_cubelist(T);
	    *result = sccc(A);
	    ceil = sccc(B);
	    (void) set_and(*result, *result, ceil);
	    set_free(ceil);
	    return TRUE;
	}
    }

    /* Not much we can do about it */
    return MAYBE;
}
ABC_NAMESPACE_IMPL_END

