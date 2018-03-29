/**CFile****************************************************************

  FileName    [xsatSolver.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [xSAT - A SAT solver written in C.
               Read the license file for more info.]

  Synopsis    [Internal definitions of the solver.]

  Author      [Bruno Schmitt <boschmitt@inf.ufrgs.br>]

  Affiliation [UC Berkeley / UFRGS]

  Date        [Ver. 1.0. Started - November 10, 2016.]

  Revision    []

***********************************************************************/
#ifndef ABC__sat__xSAT__xsatSolver_h
#define ABC__sat__xSAT__xsatSolver_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "misc/util/abc_global.h"
#include "misc/vec/vecStr.h"

#include "xsat.h"
#include "xsatBQueue.h"
#include "xsatClause.h"
#include "xsatHeap.h"
#include "xsatMemory.h"
#include "xsatWatchList.h"

ABC_NAMESPACE_HEADER_START

#ifndef __cplusplus
#ifndef false
#  define false 0
#endif
#ifndef true
#  define true 1
#endif
#endif

enum
{
    Var0 = 1,
    Var1 = 0,
    VarX = 3
};

enum
{
    LBoolUndef =  0,
    LBoolTrue  =  1,
    LBoolFalse = -1
};

enum
{
    VarUndef = -1,
    LitUndef = -2
};

#define CRefUndef 0xFFFFFFFF

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
typedef struct xSAT_SolverOptions_t_ xSAT_SolverOptions_t;
struct xSAT_SolverOptions_t_
{
    char fVerbose;

    // Limits
    iword   nConfLimit;    // external limit on the number of conflicts
    iword   nInsLimit;     // external limit on the number of implications
    abctime nRuntimeLimit; // external limit on runtime

    // Constants used for restart heuristic
    double K; // Forces a restart
    double R; // Block a restart
    int nFirstBlockRestart; // Lower bound number of conflicts for start blocking restarts
    int nSizeLBDQueue;      // Size of the moving avarege queue for LBD (force restart)
    int nSizeTrailQueue;    // Size of the moving avarege queue for Trail size (block restart)

    // Constants used for clause database reduction heuristic
    int nConfFirstReduce;  // Number of conflicts before first reduction
    int nIncReduce;        // Increment to reduce
    int nSpecialIncReduce; // Special increment to reduce
    unsigned nLBDFrozenClause;
};

typedef struct xSAT_Stats_t_ xSAT_Stats_t;
struct xSAT_Stats_t_
{
    unsigned nStarts;
    unsigned nReduceDB;

    iword nDecisions;
    iword nPropagations;
    iword nInspects;
    iword nConflicts;

    iword nClauseLits;
    iword nLearntLits;
};

struct xSAT_Solver_t_
{
    /* Clauses Database */
    xSAT_Mem_t * pMemory;
    Vec_Int_t  * vLearnts;
    Vec_Int_t  * vClauses;
    xSAT_VecWatchList_t * vWatches;
    xSAT_VecWatchList_t * vBinWatches;

    /* Activity heuristic */
    int nVarActInc;       /* Amount to bump next variable with. */
    int nClaActInc;       /* Amount to bump next clause with. */

    /* Variable Information */
    Vec_Int_t * vActivity;      /* A heuristic measurement of the activity of a variable. */
    xSAT_Heap_t * hOrder;
    Vec_Int_t * vLevels;        /* Decision level of the current assignment */
    Vec_Int_t * vReasons;       /* Reason (clause) of the current assignment */
    Vec_Str_t * vAssigns;       /* Current assignment. */
    Vec_Str_t * vPolarity;
    Vec_Str_t * vTags;

    /* Assignments */
    Vec_Int_t * vTrail;
    Vec_Int_t * vTrailLim; // Separator indices for different decision levels in 'trail'.
    int iQhead;            // Head of propagation queue (as index into the trail).

    int     nAssignSimplify; /* Number of top-level assignments since last
                              * execution of 'simplify()'. */
    iword   nPropSimplify;   /* Remaining number of propagations that must be
                              * made before next execution of 'simplify()'. */

    /* Temporary data used by Search method */
    xSAT_BQueue_t * bqTrail;
    xSAT_BQueue_t * bqLBD;
    float nSumLBD;
    int  nConfBeforeReduce;
    long nRC1;
    int  nRC2;

    /* Temporary data used by Analyze */
    Vec_Int_t * vLearntClause;
    Vec_Str_t * vSeen;
    Vec_Int_t * vTagged;
    Vec_Int_t * vStack;
    Vec_Int_t * vLastDLevel;

    /* Misc temporary */
    unsigned nStamp;
    Vec_Int_t * vStamp;        /* Multipurpose stamp used to calculate LBD and
                                * clauses minimization with binary resolution */

    xSAT_SolverOptions_t Config;
    xSAT_Stats_t Stats;
};

static inline int xSAT_Var2Lit( int Var, int c )
{
    return Var + Var + ( c != 0 );
}

static inline int  xSAT_NegLit( int Lit )
{
    return Lit ^ 1;
}

static inline int  xSAT_Lit2Var( int Lit )
{
    return Lit >> 1;
}

static inline int  xSAT_LitSign( int Lit )
{
    return Lit & 1;
}

static inline int xSAT_SolverDecisionLevel( xSAT_Solver_t * s )
{
    return Vec_IntSize( s->vTrailLim );
}

static inline xSAT_Clause_t * xSAT_SolverReadClause( xSAT_Solver_t * s, unsigned h )
{
    return xSAT_MemClauseHand( s->pMemory, h );
}

static inline int xSAT_SolverIsClauseSatisfied( xSAT_Solver_t * s, xSAT_Clause_t * pCla )
{
    int i;
    int * Lits = &( pCla->pData[0].Lit );

    for ( i = 0; i < pCla->nSize; i++ )
        if ( Vec_StrEntry( s->vAssigns, xSAT_Lit2Var( Lits[i] ) ) == xSAT_LitSign( ( Lits[i] ) ) )
            return true;

    return false;
}

static inline void xSAT_SolverPrintClauses( xSAT_Solver_t * s )
{
    int i;
    unsigned CRef;

    Vec_IntForEachEntry( s->vClauses, CRef, i )
        xSAT_ClausePrint( xSAT_SolverReadClause( s, CRef ) );
}

static inline void xSAT_SolverPrintState( xSAT_Solver_t * s )
{
    printf( "starts        : %10d\n", s->Stats.nStarts );
    printf( "conflicts     : %10ld\n", s->Stats.nConflicts );
    printf( "decisions     : %10ld\n", s->Stats.nDecisions );
    printf( "propagations  : %10ld\n", s->Stats.nPropagations );
}


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DECLARATIONS                        ///
////////////////////////////////////////////////////////////////////////
extern unsigned xSAT_SolverClaNew( xSAT_Solver_t* s, Vec_Int_t * vLits, int fLearnt );
extern char    xSAT_SolverSearch( xSAT_Solver_t * s );

extern void xSAT_SolverGarbageCollect( xSAT_Solver_t * s );

extern int xSAT_SolverEnqueue( xSAT_Solver_t* s, int Lit, unsigned From );
extern void xSAT_SolverCancelUntil( xSAT_Solver_t* s, int Level);
extern unsigned xSAT_SolverPropagate( xSAT_Solver_t* s );
extern void xSAT_SolverRebuildOrderHeap( xSAT_Solver_t* s );

ABC_NAMESPACE_HEADER_END

#endif
