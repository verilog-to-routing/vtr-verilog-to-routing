/**CFile****************************************************************

  FileName    [satMem.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT solver.]

  Synopsis    [Memory management.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: satMem.h,v 1.0 2004/01/01 1:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__sat__bsat__satMem_h
#define ABC__sat__bsat__satMem_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "misc/util/abc_global.h"

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

//#define LEARNT_MAX_START_DEFAULT  0
#define LEARNT_MAX_START_DEFAULT  10000
#define LEARNT_MAX_INCRE_DEFAULT   1000
#define LEARNT_MAX_RATIO_DEFAULT     50

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
 
//=================================================================================================
// Clause datatype + minor functions:

typedef struct clause_t clause;
struct clause_t
{
    unsigned   lrn   :   1;
    unsigned   mark  :   1;
    unsigned   partA :   1;
    unsigned   lbd   :   8;
    unsigned   size  :  21;
    lit        lits[0];
};

// learned clauses have "hidden" literal (c->lits[c->size]) to store clause ID

// data-structure for logging entries
// memory is allocated in 2^nPageSize word-sized pages
// the first 'word' of each page are stores the word limit

// although clause memory pieces are aligned to 64-bit words
// the integer clause handles are in terms of 32-bit unsigneds
// allowing for the first bit to be used for labeling 2-lit clauses


typedef struct Sat_Mem_t_ Sat_Mem_t;
struct Sat_Mem_t_
{
    int                 nEntries[2];  // entry count
    int                 BookMarkH[2]; // bookmarks for handles
    int                 BookMarkE[2]; // bookmarks for entries
    int                 iPage[2];     // current memory page
    int                 nPageSize;    // page log size in terms of ints
    unsigned            uPageMask;    // page mask
    unsigned            uLearnedMask; // learned mask
    int                 nPagesAlloc;  // page count allocated
    int **              pPages;       // page pointers
}; 

static inline int       Sat_MemLimit( int * p )                      { return p[0];                                 }
static inline int       Sat_MemIncLimit( int * p, int nInts )        { return p[0] += nInts;                        }
static inline void      Sat_MemWriteLimit( int * p, int nInts )      { p[0] = nInts;                                }

static inline int       Sat_MemHandPage( Sat_Mem_t * p, cla h )      { return h >> p->nPageSize;                    }
static inline int       Sat_MemHandShift( Sat_Mem_t * p, cla h )     { return h & p->uPageMask;                     }

static inline int       Sat_MemIntSize( int size, int lrn )          { return (size + 2 + lrn) & ~01;               }
static inline int       Sat_MemClauseSize( clause * p )              { return Sat_MemIntSize(p->size, p->lrn);      }
static inline int       Sat_MemClauseSize2( clause * p )             { return Sat_MemIntSize(p->size, 1);           }

//static inline clause *  Sat_MemClause( Sat_Mem_t * p, int i, int k ) { assert(i <= p->iPage[i&1] && k <= Sat_MemLimit(p->pPages[i]));  return (clause *)(p->pPages[i] + k ); }
static inline clause *  Sat_MemClause( Sat_Mem_t * p, int i, int k ) { assert( k ); return (clause *)(p->pPages[i] + k);         }
//static inline clause *  Sat_MemClauseHand( Sat_Mem_t * p, cla h )    { assert(Sat_MemHandPage(p, h) <= p->iPage[(h & p->uLearnedMask) > 0]); assert(Sat_MemHandShift(p, h) >= 2 && Sat_MemHandShift(p, h) < (int)p->uLearnedMask); return Sat_MemClause( p, Sat_MemHandPage(p, h), Sat_MemHandShift(p, h) );     }
static inline clause *  Sat_MemClauseHand( Sat_Mem_t * p, cla h )    { return h ? Sat_MemClause( p, Sat_MemHandPage(p, h), Sat_MemHandShift(p, h) ) : NULL;                  }
static inline int       Sat_MemEntryNum( Sat_Mem_t * p, int lrn )    { return p->nEntries[lrn];                                                                              }

static inline cla       Sat_MemHand( Sat_Mem_t * p, int i, int k )   { return (i << p->nPageSize) | k;                                                                       }
static inline cla       Sat_MemHandCurrent( Sat_Mem_t * p, int lrn ) { return (p->iPage[lrn] << p->nPageSize) | Sat_MemLimit( p->pPages[p->iPage[lrn]] );                    }

static inline int       Sat_MemClauseUsed( Sat_Mem_t * p, cla h )    { return h < p->BookMarkH[(h & p->uLearnedMask) > 0];                                                   }

static inline double    Sat_MemMemoryHand( Sat_Mem_t * p, cla h )    { return 1.0 * ((Sat_MemHandPage(p, h) + 2)/2 * (1 << (p->nPageSize+2)) + Sat_MemHandShift(p, h) * 4);  }
static inline double    Sat_MemMemoryUsed( Sat_Mem_t * p, int lrn )  { return Sat_MemMemoryHand( p, Sat_MemHandCurrent(p, lrn) );                                            }
static inline double    Sat_MemMemoryAllUsed( Sat_Mem_t * p )        { return Sat_MemMemoryUsed( p, 0 ) + Sat_MemMemoryUsed( p, 1 );                                         }
static inline double    Sat_MemMemoryAll( Sat_Mem_t * p )            { return 1.0 * (p->iPage[0] + p->iPage[1] + 2) * (1 << (p->nPageSize+2));                               }

// p is memory storage
// c is clause pointer
// i is page number
// k is page offset

// print problem clauses NOT in proof mode
#define Sat_MemForEachClause( p, c, i, k )      \
    for ( i = 0; i <= p->iPage[0]; i += 2 )     \
        for ( k = 2; k < Sat_MemLimit(p->pPages[i]) && ((c) = Sat_MemClause( p, i, k )); k += Sat_MemClauseSize(c) ) if ( i == 0 && k == 2 ) {} else

// print problem clauses in proof mode
#define Sat_MemForEachClause2( p, c, i, k )      \
    for ( i = 0; i <= p->iPage[0]; i += 2 )     \
        for ( k = 2; k < Sat_MemLimit(p->pPages[i]) && ((c) = Sat_MemClause( p, i, k )); k += Sat_MemClauseSize2(c) ) if ( i == 0 && k == 2 ) {} else

#define Sat_MemForEachLearned( p, c, i, k )     \
    for ( i = 1; i <= p->iPage[1]; i += 2 )     \
        for ( k = 2; k < Sat_MemLimit(p->pPages[i]) && ((c) = Sat_MemClause( p, i, k )); k += Sat_MemClauseSize(c) )

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////

static inline int      clause_from_lit( lit l )                     { return l + l + 1;                            }
static inline int      clause_is_lit( cla h )                       { return (h & 1);                              }
static inline lit      clause_read_lit( cla h )                     { return (lit)(h >> 1);                        }

static inline int      clause_learnt_h( Sat_Mem_t * p, cla h )      { return (h & p->uLearnedMask) > 0;            }
static inline int      clause_learnt( clause * c )                  { return c->lrn;                               }
static inline int      clause_id( clause * c )                      { return c->lits[c->size];                     }
static inline void     clause_set_id( clause * c, int id )          { c->lits[c->size] = id;                       }
static inline int      clause_size( clause * c )                    { return c->size;                              }
static inline lit *    clause_begin( clause * c )                   { return c->lits;                              }
static inline lit *    clause_end( clause * c )                     { return c->lits + c->size;                    }
static inline void     clause_print_( clause * c )               
{ 
    int i;
    printf( "{ " );
    for ( i = 0; i < clause_size(c); i++ )
        printf( "%d ", (clause_begin(c)[i] & 1)? -(clause_begin(c)[i] >> 1) : clause_begin(c)[i] >> 1 );
    printf( "}\n" );
}

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DECLARATIONS                        ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocating vector.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Sat_MemCountL( Sat_Mem_t * p )
{
    clause * c;
    int i, k, Count = 0;
    Sat_MemForEachLearned( p, c, i, k )
        Count++;
    return Count;
}

/**Function*************************************************************

  Synopsis    [Allocating vector.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Sat_MemAlloc_( Sat_Mem_t * p, int nPageSize )
{
    assert( nPageSize > 8 && nPageSize < 32 );
    memset( p, 0, sizeof(Sat_Mem_t) );
    p->nPageSize    = nPageSize;
    p->uLearnedMask = (unsigned)(1 << nPageSize);
    p->uPageMask    = (unsigned)((1 << nPageSize) - 1);
    p->nPagesAlloc  = 256;
    p->pPages       = ABC_CALLOC( int *, p->nPagesAlloc );
    p->pPages[0]    = ABC_ALLOC( int, (int)(((word)1) << p->nPageSize) );
    p->pPages[1]    = ABC_ALLOC( int, (int)(((word)1) << p->nPageSize) );
    p->iPage[0]     = 0;
    p->iPage[1]     = 1;
    Sat_MemWriteLimit( p->pPages[0], 2 );
    Sat_MemWriteLimit( p->pPages[1], 2 );
}
static inline Sat_Mem_t * Sat_MemAlloc( int nPageSize )
{
    Sat_Mem_t * p;
    p = ABC_CALLOC( Sat_Mem_t, 1 );
    Sat_MemAlloc_( p, nPageSize );
    return p;
}

/**Function*************************************************************

  Synopsis    [Resetting vector.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Sat_MemRestart( Sat_Mem_t * p )
{
    p->nEntries[0]  = 0;
    p->nEntries[1]  = 0;
    p->iPage[0]     = 0;
    p->iPage[1]     = 1;
    Sat_MemWriteLimit( p->pPages[0], 2 );
    Sat_MemWriteLimit( p->pPages[1], 2 );
}

/**Function*************************************************************

  Synopsis    [Sets the bookmark.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Sat_MemBookMark( Sat_Mem_t * p )
{
    p->BookMarkE[0] = p->nEntries[0];
    p->BookMarkE[1] = p->nEntries[1];
    p->BookMarkH[0] = Sat_MemHandCurrent( p, 0 );
    p->BookMarkH[1] = Sat_MemHandCurrent( p, 1 );
}
static inline void Sat_MemRollBack( Sat_Mem_t * p )
{
    p->nEntries[0]  = p->BookMarkE[0];
    p->nEntries[1]  = p->BookMarkE[1];
    p->iPage[0]     = Sat_MemHandPage( p, p->BookMarkH[0] );
    p->iPage[1]     = Sat_MemHandPage( p, p->BookMarkH[1] );
    Sat_MemWriteLimit( p->pPages[p->iPage[0]], Sat_MemHandShift( p, p->BookMarkH[0] ) );
    Sat_MemWriteLimit( p->pPages[p->iPage[1]], Sat_MemHandShift( p, p->BookMarkH[1] ) );
}

/**Function*************************************************************

  Synopsis    [Freeing vector.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Sat_MemFree_( Sat_Mem_t * p )
{
    int i;
    for ( i = 0; i < p->nPagesAlloc; i++ )
        ABC_FREE( p->pPages[i] );
    ABC_FREE( p->pPages );
}
static inline void Sat_MemFree( Sat_Mem_t * p )
{
    Sat_MemFree_( p );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Creates new clause.]

  Description [The resulting clause is fully initialized.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Sat_MemAppend( Sat_Mem_t * p, int * pArray, int nSize, int lrn, int fPlus1 )
{
    clause * c;
    int * pPage = p->pPages[p->iPage[lrn]];
    int nInts = Sat_MemIntSize( nSize, lrn | fPlus1 );
    assert( nInts + 3 < (1 << p->nPageSize) );
    // need two extra at the begining of the page and one extra in the end
    if ( Sat_MemLimit(pPage) + nInts + 2 >= (1 << p->nPageSize) )
    { 
        p->iPage[lrn] += 2;
        if ( p->iPage[lrn] >= p->nPagesAlloc )
        {
            p->pPages = ABC_REALLOC( int *, p->pPages, p->nPagesAlloc * 2 );
            memset( p->pPages + p->nPagesAlloc, 0, sizeof(int *) * p->nPagesAlloc );
            p->nPagesAlloc *= 2;
        }
        if ( p->pPages[p->iPage[lrn]] == NULL )
            p->pPages[p->iPage[lrn]] = ABC_ALLOC( int, (int)(((word)1) << p->nPageSize) );
        pPage = p->pPages[p->iPage[lrn]];
        Sat_MemWriteLimit( pPage, 2 );
    }
    pPage[Sat_MemLimit(pPage)] = 0;
    c = (clause *)(pPage + Sat_MemLimit(pPage));
    c->size = nSize;
    c->lrn = lrn;
    if ( pArray )
        memcpy( c->lits, pArray, sizeof(int) * nSize );
    if ( lrn | fPlus1 )
        c->lits[c->size] = p->nEntries[lrn];
    p->nEntries[lrn]++;
    Sat_MemIncLimit( pPage, nInts );
    return Sat_MemHandCurrent(p, lrn) - nInts;
}

/**Function*************************************************************

  Synopsis    [Shrinking vector size.]

  Description []

  SideEffects [This procedure does not update the number of entries.]

  SeeAlso     []

***********************************************************************/
static inline void Sat_MemShrink( Sat_Mem_t * p, int h, int lrn )
{
    assert( clause_learnt_h(p, h) == lrn );
    assert( h && h <= Sat_MemHandCurrent(p, lrn) );
    p->iPage[lrn] = Sat_MemHandPage(p, h);
    Sat_MemWriteLimit( p->pPages[p->iPage[lrn]], Sat_MemHandShift(p, h) );
}


