/**CFile****************************************************************

  FileName    [vecBit.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Resizable arrays.]

  Synopsis    [Resizable arrays of bits.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: vecBit.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__misc__vec__vecBit_h
#define ABC__misc__vec__vecBit_h


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

typedef struct Vec_Bit_t_       Vec_Bit_t;
struct Vec_Bit_t_ 
{
    int              nCap;
    int              nSize;
    int *            pArray;
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

#define Vec_BitForEachEntry( vVec, Entry, i )                                               \
    for ( i = 0; (i < Vec_BitSize(vVec)) && (((Entry) = Vec_BitEntry(vVec, i)), 1); i++ )
#define Vec_BitForEachEntryStart( vVec, Entry, i, Start )                                   \
    for ( i = Start; (i < Vec_BitSize(vVec)) && (((Entry) = Vec_BitEntry(vVec, i)), 1); i++ )
#define Vec_BitForEachEntryStop( vVec, Entry, i, Stop )                                   \
    for ( i = 0; (i < Stop) && (((Entry) = Vec_BitEntry(vVec, i)), 1); i++ )
#define Vec_BitForEachEntryStartStop( vVec, Entry, i, Start, Stop )                         \
    for ( i = Start; (i < Stop) && (((Entry) = Vec_BitEntry(vVec, i)), 1); i++ )
#define Vec_BitForEachEntryReverse( vVec, pEntry, i )                                       \
    for ( i = Vec_BitSize(vVec) - 1; (i >= 0) && (((pEntry) = Vec_BitEntry(vVec, i)), 1); i-- )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Bit_t * Vec_BitAlloc( int nCap )
{
    Vec_Bit_t * p;
    nCap = (nCap >> 5) + ((nCap & 31) > 0);
    p = ABC_ALLOC( Vec_Bit_t, 1 );
    p->nSize  = 0;
    p->nCap   = nCap * 32;
    p->pArray = nCap? ABC_ALLOC( int, nCap ) : NULL;
    return p;
}

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given size and cleans it.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Bit_t * Vec_BitStart( int nSize )
{
    Vec_Bit_t * p;
    nSize = (nSize >> 5) + ((nSize & 31) > 0);
    p = Vec_BitAlloc( nSize * 32 );
    p->nSize = nSize * 32;
    memset( p->pArray, 0, sizeof(int) * (size_t)nSize );
    return p;
}

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given size and cleans it.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Bit_t * Vec_BitStartFull( int nSize )
{
    Vec_Bit_t * p;
    nSize = (nSize >> 5) + ((nSize & 31) > 0);
    p = Vec_BitAlloc( nSize * 32 );
    p->nSize = nSize * 32;
    memset( p->pArray, 0xff, sizeof(int) * (size_t)nSize );
    return p;
}

/**Function*************************************************************

  Synopsis    [Duplicates the integer array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Bit_t * Vec_BitDup( Vec_Bit_t * pVec )
{
    Vec_Bit_t * p;
    assert( (pVec->nSize & 31) == 0 );
    p = ABC_ALLOC( Vec_Bit_t, 1 );
    p->nSize  = pVec->nSize;
    p->nCap   = pVec->nSize;
    p->pArray = p->nCap? ABC_ALLOC( int, p->nCap >> 5 ) : NULL;
    memcpy( p->pArray, pVec->pArray, sizeof(int) * (size_t)(p->nCap >> 5) );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_BitFree( Vec_Bit_t * p )
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
static inline void Vec_BitFreeP( Vec_Bit_t ** p )
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
static inline int * Vec_BitReleaseArray( Vec_Bit_t * p )
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
static inline int * Vec_BitArray( Vec_Bit_t * p )
{
    return p->pArray;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_BitSize( Vec_Bit_t * p )
{
    return p->nSize;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_BitCap( Vec_Bit_t * p )
{
    return p->nCap;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline double Vec_BitMemory( Vec_Bit_t * p )
{
    return !p ? 0.0 : 1.0 * sizeof(int) * p->nCap + sizeof(Vec_Bit_t);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_BitEntry( Vec_Bit_t * p, int i )
{
    assert( i >= 0 && i < p->nSize );
    return (p->pArray[i >> 5] >> (i & 31)) & 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_BitWriteEntry( Vec_Bit_t * p, int i, int Entry )
{
    assert( i >= 0 && i < p->nSize );
    if ( Entry == 1 )
        p->pArray[i >> 5] |=  (1 << (i & 31));
    else if ( Entry == 0 )
        p->pArray[i >> 5] &= ~(1 << (i & 31));
    else assert(0);
}
static inline int Vec_BitAddEntry( Vec_Bit_t * p, int i )
{
    if ( Vec_BitEntry(p, i) )
        return 1;
    Vec_BitWriteEntry( p, i, 1 );
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_BitEntryLast( Vec_Bit_t * p )
{
    assert( p->nSize > 0 );
    return Vec_BitEntry( p, p->nSize-1 );
}

/**Function*************************************************************

  Synopsis    [Resizes the vector to the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_BitGrow( Vec_Bit_t * p, int nCapMin )
{
    if ( p->nCap >= nCapMin )
        return;
    nCapMin = (nCapMin >> 5) + ((nCapMin & 31) > 0);
    p->pArray = ABC_REALLOC( int, p->pArray, nCapMin ); 
    assert( p->pArray );
    p->nCap   = nCapMin * 32;
}

/**Function*************************************************************

  Synopsis    [Fills the vector with given number of entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_BitFill( Vec_Bit_t * p, int nSize, int Fill )
{
    int i;
    Vec_BitGrow( p, nSize );
    nSize = (nSize >> 5) + ((nSize & 31) > 0);
    if ( Fill == 0 )
    {
        for ( i = 0; i < nSize; i++ )
            p->pArray[i] = 0;
    }
    else if ( Fill == 1 )
    {
        for ( i = 0; i < nSize; i++ )
            p->pArray[i] = ~0;
    }
    else assert( 0 );
    p->nSize = nSize * 32;
}

/**Function*************************************************************

  Synopsis    [Fills the vector with given number of entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_BitFillExtra( Vec_Bit_t * p, int nSize, int Fill )
{
    int i;
    if ( nSize <= p->nSize )
        return;
    if ( nSize > 2 * p->nCap )
        Vec_BitGrow( p, nSize );
    else if ( nSize > p->nCap )
        Vec_BitGrow( p, 2 * p->nCap );

    assert( p->nSize < nSize );
    if ( (p->nSize >> 5) == (nSize >> 5) )
    {
        unsigned Mask = (~(~0 << (nSize-p->nSize)) << p->nSize);
        if ( Fill == 1 )
            p->pArray[nSize >> 5] |= Mask;
        else if ( Fill == 0 )
            p->pArray[nSize >> 5] &= ~Mask;
        else assert( 0 );
    }
    else
    {
        unsigned Mask1 = (p->nSize & 31) ? ~0 << (p->nSize & 31) : 0;
        unsigned Mask2 = (nSize & 31)    ? ~(~0 << (nSize & 31)) : 0;
        int w1 = (p->nSize >> 5);
        int w2 = (nSize >> 5);
        if ( Fill == 1 )
        {
            p->pArray[w1] |= Mask1;
            p->pArray[w2] |= Mask2;
            for ( i = w1 + 1; i < w2; i++ )
                p->pArray[i] = ~0;
        }
        else if ( Fill == 0 )
        {
            p->pArray[w1] &= ~Mask1;
            p->pArray[w2] &= ~Mask2;
            for ( i = w1 + 1; i < w2; i++ )
                p->pArray[i] = 0;
        }
        else assert( 0 );
    }
    p->nSize = nSize;
}

/**Function*************************************************************

  Synopsis    [Returns the entry even if the place not exist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_BitGetEntry( Vec_Bit_t * p, int i )
{
    Vec_BitFillExtra( p, i + 1, 0 );
    return Vec_BitEntry( p, i );
}

/**Function*************************************************************

  Synopsis    [Inserts the entry even if the place does not exist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_BitSetEntry( Vec_Bit_t * p, int i, int Entry )
{
    Vec_BitFillExtra( p, i + 1, 0 );
    Vec_BitWriteEntry( p, i, Entry );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_BitShrink( Vec_Bit_t * p, int nSizeNew )
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
static inline void Vec_BitClear( Vec_Bit_t * p )
{
    p->nSize = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_BitPush( Vec_Bit_t * p, int Entry )
{
    if ( p->nSize == p->nCap )
    {
        if ( p->nCap < 16 )
            Vec_BitGrow( p, 16 );
        else
            Vec_BitGrow( p, 2 * p->nCap );
    }
    if ( Entry == 1 )
        p->pArray[p->nSize >> 5] |=  (1 << (p->nSize & 31));
    else if ( Entry == 0 )
        p->pArray[p->nSize >> 5] &= ~(1 << (p->nSize & 31));
    else assert( 0 );
    p->nSize++;
}

/**Function*************************************************************

  Synopsis    [Returns the last entry and removes it from the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_BitPop( Vec_Bit_t * p )
{
    int Entry;
    assert( p->nSize > 0 );
    Entry = Vec_BitEntryLast( p );
    p->nSize--;
    return Entry;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_BitCountWord( unsigned uWord )
{
    uWord = (uWord & 0x55555555) + ((uWord>>1) & 0x55555555);
    uWord = (uWord & 0x33333333) + ((uWord>>2) & 0x33333333);
    uWord = (uWord & 0x0F0F0F0F) + ((uWord>>4) & 0x0F0F0F0F);
    uWord = (uWord & 0x00FF00FF) + ((uWord>>8) & 0x00FF00FF);
    return  (uWord & 0x0000FFFF) + (uWord>>16);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_BitCount( Vec_Bit_t * p ) 
{
    unsigned * pArray = (unsigned *)p->pArray;
    int nWords = (p->nSize >> 5) + ((p->nSize & 31) > 0);
    int i, Counter = 0;
    if ( p->nSize & 31 )
    {
        assert( nWords > 0 );
        for ( i = 0; i < nWords-1; i++ )
            Counter += Vec_BitCountWord( pArray[i] );
        Counter += Vec_BitCountWord( pArray[i] & ~(~0 << (p->nSize & 31)) );
    }
    else
    {
        for ( i = 0; i < nWords; i++ )
            Counter += Vec_BitCountWord( pArray[i] );
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_BitReset( Vec_Bit_t * p ) 
{
    int i, nWords = (p->nSize >> 5) + ((p->nSize & 31) > 0);
    for ( i = 0; i < nWords; i++ )
        p->pArray[i] = 0;
}

ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

