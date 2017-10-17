/**CFile****************************************************************

  FileName    [aigSeq.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [Sequential strashing.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aigSeq.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Converts combinational AIG manager into a sequential one.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManSeqStrashConvert( Aig_Man_t * p, int nLatches, int * pInits )
{
    Aig_Obj_t * pObjLi, * pObjLo, * pLatch;
    int i;
    assert( Vec_PtrSize( p->vBufs ) == 0 );
    // collect the POs to be converted into latches
    for ( i = 0; i < nLatches; i++ )
    {
        // get the corresponding PI/PO pair
        pObjLi = Aig_ManPo( p, Aig_ManPoNum(p) - nLatches + i );
        pObjLo = Aig_ManPi( p, Aig_ManPiNum(p) - nLatches + i );
        // create latch
        pLatch = Aig_Latch( p, Aig_ObjChild0(pObjLi), pInits? pInits[i] : 0 );
        // recycle the old PO object
        Aig_ObjDisconnect( p, pObjLi );
        Vec_PtrWriteEntry( p->vObjs, pObjLi->Id, NULL );
        Aig_ManRecycleMemory( p, pObjLi );
        // convert the corresponding PI to be a buffer and connect it to the latch
        pObjLo->Type = AIG_OBJ_BUF;
        Aig_ObjConnect( p, pObjLo, pLatch, NULL );
        // save the buffer
//        Vec_PtrPush( p->vBufs, pObjLo );
    }
    // shrink the arrays
    Vec_PtrShrink( p->vPis, Aig_ManPiNum(p) - nLatches );
    Vec_PtrShrink( p->vPos, Aig_ManPoNum(p) - nLatches );
    // update the counters of different objects
    p->nObjs[AIG_OBJ_PI]  -= nLatches;
    p->nObjs[AIG_OBJ_PO]  -= nLatches;
    p->nObjs[AIG_OBJ_BUF] += nLatches;
}

/**Function*************************************************************

  Synopsis    [Collects internal nodes in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManDfsSeq_rec( Aig_Man_t * p, Aig_Obj_t * pObj, Vec_Ptr_t * vNodes )
{
    assert( !Aig_IsComplement(pObj) );
    if ( pObj == NULL )
        return;
    if ( Aig_ObjIsTravIdCurrent( p, pObj ) )
        return;
    Aig_ObjSetTravIdCurrent( p, pObj );
    if ( Aig_ObjIsPi(pObj) || Aig_ObjIsConst1(pObj) )
        return;
    Aig_ManDfsSeq_rec( p, Aig_ObjFanin0(pObj), vNodes );
    Aig_ManDfsSeq_rec( p, Aig_ObjFanin1(pObj), vNodes );
//    if ( (Aig_ObjFanin0(pObj) == NULL || Aig_ObjIsBuf(Aig_ObjFanin0(pObj))) &&
//         (Aig_ObjFanin1(pObj) == NULL || Aig_ObjIsBuf(Aig_ObjFanin1(pObj))) )
        Vec_PtrPush( vNodes, pObj );
}

/**Function*************************************************************

  Synopsis    [Collects internal nodes in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManDfsSeq( Aig_Man_t * p )
{
    Vec_Ptr_t * vNodes;
    Aig_Obj_t * pObj;
    int i;
    Aig_ManIncrementTravId( p );
    vNodes = Vec_PtrAlloc( Aig_ManNodeNum(p) );
    Aig_ManForEachPo( p, pObj, i )
        Aig_ManDfsSeq_rec( p, Aig_ObjFanin0(pObj), vNodes );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Collects internal nodes in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManDfsUnreach_rec( Aig_Man_t * p, Aig_Obj_t * pObj, Vec_Ptr_t * vNodes )
{
    assert( !Aig_IsComplement(pObj) );
    if ( pObj == NULL )
        return;
    if ( Aig_ObjIsTravIdPrevious(p, pObj) || Aig_ObjIsTravIdCurrent(p, pObj) )
        return;
    Aig_ObjSetTravIdPrevious( p, pObj ); // assume unknown
    Aig_ManDfsUnreach_rec( p, Aig_ObjFanin0(pObj), vNodes );
    Aig_ManDfsUnreach_rec( p, Aig_ObjFanin1(pObj), vNodes );
    if ( Aig_ObjIsTravIdPrevious(p, Aig_ObjFanin0(pObj)) && 
        (Aig_ObjFanin1(pObj) == NULL || Aig_ObjIsTravIdPrevious(p, Aig_ObjFanin1(pObj))) )
        Vec_PtrPush( vNodes, pObj );
    else
        Aig_ObjSetTravIdCurrent( p, pObj );
}

/**Function*************************************************************

  Synopsis    [Collects internal nodes unreachable from PIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManDfsUnreach( Aig_Man_t * p )
{
    Vec_Ptr_t * vNodes;
    Aig_Obj_t * pObj, * pFanin;
    int i, k;//, RetValue;
    // collect unreachable nodes
    Aig_ManIncrementTravId( p ); 
    Aig_ManIncrementTravId( p ); 
    // mark the constant and PIs
    Aig_ObjSetTravIdPrevious( p, Aig_ManConst1(p) );
    Aig_ManForEachPi( p, pObj, i )
        Aig_ObjSetTravIdCurrent( p, pObj );
    // curr marks visited nodes reachable from PIs
    // prev marks visited nodes unreachable or unknown

    // collect the unreachable nodes
    vNodes = Vec_PtrAlloc( 32 );
    Aig_ManForEachPo( p, pObj, i )
        Aig_ManDfsUnreach_rec( p, Aig_ObjFanin0(pObj), vNodes );

    // refine resulting nodes
    do 
    {
        k = 0;
        Vec_PtrForEachEntry( vNodes, pObj, i )
        {
            assert( Aig_ObjIsTravIdPrevious(p, pObj) );
            if ( Aig_ObjIsLatch(pObj) || Aig_ObjIsBuf(pObj) )
            {
                pFanin = Aig_ObjFanin0(pObj);
                assert( Aig_ObjIsTravIdPrevious(p, pFanin) || Aig_ObjIsTravIdCurrent(p, pFanin) );
                if ( Aig_ObjIsTravIdCurrent(p, pFanin) )
                {
                    Aig_ObjSetTravIdCurrent( p, pObj );
                    continue;
                }
            }
            else // AND gate
            {
                assert( Aig_ObjIsNode(pObj) );
                pFanin = Aig_ObjFanin0(pObj);
                assert( Aig_ObjIsTravIdPrevious(p, pFanin) || Aig_ObjIsTravIdCurrent(p, pFanin) );
                if ( Aig_ObjIsTravIdCurrent(p, pFanin) )
                {
                    Aig_ObjSetTravIdCurrent( p, pObj );
                    continue;
                }
                pFanin = Aig_ObjFanin1(pObj);
                assert( Aig_ObjIsTravIdPrevious(p, pFanin) || Aig_ObjIsTravIdCurrent(p, pFanin) );
                if ( Aig_ObjIsTravIdCurrent(p, pFanin) )
                {
                    Aig_ObjSetTravIdCurrent( p, pObj );
                    continue;
                }
            }
            // write it back
            Vec_PtrWriteEntry( vNodes, k++, pObj );
        }
        Vec_PtrShrink( vNodes, k );
    } 
    while ( k < i );

//    if ( Vec_PtrSize(vNodes) > 0 )
//        printf( "Found %d unreachable.\n", Vec_PtrSize(vNodes) ); 
    return vNodes;

/*
    // the resulting array contains all unreachable nodes except const 1
    if ( Vec_PtrSize(vNodes) == 0 )
    {
        Vec_PtrFree( vNodes );
        return 0;
    }
    RetValue = Vec_PtrSize(vNodes);

    // mark these nodes
    Aig_ManIncrementTravId( p ); 
    Vec_PtrForEachEntry( vNodes, pObj, i )
        Aig_ObjSetTravIdCurrent( p, pObj );
    Vec_PtrFree( vNodes );
    return RetValue;
*/
}


