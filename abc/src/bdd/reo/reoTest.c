/**CFile****************************************************************

  FileName    [reoTest.c]

  PackageName [REO: A specialized DD reordering engine.]

  Synopsis    [Various testing procedures (may be outdated).]

  Author      [Alan Mishchenko <alanmi@ece.pdx.edu>]
  
  Affiliation [ECE Department. Portland State University, Portland, Oregon.]

  Date        [Ver. 1.0. Started - October 15, 2002.]

  Revision    [$Id: reoTest.c,v 1.0 2002/15/10 03:00:00 alanmi Exp $]

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

  Synopsis    [Reorders the DD using REO and CUDD.]

  Description [This function can be used to test the performance of the reordering package.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_ReorderTest( DdManager * dd, DdNode * Func )
{
	reo_man * pReo;
	DdNode * Temp, * Temp1;
	int pOrder[1000];

	pReo = Extra_ReorderInit( 100, 100 );

//Extra_DumpDot( dd, &Func, 1, "beforReo.dot", 0 );
	Temp  = Extra_Reorder( pReo, dd, Func, pOrder );  Cudd_Ref( Temp );
//Extra_DumpDot( dd, &Temp, 1, "afterReo.dot", 0 );

	Temp1 = Extra_ReorderCudd(dd, Func, NULL );           Cudd_Ref( Temp1 );
printf( "Initial = %d. Final = %d. Cudd = %d.\n", Cudd_DagSize(Func), Cudd_DagSize(Temp), Cudd_DagSize(Temp1)  );
	Cudd_RecursiveDeref( dd, Temp1 );
	Cudd_RecursiveDeref( dd, Temp );
 
	Extra_ReorderQuit( pReo );
}


/**Function*************************************************************

  Synopsis    [Reorders the DD using REO and CUDD.]

  Description [This function can be used to test the performance of the reordering package.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_ReorderTestArray( DdManager * dd, DdNode * Funcs[], int nFuncs )
{
	reo_man * pReo;
	DdNode * FuncsRes[1000];
	int pOrder[1000];
	int i;

	pReo = Extra_ReorderInit( 100, 100 );
	Extra_ReorderArray( pReo, dd, Funcs, FuncsRes, nFuncs, pOrder );  
	Extra_ReorderQuit( pReo );

printf( "Initial = %d. Final = %d.\n", Cudd_SharingSize(Funcs,nFuncs), Cudd_SharingSize(FuncsRes,nFuncs) );

	for ( i = 0; i < nFuncs; i++ )
		Cudd_RecursiveDeref( dd, FuncsRes[i] );

}

/**Function*************************************************************

  Synopsis    [Reorders the DD using CUDD package.]

  Description [Transfers the DD into a temporary manager in such a way
  that the level correspondence is preserved. Reorders the manager
  and transfers the DD back into the original manager using the topmost
  levels of the manager, in such a way that the ordering of levels is
  preserved. The resulting permutation is returned in the array
  given by the user.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Extra_ReorderCudd( DdManager * dd, DdNode * aFunc, int pPermuteReo[] )
{
	static DdManager * ddReorder = NULL;
	static int * Permute     = NULL;
	static int * PermuteReo1 = NULL;
	static int * PermuteReo2 = NULL;
	DdNode * aFuncReorder, * aFuncNew;
	int lev, var;

	// start the reordering manager
	if ( ddReorder == NULL )
	{
		Permute       = ABC_ALLOC( int, dd->size );
		PermuteReo1   = ABC_ALLOC( int, dd->size );
		PermuteReo2   = ABC_ALLOC( int, dd->size );
		ddReorder = Cudd_Init( dd->size, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
		Cudd_AutodynDisable(ddReorder);
	}

	// determine the permutation of variable to make sure that var order in bFunc
	// will not change when this function is transfered into the new manager
	for ( lev = 0; lev < dd->size; lev++ )
	{
		Permute[ dd->invperm[lev] ] = ddReorder->invperm[lev];
		PermuteReo1[ ddReorder->invperm[lev] ] = dd->invperm[lev];
	}
	// transfer this function into the new manager in such a way that ordering of vars does not change
	aFuncReorder = Extra_TransferPermute( dd, ddReorder, aFunc, Permute );  Cudd_Ref( aFuncReorder );
//	assert( Cudd_DagSize(aFunc) == Cudd_DagSize(aFuncReorder)  );

	// perform the reordering
printf( "Nodes before = %d.\n", Cudd_DagSize(aFuncReorder) );
	Cudd_ReduceHeap( ddReorder, CUDD_REORDER_SYMM_SIFT, 1 );
printf( "Nodes before = %d.\n", Cudd_DagSize(aFuncReorder) );

	// determine the reverse variable permutation
	for ( lev = 0; lev < dd->size; lev++ )
	{
		Permute[ ddReorder->invperm[lev] ] = dd->invperm[lev];
		PermuteReo2[ dd->invperm[lev] ] = ddReorder->invperm[lev];
	}

	// transfer this function into the new manager in such a way that ordering of vars does not change
	aFuncNew = Extra_TransferPermute( ddReorder, dd, aFuncReorder, Permute );  Cudd_Ref( aFuncNew );
//	assert( Cudd_DagSize(aFuncNew) == Cudd_DagSize(aFuncReorder)  );
	Cudd_RecursiveDeref( ddReorder, aFuncReorder );

	// derive the resulting variable ordering
	if ( pPermuteReo )
		for ( var = 0; var < dd->size; var++ )
			pPermuteReo[var] = PermuteReo1[ PermuteReo2[var] ];

	Cudd_Deref( aFuncNew );
	return aFuncNew;
}


/**Function*************************************************************

  Synopsis    []

  Description [Transfers the BDD into another manager minimizes it and 
  returns the min number of nodes; disposes of the BDD in the new manager.
  Useful for debugging or comparing the performance of other reordering
  procedures.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_bddReorderTest( DdManager * dd, DdNode * bF )
{
	static DdManager * s_ddmin;
	DdNode * bFmin;
	int  nNodes;
//	abctime clk1;

	if ( s_ddmin == NULL )
		s_ddmin = Cudd_Init( dd->size, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);

//	Cudd_ShuffleHeap( s_ddmin, dd->invperm );

//	clk1 = Abc_Clock();
	bFmin = Cudd_bddTransfer( dd, s_ddmin, bF );  Cudd_Ref( bFmin );
	Cudd_ReduceHeap(s_ddmin,CUDD_REORDER_SIFT,1);
//	Cudd_ReduceHeap(s_ddmin,CUDD_REORDER_SYMM_SIFT,1);
	nNodes = Cudd_DagSize( bFmin );
	Cudd_RecursiveDeref( s_ddmin, bFmin );

//	printf( "Classical variable reordering time = %.2f sec\n", (float)(Abc_Clock() - clk1)/(float)(CLOCKS_PER_SEC) );
	return nNodes;
}

/**Function*************************************************************

  Synopsis    []

  Description [Transfers the ADD into another manager minimizes it and 
  returns the min number of nodes; disposes of the BDD in the new manager.
  Useful for debugging or comparing the performance of other reordering
  procedures.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_addReorderTest( DdManager * dd, DdNode * aF )
{
	static DdManager * s_ddmin;
	DdNode * bF;
	DdNode * bFmin;
	DdNode * aFmin;
	int  nNodesBeg;
	int  nNodesEnd;
	abctime clk1;

	if ( s_ddmin == NULL )
		s_ddmin = Cudd_Init( dd->size, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);

//	Cudd_ShuffleHeap( s_ddmin, dd->invperm );

	clk1 = Abc_Clock();
	bF    = Cudd_addBddPattern( dd, aF );         Cudd_Ref( bF );
	bFmin = Cudd_bddTransfer( dd, s_ddmin, bF );  Cudd_Ref( bFmin );
	Cudd_RecursiveDeref( dd, bF );
	aFmin = Cudd_BddToAdd( s_ddmin, bFmin );      Cudd_Ref( aFmin );
	Cudd_RecursiveDeref( s_ddmin, bFmin );

	nNodesBeg = Cudd_DagSize( aFmin );
	Cudd_ReduceHeap(s_ddmin,CUDD_REORDER_SIFT,1);
//	Cudd_ReduceHeap(s_ddmin,CUDD_REORDER_SYMM_SIFT,1);
	nNodesEnd = Cudd_DagSize( aFmin );
	Cudd_RecursiveDeref( s_ddmin, aFmin );

	printf( "Classical reordering of ADDs: Before = %d. After = %d.\n", nNodesBeg, nNodesEnd );
	printf( "Classical variable reordering time = %.2f sec\n", (float)(Abc_Clock() - clk1)/(float)(CLOCKS_PER_SEC) );
	return nNodesEnd;
}


////////////////////////////////////////////////////////////////////////
///                         END OF FILE                              ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END

