/**CFile****************************************************************

  FileName    [abcExact.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Find minimum size networks with a SAT solver.]

  Author      [Mathias Soeken]

  Affiliation [EPFL]

  Date        [Ver. 1.0. Started - July 15, 2016.]

  Revision    [$Id: abcFanio.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

/* This implementation is based on Exercises 477 and 478 in
 * Donald E. Knuth TAOCP Fascicle 6 (Satisfiability) Section 7.2.2.2
 */

#include "base/abc/abc.h"

#include "aig/gia/gia.h"
#include "misc/util/utilTruth.h"
#include "misc/vec/vecInt.h"
#include "misc/vec/vecPtr.h"
#include "proof/cec/cec.h"
#include "sat/bsat/satSolver.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

/***********************************************************************

  Synopsis    [Some truth table helper functions.]

***********************************************************************/

static word s_Truths8[32] = {
    ABC_CONST(0xAAAAAAAAAAAAAAAA), ABC_CONST(0xAAAAAAAAAAAAAAAA), ABC_CONST(0xAAAAAAAAAAAAAAAA), ABC_CONST(0xAAAAAAAAAAAAAAAA),
    ABC_CONST(0xCCCCCCCCCCCCCCCC), ABC_CONST(0xCCCCCCCCCCCCCCCC), ABC_CONST(0xCCCCCCCCCCCCCCCC), ABC_CONST(0xCCCCCCCCCCCCCCCC),
    ABC_CONST(0xF0F0F0F0F0F0F0F0), ABC_CONST(0xF0F0F0F0F0F0F0F0), ABC_CONST(0xF0F0F0F0F0F0F0F0), ABC_CONST(0xF0F0F0F0F0F0F0F0),
    ABC_CONST(0xFF00FF00FF00FF00), ABC_CONST(0xFF00FF00FF00FF00), ABC_CONST(0xFF00FF00FF00FF00), ABC_CONST(0xFF00FF00FF00FF00),
    ABC_CONST(0xFFFF0000FFFF0000), ABC_CONST(0xFFFF0000FFFF0000), ABC_CONST(0xFFFF0000FFFF0000), ABC_CONST(0xFFFF0000FFFF0000),
    ABC_CONST(0xFFFFFFFF00000000), ABC_CONST(0xFFFFFFFF00000000), ABC_CONST(0xFFFFFFFF00000000), ABC_CONST(0xFFFFFFFF00000000),
    ABC_CONST(0x0000000000000000), ABC_CONST(0xFFFFFFFFFFFFFFFF), ABC_CONST(0x0000000000000000), ABC_CONST(0xFFFFFFFFFFFFFFFF),
    ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000), ABC_CONST(0xFFFFFFFFFFFFFFFF), ABC_CONST(0xFFFFFFFFFFFFFFFF)
};

static word s_Truths8Neg[32] = {
    ABC_CONST(0x5555555555555555), ABC_CONST(0x5555555555555555), ABC_CONST(0x5555555555555555), ABC_CONST(0x5555555555555555),
    ABC_CONST(0x3333333333333333), ABC_CONST(0x3333333333333333), ABC_CONST(0x3333333333333333), ABC_CONST(0x3333333333333333),
    ABC_CONST(0x0F0F0F0F0F0F0F0F), ABC_CONST(0x0F0F0F0F0F0F0F0F), ABC_CONST(0x0F0F0F0F0F0F0F0F), ABC_CONST(0x0F0F0F0F0F0F0F0F),
    ABC_CONST(0x00FF00FF00FF00FF), ABC_CONST(0x00FF00FF00FF00FF), ABC_CONST(0x00FF00FF00FF00FF), ABC_CONST(0x00FF00FF00FF00FF),
    ABC_CONST(0x0000FFFF0000FFFF), ABC_CONST(0x0000FFFF0000FFFF), ABC_CONST(0x0000FFFF0000FFFF), ABC_CONST(0x0000FFFF0000FFFF),
    ABC_CONST(0x00000000FFFFFFFF), ABC_CONST(0x00000000FFFFFFFF), ABC_CONST(0x00000000FFFFFFFF), ABC_CONST(0x00000000FFFFFFFF),
    ABC_CONST(0xFFFFFFFFFFFFFFFF), ABC_CONST(0x0000000000000000), ABC_CONST(0xFFFFFFFFFFFFFFFF), ABC_CONST(0x0000000000000000),
    ABC_CONST(0xFFFFFFFFFFFFFFFF), ABC_CONST(0xFFFFFFFFFFFFFFFF), ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000)
};

static int Abc_TtIsSubsetWithMask( word * pSmall, word * pLarge, word * pMask, int nWords )
{
    int w;
    for ( w = 0; w < nWords; ++w )
        if ( ( pSmall[w] & pLarge[w] & pMask[w] ) != ( pSmall[w] & pMask[w] ) )
            return 0;
    return 1;
}

static int Abc_TtCofsOppositeWithMask( word * pTruth, word * pMask, int nWords, int iVar )
{
    if ( iVar < 6 )
    {
        int w, Shift = ( 1 << iVar );
        for ( w = 0; w < nWords; ++w )
            if ( ( ( pTruth[w] << Shift ) & s_Truths6[iVar] & pMask[w] ) != ( ~pTruth[w] & s_Truths6[iVar] & pMask[w] ) )
                return 0;
        return 1;
    }
    else
    {
        int w, Step = ( 1 << ( iVar - 6 ) );
        word * p = pTruth, * m = pMask, * pLimit = pTruth + nWords;
        for ( ; p < pLimit; p += 2 * Step, m += 2 * Step )
            for ( w = 0; w < Step; ++w )
                if ( ( p[w] & m[w] ) != ( ~p[w + Step] & m[w + Step] ) )
                    return 0;
        return 1;
    }
}

// checks whether we can decompose as OP(x^p, g) where OP in {AND, OR} and p in {0, 1}
// returns p if OP = AND, and 2 + p if OP = OR
static int Abc_TtIsTopDecomposable( word * pTruth, word * pMask, int nWords, int iVar )
{
    assert( iVar < 8 );

    if ( Abc_TtIsSubsetWithMask( pTruth, &s_Truths8[iVar << 2], pMask, nWords ) ) return 1;
    if ( Abc_TtIsSubsetWithMask( pTruth, &s_Truths8Neg[iVar << 2], pMask, nWords ) ) return 2;
    if ( Abc_TtIsSubsetWithMask( &s_Truths8[iVar << 2], pTruth, pMask, nWords ) ) return 3;
    if ( Abc_TtIsSubsetWithMask( &s_Truths8Neg[iVar << 2], pTruth, pMask, nWords ) ) return 4;
    if ( Abc_TtCofsOppositeWithMask( pTruth, pMask, nWords, iVar ) ) return 5;

    return 0;
}

// checks whether we can decompose as OP(x1, OP(x2, OP(x3, ...))) where pVars = {x1, x2, x3, ...}
// OP can be different and vars can be complemented
static int Abc_TtIsStairDecomposable( word * pTruth, int nWords, int * pVars, int nSize, int * pStairFunc )
{
    int i, d;
    word pMask[4];
    word pCopy[4];

    Abc_TtCopy( pCopy, pTruth, nWords, 0 );
    Abc_TtMask( pMask, nWords, nWords * 64 );

    for ( i = 0; i < nSize; ++i )
    {
        d = Abc_TtIsTopDecomposable( pCopy, pMask, nWords, pVars[i] );
        if ( !d )
            return 0; /* not decomposable */

        pStairFunc[i] = d;

        switch ( d )
        {
        case 1: /* AND(x, g) */
        case 4: /* OR(!x, g) */
            Abc_TtAnd( pMask, pMask, &s_Truths8[pVars[i] << 2], nWords, 0 );
            break;
        case 2: /* AND(!x, g) */
        case 3: /* OR(x, g) */
            Abc_TtAnd( pMask, pMask, &s_Truths8Neg[pVars[i] << 2], nWords, 0 );
            break;
        case 5:
            Abc_TtXor( pCopy, pCopy, &s_Truths8[pVars[i] << 2], nWords, 0 );
            break;
        }
    }

    return 1; /* decomposable */
}

/***********************************************************************

  Synopsis    [Some printing utilities.]

***********************************************************************/

static inline void Abc_DebugPrint( const char* str, int fCond )
{
    if ( fCond )
    {
        printf( "%s", str );
        fflush( stdout );
    }
}

static inline void Abc_DebugPrintInt( const char* fmt, int n, int fCond )
{
    if ( fCond )
    {
        printf( fmt, n );
        fflush( stdout );
    }
}

static inline void Abc_DebugPrintIntInt( const char* fmt, int n1, int n2, int fCond )
{
    if ( fCond )
    {
        printf( fmt, n1, n2 );
        fflush( stdout );
    }
}

static inline void Abc_DebugErase( int n, int fCond )
{
    int i;
    if ( fCond )
    {
        for ( i = 0; i < n; ++i )
            printf( "\b" );
        fflush( stdout );
    }
}

/***********************************************************************

  Synopsis    [BMS.]

***********************************************************************/

#define ABC_EXACT_SOL_NVARS  0
#define ABC_EXACT_SOL_NFUNC  1
#define ABC_EXACT_SOL_NGATES 2

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

typedef struct Ses_Man_t_ Ses_Man_t;
struct Ses_Man_t_
{
    sat_solver * pSat;                  /* SAT solver */

    word *       pSpec;                 /* specification */
    int          bSpecInv;              /* remembers whether spec was inverted for normalization */
    int          nSpecVars;             /* number of variables in specification */
    int          nSpecFunc;             /* number of functions to synthesize */
    int          nSpecWords;            /* number of words for function */
    int          nRows;                 /* number of rows in the specification (without 0) */
    int          nMaxDepth;             /* maximum depth (-1 if depth is not constrained) */
    int          nMaxDepthTmp;          /* temporary copy to modify nMaxDepth temporarily */
    int *        pArrTimeProfile;       /* arrival times of inputs (NULL if arrival times are ignored) */
    int          pArrTimeProfileTmp[8]; /* temporary copy to modify pArrTimeProfile temporarily */
    int          nArrTimeDelta;         /* delta to the original arrival times (arrival times are normalized to have 0 as minimum element) */
    int          nArrTimeMax;           /* maximum normalized arrival time */
    int          nBTLimit;              /* conflict limit */
    int          fMakeAIG;              /* create AIG instead of general network */
    int          fVerbose;              /* be verbose */
    int          fVeryVerbose;          /* be very verbose */
    int          fExtractVerbose;       /* be verbose about solution extraction */
    int          fSatVerbose;           /* be verbose about SAT solving */
    int          fReasonVerbose;        /* be verbose about give-up reasons */
    word         pTtValues[4];          /* truth table values to assign */
    Vec_Int_t *  vPolar;                /* variables with positive polarity */
    Vec_Int_t *  vAssump;               /* assumptions */
    int          nRandRowAssigns;       /* number of random row assignments to initialize CEGAR */
    int          fKeepRowAssigns;       /* if 1, keep counter examples in CEGAR for next number of gates */

    int          nGates;                /* number of gates */
    int          nStartGates;           /* number of gates to start search (-1), i.e., to start from 1 gate, one needs to specify 0 */
    int          nMaxGates;             /* maximum number of gates given max. delay and arrival times */
    int          fDecStructure;         /* set to 1 or higher if nSpecFunc = 1 and f = x_i OP g(X \ {x_i}), otherwise 0 (determined when solving) */
    int          pDecVars;              /* mask of variables that can be decomposed at top-level */
    Vec_Int_t *  vStairDecVars;         /* list of stair decomposable variables */
    int          pStairDecFunc[8];      /* list of stair decomposable functions */
    word         pTtObjs[100];          /* temporary truth tables */

    int          nSimVars;              /* number of simulation vars x(i, t) */
    int          nOutputVars;           /* number of output variables g(h, i) */
    int          nGateVars;             /* number of gate variables f(i, p, q) */
    int          nSelectVars;           /* number of select variables s(i, j, k) */
    int          nDepthVars;            /* number of depth variables d(i, j) */

    int          nSimOffset;            /* offset where gate variables start */
    int          nOutputOffset;         /* offset where output variables start */
    int          nGateOffset;           /* offset where gate variables start */
    int          nSelectOffset;         /* offset where select variables start */
    int          nDepthOffset;          /* offset where depth variables start */

    int          fHitResLimit;          /* SAT solver gave up due to resource limit */

    abctime      timeSat;               /* SAT runtime */
    abctime      timeSatSat;            /* SAT runtime (sat instance) */
    abctime      timeSatUnsat;          /* SAT runtime (unsat instance) */
    abctime      timeSatUndef;          /* SAT runtime (undef instance) */
    abctime      timeInstance;          /* creating instance runtime */
    abctime      timeTotal;             /* all runtime */

    int          nSatCalls;             /* number of SAT calls */
    int          nUnsatCalls;           /* number of UNSAT calls */
    int          nUndefCalls;           /* number of UNDEF calls */

    int          nDebugOffset;          /* for debug printing */
};

/***********************************************************************

  Synopsis    [Store truth tables based on normalized arrival times.]

***********************************************************************/

// The hash table is a list of pointers to Ses_TruthEntry_t elements, which
// are arranged in a linked list, each of which pointing to a linked list
// of Ses_TimesEntry_t elements which contain the char* representation of the
// optimum netlist according to then normalized arrival times:

typedef struct Ses_TimesEntry_t_ Ses_TimesEntry_t;
struct Ses_TimesEntry_t_
{
    int                pArrTimeProfile[8]; /* normalized arrival time profile */
    int                fResLimit;          /* solution found after resource limit */
    Ses_TimesEntry_t * next;               /* linked list pointer */
    char *             pNetwork;           /* pointer to char array representation of optimum network */
};

typedef struct Ses_TruthEntry_t_ Ses_TruthEntry_t;
struct Ses_TruthEntry_t_
{
    word               pTruth[4]; /* truth table for comparison */
    int                nVars;     /* number of variables */
    Ses_TruthEntry_t * next;      /* linked list pointer */
    Ses_TimesEntry_t * head;      /* pointer to head of sub list with arrival times */
};

#define SES_STORE_TABLE_SIZE 1024
typedef struct Ses_Store_t_ Ses_Store_t;
struct Ses_Store_t_
{
    int                fMakeAIG;                       /* create AIG instead of general network */
    int                fVerbose;                       /* be verbose */
    int                fVeryVerbose;                   /* be very verbose */
    int                nBTLimit;                       /* conflict limit */
    int                nEntriesCount;                  /* number of entries */
    int                nValidEntriesCount;             /* number of entries with network */
    Ses_TruthEntry_t * pEntries[SES_STORE_TABLE_SIZE]; /* hash table for truth table entries */
    sat_solver       * pSat;                           /* own SAT solver instance to reuse when calling exact algorithm */
    FILE             * pDebugEntries;                  /* debug unsynth. (rl) entries */
    char             * szDBName;                       /* if given, database is written every time a new entry is added */

    /* statistics */
    unsigned long      nCutCount;                      /* number of cuts investigated */
    unsigned long      pCutCount[9];                   /* -> per cut size */
    unsigned long      nUnsynthesizedImp;              /* number of cuts which couldn't be optimized at all, opt. stopped because of imp. constraints */
    unsigned long      pUnsynthesizedImp[9];           /* -> per cut size */
    unsigned long      nUnsynthesizedRL;               /* number of cuts which couldn't be optimized at all, opt. stopped because of resource limits */
    unsigned long      pUnsynthesizedRL[9];            /* -> per cut size */
    unsigned long      nSynthesizedTrivial;            /* number of cuts which could be synthesized trivially (n < 2) */
    unsigned long      pSynthesizedTrivial[9];         /* -> per cut size */
    unsigned long      nSynthesizedImp;                /* number of cuts which could be synthesized, opt. stopped because of imp. constraints */
    unsigned long      pSynthesizedImp[9];             /* -> per cut size */
    unsigned long      nSynthesizedRL;                 /* number of cuts which could be synthesized, opt. stopped because of resource limits */
    unsigned long      pSynthesizedRL[9];              /* -> per cut size */
    unsigned long      nCacheHits;                     /* number of cache hits */
    unsigned long      pCacheHits[9];                  /* -> per cut size */

    unsigned long      nSatCalls;                      /* number of total SAT calls */
    unsigned long      nUnsatCalls;                    /* number of total UNSAT calls */
    unsigned long      nUndefCalls;                    /* number of total UNDEF calls */

