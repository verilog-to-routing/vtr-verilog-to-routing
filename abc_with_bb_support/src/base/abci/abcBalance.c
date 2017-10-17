/**CFile****************************************************************

  FileName    [abcBalance.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Performs global balancing of the AIG by the number of levels.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcBalance.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
 
static void        Abc_NtkBalancePerform( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkAig, int fDuplicate, int fSelective, int fUpdateLevel );
static Abc_Obj_t * Abc_NodeBalance_rec( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pNode, Vec_Vec_t * vStorage, int Level, int fDuplicate, int fSelective, int fUpdateLevel );
static Vec_Ptr_t * Abc_NodeBalanceCone( Abc_Obj_t * pNode, Vec_Vec_t * vSuper, int Level, int fDuplicate, int fSelective );
static int         Abc_NodeBalanceCone_rec( Abc_Obj_t * pNode, Vec_Ptr_t * vSuper, int fFirst, int fDuplicate, int fSelective );
static void        Abc_NtkMarkCriticalNodes( Abc_Ntk_t * pNtk );
static Vec_Ptr_t * Abc_NodeBalanceConeExor( Abc_Obj_t * pNode );


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Balances the AIG network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkBalance( Abc_Ntk_t * pNtk, int fDuplicate, int fSelective, int fUpdateLevel )
{
//    extern void Abc_NtkHaigTranfer( Abc_Ntk_t * pNtkOld, Abc_Ntk_t * pNtkNew );
    Abc_Ntk_t * pNtkAig;
    assert( Abc_NtkIsStrash(pNtk) );
    // compute the required times
    if ( fSelective )
    {
        Abc_NtkStartReverseLevels( pNtk, 0 );
        Abc_NtkMarkCriticalNodes( pNtk );
    }
    // perform balancing
    pNtkAig = Abc_NtkStartFrom( pNtk, ABC_NTK_STRASH, ABC_FUNC_AIG );
    // transfer HAIG
//    Abc_NtkHaigTranfer( pNtk, pNtkAig );
    // perform balancing
    Abc_NtkBalancePerform( pNtk, pNtkAig, fDuplicate, fSelective, fUpdateLevel );
    Abc_NtkFinalize( pNtk, pNtkAig );
    Abc_AigCleanup( (Abc_Aig_t *)pNtkAig->pManFunc );
    // undo the required times
    if ( fSelective )
    {
        Abc_NtkStopReverseLevels( pNtk );
        Abc_NtkCleanMarkA( pNtk );
    }
    if ( pNtk->pExdc )
        pNtkAig->pExdc = Abc_NtkDup( pNtk->pExdc );
    // make sure everything is okay
    if ( !Abc_NtkCheck( pNtkAig ) )
    {
        printf( "Abc_NtkBalance: The network check has failed.\n" );
        Abc_NtkDelete( pNtkAig );
        return NULL;
    }
//Abc_NtkPrintCiLevels( pNtkAig );
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Balances the AIG network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkBalancePerform( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkAig, int fDuplicate, int fSelective, int fUpdateLevel )
{
    ProgressBar * pProgress;
    Vec_Vec_t * vStorage;
    Abc_Obj_t * pNode;
    int i;
    // transfer level
    Abc_NtkForEachCi( pNtk, pNode, i )
        pNode->pCopy->Level = pNode->Level;
    // set the level of PIs of AIG according to the arrival times of the old network
    Abc_NtkSetNodeLevelsArrival( pNtk );
    // allocate temporary storage for supergates
    vStorage = Vec_VecStart( 10 );
    // perform balancing of POs
    pProgress = Extra_ProgressBarStart( stdout, Abc_NtkCoNum(pNtk) );
    if ( pNtk->nBarBufs == 0 )
    {
        Abc_NtkForEachCo( pNtk, pNode, i )
        {
            Extra_ProgressBarUpdate( pProgress, i, NULL );
            Abc_NodeBalance_rec( pNtkAig, Abc_ObjFanin0(pNode), vStorage, 0, fDuplicate, fSelective, fUpdateLevel );
        }
    }
    else
    {
        Abc_NtkForEachLiPo( pNtk, pNode, i )
        {
            Extra_ProgressBarUpdate( pProgress, i, NULL );
            Abc_NodeBalance_rec( pNtkAig, Abc_ObjFanin0(pNode), vStorage, 0, fDuplicate, fSelective, fUpdateLevel );
            if ( i < pNtk->nBarBufs )
                Abc_ObjFanout0(Abc_ObjFanout0(pNode))->Level = Abc_ObjFanin0(pNode)->Level;
        }
    }
    Extra_ProgressBarStop( pProgress );
    Vec_VecFree( vStorage );
}

/**Function*************************************************************

  Synopsis    [Finds the left bound on the next candidate to be paired.]

  Description [The nodes in the array are in the decreasing order of levels. 
  The last node in the array has the smallest level. By default it would be paired 
  with the next node on the left. However, it may be possible to pair it with some
  other node on the left, in such a way that the new node is shared. This procedure
  finds the index of the left-most node, which can be paired with the last node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeBalanceFindLeft( Vec_Ptr_t * vSuper )
{
    Abc_Obj_t * pNodeRight, * pNodeLeft;
    int Current;
    // if two or less nodes, pair with the first
    if ( Vec_PtrSize(vSuper) < 3 )
        return 0;
    // set the pointer to the one before the last
    Current = Vec_PtrSize(vSuper) - 2;
    pNodeRight = (Abc_Obj_t *)Vec_PtrEntry( vSuper, Current );
    // go through the nodes to the left of this one
    for ( Current--; Current >= 0; Current-- )
    {
        // get the next node on the left
        pNodeLeft = (Abc_Obj_t *)Vec_PtrEntry( vSuper, Current );
        // if the level of this node is different, quit the loop
        if ( Abc_ObjRegular(pNodeLeft)->Level != Abc_ObjRegular(pNodeRight)->Level )
            break;
    }
    Current++;    
    // get the node, for which the equality holds
    pNodeLeft = (Abc_Obj_t *)Vec_PtrEntry( vSuper, Current );
    assert( Abc_ObjRegular(pNodeLeft)->Level == Abc_ObjRegular(pNodeRight)->Level );
    return Current;
}

/**Function*************************************************************

  Synopsis    [Moves closer to the end the node that is best for sharing.]

  Description [If there is no node with sharing, randomly chooses one of 
  the legal nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeBalancePermute( Abc_Ntk_t * pNtkNew, Vec_Ptr_t * vSuper, int LeftBound )
{
    Abc_Obj_t * pNode1, * pNode2, * pNode3;
    int RightBound, i;
    // get the right bound
    RightBound = Vec_PtrSize(vSuper) - 2;
    assert( LeftBound <= RightBound );
    if ( LeftBound == RightBound )
        return;
    // get the two last nodes
    pNode1 = (Abc_Obj_t *)Vec_PtrEntry( vSuper, RightBound + 1 );
    pNode2 = (Abc_Obj_t *)Vec_PtrEntry( vSuper, RightBound     );
    // find the first node that can be shared
    for ( i = RightBound; i >= LeftBound; i-- )
    {
        pNode3 = (Abc_Obj_t *)Vec_PtrEntry( vSuper, i );
        if ( Abc_AigAndLookup( (Abc_Aig_t *)pNtkNew->pManFunc, pNode1, pNode3 ) )
        {
            if ( pNode3 == pNode2 )
                return;
            Vec_PtrWriteEntry( vSuper, i,          pNode2 );
            Vec_PtrWriteEntry( vSuper, RightBound, pNode3 );
            return;
        }
    }
/*
    // we did not find the node to share, randomize choice
    {
        int Choice = rand() % (RightBound - LeftBound + 1);
        pNode3 = Vec_PtrEntry( vSuper, LeftBound + Choice );
        if ( pNode3 == pNode2 )
            return;
        Vec_PtrWriteEntry( vSuper, LeftBound + Choice, pNode2 );
        Vec_PtrWriteEntry( vSuper, RightBound,         pNode3 );
    }
*/
}

