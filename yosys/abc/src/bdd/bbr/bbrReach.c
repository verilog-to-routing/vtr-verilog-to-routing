/**CFile****************************************************************

  FileName    [bbrReach.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [BDD-based reachability analysis.]

  Synopsis    [Procedures to perform reachability analysis.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: bbrReach.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bbr.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern Abc_Cex_t * Aig_ManVerifyUsingBddsCountExample( Aig_Man_t * p, DdManager * dd, 
    DdNode ** pbParts, Vec_Ptr_t * vOnionRings, DdNode * bCubeFirst, 
    int iOutput, int fVerbose, int fSilent );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [This procedure sets default resynthesis parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bbr_ManSetDefaultParams( Saig_ParBbr_t * p )
{
    memset( p, 0, sizeof(Saig_ParBbr_t) );
    p->TimeLimit     =     0;
    p->nBddMax       = 50000;
    p->nIterMax      =  1000;
    p->fPartition    =     1;
    p->fReorder      =     1;
    p->fReorderImage =     1;
    p->fVerbose      =     0;
    p->fSilent       =     0;
    p->iFrame        =    -1;
}

/**Function********************************************************************

  Synopsis    [Performs the reordering-sensitive step of Extra_bddMove().]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * Bbr_bddComputeRangeCube( DdManager * dd, int iStart, int iStop )
{
    DdNode * bTemp, * bProd;
    int i;
    assert( iStart <= iStop );
    assert( iStart >= 0 && iStart <= dd->size );
    assert( iStop >= 0  && iStop  <= dd->size );
    bProd = (dd)->one;         Cudd_Ref( bProd );
    for ( i = iStart; i < iStop; i++ )
    {
        bProd = Cudd_bddAnd( dd, bTemp = bProd, dd->vars[i] );      Cudd_Ref( bProd );
        Cudd_RecursiveDeref( dd, bTemp ); 
    }
    Cudd_Deref( bProd );
    return bProd;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bbr_StopManager( DdManager * dd )
{
    int RetValue;
    // check for remaining references in the package
    RetValue = Cudd_CheckZeroRef( dd );
    if ( RetValue > 0 )
        printf( "\nThe number of referenced nodes = %d\n\n", RetValue );
//  Cudd_PrintInfo( dd, stdout );
    Cudd_Quit( dd );
}

/**Function*************************************************************

  Synopsis    [Computes the initial state and sets up the variable map.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Aig_ManInitStateVarMap( DdManager * dd, Aig_Man_t * p, int fVerbose )
{
    DdNode ** pbVarsX, ** pbVarsY;
    DdNode * bTemp, * bProd;
    Aig_Obj_t * pLatch;
    int i;

    // set the variable mapping for Cudd_bddVarMap()
    pbVarsX = ABC_ALLOC( DdNode *, dd->size );
    pbVarsY = ABC_ALLOC( DdNode *, dd->size );
    bProd = (dd)->one;         Cudd_Ref( bProd );
    Saig_ManForEachLo( p, pLatch, i )
    {
        pbVarsX[i] = dd->vars[ Saig_ManPiNum(p) + i ];
        pbVarsY[i] = dd->vars[ Saig_ManCiNum(p) + i ];
        // get the initial value of the latch
        bProd = Cudd_bddAnd( dd, bTemp = bProd, Cudd_Not(pbVarsX[i]) );      Cudd_Ref( bProd );
        Cudd_RecursiveDeref( dd, bTemp ); 
    }
    Cudd_SetVarMap( dd, pbVarsX, pbVarsY, Saig_ManRegNum(p) );
    ABC_FREE( pbVarsX );
    ABC_FREE( pbVarsY );

    Cudd_Deref( bProd );
    return bProd;
}

/**Function*************************************************************

  Synopsis    [Collects the array of output BDDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode ** Aig_ManCreateOutputs( DdManager * dd, Aig_Man_t * p )
{
    DdNode ** pbOutputs;
    Aig_Obj_t * pNode;
    int i;
    // compute the transition relation
    pbOutputs = ABC_ALLOC( DdNode *, Saig_ManPoNum(p) );
    Saig_ManForEachPo( p, pNode, i )
    {
        pbOutputs[i] = Aig_ObjGlobalBdd(pNode);  Cudd_Ref( pbOutputs[i] );
    }
    return pbOutputs;
}

/**Function*************************************************************

  Synopsis    [Collects the array of partition BDDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode ** Aig_ManCreatePartitions( DdManager * dd, Aig_Man_t * p, int fReorder, int fVerbose )
{
    DdNode ** pbParts;
    DdNode * bVar;
    Aig_Obj_t * pNode;
    int i;

    // extand the BDD manager to represent NS variables
    assert( dd->size == Saig_ManCiNum(p) );
    Cudd_bddIthVar( dd, Saig_ManCiNum(p) + Saig_ManRegNum(p) - 1 );

    // enable reordering
    if ( fReorder )
        Cudd_AutodynEnable( dd, CUDD_REORDER_SYMM_SIFT );
    else
        Cudd_AutodynDisable( dd );

    // compute the transition relation
    pbParts = ABC_ALLOC( DdNode *, Saig_ManRegNum(p) );
    Saig_ManForEachLi( p, pNode, i )
    {
        bVar  = Cudd_bddIthVar( dd, Saig_ManCiNum(p) + i );
        pbParts[i] = Cudd_bddXnor( dd, bVar, Aig_ObjGlobalBdd(pNode) );  Cudd_Ref( pbParts[i] );
    }
    // free global BDDs
    Aig_ManFreeGlobalBdds( p, dd );

    // reorder and disable reordering
    if ( fReorder )
    {
        if ( fVerbose )
            fprintf( stdout, "BDD nodes in the partitions before reordering %d.\n", Cudd_SharingSize(pbParts,Saig_ManRegNum(p)) );
        Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );
        Cudd_AutodynDisable( dd );
        if ( fVerbose )
            fprintf( stdout, "BDD nodes in the partitions after reordering %d.\n", Cudd_SharingSize(pbParts,Saig_ManRegNum(p)) );
    }
    return pbParts;
}

/**Function*************************************************************

  Synopsis    [Computes the set of unreachable states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManComputeReachable( DdManager * dd, Aig_Man_t * p, DdNode ** pbParts, DdNode * bInitial, DdNode ** pbOutputs, Saig_ParBbr_t * pPars, int fCheckOutputs )
{
    int fInternalReorder = 0;
    Bbr_ImageTree_t * pTree = NULL; // Suppress "might be used uninitialized"
    Bbr_ImageTree2_t * pTree2 = NULL; // Supprses "might be used uninitialized"
    DdNode * bReached, * bCubeCs;
    DdNode * bCurrent;
    DdNode * bNext = NULL; // Suppress "might be used uninitialized"
    DdNode * bTemp;
    Cudd_ReorderingType method;
    int i, nIters, nBddSize = 0, status;
    int nThreshold = 10000;
    abctime clk = Abc_Clock();
    Vec_Ptr_t * vOnionRings;
    int fixedPoint = 0;

    status = Cudd_ReorderingStatus( dd, &method );
    if ( status )
        Cudd_AutodynDisable( dd );

    // start the image computation
    bCubeCs  = Bbr_bddComputeRangeCube( dd, Saig_ManPiNum(p), Saig_ManCiNum(p) );    Cudd_Ref( bCubeCs );
    if ( pPars->fPartition )
        pTree = Bbr_bddImageStart( dd, bCubeCs, Saig_ManRegNum(p), pbParts, Saig_ManRegNum(p), dd->vars+Saig_ManCiNum(p), pPars->nBddMax, pPars->fVerbose );
    else
        pTree2 = Bbr_bddImageStart2( dd, bCubeCs, Saig_ManRegNum(p), pbParts, Saig_ManRegNum(p), dd->vars+Saig_ManCiNum(p), pPars->fVerbose );
    Cudd_RecursiveDeref( dd, bCubeCs );
    if ( pTree == NULL )
    {
        if ( !pPars->fSilent )
            printf( "BDDs blew up during qualitification scheduling.  " );
        return -1;
    }

    if ( status )
        Cudd_AutodynEnable( dd, method );

    // start the onion rings
    vOnionRings = Vec_PtrAlloc( 1000 );

    // perform reachability analysis
    bCurrent = bInitial;   Cudd_Ref( bCurrent );
    bReached = bInitial;   Cudd_Ref( bReached );
    Vec_PtrPush( vOnionRings, bCurrent );  Cudd_Ref( bCurrent );
    for ( nIters = 0; nIters < pPars->nIterMax; nIters++ )
    { 
        // check the runtime limit
        if ( pPars->TimeLimit && pPars->TimeLimit <= (Abc_Clock()-clk)/CLOCKS_PER_SEC )
        {
            printf( "Reached timeout after image computation (%d seconds).\n",  pPars->TimeLimit );
            Vec_PtrFree( vOnionRings );
            // undo the image tree
            if ( pPars->fPartition )
                Bbr_bddImageTreeDelete( pTree );
            else
                Bbr_bddImageTreeDelete2( pTree2 );
            pPars->iFrame = nIters - 1;
            return -1;
        }

        // compute the next states
        if ( pPars->fPartition )
            bNext = Bbr_bddImageCompute( pTree, bCurrent );           
        else
            bNext = Bbr_bddImageCompute2( pTree2, bCurrent );  
        if ( bNext == NULL )
        {
            if ( !pPars->fSilent )
                printf( "BDDs blew up during image computation.  " );
            if ( pPars->fPartition )
                Bbr_bddImageTreeDelete( pTree );
            else
                Bbr_bddImageTreeDelete2( pTree2 );
            Vec_PtrFree( vOnionRings );
            pPars->iFrame = nIters - 1;
            return -1;
        }
        Cudd_Ref( bNext );
        Cudd_RecursiveDeref( dd, bCurrent );

        // remap these states into the current state vars
        bNext = Cudd_bddVarMap( dd, bTemp = bNext );                    Cudd_Ref( bNext );
        Cudd_RecursiveDeref( dd, bTemp );
        // check if there are any new states
        if ( Cudd_bddLeq( dd, bNext, bReached ) ) {
            fixedPoint = 1;
            break;
        }
        // check the BDD size
        nBddSize = Cudd_DagSize(bNext);
        if ( nBddSize > pPars->nBddMax )
            break;
        // check the result
        for ( i = 0; i < Saig_ManPoNum(p); i++ )
        {
            if ( fCheckOutputs && !Cudd_bddLeq( dd, bNext, Cudd_Not(pbOutputs[i]) ) )
            {
                DdNode * bIntersect;
                bIntersect = Cudd_bddIntersect( dd, bNext, pbOutputs[i] );  Cudd_Ref( bIntersect );
                assert( p->pSeqModel == NULL );
                p->pSeqModel = Aig_ManVerifyUsingBddsCountExample( p, dd, pbParts, 
                    vOnionRings, bIntersect, i, pPars->fVerbose, pPars->fSilent ); 
                Cudd_RecursiveDeref( dd, bIntersect );
                if ( !pPars->fSilent )
                    Abc_Print( 1, "Output %d of miter \"%s\" was asserted in frame %d. ", i, p->pName, Vec_PtrSize(vOnionRings) );
                Cudd_RecursiveDeref( dd, bReached );
                bReached = NULL;
                pPars->iFrame = nIters;
                break;
            }
        }
        if ( i < Saig_ManPoNum(p) )
            break;
        // get the new states
        bCurrent = Cudd_bddAnd( dd, bNext, Cudd_Not(bReached) );        Cudd_Ref( bCurrent );
        Vec_PtrPush( vOnionRings, bCurrent );  Cudd_Ref( bCurrent );
        // minimize the new states with the reached states
//        bCurrent = Cudd_bddConstrain( dd, bTemp = bCurrent, Cudd_Not(bReached) ); Cudd_Ref( bCurrent );
//        Cudd_RecursiveDeref( dd, bTemp );
        // add to the reached states
        bReached = Cudd_bddOr( dd, bTemp = bReached, bNext );           Cudd_Ref( bReached );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bNext );
        if ( pPars->fVerbose )
            fprintf( stdout, "Frame = %3d. BDD = %5d. ", nIters, nBddSize );
        if ( fInternalReorder && pPars->fReorder && nBddSize > nThreshold )
        {
            if ( pPars->fVerbose )
                fprintf( stdout, "Reordering... Before = %5d. ", Cudd_DagSize(bReached) );
            Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );
            Cudd_AutodynDisable( dd );
            if ( pPars->fVerbose )
                fprintf( stdout, "After = %5d.\r", Cudd_DagSize(bReached) );
            nThreshold *= 2;
        }
        if ( pPars->fVerbose )
//            fprintf( stdout, "\r" );
            fprintf( stdout, "\n" );

        if ( pPars->fVerbose )
        {
            double nMints = Cudd_CountMinterm(dd, bReached, Saig_ManRegNum(p) );
//            Extra_bddPrint( dd, bReached );printf( "\n" );
            fprintf( stdout, "Reachable states = %.0f. (Ratio = %.4f %%)\n", nMints, 100.0*nMints/pow(2.0, Saig_ManRegNum(p)) );
            fflush( stdout ); 
        }

    }
    Cudd_RecursiveDeref( dd, bNext );
    // free the onion rings
    Vec_PtrForEachEntry( DdNode *, vOnionRings, bTemp, i )
        Cudd_RecursiveDeref( dd, bTemp );
    Vec_PtrFree( vOnionRings );
    // undo the image tree
    if ( pPars->fPartition )
        Bbr_bddImageTreeDelete( pTree );
    else
        Bbr_bddImageTreeDelete2( pTree2 );
    if ( bReached == NULL )
        return 0; // proved reachable
    // report the stats
    if ( pPars->fVerbose )
    {
        double nMints = Cudd_CountMinterm(dd, bReached, Saig_ManRegNum(p) );
        if ( nIters > pPars->nIterMax || nBddSize > pPars->nBddMax )
            fprintf( stdout, "Reachability analysis is stopped after %d frames.\n", nIters );
        else
            fprintf( stdout, "Reachability analysis completed after %d frames.\n", nIters );
        fprintf( stdout, "Reachable states = %.0f. (Ratio = %.4f %%)\n", nMints, 100.0*nMints/pow(2.0, Saig_ManRegNum(p)) );
        fflush( stdout );
    }
//ABC_PRB( dd, bReached );
    Cudd_RecursiveDeref( dd, bReached );
    // SPG
    //
    if ( fixedPoint ) {
      if ( !pPars->fSilent ) {
        printf( "The miter is proved unreachable after %d iterations.  ", nIters );
      }
      pPars->iFrame = nIters - 1;
      return 1;
    }
    assert(nIters >= pPars->nIterMax || nBddSize >= pPars->nBddMax);
    if ( !pPars->fSilent )
      printf( "Verified only for states reachable in %d frames.  ", nIters );
    pPars->iFrame = nIters - 1;
    return -1; // undecided
}

/**Function*************************************************************

  Synopsis    [Performs reachability to see if any PO can be asserted.]

  Description []
               
  SideEffects []
 
  SeeAlso     []

***********************************************************************/
int Aig_ManVerifyUsingBdds_int( Aig_Man_t * p, Saig_ParBbr_t * pPars )
{
    int fCheckOutputs = !pPars->fSkipOutCheck;
    DdManager * dd;
    DdNode ** pbParts, ** pbOutputs;
    DdNode * bInitial, * bTemp;
    int RetValue, i;
    abctime clk = Abc_Clock();
    Vec_Ptr_t * vOnionRings;

    assert( Saig_ManRegNum(p) > 0 );

    // compute the global BDDs of the latches
    dd = Aig_ManComputeGlobalBdds( p, pPars->nBddMax, 1, pPars->fReorder, pPars->fVerbose );    
    if ( dd == NULL )
    {
        if ( !pPars->fSilent )
            printf( "The number of intermediate BDD nodes exceeded the limit (%d).\n", pPars->nBddMax );
        return -1;
    }
    if ( pPars->fVerbose )
        printf( "Shared BDD size is %6d nodes.\n", Cudd_ReadKeys(dd) - Cudd_ReadDead(dd) );

    // check the runtime limit
    if ( pPars->TimeLimit && pPars->TimeLimit <= (Abc_Clock()-clk)/CLOCKS_PER_SEC )
    {
        printf( "Reached timeout after constructing global BDDs (%d seconds).\n",  pPars->TimeLimit );
        Cudd_Quit( dd );
        return -1;
    }

    // start the onion rings
    vOnionRings = Vec_PtrAlloc( 1000 );

    // save outputs
    pbOutputs = Aig_ManCreateOutputs( dd, p );

    // create partitions
    pbParts = Aig_ManCreatePartitions( dd, p, pPars->fReorder, pPars->fVerbose );

    // create the initial state and the variable map
    bInitial  = Aig_ManInitStateVarMap( dd, p, pPars->fVerbose );  Cudd_Ref( bInitial );

    // set reordering
    if ( pPars->fReorderImage )
        Cudd_AutodynEnable( dd, CUDD_REORDER_SYMM_SIFT );

    // check the result
    RetValue = -1;
    for ( i = 0; i < Saig_ManPoNum(p); i++ )
    {
        if ( fCheckOutputs && !Cudd_bddLeq( dd, bInitial, Cudd_Not(pbOutputs[i]) ) )
        {
            DdNode * bIntersect;
            bIntersect = Cudd_bddIntersect( dd, bInitial, pbOutputs[i] );  Cudd_Ref( bIntersect );
            assert( p->pSeqModel == NULL );
            p->pSeqModel = Aig_ManVerifyUsingBddsCountExample( p, dd, pbParts, 
                vOnionRings, bIntersect, i, pPars->fVerbose, pPars->fSilent ); 
            Cudd_RecursiveDeref( dd, bIntersect );
            if ( !pPars->fSilent )
                Abc_Print( 1, "Output %d of miter \"%s\" was asserted in frame %d. ", i, p->pName, -1 );
            RetValue = 0;
            break;
        }
    }
    // free the onion rings
    Vec_PtrForEachEntry( DdNode *, vOnionRings, bTemp, i )
        Cudd_RecursiveDeref( dd, bTemp );
    Vec_PtrFree( vOnionRings );
    // explore reachable states
    if ( RetValue == -1 )
        RetValue = Aig_ManComputeReachable( dd, p, pbParts, bInitial, pbOutputs, pPars, fCheckOutputs ); 

    // cleanup
    Cudd_RecursiveDeref( dd, bInitial );
    for ( i = 0; i < Saig_ManRegNum(p); i++ )
        Cudd_RecursiveDeref( dd, pbParts[i] );
    ABC_FREE( pbParts );
    for ( i = 0; i < Saig_ManPoNum(p); i++ )
        Cudd_RecursiveDeref( dd, pbOutputs[i] );
    ABC_FREE( pbOutputs );
//    if ( RetValue == -1 )
        Cudd_Quit( dd );
//    else
//        Bbr_StopManager( dd );

    // report the runtime
    if ( !pPars->fSilent )
    {
    ABC_PRT( "Time", Abc_Clock() - clk );
    fflush( stdout );
    }
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Performs reachability to see if any PO can be asserted.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManVerifyUsingBdds( Aig_Man_t * pInit, Saig_ParBbr_t * pPars )
{
    Abc_Cex_t * pCexOld, * pCexNew;
    Aig_Man_t * p;
    Aig_Obj_t * pObj;
    Vec_Int_t * vInputMap;
    int i, k, Entry, iBitOld, iBitNew, RetValue;
//    pPars->fVerbose = 1;
    // check if there are PIs without fanout
    Saig_ManForEachPi( pInit, pObj, i )
        if ( Aig_ObjRefs(pObj) == 0 )
            break;
    if ( i == Saig_ManPiNum(pInit) )
        return Aig_ManVerifyUsingBdds_int( pInit, pPars );
    // create new AIG
    p = Aig_ManDupTrim( pInit );
    assert( Aig_ManCiNum(p) < Aig_ManCiNum(pInit) );
    assert( Aig_ManRegNum(p) == Aig_ManRegNum(pInit) );
    RetValue = Aig_ManVerifyUsingBdds_int( p, pPars );
    if ( RetValue != 0 )
    {
        Aig_ManStop( p );
        return RetValue;
    }
    // the problem is satisfiable - remap the pattern
    pCexOld = p->pSeqModel;
    assert( pCexOld != NULL );
    // create input map
    vInputMap = Vec_IntAlloc( Saig_ManPiNum(pInit) );
    Saig_ManForEachPi( pInit, pObj, i )
        if ( pObj->pData != NULL )
            Vec_IntPush( vInputMap, Aig_ObjCioId((Aig_Obj_t *)pObj->pData) );
        else
            Vec_IntPush( vInputMap, -1 );
    // create new pattern
    pCexNew = Abc_CexAlloc( Saig_ManRegNum(pInit), Saig_ManPiNum(pInit), pCexOld->iFrame+1 );
    pCexNew->iFrame = pCexOld->iFrame;
    pCexNew->iPo    = pCexOld->iPo;
    // copy the bit-data
    for ( iBitOld = 0; iBitOld < pCexOld->nRegs; iBitOld++ )
        if ( Abc_InfoHasBit( pCexOld->pData, iBitOld ) )
            Abc_InfoSetBit( pCexNew->pData, iBitOld );
    // copy the primary input data
    iBitNew = iBitOld;
    for ( i = 0; i <= pCexNew->iFrame; i++ )
    {
        Vec_IntForEachEntry( vInputMap, Entry, k )
        {
            if ( Entry == -1 )
                continue;
            if ( Abc_InfoHasBit( pCexOld->pData, iBitOld + Entry ) )
                Abc_InfoSetBit( pCexNew->pData, iBitNew + k );
        }
        iBitOld += Saig_ManPiNum(p);
        iBitNew += Saig_ManPiNum(pInit);
    }
    assert( iBitOld < iBitNew );
    assert( iBitOld == pCexOld->nBits );
    assert( iBitNew == pCexNew->nBits );
    Vec_IntFree( vInputMap );
    pInit->pSeqModel = pCexNew;
    Aig_ManStop( p );
    return 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