    abctime            timeExact;                      /* Exact synthesis runtime */
    abctime            timeSat;                        /* SAT runtime */
    abctime            timeSatSat;                     /* SAT runtime (sat instance) */
    abctime            timeSatUnsat;                   /* SAT runtime (unsat instance) */
    abctime            timeSatUndef;                   /* SAT runtime (undef instance) */
    abctime            timeInstance;                   /* creating instance runtime */
    abctime            timeTotal;                      /* all runtime */
};

static Ses_Store_t * s_pSesStore = NULL;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

static int Abc_NormalizeArrivalTimes( int * pArrTimeProfile, int nVars, int * maxNormalized )
{
    int * p = pArrTimeProfile, * pEnd = pArrTimeProfile + nVars;
    int delta = *p;

    while ( ++p < pEnd )
        if ( *p < delta )
            delta = *p;

    *maxNormalized = 0;
    p = pArrTimeProfile;
    while ( p < pEnd )
    {
        *p -= delta;
        if ( *p > *maxNormalized )
            *maxNormalized = *p;
        ++p;
    }

    *maxNormalized += 1;

    return delta;
}

static inline Ses_Store_t * Ses_StoreAlloc( int nBTLimit, int fMakeAIG, int fVerbose )
{
    Ses_Store_t * pStore = ABC_CALLOC( Ses_Store_t, 1 );
    pStore->fMakeAIG           = fMakeAIG;
    pStore->fVerbose           = fVerbose;
    pStore->nBTLimit           = nBTLimit;
    memset( pStore->pEntries, 0, SES_STORE_TABLE_SIZE );

    pStore->pSat = sat_solver_new();

    return pStore;
}

static inline void Ses_StoreClean( Ses_Store_t * pStore )
{
    int i;
    Ses_TruthEntry_t * pTEntry, * pTEntry2;
    Ses_TimesEntry_t * pTiEntry, * pTiEntry2;

    for ( i = 0; i < SES_STORE_TABLE_SIZE; ++i )
        if ( pStore->pEntries[i] )
        {
            pTEntry = pStore->pEntries[i];

            while ( pTEntry )
            {
                pTiEntry = pTEntry->head;
                while ( pTiEntry )
                {
                    ABC_FREE( pTiEntry->pNetwork );
                    pTiEntry2 = pTiEntry;
                    pTiEntry = pTiEntry->next;
                    ABC_FREE( pTiEntry2 );
                }
                pTEntry2 = pTEntry;
                pTEntry = pTEntry->next;
                ABC_FREE( pTEntry2 );
            }
        }

    sat_solver_delete( pStore->pSat );

    if ( pStore->szDBName )
        ABC_FREE( pStore->szDBName );
    ABC_FREE( pStore );
}

static inline int Ses_StoreTableHash( word * pTruth, int nVars )
{
    static int s_Primes[4] = { 1291, 1699, 1999, 2357 };
    int i;
    unsigned uHash = 0;
    for ( i = 0; i < Abc_TtWordNum( nVars ); ++i )
        uHash ^= pTruth[i] * s_Primes[i & 0xf];
    return (int)(uHash % SES_STORE_TABLE_SIZE );
}

static inline int Ses_StoreTruthEqual( Ses_TruthEntry_t * pEntry, word * pTruth, int nVars )
{
    int i;

    if ( pEntry->nVars != nVars )
        return 0;

    for ( i = 0; i < Abc_TtWordNum( nVars ); ++i )
        if ( pEntry->pTruth[i] != pTruth[i] )
            return 0;
    return 1;
}

static inline void Ses_StoreTruthCopy( Ses_TruthEntry_t * pEntry, word * pTruthSrc, int nVars )
{
    int i;
    pEntry->nVars = nVars;
    for ( i = 0; i < Abc_TtWordNum( nVars ); ++i )
        pEntry->pTruth[i] = pTruthSrc[i];
}

static inline int Ses_StoreTimesEqual( int * pTimes1, int * pTimes2, int nVars )
{
    int i;
    for ( i = 0; i < nVars; ++i )
        if ( pTimes1[i] != pTimes2[i] )
            return 0;
    return 1;
}

static inline void Ses_StoreTimesCopy( int * pTimesDest, int * pTimesSrc, int nVars )
{
    int i;
    for ( i = 0; i < nVars; ++i )
        pTimesDest[i] = pTimesSrc[i];
}

static inline void Ses_StorePrintEntry( Ses_TruthEntry_t * pEntry, Ses_TimesEntry_t * pTiEntry )
{
    int i;

    printf( "f = " );
    Abc_TtPrintHexRev( stdout, pEntry->pTruth, pEntry->nVars );
    printf( ", n = %d", pEntry->nVars );
    printf( ", arrival =" );
    for ( i = 0; i < pEntry->nVars; ++i )
        printf( " %d", pTiEntry->pArrTimeProfile[i] );
    printf( "\n" );
}

static inline void Ses_StorePrintDebugEntry( Ses_Store_t * pStore, word * pTruth, int nVars, int * pNormalArrTime, int nMaxDepth, char * pSol, int nStartGates )
{
    int l;

    fprintf( pStore->pDebugEntries, "abc -c \"exact -v -C %d", pStore->nBTLimit );
    if ( s_pSesStore->fMakeAIG ) fprintf( pStore->pDebugEntries, " -a" );
    fprintf( pStore->pDebugEntries, " -S %d -D %d -A", nStartGates + 1, nMaxDepth );
    for ( l = 0; l < nVars; ++l )
        fprintf( pStore->pDebugEntries, "%c%d", ( l == 0 ? ' ' : ',' ), pNormalArrTime[l] );
    fprintf( pStore->pDebugEntries, " " );
    Abc_TtPrintHexRev( pStore->pDebugEntries, pTruth, nVars );
    fprintf( pStore->pDebugEntries, "\" # " );

    if ( !pSol )
        fprintf( pStore->pDebugEntries, "no " );
    fprintf( pStore->pDebugEntries, "solution found before\n" );
}

static void Abc_ExactNormalizeArrivalTimesForNetwork( int nVars, int * pArrTimeProfile, char * pSol )
{
    int nGates, i, j, k, nMax;
    Vec_Int_t * vLevels;

    nGates = pSol[ABC_EXACT_SOL_NGATES];

    /* printf( "NORMALIZE\n" ); */
    /* printf( "  #vars  = %d\n", nVars ); */
    /* printf( "  #gates = %d\n", nGates ); */

    vLevels = Vec_IntAllocArrayCopy( pArrTimeProfile, nVars );

    /* compute level of each gate based on arrival time profile (to compute depth) */
    for ( i = 0; i < nGates; ++i )
    {
        j = pSol[3 + i * 4 + 2];
        k = pSol[3 + i * 4 + 3];

        Vec_IntPush( vLevels, Abc_MaxInt( Vec_IntEntry( vLevels, j ), Vec_IntEntry( vLevels, k ) ) + 1 );

        /* printf( "  gate %d = (%d,%d)\n", nVars + i, j, k ); */
    }

    /* Vec_IntPrint( vLevels ); */

    /* reset all levels except for the last one */
    for ( i = 0; i < nVars + nGates - 1; ++i )
        Vec_IntSetEntry( vLevels, i, Vec_IntEntry( vLevels, nVars + nGates - 1 ) );

    /* Vec_IntPrint( vLevels ); */

    /* compute levels from top to bottom */
    for ( i = nGates - 1; i >= 0; --i )
    {
        j = pSol[3 + i * 4 + 2];
        k = pSol[3 + i * 4 + 3];

        Vec_IntSetEntry( vLevels, j, Abc_MinInt( Vec_IntEntry( vLevels, j ), Vec_IntEntry( vLevels, nVars + i ) - 1 ) );
        Vec_IntSetEntry( vLevels, k, Abc_MinInt( Vec_IntEntry( vLevels, k ), Vec_IntEntry( vLevels, nVars + i ) - 1 ) );
    }

    /* Vec_IntPrint( vLevels ); */

    /* normalize arrival times */
    Abc_NormalizeArrivalTimes( Vec_IntArray( vLevels ), nVars, &nMax );
    memcpy( pArrTimeProfile, Vec_IntArray( vLevels ), sizeof(int) * nVars );

    /* printf( "  nMax = %d\n", nMax ); */
    /* Vec_IntPrint( vLevels ); */

    Vec_IntFree( vLevels );
}

static void Ses_StoreWrite( Ses_Store_t * pStore, const char * pFilename, int fSynthImp, int fSynthRL, int fUnsynthImp, int fUnsynthRL )
{
    int i;
    char zero = '\0';
    unsigned long nEntries = 0;
    Ses_TruthEntry_t * pTEntry;
    Ses_TimesEntry_t * pTiEntry;
    FILE * pFile;

    pFile = fopen( pFilename, "wb" );
    if (pFile == NULL)
    {
        printf( "cannot open file \"%s\" for writing\n", pFilename );
        return;
    }

    if ( fSynthImp )   nEntries += pStore->nSynthesizedImp;
    if ( fSynthRL )    nEntries += pStore->nSynthesizedRL;
    if ( fUnsynthImp ) nEntries += pStore->nUnsynthesizedImp;
    if ( fUnsynthRL )  nEntries += pStore->nUnsynthesizedRL;
    fwrite( &nEntries, sizeof( unsigned long ), 1, pFile );

    for ( i = 0; i < SES_STORE_TABLE_SIZE; ++i )
        if ( pStore->pEntries[i] )
        {
            pTEntry = pStore->pEntries[i];

            while ( pTEntry )
            {
                pTiEntry = pTEntry->head;
                while ( pTiEntry )
                {
                    if ( !fSynthImp && pTiEntry->pNetwork && !pTiEntry->fResLimit )    { pTiEntry = pTiEntry->next; continue; }
                    if ( !fSynthRL && pTiEntry->pNetwork && pTiEntry->fResLimit )      { pTiEntry = pTiEntry->next; continue; }
                    if ( !fUnsynthImp && !pTiEntry->pNetwork && !pTiEntry->fResLimit ) { pTiEntry = pTiEntry->next; continue; }
                    if ( !fUnsynthRL && !pTiEntry->pNetwork && pTiEntry->fResLimit )   { pTiEntry = pTiEntry->next; continue; }

                    fwrite( pTEntry->pTruth, sizeof( word ), 4, pFile );
                    fwrite( &pTEntry->nVars, sizeof( int ), 1, pFile );
                    fwrite( pTiEntry->pArrTimeProfile, sizeof( int ), 8, pFile );
                    fwrite( &pTiEntry->fResLimit, sizeof( int ), 1, pFile );

                    if ( pTiEntry->pNetwork )
                    {
                        fwrite( pTiEntry->pNetwork, sizeof( char ), 3 + 4 * pTiEntry->pNetwork[ABC_EXACT_SOL_NGATES] + 2 + pTiEntry->pNetwork[ABC_EXACT_SOL_NVARS], pFile );
                    }
                    else
                    {
                        fwrite( &zero, sizeof( char ), 1, pFile );
                        fwrite( &zero, sizeof( char ), 1, pFile );
                        fwrite( &zero, sizeof( char ), 1, pFile );
                    }

                    pTiEntry = pTiEntry->next;
                }
                pTEntry = pTEntry->next;
            }
        }


    fclose( pFile );
}

// pArrTimeProfile is normalized
// returns 1 if and only if a new TimesEntry has been created
int Ses_StoreAddEntry( Ses_Store_t * pStore, word * pTruth, int nVars, int * pArrTimeProfile, char * pSol, int fResLimit )
{
    int key, fAdded;
    Ses_TruthEntry_t * pTEntry;
    Ses_TimesEntry_t * pTiEntry;

    if ( pSol )
        Abc_ExactNormalizeArrivalTimesForNetwork( nVars, pArrTimeProfile, pSol );

    key = Ses_StoreTableHash( pTruth, nVars );
    pTEntry = pStore->pEntries[key];

    /* does truth table already exist? */
    while ( pTEntry )
    {
        if ( Ses_StoreTruthEqual( pTEntry, pTruth, nVars ) )
            break;
        else
            pTEntry = pTEntry->next;
    }

    /* entry does not yet exist, so create new one and enqueue */
    if ( !pTEntry )
    {
        pTEntry = ABC_CALLOC( Ses_TruthEntry_t, 1 );
        Ses_StoreTruthCopy( pTEntry, pTruth, nVars );
        pTEntry->next = pStore->pEntries[key];
        pStore->pEntries[key] = pTEntry;
    }

    /* does arrival time already exist? */
    pTiEntry = pTEntry->head;
    while ( pTiEntry )
    {
        if ( Ses_StoreTimesEqual( pArrTimeProfile, pTiEntry->pArrTimeProfile, nVars ) )
            break;
        else
            pTiEntry = pTiEntry->next;
    }

    /* entry does not yet exist, so create new one and enqueue */
    if ( !pTiEntry )
    {
        pTiEntry = ABC_CALLOC( Ses_TimesEntry_t, 1 );
        Ses_StoreTimesCopy( pTiEntry->pArrTimeProfile, pArrTimeProfile, nVars );
        pTiEntry->pNetwork = pSol;
        pTiEntry->fResLimit = fResLimit;
        pTiEntry->next = pTEntry->head;
        pTEntry->head = pTiEntry;

        /* item has been added */
        fAdded = 1;
        pStore->nEntriesCount++;
        if ( pSol )
            pStore->nValidEntriesCount++;
    }
    else
    {
        //assert( 0 );
        /* item was already present */
        fAdded = 0;
    }

    /* statistics */
    if ( pSol )
    {
        if ( fResLimit )
        {
            pStore->nSynthesizedRL++;
            pStore->pSynthesizedRL[nVars]++;
        }
        else
        {
            pStore->nSynthesizedImp++;
            pStore->pSynthesizedImp[nVars]++;
        }
    }
    else
    {
        if ( fResLimit )
        {
            pStore->nUnsynthesizedRL++;
            pStore->pUnsynthesizedRL[nVars]++;
        }
        else
        {
            pStore->nUnsynthesizedImp++;
            pStore->pUnsynthesizedImp[nVars]++;
        }
    }

    if ( fAdded && pStore->szDBName )
        Ses_StoreWrite( pStore, pStore->szDBName, 1, 0, 0, 0 );

    return fAdded;
}

// pArrTimeProfile is normalized
// returns 1 if entry was in store, pSol may still be 0 if it couldn't be computed
int Ses_StoreGetEntrySimple( Ses_Store_t * pStore, word * pTruth, int nVars, int * pArrTimeProfile, char ** pSol )
{
    int key;
    Ses_TruthEntry_t * pTEntry;
    Ses_TimesEntry_t * pTiEntry;

    key = Ses_StoreTableHash( pTruth, nVars );
    pTEntry = pStore->pEntries[key];

    /* find truth table entry */
    while ( pTEntry )
    {
        if ( Ses_StoreTruthEqual( pTEntry, pTruth, nVars ) )
            break;
        else
            pTEntry = pTEntry->next;
    }

    /* no entry found? */
    if ( !pTEntry )
        return 0;

    /* find times entry */
    pTiEntry = pTEntry->head;
    while ( pTiEntry )
    {
        if ( Ses_StoreTimesEqual( pArrTimeProfile, pTiEntry->pArrTimeProfile, nVars ) )
            break;
        else
            pTiEntry = pTiEntry->next;
    }

    /* no entry found? */
    if ( !pTiEntry )
        return 0;

    *pSol = pTiEntry->pNetwork;
    return 1;
}

int Ses_StoreGetEntry( Ses_Store_t * pStore, word * pTruth, int nVars, int * pArrTimeProfile, char ** pSol )
{
    int key;
    Ses_TruthEntry_t * pTEntry;
    Ses_TimesEntry_t * pTiEntry;
    int pTimes[8];

    key = Ses_StoreTableHash( pTruth, nVars );
    pTEntry = pStore->pEntries[key];

    /* find truth table entry */
    while ( pTEntry )
    {
        if ( Ses_StoreTruthEqual( pTEntry, pTruth, nVars ) )
            break;
        else
            pTEntry = pTEntry->next;
    }

    /* no entry found? */
    if ( !pTEntry )
        return 0;

    /* find times entry */
    pTiEntry = pTEntry->head;
    while ( pTiEntry )
    {
        /* found after normalization wrt. to network */
        if ( pTiEntry->pNetwork )
        {
            memcpy( pTimes, pArrTimeProfile, sizeof(int) * nVars );
            Abc_ExactNormalizeArrivalTimesForNetwork( nVars, pTimes, pTiEntry->pNetwork );

            if ( Ses_StoreTimesEqual( pTimes, pTiEntry->pArrTimeProfile, nVars ) )
                break;
        }
        /* found for non synthesized network */
        else if ( Ses_StoreTimesEqual( pArrTimeProfile, pTiEntry->pArrTimeProfile, nVars ) )
            break;
        else
            pTiEntry = pTiEntry->next;
    }

    /* no entry found? */
    if ( !pTiEntry )
        return 0;

    *pSol = pTiEntry->pNetwork;
    return 1;
}

