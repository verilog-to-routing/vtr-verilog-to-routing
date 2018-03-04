/**CFile****************************************************************

  FileName    [ivyDfs.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [And-Inverter Graph package.]

  Synopsis    [DFS collection procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 11, 2006.]

  Revision    [$Id: ivyDfs.c,v 1.00 2006/05/11 00:00:00 alanmi Exp $]

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

  Synopsis    [Collects nodes in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ManDfs_rec( Ivy_Man_t * p, Ivy_Obj_t * pObj, Vec_Int_t * vNodes )
{
    if ( Ivy_ObjIsMarkA(pObj) )
        return;
    Ivy_ObjSetMarkA(pObj);
    if ( Ivy_ObjIsConst1(pObj) || Ivy_ObjIsCi(pObj) )
    {
        if ( p->pHaig == NULL && pObj->pEquiv )
            Ivy_ManDfs_rec( p, Ivy_Regular(pObj->pEquiv), vNodes );
        return;
    }
//printf( "visiting node %d\n", pObj->Id );
/*
    if ( pObj->Id == 87 || pObj->Id == 90 )
    {
        int y = 0;
    }
*/
    assert( Ivy_ObjIsBuf(pObj) || Ivy_ObjIsAnd(pObj) || Ivy_ObjIsExor(pObj) );
    Ivy_ManDfs_rec( p, Ivy_ObjFanin0(pObj), vNodes );
    if ( !Ivy_ObjIsBuf(pObj) )
        Ivy_ManDfs_rec( p, Ivy_ObjFanin1(pObj), vNodes );
    if ( p->pHaig == NULL && pObj->pEquiv )
        Ivy_ManDfs_rec( p, Ivy_Regular(pObj->pEquiv), vNodes );
    Vec_IntPush( vNodes, pObj->Id );

//printf( "adding node %d with fanins %d and %d and equiv %d (refs = %d)\n", 
//       pObj->Id, Ivy_ObjFanin0(pObj)->Id, Ivy_ObjFanin1(pObj)->Id, 
//       pObj->pEquiv? Ivy_Regular(pObj->pEquiv)->Id: -1, Ivy_ObjRefs(pObj) );
}