/**Function*************************************************************

  Synopsis    [Compacts learned clauses by removing marked entries.]

  Description [Returns the number of remaining entries.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Sat_MemCompactLearned( Sat_Mem_t * p, int fDoMove )
{
    clause * c, * cPivot = NULL;
    int i, k, iNew = 1, kNew = 2, nInts, fStartLooking, Counter = 0;
    int hLimit = Sat_MemHandCurrent(p, 1);
    if ( hLimit == Sat_MemHand(p, 1, 2) )
        return 0;
    if ( fDoMove && p->BookMarkH[1] )
    {
        // move the pivot
        assert( p->BookMarkH[1] >= Sat_MemHand(p, 1, 2) && p->BookMarkH[1] <= hLimit );
        // get the pivot and remember it may be pointed offlimit
        cPivot = Sat_MemClauseHand( p, p->BookMarkH[1] );
        if ( p->BookMarkH[1] < hLimit && !cPivot->mark )
        {
            p->BookMarkH[1] = cPivot->lits[cPivot->size];
            cPivot = NULL;
        }
        // else find the next used clause after cPivot
    }
    // iterate through the learned clauses
    fStartLooking = 0;
    Sat_MemForEachLearned( p, c, i, k )
    {
        assert( c->lrn );
        // skip marked entry
        if ( c->mark )
        {
            // if pivot is a marked clause, start looking for the next non-marked one
            if ( cPivot && cPivot == c )
            {
                fStartLooking = 1;
                cPivot = NULL;
            }
            continue;
        }
        // if we started looking before, we found it!
        if ( fStartLooking )
        {
            fStartLooking = 0;
            p->BookMarkH[1] = c->lits[c->size];
        }
        // compute entry size
        nInts = Sat_MemClauseSize(c);
        assert( !(nInts & 1) );
        // check if we need to scroll to the next page
        if ( kNew + nInts >= (1 << p->nPageSize) )
        {
            // set the limit of the current page
            if ( fDoMove )
                Sat_MemWriteLimit( p->pPages[iNew], kNew );
            // move writing position to the new page
            iNew += 2;
            kNew = 2;
        }
        if ( fDoMove )
        {
            // make sure the result is the same as previous dry run
            assert( c->lits[c->size] == Sat_MemHand(p, iNew, kNew) );
            // only copy the clause if it has changed
            if ( i != iNew || k != kNew )
            {
                memmove( p->pPages[iNew] + kNew, c, sizeof(int) * nInts );
//                c = Sat_MemClause( p, iNew, kNew ); // assersions do not hold during dry run
                c = (clause *)(p->pPages[iNew] + kNew);
                assert( nInts == Sat_MemClauseSize(c) );
            }
            // set the new ID value
            c->lits[c->size] = Counter;
        }
        else // remember the address of the clause in the new location
            c->lits[c->size] = Sat_MemHand(p, iNew, kNew);
        // update writing position
        kNew += nInts;
        assert( iNew <= i && kNew < (1 << p->nPageSize) );
        // update counter
        Counter++;
    }
    if ( fDoMove )
    {
        // update the counter
        p->nEntries[1] = Counter;
        // update the page count
        p->iPage[1] = iNew;
        // set the limit of the last page
        Sat_MemWriteLimit( p->pPages[iNew], kNew );
        // check if the pivot need to be updated
        if ( p->BookMarkH[1] )
        {
            if ( cPivot )
            {
                p->BookMarkH[1] = Sat_MemHandCurrent(p, 1);
                p->BookMarkE[1] = p->nEntries[1];
            }
            else
                p->BookMarkE[1] = clause_id(Sat_MemClauseHand( p, p->BookMarkH[1] ));
        }

    }
    return Counter;
}


ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

