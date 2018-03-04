/**CFile****************************************************************

  FileName    [retFlow.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Implementation of maximum flow (min-area retiming).]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - Oct 31, 2006.]

  Revision    [$Id: retFlow.c,v 1.00 2006/10/31 00:00:00 alanmi Exp $]

***********************************************************************/

#include "retInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static inline int         Abc_ObjSetPath( Abc_Obj_t * pObj, Abc_Obj_t * pNext ) { pObj->pCopy = pNext; return 1;   }
static inline Abc_Obj_t * Abc_ObjGetPath( Abc_Obj_t * pObj )                    { return pObj->pCopy;              }
static inline Abc_Obj_t * Abc_ObjGetFanoutPath( Abc_Obj_t * pObj ) 
{ 
    Abc_Obj_t * pFanout;
    int i;
    assert( Abc_ObjGetPath(pObj) ); 
    Abc_ObjForEachFanout( pObj, pFanout, i )
        if ( Abc_ObjGetPath(pFanout) == pObj )
            return pFanout;
    return NULL;
}
static inline Abc_Obj_t * Abc_ObjGetFaninPath( Abc_Obj_t * pObj ) 
{ 
    Abc_Obj_t * pFanin;
    int i;
    assert( Abc_ObjGetPath(pObj) ); 
    Abc_ObjForEachFanin( pObj, pFanin, i )
        if ( Abc_ObjGetPath(pFanin) == pObj )
            return pFanin;
    return NULL;
}
static inline Abc_Obj_t * Abc_ObjGetPredecessorBwd( Abc_Obj_t * pObj ) 
{ 
    Abc_Obj_t * pNext;
    int i;
    Abc_ObjForEachFanout( pObj, pNext, i )
        if ( Abc_ObjGetPath(pNext) == pObj )
            return pNext;
    Abc_ObjForEachFanin( pObj, pNext, i )
        if ( Abc_ObjGetPath(pNext) == pObj )
            return pNext;
    return NULL;
}
static inline Abc_Obj_t * Abc_ObjGetPredecessorFwd( Abc_Obj_t * pObj ) 
{ 
    Abc_Obj_t * pNext;
    int i;
    Abc_ObjForEachFanin( pObj, pNext, i )
        if ( Abc_ObjGetPath(pNext) == pObj )
            return pNext;
    Abc_ObjForEachFanout( pObj, pNext, i )
        if ( Abc_ObjGetPath(pNext) == pObj )
            return pNext;
    return NULL;
}

static int         Abc_NtkMaxFlowBwdPath_rec( Abc_Obj_t * pObj );
static int         Abc_NtkMaxFlowFwdPath_rec( Abc_Obj_t * pObj );
static int         Abc_NtkMaxFlowBwdPath2_rec( Abc_Obj_t * pObj );
static int         Abc_NtkMaxFlowFwdPath2_rec( Abc_Obj_t * pObj );
//static int         Abc_NtkMaxFlowBwdPath3_rec( Abc_Obj_t * pObj );
static int         Abc_NtkMaxFlowFwdPath3_rec( Abc_Obj_t * pObj, Abc_Obj_t * pPrev, int fFanin );
static Vec_Ptr_t * Abc_NtkMaxFlowMinCut( Abc_Ntk_t * pNtk, int fForward );
static void        Abc_NtkMaxFlowMinCutUpdate( Abc_Ntk_t * pNtk, Vec_Ptr_t * vMinCut, int fForward );
static int         Abc_NtkMaxFlowVerifyCut( Abc_Ntk_t * pNtk, Vec_Ptr_t * vMinCut, int fForward );
static void        Abc_NtkMaxFlowPrintCut( Abc_Ntk_t * pNtk, Vec_Ptr_t * vMinCut );
static void        Abc_NtkMaxFlowPrintFlow( Abc_Ntk_t * pNtk, int fForward );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Test-bench for the max-flow computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkMaxFlowTest( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vMinCut;
    Abc_Obj_t * pObj;
    int i;

    // forward flow
    Abc_NtkForEachPo( pNtk, pObj, i )
        pObj->fMarkA = 1;
    Abc_NtkForEachLatch( pNtk, pObj, i )
        pObj->fMarkA = Abc_ObjFanin0(pObj)->fMarkA = 1;
//        Abc_ObjFanin0(pObj)->fMarkA = 1;
    vMinCut = Abc_NtkMaxFlow( pNtk, 1, 1 );
    Vec_PtrFree( vMinCut );
    Abc_NtkCleanMarkA( pNtk );

    // backward flow
    Abc_NtkForEachPi( pNtk, pObj, i )
        pObj->fMarkA = 1;
    Abc_NtkForEachLatch( pNtk, pObj, i )
        pObj->fMarkA = Abc_ObjFanout0(pObj)->fMarkA = 1;
//        Abc_ObjFanout0(pObj)->fMarkA = 1;
    vMinCut = Abc_NtkMaxFlow( pNtk, 0, 1 );
    Vec_PtrFree( vMinCut );
    Abc_NtkCleanMarkA( pNtk );

}