/**Function*************************************************************

  Synopsis    [Removes nodes that do not fanout into POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManRemoveUnmarked( Aig_Man_t * p )
{
    Vec_Ptr_t * vNodes;
    Aig_Obj_t * pObj;
    int i, RetValue;
    // collect unmarked nodes
    vNodes = Vec_PtrAlloc( 100 );
    Aig_ManForEachObj( p, pObj, i )
    {
        if ( Aig_ObjIsTerm(pObj) )
            continue;
        if ( Aig_ObjIsTravIdCurrent(p, pObj) )
            continue;
//Aig_ObjPrintVerbose( pObj, 0 );
        Aig_ObjDisconnect( p, pObj );
        Vec_PtrPush( vNodes, pObj );
    }
    if ( Vec_PtrSize(vNodes) == 0 )
    {
        Vec_PtrFree( vNodes );
        return 0;
    }
    // remove the dangling objects
    RetValue = Vec_PtrSize(vNodes);
    Vec_PtrForEachEntry( vNodes, pObj, i )
        Aig_ObjDelete( p, pObj );
//    printf( "Removed %d dangling.\n", Vec_PtrSize(vNodes) ); 
    Vec_PtrFree( vNodes );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Rehashes the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManSeqRehashOne( Aig_Man_t * p, Vec_Ptr_t * vNodes, Vec_Ptr_t * vUnreach )
{
    Aig_Obj_t * pObj, * pObjNew, * pFanin0, * pFanin1;
    int i, RetValue = 0, Counter = 0, Counter2 = 0;

    // mark the unreachable nodes
    Aig_ManIncrementTravId( p );
    Vec_PtrForEachEntry( vUnreach, pObj, i )
        Aig_ObjSetTravIdCurrent(p, pObj);
/*
    // count the number of unreachable object connections
    // that is the number of unreachable objects connected to main objects
    Aig_ManForEachObj( p, pObj, i )
    {
        if ( Aig_ObjIsTravIdCurrent(p, pObj) )
            continue;

        pFanin0 = Aig_ObjFanin0(pObj);
        if ( pFanin0 == NULL )
            continue;
        if ( Aig_ObjIsTravIdCurrent(p, pFanin0) )
            pFanin0->fMarkA = 1;

        pFanin1 = Aig_ObjFanin1(pObj);
        if ( pFanin1 == NULL )
            continue;
        if ( Aig_ObjIsTravIdCurrent(p, pFanin1) )
            pFanin1->fMarkA = 1;
    }

    // count the objects
    Aig_ManForEachObj( p, pObj, i )
        Counter2 += pObj->fMarkA, pObj->fMarkA = 0;
    printf( "Connections = %d.\n", Counter2 );
*/

    // go through the nodes while skipping unreachable
    Vec_PtrForEachEntry( vNodes, pObj, i )
    {
        // skip nodes unreachable from the PIs
        if ( Aig_ObjIsTravIdCurrent(p, pObj) )
            continue;
        // process the node
        if ( Aig_ObjIsPo(pObj) )
        {
            if ( !Aig_ObjIsBuf(Aig_ObjFanin0(pObj)) )
                continue;
            pFanin0 = Aig_ObjReal_rec( Aig_ObjChild0(pObj) );
            Aig_ObjPatchFanin0( p, pObj, pFanin0 );
            continue;
        }
        if ( Aig_ObjIsLatch(pObj) )
        {
            if ( !Aig_ObjIsBuf(Aig_ObjFanin0(pObj)) )
                continue;
            pObjNew = Aig_ObjReal_rec( Aig_ObjChild0(pObj) );
            pObjNew = Aig_Latch( p, pObjNew, 0 );
            Aig_ObjReplace( p, pObj, pObjNew, 1, 0 );
            RetValue = 1;
            Counter++;
            continue;
        }
        if ( Aig_ObjIsNode(pObj) )
        {
            if ( !Aig_ObjIsBuf(Aig_ObjFanin0(pObj)) && !Aig_ObjIsBuf(Aig_ObjFanin1(pObj)) )
                continue;
            pFanin0 = Aig_ObjReal_rec( Aig_ObjChild0(pObj) );
            pFanin1 = Aig_ObjReal_rec( Aig_ObjChild1(pObj) );
            pObjNew = Aig_And( p, pFanin0, pFanin1 );
            Aig_ObjReplace( p, pObj, pObjNew, 1, 0 );
            RetValue = 1;
            Counter++;
            continue;
        }
    }
