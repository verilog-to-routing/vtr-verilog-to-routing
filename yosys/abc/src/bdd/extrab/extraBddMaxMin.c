/**CFile***********************************************************************

  FileName    [zMaxMin.c]

  PackageName [extra]

  Synopsis    [Experimental version of some ZDD operators.]

  Description [External procedures included in this module:
				<ul>
				<li> Extra_zddMaximal();
				<li> Extra_zddMinimal();
				<li> Extra_zddMaxUnion();
				<li> Extra_zddMinUnion();
				<li> Extra_zddDotProduct();
				<li> Extra_zddCrossProduct();
				<li> Extra_zddMaxDotProduct();
				</ul>
			Internal procedures included in this module:
				<ul>
				<li> extraZddMaximal();
				<li> extraZddMinimal();
				<li> extraZddMaxUnion();
				<li> extraZddMinUnion();
				<li> extraZddDotProduct();
				<li> extraZddCrossProduct();
				<li> extraZddMaxDotProduct();
				</ul>
			StaTc procedures included in this module:
				<ul>
				</ul>
				
          DotProduct and MaxDotProduct were introduced 
		  by O.Coudert to solve problems arising in two-level planar routing
          See O. Coudert, C.-J. R. Shi. Exact Multi-Layer Topological Planar 
		  Routing. Proc. of IEEE Custom Integrated Circuit Conference '96, 
		  pp. 179-182.
	      ]

  SeeAlso     []

  Author      [Alan Mishchenko]

  Copyright   []

  ReviSon    [$zMaxMin.c, v.1.2, November 26, 2000, alanmi $]

******************************************************************************/

#include "extraBdd.h"

ABC_NAMESPACE_IMPL_START

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

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**AutomaTcStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* StaTc Function prototypes                                                */
/*---------------------------------------------------------------------------*/

