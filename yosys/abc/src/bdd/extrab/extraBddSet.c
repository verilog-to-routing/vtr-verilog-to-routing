/**CFile***********************************************************************

  FileName    [zSubSet.c]

  PackageName [extra]

  Synopsis    [Experimental version of some ZDD operators.]

  Description [External procedures included in this module:
		    <ul>
		    <li> Extra_zddSubSet();
		    <li> Extra_zddSupSet();
		    <li> Extra_zddNotSubSet();
		    <li> Extra_zddNotSupSet();
		    <li> Extra_zddMaxNotSupSet();
			<li> Extra_zddEmptyBelongs();
			<li> Extra_zddIsOneSubset();
		    </ul>
	       Internal procedures included in this module:
		    <ul>
		    <li> extraZddSubSet();
		    <li> extraZddSupSet();
		    <li> extraZddNotSubSet();
		    <li> extraZddNotSupSet();
		    <li> extraZddMaxNotSupSet();
		    </ul>
	       Static procedures included in this module:
		    <ul>
		    </ul>

          SubSet, SupSet, NotSubSet, NotSupSet were introduced 
		  by O.Coudert to solve problems arising in two-level SOP 
		  minimization. See O. Coudert, "Two-Level Logic Minimization: 
		  An Overview", Integration. Vol. 17, No. 2, pp. 97-140, Oct 1994.
	      ]

  SeeAlso     []

  Author      [Alan Mishchenko]

  Copyright   []

  Revision    [$zSubSet.c, v.1.2, November 16, 2000, alanmi $]

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


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Computes subsets in X that are contained in some of the subsets of Y.]

  Description []

  SideEffects []

  SeeAlso     [Extra_zddNotSubSet, Extra_zddSupSet, Extra_zddNotSupSet]

******************************************************************************/
DdNode	*
Extra_zddSubSet(
  DdManager * dd,
  DdNode * X,
  DdNode * Y)
{
    DdNode	*res;
    int		autoDynZ;

    autoDynZ = dd->autoDynZ;
    dd->autoDynZ = 0;

    do {
	dd->reordered = 0;
	res = extraZddSubSet(dd, X, Y);
    } while (dd->reordered == 1);
    dd->autoDynZ = autoDynZ;
    return(res);

} /* end of Extra_zddSubSet */


/**Function********************************************************************

  Synopsis    [Computes subsets in X that contain some of the subsets of Y.]

  Description []

  SideEffects []

  SeeAlso     [Extra_zddSubSet, Extra_zddNotSubSet, Extra_zddNotSupSet]

******************************************************************************/
DdNode	*
Extra_zddSupSet(
  DdManager * dd,
  DdNode * X,
  DdNode * Y)
{
    DdNode	*res;
    int		autoDynZ;

    autoDynZ = dd->autoDynZ;
    dd->autoDynZ = 0;

    do {
	dd->reordered = 0;
	res = extraZddSupSet(dd, X, Y);
    } while (dd->reordered == 1);
    dd->autoDynZ = autoDynZ;
    return(res);

} /* end of Extra_zddSupSet */

/**Function********************************************************************

  Synopsis    [Computes subsets in X that are not contained in any of the subsets of Y.]

  Description []

  SideEffects []

  SeeAlso     [Extra_zddSubSet, Extra_zddSupSet, Extra_zddNotSupSet]

******************************************************************************/
DdNode	*
Extra_zddNotSubSet(
  DdManager * dd,
  DdNode * X,
  DdNode * Y)
{
    DdNode	*res;
    int		autoDynZ;

    autoDynZ = dd->autoDynZ;
    dd->autoDynZ = 0;

    do {
	dd->reordered = 0;
	res = extraZddNotSubSet(dd, X, Y);
    } while (dd->reordered == 1);
    dd->autoDynZ = autoDynZ;
    return(res);

} /* end of Extra_zddNotSubSet */


