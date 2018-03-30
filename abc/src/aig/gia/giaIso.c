/**CFile****************************************************************

  FileName    [giaIso.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Graph isomorphism.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaIso.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"

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

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Gia_IsoMan_t_       Gia_IsoMan_t;
struct Gia_IsoMan_t_ 
{
    Gia_Man_t *      pGia;
    int              nObjs;
    int              nUniques;
    int              nSingles;
    int              nEntries;
    // internal data
    int *            pLevels;
    int *            pUniques;
    word *           pStoreW;
    unsigned *       pStoreU;
    // equivalence classes
    Vec_Int_t *      vLevCounts;
    Vec_Int_t *      vClasses;
    Vec_Int_t *      vClasses2;
    // statistics 
    abctime          timeStart;
    abctime          timeSim;
    abctime          timeRefine;
    abctime          timeSort;
    abctime          timeOther;
    abctime          timeTotal;
};

static inline unsigned  Gia_IsoGetValue( Gia_IsoMan_t * p, int i )             { return (unsigned)(p->pStoreW[i]);       }
static inline unsigned  Gia_IsoGetItem( Gia_IsoMan_t * p, int i )              { return (unsigned)(p->pStoreW[i] >> 32); }

static inline void      Gia_IsoSetValue( Gia_IsoMan_t * p, int i, unsigned v ) { ((unsigned *)(p->pStoreW + i))[0] = v;  }
static inline void      Gia_IsoSetItem( Gia_IsoMan_t * p, int i, unsigned v )  { ((unsigned *)(p->pStoreW + i))[1] = v;  }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_IsoMan_t * Gia_IsoManStart( Gia_Man_t * pGia )
{
    Gia_IsoMan_t * p;
    p = ABC_CALLOC( Gia_IsoMan_t, 1 );
    p->pGia      = pGia;
    p->nObjs     = Gia_ManObjNum( pGia );
    p->nUniques  = 1;
    p->nEntries  = p->nObjs;
    // internal data
    p->pLevels   = ABC_CALLOC( int, p->nObjs );
    p->pUniques  = ABC_CALLOC( int, p->nObjs );
    p->pStoreW   = ABC_CALLOC( word, p->nObjs );
    // class representation
    p->vClasses  = Vec_IntAlloc( p->nObjs/4 );
    p->vClasses2 = Vec_IntAlloc( p->nObjs/4 );
    return p;
}
void Gia_IsoManStop( Gia_IsoMan_t * p )
{
    // class representation
    Vec_IntFree( p->vClasses );
    Vec_IntFree( p->vClasses2 );
    // internal data
    ABC_FREE( p->pLevels );
    ABC_FREE( p->pUniques );
    ABC_FREE( p->pStoreW );
    ABC_FREE( p );
}
void Gia_IsoManTransferUnique( Gia_IsoMan_t * p )
{
    Gia_Obj_t * pObj;
    int i;
    // copy unique numbers into the nodes
    Gia_ManForEachObj( p->pGia, pObj, i )
        pObj->Value = p->pUniques[i];
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_IsoPrintClasses( Gia_IsoMan_t * p )
{
    int fVerbose = 0;
    int i, k, iBegin, nSize;
    printf( "The total of %d classes:\n", Vec_IntSize(p->vClasses)/2 );
    Vec_IntForEachEntryDouble( p->vClasses, iBegin, nSize, i )
    {
        printf( "%5d : (%3d,%3d)  ", i/2, iBegin, nSize );
        if ( fVerbose )
        {
            printf( "{" );
            for ( k = 0; k < nSize; k++ )
                printf( " %3d,%08x", Gia_IsoGetItem(p, iBegin+k), Gia_IsoGetValue(p, iBegin+k) );
            printf( " }" );
        }
        printf( "\n" );
    }
}
void Gia_IsoPrint( Gia_IsoMan_t * p, int Iter, abctime Time )
{
    printf( "Iter %4d :  ", Iter );
    printf( "Entries =%8d.  ", p->nEntries );
//    printf( "Classes =%8d.  ", Vec_IntSize(p->vClasses)/2 );
    printf( "Uniques =%8d.  ", p->nUniques );
    printf( "Singles =%8d.  ", p->nSingles );
    printf( "%9.2f sec", (float)(Time)/(float)(CLOCKS_PER_SEC) );
    printf( "\n" );
    fflush( stdout );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_IsoPrepare( Gia_IsoMan_t * p )
{
    Gia_Obj_t * pObj;
    int * pLevBegins, * pLevSizes;
    int i, iObj, MaxLev = 0;
    // assign levels
    p->pLevels[0] = 0;
    Gia_ManForEachCi( p->pGia, pObj, i )
        p->pLevels[Gia_ObjId(p->pGia, pObj)] = 0;
    Gia_ManForEachAnd( p->pGia, pObj, i )
        p->pLevels[i] = 1 + Abc_MaxInt( p->pLevels[Gia_ObjFaninId0(pObj, i)], p->pLevels[Gia_ObjFaninId1(pObj, i)] );
    Gia_ManForEachCo( p->pGia, pObj, i )
    {
        iObj = Gia_ObjId(p->pGia, pObj);
        p->pLevels[iObj] = 1 + p->pLevels[Gia_ObjFaninId0(pObj, iObj)]; // "1 +" is different!
        MaxLev = Abc_MaxInt( MaxLev, p->pLevels[Gia_ObjId(p->pGia, pObj)] );
    }

    // count nodes on each level
    pLevSizes = ABC_CALLOC( int, MaxLev+1 );
    for ( i = 1; i < p->nObjs; i++ )
        pLevSizes[p->pLevels[i]]++;
    // start classes
    Vec_IntClear( p->vClasses );
    Vec_IntPush( p->vClasses, 0 );
    Vec_IntPush( p->vClasses, 1 );
    // find beginning of each level
    pLevBegins = ABC_CALLOC( int, MaxLev+2 );
    pLevBegins[0] = 1;
    for ( i = 0; i <= MaxLev; i++ )
    {
        assert( pLevSizes[i] > 0 ); // we do not allow AIG with a const node and no PIs
        Vec_IntPush( p->vClasses, pLevBegins[i] );
        Vec_IntPush( p->vClasses, pLevSizes[i] );
        pLevBegins[i+1] = pLevBegins[i] + pLevSizes[i];
    }
    assert( pLevBegins[MaxLev+1] == p->nObjs );
    // put them into the structure
    for ( i = 1; i < p->nObjs; i++ )
        Gia_IsoSetItem( p, pLevBegins[p->pLevels[i]]++, i );
    ABC_FREE( pLevBegins );
    ABC_FREE( pLevSizes );
/*
    // print the results
    for ( i = 0; i < p->nObjs; i++ )
        printf( "%3d : (%d,%d)\n", i, Gia_IsoGetItem(p, i), Gia_IsoGetValue(p, i) );
    printf( "\n" );
*/
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_IsoAssignUnique( Gia_IsoMan_t * p )
{
    int i, iBegin, nSize;
    p->nSingles = 0;
    Vec_IntClear( p->vClasses2 );
    Vec_IntForEachEntryDouble( p->vClasses, iBegin, nSize, i )
    {
        if ( nSize == 1 )
        {
            assert( p->pUniques[Gia_IsoGetItem(p, iBegin)] == 0 );
            p->pUniques[Gia_IsoGetItem(p, iBegin)] = p->nUniques++;
            p->nSingles++;
        }
        else
        {
            Vec_IntPush( p->vClasses2, iBegin );
            Vec_IntPush( p->vClasses2, nSize );
        }
    }
    ABC_SWAP( Vec_Int_t *, p->vClasses, p->vClasses2 );
    p->nEntries -= p->nSingles;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_IsoSort( Gia_IsoMan_t * p )
{
    Gia_Obj_t * pObj, * pObj0;
    int i, k, fSameValue, iBegin, iBeginOld, nSize, nSizeNew;
    int fRefined = 0;
    abctime clk;

    // go through the equiv classes
    p->nSingles = 0;
    Vec_IntClear( p->vClasses2 );
    Vec_IntForEachEntryDouble( p->vClasses, iBegin, nSize, i )
    {
        assert( nSize > 1 );
        fSameValue = 1;
        pObj0 = Gia_ManObj( p->pGia, Gia_IsoGetItem(p,iBegin) );
        for ( k = 0; k < nSize; k++ )
        {
            pObj = Gia_ManObj( p->pGia, Gia_IsoGetItem(p,iBegin+k) );
            Gia_IsoSetValue( p, iBegin+k, pObj->Value );
            if ( pObj->Value != pObj0->Value )
                fSameValue = 0;
        }
        if ( fSameValue )
        {
            Vec_IntPush( p->vClasses2, iBegin );
            Vec_IntPush( p->vClasses2, nSize );
            continue;
        }
        fRefined = 1;
        // sort objects
        clk = Abc_Clock();
        Abc_QuickSort3( p->pStoreW + iBegin, nSize, 0 );
        p->timeSort += Abc_Clock() - clk;
        // divide into new classes
        iBeginOld = iBegin;
        pObj0 = Gia_ManObj( p->pGia, Gia_IsoGetItem(p,iBegin) );
        for ( k = 1; k < nSize; k++ )
        {
            pObj = Gia_ManObj( p->pGia, Gia_IsoGetItem(p,iBegin+k) );
            if ( pObj0->Value == pObj->Value )
                continue;
            nSizeNew = iBegin + k - iBeginOld;
            if ( nSizeNew == 1 )
            {
                assert( p->pUniques[Gia_IsoGetItem(p, iBeginOld)] == 0 );
                p->pUniques[Gia_IsoGetItem(p, iBeginOld)] = p->nUniques++;
                p->nSingles++;
            }
            else
            {
                Vec_IntPush( p->vClasses2, iBeginOld );
                Vec_IntPush( p->vClasses2, nSizeNew );
            }
            iBeginOld = iBegin + k;
            pObj0 = pObj;
        }
        // add the last one
        nSizeNew = iBegin + k - iBeginOld;
        if ( nSizeNew == 1 )
        {
            assert( p->pUniques[Gia_IsoGetItem(p, iBeginOld)] == 0 );
            p->pUniques[Gia_IsoGetItem(p, iBeginOld)] = p->nUniques++;
            p->nSingles++;
        }
        else
        {
            Vec_IntPush( p->vClasses2, iBeginOld );
            Vec_IntPush( p->vClasses2, nSizeNew );
        }
    }

    ABC_SWAP( Vec_Int_t *, p->vClasses, p->vClasses2 );
    p->nEntries -= p->nSingles;
    return fRefined;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Gia_IsoCollectCosClasses( Gia_IsoMan_t * p, int fVerbose )
{
    Vec_Ptr_t * vGroups;
    Vec_Int_t * vLevel;
    Gia_Obj_t * pObj;
    int i, k, iBegin, nSize;
    // add singletons
    vGroups = Vec_PtrAlloc( 1000 );
    Gia_ManForEachPo( p->pGia, pObj, i )
        if ( p->pUniques[Gia_ObjId(p->pGia, pObj)] > 0 )
        {
            vLevel = Vec_IntAlloc( 1 );
            Vec_IntPush( vLevel, i );
            Vec_PtrPush( vGroups, vLevel );
        }

    // add groups
    Vec_IntForEachEntryDouble( p->vClasses, iBegin, nSize, i )
    {
        for ( k = 0; k < nSize; k++ )
        {
            pObj = Gia_ManObj( p->pGia, Gia_IsoGetItem(p,iBegin+k) );
            if ( Gia_ObjIsPo(p->pGia, pObj) )
                break;
        }
        if ( k == nSize )
            continue;
        vLevel = Vec_IntAlloc( 8 );
        for ( k = 0; k < nSize; k++ )
        {
            pObj = Gia_ManObj( p->pGia, Gia_IsoGetItem(p,iBegin+k) );
            if ( Gia_ObjIsPo(p->pGia, pObj) )
                Vec_IntPush( vLevel, Gia_ObjCioId(pObj) );
        }
        Vec_PtrPush( vGroups, vLevel );
    }
    // canonicize order
    Vec_PtrForEachEntry( Vec_Int_t *, vGroups, vLevel, i )
        Vec_IntSort( vLevel, 0 );
    Vec_VecSortByFirstInt( (Vec_Vec_t *)vGroups, 0 );
//    Vec_VecFree( (Vec_Vec_t *)vGroups );
//    return NULL;
    return vGroups;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Gia_IsoUpdateValue( int Value, int fCompl )
{
    return (Value+1) * s_256Primes[Abc_Var2Lit(Value, fCompl) & ISO_MASK];
}
static inline unsigned Gia_IsoUpdate( Gia_IsoMan_t * p, int Iter, int iObj, int fCompl )
{
    if ( Iter == 0 )              return Gia_IsoUpdateValue( p->pLevels[iObj], fCompl );
    if ( p->pUniques[iObj] > 0 )  return Gia_IsoUpdateValue( p->pUniques[iObj], fCompl );
//    if ( p->pUniques[iObj] > 0 )  return Gia_IsoUpdateValue( 11, fCompl );
    return 0;
}
void Gia_IsoSimulate( Gia_IsoMan_t * p, int Iter )
{
    Gia_Obj_t * pObj, * pObjF;
    int i, iObj;
    // initialize constant, inputs, and flops in the first frame
    Gia_ManConst0(p->pGia)->Value += s_256Primes[ISO_MASK];
    Gia_ManForEachPi( p->pGia, pObj, i )
        pObj->Value += s_256Primes[ISO_MASK-1];
    if ( Iter == 0 )
        Gia_ManForEachRo( p->pGia, pObj, i )
            pObj->Value += s_256Primes[ISO_MASK-2];
    // simulate nodes
    Gia_ManForEachAnd( p->pGia, pObj, i )
    {
        pObj->Value += Gia_ObjFanin0(pObj)->Value + Gia_IsoUpdate(p, Iter, Gia_ObjFaninId0(pObj, i), Gia_ObjFaninC0(pObj));
        pObj->Value += Gia_ObjFanin1(pObj)->Value + Gia_IsoUpdate(p, Iter, Gia_ObjFaninId1(pObj, i), Gia_ObjFaninC1(pObj));
    }
    // simulate COs
    Gia_ManForEachCo( p->pGia, pObj, i )
    {
        iObj = Gia_ObjId(p->pGia, pObj);
        pObj->Value += Gia_ObjFanin0(pObj)->Value + Gia_IsoUpdate(p, Iter, Gia_ObjFaninId0(pObj, iObj), Gia_ObjFaninC0(pObj));
    }
    // transfer flop values
    Gia_ManForEachRiRo( p->pGia, pObjF, pObj, i )
        pObj->Value += pObjF->Value;
}
void Gia_IsoSimulateBack( Gia_IsoMan_t * p, int Iter )
{
    Gia_Obj_t * pObj, * pObjF;
    int i, iObj;
    // simulate COs
    Gia_ManForEachCo( p->pGia, pObj, i )
    {
        iObj = Gia_ObjId(p->pGia, pObj);
        Gia_ObjFanin0(pObj)->Value += pObj->Value + Gia_IsoUpdate(p, Iter, iObj, Gia_ObjFaninC0(pObj));
    }
    // simulate objects
    Gia_ManForEachAndReverse( p->pGia, pObj, i )
    {
        Gia_ObjFanin0(pObj)->Value += pObj->Value + Gia_IsoUpdate(p, Iter, i, Gia_ObjFaninC0(pObj));
        Gia_ObjFanin1(pObj)->Value += pObj->Value + Gia_IsoUpdate(p, Iter, i, Gia_ObjFaninC1(pObj));
    }
    // transfer flop values
    Gia_ManForEachRiRo( p->pGia, pObjF, pObj, i )
        pObjF->Value += pObj->Value;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_IsoAssignOneClass2( Gia_IsoMan_t * p )
{
    int i, iBegin = -1, nSize = -1;
    // find two variable class
    assert( Vec_IntSize(p->vClasses) > 0 );
    Vec_IntForEachEntryDouble( p->vClasses, iBegin, nSize, i )
    {
        if ( nSize == 2 )
            break;
    }
    assert( nSize > 1 );

    if ( nSize == 2 )
    {
        assert( p->pUniques[Gia_IsoGetItem(p, iBegin)] == 0 );
        p->pUniques[Gia_IsoGetItem(p, iBegin)] = p->nUniques++;
        p->nSingles++;
        p->nEntries--;

        assert( p->pUniques[Gia_IsoGetItem(p, iBegin+1)] == 0 );
        p->pUniques[Gia_IsoGetItem(p, iBegin+1)] = p->nUniques++;
        p->nSingles++;
        p->nEntries--;
    }
    else
    {
        assert( p->pUniques[Gia_IsoGetItem(p, iBegin)] == 0 );
        p->pUniques[Gia_IsoGetItem(p, iBegin)] = p->nUniques++;
        p->nSingles++;
        p->nEntries--;
    }

    for ( ; i < Vec_IntSize(p->vClasses) - 2; i += 2 )
    {
        p->vClasses->pArray[i+0] = p->vClasses->pArray[i+2];
        p->vClasses->pArray[i+1] = p->vClasses->pArray[i+3];
    }
    Vec_IntShrink( p->vClasses, Vec_IntSize(p->vClasses) - 2 );

    printf( "Broke ties in class %d of size %d at level %d.\n", i/2, nSize, p->pLevels[Gia_IsoGetItem(p, iBegin)] );
}

void Gia_IsoAssignOneClass3( Gia_IsoMan_t * p )
{
    int iBegin, nSize;
    // find the last class
    assert( Vec_IntSize(p->vClasses) > 0 );
    iBegin = Vec_IntEntry( p->vClasses, Vec_IntSize(p->vClasses) - 2 );
    nSize  = Vec_IntEntry( p->vClasses, Vec_IntSize(p->vClasses) - 1 );
    Vec_IntShrink( p->vClasses, Vec_IntSize(p->vClasses) - 2 );

    // assign the class
    assert( nSize > 1 );
    if ( nSize == 2 )
    {
        assert( p->pUniques[Gia_IsoGetItem(p, iBegin)] == 0 );
        p->pUniques[Gia_IsoGetItem(p, iBegin)] = p->nUniques++;
        p->nSingles++;
        p->nEntries--;

        assert( p->pUniques[Gia_IsoGetItem(p, iBegin+1)] == 0 );
        p->pUniques[Gia_IsoGetItem(p, iBegin+1)] = p->nUniques++;
        p->nSingles++;
        p->nEntries--;
    }
    else
    {
        assert( p->pUniques[Gia_IsoGetItem(p, iBegin)] == 0 );
        p->pUniques[Gia_IsoGetItem(p, iBegin)] = p->nUniques++;
        p->nSingles++;
        p->nEntries--;
    }
    printf( "Broke ties in last class of size %d at level %d.\n", nSize, p->pLevels[Gia_IsoGetItem(p, iBegin)] );
}

void Gia_IsoAssignOneClass( Gia_IsoMan_t * p, int fVerbose )
{
    int i, k, iBegin0, iBegin, nSize, Shrink;
    // find the classes with the highest level
    assert( Vec_IntSize(p->vClasses) > 0 );
    iBegin0 = Vec_IntEntry( p->vClasses, Vec_IntSize(p->vClasses) - 2 );
    for ( i = Vec_IntSize(p->vClasses) - 2; i >= 0; i -= 2 )
    {
        iBegin = Vec_IntEntry( p->vClasses, i );
        if ( p->pLevels[Gia_IsoGetItem(p, iBegin)] != p->pLevels[Gia_IsoGetItem(p, iBegin0)] )
            break;
    }
    i += 2;
    assert( i >= 0 );
    // assign all classes starting with this one
    for ( Shrink = i; i < Vec_IntSize(p->vClasses); i += 2 )
    {
        iBegin = Vec_IntEntry( p->vClasses, i );
        nSize  = Vec_IntEntry( p->vClasses, i + 1 );
        for ( k = 0; k < nSize; k++ )
        {
            assert( p->pUniques[Gia_IsoGetItem(p, iBegin+k)] == 0 );
            p->pUniques[Gia_IsoGetItem(p, iBegin+k)] = p->nUniques++;
//            Gia_ManObj(p->pGia, Gia_IsoGetItem(p, iBegin+k))->Value += s_256Primes[0]; ///  new addition!!!
            p->nSingles++;
            p->nEntries--;
        }
        if ( fVerbose )
            printf( "Broke ties in class of size %d at level %d.\n", nSize, p->pLevels[Gia_IsoGetItem(p, iBegin)] );
    }
    Vec_IntShrink( p->vClasses, Shrink );
}

/**Function*************************************************************

  Synopsis    [Report topmost equiv nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_IsoReportTopmost( Gia_IsoMan_t * p )
{
    Gia_Obj_t * pObj;
    int i, k, iBegin, nSize, Counter = 0;
    // go through equivalence classes
    Gia_ManIncrementTravId( p->pGia );
    Vec_IntForEachEntryDouble( p->vClasses, iBegin, nSize, i )
    {
//        printf( "%d(%d) ", nSize, p->pLevels[Gia_IsoGetItem(p, iBegin)] );
        for ( k = 0; k < nSize; k++ )
        {
            pObj = Gia_ManObj( p->pGia, Gia_IsoGetItem(p, iBegin+k) );
            if ( Gia_ObjIsAnd(pObj) )
            {
                Gia_ObjSetTravIdCurrent( p->pGia, Gia_ObjFanin0(pObj) );
                Gia_ObjSetTravIdCurrent( p->pGia, Gia_ObjFanin1(pObj) );
            }
            else if ( Gia_ObjIsRo(p->pGia, pObj) )
                Gia_ObjSetTravIdCurrent( p->pGia, Gia_ObjFanin0(Gia_ObjRoToRi(p->pGia, pObj)) );
        }
    }
//    printf( "\n" );

    // report non-labeled nodes
    Vec_IntForEachEntryDouble( p->vClasses, iBegin, nSize, i )
    {
        for ( k = 0; k < nSize; k++ )
        {
            pObj = Gia_ManObj( p->pGia, Gia_IsoGetItem(p, iBegin+k) );
            if ( !Gia_ObjIsTravIdCurrent(p->pGia, pObj) )
            {
                printf( "%5d : ", ++Counter );
                printf( "Obj %6d : Level = %4d.  iBegin = %4d.  Size = %4d.\n", 
                    Gia_ObjId(p->pGia, pObj), p->pLevels[Gia_ObjId(p->pGia, pObj)], iBegin, nSize );
                break;
            }
        }
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_IsoRecognizeMuxes( Gia_Man_t * pGia )
{
    Gia_Obj_t * pObj, * pObjC, * pObj1, * pObj0;
    int i;
    Gia_ManForEachAnd( pGia, pObj, i )
    {
        if ( !Gia_ObjIsMuxType(pObj) )
            continue;
        pObjC = Gia_ObjRecognizeMux( pObj, &pObj1, &pObj0 );
        if ( Gia_Regular(pObj0) == Gia_Regular(pObj1) )
        {
            // this is XOR
            Gia_Regular(pObj)->Value += s_256Primes[233];
            Gia_Regular(pObjC)->Value += s_256Primes[234];
            Gia_Regular(pObj0)->Value += s_256Primes[234];
        }
        else
        {
            // this is MUX
            Gia_Regular(pObj)->Value += s_256Primes[235];
            Gia_Regular(pObjC)->Value += s_256Primes[236];
            Gia_Regular(pObj0)->Value += s_256Primes[237];
            Gia_Regular(pObj1)->Value += s_256Primes[237];
        }
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Gia_IsoDeriveEquivPos( Gia_Man_t * pGia, int fForward, int fVerbose )
{
    int nIterMax = 10000;
    int nFixedPoint = 1;
    Gia_IsoMan_t * p;
    Vec_Ptr_t * vEquivs = NULL;
    int fRefined, fRefinedAll;
    int i, c;
    abctime clk = Abc_Clock(), clkTotal = Abc_Clock();
    assert( Gia_ManCiNum(pGia) > 0 );
    assert( Gia_ManPoNum(pGia) > 0 );

    Gia_ManCleanValue( pGia );
    p = Gia_IsoManStart( pGia );
    Gia_IsoPrepare( p );
    Gia_IsoAssignUnique( p );
    p->timeStart = Abc_Clock() - clk;
    if ( fVerbose )
        Gia_IsoPrint( p, 0, Abc_Clock() - clkTotal );

//    Gia_IsoRecognizeMuxes( pGia );

    i = 0;
    if ( fForward )
    {
        for ( c = 0; i < nIterMax && c < nFixedPoint+1; i++, c = fRefined ? 0 : c+1 )
        {
            clk = Abc_Clock();   Gia_IsoSimulate( p, i );      p->timeSim += Abc_Clock() - clk;
            clk = Abc_Clock();   fRefined = Gia_IsoSort( p );  p->timeRefine += Abc_Clock() - clk;
            if ( fVerbose )
                Gia_IsoPrint( p, i+1, Abc_Clock() - clkTotal );
        }
    }
    else
    {
        while ( Vec_IntSize(p->vClasses) > 0 )
        {
            for ( fRefinedAll = 1; i < nIterMax && fRefinedAll; )
            {
                fRefinedAll = 0;
                for ( c = 0; i < nIterMax && c < nFixedPoint+1; i++, c = fRefined ? 0 : c+1 )
                {
                    clk = Abc_Clock();   Gia_IsoSimulate( p, i );      p->timeSim += Abc_Clock() - clk;
                    clk = Abc_Clock();   fRefined = Gia_IsoSort( p );  p->timeRefine += Abc_Clock() - clk;
                    if ( fVerbose )
                        Gia_IsoPrint( p, i+1, Abc_Clock() - clkTotal );
                    fRefinedAll |= fRefined;
                }
                for ( c = 0; i < nIterMax && c < nFixedPoint+1; i++, c = fRefined ? 0 : c+1 )
                {
                    clk = Abc_Clock();   Gia_IsoSimulateBack( p, i );  p->timeSim += Abc_Clock() - clk;
                    clk = Abc_Clock();   fRefined = Gia_IsoSort( p );  p->timeRefine += Abc_Clock() - clk;
                    if ( fVerbose )
                        Gia_IsoPrint( p, i+1, Abc_Clock() - clkTotal );
                    fRefinedAll |= fRefined;
                }
            }
            if ( !fRefinedAll )
                break;
        }

//        Gia_IsoReportTopmost( p );

        while ( Vec_IntSize(p->vClasses) > 0 )
        {
            Gia_IsoAssignOneClass( p, fVerbose );
            for ( fRefinedAll = 1; i < nIterMax && fRefinedAll; )
            {
                fRefinedAll = 0;
                for ( c = 0; i < nIterMax && c < nFixedPoint; i++, c = fRefined ? 0 : c+1 )
                {
                    clk = Abc_Clock();   Gia_IsoSimulateBack( p, i );  p->timeSim += Abc_Clock() - clk;
                    clk = Abc_Clock();   fRefined = Gia_IsoSort( p );  p->timeRefine += Abc_Clock() - clk;
                    if ( fVerbose )
                        Gia_IsoPrint( p, i+1, Abc_Clock() - clkTotal );
                    fRefinedAll |= fRefined;
                }
                for ( c = 0; i < nIterMax && c < nFixedPoint; i++, c = fRefined ? 0 : c+1 )
                {
                    clk = Abc_Clock();   Gia_IsoSimulate( p, i );      p->timeSim += Abc_Clock() - clk;
                    clk = Abc_Clock();   fRefined = Gia_IsoSort( p );  p->timeRefine += Abc_Clock() - clk;
                    if ( fVerbose )
                        Gia_IsoPrint( p, i+1, Abc_Clock() - clkTotal );
                    fRefinedAll |= fRefined;
//                    if ( fRefined )
//                        printf( "Refinedment happened.\n" );
                }
            }
        }

        if ( fVerbose )
            Gia_IsoPrint( p, i+2, Abc_Clock() - clkTotal );
    }
//    Gia_IsoPrintClasses( p );
    if ( fVerbose )
    {
        p->timeTotal = Abc_Clock() - clkTotal;
        p->timeOther = p->timeTotal - p->timeStart - p->timeSim - p->timeRefine;
        ABC_PRTP( "Start    ", p->timeStart,              p->timeTotal );
        ABC_PRTP( "Simulate ", p->timeSim,                p->timeTotal );
        ABC_PRTP( "Refine   ", p->timeRefine-p->timeSort, p->timeTotal );
        ABC_PRTP( "Sort     ", p->timeSort,               p->timeTotal );
        ABC_PRTP( "Other    ", p->timeOther,              p->timeTotal );
        ABC_PRTP( "TOTAL    ", p->timeTotal,              p->timeTotal );
    }

    if ( Gia_ManPoNum(p->pGia) > 1 )
        vEquivs = Gia_IsoCollectCosClasses( p, fVerbose );
    Gia_IsoManTransferUnique( p );
    Gia_IsoManStop( p );

    return vEquivs;
}




/**Function*************************************************************

  Synopsis    [Finds canonical ordering of CIs/COs/nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ObjCompareByValue( Gia_Obj_t ** pp1, Gia_Obj_t ** pp2 )
{
    Gia_Obj_t * pObj1 = *pp1;
    Gia_Obj_t * pObj2 = *pp2;
//    assert( pObj1->Value != pObj2->Value );
    return (int)pObj1->Value - (int)pObj2->Value;
}
void Gia_ManFindCaninicalOrder_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vAnds )
{
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return;
    Gia_ObjSetTravIdCurrent(p, pObj);
    assert( Gia_ObjIsAnd(pObj) );
    if ( !Gia_ObjIsAnd(Gia_ObjFanin0(pObj)) || !Gia_ObjIsAnd(Gia_ObjFanin1(pObj)) )
    {
        Gia_ManFindCaninicalOrder_rec( p, Gia_ObjFanin0(pObj), vAnds );
        Gia_ManFindCaninicalOrder_rec( p, Gia_ObjFanin1(pObj), vAnds );
    }
    else
    {
        assert( Gia_ObjFanin0(pObj)->Value != Gia_ObjFanin1(pObj)->Value );
        if ( Gia_ObjFanin0(pObj)->Value < Gia_ObjFanin1(pObj)->Value )
        {
            Gia_ManFindCaninicalOrder_rec( p, Gia_ObjFanin0(pObj), vAnds );
            Gia_ManFindCaninicalOrder_rec( p, Gia_ObjFanin1(pObj), vAnds );
        }
        else
        {
            Gia_ManFindCaninicalOrder_rec( p, Gia_ObjFanin1(pObj), vAnds );
            Gia_ManFindCaninicalOrder_rec( p, Gia_ObjFanin0(pObj), vAnds );
        }
    }
    Vec_IntPush( vAnds, Gia_ObjId(p, pObj) );
}
void Gia_ManFindCaninicalOrder( Gia_Man_t * p, Vec_Int_t * vCis, Vec_Int_t * vAnds, Vec_Int_t * vCos, Vec_Int_t ** pvPiPerm )
{
    Vec_Ptr_t * vTemp;
    Gia_Obj_t * pObj;
    int i;

    vTemp = Vec_PtrAlloc( 1000 );
    Vec_IntClear( vCis );
    Vec_IntClear( vAnds );
    Vec_IntClear( vCos );

    // assign unique IDs to PIs
    Vec_PtrClear( vTemp );
    Gia_ManForEachPi( p, pObj, i )
        Vec_PtrPush( vTemp, pObj );
    Vec_PtrSort( vTemp, (int (*)(void))Gia_ObjCompareByValue );
    // create the result
    Vec_PtrForEachEntry( Gia_Obj_t *, vTemp, pObj, i )
        Vec_IntPush( vCis, Gia_ObjId(p, pObj) );
    // remember PI permutation
    if ( pvPiPerm ) 
    {
        *pvPiPerm = Vec_IntAlloc( Gia_ManPiNum(p) );
        Vec_PtrForEachEntry( Gia_Obj_t *, vTemp, pObj, i )
            Vec_IntPush( *pvPiPerm, Gia_ObjCioId(pObj) );
    }

    // assign unique IDs to POs
    if ( Gia_ManPoNum(p) == 1 )
        Vec_IntPush( vCos, Gia_ObjId(p, Gia_ManPo(p, 0)) );
    else
    {
        Vec_PtrClear( vTemp );
        Gia_ManForEachPo( p, pObj, i )
        {
            pObj->Value = Abc_Var2Lit( Gia_ObjFanin0(pObj)->Value, Gia_ObjFaninC0(pObj) );
            Vec_PtrPush( vTemp, pObj );
        }
        Vec_PtrSort( vTemp, (int (*)(void))Gia_ObjCompareByValue );
        Vec_PtrForEachEntry( Gia_Obj_t *, vTemp, pObj, i )
            Vec_IntPush( vCos, Gia_ObjId(p, pObj) );
    }

    // assign unique IDs to ROs
    Vec_PtrClear( vTemp );
    Gia_ManForEachRo( p, pObj, i )
        Vec_PtrPush( vTemp, pObj );
    Vec_PtrSort( vTemp, (int (*)(void))Gia_ObjCompareByValue );
    // create the result
    Vec_PtrForEachEntry( Gia_Obj_t *, vTemp, pObj, i )
    {
        Vec_IntPush( vCis, Gia_ObjId(p, pObj) );
        Vec_IntPush( vCos, Gia_ObjId(p, Gia_ObjRoToRi(p, pObj)) );
    }
    Vec_PtrFree( vTemp );

    // assign unique IDs to internal nodes
    Gia_ManIncrementTravId( p );
    Gia_ObjSetTravIdCurrent( p, Gia_ManConst0(p) );
    Gia_ManForEachObjVec( vCis, p, pObj, i )
        Gia_ObjSetTravIdCurrent( p, pObj );
    Gia_ManForEachObjVec( vCos, p, pObj, i )
        Gia_ManFindCaninicalOrder_rec( p, Gia_ObjFanin0(pObj), vAnds );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManIsoCanonicize( Gia_Man_t * p, int fVerbose )
{
    Gia_Man_t * pRes = NULL;
    Vec_Int_t * vCis, * vAnds, * vCos;
    Vec_Ptr_t * vEquiv;
    if ( Gia_ManCiNum(p) == 0 ) // const AIG
    {
        assert( Gia_ManPoNum(p) == 1 );
        assert( Gia_ManObjNum(p) == 2 );
        return Gia_ManDup(p);
    }
    // derive canonical values
    vEquiv = Gia_IsoDeriveEquivPos( p, 0, fVerbose );
    Vec_VecFreeP( (Vec_Vec_t **)&vEquiv );
    // find canonical order of CIs/COs/nodes
    // find canonical order
    vCis  = Vec_IntAlloc( Gia_ManCiNum(p) );
    vAnds = Vec_IntAlloc( Gia_ManAndNum(p) );
    vCos  = Vec_IntAlloc( Gia_ManCoNum(p) );
    Gia_ManFindCaninicalOrder( p, vCis, vAnds, vCos, NULL );
    // derive the new AIG
    pRes = Gia_ManDupFromVecs( p, vCis, vAnds, vCos, Gia_ManRegNum(p) );
    // cleanup
    Vec_IntFree( vCis );
    Vec_IntFree( vAnds );
    Vec_IntFree( vCos );
    return pRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Str_t * Gia_ManIsoFindString( Gia_Man_t * p, int iPo, int fVerbose, Vec_Int_t ** pvPiPerm )
{
    Gia_Man_t * pPart;
    Vec_Ptr_t * vEquiv;
    Vec_Int_t * vCis, * vAnds, * vCos;
    Vec_Str_t * vStr;
    // duplicate
    pPart = Gia_ManDupCones( p, &iPo, 1, 1 );
//Gia_ManPrint( pPart );
    assert( Gia_ManPoNum(pPart) == 1 );
    if ( Gia_ManCiNum(pPart) == 0 ) // const AIG
    {
        assert( Gia_ManPoNum(pPart) == 1 );
        assert( Gia_ManObjNum(pPart) == 2 );
        vStr = Gia_AigerWriteIntoMemoryStr( pPart );
        Gia_ManStop( pPart );
        if ( pvPiPerm )
            *pvPiPerm = Vec_IntAlloc( 0 );
        return vStr;
    }
    // derive canonical values
    vEquiv = Gia_IsoDeriveEquivPos( pPart, 0, fVerbose );
    Vec_VecFreeP( (Vec_Vec_t **)&vEquiv );
    // find canonical order
    vCis  = Vec_IntAlloc( Gia_ManCiNum(pPart) );
    vAnds = Vec_IntAlloc( Gia_ManAndNum(pPart) );
    vCos  = Vec_IntAlloc( Gia_ManCoNum(pPart) );
    Gia_ManFindCaninicalOrder( pPart, vCis, vAnds, vCos, pvPiPerm );
//printf( "Internal: " );
//Vec_IntPrint( vCis );
    // derive the AIGER string
    vStr = Gia_AigerWriteIntoMemoryStrPart( pPart, vCis, vAnds, vCos, Gia_ManRegNum(pPart) );
    // cleanup
    Vec_IntFree( vCis );
    Vec_IntFree( vAnds );
    Vec_IntFree( vCos );
    Gia_ManStop( pPart );
    return vStr;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Vec_IntCountNonTrivial( Vec_Ptr_t * vEquivs, int * pnUsed )
{
    Vec_Int_t * vClass;
    int i, nClasses = 0;
    *pnUsed = 0;
    Vec_PtrForEachEntry( Vec_Int_t *, vEquivs, vClass, i )
    {
        if ( Vec_IntSize(vClass) < 2 )
            continue;
        nClasses++;
        (*pnUsed) += Vec_IntSize(vClass);
    }
    return nClasses;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManIsoReduce( Gia_Man_t * pInit, Vec_Ptr_t ** pvPosEquivs, Vec_Ptr_t ** pvPiPerms, int fEstimate, int fDualOut, int fVerbose, int fVeryVerbose )
{ 
    Gia_Man_t * p, * pPart;
    Vec_Ptr_t * vEquivs, * vEquivs2, * vStrings;
    Vec_Int_t * vRemain, * vLevel, * vLevel2;
    Vec_Str_t * vStr, * vStr2;
    int i, k, s, sStart, iPo, Counter;
    int nClasses, nUsedPos;
    abctime clk = Abc_Clock();
    if ( pvPosEquivs )
        *pvPosEquivs = NULL;
    if ( pvPiPerms )
        *pvPiPerms = Vec_PtrStart( Gia_ManPoNum(pInit) );

    if ( fDualOut )
    {
        assert( (Gia_ManPoNum(pInit) & 1) == 0 );
        if ( Gia_ManPoNum(pInit) == 2 )
            return Gia_ManDup(pInit);
        p = Gia_ManTransformMiter( pInit );
        p = Gia_ManSeqStructSweep( pPart = p, 1, 1, 0 );
        Gia_ManStop( pPart );
    }
    else
    {
        if ( Gia_ManPoNum(pInit) == 1 )
            return Gia_ManDup(pInit);
        p = pInit;
    }

    // create preliminary equivalences
    vEquivs = Gia_IsoDeriveEquivPos( p, 1, fVeryVerbose );
    if ( vEquivs == NULL )
    {
        if ( fDualOut )
            Gia_ManStop( p );
        return NULL;
    }
    nClasses = Vec_IntCountNonTrivial( vEquivs, &nUsedPos );
    printf( "Reduced %d outputs to %d candidate   classes (%d outputs are in %d non-trivial classes).  ", 
        Gia_ManPoNum(p), Vec_PtrSize(vEquivs), nUsedPos, nClasses );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    if ( fEstimate )
    {
        Vec_VecFree( (Vec_Vec_t *)vEquivs );
        return Gia_ManDup(pInit);
    }

    // perform refinement of equivalence classes
    Counter = 0;
    vEquivs2 = Vec_PtrAlloc( 100 );
    Vec_PtrForEachEntry( Vec_Int_t *, vEquivs, vLevel, i )
    {
        if ( Vec_IntSize(vLevel) < 2 )
        {
            Vec_PtrPush( vEquivs2, Vec_IntDup(vLevel) );
            for ( k = 0; k < Vec_IntSize(vLevel); k++ )
                if ( ++Counter % 100 == 0 )
                    printf( "%6d finished...\r", Counter );
            continue;
        }

        if ( fVerbose )
        {
            iPo = Vec_IntEntry(vLevel, 0);
            printf( "%6d %6d %6d : ", i, Vec_IntSize(vLevel), iPo );
            pPart = Gia_ManDupCones( p, &iPo, 1, 1 );
            Gia_ManPrintStats(pPart, NULL);
            Gia_ManStop( pPart );
        }

        sStart = Vec_PtrSize( vEquivs2 ); 
        vStrings = Vec_PtrAlloc( 100 );
        Vec_IntForEachEntry( vLevel, iPo, k )
        {
            if ( ++Counter % 100 == 0 )
                printf( "%6d finished...\r", Counter );
            assert( pvPiPerms == NULL || Vec_PtrArray(*pvPiPerms)[iPo] == NULL );
            vStr = Gia_ManIsoFindString( p, iPo, 0, pvPiPerms ? (Vec_Int_t **)Vec_PtrArray(*pvPiPerms) + iPo : NULL );

//            printf( "Output %2d : ", iPo );
//            Vec_IntPrint( Vec_PtrArray(*pvPiPerms)[iPo] );

            // check if this string already exists
            Vec_PtrForEachEntry( Vec_Str_t *, vStrings, vStr2, s )
                if ( Vec_StrCompareVec(vStr, vStr2) == 0 )
                    break;
            if ( s == Vec_PtrSize(vStrings) )
            {
                Vec_PtrPush( vStrings, vStr );
                Vec_PtrPush( vEquivs2, Vec_IntAlloc(8) );
            }
            else
                Vec_StrFree( vStr );
            // add this entry to the corresponding level
            vLevel2 = (Vec_Int_t *)Vec_PtrEntry( vEquivs2, sStart + s );
            Vec_IntPush( vLevel2, iPo );
        }
//        if ( Vec_PtrSize(vEquivs2) - sStart > 1 )
//            printf( "Refined class %d into %d classes.\n", i, Vec_PtrSize(vEquivs2) - sStart );
        Vec_VecFree( (Vec_Vec_t *)vStrings );
    }
    assert( Counter == Gia_ManPoNum(p) );
    Vec_VecSortByFirstInt( (Vec_Vec_t *)vEquivs2, 0 );
    Vec_VecFree( (Vec_Vec_t *)vEquivs );
    vEquivs = vEquivs2;

    // collect the first ones
    vRemain = Vec_IntAlloc( 100 );
    Vec_PtrForEachEntry( Vec_Int_t *, vEquivs, vLevel, i )
        Vec_IntPush( vRemain, Vec_IntEntry(vLevel, 0) );

    if ( fDualOut )
    {
        Vec_Int_t * vTemp = Vec_IntAlloc( Vec_IntSize(vRemain) );
        int i, Entry;
        Vec_IntForEachEntry( vRemain, Entry, i )
        {
//            printf( "%d ", Entry );
            Vec_IntPush( vTemp, 2*Entry );
            Vec_IntPush( vTemp, 2*Entry+1 );
        }
//        printf( "\n" );
        Vec_IntFree( vRemain );
        vRemain = vTemp;
        Gia_ManStop( p );
        p = pInit;
    }


    // derive the resulting AIG
    pPart = Gia_ManDupCones( p, Vec_IntArray(vRemain), Vec_IntSize(vRemain), 0 );
    Vec_IntFree( vRemain );
    // report the results
    nClasses = Vec_IntCountNonTrivial( vEquivs, &nUsedPos );
    if ( !fDualOut )
        printf( "Reduced %d outputs to %d equivalence classes (%d outputs are in %d non-trivial classes).  ", 
            Gia_ManPoNum(p), Vec_PtrSize(vEquivs), nUsedPos, nClasses );
    else
        printf( "Reduced %d dual outputs to %d dual outputs.  ", Gia_ManPoNum(p)/2, Gia_ManPoNum(pPart)/2 );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    if ( fVerbose )
    {
        printf( "Nontrivial classes:\n" );
        Vec_VecPrintInt( (Vec_Vec_t *)vEquivs, 1 );
    }
    if ( pvPosEquivs )
        *pvPosEquivs = vEquivs;
    else
        Vec_VecFree( (Vec_Vec_t *)vEquivs );
//    Gia_ManStopP( &pPart );
    return pPart;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_IsoTestOld( Gia_Man_t * p, int fVerbose )
{
    Vec_Ptr_t * vEquivs;
    abctime clk = Abc_Clock(); 
    vEquivs = Gia_IsoDeriveEquivPos( p, 0, fVerbose );
    printf( "Reduced %d outputs to %d.  ", Gia_ManPoNum(p), vEquivs ? Vec_PtrSize(vEquivs) : 1 );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    if ( fVerbose && vEquivs && Gia_ManPoNum(p) != Vec_PtrSize(vEquivs) )
    {
        printf( "Nontrivial classes:\n" );
//        Vec_VecPrintInt( (Vec_Vec_t *)vEquivs, 1 );
    }
    Vec_VecFreeP( (Vec_Vec_t **)&vEquivs );
}

/**Function*************************************************************

  Synopsis    [Test remapping of CEXes for isomorphic POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_IsoTestGenPerm( int nPis )
{
    Vec_Int_t * vPerm;
    int i, * pArray;
    vPerm = Vec_IntStartNatural( nPis );
    pArray = Vec_IntArray( vPerm );
    for ( i = 0; i < nPis; i++ )
    {
        int iNew = rand() % nPis;
        ABC_SWAP( int, pArray[i], pArray[iNew] );
    }
    return vPerm;
}
void Gia_IsoTest( Gia_Man_t * p, Abc_Cex_t * pCex, int fVerbose )
{
    Abc_Cex_t * pCexNew;
    Vec_Int_t * vPiPerm;
    Vec_Ptr_t * vPosEquivs, * vPisPerm;
    Vec_Int_t * vPerm0, * vPerm1;
    Gia_Man_t * pPerm, * pDouble, * pAig;
    assert( Gia_ManPoNum(p) == 1 );
    assert( Gia_ManRegNum(p) > 0 );
    // generate random permutation of PIs
    vPiPerm = Gia_IsoTestGenPerm( Gia_ManPiNum(p) );
    printf( "Considering random permutation of the primary inputs of the AIG:\n" );
    Vec_IntPrint( vPiPerm );
    // create AIG with two primary outputs (original and permuted)
    pPerm = Gia_ManDupPerm( p, vPiPerm );
    pDouble = Gia_ManDupAppendNew( p, pPerm );
//Gia_AigerWrite( pDouble, "test.aig", 0, 0 );

    // analyze the two-output miter
    pAig = Gia_ManIsoReduce( pDouble, &vPosEquivs, &vPisPerm, 0, 0, 0, 0 );
    Vec_VecFree( (Vec_Vec_t *)vPosEquivs );

    // given CEX for output 0, derive CEX for output 1
    vPerm0 = (Vec_Int_t *)Vec_PtrEntry( vPisPerm, 0 );
    vPerm1 = (Vec_Int_t *)Vec_PtrEntry( vPisPerm, 1 );
    pCexNew = Abc_CexPermuteTwo( pCex, vPerm0, vPerm1 );
    Vec_VecFree( (Vec_Vec_t *)vPisPerm );

    // check that original CEX and the resulting CEX is valid
    if ( Gia_ManVerifyCex(p, pCex, 0) )
        printf( "CEX for the init AIG is valid.\n" );
    else
        printf( "CEX for the init AIG is not valid.\n" );
    if ( Gia_ManVerifyCex(pPerm, pCexNew, 0) )
        printf( "CEX for the perm AIG is valid.\n" );
    else
        printf( "CEX for the perm AIG is not valid.\n" );
    // delete
    Gia_ManStop( pAig );
    Gia_ManStop( pDouble );
    Gia_ManStop( pPerm );
    Vec_IntFree( vPiPerm );
    Abc_CexFree( pCexNew );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

