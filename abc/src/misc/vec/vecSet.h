/**CFile****************************************************************

  FileName    [vecSet.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT solvers.]

  Synopsis    [Multi-page dynamic array.]

  Author      [Alan Mishchenko]

  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: vecSet.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__sat__bsat__vecSet_h
#define ABC__sat__bsat__vecSet_h


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

// data-structure for logging entries
// memory is allocated in 2^nPageSize word-sized pages
// the first two 'words' of each page are used for bookkeeping
// the first 'word' of bookkeeping data stores the word limit
// the second 'word' of bookkeeping data stores the shadow word limit
// (the shadow word limit is only used during garbage collection)

typedef struct Vec_Set_t_ Vec_Set_t;
struct Vec_Set_t_
{
    int               nPageSize;    // page size
    unsigned          uPageMask;    // page mask
    int               nEntries;     // entry count
    int               iPage;        // current page
    int               iPageS;       // shadow page
    int               nPagesAlloc;  // page count allocated
    word **           pPages;       // page pointers
}; 

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

static inline int     Vec_SetHandPage( Vec_Set_t * p, int h )    { return h >> p->nPageSize;      }
static inline int     Vec_SetHandShift( Vec_Set_t * p, int h )   { return h & p->uPageMask;       }
static inline int     Vec_SetWordNum( int nSize )                { return (nSize + 1) >> 1;       }

//static inline word *  Vec_SetEntry( Vec_Set_t * p, int h )       { assert(Vec_SetHandPage(p, h) >= 0 && Vec_SetHandPage(p, h) <= p->iPage); assert(Vec_SetHandShift(p, h) >= 2 && Vec_SetHandShift(p, h) < (1 << p->nPageSize)); return p->pPages[Vec_SetHandPage(p, h)] + Vec_SetHandShift(p, h);     }
static inline word *  Vec_SetEntry( Vec_Set_t * p, int h )       { return p->pPages[Vec_SetHandPage(p, h)] + Vec_SetHandShift(p, h);                     }
static inline int     Vec_SetEntryNum( Vec_Set_t * p )           { return p->nEntries;            }
static inline void    Vec_SetWriteEntryNum( Vec_Set_t * p, int i){ p->nEntries = i;               }

static inline int     Vec_SetLimit( word * p )                   { return p[0];                   }
static inline int     Vec_SetLimitS( word * p )                  { return p[1];                   }

static inline int     Vec_SetIncLimit( word * p, int nWords )    { return p[0] += nWords;         }
static inline int     Vec_SetIncLimitS( word * p, int nWords )   { return p[1] += nWords;         }

static inline void    Vec_SetWriteLimit( word * p, int nWords )  { p[0] = nWords;                 }
static inline void    Vec_SetWriteLimitS( word * p, int nWords ) { p[1] = nWords;                 }

static inline int     Vec_SetHandCurrent( Vec_Set_t * p )        { return (p->iPage << p->nPageSize)  + Vec_SetLimit(p->pPages[p->iPage]);               }
static inline int     Vec_SetHandCurrentS( Vec_Set_t * p )       { return (p->iPageS << p->nPageSize) + Vec_SetLimitS(p->pPages[p->iPageS]);             }

static inline int     Vec_SetHandMemory( Vec_Set_t * p, int h )  { return Vec_SetHandPage(p, h) * (1 << (p->nPageSize+3)) + Vec_SetHandShift(p, h) * 8;  }
static inline int     Vec_SetMemory( Vec_Set_t * p )             { return Vec_SetHandMemory(p, Vec_SetHandCurrent(p));                                   }
static inline int     Vec_SetMemoryS( Vec_Set_t * p )            { return Vec_SetHandMemory(p, Vec_SetHandCurrentS(p));                                  }
static inline int     Vec_SetMemoryAll( Vec_Set_t * p )          { return (p->iPage+1) * (1 << (p->nPageSize+3));                                        }

// Type is the Set type
// pVec is vector of set
// nSize should be given by the user
// pSet is the pointer to the set
// p (page) and s (shift) are variables used here
#define Vec_SetForEachEntry( Type, pVec, nSize, pSet, p, s )   \
    for ( p = 0; p <= pVec->iPage; p++ )                       \
        for ( s = 2; s < Vec_SetLimit(pVec->pPages[p]) && ((pSet) = (Type)(pVec->pPages[p] + (s))); s += nSize )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocating vector.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_SetAlloc_( Vec_Set_t * p, int nPageSize )
{
    assert( nPageSize > 8 );
    memset( p, 0, sizeof(Vec_Set_t) );
    p->nPageSize    = nPageSize;
    p->uPageMask    = (unsigned)((1 << nPageSize) - 1);
    p->nPagesAlloc  = 256;
    p->pPages       = ABC_CALLOC( word *, p->nPagesAlloc );
    p->pPages[0]    = ABC_ALLOC( word, (int)(((word)1) << p->nPageSize) );
    p->pPages[0][0] = ~0;
    p->pPages[0][1] = ~0;
    Vec_SetWriteLimit( p->pPages[0], 2 );
}
static inline Vec_Set_t * Vec_SetAlloc( int nPageSize )
{
    Vec_Set_t * p;
    p = ABC_CALLOC( Vec_Set_t, 1 );
    Vec_SetAlloc_( p, nPageSize );
    return p;
}

/**Function*************************************************************

  Synopsis    [Resetting vector.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_SetRestart( Vec_Set_t * p )
{
    p->nEntries     = 0;
    p->iPage        = 0;
    p->iPageS       = 0;
    p->pPages[0][0] = ~0;
    p->pPages[0][1] = ~0;
    Vec_SetWriteLimit( p->pPages[0], 2 );
}

/**Function*************************************************************

  Synopsis    [Freeing vector.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_SetFree_( Vec_Set_t * p )
{
    int i;
    if ( p == NULL ) return;
    for ( i = 0; i < p->nPagesAlloc; i++ )
        ABC_FREE( p->pPages[i] );
    ABC_FREE( p->pPages );
}
static inline void Vec_SetFree( Vec_Set_t * p )
{
    if ( p == NULL ) return;
    Vec_SetFree_( p );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Returns memory in bytes occupied by the vector.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline double Vec_ReportMemory( Vec_Set_t * p )
{
    double Mem = sizeof(Vec_Set_t);
    Mem += p->nPagesAlloc * sizeof(void *);
    Mem += sizeof(word) * (int)(((word)1) << p->nPageSize) * (1 + p->iPage);
    return Mem;
}

/**Function*************************************************************

  Synopsis    [Appending entries to vector.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_SetAppend( Vec_Set_t * p, int * pArray, int nSize )
{
    int nWords = Vec_SetWordNum( nSize );
    assert( nWords < (1 << p->nPageSize) );
    p->nEntries++;
    if ( Vec_SetLimit( p->pPages[p->iPage] ) + nWords >= (1 << p->nPageSize) )
    {
        if ( ++p->iPage == p->nPagesAlloc )
        {
            p->pPages = ABC_REALLOC( word *, p->pPages, p->nPagesAlloc * 2 );
            memset( p->pPages + p->nPagesAlloc, 0, sizeof(word *) * p->nPagesAlloc );
            p->nPagesAlloc *= 2;
        }
        if ( p->pPages[p->iPage] == NULL )
            p->pPages[p->iPage] = ABC_ALLOC( word, (int)(((word)1) << p->nPageSize) );
        Vec_SetWriteLimit( p->pPages[p->iPage], 2 );
        p->pPages[p->iPage][1] = ~0;
    }
    if ( pArray )
        memcpy( p->pPages[p->iPage] + Vec_SetLimit(p->pPages[p->iPage]), pArray, sizeof(int) * nSize );
    Vec_SetIncLimit( p->pPages[p->iPage], nWords );
    return Vec_SetHandCurrent(p) - nWords;
}
static inline int Vec_SetAppendS( Vec_Set_t * p, int nSize )
{
    int nWords = Vec_SetWordNum( nSize );
    assert( nWords < (1 << p->nPageSize) );
    if ( Vec_SetLimitS( p->pPages[p->iPageS] ) + nWords >= (1 << p->nPageSize) )
        Vec_SetWriteLimitS( p->pPages[++p->iPageS], 2 );
    Vec_SetIncLimitS( p->pPages[p->iPageS], nWords );
    return Vec_SetHandCurrentS(p) - nWords;
}
static inline int Vec_SetFetchH( Vec_Set_t * p, int nBytes )
{
    return Vec_SetAppend(p, NULL, (nBytes + 3) >> 2);
}
static inline void * Vec_SetFetch( Vec_Set_t * p, int nBytes )
{
    return (void *)Vec_SetEntry( p, Vec_SetFetchH(p, nBytes) );
}
static inline char * Vec_SetStrsav( Vec_Set_t * p, char * pName )
{
    char * pStr = (char *)Vec_SetFetch( p, (int)strlen(pName) + 1 );
    strcpy( pStr, pName );
    return pStr;
}

/**Function*************************************************************

  Synopsis    [Shrinking vector size.]

  Description []

  SideEffects [This procedure does not update the number of entries.]

  SeeAlso     []

***********************************************************************/
static inline void Vec_SetShrink( Vec_Set_t * p, int h )
{
    assert( h <= Vec_SetHandCurrent(p) );
    p->iPage = Vec_SetHandPage(p, h);
    Vec_SetWriteLimit( p->pPages[p->iPage], Vec_SetHandShift(p, h) );
}
static inline void Vec_SetShrinkS( Vec_Set_t * p, int h )
{
    assert( h <= Vec_SetHandCurrent(p) );
    p->iPageS = Vec_SetHandPage(p, h);
    Vec_SetWriteLimitS( p->pPages[p->iPageS], Vec_SetHandShift(p, h) );
}

static inline void Vec_SetShrinkLimits( Vec_Set_t * p )
{
    int i;
    for ( i = 0; i <= p->iPage; i++ )
        Vec_SetWriteLimit( p->pPages[i], Vec_SetLimitS(p->pPages[i]) );
}


ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

