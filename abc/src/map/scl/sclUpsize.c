/**CFile****************************************************************

  FileName    [sclUpsize.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Standard-cell library representation.]

  Synopsis    [Selective increase of gate sizes.]

  Author      [Alan Mishchenko, Niklas Een]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 24, 2012.]

  Revision    [$Id: sclUpsize.c,v 1.0 2012/08/24 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sclSize.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Collect TFO of nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_SclFindTFO_rec( Abc_Obj_t * pObj, Vec_Int_t * vNodes, Vec_Int_t * vCos )
{
    Abc_Obj_t * pNext;
    int i;
    if ( Abc_NodeIsTravIdCurrent( pObj ) )
        return;
    Abc_NodeSetTravIdCurrent( pObj );
    if ( Abc_ObjIsCo(pObj) )
    {
        Vec_IntPush( vCos, Abc_ObjId(pObj) );
        return;
    }
    assert( Abc_ObjIsNode(pObj) );
    Abc_ObjForEachFanout( pObj, pNext, i )
        Abc_SclFindTFO_rec( pNext, vNodes, vCos );
    if ( Abc_ObjFaninNum(pObj) > 0 )
        Vec_IntPush( vNodes, Abc_ObjId(pObj) );
}
Vec_Int_t * Abc_SclFindTFO( Abc_Ntk_t * p, Vec_Int_t * vPath )
{
    Vec_Int_t * vNodes, * vCos;
    Abc_Obj_t * pObj, * pFanin;
    int i, k;
    assert( Vec_IntSize(vPath) > 0 );
    vCos = Vec_IntAlloc( 100 );
    vNodes = Vec_IntAlloc( 100 );
    // collect nodes in the TFO
    Abc_NtkIncrementTravId( p ); 
    Abc_NtkForEachObjVec( vPath, p, pObj, i )
        Abc_ObjForEachFanin( pObj, pFanin, k )
            if ( Abc_ObjIsNode(pFanin) )
                Abc_SclFindTFO_rec( pFanin, vNodes, vCos );
    // reverse order
    Vec_IntReverseOrder( vNodes );
//    Vec_IntSort( vNodes, 0 );
//Vec_IntPrint( vNodes );
//Vec_IntPrint( vCos );
    Vec_IntAppend( vNodes, vCos );
    Vec_IntFree( vCos );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Collect near-critical COs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Abc_SclFindCriticalCoWindow( SC_Man * p, int Window )
{
    float fMaxArr = Abc_SclReadMaxDelay( p ) * (100.0 - Window) / 100.0;
    Vec_Int_t * vPivots;
    Abc_Obj_t * pObj;
    int i;
    vPivots = Vec_IntAlloc( 100 );
    Abc_NtkForEachCo( p->pNtk, pObj, i )
        if ( Abc_SclObjTimeMax(p, pObj) >= fMaxArr )
            Vec_IntPush( vPivots, Abc_ObjId(pObj) );
    assert( Vec_IntSize(vPivots) > 0 );
    return vPivots;
}

/**Function*************************************************************

  Synopsis    [Collect near-critical internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_SclFindCriticalNodeWindow_rec( SC_Man * p, Abc_Obj_t * pObj, Vec_Int_t * vPath, float fSlack, int fDept )
{
    Abc_Obj_t * pNext;
    float fArrMax, fSlackFan;
    int i;
    if ( Abc_ObjIsCi(pObj) )
        return;
    if ( Abc_NodeIsTravIdCurrent( pObj ) )
        return;
    Abc_NodeSetTravIdCurrent( pObj );
    assert( Abc_ObjIsNode(pObj) );
    // compute the max arrival time of the fanins
    if ( fDept )
//        fArrMax = p->pSlack[Abc_ObjId(pObj)];
        fArrMax = Abc_SclObjGetSlack(p, pObj, p->MaxDelay);
    else
        fArrMax = Abc_SclGetMaxDelayNodeFanins( p, pObj );
//    assert( fArrMax >= -1 );
    fArrMax = Abc_MaxFloat( fArrMax, 0 );
    // traverse all fanins whose arrival times are within a window
    Abc_ObjForEachFanin( pObj, pNext, i )
    {
        if ( Abc_ObjIsCi(pNext) || Abc_ObjFaninNum(pNext) == 0 )
            continue;
        assert( Abc_ObjIsNode(pNext) );
        if ( fDept )
//            fSlackFan = fSlack - (p->pSlack[Abc_ObjId(pNext)] - fArrMax);
            fSlackFan = fSlack - (Abc_SclObjGetSlack(p, pNext, p->MaxDelay) - fArrMax);
        else
            fSlackFan = fSlack - (fArrMax - Abc_SclObjTimeMax(p, pNext));
        if ( fSlackFan >= 0 )
            Abc_SclFindCriticalNodeWindow_rec( p, pNext, vPath, fSlackFan, fDept );
    }
    if ( Abc_ObjFaninNum(pObj) > 0 )
        Vec_IntPush( vPath, Abc_ObjId(pObj) );
}
Vec_Int_t * Abc_SclFindCriticalNodeWindow( SC_Man * p, Vec_Int_t * vPathCos, int Window, int fDept )
{
    float fMaxArr = Abc_SclReadMaxDelay( p );
    float fSlackMax = fMaxArr * Window / 100.0;
    Vec_Int_t * vPath = Vec_IntAlloc( 100 );
    Abc_Obj_t * pObj;
    int i;
    Abc_NtkIncrementTravId( p->pNtk ); 
    Abc_NtkForEachObjVec( vPathCos, p->pNtk, pObj, i )
    {
        float fSlackThis = fSlackMax - (fMaxArr - Abc_SclObjTimeMax(p, pObj));
        if ( fSlackThis >= 0 )
            Abc_SclFindCriticalNodeWindow_rec( p, Abc_ObjFanin0(pObj), vPath, fSlackThis, fDept );
    }
    // label critical nodes
    Abc_NtkForEachObjVec( vPathCos, p->pNtk, pObj, i )
        pObj->fMarkA = 1;
    Abc_NtkForEachObjVec( vPath, p->pNtk, pObj, i )
        pObj->fMarkA = 1;
    return vPath;  
}
void Abc_SclUnmarkCriticalNodeWindow( SC_Man * p, Vec_Int_t * vPath )
{
    Abc_Obj_t * pObj;
    int i;
    Abc_NtkForEachObjVec( vPath, p->pNtk, pObj, i )
        pObj->fMarkA = 0;
}
int Abc_SclCountNearCriticalNodes( SC_Man * p )
{
    int RetValue;
    Vec_Int_t * vPathPos, * vPathNodes;
    vPathPos   = Abc_SclFindCriticalCoWindow( p, 5 );
    vPathNodes = Abc_SclFindCriticalNodeWindow( p, vPathPos, 5, 0 );
    RetValue   = Vec_IntSize(vPathNodes);
    Abc_SclUnmarkCriticalNodeWindow( p, vPathNodes );
    Abc_SclUnmarkCriticalNodeWindow( p, vPathPos );
    Vec_IntFree( vPathPos );
    Vec_IntFree( vPathNodes );
    return RetValue;
}


/**Function*************************************************************

  Synopsis    [Find the array of nodes to be updated.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_SclFindNodesToUpdate( Abc_Obj_t * pPivot, Vec_Int_t ** pvNodes, Vec_Int_t ** pvEvals, Abc_Obj_t * pExtra )
{
    Abc_Ntk_t * p = Abc_ObjNtk(pPivot);
    Abc_Obj_t * pObj, * pNext, * pNext2;
    Vec_Int_t * vNodes = *pvNodes;
    Vec_Int_t * vEvals = *pvEvals;
    int i, k;
    assert( Abc_ObjIsNode(pPivot) );
    assert( pPivot->fMarkA );
    // collect fanins, node, and fanouts
    Vec_IntClear( vNodes );
    Abc_ObjForEachFanin( pPivot, pNext, i )
//        if ( Abc_ObjIsNode(pNext) && Abc_ObjFaninNum(pNext) > 0 )
        if ( Abc_ObjIsCi(pNext) || Abc_ObjFaninNum(pNext) > 0 )
            Vec_IntPush( vNodes, Abc_ObjId(pNext) );
    Vec_IntPush( vNodes, Abc_ObjId(pPivot) );
    if ( pExtra )
        Vec_IntPush( vNodes, Abc_ObjId(pExtra) );
    Abc_ObjForEachFanout( pPivot, pNext, i )
        if ( Abc_ObjIsNode(pNext) && pNext->fMarkA )
        {
            Vec_IntPush( vNodes, Abc_ObjId(pNext) );
            Abc_ObjForEachFanout( pNext, pNext2, k )
                if ( Abc_ObjIsNode(pNext2) && pNext2->fMarkA )
                    Vec_IntPush( vNodes, Abc_ObjId(pNext2) );
        }
    Vec_IntUniqify( vNodes );
    // label nodes
    Abc_NtkForEachObjVec( vNodes, p, pObj, i )
    {
        assert( pObj->fMarkB == 0 );
        pObj->fMarkB = 1;
    }
    // collect nodes visible from the critical paths
    Vec_IntClear( vEvals );
    Abc_NtkForEachObjVec( vNodes, p, pObj, i )
        Abc_ObjForEachFanout( pObj, pNext, k )
            if ( pNext->fMarkA && !pNext->fMarkB )
//            if ( !pNext->fMarkB )
            {
                assert( pObj->fMarkB );
                Vec_IntPush( vEvals, Abc_ObjId(pObj) );
                break;
            }
    assert( Vec_IntSize(vEvals) > 0 );
    // label nodes
    Abc_NtkForEachObjVec( vNodes, p, pObj, i )
        pObj->fMarkB = 0;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_SclFindBestCell( SC_Man * p, Abc_Obj_t * pObj, Vec_Int_t * vRecalcs, Vec_Int_t * vEvals, int Notches, int DelayGap, float * pGainBest )
{
    SC_Cell * pCellOld, * pCellNew;
    float dGain, dGainBest;
    int k, gateBest, NoChange = 0;
    // save old gate, timing, fanin load
    pCellOld = Abc_SclObjCell( pObj );
    Abc_SclConeStore( p, vRecalcs );
    Abc_SclEvalStore( p, vEvals );
    Abc_SclLoadStore( p, pObj );
    // try different gate sizes for this node
    gateBest = -1;
    dGainBest = -DelayGap;
    SC_RingForEachCell( pCellOld, pCellNew, k )
    {
        if ( pCellNew == pCellOld )
            continue;
        if ( k > Notches )
            break;
        // set new cell
        Abc_SclObjSetCell( pObj, pCellNew );
        Abc_SclUpdateLoad( p, pObj, pCellOld, pCellNew );
        // recompute timing
        Abc_SclTimeCone( p, vRecalcs );
        // set old cell
        Abc_SclObjSetCell( pObj, pCellOld );
        Abc_SclLoadRestore( p, pObj );
        // save best gain
        dGain = Abc_SclEvalPerform( p, vEvals );
        if ( dGainBest < dGain )
        {
            dGainBest = dGain;
            gateBest = pCellNew->Id;
            NoChange = 1;
        }
        else if ( NoChange )
            NoChange++;
        if ( NoChange == 4 )
            break;
//        printf( "%.2f ", dGain );
    }
//    printf( "Best = %.2f   ", dGainBest );
//    printf( "\n" );
    // put back old cell and timing
    Abc_SclObjSetCell( pObj, pCellOld );
    Abc_SclConeRestore( p, vRecalcs );
    *pGainBest = dGainBest;
    return gateBest;
}

/**Function*************************************************************

  Synopsis    [Computes the set of gates to upsize.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_SclFindBypasses( SC_Man * p, Vec_Int_t * vPathNodes, int Ratio, int Notches, int iIter, int DelayGap, int fVeryVerbose )
{
    SC_Cell * pCellOld, * pCellNew;
    Vec_Ptr_t * vFanouts;
    Vec_Int_t * vRecalcs, * vEvals;
    Abc_Obj_t * pBuf, * pFanin, * pFanout, * pExtra;
    int i, j, iNode, gateBest, gateBest2, fanBest, Counter = 0;
    float dGainBest, dGainBest2;

    // compute savings due to bypassing buffers
    vFanouts = Vec_PtrAlloc( 100 );
    vRecalcs = Vec_IntAlloc( 100 );
    vEvals = Vec_IntAlloc( 100 );
    Vec_QueClear( p->vNodeByGain );
    Abc_NtkForEachObjVec( vPathNodes, p->pNtk, pBuf, i )
    {
        assert( pBuf->fMarkB == 0 );
        if ( Abc_ObjFaninNum(pBuf) != 1 )
            continue;
        pFanin = Abc_ObjFanin0(pBuf);
        if ( !Abc_ObjIsNode(pFanin) )
            continue;
        pExtra = NULL;
        if ( p->pNtk->vPhases == NULL )
        {
            if ( Abc_SclIsInv(pBuf) )
            {
                if ( !Abc_SclIsInv(pFanin) )
                    continue;
                pFanin = Abc_ObjFanin0(pFanin);
                if ( !Abc_ObjIsNode(pFanin) )
                    continue;
                pExtra = pBuf;
                // we make pBuf and pFanin are in the same phase and pFanin is a node
            }
        }
        // here we have pBuf and its fanin pFanin, which is a logic node
        // compute nodes to recalculate timing and nodes to evaluate afterwards
        Abc_SclFindNodesToUpdate( pFanin, &vRecalcs, &vEvals, pExtra );
        assert( Vec_IntSize(vEvals) > 0 );
        // consider fanouts of this node
        fanBest    = -1;
        gateBest2  = -1;
        dGainBest2 =  0;
        Abc_NodeCollectFanouts( pBuf, vFanouts );
        Vec_PtrForEachEntry( Abc_Obj_t *, vFanouts, pFanout, j )
        {
            // skip COs
            if ( Abc_ObjIsCo(pFanout) )
                continue;
            // skip non-critical fanouts
            if ( !pFanout->fMarkA )
                continue;
            // skip if fanin already has fanout as a fanout
            if ( Abc_NodeFindFanin(pFanout, pFanin) >= 0 )
                continue;
            // skip if fanin already has fanout as a fanout
            if ( pExtra && Abc_NodeFindFanin(pFanout, pExtra) >= 0 )
                continue;
            // prepare
            Abc_SclLoadStore3( p, pBuf );
            Abc_SclUpdateLoadSplit( p, pBuf, pFanout );
            Abc_ObjPatchFanin( pFanout, pBuf, pFanin );
            // size the fanin
            gateBest = Abc_SclFindBestCell( p, pFanin, vRecalcs, vEvals, Notches, DelayGap, &dGainBest );
            // unprepare
            Abc_SclLoadRestore3( p, pBuf );
            Abc_ObjPatchFanin( pFanout, pFanin, pBuf );
            if ( gateBest == -1 )
                continue;
            // compare gain
            if ( dGainBest2 < dGainBest )
            {
                dGainBest2 = dGainBest;
                gateBest2 = gateBest;
                fanBest = Abc_ObjId(pFanout);
            }
        }
        // remember savings
        if ( gateBest2 >= 0 )
        {
            assert( dGainBest2 > 0.0 );
            Vec_FltWriteEntry( p->vNode2Gain, Abc_ObjId(pBuf), dGainBest2 );
            Vec_IntWriteEntry( p->vNode2Gate, Abc_ObjId(pBuf), gateBest2 );
            Vec_QuePush( p->vNodeByGain, Abc_ObjId(pBuf) );
            Vec_IntWriteEntry( p->vBestFans, Abc_ObjId(pBuf), fanBest );
        }
//        if ( ++Counter == 17 )
//            break;
    }
    Vec_PtrFree( vFanouts );
    Vec_IntFree( vRecalcs );
    Vec_IntFree( vEvals );
    if ( Vec_QueSize(p->vNodeByGain) == 0 )
        return 0;
    if ( fVeryVerbose ) 
        printf( "\n" );

    // accept changes for that are half above the average and do not overlap
    Counter = 0;
    dGainBest2 = -1;
    vFanouts = Vec_PtrAlloc( 100 );
    while ( Vec_QueSize(p->vNodeByGain) )
    {
        iNode   = Vec_QuePop(p->vNodeByGain);
        pFanout = Abc_NtkObj( p->pNtk, Vec_IntEntry(p->vBestFans, iNode) );
        pBuf    = Abc_NtkObj( p->pNtk, iNode );
        pFanin  = Abc_ObjFanin0(pBuf);
        if ( pFanout->fMarkB || pBuf->fMarkB )
            continue;
        if ( p->pNtk->vPhases == NULL )
        {
            // update fanin
            if ( Abc_SclIsInv(pBuf) )
            {
                if ( !Abc_SclIsInv(pFanin) )
                {
                    assert( 0 );
                    continue;
                }
                pFanin = Abc_ObjFanin0(pFanin);
                if ( !Abc_ObjIsNode(pFanin) )
                {
                    assert( 0 );
                    continue;
                }
            }
        }
        if ( pFanin->fMarkB )
            continue;
        pFanout->fMarkB = 1;
        pBuf->fMarkB = 1;
        pFanin->fMarkB = 1;
        Vec_PtrPush( vFanouts, pFanout );
        Vec_PtrPush( vFanouts, pBuf );
        Vec_PtrPush( vFanouts, pFanin );
        // remember gain
        if ( dGainBest2 == -1 )
            dGainBest2 = Vec_FltEntry(p->vNode2Gain, iNode);
//        else if ( dGainBest2 > 2*Vec_FltEntry(p->vNode2Gain, iNode) )
//            break;
        // redirect
        Abc_SclUpdateLoadSplit( p, pBuf, pFanout );
        Abc_SclAddWireLoad( p, pBuf, 1 );
        Abc_SclAddWireLoad( p, pFanin, 1 );
        Abc_ObjPatchFanin( pFanout, pBuf, pFanin );
        Abc_SclAddWireLoad( p, pBuf, 0 );
        Abc_SclAddWireLoad( p, pFanin, 0 );
        Abc_SclTimeIncUpdateLevel( pFanout );
        // remember
        Vec_IntPush( p->vUpdates2, Abc_ObjId(pFanout) );
        Vec_IntPush( p->vUpdates2, Abc_ObjId(pFanin) );
        Vec_IntPush( p->vUpdates2, Abc_ObjId(pBuf) );
        // update cell
        pCellOld = Abc_SclObjCell( pFanin );
        pCellNew = SC_LibCell( p->pLib, Vec_IntEntry(p->vNode2Gate, iNode) );
        p->SumArea += pCellNew->area - pCellOld->area;
        Abc_SclObjSetCell( pFanin, pCellNew );
        Abc_SclUpdateLoad( p, pFanin, pCellOld, pCellNew );
        // record the update
        Vec_IntPush( p->vUpdates, Abc_ObjId(pFanin) );
        Vec_IntPush( p->vUpdates, pCellNew->Id );
        Abc_SclTimeIncInsert( p, pFanout );
        Abc_SclTimeIncInsert( p, pBuf );
        Abc_SclTimeIncInsert( p, pFanin );
        // remember when this node was upsized
        Vec_IntWriteEntry( p->vNodeIter, Abc_ObjId(pFanout), -1 );
        Vec_IntWriteEntry( p->vNodeIter, Abc_ObjId(pBuf), -1 );
        Vec_IntWriteEntry( p->vNodeIter, Abc_ObjId(pFanin), -1 );
        // update polarity
        if ( p->pNtk->vPhases && Abc_SclIsInv(pBuf) )
            Abc_NodeInvUpdateObjFanoutPolarity( pFanin, pFanout );
        // report
        if ( fVeryVerbose )
        {
            printf( "Node %6d  Redir fanout %6d to fanin %6d.  Gain = %7.1f ps. ", 
                Abc_ObjId(pBuf), Abc_ObjId(pFanout), Abc_ObjId(pFanin), Vec_FltEntry(p->vNode2Gain, iNode) );
            printf( "Gate %12s (%2d/%2d)  -> %12s (%2d/%2d) \n", 
                pCellOld->pName, pCellOld->Order, pCellOld->nGates, 
                pCellNew->pName, pCellNew->Order, pCellNew->nGates );
        }
/*
        // check if the node became useless
        if ( Abc_ObjFanoutNum(pBuf) == 0 )
        {
            pCellOld = Abc_SclObjCell( pBuf );
            p->SumArea -= pCellOld->area;
            Abc_NtkDeleteObj_rec( pBuf, 1 );
            printf( "Removed node %d.\n", iNode );
        }
*/
        Counter++;
    }
    Vec_PtrForEachEntry( Abc_Obj_t *, vFanouts, pFanout, j )
        pFanout->fMarkB = 0;
    Vec_PtrFree( vFanouts );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Check marked fanin/fanouts.]

  Description []
               
  SideEffects []

  SeeAlso     []
 
