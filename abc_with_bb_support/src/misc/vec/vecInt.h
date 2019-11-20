/**CFile****************************************************************

  FileName    [vecInt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Resizable arrays.]

  Synopsis    [Resizable arrays of integers.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: vecInt.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef __VEC_INT_H__
#define __VEC_INT_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "extra.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Vec_Int_t_       Vec_Int_t;
struct Vec_Int_t_ 
{
    int              nCap;
    int              nSize;
    int *            pArray;
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

#define Vec_IntForEachEntry( vVec, Entry, i )                                               \
    for ( i = 0; (i < Vec_IntSize(vVec)) && (((Entry) = Vec_IntEntry(vVec, i)), 1); i++ )
#define Vec_IntForEachEntryStart( vVec, Entry, i, Start )                                   \
    for ( i = Start; (i < Vec_IntSize(vVec)) && (((Entry) = Vec_IntEntry(vVec, i)), 1); i++ )
#define Vec_IntForEachEntryStartStop( vVec, Entry, i, Start, Stop )                         \
    for ( i = Start; (i < Stop) && (((Entry) = Vec_IntEntry(vVec, i)), 1); i++ )
#define Vec_IntForEachEntryReverse( vVec, pEntry, i )                                       \
    for ( i = Vec_IntSize(vVec) - 1; (i >= 0) && (((pEntry) = Vec_IntEntry(vVec, i)), 1); i-- )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Int_t * Vec_IntAlloc( int nCap )
{
    Vec_Int_t * p;
    p = ALLOC( Vec_Int_t, 1 );
    if ( nCap > 0 && nCap < 16 )
        nCap = 16;
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ALLOC( int, p->nCap ) : NULL;
    return p;
}

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given size and cleans it.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Int_t * Vec_IntStart( int nSize )
{
    Vec_Int_t * p;
    p = Vec_IntAlloc( nSize );
    p->nSize = nSize;
    memset( p->pArray, 0, sizeof(int) * nSize );
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates the vector from an integer array of the given size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Int_t * Vec_IntAllocArray( int * pArray, int nSize )
{
    Vec_Int_t * p;
    p = ALLOC( Vec_Int_t, 1 );
    p->nSize  = nSize;
    p->nCap   = nSize;
    p->pArray = pArray;
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates the vector from an integer array of the given size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Int_t * Vec_IntAllocArrayCopy( int * pArray, int nSize )
{
    Vec_Int_t * p;
    p = ALLOC( Vec_Int_t, 1 );
    p->nSize  = nSize;
    p->nCap   = nSize;
    p->pArray = ALLOC( int, nSize );
    memcpy( p->pArray, pArray, sizeof(int) * nSize );
    return p;
}

/**Function*************************************************************

  Synopsis    [Duplicates the integer array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Int_t * Vec_IntDup( Vec_Int_t * pVec )
{
    Vec_Int_t * p;
    p = ALLOC( Vec_Int_t, 1 );
    p->nSize  = pVec->nSize;
    p->nCap   = pVec->nSize;
    p->pArray = p->nCap? ALLOC( int, p->nCap ) : NULL;
    memcpy( p->pArray, pVec->pArray, sizeof(int) * pVec->nSize );
    return p;
}

/**Function*************************************************************

  Synopsis    [Transfers the array into another vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Int_t * Vec_IntDupArray( Vec_Int_t * pVec )
{
    Vec_Int_t * p;
    p = ALLOC( Vec_Int_t, 1 );
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
static inline void Vec_IntFree( Vec_Int_t * p )
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
static inline int * Vec_IntReleaseArray( Vec_Int_t * p )
{
    int * pArray = p->pArray;
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
static inline int * Vec_IntArray( Vec_Int_t * p )
{
    return p->pArray;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_IntSize( Vec_Int_t * p )
{
    return p->nSize;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_IntEntry( Vec_Int_t * p, int i )
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
static inline void Vec_IntWriteEntry( Vec_Int_t * p, int i, int Entry )
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
static inline void Vec_IntAddToEntry( Vec_Int_t * p, int i, int Addition )
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
static inline int Vec_IntEntryLast( Vec_Int_t * p )
{
    assert( p->nSize > 0 );
    return p->pArray[p->nSize-1];
}

/**Function*************************************************************

  Synopsis    [Resizes the vector to the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntGrow( Vec_Int_t * p, int nCapMin )
{
    if ( p->nCap >= nCapMin )
        return;
    p->pArray = REALLOC( int, p->pArray, nCapMin ); 
    assert( p->pArray );
    p->nCap   = nCapMin;
}

/**Function*************************************************************

  Synopsis    [Fills the vector with given number of entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntFill( Vec_Int_t * p, int nSize, int Entry )
{
    int i;
    Vec_IntGrow( p, nSize );
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
static inline void Vec_IntFillExtra( Vec_Int_t * p, int nSize, int Entry )
{
    int i;
    if ( p->nSize >= nSize )
        return;
    Vec_IntGrow( p, nSize );
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
static inline void Vec_IntShrink( Vec_Int_t * p, int nSizeNew )
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
static inline void Vec_IntClear( Vec_Int_t * p )
{
    p->nSize = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntPush( Vec_Int_t * p, int Entry )
{
    if ( p->nSize == p->nCap )
    {
        if ( p->nCap < 16 )
            Vec_IntGrow( p, 16 );
        else
            Vec_IntGrow( p, 2 * p->nCap );
    }
    p->pArray[p->nSize++] = Entry;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntPushFirst( Vec_Int_t * p, int Entry )
{
    int i;
    if ( p->nSize == p->nCap )
    {
        if ( p->nCap < 16 )
            Vec_IntGrow( p, 16 );
        else
            Vec_IntGrow( p, 2 * p->nCap );
    }
    p->nSize++;
    for ( i = p->nSize - 1; i >= 1; i-- )
        p->pArray[i] = p->pArray[i-1];
    p->pArray[0] = Entry;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntPushMem( Extra_MmStep_t * pMemMan, Vec_Int_t * p, int Entry )
{
    if ( p->nSize == p->nCap )
    {
        int * pArray;
        int i;

        if ( p->nSize == 0 )
            p->nCap = 1;
        if ( pMemMan )
            pArray = (int *)Extra_MmStepEntryFetch( pMemMan, p->nCap * 8 );
        else
            pArray = ALLOC( int, p->nCap * 2 );
        if ( p->pArray )
        {
            for ( i = 0; i < p->nSize; i++ )
                pArray[i] = p->pArray[i];
            if ( pMemMan )
                Extra_MmStepEntryRecycle( pMemMan, (char *)p->pArray, p->nCap * 4 );
            else
                free( p->pArray );
        }
        p->nCap *= 2;
        p->pArray = pArray;
    }
    p->pArray[p->nSize++] = Entry;
}

/**Function*************************************************************

  Synopsis    [Inserts the entry while preserving the increasing order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntPushOrder( Vec_Int_t * p, int Entry )
{
    int i;
    if ( p->nSize == p->nCap )
    {
        if ( p->nCap < 16 )
            Vec_IntGrow( p, 16 );
        else
            Vec_IntGrow( p, 2 * p->nCap );
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

  Synopsis    [Inserts the entry while preserving the increasing order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_IntPushUniqueOrder( Vec_Int_t * p, int Entry )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        if ( p->pArray[i] == Entry )
            return 1;
    Vec_IntPushOrder( p, Entry );
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_IntPushUnique( Vec_Int_t * p, int Entry )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        if ( p->pArray[i] == Entry )
            return 1;
    Vec_IntPush( p, Entry );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns the pointer to the next nWords entries in the vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned * Vec_IntFetch( Vec_Int_t * p, int nWords )
{
    if ( nWords == 0 )
        return NULL;
    assert( nWords > 0 );
    p->nSize += nWords;
    if ( p->nSize > p->nCap )
    {
//         Vec_IntGrow( p, 2 * p->nSize );
        return NULL;
    }
    return ((unsigned *)p->pArray) + p->nSize - nWords;
}

/**Function*************************************************************

  Synopsis    [Returns the last entry and removes it from the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_IntPop( Vec_Int_t * p )
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
static inline int Vec_IntFind( Vec_Int_t * p, int Entry )
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
static inline int Vec_IntRemove( Vec_Int_t * p, int Entry )
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

  Synopsis    [Comparison procedure for two integers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_IntSortCompare1( int * pp1, int * pp2 )
{
    // for some reason commenting out lines (as shown) led to crashing of the release version
    if ( *pp1 < *pp2 )
        return -1;
    if ( *pp1 > *pp2 ) //
        return 1;
    return 0; //
}

/**Function*************************************************************

  Synopsis    [Comparison procedure for two integers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_IntSortCompare2( int * pp1, int * pp2 )
{
    // for some reason commenting out lines (as shown) led to crashing of the release version
    if ( *pp1 > *pp2 )
        return -1;
    if ( *pp1 < *pp2 ) //
        return 1;
    return 0; //
}

/**Function*************************************************************

  Synopsis    [Sorting the entries by their integer value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntSort( Vec_Int_t * p, int fReverse )
{
    if ( fReverse ) 
        qsort( (void *)p->pArray, p->nSize, sizeof(int), 
                (int (*)(const void *, const void *)) Vec_IntSortCompare2 );
    else
        qsort( (void *)p->pArray, p->nSize, sizeof(int), 
                (int (*)(const void *, const void *)) Vec_IntSortCompare1 );
}


/**Function*************************************************************

  Synopsis    [Comparison procedure for two integers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_IntSortCompareUnsigned( unsigned * pp1, unsigned * pp2 )
{
    if ( *pp1 < *pp2 )
        return -1;
    if ( *pp1 > *pp2 )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Sorting the entries by their integer value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntSortUnsigned( Vec_Int_t * p )
{
    qsort( (void *)p->pArray, p->nSize, sizeof(int), 
            (int (*)(const void *, const void *)) Vec_IntSortCompareUnsigned );
}

/**Function*************************************************************

  Synopsis    [Returns the number of common entries.]

  Description [Assumes that the vectors are sorted in the increasing order.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_IntTwoCountCommon( Vec_Int_t * vArr1, Vec_Int_t * vArr2 )
{
    int * pBeg1 = vArr1->pArray;
    int * pBeg2 = vArr2->pArray;
    int * pEnd1 = vArr1->pArray + vArr1->nSize;
    int * pEnd2 = vArr2->pArray + vArr2->nSize;
    int Counter = 0;
    while ( pBeg1 < pEnd1 && pBeg2 < pEnd2 )
    {
        if ( *pBeg1 == *pBeg2 )
            pBeg1++, pBeg2++, Counter++;
        else if ( *pBeg1 < *pBeg2 )
            pBeg1++;
        else 
            pBeg2++;
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Returns the result of merging the two vectors.]

  Description [Assumes that the vectors are sorted in the increasing order.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Int_t * Vec_IntTwoMerge( Vec_Int_t * vArr1, Vec_Int_t * vArr2 )
{
    Vec_Int_t * vArr = Vec_IntAlloc( vArr1->nSize + vArr2->nSize ); 
    int * pBeg  = vArr->pArray;
    int * pBeg1 = vArr1->pArray;
    int * pBeg2 = vArr2->pArray;
    int * pEnd1 = vArr1->pArray + vArr1->nSize;
    int * pEnd2 = vArr2->pArray + vArr2->nSize;
    while ( pBeg1 < pEnd1 && pBeg2 < pEnd2 )
    {
        if ( *pBeg1 == *pBeg2 )
            *pBeg++ = *pBeg1++, pBeg2++;
        else if ( *pBeg1 < *pBeg2 )
            *pBeg++ = *pBeg1++;
        else 
            *pBeg++ = *pBeg2++;
    }
    while ( pBeg1 < pEnd1 )
        *pBeg++ = *pBeg1++;
    while ( pBeg2 < pEnd2 )
        *pBeg++ = *pBeg2++;
    vArr->nSize = pBeg - vArr->pArray;
    assert( vArr->nSize <= vArr->nCap );
    assert( vArr->nSize >= vArr1->nSize );
    assert( vArr->nSize >= vArr2->nSize );
    return vArr;
}

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