/**Function*************************************************************

  Synopsis    [Rebalances the multi-input node rooted at pNodeOld.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NodeBalance_rec( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pNodeOld, Vec_Vec_t * vStorage, int Level, int fDuplicate, int fSelective, int fUpdateLevel )
{
    Abc_Aig_t * pMan = (Abc_Aig_t *)pNtkNew->pManFunc;
    Abc_Obj_t * pNodeNew, * pNode1, * pNode2;
    Vec_Ptr_t * vSuper;
    int i, LeftBound;
    assert( !Abc_ObjIsComplement(pNodeOld) );
    // return if the result if known
    if ( pNodeOld->pCopy )
        return pNodeOld->pCopy;
    assert( Abc_ObjIsNode(pNodeOld) );
    // get the implication supergate
//    Abc_NodeBalanceConeExor( pNodeOld );
    vSuper = Abc_NodeBalanceCone( pNodeOld, vStorage, Level, fDuplicate, fSelective );
    if ( vSuper->nSize == 0 )
    { // it means that the supergate contains two nodes in the opposite polarity
        pNodeOld->pCopy = Abc_ObjNot(Abc_AigConst1(pNtkNew));
        return pNodeOld->pCopy;
    }
    // for each old node, derive the new well-balanced node
    for ( i = 0; i < vSuper->nSize; i++ )
    {
        pNodeNew = Abc_NodeBalance_rec( pNtkNew, Abc_ObjRegular((Abc_Obj_t *)vSuper->pArray[i]), vStorage, Level + 1, fDuplicate, fSelective, fUpdateLevel );
        vSuper->pArray[i] = Abc_ObjNotCond( pNodeNew, Abc_ObjIsComplement((Abc_Obj_t *)vSuper->pArray[i]) );
    }
    if ( vSuper->nSize < 2 )
        printf( "BUG!\n" );
    // sort the new nodes by level in the decreasing order
    Vec_PtrSort( vSuper, (int (*)(void))Abc_NodeCompareLevelsDecrease );
    // balance the nodes
    assert( vSuper->nSize > 1 );
    while ( vSuper->nSize > 1 )
    {
        // find the left bound on the node to be paired
        LeftBound = (!fUpdateLevel)? 0 : Abc_NodeBalanceFindLeft( vSuper );
        // find the node that can be shared (if no such node, randomize choice)
        Abc_NodeBalancePermute( pNtkNew, vSuper, LeftBound );
        // pull out the last two nodes
        pNode1 = (Abc_Obj_t *)Vec_PtrPop(vSuper);
        pNode2 = (Abc_Obj_t *)Vec_PtrPop(vSuper);
        Abc_VecObjPushUniqueOrderByLevel( vSuper, Abc_AigAnd(pMan, pNode1, pNode2) );
    }
    // make sure the balanced node is not assigned
    assert( pNodeOld->pCopy == NULL );
    // mark the old node with the new node
    pNodeOld->pCopy = (Abc_Obj_t *)vSuper->pArray[0];
    vSuper->nSize = 0;
//    if ( Abc_ObjRegular(pNodeOld->pCopy) == Abc_AigConst1(pNtkNew) )
//        printf( "Constant node\n" );
//    assert( pNodeOld->Level >= Abc_ObjRegular(pNodeOld->pCopy)->Level );
    return pNodeOld->pCopy;
}

/**Function*************************************************************

  Synopsis    [Collects the nodes in the cone delimited by fMarkA==1.]

  Description [Returns -1 if the AND-cone has the same node in both polarities.
  Returns 1 if the AND-cone has the same node in the same polarity. Returns 0
  if the AND-cone has no repeated nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NodeBalanceCone( Abc_Obj_t * pNode, Vec_Vec_t * vStorage, int Level, int fDuplicate, int fSelective )
{
    Vec_Ptr_t * vNodes;
    int RetValue, i;
    assert( !Abc_ObjIsComplement(pNode) );
    // extend the storage
    if ( Vec_VecSize( vStorage ) <= Level )
        Vec_VecPush( vStorage, Level, 0 );
    // get the temporary array of nodes
    vNodes = Vec_VecEntry( vStorage, Level );
    Vec_PtrClear( vNodes );
    // collect the nodes in the implication supergate
    RetValue = Abc_NodeBalanceCone_rec( pNode, vNodes, 1, fDuplicate, fSelective );
    assert( vNodes->nSize > 1 );
    // unmark the visited nodes
    for ( i = 0; i < vNodes->nSize; i++ )
        Abc_ObjRegular((Abc_Obj_t *)vNodes->pArray[i])->fMarkB = 0;
    // if we found the node and its complement in the same implication supergate, 
    // return empty set of nodes (meaning that we should use constant-0 node)
    if ( RetValue == -1 )
        vNodes->nSize = 0;
    return vNodes;
}


/**Function*************************************************************

  Synopsis    [Collects the nodes in the cone delimited by fMarkA==1.]

  Description [Returns -1 if the AND-cone has the same node in both polarities.
  Returns 1 if the AND-cone has the same node in the same polarity. Returns 0
  if the AND-cone has no repeated nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeBalanceCone_rec( Abc_Obj_t * pNode, Vec_Ptr_t * vSuper, int fFirst, int fDuplicate, int fSelective )
{
    int RetValue1, RetValue2, i;
    // check if the node is visited
    if ( Abc_ObjRegular(pNode)->fMarkB )
    {
        // check if the node occurs in the same polarity
        for ( i = 0; i < vSuper->nSize; i++ )
            if ( vSuper->pArray[i] == pNode )
                return 1;
        // check if the node is present in the opposite polarity
        for ( i = 0; i < vSuper->nSize; i++ )
            if ( vSuper->pArray[i] == Abc_ObjNot(pNode) )
                return -1;
        assert( 0 );
        return 0;
    }
    // if the new node is complemented or a PI, another gate begins
    if ( !fFirst && (Abc_ObjIsComplement(pNode) || !Abc_ObjIsNode(pNode) || (!fDuplicate && !fSelective && (Abc_ObjFanoutNum(pNode) > 1)) || Vec_PtrSize(vSuper) > 10000) )
    {
        Vec_PtrPush( vSuper, pNode );
        Abc_ObjRegular(pNode)->fMarkB = 1;
        return 0;
    }
    assert( !Abc_ObjIsComplement(pNode) );
    assert( Abc_ObjIsNode(pNode) );
    // go through the branches
    RetValue1 = Abc_NodeBalanceCone_rec( Abc_ObjChild0(pNode), vSuper, 0, fDuplicate, fSelective );
    RetValue2 = Abc_NodeBalanceCone_rec( Abc_ObjChild1(pNode), vSuper, 0, fDuplicate, fSelective );
    if ( RetValue1 == -1 || RetValue2 == -1 )
        return -1;
    // return 1 if at least one branch has a duplicate
    return RetValue1 || RetValue2;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeBalanceConeExor_rec( Abc_Obj_t * pNode, Vec_Ptr_t * vSuper, int fFirst )
{
    int RetValue1, RetValue2, i;
    // check if the node occurs in the same polarity
    for ( i = 0; i < vSuper->nSize; i++ )
        if ( vSuper->pArray[i] == pNode )
            return 1;
    // if the new node is complemented or a PI, another gate begins
    if ( !fFirst && (!pNode->fExor || !Abc_ObjIsNode(pNode)) )
    {
        Vec_PtrPush( vSuper, pNode );
        return 0;
    }
    assert( !Abc_ObjIsComplement(pNode) );
    assert( Abc_ObjIsNode(pNode) );
    assert( pNode->fExor );
    // go through the branches
    RetValue1 = Abc_NodeBalanceConeExor_rec( Abc_ObjFanin0(Abc_ObjFanin0(pNode)), vSuper, 0 );
    RetValue2 = Abc_NodeBalanceConeExor_rec( Abc_ObjFanin1(Abc_ObjFanin0(pNode)), vSuper, 0 );
    if ( RetValue1 == -1 || RetValue2 == -1 )
        return -1;
    // return 1 if at least one branch has a duplicate
    return RetValue1 || RetValue2;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NodeBalanceConeExor( Abc_Obj_t * pNode )
{
    Vec_Ptr_t * vSuper;
    if ( !pNode->fExor )
        return NULL;
    vSuper = Vec_PtrAlloc( 10 );
    Abc_NodeBalanceConeExor_rec( pNode, vSuper, 1 );
    printf( "%d ", Vec_PtrSize(vSuper) );
    Vec_PtrFree( vSuper );
    return NULL;
}



/**Function*************************************************************

  Synopsis    [Collects the nodes in the implication supergate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NodeFindCone_rec( Abc_Obj_t * pNode )
{
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pNodeC, * pNodeT, * pNodeE;
    int RetValue, i;
    assert( !Abc_ObjIsComplement(pNode) );
    if ( Abc_ObjIsCi(pNode) )
        return NULL;
    // start the new array
    vNodes = Vec_PtrAlloc( 4 );
    // if the node is the MUX collect its fanins
    if ( Abc_NodeIsMuxType(pNode) )
    {
        pNodeC = Abc_NodeRecognizeMux( pNode, &pNodeT, &pNodeE );
        Vec_PtrPush( vNodes, Abc_ObjRegular(pNodeC) );
        Vec_PtrPushUnique( vNodes, Abc_ObjRegular(pNodeT) );
        Vec_PtrPushUnique( vNodes, Abc_ObjRegular(pNodeE) );
    }
    else
    {
        // collect the nodes in the implication supergate
        RetValue = Abc_NodeBalanceCone_rec( pNode, vNodes, 1, 1, 0 );
        assert( vNodes->nSize > 1 );
        // unmark the visited nodes
        Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
            Abc_ObjRegular(pNode)->fMarkB = 0;
        // if we found the node and its complement in the same implication supergate, 
        // return empty set of nodes (meaning that we should use constant-0 node)
        if ( RetValue == -1 )
            vNodes->nSize = 0;
    }
    // call for the fanin
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
    {
        pNode = Abc_ObjRegular(pNode);
        if ( pNode->pCopy )
            continue;
        pNode->pCopy = (Abc_Obj_t *)Abc_NodeFindCone_rec( pNode );
    }
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Attaches the implication supergates to internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkBalanceAttach( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i;
    Abc_NtkCleanCopy( pNtk );
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        pNode = Abc_ObjFanin0(pNode);
        if ( pNode->pCopy )
            continue;
        pNode->pCopy = (Abc_Obj_t *)Abc_NodeFindCone_rec( pNode );
    }
}

/**Function*************************************************************

  Synopsis    [Attaches the implication supergates to internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkBalanceDetach( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i;
    Abc_NtkForEachNode( pNtk, pNode, i )
        if ( pNode->pCopy )
        {
            Vec_PtrFree( (Vec_Ptr_t *)pNode->pCopy );
            pNode->pCopy = NULL;
        }
}

/**Function*************************************************************

  Synopsis    [Compute levels of implication supergates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkBalanceLevel_rec( Abc_Obj_t * pNode )
{
    Vec_Ptr_t * vSuper;
    Abc_Obj_t * pFanin;
    int i, LevelMax;
    assert( !Abc_ObjIsComplement(pNode) );
    if ( pNode->Level > 0 )
        return pNode->Level;
    if ( Abc_ObjIsCi(pNode) )
        return 0;
    vSuper = (Vec_Ptr_t *)pNode->pCopy;
    assert( vSuper != NULL );
    LevelMax = 0;
    Vec_PtrForEachEntry( Abc_Obj_t *, vSuper, pFanin, i )
    {
        pFanin = Abc_ObjRegular(pFanin);
        Abc_NtkBalanceLevel_rec(pFanin);
        if ( LevelMax < (int)pFanin->Level )
            LevelMax = pFanin->Level;
    }
    pNode->Level = LevelMax + 1;
    return pNode->Level;
}


/**Function*************************************************************

  Synopsis    [Compute levels of implication supergates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkBalanceLevel( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i;
    Abc_NtkForEachObj( pNtk, pNode, i )
        pNode->Level = 0;
    Abc_NtkForEachCo( pNtk, pNode, i )
        Abc_NtkBalanceLevel_rec( Abc_ObjFanin0(pNode) );
}


/**Function*************************************************************

  Synopsis    [Marks the nodes on the critical and near critical paths.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkMarkCriticalNodes( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i, Counter = 0;
    Abc_NtkForEachNode( pNtk, pNode, i )
        if ( Abc_ObjRequiredLevel(pNode) - pNode->Level <= 1 )
            pNode->fMarkA = 1, Counter++;
    printf( "The number of nodes on the critical paths = %6d  (%5.2f %%)\n", Counter, 100.0 * Counter / Abc_NtkNodeNum(pNtk) );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

