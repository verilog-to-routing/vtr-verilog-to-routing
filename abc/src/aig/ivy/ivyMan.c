/**CFile****************************************************************

  FileName    [ivyMan.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [And-Inverter Graph package.]

  Synopsis    [AIG manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 11, 2006.]

  Revision    [$Id: ivy_.c,v 1.00 2006/05/11 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ivy.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts the AIG manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ivy_Man_t * Ivy_ManStart()
{
    Ivy_Man_t * p;
    // start the manager
    p = ABC_ALLOC( Ivy_Man_t, 1 );
    memset( p, 0, sizeof(Ivy_Man_t) );
    // perform initializations
    p->Ghost.Id   = -1;
    p->nTravIds   =  1;
    p->fCatchExor =  1;
    // allocate arrays for nodes
    p->vPis = Vec_PtrAlloc( 100 );
    p->vPos = Vec_PtrAlloc( 100 );
    p->vBufs = Vec_PtrAlloc( 100 );
    p->vObjs = Vec_PtrAlloc( 100 );
    // prepare the internal memory manager
    Ivy_ManStartMemory( p );
    // create the constant node
    p->pConst1 = Ivy_ManFetchMemory( p );
    p->pConst1->fPhase = 1;
    Vec_PtrPush( p->vObjs, p->pConst1 );
    p->nCreated = 1;
    // start the table
    p->nTableSize = 10007;
    p->pTable = ABC_ALLOC( int, p->nTableSize );
    memset( p->pTable, 0, sizeof(int) * p->nTableSize );
    return p;
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ivy_Man_t * Ivy_ManStartFrom( Ivy_Man_t * p )
{
    Ivy_Man_t * pNew;
    Ivy_Obj_t * pObj;
    int i;
    // create the new manager
    pNew = Ivy_ManStart();
    // create the PIs
    Ivy_ManConst1(p)->pEquiv = Ivy_ManConst1(pNew);
    Ivy_ManForEachPi( p, pObj, i )
        pObj->pEquiv = Ivy_ObjCreatePi(pNew);
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ivy_Man_t * Ivy_ManDup( Ivy_Man_t * p )
{
    Vec_Int_t * vNodes, * vLatches;
    Ivy_Man_t * pNew;
    Ivy_Obj_t * pObj;
    int i;
    // collect latches and nodes in the DFS order
    vNodes = Ivy_ManDfsSeq( p, &vLatches );
    // create the new manager
    pNew = Ivy_ManStart();
    // create the PIs
    Ivy_ManConst1(p)->pEquiv = Ivy_ManConst1(pNew);
    Ivy_ManForEachPi( p, pObj, i )
        pObj->pEquiv = Ivy_ObjCreatePi(pNew);
    // create the fake PIs for latches
    Ivy_ManForEachNodeVec( p, vLatches, pObj, i )
        pObj->pEquiv = Ivy_ObjCreatePi(pNew);
    // duplicate internal nodes
    Ivy_ManForEachNodeVec( p, vNodes, pObj, i )
        if ( Ivy_ObjIsBuf(pObj) )
            pObj->pEquiv = Ivy_ObjChild0Equiv(pObj);
        else
            pObj->pEquiv = Ivy_And( pNew, Ivy_ObjChild0Equiv(pObj), Ivy_ObjChild1Equiv(pObj) );
    // add the POs
    Ivy_ManForEachPo( p, pObj, i )
        Ivy_ObjCreatePo( pNew, Ivy_ObjChild0Equiv(pObj) );
    // transform additional PI nodes into latches and connect them
    Ivy_ManForEachNodeVec( p, vLatches, pObj, i )
    {
        assert( !Ivy_ObjFaninC0(pObj) );
        pObj->pEquiv->Type = IVY_LATCH;
        pObj->pEquiv->Init = pObj->Init;
        Ivy_ObjConnect( pNew, pObj->pEquiv, Ivy_ObjChild0Equiv(pObj), NULL );
    }
    // shrink the arrays
    Vec_PtrShrink( pNew->vPis, Ivy_ManPiNum(p) );
    // update the counters of different objects
    pNew->nObjs[IVY_PI] -= Ivy_ManLatchNum(p);
    pNew->nObjs[IVY_LATCH] += Ivy_ManLatchNum(p);
    // free arrays
    Vec_IntFree( vNodes );
    Vec_IntFree( vLatches );
    // make sure structural hashing did not change anything
    assert( Ivy_ManNodeNum(p)  == Ivy_ManNodeNum(pNew) );
    assert( Ivy_ManLatchNum(p) == Ivy_ManLatchNum(pNew) );
    // check the resulting network
    if ( !Ivy_ManCheck(pNew) )
        printf( "Ivy_ManMakeSeq(): The check has failed.\n" );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Stops the AIG manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ivy_Man_t * Ivy_ManFrames( Ivy_Man_t * pMan, int nLatches, int nFrames, int fInit, Vec_Ptr_t ** pvMapping )
{
    Vec_Ptr_t * vMapping;
    Ivy_Man_t * pNew;
    Ivy_Obj_t * pObj;
    int i, f, nPis, nPos, nIdMax;
    assert( Ivy_ManLatchNum(pMan) == 0 );
    assert( nFrames > 0 );
    // prepare the mapping
    nPis = Ivy_ManPiNum(pMan) - nLatches;
    nPos = Ivy_ManPoNum(pMan) - nLatches;
    nIdMax = Ivy_ManObjIdMax(pMan);
    // create the new manager
    pNew = Ivy_ManStart();
    // set the starting values of latch inputs
    for ( i = 0; i < nLatches; i++ )
        Ivy_ManPo(pMan, nPos+i)->pEquiv = fInit? Ivy_Not(Ivy_ManConst1(pNew)) : Ivy_ObjCreatePi(pNew);
    // add timeframes
    vMapping = Vec_PtrStart( nIdMax * nFrames + 1 );
    for ( f = 0; f < nFrames; f++ )
    {
        // create PIs
        Ivy_ManConst1(pMan)->pEquiv = Ivy_ManConst1(pNew);
        for ( i = 0; i < nPis; i++ )
            Ivy_ManPi(pMan, i)->pEquiv = Ivy_ObjCreatePi(pNew);
        // transfer values to latch outputs
        for ( i = 0; i < nLatches; i++ )
            Ivy_ManPi(pMan, nPis+i)->pEquiv = Ivy_ManPo(pMan, nPos+i)->pEquiv;
        // perform strashing
        Ivy_ManForEachNode( pMan, pObj, i )
            pObj->pEquiv = Ivy_And( pNew, Ivy_ObjChild0Equiv(pObj), Ivy_ObjChild1Equiv(pObj) );
        // create POs
        for ( i = 0; i < nPos; i++ )
            Ivy_ManPo(pMan, i)->pEquiv = Ivy_ObjCreatePo( pNew, Ivy_ObjChild0Equiv(Ivy_ManPo(pMan, i)) );
        // set the results of latch inputs
        for ( i = 0; i < nLatches; i++ )
            Ivy_ManPo(pMan, nPos+i)->pEquiv = Ivy_ObjChild0Equiv(Ivy_ManPo(pMan, nPos+i));
        // save the pointers in this frame
        Ivy_ManForEachObj( pMan, pObj, i )
            Vec_PtrWriteEntry( vMapping, f * nIdMax + i, pObj->pEquiv );
    }
    // connect latches
    if ( !fInit )
        for ( i = 0; i < nLatches; i++ )
            Ivy_ObjCreatePo( pNew, Ivy_ManPo(pMan, nPos+i)->pEquiv );
    // remove dangling nodes
    Ivy_ManCleanup(pNew);
    *pvMapping = vMapping;
    // check the resulting network
    if ( !Ivy_ManCheck(pNew) )
        printf( "Ivy_ManFrames(): The check has failed.\n" );
    return pNew;
}


/**Function*************************************************************

  Synopsis    [Stops the AIG manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ManStop( Ivy_Man_t * p )
{
    if ( p->time1 ) { ABC_PRT( "Update lev  ", p->time1 ); }
    if ( p->time2 ) { ABC_PRT( "Update levR ", p->time2 ); }
//    Ivy_TableProfile( p );
//    if ( p->vFanouts )  Ivy_ManStopFanout( p );
    if ( p->vChunks )   Ivy_ManStopMemory( p );
    if ( p->vRequired ) Vec_IntFree( p->vRequired );
    if ( p->vPis )      Vec_PtrFree( p->vPis );
    if ( p->vPos )      Vec_PtrFree( p->vPos );
    if ( p->vBufs )     Vec_PtrFree( p->vBufs );
    if ( p->vObjs )     Vec_PtrFree( p->vObjs );
    ABC_FREE( p->pTable );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Removes nodes without fanout.]

  Description [Returns the number of dangling nodes removed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_ManCleanup( Ivy_Man_t * p )
{
    Ivy_Obj_t * pNode;
    int i, nNodesOld;
    nNodesOld = Ivy_ManNodeNum(p);
    Ivy_ManForEachObj( p, pNode, i )
        if ( Ivy_ObjIsNode(pNode) || Ivy_ObjIsLatch(pNode) || Ivy_ObjIsBuf(pNode) )
            if ( Ivy_ObjRefs(pNode) == 0 )
                Ivy_ObjDelete_rec( p, pNode, 1 );
//printf( "Cleanup removed %d nodes.\n", nNodesOld - Ivy_ManNodeNum(p) );
    return nNodesOld - Ivy_ManNodeNum(p);
}

/**Function*************************************************************

  Synopsis    [Marks nodes reachable from the given one.]

  Description [Returns the number of dangling nodes removed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ManCleanupSeq_rec( Ivy_Obj_t * pObj )
{
    if ( Ivy_ObjIsMarkA(pObj) )
        return;
    Ivy_ObjSetMarkA(pObj);
    if ( pObj->pFanin0 != NULL )
        Ivy_ManCleanupSeq_rec( Ivy_ObjFanin0(pObj) );
    if ( pObj->pFanin1 != NULL )
        Ivy_ManCleanupSeq_rec( Ivy_ObjFanin1(pObj) );
}

/**Function*************************************************************

  Synopsis    [Removes logic that does not feed into POs.]

  Description [Returns the number of dangling nodes removed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_ManCleanupSeq( Ivy_Man_t * p )
{
    Vec_Ptr_t * vNodes;
    Ivy_Obj_t * pObj;
    int i, RetValue;
    // mark the constant and PIs
    Ivy_ObjSetMarkA( Ivy_ManConst1(p) );
    Ivy_ManForEachPi( p, pObj, i )
        Ivy_ObjSetMarkA( pObj );
    // mark nodes visited from POs
    Ivy_ManForEachPo( p, pObj, i )
        Ivy_ManCleanupSeq_rec( pObj );
    // collect unmarked nodes
    vNodes = Vec_PtrAlloc( 100 );
    Ivy_ManForEachObj( p, pObj, i )
    {
        if ( Ivy_ObjIsMarkA(pObj) )
            Ivy_ObjClearMarkA(pObj);
        else
            Vec_PtrPush( vNodes, pObj );
    }
    if ( Vec_PtrSize(vNodes) == 0 )
    {
        Vec_PtrFree( vNodes );
//printf( "Sequential sweep cleaned out %d nodes.\n", 0 );
        return 0;
    }
    // disconnect the marked objects
    Vec_PtrForEachEntry( Ivy_Obj_t *, vNodes, pObj, i )
        Ivy_ObjDisconnect( p, pObj );
    // remove the dangling objects
    Vec_PtrForEachEntry( Ivy_Obj_t *, vNodes, pObj, i )
    {
        assert( Ivy_ObjIsNode(pObj) || Ivy_ObjIsLatch(pObj) || Ivy_ObjIsBuf(pObj) );
        assert( Ivy_ObjRefs(pObj) == 0 );
        // update node counters of the manager
        p->nObjs[pObj->Type]--;
        p->nDeleted++;
        // delete buffer from the array of buffers
        if ( p->fFanout && Ivy_ObjIsBuf(pObj) )
            Vec_PtrRemove( p->vBufs, pObj );
        // free the node
        Vec_PtrWriteEntry( p->vObjs, pObj->Id, NULL );
        Ivy_ManRecycleMemory( p, pObj );
    }
    // return the number of nodes freed
    RetValue = Vec_PtrSize(vNodes);
    Vec_PtrFree( vNodes );
//printf( "Sequential sweep cleaned out %d nodes.\n", RetValue );
    return RetValue;
}


/**Function*************************************************************

  Synopsis    [Checks if latches form self-loop.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_ManLatchIsSelfFeed_rec( Ivy_Obj_t * pLatch, Ivy_Obj_t * pLatchRoot )
{
    if ( !Ivy_ObjIsLatch(pLatch) && !Ivy_ObjIsBuf(pLatch) )
        return 0;
    if ( pLatch == pLatchRoot )
        return 1;
    return Ivy_ManLatchIsSelfFeed_rec( Ivy_ObjFanin0(pLatch), pLatchRoot );
}

/**Function*************************************************************

  Synopsis    [Checks if latches form self-loop.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_ManLatchIsSelfFeed( Ivy_Obj_t * pLatch )
{
    if ( !Ivy_ObjIsLatch(pLatch) )
        return 0;
    return Ivy_ManLatchIsSelfFeed_rec( Ivy_ObjFanin0(pLatch), pLatch );
}


/**Function*************************************************************

  Synopsis    [Returns the number of dangling nodes removed.]

  Description []
               
  SideEffects []

  SeeAlso     [] 

***********************************************************************/
int Ivy_ManPropagateBuffers( Ivy_Man_t * p, int fUpdateLevel )
{
    Ivy_Obj_t * pNode;
    int LimitFactor = 100;
    int NodeBeg = Ivy_ManNodeNum(p);
    int nSteps;
    for ( nSteps = 0; Vec_PtrSize(p->vBufs) > 0; nSteps++ )
    {
        pNode = (Ivy_Obj_t *)Vec_PtrEntryLast(p->vBufs);
        while ( Ivy_ObjIsBuf(pNode) )
            pNode = Ivy_ObjReadFirstFanout( p, pNode );
        // check if this buffer should remain
        if ( Ivy_ManLatchIsSelfFeed(pNode) )
        {
            Vec_PtrPop(p->vBufs);
            continue;
        }
//printf( "Propagating buffer %d with input %d and output %d\n", Ivy_ObjFaninId0(pNode), Ivy_ObjFaninId0(Ivy_ObjFanin0(pNode)), pNode->Id );
//printf( "Latch num %d\n", Ivy_ManLatchNum(p) );
        Ivy_NodeFixBufferFanins( p, pNode, fUpdateLevel );
        if ( nSteps > NodeBeg * LimitFactor )
        {
            printf( "Structural hashing is not finished after %d forward latch moves.\n", NodeBeg * LimitFactor );
            printf( "This circuit cannot be forward-retimed completely. Quitting.\n" );
            break;
        }
    }
//    printf( "Number of steps = %d. Nodes beg = %d. Nodes end = %d.\n", nSteps, NodeBeg, Ivy_ManNodeNum(p) );
    return nSteps;
}

