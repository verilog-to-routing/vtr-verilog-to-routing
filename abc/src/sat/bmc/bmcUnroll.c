/**CFile****************************************************************

  FileName    [bmcUnroll.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based bounded model checking.]

  Synopsis    [Unrolling manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: bmc.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bmc.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define UNR_DIFF_NULL 0x7FFF

typedef struct Unr_Obj_t_ Unr_Obj_t; // 24 bytes + (RankMax-1) * 4 bytes
struct Unr_Obj_t_
{
    unsigned              hFan0;        // address of the fanin
    unsigned              hFan1;        // address of the fanin
    unsigned              fCompl0 :  1; // complemented bit
    unsigned              fCompl1 :  1; // complemented bit
    unsigned              uRDiff0 : 15; // rank diff of the fanin
    unsigned              uRDiff1 : 15; // rank diff of the fanin
    unsigned              fItIsPi :  1; // remember attributes
    unsigned              fItIsPo :  1; // remember attributes
    unsigned              RankMax : 15; // max rank diff between node and its fanout
    unsigned              RankCur : 15; // cur rank of the node
    unsigned              OrigId;       // original object ID
    unsigned              Res[1];       // RankMax entries
};

struct Unr_Man_t_
{
    // input data
    Gia_Man_t *           pGia;           // the user's AIG manager
    Gia_Man_t *           pFrames;        // unrolled manager
    int                   nObjs;          // the number of objects
    // intermediate data
    Vec_Int_t *           vOrder;         // ordering of GIA objects
    Vec_Int_t *           vOrderLim;      // beginning of each time frame
    Vec_Int_t *           vTents;         // tents of GIA objects
    Vec_Int_t *           vRanks;         // ranks of GIA objects
    // unrolling data
    int *                 pObjs;          // storage for unroling objects
    int *                 pEnd;           // end of storage
    Vec_Int_t *           vObjLim;        // handle of the first object in each frame
    Vec_Int_t *           vCiMap;         // mapping of GIA CIs into unrolling objects
    Vec_Int_t *           vCoMap;         // mapping of GIA POs into unrolling objects
    Vec_Int_t *           vPiLits;        // storage for PI literals
};

static inline Unr_Obj_t * Unr_ManObj( Unr_Man_t * p, int h )  { assert( h >= 0 && h < p->pEnd - p->pObjs ); return (Unr_Obj_t *)(p->pObjs + h);  }
static inline int         Unr_ObjSizeInt( int Rank )          { return 0xFFFFFFFE & (sizeof(Unr_Obj_t) / sizeof(int) + Rank);                    }
static inline int         Unr_ObjSize( Unr_Obj_t * pObj )     { return Unr_ObjSizeInt(pObj->RankMax);                                            }

static inline int Unr_ManFanin0Value( Unr_Man_t * p, Unr_Obj_t * pObj )
{
    Unr_Obj_t * pFanin = Unr_ManObj( p, pObj->hFan0 );
    int Index = (pFanin->RankCur + pFanin->RankMax - pObj->uRDiff0) % pFanin->RankMax;
    assert( pFanin->RankCur < pFanin->RankMax );
    assert( pObj->uRDiff0 < pFanin->RankMax );
    return Abc_LitNotCond( pFanin->Res[Index], pObj->fCompl0 );
}
static inline int Unr_ManFanin1Value( Unr_Man_t * p, Unr_Obj_t * pObj )
{
    Unr_Obj_t * pFanin = Unr_ManObj( p, pObj->hFan1 );
    int Index = (pFanin->RankCur + pFanin->RankMax - pObj->uRDiff1) % pFanin->RankMax;
    assert( pFanin->RankCur < pFanin->RankMax );
    assert( pObj->uRDiff1 < pFanin->RankMax );
    return Abc_LitNotCond( pFanin->Res[Index], pObj->fCompl1 );
}
static inline int Unr_ManObjReadValue( Unr_Obj_t * pObj )
{
    assert( pObj->RankCur >= 0 && pObj->RankCur < pObj->RankMax );
    return pObj->Res[ pObj->RankCur ];
}
static inline void Unr_ManObjSetValue( Unr_Obj_t * pObj, int Value )
{
    assert( Value >= 0 );
    pObj->RankCur = (UNR_DIFF_NULL & (pObj->RankCur + 1)) % pObj->RankMax;
    pObj->Res[ pObj->RankCur ] = Value;
}

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntWriteMaxEntry( Vec_Int_t * p, int i, int Entry )
{
    assert( i >= 0 && i < p->nSize );
    p->pArray[i] = Abc_MaxInt( p->pArray[i], Entry );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Unr_ManProfileRanks( Vec_Int_t * vRanks )
{
    int RankMax = Vec_IntFindMax( vRanks );
    Vec_Int_t * vCounts = Vec_IntStart( RankMax+1 );
    int i, Rank, Count, nExtras = 0;
    Vec_IntForEachEntry( vRanks, Rank, i )
        Vec_IntAddToEntry( vCounts, Rank, 1 );
    Vec_IntForEachEntry( vCounts, Count, i )
    {
        if ( Count == 0 )
            continue;
        printf( "%2d : %8d  (%6.2f %%)\n", i, Count, 100.0 * Count / Vec_IntSize(vRanks) );
        nExtras += Count * i;
    }
    printf( "Extra space = %d (%6.2f %%)  ", nExtras, 100.0 * nExtras / Vec_IntSize(vRanks) );
    Vec_IntFree( vCounts );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Unr_ManSetup_rec( Unr_Man_t * p, int iObj, int iTent, Vec_Int_t * vRoots )
{
    Gia_Obj_t * pObj;
    int iFanin;
    if ( Vec_IntEntry(p->vTents, iObj) >= 0 )
        return;
    Vec_IntWriteEntry(p->vTents, iObj, iTent);
    pObj = Gia_ManObj( p->pGia, iObj );
    if ( Gia_ObjIsAnd(pObj) || Gia_ObjIsCo(pObj) )
    {
        Unr_ManSetup_rec( p, (iFanin = Gia_ObjFaninId0(pObj, iObj)), iTent, vRoots );
        Vec_IntWriteMaxEntry( p->vRanks, iFanin, Abc_MaxInt(0, iTent - Vec_IntEntry(p->vTents, iFanin) - 1) );
    }
    if ( Gia_ObjIsAnd(pObj) )
    {
        Unr_ManSetup_rec( p, (iFanin = Gia_ObjFaninId1(pObj, iObj)), iTent, vRoots );
        Vec_IntWriteMaxEntry( p->vRanks, iFanin, Abc_MaxInt(0, iTent - Vec_IntEntry(p->vTents, iFanin) - 1) );
    }
    else if ( Gia_ObjIsRo(p->pGia, pObj) )
    {
        Vec_IntPush( vRoots, (iFanin = Gia_ObjId(p->pGia, Gia_ObjRoToRi(p->pGia, pObj))) );
        Vec_IntWriteMaxEntry( p->vRanks, iFanin, 0 );
    }
    Vec_IntPush( p->vOrder, iObj );
}
void Unr_ManSetup( Unr_Man_t * p, int fVerbose )
{
    Vec_Int_t * vRoots, * vRoots2, * vMap;
    Unr_Obj_t * pUnrObj;
    Gia_Obj_t * pObj;
    int i, k, t, iObj, nInts, * pInts;
    abctime clk = Abc_Clock();
    // create zero rank
    assert( Vec_IntSize(p->vOrder) == 0 );
    Vec_IntPush( p->vOrder, 0 );
    Vec_IntPush( p->vOrderLim, Vec_IntSize(p->vOrder) );
    Vec_IntWriteEntry( p->vTents, 0, 0 );
    Vec_IntWriteEntry( p->vRanks, 0, 0 );
    // start from the POs
    vRoots  = Vec_IntAlloc( 100 );
    vRoots2 = Vec_IntAlloc( 100 );
    Gia_ManForEachPo( p->pGia, pObj, i )
        Unr_ManSetup_rec( p, Gia_ObjId(p->pGia, pObj), 0, vRoots );
    // collect tents
    while ( Vec_IntSize(vRoots) > 0 )
    {
        Vec_IntPush( p->vOrderLim, Vec_IntSize(p->vOrder) );
        Vec_IntClear( vRoots2 );
        Vec_IntForEachEntry( vRoots, iObj, i )
            Unr_ManSetup_rec( p, iObj, Vec_IntSize(p->vOrderLim)-1, vRoots2 );
        ABC_SWAP( Vec_Int_t *, vRoots, vRoots2 );
    }
    Vec_IntPush( p->vOrderLim, Vec_IntSize(p->vOrder) );
    Vec_IntFree( vRoots );
    Vec_IntFree( vRoots2 );
    // allocate memory
    nInts = 0;
    Vec_IntForEachEntry( p->vOrder, iObj, i )
        nInts += Unr_ObjSizeInt( Vec_IntEntry(p->vRanks, iObj) + 1 );
    p->pObjs = pInts = ABC_CALLOC( int, nInts );
    p->pEnd = p->pObjs + nInts;
    // create const0 node
    pUnrObj = Unr_ManObj( p, pInts - p->pObjs );
    pUnrObj->RankMax = Vec_IntEntry(p->vRanks, 0) + 1;
    pUnrObj->uRDiff0 = pUnrObj->uRDiff1 = UNR_DIFF_NULL;
    pUnrObj->Res[0]  = 0; // const0
    // map the objects
    vMap = Vec_IntStartFull( p->nObjs );
    Vec_IntWriteEntry( vMap, 0, pInts - p->pObjs );
    pInts += Unr_ObjSize(pUnrObj);
    // mark up the entries
    assert( Vec_IntSize(p->vObjLim) == 0 );
    for ( t = Vec_IntSize(p->vOrderLim) - 2; t >= 0; t-- )
    {
        int Beg = Vec_IntEntry(p->vOrderLim, t);
        int End = Vec_IntEntry(p->vOrderLim, t+1);
        Vec_IntPush( p->vObjLim, pInts - p->pObjs );
        Vec_IntForEachEntryStartStop( p->vOrder, iObj, i, Beg, End )
        {
            pObj = Gia_ManObj( p->pGia, iObj );
            pUnrObj = Unr_ManObj( p, pInts - p->pObjs );
            pUnrObj->uRDiff0 = pUnrObj->uRDiff1 = UNR_DIFF_NULL;
            if ( Gia_ObjIsAnd(pObj) || Gia_ObjIsCo(pObj) )
                pUnrObj->uRDiff0 = Abc_MaxInt(0, Vec_IntEntry(p->vTents, iObj) - Vec_IntEntry(p->vTents, Gia_ObjFaninId0(pObj, iObj)) - 1);
            if ( Gia_ObjIsAnd(pObj) )
                pUnrObj->uRDiff1 = Abc_MaxInt(0, Vec_IntEntry(p->vTents, iObj) - Vec_IntEntry(p->vTents, Gia_ObjFaninId1(pObj, iObj)) - 1);
            else if ( Gia_ObjIsRo(p->pGia, pObj) )
                pUnrObj->uRDiff0 = 0;
            pUnrObj->RankMax = Vec_IntEntry(p->vRanks, iObj) + 1;
            pUnrObj->RankCur = UNR_DIFF_NULL;
            pUnrObj->OrigId  = iObj;
            for ( k = 0; k < (int)pUnrObj->RankMax; k++ )
                pUnrObj->Res[k] = -1;
            assert( ((pInts - p->pObjs) & 1) == 0 ); // align for 64-bits
            Vec_IntWriteEntry( vMap, iObj, pInts - p->pObjs );
            pInts += Unr_ObjSize( pUnrObj );
        }
    }
    assert( pInts - p->pObjs == nInts );
    // label the objects
    Gia_ManForEachObj( p->pGia, pObj, i )
    {
        if ( Vec_IntEntry(vMap, i) == -1 )
            continue;
        pUnrObj = Unr_ManObj( p, Vec_IntEntry(vMap, i) );
        if ( Gia_ObjIsAnd(pObj) || Gia_ObjIsCo(pObj) )
        {
            pUnrObj->hFan0   = Vec_IntEntry( vMap, Gia_ObjFaninId0(pObj, i) );
            pUnrObj->fCompl0 = Gia_ObjFaninC0(pObj);
            pUnrObj->fItIsPo = Gia_ObjIsPo(p->pGia, pObj);
        }
        if ( Gia_ObjIsAnd(pObj) )
        {
            pUnrObj->hFan1   = Vec_IntEntry( vMap, Gia_ObjFaninId1(pObj, i) );
            pUnrObj->fCompl1 = Gia_ObjFaninC1(pObj);
        }
        else if ( Gia_ObjIsRo(p->pGia, pObj) )
        {
            pUnrObj->hFan0   = Vec_IntEntry( vMap, Gia_ObjId(p->pGia, Gia_ObjRoToRi(p->pGia, pObj)) );
            pUnrObj->fCompl0 = 0;
        }
        else if ( Gia_ObjIsPi(p->pGia, pObj) )
        {
            pUnrObj->hFan0   = Gia_ObjCioId(pObj);           // remember CIO id
            pUnrObj->hFan1   = Vec_IntEntry(p->vTents, i);   // remember tent
            pUnrObj->fItIsPi = 1;
        }
    }
    // store CI/PO objects;
    Gia_ManForEachCi( p->pGia, pObj, i )
        Vec_IntPush( p->vCiMap, Vec_IntEntry(vMap, Gia_ObjId(p->pGia, pObj)) );
    Gia_ManForEachCo( p->pGia, pObj, i )
        Vec_IntPush( p->vCoMap, Vec_IntEntry(vMap, Gia_ObjId(p->pGia, pObj)) );
    Vec_IntFreeP( &vMap );
    // print stats
    if ( fVerbose )
    {
        Unr_ManProfileRanks( p->vRanks );
        printf( "Memory usage = %6.2f MB  ", 4.0 * nInts / (1<<20) );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    }
    Vec_IntFreeP( &p->vOrder );
    Vec_IntFreeP( &p->vOrderLim );
    Vec_IntFreeP( &p->vRanks );
    Vec_IntFreeP( &p->vTents );
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Unr_Man_t * Unr_ManAlloc( Gia_Man_t * pGia )
{
    Unr_Man_t * p;
    p = ABC_CALLOC( Unr_Man_t, 1 );
    p->pGia      = pGia;
    p->nObjs     = Gia_ManObjNum(pGia);
    p->vOrder    = Vec_IntAlloc( p->nObjs );
    p->vOrderLim = Vec_IntAlloc( 100 );
    p->vTents    = Vec_IntStartFull( p->nObjs );
    p->vRanks    = Vec_IntStart( p->nObjs );
    p->vObjLim   = Vec_IntAlloc( 100 );
    p->vCiMap    = Vec_IntAlloc( Gia_ManCiNum(pGia) );
    p->vCoMap    = Vec_IntAlloc( Gia_ManCoNum(pGia) );
    p->vPiLits   = Vec_IntAlloc( 10000 );
    p->pFrames   = Gia_ManStart( 10000 );
    p->pFrames->pName = Abc_UtilStrsav( pGia->pName );
    Gia_ManHashStart( p->pFrames );
    return p;
}
void Unr_ManFree( Unr_Man_t * p )
{
    Gia_ManStop( p->pFrames );
    // intermediate data
    Vec_IntFreeP( &p->vOrder );
    Vec_IntFreeP( &p->vOrderLim );
    Vec_IntFreeP( &p->vTents );
    Vec_IntFreeP( &p->vRanks );
    // unrolling data
    Vec_IntFreeP( &p->vObjLim );
    Vec_IntFreeP( &p->vCiMap );
    Vec_IntFreeP( &p->vCoMap );
    Vec_IntFreeP( &p->vPiLits );
    ABC_FREE( p->pObjs );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Perform smart unrolling.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Unr_Man_t * Unr_ManUnrollStart( Gia_Man_t * pGia, int fVerbose )
{
    int i, iHandle;
    Unr_Man_t * p;
    p = Unr_ManAlloc( pGia );
    Unr_ManSetup( p, fVerbose );
    for ( i = 0; i < Gia_ManRegNum(p->pGia); i++ )
        if ( (iHandle = Vec_IntEntry(p->vCoMap, Gia_ManPoNum(p->pGia) + i)) != -1 )
            Unr_ManObjSetValue( Unr_ManObj(p, iHandle), 0 );
    return p;
}
Gia_Man_t * Unr_ManUnrollFrame( Unr_Man_t * p, int f )
{
    int i, iLit, iLit0, iLit1, hStart;
    for ( i = 0; i < Gia_ManPiNum(p->pGia); i++ )
        Vec_IntPush( p->vPiLits, Gia_ManAppendCi(p->pFrames) );
    hStart = Vec_IntEntry( p->vObjLim, Abc_MaxInt(0, Vec_IntSize(p->vObjLim)-1-f) );
    while ( p->pObjs + hStart < p->pEnd )
    {
        Unr_Obj_t * pUnrObj = Unr_ManObj( p, hStart );
        if ( pUnrObj->uRDiff0 != UNR_DIFF_NULL && pUnrObj->uRDiff1 != UNR_DIFF_NULL ) // AND node
        {
            iLit0 = Unr_ManFanin0Value( p, pUnrObj );
            iLit1 = Unr_ManFanin1Value( p, pUnrObj );
            iLit  = Gia_ManHashAnd( p->pFrames, iLit0, iLit1 );
            Unr_ManObjSetValue( pUnrObj, iLit );
        }
        else if ( pUnrObj->uRDiff0 != UNR_DIFF_NULL && pUnrObj->uRDiff1 == UNR_DIFF_NULL ) // PO/RI/RO
        {
            iLit  = Unr_ManFanin0Value( p, pUnrObj );
            Unr_ManObjSetValue( pUnrObj, iLit );
            if ( pUnrObj->fItIsPo )
                Gia_ManAppendCo( p->pFrames, iLit );
        }
        else // PI  (pUnrObj->hFan0 is CioId; pUnrObj->hFan1 is tent)
        {
            assert( pUnrObj->fItIsPi && f >= (int)pUnrObj->hFan1 );
            iLit = Vec_IntEntry( p->vPiLits, Gia_ManPiNum(p->pGia) * (f - pUnrObj->hFan1) + pUnrObj->hFan0 );
            Unr_ManObjSetValue( pUnrObj, iLit );
        }
        hStart += Unr_ObjSize( pUnrObj );
    }
    assert( p->pObjs + hStart == p->pEnd );
    assert( Gia_ManPoNum(p->pFrames) == (f + 1) * Gia_ManPoNum(p->pGia) );
    return p->pFrames;
}
Gia_Man_t * Unr_ManUnroll( Gia_Man_t * pGia, int nFrames )
{
    Unr_Man_t * p;
    Gia_Man_t * pFrames;
    int f;
    p = Unr_ManUnrollStart( pGia, 1 );
    for ( f = 0; f < nFrames; f++ )
        Unr_ManUnrollFrame( p, f );
    pFrames = Gia_ManCleanup( p->pFrames );
    Unr_ManFree( p );
    return pFrames;
}

/**Function*************************************************************

  Synopsis    [Perform naive unrolling.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Unr_ManUnrollSimple( Gia_Man_t * pGia, int nFrames )
{
    Gia_Man_t * pFrames;
    Gia_Obj_t * pObj, * pObjRi;
    int f, i; 
    pFrames = Gia_ManStart( 10000 );
    pFrames->pName = Abc_UtilStrsav( pGia->pName );
    Gia_ManHashAlloc( pFrames );
    Gia_ManConst0(pGia)->Value = 0;
    Gia_ManForEachRi( pGia, pObj, i )
        pObj->Value = 0;
    for ( f = 0; f < nFrames; f++ )
    {
        Gia_ManForEachPi( pGia, pObj, i )
            pObj->Value = Gia_ManAppendCi(pFrames);
        Gia_ManForEachRiRo( pGia, pObjRi, pObj, i )
            pObj->Value = pObjRi->Value;
        Gia_ManForEachAnd( pGia, pObj, i )
            pObj->Value = Gia_ManHashAnd( pFrames, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        Gia_ManForEachCo( pGia, pObj, i )
            pObj->Value = Gia_ObjFanin0Copy(pObj);
        Gia_ManForEachPo( pGia, pObj, i )
            Gia_ManAppendCo( pFrames, pObj->Value );
    }
    Gia_ManHashStop( pFrames );
    Gia_ManSetRegNum( pFrames, 0 );
    pFrames = Gia_ManCleanup( pGia = pFrames );
    Gia_ManStop( pGia );
    return pFrames;
}

/**Function*************************************************************

  Synopsis    [Perform evaluation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Unr_ManTest( Gia_Man_t * pGia, int nFrames )
{
    Gia_Man_t * pFrames0, * pFrames1;
    abctime clk = Abc_Clock();
    pFrames0 = Unr_ManUnroll( pGia, nFrames );
    Abc_PrintTime( 1, "Unroll ", Abc_Clock() - clk );
    clk = Abc_Clock();
    pFrames1 = Unr_ManUnrollSimple( pGia, nFrames );
    Abc_PrintTime( 1, "UnrollS", Abc_Clock() - clk );

Gia_ManPrintStats( pFrames0, NULL );
Gia_ManPrintStats( pFrames1, NULL );
Gia_AigerWrite( pFrames0, "frames0.aig", 0, 0 );
Gia_AigerWrite( pFrames1, "frames1.aig", 0, 0 );

    Gia_ManStop( pFrames0 );
    Gia_ManStop( pFrames1 );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

