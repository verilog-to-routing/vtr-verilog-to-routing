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
 
#ifndef __VEC_FLT_H__
#define __VEC_FLT_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>

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
    p = ALLOC( Vec_Flt_t, 1 );
    if ( nCap > 0 && nCap < 16 )
        nCap = 16;
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ALLOC( float, p->nCap ) : NULL;
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

/**Function*************************************************************

  Synopsis    [Creates the vector from a float array of the given size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Flt_t * Vec_FltAllocArray( float * pArray, int nSize )
{
    Vec_Flt_t * p;
    p = ALLOC( Vec_Flt_t, 1 );
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
    p = ALLOC( Vec_Flt_t, 1 );
    p->nSize  = nSize;
    p->nCap   = nSize;
    p->pArray = ALLOC( float, nSize );
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
    p = ALLOC( Vec_Flt_t, 1 );
    p->nSize  = pVec->nSize;
    p->nCap   = pVec->nCap;
    p->pArray = p->nCap? ALLOC( float, p->nCap ) : NULL;
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
    p = ALLOC( Vec_Flt_t, 1 );
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
static inline void Vec_FltFree( Vec_Flt_t * p )
{
    FREE( p->pArray );
    FREE( p );
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
static inline float Vec_FltEntry( Vec_Flt_t * p, int i )
{
    assert( i >= 0 && i < p->nSize );
    return p->pArray[i];
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
    p->pArray = REALLOC( float, p->pArray, nCapMin ); 
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
static inline void Vec_FltFillExtra( Vec_Flt_t * p, int nSize, float Entry )
{
    int i;
    if ( p->nSize >= nSize )
        return;
    Vec_FltGrow( p, nSize );
    for ( i = p->nSize; i < nSize; i++ )
        p->pArray[i] = Entry;
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
static inline void Vec_FltPushMem( Extra_MmStep_t * pMemMan, Vec_Flt_t * p, float Entry )
{
    if ( p->nSize == p->nCap )
    {
        float * pArray;
        int i;

        if ( p->nSize == 0 )
            p->nCap = 1;
        pArray = (float *)Extra_MmStepEntryFetch( pMemMan, p->nCap * 8 );
//        pArray = ALLOC( float, p->nCap * 2 );
        if ( p->pArray )
        {
            for ( i = 0; i < p->nSize; i++ )
                pArray[i] = p->pArray[i];
            Extra_MmStepEntryRecycle( pMemMan, (char *)p->pArray, p->nCap * 4 );
//            free( p->pArray );
        }
        p->nCap *= 2;
        p->pArray = pArray;
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

  Synopsis    [Comparison procedure for two floats.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_FltSortCompare1( float * pp1, float * pp2 )
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
static inline int Vec_FltSortCompare2( float * pp1, float * pp2 )
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

#endif

