/**CFile***********************************************************************

  FileName    [cuddAndAbs.c]

  PackageName [cudd]

  Synopsis    [Combined AND and existential abstraction for BDDs]

  Description [External procedures included in this module:
		<ul>
		<li> Cudd_bddAndAbstract()
		</ul>
	    Internal procedures included in this module:
		<ul>
		<li> cuddBddAndAbstractRecur()
		</ul>]

  Author      [Fabio Somenzi]

  Copyright   [This file was created at the University of Colorado at
  Boulder.  The University of Colorado at Boulder makes no warranty
  about the suitability of this software for any purpose.  It is
  presented on an AS IS basis.]

******************************************************************************/

#include "util_hack.h"
#include "cuddInt.h"


/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

#ifndef lint
static char rcsid[] DD_UNUSED = "$Id: cuddAndAbs.c,v 1.1.1.1 2003/02/24 22:23:51 wjiang Exp $";
#endif


/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/


/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis [Takes the AND of two BDDs and simultaneously abstracts the
  variables in cube.]

  Description [Takes the AND of two BDDs and simultaneously abstracts
  the variables in cube. The variables are existentially abstracted.
  Returns a pointer to the result is successful; NULL otherwise.
  Cudd_bddAndAbstract implements the semiring matrix multiplication
  algorithm for the boolean semiring.]

  SideEffects [None]

  SeeAlso     [Cudd_addMatrixMultiply Cudd_addTriangle Cudd_bddAnd]

******************************************************************************/
DdNode *
Cudd_bddAndAbstract(
  DdManager * manager,
  DdNode * f,
  DdNode * g,
  DdNode * cube)
{
    DdNode *res;

    do {
	manager->reordered = 0;
	res = cuddBddAndAbstractRecur(manager, f, g, cube);
    } while (manager->reordered == 1);
    return(res);

} /* end of Cudd_bddAndAbstract */


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis [Takes the AND of two BDDs and simultaneously abstracts the
  variables in cube.]

  Description [Takes the AND of two BDDs and simultaneously abstracts
  the variables in cube. The variables are existentially abstracted.
  Returns a pointer to the result is successful; NULL otherwise.]

  SideEffects [None]

  SeeAlso     [Cudd_bddAndAbstract]

