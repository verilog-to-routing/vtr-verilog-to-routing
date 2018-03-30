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
 
#ifndef ABC__misc__vec__vecInt_h
#define ABC__misc__vec__vecInt_h


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
#define Vec_IntForEachEntryStop( vVec, Entry, i, Stop )                                     \
    for ( i = 0; (i < Stop) && (((Entry) = Vec_IntEntry(vVec, i)), 1); i++ )
#define Vec_IntForEachEntryStartStop( vVec, Entry, i, Start, Stop )                         \
    for ( i = Start; (i < Stop) && (((Entry) = Vec_IntEntry(vVec, i)), 1); i++ )
#define Vec_IntForEachEntryReverse( vVec, pEntry, i )                                       \
    for ( i = Vec_IntSize(vVec) - 1; (i >= 0) && (((pEntry) = Vec_IntEntry(vVec, i)), 1); i-- )
#define Vec_IntForEachEntryTwo( vVec1, vVec2, Entry1, Entry2, i )                           \
    for ( i = 0; (i < Vec_IntSize(vVec1)) && (((Entry1) = Vec_IntEntry(vVec1, i)), 1) && (((Entry2) = Vec_IntEntry(vVec2, i)), 1); i++ )
#define Vec_IntForEachEntryDouble( vVec, Entry1, Entry2, i )                                \
    for ( i = 0; (i+1 < Vec_IntSize(vVec)) && (((Entry1) = Vec_IntEntry(vVec, i)), 1) && (((Entry2) = Vec_IntEntry(vVec, i+1)), 1); i += 2 )
#define Vec_IntForEachEntryDoubleStart( vVec, Entry1, Entry2, i, Start )                    \
    for ( i = Start; (i+1 < Vec_IntSize(vVec)) && (((Entry1) = Vec_IntEntry(vVec, i)), 1) && (((Entry2) = Vec_IntEntry(vVec, i+1)), 1); i += 2 )
#define Vec_IntForEachEntryTriple( vVec, Entry1, Entry2, Entry3, i )                        \
    for ( i = 0; (i+2 < Vec_IntSize(vVec)) && (((Entry1) = Vec_IntEntry(vVec, i)), 1) && (((Entry2) = Vec_IntEntry(vVec, i+1)), 1) && (((Entry3) = Vec_IntEntry(vVec, i+2)), 1); i += 3 )
#define Vec_IntForEachEntryThisNext( vVec, This, Next, i )                                  \
    for ( i = 0, (This) = (Next) = (Vec_IntSize(vVec) ? Vec_IntEntry(vVec, 0) : -1); (i+1 < Vec_IntSize(vVec)) && (((Next) = Vec_IntEntry(vVec, i+1)), 1); i += 2, (This) = (Next) )