/**Function*************************************************************

  Synopsis    [Collects AND/EXOR nodes in the DFS order from CIs to COs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Ivy_ManDfs( Ivy_Man_t * p )
{
    Vec_Int_t * vNodes;
    Ivy_Obj_t * pObj;
    int i;
    assert( Ivy_ManLatchNum(p) == 0 );
    // make sure the nodes are not marked
    Ivy_ManForEachObj( p, pObj, i )
        assert( !pObj->fMarkA && !pObj->fMarkB );
    // collect the nodes
    vNodes = Vec_IntAlloc( Ivy_ManNodeNum(p) );
    Ivy_ManForEachPo( p, pObj, i )
        Ivy_ManDfs_rec( p, Ivy_ObjFanin0(pObj), vNodes );
    // unmark the collected nodes
//    Ivy_ManForEachNodeVec( p, vNodes, pObj, i )
//        Ivy_ObjClearMarkA(pObj);
    Ivy_ManForEachObj( p, pObj, i )
        Ivy_ObjClearMarkA(pObj);
    // make sure network does not have dangling nodes
    assert( Vec_IntSize(vNodes) == Ivy_ManNodeNum(p) + Ivy_ManBufNum(p) );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Collects AND/EXOR nodes in the DFS order from CIs to COs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Ivy_ManDfsSeq( Ivy_Man_t * p, Vec_Int_t ** pvLatches )
{
    Vec_Int_t * vNodes, * vLatches;
    Ivy_Obj_t * pObj;
    int i;
//    assert( Ivy_ManLatchNum(p) > 0 );
    // make sure the nodes are not marked
    Ivy_ManForEachObj( p, pObj, i )
        assert( !pObj->fMarkA && !pObj->fMarkB );
    // collect the latches
    vLatches = Vec_IntAlloc( Ivy_ManLatchNum(p) );
    Ivy_ManForEachLatch( p, pObj, i )
        Vec_IntPush( vLatches, pObj->Id );
    // collect the nodes
    vNodes = Vec_IntAlloc( Ivy_ManNodeNum(p) );
    Ivy_ManForEachPo( p, pObj, i )
        Ivy_ManDfs_rec( p, Ivy_ObjFanin0(pObj), vNodes );
    Ivy_ManForEachNodeVec( p, vLatches, pObj, i )
        Ivy_ManDfs_rec( p, Ivy_ObjFanin0(pObj), vNodes );
    // unmark the collected nodes
//    Ivy_ManForEachNodeVec( p, vNodes, pObj, i )
//        Ivy_ObjClearMarkA(pObj);
    Ivy_ManForEachObj( p, pObj, i )
        Ivy_ObjClearMarkA(pObj);
    // make sure network does not have dangling nodes
//    assert( Vec_IntSize(vNodes) == Ivy_ManNodeNum(p) + Ivy_ManBufNum(p) );

// temporary!!!

    if ( pvLatches == NULL )
        Vec_IntFree( vLatches );
    else
        *pvLatches = vLatches;
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Collects nodes in the cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ManCollectCone_rec( Ivy_Obj_t * pObj, Vec_Ptr_t * vCone )
{
    if ( pObj->fMarkA )
        return;
    if ( Ivy_ObjIsBuf(pObj) )
    {
        Ivy_ManCollectCone_rec( Ivy_ObjFanin0(pObj), vCone );
        Vec_PtrPush( vCone, pObj );
        return;
    }
    assert( Ivy_ObjIsNode(pObj) );
    Ivy_ManCollectCone_rec( Ivy_ObjFanin0(pObj), vCone );
    Ivy_ManCollectCone_rec( Ivy_ObjFanin1(pObj), vCone );
    Vec_PtrPushUnique( vCone, pObj );
}

/**Function*************************************************************

  Synopsis    [Collects nodes in the cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ManCollectCone( Ivy_Obj_t * pObj, Vec_Ptr_t * vFront, Vec_Ptr_t * vCone )
{
    Ivy_Obj_t * pTemp;
    int i;
    assert( !Ivy_IsComplement(pObj) );
    assert( Ivy_ObjIsNode(pObj) );
    // mark the nodes
    Vec_PtrForEachEntry( Ivy_Obj_t *, vFront, pTemp, i )
        Ivy_Regular(pTemp)->fMarkA = 1;
    assert( pObj->fMarkA == 0 );
    // collect the cone
    Vec_PtrClear( vCone );
    Ivy_ManCollectCone_rec( pObj, vCone );
    // unmark the nodes
    Vec_PtrForEachEntry( Ivy_Obj_t *, vFront, pTemp, i )
        Ivy_Regular(pTemp)->fMarkA = 0;
}

/**Function*************************************************************

  Synopsis    [Returns the nodes by level.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Vec_t * Ivy_ManLevelize( Ivy_Man_t * p )
{
    Vec_Vec_t * vNodes;
    Ivy_Obj_t * pObj;
    int i;
    vNodes = Vec_VecAlloc( 100 );
    Ivy_ManForEachObj( p, pObj, i )
    {
        assert( !Ivy_ObjIsBuf(pObj) );
        if ( Ivy_ObjIsNode(pObj) )
            Vec_VecPush( vNodes, pObj->Level, pObj );
    }
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Computes required levels for each node.]

  Description [Assumes topological ordering of the nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Ivy_ManRequiredLevels( Ivy_Man_t * p )
{
    Ivy_Obj_t * pObj;
    Vec_Int_t * vLevelsR;
    Vec_Vec_t * vNodes;
    int i, k, Level, LevelMax;
    assert( p->vRequired == NULL );
    // start the required times
    vLevelsR = Vec_IntStart( Ivy_ManObjIdMax(p) + 1 );
    // iterate through the nodes in the reverse order
    vNodes = Ivy_ManLevelize( p );
    Vec_VecForEachEntryReverseReverse( Ivy_Obj_t *, vNodes, pObj, i, k )
    {
        Level = Vec_IntEntry( vLevelsR, pObj->Id ) + 1 + Ivy_ObjIsExor(pObj);
        if ( Vec_IntEntry( vLevelsR, Ivy_ObjFaninId0(pObj) ) < Level )
            Vec_IntWriteEntry( vLevelsR, Ivy_ObjFaninId0(pObj), Level );
        if ( Vec_IntEntry( vLevelsR, Ivy_ObjFaninId1(pObj) ) < Level )
            Vec_IntWriteEntry( vLevelsR, Ivy_ObjFaninId1(pObj), Level );
    }
    Vec_VecFree( vNodes );
    // convert it into the required times
    LevelMax = Ivy_ManLevels( p );
//printf( "max %5d\n",LevelMax );
    Ivy_ManForEachObj( p, pObj, i )
    {
        Level = Vec_IntEntry( vLevelsR, pObj->Id );
        Vec_IntWriteEntry( vLevelsR, pObj->Id, LevelMax - Level );
//printf( "%5d : %5d %5d\n", pObj->Id, Level, LevelMax - Level );
    }
    p->vRequired = vLevelsR;
    return vLevelsR;
}

/**Function*************************************************************

  Synopsis    [Recursively detects combinational loops.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_ManIsAcyclic_rec( Ivy_Man_t * p, Ivy_Obj_t * pObj )
{
    // skip the node if it is already visited
    if ( Ivy_ObjIsTravIdPrevious(p, pObj) )
        return 1;
    // check if the node is part of the combinational loop
    if ( Ivy_ObjIsTravIdCurrent(p, pObj) )
    {
        fprintf( stdout, "Manager contains combinational loop!\n" );
        fprintf( stdout, "Node \"%d\" is encountered twice on the following path:\n",  Ivy_ObjId(pObj) );
        fprintf( stdout, " %d",  Ivy_ObjId(pObj) );
        return 0;
    }
    // mark this node as a node on the current path
    Ivy_ObjSetTravIdCurrent( p, pObj );
    // explore equivalent nodes if pObj is the main node
    if ( p->pHaig == NULL && pObj->pEquiv && Ivy_ObjRefs(pObj) > 0 )
    {
        Ivy_Obj_t * pTemp;
        assert( !Ivy_IsComplement(pObj->pEquiv) );
        for ( pTemp = pObj->pEquiv; pTemp != pObj; pTemp = Ivy_Regular(pTemp->pEquiv) )
        {
            // traverse the fanin's cone searching for the loop
            if ( !Ivy_ManIsAcyclic_rec(p, pTemp) )
            {
                // return as soon as the loop is detected
                fprintf( stdout, " -> (%d", Ivy_ObjId(pObj) );
                for ( pTemp = pObj->pEquiv; pTemp != pObj; pTemp = Ivy_Regular(pTemp->pEquiv) )
                    fprintf( stdout, " %d", Ivy_ObjId(pTemp) );
                fprintf( stdout, ")" );
                return 0; 
            }
        }
    }
    // quite if it is a CI node
    if ( Ivy_ObjIsCi(pObj) || Ivy_ObjIsConst1(pObj) )
    {
        // mark this node as a visited node
        Ivy_ObjSetTravIdPrevious( p, pObj );
        return 1;
    }
    assert( Ivy_ObjIsNode(pObj) || Ivy_ObjIsBuf(pObj) );
    // traverse the fanin's cone searching for the loop
    if ( !Ivy_ManIsAcyclic_rec(p, Ivy_ObjFanin0(pObj)) )
    {
        // return as soon as the loop is detected
        fprintf( stdout, " -> %d", Ivy_ObjId(pObj) );
        return 0;
    }
    // traverse the fanin's cone searching for the loop
    if ( Ivy_ObjIsNode(pObj) && !Ivy_ManIsAcyclic_rec(p, Ivy_ObjFanin1(pObj)) )
    {
        // return as soon as the loop is detected
        fprintf( stdout, " -> %d", Ivy_ObjId(pObj) );
        return 0;
    }
    // mark this node as a visited node
    Ivy_ObjSetTravIdPrevious( p, pObj );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Detects combinational loops.]

  Description [This procedure is based on the idea suggested by Donald Chai. 
  As we traverse the network and visit the nodes, we need to distinquish 
  three types of nodes: (1) those that are visited for the first time, 
  (2) those that have been visited in this traversal but are currently not 
  on the traversal path, (3) those that have been visited and are currently 
  on the travesal path. When the node of type (3) is encountered, it means 
  that there is a combinational loop. To mark the three types of nodes, 
  two new values of the traversal IDs are used.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_ManIsAcyclic( Ivy_Man_t * p )
{
    Ivy_Obj_t * pObj;
    int fAcyclic, i;
    // set the traversal ID for this DFS ordering
    Ivy_ManIncrementTravId( p );   
    Ivy_ManIncrementTravId( p );   
    // pObj->TravId == pNet->nTravIds      means "pObj is on the path"
    // pObj->TravId == pNet->nTravIds - 1  means "pObj is visited but is not on the path"
    // pObj->TravId <  pNet->nTravIds - 1  means "pObj is not visited"
    // traverse the network to detect cycles
    fAcyclic = 1;
    Ivy_ManForEachCo( p, pObj, i )
    {
        // traverse the output logic cone
        if ( (fAcyclic = Ivy_ManIsAcyclic_rec(p, Ivy_ObjFanin0(pObj))) )
            continue;
        // stop as soon as the first loop is detected
        fprintf( stdout, " (cone of %s \"%d\")\n", Ivy_ObjIsLatch(pObj)? "latch" : "PO", Ivy_ObjId(pObj) );
        break;
    }
    return fAcyclic;
}

/**Function*************************************************************

  Synopsis    [Sets the levels of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_ManSetLevels_rec( Ivy_Obj_t * pObj, int fHaig )
{
    // quit if the node is visited
    if ( Ivy_ObjIsMarkA(pObj) )
        return pObj->Level;
    Ivy_ObjSetMarkA(pObj);
    // quit if this is a CI
    if ( Ivy_ObjIsConst1(pObj) || Ivy_ObjIsCi(pObj) )
        return 0;
    assert( Ivy_ObjIsBuf(pObj) || Ivy_ObjIsAnd(pObj) || Ivy_ObjIsExor(pObj) );
    // get levels of the fanins
    Ivy_ManSetLevels_rec( Ivy_ObjFanin0(pObj), fHaig );
    if ( !Ivy_ObjIsBuf(pObj) )
        Ivy_ManSetLevels_rec( Ivy_ObjFanin1(pObj), fHaig );
    // get level of the node
    if ( Ivy_ObjIsBuf(pObj) )
        pObj->Level = 1 + Ivy_ObjFanin0(pObj)->Level;
    else if ( Ivy_ObjIsNode(pObj) )
        pObj->Level = Ivy_ObjLevelNew( pObj );
    else assert( 0 );
    // get level of other choices
    if ( fHaig && pObj->pEquiv && Ivy_ObjRefs(pObj) > 0 )
    {
        Ivy_Obj_t * pTemp;
        unsigned LevelMax = pObj->Level;
        for ( pTemp = pObj->pEquiv; pTemp != pObj; pTemp = Ivy_Regular(pTemp->pEquiv) )
        {
            Ivy_ManSetLevels_rec( pTemp, fHaig );
            LevelMax = IVY_MAX( LevelMax, pTemp->Level );
        }
        // get this level
        pObj->Level = LevelMax;
        for ( pTemp = pObj->pEquiv; pTemp != pObj; pTemp = Ivy_Regular(pTemp->pEquiv) )
            pTemp->Level = LevelMax;
    }
    return pObj->Level;
}

/**Function*************************************************************

  Synopsis    [Sets the levels of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_ManSetLevels( Ivy_Man_t * p, int fHaig )
{
    Ivy_Obj_t * pObj;
    int i, LevelMax;
    // check if CIs have choices
    if ( fHaig )
    {
        Ivy_ManForEachCi( p, pObj, i )
            if ( pObj->pEquiv )
                printf( "CI %d has a choice, which will not be visualized.\n", pObj->Id );
    }
    // clean the levels
    Ivy_ManForEachObj( p, pObj, i )
        pObj->Level = 0;
    // compute the levels
    LevelMax = 0;
    Ivy_ManForEachCo( p, pObj, i )
    {
        Ivy_ManSetLevels_rec( Ivy_ObjFanin0(pObj), fHaig );
        LevelMax = IVY_MAX( LevelMax, (int)Ivy_ObjFanin0(pObj)->Level );
    }
    // compute levels of nodes without fanout
    Ivy_ManForEachObj( p, pObj, i )
        if ( (Ivy_ObjIsNode(pObj) || Ivy_ObjIsBuf(pObj)) && Ivy_ObjRefs(pObj) == 0 )
        {
            Ivy_ManSetLevels_rec( pObj, fHaig );
            LevelMax = IVY_MAX( LevelMax, (int)pObj->Level );
        }
    // clean the marks
    Ivy_ManForEachObj( p, pObj, i )
        Ivy_ObjClearMarkA(pObj);
    return LevelMax;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

