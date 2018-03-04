/**CFile****************************************************************

  FileName    [extraBddAuto.c]

  PackageName [extra]

  Synopsis    [Computation of autosymmetries.]

  Author      [Alan Mishchenko]

  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - September 1, 2003.]

  Revision    [$Id: extraBddAuto.c,v 1.0 2003/05/21 18:03:50 alanmi Exp $]

***********************************************************************/

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


/*
    LinearSpace(f) = Space(f,f)

    Space(f,g)
	{
		if ( f = const )
		{ 
		    if ( f = g )  return 1;
			else          return 0;
		}
		if ( g = const )  return 0;
		return x' * Space(fx',gx') * Space(fx,gx) + x * Space(fx',gx) * Space(fx,gx');
	}

	Equations(s) = Pos(s) + Neg(s);

	Pos(s)
	{
		if ( s  = 0 )   return 1;
		if ( s  = 1 )   return 0;
		if ( sx'= 0 )   return Pos(sx) + x;
		if ( sx = 0 )   return Pos(sx');
		return 1 * [Pos(sx') & Pos(sx)] + x * [Pos(sx') & Neg(sx)];
	}

	Neg(s)
	{
		if ( s  = 0 )   return 1;
		if ( s  = 1 )   return 0;
		if ( sx'= 0 )   return Neg(sx);
		if ( sx = 0 )   return Neg(sx') + x;
		return 1 * [Neg(sx') & Neg(sx)] + x * [Neg(sx') & Pos(sx)];
	}


    SpaceP(A)
	{
		if ( A = 0 )    return 1;
		if ( A = 1 )    return 1;
		return x' * SpaceP(Ax') * SpaceP(Ax) + x * SpaceP(Ax') * SpaceN(Ax);
	}

    SpaceN(A)
	{
		if ( A = 0 )    return 1;
		if ( A = 1 )    return 0;
		return x' * SpaceN(Ax') * SpaceN(Ax) + x * SpaceN(Ax') * SpaceP(Ax);
	}


    LinInd(A)
	{
		if ( A = const )     return 1;
		if ( !LinInd(Ax') )  return 0;
		if ( !LinInd(Ax)  )  return 0;
		if (  LinSumOdd(Ax')  & LinSumEven(Ax) != 0 )  return 0;
		if (  LinSumEven(Ax') & LinSumEven(Ax) != 0 )  return 0;
		return 1;
	}

	LinSumOdd(A)
	{
		if ( A = 0 )    return 0;                 // Odd0  ---e-- Odd1
		if ( A = 1 )    return 1;                 //       \   o
		Odd0  = LinSumOdd(Ax');  // x is absent   //         \ 
		Even0 = LinSumEven(Ax'); // x is absent   //       /   o  
		Odd1  = LinSumOdd(Ax);   // x is present  // Even0 ---e-- Even1 
		Even1 = LinSumEven(Ax);  // x is absent
		return 1 * [Odd0 + ExorP(Odd0, Even1)] + x * [Odd1 + ExorP(Odd1, Even0)];
	}

	LinSumEven(A)
	{
		if ( A = 0 )    return 0;
		if ( A = 1 )    return 0;
		Odd0  = LinSumOdd(Ax');  // x is absent
		Even0 = LinSumEven(Ax'); // x is absent
		Odd1  = LinSumOdd(Ax);   // x is present
		Even1 = LinSumEven(Ax);  // x is absent
		return 1 * [Even0 + Even1 + ExorP(Even0, Even1)] + x * [ExorP(Odd0, Odd1)];
	}
	
*/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Extra_bddSpaceFromFunctionFast( DdManager * dd, DdNode * bFunc )
{
	int * pSupport;
	int * pPermute;
	int * pPermuteBack;
	DdNode ** pCompose;
	DdNode * bCube, * bTemp;
	DdNode * bSpace, * bFunc1, * bFunc2, * bSpaceShift;
	int nSupp, Counter;
	int i, lev;

	// get the support
	pSupport = ABC_ALLOC( int, ddMax(dd->size,dd->sizeZ) );
	Extra_SupportArray( dd, bFunc, pSupport );
	nSupp = 0;
	for ( i = 0; i < dd->size; i++ )
		if ( pSupport[i] )
			nSupp++;

	// make sure the manager has enough variables
	if ( 2*nSupp > dd->size )
	{
		printf( "Cannot derive linear space, because DD manager does not have enough variables.\n" );
		fflush( stdout );
		ABC_FREE( pSupport );
		return NULL;
	}

	// create the permutation arrays
	pPermute     = ABC_ALLOC( int, dd->size );
	pPermuteBack = ABC_ALLOC( int, dd->size );
	pCompose     = ABC_ALLOC( DdNode *, dd->size );
	for ( i = 0; i < dd->size; i++ )
	{
		pPermute[i]     = i;
		pPermuteBack[i] = i;
		pCompose[i]     = dd->vars[i];   Cudd_Ref( pCompose[i] );
	}

	// remap the function in such a way that the variables are interleaved
	Counter = 0;
	bCube = b1;  Cudd_Ref( bCube );
	for ( lev = 0; lev < dd->size; lev++ )
		if ( pSupport[ dd->invperm[lev] ] )
		{   // var "dd->invperm[lev]" on level "lev" should go to level 2*Counter;
			pPermute[ dd->invperm[lev] ] = dd->invperm[2*Counter];
			// var from level 2*Counter+1 should go back to the place of this var
			pPermuteBack[ dd->invperm[2*Counter+1] ] = dd->invperm[lev];
			// the permutation should be defined in such a way that variable
			// on level 2*Counter is replaced by an EXOR of itself and var on the next level
			Cudd_Deref( pCompose[ dd->invperm[2*Counter] ] );
			pCompose[ dd->invperm[2*Counter] ] = 
				Cudd_bddXor( dd, dd->vars[ dd->invperm[2*Counter] ], dd->vars[ dd->invperm[2*Counter+1] ] );  
			Cudd_Ref( pCompose[ dd->invperm[2*Counter] ] );
			// add this variable to the cube
			bCube = Cudd_bddAnd( dd, bTemp = bCube, dd->vars[ dd->invperm[2*Counter] ] );  Cudd_Ref( bCube );
			Cudd_RecursiveDeref( dd, bTemp );
			// increment the counter
			Counter ++;
		}

	// permute the functions
	bFunc1 = Cudd_bddPermute( dd, bFunc, pPermute );         Cudd_Ref( bFunc1 );
	// compose to gate the function depending on both vars
	bFunc2 = Cudd_bddVectorCompose( dd, bFunc1, pCompose );  Cudd_Ref( bFunc2 );
	// gate the vector space
	// L(a) = ForAll x [ F(x) = F(x+a) ] = Not( Exist x [ F(x) (+) F(x+a) ] )
	bSpaceShift = Cudd_bddXorExistAbstract( dd, bFunc1, bFunc2, bCube );  Cudd_Ref( bSpaceShift );
	bSpaceShift = Cudd_Not( bSpaceShift );
	// permute the space back into the original mapping
	bSpace = Cudd_bddPermute( dd, bSpaceShift, pPermuteBack ); Cudd_Ref( bSpace );
	Cudd_RecursiveDeref( dd, bFunc1 );
	Cudd_RecursiveDeref( dd, bFunc2 );
	Cudd_RecursiveDeref( dd, bSpaceShift );
	Cudd_RecursiveDeref( dd, bCube );

	for ( i = 0; i < dd->size; i++ )
		Cudd_RecursiveDeref( dd, pCompose[i] );
	ABC_FREE( pPermute );
	ABC_FREE( pPermuteBack );
	ABC_FREE( pCompose );
    ABC_FREE( pSupport );

	Cudd_Deref( bSpace );
	return bSpace;
}



