/**CFile****************************************************************

  FileName    [abcRr.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Redundancy removal.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcRr.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "proof/fraig/fraig.h"
#include "opt/sim/sim.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Abc_RRMan_t_ Abc_RRMan_t;
struct Abc_RRMan_t_
{
    // the parameters
    Abc_Ntk_t *      pNtk;             // the network
    int              nFaninLevels;     // the number of fanin levels
    int              nFanoutLevels;    // the number of fanout levels
    // the node/fanin/fanout
    Abc_Obj_t *      pNode;            // the node
    Abc_Obj_t *      pFanin;           // the fanin
    Abc_Obj_t *      pFanout;          // the fanout
    // the intermediate cones
    Vec_Ptr_t *      vFaninLeaves;     // the leaves of the fanin cone
    Vec_Ptr_t *      vFanoutRoots;     // the roots of the fanout cone
    // the window
    Vec_Ptr_t *      vLeaves;          // the leaves of the window
    Vec_Ptr_t *      vCone;            // the internal nodes of the window
    Vec_Ptr_t *      vRoots;           // the roots of the window
    Abc_Ntk_t *      pWnd;             // the window derived for the edge
    // the miter 
    Abc_Ntk_t *      pMiter;           // the miter derived from the window
    Prove_Params_t * pParams;          // the miter proving parameters
    // statistical variables
    int              nNodesOld;        // the old number of nodes
    int              nLevelsOld;       // the old number of levels
    int              nEdgesTried;      // the number of nodes tried
    int              nEdgesRemoved;    // the number of nodes proved
    abctime          timeWindow;       // the time to construct the window
    abctime          timeMiter;        // the time to construct the miter
    abctime          timeProve;        // the time to prove the miter
    abctime          timeUpdate;       // the network update time
    abctime          timeTotal;        // the total runtime
};

static Abc_RRMan_t * Abc_RRManStart();
static void          Abc_RRManStop( Abc_RRMan_t * p );
static void          Abc_RRManPrintStats( Abc_RRMan_t * p );
static void          Abc_RRManClean( Abc_RRMan_t * p );
static int           Abc_NtkRRProve( Abc_RRMan_t * p );
static int           Abc_NtkRRUpdate( Abc_Ntk_t * pNtk, Abc_Obj_t * pNode, Abc_Obj_t * pFanin, Abc_Obj_t * pFanout );
static int           Abc_NtkRRWindow( Abc_RRMan_t * p );

static int           Abc_NtkRRTfi_int( Vec_Ptr_t * vLeaves, int LevelLimit );
static int           Abc_NtkRRTfo_int( Vec_Ptr_t * vLeaves, Vec_Ptr_t * vRoots, int LevelLimit, Abc_Obj_t * pEdgeFanin, Abc_Obj_t * pEdgeFanout );
static int           Abc_NtkRRTfo_rec( Abc_Obj_t * pNode, Vec_Ptr_t * vRoots, int LevelLimit );
static void          Abc_NtkRRTfi_rec( Abc_Obj_t * pNode, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vCone, int LevelLimit );
static Abc_Ntk_t *   Abc_NtkWindow( Abc_Ntk_t * pNtk, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vCone, Vec_Ptr_t * vRoots );

static void          Abc_NtkRRSimulateStart( Abc_Ntk_t * pNtk );
static void          Abc_NtkRRSimulateStop( Abc_Ntk_t * pNtk );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Removes stuck-at redundancies.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRR( Abc_Ntk_t * pNtk, int nFaninLevels, int nFanoutLevels, int fUseFanouts, int fVerbose )
{
    ProgressBar * pProgress;
    Abc_RRMan_t * p;
    Abc_Obj_t * pNode, * pFanin, * pFanout;
    int i, k, m, nNodes, RetValue;
    abctime clk, clkTotal = Abc_Clock();
    // start the manager
    p = Abc_RRManStart();
    p->pNtk          = pNtk;
    p->nFaninLevels  = nFaninLevels;
    p->nFanoutLevels = nFanoutLevels;
    p->nNodesOld     = Abc_NtkNodeNum(pNtk);
    p->nLevelsOld    = Abc_AigLevel(pNtk);
    // remember latch values
//    Abc_NtkForEachLatch( pNtk, pNode, i )
//        pNode->pNext = pNode->pData;
    // go through the nodes
    Abc_NtkCleanCopy(pNtk);
    nNodes = Abc_NtkObjNumMax(pNtk);
    Abc_NtkRRSimulateStart(pNtk);
    pProgress = Extra_ProgressBarStart( stdout, nNodes );
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        // stop if all nodes have been tried once
        if ( i >= nNodes )
            break;
        // skip the constant node
//        if ( Abc_NodeIsConst(pNode) )
//            continue;
        // skip persistant nodes
        if ( Abc_NodeIsPersistant(pNode) )
            continue;
        // skip the nodes with many fanouts
        if ( Abc_ObjFanoutNum(pNode) > 1000 )
            continue;
        // construct the window
        if ( !fUseFanouts )
        {
            Abc_ObjForEachFanin( pNode, pFanin, k )
            {
                // skip the nodes with only one fanout (tree nodes)
                if ( Abc_ObjFanoutNum(pFanin) == 1 )
                    continue;
/*
                if ( pFanin->Id == 228 && pNode->Id == 2649 )
                {
                    int k = 0;
                }
*/
                p->nEdgesTried++;
                Abc_RRManClean( p );
                p->pNode   = pNode;
                p->pFanin  = pFanin;
                p->pFanout = NULL;

                clk = Abc_Clock();
                RetValue = Abc_NtkRRWindow( p );
                p->timeWindow += Abc_Clock() - clk;
                if ( !RetValue )
                    continue;