/**AutomaTcEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported Functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Computes the maximal of a set represented by its ZDD.]

  Description []

  SideEffects []

  SeeAlso     [Extra_zddMinimal]

******************************************************************************/
DdNode	*
Extra_zddMaximal(
  DdManager * dd,
  DdNode * S)
{
    DdNode	*res;
    do {
	dd->reordered = 0;
	res = extraZddMaximal(dd, S);
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddMaximal */


/**Function********************************************************************

  Synopsis    [Computes the minimal of a set represented by its ZDD.]

  Description []

  SideEffects []

  SeeAlso     [Extra_zddMaximal]

******************************************************************************/
DdNode	*
Extra_zddMinimal(
  DdManager * dd,
  DdNode * S)
{
    DdNode	*res;
    do {
	dd->reordered = 0;
	res = extraZddMinimal(dd, S);
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddMinimal */


/**Function********************************************************************

  Synopsis    [Computes the maximal of the union of two sets represented by ZDDs.]

  Description []

  SideEffects []

  SeeAlso     [Extra_zddMaximal Extra_zddMimimal Extra_zddMinUnion]

******************************************************************************/
DdNode	*
Extra_zddMaxUnion(
  DdManager * dd,
  DdNode * S,
  DdNode * T)
{
    DdNode	*res;
    do {
	dd->reordered = 0;
	res = extraZddMaxUnion(dd, S, T);
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddMaxUnion */


/**Function********************************************************************

  Synopsis    [Computes the minimal of the union of two sets represented by ZDDs.]

  Description []

  SideEffects []

  SeeAlso     [Extra_zddMaximal Extra_zddMimimal Extra_zddMaxUnion]

******************************************************************************/
DdNode	*
Extra_zddMinUnion(
  DdManager * dd,
  DdNode * S,
  DdNode * T)
{
    DdNode	*res;
    do {
	dd->reordered = 0;
	res = extraZddMinUnion(dd, S, T);
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddMinUnion */


/**Function********************************************************************

  Synopsis    [Computes the dot product of two sets of subsets represented by ZDDs.]

  Description [The dot product is defined as a set of pair-wise unions of subsets 
  belonging to the arguments.]

  SideEffects []

  SeeAlso     [Extra_zddCrossProduct]

******************************************************************************/
DdNode	*
Extra_zddDotProduct(
  DdManager * dd,
  DdNode * S,
  DdNode * T)
{
    DdNode	*res;
    do {
	dd->reordered = 0;
	res = extraZddDotProduct(dd, S, T);
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddDotProduct */


/**Function********************************************************************

  Synopsis    [Computes the cross product of two sets of subsets represented by ZDDs.]

  Description [The cross product is defined as a set of pair-wise intersections of subsets 
  belonging to the arguments.]

  SideEffects []

  SeeAlso     [Extra_zddDotProduct]

******************************************************************************/
DdNode	*
Extra_zddCrossProduct(
  DdManager * dd,
  DdNode * S,
  DdNode * T)
{
    DdNode	*res;
    do {
	dd->reordered = 0;
	res = extraZddCrossProduct(dd, S, T);
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddCrossProduct */


/**Function********************************************************************

  Synopsis    [Computes the maximal of the DotProduct of the union of two sets represented by ZDDs.]

  Description []

  SideEffects []

  SeeAlso     [Extra_zddDotProduct Extra_zddMaximal Extra_zddMinCrossProduct]

******************************************************************************/
DdNode	*
Extra_zddMaxDotProduct(
  DdManager * dd,
  DdNode * S,
  DdNode * T)
{
    DdNode	*res;
    do {
	dd->reordered = 0;
	res = extraZddMaxDotProduct(dd, S, T);
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddMaxDotProduct */


/*---------------------------------------------------------------------------*/
/* Definition of internal Functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis [Performs the recursive step of Extra_zddMaximal.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode	*
extraZddMaximal(
  DdManager * dd,
  DdNode * zSet)
{
	DdNode *zRes;
    statLine(dd); 

	/* consider terminal cases */
	if ( zSet == z0 || zSet == z1 )
		return zSet;

    /* check cache */
	zRes = cuddCacheLookup1Zdd(dd, extraZddMaximal, zSet);
	if (zRes)
		return(zRes);
	else
	{
		DdNode *zSet0, *zSet1, *zRes0, *zRes1;

		/* compute maximal for subsets without the top-most element */
		zSet0 = extraZddMaximal(dd, cuddE(zSet));
		if ( zSet0 == NULL )
			return NULL;
		cuddRef( zSet0 );

		/* compute maximal for subsets with the top-most element */
		zSet1 = extraZddMaximal(dd, cuddT(zSet));
		if ( zSet1 == NULL )
		{
			Cudd_RecursiveDerefZdd(dd, zSet0);
			return NULL;
		}
		cuddRef( zSet1 );

		/* remove subsets without this element covered by subsets with this element */
		zRes0 = extraZddNotSubSet(dd, zSet0, zSet1);
		if ( zRes0 == NULL )
		{
			Cudd_RecursiveDerefZdd(dd, zSet0);
			Cudd_RecursiveDerefZdd(dd, zSet1);
			return NULL;
		}
		cuddRef( zRes0 );
		Cudd_RecursiveDerefZdd(dd, zSet0);

		/* subset with this element remains unchanged */
		zRes1 = zSet1;

		/* create the new node */
		zRes = cuddZddGetNode( dd, zSet->index, zRes1, zRes0 );
		if ( zRes == NULL ) 
		{
			Cudd_RecursiveDerefZdd( dd, zRes0 );
			Cudd_RecursiveDerefZdd( dd, zRes1 );
			return NULL;
		}
		cuddDeref( zRes0 );
		cuddDeref( zRes1 );

		/* insert the result into cache */
		cuddCacheInsert1(dd, extraZddMaximal, zSet, zRes);
		return zRes;
	}
} /* end of extraZddMaximal */



/**Function********************************************************************

  Synopsis [Performs the recursive step of Extra_zddMinimal.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode	*
extraZddMinimal(
  DdManager * dd,
  DdNode * zSet)
{
	DdNode *zRes;
    statLine(dd); 

	/* consider terminal cases */
	if ( zSet == z0 )
		return zSet;
	/* the empty combinaTon, if present, is the only minimal combinaTon */
	if ( Extra_zddEmptyBelongs(dd, zSet) )
		return z1; 

    /* check cache */
	zRes = cuddCacheLookup1Zdd(dd, extraZddMinimal, zSet);
	if (zRes)
		return(zRes);
	else
	{
		DdNode *zSet0, *zSet1, *zRes0, *zRes1;

		/* compute minimal for subsets without the top-most element */
		zSet0 = extraZddMinimal(dd, cuddE(zSet));
		if ( zSet0 == NULL )
			return NULL;
		cuddRef( zSet0 );

		/* compute minimal for subsets with the top-most element */
		zSet1 = extraZddMinimal(dd, cuddT(zSet));
		if ( zSet1 == NULL )
		{
			Cudd_RecursiveDerefZdd(dd, zSet0);
			return NULL;
		}
		cuddRef( zSet1 );

		/* subset without this element remains unchanged */
		zRes0 = zSet0;

		/* remove subsets with this element that contain subsets without this element */
		zRes1 = extraZddNotSupSet(dd, zSet1, zSet0);
		if ( zRes1 == NULL )
		{
			Cudd_RecursiveDerefZdd(dd, zSet0);
			Cudd_RecursiveDerefZdd(dd, zSet1);
			return NULL;
		}
		cuddRef( zRes1 );
		Cudd_RecursiveDerefZdd(dd, zSet1);

		/* create the new node */
		zRes = cuddZddGetNode( dd, zSet->index, zRes1, zRes0 );
		if ( zRes == NULL ) 
		{
			Cudd_RecursiveDerefZdd( dd, zRes0 );
			Cudd_RecursiveDerefZdd( dd, zRes1 );
			return NULL;
		}
		cuddDeref( zRes0 );
		cuddDeref( zRes1 );

		/* insert the result into cache */
		cuddCacheInsert1(dd, extraZddMinimal, zSet, zRes);
		return zRes;
	}
} /* end of extraZddMinimal */


/**Function********************************************************************

  Synopsis [Performs the recursive step of Extra_zddMaxUnion.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode	*
extraZddMaxUnion(
  DdManager * dd,
  DdNode * S,
  DdNode * T)
{
	DdNode *zRes;
	int TopS, TopT;
    statLine(dd); 

	/* consider terminal cases */
	if ( S == z0 )
		return T;
	if ( T == z0 )
		return S;
	if ( S == T )
		return S;
	if ( S == z1 )
		return T;
	if ( T == z1 )
		return S;

	/* the operation is commutative - normalize the problem */
	TopS = dd->permZ[S->index];
	TopT = dd->permZ[T->index];

	if ( TopS > TopT || (TopS == TopT && S > T) )
		return extraZddMaxUnion(dd, T, S);

    /* check cache */
	zRes = cuddCacheLookup2Zdd(dd, extraZddMaxUnion, S, T);
	if (zRes)
		return zRes;
	else
	{
		DdNode *zSet0, *zSet1, *zRes0, *zRes1;

		if ( TopS == TopT )
		{
			/* compute maximal for subsets without the top-most element */
			zSet0 = extraZddMaxUnion(dd, cuddE(S), cuddE(T) );
			if ( zSet0 == NULL )
				return NULL;
			cuddRef( zSet0 );

			/* compute maximal for subsets with the top-most element */
			zSet1 = extraZddMaxUnion(dd, cuddT(S), cuddT(T) );
			if ( zSet1 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zSet0);
				return NULL;
			}
			cuddRef( zSet1 );
		}
		else /* if ( TopS < TopT ) */
		{
			/* compute maximal for subsets without the top-most element */
			zSet0 = extraZddMaxUnion(dd, cuddE(S), T );
			if ( zSet0 == NULL )
				return NULL;
			cuddRef( zSet0 );

			/* subset with this element is just the cofactor of S */
			zSet1 = cuddT(S);
			cuddRef( zSet1 );
		}

		/* remove subsets without this element covered by subsets with this element */
		zRes0 = extraZddNotSubSet(dd, zSet0, zSet1);
		if ( zRes0 == NULL )
		{
			Cudd_RecursiveDerefZdd(dd, zSet0);
			Cudd_RecursiveDerefZdd(dd, zSet1);
			return NULL;
		}
		cuddRef( zRes0 );
		Cudd_RecursiveDerefZdd(dd, zSet0);

		/* subset with this element remains unchanged */
		zRes1 = zSet1;

		/* create the new node */
		zRes = cuddZddGetNode( dd, S->index, zRes1, zRes0 );
		if ( zRes == NULL ) 
		{
			Cudd_RecursiveDerefZdd( dd, zRes0 );
			Cudd_RecursiveDerefZdd( dd, zRes1 );
			return NULL;
		}
		cuddDeref( zRes0 );
		cuddDeref( zRes1 );

		/* insert the result into cache */
		cuddCacheInsert2(dd, extraZddMaxUnion, S, T, zRes);
		return zRes;
	}
} /* end of extraZddMaxUnion */


/**Function********************************************************************

  Synopsis [Performs the recursive step of Extra_zddMinUnion.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode	*
extraZddMinUnion(
  DdManager * dd,
  DdNode * S,
  DdNode * T)
{
	DdNode *zRes;
	int TopS, TopT;
    statLine(dd); 

	/* consider terminal cases */
	if ( S == z0 )
		return T;
	if ( T == z0 )
		return S;
	if ( S == T )
		return S;
	/* the empty combination, if present, is the only minimal combination */
	if ( Extra_zddEmptyBelongs(dd, S) || Extra_zddEmptyBelongs(dd, T) )
		return z1; 

	/* the operation is commutative - normalize the problem */
	TopS = dd->permZ[S->index];
	TopT = dd->permZ[T->index];

	if ( TopS > TopT || (TopS == TopT && S > T) )
		return extraZddMinUnion(dd, T, S);

    /* check cache */
	zRes = cuddCacheLookup2Zdd(dd, extraZddMinUnion, S, T);
	if (zRes)
		return(zRes);
	else
	{
		DdNode *zSet0, *zSet1, *zRes0, *zRes1;
		if ( TopS == TopT )
		{
			/* compute maximal for subsets without the top-most element */
			zSet0 = extraZddMinUnion(dd, cuddE(S), cuddE(T) );
			if ( zSet0 == NULL )
				return NULL;
			cuddRef( zSet0 );

			/* compute maximal for subsets with the top-most element */
			zSet1 = extraZddMinUnion(dd, cuddT(S), cuddT(T) );
			if ( zSet1 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zSet0);
				return NULL;
			}
			cuddRef( zSet1 );
		}
		else /* if ( TopS < TopT ) */
		{
			/* compute maximal for subsets without the top-most element */
			zSet0 = extraZddMinUnion(dd, cuddE(S), T );
			if ( zSet0 == NULL )
				return NULL;
			cuddRef( zSet0 );

			/* subset with this element is just the cofactor of S */
			zSet1 = cuddT(S);
			cuddRef( zSet1 );
		}

		/* subset without this element remains unchanged */
		zRes0 = zSet0;

		/* remove subsets with this element that contain subsets without this element */
		zRes1 = extraZddNotSupSet(dd, zSet1, zSet0);
		if ( zRes1 == NULL )
		{
			Cudd_RecursiveDerefZdd(dd, zSet0);
			Cudd_RecursiveDerefZdd(dd, zSet1);
			return NULL;
		}
		cuddRef( zRes1 );
		Cudd_RecursiveDerefZdd(dd, zSet1);

		/* create the new node */
		zRes = cuddZddGetNode( dd, S->index, zRes1, zRes0 );
		if ( zRes == NULL ) 
		{
			Cudd_RecursiveDerefZdd( dd, zRes0 );
			Cudd_RecursiveDerefZdd( dd, zRes1 );
			return NULL;
		}
		cuddDeref( zRes0 );
		cuddDeref( zRes1 );

		/* insert the result into cache */
		cuddCacheInsert2(dd, extraZddMinUnion, S, T, zRes);
		return zRes;
	}
} /* end of extraZddMinUnion */


/**Function********************************************************************

  Synopsis [Performs the recursive step of Extra_zddDotProduct.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode	*
extraZddDotProduct(
  DdManager * dd,
  DdNode * S,
  DdNode * T)
{
	DdNode *zRes;
	int TopS, TopT;
    statLine(dd); 

	/* consider terminal cases */
	if ( S == z0 || T == z0 )
		return z0;
	if ( S == z1 )
		return T;
	if ( T == z1 )
		return S;

	/* the operation is commutative - normalize the problem */
	TopS = dd->permZ[S->index];
	TopT = dd->permZ[T->index];

	if ( TopS > TopT || (TopS == TopT && S > T) )
		return extraZddDotProduct(dd, T, S);

    /* check cache */
	zRes = cuddCacheLookup2Zdd(dd, extraZddDotProduct, S, T);
	if (zRes)
		return zRes;
	else
	{
		DdNode *zSet0, *zSet1, *zRes0, *zRes1, *zTemp;
		if ( TopS == TopT )
		{
			/* compute the union of two cofactors of T (T0+T1) */
			zTemp = cuddZddUnion(dd, cuddE(T), cuddT(T) );
			if ( zTemp == NULL )
				return NULL;
			cuddRef( zTemp );

			/* compute DotProduct with the top element for subsets (S1, T0+T1) */
			zSet0 = extraZddDotProduct(dd, cuddT(S), zTemp );
			if ( zSet0 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zTemp);
				return NULL;
			}
			cuddRef( zSet0 );
			Cudd_RecursiveDerefZdd(dd, zTemp);

			/* compute DotProduct with the top element for subsets (S0, T1) */
			zSet1 = extraZddDotProduct(dd, cuddE(S), cuddT(T) );
			if ( zSet1 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zSet0);
				return NULL;
			}
			cuddRef( zSet1 );

			/* compute the union of these two partial results (zSet0 + zSet1) */
			zRes1 = cuddZddUnion(dd, zSet0, zSet1 );
			if ( zRes1 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zSet0);
				Cudd_RecursiveDerefZdd(dd, zSet1);
				return NULL;
			}
			cuddRef( zRes1 );
			Cudd_RecursiveDerefZdd(dd, zSet0);
			Cudd_RecursiveDerefZdd(dd, zSet1);

			/* compute DotProduct for subsets without the top-most element */
			zRes0 = extraZddDotProduct(dd, cuddE(S), cuddE(T) );
			if ( zRes0 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zRes1);
				return NULL;
			}
			cuddRef( zRes0 );
		}
		else /* if ( TopS < TopT ) */
		{
			/* compute DotProduct with the top element for subsets (S1, T) */
			zRes1 = extraZddDotProduct(dd, cuddT(S), T );
			if ( zRes1 == NULL )
				return NULL;
			cuddRef( zRes1 );

			/* compute DotProduct for subsets without the top-most element */
			zRes0 = extraZddDotProduct(dd, cuddE(S), T );
			if ( zRes0 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zRes1);
				return NULL;
			}
			cuddRef( zRes0 );
		}

		/* create the new node */
		zRes = cuddZddGetNode( dd, S->index, zRes1, zRes0 );
		if ( zRes == NULL ) 
		{
			Cudd_RecursiveDerefZdd( dd, zRes0 );
			Cudd_RecursiveDerefZdd( dd, zRes1 );
			return NULL;
		}
		cuddDeref( zRes0 );
		cuddDeref( zRes1 );

		/* insert the result into cache */
		cuddCacheInsert2(dd, extraZddDotProduct, S, T, zRes);
		return zRes;
	}
} /* end of extraZddDotProduct */


/**Function********************************************************************

  Synopsis [Performs the recursive step of Extra_zddCrossProduct.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode	*
extraZddCrossProduct(
  DdManager * dd,
  DdNode * S,
  DdNode * T)
{
	DdNode *zRes;
	int TopS, TopT;
    statLine(dd); 

	/* consider terminal cases */
	if ( S == z0 || T == z0 )
		return z0;
	if ( S == z1 || T == z1 )
		return z1;

	/* the operation is commutative - normalize the problem */
	TopS = dd->permZ[S->index];
	TopT = dd->permZ[T->index];

	if ( TopS > TopT || (TopS == TopT && S > T) )
		return extraZddCrossProduct(dd, T, S);

    /* check cache */
	zRes = cuddCacheLookup2Zdd(dd, extraZddCrossProduct, S, T);
	if (zRes)
		return zRes;
	else
	{
		DdNode *zSet0, *zSet1, *zRes0, *zRes1, *zTemp;

		if ( TopS == TopT )
		{
			/* compute the union of two cofactors of T (T0+T1) */
			zTemp = cuddZddUnion(dd, cuddE(T), cuddT(T) );
			if ( zTemp == NULL )
				return NULL;
			cuddRef( zTemp );

			/* compute CrossProduct without the top element for subsets (S0, T0+T1) */
			zSet0 = extraZddCrossProduct(dd, cuddE(S), zTemp );
			if ( zSet0 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zTemp);
				return NULL;
			}
			cuddRef( zSet0 );
			Cudd_RecursiveDerefZdd(dd, zTemp);

			/* compute CrossProduct without the top element for subsets (S1, T0) */
			zSet1 = extraZddCrossProduct(dd, cuddT(S), cuddE(T) );
			if ( zSet1 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zSet0);
				return NULL;
			}
			cuddRef( zSet1 );

			/* compute the union of these two partial results (zSet0 + zSet1) */
			zRes0 = cuddZddUnion(dd, zSet0, zSet1 );
			if ( zRes0 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zSet0);
				Cudd_RecursiveDerefZdd(dd, zSet1);
				return NULL;
			}
			cuddRef( zRes0 );
			Cudd_RecursiveDerefZdd(dd, zSet0);
			Cudd_RecursiveDerefZdd(dd, zSet1);

			/* compute CrossProduct for subsets with the top-most element */
			zRes1 = extraZddCrossProduct(dd, cuddT(S), cuddT(T) );
			if ( zRes1 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zRes0);
				return NULL;
			}
			cuddRef( zRes1 );

			/* create the new node */
			zRes = cuddZddGetNode( dd, S->index, zRes1, zRes0 );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				return NULL;
			}
			cuddDeref( zRes0 );
			cuddDeref( zRes1 );
		}
		else /* if ( TopS < TopT ) */
		{
			/* compute CrossProduct without the top element (S0, T) */
			zSet0 = extraZddCrossProduct(dd, cuddE(S), T );
			if ( zSet0 == NULL )
				return NULL;
			cuddRef( zSet0 );

			/* compute CrossProduct without the top element (S1, T) */
			zSet1 = extraZddCrossProduct(dd, cuddT(S), T );
			if ( zSet1 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zSet0);
				return NULL;
			}
			cuddRef( zSet1 );

			/* compute the union of these two partial results (zSet0 + zSet1) */
			zRes = cuddZddUnion(dd, zSet0, zSet1 );
			if ( zRes == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zSet0);
				Cudd_RecursiveDerefZdd(dd, zSet1);
				return NULL;
			}
			cuddRef( zRes );
			Cudd_RecursiveDerefZdd(dd, zSet0);
			Cudd_RecursiveDerefZdd(dd, zSet1);
			cuddDeref( zRes );
		}

		/* insert the result into cache */
		cuddCacheInsert2(dd, extraZddCrossProduct, S, T, zRes);
		return zRes;
	}
} /* end of extraZddCrossProduct */


