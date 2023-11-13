/**CFile****************************************************************

  FileName    [vecHsh.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Resizable arrays.]

  Synopsis    [Hashing vector entries.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: vecHsh.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__misc__vec__vecHsh_h
#define ABC__misc__vec__vecHsh_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>

ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Hsh_Int1Man_t_ Hsh_Int1Man_t;
struct Hsh_Int1Man_t_
{
    Vec_Int_t *  vData;       // stored data
    Vec_Int_t *  vNext;       // next items
    Vec_Int_t *  vTable;      // hash table
};



typedef struct Hsh_IntObj_t_ Hsh_IntObj_t;
struct Hsh_IntObj_t_
{
    int          iData;
    int          iNext;
};

typedef union Hsh_IntObjWord_t_ Hsh_IntObjWord_t;
union Hsh_IntObjWord_t_
{
    Hsh_IntObj_t wObj;
    word         wWord;
};

typedef struct Hsh_IntMan_t_ Hsh_IntMan_t;
struct Hsh_IntMan_t_
{
    int          nSize;       // data size
    Vec_Int_t *  vData;       // data storage
    Vec_Int_t *  vTable;      // hash table
    Vec_Wrd_t *  vObjs;       // hash objects
};



typedef struct Hsh_VecObj_t_ Hsh_VecObj_t;
struct Hsh_VecObj_t_
{
    int          nSize;
    int          iNext;
    int          pArray[0];
};

typedef struct Hsh_VecMan_t_ Hsh_VecMan_t;
struct Hsh_VecMan_t_
{
    Vec_Int_t *  vTable;      // hash table
    Vec_Int_t *  vData;       // data storage
    Vec_Int_t *  vMap;        // mapping entries into data;
    Vec_Int_t    vTemp;       // temporary array
    Vec_Int_t    vTemp1;      // temporary array
    Vec_Int_t    vTemp2;      // temporary array
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

static inline unsigned *      Hsh_IntData( Hsh_IntMan_t * p, int iData )  { return (unsigned *)Vec_IntEntryP( p->vData, p->nSize * iData );             }
static inline Hsh_IntObj_t *  Hsh_IntObj( Hsh_IntMan_t * p, int iObj )    { return iObj == -1 ? NULL : (Hsh_IntObj_t *)Vec_WrdEntryP( p->vObjs, iObj ); }
static inline word            Hsh_IntWord( int iData, int iNext )         { Hsh_IntObjWord_t Obj = { {iData, iNext} }; return Obj.wWord;                }

static inline Hsh_VecObj_t *  Hsh_VecObj( Hsh_VecMan_t * p, int i )  { return i == -1 ? NULL : (Hsh_VecObj_t *)Vec_IntEntryP(p->vData, Vec_IntEntry(p->vMap, i));  }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Hashing data entries composed of nSize integers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Hsh_Int1Man_t * Hsh_Int1ManStart( int nEntries )
{
    Hsh_Int1Man_t * p;
    p = ABC_CALLOC( Hsh_Int1Man_t, 1 );
    p->vData  = Vec_IntAlloc( nEntries );
    p->vNext  = Vec_IntAlloc( nEntries );
    p->vTable = Vec_IntStartFull( Abc_PrimeCudd(nEntries) );
    return p;
}
static inline void Hsh_Int1ManStop( Hsh_Int1Man_t * p )
{
    Vec_IntFree( p->vData );
    Vec_IntFree( p->vNext );
    Vec_IntFree( p->vTable );
    ABC_FREE( p );
}
static inline int Hsh_Int1ManEntryNum( Hsh_Int1Man_t * p )
{
    return Vec_IntSize(p->vData);
}


/**Function*************************************************************

  Synopsis    [Hashing individual integers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
// https://en.wikipedia.org/wiki/Jenkins_hash_function
static inline int Hsh_Int1ManHash( int Data, int nTableSize ) 
{
    unsigned i, hash = 0;
    unsigned char * pDataC = (unsigned char *)&Data;
    for ( i = 0; i < 4; i++ )
    {
        hash += pDataC[i];
        hash += hash << 10;
        hash ^= hash >> 6;
    }
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;
    return (int)(hash % nTableSize);
}
static inline int * Hsh_Int1ManLookupInt( Hsh_Int1Man_t * p, int Data )
{
    int Item, * pPlace = Vec_IntEntryP( p->vTable, Hsh_Int1ManHash(Data, Vec_IntSize(p->vTable)) );
    for ( Item = *pPlace; Item >= 0; Item = Vec_IntEntry(p->vNext, Item) )
        if ( Vec_IntEntry(p->vData, Item) == (int)Data )
            return pPlace;
    assert( Item == -1 );
    return pPlace;
}
static inline int Hsh_Int1ManLookup( Hsh_Int1Man_t * p, int Data )
{
    return *Hsh_Int1ManLookupInt(p, Data);
}
static inline int Hsh_Int1ManAdd( Hsh_Int1Man_t * p, int Data )
{
    int i, Entry, * pPlace;
    if ( Vec_IntSize(p->vData) > Vec_IntSize(p->vTable) )
    {
        Vec_IntFill( p->vTable, Abc_PrimeCudd(2*Vec_IntSize(p->vTable)), -1 );
        Vec_IntForEachEntry( p->vData, Entry, i )
        {
            pPlace = Vec_IntEntryP( p->vTable, Hsh_Int1ManHash(Entry, Vec_IntSize(p->vTable)) );
            Vec_IntWriteEntry( p->vNext, i, *pPlace ); *pPlace = i;
        }
    }
    pPlace = Hsh_Int1ManLookupInt( p, Data );
    if ( *pPlace == -1 )
    {
        *pPlace = Vec_IntSize(p->vData);
        Vec_IntPush( p->vData, Data );
        Vec_IntPush( p->vNext, -1 );
    }
    return *pPlace;
}

/**Function*************************************************************

  Synopsis    [Test procedure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Hsh_Int1ManHashArrayTest()
{
    Hsh_Int1Man_t * p = Hsh_Int1ManStart( 10 );

    printf( "Entry = %d.   Hashed = %d.\n", 5, Hsh_Int1ManAdd(p, 5 ) );
    printf( "Entry = %d.   Hashed = %d.\n", 7, Hsh_Int1ManAdd(p, 7 ) );
    printf( "Entry = %d.   Hashed = %d.\n", 7, Hsh_Int1ManAdd(p, 7 ) );
    printf( "Entry = %d.   Hashed = %d.\n", 7, Hsh_Int1ManAdd(p, 7 ) );
    printf( "Entry = %d.   Hashed = %d.\n", 8, Hsh_Int1ManAdd(p, 8 ) );
    printf( "Entry = %d.   Hashed = %d.\n", 5, Hsh_Int1ManAdd(p, 5 ) );
    printf( "Entry = %d.   Hashed = %d.\n", 3, Hsh_Int1ManAdd(p, 3 ) );
    printf( "Entry = %d.   Hashed = %d.\n", 4, Hsh_Int1ManAdd(p, 4 ) );

    printf( "Size = %d.\n", Hsh_Int1ManEntryNum(p) );

    Hsh_Int1ManStop( p );
}


/**Function*************************************************************

  Synopsis    [Hashing data entries composed of nSize integers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Hsh_IntMan_t * Hsh_IntManStart( Vec_Int_t * vData, int nSize, int nEntries )
{
    Hsh_IntMan_t * p;
    p = ABC_CALLOC( Hsh_IntMan_t, 1 );
    p->nSize  = nSize;
    p->vData  = vData;
    p->vTable = Vec_IntStartFull( Abc_PrimeCudd(nEntries) );
    p->vObjs  = Vec_WrdAlloc( nEntries );
    return p;
}
static inline void Hsh_IntManStop( Hsh_IntMan_t * p )
{
    Vec_IntFree( p->vTable );
    Vec_WrdFree( p->vObjs );
    ABC_FREE( p );
}
static inline int Hsh_IntManEntryNum( Hsh_IntMan_t * p )
{
    return Vec_WrdSize(p->vObjs);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
// https://en.wikipedia.org/wiki/Jenkins_hash_function
static inline int Hsh_IntManHash( unsigned * pData, int nSize, int nTableSize ) 
{
    int i = 0; unsigned hash = 0;
    unsigned char * pDataC = (unsigned char *)pData;
    nSize <<= 2;
    while ( i != nSize ) 
    {
        hash += pDataC[i++];
        hash += hash << 10;
        hash ^= hash >> 6;
    }
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;
    return (int)(hash % nTableSize);
}
static inline int Hsh_IntManHash2( unsigned * pData, int nSize, int nTableSize )
{
    static int s_Primes[7] = { 4177, 5147, 5647, 6343, 7103, 7873, 8147 };
    unsigned char * pDataC = (unsigned char *)pData;
    int c, nChars = nSize * 4;
    unsigned Key = 0;
    for ( c = 0; c < nChars; c++ )
        Key += pDataC[c] * s_Primes[c % 7];
    return (int)(Key % nTableSize);
}
static inline int * Hsh_IntManLookup( Hsh_IntMan_t * p, unsigned * pData )
{
    Hsh_IntObj_t * pObj;
    int * pPlace = Vec_IntEntryP( p->vTable, Hsh_IntManHash(pData, p->nSize, Vec_IntSize(p->vTable)) );
    for ( ; (pObj = Hsh_IntObj(p, *pPlace)); pPlace = &pObj->iNext )
        if ( !memcmp( pData, Hsh_IntData(p, pObj->iData), sizeof(int) * (size_t)p->nSize ) )
            return pPlace;
    assert( *pPlace == -1 );
    return pPlace;
}
static inline int Hsh_IntManAdd( Hsh_IntMan_t * p, int iData )
{
    int i, * pPlace;
    if ( Vec_WrdSize(p->vObjs) > Vec_IntSize(p->vTable) )
    {
        Vec_IntFill( p->vTable, Abc_PrimeCudd(2*Vec_IntSize(p->vTable)), -1 );
        for ( i = 0; i < Vec_WrdSize(p->vObjs); i++ )
        {
            pPlace = Vec_IntEntryP( p->vTable, Hsh_IntManHash(Hsh_IntData(p, Hsh_IntObj(p, i)->iData), p->nSize, Vec_IntSize(p->vTable)) );
            Hsh_IntObj(p, i)->iNext = *pPlace;  *pPlace = i;
        }
    }
    pPlace = Hsh_IntManLookup( p, Hsh_IntData(p, iData) );
    if ( *pPlace == -1 )
    {
        *pPlace = Vec_WrdSize(p->vObjs);
        Vec_WrdPush( p->vObjs, Hsh_IntWord(iData, -1) );
        return Vec_WrdSize(p->vObjs) - 1;
    }
    return (word *)Hsh_IntObj(p, *pPlace) - Vec_WrdArray(p->vObjs);
}

/**Function*************************************************************

  Synopsis    [Hashes data by value.]

  Description [Array vData contains data entries, each of 'nSize' integers.
  The resulting array contains the indexes of unique data entries.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Int_t * Hsh_IntManHashArray( Vec_Int_t * vData, int nSize )
{
    Hsh_IntMan_t * p;
    Vec_Int_t * vRes = Vec_IntAlloc( 100 );
    int i, nEntries = Vec_IntSize(vData) / nSize;
    assert( Vec_IntSize(vData) % nSize == 0 );
    p = Hsh_IntManStart( vData, nSize, nEntries );
    for ( i = 0; i < nEntries; i++ )
        Vec_IntPush( vRes, Hsh_IntManAdd(p, i) );
    Hsh_IntManStop( p );
    return vRes;
}
static inline Vec_Int_t * Hsh_WrdManHashArray( Vec_Wrd_t * vDataW, int nSize )
{
    Hsh_IntMan_t * p;
    Vec_Int_t Data = { 2*Vec_WrdCap(vDataW), 2*Vec_WrdSize(vDataW), (int *)Vec_WrdArray(vDataW) };
    Vec_Int_t * vData = &Data;
    Vec_Int_t * vRes = Vec_IntAlloc( 100 );
    int i, nEntries = Vec_IntSize(vData) / (2*nSize);
    assert( Vec_IntSize(vData) % (2*nSize) == 0 );
    p = Hsh_IntManStart( vData, (2*nSize), nEntries );
    for ( i = 0; i < nEntries; i++ )
        Vec_IntPush( vRes, Hsh_IntManAdd(p, i) );
    Hsh_IntManStop( p );
    return vRes;
}
static inline Hsh_IntMan_t * Hsh_WrdManHashArrayStart( Vec_Wrd_t * vDataW, int nSize )
{
    Hsh_IntMan_t * p;
    int i, nEntries = Vec_WrdSize(vDataW) / nSize;
    Vec_Int_t * vData = Vec_IntAlloc( 2*Vec_WrdSize(vDataW) );
    memcpy( Vec_IntArray(vData), Vec_WrdArray(vDataW), sizeof(word)*(size_t)Vec_WrdSize(vDataW) );
    vData->nSize = 2*Vec_WrdSize(vDataW);
/*
    for ( i = 0; i < 30; i++ )
    {
        extern void Extra_PrintHex( FILE * pFile, unsigned * pTruth, int nVars );
        Extra_PrintHex( stdout, (unsigned *) Vec_WrdEntryP(vDataW, i), 6 );  printf( "  " );
        Kit_DsdPrintFromTruth( (unsigned *) Vec_WrdEntryP(vDataW, i), 6 );   printf( "\n" );
    }
*/
    assert( Vec_IntSize(vData) % (2*nSize) == 0 );
    p = Hsh_IntManStart( vData, (2*nSize), nEntries );
    for ( i = 0; i < nEntries; i++ )
        Hsh_IntManAdd( p, i );
    assert( Vec_WrdSize(p->vObjs) == nEntries );
    return p;
}

