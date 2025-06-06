/**CFile****************************************************************

  FileName    [ivyUtil.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [And-Inverter Graph package.]

  Synopsis    [Various procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 11, 2006.]

  Revision    [$Id: ivyUtil.c,v 1.00 2006/05/11 00:00:00 alanmi Exp $]

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

  Synopsis    [Increments the current traversal ID of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ManIncrementTravId( Ivy_Man_t * p )
{
    if ( p->nTravIds >= (1<<30)-1 - 1000 )
        Ivy_ManCleanTravId( p );
    p->nTravIds++;
}

/**Function*************************************************************

  Synopsis    [Sets the DFS ordering of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ManCleanTravId( Ivy_Man_t * p )
{
    Ivy_Obj_t * pObj;
    int i;
    p->nTravIds = 1;
    Ivy_ManForEachObj( p, pObj, i )
        pObj->TravId = 0;
}

/**Function*************************************************************

  Synopsis    [Computes truth table of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ManCollectCut_rec( Ivy_Man_t * p, Ivy_Obj_t * pNode, Vec_Int_t * vNodes )
{
    if ( pNode->fMarkA )
        return;
    pNode->fMarkA = 1;
    assert( Ivy_ObjIsAnd(pNode) || Ivy_ObjIsExor(pNode) );
    Ivy_ManCollectCut_rec( p, Ivy_ObjFanin0(pNode), vNodes );
    Ivy_ManCollectCut_rec( p, Ivy_ObjFanin1(pNode), vNodes );
    Vec_IntPush( vNodes, pNode->Id );
}

/**Function*************************************************************

  Synopsis    [Computes truth table of the cut.]

  Description [Does not modify the array of leaves. Uses array vTruth to store 
  temporary truth tables. The returned pointer should be used immediately.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ManCollectCut( Ivy_Man_t * p, Ivy_Obj_t * pRoot, Vec_Int_t * vLeaves, Vec_Int_t * vNodes )
{
    int i, Leaf;
    // collect and mark the leaves
    Vec_IntClear( vNodes );
    Vec_IntForEachEntry( vLeaves, Leaf, i )
    {
        Vec_IntPush( vNodes, Leaf );
        Ivy_ManObj(p, Leaf)->fMarkA = 1;
    }
    // collect and mark the nodes
    Ivy_ManCollectCut_rec( p, pRoot, vNodes );
    // clean the nodes
    Vec_IntForEachEntry( vNodes, Leaf, i )
        Ivy_ManObj(p, Leaf)->fMarkA = 0;
}

/**Function*************************************************************

  Synopsis    [Returns the pointer to the truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Ivy_ObjGetTruthStore( int ObjNum, Vec_Int_t * vTruth )
{
   return ((unsigned *)Vec_IntArray(vTruth)) + 8 * ObjNum;
}

/**Function*************************************************************

  Synopsis    [Computes truth table of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ManCutTruthOne( Ivy_Man_t * p, Ivy_Obj_t * pNode, Vec_Int_t * vTruth, int nWords )
{
    unsigned * pTruth, * pTruth0, * pTruth1;
    int i;
    pTruth  = Ivy_ObjGetTruthStore( pNode->TravId, vTruth );
    pTruth0 = Ivy_ObjGetTruthStore( Ivy_ObjFanin0(pNode)->TravId, vTruth );
    pTruth1 = Ivy_ObjGetTruthStore( Ivy_ObjFanin1(pNode)->TravId, vTruth );
    if ( Ivy_ObjIsExor(pNode) )
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = pTruth0[i] ^ pTruth1[i];
    else if ( !Ivy_ObjFaninC0(pNode) && !Ivy_ObjFaninC1(pNode) )
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = pTruth0[i] & pTruth1[i];
    else if ( !Ivy_ObjFaninC0(pNode) && Ivy_ObjFaninC1(pNode) )
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = pTruth0[i] & ~pTruth1[i];
    else if ( Ivy_ObjFaninC0(pNode) && !Ivy_ObjFaninC1(pNode) )
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = ~pTruth0[i] & pTruth1[i];
    else // if ( Ivy_ObjFaninC0(pNode) && Ivy_ObjFaninC1(pNode) )
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = ~pTruth0[i] & ~pTruth1[i];
}

/**Function*************************************************************

  Synopsis    [Computes truth table of the cut.]

  Description [Does not modify the array of leaves. Uses array vTruth to store 
  temporary truth tables. The returned pointer should be used immediately.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Ivy_ManCutTruth( Ivy_Man_t * p, Ivy_Obj_t * pRoot, Vec_Int_t * vLeaves, Vec_Int_t * vNodes, Vec_Int_t * vTruth )
{
    static unsigned uTruths[8][8] = { // elementary truth tables
        { 0xAAAAAAAA,0xAAAAAAAA,0xAAAAAAAA,0xAAAAAAAA,0xAAAAAAAA,0xAAAAAAAA,0xAAAAAAAA,0xAAAAAAAA },
        { 0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC },
        { 0xF0F0F0F0,0xF0F0F0F0,0xF0F0F0F0,0xF0F0F0F0,0xF0F0F0F0,0xF0F0F0F0,0xF0F0F0F0,0xF0F0F0F0 },
        { 0xFF00FF00,0xFF00FF00,0xFF00FF00,0xFF00FF00,0xFF00FF00,0xFF00FF00,0xFF00FF00,0xFF00FF00 },
        { 0xFFFF0000,0xFFFF0000,0xFFFF0000,0xFFFF0000,0xFFFF0000,0xFFFF0000,0xFFFF0000,0xFFFF0000 }, 
        { 0x00000000,0xFFFFFFFF,0x00000000,0xFFFFFFFF,0x00000000,0xFFFFFFFF,0x00000000,0xFFFFFFFF }, 
        { 0x00000000,0x00000000,0xFFFFFFFF,0xFFFFFFFF,0x00000000,0x00000000,0xFFFFFFFF,0xFFFFFFFF }, 
        { 0x00000000,0x00000000,0x00000000,0x00000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF } 
    };
    int i, Leaf;
    // collect the cut
    Ivy_ManCollectCut( p, pRoot, vLeaves, vNodes );
    // set the node numbers
    Vec_IntForEachEntry( vNodes, Leaf, i )
        Ivy_ManObj(p, Leaf)->TravId = i;
    // alloc enough memory
    Vec_IntClear( vTruth );
    Vec_IntGrow( vTruth, 8 * Vec_IntSize(vNodes) );
    // set the elementary truth tables
    Vec_IntForEachEntry( vLeaves, Leaf, i )
        memcpy( Ivy_ObjGetTruthStore(i, vTruth), uTruths[i], 8 * sizeof(unsigned) );
    // compute truths for other nodes
    Vec_IntForEachEntryStart( vNodes, Leaf, i, Vec_IntSize(vLeaves) )
        Ivy_ManCutTruthOne( p, Ivy_ManObj(p, Leaf), vTruth, 8 );
    return Ivy_ObjGetTruthStore( pRoot->TravId, vTruth );
}

/**Function*************************************************************

  Synopsis    [Collect the latches.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Ivy_ManLatches( Ivy_Man_t * p )
{
    Vec_Int_t * vLatches;
    Ivy_Obj_t * pObj;
    int i;
    vLatches = Vec_IntAlloc( Ivy_ManLatchNum(p) );
    Ivy_ManForEachLatch( p, pObj, i )
        Vec_IntPush( vLatches, pObj->Id );
    return vLatches;
}

/**Function*************************************************************

  Synopsis    [Collect the latches.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_ManLevels( Ivy_Man_t * p )
{
    Ivy_Obj_t * pObj;
    int i, LevelMax = 0;
    Ivy_ManForEachPo( p, pObj, i )
        LevelMax = IVY_MAX( LevelMax, (int)Ivy_ObjFanin0(pObj)->Level );
    return LevelMax;
}

/**Function*************************************************************

  Synopsis    [Collect the latches.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_ManResetLevels_rec( Ivy_Obj_t * pObj )
{
    if ( pObj->Level || Ivy_ObjIsCi(pObj) || Ivy_ObjIsConst1(pObj) )
        return pObj->Level;
    if ( Ivy_ObjIsBuf(pObj) )
        return pObj->Level = Ivy_ManResetLevels_rec( Ivy_ObjFanin0(pObj) );
    assert( Ivy_ObjIsNode(pObj) );
    Ivy_ManResetLevels_rec( Ivy_ObjFanin0(pObj) );
    Ivy_ManResetLevels_rec( Ivy_ObjFanin1(pObj) );
    return pObj->Level = Ivy_ObjLevelNew( pObj );
}

/**Function*************************************************************

  Synopsis    [Collect the latches.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ManResetLevels( Ivy_Man_t * p )
{
    Ivy_Obj_t * pObj;
    int i;
    Ivy_ManForEachObj( p, pObj, i )
        pObj->Level = 0;
    Ivy_ManForEachCo( p, pObj, i )
        Ivy_ManResetLevels_rec( Ivy_ObjFanin0(pObj) );
}

/**Function*************************************************************

  Synopsis    [References/references the node and returns MFFC size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_ObjRefDeref( Ivy_Man_t * p, Ivy_Obj_t * pNode, int fReference, int fLabel )
{
    Ivy_Obj_t * pNode0, * pNode1;
    int Counter;
    // label visited nodes
    if ( fLabel )
        Ivy_ObjSetTravIdCurrent( p, pNode );
    // skip the CI
    if ( Ivy_ObjIsPi(pNode) )
        return 0;
    assert( Ivy_ObjIsNode(pNode) || Ivy_ObjIsBuf(pNode) || Ivy_ObjIsLatch(pNode) );
    // process the internal node
    pNode0 = Ivy_ObjFanin0(pNode);
    pNode1 = Ivy_ObjFanin1(pNode);
    Counter = Ivy_ObjIsNode(pNode);
    if ( fReference )
    {
        if ( pNode0->nRefs++ == 0 )
            Counter += Ivy_ObjRefDeref( p, pNode0, fReference, fLabel );
        if ( pNode1 && pNode1->nRefs++ == 0 )
            Counter += Ivy_ObjRefDeref( p, pNode1, fReference, fLabel );
    }
    else
    {
        assert( pNode0->nRefs > 0 );
        assert( pNode1 == NULL || pNode1->nRefs > 0 );
        if ( --pNode0->nRefs == 0 )
            Counter += Ivy_ObjRefDeref( p, pNode0, fReference, fLabel );
        if ( pNode1 && --pNode1->nRefs == 0 )
            Counter += Ivy_ObjRefDeref( p, pNode1, fReference, fLabel );
    }
    return Counter;
}


/**Function*************************************************************

  Synopsis    [Labels MFFC with the current label.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_ObjMffcLabel( Ivy_Man_t * p, Ivy_Obj_t * pNode )
{
    int nConeSize1, nConeSize2;
    assert( !Ivy_IsComplement( pNode ) );
    assert( Ivy_ObjIsNode( pNode ) );
    nConeSize1 = Ivy_ObjRefDeref( p, pNode, 0, 1 ); // dereference
    nConeSize2 = Ivy_ObjRefDeref( p, pNode, 1, 0 ); // reference
    assert( nConeSize1 == nConeSize2 );
    assert( nConeSize1 > 0 );
    return nConeSize1;
}

/**Function*************************************************************

  Synopsis    [Recursively updates fanout levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ObjUpdateLevel_rec( Ivy_Man_t * p, Ivy_Obj_t * pObj )
{
    Ivy_Obj_t * pFanout;
    Vec_Ptr_t * vFanouts;
    int i, LevelNew;
    assert( p->fFanout );
    assert( Ivy_ObjIsNode(pObj) );
    vFanouts = Vec_PtrAlloc( 10 );
    Ivy_ObjForEachFanout( p, pObj, vFanouts, pFanout, i )
    {
        if ( Ivy_ObjIsCo(pFanout) )
        {
//            assert( (int)Ivy_ObjFanin0(pFanout)->Level <= p->nLevelMax );
            continue;
        }
        LevelNew = Ivy_ObjLevelNew( pFanout );
        if ( (int)pFanout->Level == LevelNew )
            continue;
        pFanout->Level = LevelNew;
        Ivy_ObjUpdateLevel_rec( p, pFanout );
    }
    Vec_PtrFree( vFanouts );
}

/**Function*************************************************************

  Synopsis    [Compute the new required level.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_ObjLevelRNew( Ivy_Man_t * p, Ivy_Obj_t * pObj )
{
    Ivy_Obj_t * pFanout;
    Vec_Ptr_t * vFanouts;
    int i, Required, LevelNew = 1000000;
    assert( p->fFanout && p->vRequired );
    vFanouts = Vec_PtrAlloc( 10 );
    Ivy_ObjForEachFanout( p, pObj, vFanouts, pFanout, i )
    {
        Required = Vec_IntEntry(p->vRequired, pFanout->Id);
        LevelNew = IVY_MIN( LevelNew, Required );
    }
    Vec_PtrFree( vFanouts );
    return LevelNew - 1;
}

/**Function*************************************************************

  Synopsis    [Recursively updates fanout levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ObjUpdateLevelR_rec( Ivy_Man_t * p, Ivy_Obj_t * pObj, int ReqNew )
{
    Ivy_Obj_t * pFanin;
    if ( Ivy_ObjIsConst1(pObj) || Ivy_ObjIsCi(pObj) )
        return;
    assert( Ivy_ObjIsNode(pObj) || Ivy_ObjIsBuf(pObj) );
    // process the first fanin
    pFanin = Ivy_ObjFanin0(pObj);
    if ( Vec_IntEntry(p->vRequired, pFanin->Id) > ReqNew - 1 )
    {
        Vec_IntWriteEntry( p->vRequired, pFanin->Id, ReqNew - 1 );
        Ivy_ObjUpdateLevelR_rec( p, pFanin, ReqNew - 1 );
    }
    if ( Ivy_ObjIsBuf(pObj) )
        return;
    // process the second fanin
    pFanin = Ivy_ObjFanin1(pObj);
    if ( Vec_IntEntry(p->vRequired, pFanin->Id) > ReqNew - 1 )
    {
        Vec_IntWriteEntry( p->vRequired, pFanin->Id, ReqNew - 1 );
        Ivy_ObjUpdateLevelR_rec( p, pFanin, ReqNew - 1 );
    }
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the node is the root of MUX or EXOR/NEXOR.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_ObjIsMuxType( Ivy_Obj_t * pNode )
{
    Ivy_Obj_t * pNode0, * pNode1;
    // check that the node is regular
    assert( !Ivy_IsComplement(pNode) );
    // if the node is not AND, this is not MUX
    if ( !Ivy_ObjIsAnd(pNode) )
        return 0;
    // if the children are not complemented, this is not MUX
    if ( !Ivy_ObjFaninC0(pNode) || !Ivy_ObjFaninC1(pNode) )
        return 0;
    // get children
    pNode0 = Ivy_ObjFanin0(pNode);
    pNode1 = Ivy_ObjFanin1(pNode);
    // if the children are not ANDs, this is not MUX
    if ( !Ivy_ObjIsAnd(pNode0) || !Ivy_ObjIsAnd(pNode1) )
        return 0;
    // otherwise the node is MUX iff it has a pair of equal grandchildren
    return (Ivy_ObjFaninId0(pNode0) == Ivy_ObjFaninId0(pNode1) && (Ivy_ObjFaninC0(pNode0) ^ Ivy_ObjFaninC0(pNode1))) || 
           (Ivy_ObjFaninId0(pNode0) == Ivy_ObjFaninId1(pNode1) && (Ivy_ObjFaninC0(pNode0) ^ Ivy_ObjFaninC1(pNode1))) ||
           (Ivy_ObjFaninId1(pNode0) == Ivy_ObjFaninId0(pNode1) && (Ivy_ObjFaninC1(pNode0) ^ Ivy_ObjFaninC0(pNode1))) ||
           (Ivy_ObjFaninId1(pNode0) == Ivy_ObjFaninId1(pNode1) && (Ivy_ObjFaninC1(pNode0) ^ Ivy_ObjFaninC1(pNode1)));
}

/**Function*************************************************************

  Synopsis    [Recognizes what nodes are control and data inputs of a MUX.]

  Description [If the node is a MUX, returns the control variable C.
  Assigns nodes T and E to be the then and else variables of the MUX. 
  Node C is never complemented. Nodes T and E can be complemented.
  This function also recognizes EXOR/NEXOR gates as MUXes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ivy_Obj_t * Ivy_ObjRecognizeMux( Ivy_Obj_t * pNode, Ivy_Obj_t ** ppNodeT, Ivy_Obj_t ** ppNodeE )
{
    Ivy_Obj_t * pNode0, * pNode1;
    assert( !Ivy_IsComplement(pNode) );
    assert( Ivy_ObjIsMuxType(pNode) );
    // get children
    pNode0 = Ivy_ObjFanin0(pNode);
    pNode1 = Ivy_ObjFanin1(pNode);
    // find the control variable
//    if ( pNode1->p1 == Fraig_Not(pNode2->p1) )
    if ( Ivy_ObjFaninId0(pNode0) == Ivy_ObjFaninId0(pNode1) && (Ivy_ObjFaninC0(pNode0) ^ Ivy_ObjFaninC0(pNode1)) )
    {
//        if ( Fraig_IsComplement(pNode1->p1) )
        if ( Ivy_ObjFaninC0(pNode0) )
        { // pNode2->p1 is positive phase of C
            *ppNodeT = Ivy_Not(Ivy_ObjChild1(pNode1));//pNode2->p2);
            *ppNodeE = Ivy_Not(Ivy_ObjChild1(pNode0));//pNode1->p2);
            return Ivy_ObjChild0(pNode1);//pNode2->p1;
        }
        else
        { // pNode1->p1 is positive phase of C
            *ppNodeT = Ivy_Not(Ivy_ObjChild1(pNode0));//pNode1->p2);
            *ppNodeE = Ivy_Not(Ivy_ObjChild1(pNode1));//pNode2->p2);
            return Ivy_ObjChild0(pNode0);//pNode1->p1;
        }
    }
//    else if ( pNode1->p1 == Fraig_Not(pNode2->p2) )
    else if ( Ivy_ObjFaninId0(pNode0) == Ivy_ObjFaninId1(pNode1) && (Ivy_ObjFaninC0(pNode0) ^ Ivy_ObjFaninC1(pNode1)) )
    {
//        if ( Fraig_IsComplement(pNode1->p1) )
        if ( Ivy_ObjFaninC0(pNode0) )
        { // pNode2->p2 is positive phase of C
            *ppNodeT = Ivy_Not(Ivy_ObjChild0(pNode1));//pNode2->p1);
            *ppNodeE = Ivy_Not(Ivy_ObjChild1(pNode0));//pNode1->p2);
            return Ivy_ObjChild1(pNode1);//pNode2->p2;
        }
        else
        { // pNode1->p1 is positive phase of C
            *ppNodeT = Ivy_Not(Ivy_ObjChild1(pNode0));//pNode1->p2);
            *ppNodeE = Ivy_Not(Ivy_ObjChild0(pNode1));//pNode2->p1);
            return Ivy_ObjChild0(pNode0);//pNode1->p1;
        }
    }
//    else if ( pNode1->p2 == Fraig_Not(pNode2->p1) )
    else if ( Ivy_ObjFaninId1(pNode0) == Ivy_ObjFaninId0(pNode1) && (Ivy_ObjFaninC1(pNode0) ^ Ivy_ObjFaninC0(pNode1)) )
    {
//        if ( Fraig_IsComplement(pNode1->p2) )
        if ( Ivy_ObjFaninC1(pNode0) )
        { // pNode2->p1 is positive phase of C
            *ppNodeT = Ivy_Not(Ivy_ObjChild1(pNode1));//pNode2->p2);
            *ppNodeE = Ivy_Not(Ivy_ObjChild0(pNode0));//pNode1->p1);
            return Ivy_ObjChild0(pNode1);//pNode2->p1;
        }
        else
        { // pNode1->p2 is positive phase of C
            *ppNodeT = Ivy_Not(Ivy_ObjChild0(pNode0));//pNode1->p1);
            *ppNodeE = Ivy_Not(Ivy_ObjChild1(pNode1));//pNode2->p2);
            return Ivy_ObjChild1(pNode0);//pNode1->p2;
        }
    }
//    else if ( pNode1->p2 == Fraig_Not(pNode2->p2) )
    else if ( Ivy_ObjFaninId1(pNode0) == Ivy_ObjFaninId1(pNode1) && (Ivy_ObjFaninC1(pNode0) ^ Ivy_ObjFaninC1(pNode1)) )
    {
//        if ( Fraig_IsComplement(pNode1->p2) )
        if ( Ivy_ObjFaninC1(pNode0) )
        { // pNode2->p2 is positive phase of C
            *ppNodeT = Ivy_Not(Ivy_ObjChild0(pNode1));//pNode2->p1);
            *ppNodeE = Ivy_Not(Ivy_ObjChild0(pNode0));//pNode1->p1);
            return Ivy_ObjChild1(pNode1);//pNode2->p2;
        }
        else
        { // pNode1->p2 is positive phase of C
            *ppNodeT = Ivy_Not(Ivy_ObjChild0(pNode0));//pNode1->p1);
            *ppNodeE = Ivy_Not(Ivy_ObjChild0(pNode1));//pNode2->p1);
            return Ivy_ObjChild1(pNode0);//pNode1->p2;
        }
    }
    assert( 0 ); // this is not MUX
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Returns the real fanin.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ivy_Obj_t * Ivy_ObjReal( Ivy_Obj_t * pObj )
{
    Ivy_Obj_t * pFanin;
    if ( pObj == NULL || !Ivy_ObjIsBuf( Ivy_Regular(pObj) ) )
        return pObj;
    pFanin = Ivy_ObjReal( Ivy_ObjChild0(Ivy_Regular(pObj)) );
    return Ivy_NotCond( pFanin, Ivy_IsComplement(pObj) );
}

/**Function*************************************************************

  Synopsis    [Prints node in HAIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ObjPrintVerbose( Ivy_Man_t * p, Ivy_Obj_t * pObj, int fHaig )
{
    Ivy_Obj_t * pTemp;
    int fShowFanouts = 0;
    assert( !Ivy_IsComplement(pObj) );
    printf( "Node %5d : ", Ivy_ObjId(pObj) );
    if ( Ivy_ObjIsConst1(pObj) )
        printf( "constant 1" );
    else if ( Ivy_ObjIsPi(pObj) )
        printf( "PI" );
    else if ( Ivy_ObjIsPo(pObj) )
        printf( "PO" );
    else if ( Ivy_ObjIsLatch(pObj) )
        printf( "latch (%d%s)", Ivy_ObjFanin0(pObj)->Id, (Ivy_ObjFaninC0(pObj)? "\'" : " ") );
    else if ( Ivy_ObjIsBuf(pObj) )
        printf( "buffer (%d%s)", Ivy_ObjFanin0(pObj)->Id, (Ivy_ObjFaninC0(pObj)? "\'" : " ") );
    else
        printf( "AND( %5d%s, %5d%s )", 
            Ivy_ObjFanin0(pObj)->Id, (Ivy_ObjFaninC0(pObj)? "\'" : " "), 
            Ivy_ObjFanin1(pObj)->Id, (Ivy_ObjFaninC1(pObj)? "\'" : " ") );
    printf( " (refs = %3d)", Ivy_ObjRefs(pObj) );
    if ( fShowFanouts )
    {
        Vec_Ptr_t * vFanouts;
        Ivy_Obj_t * pFanout;
        int i;
        vFanouts = Vec_PtrAlloc( 10 );
        printf( "\nFanouts:\n" );
        Ivy_ObjForEachFanout( p, pObj, vFanouts, pFanout, i )
        {
            printf( "    " );
            printf( "Node %5d : ", Ivy_ObjId(pFanout) );
            if ( Ivy_ObjIsPo(pFanout) )
                printf( "PO" );
            else if ( Ivy_ObjIsLatch(pFanout) )
                printf( "latch (%d%s)", Ivy_ObjFanin0(pFanout)->Id, (Ivy_ObjFaninC0(pFanout)? "\'" : " ") );
            else if ( Ivy_ObjIsBuf(pFanout) )
                printf( "buffer (%d%s)", Ivy_ObjFanin0(pFanout)->Id, (Ivy_ObjFaninC0(pFanout)? "\'" : " ") );
            else
                printf( "AND( %5d%s, %5d%s )", 
                    Ivy_ObjFanin0(pFanout)->Id, (Ivy_ObjFaninC0(pFanout)? "\'" : " "), 
                    Ivy_ObjFanin1(pFanout)->Id, (Ivy_ObjFaninC1(pFanout)? "\'" : " ") );
            printf( "\n" );
        }
        Vec_PtrFree( vFanouts );
        return;
    }
    if ( !fHaig )
    {
        if ( pObj->pEquiv == NULL )
            printf( " HAIG node not given" );
        else
            printf( " HAIG node = %d%s", Ivy_Regular(pObj->pEquiv)->Id, (Ivy_IsComplement(pObj->pEquiv)? "\'" : " ") );
        return;
    }
    if ( pObj->pEquiv == NULL )
        return;
    // there are choices
    if ( Ivy_ObjRefs(pObj) > 0 )
    {
        // print equivalence class
        printf( "  { %5d ", pObj->Id );
        assert( !Ivy_IsComplement(pObj->pEquiv) );
        for ( pTemp = pObj->pEquiv; pTemp != pObj; pTemp = Ivy_Regular(pTemp->pEquiv) )
            printf( " %5d%s", pTemp->Id, (Ivy_IsComplement(pTemp->pEquiv)? "\'" : " ") );
        printf( " }" );
        return;
    }
    // this is a secondary node
    for ( pTemp = Ivy_Regular(pObj->pEquiv); Ivy_ObjRefs(pTemp) == 0; pTemp = Ivy_Regular(pTemp->pEquiv) );
    assert( Ivy_ObjRefs(pTemp) > 0 );
    printf( "  class of %d", pTemp->Id );
}

/**Function*************************************************************

  Synopsis    [Prints node in HAIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ManPrintVerbose( Ivy_Man_t * p, int fHaig )
{
    Vec_Int_t * vNodes;
    Ivy_Obj_t * pObj;
    int i;
    printf( "PIs: " );
    Ivy_ManForEachPi( p, pObj, i )
        printf( " %d", pObj->Id );
    printf( "\n" );
    printf( "POs: " );
    Ivy_ManForEachPo( p, pObj, i )
        printf( " %d", pObj->Id );
    printf( "\n" );
    printf( "Latches: " );
    Ivy_ManForEachLatch( p, pObj, i )
        printf( " %d=%d%s", pObj->Id, Ivy_ObjFanin0(pObj)->Id, (Ivy_ObjFaninC0(pObj)? "\'" : " ") );
    printf( "\n" );
    vNodes = Ivy_ManDfsSeq( p, NULL );
    Ivy_ManForEachNodeVec( p, vNodes, pObj, i )
        Ivy_ObjPrintVerbose( p, pObj, fHaig ), printf( "\n" );
    printf( "\n" );
    Vec_IntFree( vNodes );
}

/**Function*************************************************************

  Synopsis    [Performs incremental rewriting of the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_CutTruthPrint2( Ivy_Man_t * p, Ivy_Cut_t * pCut, unsigned uTruth )
{
    int i;
    printf( "Trying cut : {" );
    for ( i = 0; i < pCut->nSize; i++ )
        printf( " %6d(%d)", Ivy_LeafId(pCut->pArray[i]), Ivy_LeafLat(pCut->pArray[i]) );
    printf( " }   " );
    Extra_PrintBinary( stdout, &uTruth, 16 );  printf( "\n" );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Performs incremental rewriting of the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_CutTruthPrint( Ivy_Man_t * p, Ivy_Cut_t * pCut, unsigned uTruth )
{
    Vec_Ptr_t * vArray;
    Ivy_Obj_t * pObj, * pFanout;
    int nLatches = 0;
    int nPresent = 0;
    int i, k;
    int fVerbose = 0;

    if ( fVerbose )
        printf( "Trying cut : {" );
    for ( i = 0; i < pCut->nSize; i++ )
    {
        if ( fVerbose )
            printf( " %6d(%d)", Ivy_LeafId(pCut->pArray[i]), Ivy_LeafLat(pCut->pArray[i]) );
        nLatches += Ivy_LeafLat(pCut->pArray[i]);
    }
    if ( fVerbose )
        printf( " }   " );
    if ( fVerbose )
        printf( "Latches = %d. ", nLatches );

    // check if there are latches on the fanout edges
    vArray = Vec_PtrAlloc( 100 );
    for ( i = 0; i < pCut->nSize; i++ )
    {
        pObj = Ivy_ManObj( p, Ivy_LeafId(pCut->pArray[i]) );
        Ivy_ObjForEachFanout( p, pObj, vArray, pFanout, k )
        {
            if ( Ivy_ObjIsLatch(pFanout) )
            {
                nPresent++;
                break;
            }
        }
    }
    Vec_PtrSize( vArray );
    if ( fVerbose )
    {
        printf( "Present = %d. ", nPresent );
        if ( nLatches > nPresent )
            printf( "Clauses = %d. ", 2*(nLatches - nPresent) );
        printf( "\n" );
    }
    return ( nLatches > nPresent ) ? 2*(nLatches - nPresent) : 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

