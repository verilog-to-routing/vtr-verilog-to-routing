/**CFile****************************************************************

  FileName    [acecRe.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [CEC for arithmetic circuits.]

  Synopsis    [Core procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: acecRe.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "acecInt.h"
#include "misc/vec/vecHash.h"
#include "misc/util/utilTruth.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define Ree_ForEachCut( pList, pCut, i ) for ( i = 0, pCut = pList + 1; i < pList[0]; i++, pCut += pCut[0] + 2 )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Detecting FADDs in the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ree_TruthPrecompute()
{
    word Truths[8] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77 };
    word Truth;
    int i;
    for ( i = 0; i < 8; i++ )
    {
        Truth = Truths[i];
        Truth = Abc_Tt6SwapAdjacent( Truth, 1 );
        Abc_TtPrintHexRev( stdout, &Truth, 3 );
        printf( "\n" );
    }
    printf( "\n" );
    for ( i = 0; i < 8; i++ )
    {
        Truth = Truths[i];
        Truth = Abc_Tt6SwapAdjacent( Truth, 1 );
        Truth = Abc_Tt6SwapAdjacent( Truth, 0 );
        Abc_TtPrintHexRev( stdout, &Truth, 3 );
        printf( "\n" );
    }
    printf( "\n" );
}
void Ree_TruthPrecompute2()
{
    int i, b;
    for ( i = 0; i < 8; i++ )
    {
        word Truth = 0xE8;
        for ( b = 0; b < 3; b++ )
            if ( (i >> b) & 1 )
                Truth = Abc_Tt6Flip( Truth, b );
        printf( "%d = %X\n", i, 0xFF & (int)Truth );
    }
}

/**Function*************************************************************

  Synopsis    [Detecting FADDs in the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Ree_ManCutMergeOne( int * pCut0, int * pCut1, int * pCut )
{
    int i, k;
    for ( k = 0; k <= pCut1[0]; k++ )
        pCut[k] = pCut1[k];
    for ( i = 1; i <= pCut0[0]; i++ )
    {
        for ( k = 1; k <= pCut1[0]; k++ )
            if ( pCut0[i] == pCut1[k] )
                break;
        if ( k <= pCut1[0] )
            continue;
        if ( pCut[0] == 3 )
            return 0;
        pCut[1+pCut[0]++] = pCut0[i];
    }
    assert( pCut[0] == 2 || pCut[0] == 3 );
    if ( pCut[1] > pCut[2] )
        ABC_SWAP( int, pCut[1], pCut[2] );
    assert( pCut[1] < pCut[2] );
    if ( pCut[0] == 2 )
        return 1;
    if ( pCut[2] > pCut[3] )
        ABC_SWAP( int, pCut[2], pCut[3] );
    if ( pCut[1] > pCut[2] )
        ABC_SWAP( int, pCut[1], pCut[2] );
    assert( pCut[1] < pCut[2] );
    assert( pCut[2] < pCut[3] );
    return 1;
}
static inline int Ree_ManCutCheckEqual( Vec_Int_t * vCuts, int * pCutNew )
{
    int * pList = Vec_IntArray( vCuts );
    int i, k, * pCut;
    Ree_ForEachCut( pList, pCut, i )
    {
        for ( k = 0; k <= pCut[0]; k++ )
            if ( pCut[k] != pCutNew[k] )
                break;
        if ( k > pCut[0] )
            return 1;
    }
    return 0;
}
static inline int Ree_ManCutFind( int iObj, int * pCut )
{
    if ( pCut[1] == iObj ) return 0;
    if ( pCut[2] == iObj ) return 1;
    if ( pCut[3] == iObj ) return 2;
    assert( 0 );
    return -1;
}
static inline int Ree_ManCutNotFind( int iObj1, int iObj2, int * pCut )
{
    assert( pCut[0] == 3 );
    if ( pCut[3] != iObj1 && pCut[3] != iObj2 ) return 0;
    if ( pCut[2] != iObj1 && pCut[2] != iObj2 ) return 1;
    if ( pCut[1] != iObj1 && pCut[1] != iObj2 ) return 2;
    assert( 0 );
    return -1;
}
static inline int Ree_ManCutTruthOne( int * pCut0, int * pCut )
{
    int Truth0 = pCut0[pCut0[0]+1];
    int fComp0 = (Truth0 >> 7) & 1;
    if ( pCut0[0] == 3 )
        return Truth0;
    Truth0 = fComp0 ? ~Truth0 : Truth0;
    if ( pCut0[0] == 2 )
    {
        if ( pCut[0] == 3 )
        {
            int Truths[3][8] = {
                { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77 }, // {0,1,-}
                { 0x00, 0x05, 0x0A, 0x0F, 0x50, 0x55, 0x5A, 0x5F }, // {0,-,1}
                { 0x00, 0x03, 0x0C, 0x0F, 0x30, 0x33, 0x3C, 0x3F }  // {-,0,1}
            };
            int Truth = Truths[Ree_ManCutNotFind(pCut0[1], pCut0[2], pCut)][Truth0 & 0x7];
            return 0xFF & (fComp0 ? ~Truth : Truth);
        }
        assert( pCut[0] == 2 );
        assert( pCut[1] == pCut0[1] && pCut[2] == pCut0[2] );
        return pCut0[pCut0[0]+1];
    }
    if ( pCut0[0] == 1 )
    {
        int Truths[3] = { 0x55, 0x33, 0x0F };
        int Truth = Truths[Ree_ManCutFind(pCut0[1], pCut)];
        return 0xFF & (fComp0 ? ~Truth : Truth);
    }
    assert( 0 );
    return -1;
}
static inline int Ree_ManCutTruth( Gia_Obj_t * pObj, int * pCut0, int * pCut1, int * pCut )
{
    int Truth0 = Ree_ManCutTruthOne( pCut0, pCut );
    int Truth1 = Ree_ManCutTruthOne( pCut1, pCut );
    Truth0 = Gia_ObjFaninC0(pObj) ? ~Truth0 : Truth0;
    Truth1 = Gia_ObjFaninC1(pObj) ? ~Truth1 : Truth1;
    return 0xFF & (Gia_ObjIsXor(pObj) ? Truth0 ^ Truth1 : Truth0 & Truth1); 
}

#if 0

int Ree_ObjComputeTruth_rec( Gia_Obj_t * pObj )
{
    int Truth0, Truth1;
    if ( pObj->Value )
        return pObj->Value;
    assert( Gia_ObjIsAnd(pObj) );
    Truth0 = Ree_ObjComputeTruth_rec( Gia_ObjFanin0(pObj) );
    Truth1 = Ree_ObjComputeTruth_rec( Gia_ObjFanin1(pObj) );
    if ( Gia_ObjIsXor(pObj) )
        return (pObj->Value = (Gia_ObjFaninC0(pObj) ? ~Truth0 : Truth0) ^ (Gia_ObjFaninC1(pObj) ? ~Truth1 : Truth1));
    else
        return (pObj->Value = (Gia_ObjFaninC0(pObj) ? ~Truth0 : Truth0) & (Gia_ObjFaninC1(pObj) ? ~Truth1 : Truth1));
}
void Ree_ObjCleanTruth_rec( Gia_Obj_t * pObj )
{
    if ( !pObj->Value )
        return;
    pObj->Value = 0;
    if ( !Gia_ObjIsAnd(pObj) )
        return;
    Ree_ObjCleanTruth_rec( Gia_ObjFanin0(pObj) );
    Ree_ObjCleanTruth_rec( Gia_ObjFanin1(pObj) );
}
int Ree_ObjComputeTruth( Gia_Man_t * p, int iObj, int * pCut )
{
    unsigned Truth, Truths[3] = { 0xAA, 0xCC, 0xF0 }; int i;
    for ( i = 1; i <= pCut[0]; i++ )
        Gia_ManObj(p, pCut[i])->Value = Truths[i-1];
    Truth = 0xFF & Ree_ObjComputeTruth_rec( Gia_ManObj(p, iObj) );
    Ree_ObjCleanTruth_rec( Gia_ManObj(p, iObj) );
    return Truth;
}

#endif

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ree_ManCutPrint( int * pCut, int Count, word Truth, int iObj )
{
    int c;
    printf( "%d : %d : ", Count, iObj );
    for ( c = 1; c <= pCut[0]; c++ )
        printf( "%3d ", pCut[c] );
    for (      ; c <= 4; c++ )
        printf( "    " );
    printf( "0x" );
    Abc_TtPrintHexRev( stdout, &Truth, 3 );
    printf( "\n" );
}
void Ree_ManCutMerge( Gia_Man_t * p, int iObj, int * pList0, int * pList1, Vec_Int_t * vCuts, Hash_IntMan_t * pHash, Vec_Int_t * vData, Vec_Int_t * vXors )
{
    int fVerbose = 0;
    int i, k, c, Value, Truth, TruthC, * pCut0, * pCut1, pCut[6], Count = 0;
    int iXor2 = -1, iXor3 = -1;
    if ( fVerbose )
        printf( "Object %d\n", iObj );
    Vec_IntFill( vCuts, 2, 1 );
    Vec_IntPush( vCuts, iObj );
    Vec_IntPush( vCuts, 0xAA );
    Ree_ForEachCut( pList0, pCut0, i )
    Ree_ForEachCut( pList1, pCut1, k )
    {
        if ( !Ree_ManCutMergeOne(pCut0, pCut1, pCut) )
            continue;
        if ( Ree_ManCutCheckEqual(vCuts, pCut) )
            continue;
        Truth = TruthC = Ree_ManCutTruth(Gia_ManObj(p, iObj), pCut0, pCut1, pCut);
        //assert( Truth == Ree_ObjComputeTruth(p, iObj, pCut) );
        if ( Truth & 0x80 )
            Truth = 0xFF & ~Truth;
        if ( Truth == 0x66 && iXor2 == -1 )
            iXor2 = Vec_IntSize(vCuts);
        else if ( Truth == 0x69 && iXor3 == -1 )
            iXor3 = Vec_IntSize(vCuts);
        Vec_IntAddToEntry( vCuts, 0, 1 );  
        for ( c = 0; c <= pCut[0]; c++ )
            Vec_IntPush( vCuts, pCut[c] );
        Vec_IntPush( vCuts, TruthC );
        if ( (Truth == 0x66 || Truth == 0x11 || Truth == 0x22 || Truth == 0x44 || Truth == 0x77) && pCut[0] == 2 )
        {
            assert( pCut[0] == 2 );
            Value = Hsh_Int3ManInsert( pHash, pCut[1], pCut[2], 0 );
            Vec_IntPushThree( vData, iObj, Value, TruthC );
        }
        else if ( Truth == 0x69 || Truth == 0x17 || Truth == 0x2B || Truth == 0x4D || Truth == 0x71 )
        {
            assert( pCut[0] == 3 );
            Value = Hsh_Int3ManInsert( pHash, pCut[1], pCut[2], pCut[3] );
            Vec_IntPushThree( vData, iObj, Value, TruthC );
        }
        if ( fVerbose )
            Ree_ManCutPrint( pCut, ++Count, TruthC, iObj );
    }
    if ( !vXors )
        return;
    if ( iXor2 > 0 )
        pCut0 = Vec_IntEntryP( vCuts, iXor2 );
    else if ( iXor3 > 0 )
        pCut0 = Vec_IntEntryP( vCuts, iXor3 );
    else
        return;
    Vec_IntPush( vXors, iObj );
    for ( c = 1; c <= pCut0[0]; c++ )
        Vec_IntPush( vXors, pCut0[c] );
    if ( pCut0[0] == 2 )
        Vec_IntPush( vXors, 0 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Ree_ManDeriveAdds( Hash_IntMan_t * p, Vec_Int_t * vData, int fVerbose )
{
    int i, j, k, iObj, iObj2, Value, Truth, Truth2, CountX, CountM, Index = 0;
    int nEntries = Hash_IntManEntryNum(p);
    Vec_Int_t * vAdds = Vec_IntAlloc( 1000 );
    Vec_Int_t * vXors = Vec_IntStart( nEntries + 1 );
    Vec_Int_t * vMajs = Vec_IntStart( nEntries + 1 );
    Vec_Int_t * vIndex = Vec_IntStartFull( nEntries + 1 );
    Vec_Int_t * vIndexRev = Vec_IntAlloc( 1000 );
    Vec_Wec_t * vXorMap, * vMajMap;
    Vec_IntForEachEntryTriple( vData, iObj, Value, Truth, i )
    {
        assert( Value <= nEntries );
        if ( Truth == 0x66 || Truth == 0x99 || Truth == 0x69 || Truth == 0x96 )
            Vec_IntAddToEntry( vXors, Value, 1 );
        else
            Vec_IntAddToEntry( vMajs, Value, 1 );
    }
    // remap these into indexes
    Vec_IntForEachEntryTwo( vXors, vMajs, CountX, CountM, i )
        if ( CountX && CountM )
        {
            Vec_IntPush( vIndexRev, i );
            Vec_IntWriteEntry( vIndex, i, Index++ );
        }
    Vec_IntFree( vXors );
    Vec_IntFree( vMajs );
    //if ( fVerbose )
    //    printf( "Detected %d shared cuts among %d hashed cuts.\n", Index, nEntries );
    // collect nodes
    vXorMap = Vec_WecStart( Index );
    vMajMap = Vec_WecStart( Index );
    Vec_IntForEachEntryTriple( vData, iObj, Value, Truth, i )
    {
        Index = Vec_IntEntry( vIndex, Value );
        if ( Index == -1 )
            continue;
        if ( Truth == 0x66 || Truth == 0x99 || Truth == 0x69 || Truth == 0x96 )
            Vec_IntPushTwo( Vec_WecEntry(vXorMap, Index), iObj, Truth );
        else
            Vec_IntPushTwo( Vec_WecEntry(vMajMap, Index), iObj, Truth );
    }
    Vec_IntFree( vIndex );
    // create pairs
    Vec_IntForEachEntry( vIndexRev, Value, i )
    {
        Vec_Int_t * vXorOne = Vec_WecEntry( vXorMap, i );
        Vec_Int_t * vMajOne = Vec_WecEntry( vMajMap, i );
        Hash_IntObj_t * pObj = Hash_IntObj( p, Value );
        Vec_IntForEachEntryDouble( vXorOne, iObj, Truth, j )
        Vec_IntForEachEntryDouble( vMajOne, iObj2, Truth2, k )
        {
            int SignAnd[8] = {0x88, 0x44, 0x22, 0x11, 0x77, 0xBB, 0xDD, 0xEE};
            int SignMaj[8] = {0xE8, 0xD4, 0xB2, 0x71, 0x8E, 0x4D, 0x2B, 0x17};
            int n, SignXor = (Truth == 0x99 || Truth == 0x69) << 3;
            for ( n = 0; n < 8; n++ )
                if ( Truth2 == SignMaj[n] )
                    break;
            if ( n == 8 ) 
                for ( n = 0; n < 8; n++ )
                    if ( Truth2 == SignAnd[n] )
                        break;
            assert( n < 8 );
            Vec_IntPushThree( vAdds, pObj->iData0, pObj->iData1, pObj->iData2 ); 
            Vec_IntPushThree( vAdds, iObj, iObj2, SignXor | n );
        }
    }
    Vec_IntFree( vIndexRev );
    Vec_WecFree( vXorMap );
    Vec_WecFree( vMajMap );
    return vAdds;
}
int Ree_ManCompare( int * pCut0, int * pCut1 )
{
    if ( pCut0[3] < pCut1[3] ) return -1;
    if ( pCut0[3] > pCut1[3] ) return  1;
    if ( pCut0[4] < pCut1[4] ) return -1;
    if ( pCut0[4] > pCut1[4] ) return  1;
    return 0;
}
Vec_Int_t * Ree_ManComputeCuts( Gia_Man_t * p, Vec_Int_t ** pvXors, int fVerbose )
{
    extern void Ree_ManRemoveTrivial( Gia_Man_t * p, Vec_Int_t * vAdds );
    extern void Ree_ManRemoveContained( Gia_Man_t * p, Vec_Int_t * vAdds );
    Gia_Obj_t * pObj; 
    int * pList0, * pList1, i, nCuts = 0;
    Hash_IntMan_t * pHash = Hash_IntManStart( 1000 );
    Vec_Int_t * vAdds;
    Vec_Int_t * vTemp = Vec_IntAlloc( 1000 );
    Vec_Int_t * vData = Vec_IntAlloc( 1000 );
    Vec_Int_t * vCuts = Vec_IntAlloc( 30 * Gia_ManAndNum(p) );
    Vec_IntFill( vCuts, Gia_ManObjNum(p), 0 );
    Gia_ManCleanValue( p );
    Gia_ManForEachCi( p, pObj, i )
    {
        Vec_IntWriteEntry( vCuts, Gia_ObjId(p, pObj), Vec_IntSize(vCuts) );
        Vec_IntPush( vCuts, 1 );
        Vec_IntPush( vCuts, 1 );
        Vec_IntPush( vCuts, Gia_ObjId(p, pObj) );
        Vec_IntPush( vCuts, 0xAA );
    }
    if ( pvXors ) *pvXors = Vec_IntAlloc( 1000 );
    Gia_ManForEachAnd( p, pObj, i )
    {
        pList0 = Vec_IntEntryP( vCuts, Vec_IntEntry(vCuts, Gia_ObjFaninId0(pObj, i)) );
        pList1 = Vec_IntEntryP( vCuts, Vec_IntEntry(vCuts, Gia_ObjFaninId1(pObj, i)) );
        Ree_ManCutMerge( p, i, pList0, pList1, vTemp, pHash, vData, pvXors ? *pvXors : NULL );
        Vec_IntWriteEntry( vCuts, i, Vec_IntSize(vCuts) );
        Vec_IntAppend( vCuts, vTemp );
        nCuts += Vec_IntEntry( vTemp, 0 );
    }
    if ( fVerbose )
        printf( "AIG nodes = %d.  Cuts = %d.  Cuts/Node = %.2f.  Ints/Node = %.2f.\n", 
            Gia_ManAndNum(p), nCuts, 1.0*nCuts/Gia_ManAndNum(p), 1.0*Vec_IntSize(vCuts)/Gia_ManAndNum(p) );
    Vec_IntFree( vTemp );
    Vec_IntFree( vCuts );
    vAdds = Ree_ManDeriveAdds( pHash, vData, fVerbose );
    qsort( Vec_IntArray(vAdds), (size_t)(Vec_IntSize(vAdds)/6), 24, (int (*)(const void *, const void *))Ree_ManCompare );
    if ( fVerbose )
        printf( "Adders = %d.  Total cuts = %d.  Hashed cuts = %d.  Hashed/Adders = %.2f.\n", 
            Vec_IntSize(vAdds)/6, Vec_IntSize(vData)/3, Hash_IntManEntryNum(pHash), 6.0*Hash_IntManEntryNum(pHash)/Vec_IntSize(vAdds) );
    Vec_IntFree( vData );
    Hash_IntManStop( pHash );
    Ree_ManRemoveTrivial( p, vAdds );
    Ree_ManRemoveContained( p, vAdds );
    //Ree_ManPrintAdders( vAdds, 1 );
    return vAdds;
}

/**Function*************************************************************

  Synopsis    [Highlight nodes inside FAs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ree_CollectInsiders_rec( Gia_Man_t * pGia, int iObj, Vec_Bit_t * vVisited, Vec_Bit_t * vInsiders )
{
    if ( Vec_BitEntry(vVisited, iObj) )
        return;
    Vec_BitSetEntry( vVisited, iObj, 1 );
    Ree_CollectInsiders_rec( pGia, Gia_ObjFaninId0p(pGia, Gia_ManObj(pGia, iObj)), vVisited, vInsiders );
    Ree_CollectInsiders_rec( pGia, Gia_ObjFaninId1p(pGia, Gia_ManObj(pGia, iObj)), vVisited, vInsiders );
    Vec_BitSetEntry( vInsiders, iObj, 1 );
}
Vec_Bit_t * Ree_CollectInsiders( Gia_Man_t * pGia, Vec_Int_t * vAdds )
{
    Vec_Bit_t * vVisited = Vec_BitStart( Gia_ManObjNum(pGia) );
    Vec_Bit_t * vInsiders = Vec_BitStart( Gia_ManObjNum(pGia) );
    int i, Entry1, Entry2, Entry3;
    for ( i = 0; 6*i < Vec_IntSize(vAdds); i++ )
    {
        if ( Vec_IntEntry(vAdds, 6*i+2) == 0 ) // HADD
            continue;
        // mark inputs
        Entry1 = Vec_IntEntry( vAdds, 6*i + 0 );
        Entry2 = Vec_IntEntry( vAdds, 6*i + 1 );
        Entry3 = Vec_IntEntry( vAdds, 6*i + 2 );
        Vec_BitWriteEntry( vVisited, Entry1, 1 );
        Vec_BitWriteEntry( vVisited, Entry2, 1 );
        Vec_BitWriteEntry( vVisited, Entry3, 1 );
        // traverse from outputs
        Entry1 = Vec_IntEntry( vAdds, 6*i + 3 );
        Entry2 = Vec_IntEntry( vAdds, 6*i + 4 );
        Ree_CollectInsiders_rec( pGia, Entry1, vVisited, vInsiders );
        Ree_CollectInsiders_rec( pGia, Entry2, vVisited, vInsiders );
    }
    Vec_BitFree( vVisited );
    return vInsiders;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
// removes HAs whose AND2 is part of XOR2 without additional fanout
void Ree_ManRemoveTrivial( Gia_Man_t * p, Vec_Int_t * vAdds )
{
    Gia_Obj_t * pObjX, * pObjM;
    int i, k = 0;
    ABC_FREE( p->pRefs );
    Gia_ManCreateRefs( p );
    for ( i = 0; 6*i < Vec_IntSize(vAdds); i++ )
    {
        if ( Vec_IntEntry(vAdds, 6*i+2) == 0 ) // HADD
        {
            pObjX = Gia_ManObj( p, Vec_IntEntry(vAdds, 6*i+3) );
            pObjM = Gia_ManObj( p, Vec_IntEntry(vAdds, 6*i+4) );
            // rule out if MAJ is a fanout of XOR
            //if ( pObjX == Gia_ObjFanin0(pObjM) || pObjX == Gia_ObjFanin1(pObjM) )
            //    continue;
            // rule out if MAJ is a fanin of XOR and has no other fanouts
            if ( (pObjM == Gia_ObjFanin0(pObjX) || pObjM == Gia_ObjFanin1(pObjX)) && Gia_ObjRefNum(p, pObjM) == 1 )
                continue;
        }
        memmove( Vec_IntArray(vAdds) + 6*k++, Vec_IntArray(vAdds) + 6*i, 6*sizeof(int) );
    }
    assert( k <= i );
    Vec_IntShrink( vAdds, 6*k );
}
// removes HAs fully contained inside FAs
void Ree_ManRemoveContained( Gia_Man_t * p, Vec_Int_t * vAdds )
{
    Vec_Bit_t * vInsiders = Ree_CollectInsiders( p, vAdds );
    int i, k = 0;
    for ( i = 0; 6*i < Vec_IntSize(vAdds); i++ )
    {
        if ( Vec_IntEntry(vAdds, 6*i+2) == 0 ) // HADD
            if ( Vec_BitEntry(vInsiders, Vec_IntEntry(vAdds, 6*i+3)) && Vec_BitEntry(vInsiders, Vec_IntEntry(vAdds, 6*i+4)) )
                continue;
        memmove( Vec_IntArray(vAdds) + 6*k++, Vec_IntArray(vAdds) + 6*i, 6*sizeof(int) );
    }
    assert( k <= i );
    Vec_IntShrink( vAdds, 6*k );
    Vec_BitFree( vInsiders );
}

int Ree_ManCountFadds( Vec_Int_t * vAdds )
{
    int i, Count = 0;
    for ( i = 0; 6*i < Vec_IntSize(vAdds); i++ )
        if ( Vec_IntEntry(vAdds, 6*i+2) != 0 )
            Count++;
    return Count;
}
void Ree_ManPrintAdders( Vec_Int_t * vAdds, int fVerbose )
{
    int i;
    for ( i = 0; 6*i < Vec_IntSize(vAdds); i++ )
    {
        //if ( Vec_IntEntry(vAdds, 6*i+2) == 0 )
        //    continue;
        if ( !fVerbose )
            continue;
        printf( "%6d : ", i );
        printf( "%6d ", Vec_IntEntry(vAdds, 6*i+0) );
        printf( "%6d ", Vec_IntEntry(vAdds, 6*i+1) );
        printf( "%6d ", Vec_IntEntry(vAdds, 6*i+2) );
        printf( "   ->  " );
        printf( "%6d ", Vec_IntEntry(vAdds, 6*i+3) );
        printf( "%6d ", Vec_IntEntry(vAdds, 6*i+4) );
        printf( "  (%d)", Vec_IntEntry(vAdds, 6*i+5) );
        printf( "\n" );
    }
}
void Ree_ManComputeCutsTest( Gia_Man_t * p )
{
    abctime clk = Abc_Clock();
    Vec_Int_t * vAdds = Ree_ManComputeCuts( p, NULL, 1 );
    int nFadds = Ree_ManCountFadds( vAdds );
    Ree_ManPrintAdders( vAdds, 1 );
    printf( "Detected %d FAs and %d HAs.  ", nFadds, Vec_IntSize(vAdds)/6-nFadds );
    Vec_IntFree( vAdds );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