******************************************************************************/
DdNode *
cuddBddAndAbstractRecur(
  DdManager * manager,
  DdNode * f,
  DdNode * g,
  DdNode * cube)
{
    DdNode *F, *ft, *fe, *G, *gt, *ge;
    DdNode *one, *zero, *r, *t, *e;
    unsigned int topf, topg, topcube, top, index;

    statLine(manager);
    one = DD_ONE(manager);
    zero = Cudd_Not(one);

    /* Terminal cases. */
    if (f == zero || g == zero || f == Cudd_Not(g)) return(zero);
    if (f == one && g == one)	return(one);

    if (cube == one) {
	return(cuddBddAndRecur(manager, f, g));
    }
    if (f == one || f == g) {
	return(cuddBddExistAbstractRecur(manager, g, cube));
    }
    if (g == one) {
	return(cuddBddExistAbstractRecur(manager, f, cube));
    }
    /* At this point f, g, and cube are not constant. */

    if (f > g) { /* Try to increase cache efficiency. */
	DdNode *tmp = f;
	f = g;
	g = tmp;
    }

    /* Here we can skip the use of cuddI, because the operands are known
    ** to be non-constant.
    */
    F = Cudd_Regular(f);
    G = Cudd_Regular(g);
    topf = manager->perm[F->index];
    topg = manager->perm[G->index];
    top = ddMin(topf, topg);
    topcube = manager->perm[cube->index];

    while (topcube < top) {
	cube = cuddT(cube);
	if (cube == one) {
	    return(cuddBddAndRecur(manager, f, g));
	}
	topcube = manager->perm[cube->index];
    }
    /* Now, topcube >= top. */

    /* Check cache. */
    if (F->ref != 1 || G->ref != 1) {
	r = cuddCacheLookup(manager, DD_BDD_AND_ABSTRACT_TAG, f, g, cube);
	if (r != NULL) {
	    return(r);
	}
    }

    if (topf == top) {
	index = F->index;
	ft = cuddT(F);
	fe = cuddE(F);
	if (Cudd_IsComplement(f)) {
	    ft = Cudd_Not(ft);
	    fe = Cudd_Not(fe);
	}
    } else {
	index = G->index;
	ft = fe = f;
    }

    if (topg == top) {
	gt = cuddT(G);
	ge = cuddE(G);
	if (Cudd_IsComplement(g)) {
	    gt = Cudd_Not(gt);
	    ge = Cudd_Not(ge);
	}
    } else {
	gt = ge = g;
    }

    if (topcube == top) {	/* quantify */
	DdNode *Cube = cuddT(cube);
	t = cuddBddAndAbstractRecur(manager, ft, gt, Cube);
	if (t == NULL) return(NULL);
	/* Special case: 1 OR anything = 1. Hence, no need to compute
	** the else branch if t is 1. Likewise t + t * anything == t.
	** Notice that t == fe implies that fe does not depend on the
	** variables in Cube. Likewise for t == ge.
	*/
	if (t == one || t == fe || t == ge) {
	    if (F->ref != 1 || G->ref != 1)
		cuddCacheInsert(manager, DD_BDD_AND_ABSTRACT_TAG,
				f, g, cube, t);
	    return(t);
	}
	cuddRef(t);
	/* Special case: t + !t * anything == t + anything. */
	if (t == Cudd_Not(fe)) {
	    e = cuddBddExistAbstractRecur(manager, ge, Cube);
	} else if (t == Cudd_Not(ge)) {
	    e = cuddBddExistAbstractRecur(manager, fe, Cube);
	} else {
	    e = cuddBddAndAbstractRecur(manager, fe, ge, Cube);
	}
	if (e == NULL) {
	    Cudd_IterDerefBdd(manager, t);
	    return(NULL);
	}
	if (t == e) {
	    r = t;
	    cuddDeref(t);
	} else {
	    cuddRef(e);
	    r = cuddBddAndRecur(manager, Cudd_Not(t), Cudd_Not(e));
	    if (r == NULL) {
		Cudd_IterDerefBdd(manager, t);
		Cudd_IterDerefBdd(manager, e);
		return(NULL);
	    }
	    r = Cudd_Not(r);
	    cuddRef(r);
	    Cudd_DelayedDerefBdd(manager, t);
	    Cudd_DelayedDerefBdd(manager, e);
	    cuddDeref(r);
	}
    } else {
	t = cuddBddAndAbstractRecur(manager, ft, gt, cube);
	if (t == NULL) return(NULL);
	cuddRef(t);
	e = cuddBddAndAbstractRecur(manager, fe, ge, cube);
	if (e == NULL) {
	    Cudd_IterDerefBdd(manager, t);
	    return(NULL);
	}
	if (t == e) {
	    r = t;
	    cuddDeref(t);
	} else {
	    cuddRef(e);
	    if (Cudd_IsComplement(t)) {
		r = cuddUniqueInter(manager, (int) index,
				    Cudd_Not(t), Cudd_Not(e));
		if (r == NULL) {
		    Cudd_IterDerefBdd(manager, t);
		    Cudd_IterDerefBdd(manager, e);
		    return(NULL);
		}
		r = Cudd_Not(r);
	    } else {
		r = cuddUniqueInter(manager,(int)index,t,e);
		if (r == NULL) {
		    Cudd_IterDerefBdd(manager, t);
		    Cudd_IterDerefBdd(manager, e);
		    return(NULL);
		}
	    }
	    cuddDeref(e);
	    cuddDeref(t);
	}
    }

    if (F->ref != 1 || G->ref != 1)
	cuddCacheInsert(manager, DD_BDD_AND_ABSTRACT_TAG, f, g, cube, r);
    return (r);

} /* end of cuddBddAndAbstractRecur */


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