static void Ses_StoreRead( Ses_Store_t * pStore, const char * pFilename, int fSynthImp, int fSynthRL, int fUnsynthImp, int fUnsynthRL )
{
    int i;
    unsigned long nEntries;
    word pTruth[4];
    int nVars, fResLimit;
    int pArrTimeProfile[8];
    char pHeader[3];
    char * pNetwork;
    FILE * pFile;
    int value;

    if ( pStore->szDBName )
    {
        printf( "cannot read from database when szDBName is set" );
        return;
    }

    pFile = fopen( pFilename, "rb" );
    if (pFile == NULL)
    {
        printf( "cannot open file \"%s\" for reading\n", pFilename );
        return;
    }

    value = fread( &nEntries, sizeof( unsigned long ), 1, pFile );

    for ( i = 0; i < (int)nEntries; ++i )
    {
        value = fread( pTruth, sizeof( word ), 4, pFile );
        value = fread( &nVars, sizeof( int ), 1, pFile );
        value = fread( pArrTimeProfile, sizeof( int ), 8, pFile );
        value = fread( &fResLimit, sizeof( int ), 1, pFile );
        value = fread( pHeader, sizeof( char ), 3, pFile );

        if ( pHeader[0] == '\0' )
            pNetwork = NULL;
        else
        {
            pNetwork = ABC_CALLOC( char, 3 + 4 * pHeader[ABC_EXACT_SOL_NGATES] + 2 + pHeader[ABC_EXACT_SOL_NVARS] );
            pNetwork[0] = pHeader[0];
            pNetwork[1] = pHeader[1];
            pNetwork[2] = pHeader[2];

            value = fread( pNetwork + 3, sizeof( char ), 4 * pHeader[ABC_EXACT_SOL_NGATES] + 2 + pHeader[ABC_EXACT_SOL_NVARS], pFile );
        }

        if ( !fSynthImp && pNetwork && !fResLimit )    continue;
        if ( !fSynthRL && pNetwork && fResLimit )      continue;
        if ( !fUnsynthImp && !pNetwork && !fResLimit ) continue;
        if ( !fUnsynthRL && !pNetwork && fResLimit )   continue;

        Ses_StoreAddEntry( pStore, pTruth, nVars, pArrTimeProfile, pNetwork, fResLimit );
    }

    fclose( pFile );

    printf( "read %lu entries from file\n", (long)nEntries );
}

// computes top decomposition of variables wrt. to AND and OR
static inline void Ses_ManComputeTopDec( Ses_Man_t * pSes )
{
    int l;
    word pMask[4];

    Abc_TtMask( pMask, pSes->nSpecWords, pSes->nSpecWords * 64 );

    for ( l = 0; l < pSes->nSpecVars; ++l )
        if ( Abc_TtIsTopDecomposable( pSes->pSpec, pMask, pSes->nSpecWords, l ) )
            pSes->pDecVars |= ( 1 << l );
}

static inline Ses_Man_t * Ses_ManAlloc( word * pTruth, int nVars, int nFunc, int nMaxDepth, int * pArrTimeProfile, int fMakeAIG, int nBTLimit, int fVerbose )
{
    int h, i;

    Ses_Man_t * p = ABC_CALLOC( Ses_Man_t, 1 );
    p->pSat       = NULL;
    p->bSpecInv   = 0;
    for ( h = 0; h < nFunc; ++h )
        if ( pTruth[h << 2] & 1 )
        {
            for ( i = 0; i < 4; ++i )
                pTruth[(h << 2) + i] = ~pTruth[(h << 2) + i];
            p->bSpecInv |= ( 1 << h );
        }
    p->pSpec           = pTruth;
    p->nSpecVars       = nVars;
    p->nSpecFunc       = nFunc;
    p->nSpecWords      = Abc_TtWordNum( nVars );
    p->nRows           = ( 1 << nVars ) - 1;
    p->nMaxDepth       = nMaxDepth;
    p->pArrTimeProfile = nMaxDepth >= 0 ? pArrTimeProfile : NULL;
    if ( p->pArrTimeProfile )
        p->nArrTimeDelta = Abc_NormalizeArrivalTimes( p->pArrTimeProfile, nVars, &p->nArrTimeMax );
    else
        p->nArrTimeDelta = p->nArrTimeMax = 0;
    p->fMakeAIG        = fMakeAIG;
    p->nBTLimit        = nBTLimit;
    p->fVerbose        = fVerbose;
    p->fVeryVerbose    = 0;
    p->fExtractVerbose = 0;
    p->fSatVerbose     = 0;
    p->vPolar          = Vec_IntAlloc( 100 );
    p->vAssump         = Vec_IntAlloc( 10 );
    p->vStairDecVars   = Vec_IntAlloc( nVars );
    p->nRandRowAssigns = 2 * nVars;
    p->fKeepRowAssigns = 0;

    if ( p->nSpecFunc == 1 )
        Ses_ManComputeTopDec( p );

    srand( 0xCAFE );

    return p;
}

static inline void Ses_ManCleanLight( Ses_Man_t * pSes )
{
    int h, i;
    for ( h = 0; h < pSes->nSpecFunc; ++h )
        if ( ( pSes->bSpecInv >> h ) & 1 )
            for ( i = 0; i < 4; ++i )
                pSes->pSpec[(h << 2) + i] = ~( pSes->pSpec[(h << 2) + i] );

    if ( pSes->pArrTimeProfile )
        for ( i = 0; i < pSes->nSpecVars; ++i )
            pSes->pArrTimeProfile[i] += pSes->nArrTimeDelta;

    Vec_IntFree( pSes->vPolar );
    Vec_IntFree( pSes->vAssump );
    Vec_IntFree( pSes->vStairDecVars );

    ABC_FREE( pSes );
}

static inline void Ses_ManClean( Ses_Man_t * pSes )
{
    if ( pSes->pSat )
        sat_solver_delete( pSes->pSat );
    Ses_ManCleanLight( pSes );
}

/**Function*************************************************************

  Synopsis    [Access variables based on indexes.]

***********************************************************************/
static inline int Ses_ManSimVar( Ses_Man_t * pSes, int i, int t )
{
    assert( i < pSes->nGates );
    assert( t < pSes->nRows );

    return pSes->nSimOffset + pSes->nRows * i + t;
}

static inline int Ses_ManOutputVar( Ses_Man_t * pSes, int h, int i )
{
    assert( h < pSes->nSpecFunc );
    assert( i < pSes->nGates );

    return pSes->nOutputOffset + pSes->nGates * h + i;
}

static inline int Ses_ManGateVar( Ses_Man_t * pSes, int i, int p, int q )
{
    assert( i < pSes->nGates );
    assert( p < 2 );
    assert( q < 2 );
    assert( p > 0 || q > 0 );

    return pSes->nGateOffset + i * 3 + ( p << 1 ) + q - 1;
}

static inline int Ses_ManSelectVar( Ses_Man_t * pSes, int i, int j, int k )
{
    int a;
    int offset;

    assert( i < pSes->nGates );
    assert( k < pSes->nSpecVars + i );
    assert( j < k );

    offset = pSes->nSelectOffset;
    for ( a = pSes->nSpecVars; a < pSes->nSpecVars + i; ++a )
        offset += a * ( a - 1 ) / 2;

    return offset + ( -j * ( 1 + j - 2 * ( pSes->nSpecVars + i ) ) ) / 2 + ( k - j - 1 );
}

static inline int Ses_ManDepthVar( Ses_Man_t * pSes, int i, int j )
{
    assert( i < pSes->nGates );
    assert( j <= pSes->nArrTimeMax + i );

    return pSes->nDepthOffset + i * pSes->nArrTimeMax + ( ( i * ( i + 1 ) ) / 2 ) + j;
}

/**Function*************************************************************

  Synopsis    [Compute truth table from a solution.]

***********************************************************************/
static word * Ses_ManDeriveTruth( Ses_Man_t * pSes, char * pSol, int fInvert )
{
    int i, f, j, k, w, nGates = pSol[ABC_EXACT_SOL_NGATES];
    char * p;
    word * pTruth = NULL, * pTruth0, * pTruth1;
    assert( pSol[ABC_EXACT_SOL_NFUNC] == 1 );

    p = pSol + 3;

    memset( pSes->pTtObjs, 0, sizeof( word ) * 4 * nGates );

    for ( i = 0; i < nGates; ++i )
    {
        f = *p++;
        assert( *p++ == 2 );
        j = *p++;
        k = *p++;

        pTruth0 = j < pSes->nSpecVars ? &s_Truths8[j << 2] : &pSes->pTtObjs[( j - pSes->nSpecVars ) << 2];
        pTruth1 = k < pSes->nSpecVars ? &s_Truths8[k << 2] : &pSes->pTtObjs[( k - pSes->nSpecVars ) << 2];

        pTruth = &pSes->pTtObjs[i << 2];

        if ( f & 1 )
            for ( w = 0; w < pSes->nSpecWords; ++w )
                pTruth[w] |= ~pTruth0[w] & pTruth1[w];
        if ( ( f >> 1 ) & 1 )
            for ( w = 0; w < pSes->nSpecWords; ++w )
                pTruth[w] |= pTruth0[w] & ~pTruth1[w];
        if ( ( f >> 2 ) & 1 )
            for ( w = 0; w < pSes->nSpecWords; ++w )
                pTruth[w] |= pTruth0[w] & pTruth1[w];
    }

    assert( Abc_Lit2Var( *p ) == nGates - 1 );
    if ( fInvert && Abc_LitIsCompl( *p ) )
        Abc_TtNot( pTruth, pSes->nSpecWords );

    return pTruth;
}

/**Function*************************************************************

  Synopsis    [Setup variables to find network with nGates gates.]

***********************************************************************/
static void Ses_ManCreateVars( Ses_Man_t * pSes, int nGates )
{
    int i;

    if ( pSes->fSatVerbose )
    {
        printf( "create variables for network with %d functions over %d variables and %d/%d gates\n", pSes->nSpecFunc, pSes->nSpecVars, nGates, pSes->nMaxGates );
    }

    pSes->nGates      = nGates;
    pSes->nSimVars    = nGates * pSes->nRows;
    pSes->nOutputVars = pSes->nSpecFunc * nGates;
    pSes->nGateVars   = nGates * 3;
    pSes->nSelectVars = 0;
    for ( i = pSes->nSpecVars; i < pSes->nSpecVars + nGates; ++i )
        pSes->nSelectVars += ( i * ( i - 1 ) ) / 2;
    pSes->nDepthVars = pSes->nMaxDepth > 0 ? nGates * pSes->nArrTimeMax + ( nGates * ( nGates + 1 ) ) / 2 : 0;

    /* pSes->nSimOffset    = 0; */
    /* pSes->nOutputOffset = pSes->nSimVars; */
    /* pSes->nGateOffset   = pSes->nSimVars + pSes->nOutputVars; */
    /* pSes->nSelectOffset = pSes->nSimVars + pSes->nOutputVars + pSes->nGateVars; */
    /* pSes->nDepthOffset  = pSes->nSimVars + pSes->nOutputVars + pSes->nGateVars + pSes->nSelectVars; */

    pSes->nDepthOffset  = 0;
    pSes->nSelectOffset = pSes->nDepthVars;
    pSes->nGateOffset   = pSes->nDepthVars + pSes->nSelectVars;
    pSes->nOutputOffset = pSes->nDepthVars + pSes->nSelectVars + pSes->nGateVars;
    pSes->nSimOffset    = pSes->nDepthVars + pSes->nSelectVars + pSes->nGateVars + pSes->nOutputVars;

    /* pSes->nDepthOffset  = 0; */
    /* pSes->nSelectOffset = pSes->nDepthVars; */
    /* pSes->nOutputOffset = pSes->nDepthVars + pSes->nSelectVars; */
    /* pSes->nGateOffset   = pSes->nDepthVars + pSes->nSelectVars + pSes->nOutputVars; */
    /* pSes->nSimOffset    = pSes->nDepthVars + pSes->nSelectVars + pSes->nOutputVars + pSes->nGateVars; */

    /* if we already have a SAT solver, then restart it (this saves a lot of time) */
    if ( pSes->pSat )
        sat_solver_restart( pSes->pSat );
    else
        pSes->pSat = sat_solver_new();
    sat_solver_setnvars( pSes->pSat, pSes->nSimVars + pSes->nOutputVars + pSes->nGateVars + pSes->nSelectVars + pSes->nDepthVars );
}

/**Function*************************************************************

  Synopsis    [Create clauses.]

***********************************************************************/
static inline void Ses_ManGateCannotHaveChild( Ses_Man_t * pSes, int i, int j )
{
    int k;

    for ( k = 0; k < j; ++k )
        Vec_IntPush( pSes->vAssump, Abc_Var2Lit( Ses_ManSelectVar( pSes, i, k, j ), 1 ) );
    for ( k = j + 1; k < pSes->nSpecVars + i; ++k )
        Vec_IntPush( pSes->vAssump, Abc_Var2Lit( Ses_ManSelectVar( pSes, i, j, k ), 1 ) );
}

static inline void Ses_ManCreateMainClause( Ses_Man_t * pSes, int t, int i, int j, int k, int a, int b, int c )
{
    int pLits[5], ctr = 0;

    pLits[ctr++] = Abc_Var2Lit( Ses_ManSelectVar( pSes, i, j, k ), 1 );
    pLits[ctr++] = Abc_Var2Lit( Ses_ManSimVar( pSes, i, t ), a );

    if ( j < pSes->nSpecVars )
    {
        if ( ( ( ( t + 1 ) & ( 1 << j ) ) ? 1 : 0 ) != b )
            return;
    }
    else
        pLits[ctr++] = Abc_Var2Lit( Ses_ManSimVar( pSes, j - pSes->nSpecVars, t ), b );

    if ( k < pSes->nSpecVars )
    {
        if ( ( ( ( t + 1 ) & ( 1 << k ) ) ? 1 : 0 ) != c )
            return;
    }
    else
        pLits[ctr++] = Abc_Var2Lit( Ses_ManSimVar( pSes, k - pSes->nSpecVars, t ), c );

    if ( b > 0 || c > 0 )
        pLits[ctr++] = Abc_Var2Lit( Ses_ManGateVar( pSes, i, b, c ), 1 - a );

    sat_solver_addclause( pSes->pSat, pLits, pLits + ctr );
}

static int inline Ses_ManCreateTruthTableClause( Ses_Man_t * pSes, int t )
{
    int i, j, k, h;
    int pLits[3];

    for ( i = 0; i < pSes->nGates; ++i )
    {
        /* main clauses */
        for ( j = 0; j < pSes->nSpecVars + i; ++j )
            for ( k = j + 1; k < pSes->nSpecVars + i; ++k )
            {
                Ses_ManCreateMainClause( pSes, t, i, j, k, 0, 0, 1 );
                Ses_ManCreateMainClause( pSes, t, i, j, k, 0, 1, 0 );
                Ses_ManCreateMainClause( pSes, t, i, j, k, 0, 1, 1 );
                Ses_ManCreateMainClause( pSes, t, i, j, k, 1, 0, 0 );
                Ses_ManCreateMainClause( pSes, t, i, j, k, 1, 0, 1 );
                Ses_ManCreateMainClause( pSes, t, i, j, k, 1, 1, 0 );
                Ses_ManCreateMainClause( pSes, t, i, j, k, 1, 1, 1 );
            }

        /* output clauses */
        if ( pSes->nSpecFunc != 1 )
            for ( h = 0; h < pSes->nSpecFunc; ++h )
            {
                pLits[0] = Abc_Var2Lit( Ses_ManOutputVar( pSes, h, i ), 1 );
                pLits[1] = Abc_Var2Lit( Ses_ManSimVar( pSes, i, t ), 1 - Abc_TtGetBit( &pSes->pSpec[h << 2], t + 1 ) );
                if ( !sat_solver_addclause( pSes->pSat, pLits, pLits + 2 ) )
                    return 0;
            }
    }

    if ( pSes->nSpecFunc == 1 )
        Vec_IntPush( pSes->vAssump, Abc_Var2Lit( Ses_ManSimVar( pSes, pSes->nGates - 1, t ), 1 - Abc_TtGetBit( pSes->pSpec, t + 1 ) ) );

    return 1;
}