/**Function********************************************************************

  Synopsis    [Computes subsets in X that do not contain any of the subsets of Y.]

  Description []

  SideEffects []

  SeeAlso     [Extra_zddSubSet, Extra_zddSupSet, Extra_zddNotSubSet]

******************************************************************************/
DdNode	*
Extra_zddNotSupSet(
  DdManager * dd,
  DdNode * X,
  DdNode * Y)
{
    DdNode	*res;
    int		autoDynZ;

    autoDynZ = dd->autoDynZ;
    dd->autoDynZ = 0;

    do {
	dd->reordered = 0;
	res = extraZddNotSupSet(dd, X, Y);
    } while (dd->reordered == 1);
    dd->autoDynZ = autoDynZ;
    return(res);

} /* end of Extra_zddNotSupSet */



/**Function********************************************************************

  Synopsis    [Computes the maximal of subsets in X not contained in any of the subsets of Y.]

  Description []

  SideEffects []

  SeeAlso     [Extra_zddSubSet, Extra_zddSupSet, Extra_zddSubSet, Extra_zddNotSupSet]

******************************************************************************/
DdNode	*
Extra_zddMaxNotSupSet(
  DdManager * dd,
  DdNode * X,
  DdNode * Y)
{
    DdNode	*res;
    int		autoDynZ;

    autoDynZ = dd->autoDynZ;
    dd->autoDynZ = 0;

    do {
	dd->reordered = 0;
	res = extraZddMaxNotSupSet(dd, X, Y);
    } while (dd->reordered == 1);
    dd->autoDynZ = autoDynZ;
    return(res);

} /* end of Extra_zddMaxNotSupSet */


/**Function********************************************************************

  Synopsis    [Returns 1 if ZDD contains the empty combination; 0 otherwise.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int 
Extra_zddEmptyBelongs(
  DdManager *dd, 
  DdNode* zS )
{
	while ( zS->index != CUDD_MAXINDEX )
		zS = cuddE( zS );
	return (int)( zS == z1 );

} /* end of Extra_zddEmptyBelongs */


/**Function********************************************************************

  Synopsis    [Returns 1 if the set is empty or consists of one subset only.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int 
Extra_zddIsOneSubset(
  DdManager * dd, 
  DdNode * zS )
{
	while ( zS->index != CUDD_MAXINDEX )
	{
		assert( cuddT(zS) != z0 );
		if ( cuddE(zS) != z0 )
			return 0;
		zS = cuddT( zS );
	}
	return (int)( zS == z1 );

} /* end of Extra_zddEmptyBelongs */


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis [Performs the recursive step of Extra_zddSubSet.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode* extraZddSubSet( DdManager *dd, DdNode *X, DdNode *Y )
{	
	DdNode *zRes;
    statLine(dd); 
	/* any comb is a subset of itself */
	if ( X == Y ) 
		return X;
	/* if X is empty, the result is empty */
	if ( X == z0 ) 
		return z0;
	/* combs in X are notsubsets of non-existant combs in Y */
	if ( Y == z0 ) 
		return z0;
	/* the empty comb is contained in all combs of Y */
	if ( X == z1 ) 
		return z1;
	/* only {()} is the subset of {()} */
	if ( Y == z1 ) /* check whether the empty combination is present in X */ 
		return ( Extra_zddEmptyBelongs( dd, X )? z1: z0 );

    /* check cache */
    zRes = cuddCacheLookup2Zdd(dd, extraZddSubSet, X, Y);
    if (zRes)
    	return(zRes);
	else
	{
		DdNode *zRes0, *zRes1, *zTemp;
		int TopLevelX = dd->permZ[ X->index ];
		int TopLevelY = dd->permZ[ Y->index ];

		if ( TopLevelX < TopLevelY )
		{
			/* compute combs of X without var that are notsubsets of combs with Y */
			zRes = extraZddSubSet( dd, cuddE( X ), Y );
			if ( zRes == NULL )  return NULL;
		}
		else if ( TopLevelX == TopLevelY )
		{
			/* merge combs of Y with and without var */
			zTemp = cuddZddUnion( dd, cuddE( Y ), cuddT( Y ) );
			if ( zTemp == NULL )
				return NULL;
			cuddRef( zTemp );

			/* compute combs of X without var that are notsubsets of combs is Temp */
			zRes0 = extraZddSubSet( dd, cuddE( X ), zTemp );
			if ( zRes0 == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zTemp );
				return NULL;
			}
			cuddRef( zRes0 );
			Cudd_RecursiveDerefZdd( dd, zTemp );

			/* combs of X with var that are notsubsets of combs in Y with var */
			zRes1 = extraZddSubSet( dd, cuddT( X ), cuddT( Y ) );
			if ( zRes1 == NULL )
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				return NULL;
			}
			cuddRef( zRes1 );

			/* compose Res0 and Res1 with the given ZDD variable */
			zRes = cuddZddGetNode( dd, X->index, zRes1, zRes0 );
			if ( zRes == NULL )
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				return NULL;
			}
			cuddDeref( zRes0 );
			cuddDeref( zRes1 );
		}
		else /* if ( TopLevelX > TopLevelY ) */
		{
			/* merge combs of Y with and without var */
			zTemp = cuddZddUnion( dd, cuddE( Y ), cuddT( Y ) );
			if ( zTemp == NULL )  return NULL;
			cuddRef( zTemp );

			/* compute combs that are notsubsets of Temp */
			zRes = extraZddSubSet( dd, X, zTemp );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zTemp );
				return NULL;
			}
			cuddRef( zRes );
			Cudd_RecursiveDerefZdd( dd, zTemp );
			cuddDeref( zRes );
		}

		/* insert the result into cache */
	    cuddCacheInsert2(dd, extraZddSubSet, X, Y, zRes);
		return zRes;
	}
} /* end of extraZddSubSet */