***********************************************************************/
int Abc_SclObjCheckMarkedFanFans( Abc_Obj_t * pObj )
{
    Abc_Obj_t * pNext;
    int i;
    if ( pObj->fMarkB )
        return 1;
    Abc_ObjForEachFanin( pObj, pNext, i )
        if ( pNext->fMarkB )
            return 1;
    Abc_ObjForEachFanout( pObj, pNext, i )
        if ( pNext->fMarkB )
            return 1;
    return 0;
}
void Abc_SclObjMarkFanFans( Abc_Obj_t * pObj, Vec_Ptr_t * vNodes )
{
//    Abc_Obj_t * pNext;
//    int i;
    if ( pObj->fMarkB == 0 )
    {
        Vec_PtrPush( vNodes, pObj );
        pObj->fMarkB = 1;
    }
/*
    Abc_ObjForEachFanin( pObj, pNext, i )
        if ( pNext->fMarkB == 0 )
        {
            Vec_PtrPush( vNodes, pNext );
            pNext->fMarkB = 1;
        }
    Abc_ObjForEachFanout( pObj, pNext, i )
        if ( pNext->fMarkB == 0 )
        {
            Vec_PtrPush( vNodes, pNext );
            pNext->fMarkB = 1;
        }
*/
}

/**Function*************************************************************

  Synopsis    [Computes the set of gates to upsize.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_SclFindUpsizes( SC_Man * p, Vec_Int_t * vPathNodes, int Ratio, int Notches, int iIter, int DelayGap, int fMoreConserf )
{
    SC_Cell * pCellOld, * pCellNew;
    Vec_Int_t * vRecalcs, * vEvals;
    Vec_Ptr_t * vFanouts;
    Abc_Obj_t * pObj;
    float dGainBest, dGainBest2;
    int i, gateBest, Limit, Counter, iIterLast;

    // compute savings due to upsizing each node
    vRecalcs = Vec_IntAlloc( 100 );
    vEvals = Vec_IntAlloc( 100 );
    Vec_QueClear( p->vNodeByGain );
    Abc_NtkForEachObjVec( vPathNodes, p->pNtk, pObj, i )
    {
        assert( pObj->fMarkB == 0 );
        iIterLast = Vec_IntEntry(p->vNodeIter, Abc_ObjId(pObj));
        if ( iIterLast >= 0 && iIterLast + 5 > iIter )
            continue;
        // compute nodes to recalculate timing and nodes to evaluate afterwards
        Abc_SclFindNodesToUpdate( pObj, &vRecalcs, &vEvals, NULL );
        assert( Vec_IntSize(vEvals) > 0 );
        //printf( "%d -> %d\n", Vec_IntSize(vRecalcs), Vec_IntSize(vEvals) );
        gateBest = Abc_SclFindBestCell( p, pObj, vRecalcs, vEvals, Notches, DelayGap, &dGainBest );
        // remember savings
        if ( gateBest >= 0 )
        {
            assert( dGainBest > 0.0 );
            Vec_FltWriteEntry( p->vNode2Gain, Abc_ObjId(pObj), dGainBest );
            Vec_IntWriteEntry( p->vNode2Gate, Abc_ObjId(pObj), gateBest );
            Vec_QuePush( p->vNodeByGain, Abc_ObjId(pObj) );
        }
    }
    Vec_IntFree( vRecalcs );
    Vec_IntFree( vEvals );
    if ( Vec_QueSize(p->vNodeByGain) == 0 )
        return 0;
/*
    Limit = Abc_MinInt( Vec_QueSize(p->vNodeByGain), Abc_MaxInt((int)(0.01 * Ratio * Vec_IntSize(vPathNodes)), 1) ); 
    //printf( "\nSelecting %d out of %d\n", Limit, Vec_QueSize(p->vNodeByGain) );
    for ( i = 0; i < Limit; i++ )
    {
        // get the object
        pObj = Abc_NtkObj( p->pNtk, Vec_QuePop(p->vNodeByGain) );
        assert( pObj->fMarkA );
        // find old and new gates
        pCellOld = Abc_SclObjCell( pObj );
        pCellNew = SC_LibCell( p->pLib, Vec_IntEntry(p->vNode2Gate, Abc_ObjId(pObj)) );
        assert( pCellNew != NULL );
        //printf( "%6d  %20s -> %20s  ", Abc_ObjId(pObj), pCellOld->pName, pCellNew->pName );
        //printf( "gain is %f\n", Vec_FltEntry(p->vNode2Gain, Abc_ObjId(pObj)) );
        // update gate
        Abc_SclUpdateLoad( p, pObj, pCellOld, pCellNew );
        p->SumArea += pCellNew->area - pCellOld->area;
        Abc_SclObjSetCell( pObj, pCellNew );
        // record the update
        Vec_IntPush( p->vUpdates, Abc_ObjId(pObj) );
        Vec_IntPush( p->vUpdates, pCellNew->Id );
        Abc_SclTimeIncInsert( p, pObj );
        // remember when this node was upsized
        Vec_IntWriteEntry( p->vNodeIter, Abc_ObjId(pObj), iIter );
    }
return Limit;
*/

    Limit = Abc_MinInt( Vec_QueSize(p->vNodeByGain), Abc_MaxInt((int)(0.01 * Ratio * Vec_IntSize(vPathNodes)), 1) ); 
    dGainBest2 = -1;
    Counter = 0;
    vFanouts = Vec_PtrAlloc( 100 );
    while ( Vec_QueSize(p->vNodeByGain) )
    {
        int iNode = Vec_QuePop(p->vNodeByGain);
        Abc_Obj_t * pObj = Abc_NtkObj( p->pNtk, iNode );
        assert( pObj->fMarkA );
        if ( Abc_SclObjCheckMarkedFanFans( pObj ) )
            continue;
        Abc_SclObjMarkFanFans( pObj, vFanouts );
        // remember gain
        if ( dGainBest2 == -1 )
            dGainBest2 = Vec_FltEntry(p->vNode2Gain, iNode);
//        else if ( dGainBest2 > 3*Vec_FltEntry(p->vNode2Gain, iNode) )
//            break;
//        printf( "%.1f ", Vec_FltEntry(p->vNode2Gain, iNode) );

        // find old and new gates
        pCellOld = Abc_SclObjCell( pObj );
        pCellNew = SC_LibCell( p->pLib, Vec_IntEntry(p->vNode2Gate, Abc_ObjId(pObj)) );
        assert( pCellNew != NULL );
        //printf( "%6d  %20s -> %20s  ", Abc_ObjId(pObj), pCellOld->pName, pCellNew->pName );
        //printf( "gain is %f\n", Vec_FltEntry(p->vNode2Gain, Abc_ObjId(pObj)) );
//        if ( pCellOld->Order > 0 )
//            printf( "%.2f  %d -> %d(%d)   ", Vec_FltEntry(p->vNode2Gain, iNode), pCellOld->Order, pCellNew->Order, pCellNew->nGates );
        // update gate
        p->SumArea += pCellNew->area - pCellOld->area;
        Abc_SclObjSetCell( pObj, pCellNew );
        Abc_SclUpdateLoad( p, pObj, pCellOld, pCellNew );
        // record the update
        Vec_IntPush( p->vUpdates, Abc_ObjId(pObj) );
        Vec_IntPush( p->vUpdates, pCellNew->Id );
        Abc_SclTimeIncInsert( p, pObj );
        // remember when this node was upsized
        Vec_IntWriteEntry( p->vNodeIter, Abc_ObjId(pObj), iIter );
        Counter++;
        if ( Counter == Limit )
            break;
    }
