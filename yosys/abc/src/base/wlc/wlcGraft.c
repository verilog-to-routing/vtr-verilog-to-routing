/**CFile****************************************************************

  FileName    [wlcGraft.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Verilog parser.]

  Synopsis    [Parses several flavors of word-level Verilog.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 22, 2014.]

  Revision    [$Id: wlcGraft.c,v 1.00 2014/09/12 00:00:00 alanmi Exp $]

***********************************************************************/

#include "wlc.h"
#include "sat/bsat/satStore.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Internal simulation APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word * Wlc_ObjSim( Gia_Man_t * p, int iObj )
{
    return Vec_WrdEntryP( p->vSims, p->nSimWords * iObj );
}
static inline void Wlc_ObjSimPi( Gia_Man_t * p, int iObj )
{
    int w;
    word * pSim = Wlc_ObjSim( p, iObj );
    for ( w = 0; w < p->nSimWords; w++ )
        pSim[w] = Gia_ManRandomW( 0 );
    pSim[0] <<= 1;
}
static inline void Wlc_ObjSimRo( Gia_Man_t * p, int iObj )
{
    int w;
    word * pSimRo = Wlc_ObjSim( p, iObj );
    word * pSimRi = Wlc_ObjSim( p, Gia_ObjRoToRiId(p, iObj) );
    for ( w = 0; w < p->nSimWords; w++ )
        pSimRo[w] = pSimRi[w];
}
static inline void Wlc_ObjSimCo( Gia_Man_t * p, int iObj )
{
    int w;
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    word * pSimCo  = Wlc_ObjSim( p, iObj );
    word * pSimDri = Wlc_ObjSim( p, Gia_ObjFaninId0(pObj, iObj) );
    if ( Gia_ObjFaninC0(pObj) )
        for ( w = 0; w < p->nSimWords; w++ )
            pSimCo[w] = ~pSimDri[w];
    else
        for ( w = 0; w < p->nSimWords; w++ )
            pSimCo[w] =  pSimDri[w];
}
static inline void Wlc_ObjSimAnd( Gia_Man_t * p, int iObj )
{
    int w;
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    word * pSim  = Wlc_ObjSim( p, iObj );
    word * pSim0 = Wlc_ObjSim( p, Gia_ObjFaninId0(pObj, iObj) );
    word * pSim1 = Wlc_ObjSim( p, Gia_ObjFaninId1(pObj, iObj) );
    if ( Gia_ObjFaninC0(pObj) && Gia_ObjFaninC1(pObj) )
        for ( w = 0; w < p->nSimWords; w++ )
            pSim[w] = ~pSim0[w] & ~pSim1[w];
    else if ( Gia_ObjFaninC0(pObj) && !Gia_ObjFaninC1(pObj) )
        for ( w = 0; w < p->nSimWords; w++ )
            pSim[w] = ~pSim0[w] & pSim1[w];
    else if ( !Gia_ObjFaninC0(pObj) && Gia_ObjFaninC1(pObj) )
        for ( w = 0; w < p->nSimWords; w++ )
            pSim[w] = pSim0[w] & ~pSim1[w];
    else
        for ( w = 0; w < p->nSimWords; w++ )
            pSim[w] = pSim0[w] & pSim1[w];
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Wlc_NtkCollectObjs_rec( Wlc_Ntk_t * p, Wlc_Obj_t * pObj, Vec_Int_t * vObjs )
{
    int i, iFanin, Count = 0;
    if ( Wlc_ObjIsCi(pObj) )
        return 0;
    if ( pObj->Mark )
        return 0;
    pObj->Mark = 1;
    Wlc_ObjForEachFanin( pObj, iFanin, i )
        Count += Wlc_NtkCollectObjs_rec( p, Wlc_NtkObj(p, iFanin), vObjs );
    Vec_IntPush( vObjs, Wlc_ObjId(p, pObj) );
    return Count + (int)(pObj->Type == WLC_OBJ_ARI_MULTI);
}
Vec_Int_t * Wlc_NtkCollectObjs( Wlc_Ntk_t * p, int fEven, int * pCount )
{
    Vec_Int_t * vObjs = Vec_IntAlloc( 100 );
    Wlc_Obj_t * pObj; 
    int i, Count = 0;
    Wlc_NtkCleanMarks( p );
    Wlc_NtkForEachCo( p, pObj, i )
        if ( (i & 1) == fEven )
            Count += Wlc_NtkCollectObjs_rec( p, pObj, vObjs );
    Wlc_NtkCleanMarks( p );
    if ( pCount )
        *pCount = Count;
    return vObjs;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wlc_NtkSaveOneNode( Wlc_Ntk_t * p, Wlc_Obj_t * pWlcObj, Gia_Man_t * pGia, Vec_Mem_t * vTtMem )
{
    int k, Entry;
    int nWords = Vec_MemEntrySize(vTtMem);
    int nBits = Wlc_ObjRange(pWlcObj);
    int iFirst = Vec_IntEntry( &p->vCopies, Wlc_ObjId(p, pWlcObj) );
    for ( k = 0; k < nBits; k++ )
    {
        int iLit = Vec_IntEntry( &p->vBits, iFirst + k );
        word * pInfoObj = Wlc_ObjSim( pGia, Abc_Lit2Var(iLit) );
        int fCompl = pInfoObj[0] & 1;
        if ( fCompl ) Abc_TtNot( pInfoObj, nWords );
        Entry = Vec_MemHashInsert( vTtMem, pInfoObj );
        if ( fCompl ) Abc_TtNot( pInfoObj, nWords );
        printf( "%2d(%d) ", Entry, fCompl ^ Abc_LitIsCompl(iLit) );
        Extra_PrintHex( stdout, (unsigned*)pInfoObj, 8 );
        printf( "\n" );
    }
    printf( "\n" );
}
void Wlc_NtkFindOneNode( Wlc_Ntk_t * p, Wlc_Obj_t * pWlcObj, Gia_Man_t * pGia, Vec_Mem_t * vTtMem )
{
    int k, Entry;
    int nWords = Vec_MemEntrySize(vTtMem);
    int nBits = Wlc_ObjRange(pWlcObj);
    int iFirst = Vec_IntEntry( &p->vCopies, Wlc_ObjId(p, pWlcObj) );
    for ( k = 0; k < nBits; k++ )
    {
        int iLit = Vec_IntEntry( &p->vBits, iFirst + k );
        word * pInfoObj = Wlc_ObjSim( pGia, Abc_Lit2Var(iLit) );
        int fCompl = pInfoObj[0] & 1;
        if ( fCompl ) Abc_TtNot( pInfoObj, nWords );
        Entry = *Vec_MemHashLookup( vTtMem, pInfoObj );
        if ( Entry > 0 )
            printf( "Obj %4d.  Range = %2d.  Bit %2d.  Entry %d(%d).  %s\n", Wlc_ObjId(p, pWlcObj), Wlc_ObjRange(pWlcObj), k, Entry, fCompl ^ Abc_LitIsCompl(iLit), Wlc_ObjName(p, Wlc_ObjId(p, pWlcObj)) );
        if ( fCompl ) Abc_TtNot( pInfoObj, nWords );
        //printf( "%2d ", Entry );
        //Extra_PrintHex( stdout, (unsigned*)pInfoObj, 8 );
        //printf( "\n" );
    }
    //printf( "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Wlc_Ntk_t * Wlc_NtkGraftMulti( Wlc_Ntk_t * p, int fVerbose )
{
    int nWords = 4;
    int i, nMultiLHS, nMultiRHS;
    word * pInfoObj;
    Wlc_Ntk_t * pNew = NULL;
    Wlc_Obj_t * pWlcObj;
    Gia_Obj_t * pObj;
    Vec_Int_t * vObjsLHS = Wlc_NtkCollectObjs( p, 0, &nMultiLHS );
    Vec_Int_t * vObjsRHS = Wlc_NtkCollectObjs( p, 1, &nMultiRHS );
    Gia_Man_t * pGia = Wlc_NtkBitBlast( p, NULL ); //, -1, 0, 0, 0, 0, 1, 0, 0 ); // <= no cleanup
    Vec_Mem_t * vTtMem = Vec_MemAlloc( nWords, 10 );
    Vec_MemHashAlloc( vTtMem, 10000 );

    // check if there are multipliers
    if ( nMultiLHS == 0 && nMultiRHS == 0 )
    {
        printf( "No multipliers are present.\n" );
        return NULL;
    }
    // compare multipliers
    if ( nMultiLHS > 0 && nMultiRHS > 0 )
    {
        printf( "Multipliers are present in both sides of the miter.\n" );
        return NULL;
    }
    // swap if wrong side
    if ( nMultiRHS > 0 )
    {
        ABC_SWAP( Vec_Int_t *, vObjsLHS, vObjsRHS );
        ABC_SWAP( int, nMultiLHS, nMultiRHS );
    }
    assert( nMultiLHS > 0 );
    assert( nMultiRHS == 0 );

    // allocate simulation info for one timeframe
    Vec_WrdFreeP( &pGia->vSims );
    pGia->vSims = Vec_WrdStart( Gia_ManObjNum(pGia) * nWords );
    pGia->nSimWords = nWords;
    // perform simulation
    Gia_ManRandomW( 1 );
    Gia_ManForEachObj1( pGia, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) )
            Wlc_ObjSimAnd( pGia, i );
        else if ( Gia_ObjIsCo(pObj) )
            Wlc_ObjSimCo( pGia, i );
        else if ( Gia_ObjIsCi(pObj) )
            Wlc_ObjSimPi( pGia, i );
        else assert( 0 );
    }

    // hash constant 0
    pInfoObj = Wlc_ObjSim( pGia, 0 );
    Vec_MemHashInsert( vTtMem, pInfoObj );

    // hash sim info on the multiplier boundary
    Wlc_NtkForEachObjVec( vObjsLHS, p, pWlcObj, i )
        if ( Wlc_ObjType(pWlcObj) == WLC_OBJ_ARI_MULTI )
        {
            Wlc_NtkSaveOneNode( p, Wlc_ObjFanin0(p, pWlcObj), pGia, vTtMem );
            Wlc_NtkSaveOneNode( p, Wlc_ObjFanin1(p, pWlcObj), pGia, vTtMem );
            Wlc_NtkSaveOneNode( p, pWlcObj, pGia, vTtMem );
        }

    // check if there are similar signals in LHS
    Wlc_NtkForEachObjVec( vObjsRHS, p, pWlcObj, i )
        Wlc_NtkFindOneNode( p, pWlcObj, pGia, vTtMem );

    // perform grafting


    Vec_MemHashFree( vTtMem );
    Vec_MemFreeP( &vTtMem );

    // cleanup
    Vec_WrdFreeP( &pGia->vSims );
    pGia->nSimWords = 0;

    Vec_IntFree( vObjsLHS );
    Vec_IntFree( vObjsRHS );
    Gia_ManStop( pGia );
    return pNew;
}





/**Function*************************************************************

  Synopsis    [Generate simulation vectors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sbc_Mult( word a, word b, word r[2] )
{
    word Msk = 0xFFFFFFFF;
    word pL  = (a & Msk) * (b & Msk);
    word pM1 = (a & Msk) * (b >> 32);
    word pM2 = (a >> 32) * (b & Msk);
    word pH  = (a >> 32) * (b >> 32);
    word Car = (pM1 & Msk) + (pM2 & Msk) + (pL >> 32);
    r[0] = a * b;
    r[1] = pH + (pM1 >> 32) + (pM2 >> 32) + (Car >> 32);
}
void Sbc_SimMult( word A[64], word B[64], word R[128], int nIns )
{
    word a, b, r[2], Mask = Abc_Tt6Mask(nIns); int i, k;
    for ( i = 0; i < 64; i++ )
        A[i] = B[i] = R[i] = R[i+64] = 0;
    Gia_ManRandom(1);
    for ( i = 0; i < 64; i++ )
    {
        a = i ? (Mask & Gia_ManRandomW(0)) : 0;
        b = i ? (Mask & Gia_ManRandomW(0)) : 0;        
        Sbc_Mult( a, b, r );
        for ( k = 0; k < 64; k++ )
        {
            if ( (a    >> k) & 1 )  A[k]    |= ((word)1 << i);
            if ( (b    >> k) & 1 )  B[k]    |= ((word)1 << i);
            if ( (r[0] >> k) & 1 )  R[k]    |= ((word)1 << i);
            if ( (r[1] >> k) & 1 )  R[k+64] |= ((word)1 << i);
        }
    }
//    for ( i = 0; i < 128; i++ )
//        for ( k = 0; k < 64; k++, printf( "\n" ) )
//            printf( "%d", (R[i] >> k) & 1 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Sbc_ManDetectMult( Gia_Man_t * p, Vec_Int_t * vIns )
{
    int nWords = 1;
    Vec_Int_t * vGia2Out = Vec_IntStartFull( Gia_ManObjNum(p) );
    Gia_Obj_t * pObj; int i, Entry, nIns = Vec_IntSize(vIns)/2;
    word A[64], B[64], R[128], * pInfoObj; word Temp;

    // create hash table
    Vec_Mem_t * vTtMem = Vec_MemAlloc( nWords, 10 );
    Vec_MemHashAlloc( vTtMem, 1000 );
    Sbc_SimMult( A, B, R, nIns );
    for ( i = 0; i < 2*nIns; i++ )
    {
        Vec_MemHashInsert( vTtMem, R+i );
        //printf( "Out %2d : ", i );
        //Extra_PrintHex( stdout, (unsigned *)(R+i), 6 ); printf( "\n" );
    }
    assert( Vec_MemEntryNum(vTtMem) == 2*nIns );

    // alloc simulation info
    Vec_WrdFreeP( &p->vSims );
    p->vSims = Vec_WrdStart( Gia_ManObjNum(p) * nWords );
    p->nSimWords = nWords;

    // mark inputs
    Gia_ManIncrementTravId( p );
    Gia_ObjSetTravIdCurrentId( p, 0 );
    Gia_ManForEachCi( p, pObj, i )
    {
        Gia_ObjSetTravIdCurrent( p, pObj );
        //Wlc_ObjSimPi( p, Gia_ObjId(p, pObj) );
    }

    // assign inputs
    assert( Vec_IntSize(vIns) % 2 == 0 );
    Gia_ManForEachObjVec( vIns, p, pObj, i )
    {
        Gia_ObjSetTravIdCurrent( p, pObj );
        pInfoObj = Wlc_ObjSim( p, Gia_ObjId(p, pObj) );
        *pInfoObj = i < nIns ? A[i] : B[i - nIns];
    }

    // perform simulation
    Gia_ManForEachObj1( p, pObj, i )
    {
        if ( Gia_ObjIsTravIdCurrent(p, pObj) )
            continue;

        if ( Gia_ObjIsAnd(pObj) )
            Wlc_ObjSimAnd( p, i );
        else if ( Gia_ObjIsCo(pObj) )
            Wlc_ObjSimCo( p, i );
        else assert( 0 );

        // mark direct polarity
        pInfoObj = Wlc_ObjSim( p, i );
        Entry = *Vec_MemHashLookup( vTtMem, pInfoObj );
        if ( Entry >= 0 )
        {
            Vec_IntWriteEntry( vGia2Out, i, Abc_Var2Lit(Entry, 0) );
            continue;
        }

        // mark negated polarity
        Temp = *pInfoObj;
        Abc_TtNot( pInfoObj, nWords );
        Entry = *Vec_MemHashLookup( vTtMem, pInfoObj );
        Abc_TtNot( pInfoObj, nWords );
        assert( Temp == *pInfoObj );
        if ( Entry >= 0 )
        {
            Vec_IntWriteEntry( vGia2Out, i, Abc_Var2Lit(Entry, 1) );
            continue;
        }
    }

    Gia_ManForEachCo( p, pObj, i )
    {
        pInfoObj = Wlc_ObjSim( p, Gia_ObjId(p, pObj) );
        //printf( "Out %2d : Driver = %5d(%d)", i, Gia_ObjFaninId0p(p, pObj), Gia_ObjFaninC0(pObj) );
        //Extra_PrintHex( stdout, (unsigned *)pInfoObj, 6 ); printf( "\n" );
    }

    // cleanup
    Vec_MemHashFree( vTtMem );
    Vec_MemFreeP( &vTtMem );
    //Vec_WrdFreeP( &p->vSims );
    //p->nSimWords = 0;
    return vGia2Out;
}
Vec_Int_t * Sbc_ManWlcNodes2( Wlc_Ntk_t * pNtk, Gia_Man_t * p, Vec_Int_t * vGiaLits )
{
    Wlc_Obj_t * pObj;  int i, k, iGiaLit, iFirst, nBits;
    Vec_Int_t * vRes = Vec_IntAlloc( 100 );
    Vec_Int_t * vMap = Vec_IntStartFull( Gia_ManObjNum(p) );
    Vec_IntForEachEntry( vGiaLits, iGiaLit, i )
        if ( iGiaLit != -1 )
            Vec_IntWriteEntry( vMap, Abc_Lit2Var(iGiaLit), Abc_Var2Lit(i, Abc_LitIsCompl(iGiaLit)) );
    Wlc_NtkForEachObj( pNtk, pObj, i )
    {
        iFirst = Vec_IntEntry( &pNtk->vCopies, i );
        nBits  = Wlc_ObjRange(pObj);
        for ( k = 0; k < nBits; k++ )
        {
            int iLitGia = Vec_IntEntry( &pNtk->vBits, iFirst + k );
            int iLitOut = Vec_IntEntry( vMap, Abc_Lit2Var(iLitGia) );
            if ( iLitOut == -1 )
                continue;
            Vec_IntWriteEntry( vMap, Abc_Lit2Var(iLitGia), -1 );
            iLitOut = Abc_LitNotCond( iLitOut, Abc_LitIsCompl(iLitGia) );
            printf( "Matched out %d in phase %d with object %d (%s) bit %d (out of %d).\n", Abc_Lit2Var(iLitOut), Abc_LitIsCompl(iLitOut), i, Wlc_ObjName(pNtk, Wlc_ObjId(pNtk, pObj)), k, nBits );
            Vec_IntPushUnique( vRes, i );
        }
    }
    Vec_IntFree( vMap );
    Vec_IntSort( vRes, 0 );
    // consider the last one
    pObj   = Wlc_NtkObj( pNtk, Vec_IntEntryLast(vRes) );
    iFirst = Vec_IntEntry( &pNtk->vCopies, Wlc_ObjId(pNtk, pObj) );
    nBits  = Wlc_ObjRange(pObj);
    printf( "Considering object %d (%s):\n", Wlc_ObjId(pNtk, pObj), Wlc_ObjName(pNtk, Wlc_ObjId(pNtk, pObj)) );
    for ( k = 0; k < nBits; k++ )
    {
        int iLitGia  = Vec_IntEntry( &pNtk->vBits, iFirst + k );
        int iLitOutP = Vec_IntFind( vGiaLits, iLitGia );
        int iLitOutN = Vec_IntFind( vGiaLits, Abc_LitNot(iLitGia) );
        printf( "Matching bit %d with output %d / %d.\n", k, iLitOutP, iLitOutN );
        // print simulation signature
        {
            word * pInfoObj = Wlc_ObjSim( p, Abc_Lit2Var(iLitGia) );
            Extra_PrintHex( stdout, (unsigned *)pInfoObj, 6 ); printf( "\n" );
        }
    }
    return vRes;
}
int Sbc_ManWlcNodes( Wlc_Ntk_t * pNtk, Gia_Man_t * p, Vec_Int_t * vGia2Out, int nOuts )
{
    Wlc_Obj_t * pObj;  
    int i, k, iLitGia, iLitOut, iFirst, nBits, iObjFound = -1;
    Vec_Int_t * vMatched = Vec_IntAlloc( 100 );
    Wlc_NtkForEachObj( pNtk, pObj, i )
    {
        iFirst = Vec_IntEntry( &pNtk->vCopies, i );
        nBits  = Wlc_ObjRange(pObj);
        Vec_IntClear( vMatched );
        for ( k = 0; k < nBits; k++ )
        {
            iLitGia = Vec_IntEntry( &pNtk->vBits, iFirst + k );
            iLitOut = Vec_IntEntry( vGia2Out, Abc_Lit2Var(iLitGia) );
            if ( iLitOut == -1 )
                continue;
            iLitOut = Abc_LitNotCond( iLitOut, Abc_LitIsCompl(iLitGia) );
            printf( "Matched node %5d (%10s) bit %3d (out of %3d) with output %3d(%d).\n", 
                i, Wlc_ObjName(pNtk, Wlc_ObjId(pNtk, pObj)), k, nBits, Abc_Lit2Var(iLitOut), Abc_LitIsCompl(iLitOut) );
            Vec_IntPushOrder( vMatched, Abc_Lit2Var(iLitOut) );
        }
        if ( Vec_IntSize(vMatched) > 0 )
            printf( "\n" );
        if ( Vec_IntSize(vMatched) == nOuts )
        {
            if ( iObjFound == -1 )
                iObjFound = i;
            printf( "Found object %d with all bits matched.\n", i );
            /*
            for ( k = nBits-2; k < nBits; k++ )
            {
                iLitGia = Vec_IntEntry( &pNtk->vBits, iFirst + k );
                {
                    word * pInfoObj = Wlc_ObjSim( p, Abc_Lit2Var(iLitGia) );
                    Extra_PrintHex( stdout, (unsigned *)pInfoObj, 6 ); printf( "\n" );
                }
            }
            */
            break;
        }
    }
    Vec_IntFree( vMatched );
    return iObjFound;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sbc_ManDetectMultTest( Wlc_Ntk_t * pNtk, int fVerbose )
{
    extern Vec_Int_t * Sdb_StoComputeCutsDetect( Gia_Man_t * pGia );
    Gia_Man_t * p = Wlc_NtkBitBlast( pNtk, NULL );//, -1, 0, 0, 0, 0, 1, 0, 0 ); // <= no cleanup
    Vec_Int_t * vIns, * vGia2Out;
    int iObjFound = -1;
//    Gia_Obj_t * pObj; int i;
//    Gia_ManForEachCo( p, pObj, i )
//        printf( "Output %2d - driver %5d (%d)\n", i, Gia_ObjFaninId0p(p, pObj), Gia_ObjFaninC0(pObj) );

    vIns = Sdb_StoComputeCutsDetect( p );
    if ( vIns == NULL || Vec_IntSize(vIns) == 0 || (Vec_IntSize(vIns) % 2) != 0 )
    {
        printf( "Input identification did not work out.\n" );
        return;
    }

    vGia2Out = Sbc_ManDetectMult( p, vIns );

    iObjFound = Sbc_ManWlcNodes( pNtk, p, vGia2Out, Vec_IntSize(vIns) );

    Vec_IntFree( vGia2Out );
    Vec_IntFree( vIns );

    Gia_ManStop( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