/**Function********************************************************************

  Synopsis [Performs the recursive step of Extra_zddSupSet.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode* extraZddSupSet( DdManager *dd, DdNode *X, DdNode *Y )
{	
	DdNode *zRes;
    statLine(dd); 
	/* any comb is a superset of itself */
	if ( X == Y ) 
		return X;
	/* no comb in X is superset of non-existing combs */
	if ( Y == z0 ) 
		return z0;	
	/* any comb in X is the superset of the empty comb */
	if ( Extra_zddEmptyBelongs( dd, Y ) ) 
		return X;
	/* if X is empty, the result is empty */
	if ( X == z0 ) 
		return z0;
	/* if X is the empty comb (and Y does not contain it!), return empty */
	if ( X == z1 ) 
		return z0;

    /* check cache */
    zRes = cuddCacheLookup2Zdd(dd, extraZddSupSet, X, Y);
    if (zRes)
    	return(zRes);
	else
	{
		DdNode *zRes0, *zRes1, *zTemp;
		int TopLevelX = dd->permZ[ X->index ];
		int TopLevelY = dd->permZ[ Y->index ];

		if ( TopLevelX < TopLevelY )
		{
			/* combinations of X without label that are supersets of combinations with Y */
			zRes0 = extraZddSupSet( dd, cuddE( X ), Y );
			if ( zRes0 == NULL )  return NULL;
			cuddRef( zRes0 );

			/* combinations of X with label that are supersets of combinations with Y */
			zRes1 = extraZddSupSet( dd, cuddT( X ), Y );
			if ( zRes1 == NULL )  
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				return NULL;
			}
			cuddRef( zRes1 );

			/* compose Res0 and Res1 with the given ZDD variable */
			zRes = cuddZddGetNode( dd, X->index, zRes1, zRes0 );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				return NULL;
			}
			cuddDeref( zRes0 );
			cuddDeref( zRes1 );
		}
		else if ( TopLevelX == TopLevelY )
		{
			/* combs of X without var that are supersets of combs of Y without var */
			zRes0 = extraZddSupSet( dd, cuddE( X ), cuddE( Y ) );
			if ( zRes0 == NULL )  return NULL;
			cuddRef( zRes0 );

			/* merge combs of Y with and without var */
			zTemp = cuddZddUnion( dd, cuddE( Y ), cuddT( Y ) );
			if ( zTemp == NULL )  
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				return NULL;
			}
			cuddRef( zTemp );

			/* combs of X with label that are supersets of combs in Temp */
			zRes1 = extraZddSupSet( dd, cuddT( X ), zTemp );
			if ( zRes1 == NULL )  
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zTemp );
				return NULL;
			}
			cuddRef( zRes1 );
			Cudd_RecursiveDerefZdd( dd, zTemp );

			/* compose Res0 and Res1 with the given ZDD variable */
			zRes = cuddZddGetNode( dd, X->index, zRes1, zRes0 );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				return NULL;
			}
			cuddDeref( zRes0 );
			cuddDeref( zRes1 );
		}
		else /* if ( TopLevelX > TopLevelY ) */
		{
			/* combs of X that are supersets of combs of Y without label */
			zRes = extraZddSupSet( dd, X, cuddE( Y ) );
			if ( zRes == NULL )  return NULL;
		}

		/* insert the result into cache */
	    cuddCacheInsert2(dd, extraZddSupSet, X, Y, zRes);
		return zRes;
	}
} /* end of extraZddSupSet */


