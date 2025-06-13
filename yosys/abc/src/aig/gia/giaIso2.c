/**CFile****************************************************************

  FileName    [gia.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: gia.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "misc/vec/vecHsh.h"
#include "misc/vec/vecWec.h"


ABC_NAMESPACE_IMPL_START


#define ISO_MASK 0xFF
static unsigned int s_256Primes[ISO_MASK+1] = 
{
    0x984b6ad9,0x18a6eed3,0x950353e2,0x6222f6eb,0xdfbedd47,0xef0f9023,0xac932a26,0x590eaf55,
    0x97d0a034,0xdc36cd2e,0x22736b37,0xdc9066b0,0x2eb2f98b,0x5d9c7baf,0x85747c9e,0x8aca1055,
    0x50d66b74,0x2f01ae9e,0xa1a80123,0x3e1ce2dc,0xebedbc57,0x4e68bc34,0x855ee0cf,0x17275120,
    0x2ae7f2df,0xf71039eb,0x7c283eec,0x70cd1137,0x7cf651f3,0xa87bfa7a,0x14d87f02,0xe82e197d,
    0x8d8a5ebe,0x1e6a15dc,0x197d49db,0x5bab9c89,0x4b55dea7,0x55dede49,0x9a6a8080,0xe5e51035,
    0xe148d658,0x8a17eb3b,0xe22e4b38,0xe5be2a9a,0xbe938cbb,0x3b981069,0x7f9c0c8e,0xf756df10,
    0x8fa783f7,0x252062ce,0x3dc46b4b,0xf70f6432,0x3f378276,0x44b137a1,0x2bf74b77,0x04892ed6,
    0xfd318de1,0xd58c235e,0x94c6d25b,0x7aa5f218,0x35c9e921,0x5732fbbb,0x06026481,0xf584a44f,
    0x946e1b5f,0x8463d5b2,0x4ebca7b2,0x54887b15,0x08d1e804,0x5b22067d,0x794580f6,0xb351ea43,
    0xbce555b9,0x19ae2194,0xd32f1396,0x6fc1a7f1,0x1fd8a867,0x3a89fdb0,0xea49c61c,0x25f8a879,
    0xde1e6437,0x7c74afca,0x8ba63e50,0xb1572074,0xe4655092,0xdb6f8b1c,0xc2955f3c,0x327f85ba,
    0x60a17021,0x95bd261d,0xdea94f28,0x04528b65,0xbe0109cc,0x26dd5688,0x6ab2729d,0xc4f029ce,
    0xacf7a0be,0x4c912f55,0x34c06e65,0x4fbb938e,0x1533fb5f,0x03da06bd,0x48262889,0xc2523d7d,
    0x28a71d57,0x89f9713a,0xf574c551,0x7a99deb5,0x52834d91,0x5a6f4484,0xc67ba946,0x13ae698f,
    0x3e390f34,0x34fc9593,0x894c7932,0x6cf414a3,0xdb7928ab,0x13a3b8a3,0x4b381c1d,0xa10b54cb,
    0x55359d9d,0x35a3422a,0x58d1b551,0x0fd4de20,0x199eb3f4,0x167e09e2,0x3ee6a956,0x5371a7fa,
    0xd424efda,0x74f521c5,0xcb899ff6,0x4a42e4f4,0x747917b6,0x4b08df0b,0x090c7a39,0x11e909e4,
    0x258e2e32,0xd9fad92d,0x48fe5f69,0x0545cde6,0x55937b37,0x9b4ae4e4,0x1332b40e,0xc3792351,
    0xaff982ef,0x4dba132a,0x38b81ef1,0x28e641bf,0x227208c1,0xec4bbe37,0xc4e1821c,0x512c9d09,
    0xdaef1257,0xb63e7784,0x043e04d7,0x9c2cea47,0x45a0e59a,0x281315ca,0x849f0aac,0xa4071ed3,
    0x0ef707b3,0xfe8dac02,0x12173864,0x471f6d46,0x24a53c0a,0x35ab9265,0xbbf77406,0xa2144e79,
    0xb39a884a,0x0baf5b6d,0xcccee3dd,0x12c77584,0x2907325b,0xfd1adcd2,0xd16ee972,0x345ad6c1,
    0x315ebe66,0xc7ad2b8d,0x99e82c8d,0xe52da8c8,0xba50f1d3,0x66689cd8,0x2e8e9138,0x43e15e74,
    0xf1ced14d,0x188ec52a,0xe0ef3cbb,0xa958aedc,0x4107a1bc,0x5a9e7a3e,0x3bde939f,0xb5b28d5a,
    0x596fe848,0xe85ad00c,0x0b6b3aae,0x44503086,0x25b5695c,0xc0c31dcd,0x5ee617f0,0x74d40c3a,
    0xd2cb2b9f,0x1e19f5fa,0x81e24faf,0xa01ed68f,0xcee172fc,0x7fdf2e4d,0x002f4774,0x664f82dd,
    0xc569c39a,0xa2d4dcbe,0xaadea306,0xa4c947bf,0xa413e4e3,0x81fb5486,0x8a404970,0x752c980c,
    0x98d1d881,0x5c932c1e,0xeee65dfb,0x37592cdd,0x0fd4e65b,0xad1d383f,0x62a1452f,0x8872f68d,
    0xb58c919b,0x345c8ee3,0xb583a6d6,0x43d72cb3,0x77aaa0aa,0xeb508242,0xf2db64f8,0x86294328,
    0x82211731,0x1239a9d5,0x673ba5de,0xaf4af007,0x44203b19,0x2399d955,0xa175cd12,0x595928a7,
    0x6918928b,0xde3126bb,0x6c99835c,0x63ba1fa2,0xdebbdff0,0x3d02e541,0xd6f7aac6,0xe80b4cd0,
    0xd0fa29f1,0x804cac5e,0x2c226798,0x462f624c,0xad05b377,0x22924fcd,0xfbea205c,0x1b47586d
};

static int s_PrimeC = 49;
//static int s_PrimeC = 1;

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Gia_Iso2Man_t_       Gia_Iso2Man_t;
struct Gia_Iso2Man_t_ 
{
    Gia_Man_t *      pGia;
    int              nObjs;
    int              nUniques;
    // internal data
    Vec_Int_t *      vUniques;      // unique numbers
    Vec_Int_t *      vTied;         // tied objects   
    Vec_Int_t *      vTable;        // hash table
    Vec_Int_t *      vPlaces;       // used places in the table
    Vec_Ptr_t *      vSingles;      // singleton objects
    // isomorphism check
    Vec_Int_t *      vVec0;         // isomorphism map
    Vec_Int_t *      vVec1;         // isomorphism map
    Vec_Int_t *      vMap0;         // isomorphism map
    Vec_Int_t *      vMap1;         // isomorphism map
    // statistics 
    int              nIters;
    abctime          timeStart;
    abctime          timeSim;
    abctime          timeRefine;
    abctime          timeSort;
    abctime          timeOther;
    abctime          timeTotal;
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_Iso2ManCollectTies( Gia_Man_t * p )
{
    Vec_Int_t * vTies;
    Gia_Obj_t * pObj;
    int i;
    vTies = Vec_IntAlloc( Gia_ManCandNum(p) );
    Gia_ManForEachCand( p, pObj, i )
        Vec_IntPush( vTies, i );
    return vTies;
}
void Gia_Iso2ManPrepare( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObj( p, pObj, i )
        pObj->Value = Gia_ObjIsAnd(pObj) ? 1 + Abc_MaxInt(Gia_ObjFanin0(pObj)->Value, Gia_ObjFanin1(pObj)->Value) : 0;
    Gia_ManConst0(p)->Value = s_256Primes[ISO_MASK];
    Gia_ManForEachObj1( p, pObj, i )
        if ( Gia_ObjIsAnd(pObj) )
            pObj->Value = s_256Primes[pObj->Value & ISO_MASK] + s_256Primes[ISO_MASK - 10 + Gia_ObjFaninC0(pObj) + Gia_ObjFaninC1(pObj)];
        else if ( Gia_ObjIsPi(p, pObj) )
            pObj->Value = s_256Primes[ISO_MASK-1];
        else if ( Gia_ObjIsRo(p, pObj) )
            pObj->Value = s_256Primes[ISO_MASK-2];
}
void Gia_Iso2ManPropagate( Gia_Man_t * p )
{
    Gia_Obj_t * pObj, * pObjRo;
    int i;
    Gia_ManForEachObj1( p, pObj, i )
        if ( Gia_ObjIsAnd(pObj) )
        {
            pObj->Value += (Gia_ObjFaninC0(pObj) + s_PrimeC) * Gia_ObjFanin0(pObj)->Value + (Gia_ObjFaninC1(pObj) + s_PrimeC) * Gia_ObjFanin1(pObj)->Value;
            if ( Gia_ObjFaninC0(pObj) == Gia_ObjFaninC1(pObj) && Gia_ObjFanin0(pObj)->Value == Gia_ObjFanin1(pObj)->Value )
                pObj->Value += s_256Primes[ISO_MASK - 11];
        }
        else if ( Gia_ObjIsCo(pObj) )
            pObj->Value += (Gia_ObjFaninC0(pObj) + s_PrimeC) * Gia_ObjFanin0(pObj)->Value;
    Gia_ManForEachRiRo( p, pObj, pObjRo, i )
    {
        pObjRo->Value += pObj->Value;
        if ( pObjRo == Gia_ObjFanin0(pObj) )
            pObjRo->Value += s_256Primes[ISO_MASK - 12];
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Gia_Iso2ManCone_rec( Gia_Man_t * p, int Id, int Level )
{
    Gia_Obj_t * pObj;
    if ( Level == 0 )
        return 0;
    if ( Gia_ObjIsTravIdCurrentId(p, Id) )
        return 0;
    Gia_ObjSetTravIdCurrentId(p, Id);
    pObj = Gia_ManObj( p, Id );
    if ( Gia_ObjIsAnd(pObj) )
        return pObj->Value + Gia_Iso2ManCone_rec( p, Gia_ObjFaninId0(pObj, Id), Level-1 ) + Gia_Iso2ManCone_rec( p, Gia_ObjFaninId1(pObj, Id), Level-1 );
    if ( Gia_ObjIsPi(p, pObj) )
        return pObj->Value;
    if ( Gia_ObjIsRo(p, pObj) )
        return pObj->Value + Gia_Iso2ManCone_rec( p, Gia_ObjId(p, Gia_ObjFanin0(Gia_ObjRoToRi(p, pObj))), Level );
    assert( Gia_ObjIsConst0(pObj) );
    return pObj->Value;
}
unsigned Gia_Iso2ManCone( Gia_Man_t * p, int Id, int Level )
{
    Gia_ManIncrementTravId( p );
    return Gia_Iso2ManCone_rec( p, Id, Level );
}
void Gia_Iso2ManUpdate( Gia_Iso2Man_t * p, int Level )
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObjVec( p->vTied, p->pGia, pObj, i )
        pObj->Value += Gia_Iso2ManCone( p->pGia, Gia_ObjId(p->pGia, pObj), Level );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Iso2Man_t * Gia_Iso2ManStart( Gia_Man_t * pGia )
{
    Gia_Iso2Man_t * p;
    p = ABC_CALLOC( Gia_Iso2Man_t, 1 );
    p->pGia      = pGia;
    p->nObjs     = Gia_ManObjNum( pGia );
    p->nUniques  = 0;
    // internal data
    p->vUniques  = Vec_IntStartFull( p->nObjs );
    p->vTied     = Gia_Iso2ManCollectTies( pGia );
    p->vTable    = Vec_IntStart( Abc_PrimeCudd(1*p->nObjs) );
    p->vPlaces   = Vec_IntAlloc( 1000 );
    p->vSingles  = Vec_PtrAlloc( 1000 );
    p->vVec0     = Vec_IntAlloc( 10000 );
    p->vVec1     = Vec_IntAlloc( 10000 );
    p->vMap0     = Vec_IntStart( p->nObjs );
    p->vMap1     = Vec_IntStart( p->nObjs );
    // add constant 0 object
    Vec_IntWriteEntry( p->vUniques, 0, p->nUniques++ );
    return p;
}
void Gia_Iso2ManStop( Gia_Iso2Man_t * p )
{
    Vec_IntFree( p->vUniques );
    Vec_IntFree( p->vTied );
    Vec_IntFree( p->vTable );
    Vec_IntFree( p->vPlaces );
    Vec_PtrFree( p->vSingles );
    Vec_IntFree( p->vMap0 );
    Vec_IntFree( p->vMap1 );
    Vec_IntFree( p->vVec0 );
    Vec_IntFree( p->vVec1 );
    ABC_FREE( p );
}
void Gia_Iso2ManPrint( Gia_Iso2Man_t * p, abctime Time, int fVerbose )
{
    if ( !fVerbose )
        return;
    printf( "Iter %4d :  ", p->nIters++ );
    printf( "Entries =%8d.  ", Vec_IntSize(p->vTied) );
    printf( "Uniques =%8d.  ", p->nUniques );
    printf( "Singles =%8d.  ", Vec_PtrSize(p->vSingles) );
    printf( "%9.2f sec", (float)(Time)/(float)(CLOCKS_PER_SEC) );
    printf( "\n" );
    fflush( stdout );
}

/**Function*************************************************************

  Synopsis    [Uniqifies objects using their signature.]

  Description [Assumes the tied objects are in p->vTied. Assumes that 
  updated signature (pObj->Value) is assigned to these objects. Returns 
  the array of unique objects p->vSingles sorted by signature.  Compacts 
  the array of tied objects p->vTied.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ObjCompareByValue2( Gia_Obj_t ** pp1, Gia_Obj_t ** pp2 ) { return (int)(*pp1)->Value - (int)(*pp2)->Value; }
int Gia_Iso2ManUniqify( Gia_Iso2Man_t * p )
{
    int fVerify = 0;
    Gia_Obj_t * pObj, * pTemp;
    int * pTable = Vec_IntArray(p->vTable);
    int i, k, nSize = Vec_IntSize(p->vTable);

    if ( fVerify )
        for ( k = 0; k < nSize; k++ )
            assert( pTable[k] == 0 );
    if ( fVerify )
        Gia_ManForEachObjVec( p->vTied, p->pGia, pObj, i )
            assert( pObj->fMark0 == 0 );

#if 0
    Gia_ManForEachObjVec( p->vTied, p->pGia, pObj, i )
    {
        printf( "%3d : ", Gia_ObjId(p->pGia, pObj) );
        Extra_PrintBinary( stdout, &pObj->Value, 32 );
        printf( "\n" );
    }
#endif

    // add objects to the table
    Vec_IntClear( p->vPlaces );
    Gia_ManForEachObjVec( p->vTied, p->pGia, pObj, i )
    {
        for ( k = pObj->Value % nSize; (pTemp = pTable[k] ? Gia_ManObj(p->pGia, pTable[k]) : NULL); k = (k + 1) % nSize )
            if ( pTemp->Value == pObj->Value )
            {
                pTemp->fMark0 = 1;
                pObj->fMark0 = 1;
                break;
            }
        if ( pTemp != NULL )
            continue;
        pTable[k] = Gia_ObjId(p->pGia, pObj);
        Vec_IntPush( p->vPlaces, k );
    }
    // clean the table
    Vec_IntForEachEntry( p->vPlaces, k, i )
        pTable[k] = 0;
    // collect singleton objects and compact tied objects
    k = 0;
    Vec_PtrClear( p->vSingles );
    Gia_ManForEachObjVec( p->vTied, p->pGia, pObj, i )
        if ( pObj->fMark0 == 0 )
            Vec_PtrPush( p->vSingles, pObj );
        else 
        {
            pObj->fMark0 = 0;
            Vec_IntWriteEntry( p->vTied, k++, Gia_ObjId(p->pGia, pObj) );
        }
    Vec_IntShrink( p->vTied, k );
    // sort singletons
    Vec_PtrSort( p->vSingles, (int (*)(const void *, const void *))Gia_ObjCompareByValue2 );
    // add them to unique and increment signature
    Vec_PtrForEachEntry( Gia_Obj_t *, p->vSingles, pObj, i )
    {
        pObj->Value += s_256Primes[p->nUniques & ISO_MASK];
        assert( Vec_IntEntry(p->vUniques, Gia_ObjId(p->pGia, pObj)) == -1 );
        Vec_IntWriteEntry( p->vUniques, Gia_ObjId(p->pGia, pObj), p->nUniques++ );
    }
    return Vec_PtrSize( p->vSingles );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Wec_t * Gia_Iso2ManDerivePoClasses( Gia_Man_t * pGia )
{
    Vec_Wec_t * vEquivs;
    Vec_Int_t * vValues;
    Vec_Int_t * vMap;
    Gia_Obj_t * pObj;
    int i;
    vValues = Vec_IntAlloc( Gia_ManPoNum(pGia) );
    Gia_ManForEachPo( pGia, pObj, i )
        Vec_IntPush( vValues, pObj->Value );
    vMap = Hsh_IntManHashArray( vValues, 1 );
    Vec_IntFree( vValues );
    vEquivs = Vec_WecCreateClasses( vMap );
    Vec_IntFree( vMap );
    return vEquivs;    
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_Iso2ManCollectOrder2_rec( Gia_Man_t * p, int Id, Vec_Int_t * vVec )
{
    Gia_Obj_t * pObj;
    if ( Gia_ObjIsTravIdCurrentId(p, Id) )
        return;
    Gia_ObjSetTravIdCurrentId(p, Id);
    pObj = Gia_ManObj( p, Id );
    if ( Gia_ObjIsAnd(pObj) )
    {
        if ( Gia_ObjFanin0(pObj)->Value <= Gia_ObjFanin1(pObj)->Value )
        {
            Gia_Iso2ManCollectOrder2_rec( p, Gia_ObjFaninId0(pObj, Id), vVec );
            Gia_Iso2ManCollectOrder2_rec( p, Gia_ObjFaninId1(pObj, Id), vVec );
        }
        else
        {
            Gia_Iso2ManCollectOrder2_rec( p, Gia_ObjFaninId1(pObj, Id), vVec );
            Gia_Iso2ManCollectOrder2_rec( p, Gia_ObjFaninId0(pObj, Id), vVec );
        }
    }
    else if ( Gia_ObjIsCo(pObj) )
    {
        Gia_Iso2ManCollectOrder2_rec( p, Gia_ObjFaninId0(pObj, Id), vVec );
    }
    else if ( Gia_ObjIsPi(p, pObj) )
    {
    }
    else assert( Gia_ObjIsConst0(pObj) );
    Vec_IntPush( vVec, Id );
}
Vec_Int_t * Gia_Iso2ManCollectOrder2( Gia_Man_t * pGia, int * pPos, int nPos )
{
    Vec_Int_t * vVec;
    int i;
    vVec = Vec_IntAlloc( 1000 );
    Gia_ManIncrementTravId( pGia );
    for ( i = 0; i < nPos; i++ )
        Gia_Iso2ManCollectOrder2_rec( pGia, Gia_ObjId(pGia, Gia_ManPo(pGia, pPos[i])), vVec );
    return vVec;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_Iso2ManCollectOrder_rec( Gia_Man_t * p, int Id, Vec_Int_t * vRoots, Vec_Int_t * vVec, Vec_Int_t * vMap )
{
    Gia_Obj_t * pObj;
    if ( Gia_ObjIsTravIdCurrentId(p, Id) )
        return;
    Gia_ObjSetTravIdCurrentId(p, Id);
    pObj = Gia_ManObj( p, Id );
    if ( Gia_ObjIsAnd(pObj) )
    {
        if ( Gia_ObjFanin0(pObj)->Value <= Gia_ObjFanin1(pObj)->Value )
        {
            Gia_Iso2ManCollectOrder_rec( p, Gia_ObjFaninId0(pObj, Id), vRoots, vVec, vMap );
            Gia_Iso2ManCollectOrder_rec( p, Gia_ObjFaninId1(pObj, Id), vRoots, vVec, vMap );
        }
        else
        {
            Gia_Iso2ManCollectOrder_rec( p, Gia_ObjFaninId1(pObj, Id), vRoots, vVec, vMap );
            Gia_Iso2ManCollectOrder_rec( p, Gia_ObjFaninId0(pObj, Id), vRoots, vVec, vMap );
        }
    }
    else if ( Gia_ObjIsCo(pObj) )
    {
        Gia_Iso2ManCollectOrder_rec( p, Gia_ObjFaninId0(pObj, Id), vRoots, vVec, vMap );
    }
    else if ( Gia_ObjIsCi(pObj) )
    {
        if ( Gia_ObjIsRo(p, pObj) )
            Vec_IntPush( vRoots, Gia_ObjId(p, Gia_ObjRoToRi(p, pObj)) );
    }
    else assert( Gia_ObjIsConst0(pObj) );
    Vec_IntWriteEntry( vMap, Id, Vec_IntSize(vVec) );
    Vec_IntPush( vVec, Id );
}
void Gia_Iso2ManCollectOrder( Gia_Man_t * pGia, int * pPos, int nPos, Vec_Int_t * vRoots, Vec_Int_t * vVec, Vec_Int_t * vMap )
{
    int i, iRoot;
    Vec_IntClear( vRoots );
    for ( i = 0; i < nPos; i++ )
        Vec_IntPush( vRoots, Gia_ObjId(pGia, Gia_ManPo(pGia, pPos[i])) );
    Vec_IntClear( vVec );
    Gia_ManIncrementTravId( pGia );
    Vec_IntForEachEntry( vRoots, iRoot, i )
        Gia_Iso2ManCollectOrder_rec( pGia, iRoot, vRoots, vVec, vMap );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_Iso2ManCheckIsoPair( Gia_Man_t * p, Vec_Int_t * vVec0, Vec_Int_t * vVec1, Vec_Int_t * vMap0, Vec_Int_t * vMap1 )
{
    Gia_Obj_t * pObj0, * pObj1;
    int k, iObj0, iObj1;
    Vec_IntForEachEntryTwo( vVec0, vVec1, iObj0, iObj1, k )
    {
        if ( iObj0 == iObj1 )
            continue;
        pObj0 = Gia_ManObj(p, iObj0);
        pObj1 = Gia_ManObj(p, iObj1);
        if ( pObj0->Value != pObj1->Value )
            return 0;
        assert( pObj0->Value == pObj1->Value );
        if ( !Gia_ObjIsAnd(pObj0) )
            continue;
        if ( Gia_ObjFanin0(pObj0)->Value <= Gia_ObjFanin1(pObj0)->Value )
        {
            if ( Gia_ObjFanin0(pObj1)->Value <= Gia_ObjFanin1(pObj1)->Value )
            {
                if ( Gia_ObjFaninC0(pObj0) != Gia_ObjFaninC0(pObj1)  || Gia_ObjFaninC1(pObj0) != Gia_ObjFaninC1(pObj1)   ||
                     Vec_IntEntry(vMap0, Gia_ObjFaninId0p(p, pObj0)) != Vec_IntEntry( vMap1, Gia_ObjFaninId0p(p, pObj1)) || 
                     Vec_IntEntry(vMap0, Gia_ObjFaninId1p(p, pObj0)) != Vec_IntEntry( vMap1, Gia_ObjFaninId1p(p, pObj1)) )
                     return 0;
            }
            else
            {
                if ( Gia_ObjFaninC0(pObj0) != Gia_ObjFaninC1(pObj1)  || Gia_ObjFaninC1(pObj0) != Gia_ObjFaninC0(pObj1)   ||
                     Vec_IntEntry(vMap0, Gia_ObjFaninId0p(p, pObj0)) != Vec_IntEntry( vMap1, Gia_ObjFaninId1p(p, pObj1)) || 
                     Vec_IntEntry(vMap0, Gia_ObjFaninId1p(p, pObj0)) != Vec_IntEntry( vMap1, Gia_ObjFaninId0p(p, pObj1)) )
                     return 0;
            }
        }
        else
        {
            if ( Gia_ObjFanin0(pObj1)->Value <= Gia_ObjFanin1(pObj1)->Value )
            {
                if ( Gia_ObjFaninC1(pObj0) != Gia_ObjFaninC0(pObj1)  || Gia_ObjFaninC0(pObj0) != Gia_ObjFaninC1(pObj1)   ||
                     Vec_IntEntry(vMap0, Gia_ObjFaninId1p(p, pObj0)) != Vec_IntEntry( vMap1, Gia_ObjFaninId0p(p, pObj1)) || 
                     Vec_IntEntry(vMap0, Gia_ObjFaninId0p(p, pObj0)) != Vec_IntEntry( vMap1, Gia_ObjFaninId1p(p, pObj1)) )
                     return 0;
            }
            else
            {
                if ( Gia_ObjFaninC1(pObj0) != Gia_ObjFaninC1(pObj1)  || Gia_ObjFaninC0(pObj0) != Gia_ObjFaninC0(pObj1)   ||
                     Vec_IntEntry(vMap0, Gia_ObjFaninId1p(p, pObj0)) != Vec_IntEntry( vMap1, Gia_ObjFaninId1p(p, pObj1)) || 
                     Vec_IntEntry(vMap0, Gia_ObjFaninId0p(p, pObj0)) != Vec_IntEntry( vMap1, Gia_ObjFaninId0p(p, pObj1)) )
                     return 0;
            }
        }
    }
    return 1;
} 

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_Iso2ManCheckIsoClassOneSkip( Gia_Man_t * p, Vec_Int_t * vClass, Vec_Int_t * vRoots, Vec_Int_t * vVec0, Vec_Int_t * vVec1, Vec_Int_t * vMap0, Vec_Int_t * vMap1 )
{
    int i, iPo;
    assert( Vec_IntSize(vClass) > 1 );
    iPo = Vec_IntEntry( vClass, 0 );
    Gia_Iso2ManCollectOrder( p, &iPo, 1, vRoots, vVec0, vMap0 );
    Vec_IntForEachEntryStart( vClass, iPo, i, 1 )
    {
        Gia_Iso2ManCollectOrder( p, &iPo, 1, vRoots, vVec1, vMap1 );
        if ( Vec_IntSize(vVec0) != Vec_IntSize(vVec1) )
            return 0;
        if ( !Gia_Iso2ManCheckIsoPair( p, vVec0, vVec1, vMap0, vMap1 ) )
            return 0;
    }
    return 1;
} 
Vec_Wec_t * Gia_Iso2ManCheckIsoClassesSkip( Gia_Man_t * p, Vec_Wec_t * vEquivs )
{
    Vec_Wec_t * vEquivs2;
    Vec_Int_t * vRoots = Vec_IntAlloc( 10000 );
    Vec_Int_t * vVec0 = Vec_IntAlloc( 10000 );
    Vec_Int_t * vVec1 = Vec_IntAlloc( 10000 );
    Vec_Int_t * vMap0 = Vec_IntStart( Gia_ManObjNum(p) );
    Vec_Int_t * vMap1 = Vec_IntStart( Gia_ManObjNum(p) );
    Vec_Int_t * vClass, * vClass2;
    int i, k, Entry, Counter = 0;
    vEquivs2 = Vec_WecAlloc( 2 * Vec_WecSize(vEquivs) );
    Vec_WecForEachLevel( vEquivs, vClass, i )
    {
        if ( i % 50 == 0 )
            printf( "Finished %8d outputs (out of %8d)...\r", Counter, Gia_ManPoNum(p) ), fflush(stdout);
        Counter += Vec_IntSize(vClass);
        if ( Vec_IntSize(vClass) < 2 || Gia_Iso2ManCheckIsoClassOneSkip(p, vClass, vRoots, vVec0, vVec1, vMap0, vMap1) )
        {
            vClass2 = Vec_WecPushLevel( vEquivs2 );
            *vClass2 = *vClass;
            vClass->pArray = NULL;
            vClass->nSize = vClass->nCap = 0;
        }
        else
        {
            Vec_IntForEachEntry( vClass, Entry, k )
            {
                vClass2 = Vec_WecPushLevel( vEquivs2 );
                Vec_IntPush( vClass2, Entry );
            }
        }
    }
    Vec_IntFree( vRoots );
    Vec_IntFree( vVec0 );
    Vec_IntFree( vVec1 );
    Vec_IntFree( vMap0 );
    Vec_IntFree( vMap1 );
    return vEquivs2;
}

void Gia_Iso2ManCheckIsoClassOne( Gia_Man_t * p, Vec_Int_t * vClass, Vec_Int_t * vRoots, Vec_Int_t * vVec0, Vec_Int_t * vVec1, Vec_Int_t * vMap0, Vec_Int_t * vMap1, Vec_Int_t * vNewClass )
{
    int i, k = 1, iPo;
    Vec_IntClear( vNewClass );
    if ( Vec_IntSize(vClass) <= 1 )
        return;
    assert( Vec_IntSize(vClass) > 1 );
    iPo = Vec_IntEntry( vClass, 0 );
    Gia_Iso2ManCollectOrder( p, &iPo, 1, vRoots, vVec0, vMap0 );
    Vec_IntForEachEntryStart( vClass, iPo, i, 1 )
    {
        Gia_Iso2ManCollectOrder( p, &iPo, 1, vRoots, vVec1, vMap1 );
        if ( Vec_IntSize(vVec0) == Vec_IntSize(vVec1) && Gia_Iso2ManCheckIsoPair(p, vVec0, vVec1, vMap0, vMap1) )
            Vec_IntWriteEntry( vClass, k++, iPo );
        else
            Vec_IntPush( vNewClass, iPo );
    }
    Vec_IntShrink( vClass, k );
} 
Vec_Wec_t * Gia_Iso2ManCheckIsoClasses( Gia_Man_t * p, Vec_Wec_t * vEquivs )
{
    Vec_Wec_t * vEquivs2;
    Vec_Int_t * vRoots = Vec_IntAlloc( 10000 );
    Vec_Int_t * vVec0 = Vec_IntAlloc( 10000 );
    Vec_Int_t * vVec1 = Vec_IntAlloc( 10000 );
    Vec_Int_t * vMap0 = Vec_IntStart( Gia_ManObjNum(p) );
    Vec_Int_t * vMap1 = Vec_IntStart( Gia_ManObjNum(p) );
    Vec_Int_t * vClass, * vClass2, * vNewClass;
    int i, Counter = 0;
    vNewClass = Vec_IntAlloc( 100 );
    vEquivs2 = Vec_WecAlloc( 2 * Vec_WecSize(vEquivs) );
    Vec_WecForEachLevel( vEquivs, vClass, i )
    {
        if ( i % 50 == 0 )
            printf( "Finished %8d outputs (out of %8d)...\r", Counter, Gia_ManPoNum(p) ), fflush(stdout);
        // split this class
        Gia_Iso2ManCheckIsoClassOne( p, vClass, vRoots, vVec0, vVec1, vMap0, vMap1, vNewClass );
        Counter += Vec_IntSize(vClass);
        // add remaining class
        vClass2 = Vec_WecPushLevel( vEquivs2 );
        *vClass2 = *vClass;
        vClass->pArray = NULL;
        vClass->nSize = vClass->nCap = 0;
        // add new class
        if ( Vec_IntSize(vNewClass) == 0 )
            continue;
        vClass = Vec_WecPushLevel( vEquivs );
        Vec_IntAppend( vClass, vNewClass );
    }
    Vec_IntFree( vNewClass );   
    Vec_IntFree( vRoots );
    Vec_IntFree( vVec0 );
    Vec_IntFree( vVec1 );
    Vec_IntFree( vMap0 );
    Vec_IntFree( vMap1 );
    return vEquivs2;
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Wec_t * Gia_Iso2ManPerform( Gia_Man_t * pGia, int fVerbose )
{
    Gia_Iso2Man_t * p;
    abctime clk = Abc_Clock();
    p = Gia_Iso2ManStart( pGia );
    Gia_Iso2ManPrepare( pGia );
    Gia_Iso2ManPropagate( pGia );
    Gia_Iso2ManPrint( p, Abc_Clock() - clk, fVerbose );
    while ( Gia_Iso2ManUniqify( p ) )
    {
        Gia_Iso2ManPrint( p, Abc_Clock() - clk, fVerbose );
        Gia_Iso2ManPropagate( pGia );
    }
    Gia_Iso2ManPrint( p, Abc_Clock() - clk, fVerbose );
/*
    Gia_Iso2ManUpdate( p, 20 );
    while ( Gia_Iso2ManUniqify( p ) )
    {
        Gia_Iso2ManPrint( p, Abc_Clock() - clk, fVerbose );
        Gia_Iso2ManPropagate( pGia );
    }
    Gia_Iso2ManPrint( p, Abc_Clock() - clk, fVerbose );
*/
    Gia_Iso2ManStop( p );
    return Gia_Iso2ManDerivePoClasses( pGia );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManIsoReduce2( Gia_Man_t * pGia, Vec_Ptr_t ** pvPosEquivs, Vec_Ptr_t ** pvPiPerms, int fEstimate, int fBetterQual, int fDualOut, int fVerbose, int fVeryVerbose )
{
    Gia_Man_t * pPart;
    Vec_Wec_t * vEquivs, * vEquivs2;
    Vec_Int_t * vRemains;
    int nClasses, nUsedPos;
    abctime clk = Abc_Clock();
    vEquivs = Gia_Iso2ManPerform( pGia, fVeryVerbose );
    // report class stats
    nClasses = Vec_WecCountNonTrivial( vEquivs, &nUsedPos );
    printf( "Reduced %d outputs to %d candidate   classes (%d outputs are in %d non-trivial classes).  ", 
        Gia_ManPoNum(pGia), Vec_WecSize(vEquivs), nUsedPos, nClasses );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    if ( fEstimate )
    {
        Vec_WecFree( vEquivs );
        return Gia_ManDup(pGia);
    }
    // verify classes
    if ( fBetterQual )
        vEquivs2 = Gia_Iso2ManCheckIsoClasses( pGia, vEquivs );
    else
        vEquivs2 = Gia_Iso2ManCheckIsoClassesSkip( pGia, vEquivs );
    Vec_WecFree( vEquivs );
    vEquivs = vEquivs2;
    // sort equiv classes by the first integer
    Vec_WecSortByFirstInt( vEquivs, 0 );
    // find the first outputs
    vRemains = Vec_WecCollectFirsts( vEquivs );
    // derive the final GIA
    pPart = Gia_ManDupCones( pGia, Vec_IntArray(vRemains), Vec_IntSize(vRemains), 0 );
    Vec_IntFree( vRemains );
    // report class stats
    nClasses = Vec_WecCountNonTrivial( vEquivs, &nUsedPos );
    printf( "Reduced %d outputs to %d equivalence classes (%d outputs are in %d non-trivial classes).  ", 
        Gia_ManPoNum(pGia), Vec_WecSize(vEquivs), nUsedPos, nClasses );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    if ( fVerbose )
    { 
        printf( "Nontrivial classes:\n" );
        Vec_WecPrint( vEquivs, 1 );
    }
    if ( pvPiPerms )
        *pvPiPerms = NULL;
    if ( pvPosEquivs )
        *pvPosEquivs = Vec_WecConvertToVecPtr( vEquivs );
    Vec_WecFree( vEquivs );
//    Gia_ManStopP( &pPart );
    return pPart;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