static int Ses_ManCreateClauses( Ses_Man_t * pSes )
{
    extern int Extra_TruthVarsSymm( unsigned * pTruth, int nVars, int iVar0, int iVar1 );

    int h, i, j, k, t, ii, jj, kk, iii, p, q;
    int pLits[3];
    Vec_Int_t * vLits = NULL;

    for ( t = 0; t < pSes->nRows; ++t )
    {
        if ( Abc_TtGetBit( pSes->pTtValues, t ) )
            if ( !Ses_ManCreateTruthTableClause( pSes, t ) )
                return 0;
    }

    /* if there is only one output, we know it must point to the last gate  */
    if ( pSes->nSpecFunc == 1 )
    {
        for ( i = 0; i < pSes->nGates - 1; ++i )
            Vec_IntPush( pSes->vAssump, Abc_Var2Lit( Ses_ManOutputVar( pSes, 0, i ), 1 ) );
        Vec_IntPush( pSes->vAssump, Abc_Var2Lit( Ses_ManOutputVar( pSes, 0, pSes->nGates - 1 ), 0 ) );

        vLits = Vec_IntAlloc( 0u );
    }
    else
    {
        vLits = Vec_IntAlloc( 0u );

        /* some output is selected */
        for ( h = 0; h < pSes->nSpecFunc; ++h )
        {
            Vec_IntGrowResize( vLits, pSes->nGates );
            for ( i = 0; i < pSes->nGates; ++i )
                Vec_IntSetEntry( vLits, i, Abc_Var2Lit( Ses_ManOutputVar( pSes, h, i ), 0 ) );
            sat_solver_addclause( pSes->pSat, Vec_IntArray( vLits ), Vec_IntArray( vLits ) + pSes->nGates );
        }
    }

    /* each gate has two operands */
    for ( i = 0; i < pSes->nGates; ++i )
    {
        Vec_IntGrowResize( vLits, ( ( pSes->nSpecVars + i ) * ( pSes->nSpecVars + i - 1 ) ) / 2 );
        jj = 0;
        for ( j = 0; j < pSes->nSpecVars + i; ++j )
            for ( k = j + 1; k < pSes->nSpecVars + i; ++k )
                Vec_IntSetEntry( vLits, jj++, Abc_Var2Lit( Ses_ManSelectVar( pSes, i, j, k ), 0 ) );
        sat_solver_addclause( pSes->pSat, Vec_IntArray( vLits ), Vec_IntArray( vLits ) + jj );
    }

    /* gate decomposition (makes it worse) */
    /* if ( fDecVariable >= 0 && pSes->nGates >= 2 ) */
    /* { */
    /*     pLits[0] = Abc_Var2Lit( Ses_ManSelectVar( pSes, pSes->nGates - 1, fDecVariable, pSes->nSpecVars + pSes->nGates - 2 ), 0 ); */
    /*     if ( !sat_solver_addclause( pSes->pSat, pLits, pLits + 1 ) ) */
    /*     { */
    /*         Vec_IntFree( vLits ); */
    /*         return 0; */
    /*     } */

    /*     for ( k = 1; k < pSes->nSpecVars + pSes->nGates - 1; ++k ) */
    /*         for ( j = 0; j < k; ++j ) */
    /*             if ( j != fDecVariable || k != pSes->nSpecVars + pSes->nGates - 2 ) */
    /*             { */
    /*                 pLits[0] = Abc_Var2Lit( Ses_ManSelectVar( pSes, pSes->nGates - 1, j, k ), 1 ); */
    /*                 if ( !sat_solver_addclause( pSes->pSat, pLits, pLits + 1 ) ) */
    /*                 { */
    /*                     Vec_IntFree( vLits ); */
    /*                     return 0; */
    /*                 } */
    /*             } */
    /* } */

    /* only AIG */
    if ( pSes->fMakeAIG )
    {
        for ( i = 0; i < pSes->nGates; ++i )
        {
            /* not 2 ones */
            pLits[0] = Abc_Var2Lit( Ses_ManGateVar( pSes, i, 0, 1 ), 1 );
            pLits[1] = Abc_Var2Lit( Ses_ManGateVar( pSes, i, 1, 0 ), 1 );
            pLits[2] = Abc_Var2Lit( Ses_ManGateVar( pSes, i, 1, 1 ), 0 );
            sat_solver_addclause( pSes->pSat, pLits, pLits + 3 );

            pLits[0] = Abc_Var2Lit( Ses_ManGateVar( pSes, i, 0, 1 ), 1 );
            pLits[1] = Abc_Var2Lit( Ses_ManGateVar( pSes, i, 1, 0 ), 0 );
            pLits[2] = Abc_Var2Lit( Ses_ManGateVar( pSes, i, 1, 1 ), 1 );
            sat_solver_addclause( pSes->pSat, pLits, pLits + 3 );

            pLits[0] = Abc_Var2Lit( Ses_ManGateVar( pSes, i, 0, 1 ), 0 );
            pLits[1] = Abc_Var2Lit( Ses_ManGateVar( pSes, i, 1, 0 ), 1 );
            pLits[2] = Abc_Var2Lit( Ses_ManGateVar( pSes, i, 1, 1 ), 1 );
            sat_solver_addclause( pSes->pSat, pLits, pLits + 3 );
        }
    }

    /* only binary clauses */
    if ( 1 ) /* TODO: add some meaningful parameter */
    {
        for ( i = 0; i < pSes->nGates; ++i )
        {
            pLits[0] = Abc_Var2Lit( Ses_ManGateVar( pSes, i, 0, 1 ), 0 );
            pLits[1] = Abc_Var2Lit( Ses_ManGateVar( pSes, i, 1, 0 ), 0 );
            pLits[2] = Abc_Var2Lit( Ses_ManGateVar( pSes, i, 1, 1 ), 0 );
            sat_solver_addclause( pSes->pSat, pLits, pLits + 3 );

            pLits[0] = Abc_Var2Lit( Ses_ManGateVar( pSes, i, 0, 1 ), 1 );
            pLits[1] = Abc_Var2Lit( Ses_ManGateVar( pSes, i, 1, 0 ), 0 );
            pLits[2] = Abc_Var2Lit( Ses_ManGateVar( pSes, i, 1, 1 ), 1 );
            sat_solver_addclause( pSes->pSat, pLits, pLits + 3 );

            pLits[0] = Abc_Var2Lit( Ses_ManGateVar( pSes, i, 0, 1 ), 0 );
            pLits[1] = Abc_Var2Lit( Ses_ManGateVar( pSes, i, 1, 0 ), 1 );
            pLits[2] = Abc_Var2Lit( Ses_ManGateVar( pSes, i, 1, 1 ), 1 );
            sat_solver_addclause( pSes->pSat, pLits, pLits + 3 );
        }

        for ( i = 0; i < pSes->nGates; ++i )
            for ( k = 1; k < pSes->nSpecVars + i; ++k )
                for ( j = 0; j < k; ++j )
                {
                    pLits[0] = Abc_Var2Lit( Ses_ManSelectVar( pSes, i, j, k ), 1 );

                    for ( kk = 1; kk < pSes->nSpecVars + i; ++kk )
                        for ( jj = 0; jj < kk; ++jj )
                        {
                            if ( k == kk && j == jj ) continue;
                            pLits[1] = Abc_Var2Lit( Ses_ManSelectVar( pSes, i, jj, kk ), 1 );

                            if ( pLits[0] < pLits[1] )
                                sat_solver_addclause( pSes->pSat, pLits, pLits + 2 );
                        }
                }

        /* Vec_IntGrowResize( vLits, pSes->nGates * ( pSes->nSpecVars + pSes->nGates - 2 ) ); */

        /* for ( j = 0; j < pSes->nSpecVars; ++j ) */
        /* { */
        /*     jj = 0; */
        /*     for ( i = 0; i < pSes->nGates; ++i ) */
        /*     { */
        /*         for ( k = 0; k < j; ++k ) */
        /*             Vec_IntSetEntry( vLits, jj++, Abc_Var2Lit( Ses_ManSelectVar( pSes, i, k, j ), 0 ) ); */
        /*         for ( k = j + 1; k < pSes->nSpecVars + i; ++k ) */
        /*             Vec_IntSetEntry( vLits, jj++, Abc_Var2Lit( Ses_ManSelectVar( pSes, i, j, k ), 0 ) ); */
        /*     } */
        /*     sat_solver_addclause( pSes->pSat, Vec_IntArray( vLits ), Vec_IntArray( vLits ) + jj ); */
        /* } */
    }

    /* EXTRA stair decomposition */
    if (Vec_IntSize( pSes->vStairDecVars ) )
    {
        Vec_IntForEachEntry( pSes->vStairDecVars, j, i )
        {
            if ( pSes->nGates - 2 - i < j )
            {
                continue;
            }
            Vec_IntPush( pSes->vAssump, Abc_Var2Lit( Ses_ManSelectVar( pSes, pSes->nGates - 1 - i, j, pSes->nSpecVars + pSes->nGates - 2 - i ), 0 ) );

            //printf( "dec %d for var %d\n", pSes->pStairDecFunc[i], j );

            switch ( pSes->pStairDecFunc[i] )
            {
            case 1: /* AND(x,g) */
                Vec_IntPush( pSes->vAssump, Abc_Var2Lit( Ses_ManGateVar( pSes, pSes->nGates - 1 - i, 0, 1 ), 1 ) );
                //Vec_IntPush( pSes->vAssump, Abc_Var2Lit( Ses_ManGateVar( pSes, pSes->nGates - 1 - i, 1, 0 ), 1 ) );
                //Vec_IntPush( pSes->vAssump, Abc_Var2Lit( Ses_ManGateVar( pSes, pSes->nGates - 1 - i, 1, 1 ), 0 ) );
                break;
            case 2: /* AND(!x,g) */
                //Vec_IntPush( pSes->vAssump, Abc_Var2Lit( Ses_ManGateVar( pSes, pSes->nGates - 1 - i, 0, 1 ), 0 ) );
                Vec_IntPush( pSes->vAssump, Abc_Var2Lit( Ses_ManGateVar( pSes, pSes->nGates - 1 - i, 1, 0 ), 1 ) );
                Vec_IntPush( pSes->vAssump, Abc_Var2Lit( Ses_ManGateVar( pSes, pSes->nGates - 1 - i, 1, 1 ), 1 ) );
                break;
            case 3: /* OR(x,g) */
                //Vec_IntPush( pSes->vAssump, Abc_Var2Lit( Ses_ManGateVar( pSes, pSes->nGates - 1 - i, 0, 1 ), 0 ) );
                Vec_IntPush( pSes->vAssump, Abc_Var2Lit( Ses_ManGateVar( pSes, pSes->nGates - 1 - i, 1, 0 ), 0 ) );
                Vec_IntPush( pSes->vAssump, Abc_Var2Lit( Ses_ManGateVar( pSes, pSes->nGates - 1 - i, 1, 1 ), 0 ) );
                break;
            case 4: /* OR(!x,g) */
                ////Vec_IntPush( pSes->vAssump, Abc_Var2Lit( Ses_ManGateVar( pSes, pSes->nGates - 1 - i, 0, 1 ), 0 ) );
                ////Vec_IntPush( pSes->vAssump, Abc_Var2Lit( Ses_ManGateVar( pSes, pSes->nGates - 1 - i, 1, 0 ), 1 ) );
                ////Vec_IntPush( pSes->vAssump, Abc_Var2Lit( Ses_ManGateVar( pSes, pSes->nGates - 1 - i, 1, 1 ), 0 ) );
                break;
            case 5: /* XOR(x,g) */
                Vec_IntPush( pSes->vAssump, Abc_Var2Lit( Ses_ManGateVar( pSes, pSes->nGates - 1 - i, 0, 1 ), 0 ) );
                Vec_IntPush( pSes->vAssump, Abc_Var2Lit( Ses_ManGateVar( pSes, pSes->nGates - 1 - i, 1, 0 ), 0 ) );
                Vec_IntPush( pSes->vAssump, Abc_Var2Lit( Ses_ManGateVar( pSes, pSes->nGates - 1 - i, 1, 1 ), 1 ) );
                break;
            default:
                printf( "func: %d\n", pSes->pStairDecFunc[i] );
                assert( 0 );
            }
        }
    }

    /* EXTRA clauses: use gate i at least once */
    Vec_IntGrowResize( vLits, pSes->nSpecFunc + pSes->nGates * ( pSes->nSpecVars + pSes->nGates - 2 ) );
    for ( i = 0; i < pSes->nGates; ++i )
    {
        jj = 0;
        for ( h = 0; h < pSes->nSpecFunc; ++h )
            Vec_IntSetEntry( vLits, jj++, Abc_Var2Lit( Ses_ManOutputVar( pSes, h, i ), 0 ) );
        for ( ii = i + 1; ii < pSes->nGates; ++ii )
        {
            for ( j = 0; j < pSes->nSpecVars + i; ++j )
                Vec_IntSetEntry( vLits, jj++, Abc_Var2Lit( Ses_ManSelectVar( pSes, ii, j, pSes->nSpecVars + i ), 0 ) );
            for ( j = pSes->nSpecVars + i + 1; j < pSes->nSpecVars + ii; ++j )
                Vec_IntSetEntry( vLits, jj++, Abc_Var2Lit( Ses_ManSelectVar( pSes, ii, pSes->nSpecVars + i, j ), 0 ) );
        }
        sat_solver_addclause( pSes->pSat, Vec_IntArray( vLits ), Vec_IntArray( vLits ) + jj );
    }
    Vec_IntFree( vLits );

    /* EXTRA clauses: no reapplying operand */
    if ( pSes->nGates > 1 )
        for ( i = 0; i < pSes->nGates - 1; ++i )
            for ( ii = i + 1; ii < pSes->nGates; ++ii )
                for ( k = 1; k < pSes->nSpecVars + i; ++k )
                    for ( j = 0; j < k; ++j )
                    {
                        pLits[0] = Abc_Var2Lit( Ses_ManSelectVar( pSes, i, j, k ), 1 );
                        pLits[1] = Abc_Var2Lit( Ses_ManSelectVar( pSes, ii, j, pSes->nSpecVars + i ), 1 );
                        sat_solver_addclause( pSes->pSat, pLits, pLits + 2 );

                        pLits[1] = Abc_Var2Lit( Ses_ManSelectVar( pSes, ii, k, pSes->nSpecVars + i ), 1 );
                        sat_solver_addclause( pSes->pSat, pLits, pLits + 2 );
                    }
    if ( pSes->nGates > 2 )
        for ( i = 0; i < pSes->nGates - 2; ++i )
            for ( ii = i + 1; ii < pSes->nGates - 1; ++ii )
                for ( iii = ii + 1; iii < pSes->nGates; ++iii )
                    for ( k = 1; k < pSes->nSpecVars + i; ++k )
                        for ( j = 0; j < k; ++j )
                            {
                                pLits[0] = Abc_Var2Lit( Ses_ManSelectVar( pSes, i, j, k ), 1 );
                                pLits[1] = Abc_Var2Lit( Ses_ManSelectVar( pSes, ii, j, k ), 1 );
                                pLits[2] = Abc_Var2Lit( Ses_ManSelectVar( pSes, iii, i, ii ), 1 );
                                sat_solver_addclause( pSes->pSat, pLits, pLits + 3 );
                            }

    /* EXTRA clauses: co-lexicographic order */
    for ( i = 0; i < pSes->nGates - 1; ++i )
    {
        for ( k = 2; k < pSes->nSpecVars + i; ++k )
        {
            for ( j = 1; j < k; ++j )
                for ( jj = 0; jj < j; ++jj )
                {
                    pLits[0] = Abc_Var2Lit( Ses_ManSelectVar( pSes, i, j, k ), 1 );
                    pLits[1] = Abc_Var2Lit( Ses_ManSelectVar( pSes, i + 1, jj, k ), 1 );
                    sat_solver_addclause( pSes->pSat, pLits, pLits + 2 );
                }

            for ( j = 0; j < k; ++j )
                for ( kk = 1; kk < k; ++kk )
                    for ( jj = 0; jj < kk; ++jj )
                    {
                        pLits[0] = Abc_Var2Lit( Ses_ManSelectVar( pSes, i, j, k ), 1 );
                        pLits[1] = Abc_Var2Lit( Ses_ManSelectVar( pSes, i + 1, jj, kk ), 1 );
                        sat_solver_addclause( pSes->pSat, pLits, pLits + 2 );
                    }
        }
    }

    /* EXTRA clauses: symmetric variables */
    if ( /*pSes->nMaxDepth == -1 &&*/ pSes->nSpecFunc == 1 ) /* only check if there is one output function */
        for ( q = 1; q < pSes->nSpecVars; ++q )
            for ( p = 0; p < q; ++p )
                if ( Extra_TruthVarsSymm( (unsigned*)( pSes->pSpec ), pSes->nSpecVars, p, q ) &&
                     ( !pSes->pArrTimeProfile || ( pSes->pArrTimeProfile[p] <= pSes->pArrTimeProfile[q] ) ) )
                {
                    if ( pSes->fSatVerbose )
                        printf( "variables %d and %d are symmetric\n", p, q );
                    for ( i = 0; i < pSes->nGates; ++i )
                        for ( j = 0; j < q; ++j )
                        {
                            if ( j == p ) continue;

                            vLits = Vec_IntAlloc( 0 );
                            Vec_IntPush( vLits, Abc_Var2Lit( Ses_ManSelectVar( pSes, i, j, q ), 1 ) );
                            for ( ii = 0; ii < i; ++ii )
                                for ( kk = 1; kk < pSes->nSpecVars + ii; ++kk )
                                    for ( jj = 0; jj < kk; ++jj )
                                        if ( jj == p || kk == p )
                                            Vec_IntPush( vLits, Abc_Var2Lit( Ses_ManSelectVar( pSes, ii, jj, kk ), 0 ) );
                            sat_solver_addclause( pSes->pSat, Vec_IntArray( vLits ), Vec_IntLimit( vLits ) );
                            Vec_IntFree( vLits );
                        }
                }

    return 1;
}

