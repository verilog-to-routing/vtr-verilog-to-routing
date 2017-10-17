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


static bool primes_consensus_special_cases();
static pcover primes_consensus_merge();
static pcover and_with_cofactor(); 


/* primes_consensus -- generate primes using consensus */
pcover primes_consensus(T)
pcube *T;			/* T will be disposed of */
{
    register pcube cl, cr;
    register int best;
    pcover Tnew, Tl, Tr;

    if (primes_consensus_special_cases(T, &Tnew) == MAYBE) {
	cl = new_cube();
	cr = new_cube();
	best = binate_split_select(T, cl, cr, COMPL);

	Tl = primes_consensus(scofactor(T, cl, best));
	Tr = primes_consensus(scofactor(T, cr, best));
	Tnew = primes_consensus_merge(Tl, Tr, cl, cr);

	free_cube(cl);
	free_cube(cr);
	free_cubelist(T);
    }

    return Tnew;
}

static bool 
primes_consensus_special_cases(T, Tnew)
pcube *T;			/* will be disposed if answer is determined */
pcover *Tnew;			/* returned only if answer determined */
{
    register pcube *T1, p, ceil, cof=T[0];
    pcube last;
    pcover A;

    /* Check for no cubes in the cover */
    if (T[2] == NULL) {
	*Tnew = new_cover(0);
	free_cubelist(T);
	return TRUE;
    }

    /* Check for only a single cube in the cover */
    if (T[3] == NULL) {
	*Tnew = sf_addset(new_cover(1), set_or(cof, cof, T[2]));
	free_cubelist(T);
	return TRUE;
    }

    /* Check for a row of all 1's (implies function is a tautology) */
    for(T1 = T+2; (p = *T1++) != NULL; ) {
	if (full_row(p, cof)) {
	    *Tnew = sf_addset(new_cover(1), cube.fullset);
	    free_cubelist(T);
	    return TRUE;
	}
    }

    /* Check for a column of all 0's which can be factored out */
    ceil = set_save(cof);
    for(T1 = T+2; (p = *T1++) != NULL; ) {
	INLINEset_or(ceil, ceil, p);
    }
    if (! setp_equal(ceil, cube.fullset)) {
	p = new_cube();
	(void) set_diff(p, cube.fullset, ceil);
	(void) set_or(cof, cof, p);
	free_cube(p);

	A = primes_consensus(T);
	foreach_set(A, last, p) {
	    INLINEset_and(p, p, ceil);
	}
	*Tnew = A;
	set_free(ceil);
	return TRUE;
    }
    set_free(ceil);

    /* Collect column counts, determine unate variables, etc. */
    massive_count(T);

    /* If single active variable not factored out above, then tautology ! */
    if (cdata.vars_active == 1) {
	*Tnew = sf_addset(new_cover(1), cube.fullset);
	free_cubelist(T);
	return TRUE;

    /* Check for unate cover */
    } else if (cdata.vars_unate == cdata.vars_active) {
	A = cubeunlist(T);
	*Tnew = sf_contain(A);
	free_cubelist(T);
	return TRUE;

    /* Not much we can do about it */
    } else {
	return MAYBE;
    }
}

static pcover 
primes_consensus_merge(Tl, Tr, cl, cr)
pcover Tl, Tr;
pcube cl, cr;
{
    register pcube pl, pr, lastl, lastr, pt;
    pcover T, Tsave;

    Tl = and_with_cofactor(Tl, cl);
    Tr = and_with_cofactor(Tr, cr);

    T = sf_new(500, Tl->sf_size);
    pt = T->data;
    Tsave = sf_contain(sf_join(Tl, Tr));

    foreach_set(Tl, lastl, pl) {
	foreach_set(Tr, lastr, pr) {
	    if (cdist01(pl, pr) == 1) {
		consensus(pt, pl, pr);
		if (++T->count >= T->capacity) {
		    Tsave = sf_union(Tsave, sf_contain(T));
		    T = sf_new(500, Tl->sf_size);
		    pt = T->data;
		} else {
		    pt += T->wsize;
		}
	    }
	}
    }
    free_cover(Tl);
    free_cover(Tr);

    Tsave = sf_union(Tsave, sf_contain(T));
    return Tsave;
}


static pcover
and_with_cofactor(A, cof)
pset_family A;
register pset cof;
{
    register pset last, p;

    foreach_set(A, last, p) {
	INLINEset_and(p, p, cof);
	if (cdist(p, cube.fullset) > 0) {
	    RESET(p, ACTIVE);
	} else {
	    SET(p, ACTIVE);
	}
    }
    return sf_inactive(A);
}
ABC_NAMESPACE_IMPL_END

