/**CFile****************************************************************

  FileName    [giaShrink7.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Implementation of DAG-aware unmapping for 6-input cuts.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaShrink6.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "misc/vec/vecHash.h"
#include "misc/util/utilTruth.h"


ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// operation manager
typedef struct Unm_Man_t_ Unm_Man_t;
struct Unm_Man_t_
{
    Gia_Man_t *        pGia;           // user's AIG
    Gia_Man_t *        pNew;           // constructed AIG
    Hash_IntMan_t *    pHash;          // hash table
    int                nNewSize;       // expected size of new manager
    Vec_Int_t *        vUsed;          // used nodes
    Vec_Int_t *        vId2Used;       // mapping of obj IDs into used node IDs
    Vec_Wrd_t *        vTruths;        // truth tables
    Vec_Int_t *        vLeaves;        // temporary storage for leaves
    abctime            clkStart;       // starting the clock
};

extern word Shr_ManComputeTruth6( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vLeaves, Vec_Wrd_t * vTruths );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Unm_Man_t * Unm_ManAlloc( Gia_Man_t * pGia )
{
    Unm_Man_t * p;
    p = ABC_CALLOC( Unm_Man_t, 1 );
    p->clkStart    = Abc_Clock();
    p->nNewSize    = 3 * Gia_ManObjNum(pGia) / 2;
    p->pGia        = pGia;
    p->pNew        = Gia_ManStart( p->nNewSize );
    p->pNew->pName = Abc_UtilStrsav( pGia->pName );
    p->pNew->pSpec = Abc_UtilStrsav( pGia->pSpec );
    Gia_ManHashAlloc( p->pNew );
    Gia_ManCleanLevels( p->pNew, p->nNewSize );
    // allocate traversal IDs
    p->pNew->nObjs = p->nNewSize;
    Gia_ManIncrementTravId( p->pNew );
    p->pNew->nObjs = 1;
    // start hashing
    p->pHash = Hash_IntManStart( 1000 );
    // truth tables
    p->vLeaves = Vec_IntStart( 10 );
    return p;
}
Gia_Man_t * Unm_ManFree( Unm_Man_t * p )
{
    Gia_Man_t * pTemp = p->pNew; p->pNew = NULL;
    Gia_ManHashStop( pTemp );
    Vec_IntFreeP( &pTemp->vLevels );
    Gia_ManSetRegNum( pTemp, Gia_ManRegNum(p->pGia) );
    // truth tables
    Vec_WrdFreeP( &p->vTruths );
    Vec_IntFreeP( &p->vLeaves );
    Vec_IntFreeP( &p->vUsed );
    Vec_IntFreeP( &p->vId2Used );
    // free data structures
    Hash_IntManStop( p->pHash );
    ABC_FREE( p );

    Gia_ManStop( pTemp );
    pTemp = NULL;

    return pTemp;
}

/**Function*************************************************************

  Synopsis    [Computes information about node pairs.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Unm_ManPrintPairStats( Hash_IntMan_t * pHash, int nTotal0, int nPairs0, int nPairs1, int fUseLit )
{
    int i, Num, nRefs, nPairs = 0, nTotal = 0, Counter[21] = {0};
    Num = Hash_IntManEntryNum( pHash );
    for ( i = 1; i <= Num; i++ )
    {
        nRefs = Abc_MinInt( 20, Hash_IntObjData2(pHash, i) );
        nTotal += nRefs;
        Counter[nRefs]++;
        nPairs += (nRefs > 1);
/*
        if ( fUseLit )
            printf( "(%c%c, %c%c) %d\n", 
                Abc_LitIsCompl(Hash_IntObjData0(pHash, i)-2) ? '!' : ' ', 
                'a' + Abc_Lit2Var(Hash_IntObjData0(pHash, i)-2), 
                Abc_LitIsCompl(Hash_IntObjData1(pHash, i)-2) ? '!' : ' ', 
                'a' + Abc_Lit2Var(Hash_IntObjData1(pHash, i)-2), nRefs );
        else
            printf( "( %c,  %c) %d\n", 
                'a' + Hash_IntObjData0(pHash, i)-1, 
                'a' + Hash_IntObjData1(pHash, i)-1, nRefs );
*/
//        printf( "(%4d, %4d) %d\n", Hash_IntObjData0(pHash, i), Hash_IntObjData1(pHash, i), nRefs );

    }
    printf( "Statistics for pairs appearing less than 20 times:\n" );
    for ( i = 0; i < 21; i++ )
        if ( Counter[i] > 0 )
            printf( "%3d : %7d  %7.2f %%\n", i, Counter[i], 100.0 * Counter[i] * i / Abc_MaxInt(nTotal, 1) );
    printf( "Pairs:  Total = %8d    Init = %8d %7.2f %%    Final = %8d %7.2f %%    Real = %8d %7.2f %%\n", nTotal0, 
        nPairs0,  100.0 * nPairs0 / Abc_MaxInt(nTotal0, 1), 
        nPairs, 100.0 * nPairs / Abc_MaxInt(nTotal0, 1), 
        nPairs1, 100.0 * nPairs1 / Abc_MaxInt(nTotal0, 1) );
    return nPairs;
}
Vec_Int_t * Unm_ManComputePairs( Unm_Man_t * p, int fVerbose )
{
    Gia_Obj_t * pObj;
    Vec_Int_t * vPairs = Vec_IntAlloc( 1000 );
    Vec_Int_t * vNum2Obj = Vec_IntStart( 1 );
    Hash_IntMan_t * pHash = Hash_IntManStart( 1000 );
    int nTotal = 0, nPairs0 = 0, nPairs = 0;
    int i, k, j, FanK, FanJ, Num, nRefs;
    Gia_ManSetRefsMapped( p->pGia );
    Gia_ManForEachLut( p->pGia, i )
    {
        nTotal += Gia_ObjLutSize(p->pGia, i) * (Gia_ObjLutSize(p->pGia, i) - 1) / 2;
        pObj = Gia_ManObj( p->pGia, i );
        // collect leaves of this gate  
        Vec_IntClear( p->vLeaves );
        Gia_LutForEachFanin( p->pGia, i, Num, k )
            if ( Gia_ObjRefNumId(p->pGia, Num) > 1 )
                Vec_IntPush( p->vLeaves, Num );
        if ( Vec_IntSize(p->vLeaves) < 2 )
            continue;
        nPairs0 += Vec_IntSize(p->vLeaves) * (Vec_IntSize(p->vLeaves) - 1) / 2;
        // enumerate pairs
        Vec_IntForEachEntry( p->vLeaves, FanK, k )
        Vec_IntForEachEntryStart( p->vLeaves, FanJ, j, k+1 )
        {
            if ( FanK > FanJ )
                ABC_SWAP( int, FanK, FanJ );
            Num = Hash_Int2ManInsert( pHash, FanK, FanJ, 0 );
            nRefs = Hash_Int2ObjInc(pHash, Num);
            if ( nRefs == 0 )
            {
                assert( Num == Hash_IntManEntryNum(pHash) );
                assert( Num == Vec_IntSize(vNum2Obj) );
                Vec_IntPush( vNum2Obj, i );
                continue;
            }
            if ( nRefs == 1 )
            {
                assert( Num < Vec_IntSize(vNum2Obj) );
                Vec_IntPush( vPairs, Vec_IntEntry(vNum2Obj, Num) );
                Vec_IntPush( vPairs, FanK );
                Vec_IntPush( vPairs, FanJ);
            }
            Vec_IntPush( vPairs, i );
            Vec_IntPush( vPairs, FanK );
            Vec_IntPush( vPairs, FanJ );
        }
    }
    Vec_IntFree( vNum2Obj );
    if ( fVerbose )
        nPairs = Unm_ManPrintPairStats( pHash, nTotal, nPairs0, Vec_IntSize(vPairs) / 3, 0 );
    Hash_IntManStop( pHash );
    return vPairs;    
}
// finds used nodes
Vec_Int_t * Unm_ManFindUsedNodes( Vec_Int_t * vPairs, int nObjs )
{
    Vec_Int_t * vNodes = Vec_IntAlloc( 1000 );
    Vec_Str_t * vMarks = Vec_StrStart( nObjs ); int i;
    for ( i = 0; i < Vec_IntSize(vPairs); i += 3 )
        Vec_StrWriteEntry( vMarks, Vec_IntEntry(vPairs, i), 1 );
    for ( i = 0; i < nObjs; i++ )
        if ( Vec_StrEntry( vMarks, i ) )
            Vec_IntPush( vNodes, i );
    Vec_StrFree( vMarks );
    printf( "The number of used nodes = %d\n", Vec_IntSize(vNodes) );
    return vNodes;
}
// computes truth table for selected nodes
Vec_Wrd_t * Unm_ManComputeTruths( Unm_Man_t * p )
{
    Vec_Wrd_t * vTruthsTemp, * vTruths;
    int i, k, iObj, iNode;
    word uTruth;
    vTruths = Vec_WrdAlloc( Vec_IntSize(p->vUsed) );
    vTruthsTemp = Vec_WrdStart( Gia_ManObjNum(p->pGia) );
    Vec_IntForEachEntry( p->vUsed, iObj, i )
    {
        assert( Gia_ObjIsLut(p->pGia, iObj) );
        // collect leaves of this gate  
        Vec_IntClear( p->vLeaves );
        Gia_LutForEachFanin( p->pGia, iObj, iNode, k )
            Vec_IntPush( p->vLeaves, iNode );
        assert( Vec_IntSize(p->vLeaves) <= 6 );
        // compute truth table 
        uTruth = Shr_ManComputeTruth6( p->pGia, Gia_ManObj(p->pGia, iObj), p->vLeaves, vTruthsTemp );
        Vec_WrdPush( vTruths, uTruth );
//        if ( i % 100 == 0 )
//            Kit_DsdPrintFromTruth( (unsigned *)&uTruth, 6 ), printf( "\n" );
    }
    Vec_WrdFreeP( &vTruthsTemp );
    return vTruths;
}