/**Function********************************************************************

  Synopsis [Performs the recursive step of Extra_zddNotSubSet.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode* extraZddNotSubSet( DdManager *dd, DdNode *X, DdNode *Y )
{	
	DdNode *zRes;
    statLine(dd); 
	/* any comb is a subset of itself */
	if ( X == Y ) 
		return z0;
	/* combs in X are notsubsets of non-existant combs in Y */
	if ( Y == z0 ) 
		return X;	
	/* only {()} is the subset of {()} */
	if ( Y == z1 ) /* remove empty combination from X */
		return cuddZddDiff( dd, X, z1 );
	/* if X is empty, the result is empty */
	if ( X == z0 ) 
		return z0;
	/* the empty comb is contained in all combs of Y */
	if ( X == z1 ) 
		return z0;

    /* check cache */
    zRes = cuddCacheLookup2Zdd(dd, extraZddNotSubSet, X, Y);
    if (zRes)
    	return(zRes);
	else
	{
		DdNode *zRes0, *zRes1, *zTemp;
		int TopLevelX = dd->permZ[ X->index ];
		int TopLevelY = dd->permZ[ Y->index ];

		if ( TopLevelX < TopLevelY )
		{
			/* compute combs of X without var that are notsubsets of combs with Y */
			zRes0 = extraZddNotSubSet( dd, cuddE( X ), Y );
			if ( zRes0 == NULL )  
				return NULL;
			cuddRef( zRes0 );

			/* combs of X with var cannot be subsets of combs without var in Y */
			zRes1 = cuddT( X );

			/* compose Res0 and Res1 with the given ZDD variable */
			zRes = cuddZddGetNode( dd, X->index, zRes1, zRes0 );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				return NULL;
			}
			cuddDeref( zRes0 );
		}
		else if ( TopLevelX == TopLevelY )
		{
			/* merge combs of Y with and without var */
			zTemp = cuddZddUnion( dd, cuddE( Y ), cuddT( Y ) );
			if ( zTemp == NULL )  
				return NULL;
			cuddRef( zTemp );

			/* compute combs of X without var that are notsubsets of combs is Temp */
			zRes0 = extraZddNotSubSet( dd, cuddE( X ), zTemp );
			if ( zRes0 == NULL )  
			{
				Cudd_RecursiveDerefZdd( dd, zTemp );
				return NULL;
			}
			cuddRef( zRes0 );
			Cudd_RecursiveDerefZdd( dd, zTemp );

			/* combs of X with var that are notsubsets of combs in Y with var */
			zRes1 = extraZddNotSubSet( dd, cuddT( X ), cuddT( Y ) );
			if ( zRes1 == NULL )  
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				return NULL;
			}
			cuddRef( zRes1 );

			/* compose Res0 and Res1 with the given ZDD variable */
			zRes = cuddZddGetNode( dd, X->index, zRes1, zRes0 );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				return NULL;
			}
			cuddDeref( zRes0 );
			cuddDeref( zRes1 );
		}
		else /* if ( TopLevelX > TopLevelY ) */
		{
			/* merge combs of Y with and without var */
			zTemp = cuddZddUnion( dd, cuddE( Y ), cuddT( Y ) );
			if ( zTemp == NULL )  
				return NULL;
			cuddRef( zTemp );

			/* compute combs that are notsubsets of Temp */
			zRes = extraZddNotSubSet( dd, X, zTemp );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zTemp );
				return NULL;
			}
			cuddRef( zRes );
			Cudd_RecursiveDerefZdd( dd, zTemp );
			cuddDeref( zRes );
		}

		/* insert the result into cache */
	    cuddCacheInsert2(dd, extraZddNotSubSet, X, Y, zRes);
		return zRes;
	}
} /* end of extraZddNotSubSet */


