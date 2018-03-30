/**CFile****************************************************************

  FileName    [acecTree.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [CEC for arithmetic circuits.]

  Synopsis    [Adder tree construction.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: acecTree.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "acecInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Acec_BoxFree( Acec_Box_t * pBox )
{
    Vec_WecFreeP( &pBox->vAdds );
    Vec_WecFreeP( &pBox->vLeafLits );
    Vec_WecFreeP( &pBox->vRootLits );
    Vec_WecFreeP( &pBox->vUnique );
    Vec_WecFreeP( &pBox->vShared );
    ABC_FREE( pBox );
}
void Acec_BoxFreeP( Acec_Box_t ** ppBox )
{
    if ( *ppBox )
        Acec_BoxFree( *ppBox );
    *ppBox = NULL;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Acec_VerifyBoxLeaves( Acec_Box_t * pBox, Vec_Bit_t * vIgnore )
{
    Vec_Int_t * vLevel;
    int i, k, iLit, Count = 0;
    if ( vIgnore == NULL )
        return;
    Vec_WecForEachLevel( pBox->vLeafLits, vLevel, i )
        Vec_IntForEachEntry( vLevel, iLit, k )
            if ( Gia_ObjIsAnd(Gia_ManObj(pBox->pGia, Abc_Lit2Var(iLit))) && !Vec_BitEntry(vIgnore, Abc_Lit2Var(iLit)) )
                printf( "Internal node %d of rank %d is not part of PPG.\n", Abc_Lit2Var(iLit), i ), Count++;
    printf( "Detected %d suspicious leaves.\n", Count );
}

/**Function*************************************************************

  Synopsis    [Filters trees by removing TFO of roots.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Acec_TreeFilterOne( Gia_Man_t * p, Vec_Int_t * vAdds, Vec_Int_t * vTree )
{
    Vec_Bit_t * vIsRoot = Vec_BitStart( Gia_ManObjNum(p) );
    Vec_Bit_t * vMarked = Vec_BitStart( Gia_ManObjNum(p) ) ;
    Gia_Obj_t * pObj;
    int i, k = 0, Box, Rank;
    // mark roots
    Vec_IntForEachEntryDouble( vTree, Box, Rank, i )
    {
        Vec_BitWriteEntry( vIsRoot, Vec_IntEntry(vAdds, 6*Box+3), 1 );
        Vec_BitWriteEntry( vIsRoot, Vec_IntEntry(vAdds, 6*Box+4), 1 );
    }
    Vec_IntForEachEntryDouble( vTree, Box, Rank, i )
    {
        Vec_BitWriteEntry( vIsRoot, Vec_IntEntry(vAdds, 6*Box+0), 0 );
        Vec_BitWriteEntry( vIsRoot, Vec_IntEntry(vAdds, 6*Box+1), 0 );
        Vec_BitWriteEntry( vIsRoot, Vec_IntEntry(vAdds, 6*Box+2), 0 );
    }
    // iterate through nodes to detect TFO of roots
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( Vec_BitEntry(vIsRoot, Gia_ObjFaninId0(pObj,i)) || Vec_BitEntry(vIsRoot, Gia_ObjFaninId1(pObj,i)) ||
             Vec_BitEntry(vMarked, Gia_ObjFaninId0(pObj,i)) || Vec_BitEntry(vMarked, Gia_ObjFaninId1(pObj,i)) )
            Vec_BitWriteEntry( vMarked, i, 1 );
    }
    // remove those that overlap with roots
    Vec_IntForEachEntryDouble( vTree, Box, Rank, i )
    {
        // special case of the first bit
//        if ( i == 0 )
//            continue;

/*
        if ( Vec_IntEntry(vAdds, 6*Box+3) == 24 && Vec_IntEntry(vAdds, 6*Box+4) == 22 )
        {
            printf( "**** removing special one \n" );
            continue;
        }
        if ( Vec_IntEntry(vAdds, 6*Box+3) == 48 && Vec_IntEntry(vAdds, 6*Box+4) == 49 )
        {
            printf( "**** removing special one \n" );
            continue;
        }
*/
        if ( Vec_BitEntry(vMarked, Vec_IntEntry(vAdds, 6*Box+3)) || Vec_BitEntry(vMarked, Vec_IntEntry(vAdds, 6*Box+4)) )
        {
            printf( "Removing box %d=(%d,%d) of rank %d.\n", Box, Vec_IntEntry(vAdds, 6*Box+3), Vec_IntEntry(vAdds, 6*Box+4), Rank ); 
            continue;
        }
        Vec_IntWriteEntry( vTree, k++, Box );
        Vec_IntWriteEntry( vTree, k++, Rank );
    }
    Vec_IntShrink( vTree, k );
    Vec_BitFree( vIsRoot );
    Vec_BitFree( vMarked );
}
void Acec_TreeFilterTrees( Gia_Man_t * p, Vec_Int_t * vAdds, Vec_Wec_t * vTrees )
{
    Vec_Int_t * vLevel;
    int i;
    Vec_WecForEachLevel( vTrees, vLevel, i )
        Acec_TreeFilterOne( p, vAdds, vLevel );
}