/**Function*************************************************************

  Synopsis    [Collects decomposable pairs.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Unm_ManCollectDecomp( Unm_Man_t * p, Vec_Int_t * vPairs, int fVerbose )
{
    word uTruth; int nNonUnique = 0;
    int i, k, j, s, iObj, iNode, iUsed, FanK, FanJ, Res, Num, nRefs;
    Vec_Int_t * vNum2Obj = Vec_IntStart( 1 );
    Vec_Int_t * vPairs2 = Vec_IntAlloc( 1000 );
    assert( Hash_IntManEntryNum(p->pHash) == 0 );
    for ( i = 0; i < Vec_IntSize(vPairs); i += 3 )
    {
        iObj = Vec_IntEntry( vPairs, i );
        assert( Gia_ObjIsLut(p->pGia, iObj) );
        // collect leaves of this gate  
        Vec_IntClear( p->vLeaves );
        Gia_LutForEachFanin( p->pGia, iObj, iNode, s )
            Vec_IntPush( p->vLeaves, iNode );
        assert( Vec_IntSize(p->vLeaves) <= 6 );
        FanK = Vec_IntEntry(vPairs, i+1);
        FanJ = Vec_IntEntry(vPairs, i+2);
        k = Vec_IntFind( p->vLeaves, FanK );
        j = Vec_IntFind( p->vLeaves, FanJ );
        assert( FanK < FanJ );
        iUsed = Vec_IntEntry( p->vId2Used, iObj );
        uTruth = Vec_WrdEntry( p->vTruths, iUsed );
        Res = Abc_TtCheckDsdAnd( uTruth, k, j, NULL );
        if ( Res == -1 )
            continue;
        // derive literals
        FanK = Abc_Var2Lit( FanK, ((Res >> 0) & 1) );
        FanJ = Abc_Var2Lit( FanJ, ((Res >> 1) & 1) );
        if ( Res == 4 )
            ABC_SWAP( int, FanK, FanJ );
        Num = Hash_Int2ManInsert( p->pHash, FanK, FanJ, 0 );
        nRefs = Hash_Int2ObjInc(p->pHash, Num);
        if ( nRefs == 0 )
        {
            assert( Num == Hash_IntManEntryNum(p->pHash) );
            assert( Num == Vec_IntSize(vNum2Obj) );
            Vec_IntPush( vNum2Obj, iObj );
            continue;
        }
        if ( nRefs == 1 )
        {
            assert( Num < Vec_IntSize(vNum2Obj) );
            Vec_IntPush( vPairs2, Vec_IntEntry(vNum2Obj, Num) );
            Vec_IntPush( vPairs2, FanK );
            Vec_IntPush( vPairs2, FanJ );
            nNonUnique++;
        }
        Vec_IntPush( vPairs2, iObj );
        Vec_IntPush( vPairs2, FanK );
        Vec_IntPush( vPairs2, FanJ );
    }
    Vec_IntFree( vNum2Obj );
    if ( fVerbose )
        Unm_ManPrintPairStats( p->pHash, Vec_IntSize(vPairs)/3, Hash_IntManEntryNum(p->pHash), Vec_IntSize(vPairs2)/3, 1 );
//    Hash_IntManStop( pHash );
    return vPairs2;    
}

/**Function*************************************************************

  Synopsis    [Compute truth tables for the selected nodes.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Unm_ManWork( Unm_Man_t * p )
{
    Vec_Int_t * vPairs, * vPairs2;
    // find the duplicated pairs
    vPairs = Unm_ManComputePairs( p, 1 );
    // find the used nodes
    p->vUsed = Unm_ManFindUsedNodes( vPairs, Gia_ManObjNum(p->pGia) );
    p->vId2Used = Vec_IntInvert( p->vUsed, -1 );
    Vec_IntFillExtra( p->vId2Used, Gia_ManObjNum(p->pGia), -1 );
    // compute truth tables for used nodes
    p->vTruths = Unm_ManComputeTruths( p );
    // derive new pairs
    vPairs2 = Unm_ManCollectDecomp( p, vPairs, 1 );
    Vec_IntFreeP( &vPairs );
    Vec_IntFreeP( &vPairs2 );
}


/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Unm_ManTest( Gia_Man_t * pGia )
{
    Unm_Man_t * p;
    p = Unm_ManAlloc( pGia );
    Unm_ManWork( p );

    Abc_PrintTime( 1, "Time", Abc_Clock() - p->clkStart );
    return Unm_ManFree( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

