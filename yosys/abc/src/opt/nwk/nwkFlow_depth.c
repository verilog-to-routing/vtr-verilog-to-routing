/**CFile****************************************************************

  FileName    [nwkFlow.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Netlist representation.]

  Synopsis    [Max-flow/min-cut computation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: nwkFlow.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "nwk.h"

ABC_NAMESPACE_IMPL_START


/*
    This code is based on the papers:
    A. Hurst, A. Mishchenko, and R. Brayton, "Fast minimum-register retiming 
    via binary maximum-flow", Proc. FMCAD '07, pp. 181-187. 
    A. Hurst, A. Mishchenko, and R. Brayton, "Scalable min-area retiming 
    under simultaneous delay and initial state constraints". Proc. DAC'08. 
*/

int DepthFwd, DepthBwd, DepthFwdMax, DepthBwdMax;

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// predecessors
static inline Nwk_Obj_t * Nwk_ObjPred( Nwk_Obj_t * pObj )                   { return pObj->pCopy;        }
static inline int         Nwk_ObjSetPred( Nwk_Obj_t * pObj, Nwk_Obj_t * p ) { pObj->pCopy = p; return 1; }
// sink
static inline int         Nwk_ObjIsSink( Nwk_Obj_t * pObj )                 { return pObj->MarkA;        }
static inline void        Nwk_ObjSetSink( Nwk_Obj_t * pObj )                { pObj->MarkA = 1;           }
// flow
static inline int         Nwk_ObjHasFlow( Nwk_Obj_t * pObj )                { return pObj->MarkB;        }
static inline void        Nwk_ObjSetFlow( Nwk_Obj_t * pObj )                { pObj->MarkB = 1;           }
static inline void        Nwk_ObjClearFlow( Nwk_Obj_t * pObj )              { pObj->MarkB = 0;           }
 
// representation of visited nodes
// pObj->TravId  < pNtk->nTravIds-2   --- not visited
// pObj->TravId == pNtk->nTravIds-2   --- visited bot only
// pObj->TravId == pNtk->nTravIds-1   --- visited top only
// pObj->TravId == pNtk->nTravIds     --- visited bot and top
static inline int  Nwk_ObjVisitedBotOnly( Nwk_Obj_t * pObj )   
{
    return pObj->TravId == pObj->pMan->nTravIds - 2;
}
static inline int  Nwk_ObjVisitedBot( Nwk_Obj_t * pObj )     
{ 
    return pObj->TravId == pObj->pMan->nTravIds - 2 || pObj->TravId == pObj->pMan->nTravIds; 
}
static inline int  Nwk_ObjVisitedTop( Nwk_Obj_t * pObj )     
{ 
    return pObj->TravId == pObj->pMan->nTravIds - 1 || pObj->TravId == pObj->pMan->nTravIds; 
}
static inline void Nwk_ObjSetVisitedBot( Nwk_Obj_t * pObj )  
{
    if ( pObj->TravId < pObj->pMan->nTravIds - 2 )
        pObj->TravId = pObj->pMan->nTravIds - 2;
    else if ( pObj->TravId == pObj->pMan->nTravIds - 1 )
        pObj->TravId = pObj->pMan->nTravIds;
    else
        assert( 0 );
}
static inline void Nwk_ObjSetVisitedTop( Nwk_Obj_t * pObj )  
{
    if ( pObj->TravId < pObj->pMan->nTravIds - 2 )
        pObj->TravId = pObj->pMan->nTravIds - 1;
    else if ( pObj->TravId == pObj->pMan->nTravIds - 2 )
        pObj->TravId = pObj->pMan->nTravIds;
    else
        assert( 0 );
}
static inline Nwk_ManIncrementTravIdFlow( Nwk_Man_t * pMan )
{
    Nwk_ManIncrementTravId( pMan );
    Nwk_ManIncrementTravId( pMan );
    Nwk_ManIncrementTravId( pMan );
}

static int Nwk_ManPushForwardTop_rec( Nwk_Obj_t * pObj, Nwk_Obj_t * pPred );
static int Nwk_ManPushForwardBot_rec( Nwk_Obj_t * pObj, Nwk_Obj_t * pPred );

