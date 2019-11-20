/**CFile***********************************************************************

  FileName    [cuddAddNeg.c]

  PackageName [cudd]

  Synopsis    [function to compute the negation of an ADD.]

  Description [External procedures included in this module:
		<ul>
		<li> Cudd_addNegate()
		<li> Cudd_addRoundOff()
		</ul>
	Internal procedures included in this module:
		<ul>
		<li> cuddAddNegateRecur()
		<li> cuddAddRoundOffRecur()
		</ul> ]

  Author      [Fabio Somenzi, Balakrishna Kumthekar]

  Copyright   [This file was created at the University of Colorado at
  Boulder.  The University of Colorado at Boulder makes no warranty
  about the suitability of this software for any purpose.  It is
  presented on an AS IS basis.]

******************************************************************************/

#include    "util_hack.h"
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
static char rcsid[] DD_UNUSED = "$Id: cuddAddNeg.c,v 1.1.1.1 2003/02/24 22:23:50 wjiang Exp $";
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

  Synopsis    [Computes the additive inverse of an ADD.]

  Description [Computes the additive inverse of an ADD. Returns a pointer
  to the result if successful; NULL otherwise.]

  SideEffects [None]

  SeeAlso     [Cudd_addCmpl]

******************************************************************************/
DdNode *
Cudd_addNegate(
  DdManager * dd,
  DdNode * f)
{
    DdNode *res;

    do {
	res = cuddAddNegateRecur(dd,f);
    } while (dd->reordered == 1);
    return(res);

} /* end of Cudd_addNegate */


/**Function********************************************************************

  Synopsis    [Rounds off the discriminants of an ADD.]

  Description [Rounds off the discriminants of an ADD. The discriminants are
  rounded off to N digits after the decimal. Returns a pointer to the result
  ADD if successful; NULL otherwise.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode *
Cudd_addRoundOff(
  DdManager * dd,
  DdNode * f,
  int  N)
{
    DdNode *res;
    double trunc = pow(10.0,(double)N);

    do {
	res = cuddAddRoundOffRecur(dd,f,trunc);
    } while (dd->reordered == 1);
    return(res);

} /* end of Cudd_addRoundOff */


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Implements the recursive step of Cudd_addNegate.]

  Description [Implements the recursive step of Cudd_addNegate.
  Returns a pointer to the result.]

  SideEffects [None]

******************************************************************************/
DdNode *
cuddAddNegateRecur(
  DdManager * dd,
  DdNode * f)
{
    DdNode *res,
	    *fv, *fvn,
	    *T, *E;

    statLine(dd);
    /* Check terminal cases. */
    if (cuddIsConstant(f)) {
	res = cuddUniqueConst(dd,-cuddV(f));
	return(res);
    }

    /* Check cache */
    res = cuddCacheLookup1(dd,Cudd_addNegate,f);
    if (res != NULL) return(res);

    /* Recursive Step */
    fv = cuddT(f);
    fvn = cuddE(f);
    T = cuddAddNegateRecur(dd,fv);
    if (T == NULL) return(NULL);
    cuddRef(T);

    E = cuddAddNegateRecur(dd,fvn);
    if (E == NULL) {
	Cudd_RecursiveDeref(dd,T);
	return(NULL);
    }
    cuddRef(E);
    res = (T == E) ? T : cuddUniqueInter(dd,(int)f->index,T,E);
    if (res == NULL) {
	Cudd_RecursiveDeref(dd, T);
	Cudd_RecursiveDeref(dd, E);
	return(NULL);
    }
    cuddDeref(T);
    cuddDeref(E);

    /* Store result. */
    cuddCacheInsert1(dd,Cudd_addNegate,f,res);

    return(res);

} /* end of cuddAddNegateRecur */


/**Function********************************************************************

  Synopsis    [Implements the recursive step of Cudd_addRoundOff.]

  Description [Implements the recursive step of Cudd_addRoundOff.
  Returns a pointer to the result.]

  SideEffects [None]

******************************************************************************/
DdNode *
cuddAddRoundOffRecur(
  DdManager * dd,
  DdNode * f,
  double  trunc)
{

    DdNode *res, *fv, *fvn, *T, *E;
    double n;
    DdNode *(*cacheOp)(DdManager *, DdNode *);
  
    statLine(dd);
    if (cuddIsConstant(f)) {
        n = ceil(cuddV(f)*trunc)/trunc;
	res = cuddUniqueConst(dd,n);
	return(res);
    }
    cacheOp = (DdNode *(*)(DdManager *, DdNode *)) Cudd_addRoundOff;
    res = cuddCacheLookup1(dd,cacheOp,f);
    if (res != NULL) {
        return(res);
    }
    /* Recursive Step */
    fv = cuddT(f);
    fvn = cuddE(f);
    T = cuddAddRoundOffRecur(dd,fv,trunc);
    if (T == NULL) {
       return(NULL);
    }
    cuddRef(T);
    E = cuddAddRoundOffRecur(dd,fvn,trunc);
    if (E == NULL) {
        Cudd_RecursiveDeref(dd,T);
	return(NULL);
    }
    cuddRef(E);
    res = (T == E) ? T : cuddUniqueInter(dd,(int)f->index,T,E);
    if (res == NULL) {
        Cudd_RecursiveDeref(dd,T);
	Cudd_RecursiveDeref(dd,E);
	return(NULL);
    }
    cuddDeref(T);
    cuddDeref(E);

    /* Store result. */
    cuddCacheInsert1(dd,cacheOp,f,res);
    return(res);
  
} /* end of cuddAddRoundOffRecur */

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