//    printf( "\n" );

    Vec_PtrForEachEntry( Abc_Obj_t *, vFanouts, pObj, i )
        pObj->fMarkB = 0;
    Vec_PtrFree( vFanouts );
    return Counter;
}
void Abc_SclApplyUpdateToBest( Vec_Int_t * vGatesBest, Vec_Int_t * vGates, Vec_Int_t * vUpdate )
{
    int i, ObjId, GateId, GateId2; 
    Vec_IntForEachEntryDouble( vUpdate, ObjId, GateId, i )
        Vec_IntWriteEntry( vGatesBest, ObjId, GateId );
    Vec_IntClear( vUpdate );
    Vec_IntForEachEntryTwo( vGatesBest, vGates, GateId, GateId2, i )
        assert( GateId == GateId2 );
//    Vec_IntClear( vGatesBest );
//    Vec_IntAppend( vGatesBest, vGates );
}
void Abc_SclUndoRecentChanges( Abc_Ntk_t * pNtk, Vec_Int_t * vTrans )
{
    int i;
    assert( Vec_IntSize(vTrans) % 3 == 0 );
    for ( i = Vec_IntSize(vTrans)/3 - 1; i >= 0; i-- )
    {
        Abc_Obj_t * pFanout = Abc_NtkObj( pNtk, Vec_IntEntry(vTrans, 3*i+0) );
        Abc_Obj_t * pFanin  = Abc_NtkObj( pNtk, Vec_IntEntry(vTrans, 3*i+1) );
        Abc_Obj_t * pObj    = Abc_NtkObj( pNtk, Vec_IntEntry(vTrans, 3*i+2) );
        // we do not update load here because times will be recomputed
        Abc_ObjPatchFanin( pFanout, pFanin, pObj );
        Abc_SclTimeIncUpdateLevel( pFanout );
//        printf( "Node %6d  Redir fanout %6d from fanin %6d. \n", 
//            Abc_ObjId(pObj), Abc_ObjId(pFanout), Abc_ObjId(pFanin) );
        // update polarity
        if ( pNtk->vPhases && Abc_SclIsInv(pObj) )
            Abc_NodeInvUpdateObjFanoutPolarity( pObj, pFanout );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_SclUpsizePrintDiffs( SC_Man * p, SC_Lib * pLib, Abc_Ntk_t * pNtk )
{
    float fDiff = (float)0.001;
    int k;
    Abc_Obj_t * pObj;

    SC_Pair * pTimes = ABC_ALLOC( SC_Pair, p->nObjs );
    SC_Pair * pSlews = ABC_ALLOC( SC_Pair, p->nObjs );
    SC_Pair * pLoads = ABC_ALLOC( SC_Pair, p->nObjs );

    memcpy( pTimes, p->pTimes, sizeof(SC_Pair) * p->nObjs );
    memcpy( pSlews, p->pSlews, sizeof(SC_Pair) * p->nObjs );
    memcpy( pLoads, p->pLoads, sizeof(SC_Pair) * p->nObjs );

    Abc_SclTimeNtkRecompute( p, NULL, NULL, 0, 0 );

    Abc_NtkForEachNode( pNtk, pObj, k )
    {
        if ( Abc_AbsFloat(p->pLoads[k].rise - pLoads[k].rise) > fDiff )
            printf( "%6d : load rise differs %12.6f   %f %f\n", k, p->pLoads[k].rise-pLoads[k].rise, p->pLoads[k].rise, pLoads[k].rise );
        if ( Abc_AbsFloat(p->pLoads[k].fall - pLoads[k].fall) > fDiff )
            printf( "%6d : load fall differs %12.6f   %f %f\n", k, p->pLoads[k].fall-pLoads[k].fall, p->pLoads[k].fall, pLoads[k].fall );

        if ( Abc_AbsFloat(p->pSlews[k].rise - pSlews[k].rise) > fDiff )
            printf( "%6d : slew rise differs %12.6f   %f %f\n", k, p->pSlews[k].rise-pSlews[k].rise, p->pSlews[k].rise, pSlews[k].rise );
        if ( Abc_AbsFloat(p->pSlews[k].fall - pSlews[k].fall) > fDiff )
            printf( "%6d : slew fall differs %12.6f   %f %f\n", k, p->pSlews[k].fall-pSlews[k].fall, p->pSlews[k].fall, pSlews[k].fall );

        if ( Abc_AbsFloat(p->pTimes[k].rise - pTimes[k].rise) > fDiff )
            printf( "%6d : time rise differs %12.6f   %f %f\n", k, p->pTimes[k].rise-pTimes[k].rise, p->pTimes[k].rise, pTimes[k].rise );
        if ( Abc_AbsFloat(p->pTimes[k].fall - pTimes[k].fall) > fDiff )
            printf( "%6d : time fall differs %12.6f   %f %f\n", k, p->pTimes[k].fall-pTimes[k].fall, p->pTimes[k].fall, pTimes[k].fall );
    }

/*
if ( memcmp( pTimes, p->pTimes, sizeof(SC_Pair) * p->nObjs ) )
    printf( "Times differ!\n" );
if ( memcmp( pSlews, p->pSlews, sizeof(SC_Pair) * p->nObjs ) )
    printf( "Slews differ!\n" );
if ( memcmp( pLoads, p->pLoads, sizeof(SC_Pair) * p->nObjs ) )
    printf( "Loads differ!\n" );
*/

    ABC_FREE( pTimes );
    ABC_FREE( pSlews );
    ABC_FREE( pLoads );
}

/**Function*************************************************************

  Synopsis    [Print cumulative statistics.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_SclUpsizePrint( SC_Man * p, int Iter, int win, int nPathPos, int nPathNodes, int nUpsizes, int nTFOs, int fVerbose )
{
    printf( "%4d ",          Iter );
    printf( "Win:%3d. ",     win );
    printf( "PO:%6d. ",      nPathPos );
    printf( "Path:%7d. ",    nPathNodes );
    printf( "Gate:%5d. ",    nUpsizes );
    printf( "TFO:%7d. ",     nTFOs );
    printf( "A: " );
    printf( "%.2f ",         p->SumArea );
    printf( "(%+5.1f %%)  ", 100.0 * (p->SumArea - p->SumArea0)/ p->SumArea0 );
    printf( "D: " );
    printf( "%.2f ps ",      p->MaxDelay );
    printf( "(%+5.1f %%)  ", 100.0 * (p->MaxDelay - p->MaxDelay0)/ p->MaxDelay0 );
    printf( "B: " );
    printf( "%.2f ps ",      p->BestDelay );
    printf( "(%+5.1f %%)",   100.0 * (p->BestDelay - p->MaxDelay0)/ p->MaxDelay0 );
    printf( "%8.2f sec    ", 1.0*(Abc_Clock() - p->timeTotal)/(CLOCKS_PER_SEC) );
    printf( "%c", fVerbose ? '\n' : '\r' );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_SclUpsizeRemoveDangling( SC_Man * p, Abc_Ntk_t * pNtk )
{
    SC_Cell * pCell;
    Abc_Obj_t * pObj;
    int i;
    Abc_NtkForEachNodeNotBarBuf( pNtk, pObj, i )
        if ( Abc_ObjFanoutNum(pObj) == 0 )
        {
            pCell = Abc_SclObjCell( pObj );
            p->SumArea -= pCell->area;
            Abc_NtkDeleteObj_rec( pObj, 1 );
//            printf( "Removed node %d.\n", i );
        }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_SclUpsizePerformInt( SC_Lib * pLib, Abc_Ntk_t * pNtk, SC_SizePars * pPars )
{
    SC_Man * p;
    Vec_Int_t * vPathPos = NULL;    // critical POs
    Vec_Int_t * vPathNodes = NULL;  // critical nodes and PIs
    abctime clk, nRuntimeLimit = pPars->TimeOut ? pPars->TimeOut * CLOCKS_PER_SEC + Abc_Clock() : 0;
    int i = 0, win, nUpsizes = -1, nFramesNoChange = 0, nConeSize = 0;
    int nAllPos, nAllNodes, nAllTfos, nAllUpsizes;
    if ( pPars->fVerbose )
    {
        printf( "Parameters: " );
        printf( "Iters =%5d.  ",          pPars->nIters   );
        printf( "Time win =%3d %%. ",     pPars->Window   );
        printf( "Update ratio =%3d %%. ", pPars->Ratio    );
        printf( "UseDept =%2d. ",         pPars->fUseDept );
        printf( "UseWL =%2d. ",           pPars->fUseWireLoads );
        printf( "Target =%5d ps. ",       pPars->DelayUser );
        printf( "DelayGap =%3d ps. ",     pPars->DelayGap );
        printf( "Timeout =%4d sec",       pPars->TimeOut  );
        printf( "\n" );
    }
    // increase window for larger networks
    if ( pPars->Window == 1 )
        pPars->Window += (Abc_NtkNodeNum(pNtk) > 40000);
    // prepare the manager; collect init stats
    p = Abc_SclManStart( pLib, pNtk, pPars->fUseWireLoads, pPars->fUseDept, 0, pPars->BuffTreeEst );
    p->timeTotal  = Abc_Clock();
    assert( p->vGatesBest == NULL );
    p->vGatesBest = Vec_IntDup( p->pNtk->vGates );
    p->BestDelay  = p->MaxDelay0;
    // perform upsizing
    nAllPos = nAllNodes = nAllTfos = nAllUpsizes = 0;
    if ( p->BestDelay <= pPars->DelayUser )
        printf( "Current delay (%.2f ps) does not exceed the target delay (%.2f ps). Upsizing is not performed.\n", p->BestDelay, (float)pPars->DelayUser );
    else
    for ( i = 0; i < pPars->nIters; i++ )
    {
        for ( win = pPars->Window + ((i % 7) == 6); win <= 100;  win *= 2 )
        {
            // detect critical path
            clk = Abc_Clock();
            vPathPos   = Abc_SclFindCriticalCoWindow( p, win );
            vPathNodes = Abc_SclFindCriticalNodeWindow( p, vPathPos, win, pPars->fUseDept );
            p->timeCone += Abc_Clock() - clk;

            // selectively upsize the nodes
            clk = Abc_Clock();
            if ( pPars->BypassFreq && i && (i % pPars->BypassFreq) == 0 )
                nUpsizes = Abc_SclFindBypasses( p, vPathNodes, pPars->Ratio, pPars->Notches, i, pPars->DelayGap, pPars->fVeryVerbose );
            else
                nUpsizes = Abc_SclFindUpsizes( p, vPathNodes, pPars->Ratio, pPars->Notches, i, pPars->DelayGap, (pPars->BypassFreq > 0) );
            p->timeSize += Abc_Clock() - clk;

            // unmark critical path
            clk = Abc_Clock();
            Abc_SclUnmarkCriticalNodeWindow( p, vPathNodes );
            Abc_SclUnmarkCriticalNodeWindow( p, vPathPos );
            p->timeCone += Abc_Clock() - clk;
            if ( nUpsizes > 0 )
                break;
            Vec_IntFree( vPathPos );
            Vec_IntFree( vPathNodes );
        }
        if ( nUpsizes == 0 )
            break;

        // update timing information
        clk = Abc_Clock();
        if ( pPars->fUseDept )
        {
            if ( Vec_IntSize(p->vChanged) && !(pPars->BypassFreq && i && (i % pPars->BypassFreq) == 0) )
                nConeSize = Abc_SclTimeIncUpdate( p );
            else
                Abc_SclTimeNtkRecompute( p, NULL, NULL, pPars->fUseDept, 0 );
        }
        else
        {
            Vec_Int_t * vTFO = Abc_SclFindTFO( p->pNtk, vPathNodes );
            Abc_SclTimeCone( p, vTFO );
            nConeSize = Vec_IntSize( vTFO );
            Vec_IntFree( vTFO );
        }
        p->timeTime += Abc_Clock() - clk;
//        Abc_SclUpsizePrintDiffs( p, pLib, pNtk );

        // save the best network
        p->MaxDelay = Abc_SclReadMaxDelay( p );
        if ( p->BestDelay > p->MaxDelay )
        {
            p->BestDelay = p->MaxDelay;
            Abc_SclApplyUpdateToBest( p->vGatesBest, p->pNtk->vGates, p->vUpdates );
            Vec_IntClear( p->vUpdates2 );
            nFramesNoChange = 0;
        }
        else
            nFramesNoChange++;

        // report and cleanup
        Abc_SclUpsizePrint( p, i, win, Vec_IntSize(vPathPos), Vec_IntSize(vPathNodes), nUpsizes, nConeSize, pPars->fVeryVerbose || (pPars->fVerbose && nFramesNoChange == 0) ); //|| (i == nIters-1) );
        nAllPos     += Vec_IntSize(vPathPos);
        nAllNodes   += Vec_IntSize(vPathNodes);
        nAllTfos    += nConeSize;
        nAllUpsizes += nUpsizes;
        Vec_IntFree( vPathPos );
        Vec_IntFree( vPathNodes );
        // check timeout
        if ( nRuntimeLimit && Abc_Clock() > nRuntimeLimit )
            break;
        // check no change
        if ( nFramesNoChange > pPars->nIterNoChange )
            break;
        // check best delay
        if ( p->BestDelay <= pPars->DelayUser )
            break;
    }
    // update for best gates and recompute timing
    ABC_SWAP( Vec_Int_t *, p->vGatesBest, p->pNtk->vGates );
    if ( pPars->BypassFreq != 0 )
        Abc_SclUndoRecentChanges( p->pNtk, p->vUpdates2 );
    if ( pPars->BypassFreq != 0 )
        Abc_SclUpsizeRemoveDangling( p, pNtk );
    Abc_SclTimeNtkRecompute( p, &p->SumArea, &p->MaxDelay, 0, 0 );
    if ( pPars->fVerbose )
        Abc_SclUpsizePrint( p, i, pPars->Window, nAllPos/(i?i:1), nAllNodes/(i?i:1), nAllUpsizes/(i?i:1), nAllTfos/(i?i:1), 1 );
    else
        printf( "                                                                                                                                                  \r" );
    // report runtime
    p->timeTotal = Abc_Clock() - p->timeTotal;
    if ( pPars->fVerbose )
    {
        p->timeOther = p->timeTotal - p->timeCone - p->timeSize - p->timeTime;
        ABC_PRTP( "Runtime: Critical path", p->timeCone,  p->timeTotal );
        ABC_PRTP( "Runtime: Sizing eval  ", p->timeSize,  p->timeTotal );
        ABC_PRTP( "Runtime: Timing update", p->timeTime,  p->timeTotal );
        ABC_PRTP( "Runtime: Other        ", p->timeOther, p->timeTotal );
        ABC_PRTP( "Runtime: TOTAL        ", p->timeTotal, p->timeTotal );
    }
    if ( pPars->fDumpStats )
        Abc_SclDumpStats( p, "stats2.txt", p->timeTotal );
    if ( nRuntimeLimit && Abc_Clock() > nRuntimeLimit )
        printf( "Gate sizing timed out at %d seconds.\n", pPars->TimeOut );

    // save the result and quit
    Abc_SclSclGates2MioGates( pLib, pNtk ); // updates gate pointers
    Abc_SclManFree( p );
//    Abc_NtkCleanMarkAB( pNtk );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_SclUpsizePerform( SC_Lib * pLib, Abc_Ntk_t * pNtk, SC_SizePars * pPars )
{
    Abc_Ntk_t * pNtkNew = pNtk;
    if ( pNtk->nBarBufs2 > 0 )
        pNtkNew = Abc_NtkDupDfsNoBarBufs( pNtk );
    Abc_SclUpsizePerformInt( pLib, pNtkNew, pPars );
    if ( pNtk->nBarBufs2 > 0 )
        Abc_SclTransferGates( pNtk, pNtkNew );
    if ( pNtk->nBarBufs2 > 0 )
        Abc_NtkDelete( pNtkNew );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