/**Function*************************************************************

  Synopsis    [Filters trees by removing TFO of roots.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Acec_TreeMarkTFI_rec( Gia_Man_t * p, int Id, Vec_Bit_t * vMarked )
{
    Gia_Obj_t * pObj = Gia_ManObj(p, Id);
    if ( Vec_BitEntry(vMarked, Id) )
        return;
    Vec_BitWriteEntry( vMarked, Id, 1 );
    if ( !Gia_ObjIsAnd(pObj) )
        return;
    Acec_TreeMarkTFI_rec( p, Gia_ObjFaninId0(pObj, Id), vMarked );
    Acec_TreeMarkTFI_rec( p, Gia_ObjFaninId1(pObj, Id), vMarked );
}
void Acec_TreeFilterOne2( Gia_Man_t * p, Vec_Int_t * vAdds, Vec_Int_t * vTree )
{
    Vec_Bit_t * vIsLeaf = Vec_BitStart( Gia_ManObjNum(p) );
    Vec_Bit_t * vMarked = Vec_BitStart( Gia_ManObjNum(p) ) ;
    Gia_Obj_t * pObj;
    int i, k = 0, Box, Rank;
    // mark leaves
    Vec_IntForEachEntryDouble( vTree, Box, Rank, i )
    {
        Vec_BitWriteEntry( vIsLeaf, Vec_IntEntry(vAdds, 6*Box+0), 1 );
        Vec_BitWriteEntry( vIsLeaf, Vec_IntEntry(vAdds, 6*Box+1), 1 );
        Vec_BitWriteEntry( vIsLeaf, Vec_IntEntry(vAdds, 6*Box+2), 1 );
    }
    Vec_IntForEachEntryDouble( vTree, Box, Rank, i )
    {
        Vec_BitWriteEntry( vIsLeaf, Vec_IntEntry(vAdds, 6*Box+3), 0 );
        Vec_BitWriteEntry( vIsLeaf, Vec_IntEntry(vAdds, 6*Box+4), 0 );
    }
    // mark TFI of leaves
    Gia_ManForEachAnd( p, pObj, i )
        if ( Vec_BitEntry(vIsLeaf, i) )
            Acec_TreeMarkTFI_rec( p, i, vMarked );
    // additional one
//if ( 10942 < Gia_ManObjNum(p) )
//    Acec_TreeMarkTFI_rec( p, 10942, vMarked );
    // remove those that overlap with the marked TFI
    Vec_IntForEachEntryDouble( vTree, Box, Rank, i )
    {
        if ( Vec_BitEntry(vMarked, Vec_IntEntry(vAdds, 6*Box+3)) || Vec_BitEntry(vMarked, Vec_IntEntry(vAdds, 6*Box+4)) )
        {
            printf( "Removing box %d=(%d,%d) of rank %d.\n", Box, Vec_IntEntry(vAdds, 6*Box+3), Vec_IntEntry(vAdds, 6*Box+4), Rank ); 
            continue;
        }
        Vec_IntWriteEntry( vTree, k++, Box );
        Vec_IntWriteEntry( vTree, k++, Rank );
    }
    Vec_IntShrink( vTree, k );
    Vec_BitFree( vIsLeaf );
    Vec_BitFree( vMarked );
}
void Acec_TreeFilterTrees2( Gia_Man_t * p, Vec_Int_t * vAdds, Vec_Wec_t * vTrees )
{
    Vec_Int_t * vLevel;
    int i;
    Vec_WecForEachLevel( vTrees, vLevel, i )
        Acec_TreeFilterOne2( p, vAdds, vLevel );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Acec_TreeVerifyPhaseOne_rec( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    int Truth0, Truth1;
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return pObj->Value;
    Gia_ObjSetTravIdCurrent(p, pObj);
    assert( Gia_ObjIsAnd(pObj) );
    assert( !Gia_ObjIsXor(pObj) );
    Truth0 = Acec_TreeVerifyPhaseOne_rec( p, Gia_ObjFanin0(pObj) );
    Truth1 = Acec_TreeVerifyPhaseOne_rec( p, Gia_ObjFanin1(pObj) );
    Truth0 = Gia_ObjFaninC0(pObj) ? 0xFF & ~Truth0 : Truth0;
    Truth1 = Gia_ObjFaninC1(pObj) ? 0xFF & ~Truth1 : Truth1;
    return (pObj->Value = Truth0 & Truth1);
}
void Acec_TreeVerifyPhaseOne( Gia_Man_t * p, Vec_Int_t * vAdds, int iBox )
{
    Gia_Obj_t * pObj;
    unsigned TruthXor, TruthMaj, Truths[3] = { 0xAA, 0xCC, 0xF0 };
    int k, iObj, fFadd = Vec_IntEntry(vAdds, 6*iBox+2) > 0;
    int fFlip = !fFadd && Acec_SignBit2(vAdds, iBox, 2);

    Gia_ManIncrementTravId( p );
    for ( k = 0; k < 3; k++ )
    {
        iObj = Vec_IntEntry( vAdds, 6*iBox+k );
        if ( iObj == 0 )
            continue;
        pObj = Gia_ManObj( p, iObj );
        pObj->Value = (Acec_SignBit2(vAdds, iBox, k) ^ fFlip) ? 0xFF & ~Truths[k] : Truths[k];
        Gia_ObjSetTravIdCurrent( p, pObj );
    }

    iObj = Vec_IntEntry( vAdds, 6*iBox+3 );
    TruthXor = Acec_TreeVerifyPhaseOne_rec( p, Gia_ManObj(p, iObj) );
    TruthXor = (Acec_SignBit2(vAdds, iBox, 3) ^ fFlip) ? 0xFF & ~TruthXor : TruthXor;

    iObj = Vec_IntEntry( vAdds, 6*iBox+4 );
    TruthMaj = Acec_TreeVerifyPhaseOne_rec( p, Gia_ManObj(p, iObj) );
    TruthMaj = (Acec_SignBit2(vAdds, iBox, 4) ^ fFlip) ? 0xFF & ~TruthMaj : TruthMaj;

    if ( fFadd ) // FADD
    {
        if ( TruthXor != 0x96 )
            printf( "Fadd %d sum %d is wrong.\n", iBox, Vec_IntEntry( vAdds, 6*iBox+3 ) );
        if ( TruthMaj != 0xE8 )
            printf( "Fadd %d carry %d is wrong.\n", iBox, Vec_IntEntry( vAdds, 6*iBox+4 ) );
    }
    else
    {
        //printf( "Sign1 = %d%d%d %d\n", Acec_SignBit(vAdds, iBox, 0), Acec_SignBit(vAdds, iBox, 1), Acec_SignBit(vAdds, iBox, 2), Acec_SignBit(vAdds, iBox, 3) );
        //printf( "Sign2 = %d%d%d %d%d\n", Acec_SignBit2(vAdds, iBox, 0), Acec_SignBit2(vAdds, iBox, 1), Acec_SignBit2(vAdds, iBox, 2), Acec_SignBit2(vAdds, iBox, 3), Acec_SignBit2(vAdds, iBox, 4) );
        if ( TruthXor != 0x66 )
            printf( "Hadd %d sum %d is wrong.\n", iBox, Vec_IntEntry( vAdds, 6*iBox+3 ) );
        if ( TruthMaj != 0x88 )
            printf( "Hadd %d carry %d is wrong.\n", iBox, Vec_IntEntry( vAdds, 6*iBox+4 ) );
    }
}
void Acec_TreeVerifyPhases( Gia_Man_t * p, Vec_Int_t * vAdds, Vec_Wec_t * vBoxes )
{
    Vec_Int_t * vLevel;
    int i, k, Box;
    Vec_WecForEachLevel( vBoxes, vLevel, i )
        Vec_IntForEachEntry( vLevel, Box, k )
            Acec_TreeVerifyPhaseOne( p, vAdds, Box );
}
void Acec_TreeVerifyPhases2( Gia_Man_t * p, Vec_Int_t * vAdds, Vec_Wec_t * vBoxes )
{
    Vec_Bit_t * vPhase = Vec_BitStart( Gia_ManObjNum(p) );
    Vec_Bit_t * vRoots = Vec_BitStart( Gia_ManObjNum(p) );
    Vec_Int_t * vLevel;
    int i, k, n, Box;
    // mark all output points and their values
    Vec_WecForEachLevel( vBoxes, vLevel, i )
        Vec_IntForEachEntry( vLevel, Box, k )
        {
            Vec_BitWriteEntry( vRoots, Vec_IntEntry( vAdds, 6*Box+3 ), 1 );
            Vec_BitWriteEntry( vRoots, Vec_IntEntry( vAdds, 6*Box+4 ), 1 );
            Vec_BitWriteEntry( vPhase, Vec_IntEntry( vAdds, 6*Box+3 ), Acec_SignBit2(vAdds, Box, 3) );
            Vec_BitWriteEntry( vPhase, Vec_IntEntry( vAdds, 6*Box+4 ), Acec_SignBit2(vAdds, Box, 4) );
        }
    // compare with input points
    Vec_WecForEachLevel( vBoxes, vLevel, i )
        Vec_IntForEachEntry( vLevel, Box, k )
            for ( n = 0; n < 3; n++ )
            {
                if ( !Vec_BitEntry(vRoots, Vec_IntEntry(vAdds, 6*Box+n)) )
                    continue;
                if ( Vec_BitEntry(vPhase, Vec_IntEntry(vAdds, 6*Box+n)) == Acec_SignBit2(vAdds, Box, n) )
                    continue;
                printf( "Phase of input %d=%d is mismatched in box %d=(%d,%d).\n",
                    n, Vec_IntEntry(vAdds, 6*Box+n), Box, Vec_IntEntry(vAdds, 6*Box+3), Vec_IntEntry(vAdds, 6*Box+4) );
            }
    Vec_BitFree( vPhase );
    Vec_BitFree( vRoots );
}
void Acec_TreeVerifyConnections( Gia_Man_t * p, Vec_Int_t * vAdds, Vec_Wec_t * vBoxes )
{
    Vec_Int_t * vCounts = Vec_IntStartFull( Gia_ManObjNum(p) );
    Vec_Int_t * vLevel;
    int i, k, n, Box;
    // mark outputs
    Vec_WecForEachLevel( vBoxes, vLevel, i )
        Vec_IntForEachEntry( vLevel, Box, k )
        {
            Vec_IntWriteEntry( vCounts, Vec_IntEntry( vAdds, 6*Box+3 ), 0 );
            Vec_IntWriteEntry( vCounts, Vec_IntEntry( vAdds, 6*Box+4 ), 0 );
        }
    // count fanouts
    Vec_WecForEachLevel( vBoxes, vLevel, i )
        Vec_IntForEachEntry( vLevel, Box, k )
            for ( n = 0; n < 3; n++ )
                if ( Vec_IntEntry( vCounts, Vec_IntEntry(vAdds, 6*Box+n) ) != -1 )
                    Vec_IntAddToEntry( vCounts, Vec_IntEntry(vAdds, 6*Box+n), 1 );
    // print out
    printf( "The adder tree has %d internal cut points. ", Vec_IntCountLarger(vCounts, -1) );
    if ( Vec_IntCountLarger(vCounts, 1) == 0 )
        printf( "There is no internal fanouts.\n" );
    else
    {
        printf( "These %d points have more than one fanout:\n", Vec_IntCountLarger(vCounts, 1) );
        Vec_IntForEachEntry( vCounts, Box, i )
            if ( Box > 1 )
                printf( "Node %d(lev %d) has fanout %d.\n", i, Gia_ObjLevelId(p, i), Box );
    }
    Vec_IntFree( vCounts );
}

/**Function*************************************************************

  Synopsis    [Creates polarity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Acec_TreeCarryMap( Gia_Man_t * p, Vec_Int_t * vAdds, Vec_Wec_t * vBoxes )
{
    Vec_Int_t * vMap = Vec_IntStartFull( Gia_ManObjNum(p) );
    Vec_Int_t * vLevel;
    int i, k, Box;
    Vec_WecForEachLevel( vBoxes, vLevel, i )
        Vec_IntForEachEntry( vLevel, Box, k )
            Vec_IntWriteEntry( vMap, Vec_IntEntry(vAdds, 6*Box+4), Box );
    return vMap;
}
void Acec_TreePhases_rec( Gia_Man_t * p, Vec_Int_t * vAdds, Vec_Int_t * vMap, int Node, int fPhase, Vec_Bit_t * vVisit )
{
    int k, iBox, iXor, fXorPhase, fPhaseThis;
    assert( Node != 0 );
    iBox = Vec_IntEntry( vMap, Node );
    if ( iBox == -1 )
        return;
    assert( Node == Vec_IntEntry( vAdds, 6*iBox+4 ) );
    if ( Vec_BitEntry(vVisit, iBox) )
        return;
    Vec_BitWriteEntry( vVisit, iBox, 1 );
    iXor = Vec_IntEntry( vAdds, 6*iBox+3 );
    fXorPhase = Acec_SignBit(vAdds, iBox, 3);
    if ( Vec_IntEntry(vAdds, 6*iBox+2) == 0 )
    {
        //fPhaseThis = Acec_SignBit( vAdds, iBox, 2 ) ^ fPhase;
        //fXorPhase ^= fPhaseThis;
        //Acec_SignSetBit2( vAdds, iBox, 2, fPhaseThis ); // complemented HADD -- create const1 input
        fPhase ^= Acec_SignBit( vAdds, iBox, 2 );
        fXorPhase ^= fPhase;
        Acec_SignSetBit2( vAdds, iBox, 2, fPhase ); // complemented HADD -- create const1 input
    }
    for ( k = 0; k < 3; k++ )
    {
        int iObj = Vec_IntEntry( vAdds, 6*iBox+k );
        if ( iObj == 0 )
            continue;
        fPhaseThis = Acec_SignBit(vAdds, iBox, k) ^ fPhase;
        fXorPhase ^= fPhaseThis;
        Acec_TreePhases_rec( p, vAdds, vMap, iObj, fPhaseThis, vVisit );
        Acec_SignSetBit2( vAdds, iBox, k, fPhaseThis );
    }
    Acec_SignSetBit2( vAdds, iBox, 3, fXorPhase );
    Acec_SignSetBit2( vAdds, iBox, 4, fPhase );
}

/**Function*************************************************************

  Synopsis    [Find internal cut points with exactly one adder fanin/fanout.]

  Description [Returns a map of point into its input/output adder.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Acec_TreeAddInOutPoint( Vec_Int_t * vMap, int iObj, int iAdd, int fOut )
{
    int * pPlace = Vec_IntEntryP( vMap, Abc_Var2Lit(iObj, fOut) );
    if ( *pPlace == -1 )
        *pPlace = iAdd;
    else if ( *pPlace >= 0 )
        *pPlace = -2;
}
Vec_Int_t * Acec_TreeFindPoints( Gia_Man_t * p, Vec_Int_t * vAdds, Vec_Bit_t * vIgnore )
{
    Vec_Int_t * vMap = Vec_IntStartFull( 2*Gia_ManObjNum(p) );
    int i;
    for ( i = 0; 6*i < Vec_IntSize(vAdds); i++ )
    {
        if ( vIgnore && (Vec_BitEntry(vIgnore, Vec_IntEntry(vAdds, 6*i+3)) || Vec_BitEntry(vIgnore, Vec_IntEntry(vAdds, 6*i+4))) )
            continue;
        Acec_TreeAddInOutPoint( vMap, Vec_IntEntry(vAdds, 6*i+0), i, 0 );
        Acec_TreeAddInOutPoint( vMap, Vec_IntEntry(vAdds, 6*i+1), i, 0 );
        Acec_TreeAddInOutPoint( vMap, Vec_IntEntry(vAdds, 6*i+2), i, 0 );
        Acec_TreeAddInOutPoint( vMap, Vec_IntEntry(vAdds, 6*i+3), i, 1 );
        Acec_TreeAddInOutPoint( vMap, Vec_IntEntry(vAdds, 6*i+4), i, 1 );
    }
    return vMap;
}

/**Function*************************************************************

  Synopsis    [Find adder trees as groups of adders connected vis cut-points.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Acec_TreeWhichPoint( Vec_Int_t * vAdds, int iAdd, int iObj )
{
    int k;
    for ( k = 0; k < 5; k++ )
        if ( Vec_IntEntry(vAdds, 6*iAdd+k) == iObj )
            return k;
    assert( 0 );
    return -1;
}
void Acec_TreeFindTrees2_rec( Vec_Int_t * vAdds, Vec_Int_t * vMap, int iAdd, int Rank, Vec_Int_t * vTree, Vec_Bit_t * vFound )
{
    extern void Acec_TreeFindTrees_rec( Vec_Int_t * vAdds, Vec_Int_t * vMap, int iObj, int Rank, Vec_Int_t * vTree, Vec_Bit_t * vFound );
    int k;
    if ( Vec_BitEntry(vFound, iAdd) )
        return;
    Vec_BitWriteEntry( vFound, iAdd, 1 );
    Vec_IntPush( vTree, iAdd );
    Vec_IntPush( vTree, Rank );
    //printf( "Assigning rank %d to (%d:%d).\n", Rank, Vec_IntEntry(vAdds, 6*iAdd+3), Vec_IntEntry(vAdds, 6*iAdd+4) );
    for ( k = 0; k < 5; k++ )
        Acec_TreeFindTrees_rec( vAdds, vMap, Vec_IntEntry(vAdds, 6*iAdd+k), k == 4 ? Rank + 1 : Rank, vTree, vFound );
}
void Acec_TreeFindTrees_rec( Vec_Int_t * vAdds, Vec_Int_t * vMap, int iObj, int Rank, Vec_Int_t * vTree, Vec_Bit_t * vFound )
{
    int In  = Vec_IntEntry( vMap, Abc_Var2Lit(iObj, 1) );
    int Out = Vec_IntEntry( vMap, Abc_Var2Lit(iObj, 0) );
    if ( In < 0 || Out < 0 )
        return;
    Acec_TreeFindTrees2_rec( vAdds, vMap, In,  Acec_TreeWhichPoint(vAdds, In, iObj) == 4 ? Rank-1 : Rank, vTree, vFound );
    Acec_TreeFindTrees2_rec( vAdds, vMap, Out, Rank, vTree, vFound );
}
Vec_Wec_t * Acec_TreeFindTrees( Gia_Man_t * p, Vec_Int_t * vAdds, Vec_Bit_t * vIgnore, int fFilterIn, int fFilterOut )
{
    Vec_Wec_t * vTrees = Vec_WecAlloc( 10 );
    Vec_Int_t * vMap   = Acec_TreeFindPoints( p, vAdds, vIgnore );
    Vec_Bit_t * vFound = Vec_BitStart( Vec_IntSize(vAdds)/6 );
    Vec_Int_t * vTree;
    int i, k, In, Out, Box, Rank, MinRank;
    // go through the cut-points
    Vec_IntForEachEntryDouble( vMap, In, Out, i )
    {
        if ( In < 0 || Out < 0 )
            continue;
        assert( Vec_BitEntry(vFound, In) == Vec_BitEntry(vFound, Out) );
        if ( Vec_BitEntry(vFound, In) )
            continue;
        vTree = Vec_WecPushLevel( vTrees );
        Acec_TreeFindTrees_rec( vAdds, vMap, i/2, 0, vTree, vFound );
        // normalize rank
        MinRank = ABC_INFINITY;
        Vec_IntForEachEntryDouble( vTree, Box, Rank, k )
            MinRank = Abc_MinInt( MinRank, Rank );
        Vec_IntForEachEntryDouble( vTree, Box, Rank, k )
            Vec_IntWriteEntry( vTree, k+1, Rank - MinRank );
    }
    Vec_BitFree( vFound );
    Vec_IntFree( vMap );
    // filter trees
    if ( fFilterIn )
        Acec_TreeFilterTrees2( p, vAdds, vTrees );
    else if ( fFilterOut )
        Acec_TreeFilterTrees( p, vAdds, vTrees );
    // sort by size
    Vec_WecSort( vTrees, 1 );
    return vTrees;
}
void Acec_TreeFindTreesTest( Gia_Man_t * p )
{
    Vec_Wec_t * vTrees;

    abctime clk = Abc_Clock();
    Vec_Int_t * vAdds = Ree_ManComputeCuts( p, NULL, 1 );
    int nFadds = Ree_ManCountFadds( vAdds );
    printf( "Detected %d adders (%d FAs and %d HAs).  ", Vec_IntSize(vAdds)/6, nFadds, Vec_IntSize(vAdds)/6-nFadds );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );

    clk = Abc_Clock();
    vTrees = Acec_TreeFindTrees( p, vAdds, NULL, 0, 0 );
    printf( "Collected %d trees with %d adders in them.  ", Vec_WecSize(vTrees), Vec_WecSizeSize(vTrees)/2 );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    Vec_WecPrint( vTrees, 0 );

    Vec_WecFree( vTrees );
    Vec_IntFree( vAdds );
}


/**Function*************************************************************

  Synopsis    [Derives one adder tree.]

  Description []
               
  SideEffects []

  SeeAlso     []
`
***********************************************************************/
void Acec_PrintAdders( Vec_Wec_t * vBoxes, Vec_Int_t * vAdds )
{
    Vec_Int_t * vLevel;
    int i, k, iBox;
    Vec_WecForEachLevel( vBoxes, vLevel, i )
    {
        printf( " %4d : %2d  {", i, Vec_IntSize(vLevel) );
        Vec_IntForEachEntry( vLevel, iBox, k )
        {
            printf( " %s%d=(%d,%d)", Vec_IntEntry(vAdds, 6*iBox+2) == 0 ? "*":"", iBox, 
                                     Vec_IntEntry(vAdds, 6*iBox+3), Vec_IntEntry(vAdds, 6*iBox+4) );
            //printf( "(%d,%d,%d)", Vec_IntEntry(vAdds, 6*iBox+0), Vec_IntEntry(vAdds, 6*iBox+1), Vec_IntEntry(vAdds, 6*iBox+2) );
        }
        printf( " }\n" );
    }
}
void Acec_TreePrintBox( Acec_Box_t * pBox, Vec_Int_t * vAdds )
{
    printf( "Adders:\n" );
    Acec_PrintAdders( pBox->vAdds, vAdds );
    printf( "Inputs:\n" );
    Vec_WecPrintLits( pBox->vLeafLits );
    printf( "Outputs:\n" );
    Vec_WecPrintLits( pBox->vRootLits );
//    printf( "Node %d has level %d.\n", 3715, Gia_ObjLevelId(pBox->pGia, 3715) );
//    printf( "Node %d has level %d.\n", 167, Gia_ObjLevelId(pBox->pGia, 167) );
//    printf( "Node %d has level %d.\n", 278, Gia_ObjLevelId(pBox->pGia, 278) );
//    printf( "Node %d has level %d.\n", 597, Gia_ObjLevelId(pBox->pGia, 597) );
}

