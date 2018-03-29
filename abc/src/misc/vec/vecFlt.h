/**CFile****************************************************************

  FileName    [vecFlt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Resizable arrays.]

  Synopsis    [Resizable arrays of floats.]

  Author      [Aaron P. Hurst]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: vecInt.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__misc__vec__vecFlt_h
#define ABC__misc__vec__vecFlt_h


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

typedef struct Vec_Flt_t_       Vec_Flt_t;
struct Vec_Flt_t_ 
{
    int              nCap;
    int              nSize;
    float *          pArray;
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

#define Vec_FltForEachEntry( vVec, Entry, i )                                               \
    for ( i = 0; (i < Vec_FltSize(vVec)) && (((Entry) = Vec_FltEntry(vVec, i)), 1); i++ )
#define Vec_FltForEachEntryStart( vVec, Entry, i, Start )                                   \
    for ( i = Start; (i < Vec_FltSize(vVec)) && (((Entry) = Vec_FltEntry(vVec, i)), 1); i++ )
#define Vec_FltForEachEntryStartStop( vVec, Entry, i, Start, Stop )                         \
    for ( i = Start; (i < Stop) && (((Entry) = Vec_FltEntry(vVec, i)), 1); i++ )
#define Vec_FltForEachEntryReverse( vVec, pEntry, i )                                       \
    for ( i = Vec_FltSize(vVec) - 1; (i >= 0) && (((pEntry) = Vec_FltEntry(vVec, i)), 1); i-- )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Flt_t * Vec_FltAlloc( int nCap )
{
    Vec_Flt_t * p;
    p = ABC_ALLOC( Vec_Flt_t, 1 );
    if ( nCap > 0 && nCap < 16 )
        nCap = 16;
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ABC_ALLOC( float, p->nCap ) : NULL;
    return p;
}
static inline Vec_Flt_t * Vec_FltAllocExact( int nCap )
{
    Vec_Flt_t * p;
    assert( nCap >= 0 );
    p = ABC_ALLOC( Vec_Flt_t, 1 );
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ABC_ALLOC( float, p->nCap ) : NULL;
    return p;
}

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given size and cleans it.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Flt_t * Vec_FltStart( int nSize )
{
    Vec_Flt_t * p;
    p = Vec_FltAlloc( nSize );
    p->nSize = nSize;
    memset( p->pArray, 0, sizeof(float) * nSize );
    return p;
}
static inline Vec_Flt_t * Vec_FltStartFull( int nSize )
{
    Vec_Flt_t * p;
    p = Vec_FltAlloc( nSize );
    p->nSize = nSize;
    memset( p->pArray, 0xFF, sizeof(float) * nSize );
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates the vector from a float array of the given size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Flt_t * Vec_FltAllocArray( float * pArray, int nSize )
{
    Vec_Flt_t * p;
    p = ABC_ALLOC( Vec_Flt_t, 1 );
    p->nSize  = nSize;
    p->nCap   = nSize;
    p->pArray = pArray;
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates the vector from a float array of the given size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Flt_t * Vec_FltAllocArrayCopy( float * pArray, int nSize )
{
    Vec_Flt_t * p;
    p = ABC_ALLOC( Vec_Flt_t, 1 );
    p->nSize  = nSize;
    p->nCap   = nSize;
    p->pArray = ABC_ALLOC( float, nSize );
    memcpy( p->pArray, pArray, sizeof(float) * nSize );
    return p;
}

/**Function*************************************************************

  Synopsis    [Duplicates the float array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Flt_t * Vec_FltDup( Vec_Flt_t * pVec )
{
    Vec_Flt_t * p;
    p = ABC_ALLOC( Vec_Flt_t, 1 );
    p->nSize  = pVec->nSize;
    p->nCap   = pVec->nCap;
    p->pArray = p->nCap? ABC_ALLOC( float, p->nCap ) : NULL;
    memcpy( p->pArray, pVec->pArray, sizeof(float) * pVec->nSize );
    return p;
}

/**Function*************************************************************

  Synopsis    [Transfers the array into another vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Flt_t * Vec_FltDupArray( Vec_Flt_t * pVec )
{
    Vec_Flt_t * p;
    p = ABC_ALLOC( Vec_Flt_t, 1 );
    p->nSize  = pVec->nSize;
    p->nCap   = pVec->nCap;
    p->pArray = pVec->pArray;
    pVec->nSize  = 0;
    pVec->nCap   = 0;
    pVec->pArray = NULL;
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_FltZero( Vec_Flt_t * p )
{
    p->pArray = NULL;
    p->nSize = 0;
    p->nCap = 0;
}
static inline void Vec_FltErase( Vec_Flt_t * p )
{
    ABC_FREE( p->pArray );
    p->nSize = 0;
    p->nCap = 0;
}
static inline void Vec_FltFree( Vec_Flt_t * p )
{
    ABC_FREE( p->pArray );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_FltFreeP( Vec_Flt_t ** p )
{
    if ( *p == NULL )
        return;
    ABC_FREE( (*p)->pArray );
    ABC_FREE( (*p) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline float * Vec_FltReleaseArray( Vec_Flt_t * p )
{
    float * pArray = p->pArray;
    p->nCap = 0;
    p->nSize = 0;
    p->pArray = NULL;
    return pArray;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline float * Vec_FltArray( Vec_Flt_t * p )
{
    return p->pArray;
}
static inline float ** Vec_FltArrayP( Vec_Flt_t * p )
{
    return &p->pArray;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_FltSize( Vec_Flt_t * p )
{
    return p->nSize;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_FltCap( Vec_Flt_t * p )
{
    return p->nCap;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline double Vec_FltMemory( Vec_Flt_t * p )
{
    return !p ? 0.0 : 1.0 * sizeof(float) * p->nCap + sizeof(Vec_Flt_t);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline float Vec_FltEntry( Vec_Flt_t * p, int i )
{
    assert( i >= 0 && i < p->nSize );
    return p->pArray[i];
}
static inline float * Vec_FltEntryP( Vec_Flt_t * p, int i )
{
    assert( i >= 0 && i < p->nSize );
    return p->pArray + i;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_FltWriteEntry( Vec_Flt_t * p, int i, float Entry )
{
    assert( i >= 0 && i < p->nSize );
    p->pArray[i] = Entry;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_FltAddToEntry( Vec_Flt_t * p, int i, float Addition )
{
    assert( i >= 0 && i < p->nSize );
    p->pArray[i] += Addition;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_FltUpdateEntry( Vec_Flt_t * p, int i, float Value )
{
    if ( Vec_FltEntry( p, i ) < Value )
        Vec_FltWriteEntry( p, i, Value );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline float Vec_FltEntryLast( Vec_Flt_t * p )
{
    return p->pArray[p->nSize-1];
}

/**Function*************************************************************

  Synopsis    [Resizes the vector to the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_FltGrow( Vec_Flt_t * p, int nCapMin )
{
    if ( p->nCap >= nCapMin )
        return;
    p->pArray = ABC_REALLOC( float, p->pArray, nCapMin ); 
    p->nCap   = nCapMin;
}

/**Function*************************************************************

  Synopsis    [Fills the vector with given number of entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_FltFill( Vec_Flt_t * p, int nSize, float Entry )
{
    int i;
    Vec_FltGrow( p, nSize );
    for ( i = 0; i < nSize; i++ )
        p->pArray[i] = Entry;
    p->nSize = nSize;
}

/**Function*************************************************************

  Synopsis    [Fills the vector with given number of entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_FltFillExtra( Vec_Flt_t * p, int nSize, float Fill )
{
    int i;
    if ( nSize <= p->nSize )
        return;
    if ( nSize > 2 * p->nCap )
        Vec_FltGrow( p, nSize );
    else if ( nSize > p->nCap )
        Vec_FltGrow( p, 2 * p->nCap );
    for ( i = p->nSize; i < nSize; i++ )
        p->pArray[i] = Fill;
    p->nSize = nSize;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_FltShrink( Vec_Flt_t * p, int nSizeNew )
{
    assert( p->nSize >= nSizeNew );
    p->nSize = nSizeNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_FltClear( Vec_Flt_t * p )
{
    p->nSize = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_FltPush( Vec_Flt_t * p, float Entry )
{
    if ( p->nSize == p->nCap )
    {
        if ( p->nCap < 16 )
            Vec_FltGrow( p, 16 );
        else
            Vec_FltGrow( p, 2 * p->nCap );
    }
    p->pArray[p->nSize++] = Entry;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_FltPushOrder( Vec_Flt_t * p, float Entry )
{
    int i;
    if ( p->nSize == p->nCap )
    {
        if ( p->nCap < 16 )
            Vec_FltGrow( p, 16 );
        else
            Vec_FltGrow( p, 2 * p->nCap );
    }
    p->nSize++;
    for ( i = p->nSize-2; i >= 0; i-- )
        if ( p->pArray[i] > Entry )
            p->pArray[i+1] = p->pArray[i];
        else
            break;
    p->pArray[i+1] = Entry;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_FltPushUnique( Vec_Flt_t * p, float Entry )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        if ( p->pArray[i] == Entry )
            return 1;
    Vec_FltPush( p, Entry );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns the last entry and removes it from the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline float Vec_FltPop( Vec_Flt_t * p )
{
    assert( p->nSize > 0 );
    return p->pArray[--p->nSize];
}

/**Function*************************************************************

  Synopsis    [Find entry.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_FltFind( Vec_Flt_t * p, float Entry )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        if ( p->pArray[i] == Entry )
            return i;
    return -1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_FltRemove( Vec_Flt_t * p, float Entry )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        if ( p->pArray[i] == Entry )
            break;
    if ( i == p->nSize )
        return 0;
    assert( i < p->nSize );
    for ( i++; i < p->nSize; i++ )
        p->pArray[i-1] = p->pArray[i];
    p->nSize--;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Find entry.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline float Vec_FltFindMax( Vec_Flt_t * p )
{
    int i;
    float Best;
    if ( p->nSize == 0 )
        return 0;
    Best = p->pArray[0];
    for ( i = 1; i < p->nSize; i++ )
        if ( Best < p->pArray[i] )
            Best = p->pArray[i];
    return Best;
}
static inline float Vec_FltFindMin( Vec_Flt_t * p )
{
    int i;
    float Best;
    if ( p->nSize == 0 )
        return 0;
    Best = p->pArray[0];
    for ( i = 1; i < p->nSize; i++ )
        if ( Best > p->pArray[i] )
            Best = p->pArray[i];
    return Best;
}

/**Function*************************************************************

  Synopsis    [Checks if two vectors are equal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_FltEqual( Vec_Flt_t * p1, Vec_Flt_t * p2 ) 
{
    int i;
    if ( p1->nSize != p2->nSize )
        return 0;
    for ( i = 0; i < p1->nSize; i++ )
        if ( p1->pArray[i] != p2->pArray[i] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_FltPrint( Vec_Flt_t * vVec )
{
    int i; float Entry;
    printf( "Vector has %d entries: {", Vec_FltSize(vVec) );
    Vec_FltForEachEntry( vVec, Entry, i )
        printf( " %f", Entry );
    printf( " }\n" );
}

/**Function*************************************************************

  Synopsis    [Comparison procedure for two floats.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Vec_FltSortCompare1( float * pp1, float * pp2 )
{
    // for some reason commenting out lines (as shown) led to crashing of the release version
    if ( *pp1 < *pp2 )
        return -1;
    if ( *pp1 > *pp2 ) //
        return 1;
    return 0; //
}

/**Function*************************************************************

  Synopsis    [Comparison procedure for two floats.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Vec_FltSortCompare2( float * pp1, float * pp2 )
{
    // for some reason commenting out lines (as shown) led to crashing of the release version
    if ( *pp1 > *pp2 )
        return -1;
    if ( *pp1 < *pp2 ) //
        return 1;
    return 0; //
}

/**Function*************************************************************

  Synopsis    [Sorting the entries by their value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_FltSort( Vec_Flt_t * p, int fReverse )
{
    if ( fReverse ) 
        qsort( (void *)p->pArray, p->nSize, sizeof(float), 
                (int (*)(const void *, const void *)) Vec_FltSortCompare2 );
    else
        qsort( (void *)p->pArray, p->nSize, sizeof(float), 
                (int (*)(const void *, const void *)) Vec_FltSortCompare1 );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_END

#endif