/*
                if ( pFanin->Id == 228 && pNode->Id == 2649 )
                {
                    Abc_NtkShowAig( p->pWnd, 0 );
                }
*/
                clk = Abc_Clock();
                RetValue = Abc_NtkRRProve( p );
                p->timeMiter += Abc_Clock() - clk;
                if ( !RetValue )
                    continue;
//printf( "%d -> %d (%d)\n", pFanin->Id, pNode->Id, k );

                clk = Abc_Clock();
                Abc_NtkRRUpdate( pNtk, p->pNode, p->pFanin, p->pFanout );
                p->timeUpdate += Abc_Clock() - clk;

                p->nEdgesRemoved++;
                break;
            }
            continue;
        }
        // use the fanouts
        Abc_ObjForEachFanin( pNode, pFanin, k )
        Abc_ObjForEachFanout( pNode, pFanout, m )
        {
            // skip the nodes with only one fanout (tree nodes)
//            if ( Abc_ObjFanoutNum(pFanin) == 1 && Abc_ObjFanoutNum(pNode) == 1 )
//                continue;

            p->nEdgesTried++;
            Abc_RRManClean( p );
            p->pNode   = pNode;
            p->pFanin  = pFanin;
            p->pFanout = pFanout;

            clk = Abc_Clock();
            RetValue = Abc_NtkRRWindow( p );
            p->timeWindow += Abc_Clock() - clk;
            if ( !RetValue )
                continue;

            clk = Abc_Clock();
            RetValue = Abc_NtkRRProve( p );
            p->timeMiter += Abc_Clock() - clk;
            if ( !RetValue )
                continue;

            clk = Abc_Clock();
            Abc_NtkRRUpdate( pNtk, p->pNode, p->pFanin, p->pFanout );
            p->timeUpdate += Abc_Clock() - clk;

            p->nEdgesRemoved++;
            break;
        }
    }
    Abc_NtkRRSimulateStop(pNtk);
    Extra_ProgressBarStop( pProgress );
    p->timeTotal = Abc_Clock() - clkTotal;
    if ( fVerbose )
        Abc_RRManPrintStats( p );
    Abc_RRManStop( p );
    // restore latch values
