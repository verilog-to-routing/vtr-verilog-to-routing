/**CFile***********************************************************************

  FileName    [cuddClip.c]

  PackageName [cudd]

  Synopsis    [Clipping functions.]

  Description [External procedures included in this module:
		<ul>
		<li> Cudd_bddClippingAnd()
		<li> Cudd_bddClippingAndAbstract()
		</ul>
       Internal procedures included in this module:
		<ul>
		<li> cuddBddClippingAnd()
		<li> cuddBddClippingAndAbstract()
		</ul>
       Static procedures included in this module:
		<ul>
		<li> cuddBddClippingAndRecur()
		<li> cuddBddClipAndAbsRecur()
		</ul>

  SeeAlso     []

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
static char rcsid[] DD_UNUSED = "$Id: cuddClip.c,v 1.1.1.1 2003/02/24 22:23:51 wjiang Exp $";
#endif

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static DdNode * cuddBddClippingAndRecur ARGS((DdManager *manager, DdNode *f, DdNode *g, int distance, int direction));
static DdNode * cuddBddClipAndAbsRecur ARGS((DdManager *manager, DdNode *f, DdNode *g, DdNode *cube, int distance, int direction));

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Approximates the conjunction of two BDDs f and g.]

  Description [Approximates the conjunction of two BDDs f and g. Returns a
  pointer to the resulting BDD if successful; NULL if the intermediate
  result blows up.]

  SideEffects [None]

  SeeAlso     [Cudd_bddAnd]

******************************************************************************/
DdNode *
Cudd_bddClippingAnd(
  DdManager * dd /* manager */,
  DdNode * f /* first conjunct */,
  DdNode * g /* second conjunct */,
  int  maxDepth /* maximum recursion depth */,
  int  direction /* under (0) or over (1) approximation */)
{
    DdNode *res;

    do {
	dd->reordered = 0;
	res = cuddBddClippingAnd(dd,f,g,maxDepth,direction);
    } while (dd->reordered == 1);
    return(res);

} /* end of Cudd_bddClippingAnd */


/**Function********************************************************************

  Synopsis    [Approximates the conjunction of two BDDs f and g and
  simultaneously abstracts the variables in cube.]

  Description [Approximates the conjunction of two BDDs f and g and
  simultaneously abstracts the variables in cube. The variables are
  existentially abstracted. Returns a pointer to the resulting BDD if
  successful; NULL if the intermediate result blows up.]

  SideEffects [None]

  SeeAlso     [Cudd_bddAndAbstract Cudd_bddClippingAnd]

******************************************************************************/
DdNode *
Cudd_bddClippingAndAbstract(
  DdManager * dd /* manager */,
  DdNode * f /* first conjunct */,
  DdNode * g /* second conjunct */,
  DdNode * cube /* cube of variables to be abstracted */,
  int  maxDepth /* maximum recursion depth */,
  int  direction /* under (0) or over (1) approximation */)
{
    DdNode *res;

    do {
	dd->reordered = 0;
	res = cuddBddClippingAndAbstract(dd,f,g,cube,maxDepth,direction);
    } while (dd->reordered == 1);
    return(res);

} /* end of Cudd_bddClippingAndAbstract */


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Approximates the conjunction of two BDDs f and g.]

  Description [Approximates the conjunction of two BDDs f and g. Returns a
  pointer to the resulting BDD if successful; NULL if the intermediate
  result blows up.]

  SideEffects [None]

  SeeAlso     [Cudd_bddClippingAnd]

******************************************************************************/
DdNode *
cuddBddClippingAnd(
  DdManager * dd /* manager */,
  DdNode * f /* first conjunct */,
  DdNode * g /* second conjunct */,
  int  maxDepth /* maximum recursion depth */,
  int  direction /* under (0) or over (1) approximation */)
{
    DdNode *res;

    res = cuddBddClippingAndRecur(dd,f,g,maxDepth,direction);

    return(res);

} /* end of cuddBddClippingAnd */


/**Function********************************************************************

  Synopsis    [Approximates the conjunction of two BDDs f and g and
  simultaneously abstracts the variables in cube.]

  Description [Approximates the conjunction of two BDDs f and g and
  simultaneously abstracts the variables in cube. Returns a
  pointer to the resulting BDD if successful; NULL if the intermediate
  result blows up.]

  SideEffects [None]

  SeeAlso     [Cudd_bddClippingAndAbstract]

******************************************************************************/
DdNode *
cuddBddClippingAndAbstract(
  DdManager * dd /* manager */,
  DdNode * f /* first conjunct */,
  DdNode * g /* second conjunct */,
  DdNode * cube /* cube of variables to be abstracted */,
  int  maxDepth /* maximum recursion depth */,
  int  direction /* under (0) or over (1) approximation */)
{
    DdNode *res;

    res = cuddBddClipAndAbsRecur(dd,f,g,cube,maxDepth,direction);

    return(res);

} /* end of cuddBddClippingAndAbstract */


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis [Implements the recursive step of Cudd_bddClippingAnd.]

  Description [Implements the recursive step of Cudd_bddClippingAnd by taking
  the conjunction of two BDDs.  Returns a pointer to the result is
  successful; NULL otherwise.]

  SideEffects [None]

  SeeAlso     [cuddBddClippingAnd]

