/**CFile****************************************************************

  FileName    [ifExpand.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [FPGA mapping based on priority cuts.]

  Synopsis    [Incremental improvement of current mapping.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 21, 2006.]

  Revision    [$Id: ifExpand.c,v 1.00 2006/11/21 00:00:00 alanmi Exp $]

***********************************************************************/

#include "if.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void If_ManImproveReduce( If_Man_t * p, int nLimit );
static void If_ManImproveExpand( If_Man_t * p, int nLimit );
static void If_ManImproveNodeExpand( If_Man_t * p, If_Obj_t * pObj, int nLimit, Vec_Ptr_t * vFront, Vec_Ptr_t * vFrontOld, Vec_Ptr_t * vVisited );
static void If_ManImproveNodePrepare( If_Man_t * p, If_Obj_t * pObj, int nLimit, Vec_Ptr_t * vFront, Vec_Ptr_t * vFrontOld, Vec_Ptr_t * vVisited );
static void If_ManImproveNodeUpdate( If_Man_t * p, If_Obj_t * pObj, Vec_Ptr_t * vFront );
static void If_ManImproveNodeFaninCompact( If_Man_t * p, If_Obj_t * pObj, int nLimit, Vec_Ptr_t * vFront, Vec_Ptr_t * vVisited );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Improves current mapping using expand/Expand of one cut.]

  Description [Assumes current mapping assigned and required times computed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManImproveMapping( If_Man_t * p )
{
    int clk;

    clk = clock();
    If_ManImproveExpand( p, p->pPars->nLutSize );
    If_ManComputeRequired( p );
    if ( p->pPars->fVerbose )
    {
        printf( "E: Del = %7.2f. Ar = %9.1f. Net = %8d. Cut = %8d. ", 
            p->RequiredGlo, p->AreaGlo, p->nNets, p->nCutsMerged );
        PRT( "T", clock() - clk );
    }

/*
    clk = clock();
    If_ManImproveReduce( p, p->pPars->nLutSize );
    If_ManComputeRequired( p, 0 );
    if ( p->pPars->fVerbose )
    {
        printf( "R:  Del = %6.2f. Area = %8.2f. Nets = %6d. Cuts = %8d. Lim = %2d. Ave = %5.2f. ", 
            p->RequiredGlo, p->AreaGlo, p->nNets, p->nCutsMerged, p->nCutsUsed, 1.0 * p->nCutsMerged / If_ManAndNum(p) );
        PRT( "T", clock() - clk );
    }
*/
/*
    clk = clock();
    If_ManImproveExpand( p, p->pPars->nLutSize );
    If_ManComputeRequired( p, 0 );
    if ( p->pPars->fVerbose )
    {
        printf( "E:  Del = %6.2f. Area = %8.2f. Nets = %6d. Cuts = %8d. Lim = %2d. Ave = %5.2f. ", 
            p->RequiredGlo, p->AreaGlo, p->nNets, p->nCutsMerged, p->nCutsUsed, 1.0 * p->nCutsMerged / If_ManAndNum(p) );
        PRT( "T", clock() - clk );
    }
*/
}

/**Function*************************************************************

  Synopsis    [Performs area recovery for each node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManImproveExpand( If_Man_t * p, int nLimit )
{
    Vec_Ptr_t * vFront, * vFrontOld, * vVisited;
    If_Obj_t * pObj;
    int i;
    vFront    = Vec_PtrAlloc( nLimit );
    vFrontOld = Vec_PtrAlloc( nLimit );
    vVisited  = Vec_PtrAlloc( 100 );
    // iterate through all nodes in the topological order
    If_ManForEachNode( p, pObj, i )
        If_ManImproveNodeExpand( p, pObj, nLimit, vFront, vFrontOld, vVisited );
    Vec_PtrFree( vFront );
    Vec_PtrFree( vFrontOld );
    Vec_PtrFree( vVisited );
}

/**Function*************************************************************

  Synopsis    [Counts the number of nodes with no external fanout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_ManImproveCutCost( If_Man_t * p, Vec_Ptr_t * vFront )
{
    If_Obj_t * pFanin;
    int i, Counter = 0;
    Vec_PtrForEachEntry( vFront, pFanin, i )
        if ( pFanin->nRefs == 0 )
            Counter++;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Performs area recovery for each node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManImproveNodeExpand( If_Man_t * p, If_Obj_t * pObj, int nLimit, Vec_Ptr_t * vFront, Vec_Ptr_t * vFrontOld, Vec_Ptr_t * vVisited )
{
    If_Obj_t * pFanin;
    If_Cut_t * pCut;
    int CostBef, CostAft, i;
    float DelayOld, AreaBef, AreaAft;
    pCut = If_ObjCutBest(pObj);
    assert( pCut->Delay <= pObj->Required + p->fEpsilon );
    if ( pObj->nRefs == 0 )
        return;
    // get the delay
    DelayOld = pCut->Delay;
    // get the area
    AreaBef = If_CutAreaRefed( p, pCut, IF_INFINITY );
//    if ( AreaBef == 1 )
//        return;
    // the cut is non-trivial
    If_ManImproveNodePrepare( p, pObj, nLimit, vFront, vFrontOld, vVisited );
    // iteratively modify the cut
    If_CutDeref( p, pCut, IF_INFINITY );
    CostBef = If_ManImproveCutCost( p, vFront );
    If_ManImproveNodeFaninCompact( p, pObj, nLimit, vFront, vVisited );
    CostAft = If_ManImproveCutCost( p, vFront );
    If_CutRef( p, pCut, IF_INFINITY );
    assert( CostBef >= CostAft );
    // clean up
    Vec_PtrForEachEntry( vVisited, pFanin, i )
        pFanin->fMark = 0;
    // update the node
    If_ManImproveNodeUpdate( p, pObj, vFront );
    pCut->Delay = If_CutDelay( p, pCut );
    // get the new area
    AreaAft = If_CutAreaRefed( p, pCut, IF_INFINITY );
    if ( AreaAft > AreaBef || pCut->Delay > pObj->Required + p->fEpsilon )
    {
        If_ManImproveNodeUpdate( p, pObj, vFrontOld );
        AreaAft = If_CutAreaRefed( p, pCut, IF_INFINITY );
        assert( AreaAft == AreaBef );
        pCut->Delay = DelayOld;
    }
}

/**Function*************************************************************

  Synopsis    [Performs area recovery for each node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManImproveMark_rec( If_Man_t * p, If_Obj_t * pObj, Vec_Ptr_t * vVisited )
{
    if ( pObj->fMark )
        return;
    assert( If_ObjIsAnd(pObj) );
    If_ManImproveMark_rec( p, If_ObjFanin0(pObj), vVisited );
    If_ManImproveMark_rec( p, If_ObjFanin1(pObj), vVisited );
    Vec_PtrPush( vVisited, pObj );
    pObj->fMark = 1;
}

/**Function*************************************************************

  Synopsis    [Prepares node mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManImproveNodePrepare( If_Man_t * p, If_Obj_t * pObj, int nLimit, Vec_Ptr_t * vFront, Vec_Ptr_t * vFrontOld, Vec_Ptr_t * vVisited )
{
    If_Cut_t * pCut;
    If_Obj_t * pLeaf;
    int i;
    Vec_PtrClear( vFront );
    Vec_PtrClear( vFrontOld );
    Vec_PtrClear( vVisited );
    // expand the cut downwards from the given place
    pCut = If_ObjCutBest(pObj);
    If_CutForEachLeaf( p, pCut, pLeaf, i )
    {
        Vec_PtrPush( vFront, pLeaf );
        Vec_PtrPush( vFrontOld, pLeaf );
        Vec_PtrPush( vVisited, pLeaf );
        pLeaf->fMark = 1;
    }
    // mark the nodes in the cone
    If_ManImproveMark_rec( p, pObj, vVisited );
}

/**Function*************************************************************

  Synopsis    [Updates the frontier.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManImproveNodeUpdate( If_Man_t * p, If_Obj_t * pObj, Vec_Ptr_t * vFront )
{
    If_Cut_t * pCut;
    If_Obj_t * pFanin;
    int i;
    pCut = If_ObjCutBest(pObj);
    // deref node's cut
    If_CutDeref( p, pCut, IF_INFINITY );
    // update the node's cut
    pCut->nLeaves = Vec_PtrSize(vFront);
    Vec_PtrForEachEntry( vFront, pFanin, i )
        pCut->pLeaves[i] = pFanin->Id;
    // ref the new cut
    If_CutRef( p, pCut, IF_INFINITY );
}


/**Function*************************************************************

  Synopsis    [Returns 1 if the number of fanins will grow.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_ManImproveNodeWillGrow( If_Man_t * p, If_Obj_t * pObj )
{
    If_Obj_t * pFanin0, * pFanin1;
    assert( If_ObjIsAnd(pObj) );
    pFanin0 = If_ObjFanin0(pObj);
    pFanin1 = If_ObjFanin1(pObj);
    return !pFanin0->fMark && !pFanin1->fMark;
}

/**Function*************************************************************

  Synopsis    [Returns the increase in the number of fanins with no external refs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_ManImproveNodeFaninCost( If_Man_t * p, If_Obj_t * pObj )
{
    int Counter = 0;
    assert( If_ObjIsAnd(pObj) );
    // check if the node has external refs
    if ( pObj->nRefs == 0 )
        Counter--;
    // increment the number of fanins without external refs
    if ( !If_ObjFanin0(pObj)->fMark && If_ObjFanin0(pObj)->nRefs == 0 )
        Counter++;
    // increment the number of fanins without external refs
    if ( !If_ObjFanin1(pObj)->fMark && If_ObjFanin1(pObj)->nRefs == 0 )
        Counter++;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Updates the frontier.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManImproveNodeFaninUpdate( If_Man_t * p, If_Obj_t * pObj, Vec_Ptr_t * vFront, Vec_Ptr_t * vVisited )
{
    If_Obj_t * pFanin;
    assert( If_ObjIsAnd(pObj) );
    Vec_PtrRemove( vFront, pObj );
    pFanin = If_ObjFanin0(pObj);
    if ( !pFanin->fMark )
    {
        Vec_PtrPush( vFront, pFanin );
        Vec_PtrPush( vVisited, pFanin );
        pFanin->fMark = 1;
    }
    pFanin = If_ObjFanin1(pObj);
    if ( !pFanin->fMark )
    {
        Vec_PtrPush( vFront, pFanin );
        Vec_PtrPush( vVisited, pFanin );
        pFanin->fMark = 1;
    }
}

/**Function*************************************************************

  Synopsis    [Compacts the number of external refs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_ManImproveNodeFaninCompact0( If_Man_t * p, If_Obj_t * pObj, int nLimit, Vec_Ptr_t * vFront, Vec_Ptr_t * vVisited )
{
    If_Obj_t * pFanin;
    int i;
    Vec_PtrForEachEntry( vFront, pFanin, i )
    {
        if ( If_ObjIsCi(pFanin) )
            continue;
        if ( If_ManImproveNodeWillGrow(p, pFanin) )
            continue;
        if ( If_ManImproveNodeFaninCost(p, pFanin) <= 0 )
        {
            If_ManImproveNodeFaninUpdate( p, pFanin, vFront, vVisited );
            return 1;
        }
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Compacts the number of external refs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_ManImproveNodeFaninCompact1( If_Man_t * p, If_Obj_t * pObj, int nLimit, Vec_Ptr_t * vFront, Vec_Ptr_t * vVisited )
{
    If_Obj_t * pFanin;
    int i;
    Vec_PtrForEachEntry( vFront, pFanin, i )
    {
        if ( If_ObjIsCi(pFanin) )
            continue;
        if ( If_ManImproveNodeFaninCost(p, pFanin) < 0 )
        {
            If_ManImproveNodeFaninUpdate( p, pFanin, vFront, vVisited );
            return 1;
        }
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Compacts the number of external refs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_ManImproveNodeFaninCompact2( If_Man_t * p, If_Obj_t * pObj, int nLimit, Vec_Ptr_t * vFront, Vec_Ptr_t * vVisited )
{
    If_Obj_t * pFanin;
    int i;
    Vec_PtrForEachEntry( vFront, pFanin, i )
    {
        if ( If_ObjIsCi(pFanin) )
            continue;
        if ( If_ManImproveNodeFaninCost(p, pFanin) <= 0 )
        {
            If_ManImproveNodeFaninUpdate( p, pFanin, vFront, vVisited );
            return 1;
        }
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Compacts the number of external refs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_ManImproveNodeFaninCompact_int( If_Man_t * p, If_Obj_t * pObj, int nLimit, Vec_Ptr_t * vFront, Vec_Ptr_t * vVisited )
{
    if ( If_ManImproveNodeFaninCompact0(p, pObj, nLimit, vFront, vVisited) )
        return 1;
    if (  Vec_PtrSize(vFront) < nLimit && If_ManImproveNodeFaninCompact1(p, pObj, nLimit, vFront, vVisited) )
        return 1;
    if ( Vec_PtrSize(vFront) < nLimit && If_ManImproveNodeFaninCompact2(p, pObj, nLimit, vFront, vVisited) )
        return 1;
    assert( Vec_PtrSize(vFront) <= nLimit );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Compacts the number of external refs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManImproveNodeFaninCompact( If_Man_t * p, If_Obj_t * pObj, int nLimit, Vec_Ptr_t * vFront, Vec_Ptr_t * vVisited )
{
    while ( If_ManImproveNodeFaninCompact_int( p, pObj, nLimit, vFront, vVisited ) );
}





/**Function*************************************************************

  Synopsis    [Performs fast mapping for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManImproveNodeReduce( If_Man_t * p, If_Obj_t * pObj, int nLimit )
{
/*
    If_Cut_t * pCut, * pCut0, * pCut1, * pCutR;
    If_Obj_t * pFanin0, * pFanin1;
    float AreaBef, AreaAft;
    int RetValue;

    assert( nLimit <= 32 );
    assert( If_ObjIsAnd(pObj) );
    // get the fanins
    pFanin0 = If_ObjFanin0(pObj);
    pFanin1 = If_ObjFanin1(pObj);
    // get the cuts
    pCut  = If_ObjCutBest(pObj);
    pCut0 = If_ObjIsCi(pFanin0) ? If_ObjCutTriv(pFanin0) : If_ObjCutBest(pFanin0);
    pCut1 = If_ObjIsCi(pFanin1) ? If_ObjCutTriv(pFanin1) : If_ObjCutBest(pFanin1);
    assert( pCut->Delay <= pObj->Required + p->fEpsilon );

    // deref the cut if the node is refed
    if ( pObj->nRefs > 0 )
        If_CutDeref( p, pCut, IF_INFINITY );
    // get the area
    AreaBef = If_CutAreaDerefed( p, pCut, IF_INFINITY );
    // get the fanin support
    if ( pFanin0->nRefs > 2 && pCut0->Delay < pObj->Required + p->fEpsilon )
//    if ( pSupp0->nRefs > 0 && pSupp0->Delay < pSupp->DelayR ) // this leads to 2% worse results
    {
        pCut0 = If_ObjCutTriv(pFanin0);
    }
    // get the fanin support
    if ( pFanin1->nRefs > 2 && pCut1->Delay < pObj->Required + p->fEpsilon )
//    if ( pSupp1->nRefs > 0 && pSupp1->Delay < pSupp->DelayR )
    {
        pCut1 = If_ObjCutTriv(pFanin1);
    }

    // merge the cuts
    pCutR = p->ppCuts[0];
    RetValue = If_CutMerge( pCut0, pCut1, pCutR );
    // try very simple cut
    if ( !RetValue )
    {
        RetValue = If_CutMerge( If_ObjCutTriv(pFanin0), If_ObjCutTriv(pFanin1), pCutR );
        assert( RetValue == 1 );
    }
    if ( RetValue )
    {
        pCutR->Delay = If_CutDelay( p, pCutR );
        AreaAft = If_CutAreaDerefed( p, pCutR, IF_INFINITY );
        // update the best cut
        if ( AreaAft < AreaBef - p->fEpsilon && pCutR->Delay < pObj->Required + p->fEpsilon )
            If_CutCopy( p, pCut, pCutR );
    }
    // recompute the delay of the best cut
    pCut->Delay = If_CutDelay( p, pCut );
    // ref the cut if the node is refed
    if ( pObj->nRefs > 0 )
        If_CutRef( p, pCut, IF_INFINITY );
*/
}

/**Function*************************************************************

  Synopsis    [Performs area recovery for each node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManImproveReduce( If_Man_t * p, int nLimit )
{
    If_Obj_t * pObj;
    int i;
    If_ManForEachNode( p, pObj, i )
        If_ManImproveNodeReduce( p, pObj, nLimit );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


