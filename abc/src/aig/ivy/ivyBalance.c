/**CFile****************************************************************

  FileName    [ivyBalance.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [And-Inverter Graph package.]

  Synopsis    [Algebraic AIG balancing.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 11, 2006.]

  Revision    [$Id: ivyBalance.c,v 1.00 2006/05/11 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ivy.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int         Ivy_NodeBalance_rec( Ivy_Man_t * pNew, Ivy_Obj_t * pObj, Vec_Vec_t * vStore, int Level, int fUpdateLevel );
static Vec_Ptr_t * Ivy_NodeBalanceCone( Ivy_Obj_t * pObj, Vec_Vec_t * vStore, int Level );
static int         Ivy_NodeBalanceFindLeft( Vec_Ptr_t * vSuper );
static void        Ivy_NodeBalancePermute( Ivy_Man_t * p, Vec_Ptr_t * vSuper, int LeftBound, int fExor );
static void        Ivy_NodeBalancePushUniqueOrderByLevel( Vec_Ptr_t * vStore, Ivy_Obj_t * pObj );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs algebraic balancing of the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ivy_Man_t * Ivy_ManBalance( Ivy_Man_t * p, int fUpdateLevel )
{
    Ivy_Man_t * pNew;
    Ivy_Obj_t * pObj, * pDriver;
    Vec_Vec_t * vStore;
    int i, NewNodeId;
    // clean the old manager
    Ivy_ManCleanTravId( p );
    // create the new manager 
    pNew = Ivy_ManStart();
    // map the nodes
    Ivy_ManConst1(p)->TravId = Ivy_EdgeFromNode( Ivy_ManConst1(pNew) );
    Ivy_ManForEachPi( p, pObj, i )
        pObj->TravId = Ivy_EdgeFromNode( Ivy_ObjCreatePi(pNew) );
    // if HAIG is defined, trasfer the pointers to the PIs/latches
//    if ( p->pHaig )
//        Ivy_ManHaigTrasfer( p, pNew );
    // balance the AIG
    vStore = Vec_VecAlloc( 50 );
    Ivy_ManForEachPo( p, pObj, i )
    {
        pDriver   = Ivy_ObjReal( Ivy_ObjChild0(pObj) );
        NewNodeId = Ivy_NodeBalance_rec( pNew, Ivy_Regular(pDriver), vStore, 0, fUpdateLevel );
        NewNodeId = Ivy_EdgeNotCond( NewNodeId, Ivy_IsComplement(pDriver) );
        Ivy_ObjCreatePo( pNew, Ivy_EdgeToNode(pNew, NewNodeId) );
    }
    Vec_VecFree( vStore );
    if ( (i = Ivy_ManCleanup( pNew )) )
    {
//        printf( "Cleanup after balancing removed %d dangling nodes.\n", i );
    }
    // check the resulting AIG
    if ( !Ivy_ManCheck(pNew) )
        printf( "Ivy_ManBalance(): The check has failed.\n" );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Procedure used for sorting the nodes in decreasing order of levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_NodeCompareLevelsDecrease( Ivy_Obj_t ** pp1, Ivy_Obj_t ** pp2 )
{
    int Diff = Ivy_Regular(*pp1)->Level - Ivy_Regular(*pp2)->Level;
    if ( Diff > 0 )
        return -1;
    if ( Diff < 0 ) 
        return 1;
    Diff = Ivy_Regular(*pp1)->Id - Ivy_Regular(*pp2)->Id;
    if ( Diff > 0 )
        return -1;
    if ( Diff < 0 ) 
        return 1;
    return 0; 
}

/**Function*************************************************************

  Synopsis    [Returns the ID of new node constructed.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_NodeBalance_rec( Ivy_Man_t * pNew, Ivy_Obj_t * pObjOld, Vec_Vec_t * vStore, int Level, int fUpdateLevel )
{
    Ivy_Obj_t * pObjNew;
    Vec_Ptr_t * vSuper;
    int i, NewNodeId;
    assert( !Ivy_IsComplement(pObjOld) );
    assert( !Ivy_ObjIsBuf(pObjOld) );
    // return if the result is known
    if ( Ivy_ObjIsConst1(pObjOld) )
        return pObjOld->TravId;
    if ( pObjOld->TravId )
        return pObjOld->TravId;
    assert( Ivy_ObjIsNode(pObjOld) );
    // get the implication supergate
    vSuper = Ivy_NodeBalanceCone( pObjOld, vStore, Level );
    if ( vSuper->nSize == 0 )
    { // it means that the supergate contains two nodes in the opposite polarity
        pObjOld->TravId = Ivy_EdgeFromNode( Ivy_ManConst0(pNew) );
        return pObjOld->TravId;
    }
    if ( vSuper->nSize < 2 )
        printf( "BUG!\n" );
    // for each old node, derive the new well-balanced node
    for ( i = 0; i < vSuper->nSize; i++ )
    {
        NewNodeId = Ivy_NodeBalance_rec( pNew, Ivy_Regular((Ivy_Obj_t *)vSuper->pArray[i]), vStore, Level + 1, fUpdateLevel );
        NewNodeId = Ivy_EdgeNotCond( NewNodeId, Ivy_IsComplement((Ivy_Obj_t *)vSuper->pArray[i]) );
        vSuper->pArray[i] = Ivy_EdgeToNode( pNew, NewNodeId );
    }
    // build the supergate
    pObjNew = Ivy_NodeBalanceBuildSuper( pNew, vSuper, Ivy_ObjType(pObjOld), fUpdateLevel );
    vSuper->nSize = 0;
    // make sure the balanced node is not assigned
    assert( pObjOld->TravId == 0 );
    pObjOld->TravId = Ivy_EdgeFromNode( pObjNew );
//    assert( pObjOld->Level >= Ivy_Regular(pObjNew)->Level );
    return pObjOld->TravId;
}

/**Function*************************************************************

  Synopsis    [Builds implication supergate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ivy_Obj_t * Ivy_NodeBalanceBuildSuper( Ivy_Man_t * p, Vec_Ptr_t * vSuper, Ivy_Type_t Type, int fUpdateLevel )
{
    Ivy_Obj_t * pObj1, * pObj2;
    int LeftBound;
    assert( vSuper->nSize > 1 );
    // sort the new nodes by level in the decreasing order
    Vec_PtrSort( vSuper, (int (*)(void))Ivy_NodeCompareLevelsDecrease );
    // balance the nodes
    while ( vSuper->nSize > 1 )
    {
        // find the left bound on the node to be paired
        LeftBound = (!fUpdateLevel)? 0 : Ivy_NodeBalanceFindLeft( vSuper );
        // find the node that can be shared (if no such node, randomize choice)
        Ivy_NodeBalancePermute( p, vSuper, LeftBound, Type == IVY_EXOR );
        // pull out the last two nodes
        pObj1 = (Ivy_Obj_t *)Vec_PtrPop(vSuper);
        pObj2 = (Ivy_Obj_t *)Vec_PtrPop(vSuper);
        Ivy_NodeBalancePushUniqueOrderByLevel( vSuper, Ivy_Oper(p, pObj1, pObj2, Type) );
    }
    return (Ivy_Obj_t *)Vec_PtrEntry(vSuper, 0);
}

/**Function*************************************************************

  Synopsis    [Collects the nodes of the supergate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_NodeBalanceCone_rec( Ivy_Obj_t * pRoot, Ivy_Obj_t * pObj, Vec_Ptr_t * vSuper )
{
    int RetValue1, RetValue2, i;
    // check if the node is visited
    if ( Ivy_Regular(pObj)->fMarkB )
    {
        // check if the node occurs in the same polarity
        for ( i = 0; i < vSuper->nSize; i++ )
            if ( vSuper->pArray[i] == pObj )
                return 1;
        // check if the node is present in the opposite polarity
        for ( i = 0; i < vSuper->nSize; i++ )
            if ( vSuper->pArray[i] == Ivy_Not(pObj) )
                return -1;
        assert( 0 );
        return 0;
    }
    // if the new node is complemented or a PI, another gate begins
    if ( pObj != pRoot && (Ivy_IsComplement(pObj) || Ivy_ObjType(pObj) != Ivy_ObjType(pRoot) || Ivy_ObjRefs(pObj) > 1 || Vec_PtrSize(vSuper) > 10000) )
    {
        Vec_PtrPush( vSuper, pObj );
        Ivy_Regular(pObj)->fMarkB = 1;
        return 0;
    }
    assert( !Ivy_IsComplement(pObj) );
    assert( Ivy_ObjIsNode(pObj) );
    // go through the branches
    RetValue1 = Ivy_NodeBalanceCone_rec( pRoot, Ivy_ObjReal( Ivy_ObjChild0(pObj) ), vSuper );
    RetValue2 = Ivy_NodeBalanceCone_rec( pRoot, Ivy_ObjReal( Ivy_ObjChild1(pObj) ), vSuper );
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
Vec_Ptr_t * Ivy_NodeBalanceCone( Ivy_Obj_t * pObj, Vec_Vec_t * vStore, int Level )
{
    Vec_Ptr_t * vNodes;
    int RetValue, i;
    assert( !Ivy_IsComplement(pObj) );
    // extend the storage
    if ( Vec_VecSize( vStore ) <= Level )
        Vec_VecPush( vStore, Level, 0 );
    // get the temporary array of nodes
    vNodes = Vec_VecEntry( vStore, Level );
    Vec_PtrClear( vNodes );
    // collect the nodes in the implication supergate
    RetValue = Ivy_NodeBalanceCone_rec( pObj, pObj, vNodes );
    assert( vNodes->nSize > 1 );
    // unmark the visited nodes
    Vec_PtrForEachEntry( Ivy_Obj_t *, vNodes, pObj, i )
        Ivy_Regular(pObj)->fMarkB = 0;
    // if we found the node and its complement in the same implication supergate, 
    // return empty set of nodes (meaning that we should use constant-0 node)
    if ( RetValue == -1 )
        vNodes->nSize = 0;
    return vNodes;
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
int Ivy_NodeBalanceFindLeft( Vec_Ptr_t * vSuper )
{
    Ivy_Obj_t * pObjRight, * pObjLeft;
    int Current;
    // if two or less nodes, pair with the first
    if ( Vec_PtrSize(vSuper) < 3 )
        return 0;
    // set the pointer to the one before the last
    Current = Vec_PtrSize(vSuper) - 2;
    pObjRight = (Ivy_Obj_t *)Vec_PtrEntry( vSuper, Current );
    // go through the nodes to the left of this one
    for ( Current--; Current >= 0; Current-- )
    {
        // get the next node on the left
        pObjLeft = (Ivy_Obj_t *)Vec_PtrEntry( vSuper, Current );
        // if the level of this node is different, quit the loop
        if ( Ivy_Regular(pObjLeft)->Level != Ivy_Regular(pObjRight)->Level )
            break;
    }
    Current++;    
    // get the node, for which the equality holds
    pObjLeft = (Ivy_Obj_t *)Vec_PtrEntry( vSuper, Current );
    assert( Ivy_Regular(pObjLeft)->Level == Ivy_Regular(pObjRight)->Level );
    return Current;
}

/**Function*************************************************************

  Synopsis    [Moves closer to the end the node that is best for sharing.]

  Description [If there is no node with sharing, randomly chooses one of 
  the legal nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_NodeBalancePermute( Ivy_Man_t * p, Vec_Ptr_t * vSuper, int LeftBound, int fExor )
{
    Ivy_Obj_t * pObj1, * pObj2, * pObj3, * pGhost;
    int RightBound, i;
    // get the right bound
    RightBound = Vec_PtrSize(vSuper) - 2;
    assert( LeftBound <= RightBound );
    if ( LeftBound == RightBound )
        return;
    // get the two last nodes
    pObj1 = (Ivy_Obj_t *)Vec_PtrEntry( vSuper, RightBound + 1 );
    pObj2 = (Ivy_Obj_t *)Vec_PtrEntry( vSuper, RightBound     );
    if ( Ivy_Regular(pObj1) == p->pConst1 || Ivy_Regular(pObj2) == p->pConst1 )
        return;
    // find the first node that can be shared
    for ( i = RightBound; i >= LeftBound; i-- )
    {
        pObj3 = (Ivy_Obj_t *)Vec_PtrEntry( vSuper, i );
        if ( Ivy_Regular(pObj3) == p->pConst1 )
        {
            Vec_PtrWriteEntry( vSuper, i,          pObj2 );
            Vec_PtrWriteEntry( vSuper, RightBound, pObj3 );
            return;
        }
        pGhost = Ivy_ObjCreateGhost( p, pObj1, pObj3, fExor? IVY_EXOR : IVY_AND, IVY_INIT_NONE );
        if ( Ivy_TableLookup( p, pGhost ) )
        {
            if ( pObj3 == pObj2 )
                return;
            Vec_PtrWriteEntry( vSuper, i,          pObj2 );
            Vec_PtrWriteEntry( vSuper, RightBound, pObj3 );
            return;
        }
    }
/*
    // we did not find the node to share, randomize choice
    {
        int Choice = rand() % (RightBound - LeftBound + 1);
        pObj3 = Vec_PtrEntry( vSuper, LeftBound + Choice );
        if ( pObj3 == pObj2 )
            return;
        Vec_PtrWriteEntry( vSuper, LeftBound + Choice, pObj2 );
        Vec_PtrWriteEntry( vSuper, RightBound,         pObj3 );
    }
*/
}

/**Function*************************************************************

  Synopsis    [Inserts a new node in the order by levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_NodeBalancePushUniqueOrderByLevel( Vec_Ptr_t * vStore, Ivy_Obj_t * pObj )
{
    Ivy_Obj_t * pObj1, * pObj2;
    int i;
    if ( Vec_PtrPushUnique(vStore, pObj) )
        return;
    // find the p of the node
    for ( i = vStore->nSize-1; i > 0; i-- )
    {
        pObj1 = (Ivy_Obj_t *)vStore->pArray[i  ];
        pObj2 = (Ivy_Obj_t *)vStore->pArray[i-1];
        if ( Ivy_Regular(pObj1)->Level <= Ivy_Regular(pObj2)->Level )
            break;
        vStore->pArray[i  ] = pObj2;
        vStore->pArray[i-1] = pObj1;
    }
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

