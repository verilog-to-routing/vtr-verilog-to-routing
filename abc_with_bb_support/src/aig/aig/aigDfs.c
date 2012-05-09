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

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

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
    assert( Aig_ObjIsNode(pObj) || Aig_ObjIsBuf(pObj) );
    Aig_ManDfs_rec( p, Aig_ObjFanin0(pObj), vNodes );
    Aig_ManDfs_rec( p, Aig_ObjFanin1(pObj), vNodes );
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
Vec_Ptr_t * Aig_ManDfs( Aig_Man_t * p )
{
    Vec_Ptr_t * vNodes;
    Aig_Obj_t * pObj;
    int i;
    Aig_ManIncrementTravId( p );
    // mark constant and PIs
    Aig_ObjSetTravIdCurrent( p, Aig_ManConst1(p) );
    Aig_ManForEachPi( p, pObj, i )
        Aig_ObjSetTravIdCurrent( p, pObj );
    // if there are latches, mark them
    if ( Aig_ManLatchNum(p) > 0 )
        Aig_ManForEachObj( p, pObj, i )
            if ( Aig_ObjIsLatch(pObj) )
                Aig_ObjSetTravIdCurrent( p, pObj );
    // go through the nodes
    vNodes = Vec_PtrAlloc( Aig_ManNodeNum(p) );
    Aig_ManForEachObj( p, pObj, i )
        if ( Aig_ObjIsNode(pObj) || Aig_ObjIsBuf(pObj) )
            Aig_ManDfs_rec( p, pObj, vNodes );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Collects internal nodes in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManDfsNodes( Aig_Man_t * p, Aig_Obj_t ** ppNodes, int nNodes )
{
    Vec_Ptr_t * vNodes;
    Aig_Obj_t * pObj;
    int i;
    assert( Aig_ManLatchNum(p) == 0 );
    Aig_ManIncrementTravId( p );
    // mark constant and PIs
    Aig_ObjSetTravIdCurrent( p, Aig_ManConst1(p) );
    Aig_ManForEachPi( p, pObj, i )
        Aig_ObjSetTravIdCurrent( p, pObj );
    // go through the nodes
    vNodes = Vec_PtrAlloc( Aig_ManNodeNum(p) );
    for ( i = 0; i < nNodes; i++ )
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
    Aig_ManDfsChoices_rec( p, p->pEquivs[pObj->Id], vNodes );
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
    int i;
    assert( p->pEquivs != NULL );
    Aig_ManIncrementTravId( p );
    // mark constant and PIs
    Aig_ObjSetTravIdCurrent( p, Aig_ManConst1(p) );
    Aig_ManForEachPi( p, pObj, i )
        Aig_ObjSetTravIdCurrent( p, pObj );
    // go through the nodes
    vNodes = Vec_PtrAlloc( Aig_ManNodeNum(p) );
    Aig_ManForEachPo( p, pObj, i )
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
    int iFanout, i;
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
    Aig_ManForEachPo( p, pObj, i )
        Aig_ObjSetTravIdCurrent( p, pObj );
    // if there are latches, mark them
    if ( Aig_ManLatchNum(p) > 0 )
        Aig_ManForEachObj( p, pObj, i )
            if ( Aig_ObjIsLatch(pObj) )
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
int Aig_ManCountLevels( Aig_Man_t * p )
{
    Vec_Ptr_t * vNodes;
    Aig_Obj_t * pObj;
    int i, LevelsMax, Level0, Level1;
    // initialize the levels
    Aig_ManConst1(p)->pData = NULL;
    Aig_ManForEachPi( p, pObj, i )
        pObj->pData = NULL;
    // compute levels in a DFS order
    vNodes = Aig_ManDfs( p );
    Vec_PtrForEachEntry( vNodes, pObj, i )
    {
        Level0 = (int)Aig_ObjFanin0(pObj)->pData;
        Level1 = (int)Aig_ObjFanin1(pObj)->pData;
        pObj->pData = (void *)(1 + Aig_ObjIsExor(pObj) + AIG_MAX(Level0, Level1));
    }
    Vec_PtrFree( vNodes );
    // get levels of the POs
    LevelsMax = 0;
    Aig_ManForEachPo( p, pObj, i )
        LevelsMax = AIG_MAX( LevelsMax, (int)Aig_ObjFanin0(pObj)->pData );
    return LevelsMax;
}

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
    Aig_ManForEachPi( pSour, pObj, i )
    {
        if ( i == nVars )
           break;
        pObj->pData = Aig_IthVar(pDest, i);
    }
    // transfer and set markings
    Aig_Transfer_rec( pDest, Aig_Regular(pRoot) );
    // clear the markings
    Aig_ConeUnmark_rec( Aig_Regular(pRoot) );
    return Aig_NotCond( Aig_Regular(pRoot)->pData, Aig_IsComplement(pRoot) );
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
    if ( Aig_ObjIsConst1(pObj) || Aig_ObjIsPi(pObj) )
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
    if ( iVar >= Aig_ManPiNum(p) )
    {
        printf( "Aig_Compose(): The PI variable %d is not defined.\n", iVar );
        return NULL;
    }
    // recursively perform composition
    Aig_Compose_rec( p, Aig_Regular(pRoot), pFunc, Aig_ManPi(p, iVar) );
    // clear the markings
    Aig_ConeUnmark_rec( Aig_Regular(pRoot) );
    return Aig_NotCond( Aig_Regular(pRoot)->pData, Aig_IsComplement(pRoot) );
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
    Vec_PtrForEachEntry( vLeaves, pObj, i )
    {
        assert( pObj->fMarkA == 0 );
        pObj->fMarkA = 1;
//        printf( "%d " , pObj->Id );
    }
//printf( "\n" );
    // collect and mark the nodes
    Aig_ObjCollectCut_rec( pRoot, vNodes );
    // clean the nodes
    Vec_PtrForEachEntry( vNodes, pObj, i )
        pObj->fMarkA = 0;
    Vec_PtrForEachEntry( vLeaves, pObj, i )
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
    Vec_PtrForEachEntry( vSuper, pObj, i )
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