//    Abc_NtkForEachLatch( pNtk, pNode, i )
//        pNode->pData = pNode->pNext, pNode->pNext = NULL;
    // put the nodes into the DFS order and reassign their IDs
    Abc_NtkReassignIds( pNtk );
    Abc_NtkLevel( pNtk );
    // check
    if ( !Abc_NtkCheck( pNtk ) )
    {
        printf( "Abc_NtkRR: The network check has failed.\n" );
        return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Start the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_RRMan_t * Abc_RRManStart()
{
    Abc_RRMan_t * p;
    p = ABC_ALLOC( Abc_RRMan_t, 1 );
    memset( p, 0, sizeof(Abc_RRMan_t) );
    p->vFaninLeaves  = Vec_PtrAlloc( 100 );  // the leaves of the fanin cone
    p->vFanoutRoots  = Vec_PtrAlloc( 100 );  // the roots of the fanout cone
    p->vLeaves       = Vec_PtrAlloc( 100 );  // the leaves of the window
    p->vCone         = Vec_PtrAlloc( 100 );  // the internal nodes of the window
    p->vRoots        = Vec_PtrAlloc( 100 );  // the roots of the window
    p->pParams       = ABC_ALLOC( Prove_Params_t, 1 );
    memset( p->pParams, 0, sizeof(Prove_Params_t) );
    Prove_ParamsSetDefault( p->pParams );
    return p;
}

/**Function*************************************************************

  Synopsis    [Stop the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_RRManStop( Abc_RRMan_t * p )
{
    Abc_RRManClean( p );
    Vec_PtrFree( p->vFaninLeaves  );  
    Vec_PtrFree( p->vFanoutRoots  );  
    Vec_PtrFree( p->vLeaves );  
    Vec_PtrFree( p->vCone );  
    Vec_PtrFree( p->vRoots );  
    ABC_FREE( p->pParams );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Stop the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_RRManPrintStats( Abc_RRMan_t * p )
{
    double Ratio = 100.0*(p->nNodesOld  - Abc_NtkNodeNum(p->pNtk))/p->nNodesOld;
    printf( "Redundancy removal statistics:\n" );
    printf( "Edges tried     = %6d.\n", p->nEdgesTried );
    printf( "Edges removed   = %6d. (%5.2f %%)\n", p->nEdgesRemoved, 100.0*p->nEdgesRemoved/p->nEdgesTried );
    printf( "Node gain       = %6d. (%5.2f %%)\n", p->nNodesOld  - Abc_NtkNodeNum(p->pNtk), Ratio );
    printf( "Level gain      = %6d.\n", p->nLevelsOld - Abc_AigLevel(p->pNtk) );
    ABC_PRT( "Windowing      ", p->timeWindow );
    ABC_PRT( "Miter          ", p->timeMiter );
    ABC_PRT( "    Construct  ", p->timeMiter - p->timeProve );
    ABC_PRT( "    Prove      ", p->timeProve );
    ABC_PRT( "Update         ", p->timeUpdate );
    ABC_PRT( "TOTAL          ", p->timeTotal );
}

/**Function*************************************************************

  Synopsis    [Clean the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_RRManClean( Abc_RRMan_t * p )
{
    p->pNode   = NULL; 
    p->pFanin  = NULL; 
    p->pFanout = NULL; 
    Vec_PtrClear( p->vFaninLeaves  );  
    Vec_PtrClear( p->vFanoutRoots  );  
    Vec_PtrClear( p->vLeaves );  
    Vec_PtrClear( p->vCone );  
    Vec_PtrClear( p->vRoots );  
    if ( p->pWnd )   Abc_NtkDelete( p->pWnd );
    if ( p->pMiter ) Abc_NtkDelete( p->pMiter );
    p->pWnd   = NULL;
    p->pMiter = NULL;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the miter is constant 0.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRRProve( Abc_RRMan_t * p )
{
    Abc_Ntk_t * pWndCopy;
    int RetValue;
    abctime clk;
//    Abc_NtkShowAig( p->pWnd, 0 );
    pWndCopy = Abc_NtkDup( p->pWnd );
    Abc_NtkRRUpdate( pWndCopy, p->pNode->pCopy->pCopy, p->pFanin->pCopy->pCopy, p->pFanout? p->pFanout->pCopy->pCopy : NULL );
    if ( !Abc_NtkIsDfsOrdered(pWndCopy) )
        Abc_NtkReassignIds(pWndCopy);
    p->pMiter = Abc_NtkMiter( p->pWnd, pWndCopy, 1, 0, 0, 0 );
    Abc_NtkDelete( pWndCopy );
clk = Abc_Clock();
    RetValue  = Abc_NtkMiterProve( &p->pMiter, p->pParams );
p->timeProve += Abc_Clock() - clk;
    if ( RetValue == 1 )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Updates the network after redundancy removal.]

  Description [This procedure assumes that non-control value of the fanin
  was proved redundant. It is okay to concentrate on non-control values
  because the control values can be seen as redundancy of the fanout edge.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRRUpdate( Abc_Ntk_t * pNtk, Abc_Obj_t * pNode, Abc_Obj_t * pFanin, Abc_Obj_t * pFanout )
{
    Abc_Obj_t * pNodeNew = NULL, * pFanoutNew = NULL;
    assert( pFanout == NULL );
    assert( !Abc_ObjIsComplement(pNode) );
    assert( !Abc_ObjIsComplement(pFanin) );
    assert( !Abc_ObjIsComplement(pFanout) );
    // find the node after redundancy removal
    if ( pFanin == Abc_ObjFanin0(pNode) )
        pNodeNew = Abc_ObjChild1(pNode);
    else if ( pFanin == Abc_ObjFanin1(pNode) )
        pNodeNew = Abc_ObjChild0(pNode);
    else assert( 0 );
    // replace
    if ( pFanout == NULL )
    {
        Abc_AigReplace( (Abc_Aig_t *)pNtk->pManFunc, pNode, pNodeNew, 1 );
        return 1;
    }
    // find the fanout after redundancy removal
    if ( pNode == Abc_ObjFanin0(pFanout) )
        pFanoutNew = Abc_AigAnd( (Abc_Aig_t *)pNtk->pManFunc, Abc_ObjNotCond(pNodeNew,Abc_ObjFaninC0(pFanout)), Abc_ObjChild1(pFanout) );
    else if ( pNode == Abc_ObjFanin1(pFanout) )
        pFanoutNew = Abc_AigAnd( (Abc_Aig_t *)pNtk->pManFunc, Abc_ObjNotCond(pNodeNew,Abc_ObjFaninC1(pFanout)), Abc_ObjChild0(pFanout) );
    else assert( 0 );
    // replace
    Abc_AigReplace( (Abc_Aig_t *)pNtk->pManFunc, pFanout, pFanoutNew, 1 );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Constructs window for checking RR.]

  Description [If the window (p->pWnd) with the given scope (p->nFaninLevels, 
  p->nFanoutLevels) cannot be constructed, returns 0. Otherwise, returns 1.
  The levels are measured from the fanin node (pFanin) and the fanout node
  (pEdgeFanout), respectively.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRRWindow( Abc_RRMan_t * p )
{
    Abc_Obj_t * pObj, * pEdgeFanin, * pEdgeFanout;
    int i, LevelMin, LevelMax, RetValue;

    // get the edge
    pEdgeFanout = p->pFanout? p->pFanout : p->pNode;
    pEdgeFanin  = p->pFanout? p->pNode : p->pFanin;
    // get the minimum and maximum levels of the window
    LevelMin = Abc_MaxInt( 0, ((int)p->pFanin->Level) - p->nFaninLevels );
    LevelMax = (int)pEdgeFanout->Level + p->nFanoutLevels;

    // start the TFI leaves with the fanin
    Abc_NtkIncrementTravId( p->pNtk );
    Abc_NodeSetTravIdCurrent( p->pFanin );
    Vec_PtrPush( p->vFaninLeaves, p->pFanin );
    // mark the TFI cone and collect the leaves down to the given level
    while ( Abc_NtkRRTfi_int(p->vFaninLeaves, LevelMin) );

    // mark the leaves with the new TravId
    Abc_NtkIncrementTravId( p->pNtk );
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vFaninLeaves, pObj, i )
        Abc_NodeSetTravIdCurrent( pObj );
    // traverse the TFO cone of the leaves (while skipping the edge)
    // (a) mark the nodes in the cone using the current TravId
    // (b) collect the nodes that have external fanouts into p->vFanoutRoots
    while ( Abc_NtkRRTfo_int(p->vFaninLeaves, p->vFanoutRoots, LevelMax, pEdgeFanin, pEdgeFanout) );

    // mark the fanout roots
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vFanoutRoots, pObj, i )
        pObj->fMarkA = 1;
    // collect roots reachable from the fanout (p->vRoots)
    RetValue = Abc_NtkRRTfo_rec( pEdgeFanout, p->vRoots, LevelMax + 1 );
    // unmark the fanout roots
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vFanoutRoots, pObj, i )
        pObj->fMarkA = 0;

    // return if the window is infeasible
    if ( RetValue == 0 )
        return 0;

    // collect the DFS-ordered new cone (p->vCone) and new leaves (p->vLeaves)
    // using the previous marks coming from the TFO cone
    Abc_NtkIncrementTravId( p->pNtk );
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vRoots, pObj, i )
        Abc_NtkRRTfi_rec( pObj, p->vLeaves, p->vCone, LevelMin );

    // create a new network
    p->pWnd = Abc_NtkWindow( p->pNtk, p->vLeaves, p->vCone, p->vRoots );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Marks the nodes in the TFI and collects their leaves.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRRTfi_int( Vec_Ptr_t * vLeaves, int LevelLimit )
{
    Abc_Obj_t * pObj, * pNext;
    int i, k, LevelMax, nSize;
    assert( LevelLimit >= 0 );
    // find the maximum level of leaves
    LevelMax = 0;
    Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pObj, i )
        if ( LevelMax < (int)pObj->Level )
            LevelMax = pObj->Level;
    // if the nodes are all PIs, LevelMax == 0
    if ( LevelMax <= LevelLimit )
        return 0;
    // expand the nodes with the minimum level
    nSize = Vec_PtrSize(vLeaves);
    Vec_PtrForEachEntryStop( Abc_Obj_t *, vLeaves, pObj, i, nSize )
    {
        if ( LevelMax != (int)pObj->Level )
            continue;
        Abc_ObjForEachFanin( pObj, pNext, k )
        {
            if ( Abc_NodeIsTravIdCurrent(pNext) )
                continue;
            Abc_NodeSetTravIdCurrent( pNext );
            Vec_PtrPush( vLeaves, pNext );
        }
    }
    // remove old nodes (cannot remove a PI)
    k = 0;
    Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pObj, i )
    {
        if ( LevelMax == (int)pObj->Level )
            continue;
        Vec_PtrWriteEntry( vLeaves, k++, pObj );
    }
    Vec_PtrShrink( vLeaves, k );
    if ( Vec_PtrSize(vLeaves) > 2000 )
        return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Marks the nodes in the TFO and collects their roots.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRRTfo_int( Vec_Ptr_t * vLeaves, Vec_Ptr_t * vRoots, int LevelLimit, Abc_Obj_t * pEdgeFanin, Abc_Obj_t * pEdgeFanout )
{
    Abc_Obj_t * pObj, * pNext;
    int i, k, LevelMin, nSize, fObjIsRoot;
    // find the minimum level of leaves
    LevelMin = ABC_INFINITY;
    Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pObj, i )
        if ( LevelMin > (int)pObj->Level )
            LevelMin = pObj->Level;
    // if the minimum level exceed the limit, we are done
    if ( LevelMin > LevelLimit )
        return 0;
    // expand the nodes with the minimum level
    nSize = Vec_PtrSize(vLeaves);
    Vec_PtrForEachEntryStop( Abc_Obj_t *, vLeaves, pObj, i, nSize )
    {
        if ( LevelMin != (int)pObj->Level )
            continue;
        fObjIsRoot = 0;
        Abc_ObjForEachFanout( pObj, pNext, k )
        {
            // check if the fanout is outside of the cone
            if ( Abc_ObjIsCo(pNext) || pNext->Level > (unsigned)LevelLimit )
            {
                fObjIsRoot = 1;
                continue;
            }
            // skip the edge under check
            if ( pObj == pEdgeFanin && pNext == pEdgeFanout )
                continue;
            // skip the visited fanouts
            if ( Abc_NodeIsTravIdCurrent(pNext) )
                continue;
            Abc_NodeSetTravIdCurrent( pNext );
            Vec_PtrPush( vLeaves, pNext );
        }
        if ( fObjIsRoot )
            Vec_PtrPush( vRoots, pObj );
    }
    // remove old nodes
    k = 0;
    Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pObj, i )
    {
        if ( LevelMin == (int)pObj->Level )
            continue;
        Vec_PtrWriteEntry( vLeaves, k++, pObj );
    }
    Vec_PtrShrink( vLeaves, k );
    if ( Vec_PtrSize(vLeaves) > 2000 )
        return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Collects the roots in the TFO of the node.]

  Description [Note that this procedure can be improved by
  marking and skipping the visited nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRRTfo_rec( Abc_Obj_t * pNode, Vec_Ptr_t * vRoots, int LevelLimit )
{
    Abc_Obj_t * pFanout;
    int i;
    // if we encountered a node outside of the TFO cone of the fanins, quit
    if ( Abc_ObjIsCo(pNode) || pNode->Level > (unsigned)LevelLimit )
        return 0;
    // if we encountered a node on the boundary, add it to the roots
    if ( pNode->fMarkA )
    {
        Vec_PtrPushUnique( vRoots, pNode );
        return 1;
    }
    // mark the node with the current TravId (needed to have all internal nodes marked)
    Abc_NodeSetTravIdCurrent( pNode );
    // traverse the fanouts
    Abc_ObjForEachFanout( pNode, pFanout, i )
        if ( !Abc_NtkRRTfo_rec( pFanout, vRoots, LevelLimit ) )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Collects the leaves and cone of the roots.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkRRTfi_rec( Abc_Obj_t * pNode, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vCone, int LevelLimit )
{
    Abc_Obj_t * pFanin;
    int i;
    // skip visited nodes
    if ( Abc_NodeIsTravIdCurrent(pNode) )
        return;
    // add node to leaves if it is not in TFI cone of the leaves (marked before) or below the limit
    if ( !Abc_NodeIsTravIdPrevious(pNode) || (int)pNode->Level <= LevelLimit )
    {
        Abc_NodeSetTravIdCurrent( pNode );
        Vec_PtrPush( vLeaves, pNode );
        return;
    }
    // mark the node as visited
    Abc_NodeSetTravIdCurrent( pNode );
    // call for the node's fanins
    Abc_ObjForEachFanin( pNode, pFanin, i )
        Abc_NtkRRTfi_rec( pFanin, vLeaves, vCone, LevelLimit );
    // add the node to the cone in topological order
    Vec_PtrPush( vCone, pNode );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkWindow( Abc_Ntk_t * pNtk, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vCone, Vec_Ptr_t * vRoots )
{
    Abc_Ntk_t * pNtkNew; 
    Abc_Obj_t * pObj;
    int fCheck = 1;
    int i;
    assert( Abc_NtkIsStrash(pNtk) );
    // start the network
    pNtkNew = Abc_NtkAlloc( pNtk->ntkType, pNtk->ntkFunc, 1 );
    // duplicate the name and the spec
    pNtkNew->pName = Extra_UtilStrsav( "temp" );
    // map the constant nodes
    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);
    // create and map the PIs
    Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pObj, i )
        pObj->pCopy = Abc_NtkCreatePi(pNtkNew);
    // copy the AND gates
    Vec_PtrForEachEntry( Abc_Obj_t *, vCone, pObj, i )
        pObj->pCopy = Abc_AigAnd( (Abc_Aig_t *)pNtkNew->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj) );
    // compare the number of nodes before and after
    if ( Vec_PtrSize(vCone) != Abc_NtkNodeNum(pNtkNew) )
        printf( "Warning: Structural hashing during windowing reduced %d nodes (this is a bug).\n",
            Vec_PtrSize(vCone) - Abc_NtkNodeNum(pNtkNew) );
    // create the POs
    Vec_PtrForEachEntry( Abc_Obj_t *, vRoots, pObj, i )
    {
        assert( !Abc_ObjIsComplement(pObj->pCopy) );
        Abc_ObjAddFanin( Abc_NtkCreatePo(pNtkNew), pObj->pCopy );
    }
    // add the PI/PO names
    Abc_NtkAddDummyPiNames( pNtkNew );
    Abc_NtkAddDummyPoNames( pNtkNew );
    // check
    if ( fCheck && !Abc_NtkCheck( pNtkNew ) )
    {
        printf( "Abc_NtkWindow: The network check has failed.\n" );
        return NULL;
    }
    return pNtkNew;
}


/**Function*************************************************************

  Synopsis    [Starts simulation to detect non-redundant edges.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkRRSimulateStart( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    unsigned uData, uData0, uData1;
    int i;
    Abc_AigConst1(pNtk)->pData = (void *)~((unsigned)0);
    Abc_NtkForEachCi( pNtk, pObj, i )
        pObj->pData = (void *)(ABC_PTRUINT_T)SIM_RANDOM_UNSIGNED;
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        if ( i == 0 ) continue;
        uData0 = (unsigned)(ABC_PTRUINT_T)Abc_ObjFanin0(pObj)->pData;
        uData1 = (unsigned)(ABC_PTRUINT_T)Abc_ObjFanin1(pObj)->pData;
        uData  = Abc_ObjFaninC0(pObj)? ~uData0 : uData0;
        uData &= Abc_ObjFaninC1(pObj)? ~uData1 : uData1;
        assert( pObj->pData == NULL );
        pObj->pData = (void *)(ABC_PTRUINT_T)uData;
    }
}

/**Function*************************************************************

  Synopsis    [Stops simulation to detect non-redundant edges.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkRRSimulateStop( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;
    Abc_NtkForEachObj( pNtk, pObj, i )
        pObj->pData = NULL;
}







static void Sim_TraverseNodes_rec( Abc_Obj_t * pRoot, Vec_Str_t * vTargets, Vec_Ptr_t * vNodes );
static void Sim_CollectNodes_rec( Abc_Obj_t * pRoot, Vec_Ptr_t * vField );
static void Sim_SimulateCollected( Vec_Str_t * vTargets, Vec_Ptr_t * vNodes, Vec_Ptr_t * vField );

/**Function*************************************************************

  Synopsis    [Simulation to detect non-redundant edges.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Str_t * Abc_NtkRRSimulate( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vNodes, * vField;
    Vec_Str_t * vTargets;
    Abc_Obj_t * pObj;
    unsigned uData, uData0, uData1;
    int PrevCi, Phase, i, k;

    // start the candidates
    vTargets = Vec_StrStart( Abc_NtkObjNumMax(pNtk) + 1 );
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        Phase = ((Abc_ObjFanoutNum(Abc_ObjFanin1(pObj)) > 1) << 1);
        Phase |= (Abc_ObjFanoutNum(Abc_ObjFanin0(pObj)) > 1);
        Vec_StrWriteEntry( vTargets, pObj->Id, (char)Phase );
    }

    // simulate patters and store them in copy
    Abc_AigConst1(pNtk)->pCopy = (Abc_Obj_t *)~((unsigned)0);
    Abc_NtkForEachCi( pNtk, pObj, i )
        pObj->pCopy = (Abc_Obj_t *)(ABC_PTRUINT_T)SIM_RANDOM_UNSIGNED;
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        if ( i == 0 ) continue;
        uData0 = (unsigned)(ABC_PTRUINT_T)Abc_ObjFanin0(pObj)->pData;
        uData1 = (unsigned)(ABC_PTRUINT_T)Abc_ObjFanin1(pObj)->pData;
        uData  = Abc_ObjFaninC0(pObj)? ~uData0 : uData0;
        uData &= Abc_ObjFaninC1(pObj)? ~uData1 : uData1;
        pObj->pCopy = (Abc_Obj_t *)(ABC_PTRUINT_T)uData;
    }
    // store the result in data
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        uData0 = (unsigned)(ABC_PTRUINT_T)Abc_ObjFanin0(pObj)->pData;
        if ( Abc_ObjFaninC0(pObj) )
            pObj->pData = (void *)(ABC_PTRUINT_T)~uData0;
        else
            pObj->pData = (void *)(ABC_PTRUINT_T)uData0;
    }

    // refine the candidates
    for ( PrevCi = 0; PrevCi < Abc_NtkCiNum(pNtk); PrevCi = i )
    {
        vNodes = Vec_PtrAlloc( 10 );
        Abc_NtkIncrementTravId( pNtk );
        for ( i = PrevCi; i < Abc_NtkCiNum(pNtk); i++ )
        {
            Sim_TraverseNodes_rec( Abc_NtkCi(pNtk, i), vTargets, vNodes );
            if ( Vec_PtrSize(vNodes) > 128 )
                break;
        }
        // collect the marked nodes in the topological order
        vField = Vec_PtrAlloc( 10 );
        Abc_NtkIncrementTravId( pNtk );
        Abc_NtkForEachCo( pNtk, pObj, k )
            Sim_CollectNodes_rec( pObj, vField );

        // simulate these nodes
        Sim_SimulateCollected( vTargets, vNodes, vField );
        // prepare for the next loop
        Vec_PtrFree( vNodes );
    }

    // clean
    Abc_NtkForEachObj( pNtk, pObj, i )
        pObj->pData = NULL;
    return vTargets;
}

/**Function*************************************************************

  Synopsis    [Collects nodes starting from the given node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_TraverseNodes_rec( Abc_Obj_t * pRoot, Vec_Str_t * vTargets, Vec_Ptr_t * vNodes )
{
    Abc_Obj_t * pFanout;
    char Entry;
    int k;
    if ( Abc_NodeIsTravIdCurrent(pRoot) )
        return;
    Abc_NodeSetTravIdCurrent( pRoot );
    // save the reached targets
    Entry = Vec_StrEntry(vTargets, pRoot->Id);
    if ( Entry & 1 )
        Vec_PtrPush( vNodes, Abc_ObjNot(pRoot) );
    if ( Entry & 2 )
        Vec_PtrPush( vNodes, pRoot );
    // explore the fanouts
    Abc_ObjForEachFanout( pRoot, pFanout, k )
        Sim_TraverseNodes_rec( pFanout, vTargets, vNodes );
}

/**Function*************************************************************

  Synopsis    [Collects nodes starting from the given node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_CollectNodes_rec( Abc_Obj_t * pRoot, Vec_Ptr_t * vField )
{
    Abc_Obj_t * pFanin;
    int i;
    if ( Abc_NodeIsTravIdCurrent(pRoot) )
        return;
    if ( !Abc_NodeIsTravIdPrevious(pRoot) )
        return;
    Abc_NodeSetTravIdCurrent( pRoot );
    Abc_ObjForEachFanin( pRoot, pFanin, i )
        Sim_CollectNodes_rec( pFanin, vField );
    if ( !Abc_ObjIsCo(pRoot) )
        pRoot->pData = (void *)(ABC_PTRUINT_T)Vec_PtrSize(vField);
    Vec_PtrPush( vField, pRoot );
}

/**Function*************************************************************

  Synopsis    [Simulate the given nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_SimulateCollected( Vec_Str_t * vTargets, Vec_Ptr_t * vNodes, Vec_Ptr_t * vField )
{
    Abc_Obj_t * pObj, * pFanin0, * pFanin1, * pDisproved;
    Vec_Ptr_t * vSims;
    unsigned * pUnsigned, * pUnsignedF;
    int i, k, Phase, fCompl;
    // get simulation info
    vSims = Sim_UtilInfoAlloc( Vec_PtrSize(vField), Vec_PtrSize(vNodes), 0 );
    // simulate the nodes
    Vec_PtrForEachEntry( Abc_Obj_t *, vField, pObj, i )
    {
        if ( Abc_ObjIsCi(pObj) )
        {
            pUnsigned = (unsigned *)Vec_PtrEntry( vSims, i );
            for ( k = 0; k < Vec_PtrSize(vNodes); k++ )
                pUnsigned[k] = (unsigned)(ABC_PTRUINT_T)pObj->pCopy;
            continue;
        }
        if ( Abc_ObjIsCo(pObj) )
        {
            pUnsigned  = (unsigned *)Vec_PtrEntry( vSims, i );
            pUnsignedF = (unsigned *)Vec_PtrEntry( vSims, (int)(ABC_PTRUINT_T)Abc_ObjFanin0(pObj)->pData );
            if ( Abc_ObjFaninC0(pObj) )
                for ( k = 0; k < Vec_PtrSize(vNodes); k++ )
                    pUnsigned[k] = ~pUnsignedF[k];
            else
                for ( k = 0; k < Vec_PtrSize(vNodes); k++ )
                    pUnsigned[k] = pUnsignedF[k];
            // update targets
            for ( k = 0; k < Vec_PtrSize(vNodes); k++ )
            {
                if ( pUnsigned[k] == (unsigned)(ABC_PTRUINT_T)pObj->pData )
                    continue;
                pDisproved = (Abc_Obj_t *)Vec_PtrEntry( vNodes, k );
                fCompl = Abc_ObjIsComplement(pDisproved);
                pDisproved = Abc_ObjRegular(pDisproved);
                Phase = Vec_StrEntry( vTargets, pDisproved->Id );
                if ( fCompl )
                    Phase = (Phase & 2);
                else
                    Phase = (Phase & 1);
                Vec_StrWriteEntry( vTargets, pDisproved->Id, (char)Phase );
            }
            continue;
        }
        // simulate the node
        pFanin0 = Abc_ObjFanin0(pObj);
        pFanin1 = Abc_ObjFanin1(pObj);
    }
}



/*
                {
                    unsigned uData;
                    if ( pFanin == Abc_ObjFanin0(pNode) )
                    {
                        uData = (unsigned)Abc_ObjFanin1(pNode)->pData;
                        uData = Abc_ObjFaninC1(pNode)? ~uData : uData;
                    }
                    else if ( pFanin == Abc_ObjFanin1(pNode) )
                    {
                        uData = (unsigned)Abc_ObjFanin0(pNode)->pData;
                        uData = Abc_ObjFaninC0(pNode)? ~uData : uData;
                    }
                    uData ^= (unsigned)pNode->pData;
//                    Extra_PrintBinary( stdout, &uData, 32 ); printf( "\n" );
                    if ( Extra_WordCountOnes(uData) > 8 )
                        continue;
                }
*/

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

