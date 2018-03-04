/**CFile****************************************************************

  FileName    [reoShuffle.c]

  PackageName [REO: A specialized DD reordering engine.]

  Synopsis    [Implementation of the two-variable swap.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - October 15, 2002.]

  Revision    [$Id: reoShuffle.c,v 1.0 2002/15/10 03:00:00 alanmi Exp $]

***********************************************************************/

#include "reo.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [This procedure is similar to Cudd_ShuffleHeap() and Cudd_bddPermute().]

  Description [The first argument is the REO manager. The 2nd/3d
  arguments are the function and its CUDD manager. The last argument
  is the permutation to be implemented. The i-th entry of the permutation 
  array contains the index of the variable that should be brought to the 
  i-th level.  The size of the array should be equal or greater to 
  the number of variables currently in use (that is, the size of CUDD
  manager and the size of REO manager).]

  SideEffects [Note that the resulting BDD is not referenced.]

  SeeAlso     []

***********************************************************************/
DdNode * reoShuffle( reo_man * p, DdManager * dd, DdNode * bFunc, int * pPerm, int * pPermInv )
{
    DdNode * bFuncRes = NULL;
    int i, k, v;

    if ( Cudd_IsConstant(bFunc) )
        return bFunc;

	// set the initial parameters
	p->dd    = dd;
    p->nSupp = Cudd_SupportSize( dd, bFunc );
	p->nTops = 1;
//    p->nNodesBeg = Cudd_DagSize( bFunc );

    // set the starting permutation
	for ( i = 0; i < p->nSupp; i++ )
    {
        p->pOrderInt[i] = i;
		p->pMapToPlanes[ dd->invperm[i] ] = i;
		p->pMapToDdVarsFinal[i] = dd->invperm[i];
    }

	// set the initial parameters
	p->nUnitsUsed = 0;
	p->nNodesCur  = 0;
	p->fThisIsAdd = 0;
	p->Signature++;

	// transfer the function from the CUDD package into REO's internal data structure
    p->pTops[0] = reoTransferNodesToUnits_rec( p, bFunc );
//	assert( p->nNodesBeg == p->nNodesCur );

    // reorder one variable at a time
    for ( i = 0; i < p->nSupp; i++ )
    {
        if ( p->pOrderInt[i] == pPerm[i] )
            continue;
        // find where is variable number pPerm[i]
        for ( k = i + 1; k < p->nSupp; k++ )
            if ( pPerm[i] == p->pOrderInt[k] )
                break;
        if ( k == p->nSupp )
        {
            printf( "reoShuffle() Error: Cannot find a variable.\n" );
            goto finish;
        }
        // move the variable up
        for ( v = k - 1; v >= i; v-- )
        {
            reoReorderSwapAdjacentVars( p, v, 1 );
            // check if the number of nodes is not too large
            if ( p->nNodesCur > 10000 )
            {
                printf( "reoShuffle() Error: BDD size is too large.\n" );
                goto finish;
            }
        }
        assert( p->pOrderInt[i] == pPerm[i] );
    }

	// set the initial parameters
	p->nRefNodes  = 0;
	p->nNodesCur  = 0;
	p->Signature++;
	// transfer the BDDs from REO's internal data structure to CUDD
    bFuncRes = reoTransferUnitsToNodes_rec( p, p->pTops[0] ); Cudd_Ref( bFuncRes );
	// undo the DDs referenced for storing in the cache
	for ( i = 0; i < p->nRefNodes; i++ )
		Cudd_RecursiveDeref( dd, p->pRefNodes[i] );

	// verify zero refs of the terminal nodes
//    assert( reoRecursiveDeref( p->pTops[0] ) );
//    assert( reoCheckZeroRefs( &(p->pPlanes[p->nSupp]) ) );

    // perform verification
	if ( p->fVerify )
	{
		DdNode * bFuncPerm;
		bFuncPerm = Cudd_bddPermute( dd, bFunc, pPermInv );  Cudd_Ref( bFuncPerm );
		if ( bFuncPerm != bFuncRes )
		{
			printf( "REO: Internal verification has failed!\n" );
			fflush( stdout );
		}
		Cudd_RecursiveDeref( dd, bFuncPerm );
	}

	// recycle the data structure
	for ( i = 0; i <= p->nSupp; i++ )
		reoUnitsRecycleUnitList( p, p->pPlanes + i );

finish :
    if ( bFuncRes )
        Cudd_Deref( bFuncRes );
    return bFuncRes;
}



/**Function*************************************************************

  Synopsis    [Reorders the DD using REO and CUDD.]

  Description [This function can be used to test the performance of the reordering package.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_ShuffleTest( reo_man * pReo, DdManager * dd, DdNode * Func )
{
//    extern int runtime1, runtime2;

	DdNode * Temp, * bRemap;
    int nSuppSize, OffSet, Num, i;
    abctime clk;
	int pOrder[1000], pOrderInv[1000];
    assert( dd->size < 1000 );

    srand( 0x12341234 );
    nSuppSize = Cudd_SupportSize( dd, Func );
    if ( nSuppSize < 2 )
        return;

    for ( i = 0; i < nSuppSize; i++ )
        pOrder[i] = i;
    for ( i = 0; i < 120; i++ )
    {
        OffSet = rand() % (nSuppSize - 1);
        Num = pOrder[OffSet];
        pOrder[OffSet] = pOrder[OffSet+1];
        pOrder[OffSet+1] = Num;
    }
    for ( i = 0; i < nSuppSize; i++ )
        pOrderInv[pOrder[i]] = i;

/*
    printf( "Permutation: " );
    for ( i = 0; i < nSuppSize; i++ )
        printf( "%d ", pOrder[i] );
    printf( "\n" );
    printf( "Inverse permutation: " );
    for ( i = 0; i < nSuppSize; i++ )
        printf( "%d ", pOrderInv[i] );
    printf( "\n" );
*/

    // create permutation
//    Extra_ReorderSetVerification( pReo, 1 );
    bRemap = Extra_bddRemapUp( dd, Func );  Cudd_Ref( bRemap );

clk = Abc_Clock();
	Temp  = reoShuffle( pReo, dd, bRemap, pOrder, pOrderInv );  Cudd_Ref( Temp );
//runtime1 += Abc_Clock() - clk;

//printf( "Initial = %d. Final = %d.\n", Cudd_DagSize(bRemap), Cudd_DagSize(Temp)  );

	{
		DdNode * bFuncPerm;
clk = Abc_Clock();
		bFuncPerm = Cudd_bddPermute( dd, bRemap, pOrderInv );  Cudd_Ref( bFuncPerm );
//runtime2 += Abc_Clock() - clk;
		if ( bFuncPerm != Temp )
		{
			printf( "REO: Internal verification has failed!\n" );
			fflush( stdout );
		}
		Cudd_RecursiveDeref( dd, bFuncPerm );
	}

	Cudd_RecursiveDeref( dd, Temp );
	Cudd_RecursiveDeref( dd, bRemap );
}


////////////////////////////////////////////////////////////////////////
///                         END OF FILE                              ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END

