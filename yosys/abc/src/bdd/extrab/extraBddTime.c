/**CFile****************************************************************

  FileName    [extraBddTime.c]

  PackageName [extra]

  Synopsis    [Procedures to control runtime in BDD operators.]

  Author      [Alan Mishchenko]

  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - September 1, 2003.]

  Revision    [$Id: extraBddTime.c,v 1.0 2003/05/21 18:03:50 alanmi Exp $]

***********************************************************************/

#include "extraBdd.h"

ABC_NAMESPACE_IMPL_START


/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

#define CHECK_FACTOR 10

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/



/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static DdNode * cuddBddAndRecurTime( DdManager * manager, DdNode * f, DdNode * g, int * pRecCalls, int TimeOut );
static DdNode * cuddBddAndAbstractRecurTime( DdManager * manager, DdNode * f, DdNode * g, DdNode * cube, int * pRecCalls, int TimeOut );
static DdNode * extraTransferPermuteTime( DdManager * ddS, DdManager * ddD, DdNode * f, int * Permute, int TimeOut );
static DdNode * extraTransferPermuteRecurTime( DdManager * ddS, DdManager * ddD, DdNode * f, st__table * table, int * Permute, int TimeOut );

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Computes the conjunction of two BDDs f and g.]

  Description [Computes the conjunction of two BDDs f and g. Returns a
  pointer to the resulting BDD if successful; NULL if the intermediate
  result blows up.]

  SideEffects [None]

  SeeAlso     [Cudd_bddIte Cudd_addApply Cudd_bddAndAbstract Cudd_bddIntersect
  Cudd_bddOr Cudd_bddNand Cudd_bddNor Cudd_bddXor Cudd_bddXnor]