/**Function********************************************************************

  Synopsis [Performs the recursive step of Extra_zddNotSupSet.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode* extraZddNotSupSet( DdManager *dd, DdNode *X, DdNode *Y )
{	
	DdNode *zRes;
    statLine(dd); 
	/* any comb is a superset of itself */
	if ( X == Y ) 
		return z0;
	/* no comb in X is superset of non-existing combs */
	if ( Y == z0 ) 
		return X;	
	/* any comb in X is the superset of the empty comb */
	if ( Extra_zddEmptyBelongs( dd, Y ) ) 
		return z0;
	/* if X is empty, the result is empty */
	if ( X == z0 ) 
		return z0;
	/* if X is the empty comb (and Y does not contain it!), return it */
	if ( X == z1 ) 
		return z1;

    /* check cache */
    zRes = cuddCacheLookup2Zdd(dd, extraZddNotSupSet, X, Y);
    if (zRes)
    	return(zRes);
	else
	{
		DdNode *zRes0, *zRes1, *zTemp;
		int TopLevelX = dd->permZ[ X->index ];
		int TopLevelY = dd->permZ[ Y->index ];

		if ( TopLevelX < TopLevelY )
		{
			/* combinations of X without label that are supersets of combinations of Y */
			zRes0 = extraZddNotSupSet( dd, cuddE( X ), Y );
			if ( zRes0 == NULL )  
				return NULL;
			cuddRef( zRes0 );

			/* combinations of X with label that are supersets of combinations of Y */
			zRes1 = extraZddNotSupSet( dd, cuddT( X ), Y );
			if ( zRes1 == NULL )  
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				return NULL;
			}
			cuddRef( zRes1 );

			/* compose Res0 and Res1 with the given ZDD variable */
			zRes = cuddZddGetNode( dd, X->index, zRes1, zRes0 );
			if ( zRes == NULL )
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				return NULL;
			}
			cuddDeref( zRes0 );
			cuddDeref( zRes1 );
		}
		else if ( TopLevelX == TopLevelY )
		{
			/* combs of X without var that are not supersets of combs of Y without var */
			zRes0 = extraZddNotSupSet( dd, cuddE( X ), cuddE( Y ) );
			if ( zRes0 == NULL )  
				return NULL;
			cuddRef( zRes0 );

			/* merge combs of Y with and without var */
			zTemp = cuddZddUnion( dd, cuddE( Y ), cuddT( Y ) );
			if ( zTemp == NULL )
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				return NULL;
			}
			cuddRef( zTemp );

			/* combs of X with label that are supersets of combs in Temp */
			zRes1 = extraZddNotSupSet( dd, cuddT( X ), zTemp );
			if ( zRes1 == NULL )
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zTemp );
				return NULL;
			}
			cuddRef( zRes1 );
			Cudd_RecursiveDerefZdd( dd, zTemp );

			/* compose Res0 and Res1 with the given ZDD variable */
			zRes = cuddZddGetNode( dd, X->index, zRes1, zRes0 );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				return NULL;
			}
			cuddDeref( zRes0 );
			cuddDeref( zRes1 );
		}
		else /* if ( TopLevelX > TopLevelY ) */
		{
			/* combs of X that are supersets of combs of Y without label */
			zRes = extraZddNotSupSet( dd, X, cuddE( Y ) );
			if ( zRes == NULL )  return NULL;
		}

		/* insert the result into cache */
	    cuddCacheInsert2(dd, extraZddNotSupSet, X, Y, zRes);
		return zRes;
	}
} /* end of extraZddNotSupSet */



