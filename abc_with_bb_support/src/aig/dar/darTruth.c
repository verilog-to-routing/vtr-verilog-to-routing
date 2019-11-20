/**CFile****************************************************************

  FileName    [darTruth.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware AIG rewriting.]

  Synopsis    [Computes the truth table of a cut.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: darTruth.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "darInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

#if 0 

/**Function*************************************************************

  Synopsis    [Computes truth table of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManCollectCut_rec( Aig_Man_t * p, Aig_Obj_t * pNode, Vec_Int_t * vNodes )
{
    if ( pNode->fMarkA )
        return;
    pNode->fMarkA = 1;
    assert( Aig_ObjIsAnd(pNode) || Aig_ObjIsExor(pNode) );
    Aig_ManCollectCut_rec( p, Aig_ObjFanin0(pNode), vNodes );
    Aig_ManCollectCut_rec( p, Aig_ObjFanin1(pNode), vNodes );
    Vec_IntPush( vNodes, pNode->Id );
}

/**Function*************************************************************

  Synopsis    [Computes truth table of the cut.]

  Description [Does not modify the array of leaves. Uses array vTruth to store 
  temporary truth tables. The returned pointer should be used immediately.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManCollectCut( Aig_Man_t * p, Aig_Obj_t * pRoot, Vec_Int_t * vLeaves, Vec_Int_t * vNodes )
{
    int i, Leaf;
    // collect and mark the leaves
    Vec_IntClear( vNodes );
    Vec_IntForEachEntry( vLeaves, Leaf, i )
    {
        Vec_IntPush( vNodes, Leaf );
        Aig_ManObj(p, Leaf)->fMarkA = 1;
    }
    // collect and mark the nodes
    Aig_ManCollectCut_rec( p, pRoot, vNodes );
    // clean the nodes
    Vec_IntForEachEntry( vNodes, Leaf, i )
        Aig_ManObj(p, Leaf)->fMarkA = 0;
}

/**Function*************************************************************

  Synopsis    [Returns the pointer to the truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Aig_ObjGetTruthStore( int ObjNum, Vec_Int_t * vTruth )
{
   return ((unsigned *)Vec_IntArray(vTruth)) + 8 * ObjNum;
}

/**Function*************************************************************

  Synopsis    [Computes truth table of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManCutTruthOne( Aig_Man_t * p, Aig_Obj_t * pNode, Vec_Int_t * vTruth, int nWords )
{
    unsigned * pTruth, * pTruth0, * pTruth1;
    int i;
    pTruth  = Aig_ObjGetTruthStore( pNode->Level, vTruth );
    pTruth0 = Aig_ObjGetTruthStore( Aig_ObjFanin0(pNode)->Level, vTruth );
    pTruth1 = Aig_ObjGetTruthStore( Aig_ObjFanin1(pNode)->Level, vTruth );
    if ( Aig_ObjIsExor(pNode) )
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = pTruth0[i] ^ pTruth1[i];
    else if ( !Aig_ObjFaninC0(pNode) && !Aig_ObjFaninC1(pNode) )
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = pTruth0[i] & pTruth1[i];
    else if ( !Aig_ObjFaninC0(pNode) && Aig_ObjFaninC1(pNode) )
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = pTruth0[i] & ~pTruth1[i];
    else if ( Aig_ObjFaninC0(pNode) && !Aig_ObjFaninC1(pNode) )
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = ~pTruth0[i] & pTruth1[i];
    else // if ( Aig_ObjFaninC0(pNode) && Aig_ObjFaninC1(pNode) )
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
unsigned * Aig_ManCutTruth( Aig_Man_t * p, Aig_Obj_t * pRoot, Vec_Int_t * vLeaves, Vec_Int_t * vNodes, Vec_Int_t * vTruth )
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
//    Aig_ManCollectCut( p, pRoot, vLeaves, vNodes );
    // set the node numbers
    Vec_IntForEachEntry( vNodes, Leaf, i )
        Aig_ManObj(p, Leaf)->Level = i;
    // alloc enough memory
    Vec_IntClear( vTruth );
    Vec_IntGrow( vTruth, 8 * Vec_IntSize(vNodes) );
    // set the elementary truth tables
    Vec_IntForEachEntry( vLeaves, Leaf, i )
        memcpy( Aig_ObjGetTruthStore(i, vTruth), uTruths[i], 8 * sizeof(unsigned) );
    // compute truths for other nodes
    Vec_IntForEachEntryStart( vNodes, Leaf, i, Vec_IntSize(vLeaves) )
        Aig_ManCutTruthOne( p, Aig_ManObj(p, Leaf), vTruth, 8 );
    return Aig_ObjGetTruthStore( pRoot->Level, vTruth );
}

static inline int          Kit_TruthWordNum( int nVars )  { return nVars <= 5 ? 1 : (1 << (nVars - 5));                             }
static inline void Kit_TruthNot( unsigned * pOut, unsigned * pIn, int nVars )
{
    int w;
    for ( w = Kit_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = ~pIn[w];
}

/**Function*************************************************************

  Synopsis    [Computes the cost based on two ISOPs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManLargeCutEvalIsop( unsigned * pTruth, int nVars, Vec_Int_t * vMemory )
{
    extern int Kit_TruthIsop( unsigned * puTruth, int nVars, Vec_Int_t * vMemory, int fTryBoth );
    int RetValue, nClauses;
    // compute ISOP for the positive phase
    RetValue = Kit_TruthIsop( pTruth, nVars, vMemory, 0 );
    if ( RetValue == -1 )
        return AIG_INFINITY;
    assert( RetValue == 0 || RetValue == 1 );
    nClauses = Vec_IntSize( vMemory );
    // compute ISOP for the negative phase
    Kit_TruthNot( pTruth, pTruth, nVars );
    RetValue = Kit_TruthIsop( pTruth, nVars, vMemory, 0 );
    if ( RetValue == -1 )
        return AIG_INFINITY;
    Kit_TruthNot( pTruth, pTruth, nVars );
    assert( RetValue == 0 || RetValue == 1 );
    nClauses += Vec_IntSize( vMemory );
    return nClauses;
}

/**Function*************************************************************

  Synopsis    [Computes truth table of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManLargeCutCollect_rec( Aig_Man_t * p, Aig_Obj_t * pNode, Vec_Int_t * vLeaves, Vec_Int_t * vNodes )
{
    if ( Aig_ObjIsTravIdCurrent(p, pNode) )
        return;
    if ( Aig_ObjIsTravIdPrevious(p, pNode) )
    {
        Vec_IntPush( vLeaves, pNode->Id );
//        Vec_IntPush( vNodes, pNode->Id );
        Aig_ObjSetTravIdCurrent( p, pNode );
        return;
    }
    assert( Aig_ObjIsAnd(pNode) || Aig_ObjIsExor(pNode) );
    Aig_ObjSetTravIdCurrent( p, pNode );
    Aig_ManLargeCutCollect_rec( p, Aig_ObjFanin0(pNode), vLeaves, vNodes );
    Aig_ManLargeCutCollect_rec( p, Aig_ObjFanin1(pNode), vLeaves, vNodes );
    Vec_IntPush( vNodes, pNode->Id );
}

/**Function*************************************************************

  Synopsis    [Collect leaves and nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManLargeCutCollect( Aig_Man_t * p, Aig_Obj_t * pRoot, Aig_Cut_t * pCutR, Aig_Cut_t * pCutL, int Leaf, 
                        Vec_Int_t * vLeaves, Vec_Int_t * vNodes )
{
    Vec_Int_t * vTemp;
    Aig_Obj_t * pObj;
    int Node, i;

    Aig_ManIncrementTravId( p );
    Aig_CutForEachLeaf( p, pCutR, pObj, i )
        if ( pObj->Id != Leaf )
           Aig_ObjSetTravIdCurrent( p, pObj );
    Aig_CutForEachLeaf( p, pCutL, pObj, i )
        Aig_ObjSetTravIdCurrent( p, pObj );

    // collect the internal nodes and leaves
    Aig_ManIncrementTravId( p );
    vTemp = Vec_IntAlloc( 100 );
    Aig_ManLargeCutCollect_rec( p, pRoot, vLeaves, vTemp );

    Vec_IntForEachEntry( vLeaves, Node, i )
        Vec_IntPush( vNodes, Node );
    Vec_IntForEachEntry( vTemp, Node, i )
        Vec_IntPush( vNodes, Node );

    Vec_IntFree( vTemp );

}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManLargeCutEval( Aig_Man_t * p, Aig_Obj_t * pRoot, Aig_Cut_t * pCutR, Aig_Cut_t * pCutL, int Leaf )
{
    Vec_Int_t * vLeaves, * vNodes, * vTruth, * vMemory;
    unsigned * pTruth;
    int RetValue;
//    Aig_Obj_t * pObj;

    vMemory = Vec_IntAlloc( 1 << 16 );
    vTruth = Vec_IntAlloc( 1 << 16 );
    vLeaves = Vec_IntAlloc( 100 );
    vNodes = Vec_IntAlloc( 100 );

    Aig_ManLargeCutCollect( p, pRoot, pCutR, pCutL, Leaf, vLeaves, vNodes );
/*
    // collect the nodes
    Aig_CutForEachLeaf( p, pCutR, pObj, i )
    {
        if ( pObj->Id == Leaf )
            continue;
        if ( pObj->fMarkA )
            continue;
        pObj->fMarkA = 1;
        Vec_IntPush( vLeaves, pObj->Id );
        Vec_IntPush( vNodes, pObj->Id );
    }
    Aig_CutForEachLeaf( p, pCutL, pObj, i )
    {
        if ( pObj->fMarkA )
            continue;
        pObj->fMarkA = 1;
        Vec_IntPush( vLeaves, pObj->Id );
        Vec_IntPush( vNodes, pObj->Id );
    }
    // collect and mark the nodes
    Aig_ManCollectCut_rec( p, pRoot, vNodes );
    // clean the nodes
    Vec_IntForEachEntry( vNodes, Leaf, i )
        Aig_ManObj(p, Leaf)->fMarkA = 0;
*/

    pTruth = Aig_ManCutTruth( p, pRoot, vLeaves, vNodes, vTruth );
    RetValue = Aig_ManLargeCutEvalIsop( pTruth, Vec_IntSize(vLeaves), vMemory );

    Vec_IntFree( vLeaves );
    Vec_IntFree( vNodes );
    Vec_IntFree( vTruth );
    Vec_IntFree( vMemory );

    return RetValue;
}

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


