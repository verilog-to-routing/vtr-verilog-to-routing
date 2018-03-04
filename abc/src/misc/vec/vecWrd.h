/**CFile****************************************************************

  FileName    [vecWrd.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Resizable arrays.]

  Synopsis    [Resizable arrays of long unsigned integers.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: vecWrd.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__misc__vec__vecWrd_h
#define ABC__misc__vec__vecWrd_h


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

typedef struct Vec_Wrd_t_       Vec_Wrd_t;
struct Vec_Wrd_t_ 
{
    int              nCap;
    int              nSize;
    word *           pArray;
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

#define Vec_WrdForEachEntry( vVec, Entry, i )                                               \
    for ( i = 0; (i < Vec_WrdSize(vVec)) && (((Entry) = Vec_WrdEntry(vVec, i)), 1); i++ )
#define Vec_WrdForEachEntryStart( vVec, Entry, i, Start )                                   \
    for ( i = Start; (i < Vec_WrdSize(vVec)) && (((Entry) = Vec_WrdEntry(vVec, i)), 1); i++ )
#define Vec_WrdForEachEntryStop( vVec, Entry, i, Stop )                                   \
    for ( i = 0; (i < Stop) && (((Entry) = Vec_WrdEntry(vVec, i)), 1); i++ )
#define Vec_WrdForEachEntryStartStop( vVec, Entry, i, Start, Stop )                         \
    for ( i = Start; (i < Stop) && (((Entry) = Vec_WrdEntry(vVec, i)), 1); i++ )
#define Vec_WrdForEachEntryReverse( vVec, pEntry, i )                                       \
    for ( i = Vec_WrdSize(vVec) - 1; (i >= 0) && (((pEntry) = Vec_WrdEntry(vVec, i)), 1); i-- )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Wrd_t * Vec_WrdAlloc( int nCap )
{
    Vec_Wrd_t * p;
    p = ABC_ALLOC( Vec_Wrd_t, 1 );
    if ( nCap > 0 && nCap < 16 )
        nCap = 16;
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ABC_ALLOC( word, p->nCap ) : NULL;
    return p;
}
static inline Vec_Wrd_t * Vec_WrdAllocExact( int nCap )
{
    Vec_Wrd_t * p;
    assert( nCap >= 0 );
    p = ABC_ALLOC( Vec_Wrd_t, 1 );
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ABC_ALLOC( word, p->nCap ) : NULL;
    return p;
}

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given size and cleans it.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Wrd_t * Vec_WrdStart( int nSize )
{
    Vec_Wrd_t * p;
    p = Vec_WrdAlloc( nSize );
    p->nSize = nSize;
    memset( p->pArray, 0, sizeof(word) * nSize );
    return p;
}

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given size and cleans it.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Wrd_t * Vec_WrdStartFull( int nSize )
{
    Vec_Wrd_t * p;
    p = Vec_WrdAlloc( nSize );
    p->nSize = nSize;
    memset( p->pArray, 0xff, sizeof(word) * nSize );
    return p;
}

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given size and cleans it.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Wrd_t * Vec_WrdStartNatural( int nSize )
{
    Vec_Wrd_t * p;
    int i;
    p = Vec_WrdAlloc( nSize );
    p->nSize = nSize;
    for ( i = 0; i < nSize; i++ )
        p->pArray[i] = i;
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates the vector from an integer array of the given size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Wrd_t * Vec_WrdAllocArray( word * pArray, int nSize )
{
    Vec_Wrd_t * p;
    p = ABC_ALLOC( Vec_Wrd_t, 1 );
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
static inline Vec_Wrd_t * Vec_WrdAllocArrayCopy( word * pArray, int nSize )
{
    Vec_Wrd_t * p;
    p = ABC_ALLOC( Vec_Wrd_t, 1 );
    p->nSize  = nSize;
    p->nCap   = nSize;
    p->pArray = ABC_ALLOC( word, nSize );
    memcpy( p->pArray, pArray, sizeof(word) * nSize );
    return p;
}

/**Function*************************************************************

  Synopsis    [Duplicates the integer array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Wrd_t * Vec_WrdDup( Vec_Wrd_t * pVec )
{
    Vec_Wrd_t * p;
    p = ABC_ALLOC( Vec_Wrd_t, 1 );
    p->nSize  = pVec->nSize;
    p->nCap   = pVec->nSize;
    p->pArray = p->nCap? ABC_ALLOC( word, p->nCap ) : NULL;
    memcpy( p->pArray, pVec->pArray, sizeof(word) * pVec->nSize );
    return p;
}

/**Function*************************************************************

  Synopsis    [Transfers the array into another vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Wrd_t * Vec_WrdDupArray( Vec_Wrd_t * pVec )
{
    Vec_Wrd_t * p;
    p = ABC_ALLOC( Vec_Wrd_t, 1 );
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
static inline void Vec_WrdErase( Vec_Wrd_t * p )
{
    ABC_FREE( p->pArray );
    p->nSize = 0;
    p->nCap = 0;
}
static inline void Vec_WrdFree( Vec_Wrd_t * p )
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
static inline void Vec_WrdFreeP( Vec_Wrd_t ** p )
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
static inline word * Vec_WrdReleaseArray( Vec_Wrd_t * p )
{
    word * pArray = p->pArray;
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
static inline word * Vec_WrdArray( Vec_Wrd_t * p )
{
    return p->pArray;
}
static inline word * Vec_WrdLimit( Vec_Wrd_t * p )
{
    return p->pArray + p->nSize;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_WrdSize( Vec_Wrd_t * p )
{
    return p->nSize;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_WrdCap( Vec_Wrd_t * p )
{
    return p->nCap;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline double Vec_WrdMemory( Vec_Wrd_t * p )
{
    return !p ? 0.0 : 1.0 * sizeof(word) * p->nCap + sizeof(Vec_Wrd_t);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word Vec_WrdEntry( Vec_Wrd_t * p, int i )
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
static inline word * Vec_WrdEntryP( Vec_Wrd_t * p, int i )
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
static inline void Vec_WrdWriteEntry( Vec_Wrd_t * p, int i, word Entry )
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
static inline word Vec_WrdAddToEntry( Vec_Wrd_t * p, int i, word Addition )
{
    assert( i >= 0 && i < p->nSize );
    return p->pArray[i] += Addition;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word Vec_WrdEntryLast( Vec_Wrd_t * p )
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
static inline void Vec_WrdGrow( Vec_Wrd_t * p, int nCapMin )
{
    if ( p->nCap >= nCapMin )
        return;
    p->pArray = ABC_REALLOC( word, p->pArray, nCapMin ); 
    assert( p->pArray );
    p->nCap   = nCapMin;
}

/**Function*************************************************************

  Synopsis    [Fills the vector with given number of entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_WrdFill( Vec_Wrd_t * p, int nSize, word Fill )
{
    int i;
    Vec_WrdGrow( p, nSize );
    for ( i = 0; i < nSize; i++ )
        p->pArray[i] = Fill;
    p->nSize = nSize;
}

/**Function*************************************************************

  Synopsis    [Fills the vector with given number of entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_WrdFillExtra( Vec_Wrd_t * p, int nSize, word Fill )
{
    int i;
    if ( nSize <= p->nSize )
        return;
    if ( nSize > 2 * p->nCap )
        Vec_WrdGrow( p, nSize );
    else if ( nSize > p->nCap )
        Vec_WrdGrow( p, 2 * p->nCap );
    for ( i = p->nSize; i < nSize; i++ )
        p->pArray[i] = Fill;
    p->nSize = nSize;
}

/**Function*************************************************************

  Synopsis    [Returns the entry even if the place not exist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word Vec_WrdGetEntry( Vec_Wrd_t * p, int i )
{
    Vec_WrdFillExtra( p, i + 1, 0 );
    return Vec_WrdEntry( p, i );
}

/**Function*************************************************************

  Synopsis    [Returns the entry even if the place not exist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word * Vec_WrdGetEntryP( Vec_Wrd_t * p, int i )
{
    Vec_WrdFillExtra( p, i + 1, 0 );
    return Vec_WrdEntryP( p, i );
}

/**Function*************************************************************

  Synopsis    [Inserts the entry even if the place does not exist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_WrdSetEntry( Vec_Wrd_t * p, int i, word Entry )
{
    Vec_WrdFillExtra( p, i + 1, 0 );
    Vec_WrdWriteEntry( p, i, Entry );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_WrdShrink( Vec_Wrd_t * p, int nSizeNew )
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
static inline void Vec_WrdClear( Vec_Wrd_t * p )
{
    p->nSize = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_WrdPush( Vec_Wrd_t * p, word Entry )
{
    if ( p->nSize == p->nCap )
    {
        if ( p->nCap < 16 )
            Vec_WrdGrow( p, 16 );
        else
            Vec_WrdGrow( p, 2 * p->nCap );
    }
    p->pArray[p->nSize++] = Entry;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_WrdPushFirst( Vec_Wrd_t * p, word Entry )
{
    int i;
    if ( p->nSize == p->nCap )
    {
        if ( p->nCap < 16 )
            Vec_WrdGrow( p, 16 );
        else
            Vec_WrdGrow( p, 2 * p->nCap );
    }
    p->nSize++;
    for ( i = p->nSize - 1; i >= 1; i-- )
        p->pArray[i] = p->pArray[i-1];
    p->pArray[0] = Entry;
}

/**Function*************************************************************

  Synopsis    [Inserts the entry while preserving the increasing order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_WrdPushOrder( Vec_Wrd_t * p, word Entry )
{
    int i;
    if ( p->nSize == p->nCap )
    {
        if ( p->nCap < 16 )
            Vec_WrdGrow( p, 16 );
        else
            Vec_WrdGrow( p, 2 * p->nCap );
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
static inline int Vec_WrdPushUniqueOrder( Vec_Wrd_t * p, word Entry )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        if ( p->pArray[i] == Entry )
            return 1;
    Vec_WrdPushOrder( p, Entry );
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_WrdPushUnique( Vec_Wrd_t * p, word Entry )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        if ( p->pArray[i] == Entry )
            return 1;
    Vec_WrdPush( p, Entry );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns the pointer to the next nWords entries in the vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word * Vec_WrdFetch( Vec_Wrd_t * p, int nWords )
{
    if ( nWords == 0 )
        return NULL;
    assert( nWords > 0 );
    p->nSize += nWords;
    if ( p->nSize > p->nCap )
    {
//         Vec_WrdGrow( p, 2 * p->nSize );
        return NULL;
    }
    return p->pArray + p->nSize - nWords;
}

/**Function*************************************************************

  Synopsis    [Returns the last entry and removes it from the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word Vec_WrdPop( Vec_Wrd_t * p )
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
static inline int Vec_WrdFind( Vec_Wrd_t * p, word Entry )
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
static inline int Vec_WrdRemove( Vec_Wrd_t * p, word Entry )
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

  Synopsis    [Interts entry at the index iHere. Shifts other entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_WrdInsert( Vec_Wrd_t * p, int iHere, word Entry )
{
    int i;
    assert( iHere >= 0 && iHere < p->nSize );
    Vec_WrdPush( p, 0 );
    for ( i = p->nSize - 1; i > iHere; i-- )
        p->pArray[i] = p->pArray[i-1];
    p->pArray[i] = Entry;
}

/**Function*************************************************************

  Synopsis    [Find entry.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word Vec_WrdFindMax( Vec_Wrd_t * p )
{
    word Best;
    int i;
    if ( p->nSize == 0 )
        return 0;
    Best = p->pArray[0];
    for ( i = 1; i < p->nSize; i++ )
        if ( Best < p->pArray[i] )
            Best = p->pArray[i];
    return Best;
}

/**Function*************************************************************

  Synopsis    [Find entry.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word Vec_WrdFindMin( Vec_Wrd_t * p )
{
    word Best;
    int i;
    if ( p->nSize == 0 )
        return 0;
    Best = p->pArray[0];
    for ( i = 1; i < p->nSize; i++ )
        if ( Best > p->pArray[i] )
            Best = p->pArray[i];
    return Best;
}

/**Function*************************************************************

  Synopsis    [Reverses the order of entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_WrdReverseOrder( Vec_Wrd_t * p )
{
    word Temp;
    int i;
    for ( i = 0; i < p->nSize/2; i++ )
    {
        Temp = p->pArray[i];
        p->pArray[i] = p->pArray[p->nSize-1-i];
        p->pArray[p->nSize-1-i] = Temp;
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Wrd_t * Vec_WrdInvert( Vec_Wrd_t * p, word Fill ) 
{
    int i;
    word Entry;
    Vec_Wrd_t * vRes = Vec_WrdAlloc( 0 );
    Vec_WrdFill( vRes, Vec_WrdFindMax(p) + 1, Fill );
    Vec_WrdForEachEntry( p, Entry, i )
        if ( Entry != Fill )
            Vec_WrdWriteEntry( vRes, Entry, i );
    return vRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word Vec_WrdSum( Vec_Wrd_t * p ) 
{
    word Counter = 0;
    int i;
    for ( i = 0; i < p->nSize; i++ )
        Counter += p->pArray[i];
    return Counter;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_WrdCountZero( Vec_Wrd_t * p ) 
{
    int i, Counter = 0;
    for ( i = 0; i < p->nSize; i++ )
        Counter += (p->pArray[i] == 0);
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Checks if two vectors are equal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_WrdEqual( Vec_Wrd_t * p1, Vec_Wrd_t * p2 ) 
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

  Synopsis    [Counts the number of common entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_WrdCountCommon( Vec_Wrd_t * p1, Vec_Wrd_t * p2 ) 
{
    Vec_Wrd_t * vTemp;
    word Entry;
    int i, Counter = 0;
    if ( Vec_WrdSize(p1) < Vec_WrdSize(p2) )
        vTemp = p1, p1 = p2, p2 = vTemp;
    assert( Vec_WrdSize(p1) >= Vec_WrdSize(p2) );
    vTemp = Vec_WrdInvert( p2, -1 );
    Vec_WrdFillExtra( vTemp, Vec_WrdFindMax(p1) + 1, ~((word)0) );
    Vec_WrdForEachEntry( p1, Entry, i )
        if ( Vec_WrdEntry(vTemp, Entry) != ~((word)0) )
            Counter++;
    Vec_WrdFree( vTemp );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Comparison procedure for two integers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Vec_WrdSortCompare1( word * pp1, word * pp2 )
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
static int Vec_WrdSortCompare2( word * pp1, word * pp2 )
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
static inline void Vec_WrdSort( Vec_Wrd_t * p, int fReverse )
{
    if ( fReverse ) 
        qsort( (void *)p->pArray, p->nSize, sizeof(word), 
                (int (*)(const void *, const void *)) Vec_WrdSortCompare2 );
    else
        qsort( (void *)p->pArray, p->nSize, sizeof(word), 
                (int (*)(const void *, const void *)) Vec_WrdSortCompare1 );
}

/**Function*************************************************************

  Synopsis    [Leaves only unique entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_WrdUniqify( Vec_Wrd_t * p )
{
    int i, k;
    if ( p->nSize < 2 )
        return;
    Vec_WrdSort( p, 0 );
    for ( i = k = 1; i < p->nSize; i++ )
        if ( p->pArray[i] != p->pArray[i-1] )
            p->pArray[k++] = p->pArray[i];
    p->nSize = k;
}
static inline int Vec_WrdUniqueCount( Vec_Wrd_t * vData, int nWordSize, Vec_Int_t ** pvMap )
{
    int Result;
    Vec_Int_t * vDataInt = (Vec_Int_t *)vData;
    vDataInt->nSize *= 2;
    vDataInt->nCap *= 2;
    Result = Vec_IntUniqueCount( vDataInt, 2 * nWordSize, pvMap );
    vDataInt->nSize /= 2;
    vDataInt->nCap /= 2;
    return Result;
}
static inline Vec_Wrd_t * Vec_WrdUniqifyHash( Vec_Wrd_t * vData, int nWordSize )
{
    Vec_Int_t * vResInt;
    Vec_Int_t * vDataInt = (Vec_Int_t *)vData;
    vDataInt->nSize *= 2;
    vDataInt->nCap *= 2;
    vResInt = Vec_IntUniqifyHash( vDataInt, 2 * nWordSize );
    vDataInt->nSize /= 2;
    vDataInt->nCap /= 2;
    vResInt->nSize /= 2;
    vResInt->nCap /= 2;
    return (Vec_Wrd_t *)vResInt;
}

/**Function*************************************************************

  Synopsis    [Comparison procedure for two integers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Vec_WrdSortCompareUnsigned( word * pp1, word * pp2 )
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
static inline void Vec_WrdSortUnsigned( Vec_Wrd_t * p )
{
    qsort( (void *)p->pArray, p->nSize, sizeof(word), 
            (int (*)(const void *, const void *)) Vec_WrdSortCompareUnsigned );
}


/**Function*************************************************************

  Synopsis    [Appends the contents of the second vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_WrdAppend( Vec_Wrd_t * vVec1, Vec_Wrd_t * vVec2 )
{
    word Entry; int i;
    Vec_WrdForEachEntry( vVec2, Entry, i )
        Vec_WrdPush( vVec1, Entry );
}


ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

