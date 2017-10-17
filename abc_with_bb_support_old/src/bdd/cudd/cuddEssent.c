/**CFile***********************************************************************

  FileName    [cuddEssent.c]

  PackageName [cudd]

  Synopsis    [Functions for the detection of essential variables.]

  Description [External procedures included in this file:
		<ul>
		<li> Cudd_FindEssential()
		<li> Cudd_bddIsVarEssential()
		</ul>
	Static procedures included in this module:
		<ul>
		<li> ddFindEssentialRecur()
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
static char rcsid[] DD_UNUSED = "$Id: cuddEssent.c,v 1.1.1.1 2003/02/24 22:23:51 wjiang Exp $";
#endif

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static DdNode * ddFindEssentialRecur ARGS((DdManager *dd, DdNode *f));

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Finds the essential variables of a DD.]

  Description [Returns the cube of the essential variables. A positive
  literal means that the variable must be set to 1 for the function to be
  1. A negative literal means that the variable must be set to 0 for the
  function to be 1. Returns a pointer to the cube BDD if successful;
  NULL otherwise.]

  SideEffects [None]

  SeeAlso     [Cudd_bddIsVarEssential]

******************************************************************************/
DdNode *
Cudd_FindEssential(
  DdManager * dd,
  DdNode * f)
{
    DdNode *res;

    do {
	dd->reordered = 0;
	res = ddFindEssentialRecur(dd,f);
    } while (dd->reordered == 1);
    return(res);

} /* end of Cudd_FindEssential */


/**Function********************************************************************

  Synopsis    [Determines whether a given variable is essential with a
  given phase in a BDD.]

  Description [Determines whether a given variable is essential with a
  given phase in a BDD. Uses Cudd_bddIteConstant. Returns 1 if phase == 1
  and f-->x_id, or if phase == 0 and f-->x_id'.]

  SideEffects [None]

  SeeAlso     [Cudd_FindEssential]

******************************************************************************/
int
Cudd_bddIsVarEssential(
  DdManager * manager,
  DdNode * f,
  int  id,
  int  phase)
{
    DdNode	*var;
    int		res;
    DdNode	*one, *zero;

    one = DD_ONE(manager);
    zero = Cudd_Not(one);

    var = cuddUniqueInter(manager, id, one, zero);

    var = Cudd_NotCond(var,phase == 0);

    res = Cudd_bddIteConstant(manager, Cudd_Not(f), one, var) == one;

    return(res);

} /* end of Cudd_bddIsVarEssential */


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Implements the recursive step of Cudd_FindEssential.]

  Description [Implements the recursive step of Cudd_FindEssential.
  Returns a pointer to the cube BDD if successful; NULL otherwise.]

  SideEffects [None]

******************************************************************************/
static DdNode *
ddFindEssentialRecur(
  DdManager * dd,
  DdNode * f)
{
    DdNode	*T, *E, *F;
    DdNode	*essT, *essE, *res;
    int		index;
    DdNode	*one, *lzero, *azero;

    one = DD_ONE(dd);
    F = Cudd_Regular(f);
    /* If f is constant the set of essential variables is empty. */
    if (cuddIsConstant(F)) return(one);

    res = cuddCacheLookup1(dd,Cudd_FindEssential,f);
    if (res != NULL) {
	return(res);
    }

    lzero = Cudd_Not(one);
    azero = DD_ZERO(dd);
    /* Find cofactors: here f is non-constant. */
    T = cuddT(F);
    E = cuddE(F);
    if (Cudd_IsComplement(f)) {
	T = Cudd_Not(T); E = Cudd_Not(E);
    }

    index = F->index;
    if (Cudd_IsConstant(T) && T != lzero && T != azero) {
	/* if E is zero, index is essential, otherwise there are no
	** essentials, because index is not essential and no other variable
	** can be, since setting index = 1 makes the function constant and
	** different from 0.
	*/
	if (E == lzero || E == azero) {
	    res = dd->vars[index];
	} else {
	    res = one;
	}
    } else if (T == lzero || T == azero) {
	if (Cudd_IsConstant(E)) { /* E cannot be zero here */
	    res = Cudd_Not(dd->vars[index]);
	} else { /* E == non-constant */
	    /* find essentials in the else branch */
	    essE = ddFindEssentialRecur(dd,E);
	    if (essE == NULL) {
		return(NULL);
	    }
	    cuddRef(essE);

	    /* add index to the set with negative phase */
	    res = cuddUniqueInter(dd,index,one,Cudd_Not(essE));
	    if (res == NULL) {
		Cudd_RecursiveDeref(dd,essE);
		return(NULL);
	    }
	    res = Cudd_Not(res);
	    cuddDeref(essE);
	}
    } else { /* T == non-const */
	if (E == lzero || E == azero) {
	    /* find essentials in the then branch */
	    essT = ddFindEssentialRecur(dd,T);
	    if (essT == NULL) {
		return(NULL);
	    }
	    cuddRef(essT);

	    /* add index to the set with positive phase */
	    /* use And because essT may be complemented */
	    res = cuddBddAndRecur(dd,dd->vars[index],essT);
	    if (res == NULL) {
		Cudd_RecursiveDeref(dd,essT);
		return(NULL);
	    }
	    cuddDeref(essT);
	} else if (!Cudd_IsConstant(E)) {
	    /* if E is a non-zero constant there are no essentials
	    ** because T is non-constant.
	    */
	    essT = ddFindEssentialRecur(dd,T);
	    if (essT == NULL) {
		return(NULL);
	    }
	    if (essT == one) {
		res = one;
	    } else {
		cuddRef(essT);
		essE = ddFindEssentialRecur(dd,E);
		if (essE == NULL) {
		    Cudd_RecursiveDeref(dd,essT);
		    return(NULL);
		}
		cuddRef(essE);

		/* res = intersection(essT, essE) */
		res = cuddBddLiteralSetIntersectionRecur(dd,essT,essE);
		if (res == NULL) {
		    Cudd_RecursiveDeref(dd,essT);
		    Cudd_RecursiveDeref(dd,essE);
		    return(NULL);
		}
		cuddRef(res);
		Cudd_RecursiveDeref(dd,essT);
		Cudd_RecursiveDeref(dd,essE);
		cuddDeref(res);
	    }
	} else {	/* E is a non-zero constant */
	    res = one;
	}
    }

    cuddCacheInsert1(dd,Cudd_FindEssential, f, res);
    return(res);

} /* end of ddFindEssentialRecur */

