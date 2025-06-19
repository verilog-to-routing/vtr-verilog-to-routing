/**CFile****************************************************************

  FileName    [abcMerge.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [LUT merging algorithm.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcMerge.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "aig/aig/aig.h"
#include "opt/nwk/nwkMerge.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Marks the fanins of the node with the current trav ID.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkMarkFanins_rec( Abc_Obj_t * pLut, int nLevMin )
{
    Abc_Obj_t * pNext;
    int i;
    if ( !Abc_ObjIsNode(pLut) )
        return;
    if ( Abc_NodeIsTravIdCurrent( pLut ) )
        return;
    Abc_NodeSetTravIdCurrent( pLut );
    if ( Abc_ObjLevel(pLut) < nLevMin )
        return;
    Abc_ObjForEachFanin( pLut, pNext, i )
        Abc_NtkMarkFanins_rec( pNext, nLevMin );
}

/**Function*************************************************************

  Synopsis    [Marks the fanouts of the node with the current trav ID.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkMarkFanouts_rec( Abc_Obj_t * pLut, int nLevMax, int nFanMax )
{
    Abc_Obj_t * pNext;
    int i;
    if ( !Abc_ObjIsNode(pLut) )
        return;
    if ( Abc_NodeIsTravIdCurrent( pLut ) )
        return;
    Abc_NodeSetTravIdCurrent( pLut );
    if ( Abc_ObjLevel(pLut) > nLevMax )
        return;
    if ( Abc_ObjFanoutNum(pLut) > nFanMax )
        return;
    Abc_ObjForEachFanout( pLut, pNext, i )
        Abc_NtkMarkFanouts_rec( pNext, nLevMax, nFanMax );
}

/**Function*************************************************************

  Synopsis    [Collects the circle of nodes around the given set.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkCollectCircle( Vec_Ptr_t * vStart, Vec_Ptr_t * vNext, int nFanMax )
{
    Abc_Obj_t * pObj, * pNext;
    int i, k;
    Vec_PtrClear( vNext );
    Vec_PtrForEachEntry( Abc_Obj_t *, vStart, pObj, i )
    {
        Abc_ObjForEachFanin( pObj, pNext, k )
        {
            if ( !Abc_ObjIsNode(pNext) )
                continue;
            if ( Abc_NodeIsTravIdCurrent( pNext ) )
                continue;
            Abc_NodeSetTravIdCurrent( pNext );
            Vec_PtrPush( vNext, pNext );
        }
        Abc_ObjForEachFanout( pObj, pNext, k )
        {
            if ( !Abc_ObjIsNode(pNext) )
                continue;
            if ( Abc_NodeIsTravIdCurrent( pNext ) )
                continue;
            Abc_NodeSetTravIdCurrent( pNext );
            if ( Abc_ObjFanoutNum(pNext) > nFanMax )
                continue;
            Vec_PtrPush( vNext, pNext );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Collects the circle of nodes removes from the given one.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkCollectNonOverlapCands( Abc_Obj_t * pLut, Vec_Ptr_t * vStart, Vec_Ptr_t * vNext, Vec_Ptr_t * vCands, Nwk_LMPars_t * pPars )
{
    Vec_Ptr_t * vTemp;
    Abc_Obj_t * pObj;
    int i, k;
    Vec_PtrClear( vCands );
    if ( pPars->nMaxSuppSize - Abc_ObjFaninNum(pLut) <= 1 )
        return;

    // collect nodes removed by this distance
    assert( pPars->nMaxDistance > 0 );
    Vec_PtrClear( vStart );
    Vec_PtrPush( vStart, pLut );
    Abc_NtkIncrementTravId( pLut->pNtk );
    Abc_NodeSetTravIdCurrent( pLut );
    for ( i = 1; i <= pPars->nMaxDistance; i++ )
    {
        Abc_NtkCollectCircle( vStart, vNext, pPars->nMaxFanout );
        vTemp  = vStart;
        vStart = vNext;
        vNext  = vTemp;
        // collect the nodes in vStart
        Vec_PtrForEachEntry( Abc_Obj_t *, vStart, pObj, k )
            Vec_PtrPush( vCands, pObj );
    }

    // mark the TFI/TFO nodes
    Abc_NtkIncrementTravId( pLut->pNtk );
    if ( pPars->fUseTfiTfo )
        Abc_NodeSetTravIdCurrent( pLut );
    else
    {
        Abc_NodeSetTravIdPrevious( pLut );
        Abc_NtkMarkFanins_rec( pLut, Abc_ObjLevel(pLut) - pPars->nMaxDistance );
        Abc_NodeSetTravIdPrevious( pLut );
        Abc_NtkMarkFanouts_rec( pLut, Abc_ObjLevel(pLut) + pPars->nMaxDistance, pPars->nMaxFanout );
    }

    // collect nodes satisfying the following conditions:
    // - they are close enough in terms of distance
    // - they are not in the TFI/TFO of the LUT
    // - they have no more than the given number of fanins
    // - they have no more than the given diff in delay
    k = 0;
    Vec_PtrForEachEntry( Abc_Obj_t *, vCands, pObj, i )
    {
        if ( Abc_NodeIsTravIdCurrent(pObj) )
            continue;
        if ( Abc_ObjFaninNum(pLut) + Abc_ObjFaninNum(pObj) > pPars->nMaxSuppSize )
            continue;
        if ( Abc_ObjLevel(pLut) - Abc_ObjLevel(pObj) > pPars->nMaxLevelDiff || 
             Abc_ObjLevel(pObj) - Abc_ObjLevel(pLut) > pPars->nMaxLevelDiff )
             continue;
        Vec_PtrWriteEntry( vCands, k++, pObj );
    }
    Vec_PtrShrink( vCands, k );
}


/**Function*************************************************************

  Synopsis    [Count the total number of fanins.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCountTotalFanins( Abc_Obj_t * pLut, Abc_Obj_t * pCand )
{
    Abc_Obj_t * pFanin;
    int i, nCounter = Abc_ObjFaninNum(pLut);
    Abc_ObjForEachFanin( pCand, pFanin, i )
        nCounter += !pFanin->fMarkC;
    return nCounter;
}

/**Function*************************************************************

  Synopsis    [Collects overlapping candidates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkCollectOverlapCands( Abc_Obj_t * pLut, Vec_Ptr_t * vCands, Nwk_LMPars_t * pPars )
{
    Abc_Obj_t * pFanin, * pObj;
    int i, k;
    // mark fanins of pLut
    Abc_ObjForEachFanin( pLut, pFanin, i )
        pFanin->fMarkC = 1;
    // collect the matching fanouts of each fanin of the node
    Vec_PtrClear( vCands );
    Abc_NtkIncrementTravId( pLut->pNtk );
    Abc_NodeSetTravIdCurrent( pLut );
    Abc_ObjForEachFanin( pLut, pFanin, i )
    {
        if ( !Abc_ObjIsNode(pFanin) )
            continue;
        if ( Abc_ObjFanoutNum(pFanin) > pPars->nMaxFanout )
            continue;
        Abc_ObjForEachFanout( pFanin, pObj, k )
        {
            if ( !Abc_ObjIsNode(pObj) )
                continue;
            if ( Abc_NodeIsTravIdCurrent( pObj ) )
                continue;
            Abc_NodeSetTravIdCurrent( pObj );
            // check the difference in delay
            if ( Abc_ObjLevel(pLut) - Abc_ObjLevel(pObj) > pPars->nMaxLevelDiff || 
                 Abc_ObjLevel(pObj) - Abc_ObjLevel(pLut) > pPars->nMaxLevelDiff )
                 continue;
            // check the total number of fanins of the node
            if ( Abc_NtkCountTotalFanins(pLut, pObj) > pPars->nMaxSuppSize )
                continue;
            Vec_PtrPush( vCands, pObj );
        }
    }
    // unmark fanins of pLut
    Abc_ObjForEachFanin( pLut, pFanin, i )
        pFanin->fMarkC = 0;
}

/**Function*************************************************************

  Synopsis    [Performs LUT merging with parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Abc_NtkLutMerge( Abc_Ntk_t * pNtk, Nwk_LMPars_t * pPars )
{
    Nwk_Grf_t * p;
    Vec_Int_t * vResult;
    Vec_Ptr_t * vStart, * vNext, * vCands1, * vCands2;
    Abc_Obj_t * pLut, * pCand;
    int i, k, nVertsMax, nCands;
    abctime clk = Abc_Clock();
    // count the number of vertices
    nVertsMax = 0;
    Abc_NtkForEachNode( pNtk, pLut, i )
        nVertsMax += (int)(Abc_ObjFaninNum(pLut) <= pPars->nMaxLutSize);
    p = Nwk_ManGraphAlloc( nVertsMax );
    // create graph
    vStart  = Vec_PtrAlloc( 1000 );
    vNext   = Vec_PtrAlloc( 1000 );
    vCands1 = Vec_PtrAlloc( 1000 );
    vCands2 = Vec_PtrAlloc( 1000 );
    nCands  = 0;
    Abc_NtkForEachNode( pNtk, pLut, i )
    {
        if ( Abc_ObjFaninNum(pLut) > pPars->nMaxLutSize )
            continue;
        Abc_NtkCollectOverlapCands( pLut, vCands1, pPars );
        if ( pPars->fUseDiffSupp )
            Abc_NtkCollectNonOverlapCands( pLut, vStart, vNext, vCands2, pPars );
        if ( Vec_PtrSize(vCands1) == 0 && Vec_PtrSize(vCands2) == 0 )
            continue;
        nCands += Vec_PtrSize(vCands1) + Vec_PtrSize(vCands2);
        // save candidates
        Vec_PtrForEachEntry( Abc_Obj_t *, vCands1, pCand, k )
            Nwk_ManGraphHashEdge( p, Abc_ObjId(pLut), Abc_ObjId(pCand) );
        Vec_PtrForEachEntry( Abc_Obj_t *, vCands2, pCand, k )
            Nwk_ManGraphHashEdge( p, Abc_ObjId(pLut), Abc_ObjId(pCand) );
        // print statistics about this node
        if ( pPars->fVeryVerbose )
        printf( "Node %6d : Fanins = %d. Fanouts = %3d.  Cand1 = %3d. Cand2 = %3d.\n",
            Abc_ObjId(pLut), Abc_ObjFaninNum(pLut), Abc_ObjFaninNum(pLut), 
            Vec_PtrSize(vCands1), Vec_PtrSize(vCands2) );
    }
    Vec_PtrFree( vStart );
    Vec_PtrFree( vNext );
    Vec_PtrFree( vCands1 );
    Vec_PtrFree( vCands2 );
    if ( pPars->fVerbose )
    {
        printf( "Mergable LUTs = %6d. Total cands = %6d. ", p->nVertsMax, nCands );
        ABC_PRT( "Deriving graph", Abc_Clock() - clk );
    }
    // solve the graph problem
    clk = Abc_Clock();
    Nwk_ManGraphSolve( p );
    if ( pPars->fVerbose )
    {
        printf( "GRAPH: Nodes = %6d. Edges = %6d.  Pairs = %6d.  ", 
            p->nVerts, p->nEdges, Vec_IntSize(p->vPairs)/2 );
        ABC_PRT( "Solving", Abc_Clock() - clk );
        Nwk_ManGraphReportMemoryUsage( p );
    }
    vResult = p->vPairs; p->vPairs = NULL;
/*
    for ( i = 0; i < vResult->nSize; i += 2 )
        printf( "(%d,%d) ", vResult->pArray[i], vResult->pArray[i+1] );
    printf( "\n" );
*/
    Nwk_ManGraphFree( p );
    return vResult;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

