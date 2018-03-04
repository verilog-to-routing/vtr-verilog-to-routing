/**CFile****************************************************************

  FileName    [vecHash.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Resizable arrays.]

  Synopsis    [Hashing integer pairs/triples into an integer.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: vecHash.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__misc__vec__vecHash_h
#define ABC__misc__vec__vecHash_h


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

typedef struct Hash_IntObj_t_ Hash_IntObj_t;
struct Hash_IntObj_t_
{
    int          iData0;
    int          iData1;
    int          iData2;
    int          iNext;
};

typedef struct Hash_IntMan_t_ Hash_IntMan_t;
struct Hash_IntMan_t_
{
    Vec_Int_t *  vTable;      // hash table
    Vec_Int_t *  vObjs;       // hash objects
    int          nRefs;       // reference counter for the manager
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

static inline Hash_IntObj_t * Hash_IntObj( Hash_IntMan_t * p, int i )           { return i ? (Hash_IntObj_t *)Vec_IntEntryP(p->vObjs, 4*i) : NULL;  }
static inline int             Hash_IntObjData0( Hash_IntMan_t * p, int i )      { return Hash_IntObj(p, i)->iData0;                                 }
static inline int             Hash_IntObjData1( Hash_IntMan_t * p, int i )      { return Hash_IntObj(p, i)->iData1;                                 }
static inline int             Hash_IntObjData2( Hash_IntMan_t * p, int i )      { return Hash_IntObj(p, i)->iData2;                                 }

static inline int             Hash_Int2ObjInc( Hash_IntMan_t * p, int i )             { return Hash_IntObj(p, i)->iData2++;                         }
static inline int             Hash_Int2ObjDec( Hash_IntMan_t * p, int i )             { return --Hash_IntObj(p, i)->iData2;                         }
static inline void            Hash_Int2ObjSetData2( Hash_IntMan_t * p, int i, int d ) { Hash_IntObj(p, i)->iData2 = d;                              }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Hashes pairs of intergers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Hash_IntMan_t * Hash_IntManStart( int nSize )
{
    Hash_IntMan_t * p;  nSize += 100;
    p = ABC_CALLOC( Hash_IntMan_t, 1 );
    p->vTable = Vec_IntStart( Abc_PrimeCudd(nSize) );
    p->vObjs  = Vec_IntAlloc( 4*nSize );
    Vec_IntFill( p->vObjs, 4, 0 );
    p->nRefs  = 1;
    return p;
}
static inline void Hash_IntManStop( Hash_IntMan_t * p )
{
    Vec_IntFree( p->vObjs );
    Vec_IntFree( p->vTable );
    ABC_FREE( p );
}
static inline Hash_IntMan_t * Hash_IntManRef( Hash_IntMan_t * p )
{
    p->nRefs++;
    return p;
}
static inline void Hash_IntManDeref( Hash_IntMan_t * p )
{
    if ( p == NULL )
        return;
    if ( --p->nRefs == 0 )
        Hash_IntManStop( p );
}
static inline int Hash_IntManEntryNum( Hash_IntMan_t * p )
{
    return Vec_IntSize(p->vObjs)/4 - 1;
}
static inline void Hash_IntManProfile( Hash_IntMan_t * p )
{
    Hash_IntObj_t * pObj;
    int i, Count, Entry;
    Vec_IntForEachEntry( p->vTable, Entry, i )
    {
        Count = 0;
        for ( pObj = Hash_IntObj( p, Entry ); pObj; pObj = Hash_IntObj( p, pObj->iNext ) )
            Count++;
        printf( "%d ", Count );
    }
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Hash_Int2ManHash( int iData0, int iData1, int nTableSize )
{
    return (4177 * (unsigned)iData0 + 7873 * (unsigned)iData1) % (unsigned)nTableSize;
}
static inline int * Hash_Int2ManLookup( Hash_IntMan_t * p, int iData0, int iData1 )
{
    Hash_IntObj_t * pObj;
    int * pPlace = Vec_IntEntryP( p->vTable, Hash_Int2ManHash(iData0, iData1, Vec_IntSize(p->vTable)) );
    for ( ; (pObj = Hash_IntObj(p, *pPlace)); pPlace = &pObj->iNext )
        if ( pObj->iData0 == iData0 && pObj->iData1 == iData1 )
            return pPlace;
    assert( *pPlace == 0 );
    return pPlace;
}
static inline int Hash_Int2ManInsert( Hash_IntMan_t * p, int iData0, int iData1, int iData2 )
{
    Hash_IntObj_t * pObj;
    int i, nObjs, * pPlace;
    nObjs = Vec_IntSize(p->vObjs)/4;
    if ( nObjs > Vec_IntSize(p->vTable) )
    {
//        printf( "Resizing...\n" );
        Vec_IntFill( p->vTable, Abc_PrimeCudd(2*Vec_IntSize(p->vTable)), 0 );
        for ( i = 1; i < nObjs; i++ )
        {
            pObj = Hash_IntObj( p, i );
            pObj->iNext = 0;
            pPlace = Hash_Int2ManLookup( p, pObj->iData0, pObj->iData1 );
            assert( *pPlace == 0 );
            *pPlace = i;
        }
    }
    pPlace = Hash_Int2ManLookup( p, iData0, iData1 );
    if ( *pPlace )
        return *pPlace;
    *pPlace = nObjs;
    Vec_IntPush( p->vObjs, iData0 );
    Vec_IntPush( p->vObjs, iData1 );
    Vec_IntPush( p->vObjs, iData2 );
    Vec_IntPush( p->vObjs, 0 );
    return nObjs;
}

/**Function*************************************************************

  Synopsis    [Hashes triples of intergers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Hsh_Int3ManHash( int iData0, int iData1, int iData2, int nTableSize )
{
    return (4177 * (unsigned)iData0 + 7873 * (unsigned)iData1 + 1699 * (unsigned)iData2) % (unsigned)nTableSize;
}
static inline int * Hsh_Int3ManLookup( Hash_IntMan_t * p, int iData0, int iData1, int iData2 )
{
    Hash_IntObj_t * pObj;
    int * pPlace = Vec_IntEntryP( p->vTable, Hsh_Int3ManHash(iData0, iData1, iData2, Vec_IntSize(p->vTable)) );
    for ( ; (pObj = Hash_IntObj(p, *pPlace)); pPlace = &pObj->iNext )
        if ( pObj->iData0 == iData0 && pObj->iData1 == iData1 && pObj->iData2 == iData2 )
            return pPlace;
    assert( *pPlace == 0 );
    return pPlace;
}
static inline int Hsh_Int3ManInsert( Hash_IntMan_t * p, int iData0, int iData1, int iData2 )
{
    Hash_IntObj_t * pObj;
    int i, nObjs, * pPlace;
    nObjs = Vec_IntSize(p->vObjs)/4;
    if ( nObjs > Vec_IntSize(p->vTable) )
    {
//        printf( "Resizing...\n" );
        Vec_IntFill( p->vTable, Abc_PrimeCudd(2*Vec_IntSize(p->vTable)), 0 );
        for ( i = 1; i < nObjs; i++ )
        {
            pObj = Hash_IntObj( p, i );
            pObj->iNext = 0;
            pPlace = Hsh_Int3ManLookup( p, pObj->iData0, pObj->iData1, pObj->iData2 );
            assert( *pPlace == 0 );
            *pPlace = i;
        }
    }
    pPlace = Hsh_Int3ManLookup( p, iData0, iData1, iData2 );
    if ( *pPlace )
        return *pPlace;
    *pPlace = nObjs;
    Vec_IntPush( p->vObjs, iData0 );
    Vec_IntPush( p->vObjs, iData1 );
    Vec_IntPush( p->vObjs, iData2 );
    Vec_IntPush( p->vObjs, 0 );
    return nObjs;
}

/**Function*************************************************************

  Synopsis    [Test procedure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Hash_IntManHashArrayTest()
{
    Hash_IntMan_t * p;
    int RetValue;

    p = Hash_IntManStart( 10 );

    RetValue = Hash_Int2ManInsert( p, 10, 11, 12 );
    assert( RetValue );

    RetValue = Hash_Int2ManInsert( p, 20, 21, 22 );
    assert( RetValue );

    RetValue = Hash_Int2ManInsert( p, 10, 11, 12 );
    assert( !RetValue );
 
    Hash_IntManStop( p );
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_END

#endif