static int Nwk_ManPushBackwardTop_rec( Nwk_Obj_t * pObj, Nwk_Obj_t * pPred );
static int Nwk_ManPushBackwardBot_rec( Nwk_Obj_t * pObj, Nwk_Obj_t * pPred );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Marks TFI of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManMarkTfiCone_rec( Nwk_Obj_t * pObj )
{
    Nwk_Obj_t * pNext;
    int i;
    if ( pObj->MarkA )
        return;
    pObj->MarkA = 1;
    Nwk_ObjForEachFanin( pObj, pNext, i )
        Nwk_ManMarkTfiCone_rec( pNext );
}

/**Function*************************************************************

  Synopsis    [Marks TFO of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManMarkTfoCone_rec( Nwk_Obj_t * pObj )
{
    Nwk_Obj_t * pNext;
    int i;
    if ( pObj->MarkA )
        return;
    pObj->MarkA = 1;
    Nwk_ObjForEachFanout( pObj, pNext, i )
        Nwk_ManMarkTfoCone_rec( pNext );
}

/**Function*************************************************************

  Synopsis    [Fast forward flow pushing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nwk_ManPushForwardFast_rec( Nwk_Obj_t * pObj, Nwk_Obj_t * pPred )
{
    Nwk_Obj_t * pNext;
    int i;
    if ( Nwk_ObjIsTravIdCurrent( pObj ) )
        return 0;
    Nwk_ObjSetTravIdCurrent( pObj );
    if ( Nwk_ObjHasFlow(pObj) )
        return 0;
    if ( Nwk_ObjIsSink(pObj) )
    {
        Nwk_ObjSetFlow(pObj);
        return Nwk_ObjSetPred( pObj, pPred );
    }
    Nwk_ObjForEachFanout( pObj, pNext, i )
        if ( Nwk_ManPushForwardFast_rec( pNext, pObj ) )
        {
            Nwk_ObjSetFlow(pObj);
            return Nwk_ObjSetPred( pObj, pPred );
        }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Fast backward flow pushing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nwk_ManPushBackwardFast_rec( Nwk_Obj_t * pObj, Nwk_Obj_t * pPred )
{
    Nwk_Obj_t * pNext;
    int i;
    if ( Nwk_ObjIsTravIdCurrent( pObj ) )
        return 0;
    Nwk_ObjSetTravIdCurrent( pObj );
    if ( Nwk_ObjHasFlow(pObj) )
        return 0;
    if ( Nwk_ObjIsSink(pObj) )
    {
        Nwk_ObjSetFlow(pObj);
        return Nwk_ObjSetPred( pObj, pPred );
    }
    Nwk_ObjForEachFanin( pObj, pNext, i )
        if ( Nwk_ManPushBackwardFast_rec( pNext, pObj ) )
        {
            Nwk_ObjSetFlow(pObj);
            return Nwk_ObjSetPred( pObj, pPred );
        }
    return 0;
}
  
/**Function*************************************************************

  Synopsis    [Pushing the flow through the bottom part of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nwk_ManPushForwardBot_rec( Nwk_Obj_t * pObj, Nwk_Obj_t * pPred )
{
    Nwk_Obj_t * pNext;
    int i;
    if ( Nwk_ObjVisitedBot(pObj) )
        return 0;
    Nwk_ObjSetVisitedBot(pObj);
    DepthFwd++;
    if ( DepthFwdMax < DepthFwd )
        DepthFwdMax = DepthFwd;
    // propagate through the internal edge
    if ( Nwk_ObjHasFlow(pObj) )
    {
        if ( Nwk_ObjPred(pObj) )
            if ( Nwk_ManPushForwardTop_rec( Nwk_ObjPred(pObj), Nwk_ObjPred(pObj) ) )
            {
                DepthFwd--;
                return Nwk_ObjSetPred( pObj, pPred ); 
            }
    }
    else if ( Nwk_ManPushForwardTop_rec(pObj, pObj) )
    {
        DepthFwd--;
        Nwk_ObjSetFlow( pObj );
        return Nwk_ObjSetPred( pObj, pPred );
    }
    // try to push through the fanins
    Nwk_ObjForEachFanin( pObj, pNext, i )
        if ( Nwk_ManPushForwardBot_rec( pNext, pPred ) )
        {
            DepthFwd--;
            return 1;
        }
    DepthFwd--;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Pushing the flow through the top part of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nwk_ManPushForwardTop_rec( Nwk_Obj_t * pObj, Nwk_Obj_t * pPred )
{
    Nwk_Obj_t * pNext;
    int i;
    if ( Nwk_ObjVisitedTop(pObj) )
        return 0;
    Nwk_ObjSetVisitedTop(pObj);
    // check if this is the sink
    if ( Nwk_ObjIsSink(pObj) )
        return 1;
    DepthFwd++;
    if ( DepthFwdMax < DepthFwd )
        DepthFwdMax = DepthFwd;
    // try to push through the fanouts
    Nwk_ObjForEachFanout( pObj, pNext, i )
        if ( Nwk_ManPushForwardBot_rec( pNext, pPred ) )
        {
            DepthFwd--;
            return 1;
        }
    // redirect the flow
    if ( Nwk_ObjHasFlow(pObj) && !Nwk_ObjIsCi(pObj) )
        if ( Nwk_ManPushForwardBot_rec( pObj, Nwk_ObjPred(pObj) ) )
        {
            DepthFwd--;
            Nwk_ObjClearFlow( pObj );
            return Nwk_ObjSetPred( pObj, NULL );
        }
    DepthFwd--;
    return 0;
}
  
/**Function*************************************************************

  Synopsis    [Pushing the flow through the bottom part of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nwk_ManPushBackwardBot_rec( Nwk_Obj_t * pObj, Nwk_Obj_t * pPred )
{
    if ( Nwk_ObjVisitedBot(pObj) )
        return 0;
    Nwk_ObjSetVisitedBot(pObj);
    // propagate through the internal edge
    if ( Nwk_ObjHasFlow(pObj) )
    {
        if ( Nwk_ObjPred(pObj) )
            if ( Nwk_ManPushBackwardTop_rec( Nwk_ObjPred(pObj), Nwk_ObjPred(pObj) ) )
                return Nwk_ObjSetPred( pObj, pPred ); 
    }
    else if ( Nwk_ManPushBackwardTop_rec(pObj, pObj) )
    {
        Nwk_ObjSetFlow( pObj );
        return Nwk_ObjSetPred( pObj, pPred );
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Pushing the flow through the top part of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nwk_ManPushBackwardTop_rec( Nwk_Obj_t * pObj, Nwk_Obj_t * pPred )
{
    Nwk_Obj_t * pNext;
    int i;
    if ( Nwk_ObjVisitedTop(pObj) )
        return 0;
    Nwk_ObjSetVisitedTop(pObj);
    // check if this is the sink
    if ( Nwk_ObjIsSink(pObj) )
        return 1;
    // try to push through the fanins
    Nwk_ObjForEachFanin( pObj, pNext, i )
        if ( Nwk_ManPushBackwardBot_rec( pNext, pPred ) )
            return 1;
    // try to push through the fanouts
    Nwk_ObjForEachFanout( pObj, pNext, i )
        if ( !Nwk_ObjIsCo(pObj) && Nwk_ManPushBackwardTop_rec( pNext, pPred ) )
            return 1;
    // redirect the flow
    if ( Nwk_ObjHasFlow(pObj) )
        if ( Nwk_ObjPred(pObj) && Nwk_ManPushBackwardBot_rec( pObj, Nwk_ObjPred(pObj) ) )
        {
            Nwk_ObjClearFlow( pObj );
            return Nwk_ObjSetPred( pObj, NULL );
        }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns 0 if there is an unmarked path to a CI.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nwk_ManVerifyCut_rec( Nwk_Obj_t * pObj )
{
    Nwk_Obj_t * pNext;
    int i;
    if ( pObj->MarkA )
        return 1;
    if ( Nwk_ObjIsLo(pObj) )
        return 0;
    if ( Nwk_ObjIsTravIdCurrent( pObj ) )
        return 1;
    Nwk_ObjSetTravIdCurrent( pObj );
    Nwk_ObjForEachFanin( pObj, pNext, i )
        if ( !Nwk_ManVerifyCut_rec( pNext ) )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Verifies the forward cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nwk_ManRetimeVerifyCutForward( Nwk_Man_t * pMan, Vec_Ptr_t * vNodes )
{
    Nwk_Obj_t * pObj;
    int i;
    // mark the nodes
    Vec_PtrForEachEntry( Nwk_Obj_t *, vNodes, pObj, i )
    {
        assert( pObj->MarkA == 0 );
        pObj->MarkA = 1;
    }
    // traverse from the COs
    Nwk_ManIncrementTravId( pMan );
    Nwk_ManForEachCo( pMan, pObj, i )
        if ( !Nwk_ManVerifyCut_rec( pObj ) )
            printf( "Nwk_ManRetimeVerifyCutForward(): Internal cut verification failed.\n" );
    // unmark the nodes
    Vec_PtrForEachEntry( Nwk_Obj_t *, vNodes, pObj, i )
        pObj->MarkA = 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Verifies the forward cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nwk_ManRetimeVerifyCutBackward( Nwk_Man_t * pMan, Vec_Ptr_t * vNodes )
{
    return 1;
}

/**Function*************************************************************

  Synopsis    [Computes minimum cut for forward retiming.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Nwk_ManRetimeCutForward( Nwk_Man_t * pMan, int nLatches, int fVerbose )
{
    Vec_Ptr_t * vNodes;
    Nwk_Obj_t * pObj;
    int i, RetValue, Counter = 0, Counter2 = 0;
    clock_t clk = clock();
    // set the sequential parameters
    pMan->nLatches = nLatches;
    pMan->nTruePis = Nwk_ManCiNum(pMan) - nLatches;
    pMan->nTruePos = Nwk_ManCoNum(pMan) - nLatches;
    // mark the COs and the TFO of PIs
    Nwk_ManForEachCo( pMan, pObj, i )
        pObj->MarkA = 1;
    Nwk_ManForEachPiSeq( pMan, pObj, i )
        Nwk_ManMarkTfoCone_rec( pObj );
    // start flow computation from each LO
    Nwk_ManIncrementTravIdFlow( pMan );
    Nwk_ManForEachLoSeq( pMan, pObj, i )
    {
        if ( !Nwk_ManPushForwardFast_rec( pObj, NULL ) )
            continue;
        Nwk_ManIncrementTravIdFlow( pMan );
        Counter++;
    }
    if ( fVerbose )
    printf( "Forward:  Max-flow = %4d -> ", Counter );
    // continue flow computation from each LO
    DepthFwdMax = DepthFwd = 0;
    Nwk_ManIncrementTravIdFlow( pMan );
    Nwk_ManForEachLoSeq( pMan, pObj, i )
    {
    printf( "%d ", DepthFwdMax );
        if ( !Nwk_ManPushForwardBot_rec( pObj, NULL ) )
            continue;
        assert( DepthFwd == 0 );
        Nwk_ManIncrementTravIdFlow( pMan );
        Counter2++;
    }
    printf( "DepthMax = %d.\n", DepthFwdMax );
    if ( fVerbose )
    printf( "%4d.  ", Counter+Counter2 );
    // repeat flow computation from each LO
    if ( Counter2 > 0 )
    {
        Nwk_ManIncrementTravIdFlow( pMan );
        Nwk_ManForEachLoSeq( pMan, pObj, i )
        {
            RetValue = Nwk_ManPushForwardBot_rec( pObj, NULL );
            assert( !RetValue );
        }
    }
    // cut is a set of nodes whose bottom is visited but top is not visited
    vNodes = Vec_PtrAlloc( Counter+Counter2 );
    Counter = 0;
    Nwk_ManForEachObj( pMan, pObj, i )
    {
        if ( Nwk_ObjVisitedBotOnly(pObj) )
        {
            assert( Nwk_ObjHasFlow(pObj) );
            assert( !Nwk_ObjIsCo(pObj) );
            Vec_PtrPush( vNodes, pObj );
            Counter += Nwk_ObjIsCi(pObj);
        }
    }
    Nwk_ManCleanMarks( pMan );
    assert( Nwk_ManRetimeVerifyCutForward(pMan, vNodes) );
    if ( fVerbose )
    {
    printf( "Min-cut = %4d.  Unmoved = %4d. ", Vec_PtrSize(vNodes), Counter );
    PRT( "Time", clock() - clk );
    }
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Computes minimum cut for backward retiming.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Nwk_ManRetimeCutBackward( Nwk_Man_t * pMan, int nLatches, int fVerbose )
{
    Vec_Ptr_t * vNodes;
    Nwk_Obj_t * pObj;
    int i, RetValue, Counter = 0, Counter2 = 0;
    clock_t clk = clock();
    // set the sequential parameters
    pMan->nLatches = nLatches;
    pMan->nTruePis = Nwk_ManCiNum(pMan) - nLatches;
    pMan->nTruePos = Nwk_ManCoNum(pMan) - nLatches;
    // mark the CIs, the TFI of POs, and the constant nodes
    Nwk_ManForEachCi( pMan, pObj, i )
        pObj->MarkA = 1;
    Nwk_ManForEachPoSeq( pMan, pObj, i )
        Nwk_ManMarkTfiCone_rec( pObj );
    Nwk_ManForEachNode( pMan, pObj, i )
        if ( Nwk_ObjFaninNum(pObj) == 0 )
            pObj->MarkA = 1;
    // start flow computation from each LI driver
    Nwk_ManIncrementTravIdFlow( pMan );
    Nwk_ManForEachLiSeq( pMan, pObj, i )
    {
        if ( !Nwk_ManPushBackwardFast_rec( Nwk_ObjFanin0(pObj), NULL ) )
            continue;
        Nwk_ManIncrementTravIdFlow( pMan );
        Counter++; 
    }
    if ( fVerbose ) 
    printf( "Backward: Max-flow = %4d -> ", Counter );
    // continue flow computation from each LI driver
    Nwk_ManIncrementTravIdFlow( pMan );
    Nwk_ManForEachLiSeq( pMan, pObj, i )
    {
        if ( !Nwk_ManPushBackwardBot_rec( Nwk_ObjFanin0(pObj), NULL ) )
            continue;
        Nwk_ManIncrementTravIdFlow( pMan );
        Counter2++;
    }
    if ( fVerbose )
    printf( "%4d.  ", Counter+Counter2 );
    // repeat flow computation from each LI driver
    if ( Counter2 > 0 )
    {
        Nwk_ManIncrementTravIdFlow( pMan );
        Nwk_ManForEachLiSeq( pMan, pObj, i )
        {
            RetValue = Nwk_ManPushBackwardBot_rec( Nwk_ObjFanin0(pObj), NULL );
            assert( !RetValue );
        }
    }
    // cut is a set of nodes whose bottom is visited but top is not visited
    vNodes = Vec_PtrAlloc( Counter+Counter2 );
    Nwk_ManForEachObj( pMan, pObj, i )
    {
        if ( Nwk_ObjVisitedBotOnly(pObj) )
        {
            assert( Nwk_ObjHasFlow(pObj) );
            assert( !Nwk_ObjIsCo(pObj) );
            Vec_PtrPush( vNodes, pObj );
        }
    }
    // count CO drivers
    Counter = 0;
    Nwk_ManForEachLiSeq( pMan, pObj, i )
        if ( Nwk_ObjVisitedBotOnly( Nwk_ObjFanin0(pObj) ) )
            Counter++;
    Nwk_ManCleanMarks( pMan );
    assert( Nwk_ManRetimeVerifyCutBackward(pMan, vNodes) );
    if ( fVerbose )
    {
    printf( "Min-cut = %4d.  Unmoved = %4d. ", Vec_PtrSize(vNodes), Counter );
    PRT( "Time", clock() - clk );
    }
    return vNodes;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

