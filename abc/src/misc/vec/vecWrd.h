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
#define Vec_WrdForEachEntryDouble( vVec, Entry1, Entry2, i )                                \
    for ( i = 0; (i+1 < Vec_WrdSize(vVec)) && (((Entry1) = Vec_WrdEntry(vVec, i)), 1) && (((Entry2) = Vec_WrdEntry(vVec, i+1)), 1); i += 2 )

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
    memset( p->pArray, 0, sizeof(word) * (size_t)nSize );
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
    memset( p->pArray, 0xff, sizeof(word) * (size_t)nSize );
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
static inline Vec_Wrd_t * Vec_WrdStartRandom( int nSize )
{
    Vec_Wrd_t * vSims = Vec_WrdStart( nSize ); int i;
    for ( i = 0; i < nSize; i++ )
        vSims->pArray[i] = Abc_RandomW(0);
    return vSims;
}
static inline Vec_Wrd_t * Vec_WrdStartTruthTables( int nVars )
{
    Vec_Wrd_t * p;
    unsigned Masks[5] = { 0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000 };
    int i, k, nWords;
    nWords = nVars <= 6 ? 1 : (1 << (nVars - 6));
    p = Vec_WrdStart( nWords * nVars );
    for ( i = 0; i < nVars; i++ )
    {
        unsigned * pTruth = (unsigned *)(p->pArray + nWords * i);
        if ( i < 5 )
        {
            for ( k = 0; k < 2*nWords; k++ )
                pTruth[k] = Masks[i];
        }
        else
        {
            for ( k = 0; k < 2*nWords; k++ )
                if ( k & (1 << (i-5)) )
                    pTruth[k] = ~(unsigned)0;
                else
                    pTruth[k] = 0;
        }
    }
    return p;
}
static inline int Vec_WrdShiftOne( Vec_Wrd_t * p, int nWords )
{
    int i, nObjs = p->nSize/nWords;
    assert( nObjs * nWords == p->nSize );
    for ( i = 0; i < nObjs; i++ )
        p->pArray[i*nWords] <<= 1;
    return nObjs;
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
    memcpy( p->pArray, pArray, sizeof(word) * (size_t)nSize );
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
    memcpy( p->pArray, pVec->pArray, sizeof(word) * (size_t)pVec->nSize );
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
static inline int Vec_WrdChangeSize( Vec_Wrd_t * p, int Shift )
{
    return p->nSize += Shift;
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
    return !p ? 0.0 : 1.0 * sizeof(word) * (size_t)p->nCap + sizeof(Vec_Wrd_t);
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
static inline void Vec_WrdPushTwo( Vec_Wrd_t * p, word Entry1, word Entry2 )
{
    Vec_WrdPush( p, Entry1 );
    Vec_WrdPush( p, Entry2 );
}
static inline void Vec_WrdPushThree( Vec_Wrd_t * p, word Entry1, word Entry2, word Entry3 )
{
    Vec_WrdPush( p, Entry1 );
    Vec_WrdPush( p, Entry2 );
    Vec_WrdPush( p, Entry3 );
}
static inline void Vec_WrdPushFour( Vec_Wrd_t * p, word Entry1, word Entry2, word Entry3, word Entry4 )
{
    Vec_WrdPush( p, Entry1 );
    Vec_WrdPush( p, Entry2 );
    Vec_WrdPush( p, Entry3 );
    Vec_WrdPush( p, Entry4 );
}
static inline void Vec_WrdPushArray( Vec_Wrd_t * p, word * pEntries, int nEntries )
{
    int i;
    for ( i = 0; i < nEntries; i++ )
        Vec_WrdPush( p, pEntries[i] );
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
static inline void Vec_WrdDrop( Vec_Wrd_t * p, int i )
{
    int k;
    assert( i >= 0 && i < Vec_WrdSize(p) );
    p->nSize--;
    for ( k = i; k < p->nSize; k++ )
        p->pArray[k] = p->pArray[k+1];
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
        qsort( (void *)p->pArray, (size_t)p->nSize, sizeof(word), 
                (int (*)(const void *, const void *)) Vec_WrdSortCompare2 );
    else
        qsort( (void *)p->pArray, (size_t)p->nSize, sizeof(word), 
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

  Synopsis    [Returns the number of common entries.]

  Description [Assumes that the vectors are sorted in the increasing order.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_WrdTwoCountCommon( Vec_Wrd_t * vArr1, Vec_Wrd_t * vArr2 )
{
    word * pBeg1 = vArr1->pArray;
    word * pBeg2 = vArr2->pArray;
    word * pEnd1 = vArr1->pArray + vArr1->nSize;
    word * pEnd2 = vArr2->pArray + vArr2->nSize;
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
    qsort( (void *)p->pArray, (size_t)p->nSize, sizeof(word), 
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

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_WrdDumpBoolOne( FILE * pFile, word * pSim, int nBits, int fReverse )
{
    int k;
    if ( fReverse )
        for ( k = nBits-1; k >= 0; k-- )
            fprintf( pFile, "%d", (int)((pSim[k/64] >> (k%64)) & 1) );
    else
        for ( k = 0; k < nBits; k++ )
            fprintf( pFile, "%d", (int)((pSim[k/64] >> (k%64)) & 1) );
    fprintf( pFile, "\n" );
}
static inline void Vec_WrdDumpBool( char * pFileName, Vec_Wrd_t * p, int nWords, int nBits, int fReverse, int fVerbose )
{
    int i, nNodes = Vec_WrdSize(p) / nWords;
    FILE * pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" for writing.\n", pFileName );
        return;
    }
    assert( Vec_WrdSize(p) % nWords == 0 );
    for ( i = 0; i < nNodes; i++ )
        Vec_WrdDumpBoolOne( pFile, Vec_WrdEntryP(p, i*nWords), nBits, fReverse );
    fclose( pFile );
    if ( fVerbose )
        printf( "Written %d bits of simulation data for %d objects into file \"%s\".\n", nBits, Vec_WrdSize(p)/nWords, pFileName );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_WrdDumpHexOne( FILE * pFile, word * pSim, int nWords )
{
    int k, Digit, nDigits = nWords*16;
    for ( k = 0; k < nDigits; k++ )
    {
        Digit = (int)((pSim[k/16] >> ((k%16) * 4)) & 15);
        if ( Digit < 10 )
            fprintf( pFile, "%d", Digit );
        else
            fprintf( pFile, "%c", 'A' + Digit-10 );
    }
    fprintf( pFile, "\n" );
}
static inline void Vec_WrdPrintHex( Vec_Wrd_t * p, int nWords )
{
    int i, nNodes = Vec_WrdSize(p) / nWords;
    assert( Vec_WrdSize(p) % nWords == 0 );
    for ( i = 0; i < nNodes; i++ )
        Vec_WrdDumpHexOne( stdout, Vec_WrdEntryP(p, i*nWords), nWords );
}
static inline void Vec_WrdDumpHex( char * pFileName, Vec_Wrd_t * p, int nWords, int fVerbose )
{
    int i, nNodes = Vec_WrdSize(p) / nWords;
    FILE * pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" for writing.\n", pFileName );
        return;
    }
    assert( Vec_WrdSize(p) % nWords == 0 );
    for ( i = 0; i < nNodes; i++ )
        Vec_WrdDumpHexOne( pFile, Vec_WrdEntryP(p, i*nWords), nWords );
    fclose( pFile );
    if ( fVerbose )
        printf( "Written %d words of simulation data for %d objects into file \"%s\".\n", nWords, Vec_WrdSize(p)/nWords, pFileName );
}
static inline int Vec_WrdReadHexOne( char c )
{
    int Digit = 0;
    if ( c >= '0' && c <= '9' )
        Digit = c - '0';
    else if ( c >= 'A' && c <= 'F' )
        Digit = c - 'A' + 10;
    else if ( c >= 'a' && c <= 'f' )
        Digit = c - 'a' + 10;
    else assert( 0 );
    assert( Digit >= 0 && Digit < 16 );
    return Digit;
}
static inline Vec_Wrd_t * Vec_WrdReadHex( char * pFileName, int * pnWords, int fVerbose )
{
    Vec_Wrd_t * p = NULL; 
    int c, nWords = -1, nChars = 0; word Num = 0;
    FILE * pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" for reading.\n", pFileName );
        return NULL;
    }
    p = Vec_WrdAlloc( 1000 );
    while ( (c = fgetc(pFile)) != EOF )
    {
        if ( c == '\r' || c == '\t' || c == ' ' )
            continue;
        if ( c == '\n' )
        {
            if ( nChars > 0 )
            {
                Vec_WrdPush( p, Num );
                nChars = 0; 
                Num = 0;
            }
            if ( nWords == -1 && Vec_WrdSize(p) > 0 )
                nWords = Vec_WrdSize(p);
            continue;
        }
        Num |= (word)Vec_WrdReadHexOne((char)c) << (nChars * 4);
        if ( ++nChars < 16 )
            continue;
        Vec_WrdPush( p, Num );
        nChars = 0; 
        Num = 0;
    }
    assert( Vec_WrdSize(p) % nWords == 0 );
    fclose( pFile );
    if ( pnWords )
        *pnWords = nWords;
    if ( fVerbose )
        printf( "Read %d words of simulation data for %d objects.\n", nWords, Vec_WrdSize(p)/nWords );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_WrdDumpBin( char * pFileName, Vec_Wrd_t * p, int fVerbose )
{
    int RetValue;
    FILE * pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" for writing.\n", pFileName );
        return;
    }
    RetValue = fwrite( Vec_WrdArray(p), 1, 8*Vec_WrdSize(p), pFile );
    fclose( pFile );
    if ( RetValue != 8*Vec_WrdSize(p) )
        printf( "Error reading data from file.\n" );
    if ( fVerbose )
        printf( "Written %d words of simulation data into file \"%s\".\n", Vec_WrdSize(p), pFileName );
}
static inline Vec_Wrd_t * Vec_WrdReadBin( char * pFileName, int fVerbose )
{
    Vec_Wrd_t * p = NULL; int nSize, RetValue;
    FILE * pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" for reading.\n", pFileName );
        return NULL;
    }
    fseek( pFile, 0, SEEK_END );
    nSize = ftell( pFile );
    if ( nSize == 0 )
    {
        printf( "The input file is empty.\n" );
        fclose( pFile );
        return NULL;
    }
    if ( nSize % 8 > 0 )
    {
        printf( "Cannot read file with simulation data that is not aligned at 8 bytes (remainder = %d).\n", nSize % 8 );
        fclose( pFile );
        return NULL;
    }
    rewind( pFile );
    p = Vec_WrdStart( nSize/8 );
    RetValue = fread( Vec_WrdArray(p), 1, nSize, pFile );
    fclose( pFile );
    if ( RetValue != nSize )
        printf( "Error reading data from file.\n" );
    if ( fVerbose )
        printf( "Read %d words of simulation data from file \"%s\".\n", nSize/8, pFileName );
    return p;
}

ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