static int Ses_ManCreateDepthClauses( Ses_Man_t * pSes )
{
    int i, j, k, jj, kk, d, h;
    int pLits[3];

    for ( i = 0; i < pSes->nGates; ++i )
    {
        /* propagate depths from children */
        for ( k = 1; k < i; ++k )
            for ( j = 0; j < k; ++j )
            {
                pLits[0] = Abc_Var2Lit( Ses_ManSelectVar( pSes, i, pSes->nSpecVars + j, pSes->nSpecVars + k ), 1 );
                for ( jj = 0; jj <= pSes->nArrTimeMax + j; ++jj )
                {
                    pLits[1] = Abc_Var2Lit( Ses_ManDepthVar( pSes, j, jj ), 1 );
                    pLits[2] = Abc_Var2Lit( Ses_ManDepthVar( pSes, i, jj + 1 ), 0 );
                    sat_solver_addclause( pSes->pSat, pLits, pLits + 3 );
                }
            }

        for ( k = 0; k < i; ++k )
            for ( j = 0; j < pSes->nSpecVars + k; ++j )
            {
                pLits[0] = Abc_Var2Lit( Ses_ManSelectVar( pSes, i, j, pSes->nSpecVars + k ), 1 );
                for ( kk = 0; kk <= pSes->nArrTimeMax + k; ++kk )
                {
                    pLits[1] = Abc_Var2Lit( Ses_ManDepthVar( pSes, k, kk ), 1 );
                    pLits[2] = Abc_Var2Lit( Ses_ManDepthVar( pSes, i, kk + 1 ), 0 );
                    sat_solver_addclause( pSes->pSat, pLits, pLits + 3 );
                }
            }

        /* propagate depths from arrival times at PIs */
        if ( pSes->pArrTimeProfile )
        {
            for ( k = 1; k < pSes->nSpecVars + i; ++k )
                for ( j = 0; j < ( ( k < pSes->nSpecVars ) ? k : pSes->nSpecVars ); ++j )
                {
                    d = pSes->pArrTimeProfile[j];
                    if ( k < pSes->nSpecVars && pSes->pArrTimeProfile[k] > d )
                        d = pSes->pArrTimeProfile[k];

                    pLits[0] = Abc_Var2Lit( Ses_ManSelectVar( pSes, i, j, k ), 1 );
                    pLits[1] = Abc_Var2Lit( Ses_ManDepthVar( pSes, i, d ), 0 );
                    sat_solver_addclause( pSes->pSat, pLits, pLits + 2 );
                }
        }
        else
            /* arrival times are 0 */
            Vec_IntPush( pSes->vAssump, Abc_Var2Lit( Ses_ManDepthVar( pSes, i, 0 ), 0 ) );

        /* reverse order encoding of depth variables */
        for ( j = 1; j <= pSes->nArrTimeMax + i; ++j )
        {
            pLits[0] = Abc_Var2Lit( Ses_ManDepthVar( pSes, i, j ), 1 );
            pLits[1] = Abc_Var2Lit( Ses_ManDepthVar( pSes, i, j - 1 ), 0 );
            sat_solver_addclause( pSes->pSat, pLits, pLits + 2 );
        }

        /* constrain maximum depth */
        if ( pSes->nMaxDepth < pSes->nArrTimeMax + i )
            for ( h = 0; h < pSes->nSpecFunc; ++h )
            {
                pLits[0] = Abc_Var2Lit( Ses_ManOutputVar( pSes, h, i ), 1 );
                pLits[1] = Abc_Var2Lit( Ses_ManDepthVar( pSes, i, pSes->nMaxDepth ), 1 );
                if ( !sat_solver_addclause( pSes->pSat, pLits, pLits + 2 ) )
                    return 0;
            }
    }

    return 1;
}

/**Function*************************************************************

  Synopsis    [Solve.]

***********************************************************************/
static inline double Sat_Wrd2Dbl( word w )  { return (double)(unsigned)(w&0x3FFFFFFF) + (double)(1<<30)*(unsigned)(w>>30); }

static inline int Ses_ManSolve( Ses_Man_t * pSes )
{
    int status;
    abctime timeStart, timeDelta;

    if ( pSes->fSatVerbose )
    {
        printf( "SAT   CL: %7d   VA: %5d", sat_solver_nclauses( pSes->pSat ), sat_solver_nvars( pSes->pSat ) );
        fflush( stdout );
    }

    timeStart = Abc_Clock();
    status = sat_solver_solve( pSes->pSat, Vec_IntArray( pSes->vAssump ), Vec_IntLimit( pSes->vAssump ), pSes->nBTLimit, 0, 0, 0 );
    timeDelta = Abc_Clock() - timeStart;

    if ( pSes->fSatVerbose )
        printf( "   RE:   %2d   ST: %4.f   CO: %7.0f   DE: %6.0f    PR: %6.0f\n",
                status,
                Sat_Wrd2Dbl( pSes->pSat->stats.starts ), Sat_Wrd2Dbl( pSes->pSat->stats.conflicts ),
                Sat_Wrd2Dbl( pSes->pSat->stats.decisions ), Sat_Wrd2Dbl( pSes->pSat->stats.propagations ) );
        //Sat_SolverPrintStats( stdout, pSes->pSat );

    pSes->timeSat += timeDelta;

    if ( status == l_True )
    {
        pSes->nSatCalls++;
        pSes->timeSatSat += timeDelta;
        return 1;
    }
    else if ( status == l_False )
    {
        pSes->nUnsatCalls++;
        pSes->timeSatUnsat += timeDelta;
        return 0;
    }
    else
    {
        pSes->nUndefCalls++;
        pSes->timeSatUndef += timeDelta;
        if ( pSes->fSatVerbose )
        {
            //Sat_SolverWriteDimacs( pSes->pSat, "/tmp/test.cnf", Vec_IntArray( pSes->vAssump ), Vec_IntLimit( pSes->vAssump ), 1 );
            //exit( 42 );

            printf( "resource limit reached\n" );
        }
        return 2;
    }
}

/**Function*************************************************************

  Synopsis    [Extract solution.]

***********************************************************************/
// char is an array of short integers that stores the synthesized network
// using the following format
// | nvars | nfunc | ngates | gate[1] | ... | gate[ngates] | func[1] | .. | func[nfunc] |
// nvars:       integer with number of variables
// nfunc:       integer with number of functions
// ngates:      integer with number of gates
// gate[i]:     | op | nfanin | fanin[1] | ... | fanin[nfanin] |
//   op:        integer of gate's truth table (divided by 2, because gate is normal)
//   nfanin:    integer with number of fanins
//   fanin[i]:  integer to primary input or other gate
// func[i]:     | fanin | delay | pin[1] | ... | pin[nvars] |
//   fanin:     integer as literal to some gate (not primary input), can be complemented
//   delay:     maximum delay to output (taking arrival times into account, not normalized) or 0 if not specified
//   pin[i]:    pin to pin delay to input i or 0 if not specified or if there is no connection to input i
// NOTE: since outputs can only point to gates, delay and pin-to-pin times cannot be 0
static char * Ses_ManExtractSolution( Ses_Man_t * pSes )
{
    int nSol, h, i, j, k, l, aj, ak, d, nOp;
    char * pSol, * p;
    int * pPerm = NULL; /* will be a 2d array [i][l] where is is gate id and l is PI id */

    /* compute length of solution, for now all gates have 2 inputs */
    nSol = 3 + pSes->nGates * 4 + pSes->nSpecFunc * ( 2 + pSes->nSpecVars );

    p = pSol = ABC_CALLOC( char, nSol );

    /* header */
    *p++ = pSes->nSpecVars;
    *p++ = pSes->nSpecFunc;
    *p++ = pSes->nGates;

    /* gates */
    for ( i = 0; i < pSes->nGates; ++i )
    {
        nOp  = sat_solver_var_value( pSes->pSat, Ses_ManGateVar( pSes, i, 0, 1 ) );
        nOp |= sat_solver_var_value( pSes->pSat, Ses_ManGateVar( pSes, i, 1, 0 ) ) << 1;
        nOp |= sat_solver_var_value( pSes->pSat, Ses_ManGateVar( pSes, i, 1, 1 ) ) << 2;

        *p++ = nOp;
        *p++ = 2;

        if ( pSes->fExtractVerbose )
            printf( "add gate %d with operation %d", pSes->nSpecVars + i, nOp );

        for ( k = 0; k < pSes->nSpecVars + i; ++k )
            for ( j = 0; j < k; ++j )
                if ( sat_solver_var_value( pSes->pSat, Ses_ManSelectVar( pSes, i, j, k ) ) )
                {
                    if ( pSes->fExtractVerbose )
                        printf( " and operands %d and %d", j, k );
                    *p++ = j;
                    *p++ = k;
                    k = pSes->nSpecVars + i;
                    break;
                }

        if ( pSes->fExtractVerbose )
        {
            if ( pSes->nMaxDepth > 0 )
            {
                printf( " and depth vector " );
                for ( j = 0; j <= pSes->nArrTimeMax + i; ++j )
                    printf( "%d", sat_solver_var_value( pSes->pSat, Ses_ManDepthVar( pSes, i, j ) ) );
            }
            printf( "\n" );
        }
    }

    /* pin-to-pin delay */
    if ( pSes->nMaxDepth != -1 )
    {
        pPerm = ABC_CALLOC( int, pSes->nGates * pSes->nSpecVars );
        for ( i = 0; i < pSes->nGates; ++i )
        {
            /* since all gates are binary for now */
            j = pSol[3 + i * 4 + 2];
            k = pSol[3 + i * 4 + 3];

            for ( l = 0; l < pSes->nSpecVars; ++l )
            {
                /* pin-to-pin delay to input l of child nodes */
                aj = j < pSes->nSpecVars ? 0 : pPerm[(j - pSes->nSpecVars) * pSes->nSpecVars + l];
                ak = k < pSes->nSpecVars ? 0 : pPerm[(k - pSes->nSpecVars) * pSes->nSpecVars + l];

                if ( aj == 0 && ak == 0 )
                    pPerm[i * pSes->nSpecVars + l] = ( l == j || l == k ) ? 1 : 0;
                else
                    pPerm[i * pSes->nSpecVars + l] = Abc_MaxInt( aj, ak ) + 1;
            }
        }
    }

    /* outputs */
    for ( h = 0; h < pSes->nSpecFunc; ++h )
        for ( i = 0; i < pSes->nGates; ++i )
            if ( sat_solver_var_value( pSes->pSat, Ses_ManOutputVar( pSes, h, i ) ) )
            {
                *p++ = Abc_Var2Lit( i, ( pSes->bSpecInv >> h ) & 1 );
                d = 0;
                if ( pSes->nMaxDepth != -1 )
                    for ( l = 0; l < pSes->nSpecVars; ++l )
                    {
                        if ( pSes->pArrTimeProfile )
                            d = Abc_MaxInt( d, pSes->pArrTimeProfile[l] + pPerm[i * pSes->nSpecVars + l] );
                        else
                            d = Abc_MaxInt( d, pPerm[i * pSes->nSpecVars + l] );
                    }
                *p++ = d;
                if ( pSes->pArrTimeProfile && pSes->fExtractVerbose )
                    printf( "output %d points to gate %d and has normalized delay %d (nArrTimeDelta = %d)\n", h, pSes->nSpecVars + i, d, pSes->nArrTimeDelta );
                for ( l = 0; l < pSes->nSpecVars; ++l )
                {
                    d = ( pSes->nMaxDepth != -1 ) ? pPerm[i * pSes->nSpecVars + l] : 0;
                    if ( pSes->pArrTimeProfile && pSes->fExtractVerbose )
                        printf( "  pin-to-pin arrival time from input %d is %d (pArrTimeProfile = %d)\n", l, d, pSes->pArrTimeProfile[l] );
                    *p++ = d;
                }
            }

    /* pin-to-pin delays */
    if ( pSes->nMaxDepth != -1 )
        ABC_FREE( pPerm );

    /* have we used all the fields? */
    assert( ( p - pSol ) == nSol );

    /* printf( "found network: " ); */
    /* Abc_TtPrintHexRev( stdout, Ses_ManDeriveTruth( pSes, pSol, 1 ), pSes->nSpecVars ); */
    /* printf( "\n" ); */

    return pSol;
}

static Abc_Ntk_t * Ses_ManExtractNtk( char const * pSol )
{
    int h, i;
    char const * p;
    Abc_Ntk_t * pNtk;
    Abc_Obj_t * pObj;
    Vec_Ptr_t * pGates, * vNames;
    char pGateTruth[5];
    char * pSopCover;

    pNtk = Abc_NtkAlloc( ABC_NTK_LOGIC, ABC_FUNC_SOP, 1 );
    pNtk->pName = Extra_UtilStrsav( "exact" );
    pGates = Vec_PtrAlloc( pSol[ABC_EXACT_SOL_NVARS] + pSol[ABC_EXACT_SOL_NGATES] );
    pGateTruth[3] = '0';
    pGateTruth[4] = '\0';
    vNames = Abc_NodeGetFakeNames( pSol[ABC_EXACT_SOL_NVARS] + pSol[ABC_EXACT_SOL_NFUNC] );

    /* primary inputs */
    Vec_PtrPush( pNtk->vObjs, NULL );
    for ( i = 0; i < pSol[ABC_EXACT_SOL_NVARS]; ++i )
    {
        pObj = Abc_NtkCreatePi( pNtk );
        Abc_ObjAssignName( pObj, (char*)Vec_PtrEntry( vNames, i ), NULL );
        Vec_PtrPush( pGates, pObj );
    }

    /* gates */
    p = pSol + 3;
    for ( i = 0; i < pSol[ABC_EXACT_SOL_NGATES]; ++i )
    {
        pGateTruth[2] = '0' + ( *p & 1 );
        pGateTruth[1] = '0' + ( ( *p >> 1 ) & 1 );
        pGateTruth[0] = '0' + ( ( *p >> 2 ) & 1 );
        ++p;

        assert( *p == 2 ); /* binary gate */
        ++p;

        pSopCover = Abc_SopFromTruthBin( pGateTruth );
        pObj = Abc_NtkCreateNode( pNtk );
        pObj->pData = Abc_SopRegister( (Mem_Flex_t*)pNtk->pManFunc, pSopCover );
        Vec_PtrPush( pGates, pObj );
        ABC_FREE( pSopCover );

        Abc_ObjAddFanin( pObj, (Abc_Obj_t *)Vec_PtrEntry( pGates, *p++ ) );
        Abc_ObjAddFanin( pObj, (Abc_Obj_t *)Vec_PtrEntry( pGates, *p++ ) );
    }

    /* outputs */
    for ( h = 0; h < pSol[ABC_EXACT_SOL_NFUNC]; ++h )
    {
        pObj = Abc_NtkCreatePo( pNtk );
        Abc_ObjAssignName( pObj, (char*)Vec_PtrEntry( vNames, pSol[ABC_EXACT_SOL_NVARS] + h ), NULL );
        if ( Abc_LitIsCompl( *p ) )
            Abc_ObjAddFanin( pObj, Abc_NtkCreateNodeInv( pNtk, (Abc_Obj_t *)Vec_PtrEntry( pGates, pSol[ABC_EXACT_SOL_NVARS] + Abc_Lit2Var( *p ) ) ) );
        else
            Abc_ObjAddFanin( pObj, (Abc_Obj_t *)Vec_PtrEntry( pGates, pSol[ABC_EXACT_SOL_NVARS] + Abc_Lit2Var( *p ) ) );
        p += ( 2 + pSol[ABC_EXACT_SOL_NVARS] );
    }
    Abc_NodeFreeNames( vNames );

    Vec_PtrFree( pGates );

    if ( !Abc_NtkCheck( pNtk ) )
        printf( "Ses_ManExtractSolution(): Network check has failed.\n" );

    return pNtk;
}