******************************************************************************/
static DdNode *
cuddBddClippingAndRecur(
  DdManager * manager,
  DdNode * f,
  DdNode * g,
  int  distance,
  int  direction)
{
    DdNode *F, *ft, *fe, *G, *gt, *ge;
    DdNode *one, *zero, *r, *t, *e;
    unsigned int topf, topg, index;
    DdNode *(*cacheOp)(DdManager *, DdNode *, DdNode *);

    statLine(manager);
    one = DD_ONE(manager);
    zero = Cudd_Not(one);

    /* Terminal cases. */
    if (f == zero || g == zero || f == Cudd_Not(g)) return(zero);
    if (f == g || g == one) return(f);
    if (f == one) return(g);
    if (distance == 0) {
	/* One last attempt at returning the right result. We sort of
	** cheat by calling Cudd_bddLeq. */
	if (Cudd_bddLeq(manager,f,g)) return(f);
	if (Cudd_bddLeq(manager,g,f)) return(g);
	if (direction == 1) {
	    if (Cudd_bddLeq(manager,f,Cudd_Not(g)) ||
		Cudd_bddLeq(manager,g,Cudd_Not(f))) return(zero);
	}
	return(Cudd_NotCond(one,(direction == 0)));
    }

    /* At this point f and g are not constant. */
    distance--;

    /* Check cache. Try to increase cache efficiency by sorting the
    ** pointers. */
    if (f > g) {
	DdNode *tmp = f;
	f = g; g = tmp;
    }
    F = Cudd_Regular(f);
    G = Cudd_Regular(g);
    cacheOp = (DdNode *(*)(DdManager *, DdNode *, DdNode *))
	(direction ? Cudd_bddClippingAnd : cuddBddClippingAnd);
    if (F->ref != 1 || G->ref != 1) {
	r = cuddCacheLookup2(manager, cacheOp, f, g);
	if (r != NULL) return(r);
    }

    /* Here we can skip the use of cuddI, because the operands are known
    ** to be non-constant.
    */
    topf = manager->perm[F->index];
    topg = manager->perm[G->index];

    /* Compute cofactors. */
    if (topf <= topg) {
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

    if (topg <= topf) {
	gt = cuddT(G);
	ge = cuddE(G);
	if (Cudd_IsComplement(g)) {
	    gt = Cudd_Not(gt);
	    ge = Cudd_Not(ge);
	}
    } else {
	gt = ge = g;
    }

    t = cuddBddClippingAndRecur(manager, ft, gt, distance, direction);
    if (t == NULL) return(NULL);
    cuddRef(t);
    e = cuddBddClippingAndRecur(manager, fe, ge, distance, direction);
    if (e == NULL) {
	Cudd_RecursiveDeref(manager, t);
	return(NULL);
    }
    cuddRef(e);

    if (t == e) {
	r = t;
    } else {
	if (Cudd_IsComplement(t)) {
	    r = cuddUniqueInter(manager,(int)index,Cudd_Not(t),Cudd_Not(e));
	    if (r == NULL) {
		Cudd_RecursiveDeref(manager, t);
		Cudd_RecursiveDeref(manager, e);
		return(NULL);
	    }
	    r = Cudd_Not(r);
	} else {
	    r = cuddUniqueInter(manager,(int)index,t,e);
	    if (r == NULL) {
		Cudd_RecursiveDeref(manager, t);
		Cudd_RecursiveDeref(manager, e);
		return(NULL);
	    }
	}
    }
    cuddDeref(e);
    cuddDeref(t);
    if (F->ref != 1 || G->ref != 1)
	cuddCacheInsert2(manager, cacheOp, f, g, r);
    return(r);

} /* end of cuddBddClippingAndRecur */


/**Function********************************************************************

  Synopsis [Approximates the AND of two BDDs and simultaneously abstracts the
  variables in cube.]

  Description [Approximates the AND of two BDDs and simultaneously
  abstracts the variables in cube. The variables are existentially
  abstracted.  Returns a pointer to the result is successful; NULL
  otherwise.]

  SideEffects [None]

  SeeAlso     [Cudd_bddClippingAndAbstract]

******************************************************************************/
static DdNode *
cuddBddClipAndAbsRecur(
  DdManager * manager,
  DdNode * f,
  DdNode * g,
  DdNode * cube,
  int  distance,
  int  direction)
{
    DdNode *F, *ft, *fe, *G, *gt, *ge;
    DdNode *one, *zero, *r, *t, *e, *Cube;
    unsigned int topf, topg, topcube, top, index;
    ptruint cacheTag;

    statLine(manager);
    one = DD_ONE(manager);
    zero = Cudd_Not(one);

    /* Terminal cases. */
    if (f == zero || g == zero || f == Cudd_Not(g)) return(zero);
    if (f == one && g == one)	return(one);
    if (cube == one) {
	return(cuddBddClippingAndRecur(manager, f, g, distance, direction));
    }
    if (f == one || f == g) {
	return (cuddBddExistAbstractRecur(manager, g, cube));
    }
    if (g == one) {
	return (cuddBddExistAbstractRecur(manager, f, cube));
    }
    if (distance == 0) return(Cudd_NotCond(one,(direction == 0)));

    /* At this point f, g, and cube are not constant. */
    distance--;

    /* Check cache. */
    if (f > g) { /* Try to increase cache efficiency. */
	DdNode *tmp = f;
	f = g; g = tmp;
    }
    F = Cudd_Regular(f);
    G = Cudd_Regular(g);
    cacheTag = direction ? DD_BDD_CLIPPING_AND_ABSTRACT_UP_TAG :
	DD_BDD_CLIPPING_AND_ABSTRACT_DOWN_TAG;
    if (F->ref != 1 || G->ref != 1) {
	r = cuddCacheLookup(manager, cacheTag,
			    f, g, cube);
	if (r != NULL) {
	    return(r);
	}
    }

    /* Here we can skip the use of cuddI, because the operands are known
    ** to be non-constant.
    */
    topf = manager->perm[F->index];
    topg = manager->perm[G->index];
    top = ddMin(topf, topg);
    topcube = manager->perm[cube->index];

    if (topcube < top) {
	return(cuddBddClipAndAbsRecur(manager, f, g, cuddT(cube),
				      distance, direction));
    }
    /* Now, topcube >= top. */

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

    if (topcube == top) {
	Cube = cuddT(cube);
    } else {
	Cube = cube;
    }

    t = cuddBddClipAndAbsRecur(manager, ft, gt, Cube, distance, direction);
    if (t == NULL) return(NULL);

    /* Special case: 1 OR anything = 1. Hence, no need to compute
    ** the else branch if t is 1.
    */
    if (t == one && topcube == top) {
	if (F->ref != 1 || G->ref != 1)
	    cuddCacheInsert(manager, cacheTag, f, g, cube, one);
	return(one);
    }
    cuddRef(t);

    e = cuddBddClipAndAbsRecur(manager, fe, ge, Cube, distance, direction);
    if (e == NULL) {
	Cudd_RecursiveDeref(manager, t);
	return(NULL);
    }
    cuddRef(e);

    if (topcube == top) {	/* abstract */
	r = cuddBddClippingAndRecur(manager, Cudd_Not(t), Cudd_Not(e),
				    distance, (direction == 0));
	if (r == NULL) {
	    Cudd_RecursiveDeref(manager, t);
	    Cudd_RecursiveDeref(manager, e);
	    return(NULL);
	}
	r = Cudd_Not(r);
	cuddRef(r);
	Cudd_RecursiveDeref(manager, t);
	Cudd_RecursiveDeref(manager, e);
	cuddDeref(r);
    } else if (t == e) {
	r = t;
	cuddDeref(t);
	cuddDeref(e);
    } else {
	if (Cudd_IsComplement(t)) {
	    r = cuddUniqueInter(manager,(int)index,Cudd_Not(t),Cudd_Not(e));
	    if (r == NULL) {
		Cudd_RecursiveDeref(manager, t);
		Cudd_RecursiveDeref(manager, e);
		return(NULL);
	    }
	    r = Cudd_Not(r);
	} else {
	    r = cuddUniqueInter(manager,(int)index,t,e);
	    if (r == NULL) {
		Cudd_RecursiveDeref(manager, t);
		Cudd_RecursiveDeref(manager, e);
		return(NULL);
	    }
	}
	cuddDeref(e);
	cuddDeref(t);
    }
    if (F->ref != 1 || G->ref != 1)
	cuddCacheInsert(manager, cacheTag, f, g, cube, r);
    return (r);

} /* end of cuddBddClipAndAbsRecur */