//    printf( "Rehashings = %d.\n", Counter++ );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [If AIG contains buffers, this procedure removes them.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManRemoveBuffers( Aig_Man_t * p )
{
    Aig_Obj_t * pObj, * pObjNew, * pFanin0, * pFanin1;
    int i;
    if ( Aig_ManBufNum(p) == 0 )
        return;
    Aig_ManForEachObj( p, pObj, i )
    {
        if ( Aig_ObjIsPo(pObj) )
        {
            if ( !Aig_ObjIsBuf(Aig_ObjFanin0(pObj)) )
                continue;
            pFanin0 = Aig_ObjReal_rec( Aig_ObjChild0(pObj) );
            Aig_ObjPatchFanin0( p, pObj, pFanin0 );
        }
        else if ( Aig_ObjIsLatch(pObj) )
        {
            if ( !Aig_ObjIsBuf(Aig_ObjFanin0(pObj)) )
                continue;
            pFanin0 = Aig_ObjReal_rec( Aig_ObjChild0(pObj) );
            pObjNew = Aig_Latch( p, pFanin0, 0 );
            Aig_ObjReplace( p, pObj, pObjNew, 0, 0 );
        }
        else if ( Aig_ObjIsAnd(pObj) )
        {
            if ( !Aig_ObjIsBuf(Aig_ObjFanin0(pObj)) && !Aig_ObjIsBuf(Aig_ObjFanin1(pObj)) )
                continue;
            pFanin0 = Aig_ObjReal_rec( Aig_ObjChild0(pObj) );
            pFanin1 = Aig_ObjReal_rec( Aig_ObjChild1(pObj) );
            pObjNew = Aig_And( p, pFanin0, pFanin1 );
            Aig_ObjReplace( p, pObj, pObjNew, 0, 0 );
        }
    }
    assert( Aig_ManBufNum(p) == 0 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManSeqStrash( Aig_Man_t * p, int nLatches, int * pInits )
{
    Vec_Ptr_t * vNodes, * vUnreach;
//    Aig_Obj_t * pObj, * pFanin;
//    int i;
    int Iter, RetValue = 1;

    // create latches out of the additional PI/PO pairs
    Aig_ManSeqStrashConvert( p, nLatches, pInits );

    // iteratively rehash the network
    for ( Iter = 0; RetValue; Iter++ )
    {
//        Aig_ManPrintStats( p );
/*
        Aig_ManForEachObj( p, pObj, i )
        {
            assert( pObj->Type > 0 );
            pFanin = Aig_ObjFanin0(pObj);
            assert( pFanin == NULL || pFanin->Type > 0 );
            pFanin = Aig_ObjFanin1(pObj);
            assert( pFanin == NULL || pFanin->Type > 0 );
        }
*/
        // mark nodes unreachable from the PIs
        vUnreach = Aig_ManDfsUnreach( p );
        if ( Iter == 0 && Vec_PtrSize(vUnreach) > 0 )
            printf( "Unreachable objects = %d.\n", Vec_PtrSize(vUnreach) );
        // collect nodes reachable from the POs
        vNodes = Aig_ManDfsSeq( p );
        // remove nodes unreachable from the POs
        if ( Iter == 0 )
            Aig_ManRemoveUnmarked( p );
        // continue rehashing as long as there are changes
        RetValue = Aig_ManSeqRehashOne( p, vNodes, vUnreach );
        Vec_PtrFree( vNodes );
        Vec_PtrFree( vUnreach );
    }

    // perform the final cleanup
    Aig_ManIncrementTravId( p );
    vNodes = Aig_ManDfsSeq( p );
    Aig_ManRemoveUnmarked( p );
    Vec_PtrFree( vNodes );
    // remove buffers if they are left
//    Aig_ManRemoveBuffers( p );

    // clean up
    if ( !Aig_ManCheck( p ) )
    {
        printf( "Aig_ManSeqStrash: The network check has failed.\n" );
        return 0;
    }
    return 1;

}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