static Gia_Man_t * Ses_ManExtractGia( char const * pSol )
{
    int h, i;
    char const * p;
    Gia_Man_t * pGia;
    Vec_Int_t * pGates;
    Vec_Ptr_t * vNames;
    int nObj, nChild1, nChild2, fChild1Comp, fChild2Comp;

    pGia = Gia_ManStart( pSol[ABC_EXACT_SOL_NVARS] + pSol[ABC_EXACT_SOL_NGATES] + pSol[ABC_EXACT_SOL_NFUNC] + 1 );
    pGia->nConstrs = 0;
    pGia->pName = Extra_UtilStrsav( "exact" );

    pGates = Vec_IntAlloc( pSol[ABC_EXACT_SOL_NVARS] + pSol[ABC_EXACT_SOL_NGATES] );
    vNames = Abc_NodeGetFakeNames( pSol[ABC_EXACT_SOL_NVARS] + pSol[ABC_EXACT_SOL_NFUNC] );

    /* primary inputs */
    pGia->vNamesIn = Vec_PtrStart( pSol[ABC_EXACT_SOL_NVARS] );
    for ( i = 0; i < pSol[ABC_EXACT_SOL_NVARS]; ++i )
    {
        nObj = Gia_ManAppendCi( pGia );
        Vec_IntPush( pGates, nObj );
        Vec_PtrSetEntry( pGia->vNamesIn, i, Extra_UtilStrsav( (const char*)Vec_PtrEntry( vNames, i ) ) );
    }

    /* gates */
    p = pSol + 3;
    for ( i = 0; i < pSol[ABC_EXACT_SOL_NGATES]; ++i )
    {
        assert( p[1] == 2 );
        nChild1 = Vec_IntEntry( pGates, p[2] );
        nChild2 = Vec_IntEntry( pGates, p[3] );
        fChild1Comp = fChild2Comp = 0;

        if ( *p & 1 )
        {
            nChild1 = Abc_LitNot( nChild1 );
            fChild1Comp = 1;
        }
        if ( ( *p >> 1 ) & 1 )
        {
            nChild2 = Abc_LitNot( nChild2 );
            fChild2Comp = 1;
        }
        nObj = Gia_ManAppendAnd( pGia, nChild1, nChild2 );
        if ( fChild1Comp && fChild2Comp )
        {
            assert( ( *p >> 2 ) & 1 );
            nObj = Abc_LitNot( nObj );
        }

        Vec_IntPush( pGates, nObj );

        p += 4;
    }

    /* outputs */
    pGia->vNamesOut = Vec_PtrStart( pSol[ABC_EXACT_SOL_NFUNC] );
    for ( h = 0; h < pSol[ABC_EXACT_SOL_NFUNC]; ++h )
    {
        nObj = Vec_IntEntry( pGates, pSol[ABC_EXACT_SOL_NVARS] + Abc_Lit2Var( *p ) );
        if ( Abc_LitIsCompl( *p ) )
            nObj = Abc_LitNot( nObj );
        Gia_ManAppendCo( pGia, nObj );
        Vec_PtrSetEntry( pGia->vNamesOut, h, Extra_UtilStrsav( (const char*)Vec_PtrEntry( vNames, pSol[ABC_EXACT_SOL_NVARS] + h ) ) );
        p += ( 2 + pSol[ABC_EXACT_SOL_NVARS] );
    }
    Abc_NodeFreeNames( vNames );

    Vec_IntFree( pGates );

    return pGia;
}

/**Function*************************************************************

  Synopsis    [Debug.]

***********************************************************************/
static void Ses_ManPrintRuntime( Ses_Man_t * pSes )
{
    printf( "Runtime breakdown:\n" );
    ABC_PRTP( "Sat     ",  pSes->timeSat,      pSes->timeTotal );
    ABC_PRTP( " Sat    ",  pSes->timeSatSat,   pSes->timeTotal );
    ABC_PRTP( " Unsat  ",  pSes->timeSatUnsat, pSes->timeTotal );
    ABC_PRTP( " Undef  ",  pSes->timeSatUndef, pSes->timeTotal );
    ABC_PRTP( "Instance", pSes->timeInstance,  pSes->timeTotal );
    ABC_PRTP( "ALL     ",  pSes->timeTotal,    pSes->timeTotal );
}

static inline void Ses_ManPrintFuncs( Ses_Man_t * pSes )
{
    int h;

    printf( "find optimum circuit for %d %d-variable functions:\n", pSes->nSpecFunc, pSes->nSpecVars );
    for ( h = 0; h < pSes->nSpecFunc; ++h )
    {
        printf( "  func %d: ", h + 1 );
        Abc_TtPrintHexRev( stdout, &pSes->pSpec[h << 2], pSes->nSpecVars );
        printf( "\n" );
    }

    if ( pSes->nMaxDepth != -1 )
    {
        printf( "  max depth = %d\n", pSes->nMaxDepth );
        if ( pSes->pArrTimeProfile )
        {
            printf( "  arrival times =" );
            for ( h = 0; h < pSes->nSpecVars; ++h )
                printf( " %d", pSes->pArrTimeProfile[h] );
            printf( "\n" );
        }
    }
}

static inline void Ses_ManPrintVars( Ses_Man_t * pSes )
{
    int h, i, j, k, p, q, t;

    for ( i = 0; i < pSes->nGates; ++i )
        for ( t = 0; t < pSes->nRows; ++t )
            printf( "x(%d, %d) : %d\n", i, t, Ses_ManSimVar( pSes, i, t ) );

    for ( h = 0; h < pSes->nSpecFunc; ++h )
        for ( i = 0; i < pSes->nGates; ++i )
            printf( "h(%d, %d) : %d\n", h, i, Ses_ManOutputVar( pSes, h, i ) );

    for ( i = 0; i < pSes->nGates; ++i )
        for ( p = 0; p <= 1; ++p )
            for ( q = 0; q <= 1; ++ q)
            {
                if ( p == 0 && q == 0 ) { continue; }
                printf( "f(%d, %d, %d) : %d\n", i, p, q, Ses_ManGateVar( pSes, i, p, q ) );
            }

    for ( i = 0; i < pSes->nGates; ++i )
        for ( j = 0; j < pSes->nSpecVars + i; ++j )
            for ( k = j + 1; k < pSes->nSpecVars + i; ++k )
                printf( "s(%d, %d, %d) : %d\n", i, j, k, Ses_ManSelectVar( pSes, i, j, k ) );

    if ( pSes->nMaxDepth > 0 )
        for ( i = 0; i < pSes->nGates; ++i )
            for ( j = 0; j <= i; ++j )
                printf( "d(%d, %d) : %d\n", i, j, Ses_ManDepthVar( pSes, i, j ) );

}

/**Function*************************************************************

  Synopsis    [Synthesis algorithm.]

***********************************************************************/
static void Ses_ManComputeMaxGates( Ses_Man_t * pSes )
{
    int s = 1, lvl = pSes->nMaxDepth, avail = pSes->nSpecVars, l;

    pSes->nMaxGates = 0;

    /* s is the number of gates we have in the current level */
    while ( s && /* while there are nodes to branch from */
            lvl && /* while we are at some level */
            avail * 2 > s /* while there are still enough available nodes (heuristic) */ )
    {
        /* erase from s if we have arriving nodes */
        for ( l = 0; l < pSes->nSpecVars; ++l )
            if ( pSes->pArrTimeProfile[l] == lvl )
            {
                --s;
                --avail;
            }

        --lvl;
        pSes->nMaxGates += s;
        s *= 2;
    }
}

// returns 0, if current max depth and arrival times are not consistent
static int Ses_CheckDepthConsistency( Ses_Man_t * pSes )
{
    int l, i, fAdded, nLevel;
    int fMaxGatesLevel2 = 1;

    Vec_IntClear( pSes->vStairDecVars );
    pSes->fDecStructure = 0;

    for ( l = 0; l < pSes->nSpecVars; ++l )
    {
        if ( pSes->pArrTimeProfile[l] >= pSes->nMaxDepth )
        {
            if ( pSes->fReasonVerbose )
                printf( "give up due to impossible arrival time (depth = %d, input = %d, arrival time = %d)", pSes->nMaxDepth, l, pSes->pArrTimeProfile[l] );
            return 0;
        }
        else if ( pSes->nSpecFunc == 1 && pSes->pArrTimeProfile[l] + 1 == pSes->nMaxDepth )
        {
            if ( ( pSes->fDecStructure == 1 && pSes->nSpecVars > 2 ) || pSes->fDecStructure == 2 )
            {
                if ( pSes->fReasonVerbose )
                    printf( "give up due to impossible decomposition (depth = %d, input = %d, arrival time = %d)", pSes->nMaxDepth, l, pSes->pArrTimeProfile[l] );
                return 0;
            }

            pSes->fDecStructure++;

            /* A subset B <=> A and B = A */
            if ( !( ( pSes->pDecVars >> l ) & 1 ) )
            {
                if ( pSes->fReasonVerbose )
                    printf( "give up due to impossible decomposition (depth = %d, input = %d, arrival time = %d)", pSes->nMaxDepth, l, pSes->pArrTimeProfile[l] );
                return 0;
            }
        }
    }

    /* more complicated decomposition */
    if ( pSes->fDecStructure )
    {
        nLevel = 1;
        while ( 1 )
        {
            fAdded = 0;

            for ( l = 0; l < pSes->nSpecVars; ++l )
                if ( pSes->pArrTimeProfile[l] + nLevel == pSes->nMaxDepth )
                {
                    if ( fAdded )
                    {
                        assert( nLevel == Vec_IntSize( pSes->vStairDecVars ) );
                        if ( fAdded > 1 || ( nLevel + 1 < pSes->nSpecVars ) )
                        {
                            if ( pSes->fReasonVerbose )
                                printf( "give up due to impossible decomposition at level %d", nLevel );
                            return 0;
                        }
                    }
                    else
                    {
                        Vec_IntPush( pSes->vStairDecVars, l );
                    }
                    fAdded++;
                }

            if ( !fAdded ) break;
            ++nLevel;
        }

        if ( Vec_IntSize( pSes->vStairDecVars ) && !Abc_TtIsStairDecomposable( pSes->pSpec, pSes->nSpecWords, Vec_IntArray( pSes->vStairDecVars ), Vec_IntSize( pSes->vStairDecVars ), pSes->pStairDecFunc ) )
        {
            if ( pSes->fReasonVerbose )
            {
                printf( "give up due to impossible stair decomposition at level %d: ", nLevel );
                Vec_IntPrint( pSes->vStairDecVars );
            }
            return 0;
        }
    }

    /* check if depth's match with structure at second level from top */
    if ( pSes->fDecStructure )
        fMaxGatesLevel2 = ( pSes->nSpecVars == 3 ) ? 2 : 1;
    else
        fMaxGatesLevel2 = ( pSes->nSpecVars == 4 ) ? 4 : 3;

    i = 0;
    for ( l = 0; l < pSes->nSpecVars; ++l )
        if ( pSes->pArrTimeProfile[l] + 2 == pSes->nMaxDepth )
            if ( ++i > fMaxGatesLevel2 )
            {
                if ( pSes->fReasonVerbose )
                    printf( "give up due to impossible decomposition at second level (depth = %d, input = %d, arrival time = %d)", pSes->nMaxDepth, l, pSes->pArrTimeProfile[l] );
                return 0;
            }

    /* check if depth's match with structure at third level from top */
    if ( pSes->nSpecVars > 4 && pSes->fDecStructure && i == 1 ) /* we have f = AND(x_i, AND(x_j, g)) (x_i and x_j may be complemented) */
    {
        i = 0;
        for ( l = 0; l < pSes->nSpecVars; ++l )
            if ( pSes->pArrTimeProfile[l] + 3 == pSes->nMaxDepth )
                if ( ++i > 1 )
                {
                    if ( pSes->fReasonVerbose )
                        printf( "give up due to impossible decomposition at third level (depth = %d, input = %d, arrival time = %d)", pSes->nMaxDepth, l, pSes->pArrTimeProfile[l] );
                    return 0;
                }
    }

    return 1;
}

// returns 0, if current max depth and #gates are not consistent
static int Ses_CheckGatesConsistency( Ses_Man_t * pSes, int nGates )
{
    /* give up if number of gates is impossible for given depth */
    if ( pSes->nMaxDepth != -1 && nGates >= ( 1 << pSes->nMaxDepth ) )
    {
        if ( pSes->fReasonVerbose )
            printf( "give up due to impossible depth (depth = %d, gates = %d)", pSes->nMaxDepth, nGates );
        return 0;
    }

    /* give up if number of gates is impossible for given depth and arrival times */
    if ( pSes->nMaxDepth != -1 && pSes->pArrTimeProfile && nGates > pSes->nMaxGates )
    {
        if ( pSes->fReasonVerbose )
            printf( "give up due to impossible depth and arrival times (depth = %d, gates = %d)", pSes->nMaxDepth, nGates );
        return 0;
    }

    if ( pSes->fDecStructure && nGates >= ( 1 << ( pSes->nMaxDepth - 1 ) ) + 1 )
    {
        if ( pSes->fReasonVerbose )
            printf( "give up due to impossible depth in AND-dec structure (depth = %d, gates = %d)", pSes->nMaxDepth, nGates );
        return 0;
    }

    /* give up if number of gates gets practically too large */
    if ( nGates >= ( 1 << pSes->nSpecVars ) )
    {
        if ( pSes->fReasonVerbose )
            printf( "give up due to impossible number of gates" );
        return 0;
    }

    return 1;
}

static void Abc_ExactCopyDepthAndArrivalTimes( int nVars, int nDepthFrom, int * nDepthTo, int * pTimesFrom, int * pTimesTo )
{
    int l;

    *nDepthTo = nDepthFrom;
    for ( l = 0; l < nVars; ++l )
        pTimesTo[l] = pTimesFrom[l];
}

static void Ses_ManStoreDepthAndArrivalTimes( Ses_Man_t * pSes )
{
    if ( pSes->nMaxDepth == -1 ) return;
    Abc_ExactCopyDepthAndArrivalTimes( pSes->nSpecVars, pSes->nMaxDepth, &pSes->nMaxDepthTmp, pSes->pArrTimeProfile, pSes->pArrTimeProfileTmp );
}

static void Ses_ManRestoreDepthAndArrivalTimes( Ses_Man_t * pSes )
{
    if ( pSes->nMaxDepth == -1 ) return;
    Abc_ExactCopyDepthAndArrivalTimes( pSes->nSpecVars, pSes->nMaxDepthTmp, &pSes->nMaxDepth, pSes->pArrTimeProfileTmp, pSes->pArrTimeProfile );
}

static void Abc_ExactAdjustDepthAndArrivalTimes( int nVars, int nGates, int nDepthFrom, int * nDepthTo, int * pTimesFrom, int * pTimesTo, int nTimesMax )
{
    int l;

    *nDepthTo = Abc_MinInt( nDepthFrom, nGates );
    for ( l = 0; l < nVars; ++l )
        pTimesTo[l] = Abc_MinInt( pTimesFrom[l], Abc_MaxInt( 0, nGates - 1 - nTimesMax + pTimesFrom[l] ) );
}

static void Ses_AdjustDepthAndArrivalTimes( Ses_Man_t * pSes, int nGates )
{
    Abc_ExactAdjustDepthAndArrivalTimes( pSes->nSpecVars, nGates, pSes->nMaxDepthTmp, &pSes->nMaxDepth, pSes->pArrTimeProfileTmp, pSes->pArrTimeProfile, pSes->nArrTimeMax - 1 );
}