/**Function********************************************************************

  Synopsis [Performs the recursive step of Extra_zddMaxNotSupSet.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode* extraZddMaxNotSupSet( DdManager *dd, DdNode *X, DdNode *Y )
{	
	DdNode *zRes;
    statLine(dd); 
	/* any comb is a superset of itself */
	if ( X == Y ) 
		return z0;
	/* no comb in X is superset of non-existing combs */
	if ( Y == z0 ) 
		return extraZddMaximal( dd, X );	
	/* any comb in X is the superset of the empty comb */
	if ( Extra_zddEmptyBelongs( dd, Y ) ) 
		return z0;
	/* if X is empty, the result is empty */
	if ( X == z0 ) 
		return z0;
	/* if X is the empty comb (and Y does not contain it!), return it */
	if ( X == z1 ) 
		return z1;

    /* check cache */
    zRes = cuddCacheLookup2Zdd(dd, extraZddMaxNotSupSet, X, Y);
    if (zRes)
    	return(zRes);
	else
	{
		DdNode *zRes0, *zRes1, *zTemp;
		int TopLevelX = dd->permZ[ X->index ];
		int TopLevelY = dd->permZ[ Y->index ];

		if ( TopLevelX < TopLevelY )
		{
			/* combinations of X without label that are supersets of combinations with Y */
			zRes0 = extraZddMaxNotSupSet( dd, cuddE( X ), Y );
			if ( zRes0 == NULL )  
				return NULL;
			cuddRef( zRes0 );

			/* combinations of X with label that are supersets of combinations with Y */
			zRes1 = extraZddMaxNotSupSet( dd, cuddT( X ), Y );
			if ( zRes1 == NULL )  
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				return NULL;
			}
			cuddRef( zRes1 );

			/* ---------------------------------------------------- */
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
			/* ---------------------------------------------------- */

			/* compose Res0 and Res1 with the given ZDD variable */
			zRes = cuddZddGetNode( dd, X->index, zRes1, zRes0 );
			if ( zRes == NULL )
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				return NULL;
			}
			cuddDeref( zRes0 );
			cuddDeref( zRes1 );
		}
		else if ( TopLevelX == TopLevelY )
		{
			/* combs of X without var that are supersets of combs of Y without var */
			zRes0 = extraZddMaxNotSupSet( dd, cuddE( X ), cuddE( Y ) );
			if ( zRes0 == NULL )  
				return NULL;
			cuddRef( zRes0 );

			/* merge combs of Y with and without var */
			zTemp = cuddZddUnion( dd, cuddE( Y ), cuddT( Y ) );
			if ( zTemp == NULL )
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				return NULL;
			}
			cuddRef( zTemp );

			/* combs of X with label that are supersets of combs in Temp */
			zRes1 = extraZddMaxNotSupSet( dd, cuddT( X ), zTemp );
			if ( zRes1 == NULL )
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zTemp );
				return NULL;
			}
			cuddRef( zRes1 );
			Cudd_RecursiveDerefZdd( dd, zTemp );

			/* ---------------------------------------------------- */
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
			/* ---------------------------------------------------- */

			/* compose Res0 and Res1 with the given ZDD variable */
			zRes = cuddZddGetNode( dd, X->index, zRes1, zRes0 );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				return NULL;
			}
			cuddDeref( zRes0 );
			cuddDeref( zRes1 );
		}
		else /* if ( TopLevelX > TopLevelY ) */
		{
			/* combs of X that are supersets of combs of Y without label */
			zRes = extraZddMaxNotSupSet( dd, X, cuddE( Y ) );
			if ( zRes == NULL )  return NULL;
		}

		/* insert the result into cache */
	    cuddCacheInsert2(dd, extraZddMaxNotSupSet, X, Y, zRes);
		return zRes;
	}
} /* end of extraZddMaxNotSupSet */


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/


ABC_NAMESPACE_IMPL_END