/**Function********************************************************************

  Synopsis [Performs the recursive step of Extra_zddMaxDotProduct.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode	*
extraZddMaxDotProduct(
  DdManager * dd,
  DdNode * S,
  DdNode * T)
{
	DdNode *zRes;
	int TopS, TopT;
    statLine(dd); 

	/* consider terminal cases */
	if ( S == z0 || T == z0 )
		return z0;
	if ( S == z1 )
		return T;
	if ( T == z1 )
		return S;

	/* the operation is commutative - normalize the problem */
	TopS = dd->permZ[S->index];
	TopT = dd->permZ[T->index];

	if ( TopS > TopT || (TopS == TopT && S > T) )
		return extraZddMaxDotProduct(dd, T, S);

    /* check cache */
	zRes = cuddCacheLookup2Zdd(dd, extraZddMaxDotProduct, S, T);
	if (zRes)
		return zRes;
	else
	{
		DdNode *zSet0, *zSet1, *zRes0, *zRes1, *zTemp;
		if ( TopS == TopT )
		{
			/* compute the union of two cofactors of T (T0+T1) */
			zTemp = extraZddMaxUnion(dd, cuddE(T), cuddT(T) );
			if ( zTemp == NULL )
				return NULL;
			cuddRef( zTemp );

			/* compute MaxDotProduct with the top element for subsets (S1, T0+T1) */
			zSet0 = extraZddMaxDotProduct(dd, cuddT(S), zTemp );
			if ( zSet0 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zTemp);
				return NULL;
			}
			cuddRef( zSet0 );
			Cudd_RecursiveDerefZdd(dd, zTemp);

			/* compute MaxDotProduct with the top element for subsets (S0, T1) */
			zSet1 = extraZddMaxDotProduct(dd, cuddE(S), cuddT(T) );
			if ( zSet1 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zSet0);
				return NULL;
			}
			cuddRef( zSet1 );

			/* compute the union of these two partial results (zSet0 + zSet1) */
			zRes1 = extraZddMaxUnion(dd, zSet0, zSet1 );
			if ( zRes1 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zSet0);
				Cudd_RecursiveDerefZdd(dd, zSet1);
				return NULL;
			}
			cuddRef( zRes1 );
			Cudd_RecursiveDerefZdd(dd, zSet0);
			Cudd_RecursiveDerefZdd(dd, zSet1);

			/* compute MaxDotProduct for subsets without the top-most element */
			zRes0 = extraZddMaxDotProduct(dd, cuddE(S), cuddE(T) );
			if ( zRes0 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zRes1);
				return NULL;
			}
			cuddRef( zRes0 );
		}
		else /* if ( TopS < TopT ) */
		{
			/* compute MaxDotProduct with the top element for subsets (S1, T) */
			zRes1 = extraZddMaxDotProduct(dd, cuddT(S), T );
			if ( zRes1 == NULL )
				return NULL;
			cuddRef( zRes1 );

			/* compute MaxDotProduct for subsets without the top-most element */
			zRes0 = extraZddMaxDotProduct(dd, cuddE(S), T );
			if ( zRes0 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zRes1);
				return NULL;
			}
			cuddRef( zRes0 );
		}

		/* remove subsets without this element covered by subsets with this element */
		zRes0 = extraZddNotSubSet(dd, zTemp = zRes0, zRes1);
		if ( zRes0 == NULL )
		{
			Cudd_RecursiveDerefZdd(dd, zTemp);
			Cudd_RecursiveDerefZdd(dd, zRes1);
			return NULL;
		}
		cuddRef( zRes0 );
		Cudd_RecursiveDerefZdd(dd, zTemp);

		/* create the new node */
		zRes = cuddZddGetNode( dd, S->index, zRes1, zRes0 );
		if ( zRes == NULL ) 
		{
			Cudd_RecursiveDerefZdd( dd, zRes0 );
			Cudd_RecursiveDerefZdd( dd, zRes1 );
			return NULL;
		}
		cuddDeref( zRes0 );
		cuddDeref( zRes1 );

		/* insert the result into cache */
		cuddCacheInsert2(dd, extraZddMaxDotProduct, S, T, zRes);
		return zRes;
	}
} /* end of extraZddMaxDotProduct */

/*---------------------------------------------------------------------------*/
/* Definition of staTc Functions                                            */
/*---------------------------------------------------------------------------*/

ABC_NAMESPACE_IMPL_END
