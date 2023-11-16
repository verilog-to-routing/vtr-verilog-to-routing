/**CFile****************************************************************

  FileName    [acecPool.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [CEC for arithmetic circuits.]

  Synopsis    [Core procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: acecPool.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "acecInt.h"
#include "misc/vec/vecWec.h"
#include "misc/extra/extra.h"

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
Vec_Int_t * Acec_ManCreateCarryMap( Gia_Man_t * p, Vec_Int_t * vAdds )
{
    // map carries into their boxes
    Vec_Int_t * vCarryMap = Vec_IntStartFull( Gia_ManObjNum(p) );  int i;
    for ( i = 0; 6*i < Vec_IntSize(vAdds); i++ )
        Vec_IntWriteEntry( vCarryMap, Vec_IntEntry(vAdds, 6*i+4), i );
    return vCarryMap;
}
int Acec_ManCheckCarryMap( Gia_Man_t * p, int Carry, Vec_Int_t * vAdds, Vec_Int_t * vCarryMap )
{
    int iBox = Vec_IntEntry( vCarryMap, Carry );
    assert( iBox >= 0 );
    return Vec_IntEntry( vCarryMap, Vec_IntEntry(vAdds, 6*iBox+0) ) >= 0 ||
           Vec_IntEntry( vCarryMap, Vec_IntEntry(vAdds, 6*iBox+1) ) >= 0 ||
           Vec_IntEntry( vCarryMap, Vec_IntEntry(vAdds, 6*iBox+2) ) >= 0;
}
Vec_Int_t * Acec_ManCollectCarryRoots( Gia_Man_t * p, Vec_Int_t * vAdds )
{
    Vec_Int_t * vCarryRoots = Vec_IntAlloc( 100 );
    Vec_Bit_t * vIns = Vec_BitStart( Gia_ManObjNum(p) );  int i;
    // marks box inputs
    for ( i = 0; 6*i < Vec_IntSize(vAdds); i++ )
    {
        Vec_BitWriteEntry( vIns, Vec_IntEntry(vAdds, 6*i+0), 1 );
        Vec_BitWriteEntry( vIns, Vec_IntEntry(vAdds, 6*i+1), 1 );
        Vec_BitWriteEntry( vIns, Vec_IntEntry(vAdds, 6*i+2), 1 );
    }
    // collect roots
    for ( i = 0; 6*i < Vec_IntSize(vAdds); i++ )
        if ( !Vec_BitEntry(vIns, Vec_IntEntry(vAdds, 6*i+4)) )
            Vec_IntPush( vCarryRoots, Vec_IntEntry(vAdds, 6*i+4) );
    Vec_BitFree( vIns );
    return vCarryRoots;
}
Vec_Int_t * Acec_ManCollectXorRoots( Gia_Man_t * p, Vec_Int_t * vXors )
{
    Vec_Int_t * vXorRoots = Vec_IntAlloc( 100 );
    Vec_Bit_t * vIns = Vec_BitStart( Gia_ManObjNum(p) );  int i;
    // marks box inputs
    for ( i = 0; 4*i < Vec_IntSize(vXors); i++ )
    {
        Vec_BitWriteEntry( vIns, Vec_IntEntry(vXors, 4*i+1), 1 );
        Vec_BitWriteEntry( vIns, Vec_IntEntry(vXors, 4*i+2), 1 );
        Vec_BitWriteEntry( vIns, Vec_IntEntry(vXors, 4*i+3), 1 );
    }
    // collect roots
    for ( i = 0; 4*i < Vec_IntSize(vXors); i++ )
        if ( !Vec_BitEntry(vIns, Vec_IntEntry(vXors, 4*i)) )
            Vec_IntPush( vXorRoots, Vec_IntEntry(vXors, 4*i) );
    Vec_BitFree( vIns );
    return vXorRoots;
}
void Acec_ManCountXorTreeInputs_rec( Gia_Man_t * p, int Node, Vec_Int_t * vXors, Vec_Int_t * vXorMap, Vec_Bit_t * vIsCarryRoot, Vec_Int_t * vCarryRootSet, Vec_Int_t * vXorSet )
{
    int k, iXorBox;
    if ( Node == 0 || Gia_ObjIsTravIdCurrentId(p, Node) )
        return;
    Gia_ObjSetTravIdCurrentId(p, Node);
    iXorBox = Vec_IntEntry( vXorMap, Node );
    if ( iXorBox == -1 )
    {
        if ( Vec_BitEntry(vIsCarryRoot, Node) )
            Vec_IntPush( vCarryRootSet, Node );
        return;
    }
    for ( k = 1; k < 4; k++ )
        Acec_ManCountXorTreeInputs_rec( p, Vec_IntEntry(vXors, 4*iXorBox+k), vXors, vXorMap, vIsCarryRoot, vCarryRootSet, vXorSet );
    Vec_IntPush( vXorSet, Vec_IntEntry(vXors, 4*iXorBox) );
}
Vec_Wec_t * Acec_ManCollectCarryRootSets( Gia_Man_t * p, Vec_Int_t * vAdds, Vec_Int_t * vCarryMap, Vec_Int_t * vXors, Vec_Int_t * vXorRoots, Vec_Int_t * vCarryRoots )
{
    Vec_Wec_t * vCarryRootSets = Vec_WecAlloc( 100 ); // XorBoxes, CarryRoots, AdderBoxes, Ins/Ranks, Outs/Ranks
    Vec_Int_t * vCarryRootSet = Vec_IntAlloc( 100 );
    Vec_Bit_t * vIsCarryRoot = Vec_BitStart( Gia_ManObjNum(p) );
    Vec_Int_t * vXorSet = Vec_IntAlloc( 100 ), * vLevel;
    int i, k, XorRoot, CarryRoot;
    // map XORs into their cuts
    Vec_Int_t * vXorMap = Vec_IntStartFull( Gia_ManObjNum(p) );  
    for ( i = 0; 4*i < Vec_IntSize(vXors); i++ )
        Vec_IntWriteEntry( vXorMap, Vec_IntEntry(vXors, 4*i), i );
    // create carry root marks
    Vec_IntForEachEntry( vCarryRoots, CarryRoot, i )
        Vec_BitWriteEntry( vIsCarryRoot, CarryRoot, 1 );
    // collect carry roots attached to each XOR root
    Vec_IntForEachEntry( vXorRoots, XorRoot, i )
    {
        Vec_IntClear( vXorSet );
        Vec_IntClear( vCarryRootSet );
        Gia_ManIncrementTravId( p );
        Acec_ManCountXorTreeInputs_rec( p, XorRoot, vXors, vXorMap, vIsCarryRoot, vCarryRootSet, vXorSet );
        // skip trivial
        Vec_IntForEachEntry( vCarryRootSet, CarryRoot, k )
            if ( Acec_ManCheckCarryMap(p, CarryRoot, vAdds, vCarryMap) )
                break;
        if ( k == Vec_IntSize(vCarryRootSet) )
            continue;
        Vec_IntSort( vCarryRootSet, 0 );
        vLevel = Vec_WecPushLevel( vCarryRootSets );
        Vec_IntAppend( vLevel, vXorSet );
        vLevel = Vec_WecPushLevel( vCarryRootSets );
        Vec_IntAppend( vLevel, vCarryRootSet );
        vLevel = Vec_WecPushLevel( vCarryRootSets );
        vLevel = Vec_WecPushLevel( vCarryRootSets );
        vLevel = Vec_WecPushLevel( vCarryRootSets );
        // unmark carry root set
        Vec_IntForEachEntry( vCarryRootSet, CarryRoot, k )
        {
            assert( Vec_BitEntry(vIsCarryRoot, CarryRoot) );
            Vec_BitWriteEntry( vIsCarryRoot, CarryRoot, 0 );
        }
    }
    Vec_IntFree( vCarryRootSet );
    Vec_IntFree( vXorSet );
    Vec_IntFree( vXorMap );
    // collect unmarked carry roots
    Vec_IntForEachEntry( vCarryRoots, CarryRoot, k )
    {
        if ( !Vec_BitEntry(vIsCarryRoot, CarryRoot) )
            continue;
        if ( !Acec_ManCheckCarryMap(p, CarryRoot, vAdds, vCarryMap) )
            continue;
        vLevel = Vec_WecPushLevel( vCarryRootSets );
        vLevel = Vec_WecPushLevel( vCarryRootSets );
        Vec_IntFill( vLevel, 1, CarryRoot );
        vLevel = Vec_WecPushLevel( vCarryRootSets );
        vLevel = Vec_WecPushLevel( vCarryRootSets );
        vLevel = Vec_WecPushLevel( vCarryRootSets );
    }
    Vec_BitFree( vIsCarryRoot );
    return vCarryRootSets;
}
int Acec_ManCompareTwo( int * pPair0, int * pPair1 )
{
    if ( pPair0[1] < pPair1[1] ) return -1;
    if ( pPair0[1] > pPair1[1] ) return  1;
    return 0;
}
void Acec_ManCollectInsOuts( Gia_Man_t * p, Vec_Int_t * vAdds, Vec_Int_t * vBoxes, Vec_Int_t * vBoxRanks, Vec_Bit_t * vBoxIns, Vec_Bit_t * vBoxOuts, Vec_Int_t * vResIns, Vec_Int_t * vResOuts )
{
    int i, k, iBox, iObj, Rank, RankMax = 0;
    // mark box inputs/outputs
    Vec_IntForEachEntry( vBoxes, iBox, i )
    {
        Vec_BitWriteEntry( vBoxIns, Vec_IntEntry(vAdds, 6*iBox+0), 1 );
        Vec_BitWriteEntry( vBoxIns, Vec_IntEntry(vAdds, 6*iBox+1), 1 );
        Vec_BitWriteEntry( vBoxIns, Vec_IntEntry(vAdds, 6*iBox+2), 1 );
        Vec_BitWriteEntry( vBoxOuts, Vec_IntEntry(vAdds, 6*iBox+3), 1 );
        Vec_BitWriteEntry( vBoxOuts, Vec_IntEntry(vAdds, 6*iBox+4), 1 );
    }
    // collect unmarked inputs/output and their ranks
    Vec_IntForEachEntry( vBoxes, iBox, i )
    {
        for ( k = 0; k < 3; k++ )
            if ( !Vec_BitEntry(vBoxOuts, Vec_IntEntry(vAdds, 6*iBox+k)) )
                Vec_IntPushTwo( vResIns, Vec_IntEntry(vAdds, 6*iBox+k), Vec_IntEntry(vBoxRanks, iBox) );
        for ( k = 3; k < 5; k++ )
            if ( Vec_IntEntry(vAdds, 6*iBox+k) && !Vec_BitEntry(vBoxIns, Vec_IntEntry(vAdds, 6*iBox+k)) )
                Vec_IntPushTwo( vResOuts, Vec_IntEntry(vAdds, 6*iBox+k), Vec_IntEntry(vBoxRanks, iBox)-(int)(k==4) );
    }
    // unmark box inputs/outputs
    Vec_IntForEachEntry( vBoxes, iBox, i )
    {
        Vec_BitWriteEntry( vBoxIns, Vec_IntEntry(vAdds, 6*iBox+0), 0 );
        Vec_BitWriteEntry( vBoxIns, Vec_IntEntry(vAdds, 6*iBox+1), 0 );
        Vec_BitWriteEntry( vBoxIns, Vec_IntEntry(vAdds, 6*iBox+2), 0 );
        Vec_BitWriteEntry( vBoxOuts, Vec_IntEntry(vAdds, 6*iBox+3), 0 );
        Vec_BitWriteEntry( vBoxOuts, Vec_IntEntry(vAdds, 6*iBox+4), 0 );
    }
    // normalize ranks
    Vec_IntForEachEntryDouble( vResIns, iObj, Rank, k )
        RankMax = Abc_MaxInt( RankMax, Rank );
    Vec_IntForEachEntryDouble( vResOuts, iObj, Rank, k )
        RankMax = Abc_MaxInt( RankMax, Rank );
    Vec_IntForEachEntryDouble( vResIns, iObj, Rank, k )
        Vec_IntWriteEntry( vResIns, k+1, 1 + RankMax - Rank );
    Vec_IntForEachEntryDouble( vResOuts, iObj, Rank, k )
        Vec_IntWriteEntry( vResOuts, k+1, 1 + RankMax - Rank );
    // sort by rank
    qsort( Vec_IntArray(vResIns),  (size_t)(Vec_IntSize(vResIns)/2),  8, (int (*)(const void *, const void *))Acec_ManCompareTwo );
    qsort( Vec_IntArray(vResOuts), (size_t)(Vec_IntSize(vResOuts)/2), 8, (int (*)(const void *, const void *))Acec_ManCompareTwo );
}
void Acec_ManCollectBoxSets_rec( Gia_Man_t * p, int Carry, int iRank, Vec_Int_t * vAdds, Vec_Int_t * vCarryMap, Vec_Int_t * vBoxes, Vec_Int_t * vBoxRanks )
{
    int iBox = Vec_IntEntry( vCarryMap, Carry );
    if ( iBox == -1 )
        return;
    Acec_ManCollectBoxSets_rec( p, Vec_IntEntry(vAdds, 6*iBox+0), iRank+1, vAdds, vCarryMap, vBoxes, vBoxRanks );
    Acec_ManCollectBoxSets_rec( p, Vec_IntEntry(vAdds, 6*iBox+1), iRank+1, vAdds, vCarryMap, vBoxes, vBoxRanks );
    if ( Vec_IntEntry(vAdds, 6*iBox+2) )
        Acec_ManCollectBoxSets_rec( p, Vec_IntEntry(vAdds, 6*iBox+2), iRank+1, vAdds, vCarryMap, vBoxes, vBoxRanks );
    Vec_IntPush( vBoxes, iBox );
    Vec_IntWriteEntry( vBoxRanks, iBox, iRank );
}
Vec_Wec_t * Acec_ManCollectBoxSets( Gia_Man_t * p, Vec_Int_t * vAdds, Vec_Int_t * vXors )
{
    Vec_Int_t * vCarryMap   = Acec_ManCreateCarryMap( p, vAdds );
    Vec_Int_t * vCarryRoots = Acec_ManCollectCarryRoots( p, vAdds );
    Vec_Int_t * vXorRoots   = Acec_ManCollectXorRoots( p, vXors );
    Vec_Wec_t * vBoxSets    = Acec_ManCollectCarryRootSets( p, vAdds, vCarryMap, vXors, vXorRoots, vCarryRoots );
    Vec_Int_t * vBoxRanks   = Vec_IntStart( Vec_IntSize(vAdds)/6 );
    Vec_Bit_t * vBoxIns     = Vec_BitStart( Gia_ManObjNum(p) );
    Vec_Bit_t * vBoxOuts    = Vec_BitStart( Gia_ManObjNum(p) ); int i, k, Root;
    Vec_IntFree( vCarryRoots );
    Vec_IntFree( vXorRoots );
    // collect boxes for each carry set
    assert( Vec_WecSize(vBoxSets) % 5 == 0 );
    for ( i = 0; 5*i < Vec_WecSize(vBoxSets); i++ )
    {
        Vec_Int_t * vRoots = Vec_WecEntry( vBoxSets, 5*i+1 );
        Vec_Int_t * vBoxes = Vec_WecEntry( vBoxSets, 5*i+2 );
        Vec_Int_t * vIns   = Vec_WecEntry( vBoxSets, 5*i+3 );
        Vec_Int_t * vOuts  = Vec_WecEntry( vBoxSets, 5*i+4 );
        Vec_IntForEachEntry( vRoots, Root, k )
            Acec_ManCollectBoxSets_rec( p, Root, 1, vAdds, vCarryMap, vBoxes, vBoxRanks );
        Acec_ManCollectInsOuts( p, vAdds, vBoxes, vBoxRanks, vBoxIns, vBoxOuts, vIns, vOuts );
    }
    Vec_IntFree( vBoxRanks );
    Vec_BitFree( vBoxIns );
    Vec_BitFree( vBoxOuts );
    Vec_IntFree( vCarryMap );
    return vBoxSets;
}
void Acec_ManPrintRanks2( Vec_Int_t * vPairs )
{
    int k, iObj, Rank;
    Vec_IntForEachEntryDouble( vPairs, iObj, Rank, k )
        printf( "%d ", Rank );
    printf( "\n" );
}
void Acec_ManPrintRanks( Vec_Int_t * vPairs )
{
    int k, iObj, Count, Rank, RankMax = 0;
    Vec_Int_t * vCounts = Vec_IntStart( 100 );
    Vec_IntForEachEntryDouble( vPairs, iObj, Rank, k )
    {
        Vec_IntFillExtra( vCounts, Rank+1, 0 );
        Vec_IntAddToEntry( vCounts, Rank, 1 );
        RankMax = Abc_MaxInt( RankMax, Rank );
    }
    Vec_IntForEachEntryStartStop( vCounts, Count, Rank, 1, RankMax+1 )
        printf( "%2d=%2d ", Rank, Count );
    printf( "\n" );
    Vec_IntFree( vCounts );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Acec_ManProfile( Gia_Man_t * p, int fVerbose )
{
    abctime clk = Abc_Clock();
    Vec_Wec_t * vBoxes; int i;
    Vec_Int_t * vXors, * vAdds = Ree_ManComputeCuts( p, &vXors, fVerbose );

    //Ree_ManPrintAdders( vAdds, 1 );
    printf( "Detected %d full-adders and %d half-adders.  Found %d XOR-cuts.  ", Ree_ManCountFadds(vAdds), Vec_IntSize(vAdds)/6-Ree_ManCountFadds(vAdds), Vec_IntSize(vXors)/4 );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );

    clk = Abc_Clock();
    vBoxes = Acec_ManCollectBoxSets( p, vAdds, vXors );
    printf( "Detected %d adder-tree%s.  ", Vec_WecSize(vBoxes)/5, Vec_WecSize(vBoxes)/5 > 1 ? "s":"" );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );

    if ( fVerbose )
    for ( i = 0; 5*i < Vec_WecSize(vBoxes); i++ )
    {
        printf( "Tree %3d : ",     i );
        printf( "Xor = %4d  ",     Vec_IntSize(Vec_WecEntry(vBoxes,5*i+0)) );
        printf( "Root = %4d  ",    Vec_IntSize(Vec_WecEntry(vBoxes,5*i+1)) );
        //printf( "(Top = %5d)  ",   Vec_IntEntryLast(Vec_WecEntry(vBoxes,5*i+1)) );
        printf( "Adder = %4d  ",   Vec_IntSize(Vec_WecEntry(vBoxes,5*i+2)) );
        printf( "In = %4d  ",      Vec_IntSize(Vec_WecEntry(vBoxes,5*i+3))/2 );
        printf( "Out = %4d  ",     Vec_IntSize(Vec_WecEntry(vBoxes,5*i+4))/2 );
        printf( "\n" );
        printf( "           Ins:  " );
        Acec_ManPrintRanks( Vec_WecEntry(vBoxes,5*i+3) );
        printf( "           Outs: " );
        Acec_ManPrintRanks( Vec_WecEntry(vBoxes,5*i+4) );
    }

    Vec_IntFree( vXors );
    Vec_IntFree( vAdds );
    Vec_WecFree( vBoxes );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Bit_t * Acec_ManPoolGetPointed( Gia_Man_t * p, Vec_Int_t * vAdds )
{
    Vec_Bit_t * vMarks = Vec_BitStart( Gia_ManObjNum(p) );
    int i, k;
    for ( i = 0; 6*i < Vec_IntSize(vAdds); i++ )
        for ( k = 0; k < 3; k++ )
            Vec_BitWriteEntry( vMarks, Vec_IntEntry(vAdds, 6*i+k), 1 );
    return vMarks;
}

Vec_Int_t * Acec_ManPoolTopMost( Gia_Man_t * p, Vec_Int_t * vAdds )
{
    int i, k, iTop, fVerbose = 0;
    Vec_Int_t * vTops = Vec_IntAlloc( 1000 );
    Vec_Bit_t * vMarks = Acec_ManPoolGetPointed( p, vAdds );

    for ( i = 0; 6*i < Vec_IntSize(vAdds); i++ )
        if ( !Vec_BitEntry(vMarks, Vec_IntEntry(vAdds, 6*i+3)) && 
             !Vec_BitEntry(vMarks, Vec_IntEntry(vAdds, 6*i+4)) )
            Vec_IntPush( vTops, i );

    if ( fVerbose )
    Vec_IntForEachEntry( vTops, iTop, i )
    {
        printf( "%4d : ", iTop );
        for ( k = 0; k < 3; k++ )
            printf( "%4d ", Vec_IntEntry(vAdds, 6*iTop+k) );
        printf( " -> " );
        for ( k = 3; k < 5; k++ )
            printf( "%4d ", Vec_IntEntry(vAdds, 6*iTop+k) );
        printf( "\n" );
    }

    Vec_BitFree( vMarks );
    return vTops;
}
void Acec_ManPool( Gia_Man_t * p )
{
    Vec_Int_t * vTops, * vTree;
    Vec_Wec_t * vTrees;

    abctime clk = Abc_Clock();
    Vec_Int_t * vAdds = Ree_ManComputeCuts( p, NULL, 1 );
    int i, nFadds = Ree_ManCountFadds( vAdds );
    printf( "Detected %d FAs and %d HAs.  ", nFadds, Vec_IntSize(vAdds)/6-nFadds );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );

    clk = Abc_Clock();
    nFadds = Ree_ManCountFadds( vAdds );
    printf( "Detected %d FAs and %d HAs.  ", nFadds, Vec_IntSize(vAdds)/6-nFadds );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );

    //Ree_ManPrintAdders( vAdds, 1 );

    // detect topmost nodes
    vTops = Acec_ManPoolTopMost( p, vAdds );
    printf( "Detected %d topmost adder%s.\n", Vec_IntSize(vTops), Vec_IntSize(vTops) > 1 ? "s":"" );

    // collect adder trees
    vTrees = Gia_PolynCoreOrderArray( p, vAdds, vTops );
    Vec_WecForEachLevel( vTrees, vTree, i )
        printf( "Adder %5d : Tree with %5d nodes.\n", Vec_IntEntry(vTops, i), Vec_IntSize(vTree) );

    Vec_WecFree( vTrees );
    Vec_IntFree( vAdds );
    Vec_IntFree( vTops );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

