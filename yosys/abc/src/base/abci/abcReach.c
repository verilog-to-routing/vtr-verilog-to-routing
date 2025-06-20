/**CFile****************************************************************

  FileName    [abcReach.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Performs reachability analysis.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcReach.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"

#ifdef ABC_USE_CUDD
#include "bdd/extrab/extraBdd.h"
#endif

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

#ifdef ABC_USE_CUDD

/**Function*************************************************************

  Synopsis    [Computes the initial state and sets up the variable map.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Abc_NtkInitStateVarMap( DdManager * dd, Abc_Ntk_t * pNtk, int fVerbose )
{
    DdNode ** pbVarsX, ** pbVarsY;
    DdNode * bTemp, * bProd, * bVar;
    Abc_Obj_t * pLatch;
    int i;

    // set the variable mapping for Cudd_bddVarMap()
    pbVarsX = ABC_ALLOC( DdNode *, dd->size );
    pbVarsY = ABC_ALLOC( DdNode *, dd->size );
    bProd = b1;         Cudd_Ref( bProd );
    Abc_NtkForEachLatch( pNtk, pLatch, i )
    {
        pbVarsX[i] = dd->vars[ Abc_NtkPiNum(pNtk) + i ];
        pbVarsY[i] = dd->vars[ Abc_NtkCiNum(pNtk) + i ];
        // get the initial value of the latch
        bVar  = Cudd_NotCond( pbVarsX[i], !Abc_LatchIsInit1(pLatch) );
        bProd = Cudd_bddAnd( dd, bTemp = bProd, bVar );      Cudd_Ref( bProd );
        Cudd_RecursiveDeref( dd, bTemp ); 
    }
    Cudd_SetVarMap( dd, pbVarsX, pbVarsY, Abc_NtkLatchNum(pNtk) );
    ABC_FREE( pbVarsX );
    ABC_FREE( pbVarsY );

    Cudd_Deref( bProd );
    return bProd;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode ** Abc_NtkCreatePartitions( DdManager * dd, Abc_Ntk_t * pNtk, int fReorder, int fVerbose )
{
    DdNode ** pbParts;
    DdNode * bVar;
    Abc_Obj_t * pNode;
    int i;

    // extand the BDD manager to represent NS variables
    assert( dd->size == Abc_NtkCiNum(pNtk) );
    Cudd_bddIthVar( dd, Abc_NtkCiNum(pNtk) + Abc_NtkLatchNum(pNtk) - 1 );

    // enable reordering
    if ( fReorder )
        Cudd_AutodynEnable( dd, CUDD_REORDER_SYMM_SIFT );
    else
        Cudd_AutodynDisable( dd );

    // compute the transition relation
    pbParts = ABC_ALLOC( DdNode *, Abc_NtkLatchNum(pNtk) );
    Abc_NtkForEachLatch( pNtk, pNode, i )
    {
        bVar  = Cudd_bddIthVar( dd, Abc_NtkCiNum(pNtk) + i );
        pbParts[i] = Cudd_bddXnor( dd, bVar, (DdNode *)Abc_ObjGlobalBdd(Abc_ObjFanin0(pNode)) );  Cudd_Ref( pbParts[i] );
    }
    // free the global BDDs
    Abc_NtkFreeGlobalBdds( pNtk, 0 );

    // reorder and disable reordering
    if ( fReorder )
    {
        if ( fVerbose )
            fprintf( stdout, "BDD nodes in the partitions before reordering %d.\n", Cudd_SharingSize(pbParts,Abc_NtkLatchNum(pNtk)) );
        Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );
        Cudd_AutodynDisable( dd );
        if ( fVerbose )
            fprintf( stdout, "BDD nodes in the partitions after reordering %d.\n", Cudd_SharingSize(pbParts,Abc_NtkLatchNum(pNtk)) );
    }
    return pbParts;
}

/**Function*************************************************************

  Synopsis    [Computes the set of unreachable states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Abc_NtkComputeReachable( DdManager * dd, Abc_Ntk_t * pNtk, DdNode ** pbParts, DdNode * bInitial, DdNode * bOutput, int nBddMax, int nIterMax, int fPartition, int fReorder, int fVerbose )
{
    int fInternalReorder = 0;
    Extra_ImageTree_t * pTree = NULL;
    Extra_ImageTree2_t * pTree2 = NULL;
    DdNode * bReached, * bCubeCs;
    DdNode * bCurrent, * bNext = NULL, * bTemp;
    DdNode ** pbVarsY;
    Abc_Obj_t * pLatch;
    int i, nIters, nBddSize;
    int nThreshold = 10000;

    // collect the NS variables
    // set the variable mapping for Cudd_bddVarMap()
    pbVarsY = ABC_ALLOC( DdNode *, dd->size );
    Abc_NtkForEachLatch( pNtk, pLatch, i )
        pbVarsY[i] = dd->vars[ Abc_NtkCiNum(pNtk) + i ];

    // start the image computation
    bCubeCs  = Extra_bddComputeRangeCube( dd, Abc_NtkPiNum(pNtk), Abc_NtkCiNum(pNtk) );    Cudd_Ref( bCubeCs );
    if ( fPartition )
        pTree = Extra_bddImageStart( dd, bCubeCs, Abc_NtkLatchNum(pNtk), pbParts, Abc_NtkLatchNum(pNtk), pbVarsY, fVerbose );
    else
        pTree2 = Extra_bddImageStart2( dd, bCubeCs, Abc_NtkLatchNum(pNtk), pbParts, Abc_NtkLatchNum(pNtk), pbVarsY, fVerbose );
    ABC_FREE( pbVarsY );
    Cudd_RecursiveDeref( dd, bCubeCs );

    // perform reachability analisys
    bCurrent = bInitial;   Cudd_Ref( bCurrent );
    bReached = bInitial;   Cudd_Ref( bReached );
    assert( nIterMax > 1 ); // required to not deref uninitialized bNext
    for ( nIters = 1; nIters <= nIterMax; nIters++ )
    {
        // compute the next states
        if ( fPartition )
            bNext = Extra_bddImageCompute( pTree, bCurrent );           
        else
            bNext = Extra_bddImageCompute2( pTree2, bCurrent );         
        Cudd_Ref( bNext );
        Cudd_RecursiveDeref( dd, bCurrent );
        // remap these states into the current state vars
        bNext = Cudd_bddVarMap( dd, bTemp = bNext );                    Cudd_Ref( bNext );
        Cudd_RecursiveDeref( dd, bTemp );
        // check if there are any new states
        if ( Cudd_bddLeq( dd, bNext, bReached ) )
            break;
        // check the BDD size
        nBddSize = Cudd_DagSize(bNext);
        if ( nBddSize > nBddMax )
            break;
        // check the result
        if ( !Cudd_bddLeq( dd, bNext, Cudd_Not(bOutput) ) )
        {
            printf( "The miter is proved REACHABLE in %d iterations.  ", nIters );
            Cudd_RecursiveDeref( dd, bReached );
            bReached = NULL;
            break;
        }
        // get the new states
        bCurrent = Cudd_bddAnd( dd, bNext, Cudd_Not(bReached) );        Cudd_Ref( bCurrent );
        // minimize the new states with the reached states
//        bCurrent = Cudd_bddConstrain( dd, bTemp = bCurrent, Cudd_Not(bReached) ); Cudd_Ref( bCurrent );
//        Cudd_RecursiveDeref( dd, bTemp );
        // add to the reached states
        bReached = Cudd_bddOr( dd, bTemp = bReached, bNext );           Cudd_Ref( bReached );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bNext );
        if ( fVerbose )
            fprintf( stdout, "Iteration = %3d. BDD = %5d. ", nIters, nBddSize );
        if ( fInternalReorder && fReorder && nBddSize > nThreshold )
        {
            if ( fVerbose )
                fprintf( stdout, "Reordering... Before = %5d. ", Cudd_DagSize(bReached) );
            Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );
            Cudd_AutodynDisable( dd );
            if ( fVerbose )
                fprintf( stdout, "After = %5d.\r", Cudd_DagSize(bReached) );
            nThreshold *= 2;
        }
        if ( fVerbose )
            fprintf( stdout, "\r" );
    }
    Cudd_RecursiveDeref( dd, bNext );
    // undo the image tree
    if ( fPartition )
        Extra_bddImageTreeDelete( pTree );
    else
        Extra_bddImageTreeDelete2( pTree2 );
    if ( bReached == NULL )
        return NULL;
    // report the stats
    if ( fVerbose )
    {
        double nMints = Cudd_CountMinterm(dd, bReached, Abc_NtkLatchNum(pNtk) );
        if ( nIters > nIterMax || Cudd_DagSize(bReached) > nBddMax )
            fprintf( stdout, "Reachability analysis is stopped after %d iterations.\n", nIters );
        else
            fprintf( stdout, "Reachability analysis completed in %d iterations.\n", nIters );
        fprintf( stdout, "Reachable states = %.0f. (Ratio = %.4f %%)\n", nMints, 100.0*nMints/pow(2.0, Abc_NtkLatchNum(pNtk)) );
        fflush( stdout );
    }
//ABC_PRB( dd, bReached );
    Cudd_Deref( bReached );
    if ( nIters > nIterMax || Cudd_DagSize(bReached) > nBddMax )
         printf( "Verified ONLY FOR STATES REACHED in %d iterations. \n", nIters );
    printf( "The miter is proved unreachable in %d iteration.  ", nIters );
    return bReached;
}

/**Function*************************************************************

  Synopsis    [Performs reachability to see if any .]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkVerifyUsingBdds( Abc_Ntk_t * pNtk, int nBddMax, int nIterMax, int fPartition, int fReorder, int fVerbose )
{
    DdManager * dd;
    DdNode ** pbParts;
    DdNode * bOutput, * bReached, * bInitial;
    int i;
    abctime clk = Abc_Clock();

    assert( Abc_NtkIsStrash(pNtk) );
    assert( Abc_NtkPoNum(pNtk) == 1 );
    assert( Abc_ObjFanoutNum(Abc_NtkPo(pNtk,0)) == 0 ); // PO should go first

    // compute the global BDDs of the latches
    dd = (DdManager *)Abc_NtkBuildGlobalBdds( pNtk, nBddMax, 1, fReorder, 0, fVerbose );    
    if ( dd == NULL )
    {
        printf( "The number of intermediate BDD nodes exceeded the limit (%d).\n", nBddMax );
        return;
    }
    if ( fVerbose )
        printf( "Shared BDD size is %6d nodes.\n", Cudd_ReadKeys(dd) - Cudd_ReadDead(dd) );

    // save the output BDD
    bOutput = (DdNode *)Abc_ObjGlobalBdd(Abc_NtkPo(pNtk,0)); Cudd_Ref( bOutput );

    // create partitions
    pbParts = Abc_NtkCreatePartitions( dd, pNtk, fReorder, fVerbose );

    // create the initial state and the variable map
    bInitial  = Abc_NtkInitStateVarMap( dd, pNtk, fVerbose );  Cudd_Ref( bInitial );

    // check the result
    if ( !Cudd_bddLeq( dd, bInitial, Cudd_Not(bOutput) ) )
        printf( "The miter is proved REACHABLE in the initial state.  " );
    else
    {
        // compute the reachable states
        bReached = Abc_NtkComputeReachable( dd, pNtk, pbParts, bInitial, bOutput, nBddMax, nIterMax, fPartition, fReorder, fVerbose ); 
        if ( bReached != NULL )
        {
            Cudd_Ref( bReached );
            Cudd_RecursiveDeref( dd, bReached );
        }
    }

    // cleanup
    Cudd_RecursiveDeref( dd, bOutput );
    Cudd_RecursiveDeref( dd, bInitial );
    for ( i = 0; i < Abc_NtkLatchNum(pNtk); i++ )
        Cudd_RecursiveDeref( dd, pbParts[i] );
    ABC_FREE( pbParts );
    Extra_StopManager( dd );

    // report the runtime
    ABC_PRT( "Time", Abc_Clock() - clk );
    fflush( stdout );
}

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