/* return: (2: continue, 1: found, 0: gave up) */
static int Ses_ManFindNetworkExact( Ses_Man_t * pSes, int nGates )
{
    int f, fSat;
    abctime timeStart;

    /* solve */
    timeStart = Abc_Clock();
    Vec_IntClear( pSes->vPolar );
    Vec_IntClear( pSes->vAssump );
    Ses_ManCreateVars( pSes, nGates );

    if ( pSes->nMaxDepth != -1 )
    {
        f = Ses_ManCreateDepthClauses( pSes );
        pSes->timeInstance += ( Abc_Clock() - timeStart );
        if ( !f ) return 2; /* proven UNSAT while creating clauses */
    }

    sat_solver_set_polarity( pSes->pSat, Vec_IntArray( pSes->vPolar ), Vec_IntSize( pSes->vPolar ) );

    /* first solve */
    fSat = Ses_ManSolve( pSes );
    if ( fSat == 0 )
        return 2; /* UNSAT, continue with next # of gates */
    else if ( fSat == 2 )
    {
        pSes->fHitResLimit = 1;
        return 0;
    }

    timeStart = Abc_Clock();
    f = Ses_ManCreateClauses( pSes );
    pSes->timeInstance += ( Abc_Clock() - timeStart );
    if ( !f ) return 2; /* proven UNSAT while creating clauses */

    fSat = Ses_ManSolve( pSes );
    if ( fSat == 1 )
        return 1;
    else if ( fSat == 2 )
    {
        pSes->fHitResLimit = 1;
        return 0;
    }

    return 2; /* UNSAT continue */
}

// is there a network for a given number of gates
/* return: (3: impossible, 2: continue, 1: found, 0: gave up) */
static int Ses_ManFindNetworkExactCEGAR( Ses_Man_t * pSes, int nGates, char ** pSol )
{
    int fRes, iMint, fSat, i;
    word pTruth[4];

    /* debug */
    Abc_DebugErase( pSes->nDebugOffset + ( nGates > 10 ? 5 : 4 ), pSes->fVeryVerbose );
    Abc_DebugPrintIntInt( " (%d/%d)", nGates, pSes->nMaxGates, pSes->fVeryVerbose );

    /* do #gates and max depth allow for a network? */
    if ( !Ses_CheckGatesConsistency( pSes, nGates ) )
        return 3;

    for ( i = 0; i < pSes->nRandRowAssigns; ++i )
        Abc_TtSetBit( pSes->pTtValues, rand() % pSes->nRows );

    fRes = Ses_ManFindNetworkExact( pSes, nGates );
    if ( fRes != 1 ) return fRes;

    while ( true )
    {
        *pSol = Ses_ManExtractSolution( pSes );
        Abc_TtXor( pTruth, Ses_ManDeriveTruth( pSes, *pSol, 0 ), pSes->pSpec, pSes->nSpecWords, 0 );
        iMint = Abc_TtFindFirstBit( pTruth, pSes->nSpecVars );

        if ( iMint == -1 || (pSes->nSpecVars < 6 && iMint > pSes->nRows) )
        {
            assert( fRes == 1 );
            return 1;
        }
        ABC_FREE( *pSol );

        if ( pSes->fKeepRowAssigns )
            Abc_TtSetBit( pSes->pTtValues, iMint - 1 );
        if ( !Ses_ManCreateTruthTableClause( pSes, iMint - 1 ) ) /* UNSAT, continue */
            return 2;

        if ( ( fSat = Ses_ManSolve( pSes ) ) == 1 ) continue;

        return ( fSat == 2 ) ? 0 : 2;
    }
}

// find minimum size by increasing the number of gates
static char * Ses_ManFindMinimumSizeBottomUp( Ses_Man_t * pSes )
{
    int nGates = pSes->nStartGates, fRes;
    char * pSol = NULL;

    /* store whether call was unsuccessful due to resource limit and not due to impossible constraint */
    pSes->fHitResLimit = 0;

    pSes->nDebugOffset = pSes->nMaxGates >= 10 ? 3 : 2;

    /* adjust number of gates if there is a stair decomposition */
    if ( Vec_IntSize( pSes->vStairDecVars ) )
        nGates = Abc_MaxInt( nGates, Vec_IntSize( pSes->vStairDecVars ) - 1 );

    //Ses_ManStoreDepthAndArrivalTimes( pSes );

    memset( pSes->pTtValues, 0, 4 * sizeof( word ) );

    Abc_DebugPrintIntInt( " (%d/%d)", nGates, pSes->nMaxGates, pSes->fVeryVerbose );

    while ( true )
    {
        ++nGates;

        fRes = Ses_ManFindNetworkExactCEGAR( pSes, nGates, &pSol );

        if ( fRes == 0 )
        {
            pSes->fHitResLimit = 1;
            break;
        }
        else if ( fRes == 1 || fRes == 3 )
            break;
    }

    Abc_DebugErase( pSes->nDebugOffset + ( nGates >= 10 ? 5 : 4 ), pSes->fVeryVerbose );

    return pSol;
}

static char * Ses_ManFindMinimumSizeTopDown( Ses_Man_t * pSes, int nMinGates )
{
    int nGates = pSes->nMaxGates, fRes;
    char * pSol = NULL, * pSol2 = NULL;

    pSes->fHitResLimit = 0;

    Abc_DebugPrintIntInt( " (%d/%d)", nGates, pSes->nMaxGates, pSes->fVeryVerbose );

    while ( true )
    {
        fRes = Ses_ManFindNetworkExactCEGAR( pSes, nGates, &pSol2 );

        if ( fRes == 0 )
        {
            pSes->fHitResLimit = 1;
            break;
        }
        else if ( fRes == 2 || fRes == 3 )
            break;

        pSol = pSol2;

        if ( nGates == nMinGates )
            break;

        --nGates;
    }

    Abc_DebugErase( pSes->nDebugOffset + ( nGates >= 10 ? 5 : 4 ), pSes->fVeryVerbose );

    return pSol;
}

static char * Ses_ManFindMinimumSize( Ses_Man_t * pSes )
{
    char * pSol = NULL;
    int i = pSes->nStartGates + 1, fRes;

    /* if more than one function, no CEGAR */
    if ( pSes->nSpecFunc > 1 )
    {
      while ( true )
      {
        if ( pSes->fVerbose )
        {
          printf( "try with %d gates\n", i );
        }

        memset( pSes->pTtValues, ~0, 4 * sizeof( word ) );
        fRes = Ses_ManFindNetworkExact( pSes, i++ );
        if ( fRes == 2 ) continue;
        if ( fRes == 0 ) break;

        pSol = Ses_ManExtractSolution( pSes );
        break;
      }

      return pSol;
    }

    /* do the arrival times allow for a network? */
    if ( pSes->nMaxDepth != -1 && pSes->pArrTimeProfile )
    {
        if ( !Ses_CheckDepthConsistency( pSes ) )
            return 0;
        Ses_ManComputeMaxGates( pSes );
    }

    pSol = Ses_ManFindMinimumSizeBottomUp( pSes );

    if ( !pSol && pSes->nMaxDepth != -1 && pSes->fHitResLimit && pSes->nGates != pSes->nMaxGates )
        return Ses_ManFindMinimumSizeTopDown( pSes, pSes->nGates + 1 );
    else
        return pSol;
}


/**Function*************************************************************

  Synopsis    [Find minimum size networks with a SAT solver.]

  Description [If nMaxDepth is -1, then depth constraints are ignored.
               If nMaxDepth is not -1, one can set pArrTimeProfile which should have the length of nVars.
               One can ignore pArrTimeProfile by setting it to NULL.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkFindExact( word * pTruth, int nVars, int nFunc, int nMaxDepth, int * pArrTimeProfile, int nBTLimit, int nStartGates, int fVerbose )
{
    Ses_Man_t * pSes;
    char * pSol;
    Abc_Ntk_t * pNtk = NULL;
    abctime timeStart;

    /* some checks */
    assert( nVars >= 2 && nVars <= 8 );

    timeStart = Abc_Clock();

    pSes = Ses_ManAlloc( pTruth, nVars, nFunc, nMaxDepth, pArrTimeProfile, 0, nBTLimit, fVerbose );
    pSes->nStartGates = nStartGates;
    pSes->fReasonVerbose = 0;
    pSes->fSatVerbose = 0;
    if ( fVerbose )
        Ses_ManPrintFuncs( pSes );

    if ( ( pSol = Ses_ManFindMinimumSize( pSes ) ) != NULL )
    {
        pNtk = Ses_ManExtractNtk( pSol );
        ABC_FREE( pSol );
    }

    pSes->timeTotal = Abc_Clock() - timeStart;

    if ( fVerbose )
        Ses_ManPrintRuntime( pSes );

    /* cleanup */
    Ses_ManClean( pSes );

    return pNtk;
}

Gia_Man_t * Gia_ManFindExact( word * pTruth, int nVars, int nFunc, int nMaxDepth, int * pArrTimeProfile, int nBTLimit, int nStartGates, int fVerbose )
{
    Ses_Man_t * pSes;
    char * pSol;
    Gia_Man_t * pGia = NULL;
    abctime timeStart;

    /* some checks */
    assert( nVars >= 2 && nVars <= 8 );

    timeStart = Abc_Clock();

    pSes = Ses_ManAlloc( pTruth, nVars, nFunc, nMaxDepth, pArrTimeProfile, 1, nBTLimit, fVerbose );
    pSes->nStartGates = nStartGates;
    pSes->fVeryVerbose = 1;
    pSes->fExtractVerbose = 0;
    pSes->fSatVerbose = 0;
    pSes->fReasonVerbose = 1;
    if ( fVerbose )
        Ses_ManPrintFuncs( pSes );

    if ( ( pSol = Ses_ManFindMinimumSize( pSes ) ) != NULL )
    {
        pGia = Ses_ManExtractGia( pSol );
        ABC_FREE( pSol );
    }

    pSes->timeTotal = Abc_Clock() - timeStart;

    if ( fVerbose )
        Ses_ManPrintRuntime( pSes );

    /* cleanup */
    Ses_ManClean( pSes );

    return pGia;
}

/**Function*************************************************************

  Synopsis    [Some test cases.]

***********************************************************************/

Abc_Ntk_t * Abc_NtkFromTruthTable( word * pTruth, int nVars )
{
    Abc_Ntk_t * pNtk;
    Mem_Flex_t * pMan;
    char * pSopCover;

    pMan = Mem_FlexStart();
    pSopCover = Abc_SopCreateFromTruth( pMan, nVars, (unsigned*)pTruth );
    pNtk = Abc_NtkCreateWithNode( pSopCover );
    Abc_NtkShortNames( pNtk );
    Mem_FlexStop( pMan, 0 );

    return pNtk;
}

void Abc_ExactTestSingleOutput( int fVerbose )
{
    extern void Abc_NtkCecSat( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int nConfLimit, int nInsLimit );

    word pTruth[4] = {0xcafe, 0, 0, 0};
    Abc_Ntk_t * pNtk, * pNtk2, * pNtk3, * pNtk4;
    int pArrTimeProfile[4] = {6, 2, 8, 5};

    pNtk = Abc_NtkFromTruthTable( pTruth, 4 );

    pNtk2 = Abc_NtkFindExact( pTruth, 4, 1, -1, NULL, 0, 0, fVerbose );
    Abc_NtkShortNames( pNtk2 );
    Abc_NtkCecSat( pNtk, pNtk2, 10000, 0 );
    assert( pNtk2 );
    assert( Abc_NtkNodeNum( pNtk2 ) == 6 );
    Abc_NtkDelete( pNtk2 );

    pNtk3 = Abc_NtkFindExact( pTruth, 4, 1, 3, NULL, 0, 0, fVerbose );
    Abc_NtkShortNames( pNtk3 );
    Abc_NtkCecSat( pNtk, pNtk3, 10000, 0 );
    assert( pNtk3 );
    assert( Abc_NtkLevel( pNtk3 ) <= 3 );
    Abc_NtkDelete( pNtk3 );

    pNtk4 = Abc_NtkFindExact( pTruth, 4, 1, 9, pArrTimeProfile, 50000, 0, fVerbose );
    Abc_NtkShortNames( pNtk4 );
    Abc_NtkCecSat( pNtk, pNtk4, 10000, 0 );
    assert( pNtk4 );
    assert( Abc_NtkLevel( pNtk4 ) <= 9 );
    Abc_NtkDelete( pNtk4 );

    assert( !Abc_NtkFindExact( pTruth, 4, 1, 2, NULL, 50000, 0, fVerbose ) );

    assert( !Abc_NtkFindExact( pTruth, 4, 1, 8, pArrTimeProfile, 50000, 0, fVerbose ) );

    Abc_NtkDelete( pNtk );
}

void Abc_ExactTestSingleOutputAIG( int fVerbose )
{
    word pTruth[4] = {0xcafe, 0, 0, 0};
    Abc_Ntk_t * pNtk;
    Gia_Man_t * pGia, * pGia2, * pGia3, * pGia4, * pMiter;
    Cec_ParCec_t ParsCec, * pPars = &ParsCec;
    int pArrTimeProfile[4] = {6, 2, 8, 5};

    Cec_ManCecSetDefaultParams( pPars );

    pNtk = Abc_NtkFromTruthTable( pTruth, 4 );
    Abc_NtkToAig( pNtk );
    pGia = Abc_NtkAigToGia( pNtk, 1 );

    pGia2 = Gia_ManFindExact( pTruth, 4, 1, -1, NULL, 0, 0, fVerbose );
    pMiter = Gia_ManMiter( pGia, pGia2, 0, 1, 0, 0, 1 );
    assert( pMiter );
    Cec_ManVerify( pMiter, pPars );
    Gia_ManStop( pMiter );

    pGia3 = Gia_ManFindExact( pTruth, 4, 1, 3, NULL, 0, 0, fVerbose );
    pMiter = Gia_ManMiter( pGia, pGia3, 0, 1, 0, 0, 1 );
    assert( pMiter );
    Cec_ManVerify( pMiter, pPars );
    Gia_ManStop( pMiter );

    pGia4 = Gia_ManFindExact( pTruth, 4, 1, 9, pArrTimeProfile, 50000, 0, fVerbose );
    pMiter = Gia_ManMiter( pGia, pGia4, 0, 1, 0, 0, 1 );
    assert( pMiter );
    Cec_ManVerify( pMiter, pPars );
    Gia_ManStop( pMiter );

    assert( !Gia_ManFindExact( pTruth, 4, 1, 2, NULL, 50000, 0, fVerbose ) );

    assert( !Gia_ManFindExact( pTruth, 4, 1, 8, pArrTimeProfile, 50000, 0, fVerbose ) );

    Gia_ManStop( pGia );
    Gia_ManStop( pGia2 );
    Gia_ManStop( pGia3 );
    Gia_ManStop( pGia4 );
}

void Abc_ExactTest( int fVerbose )
{
    Abc_ExactTestSingleOutput( fVerbose );
    Abc_ExactTestSingleOutputAIG( fVerbose );

    printf( "\n" );
}