/**Function*************************************************************

  Synopsis    [Implementation of max-flow/min-cut computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NtkMaxFlow( Abc_Ntk_t * pNtk, int fForward, int fVerbose )
{
    Vec_Ptr_t * vMinCut;
    Abc_Obj_t * pLatch;
    int Flow, FlowCur, RetValue, i;
    abctime clk = Abc_Clock();
    int fUseDirectedFlow = 1;

    // find the max-flow
    Abc_NtkCleanCopy( pNtk );
    Flow = 0;
    Abc_NtkIncrementTravId(pNtk);
    Abc_NtkForEachLatch( pNtk, pLatch, i )
    {
        if ( fForward )
        {
//            assert( !Abc_ObjFanout0(pLatch)->fMarkA );
            FlowCur  = Abc_NtkMaxFlowFwdPath2_rec( Abc_ObjFanout0(pLatch) );
//            FlowCur  = Abc_NtkMaxFlowFwdPath3_rec( Abc_ObjFanout0(pLatch), pLatch, 1 );
            Flow    += FlowCur;
        }
        else
        {
            assert( !Abc_ObjFanin0(pLatch)->fMarkA );
            FlowCur  = Abc_NtkMaxFlowBwdPath2_rec( Abc_ObjFanin0(pLatch) );
            Flow    += FlowCur;
        }
        if ( FlowCur )
            Abc_NtkIncrementTravId(pNtk);
    }

    if ( !fUseDirectedFlow )
    {
        Abc_NtkIncrementTravId(pNtk);
        Abc_NtkForEachLatch( pNtk, pLatch, i )
        {
            if ( fForward )
            {
    //            assert( !Abc_ObjFanout0(pLatch)->fMarkA );
                FlowCur  = Abc_NtkMaxFlowFwdPath_rec( Abc_ObjFanout0(pLatch) );
                Flow    += FlowCur;
            }
            else
            {
                assert( !Abc_ObjFanin0(pLatch)->fMarkA );
                FlowCur  = Abc_NtkMaxFlowBwdPath_rec( Abc_ObjFanin0(pLatch) );
                Flow    += FlowCur;
            }
            if ( FlowCur )
                Abc_NtkIncrementTravId(pNtk);
        }
    }
//    Abc_NtkMaxFlowPrintFlow( pNtk, fForward );

    // mark the nodes reachable from the latches
    Abc_NtkIncrementTravId(pNtk);
    Abc_NtkForEachLatch( pNtk, pLatch, i )
    {
        if ( fForward )
        {
//            assert( !Abc_ObjFanout0(pLatch)->fMarkA );
            if ( fUseDirectedFlow )
                RetValue = Abc_NtkMaxFlowFwdPath2_rec( Abc_ObjFanout0(pLatch) );
//                RetValue = Abc_NtkMaxFlowFwdPath3_rec( Abc_ObjFanout0(pLatch), pLatch, 1 );
            else
                RetValue = Abc_NtkMaxFlowFwdPath_rec( Abc_ObjFanout0(pLatch) );
        }
        else
        {
            assert( !Abc_ObjFanin0(pLatch)->fMarkA );
            if ( fUseDirectedFlow )
                RetValue = Abc_NtkMaxFlowBwdPath2_rec( Abc_ObjFanin0(pLatch) );
            else
                RetValue = Abc_NtkMaxFlowBwdPath_rec( Abc_ObjFanin0(pLatch) );
        }
        assert( RetValue == 0 );
    }

    // find the min-cut with the smallest volume
    vMinCut = Abc_NtkMaxFlowMinCut( pNtk, fForward );
    // verify the cut
    if ( !Abc_NtkMaxFlowVerifyCut(pNtk, vMinCut, fForward) )
        printf( "Abc_NtkMaxFlow() error! The computed min-cut is not a cut!\n" );
    // make the cut retimable
    Abc_NtkMaxFlowMinCutUpdate( pNtk, vMinCut, fForward );

    // report the results
    if ( fVerbose )
    {
    printf( "L = %6d. %s max-flow = %6d.  Min-cut = %6d.  ", 
        Abc_NtkLatchNum(pNtk), fForward? "Forward " : "Backward", Flow, Vec_PtrSize(vMinCut) );
ABC_PRT( "Time", Abc_Clock() - clk );
    }

//    Abc_NtkMaxFlowPrintCut( pNtk, vMinCut );
    return vMinCut;
}

/**Function*************************************************************

  Synopsis    [Tries to find an augmenting path originating in this node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkMaxFlowBwdPath_rec( Abc_Obj_t * pObj )
{
    Abc_Obj_t * pNext, * pPred;
    int i;
    // skip visited nodes
    if ( Abc_NodeIsTravIdCurrent(pObj) )
        return 0;
    Abc_NodeSetTravIdCurrent(pObj);
    // get the predecessor
    pPred = Abc_ObjGetPredecessorBwd( pObj );
    // process node without flow
    if ( !Abc_ObjGetPath(pObj) )
    {
        // start the path if we reached a terminal node
        if ( pObj->fMarkA )
            return Abc_ObjSetPath( pObj, (Abc_Obj_t *)1 );
        // explore the fanins
        Abc_ObjForEachFanin( pObj, pNext, i )
            if ( pNext != pPred && !Abc_ObjIsLatch(pNext) && Abc_NtkMaxFlowBwdPath_rec(pNext) )
                return Abc_ObjSetPath( pObj, pNext );
        Abc_ObjForEachFanout( pObj, pNext, i )
            if ( pNext != pPred && !Abc_ObjIsLatch(pNext) && Abc_NtkMaxFlowBwdPath_rec(pNext) )
                return Abc_ObjSetPath( pObj, pNext );
        return 0;
    }
    // pObj has flow - find the fanout with flow
    if ( pPred == NULL )
        return 0;
    // go through the successors of the predecessor
    Abc_ObjForEachFanin( pPred, pNext, i )
        if ( !Abc_ObjIsLatch(pNext) && Abc_NtkMaxFlowBwdPath_rec( pNext ) )
            return Abc_ObjSetPath( pPred, pNext );
    Abc_ObjForEachFanout( pPred, pNext, i )
        if ( !Abc_ObjIsLatch(pNext) && Abc_NtkMaxFlowBwdPath_rec( pNext ) )
            return Abc_ObjSetPath( pPred, pNext );
    // try the fanout
    if ( Abc_NtkMaxFlowBwdPath_rec( pPred ) )
        return Abc_ObjSetPath( pPred, NULL );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Tries to find an augmenting path originating in this node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkMaxFlowFwdPath_rec( Abc_Obj_t * pObj )
{
    Abc_Obj_t * pNext, * pPred;
    int i;
    // skip visited nodes
    if ( Abc_NodeIsTravIdCurrent(pObj) )
        return 0;
    Abc_NodeSetTravIdCurrent(pObj);
    // get the predecessor
    pPred = Abc_ObjGetPredecessorFwd( pObj );
    // process node without flow
    if ( !Abc_ObjGetPath(pObj) )
    {
        // start the path if we reached a terminal node
        if ( pObj->fMarkA )
            return Abc_ObjSetPath( pObj, (Abc_Obj_t *)1 );
        // explore the fanins
        Abc_ObjForEachFanout( pObj, pNext, i )
            if ( pNext != pPred && !Abc_ObjIsLatch(pNext) && Abc_NtkMaxFlowFwdPath_rec(pNext) )
                return Abc_ObjSetPath( pObj, pNext );
        Abc_ObjForEachFanin( pObj, pNext, i )
            if ( pNext != pPred && !Abc_ObjIsLatch(pNext) && Abc_NtkMaxFlowFwdPath_rec(pNext) )
                return Abc_ObjSetPath( pObj, pNext );
        return 0;
    }
    // pObj has flow - find the fanout with flow
    if ( pPred == NULL )
        return 0;
    // go through the successors of the predecessor
    Abc_ObjForEachFanout( pPred, pNext, i )
        if ( !Abc_ObjIsLatch(pNext) && Abc_NtkMaxFlowFwdPath_rec( pNext ) )
            return Abc_ObjSetPath( pPred, pNext );
    Abc_ObjForEachFanin( pPred, pNext, i )
        if ( !Abc_ObjIsLatch(pNext) && Abc_NtkMaxFlowFwdPath_rec( pNext ) )
            return Abc_ObjSetPath( pPred, pNext );
    // try the fanout
    if ( Abc_NtkMaxFlowFwdPath_rec( pPred ) )
        return Abc_ObjSetPath( pPred, NULL );
    return 0;
}


/**Function*************************************************************

  Synopsis    [Tries to find an augmenting path originating in this edge.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkMaxFlowFwdPath3_rec( Abc_Obj_t * pObj, Abc_Obj_t * pPrev, int fFanin )
{
    Abc_Obj_t * pFanin, * pFanout;
    int i;
    // skip visited nodes
    if ( Abc_NodeIsTravIdCurrent(pObj) )
        return 0;
    Abc_NodeSetTravIdCurrent(pObj);
    // skip the fanin which already has flow
    if ( fFanin && Abc_ObjGetPath(pPrev) )
        return 0;
    // if the node has no flow, try to push through the fanouts
    if ( !Abc_ObjGetPath(pObj) )
    {
        // start the path if we reached a terminal node
        if ( pObj->fMarkA )
            return Abc_ObjSetPath( pObj, (Abc_Obj_t *)1 );
        // try to push flow through the fanouts
        Abc_ObjForEachFanout( pObj, pFanout, i )
            if ( Abc_NtkMaxFlowFwdPath3_rec(pFanout, pObj, 1) )
                return fFanin? Abc_ObjSetPath(pPrev, pObj) : 1;
    }
    // try to push through the fanins
    Abc_ObjForEachFanin( pObj, pFanin, i )
        if ( !Abc_ObjIsLatch(pFanin) && Abc_NtkMaxFlowFwdPath3_rec(pFanin, pObj, 0) )
            return Abc_ObjSetPath( pFanin, NULL );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Tries to find an augmenting path originating in this node.]

  Description [This procedure works for directed graphs only!]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkMaxFlowBwdPath2_rec( Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFanout, * pFanin;
    int i;
    // skip visited nodes
    if ( Abc_NodeIsTravIdCurrent(pObj) )
        return 0;
    Abc_NodeSetTravIdCurrent(pObj);
    // process node without flow
    if ( !Abc_ObjGetPath(pObj) )
    {
        // start the path if we reached a terminal node
        if ( pObj->fMarkA )
            return Abc_ObjSetPath( pObj, (Abc_Obj_t *)1 );
        // explore the fanins
        Abc_ObjForEachFanin( pObj, pFanin, i )
            if ( Abc_NtkMaxFlowBwdPath2_rec(pFanin) )
                return Abc_ObjSetPath( pObj, pFanin );
        return 0;
    }
    // pObj has flow - find the fanout with flow
    pFanout = Abc_ObjGetFanoutPath( pObj );
    if ( pFanout == NULL )
        return 0;
    // go through the fanins of the fanout with flow
    Abc_ObjForEachFanin( pFanout, pFanin, i )
        if ( Abc_NtkMaxFlowBwdPath2_rec( pFanin ) )
            return Abc_ObjSetPath( pFanout, pFanin );
    // try the fanout
    if ( Abc_NtkMaxFlowBwdPath2_rec( pFanout ) )
        return Abc_ObjSetPath( pFanout, NULL );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Tries to find an augmenting path originating in this node.]

  Description [This procedure works for directed graphs only!]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkMaxFlowFwdPath2_rec( Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFanout, * pFanin;
    int i;
    // skip visited nodes
    if ( Abc_NodeIsTravIdCurrent(pObj) )
        return 0;
    Abc_NodeSetTravIdCurrent(pObj);
    // process node without flow
    if ( !Abc_ObjGetPath(pObj) )
    { 
        // start the path if we reached a terminal node
        if ( pObj->fMarkA )
            return Abc_ObjSetPath( pObj, (Abc_Obj_t *)1 );
        // explore the fanins
        Abc_ObjForEachFanout( pObj, pFanout, i )
            if ( Abc_NtkMaxFlowFwdPath2_rec(pFanout) )
                return Abc_ObjSetPath( pObj, pFanout );
        return 0;
    }
    // pObj has flow - find the fanout with flow
    pFanin = Abc_ObjGetFaninPath( pObj );
    if ( pFanin == NULL )
        return 0;
    // go through the fanins of the fanout with flow
    Abc_ObjForEachFanout( pFanin, pFanout, i )
        if ( Abc_NtkMaxFlowFwdPath2_rec( pFanout ) )
            return Abc_ObjSetPath( pFanin, pFanout );
    // try the fanout
    if ( Abc_NtkMaxFlowFwdPath2_rec( pFanin ) )
        return Abc_ObjSetPath( pFanin, NULL );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Find minimum-volume minumum cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NtkMaxFlowMinCut( Abc_Ntk_t * pNtk, int fForward )
{
    Vec_Ptr_t * vMinCut;
    Abc_Obj_t * pObj;
    int i;
    // collect the cut nodes
    vMinCut = Vec_PtrAlloc( Abc_NtkLatchNum(pNtk) );
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        // node without flow is not a cut node
        if ( !Abc_ObjGetPath(pObj) )
            continue;
        // unvisited node is below the cut
        if ( !Abc_NodeIsTravIdCurrent(pObj) )
            continue;
        // add terminal with flow or node whose path is not visited
        if ( pObj->fMarkA || !Abc_NodeIsTravIdCurrent( Abc_ObjGetPath(pObj) ) )
            Vec_PtrPush( vMinCut, pObj );
    }
    return vMinCut;
}

/**Function*************************************************************

  Synopsis    [Marks the TFI cone with MarkA.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkMaxFlowMarkCut_rec( Abc_Obj_t * pObj )
{
    Abc_Obj_t * pNext;
    int i;
    if ( pObj->fMarkA )
        return;
    pObj->fMarkA = 1;
    Abc_ObjForEachFanin( pObj, pNext, i )
        Abc_NtkMaxFlowMarkCut_rec( pNext );
}

/**Function*************************************************************

  Synopsis    [Visits the TFI up to marked nodes and collects marked nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkMaxFlowCollectCut_rec( Abc_Obj_t * pObj, Vec_Ptr_t * vNodes )
{
    Abc_Obj_t * pNext;
    int i;
    if ( Abc_NodeIsTravIdCurrent(pObj) )
        return;
    Abc_NodeSetTravIdCurrent(pObj);
    if ( pObj->fMarkA )
    {
        Vec_PtrPush( vNodes, pObj );
        return;
    }
    Abc_ObjForEachFanin( pObj, pNext, i )
        Abc_NtkMaxFlowCollectCut_rec( pNext, vNodes );
}

/**Function*************************************************************

  Synopsis    [Updates the minimum cut to be retimable.]

  Description [This procedure also labels the nodes reachable from
  the latches to the cut with fMarkA.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkMaxFlowMinCutUpdate( Abc_Ntk_t * pNtk, Vec_Ptr_t * vMinCut, int fForward )
{
    Abc_Obj_t * pObj, * pNext;
    int i, k;
    // clean marks
    Abc_NtkForEachObj( pNtk, pObj, i )
        pObj->fMarkA = 0;
    // set latch outputs
    Abc_NtkForEachLatch( pNtk, pObj, i )
        Abc_ObjFanout0(pObj)->fMarkA = 1;
    // traverse from cut nodes
    Vec_PtrForEachEntry( Abc_Obj_t *, vMinCut, pObj, i )
        Abc_NtkMaxFlowMarkCut_rec( pObj );
    if ( fForward )
    {
        // change mincut to be nodes with unmarked fanouts
        Vec_PtrClear( vMinCut );
        Abc_NtkForEachObj( pNtk, pObj, i )
        {
            if ( !pObj->fMarkA )
                continue;
            Abc_ObjForEachFanout( pObj, pNext, k )
            {
                if ( pNext->fMarkA )
                    continue;
                Vec_PtrPush( vMinCut, pObj );
                break;
            }
        }
    }
    else
    {
        // change mincut to be marked fanins of the unmarked nodes
        Vec_PtrClear( vMinCut );
        Abc_NtkIncrementTravId(pNtk);
        Abc_NtkForEachLatch( pNtk, pObj, i )
            Abc_NtkMaxFlowCollectCut_rec( Abc_ObjFanin0(pObj), vMinCut );
        // transfer the attribute
        Abc_NtkForEachObj( pNtk, pObj, i )
            pObj->fMarkA = Abc_NodeIsTravIdCurrent(pObj);
        // unmark the cut nodes
        Vec_PtrForEachEntry( Abc_Obj_t *, vMinCut, pObj, i )
            pObj->fMarkA = 0;
    }
}

/**Function*************************************************************

  Synopsis    [Verifies the min-cut is indeed a cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkMaxFlowVerifyCut_rec( Abc_Obj_t * pObj, int fForward )
{
    Abc_Obj_t * pNext;
    int i;
    // skip visited nodes
    if ( Abc_NodeIsTravIdCurrent(pObj) )
        return 1;
    Abc_NodeSetTravIdCurrent(pObj);
    // visit the node
    if ( fForward )
    {
        if ( Abc_ObjIsCo(pObj) )
            return 0;
        // explore the fanouts
        Abc_ObjForEachFanout( pObj, pNext, i )
            if ( !Abc_NtkMaxFlowVerifyCut_rec(pNext, fForward) )
                return 0;
    }
    else
    {
        if ( Abc_ObjIsCi(pObj) )
            return 0;
        // explore the fanins
        Abc_ObjForEachFanin( pObj, pNext, i )
            if ( !Abc_NtkMaxFlowVerifyCut_rec(pNext, fForward) )
                return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Verifies the min-cut is indeed a cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkMaxFlowVerifyCut( Abc_Ntk_t * pNtk, Vec_Ptr_t * vMinCut, int fForward )
{
    Abc_Obj_t * pObj;
    int i;
    // mark the cut with the current traversal ID
    Abc_NtkIncrementTravId(pNtk);
    Vec_PtrForEachEntry( Abc_Obj_t *, vMinCut, pObj, i )
        Abc_NodeSetTravIdCurrent( pObj );
    // search from the latches for a path to the COs/CIs
    Abc_NtkForEachLatch( pNtk, pObj, i )
    {
        if ( fForward )
        {
            if ( !Abc_NtkMaxFlowVerifyCut_rec( Abc_ObjFanout0(pObj), fForward ) )
                return 0;
        }
        else
        {
            if ( !Abc_NtkMaxFlowVerifyCut_rec( Abc_ObjFanin0(pObj), fForward ) )
                return 0;
        }
    }
/*
    {
        // count the volume of the cut
        int Counter = 0;
        Abc_NtkForEachObj( pNtk, pObj, i )
            Counter += Abc_NodeIsTravIdCurrent( pObj );
        printf( "Volume = %d.\n", Counter );
    }
*/
    return 1;
}