/**Function*************************************************************

  Synopsis    [Stops the AIG manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ManPrintStats( Ivy_Man_t * p )
{
    printf( "PI/PO = %d/%d ", Ivy_ManPiNum(p), Ivy_ManPoNum(p) );
    printf( "A = %7d. ",       Ivy_ManAndNum(p) );
    printf( "L = %5d. ",       Ivy_ManLatchNum(p) );
//    printf( "X = %d. ",       Ivy_ManExorNum(p) );
//    printf( "B = %3d. ",       Ivy_ManBufNum(p) );
    printf( "MaxID = %7d. ",   Ivy_ManObjIdMax(p) );
//    printf( "Cre = %d. ",     p->nCreated );
//    printf( "Del = %d. ",     p->nDeleted );
    printf( "Lev = %3d. ",     Ivy_ManLatchNum(p)? -1 : Ivy_ManLevels(p) );
    printf( "\n" );
    fflush( stdout );
}

/**Function*************************************************************

  Synopsis    [Converts a combinational AIG manager into a sequential one.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ManMakeSeq( Ivy_Man_t * p, int nLatches, int * pInits )
{
    Ivy_Obj_t * pObj, * pLatch;
    Ivy_Init_t Init;
    int i;
    if ( nLatches == 0 )
        return;
    assert( nLatches < Ivy_ManPiNum(p) && nLatches < Ivy_ManPoNum(p) );
    assert( Ivy_ManPiNum(p) == Vec_PtrSize(p->vPis) );
    assert( Ivy_ManPoNum(p) == Vec_PtrSize(p->vPos) );
    assert( Vec_PtrSize( p->vBufs ) == 0 );
    // create fanouts
    if ( p->fFanout == 0 )
        Ivy_ManStartFanout( p );
    // collect the POs to be converted into latches
    for ( i = 0; i < nLatches; i++ )
    {
        // get the latch value
        Init = pInits? (Ivy_Init_t)pInits[i] : IVY_INIT_0;
        // create latch
        pObj = Ivy_ManPo( p, Ivy_ManPoNum(p) - nLatches + i );
        pLatch = Ivy_Latch( p, Ivy_ObjChild0(pObj), Init );
        Ivy_ObjDisconnect( p, pObj );
        // recycle the old PO object
        Vec_PtrWriteEntry( p->vObjs, pObj->Id, NULL );
        Ivy_ManRecycleMemory( p, pObj );
        // convert the corresponding PI to a buffer and connect it to the latch
        pObj = Ivy_ManPi( p, Ivy_ManPiNum(p) - nLatches + i );
        pObj->Type = IVY_BUF;
        Ivy_ObjConnect( p, pObj, pLatch, NULL );
        // save the buffer
        Vec_PtrPush( p->vBufs, pObj );
    }
    // shrink the arrays
    Vec_PtrShrink( p->vPis, Ivy_ManPiNum(p) - nLatches );
    Vec_PtrShrink( p->vPos, Ivy_ManPoNum(p) - nLatches );
    // update the counters of different objects
    p->nObjs[IVY_PI] -= nLatches;
    p->nObjs[IVY_PO] -= nLatches;
    p->nObjs[IVY_BUF] += nLatches;
    p->nDeleted -= 2 * nLatches;
    // remove dangling nodes
    Ivy_ManCleanup(p);
    Ivy_ManCleanupSeq(p);
/* 
    // check for dangling nodes
    Ivy_ManForEachObj( p, pObj, i )
        if ( !Ivy_ObjIsPi(pObj) && !Ivy_ObjIsPo(pObj) && !Ivy_ObjIsConst1(pObj) )
        {
            assert( Ivy_ObjRefs(pObj) > 0 );
            assert( Ivy_ObjRefs(pObj) == Ivy_ObjFanoutNum(p, pObj) );
        }
*/
    // perform hashing by propagating the buffers
    Ivy_ManPropagateBuffers( p, 0 );
    if ( Ivy_ManBufNum(p) )
        printf( "The number of remaining buffers is %d.\n", Ivy_ManBufNum(p) );
    // fix the levels
    Ivy_ManResetLevels( p );
    // check the resulting network
    if ( !Ivy_ManCheck(p) )
        printf( "Ivy_ManMakeSeq(): The check has failed.\n" );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