******************************************************************************/
DdNode *
Extra_bddAndTime(
  DdManager * dd,
  DdNode * f,
  DdNode * g,
  int TimeOut)
{
    DdNode *res;
    int Counter = 0;

    do {
    dd->reordered = 0;
    res = cuddBddAndRecurTime(dd,f,g, &Counter, TimeOut);
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_bddAndTime */

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
Extra_bddAndAbstractTime(
  DdManager * manager,
  DdNode * f,
  DdNode * g,
  DdNode * cube,
  int TimeOut)
{
    DdNode *res;
    int Counter = 0;

    do {
    manager->reordered = 0;
    res = cuddBddAndAbstractRecurTime(manager, f, g, cube, &Counter, TimeOut);
    } while (manager->reordered == 1);
    return(res);

} /* end of Extra_bddAndAbstractTime */

/**Function********************************************************************

  Synopsis    [Convert a {A,B}DD from a manager to another with variable remapping.]

  Description [Convert a {A,B}DD from a manager to another one. The orders of the
  variables in the two managers may be different. Returns a
  pointer to the {A,B}DD in the destination manager if successful; NULL
  otherwise. The i-th entry in the array Permute tells what is the index
  of the i-th variable from the old manager in the new manager.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode * Extra_TransferPermuteTime( DdManager * ddSource, DdManager * ddDestination, DdNode * f, int * Permute, int TimeOut )
{
    DdNode * bRes;
    do
    {
        ddDestination->reordered = 0;
        bRes = extraTransferPermuteTime( ddSource, ddDestination, f, Permute, TimeOut );
    }
    while ( ddDestination->reordered == 1 );
    return ( bRes );

} /* end of Extra_TransferPermuteTime */


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis [Implements the recursive step of Cudd_bddAnd.]

  Description [Implements the recursive step of Cudd_bddAnd by taking
  the conjunction of two BDDs.  Returns a pointer to the result is
  successful; NULL otherwise.]

  SideEffects [None]

  SeeAlso     [Cudd_bddAnd]

******************************************************************************/
DdNode *
cuddBddAndRecurTime(
  DdManager * manager,
  DdNode * f,
  DdNode * g,
  int * pRecCalls,
  int TimeOut)
{
    DdNode *F, *fv, *fnv, *G, *gv, *gnv;
    DdNode *one, *r, *t, *e;
    unsigned int topf, topg, index;

    statLine(manager);
    one = DD_ONE(manager);

    /* Terminal cases. */
    F = Cudd_Regular(f);
    G = Cudd_Regular(g);
    if (F == G) {
    if (f == g) return(f);
    else return(Cudd_Not(one));
    }
    if (F == one) {
    if (f == one) return(g);
    else return(f);
    }
    if (G == one) {
    if (g == one) return(f);
    else return(g);
    }

    /* At this point f and g are not constant. */
    if (f > g) { /* Try to increase cache efficiency. */
    DdNode *tmp = f;
    f = g;
    g = tmp;
    F = Cudd_Regular(f);
    G = Cudd_Regular(g);
    }

    /* Check cache. */
    if (F->ref != 1 || G->ref != 1) {
    r = cuddCacheLookup2(manager, Cudd_bddAnd, f, g);
    if (r != NULL) return(r);
    }

//    if ( TimeOut && ((*pRecCalls)++ % CHECK_FACTOR) == 0 && TimeOut < Abc_Clock() )
    if ( TimeOut && Abc_Clock() > TimeOut )
        return NULL;

    /* Here we can skip the use of cuddI, because the operands are known
    ** to be non-constant.
    */
    topf = manager->perm[F->index];
    topg = manager->perm[G->index];

    /* Compute cofactors. */
    if (topf <= topg) {
    index = F->index;
    fv = cuddT(F);
    fnv = cuddE(F);
    if (Cudd_IsComplement(f)) {
        fv = Cudd_Not(fv);
        fnv = Cudd_Not(fnv);
    }
    } else {
    index = G->index;
    fv = fnv = f;
    }

    if (topg <= topf) {
    gv = cuddT(G);
    gnv = cuddE(G);
    if (Cudd_IsComplement(g)) {
        gv = Cudd_Not(gv);
        gnv = Cudd_Not(gnv);
    }
    } else {
    gv = gnv = g;
    }

    t = cuddBddAndRecurTime(manager, fv, gv, pRecCalls, TimeOut);
    if (t == NULL) return(NULL);
    cuddRef(t);

    e = cuddBddAndRecurTime(manager, fnv, gnv, pRecCalls, TimeOut);
    if (e == NULL) {
    Cudd_IterDerefBdd(manager, t);
    return(NULL);
    }
    cuddRef(e);

    if (t == e) {
    r = t;
    } else {
    if (Cudd_IsComplement(t)) {
        r = cuddUniqueInter(manager,(int)index,Cudd_Not(t),Cudd_Not(e));
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
    }
    cuddDeref(e);
    cuddDeref(t);
    if (F->ref != 1 || G->ref != 1)
    cuddCacheInsert2(manager, Cudd_bddAnd, f, g, r);
    return(r);

} /* end of cuddBddAndRecur */


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
cuddBddAndAbstractRecurTime(
  DdManager * manager,
  DdNode * f,
  DdNode * g,
  DdNode * cube,
  int * pRecCalls,
  int TimeOut)
{
    DdNode *F, *ft, *fe, *G, *gt, *ge;
    DdNode *one, *zero, *r, *t, *e;
    unsigned int topf, topg, topcube, top, index;

    statLine(manager);
    one = DD_ONE(manager);
    zero = Cudd_Not(one);

    /* Terminal cases. */
    if (f == zero || g == zero || f == Cudd_Not(g)) return(zero);
    if (f == one && g == one)    return(one);

    if (cube == one) {
    return(cuddBddAndRecurTime(manager, f, g, pRecCalls, TimeOut));
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
        return(cuddBddAndRecurTime(manager, f, g, pRecCalls, TimeOut));
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

//    if ( TimeOut && ((*pRecCalls)++ % CHECK_FACTOR) == 0 && TimeOut < Abc_Clock() )
    if ( TimeOut && Abc_Clock() > TimeOut )
        return NULL;

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

    if (topcube == top) {    /* quantify */
    DdNode *Cube = cuddT(cube);
    t = cuddBddAndAbstractRecurTime(manager, ft, gt, Cube, pRecCalls, TimeOut);
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
        e = cuddBddAndAbstractRecurTime(manager, fe, ge, Cube, pRecCalls, TimeOut);
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
        r = cuddBddAndRecurTime(manager, Cudd_Not(t), Cudd_Not(e), pRecCalls, TimeOut);
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
    t = cuddBddAndAbstractRecurTime(manager, ft, gt, cube, pRecCalls, TimeOut);
    if (t == NULL) return(NULL);
    cuddRef(t);
    e = cuddBddAndAbstractRecurTime(manager, fe, ge, cube, pRecCalls, TimeOut);
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

/**Function********************************************************************

  Synopsis    [Convert a BDD from a manager to another one.]

  Description [Convert a BDD from a manager to another one. Returns a
  pointer to the BDD in the destination manager if successful; NULL
  otherwise.]

  SideEffects [None]

  SeeAlso     [Extra_TransferPermute]

******************************************************************************/
DdNode * extraTransferPermuteTime( DdManager * ddS, DdManager * ddD, DdNode * f, int * Permute, int TimeOut )
{
    DdNode *res;
    st__table *table = NULL;
    st__generator *gen = NULL;
    DdNode *key, *value;

    table = st__init_table( st__ptrcmp, st__ptrhash );
    if ( table == NULL )
        goto failure;
    res = extraTransferPermuteRecurTime( ddS, ddD, f, table, Permute, TimeOut );
    if ( res != NULL )
        cuddRef( res );

    /* Dereference all elements in the table and dispose of the table.
       ** This must be done also if res is NULL to avoid leaks in case of
       ** reordering. */
    gen = st__init_gen( table );
    if ( gen == NULL )
        goto failure;
    while ( st__gen( gen, ( const char ** ) &key, ( char ** ) &value ) )
    {
        Cudd_RecursiveDeref( ddD, value );
    }
    st__free_gen( gen );
    gen = NULL;
    st__free_table( table );
    table = NULL;

    if ( res != NULL )
        cuddDeref( res );
    return ( res );

  failure:
    if ( table != NULL )
        st__free_table( table );
    if ( gen != NULL )
        st__free_gen( gen );
    return ( NULL );

} /* end of extraTransferPermuteTime */


/**Function********************************************************************

  Synopsis    [Performs the recursive step of Extra_TransferPermute.]

  Description [Performs the recursive step of Extra_TransferPermute.
  Returns a pointer to the result if successful; NULL otherwise.]

  SideEffects [None]

  SeeAlso     [extraTransferPermuteTime]

******************************************************************************/
static DdNode * 
extraTransferPermuteRecurTime( 
  DdManager * ddS, 
  DdManager * ddD, 
  DdNode * f, 
  st__table * table, 
  int * Permute,
  int TimeOut )
{
    DdNode *ft, *fe, *t, *e, *var, *res;
    DdNode *one, *zero;
    int index;
    int comple = 0;

    statLine( ddD );
    one = DD_ONE( ddD );
    comple = Cudd_IsComplement( f );

    /* Trivial cases. */
    if ( Cudd_IsConstant( f ) )
        return ( Cudd_NotCond( one, comple ) );


    /* Make canonical to increase the utilization of the cache. */
    f = Cudd_NotCond( f, comple );
    /* Now f is a regular pointer to a non-constant node. */

    /* Check the cache. */
    if ( st__lookup( table, ( char * ) f, ( char ** ) &res ) )
        return ( Cudd_NotCond( res, comple ) );

    if ( TimeOut && Abc_Clock() > TimeOut )
        return NULL;

    /* Recursive step. */
    if ( Permute )
        index = Permute[f->index];
    else
        index = f->index;

    ft = cuddT( f );
    fe = cuddE( f );

    t = extraTransferPermuteRecurTime( ddS, ddD, ft, table, Permute, TimeOut );
    if ( t == NULL )
    {
        return ( NULL );
    }
    cuddRef( t );

    e = extraTransferPermuteRecurTime( ddS, ddD, fe, table, Permute, TimeOut );
    if ( e == NULL )
    {
        Cudd_RecursiveDeref( ddD, t );
        return ( NULL );
    }
    cuddRef( e );

    zero = Cudd_Not(ddD->one);
    var = cuddUniqueInter( ddD, index, one, zero );
    if ( var == NULL )
    {
        Cudd_RecursiveDeref( ddD, t );
        Cudd_RecursiveDeref( ddD, e );
        return ( NULL );
    }
    res = cuddBddIteRecur( ddD, var, t, e );

    if ( res == NULL )
    {
        Cudd_RecursiveDeref( ddD, t );
        Cudd_RecursiveDeref( ddD, e );
        return ( NULL );
    }
    cuddRef( res );
    Cudd_RecursiveDeref( ddD, t );
    Cudd_RecursiveDeref( ddD, e );

    if ( st__add_direct( table, ( char * ) f, ( char * ) res ) ==
         st__OUT_OF_MEM )
    {
        Cudd_RecursiveDeref( ddD, res );
        return ( NULL );
    }
    return ( Cudd_NotCond( res, comple ) );

}  /* end of extraTransferPermuteRecurTime */

////////////////////////////////////////////////////////////////////////
///                           END OF FILE                            ///
////////////////////////////////////////////////////////////////////////
ABC_NAMESPACE_IMPL_END

