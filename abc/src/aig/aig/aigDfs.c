/**CFile****************************************************************

  FileName    [aigDfs.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [DFS traversal procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aigDfs.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig.h"
#include "misc/tim/tim.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Verifies that the objects are in a topo order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManVerifyTopoOrder( Aig_Man_t * p )
{
    Aig_Obj_t * pObj, * pNext;
    int i, k, iBox, iTerm1, nTerms;
    Aig_ManSetCioIds( p );
    Aig_ManIncrementTravId( p );
    Aig_ManForEachObj( p, pObj, i )
    {
        if ( Aig_ObjIsNode(pObj) )
        {
            pNext = Aig_ObjFanin0(pObj);
            if ( !Aig_ObjIsTravIdCurrent(p,pNext) )
            {
                printf( "Node %d has fanin %d that is not in a topological order.\n", pObj->Id, pNext->Id );
                return 0;
            }
            pNext = Aig_ObjFanin1(pObj);
            if ( !Aig_ObjIsTravIdCurrent(p,pNext) )
            {
                printf( "Node %d has fanin %d that is not in a topological order.\n", pObj->Id, pNext->Id );
                return 0;
            }
        }
        else if ( Aig_ObjIsCo(pObj) || Aig_ObjIsBuf(pObj) )
        {
            pNext = Aig_ObjFanin0(pObj);
            if ( !Aig_ObjIsTravIdCurrent(p,pNext) )
            {
                printf( "Node %d has fanin %d that is not in a topological order.\n", pObj->Id, pNext->Id );
                return 0;
            }
        }
        else if ( Aig_ObjIsCi(pObj) )
        {
            if ( p->pManTime )
            {
                iBox = Tim_ManBoxForCi( (Tim_Man_t *)p->pManTime, Aig_ObjCioId(pObj) );
                if ( iBox >= 0 ) // this is not a true PI
                {
                    iTerm1 = Tim_ManBoxInputFirst( (Tim_Man_t *)p->pManTime, iBox );
                    nTerms = Tim_ManBoxInputNum( (Tim_Man_t *)p->pManTime, iBox );
                    for ( k = 0; k < nTerms; k++ )
                    {
                        pNext = Aig_ManCo( p, iTerm1 + k );
                        assert( Tim_ManBoxForCo( (Tim_Man_t *)p->pManTime, Aig_ObjCioId(pNext) ) == iBox ); 
                        if ( !Aig_ObjIsTravIdCurrent(p,pNext) )
                        {
                            printf( "Box %d has input %d that is not in a topological order.\n", iBox, pNext->Id );
                            return 0;
                        }
                    }
                }
            }
        }
        else if ( !Aig_ObjIsConst1(pObj) )
            assert( 0 );
        Aig_ObjSetTravIdCurrent( p, pObj );
    }
    Aig_ManCleanCioIds( p );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Collects internal nodes in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManDfs_rec( Aig_Man_t * p, Aig_Obj_t * pObj, Vec_Ptr_t * vNodes )
{
    if ( pObj == NULL )
        return;
    assert( !Aig_IsComplement(pObj) );
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(p, pObj);
    if ( p->pEquivs && Aig_ObjEquiv(p, pObj) )
        Aig_ManDfs_rec( p, Aig_ObjEquiv(p, pObj), vNodes );
    Aig_ManDfs_rec( p, Aig_ObjFanin0(pObj), vNodes );
    Aig_ManDfs_rec( p, Aig_ObjFanin1(pObj), vNodes );
    Vec_PtrPush( vNodes, pObj );
}

/**Function*************************************************************

  Synopsis    [Collects objects of the AIG in the DFS order.]

  Description [Works with choice nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManDfs( Aig_Man_t * p, int fNodesOnly )
{
    Vec_Ptr_t * vNodes;
    Aig_Obj_t * pObj;
    int i;
    Aig_ManIncrementTravId( p );
    Aig_ObjSetTravIdCurrent( p, Aig_ManConst1(p) );
    // start the array of nodes
    vNodes = Vec_PtrAlloc( Aig_ManObjNumMax(p) );
    // mark PIs if they should not be collected
    if ( fNodesOnly )
        Aig_ManForEachCi( p, pObj, i )
            Aig_ObjSetTravIdCurrent( p, pObj );
    else
        Vec_PtrPush( vNodes, Aig_ManConst1(p) );
    // collect nodes reachable in the DFS order
    Aig_ManForEachCo( p, pObj, i )
        Aig_ManDfs_rec( p, fNodesOnly? Aig_ObjFanin0(pObj): pObj, vNodes );
    if ( fNodesOnly )
        assert( Vec_PtrSize(vNodes) == Aig_ManNodeNum(p) );
    else
        assert( Vec_PtrSize(vNodes) == Aig_ManObjNum(p) );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Collects internal nodes in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManDfsAll_rec( Aig_Man_t * p, Aig_Obj_t * pObj, Vec_Ptr_t * vNodes )
{
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(p, pObj);
    if ( Aig_ObjIsCi(pObj) )
    {
        Vec_PtrPush( vNodes, pObj );
        return;
    }
    if ( Aig_ObjIsCo(pObj) )
    {
        Aig_ManDfsAll_rec( p, Aig_ObjFanin0(pObj), vNodes );
        Vec_PtrPush( vNodes, pObj );
        return;
    }
    assert( Aig_ObjIsNode(pObj) );
    Aig_ManDfsAll_rec( p, Aig_ObjFanin0(pObj), vNodes );
    Aig_ManDfsAll_rec( p, Aig_ObjFanin1(pObj), vNodes );
    Vec_PtrPush( vNodes, pObj );
}
Vec_Ptr_t * Aig_ManDfsArray( Aig_Man_t * p, Aig_Obj_t ** pNodes, int nNodes )
{
    Vec_Ptr_t * vNodes;
    int i;
    Aig_ManIncrementTravId( p );
    vNodes = Vec_PtrAlloc( Aig_ManObjNumMax(p) );
    // add constant
    Aig_ObjSetTravIdCurrent( p, Aig_ManConst1(p) );
    Vec_PtrPush( vNodes, Aig_ManConst1(p) );
    // collect nodes reachable in the DFS order
    for ( i = 0; i < nNodes; i++ )
        Aig_ManDfsAll_rec( p, pNodes[i], vNodes );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Collects objects of the AIG in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManDfsAll( Aig_Man_t * p )
{
    Vec_Ptr_t * vNodes;
    Aig_Obj_t * pObj;
    int i;
    Aig_ManIncrementTravId( p );
    vNodes = Vec_PtrAlloc( Aig_ManObjNumMax(p) );
    // add constant
    Aig_ObjSetTravIdCurrent( p, Aig_ManConst1(p) );
    Vec_PtrPush( vNodes, Aig_ManConst1(p) );
    // collect nodes reachable in the DFS order
    Aig_ManForEachCo( p, pObj, i )
        Aig_ManDfsAll_rec( p, pObj, vNodes );
    Aig_ManForEachCi( p, pObj, i )
        if ( !Aig_ObjIsTravIdCurrent(p, pObj) )
            Vec_PtrPush( vNodes, pObj );
    assert( Vec_PtrSize(vNodes) == Aig_ManObjNum(p) );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Collects internal nodes in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManDfsPreorder_rec( Aig_Man_t * p, Aig_Obj_t * pObj, Vec_Ptr_t * vNodes )
{
    if ( pObj == NULL )
        return;
    assert( !Aig_IsComplement(pObj) );
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(p, pObj);
    Vec_PtrPush( vNodes, pObj );
    if ( p->pEquivs && Aig_ObjEquiv(p, pObj) )
        Aig_ManDfs_rec( p, Aig_ObjEquiv(p, pObj), vNodes );
    Aig_ManDfsPreorder_rec( p, Aig_ObjFanin0(pObj), vNodes );
    Aig_ManDfsPreorder_rec( p, Aig_ObjFanin1(pObj), vNodes );
}

/**Function*************************************************************

  Synopsis    [Collects objects of the AIG in the DFS order.]

  Description [Works with choice nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManDfsPreorder( Aig_Man_t * p, int fNodesOnly )
{
    Vec_Ptr_t * vNodes;
    Aig_Obj_t * pObj;
    int i;
    Aig_ManIncrementTravId( p );
    Aig_ObjSetTravIdCurrent( p, Aig_ManConst1(p) );
    // start the array of nodes
    vNodes = Vec_PtrAlloc( Aig_ManObjNumMax(p) );
    // mark PIs if they should not be collected
    if ( fNodesOnly )
        Aig_ManForEachCi( p, pObj, i )
            Aig_ObjSetTravIdCurrent( p, pObj );
    else
        Vec_PtrPush( vNodes, Aig_ManConst1(p) );
    // collect nodes reachable in the DFS order
    Aig_ManForEachCo( p, pObj, i )
        Aig_ManDfsPreorder_rec( p, fNodesOnly? Aig_ObjFanin0(pObj): pObj, vNodes );
    if ( fNodesOnly )
        assert( Vec_PtrSize(vNodes) == Aig_ManNodeNum(p) );
    else
        assert( Vec_PtrSize(vNodes) == Aig_ManObjNum(p) );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Levelizes the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Vec_t * Aig_ManLevelize( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    Vec_Vec_t * vLevels;
    int nLevels, i;
    nLevels = Aig_ManLevelNum( p );
    vLevels = Vec_VecStart( nLevels + 1 );
    Aig_ManForEachObj( p, pObj, i )
    {
        assert( (int)pObj->Level <= nLevels );
        Vec_VecPush( vLevels, pObj->Level, pObj );
    }
    return vLevels;
}

/**Function*************************************************************

  Synopsis    [Collects internal nodes and PIs in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManDfsNodes( Aig_Man_t * p, Aig_Obj_t ** ppNodes, int nNodes )
{
    Vec_Ptr_t * vNodes;
//    Aig_Obj_t * pObj;
    int i;
    Aig_ManIncrementTravId( p );
    // mark constant and PIs
    Aig_ObjSetTravIdCurrent( p, Aig_ManConst1(p) );
//    Aig_ManForEachCi( p, pObj, i )
//        Aig_ObjSetTravIdCurrent( p, pObj );
    // go through the nodes
    vNodes = Vec_PtrAlloc( Aig_ManNodeNum(p) );
    for ( i = 0; i < nNodes; i++ )
        if ( Aig_ObjIsCo(ppNodes[i]) )
            Aig_ManDfs_rec( p, Aig_ObjFanin0(ppNodes[i]), vNodes );
        else
            Aig_ManDfs_rec( p, ppNodes[i], vNodes );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Collects internal nodes in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManDfsChoices_rec( Aig_Man_t * p, Aig_Obj_t * pObj, Vec_Ptr_t * vNodes )
{
    if ( pObj == NULL )
        return;
    assert( !Aig_IsComplement(pObj) );
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return;
    assert( Aig_ObjIsNode(pObj) );
    Aig_ManDfsChoices_rec( p, Aig_ObjFanin0(pObj), vNodes );
    Aig_ManDfsChoices_rec( p, Aig_ObjFanin1(pObj), vNodes );
    Aig_ManDfsChoices_rec( p, Aig_ObjEquiv(p, pObj), vNodes );
    assert( !Aig_ObjIsTravIdCurrent(p, pObj) ); // loop detection
    Aig_ObjSetTravIdCurrent(p, pObj);
    Vec_PtrPush( vNodes, pObj );
}

/**Function*************************************************************

  Synopsis    [Collects internal nodes in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManDfsChoices( Aig_Man_t * p )
{
    Vec_Ptr_t * vNodes;
    Aig_Obj_t * pObj;
    int i, Counter = 0;

    Aig_ManForEachNode( p, pObj, i )
    {
        if ( Aig_ObjEquiv(p, pObj) == NULL )
            continue;
        Counter = 0;
        for ( pObj = Aig_ObjEquiv(p, pObj) ; pObj; pObj = Aig_ObjEquiv(p, pObj) )
            Counter++;
//        printf( "%d ", Counter );
    }
//    printf( "\n" );

    assert( p->pEquivs != NULL );
    Aig_ManIncrementTravId( p );
    // mark constant and PIs
    Aig_ObjSetTravIdCurrent( p, Aig_ManConst1(p) );
    Aig_ManForEachCi( p, pObj, i )
        Aig_ObjSetTravIdCurrent( p, pObj );
    // go through the nodes
    vNodes = Vec_PtrAlloc( Aig_ManNodeNum(p) );
    Aig_ManForEachCo( p, pObj, i )
        Aig_ManDfsChoices_rec( p, Aig_ObjFanin0(pObj), vNodes );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Collects internal nodes in the reverse DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManDfsReverse_rec( Aig_Man_t * p, Aig_Obj_t * pObj, Vec_Ptr_t * vNodes )
{
    Aig_Obj_t * pFanout;
    int iFanout = -1, i;
    assert( !Aig_IsComplement(pObj) );
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return;
    assert( Aig_ObjIsNode(pObj) || Aig_ObjIsBuf(pObj) );
    Aig_ObjForEachFanout( p, pObj, pFanout, iFanout, i )
        Aig_ManDfsReverse_rec( p, pFanout, vNodes );
    assert( !Aig_ObjIsTravIdCurrent(p, pObj) ); // loop detection
    Aig_ObjSetTravIdCurrent(p, pObj);
    Vec_PtrPush( vNodes, pObj );
}

/**Function*************************************************************

  Synopsis    [Collects internal nodes in the reverse DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManDfsReverse( Aig_Man_t * p )
{
    Vec_Ptr_t * vNodes;
    Aig_Obj_t * pObj;
    int i;
    Aig_ManIncrementTravId( p );
    // mark POs
    Aig_ManForEachCo( p, pObj, i )
        Aig_ObjSetTravIdCurrent( p, pObj );
    // go through the nodes
    vNodes = Vec_PtrAlloc( Aig_ManNodeNum(p) );
    Aig_ManForEachObj( p, pObj, i )
        if ( Aig_ObjIsNode(pObj) || Aig_ObjIsBuf(pObj) )
            Aig_ManDfsReverse_rec( p, pObj, vNodes );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Computes the max number of levels in the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManLevelNum( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i, LevelsMax;
    LevelsMax = 0;
    Aig_ManForEachCo( p, pObj, i )
        LevelsMax = Abc_MaxInt( LevelsMax, (int)Aig_ObjFanin0(pObj)->Level );
    return LevelsMax;
}

//#if 0

/**Function*************************************************************

  Synopsis    [Computes levels for AIG with choices and white boxes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManChoiceLevel_rec( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    Aig_Obj_t * pNext;
    int i, iBox, iTerm1, nTerms, LevelMax = 0;
    if ( Aig_ObjIsTravIdCurrent( p, pObj ) )
        return;
    Aig_ObjSetTravIdCurrent( p, pObj );
    if ( Aig_ObjIsCi(pObj) )
    {
        if ( p->pManTime )
        {
            iBox = Tim_ManBoxForCi( (Tim_Man_t *)p->pManTime, Aig_ObjCioId(pObj) );
            if ( iBox >= 0 ) // this is not a true PI
            {
                iTerm1 = Tim_ManBoxInputFirst( (Tim_Man_t *)p->pManTime, iBox );
                nTerms = Tim_ManBoxInputNum( (Tim_Man_t *)p->pManTime, iBox );
                for ( i = 0; i < nTerms; i++ )
                {
                    pNext = Aig_ManCo(p, iTerm1 + i);
                    Aig_ManChoiceLevel_rec( p, pNext );
                    if ( LevelMax < Aig_ObjLevel(pNext) )
                        LevelMax = Aig_ObjLevel(pNext);
                }
                LevelMax++;
            }
        }
//        printf( "%d ", pObj->Level );
    }
    else if ( Aig_ObjIsCo(pObj) )
    {
        pNext = Aig_ObjFanin0(pObj);
        Aig_ManChoiceLevel_rec( p, pNext );
        if ( LevelMax < Aig_ObjLevel(pNext) )
            LevelMax = Aig_ObjLevel(pNext);
    }
    else if ( Aig_ObjIsNode(pObj) )
    { 
        // get the maximum level of the two fanins
        pNext = Aig_ObjFanin0(pObj);
        Aig_ManChoiceLevel_rec( p, pNext );
        if ( LevelMax < Aig_ObjLevel(pNext) )
            LevelMax = Aig_ObjLevel(pNext);
        pNext = Aig_ObjFanin1(pObj);
        Aig_ManChoiceLevel_rec( p, pNext );
        if ( LevelMax < Aig_ObjLevel(pNext) )
            LevelMax = Aig_ObjLevel(pNext);
        LevelMax++;

        // get the level of the nodes in the choice node
        if ( p->pEquivs && (pNext = Aig_ObjEquiv(p, pObj)) )
        {
            Aig_ManChoiceLevel_rec( p, pNext );
            if ( LevelMax < Aig_ObjLevel(pNext) )
                LevelMax = Aig_ObjLevel(pNext);
        }
    }
    else if ( !Aig_ObjIsConst1(pObj) )
        assert( 0 );
    Aig_ObjSetLevel( pObj, LevelMax );
}

/**Function*************************************************************

  Synopsis    [Computes levels for AIG with choices and white boxes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManChoiceLevel( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i, LevelMax = 0;
    Aig_ManForEachObj( p, pObj, i )
        Aig_ObjSetLevel( pObj, 0 );
    Aig_ManSetCioIds( p );
    Aig_ManIncrementTravId( p );
    Aig_ManForEachCo( p, pObj, i )
    {
        Aig_ManChoiceLevel_rec( p, pObj );
        if ( LevelMax < Aig_ObjLevel(pObj) )
            LevelMax = Aig_ObjLevel(pObj);
    }
    // account for dangling boxes
    Aig_ManForEachCi( p, pObj, i )
    {
        Aig_ManChoiceLevel_rec( p, pObj );
        if ( LevelMax < Aig_ObjLevel(pObj) )
            LevelMax = Aig_ObjLevel(pObj);
    }
    Aig_ManCleanCioIds( p );
//    Aig_ManForEachNode( p, pObj, i )
//        assert( Aig_ObjLevel(pObj) > 0 );
    return LevelMax;
} 

//#endif

/**Function*************************************************************

  Synopsis    [Counts the number of AIG nodes rooted at this cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ConeMark_rec( Aig_Obj_t * pObj )
{
    assert( !Aig_IsComplement(pObj) );
    if ( !Aig_ObjIsNode(pObj) || Aig_ObjIsMarkA(pObj) )
        return;
    Aig_ConeMark_rec( Aig_ObjFanin0(pObj) );
    Aig_ConeMark_rec( Aig_ObjFanin1(pObj) );
    assert( !Aig_ObjIsMarkA(pObj) ); // loop detection
    Aig_ObjSetMarkA( pObj );
}

/**Function*************************************************************

  Synopsis    [Counts the number of AIG nodes rooted at this cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ConeCleanAndMark_rec( Aig_Obj_t * pObj )
{
    assert( !Aig_IsComplement(pObj) );
    if ( !Aig_ObjIsNode(pObj) || Aig_ObjIsMarkA(pObj) )
        return;
    Aig_ConeCleanAndMark_rec( Aig_ObjFanin0(pObj) );
    Aig_ConeCleanAndMark_rec( Aig_ObjFanin1(pObj) );
    assert( !Aig_ObjIsMarkA(pObj) ); // loop detection
    Aig_ObjSetMarkA( pObj );
    pObj->pData = NULL;
}

/**Function*************************************************************

  Synopsis    [Counts the number of AIG nodes rooted at this cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ConeCountAndMark_rec( Aig_Obj_t * pObj )
{
    int Counter;
    assert( !Aig_IsComplement(pObj) );
    if ( !Aig_ObjIsNode(pObj) || Aig_ObjIsMarkA(pObj) )
        return 0;
    Counter = 1 + Aig_ConeCountAndMark_rec( Aig_ObjFanin0(pObj) ) + 
        Aig_ConeCountAndMark_rec( Aig_ObjFanin1(pObj) );
    assert( !Aig_ObjIsMarkA(pObj) ); // loop detection
    Aig_ObjSetMarkA( pObj );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Counts the number of AIG nodes rooted at this cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ConeUnmark_rec( Aig_Obj_t * pObj )
{
    assert( !Aig_IsComplement(pObj) );
    if ( !Aig_ObjIsNode(pObj) || !Aig_ObjIsMarkA(pObj) )
        return;
    Aig_ConeUnmark_rec( Aig_ObjFanin0(pObj) ); 
    Aig_ConeUnmark_rec( Aig_ObjFanin1(pObj) );
    assert( Aig_ObjIsMarkA(pObj) ); // loop detection
    Aig_ObjClearMarkA( pObj );
}

/**Function*************************************************************

  Synopsis    [Counts the number of AIG nodes rooted at this cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_DagSize( Aig_Obj_t * pObj )
{
    int Counter;
    Counter = Aig_ConeCountAndMark_rec( Aig_Regular(pObj) );
    Aig_ConeUnmark_rec( Aig_Regular(pObj) );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Counts the support size of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_SupportSize_rec( Aig_Man_t * p, Aig_Obj_t * pObj, int * pCounter )
{
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(p, pObj);
    if ( Aig_ObjIsCi(pObj) )
    {
        (*pCounter)++;
        return;
    }
    assert( Aig_ObjIsNode(pObj) || Aig_ObjIsBuf(pObj) );
    Aig_SupportSize_rec( p, Aig_ObjFanin0(pObj), pCounter );
    if ( Aig_ObjFanin1(pObj) )
        Aig_SupportSize_rec( p, Aig_ObjFanin1(pObj), pCounter );
}

/**Function*************************************************************

  Synopsis    [Counts the support size of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_SupportSize( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    int Counter = 0;
    assert( !Aig_IsComplement(pObj) );
    assert( !Aig_ObjIsCo(pObj) );
    Aig_ManIncrementTravId( p );
    Aig_SupportSize_rec( p, pObj, &Counter );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Counts the support size of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_SupportSizeTest( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i, Counter = 0;
    abctime clk = Abc_Clock();
    Aig_ManForEachObj( p, pObj, i )
        if ( Aig_ObjIsNode(pObj) )
            Counter += (Aig_SupportSize(p, pObj) <= 16);
    printf( "Nodes with small support %d (out of %d)\n", Counter, Aig_ManNodeNum(p) );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Counts the support size of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_Support_rec( Aig_Man_t * p, Aig_Obj_t * pObj, Vec_Ptr_t * vSupp )
{
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(p, pObj);
    if ( Aig_ObjIsConst1(pObj) )
        return;
    if ( Aig_ObjIsCi(pObj) )
    {
        Vec_PtrPush( vSupp, pObj );
        return;
    }
    assert( Aig_ObjIsNode(pObj) || Aig_ObjIsBuf(pObj) );
    Aig_Support_rec( p, Aig_ObjFanin0(pObj), vSupp );
    if ( Aig_ObjFanin1(pObj) )
        Aig_Support_rec( p, Aig_ObjFanin1(pObj), vSupp );
}

/**Function*************************************************************

  Synopsis    [Counts the support size of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_Support( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    Vec_Ptr_t * vSupp;
    assert( !Aig_IsComplement(pObj) );
    assert( !Aig_ObjIsCo(pObj) );
    Aig_ManIncrementTravId( p );
    vSupp = Vec_PtrAlloc( 100 );
    Aig_Support_rec( p, pObj, vSupp );
    return vSupp;
}

/**Function*************************************************************

  Synopsis    [Counts the support size of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_SupportNodes( Aig_Man_t * p, Aig_Obj_t ** ppObjs, int nObjs, Vec_Ptr_t * vSupp )
{
    int i;
    Vec_PtrClear( vSupp );
    Aig_ManIncrementTravId( p );
    Aig_ObjSetTravIdCurrent( p, Aig_ManConst1(p) );
    for ( i = 0; i < nObjs; i++ )
    {
        assert( !Aig_IsComplement(ppObjs[i]) );
        if ( Aig_ObjIsCo(ppObjs[i]) )
            Aig_Support_rec( p, Aig_ObjFanin0(ppObjs[i]), vSupp );
        else
            Aig_Support_rec( p, ppObjs[i], vSupp );
    }
}

/**Function*************************************************************

  Synopsis    [Transfers the AIG from one manager into another.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_Transfer_rec( Aig_Man_t * pDest, Aig_Obj_t * pObj )
{
    assert( !Aig_IsComplement(pObj) );
    if ( !Aig_ObjIsNode(pObj) || Aig_ObjIsMarkA(pObj) )
        return;
    Aig_Transfer_rec( pDest, Aig_ObjFanin0(pObj) ); 
    Aig_Transfer_rec( pDest, Aig_ObjFanin1(pObj) );
    pObj->pData = Aig_And( pDest, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    assert( !Aig_ObjIsMarkA(pObj) ); // loop detection
    Aig_ObjSetMarkA( pObj );
}

/**Function*************************************************************

  Synopsis    [Transfers the AIG from one manager into another.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Aig_Transfer( Aig_Man_t * pSour, Aig_Man_t * pDest, Aig_Obj_t * pRoot, int nVars )
{
    Aig_Obj_t * pObj;
    int i;
    // solve simple cases
    if ( pSour == pDest )
        return pRoot;
    if ( Aig_ObjIsConst1( Aig_Regular(pRoot) ) )
        return Aig_NotCond( Aig_ManConst1(pDest), Aig_IsComplement(pRoot) );
    // set the PI mapping
    Aig_ManForEachCi( pSour, pObj, i )
    {
        if ( i == nVars )
           break;
        pObj->pData = Aig_IthVar(pDest, i);
    }
    // transfer and set markings
    Aig_Transfer_rec( pDest, Aig_Regular(pRoot) );
    // clear the markings
    Aig_ConeUnmark_rec( Aig_Regular(pRoot) );
    return Aig_NotCond( (Aig_Obj_t *)Aig_Regular(pRoot)->pData, Aig_IsComplement(pRoot) );
}

/**Function*************************************************************

  Synopsis    [Composes the AIG (pRoot) with the function (pFunc) using PI var (iVar).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_Compose_rec( Aig_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pFunc, Aig_Obj_t * pVar )
{
    assert( !Aig_IsComplement(pObj) );
    if ( Aig_ObjIsMarkA(pObj) )
        return;
    if ( Aig_ObjIsConst1(pObj) || Aig_ObjIsCi(pObj) )
    {
        pObj->pData = pObj == pVar ? pFunc : pObj;
        return;
    }
    Aig_Compose_rec( p, Aig_ObjFanin0(pObj), pFunc, pVar ); 
    Aig_Compose_rec( p, Aig_ObjFanin1(pObj), pFunc, pVar );
    pObj->pData = Aig_And( p, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    assert( !Aig_ObjIsMarkA(pObj) ); // loop detection
    Aig_ObjSetMarkA( pObj );
}

/**Function*************************************************************

  Synopsis    [Composes the AIG (pRoot) with the function (pFunc) using PI var (iVar).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Aig_Compose( Aig_Man_t * p, Aig_Obj_t * pRoot, Aig_Obj_t * pFunc, int iVar )
{
    // quit if the PI variable is not defined
    if ( iVar >= Aig_ManCiNum(p) )
    {
        printf( "Aig_Compose(): The PI variable %d is not defined.\n", iVar );
        return NULL;
    }
    // recursively perform composition
    Aig_Compose_rec( p, Aig_Regular(pRoot), pFunc, Aig_ManCi(p, iVar) );
    // clear the markings
    Aig_ConeUnmark_rec( Aig_Regular(pRoot) );
    return Aig_NotCond( (Aig_Obj_t *)Aig_Regular(pRoot)->pData, Aig_IsComplement(pRoot) );
}

/**Function*************************************************************

  Synopsis    [Computes the internal nodes of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjCollectCut_rec( Aig_Obj_t * pNode, Vec_Ptr_t * vNodes )
{
//    Aig_Obj_t * pFan0 = Aig_ObjFanin0(pNode);
//    Aig_Obj_t * pFan1 = Aig_ObjFanin1(pNode);
    if ( pNode->fMarkA )
        return;
    pNode->fMarkA = 1;
    assert( Aig_ObjIsNode(pNode) );
    Aig_ObjCollectCut_rec( Aig_ObjFanin0(pNode), vNodes );
    Aig_ObjCollectCut_rec( Aig_ObjFanin1(pNode), vNodes );
    Vec_PtrPush( vNodes, pNode );
//printf( "added %d  ", pNode->Id );
}

/**Function*************************************************************

  Synopsis    [Computes the internal nodes of the cut.]

  Description [Does not include the leaves of the cut.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjCollectCut( Aig_Obj_t * pRoot, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vNodes )
{
    Aig_Obj_t * pObj;
    int i;
    // collect and mark the leaves
    Vec_PtrClear( vNodes );
    Vec_PtrForEachEntry( Aig_Obj_t *, vLeaves, pObj, i )
    {
        assert( pObj->fMarkA == 0 );
        pObj->fMarkA = 1;
//        printf( "%d " , pObj->Id );
    }
//printf( "\n" );
    // collect and mark the nodes
    Aig_ObjCollectCut_rec( pRoot, vNodes );
    // clean the nodes
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
        pObj->fMarkA = 0;
    Vec_PtrForEachEntry( Aig_Obj_t *, vLeaves, pObj, i )
        pObj->fMarkA = 0;
}


/**Function*************************************************************

  Synopsis    [Collects the nodes of the supergate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ObjCollectSuper_rec( Aig_Obj_t * pRoot, Aig_Obj_t * pObj, Vec_Ptr_t * vSuper )
{
    int RetValue1, RetValue2, i;
    // check if the node is visited
    if ( Aig_Regular(pObj)->fMarkA )
    {
        // check if the node occurs in the same polarity
        for ( i = 0; i < vSuper->nSize; i++ )
            if ( vSuper->pArray[i] == pObj )
                return 1;
        // check if the node is present in the opposite polarity
        for ( i = 0; i < vSuper->nSize; i++ )
            if ( vSuper->pArray[i] == Aig_Not(pObj) )
                return -1;
        assert( 0 );
        return 0;
    }
    // if the new node is complemented or a PI, another gate begins
    if ( pObj != pRoot && (Aig_IsComplement(pObj) || Aig_ObjType(pObj) != Aig_ObjType(pRoot) || Aig_ObjRefs(pObj) > 1) )
    {
        Vec_PtrPush( vSuper, pObj );
        Aig_Regular(pObj)->fMarkA = 1;
        return 0;
    }
    assert( !Aig_IsComplement(pObj) );
    assert( Aig_ObjIsNode(pObj) );
    // go through the branches
    RetValue1 = Aig_ObjCollectSuper_rec( pRoot, Aig_ObjReal_rec( Aig_ObjChild0(pObj) ), vSuper );
    RetValue2 = Aig_ObjCollectSuper_rec( pRoot, Aig_ObjReal_rec( Aig_ObjChild1(pObj) ), vSuper );
    if ( RetValue1 == -1 || RetValue2 == -1 )
        return -1;
    // return 1 if at least one branch has a duplicate
    return RetValue1 || RetValue2;
}

/**Function*************************************************************

  Synopsis    [Collects the nodes of the supergate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ObjCollectSuper( Aig_Obj_t * pObj, Vec_Ptr_t * vSuper )
{
    int RetValue, i;
    assert( !Aig_IsComplement(pObj) );
    assert( Aig_ObjIsNode(pObj) );
    // collect the nodes in the implication supergate
    Vec_PtrClear( vSuper );
    RetValue = Aig_ObjCollectSuper_rec( pObj, pObj, vSuper );
    assert( Vec_PtrSize(vSuper) > 1 );
    // unmark the visited nodes
    Vec_PtrForEachEntry( Aig_Obj_t *, vSuper, pObj, i )
        Aig_Regular(pObj)->fMarkA = 0;
    // if we found the node and its complement in the same implication supergate, 
    // return empty set of nodes (meaning that we should use constant-0 node)
    if ( RetValue == -1 )
        vSuper->nSize = 0;
    return RetValue;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

