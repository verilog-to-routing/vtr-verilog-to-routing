/**CFile****************************************************************

  FileName    [hopBalance.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Minimalistic And-Inverter Graph package.]

  Synopsis    [Algebraic AIG balancing.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 11, 2006.]

  Revision    [$Id: hopBalance.c,v 1.00 2006/05/11 00:00:00 alanmi Exp $]

***********************************************************************/

#include "hop.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Hop_Obj_t * Hop_NodeBalance_rec( Hop_Man_t * pNew, Hop_Obj_t * pObj, Vec_Vec_t * vStore, int Level, int fUpdateLevel );
static Vec_Ptr_t * Hop_NodeBalanceCone( Hop_Obj_t * pObj, Vec_Vec_t * vStore, int Level );
static int         Hop_NodeBalanceFindLeft( Vec_Ptr_t * vSuper );
static void        Hop_NodeBalancePermute( Hop_Man_t * p, Vec_Ptr_t * vSuper, int LeftBound, int fExor );
static void        Hop_NodeBalancePushUniqueOrderByLevel( Vec_Ptr_t * vStore, Hop_Obj_t * pObj );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs algebraic balancing of the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Man_t * Hop_ManBalance( Hop_Man_t * p, int fUpdateLevel )
{
    Hop_Man_t * pNew;
    Hop_Obj_t * pObj, * pObjNew;
    Vec_Vec_t * vStore;
    int i;
    // create the new manager 
    pNew = Hop_ManStart();
    pNew->fRefCount = 0;
    // map the PI nodes
    Hop_ManCleanData( p );
    Hop_ManConst1(p)->pData = Hop_ManConst1(pNew);
    Hop_ManForEachPi( p, pObj, i )
        pObj->pData = Hop_ObjCreatePi(pNew);
    // balance the AIG
    vStore = Vec_VecAlloc( 50 );
    Hop_ManForEachPo( p, pObj, i )
    {
        pObjNew = Hop_NodeBalance_rec( pNew, Hop_ObjFanin0(pObj), vStore, 0, fUpdateLevel );
        Hop_ObjCreatePo( pNew, Hop_NotCond( pObjNew, Hop_ObjFaninC0(pObj) ) );
    }
    Vec_VecFree( vStore );
    // remove dangling nodes
//    Hop_ManCreateRefs( pNew );
//    if ( i = Hop_ManCleanup( pNew ) )
//        printf( "Cleanup after balancing removed %d dangling nodes.\n", i );
    // check the resulting AIG
    if ( !Hop_ManCheck(pNew) )
        printf( "Hop_ManBalance(): The check has failed.\n" );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Returns the new node constructed.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Hop_NodeBalance_rec( Hop_Man_t * pNew, Hop_Obj_t * pObjOld, Vec_Vec_t * vStore, int Level, int fUpdateLevel )
{
    Hop_Obj_t * pObjNew;
    Vec_Ptr_t * vSuper;
    int i;
    assert( !Hop_IsComplement(pObjOld) );
    // return if the result is known
    if ( pObjOld->pData )
        return (Hop_Obj_t *)pObjOld->pData;
    assert( Hop_ObjIsNode(pObjOld) );
    // get the implication supergate
    vSuper = Hop_NodeBalanceCone( pObjOld, vStore, Level );
    // check if supergate contains two nodes in the opposite polarity
    if ( vSuper->nSize == 0 )
        return (Hop_Obj_t *)(pObjOld->pData = Hop_ManConst0(pNew));
    if ( Vec_PtrSize(vSuper) < 2 )
        printf( "BUG!\n" );
    // for each old node, derive the new well-balanced node
    for ( i = 0; i < Vec_PtrSize(vSuper); i++ )
    {
        pObjNew = Hop_NodeBalance_rec( pNew, Hop_Regular((Hop_Obj_t *)vSuper->pArray[i]), vStore, Level + 1, fUpdateLevel );
        vSuper->pArray[i] = Hop_NotCond( pObjNew, Hop_IsComplement((Hop_Obj_t *)vSuper->pArray[i]) );
    }
    // build the supergate
    pObjNew = Hop_NodeBalanceBuildSuper( pNew, vSuper, Hop_ObjType(pObjOld), fUpdateLevel );
    // make sure the balanced node is not assigned
//    assert( pObjOld->Level >= Hop_Regular(pObjNew)->Level );
    assert( pObjOld->pData == NULL );
    return (Hop_Obj_t *)(pObjOld->pData = pObjNew);
}

/**Function*************************************************************

  Synopsis    [Collects the nodes of the supergate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Hop_NodeBalanceCone_rec( Hop_Obj_t * pRoot, Hop_Obj_t * pObj, Vec_Ptr_t * vSuper )
{
    int RetValue1, RetValue2, i;
    // check if the node is visited
    if ( Hop_Regular(pObj)->fMarkB )
    {
        // check if the node occurs in the same polarity
        for ( i = 0; i < vSuper->nSize; i++ )
            if ( vSuper->pArray[i] == pObj )
                return 1;
        // check if the node is present in the opposite polarity
        for ( i = 0; i < vSuper->nSize; i++ )
            if ( vSuper->pArray[i] == Hop_Not(pObj) )
                return -1;
        assert( 0 );
        return 0;
    }
    // if the new node is complemented or a PI, another gate begins
    if ( pObj != pRoot && (Hop_IsComplement(pObj) || Hop_ObjType(pObj) != Hop_ObjType(pRoot) || Hop_ObjRefs(pObj) > 1 || Vec_PtrSize(vSuper) > 10000) )
    {
        Vec_PtrPush( vSuper, pObj );
        Hop_Regular(pObj)->fMarkB = 1;
        return 0;
    }
    assert( !Hop_IsComplement(pObj) );
    assert( Hop_ObjIsNode(pObj) );
    // go through the branches
    RetValue1 = Hop_NodeBalanceCone_rec( pRoot, Hop_ObjChild0(pObj), vSuper );
    RetValue2 = Hop_NodeBalanceCone_rec( pRoot, Hop_ObjChild1(pObj), vSuper );
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
Vec_Ptr_t * Hop_NodeBalanceCone( Hop_Obj_t * pObj, Vec_Vec_t * vStore, int Level )
{
    Vec_Ptr_t * vNodes;
    int RetValue, i;
    assert( !Hop_IsComplement(pObj) );
    // extend the storage
    if ( Vec_VecSize( vStore ) <= Level )
        Vec_VecPush( vStore, Level, 0 );
    // get the temporary array of nodes
    vNodes = Vec_VecEntry( vStore, Level );
    Vec_PtrClear( vNodes );
    // collect the nodes in the implication supergate
    RetValue = Hop_NodeBalanceCone_rec( pObj, pObj, vNodes );
    assert( vNodes->nSize > 1 );
    // unmark the visited nodes
    Vec_PtrForEachEntry( Hop_Obj_t *, vNodes, pObj, i )
        Hop_Regular(pObj)->fMarkB = 0;
    // if we found the node and its complement in the same implication supergate, 
    // return empty set of nodes (meaning that we should use constant-0 node)
    if ( RetValue == -1 )
        vNodes->nSize = 0;
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Procedure used for sorting the nodes in decreasing order of levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Hop_NodeCompareLevelsDecrease( Hop_Obj_t ** pp1, Hop_Obj_t ** pp2 )
{
    int Diff = Hop_ObjLevel(Hop_Regular(*pp1)) - Hop_ObjLevel(Hop_Regular(*pp2));
    if ( Diff > 0 )
        return -1;
    if ( Diff < 0 ) 
        return 1;
    Diff = Hop_Regular(*pp1)->Id - Hop_Regular(*pp2)->Id;
    if ( Diff > 0 )
        return -1;
    if ( Diff < 0 ) 
        return 1;
    return 0; 
}

/**Function*************************************************************

  Synopsis    [Builds implication supergate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Hop_NodeBalanceBuildSuper( Hop_Man_t * p, Vec_Ptr_t * vSuper, Hop_Type_t Type, int fUpdateLevel )
{
    Hop_Obj_t * pObj1, * pObj2;
    int LeftBound;
    assert( vSuper->nSize > 1 );
    // sort the new nodes by level in the decreasing order
    Vec_PtrSort( vSuper, (int (*)(const void *, const void *))Hop_NodeCompareLevelsDecrease );
    // balance the nodes
    while ( vSuper->nSize > 1 )
    {
        // find the left bound on the node to be paired
        LeftBound = (!fUpdateLevel)? 0 : Hop_NodeBalanceFindLeft( vSuper );
        // find the node that can be shared (if no such node, randomize choice)
        Hop_NodeBalancePermute( p, vSuper, LeftBound, Type == AIG_EXOR );
        // pull out the last two nodes
        pObj1 = (Hop_Obj_t *)Vec_PtrPop(vSuper);
        pObj2 = (Hop_Obj_t *)Vec_PtrPop(vSuper);
        Hop_NodeBalancePushUniqueOrderByLevel( vSuper, Hop_Oper(p, pObj1, pObj2, Type) );
    }
    return (Hop_Obj_t *)Vec_PtrEntry(vSuper, 0);
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
int Hop_NodeBalanceFindLeft( Vec_Ptr_t * vSuper )
{
    Hop_Obj_t * pObjRight, * pObjLeft;
    int Current;
    // if two or less nodes, pair with the first
    if ( Vec_PtrSize(vSuper) < 3 )
        return 0;
    // set the pointer to the one before the last
    Current = Vec_PtrSize(vSuper) - 2;
    pObjRight = (Hop_Obj_t *)Vec_PtrEntry( vSuper, Current );
    // go through the nodes to the left of this one
    for ( Current--; Current >= 0; Current-- )
    {
        // get the next node on the left
        pObjLeft = (Hop_Obj_t *)Vec_PtrEntry( vSuper, Current );
        // if the level of this node is different, quit the loop
        if ( Hop_ObjLevel(Hop_Regular(pObjLeft)) != Hop_ObjLevel(Hop_Regular(pObjRight)) )
            break;
    }
    Current++;    
    // get the node, for which the equality holds
    pObjLeft = (Hop_Obj_t *)Vec_PtrEntry( vSuper, Current );
    assert( Hop_ObjLevel(Hop_Regular(pObjLeft)) == Hop_ObjLevel(Hop_Regular(pObjRight)) );
    return Current;
}

/**Function*************************************************************

  Synopsis    [Moves closer to the end the node that is best for sharing.]

  Description [If there is no node with sharing, randomly chooses one of 
  the legal nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hop_NodeBalancePermute( Hop_Man_t * p, Vec_Ptr_t * vSuper, int LeftBound, int fExor )
{
    Hop_Obj_t * pObj1, * pObj2, * pObj3, * pGhost;
    int RightBound, i;
    // get the right bound
    RightBound = Vec_PtrSize(vSuper) - 2;
    assert( LeftBound <= RightBound );
    if ( LeftBound == RightBound )
        return;
    // get the two last nodes
    pObj1 = (Hop_Obj_t *)Vec_PtrEntry( vSuper, RightBound + 1 );
    pObj2 = (Hop_Obj_t *)Vec_PtrEntry( vSuper, RightBound     );
    if ( Hop_Regular(pObj1) == p->pConst1 || Hop_Regular(pObj2) == p->pConst1 )
        return;
    // find the first node that can be shared
    for ( i = RightBound; i >= LeftBound; i-- )
    {
        pObj3 = (Hop_Obj_t *)Vec_PtrEntry( vSuper, i );
        if ( Hop_Regular(pObj3) == p->pConst1 )
        {
            Vec_PtrWriteEntry( vSuper, i,          pObj2 );
            Vec_PtrWriteEntry( vSuper, RightBound, pObj3 );
            return;
        }
        pGhost = Hop_ObjCreateGhost( p, pObj1, pObj3, fExor? AIG_EXOR : AIG_AND );
        if ( Hop_TableLookup( p, pGhost ) )
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
void Hop_NodeBalancePushUniqueOrderByLevel( Vec_Ptr_t * vStore, Hop_Obj_t * pObj )
{
    Hop_Obj_t * pObj1, * pObj2;
    int i;
    if ( Vec_PtrPushUnique(vStore, pObj) )
        return;
    // find the p of the node
    for ( i = vStore->nSize-1; i > 0; i-- )
    {
        pObj1 = (Hop_Obj_t *)vStore->pArray[i  ];
        pObj2 = (Hop_Obj_t *)vStore->pArray[i-1];
        if ( Hop_ObjLevel(Hop_Regular(pObj1)) <= Hop_ObjLevel(Hop_Regular(pObj2)) )
            break;
        vStore->pArray[i  ] = pObj2;
        vStore->pArray[i-1] = pObj1;
    }
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