/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Extra_bddSpaceFromFunction( DdManager * dd, DdNode * bF, DdNode * bG )
{
    DdNode * bRes;
    do {
		dd->reordered = 0;
		bRes = extraBddSpaceFromFunction( dd, bF, bG );
    } while (dd->reordered == 1);
    return bRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Extra_bddSpaceFromFunctionPos( DdManager * dd, DdNode * bFunc )
{
    DdNode * bRes;
    do {
		dd->reordered = 0;
		bRes = extraBddSpaceFromFunctionPos( dd, bFunc );
    } while (dd->reordered == 1);
    return bRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Extra_bddSpaceFromFunctionNeg( DdManager * dd, DdNode * bFunc )
{
    DdNode * bRes;
    do {
		dd->reordered = 0;
		bRes = extraBddSpaceFromFunctionNeg( dd, bFunc );
    } while (dd->reordered == 1);
    return bRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Extra_bddSpaceCanonVars( DdManager * dd, DdNode * bSpace )
{
    DdNode * bRes;
    do {
		dd->reordered = 0;
		bRes = extraBddSpaceCanonVars( dd, bSpace );
    } while (dd->reordered == 1);
    return bRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Extra_bddSpaceReduce( DdManager * dd, DdNode * bFunc, DdNode * bCanonVars )
{
	DdNode * bNegCube;
	DdNode * bResult;
	bNegCube = Extra_bddSupportNegativeCube( dd, bCanonVars );  Cudd_Ref( bNegCube );
	bResult  = Cudd_Cofactor( dd, bFunc, bNegCube );            Cudd_Ref( bResult );
	Cudd_RecursiveDeref( dd, bNegCube );
	Cudd_Deref( bResult );
	return bResult;
}



/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Extra_bddSpaceEquations( DdManager * dd, DdNode * bSpace )
{
    DdNode * zRes;
	DdNode * zEquPos;
	DdNode * zEquNeg;
	zEquPos = Extra_bddSpaceEquationsPos( dd, bSpace );  Cudd_Ref( zEquPos );
	zEquNeg = Extra_bddSpaceEquationsNeg( dd, bSpace );  Cudd_Ref( zEquNeg );
	zRes    = Cudd_zddUnion( dd, zEquPos, zEquNeg );     Cudd_Ref( zRes );
	Cudd_RecursiveDerefZdd( dd, zEquPos );
	Cudd_RecursiveDerefZdd( dd, zEquNeg );
	Cudd_Deref( zRes );
    return zRes;
}


/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Extra_bddSpaceEquationsPos( DdManager * dd, DdNode * bSpace )
{
    DdNode * zRes;
    do {
		dd->reordered = 0;
		zRes = extraBddSpaceEquationsPos( dd, bSpace );
    } while (dd->reordered == 1);
    return zRes;
}


/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Extra_bddSpaceEquationsNeg( DdManager * dd, DdNode * bSpace )
{
    DdNode * zRes;
    do {
		dd->reordered = 0;
		zRes = extraBddSpaceEquationsNeg( dd, bSpace );
    } while (dd->reordered == 1);
    return zRes;
}


/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Extra_bddSpaceFromMatrixPos( DdManager * dd, DdNode * zA )
{
    DdNode * bRes;
    do {
		dd->reordered = 0;
		bRes = extraBddSpaceFromMatrixPos( dd, zA );
    } while (dd->reordered == 1);
    return bRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Extra_bddSpaceFromMatrixNeg( DdManager * dd, DdNode * zA )
{
    DdNode * bRes;
    do {
		dd->reordered = 0;
		bRes = extraBddSpaceFromMatrixNeg( dd, zA );
    } while (dd->reordered == 1);
    return bRes;
}

/**Function*************************************************************

  Synopsis    [Counts the number of literals in one combination.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_zddLitCountComb( DdManager * dd, DdNode * zComb )
{
	int Counter;
	if ( zComb == z0 )
		return 0;
	Counter = 0;
	for ( ; zComb != z1; zComb = cuddT(zComb) )
		Counter++;
	return Counter;
}

/**Function*************************************************************

  Synopsis    []

  Description [Returns the array of ZDDs with the number equal to the number of 
  vars in the DD manager. If the given var is non-canonical, this array contains
  the referenced ZDD representing literals in the corresponding EXOR equation.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode ** Extra_bddSpaceExorGates( DdManager * dd, DdNode * bFuncRed, DdNode * zEquations )
{
	DdNode ** pzRes;
	int * pVarsNonCan;
	DdNode * zEquRem;
	int iVarNonCan;
	DdNode * zExor, * zTemp;

	// get the set of non-canonical variables
	pVarsNonCan = ABC_ALLOC( int, ddMax(dd->size,dd->sizeZ) );
	Extra_SupportArray( dd, bFuncRed, pVarsNonCan );

	// allocate storage for the EXOR sets
	pzRes = ABC_ALLOC( DdNode *, dd->size );
	memset( pzRes, 0, sizeof(DdNode *) * dd->size );

	// go through all the equations
	zEquRem = zEquations;  Cudd_Ref( zEquRem );
	while ( zEquRem != z0 )
	{
		// extract one product
		zExor = Extra_zddSelectOneSubset( dd, zEquRem );   Cudd_Ref( zExor );
		// remove it from the set
		zEquRem = Cudd_zddDiff( dd, zTemp = zEquRem, zExor );  Cudd_Ref( zEquRem );
		Cudd_RecursiveDerefZdd( dd, zTemp );

		// locate the non-canonical variable
		iVarNonCan = -1;
		for ( zTemp = zExor; zTemp != z1; zTemp = cuddT(zTemp) )
		{
			if ( pVarsNonCan[zTemp->index/2] == 1 )
			{
				assert( iVarNonCan == -1 );
				iVarNonCan = zTemp->index/2;
			}
		}
		assert( iVarNonCan != -1 );

		if ( Extra_zddLitCountComb( dd, zExor ) > 1 )
			pzRes[ iVarNonCan ] = zExor; // takes ref
		else
			Cudd_RecursiveDerefZdd( dd, zExor );
	}
	Cudd_RecursiveDerefZdd( dd, zEquRem );

	ABC_FREE( pVarsNonCan );
	return pzRes;
}


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Performs the recursive steps of Extra_bddSpaceFromFunction.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * extraBddSpaceFromFunction( DdManager * dd, DdNode * bF, DdNode * bG )
{
	DdNode * bRes;
	DdNode * bFR, * bGR;

	bFR = Cudd_Regular( bF ); 
	bGR = Cudd_Regular( bG ); 
	if ( cuddIsConstant(bFR) )
	{
		if ( bF == bG )
			return b1;
		else 
			return b0;
	}
	if ( cuddIsConstant(bGR) )
		return b0;
	// both bFunc and bCore are not constants

	// the operation is commutative - normalize the problem
	if ( (unsigned)(ABC_PTRUINT_T)bF > (unsigned)(ABC_PTRUINT_T)bG )
		return extraBddSpaceFromFunction(dd, bG, bF);


    if ( (bRes = cuddCacheLookup2(dd, extraBddSpaceFromFunction, bF, bG)) )
    	return bRes;
	else
	{
		DdNode * bF0, * bF1;
		DdNode * bG0, * bG1;
		DdNode * bTemp1, * bTemp2;
		DdNode * bRes0, * bRes1;
		int LevelF, LevelG;
		int index;

		LevelF = dd->perm[bFR->index];
		LevelG = dd->perm[bGR->index];
		if ( LevelF <= LevelG )
		{
			index = dd->invperm[LevelF];
			if ( bFR != bF )
			{
				bF0 = Cudd_Not( cuddE(bFR) );
				bF1 = Cudd_Not( cuddT(bFR) );
			}
			else
			{
				bF0 = cuddE(bFR);
				bF1 = cuddT(bFR);
			}
		}
		else
		{
			index = dd->invperm[LevelG];
			bF0 = bF1 = bF;
		}

		if ( LevelG <= LevelF )
		{
			if ( bGR != bG )
			{
				bG0 = Cudd_Not( cuddE(bGR) );
				bG1 = Cudd_Not( cuddT(bGR) );
			}
			else
			{
				bG0 = cuddE(bGR);
				bG1 = cuddT(bGR);
			}
		}
		else
			bG0 = bG1 = bG;

		bTemp1 = extraBddSpaceFromFunction( dd, bF0, bG0 );
		if ( bTemp1 == NULL ) 
			return NULL;
		cuddRef( bTemp1 );

		bTemp2 = extraBddSpaceFromFunction( dd, bF1, bG1 );
		if ( bTemp2 == NULL ) 
		{
			Cudd_RecursiveDeref( dd, bTemp1 );
			return NULL;
		}
		cuddRef( bTemp2 );


		bRes0  = cuddBddAndRecur( dd, bTemp1, bTemp2 );
		if ( bRes0 == NULL )
		{
			Cudd_RecursiveDeref( dd, bTemp1 );
			Cudd_RecursiveDeref( dd, bTemp2 );
			return NULL;
		}
		cuddRef( bRes0 );
		Cudd_RecursiveDeref( dd, bTemp1 );
		Cudd_RecursiveDeref( dd, bTemp2 );


		bTemp1  = extraBddSpaceFromFunction( dd, bF0, bG1 );
		if ( bTemp1 == NULL )
		{
			Cudd_RecursiveDeref( dd, bRes0 );
			return NULL;
		}
		cuddRef( bTemp1 );

		bTemp2  = extraBddSpaceFromFunction( dd, bF1, bG0 );
		if ( bTemp2 == NULL )
		{
			Cudd_RecursiveDeref( dd, bRes0 );
			Cudd_RecursiveDeref( dd, bTemp1 );
			return NULL;
		}
		cuddRef( bTemp2 );

		bRes1  = cuddBddAndRecur( dd, bTemp1, bTemp2 );
		if ( bRes1 == NULL )
		{
			Cudd_RecursiveDeref( dd, bRes0 );
			Cudd_RecursiveDeref( dd, bTemp1 );
			Cudd_RecursiveDeref( dd, bTemp2 );
			return NULL;
		}
		cuddRef( bRes1 );
		Cudd_RecursiveDeref( dd, bTemp1 );
		Cudd_RecursiveDeref( dd, bTemp2 );



		// consider the case when Res0 and Res1 are the same node 
		if ( bRes0 == bRes1 )
			bRes = bRes1;
		// consider the case when Res1 is complemented 
		else if ( Cudd_IsComplement(bRes1) ) 
		{
			bRes = cuddUniqueInter(dd, index, Cudd_Not(bRes1), Cudd_Not(bRes0));
			if ( bRes == NULL ) 
			{
				Cudd_RecursiveDeref(dd,bRes0);
				Cudd_RecursiveDeref(dd,bRes1);
				return NULL;
			}
			bRes = Cudd_Not(bRes);
		} 
		else 
		{
			bRes = cuddUniqueInter( dd, index, bRes1, bRes0 );
			if ( bRes == NULL ) 
			{
				Cudd_RecursiveDeref(dd,bRes0);
				Cudd_RecursiveDeref(dd,bRes1);
				return NULL;
			}
		}
		cuddDeref( bRes0 );
		cuddDeref( bRes1 );
			
		// insert the result into cache 
		cuddCacheInsert2(dd, extraBddSpaceFromFunction, bF, bG, bRes);
		return bRes;
	}
}  /* end of extraBddSpaceFromFunction */



/**Function*************************************************************

  Synopsis    [Performs the recursive step of Extra_bddSpaceFromFunctionPos().]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * extraBddSpaceFromFunctionPos( DdManager * dd, DdNode * bF )
{
	DdNode * bRes, * bFR;
	statLine( dd );

	bFR = Cudd_Regular(bF);
	if ( cuddIsConstant(bFR) )
		return b1;

    if ( (bRes = cuddCacheLookup1(dd, extraBddSpaceFromFunctionPos, bF)) )
    	return bRes;
	else
	{
		DdNode * bF0,   * bF1;
		DdNode * bPos0, * bPos1;
		DdNode * bNeg0, * bNeg1;
		DdNode * bRes0, * bRes1; 

		if ( bFR != bF ) // bF is complemented 
		{
			bF0 = Cudd_Not( cuddE(bFR) );
			bF1 = Cudd_Not( cuddT(bFR) );
		}
		else
		{
			bF0 = cuddE(bFR);
			bF1 = cuddT(bFR);
		}


		bPos0  = extraBddSpaceFromFunctionPos( dd, bF0 );
		if ( bPos0 == NULL )
			return NULL;
		cuddRef( bPos0 );

		bPos1  = extraBddSpaceFromFunctionPos( dd, bF1 );
		if ( bPos1 == NULL )
		{
			Cudd_RecursiveDeref( dd, bPos0 );
			return NULL;
		}
		cuddRef( bPos1 );

		bRes0  = cuddBddAndRecur( dd, bPos0, bPos1 );
		if ( bRes0 == NULL )
		{
			Cudd_RecursiveDeref( dd, bPos0 );
			Cudd_RecursiveDeref( dd, bPos1 );
			return NULL;
		}
		cuddRef( bRes0 );
		Cudd_RecursiveDeref( dd, bPos0 );
		Cudd_RecursiveDeref( dd, bPos1 );


		bNeg0  = extraBddSpaceFromFunctionNeg( dd, bF0 );
		if ( bNeg0 == NULL )
		{
			Cudd_RecursiveDeref( dd, bRes0 );
			return NULL;
		}
		cuddRef( bNeg0 );

		bNeg1  = extraBddSpaceFromFunctionNeg( dd, bF1 );
		if ( bNeg1 == NULL )
		{
			Cudd_RecursiveDeref( dd, bRes0 );
			Cudd_RecursiveDeref( dd, bNeg0 );
			return NULL;
		}
		cuddRef( bNeg1 );

		bRes1  = cuddBddAndRecur( dd, bNeg0, bNeg1 );
		if ( bRes1 == NULL )
		{
			Cudd_RecursiveDeref( dd, bRes0 );
			Cudd_RecursiveDeref( dd, bNeg0 );
			Cudd_RecursiveDeref( dd, bNeg1 );
			return NULL;
		}
		cuddRef( bRes1 );
		Cudd_RecursiveDeref( dd, bNeg0 );
		Cudd_RecursiveDeref( dd, bNeg1 );


		// consider the case when Res0 and Res1 are the same node 
		if ( bRes0 == bRes1 )
			bRes = bRes1;
		// consider the case when Res1 is complemented 
		else if ( Cudd_IsComplement(bRes1) ) 
		{
			bRes = cuddUniqueInter( dd, bFR->index, Cudd_Not(bRes1), Cudd_Not(bRes0) );
			if ( bRes == NULL ) 
			{
				Cudd_RecursiveDeref(dd,bRes0);
				Cudd_RecursiveDeref(dd,bRes1);
				return NULL;
			}
			bRes = Cudd_Not(bRes);
		} 
		else 
		{
			bRes = cuddUniqueInter( dd, bFR->index, bRes1, bRes0 );
			if ( bRes == NULL ) 
			{
				Cudd_RecursiveDeref(dd,bRes0);
				Cudd_RecursiveDeref(dd,bRes1);
				return NULL;
			}
		}
		cuddDeref( bRes0 );
		cuddDeref( bRes1 );

		cuddCacheInsert1( dd, extraBddSpaceFromFunctionPos, bF, bRes );
		return bRes;
	}
}



/**Function*************************************************************

  Synopsis    [Performs the recursive step of Extra_bddSpaceFromFunctionPos().]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * extraBddSpaceFromFunctionNeg( DdManager * dd, DdNode * bF )
{
	DdNode * bRes, * bFR;
	statLine( dd );

	bFR = Cudd_Regular(bF);
	if ( cuddIsConstant(bFR) )
		return b0;

    if ( (bRes = cuddCacheLookup1(dd, extraBddSpaceFromFunctionNeg, bF)) )
    	return bRes;
	else
	{
		DdNode * bF0,   * bF1;
		DdNode * bPos0, * bPos1;
		DdNode * bNeg0, * bNeg1;
		DdNode * bRes0, * bRes1; 

		if ( bFR != bF ) // bF is complemented 
		{
			bF0 = Cudd_Not( cuddE(bFR) );
			bF1 = Cudd_Not( cuddT(bFR) );
		}
		else
		{
			bF0 = cuddE(bFR);
			bF1 = cuddT(bFR);
		}


		bPos0  = extraBddSpaceFromFunctionNeg( dd, bF0 );
		if ( bPos0 == NULL )
			return NULL;
		cuddRef( bPos0 );

		bPos1  = extraBddSpaceFromFunctionNeg( dd, bF1 );
		if ( bPos1 == NULL )
		{
			Cudd_RecursiveDeref( dd, bPos0 );
			return NULL;
		}
		cuddRef( bPos1 );

		bRes0  = cuddBddAndRecur( dd, bPos0, bPos1 );
		if ( bRes0 == NULL )
		{
			Cudd_RecursiveDeref( dd, bPos0 );
			Cudd_RecursiveDeref( dd, bPos1 );
			return NULL;
		}
		cuddRef( bRes0 );
		Cudd_RecursiveDeref( dd, bPos0 );
		Cudd_RecursiveDeref( dd, bPos1 );


		bNeg0  = extraBddSpaceFromFunctionPos( dd, bF0 );
		if ( bNeg0 == NULL )
		{
			Cudd_RecursiveDeref( dd, bRes0 );
			return NULL;
		}
		cuddRef( bNeg0 );

		bNeg1  = extraBddSpaceFromFunctionPos( dd, bF1 );
		if ( bNeg1 == NULL )
		{
			Cudd_RecursiveDeref( dd, bRes0 );
			Cudd_RecursiveDeref( dd, bNeg0 );
			return NULL;
		}
		cuddRef( bNeg1 );

		bRes1  = cuddBddAndRecur( dd, bNeg0, bNeg1 );
		if ( bRes1 == NULL )
		{
			Cudd_RecursiveDeref( dd, bRes0 );
			Cudd_RecursiveDeref( dd, bNeg0 );
			Cudd_RecursiveDeref( dd, bNeg1 );
			return NULL;
		}
		cuddRef( bRes1 );
		Cudd_RecursiveDeref( dd, bNeg0 );
		Cudd_RecursiveDeref( dd, bNeg1 );


		// consider the case when Res0 and Res1 are the same node 
		if ( bRes0 == bRes1 )
			bRes = bRes1;
		// consider the case when Res1 is complemented 
		else if ( Cudd_IsComplement(bRes1) ) 
		{
			bRes = cuddUniqueInter( dd, bFR->index, Cudd_Not(bRes1), Cudd_Not(bRes0) );
			if ( bRes == NULL ) 
			{
				Cudd_RecursiveDeref(dd,bRes0);
				Cudd_RecursiveDeref(dd,bRes1);
				return NULL;
			}
			bRes = Cudd_Not(bRes);
		} 
		else 
		{
			bRes = cuddUniqueInter( dd, bFR->index, bRes1, bRes0 );
			if ( bRes == NULL ) 
			{
				Cudd_RecursiveDeref(dd,bRes0);
				Cudd_RecursiveDeref(dd,bRes1);
				return NULL;
			}
		}
		cuddDeref( bRes0 );
		cuddDeref( bRes1 );

		cuddCacheInsert1( dd, extraBddSpaceFromFunctionNeg, bF, bRes );
		return bRes;
	}
}



/**Function*************************************************************

  Synopsis    [Performs the recursive step of Extra_bddSpaceCanonVars().]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * extraBddSpaceCanonVars( DdManager * dd, DdNode * bF )
{
	DdNode * bRes, * bFR;
	statLine( dd );

	bFR = Cudd_Regular(bF);
	if ( cuddIsConstant(bFR) )
		return bF;

    if ( (bRes = cuddCacheLookup1(dd, extraBddSpaceCanonVars, bF)) )
    	return bRes;
	else
	{
		DdNode * bF0,  * bF1;
		DdNode * bRes, * bRes0; 

		if ( bFR != bF ) // bF is complemented 
		{
			bF0 = Cudd_Not( cuddE(bFR) );
			bF1 = Cudd_Not( cuddT(bFR) );
		}
		else
		{
			bF0 = cuddE(bFR);
			bF1 = cuddT(bFR);
		}

		if ( bF0 == b0 )
		{
			bRes = extraBddSpaceCanonVars( dd, bF1 );
			if ( bRes == NULL )
				return NULL;
		}
		else if ( bF1 == b0 )
		{
			bRes = extraBddSpaceCanonVars( dd, bF0 );
			if ( bRes == NULL )
				return NULL;
		}
		else
		{
			bRes0 = extraBddSpaceCanonVars( dd, bF0 );
			if ( bRes0 == NULL )
				return NULL;
			cuddRef( bRes0 );

			bRes = cuddUniqueInter( dd, bFR->index, bRes0, b0 );
			if ( bRes == NULL ) 
			{
				Cudd_RecursiveDeref( dd,bRes0 );
				return NULL;
			}
			cuddDeref( bRes0 );
		}

		cuddCacheInsert1( dd, extraBddSpaceCanonVars, bF, bRes );
		return bRes;
	}
}

/**Function*************************************************************

  Synopsis    [Performs the recursive step of Extra_bddSpaceEquationsPos().]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * extraBddSpaceEquationsPos( DdManager * dd, DdNode * bF )
{
	DdNode * zRes;
	statLine( dd );

	if ( bF == b0 )
		return z1;
	if ( bF == b1 )
		return z0;
	
    if ( (zRes = cuddCacheLookup1Zdd(dd, extraBddSpaceEquationsPos, bF)) )
    	return zRes;
	else
	{
		DdNode * bFR, * bF0,  * bF1;
		DdNode * zPos0, * zPos1, * zNeg1; 
		DdNode * zRes, * zRes0, * zRes1;

		bFR = Cudd_Regular(bF);
		if ( bFR != bF ) // bF is complemented 
		{
			bF0 = Cudd_Not( cuddE(bFR) );
			bF1 = Cudd_Not( cuddT(bFR) );
		}
		else
		{
			bF0 = cuddE(bFR);
			bF1 = cuddT(bFR);
		}

		if ( bF0 == b0 )
		{
			zRes1 = extraBddSpaceEquationsPos( dd, bF1 );
			if ( zRes1 == NULL )
				return NULL;
			cuddRef( zRes1 );

			// add the current element to the set
			zRes = cuddZddGetNode( dd, 2*bFR->index, z1, zRes1 );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd(dd, zRes1);
				return NULL;
			}
			cuddDeref( zRes1 );
		}
		else if ( bF1 == b0 )
		{
			zRes = extraBddSpaceEquationsPos( dd, bF0 );
			if ( zRes == NULL )
				return NULL;
		}
		else
		{
			zPos0 = extraBddSpaceEquationsPos( dd, bF0 );
			if ( zPos0 == NULL )
				return NULL;
			cuddRef( zPos0 );

			zPos1 = extraBddSpaceEquationsPos( dd, bF1 );
			if ( zPos1 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zPos0);
				return NULL;
			}
			cuddRef( zPos1 );

			zNeg1 = extraBddSpaceEquationsNeg( dd, bF1 );
			if ( zNeg1 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zPos0);
				Cudd_RecursiveDerefZdd(dd, zPos1);
				return NULL;
			}
			cuddRef( zNeg1 );


			zRes0 = cuddZddIntersect( dd, zPos0, zPos1 );
			if ( zRes0 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zNeg1);
				Cudd_RecursiveDerefZdd(dd, zPos0);
				Cudd_RecursiveDerefZdd(dd, zPos1);
				return NULL;
			}
			cuddRef( zRes0 );

			zRes1 = cuddZddIntersect( dd, zPos0, zNeg1 );
			if ( zRes1 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zRes0);
				Cudd_RecursiveDerefZdd(dd, zNeg1);
				Cudd_RecursiveDerefZdd(dd, zPos0);
				Cudd_RecursiveDerefZdd(dd, zPos1);
				return NULL;
			}
			cuddRef( zRes1 );
			Cudd_RecursiveDerefZdd(dd, zNeg1);
			Cudd_RecursiveDerefZdd(dd, zPos0);
			Cudd_RecursiveDerefZdd(dd, zPos1);
			// only zRes0 and zRes1 are refed at this point

			zRes = cuddZddGetNode( dd, 2*bFR->index, zRes1, zRes0 );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd(dd, zRes0);
				Cudd_RecursiveDerefZdd(dd, zRes1);
				return NULL;
			}
			cuddDeref( zRes0 );
			cuddDeref( zRes1 );
		}

		cuddCacheInsert1( dd, extraBddSpaceEquationsPos, bF, zRes );
		return zRes;
	}
}


/**Function*************************************************************

  Synopsis    [Performs the recursive step of Extra_bddSpaceEquationsNev().]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * extraBddSpaceEquationsNeg( DdManager * dd, DdNode * bF )
{
	DdNode * zRes;
	statLine( dd );

	if ( bF == b0 )
		return z1;
	if ( bF == b1 )
		return z0;
	
    if ( (zRes = cuddCacheLookup1Zdd(dd, extraBddSpaceEquationsNeg, bF)) )
    	return zRes;
	else
	{
		DdNode * bFR, * bF0,  * bF1;
		DdNode * zPos0, * zPos1, * zNeg1; 
		DdNode * zRes, * zRes0, * zRes1;

		bFR = Cudd_Regular(bF);
		if ( bFR != bF ) // bF is complemented 
		{
			bF0 = Cudd_Not( cuddE(bFR) );
			bF1 = Cudd_Not( cuddT(bFR) );
		}
		else
		{
			bF0 = cuddE(bFR);
			bF1 = cuddT(bFR);
		}

		if ( bF0 == b0 )
		{
			zRes = extraBddSpaceEquationsNeg( dd, bF1 );
			if ( zRes == NULL )
				return NULL;
		}
		else if ( bF1 == b0 )
		{
			zRes0 = extraBddSpaceEquationsNeg( dd, bF0 );
			if ( zRes0 == NULL )
				return NULL;
			cuddRef( zRes0 );

			// add the current element to the set
			zRes = cuddZddGetNode( dd, 2*bFR->index, z1, zRes0 );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd(dd, zRes0);
				return NULL;
			}
			cuddDeref( zRes0 );
		}
		else
		{
			zPos0 = extraBddSpaceEquationsNeg( dd, bF0 );
			if ( zPos0 == NULL )
				return NULL;
			cuddRef( zPos0 );

			zPos1 = extraBddSpaceEquationsNeg( dd, bF1 );
			if ( zPos1 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zPos0);
				return NULL;
			}
			cuddRef( zPos1 );

			zNeg1 = extraBddSpaceEquationsPos( dd, bF1 );
			if ( zNeg1 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zPos0);
				Cudd_RecursiveDerefZdd(dd, zPos1);
				return NULL;
			}
			cuddRef( zNeg1 );


			zRes0 = cuddZddIntersect( dd, zPos0, zPos1 );
			if ( zRes0 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zNeg1);
				Cudd_RecursiveDerefZdd(dd, zPos0);
				Cudd_RecursiveDerefZdd(dd, zPos1);
				return NULL;
			}
			cuddRef( zRes0 );

			zRes1 = cuddZddIntersect( dd, zPos0, zNeg1 );
			if ( zRes1 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zRes0);
				Cudd_RecursiveDerefZdd(dd, zNeg1);
				Cudd_RecursiveDerefZdd(dd, zPos0);
				Cudd_RecursiveDerefZdd(dd, zPos1);
				return NULL;
			}
			cuddRef( zRes1 );
			Cudd_RecursiveDerefZdd(dd, zNeg1);
			Cudd_RecursiveDerefZdd(dd, zPos0);
			Cudd_RecursiveDerefZdd(dd, zPos1);
			// only zRes0 and zRes1 are refed at this point

			zRes = cuddZddGetNode( dd, 2*bFR->index, zRes1, zRes0 );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd(dd, zRes0);
				Cudd_RecursiveDerefZdd(dd, zRes1);
				return NULL;
			}
			cuddDeref( zRes0 );
			cuddDeref( zRes1 );
		}

		cuddCacheInsert1( dd, extraBddSpaceEquationsNeg, bF, zRes );
		return zRes;
	}
}




/**Function*************************************************************

  Synopsis    [Performs the recursive step of Extra_bddSpaceFromFunctionPos().]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * extraBddSpaceFromMatrixPos( DdManager * dd, DdNode * zA )
{
	DdNode * bRes;
	statLine( dd );

	if ( zA == z0 )
		return b1;
	if ( zA == z1 )
		return b1;

    if ( (bRes = cuddCacheLookup1(dd, extraBddSpaceFromMatrixPos, zA)) )
    	return bRes;
	else
	{
		DdNode * bP0, * bP1;
		DdNode * bN0, * bN1;
		DdNode * bRes0, * bRes1; 

		bP0  = extraBddSpaceFromMatrixPos( dd, cuddE(zA) );
		if ( bP0 == NULL )
			return NULL;
		cuddRef( bP0 );

		bP1  = extraBddSpaceFromMatrixPos( dd, cuddT(zA) );
		if ( bP1 == NULL )
		{
			Cudd_RecursiveDeref( dd, bP0 );
			return NULL;
		}
		cuddRef( bP1 );

		bRes0  = cuddBddAndRecur( dd, bP0, bP1 );
		if ( bRes0 == NULL )
		{
			Cudd_RecursiveDeref( dd, bP0 );
			Cudd_RecursiveDeref( dd, bP1 );
			return NULL;
		}
		cuddRef( bRes0 );
		Cudd_RecursiveDeref( dd, bP0 );
		Cudd_RecursiveDeref( dd, bP1 );


		bN0  = extraBddSpaceFromMatrixPos( dd, cuddE(zA) );
		if ( bN0 == NULL )
		{
			Cudd_RecursiveDeref( dd, bRes0 );
			return NULL;
		}
		cuddRef( bN0 );

		bN1  = extraBddSpaceFromMatrixNeg( dd, cuddT(zA) );
		if ( bN1 == NULL )
		{
			Cudd_RecursiveDeref( dd, bRes0 );
			Cudd_RecursiveDeref( dd, bN0 );
			return NULL;
		}
		cuddRef( bN1 );

		bRes1  = cuddBddAndRecur( dd, bN0, bN1 );
		if ( bRes1 == NULL )
		{
			Cudd_RecursiveDeref( dd, bRes0 );
			Cudd_RecursiveDeref( dd, bN0 );
			Cudd_RecursiveDeref( dd, bN1 );
			return NULL;
		}
		cuddRef( bRes1 );
		Cudd_RecursiveDeref( dd, bN0 );
		Cudd_RecursiveDeref( dd, bN1 );


		// consider the case when Res0 and Res1 are the same node 
		if ( bRes0 == bRes1 )
			bRes = bRes1;
		// consider the case when Res1 is complemented 
		else if ( Cudd_IsComplement(bRes1) ) 
		{
			bRes = cuddUniqueInter( dd, zA->index/2, Cudd_Not(bRes1), Cudd_Not(bRes0) );
			if ( bRes == NULL ) 
			{
				Cudd_RecursiveDeref(dd,bRes0);
				Cudd_RecursiveDeref(dd,bRes1);
				return NULL;
			}
			bRes = Cudd_Not(bRes);
		} 
		else 
		{
			bRes = cuddUniqueInter( dd, zA->index/2, bRes1, bRes0 );
			if ( bRes == NULL ) 
			{
				Cudd_RecursiveDeref(dd,bRes0);
				Cudd_RecursiveDeref(dd,bRes1);
				return NULL;
			}
		}
		cuddDeref( bRes0 );
		cuddDeref( bRes1 );

		cuddCacheInsert1( dd, extraBddSpaceFromMatrixPos, zA, bRes );
		return bRes;
	}
}


/**Function*************************************************************

  Synopsis    [Performs the recursive step of Extra_bddSpaceFromFunctionPos().]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * extraBddSpaceFromMatrixNeg( DdManager * dd, DdNode * zA )
{
	DdNode * bRes;
	statLine( dd );

	if ( zA == z0 )
		return b1;
	if ( zA == z1 )
		return b0;

    if ( (bRes = cuddCacheLookup1(dd, extraBddSpaceFromMatrixNeg, zA)) )
    	return bRes;
	else
	{
		DdNode * bP0, * bP1;
		DdNode * bN0, * bN1;
		DdNode * bRes0, * bRes1; 

		bP0  = extraBddSpaceFromMatrixNeg( dd, cuddE(zA) );
		if ( bP0 == NULL )
			return NULL;
		cuddRef( bP0 );

		bP1  = extraBddSpaceFromMatrixNeg( dd, cuddT(zA) );
		if ( bP1 == NULL )
		{
			Cudd_RecursiveDeref( dd, bP0 );
			return NULL;
		}
		cuddRef( bP1 );

		bRes0  = cuddBddAndRecur( dd, bP0, bP1 );
		if ( bRes0 == NULL )
		{
			Cudd_RecursiveDeref( dd, bP0 );
			Cudd_RecursiveDeref( dd, bP1 );
			return NULL;
		}
		cuddRef( bRes0 );
		Cudd_RecursiveDeref( dd, bP0 );
		Cudd_RecursiveDeref( dd, bP1 );


		bN0  = extraBddSpaceFromMatrixNeg( dd, cuddE(zA) );
		if ( bN0 == NULL )
		{
			Cudd_RecursiveDeref( dd, bRes0 );
			return NULL;
		}
		cuddRef( bN0 );

		bN1  = extraBddSpaceFromMatrixPos( dd, cuddT(zA) );
		if ( bN1 == NULL )
		{
			Cudd_RecursiveDeref( dd, bRes0 );
			Cudd_RecursiveDeref( dd, bN0 );
			return NULL;
		}
		cuddRef( bN1 );

		bRes1  = cuddBddAndRecur( dd, bN0, bN1 );
		if ( bRes1 == NULL )
		{
			Cudd_RecursiveDeref( dd, bRes0 );
			Cudd_RecursiveDeref( dd, bN0 );
			Cudd_RecursiveDeref( dd, bN1 );
			return NULL;
		}
		cuddRef( bRes1 );
		Cudd_RecursiveDeref( dd, bN0 );
		Cudd_RecursiveDeref( dd, bN1 );


		// consider the case when Res0 and Res1 are the same node 
		if ( bRes0 == bRes1 )
			bRes = bRes1;
		// consider the case when Res1 is complemented 
		else if ( Cudd_IsComplement(bRes1) ) 
		{
			bRes = cuddUniqueInter( dd, zA->index/2, Cudd_Not(bRes1), Cudd_Not(bRes0) );
			if ( bRes == NULL ) 
			{
				Cudd_RecursiveDeref(dd,bRes0);
				Cudd_RecursiveDeref(dd,bRes1);
				return NULL;
			}
			bRes = Cudd_Not(bRes);
		} 
		else 
		{
			bRes = cuddUniqueInter( dd, zA->index/2, bRes1, bRes0 );
			if ( bRes == NULL ) 
			{
				Cudd_RecursiveDeref(dd,bRes0);
				Cudd_RecursiveDeref(dd,bRes1);
				return NULL;
			}
		}
		cuddDeref( bRes0 );
		cuddDeref( bRes1 );

		cuddCacheInsert1( dd, extraBddSpaceFromMatrixNeg, zA, bRes );
		return bRes;
	}
}


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

ABC_NAMESPACE_IMPL_END