/**Function*************************************************************

  Synopsis    [Test procedure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Hsh_IntManHashArrayTest()
{
    Vec_Int_t * vData = Vec_IntAlloc( 10 );
    Vec_Int_t * vRes;
    Vec_IntPush( vData, 12 );
    Vec_IntPush( vData, 17 );
    Vec_IntPush( vData, 13 );
    Vec_IntPush( vData, 12 );
    Vec_IntPush( vData, 15 );
    Vec_IntPush( vData, 3 );
    Vec_IntPush( vData, 16 );
    Vec_IntPush( vData, 16 );
    Vec_IntPush( vData, 12 );
    Vec_IntPush( vData, 17 );
    Vec_IntPush( vData, 12 );
    Vec_IntPush( vData, 12 );

    vRes = Hsh_IntManHashArray( vData, 2 );

    Vec_IntPrint( vData );
    Vec_IntPrint( vRes );

    Vec_IntFree( vData );
    Vec_IntFree( vRes );
}





/**Function*************************************************************

  Synopsis    [Hashing integer arrays.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Hsh_VecMan_t * Hsh_VecManStart( int nEntries )
{
    Hsh_VecMan_t * p;
    p = ABC_CALLOC( Hsh_VecMan_t, 1 );
    p->vTable = Vec_IntStartFull( Abc_PrimeCudd(nEntries) );
    p->vData  = Vec_IntAlloc( nEntries * 4 );
    p->vMap   = Vec_IntAlloc( nEntries );
    return p;
}
static inline void Hsh_VecManStop( Hsh_VecMan_t * p )
{
    Vec_IntFree( p->vTable );
    Vec_IntFree( p->vData );
    Vec_IntFree( p->vMap );
    ABC_FREE( p );
}
static inline int * Hsh_VecReadArray( Hsh_VecMan_t * p, int i )
{
    return (int*)Hsh_VecObj(p, i) + 2;
}
static inline Vec_Int_t * Hsh_VecReadEntry( Hsh_VecMan_t * p, int i )
{
    Hsh_VecObj_t * pObj = Hsh_VecObj( p, i );
    p->vTemp.nSize = p->vTemp.nCap = pObj->nSize;
    p->vTemp.pArray = (int*)pObj + 2;
    return &p->vTemp;
}
static inline Vec_Int_t * Hsh_VecReadEntry1( Hsh_VecMan_t * p, int i )
{
    Hsh_VecObj_t * pObj = Hsh_VecObj( p, i );
    p->vTemp1.nSize = p->vTemp1.nCap = pObj->nSize;
    p->vTemp1.pArray = (int*)pObj + 2;
    return &p->vTemp1;
}
static inline Vec_Int_t * Hsh_VecReadEntry2( Hsh_VecMan_t * p, int i )
{
    Hsh_VecObj_t * pObj = Hsh_VecObj( p, i );
    p->vTemp2.nSize = p->vTemp2.nCap = pObj->nSize;
    p->vTemp2.pArray = (int*)pObj + 2;
    return &p->vTemp2;
}
static inline int Hsh_VecSize( Hsh_VecMan_t * p )
{
    return Vec_IntSize(p->vMap);
}
static inline double Hsh_VecManMemory( Hsh_VecMan_t * p )
{
    return !p ? 0.0 : Vec_IntMemory(p->vTable) + Vec_IntMemory(p->vData) + Vec_IntMemory(p->vMap);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Hsh_VecManHash( Vec_Int_t * vVec, int nTableSize )
{
    static unsigned s_Primes[7] = {4177, 5147, 5647, 6343, 7103, 7873, 8147};
    unsigned Key = 0;
    int i, Entry;
    Vec_IntForEachEntry( vVec, Entry, i )
        Key += (unsigned)Entry * s_Primes[i % 7];
    return (int)(Key % nTableSize);
}
static inline int Hsh_VecManAdd( Hsh_VecMan_t * p, Vec_Int_t * vVec )
{
    Hsh_VecObj_t * pObj;
    int i, Ent, * pPlace;
    if ( Vec_IntSize(p->vMap) > Vec_IntSize(p->vTable) )
    {
        Vec_IntFill( p->vTable, Abc_PrimeCudd(2*Vec_IntSize(p->vTable)), -1 );
        for ( i = 0; i < Vec_IntSize(p->vMap); i++ )
        {
            pPlace = Vec_IntEntryP( p->vTable, Hsh_VecManHash(Hsh_VecReadEntry(p, i), Vec_IntSize(p->vTable)) );
            Hsh_VecObj(p, i)->iNext = *pPlace; *pPlace = i;
        }
    }
    pPlace = Vec_IntEntryP( p->vTable, Hsh_VecManHash(vVec, Vec_IntSize(p->vTable)) );
    for ( ; (pObj = Hsh_VecObj(p, *pPlace)); pPlace = &pObj->iNext )
        if ( pObj->nSize == Vec_IntSize(vVec) && !memcmp( pObj->pArray, Vec_IntArray(vVec), sizeof(int) * (size_t)pObj->nSize ) )
            return *pPlace;
    *pPlace = Vec_IntSize(p->vMap);
    assert( Vec_IntSize(p->vData) % 2 == 0 );
    Vec_IntPush( p->vMap, Vec_IntSize(p->vData) );
    Vec_IntPush( p->vData, Vec_IntSize(vVec) );
    Vec_IntPush( p->vData, -1 );
    Vec_IntForEachEntry( vVec, Ent, i )
        Vec_IntPush( p->vData, Ent );
    if ( Vec_IntSize(vVec) & 1 )
        Vec_IntPush( p->vData, -1 );
    return Vec_IntSize(p->vMap) - 1;
}

/**Function*************************************************************

  Synopsis    [Test procedure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Hsh_VecManHashTest()
{
    Hsh_VecMan_t * p;
    Vec_Int_t * vTemp;
    Vec_Int_t * vRes = Vec_IntAlloc( 1000 );
    int i;
    
    p = Hsh_VecManStart( 5 );
    for ( i = 0; i < 20; i++ )
    {
        vTemp = Vec_IntStartNatural( i );
        Vec_IntPush( vRes, Hsh_VecManAdd( p, vTemp ) );
        Vec_IntFree( vTemp );
    }
    for ( ; i > 0; i-- )
    {
        vTemp = Vec_IntStartNatural( i );
        Vec_IntPush( vRes, Hsh_VecManAdd( p, vTemp ) );
        Vec_IntFree( vTemp );
    }
    Vec_IntPrint( vRes );
    Vec_IntFree( vRes );

    Hsh_VecManStop( p );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_END

#endif