/**Function*************************************************************

  Synopsis    [Prints the flows.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkMaxFlowPrintFlow( Abc_Ntk_t * pNtk, int fForward )
{
    Abc_Obj_t * pLatch, * pNext;
    Abc_Obj_t * pPrev = NULL; // Suppress "might be used uninitialized"
    int i;
    if ( fForward )
    {
        Vec_PtrForEachEntry( Abc_Obj_t *, pNtk->vBoxes, pLatch, i )
        {
            assert( !Abc_ObjFanout0(pLatch)->fMarkA );
            if ( Abc_ObjGetPath(Abc_ObjFanout0(pLatch)) == NULL ) // no flow through this latch
                continue;
            printf( "Path = " );
            for ( pNext = Abc_ObjFanout0(pLatch); pNext != (void *)1; pNext = Abc_ObjGetPath(pNext) )
            {
                printf( "%s(%d) ", Abc_ObjName(pNext), pNext->Id );
                pPrev = pNext;
            }
            if ( !Abc_ObjIsPo(pPrev) )
            printf( "%s(%d) ", Abc_ObjName(Abc_ObjFanout0(pPrev)), Abc_ObjFanout0(pPrev)->Id );
            printf( "\n" );
        }
    }
    else
    {
        Vec_PtrForEachEntry( Abc_Obj_t *, pNtk->vBoxes, pLatch, i )
        {
            assert( !Abc_ObjFanin0(pLatch)->fMarkA );
            if ( Abc_ObjGetPath(Abc_ObjFanin0(pLatch)) == NULL ) // no flow through this latch
                continue;
            printf( "Path = " );
            for ( pNext = Abc_ObjFanin0(pLatch); pNext != (void *)1; pNext = Abc_ObjGetPath(pNext) )
            {
                printf( "%s(%d) ", Abc_ObjName(pNext), pNext->Id );
                pPrev = pNext;
            }
            if ( !Abc_ObjIsPi(pPrev) )
            printf( "%s(%d) ", Abc_ObjName(Abc_ObjFanin0(pPrev)), Abc_ObjFanin0(pPrev)->Id );
            printf( "\n" );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Prints the min-cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkMaxFlowPrintCut( Abc_Ntk_t * pNtk, Vec_Ptr_t * vMinCut )
{
    Abc_Obj_t * pObj;
    int i;
    printf( "Min-cut: " );
    Vec_PtrForEachEntry( Abc_Obj_t *, vMinCut, pObj, i )
        printf( "%s(%d) ", Abc_ObjName(pObj), pObj->Id );
    printf( "\n" );
    printf( "Marked nodes: " );
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( pObj->fMarkA )
            printf( "%s(%d) ", Abc_ObjName(pObj), pObj->Id );
    printf( "\n" );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