/**Function*************************************************************

  Synopsis    [APIs for integraging with the mapper.]

***********************************************************************/
// may need to have a static pointer to the SAT-based synthesis engine and/or loaded library
// this procedure should return 1, if the engine/library are available, and 0 otherwise
int Abc_ExactIsRunning()
{
    return s_pSesStore != NULL;
}
// this procedure returns the number of inputs of the library
// for example, somebody may try to map into 10-cuts while the library only contains 8-functions
int Abc_ExactInputNum()
{
    return 8;
}
// start exact store manager
void Abc_ExactStart( int nBTLimit, int fMakeAIG, int fVerbose, int fVeryVerbose, const char * pFilename )
{
    if ( !s_pSesStore )
    {
        s_pSesStore = Ses_StoreAlloc( nBTLimit, fMakeAIG, fVerbose );
        s_pSesStore->fVeryVerbose = fVeryVerbose;
        if ( pFilename )
        {
            Ses_StoreRead( s_pSesStore, pFilename, 1, 0, 0, 0 );

            s_pSesStore->szDBName = ABC_CALLOC( char, strlen( pFilename ) + 1 );
            strcpy( s_pSesStore->szDBName, pFilename );
        }
        if ( s_pSesStore->fVeryVerbose )
        {
            s_pSesStore->pDebugEntries = fopen( "bms.debug", "w" );
        }
    }
    else
        printf( "BMS manager already started\n" );
}
// stop exact store manager
void Abc_ExactStop( const char * pFilename )
{
    if ( s_pSesStore )
    {
        if ( pFilename )
            Ses_StoreWrite( s_pSesStore, pFilename, 1, 0, 0, 0 );
        if ( s_pSesStore->pDebugEntries )
            fclose( s_pSesStore->pDebugEntries );
        Ses_StoreClean( s_pSesStore );
    }
    else
        printf( "BMS manager has not been started\n" );
}
// show statistics about store manager
void Abc_ExactStats()
{
    int i;

    if ( !s_pSesStore )
    {
        printf( "BMS manager has not been started\n" );
        return;
    }

    printf( "-------------------------------------------------------------------------------------------------------------------------------\n" );
    printf( "                                    0         1         2         3         4         5         6         7         8     total\n" );
    printf( "-------------------------------------------------------------------------------------------------------------------------------\n" );
    printf( "number of considered cuts :" );
    for ( i = 0; i < 9; ++i )
        printf( "%10lu", s_pSesStore->pCutCount[i] );
    printf( "%10lu\n", s_pSesStore->nCutCount );
    printf( " - trivial                :" );
    for ( i = 0; i < 9; ++i )
        printf( "%10lu", s_pSesStore->pSynthesizedTrivial[i] );
    printf( "%10lu\n", s_pSesStore->nSynthesizedTrivial );
    printf( " - synth (imp)            :" );
    for ( i = 0; i < 9; ++i )
        printf( "%10lu", s_pSesStore->pSynthesizedImp[i] );
    printf( "%10lu\n", s_pSesStore->nSynthesizedImp );
    printf( " - synth (res)            :" );
    for ( i = 0; i < 9; ++i )
        printf( "%10lu", s_pSesStore->pSynthesizedRL[i] );
    printf( "%10lu\n", s_pSesStore->nSynthesizedRL );
    printf( " - not synth (imp)        :" );
    for ( i = 0; i < 9; ++i )
        printf( "%10lu", s_pSesStore->pUnsynthesizedImp[i] );
    printf( "%10lu\n", s_pSesStore->nUnsynthesizedImp );
    printf( " - not synth (res)        :" );
    for ( i = 0; i < 9; ++i )
        printf( "%10lu", s_pSesStore->pUnsynthesizedRL[i] );
    printf( "%10lu\n", s_pSesStore->nUnsynthesizedRL );
    printf( " - cache hits             :" );
    for ( i = 0; i < 9; ++i )
        printf( "%10lu", s_pSesStore->pCacheHits[i] );
    printf( "%10lu\n", s_pSesStore->nCacheHits );
    printf( "-------------------------------------------------------------------------------------------------------------------------------\n" );
    printf( "number of entries         : %d\n", s_pSesStore->nEntriesCount );
    printf( "number of valid entries   : %d\n", s_pSesStore->nValidEntriesCount );
    printf( "number of invalid entries : %d\n", s_pSesStore->nEntriesCount - s_pSesStore->nValidEntriesCount );
    printf( "-------------------------------------------------------------------------------------------------------------------------------\n" );
    printf( "number of SAT calls       : %lu\n", s_pSesStore->nSatCalls );
    printf( "number of UNSAT calls     : %lu\n", s_pSesStore->nUnsatCalls );
    printf( "number of UNDEF calls     : %lu\n", s_pSesStore->nUndefCalls );

    printf( "-------------------------------------------------------------------------------------------------------------------------------\n" );
    printf( "Runtime breakdown:\n" );
    ABC_PRTP( "Exact    ", s_pSesStore->timeExact,                          s_pSesStore->timeTotal );
    ABC_PRTP( " Sat     ", s_pSesStore->timeSat,                            s_pSesStore->timeTotal );
    ABC_PRTP( "  Sat    ", s_pSesStore->timeSatSat,                         s_pSesStore->timeTotal );
    ABC_PRTP( "  Unsat  ", s_pSesStore->timeSatUnsat,                       s_pSesStore->timeTotal );
    ABC_PRTP( "  Undef  ", s_pSesStore->timeSatUndef,                       s_pSesStore->timeTotal );
    ABC_PRTP( " Instance", s_pSesStore->timeInstance,                       s_pSesStore->timeTotal );
    ABC_PRTP( "Other    ", s_pSesStore->timeTotal - s_pSesStore->timeExact, s_pSesStore->timeTotal );
    ABC_PRTP( "ALL      ", s_pSesStore->timeTotal,                          s_pSesStore->timeTotal );
}
// this procedure takes TT and input arrival times (pArrTimeProfile) and return the smallest output arrival time;
// it also returns the pin-to-pin delays (pPerm) between each cut leaf and the cut output and the cut area cost (Cost)
// the area cost should not exceed 2048, if the cut is implementable; otherwise, it should be ABC_INFINITY
int Abc_ExactDelayCost( word * pTruth, int nVars, int * pArrTimeProfile, char * pPerm, int * Cost, int AigLevel )
{
    int i, nMaxArrival, nDelta, l;
    Ses_Man_t * pSes = NULL;
    char * pSol = NULL, * pSol2 = NULL, * p;
    int pNormalArrTime[8];
    int Delay = ABC_INFINITY, nMaxDepth, fResLimit;
    abctime timeStart = Abc_Clock(), timeStartExact;

    /* some checks */
    if ( nVars < 0 || nVars > 8 )
    {
        printf( "invalid truth table size %d\n", nVars );
        assert( 0 );
    }

    /* statistics */
    s_pSesStore->nCutCount++;
    s_pSesStore->pCutCount[nVars]++;

    if ( nVars == 0 )
    {
        s_pSesStore->nSynthesizedTrivial++;
        s_pSesStore->pSynthesizedTrivial[0]++;

        *Cost = 0;
        s_pSesStore->timeTotal += ( Abc_Clock() - timeStart );
        return 0;
    }

    if ( nVars == 1 )
    {
        s_pSesStore->nSynthesizedTrivial++;
        s_pSesStore->pSynthesizedTrivial[1]++;

        *Cost = 0;
        pPerm[0] = (char)0;
        s_pSesStore->timeTotal += ( Abc_Clock() - timeStart );
        return pArrTimeProfile[0];
    }

    for ( l = 0; l < nVars; ++l )
        pNormalArrTime[l] = pArrTimeProfile[l];

    nDelta = Abc_NormalizeArrivalTimes( pNormalArrTime, nVars, &nMaxArrival );

    *Cost = ABC_INFINITY;

    if ( Ses_StoreGetEntry( s_pSesStore, pTruth, nVars, pNormalArrTime, &pSol ) )
    {
        s_pSesStore->nCacheHits++;
        s_pSesStore->pCacheHits[nVars]++;
    }
    else
    {
        if ( s_pSesStore->fVeryVerbose )
        {
            printf( ANSI_COLOR_CYAN );
            Abc_TtPrintHexRev( stdout, pTruth, nVars );
            printf( ANSI_COLOR_RESET );
            printf( " [%d", pNormalArrTime[0] );
            for ( l = 1; l < nVars; ++l )
                printf( " %d", pNormalArrTime[l] );
            printf( "]@%d:", AigLevel );
            fflush( stdout );
        }

        nMaxDepth = pNormalArrTime[0];
        for ( i = 1; i < nVars; ++i )
            nMaxDepth = Abc_MaxInt( nMaxDepth, pNormalArrTime[i] );
        nMaxDepth += nVars + 1;
        if ( AigLevel != -1 )
            nMaxDepth = Abc_MinInt( AigLevel - nDelta, nMaxDepth + nVars + 1 );

        timeStartExact = Abc_Clock();

        pSes = Ses_ManAlloc( pTruth, nVars, 1 /* nSpecFunc */, nMaxDepth, pNormalArrTime, s_pSesStore->fMakeAIG, s_pSesStore->nBTLimit, s_pSesStore->fVerbose );
        pSes->fVeryVerbose = s_pSesStore->fVeryVerbose;
        pSes->pSat = s_pSesStore->pSat;
        pSes->nStartGates = nVars - 2;

        while ( pSes->nMaxDepth ) /* there is improvement */
        {
            if ( s_pSesStore->fVeryVerbose )
            {
                printf( " %d", pSes->nMaxDepth );
                fflush( stdout );
            }

            if ( ( pSol2 = Ses_ManFindMinimumSize( pSes ) ) != NULL )
            {
                if ( s_pSesStore->fVeryVerbose )
                {
                    if ( pSes->nMaxDepth >= 10 ) printf( "\b" );
                    printf( "\b" ANSI_COLOR_GREEN "%d" ANSI_COLOR_RESET, pSes->nMaxDepth );
                }
                if ( pSol )
                    ABC_FREE( pSol );
                pSol = pSol2;
                pSes->nMaxDepth--;
            }
            else
            {
                if ( s_pSesStore->fVeryVerbose )
                {
                    if ( pSes->nMaxDepth >= 10 ) printf( "\b" );
                    printf( "\b%s%d" ANSI_COLOR_RESET, pSes->fHitResLimit ? ANSI_COLOR_RED : ANSI_COLOR_YELLOW, pSes->nMaxDepth );
                }
                break;
            }
        }

        if ( s_pSesStore->fVeryVerbose )
            printf( "        \n" );

        /* log unsuccessful case for debugging */
        if ( s_pSesStore->pDebugEntries && pSes->fHitResLimit )
            Ses_StorePrintDebugEntry( s_pSesStore, pTruth, nVars, pNormalArrTime, pSes->nMaxDepth, pSol, nVars - 2 );

        pSes->timeTotal = Abc_Clock() - timeStartExact;

        /* statistics */
        s_pSesStore->nSatCalls += pSes->nSatCalls;
        s_pSesStore->nUnsatCalls += pSes->nUnsatCalls;
        s_pSesStore->nUndefCalls += pSes->nUndefCalls;

        s_pSesStore->timeSat += pSes->timeSat;
        s_pSesStore->timeSatSat += pSes->timeSatSat;
        s_pSesStore->timeSatUnsat += pSes->timeSatUnsat;
        s_pSesStore->timeSatUndef += pSes->timeSatUndef;
        s_pSesStore->timeInstance += pSes->timeInstance;
        s_pSesStore->timeExact += pSes->timeTotal;

        /* cleanup (we need to clean before adding since pTruth may have been modified by pSes) */
        fResLimit = pSes->fHitResLimit;
        Ses_ManCleanLight( pSes );

        /* store solution */
        Ses_StoreAddEntry( s_pSesStore, pTruth, nVars, pNormalArrTime, pSol, fResLimit );
    }

    if ( pSol )
    {
        *Cost = pSol[ABC_EXACT_SOL_NGATES];
        p = pSol + 3 + 4 * pSol[ABC_EXACT_SOL_NGATES] + 1;
        Delay = *p++;
        for ( l = 0; l < nVars; ++l )
            pPerm[l] = *p++;
    }

    if ( pSol )
    {
        int Delay2 = 0;
        for ( l = 0; l < nVars; ++l )
        {
            //printf( "%d ", pPerm[l] );
            Delay2 = Abc_MaxInt( Delay2, pArrTimeProfile[l] + pPerm[l] );
        }
        //printf( "  output arrival = %d    recomputed = %d\n", Delay, Delay2 );
        //if ( Delay != Delay2 )
        //{
        //    printf( "^--- BUG!\n" );
        //    assert( 0 );
        //}

        s_pSesStore->timeTotal += ( Abc_Clock() - timeStart );
        return Delay2;
    }
    else
    {
        assert( *Cost == ABC_INFINITY );

        s_pSesStore->timeTotal += ( Abc_Clock() - timeStart );
        return ABC_INFINITY;
    }
}
// this procedure returns a new node whose output in terms of the given fanins
// has the smallest possible arrival time (in agreement with the above Abc_ExactDelayCost)
Abc_Obj_t * Abc_ExactBuildNode( word * pTruth, int nVars, int * pArrTimeProfile, Abc_Obj_t ** pFanins, Abc_Ntk_t * pNtk )
{
    char * pSol = NULL;
    int i, j, nMaxArrival;
    int pNormalArrTime[8];
    char const * p;
    Abc_Obj_t * pObj;
    Vec_Ptr_t * pGates;
    char pGateTruth[5];
    char * pSopCover;
    abctime timeStart = Abc_Clock();

    if ( nVars == 0 )
    {
        s_pSesStore->timeTotal += ( Abc_Clock() - timeStart );
        return (pTruth[0] & 1) ? Abc_NtkCreateNodeConst1(pNtk) : Abc_NtkCreateNodeConst0(pNtk);
    }
    if ( nVars == 1 )
    {
        s_pSesStore->timeTotal += ( Abc_Clock() - timeStart );
        return (pTruth[0] & 1) ? Abc_NtkCreateNodeInv(pNtk, pFanins[0]) : Abc_NtkCreateNodeBuf(pNtk, pFanins[0]);
    }

    for ( i = 0; i < nVars; ++i )
        pNormalArrTime[i] = pArrTimeProfile[i];
    Abc_NormalizeArrivalTimes( pNormalArrTime, nVars, &nMaxArrival );
    assert( Ses_StoreGetEntry( s_pSesStore, pTruth, nVars, pNormalArrTime, &pSol ) );
    if ( !pSol )
    {
        s_pSesStore->timeTotal += ( Abc_Clock() - timeStart );
        return NULL;
    }

    assert( pSol[ABC_EXACT_SOL_NVARS] == nVars );
    assert( pSol[ABC_EXACT_SOL_NFUNC] == 1 );

    pGates = Vec_PtrAlloc( nVars + pSol[ABC_EXACT_SOL_NGATES] );
    pGateTruth[3] = '0';
    pGateTruth[4] = '\0';

    /* primary inputs */
    for ( i = 0; i < nVars; ++i )
    {
        assert( pFanins[i] );
        Vec_PtrPush( pGates, pFanins[i] );
    }

    /* gates */
    p = pSol + 3;
    for ( i = 0; i < pSol[ABC_EXACT_SOL_NGATES]; ++i )
    {
        pGateTruth[2] = '0' + ( *p & 1 );
        pGateTruth[1] = '0' + ( ( *p >> 1 ) & 1 );
        pGateTruth[0] = '0' + ( ( *p >> 2 ) & 1 );
        ++p;

        assert( *p == 2 ); /* binary gate */
        ++p;

        /* invert truth table if we are last gate and inverted */
        if ( i + 1 == pSol[ABC_EXACT_SOL_NGATES] && Abc_LitIsCompl( *( p + 2 ) ) )
            for ( j = 0; j < 4; ++j )
                pGateTruth[j] = ( pGateTruth[j] == '0' ) ? '1' : '0';

        pSopCover = Abc_SopFromTruthBin( pGateTruth );
        pObj = Abc_NtkCreateNode( pNtk );
        assert( pObj );
        pObj->pData = Abc_SopRegister( (Mem_Flex_t*)pNtk->pManFunc, pSopCover );
        Vec_PtrPush( pGates, pObj );
        ABC_FREE( pSopCover );

        Abc_ObjAddFanin( pObj, (Abc_Obj_t *)Vec_PtrEntry( pGates, *p++ ) );
        Abc_ObjAddFanin( pObj, (Abc_Obj_t *)Vec_PtrEntry( pGates, *p++ ) );
    }

    /* output */
    pObj = (Abc_Obj_t *)Vec_PtrEntry( pGates, nVars + Abc_Lit2Var( *p ) );

    Vec_PtrFree( pGates );

    s_pSesStore->timeTotal += ( Abc_Clock() - timeStart );
    return pObj;
}

void Abc_ExactStoreTest( int fVerbose )
{
    int i;
    word pTruth[4] = {0xcafe, 0, 0, 0};
    int pArrTimeProfile[4] = {6, 2, 8, 5};
    Abc_Ntk_t * pNtk;
    Abc_Obj_t * pFanins[4];
    Vec_Ptr_t * vNames;
    char pPerm[4];
    int Cost;

    pNtk = Abc_NtkAlloc( ABC_NTK_LOGIC, ABC_FUNC_SOP, 1 );
    pNtk->pName = Extra_UtilStrsav( "exact" );
    vNames = Abc_NodeGetFakeNames( 4u );

    /* primary inputs */
    Vec_PtrPush( pNtk->vObjs, NULL );
    for ( i = 0; i < 4; ++i )
    {
        pFanins[i] = Abc_NtkCreatePi( pNtk );
        Abc_ObjAssignName( pFanins[i], (char*)Vec_PtrEntry( vNames, i ), NULL );
    }
    Abc_NodeFreeNames( vNames );

    Abc_ExactStart( 10000, 1, fVerbose, 0, NULL );

    assert( !Abc_ExactBuildNode( pTruth, 4, pArrTimeProfile, pFanins, pNtk ) );

    assert( Abc_ExactDelayCost( pTruth, 4, pArrTimeProfile, pPerm, &Cost, 12 ) == 1 );

    assert( Abc_ExactBuildNode( pTruth, 4, pArrTimeProfile, pFanins, pNtk ) );

    (*pArrTimeProfile)++;
    assert( !Abc_ExactBuildNode( pTruth, 4, pArrTimeProfile, pFanins, pNtk ) );
    (*pArrTimeProfile)--;

    Abc_ExactStop( NULL );

    Abc_NtkDelete( pNtk );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END