int Acec_CreateBoxMaxRank( Vec_Int_t * vTree )
{
    int k, Box, Rank, MaxRank = 0;
    Vec_IntForEachEntryDouble( vTree, Box, Rank, k )
        MaxRank = Abc_MaxInt( MaxRank, Rank );
    return MaxRank;
}
Acec_Box_t * Acec_CreateBox( Gia_Man_t * p, Vec_Int_t * vAdds, Vec_Int_t * vTree )
{
    int MaxRank = Acec_CreateBoxMaxRank(vTree);
    Vec_Bit_t * vVisit  = Vec_BitStart( Vec_IntSize(vAdds)/6 );
    Vec_Bit_t * vIsLeaf = Vec_BitStart( Gia_ManObjNum(p) );
    Vec_Bit_t * vIsRoot = Vec_BitStart( Gia_ManObjNum(p) );
    Vec_Int_t * vLevel, * vMap;
    int i, j, k, Box, Rank;//, Count = 0;

    Acec_Box_t * pBox = ABC_CALLOC( Acec_Box_t, 1 );
    pBox->pGia        = p;
    pBox->vAdds       = Vec_WecStart( MaxRank + 1 );
    pBox->vLeafLits   = Vec_WecStart( MaxRank + 1 );
    pBox->vRootLits   = Vec_WecStart( MaxRank + 2 );

    // collect boxes; mark inputs/outputs
    Vec_IntForEachEntryDouble( vTree, Box, Rank, i )
    {
//        if ( 37 == Box && 6 == Rank )
//        {
//            printf( "Skipping one adder...\n" );
//            continue;
//        }
        Vec_BitWriteEntry( vIsLeaf, Vec_IntEntry(vAdds, 6*Box+0), 1 );
        Vec_BitWriteEntry( vIsLeaf, Vec_IntEntry(vAdds, 6*Box+1), 1 );
        Vec_BitWriteEntry( vIsLeaf, Vec_IntEntry(vAdds, 6*Box+2), 1 );
        Vec_BitWriteEntry( vIsRoot, Vec_IntEntry(vAdds, 6*Box+3), 1 );
        Vec_BitWriteEntry( vIsRoot, Vec_IntEntry(vAdds, 6*Box+4), 1 );
        Vec_WecPush( pBox->vAdds, Rank, Box );
    }
    // sort each level
    Vec_WecForEachLevel( pBox->vAdds, vLevel, i )
        Vec_IntSort( vLevel, 0 );

    // set phases starting from roots
    vMap = Acec_TreeCarryMap( p, vAdds, pBox->vAdds );
    Vec_WecForEachLevelReverse( pBox->vAdds, vLevel, i )
        Vec_IntForEachEntry( vLevel, Box, k )
            if ( !Vec_BitEntry( vIsLeaf, Vec_IntEntry(vAdds, 6*Box+4) ) )
            {
                //printf( "Pushing phase of output %d of box %d\n", Vec_IntEntry(vAdds, 6*Box+4), Box );
                Acec_TreePhases_rec( p, vAdds, vMap, Vec_IntEntry(vAdds, 6*Box+4), Vec_IntEntry(vAdds, 6*Box+2) != 0, vVisit );
            }
    Acec_TreeVerifyPhases( p, vAdds, pBox->vAdds );
    Acec_TreeVerifyPhases2( p, vAdds, pBox->vAdds );
    Vec_BitFree( vVisit );
    Vec_IntFree( vMap );

    // collect inputs/outputs
    Vec_BitWriteEntry( vIsRoot, 0, 1 );
    Vec_WecForEachLevel( pBox->vAdds, vLevel, i )
        Vec_IntForEachEntry( vLevel, Box, j )
        {
            for ( k = 0; k < 3; k++ )
                if ( !Vec_BitEntry( vIsRoot, Vec_IntEntry(vAdds, 6*Box+k) ) )
                    Vec_WecPush( pBox->vLeafLits, i, Abc_Var2Lit(Vec_IntEntry(vAdds, 6*Box+k), Acec_SignBit2(vAdds, Box, k)) );
            for ( k = 3; k < 5; k++ )
                if ( !Vec_BitEntry( vIsLeaf, Vec_IntEntry(vAdds, 6*Box+k) ) )
                {
                    //if ( Vec_IntEntry(vAdds, 6*Box+k) == 10942 )
                    //{
                    //    printf( "++++++++++++ Skipping special\n" );
                    //    continue;
                    //}
                    Vec_WecPush( pBox->vRootLits, k == 4 ? i + 1 : i, Abc_Var2Lit(Vec_IntEntry(vAdds, 6*Box+k), Acec_SignBit2(vAdds, Box, k)) );
                }
            if ( Vec_IntEntry(vAdds, 6*Box+2) == 0 && Acec_SignBit2(vAdds, Box, 2) )
                Vec_WecPush( pBox->vLeafLits, i, 1 );
        }
    Vec_BitFree( vIsLeaf );
    Vec_BitFree( vIsRoot );
    // sort each level
    Vec_WecForEachLevel( pBox->vLeafLits, vLevel, i )
        Vec_IntSort( vLevel, 0 );
    Vec_WecForEachLevel( pBox->vRootLits, vLevel, i )
        Vec_IntSort( vLevel, 1 );
    //return pBox;
/*
    // push literals forward
    //Vec_WecPrint( pBox->vLeafLits, 0 );
    Vec_WecForEachLevel( pBox->vLeafLits, vLevel, i )
    {
        int This, Prev = Vec_IntEntry(vLevel, 0);
        Vec_IntForEachEntryStart( vLevel, This, j, 1 )
        {
            if ( Prev != This )
            {
                Prev = This;
                continue;
            }
            if ( i+1 >= Vec_WecSize(pBox->vLeafLits) )
                continue;
            Vec_IntPushOrder( Vec_WecEntry(pBox->vLeafLits, i+1), This );
            Vec_IntDrop( vLevel, j-- );
            Vec_IntDrop( vLevel, j-- );
            Prev = -1;
            Count++;
        }
    }
    printf( "Pushed forward %d input literals.\n", Count );
*/
    //Vec_WecPrint( pBox->vLeafLits, 0 );
    return pBox;
}
void Acec_CreateBoxTest( Gia_Man_t * p )
{
    Acec_Box_t * pBox;
    Vec_Wec_t * vTrees;
    Vec_Int_t * vTree;

    abctime clk = Abc_Clock();
    Vec_Int_t * vAdds = Ree_ManComputeCuts( p, NULL, 1 );
    int i, nFadds = Ree_ManCountFadds( vAdds );
    printf( "Detected %d adders (%d FAs and %d HAs).  ", Vec_IntSize(vAdds)/6, nFadds, Vec_IntSize(vAdds)/6-nFadds );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );

    clk = Abc_Clock();
    vTrees = Acec_TreeFindTrees( p, vAdds, NULL, 0, 0 );
    printf( "Collected %d trees with %d adders in them.  ", Vec_WecSize(vTrees), Vec_WecSizeSize(vTrees)/2 );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    //Vec_WecPrint( vTrees, 0 );

    Vec_WecForEachLevel( vTrees, vTree, i )
    {
        pBox = Acec_CreateBox( p, vAdds, Vec_WecEntry(vTrees, i) );
        printf( "Processing tree %d:  Ranks = %d.  Adders = %d.  Leaves = %d.  Roots = %d.\n", 
            i, Vec_WecSize(pBox->vAdds), Vec_WecSizeSize(pBox->vAdds), 
            Vec_WecSizeSize(pBox->vLeafLits), Vec_WecSizeSize(pBox->vRootLits)  );
        Acec_TreePrintBox( pBox, vAdds );
        Acec_BoxFreeP( &pBox );
    }

    Vec_WecFree( vTrees );
    Vec_IntFree( vAdds );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Acec_Box_t * Acec_DeriveBox( Gia_Man_t * p, Vec_Bit_t * vIgnore, int fFilterIn, int fFilterOut, int fVerbose )
{
    Acec_Box_t * pBox = NULL;
    Vec_Int_t * vAdds = Ree_ManComputeCuts( p, NULL, fVerbose );
    Vec_Wec_t * vTrees = Acec_TreeFindTrees( p, vAdds, vIgnore, fFilterIn, fFilterOut );
    if ( vTrees && Vec_WecSize(vTrees) > 0 )
    {
        pBox = Acec_CreateBox( p, vAdds, Vec_WecEntry(vTrees, 0) );
        Acec_VerifyBoxLeaves( pBox, vIgnore );
    }
    if ( pBox )//&& fVerbose )
        printf( "Processing tree %d:  Ranks = %d.  Adders = %d.  Leaves = %d.  Roots = %d.\n", 
            0, Vec_WecSize(pBox->vAdds), Vec_WecSizeSize(pBox->vAdds), 
            Vec_WecSizeSize(pBox->vLeafLits), Vec_WecSizeSize(pBox->vRootLits)  );
    if ( pBox && fVerbose )
        Acec_TreePrintBox( pBox, vAdds );
    //Acec_PrintAdders( pBox0->vAdds, vAdds );
    //Acec_MultDetectInputs( p, pBox->vLeafLits, pBox->vRootLits );
    Vec_WecFreeP( &vTrees );
    Vec_IntFree( vAdds );
    return pBox;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

