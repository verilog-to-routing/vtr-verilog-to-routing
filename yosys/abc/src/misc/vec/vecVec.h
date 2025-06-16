/**CFile****************************************************************

  FileName    [vecVec.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Resizable arrays.]

  Synopsis    [Resizable vector of resizable vectors.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: vecVec.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__misc__vec__vecVec_h
#define ABC__misc__vec__vecVec_h


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

typedef struct Vec_Vec_t_       Vec_Vec_t;
struct Vec_Vec_t_ 
{
    int              nCap;
    int              nSize;
    void **          pArray;
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

// iterators through levels 
#define Vec_VecForEachLevel( vGlob, vVec, i )                                                 \
    for ( i = 0; (i < Vec_VecSize(vGlob)) && (((vVec) = Vec_VecEntry(vGlob, i)), 1); i++ )
#define Vec_VecForEachLevelStart( vGlob, vVec, i, LevelStart )                                \
    for ( i = LevelStart; (i < Vec_VecSize(vGlob)) && (((vVec) = Vec_VecEntry(vGlob, i)), 1); i++ )
#define Vec_VecForEachLevelStop( vGlob, vVec, i, LevelStop )                                  \
    for ( i = 0; (i < LevelStop) && (((vVec) = Vec_VecEntry(vGlob, i)), 1); i++ )
#define Vec_VecForEachLevelStartStop( vGlob, vVec, i, LevelStart, LevelStop )                 \
    for ( i = LevelStart; (i < LevelStop) && (((vVec) = Vec_VecEntry(vGlob, i)), 1); i++ )
#define Vec_VecForEachLevelReverse( vGlob, vVec, i )                                          \
    for ( i = Vec_VecSize(vGlob)-1; (i >= 0) && (((vVec) = Vec_VecEntry(vGlob, i)), 1); i-- )
#define Vec_VecForEachLevelReverseStartStop( vGlob, vVec, i, LevelStart, LevelStop )          \
    for ( i = LevelStart-1; (i >= LevelStop) && (((vVec) = Vec_VecEntry(vGlob, i)), 1); i-- )
#define Vec_VecForEachLevelTwo( vGlob1, vGlob2, vVec1, vVec2, i )                                                 \
    for ( i = 0; (i < Vec_VecSize(vGlob1)) && (((vVec1) = Vec_VecEntry(vGlob1, i)), 1) && (((vVec2) = Vec_VecEntry(vGlob2, i)), 1); i++ )

// iterators through levels 
#define Vec_VecForEachLevelInt( vGlob, vVec, i )                                              \
    for ( i = 0; (i < Vec_VecSize(vGlob)) && (((vVec) = Vec_VecEntryInt(vGlob, i)), 1); i++ )
#define Vec_VecForEachLevelIntStart( vGlob, vVec, i, LevelStart )                             \
    for ( i = LevelStart; (i < Vec_VecSize(vGlob)) && (((vVec) = Vec_VecEntryInt(vGlob, i)), 1); i++ )
#define Vec_VecForEachLevelIntStop( vGlob, vVec, i, LevelStop )                               \
    for ( i = 0; (i < LevelStop) && (((vVec) = Vec_VecEntryInt(vGlob, i)), 1); i++ )
#define Vec_VecForEachLevelIntStartStop( vGlob, vVec, i, LevelStart, LevelStop )              \
    for ( i = LevelStart; (i < LevelStop) && (((vVec) = Vec_VecEntryInt(vGlob, i)), 1); i++ )
#define Vec_VecForEachLevelIntReverse( vGlob, vVec, i )                                       \
    for ( i = Vec_VecSize(vGlob)-1; (i >= 0) && (((vVec) = Vec_VecEntryInt(vGlob, i)), 1); i-- )
#define Vec_VecForEachLevelIntReverseStartStop( vGlob, vVec, i, LevelStart, LevelStop )       \
    for ( i = LevelStart-1; (i >= LevelStop) && (((vVec) = Vec_VecEntryInt(vGlob, i)), 1); i-- )
#define Vec_VecForEachLevelIntTwo( vGlob1, vGlob2, vVec1, vVec2, i )                          \
    for ( i = 0; (i < Vec_VecSize(vGlob1)) && (((vVec1) = Vec_VecEntryInt(vGlob1, i)), 1) && (((vVec2) = Vec_VecEntryInt(vGlob2, i)), 1); i++ )

// iteratores through entries
#define Vec_VecForEachEntry( Type, vGlob, pEntry, i, k )                                      \
    for ( i = 0; i < Vec_VecSize(vGlob); i++ )                                                \
        Vec_PtrForEachEntry( Type, Vec_VecEntry(vGlob, i), pEntry, k ) 
#define Vec_VecForEachEntryLevel( Type, vGlob, pEntry, i, Level )                             \
        Vec_PtrForEachEntry( Type, Vec_VecEntry(vGlob, Level), pEntry, i ) 
#define Vec_VecForEachEntryStart( Type, vGlob, pEntry, i, k, LevelStart )                     \
    for ( i = LevelStart; i < Vec_VecSize(vGlob); i++ )                                       \
        Vec_PtrForEachEntry( Type, Vec_VecEntry(vGlob, i), pEntry, k ) 
#define Vec_VecForEachEntryStartStop( Type, vGlob, pEntry, i, k, LevelStart, LevelStop )      \
    for ( i = LevelStart; i < LevelStop; i++ )                                                \
        Vec_PtrForEachEntry( Type, Vec_VecEntry(vGlob, i), pEntry, k ) 
#define Vec_VecForEachEntryReverse( Type, vGlob, pEntry, i, k )                               \
    for ( i = 0; i < Vec_VecSize(vGlob); i++ )                                                \
        Vec_PtrForEachEntryReverse( Type, Vec_VecEntry(vGlob, i), pEntry, k ) 
#define Vec_VecForEachEntryReverseReverse( Type, vGlob, pEntry, i, k )                        \
    for ( i = Vec_VecSize(vGlob) - 1; i >= 0; i-- )                                           \
        Vec_PtrForEachEntryReverse( Type, Vec_VecEntry(vGlob, i), pEntry, k ) 
#define Vec_VecForEachEntryReverseStart( Type, vGlob, pEntry, i, k, LevelStart )              \
    for ( i = LevelStart-1; i >= 0; i-- )                                                     \
        Vec_PtrForEachEntry( Type, Vec_VecEntry(vGlob, i), pEntry, k ) 

// iteratores through entries
#define Vec_VecForEachEntryInt( vGlob, Entry, i, k )                                          \
    for ( i = 0; i < Vec_VecSize(vGlob); i++ )                                                \
        Vec_IntForEachEntry( Vec_VecEntryInt(vGlob, i), Entry, k ) 
#define Vec_VecForEachEntryIntLevel( vGlob, Entry, i, Level )                                 \
        Vec_IntForEachEntry( Vec_VecEntryInt(vGlob, Level), Entry, i ) 
#define Vec_VecForEachEntryIntStart( vGlob, Entry, i, k, LevelStart )                         \
    for ( i = LevelStart; i < Vec_VecSize(vGlob); i++ )                                       \
        Vec_IntForEachEntry( Vec_VecEntryInt(vGlob, i), Entry, k ) 
#define Vec_VecForEachEntryIntStartStop( vGlob, Entry, i, k, LevelStart, LevelStop )          \
    for ( i = LevelStart; i < LevelStop; i++ )                                                \
        Vec_IntForEachEntry( Vec_VecEntryInt(vGlob, i), Entry, k ) 
#define Vec_VecForEachEntryIntReverse( vGlob, Entry, i, k )                                   \
    for ( i = 0; i < Vec_VecSize(vGlob); i++ )                                                \
        Vec_IntForEachEntryReverse( Vec_VecEntryInt(vGlob, i), Entry, k ) 
#define Vec_VecForEachEntryIntReverseReverse( vGlob, Entry, i, k )                            \
    for ( i = Vec_VecSize(vGlob) - 1; i >= 0; i-- )                                           \
        Vec_IntForEachEntryReverse( Vec_VecEntryInt(vGlob, i), Entry, k ) 
#define Vec_VecForEachEntryIntReverseStart( vGlob, Entry, i, k, LevelStart )                  \
    for ( i = LevelStart-1; i >= 0; i-- )                                                     \
        Vec_IntForEachEntry( Vec_VecEntryInt(vGlob, i), Entry, k ) 

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Vec_t * Vec_VecAlloc( int nCap )
{
    Vec_Vec_t * p;
    p = ABC_ALLOC( Vec_Vec_t, 1 );
    if ( nCap > 0 && nCap < 8 )
        nCap = 8;
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ABC_ALLOC( void *, p->nCap ) : NULL;
    return p;
}

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Vec_t * Vec_VecStart( int nSize )
{
    Vec_Vec_t * p;
    int i;
    p = Vec_VecAlloc( nSize );
    for ( i = 0; i < nSize; i++ )
        p->pArray[i] = Vec_PtrAlloc( 0 );
    p->nSize = nSize;
    return p;
}

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_VecExpand( Vec_Vec_t * p, int Level )
{
    int i;
    if ( p->nSize >= Level + 1 )
        return;
    Vec_PtrGrow( (Vec_Ptr_t *)p, Level + 1 );
    for ( i = p->nSize; i <= Level; i++ )
        p->pArray[i] = Vec_PtrAlloc( 0 );
    p->nSize = Level + 1;
}
static inline void Vec_VecExpandInt( Vec_Vec_t * p, int Level )
{
    int i;
    if ( p->nSize >= Level + 1 )
        return;
    Vec_IntGrow( (Vec_Int_t *)p, Level + 1 );
    for ( i = p->nSize; i <= Level; i++ )
        p->pArray[i] = Vec_PtrAlloc( 0 );
    p->nSize = Level + 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_VecSize( Vec_Vec_t * p )
{
    return p->nSize;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_VecCap( Vec_Vec_t * p )
{
    return p->nCap;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_VecLevelSize( Vec_Vec_t * p, int i )
{
    assert( i >= 0 && i < p->nSize );
    return Vec_PtrSize( (Vec_Ptr_t *)p->pArray[i] );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Ptr_t * Vec_VecEntry( Vec_Vec_t * p, int i )
{
    assert( i >= 0 && i < p->nSize );
    return (Vec_Ptr_t *)p->pArray[i];
}
static inline Vec_Int_t * Vec_VecEntryInt( Vec_Vec_t * p, int i )
{
    assert( i >= 0 && i < p->nSize );
    return (Vec_Int_t *)p->pArray[i];
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline double Vec_VecMemory( Vec_Vec_t * p )
{
    int i;
    double Mem;
    if ( p == NULL )  return 0.0;
    Mem = Vec_PtrMemory( (Vec_Ptr_t *)p );
    for ( i = 0; i < p->nSize; i++ )
        if ( Vec_VecEntry(p, i) )
            Mem += Vec_PtrMemory( Vec_VecEntry(p, i) );
    return Mem;
}
static inline double Vec_VecMemoryInt( Vec_Vec_t * p )
{
    int i;
    double Mem;
    if ( p == NULL )  return 0.0;
    Mem = Vec_PtrMemory( (Vec_Ptr_t *)p );
    for ( i = 0; i < p->nSize; i++ )
        if ( Vec_VecEntry(p, i) )
            Mem += Vec_IntMemory( Vec_VecEntryInt(p, i) );
    return Mem;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void * Vec_VecEntryEntry( Vec_Vec_t * p, int i, int k )
{
    return Vec_PtrEntry( Vec_VecEntry(p, i), k );
}
static inline int Vec_VecEntryEntryInt( Vec_Vec_t * p, int i, int k )
{
    return Vec_IntEntry( Vec_VecEntryInt(p, i), k );
}

/**Function*************************************************************

  Synopsis    [Frees the vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_VecFree( Vec_Vec_t * p )
{
    Vec_Ptr_t * vVec;
    int i;
    Vec_VecForEachLevel( p, vVec, i )
        if ( vVec ) Vec_PtrFree( vVec );
    Vec_PtrFree( (Vec_Ptr_t *)p );
}
static inline void Vec_VecErase( Vec_Vec_t * p )
{
    Vec_Ptr_t * vVec;
    int i;
    Vec_VecForEachLevel( p, vVec, i )
        if ( vVec ) Vec_PtrFree( vVec );
    Vec_PtrErase( (Vec_Ptr_t *)p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_VecFreeP( Vec_Vec_t ** p )
{
    if ( *p == NULL )
        return;
    Vec_VecFree( *p );
    *p = NULL;
}

/**Function*************************************************************

  Synopsis    [Frees the vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Vec_t * Vec_VecDup( Vec_Vec_t * p )
{
    Vec_Ptr_t * vNew, * vVec;
    int i;
    vNew = Vec_PtrAlloc( Vec_VecSize(p) );
    Vec_VecForEachLevel( p, vVec, i )
        Vec_PtrPush( vNew, Vec_PtrDup(vVec) );
    return (Vec_Vec_t *)vNew;
}
static inline Vec_Vec_t * Vec_VecDupInt( Vec_Vec_t * p )
{
    Vec_Ptr_t * vNew;
    Vec_Int_t * vVec;
    int i;
    vNew = Vec_PtrAlloc( Vec_VecSize(p) );
    Vec_VecForEachLevelInt( p, vVec, i )
        Vec_PtrPush( vNew, Vec_IntDup(vVec) );
    return (Vec_Vec_t *)vNew;
}

/**Function*************************************************************

  Synopsis    [Frees the vector of vectors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_VecSizeSize( Vec_Vec_t * p )
{
    Vec_Ptr_t * vVec;
    int i, Counter = 0;
    Vec_VecForEachLevel( p, vVec, i )
        Counter += vVec->nSize;
    return Counter;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_VecClear( Vec_Vec_t * p )
{
    Vec_Ptr_t * vVec;
    int i;
    Vec_VecForEachLevel( p, vVec, i )
        Vec_PtrClear( vVec );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_VecPush( Vec_Vec_t * p, int Level, void * Entry )
{
    if ( p->nSize < Level + 1 )
    {
        int i;
        Vec_PtrGrow( (Vec_Ptr_t *)p, Level + 1 );
        for ( i = p->nSize; i < Level + 1; i++ )
            p->pArray[i] = Vec_PtrAlloc( 0 );
        p->nSize = Level + 1;
    }
    Vec_PtrPush( Vec_VecEntry(p, Level), Entry );
}
static inline void Vec_VecPushInt( Vec_Vec_t * p, int Level, int Entry )
{
    if ( p->nSize < Level + 1 )
    {
        int i;
        Vec_PtrGrow( (Vec_Ptr_t *)p, Level + 1 );
        for ( i = p->nSize; i < Level + 1; i++ )
            p->pArray[i] = Vec_IntAlloc( 0 );
        p->nSize = Level + 1;
    }
    Vec_IntPush( Vec_VecEntryInt(p, Level), Entry );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_VecPushUnique( Vec_Vec_t * p, int Level, void * Entry )
{
    if ( p->nSize < Level + 1 )
        Vec_VecPush( p, Level, Entry );
    else
        Vec_PtrPushUnique( Vec_VecEntry(p, Level), Entry );
}
static inline void Vec_VecPushUniqueInt( Vec_Vec_t * p, int Level, int Entry )
{
    if ( p->nSize < Level + 1 )
        Vec_VecPushInt( p, Level, Entry );
    else
        Vec_IntPushUnique( Vec_VecEntryInt(p, Level), Entry );
}

/**Function*************************************************************

  Synopsis    [Comparison procedure for two arrays.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Vec_VecSortCompare1( Vec_Ptr_t ** pp1, Vec_Ptr_t ** pp2 )
{
    if ( Vec_PtrSize(*pp1) < Vec_PtrSize(*pp2) )
        return -1;
    if ( Vec_PtrSize(*pp1) > Vec_PtrSize(*pp2) ) 
        return 1;
    return 0; 
}
static int Vec_VecSortCompare2( Vec_Ptr_t ** pp1, Vec_Ptr_t ** pp2 )
{
    if ( Vec_PtrSize(*pp1) > Vec_PtrSize(*pp2) )
        return -1;
    if ( Vec_PtrSize(*pp1) < Vec_PtrSize(*pp2) ) 
        return 1;
    return 0; 
}

/**Function*************************************************************

  Synopsis    [Sorting the entries by their integer value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_VecSort( Vec_Vec_t * p, int fReverse )
{
    if ( fReverse ) 
        qsort( (void *)p->pArray, (size_t)p->nSize, sizeof(void *), 
                (int (*)(const void *, const void *)) Vec_VecSortCompare2 );
    else
        qsort( (void *)p->pArray, (size_t)p->nSize, sizeof(void *), 
                (int (*)(const void *, const void *)) Vec_VecSortCompare1 );
}

/**Function*************************************************************

  Synopsis    [Comparison procedure for two integers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Vec_VecSortCompare3( Vec_Int_t ** pp1, Vec_Int_t ** pp2 )
{
    if ( Vec_IntEntry(*pp1,0) < Vec_IntEntry(*pp2,0) )
        return -1;
    if ( Vec_IntEntry(*pp1,0) > Vec_IntEntry(*pp2,0) ) 
        return 1;
    return 0; 
}
static int Vec_VecSortCompare4( Vec_Int_t ** pp1, Vec_Int_t ** pp2 )
{
    if ( Vec_IntEntry(*pp1,0) > Vec_IntEntry(*pp2,0) )
        return -1;
    if ( Vec_IntEntry(*pp1,0) < Vec_IntEntry(*pp2,0) ) 
        return 1;
    return 0; 
}

/**Function*************************************************************

  Synopsis    [Sorting the entries by their integer value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_VecSortByFirstInt( Vec_Vec_t * p, int fReverse )
{
    if ( fReverse ) 
        qsort( (void *)p->pArray, (size_t)p->nSize, sizeof(void *), 
                (int (*)(const void *, const void *)) Vec_VecSortCompare4 );
    else
        qsort( (void *)p->pArray, (size_t)p->nSize, sizeof(void *), 
                (int (*)(const void *, const void *)) Vec_VecSortCompare3 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_VecPrintInt( Vec_Vec_t * p, int fSkipSingles )
{
    int i, k, Entry;
    Vec_VecForEachEntryInt( p, Entry, i, k )
    {
        if ( fSkipSingles && Vec_VecLevelSize(p, i) == 1 )
            break;
        if ( k == 0 )
            printf( " %4d : {", i );
        printf( " %d", Entry );
        if ( k == Vec_VecLevelSize(p, i) - 1 )
            printf( " }\n" );
    }
}

ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