#define Vec_IntForEachEntryInVec( vVec2, vVec, Entry, i )                                   \
    for ( i = 0; (i < Vec_IntSize(vVec)) && (((Entry) = Vec_IntEntry(vVec2, Vec_IntEntry(vVec, i))), 1); i++ )

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
    p = ABC_ALLOC( Vec_Int_t, 1 );
    if ( nCap > 0 && nCap < 16 )
        nCap = 16;
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ABC_ALLOC( int, p->nCap ) : NULL;
    return p;
}
static inline Vec_Int_t * Vec_IntAllocExact( int nCap )
{
    Vec_Int_t * p;
    assert( nCap >= 0 );
    p = ABC_ALLOC( Vec_Int_t, 1 );
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ABC_ALLOC( int, p->nCap ) : NULL;
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
static inline Vec_Int_t * Vec_IntStartFull( int nSize )
{
    Vec_Int_t * p;
    p = Vec_IntAlloc( nSize );
    p->nSize = nSize;
    memset( p->pArray, 0xff, sizeof(int) * nSize );
    return p;
}
static inline Vec_Int_t * Vec_IntStartRange( int First, int Range )
{
    Vec_Int_t * p;
    int i;
    p = Vec_IntAlloc( Range );
    p->nSize = Range;
    for ( i = 0; i < Range; i++ )
        p->pArray[i] = First + i;
    return p;
}

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given size and cleans it.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Int_t * Vec_IntStartNatural( int nSize )
{
    Vec_Int_t * p;
    int i;
    p = Vec_IntAlloc( nSize );
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
static inline Vec_Int_t * Vec_IntAllocArray( int * pArray, int nSize )
{
    Vec_Int_t * p;
    p = ABC_ALLOC( Vec_Int_t, 1 );
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
    p = ABC_ALLOC( Vec_Int_t, 1 );
    p->nSize  = nSize;
    p->nCap   = nSize;
    p->pArray = ABC_ALLOC( int, nSize );
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
    p = ABC_ALLOC( Vec_Int_t, 1 );
    p->nSize  = pVec->nSize;
    p->nCap   = pVec->nSize;
    p->pArray = p->nCap? ABC_ALLOC( int, p->nCap ) : NULL;
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
    p = ABC_ALLOC( Vec_Int_t, 1 );
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
static inline void Vec_IntZero( Vec_Int_t * p )
{
    p->pArray = NULL;
    p->nSize = 0;
    p->nCap = 0;
}
static inline void Vec_IntErase( Vec_Int_t * p )
{
    ABC_FREE( p->pArray );
    p->nSize = 0;
    p->nCap = 0;
}
static inline void Vec_IntFree( Vec_Int_t * p )
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
static inline void Vec_IntFreeP( Vec_Int_t ** p )
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
static inline int ** Vec_IntArrayP( Vec_Int_t * p )
{
    return &p->pArray;
}
static inline int * Vec_IntLimit( Vec_Int_t * p )
{
    return p->pArray + p->nSize;
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
static inline int Vec_IntCap( Vec_Int_t * p )
{
    return p->nCap;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline double Vec_IntMemory( Vec_Int_t * p )
{
    return !p ? 0.0 : 1.0 * sizeof(int) * p->nCap + sizeof(Vec_Int_t) ;
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
static inline int * Vec_IntEntryP( Vec_Int_t * p, int i )
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
static inline int Vec_IntAddToEntry( Vec_Int_t * p, int i, int Addition )
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
static inline void Vec_IntUpdateEntry( Vec_Int_t * p, int i, int Value )
{
    if ( Vec_IntEntry( p, i ) < Value )
        Vec_IntWriteEntry( p, i, Value );
}
static inline void Vec_IntDowndateEntry( Vec_Int_t * p, int i, int Value )
{
    if ( Vec_IntEntry( p, i ) > Value )
        Vec_IntWriteEntry( p, i, Value );
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
    p->pArray = ABC_REALLOC( int, p->pArray, nCapMin ); 
    assert( p->pArray );
    p->nCap   = nCapMin;
}

/**Function*************************************************************

  Synopsis    [Resizes the vector to the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntGrowResize( Vec_Int_t * p, int nCapMin )
{
    p->nSize  = nCapMin;
    if ( p->nCap >= nCapMin )
        return;
    p->pArray = ABC_REALLOC( int, p->pArray, nCapMin ); 
    assert( p->pArray );
    p->nCap   = nCapMin;
}

/**Function*************************************************************

  Synopsis    [Fills the vector with given number of entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntFill( Vec_Int_t * p, int nSize, int Fill )
{
    int i;
    Vec_IntGrow( p, nSize );
    for ( i = 0; i < nSize; i++ )
        p->pArray[i] = Fill;
    p->nSize = nSize;
}
static inline void Vec_IntFillTwo( Vec_Int_t * p, int nSize, int FillEven, int FillOdd )
{
    int i;
    Vec_IntGrow( p, nSize );
    for ( i = 0; i < nSize; i++ )
        p->pArray[i] = (i & 1) ? FillOdd : FillEven;
    p->nSize = nSize;
}
static inline void Vec_IntFillNatural( Vec_Int_t * p, int nSize )
{
    int i;
    Vec_IntGrow( p, nSize );
    for ( i = 0; i < nSize; i++ )
        p->pArray[i] = i;
    p->nSize = nSize;
}

/**Function*************************************************************

  Synopsis    [Fills the vector with given number of entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntFillExtra( Vec_Int_t * p, int nSize, int Fill )
{
    int i;
    if ( nSize <= p->nSize )
        return;
    if ( nSize > 2 * p->nCap )
        Vec_IntGrow( p, nSize );
    else if ( nSize > p->nCap )
        Vec_IntGrow( p, 2 * p->nCap );
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
static inline int Vec_IntGetEntry( Vec_Int_t * p, int i )
{
    Vec_IntFillExtra( p, i + 1, 0 );
    return Vec_IntEntry( p, i );
}
static inline int Vec_IntGetEntryFull( Vec_Int_t * p, int i )
{
    Vec_IntFillExtra( p, i + 1, -1 );
    return Vec_IntEntry( p, i );
}

/**Function*************************************************************

  Synopsis    [Returns the entry even if the place not exist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int * Vec_IntGetEntryP( Vec_Int_t * p, int i )
{
    Vec_IntFillExtra( p, i + 1, 0 );
    return Vec_IntEntryP( p, i );
}

/**Function*************************************************************

  Synopsis    [Inserts the entry even if the place does not exist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntSetEntry( Vec_Int_t * p, int i, int Entry )
{
    Vec_IntFillExtra( p, i + 1, 0 );
    Vec_IntWriteEntry( p, i, Entry );
}
static inline void Vec_IntSetEntryFull( Vec_Int_t * p, int i, int Entry )
{
    Vec_IntFillExtra( p, i + 1, -1 );
    Vec_IntWriteEntry( p, i, Entry );
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
static inline void Vec_IntPushTwo( Vec_Int_t * p, int Entry1, int Entry2 )
{
    Vec_IntPush( p, Entry1 );
    Vec_IntPush( p, Entry2 );
}
static inline void Vec_IntPushThree( Vec_Int_t * p, int Entry1, int Entry2, int Entry3 )
{
    Vec_IntPush( p, Entry1 );
    Vec_IntPush( p, Entry2 );
    Vec_IntPush( p, Entry3 );
}
static inline void Vec_IntPushFour( Vec_Int_t * p, int Entry1, int Entry2, int Entry3, int Entry4 )
{
    Vec_IntPush( p, Entry1 );
    Vec_IntPush( p, Entry2 );
    Vec_IntPush( p, Entry3 );
    Vec_IntPush( p, Entry4 );
}
static inline void Vec_IntPushArray( Vec_Int_t * p, int * pEntries, int nEntries )
{
    int i;
    for ( i = 0; i < nEntries; i++ )
        Vec_IntPush( p, pEntries[i] );
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
static inline void Vec_IntPushOrderCost( Vec_Int_t * p, int Entry, Vec_Int_t * vCost )
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
        if ( Vec_IntEntry(vCost, p->pArray[i]) > Vec_IntEntry(vCost, Entry) )
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
static inline void Vec_IntPushOrderReverse( Vec_Int_t * p, int Entry )
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
        if ( p->pArray[i] < Entry )
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
static inline int Vec_IntPushUniqueOrderCost( Vec_Int_t * p, int Entry, Vec_Int_t * vCost )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        if ( p->pArray[i] == Entry )
            return 1;
    Vec_IntPushOrderCost( p, Entry, vCost );
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
static inline int Vec_IntRemove1( Vec_Int_t * p, int Entry )
{
    int i;
    for ( i = 1; i < p->nSize; i++ )
        if ( p->pArray[i] == Entry )
            break;
    if ( i >= p->nSize )
        return 0;
    assert( i < p->nSize );
    for ( i++; i < p->nSize; i++ )
        p->pArray[i-1] = p->pArray[i];
    p->nSize--;
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntDrop( Vec_Int_t * p, int i )
{
    int k;
    assert( i >= 0 && i < Vec_IntSize(p) );
    p->nSize--;
    for ( k = i; k < p->nSize; k++ )
        p->pArray[k] = p->pArray[k+1];
}

/**Function*************************************************************

  Synopsis    [Interts entry at the index iHere. Shifts other entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntInsert( Vec_Int_t * p, int iHere, int Entry )
{
    int i;
    assert( iHere >= 0 && iHere <= p->nSize );
    Vec_IntPush( p, 0 );
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
static inline int Vec_IntFindMax( Vec_Int_t * p )
{
    int i, Best;
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
static inline int Vec_IntFindMin( Vec_Int_t * p )
{
    int i, Best;
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
static inline void Vec_IntReverseOrder( Vec_Int_t * p )
{
    int i, Temp;
    for ( i = 0; i < p->nSize/2; i++ )
    {
        Temp = p->pArray[i];
        p->pArray[i] = p->pArray[p->nSize-1-i];
        p->pArray[p->nSize-1-i] = Temp;
    }
}

/**Function*************************************************************

  Synopsis    [Removes odd entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntRemoveOdd( Vec_Int_t * p )
{
    int i;
    assert( (p->nSize & 1) == 0 );
    p->nSize >>= 1;
    for ( i = 0; i < p->nSize; i++ )
        p->pArray[i] = p->pArray[2*i];
}
static inline void Vec_IntRemoveEven( Vec_Int_t * p )
{
    int i;
    assert( (p->nSize & 1) == 0 );
    p->nSize >>= 1;
    for ( i = 0; i < p->nSize; i++ )
        p->pArray[i] = p->pArray[2*i+1];
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Int_t * Vec_IntInvert( Vec_Int_t * p, int Fill ) 
{
    int Entry, i;
    Vec_Int_t * vRes = Vec_IntAlloc( 0 );
    if ( Vec_IntSize(p) == 0 )
        return vRes;
    Vec_IntFill( vRes, Vec_IntFindMax(p) + 1, Fill );
    Vec_IntForEachEntry( p, Entry, i )
        if ( Entry != Fill )
            Vec_IntWriteEntry( vRes, Entry, i );
    return vRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Int_t * Vec_IntCondense( Vec_Int_t * p, int Fill ) 
{
    int Entry, i;
    Vec_Int_t * vRes = Vec_IntAlloc( Vec_IntSize(p) );
    Vec_IntForEachEntry( p, Entry, i )
        if ( Entry != Fill )
            Vec_IntPush( vRes, Entry );
    return vRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_IntSum( Vec_Int_t * p ) 
{
    int i, Counter = 0;
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
static inline int Vec_IntCountEntry( Vec_Int_t * p, int Entry ) 
{
    int i, Counter = 0;
    for ( i = 0; i < p->nSize; i++ )
        Counter += (p->pArray[i] == Entry);
    return Counter;
}
static inline int Vec_IntCountLarger( Vec_Int_t * p, int Entry ) 
{
    int i, Counter = 0;
    for ( i = 0; i < p->nSize; i++ )
        Counter += (p->pArray[i] > Entry);
    return Counter;
}
static inline int Vec_IntCountSmaller( Vec_Int_t * p, int Entry ) 
{
    int i, Counter = 0;
    for ( i = 0; i < p->nSize; i++ )
        Counter += (p->pArray[i] < Entry);
    return Counter;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_IntCountPositive( Vec_Int_t * p ) 
{
    int i, Counter = 0;
    for ( i = 0; i < p->nSize; i++ )
        Counter += (p->pArray[i] > 0);
    return Counter;
}
static inline int Vec_IntCountZero( Vec_Int_t * p ) 
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
static inline int Vec_IntEqual( Vec_Int_t * p1, Vec_Int_t * p2 ) 
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

  Description [Assumes that the entries are non-negative integers that
  are not very large, so inversion of the array can be performed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_IntCountCommon( Vec_Int_t * p1, Vec_Int_t * p2 ) 
{
    Vec_Int_t * vTemp;
    int Entry, i, Counter = 0;
    if ( Vec_IntSize(p1) < Vec_IntSize(p2) )
        vTemp = p1, p1 = p2, p2 = vTemp;
    assert( Vec_IntSize(p1) >= Vec_IntSize(p2) );
    vTemp = Vec_IntInvert( p2, -1 );
    Vec_IntFillExtra( vTemp, Vec_IntFindMax(p1) + 1, -1 );
    Vec_IntForEachEntry( p1, Entry, i )
        if ( Vec_IntEntry(vTemp, Entry) >= 0 )
            Counter++;
    Vec_IntFree( vTemp );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Comparison procedure for two integers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Vec_IntSortCompare1( int * pp1, int * pp2 )
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
static int Vec_IntSortCompare2( int * pp1, int * pp2 )
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
static inline void Vec_IntSortMulti( Vec_Int_t * p, int nMulti, int fReverse )
{
    assert( Vec_IntSize(p) % nMulti == 0 );
    if ( fReverse ) 
        qsort( (void *)p->pArray, p->nSize/nMulti, nMulti*sizeof(int), 
                (int (*)(const void *, const void *)) Vec_IntSortCompare2 );
    else
        qsort( (void *)p->pArray, p->nSize/nMulti, nMulti*sizeof(int), 
                (int (*)(const void *, const void *)) Vec_IntSortCompare1 );
}

/**Function*************************************************************

  Synopsis    [Leaves only unique entries.]

  Description [Returns the number of duplicated entried found.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_IntUniqify( Vec_Int_t * p )
{
    int i, k, RetValue;
    if ( p->nSize < 2 )
        return 0;
    Vec_IntSort( p, 0 );
    for ( i = k = 1; i < p->nSize; i++ )
        if ( p->pArray[i] != p->pArray[i-1] )
            p->pArray[k++] = p->pArray[i];
    RetValue = p->nSize - k;
    p->nSize = k;
    return RetValue;
}
static inline int Vec_IntCountDuplicates( Vec_Int_t * p )
{
    int RetValue;
    Vec_Int_t * pDup = Vec_IntDup( p );
    Vec_IntUniqify( pDup );
    RetValue = Vec_IntSize(p) - Vec_IntSize(pDup);
    Vec_IntFree( pDup );
    return RetValue;
}
static inline int Vec_IntCheckUniqueSmall( Vec_Int_t * p )
{
    int i, k;
    for ( i = 0; i < p->nSize; i++ )
        for ( k = i+1; k < p->nSize; k++ )
            if ( p->pArray[i] == p->pArray[k] )
                return 0;
    return 1;
}
static inline int Vec_IntCountUnique( Vec_Int_t * p )
{
    int i, Count = 0, Max = Vec_IntFindMax(p);
    unsigned char * pPres = ABC_CALLOC( unsigned char, Max+1 );
    for ( i = 0; i < p->nSize; i++ )
        if ( pPres[p->pArray[i]] == 0 )
            pPres[p->pArray[i]] = 1, Count++;
    ABC_FREE( pPres );
    return Count;
}

/**Function*************************************************************

  Synopsis    [Counts the number of unique pairs.]

  Description []
               
  SideEffects [] 

  SeeAlso     []

***********************************************************************/
static inline int Vec_IntUniqifyPairs( Vec_Int_t * p )
{
    int i, k, RetValue;
    assert( p->nSize % 2 == 0 );
    if ( p->nSize < 4 )
        return 0;
    Vec_IntSortMulti( p, 2, 0 );
    for ( i = k = 1; i < p->nSize/2; i++ )
        if ( p->pArray[2*i] != p->pArray[2*(i-1)] || p->pArray[2*i+1] != p->pArray[2*(i-1)+1] )
        {
            p->pArray[2*k]   = p->pArray[2*i];
            p->pArray[2*k+1] = p->pArray[2*i+1];
            k++;
        }
    RetValue = p->nSize/2 - k;
    p->nSize = 2*k;
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Counts the number of unique entries.]

  Description []
               
  SideEffects [] 

  SeeAlso     []

***********************************************************************/
static inline unsigned Vec_IntUniqueHashKeyDebug( unsigned char * pStr, int nChars, int TableMask )
{
    static unsigned s_BigPrimes[4] = {12582917, 25165843, 50331653, 100663319};
    unsigned Key = 0; int c;
    for ( c = 0; c < nChars; c++ )
    {
        Key += (unsigned)pStr[c] * s_BigPrimes[c & 3];
        printf( "%d : ", c );
        printf( "%3d  ", pStr[c] );
        printf( "%12u ", Key );
        printf( "%12u ", Key&TableMask );
        printf( "\n" );
    }
    return Key;
}
static inline void Vec_IntUniqueProfile( Vec_Int_t * vData, int * pTable, int * pNexts, int TableMask, int nIntSize )
{
    int i, Key, Counter;
    for ( i = 0; i <= TableMask; i++ )
    {
        Counter = 0;
        for ( Key = pTable[i]; Key != -1; Key = pNexts[Key] )
            Counter++;
        if ( Counter < 7 )
            continue;
        printf( "%d\n", Counter );
        for ( Key = pTable[i]; Key != -1; Key = pNexts[Key] )
        {
//            Extra_PrintBinary( stdout, (unsigned *)Vec_IntEntryP(vData, Key*nIntSize), 40 ), printf( "\n" );
//            Vec_IntUniqueHashKeyDebug( (unsigned char *)Vec_IntEntryP(vData, Key*nIntSize), 4*nIntSize, TableMask );
        }
    }
    printf( "\n" );
}

static inline unsigned Vec_IntUniqueHashKey2( unsigned char * pStr, int nChars )
{
    static unsigned s_BigPrimes[4] = {12582917, 25165843, 50331653, 100663319};
    unsigned Key = 0; int c;
    for ( c = 0; c < nChars; c++ )
        Key += (unsigned)pStr[c] * s_BigPrimes[c & 3];
    return Key;
}

static inline unsigned Vec_IntUniqueHashKey( unsigned char * pStr, int nChars )
{
    static unsigned s_BigPrimes[16] = 
    {
        0x984b6ad9,0x18a6eed3,0x950353e2,0x6222f6eb,0xdfbedd47,0xef0f9023,0xac932a26,0x590eaf55,
        0x97d0a034,0xdc36cd2e,0x22736b37,0xdc9066b0,0x2eb2f98b,0x5d9c7baf,0x85747c9e,0x8aca1055
    };
    static unsigned s_BigPrimes2[16] = 
    {
        0x8d8a5ebe,0x1e6a15dc,0x197d49db,0x5bab9c89,0x4b55dea7,0x55dede49,0x9a6a8080,0xe5e51035,
        0xe148d658,0x8a17eb3b,0xe22e4b38,0xe5be2a9a,0xbe938cbb,0x3b981069,0x7f9c0c8e,0xf756df10
    };
    unsigned Key = 0; int c;
    for ( c = 0; c < nChars; c++ )
        Key += s_BigPrimes2[(2*c)&15]   * s_BigPrimes[(unsigned)pStr[c] & 15] +
               s_BigPrimes2[(2*c+1)&15] * s_BigPrimes[(unsigned)pStr[c] >> 4];
    return Key;
}
static inline int * Vec_IntUniqueLookup( Vec_Int_t * vData, int i, int nIntSize, int * pNexts, int * pStart )
{
    int * pData = Vec_IntEntryP( vData, i*nIntSize );
    for ( ; *pStart != -1; pStart = pNexts + *pStart )
        if ( !memcmp( pData, Vec_IntEntryP(vData, *pStart*nIntSize), sizeof(int) * nIntSize ) )
            return pStart;
    return pStart;
}
static inline int Vec_IntUniqueCount( Vec_Int_t * vData, int nIntSize, Vec_Int_t ** pvMap )
{
    int nEntries  = Vec_IntSize(vData) / nIntSize;
    int TableMask = (1 << Abc_Base2Log(nEntries)) - 1;
    int * pTable  = ABC_FALLOC( int, TableMask+1 );
    int * pNexts  = ABC_FALLOC( int, TableMask+1 );
    int * pClass  = ABC_ALLOC( int, nEntries );
    int i, Key, * pEnt, nUnique = 0;
    assert( nEntries * nIntSize == Vec_IntSize(vData) );
    for ( i = 0; i < nEntries; i++ )
    {
        pEnt = Vec_IntEntryP( vData, i*nIntSize );
        Key  = TableMask & Vec_IntUniqueHashKey( (unsigned char *)pEnt, 4*nIntSize );
        pEnt = Vec_IntUniqueLookup( vData, i, nIntSize, pNexts, pTable+Key );
        if ( *pEnt == -1 )
            *pEnt = i, nUnique++;
        pClass[i] = *pEnt;
    }
//    Vec_IntUniqueProfile( vData, pTable, pNexts, TableMask, nIntSize );
    ABC_FREE( pTable );
    ABC_FREE( pNexts );
    if ( pvMap )
        *pvMap = Vec_IntAllocArray( pClass, nEntries );
    else
        ABC_FREE( pClass );
    return nUnique;
}
static inline Vec_Int_t * Vec_IntUniqifyHash( Vec_Int_t * vData, int nIntSize )
{
    Vec_Int_t * vMap, * vUnique;
    int i, Ent, nUnique = Vec_IntUniqueCount( vData, nIntSize, &vMap );
    vUnique = Vec_IntAlloc( nUnique * nIntSize );
    Vec_IntForEachEntry( vMap, Ent, i )
    {
        if ( Ent < i ) continue;
        assert( Ent == i );
        Vec_IntPushArray( vUnique, Vec_IntEntryP(vData, i*nIntSize), nIntSize );
    }
    assert( Vec_IntSize(vUnique) == nUnique * nIntSize );
    Vec_IntFree( vMap );
    return vUnique;
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

  Synopsis    [Collects common entries.]

  Description [Assumes that the vectors are sorted in the increasing order.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_IntTwoFindCommon( Vec_Int_t * vArr1, Vec_Int_t * vArr2, Vec_Int_t * vArr )
{
    int * pBeg1 = vArr1->pArray;
    int * pBeg2 = vArr2->pArray;
    int * pEnd1 = vArr1->pArray + vArr1->nSize;
    int * pEnd2 = vArr2->pArray + vArr2->nSize;
    Vec_IntClear( vArr );
    while ( pBeg1 < pEnd1 && pBeg2 < pEnd2 )
    {
        if ( *pBeg1 == *pBeg2 )
            Vec_IntPush( vArr, *pBeg1 ), pBeg1++, pBeg2++;
        else if ( *pBeg1 < *pBeg2 )
            pBeg1++;
        else 
            pBeg2++;
    }
    return Vec_IntSize(vArr);
}
static inline int Vec_IntTwoFindCommonReverse( Vec_Int_t * vArr1, Vec_Int_t * vArr2, Vec_Int_t * vArr )
{
    int * pBeg1 = vArr1->pArray;
    int * pBeg2 = vArr2->pArray;
    int * pEnd1 = vArr1->pArray + vArr1->nSize;
    int * pEnd2 = vArr2->pArray + vArr2->nSize;
    Vec_IntClear( vArr );
    while ( pBeg1 < pEnd1 && pBeg2 < pEnd2 )
    {
        if ( *pBeg1 == *pBeg2 )
            Vec_IntPush( vArr, *pBeg1 ), pBeg1++, pBeg2++;
        else if ( *pBeg1 > *pBeg2 )
            pBeg1++;
        else 
            pBeg2++;
    }
    return Vec_IntSize(vArr);
}

/**Function*************************************************************

  Synopsis    [Collects and removes common entries]

  Description [Assumes that the vectors are sorted in the increasing order.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_IntTwoRemoveCommon( Vec_Int_t * vArr1, Vec_Int_t * vArr2, Vec_Int_t * vArr )
{
    int * pBeg1 = vArr1->pArray;
    int * pBeg2 = vArr2->pArray;
    int * pEnd1 = vArr1->pArray + vArr1->nSize;
    int * pEnd2 = vArr2->pArray + vArr2->nSize;
    int * pBeg1New = vArr1->pArray;
    int * pBeg2New = vArr2->pArray;
    Vec_IntClear( vArr );
    while ( pBeg1 < pEnd1 && pBeg2 < pEnd2 )
    {
        if ( *pBeg1 == *pBeg2 )
            Vec_IntPush( vArr, *pBeg1 ), pBeg1++, pBeg2++;
        else if ( *pBeg1 < *pBeg2 )
            *pBeg1New++ = *pBeg1++;
        else 
            *pBeg2New++ = *pBeg2++;
    }
    while ( pBeg1 < pEnd1 )
        *pBeg1New++ = *pBeg1++;
    while ( pBeg2 < pEnd2 )
        *pBeg2New++ = *pBeg2++;
    Vec_IntShrink( vArr1, pBeg1New - vArr1->pArray );
    Vec_IntShrink( vArr2, pBeg2New - vArr2->pArray );
    return Vec_IntSize(vArr);
}

/**Function*************************************************************

  Synopsis    [Removes entries of the second one from the first one.]

  Description [Assumes that the vectors are sorted in the increasing order.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_IntTwoRemove( Vec_Int_t * vArr1, Vec_Int_t * vArr2 )
{
    int * pBeg1 = vArr1->pArray;
    int * pBeg2 = vArr2->pArray;
    int * pEnd1 = vArr1->pArray + vArr1->nSize;
    int * pEnd2 = vArr2->pArray + vArr2->nSize;
    int * pBeg1New = vArr1->pArray;
    while ( pBeg1 < pEnd1 && pBeg2 < pEnd2 )
    {
        if ( *pBeg1 == *pBeg2 )
            pBeg1++, pBeg2++;
        else if ( *pBeg1 < *pBeg2 )
            *pBeg1New++ = *pBeg1++;
        else 
            pBeg2++;
    }
    while ( pBeg1 < pEnd1 )
        *pBeg1New++ = *pBeg1++;
    Vec_IntShrink( vArr1, pBeg1New - vArr1->pArray );
    return Vec_IntSize(vArr1);
}

/**Function*************************************************************

  Synopsis    [Returns the result of merging the two vectors.]

  Description [Assumes that the vectors are sorted in the increasing order.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntTwoMerge2Int( Vec_Int_t * vArr1, Vec_Int_t * vArr2, Vec_Int_t * vArr )
{
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
}
static inline Vec_Int_t * Vec_IntTwoMerge( Vec_Int_t * vArr1, Vec_Int_t * vArr2 )
{
    Vec_Int_t * vArr = Vec_IntAlloc( vArr1->nSize + vArr2->nSize ); 
    Vec_IntTwoMerge2Int( vArr1, vArr2, vArr );
    return vArr;
}
static inline void Vec_IntTwoMerge2( Vec_Int_t * vArr1, Vec_Int_t * vArr2, Vec_Int_t * vArr )
{
    Vec_IntGrow( vArr, Vec_IntSize(vArr1) + Vec_IntSize(vArr2) );
    Vec_IntTwoMerge2Int( vArr1, vArr2, vArr );
}

/**Function*************************************************************

  Synopsis    [Returns the result of splitting of the two vectors.]

  Description [Assumes that the vectors are sorted in the increasing order.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntTwoSplit( Vec_Int_t * vArr1, Vec_Int_t * vArr2, Vec_Int_t * vArr, Vec_Int_t * vArr1n, Vec_Int_t * vArr2n )
{
    int * pBeg1 = vArr1->pArray;
    int * pBeg2 = vArr2->pArray;
    int * pEnd1 = vArr1->pArray + vArr1->nSize;
    int * pEnd2 = vArr2->pArray + vArr2->nSize;
    while ( pBeg1 < pEnd1 && pBeg2 < pEnd2 )
    {
        if ( *pBeg1 == *pBeg2 )
            Vec_IntPush( vArr, *pBeg1++ ), pBeg2++;
        else if ( *pBeg1 < *pBeg2 )
            Vec_IntPush( vArr1n, *pBeg1++ );
        else 
            Vec_IntPush( vArr2n, *pBeg2++ );
    }
    while ( pBeg1 < pEnd1 )
        Vec_IntPush( vArr1n, *pBeg1++ );
    while ( pBeg2 < pEnd2 )
        Vec_IntPush( vArr2n, *pBeg2++ );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntSelectSort( int * pArray, int nSize )
{
    int temp, i, j, best_i;
    for ( i = 0; i < nSize-1; i++ )
    {
        best_i = i;
        for ( j = i+1; j < nSize; j++ )
            if ( pArray[j] < pArray[best_i] )
                best_i = j;
        temp = pArray[i]; 
        pArray[i] = pArray[best_i]; 
        pArray[best_i] = temp;
    }
}
static inline void Vec_IntSelectSortReverse( int * pArray, int nSize )
{
    int temp, i, j, best_i;
    for ( i = 0; i < nSize-1; i++ )
    {
        best_i = i;
        for ( j = i+1; j < nSize; j++ )
            if ( pArray[j] > pArray[best_i] )
                best_i = j;
        temp = pArray[i]; 
        pArray[i] = pArray[best_i]; 
        pArray[best_i] = temp;
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntSelectSortCost( int * pArray, int nSize, Vec_Int_t * vCosts )
{
    int i, j, best_i;
    for ( i = 0; i < nSize-1; i++ )
    {
        best_i = i;
        for ( j = i+1; j < nSize; j++ )
            if ( Vec_IntEntry(vCosts, pArray[j]) < Vec_IntEntry(vCosts, pArray[best_i]) )
                best_i = j;
        ABC_SWAP( int, pArray[i], pArray[best_i] );
    }
}
static inline void Vec_IntSelectSortCostReverse( int * pArray, int nSize, Vec_Int_t * vCosts )
{
    int i, j, best_i;
    for ( i = 0; i < nSize-1; i++ )
    {
        best_i = i;
        for ( j = i+1; j < nSize; j++ )
            if ( Vec_IntEntry(vCosts, pArray[j]) > Vec_IntEntry(vCosts, pArray[best_i]) )
                best_i = j;
        ABC_SWAP( int, pArray[i], pArray[best_i] );
    }
}

static inline void Vec_IntSelectSortCost2( int * pArray, int nSize, int * pCosts )
{
    int i, j, best_i;
    for ( i = 0; i < nSize-1; i++ )
    {
        best_i = i;
        for ( j = i+1; j < nSize; j++ )
            if ( pCosts[j] < pCosts[best_i] )
                best_i = j;
        ABC_SWAP( int, pArray[i], pArray[best_i] );
        ABC_SWAP( int, pCosts[i], pCosts[best_i] );
    }
}
static inline void Vec_IntSelectSortCost2Reverse( int * pArray, int nSize, int * pCosts )
{
    int i, j, best_i;
    for ( i = 0; i < nSize-1; i++ )
    {
        best_i = i;
        for ( j = i+1; j < nSize; j++ )
            if ( pCosts[j] > pCosts[best_i] )
                best_i = j;
        ABC_SWAP( int, pArray[i], pArray[best_i] );
        ABC_SWAP( int, pCosts[i], pCosts[best_i] );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntPrint( Vec_Int_t * vVec )
{
    int i, Entry;
    printf( "Vector has %d entries: {", Vec_IntSize(vVec) );
    Vec_IntForEachEntry( vVec, Entry, i )
        printf( " %d", Entry );
    printf( " }\n" );
}
static inline void Vec_IntPrintBinary( Vec_Int_t * vVec )
{
    int i, Entry;
    Vec_IntForEachEntry( vVec, Entry, i )
        printf( "%d", (int)(Entry != 0) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_IntCompareVec( Vec_Int_t * p1, Vec_Int_t * p2 )
{
    if ( p1 == NULL || p2 == NULL )
        return (p1 != NULL) - (p2 != NULL);
    if ( Vec_IntSize(p1) != Vec_IntSize(p2) )
        return Vec_IntSize(p1) - Vec_IntSize(p2);
    return memcmp( Vec_IntArray(p1), Vec_IntArray(p2), sizeof(int)*Vec_IntSize(p1) );
}

/**Function*************************************************************

  Synopsis    [Appends the contents of the second vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntAppend( Vec_Int_t * vVec1, Vec_Int_t * vVec2 )
{
    int Entry, i;
    Vec_IntForEachEntry( vVec2, Entry, i )
        Vec_IntPush( vVec1, Entry );
}
static inline void Vec_IntAppendSkip( Vec_Int_t * vVec1, Vec_Int_t * vVec2, int iVar )
{
    int Entry, i;
    Vec_IntForEachEntry( vVec2, Entry, i )
        if ( i != iVar )
            Vec_IntPush( vVec1, Entry );
}
static inline void Vec_IntAppendMinus( Vec_Int_t * vVec1, Vec_Int_t * vVec2, int fMinus )
{
    int Entry, i;
    Vec_IntClear( vVec1 );
    Vec_IntForEachEntry( vVec2, Entry, i )
        Vec_IntPush( vVec1, fMinus ? -Entry : Entry );
}

/**Function*************************************************************

  Synopsis    [Remapping attributes after objects were duplicated.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntRemapArray( Vec_Int_t * vOld2New, Vec_Int_t * vOld, Vec_Int_t * vNew, int nNew )
{
    int iOld, iNew;
    if ( Vec_IntSize(vOld) == 0 )
        return;
    Vec_IntFill( vNew, nNew, 0 );
    Vec_IntForEachEntry( vOld2New, iNew, iOld )
        if ( iNew > 0 && iNew < nNew && iOld < Vec_IntSize(vOld) && Vec_IntEntry(vOld, iOld) != 0 )
            Vec_IntWriteEntry( vNew, iNew, Vec_IntEntry(vOld, iOld) );
}

ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

