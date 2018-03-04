/**CFile****************************************************************

  FileName    [mem2.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Memory management.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: mem2.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__aig__mem__mem2_h
#define ABC__aig__mem__mem2_h

#include "misc/vec/vec.h"

ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Mmr_Flex_t_     Mmr_Flex_t;     
typedef struct Mmr_Fixed_t_    Mmr_Fixed_t;    
typedef struct Mmr_Step_t_     Mmr_Step_t;     

struct Mmr_Flex_t_
{
    int           nPageBase;     // log2 page size in words
    int           PageMask;      // page mask
    int           nEntries;      // entries allocated
    int           nEntriesMax;   // max number of enries used
    int           iNext;         // next word to be used
    Vec_Ptr_t     vPages;        // memory pages
};

struct Mmr_Fixed_t_
{
    int           nPageBase;     // log2 page size in words
    int           PageMask;      // page mask
    int           nEntryWords;   // entry size in words
    int           nEntries;      // entries allocated
    int           nEntriesMax;   // max number of enries used
    Vec_Ptr_t     vPages;        // memory pages
    Vec_Int_t     vFrees;        // free entries
};

struct Mmr_Step_t_
{
    int           nBits;         // the number of bits
    int           uMask;         // the number of managers minus 1
    int           nEntries;      // the number of entries
    int           nEntriesMax;   // the max number of entries
    int           nEntriesAll;   // the total number of entries
    Mmr_Fixed_t   pMems[0];      // memory managers: 2^0 words, 2^1 words, etc
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
 
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Mmr_Flex_t * Mmr_FlexStart( int nPageBase )
{
    Mmr_Flex_t * p;
    p = ABC_CALLOC( Mmr_Flex_t, 1 );
    p->nPageBase = nPageBase;
    p->PageMask  = (1 << nPageBase) - 1;
    p->iNext     = (1 << nPageBase);
    return p;
}
static inline void Mmr_FlexStop( Mmr_Flex_t * p )
{
    word * pPage;
    int i;
    if ( 0 && Vec_PtrSize(&p->vPages) )
        printf( "Using %3d pages of %6d words each with %6d entries (max = %6d). Total memory = %5.2f MB.\n", 
            Vec_PtrSize(&p->vPages), p->nPageBase ? 1 << p->nPageBase : 0, p->nEntries, p->nEntriesMax, 
            1.0 * Vec_PtrSize(&p->vPages) * (1 << p->nPageBase) * 8 / (1 << 20) );
    Vec_PtrForEachEntry( word *, &p->vPages, pPage, i )
        ABC_FREE( pPage );
    ABC_FREE( p->vPages.pArray );
    ABC_FREE( p );
}
static inline word * Mmr_FlexEntry( Mmr_Flex_t * p, int h )
{
    assert( h > 0 && h < p->iNext );
    return (word *)Vec_PtrEntry(&p->vPages, (h >> p->nPageBase)) + (h & p->PageMask);
}
static inline int Mmr_FlexFetch( Mmr_Flex_t * p, int nWords )
{
    int hEntry;
    assert( nWords > 0 && nWords < p->PageMask );
    if ( p->iNext + nWords >= p->PageMask )
    {
        Vec_PtrPush( &p->vPages, ABC_FALLOC( word, p->PageMask + 1 ) );
        p->iNext = 1;
    }
    hEntry = ((Vec_PtrSize(&p->vPages) - 1) << p->nPageBase) | p->iNext;
    p->iNext += nWords;
    p->nEntries++;
    p->nEntriesMax = Abc_MaxInt( p->nEntriesMax, p->nEntries );
    return hEntry;
}
static inline void Mmr_FlexRelease( Mmr_Flex_t * p, int h )
{
    assert( h > 0 && h < p->iNext );
    if ( (h >> p->nPageBase) && Vec_PtrEntry(&p->vPages, (h >> p->nPageBase) - 1) )
    {
        word * pPage = (word *)Vec_PtrEntry(&p->vPages, (h >> p->nPageBase) - 1);
        Vec_PtrWriteEntry( &p->vPages, (h >> p->nPageBase) - 1, NULL );
        ABC_FREE( pPage );
    }
}

 
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Mmr_FixedCreate( Mmr_Fixed_t * p, int nPageBase, int nEntryWords )
{
    assert( nEntryWords > 0 && nEntryWords < (1 << nPageBase) );
    p->nPageBase   = nPageBase;
    p->PageMask    = (1 << nPageBase) - 1;
    p->nEntryWords = nEntryWords;
}
static inline Mmr_Fixed_t * Mmr_FixedStart( int nPageBase, int nEntryWords )
{
    Mmr_Fixed_t * p = ABC_CALLOC( Mmr_Fixed_t, 1 );
    Mmr_FixedCreate( p, nPageBase, nEntryWords );
    return p;
}
static inline void Mmr_FixedStop( Mmr_Fixed_t * p, int fFreeLast )
{
    word * pPage;
    int i;
    if ( 0 && Vec_PtrSize(&p->vPages) )
        printf( "Using %3d pages of %6d words each with %6d entries (max = %6d) of size %d. Total memory = %5.2f MB.\n", 
            Vec_PtrSize(&p->vPages), p->nPageBase ? 1 << p->nPageBase : 0, p->nEntries, p->nEntriesMax, p->nEntryWords,
            1.0 * Vec_PtrSize(&p->vPages) * (1 << p->nPageBase) * 8 / (1 << 20) );
    Vec_PtrForEachEntry( word *, &p->vPages, pPage, i )
        ABC_FREE( pPage );
    ABC_FREE( p->vPages.pArray );
    ABC_FREE( p->vFrees.pArray );
    if ( fFreeLast )
        ABC_FREE( p );
}
static inline word * Mmr_FixedEntry( Mmr_Fixed_t * p, int h )
{
    assert( h > 0 && h < (Vec_PtrSize(&p->vPages) << p->nPageBase) );
    return (word *)Vec_PtrEntry(&p->vPages, (h >> p->nPageBase)) + (h & p->PageMask);
}
static inline int Mmr_FixedFetch( Mmr_Fixed_t * p )
{
    if ( Vec_IntSize(&p->vFrees) == 0 )
    {
        int i, hEntry = Vec_PtrSize(&p->vPages) << p->nPageBase;
        Vec_PtrPush( &p->vPages, ABC_FALLOC( word, p->PageMask + 1 ) );
        for ( i = 1; i + p->nEntryWords <= p->PageMask; i += p->nEntryWords )
            Vec_IntPush( &p->vFrees, hEntry | i );
        Vec_IntReverseOrder( &p->vFrees );
    }
    p->nEntries++;
    p->nEntriesMax = Abc_MaxInt( p->nEntriesMax, p->nEntries );
    return Vec_IntPop( &p->vFrees );
}
static inline void Mmr_FixedRecycle( Mmr_Fixed_t * p, int h )
{
    p->nEntries--;
    memset( Mmr_FixedEntry(p, h), 0xFF, sizeof(word) * p->nEntryWords );
    Vec_IntPush( &p->vFrees, h );
}
static inline int Mmr_FixedMemory( Mmr_Fixed_t * p )
{
    return Vec_PtrSize(&p->vPages) * (p->PageMask + 1);
}

 
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Mmr_Step_t * Mmr_StepStart( int nPageBase, int nWordBase )
{
    char * pMemory = ABC_CALLOC( char, sizeof(Mmr_Step_t) + sizeof(Mmr_Fixed_t) * (1 << nWordBase) );
    Mmr_Step_t * p = (Mmr_Step_t *)pMemory;
    int i;
    p->nBits = nWordBase;
    p->uMask = (1 << nWordBase) - 1;
    for ( i = 1; i <= p->uMask; i++ )
        Mmr_FixedCreate( p->pMems + i, nPageBase, i );
    return p;
}
static inline void Mmr_StepStop( Mmr_Step_t * p )
{
    int i;
    for ( i = 0; i <= p->uMask; i++ )
        Mmr_FixedStop( p->pMems + i, 0 );
    ABC_FREE( p );
}
static inline word * Mmr_StepEntry( Mmr_Step_t * p, int h )
{
    assert( (h & p->uMask) > 0 );
    return Mmr_FixedEntry( p->pMems + (h & p->uMask), (h >> p->nBits) );
}
static inline int Mmr_StepFetch( Mmr_Step_t * p, int nWords )
{
    assert( nWords > 0 && nWords <= p->uMask );
    p->nEntries++;
    p->nEntriesAll++;
    p->nEntriesMax = Abc_MaxInt( p->nEntriesMax, p->nEntries );
    return (Mmr_FixedFetch(p->pMems + nWords) << p->nBits) | nWords;
}
static inline void Mmr_StepRecycle( Mmr_Step_t * p, int h )
{
    p->nEntries--;
    Mmr_FixedRecycle( p->pMems + (h & p->uMask), (h >> p->nBits) );
}
static inline int Mmr_StepMemory( Mmr_Step_t * p )
{
    int i, Mem = 0;
    for ( i = 1; i <= p->uMask; i++ )
        Mem += Mmr_FixedMemory( p->pMems + i );
    return Mem;
}


ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


