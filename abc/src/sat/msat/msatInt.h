/**CFile****************************************************************

  FileName    [msatInt.c]

  PackageName [A C version of SAT solver MINISAT, originally developed 
  in C++ by Niklas Een and Niklas Sorensson, Chalmers University of 
  Technology, Sweden: http://www.cs.chalmers.se/~een/Satzoo.]

  Synopsis    [Internal definitions of the solver.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: msatInt.c,v 1.0 2004/01/01 1:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__sat__msat__msatInt_h
#define ABC__sat__msat__msatInt_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "misc/util/abc_global.h"
#include "msat.h"

ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

// By default, custom memory management is used
// which guarantees constant time allocation/deallocation 
// for SAT clauses and other frequently modified objects.
// For debugging, it is possible use system memory management
// directly. In which case, uncomment the macro below.
//#define USE_SYSTEM_MEMORY_MANAGEMENT 

// internal data structures
typedef struct Msat_Clause_t_      Msat_Clause_t;
typedef struct Msat_Queue_t_       Msat_Queue_t;
typedef struct Msat_Order_t_       Msat_Order_t;
// memory managers (duplicated from Extra for stand-aloneness)
typedef struct Msat_MmFixed_t_     Msat_MmFixed_t;    
typedef struct Msat_MmFlex_t_      Msat_MmFlex_t;     
typedef struct Msat_MmStep_t_      Msat_MmStep_t;     
// variables and literals
typedef int                       Msat_Lit_t;  
typedef int                       Msat_Var_t;  
// the type of return value
#define MSAT_VAR_UNASSIGNED (-1)
#define MSAT_LIT_UNASSIGNED (-2)
#define MSAT_ORDER_UNKNOWN  (-3)

// printing the search tree
#define L_IND      "%-*d"
#define L_ind      Msat_SolverReadDecisionLevel(p)*3+3,Msat_SolverReadDecisionLevel(p)
#define L_LIT      "%s%d"
#define L_lit(Lit) MSAT_LITSIGN(Lit)?"-":"", MSAT_LIT2VAR(Lit)+1

typedef struct Msat_SolverStats_t_ Msat_SolverStats_t;
struct Msat_SolverStats_t_
{
    ABC_INT64_T   nStarts;        // the number of restarts
    ABC_INT64_T   nDecisions;     // the number of decisions
    ABC_INT64_T   nPropagations;  // the number of implications
    ABC_INT64_T   nInspects;      // the number of times clauses are vising while watching them
    ABC_INT64_T   nConflicts;     // the number of conflicts
    ABC_INT64_T   nSuccesses;     // the number of sat assignments found
};

typedef struct Msat_SearchParams_t_ Msat_SearchParams_t;
struct Msat_SearchParams_t_
{
    double  dVarDecay;
    double  dClaDecay;
};

// sat solver data structure visible through all the internal files
struct Msat_Solver_t_
{
    int                 nClauses;    // the total number of clauses
    int                 nClausesStart; // the number of clauses before adding
    Msat_ClauseVec_t *  vClauses;    // problem clauses
    Msat_ClauseVec_t *  vLearned;    // learned clauses
    double              dClaInc;     // Amount to bump next clause with.
    double              dClaDecay;   // INVERSE decay factor for clause activity: stores 1/decay.

    double *            pdActivity;  // A heuristic measurement of the activity of a variable.
    float *             pFactors;    // the multiplicative factors of variable activity
    double              dVarInc;     // Amount to bump next variable with.
    double              dVarDecay;   // INVERSE decay factor for variable activity: stores 1/decay. Use negative value for static variable order.
    Msat_Order_t *      pOrder;      // Keeps track of the decision variable order.

    Msat_ClauseVec_t ** pvWatched;   // 'pvWatched[lit]' is a list of constraints watching 'lit' (will go there if literal becomes true).
    Msat_Queue_t *      pQueue;      // Propagation queue.

    int                 nVars;       // the current number of variables
    int                 nVarsAlloc;  // the maximum allowed number of variables
    int *               pAssigns;    // The current assignments (literals or MSAT_VAR_UNKOWN)
    int *               pModel;      // The satisfying assignment
    Msat_IntVec_t *     vTrail;      // List of assignments made. 
    Msat_IntVec_t *     vTrailLim;   // Separator indices for different decision levels in 'trail'.
    Msat_Clause_t **    pReasons;    // 'reason[var]' is the clause that implied the variables current value, or 'NULL' if none.
    int *               pLevel;      // 'level[var]' is the decision level at which assignment was made. 
    int                 nLevelRoot;  // Level of first proper decision.

    double              dRandSeed;   // For the internal random number generator (makes solver deterministic over different platforms). 

    int                 fVerbose;    // the verbosity flag
    double              dProgress;   // Set by 'search()'.

    // the variable cone and variable connectivity
    Msat_IntVec_t *     vConeVars;
    Msat_IntVec_t *     vVarsUsed;
    Msat_ClauseVec_t *  vAdjacents;

    // internal data used during conflict analysis
    int *               pSeen;       // time when a lit was seen for the last time
    int                 nSeenId;     // the id of current seeing
    Msat_IntVec_t *     vReason;     // the temporary array of literals
    Msat_IntVec_t *     vTemp;       // the temporary array of literals
    int *               pFreq;       // the number of times each var participated in conflicts

    // the memory manager
    Msat_MmStep_t *     pMem;

    // statistics
    Msat_SolverStats_t  Stats;
    int                 nTwoLits;
    int                 nTwoLitsL;
    int                 nClausesInit;
    int                 nClausesAlloc;
    int                 nClausesAllocL;
    int                 nBackTracks;
};

struct Msat_ClauseVec_t_
{
    Msat_Clause_t **    pArray;
    int                 nSize;
    int                 nCap;
};

struct Msat_IntVec_t_
{
    int *               pArray;
    int                 nSize;
    int                 nCap;
};

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DECLARATIONS                        ///
////////////////////////////////////////////////////////////////////////

/*=== satActivity.c ===========================================================*/
extern void                Msat_SolverVarDecayActivity( Msat_Solver_t * p );
extern void                Msat_SolverVarRescaleActivity( Msat_Solver_t * p );
extern void                Msat_SolverClaDecayActivity( Msat_Solver_t * p );
extern void                Msat_SolverClaRescaleActivity( Msat_Solver_t * p );
/*=== satSolverApi.c ===========================================================*/
extern Msat_Clause_t *     Msat_SolverReadClause( Msat_Solver_t * p, int Num );
/*=== satSolver.c ===========================================================*/
extern int                 Msat_SolverReadDecisionLevel( Msat_Solver_t * p );
extern int *               Msat_SolverReadDecisionLevelArray( Msat_Solver_t * p );
extern Msat_Clause_t **    Msat_SolverReadReasonArray( Msat_Solver_t * p );
extern Msat_Type_t         Msat_SolverReadVarValue( Msat_Solver_t * p, Msat_Var_t Var );
extern Msat_ClauseVec_t *  Msat_SolverReadLearned( Msat_Solver_t * p );
extern Msat_ClauseVec_t ** Msat_SolverReadWatchedArray( Msat_Solver_t * p );
extern int *               Msat_SolverReadSeenArray( Msat_Solver_t * p );
extern int                 Msat_SolverIncrementSeenId( Msat_Solver_t * p );
extern Msat_MmStep_t *     Msat_SolverReadMem( Msat_Solver_t * p );
extern void                Msat_SolverClausesIncrement( Msat_Solver_t * p );
extern void                Msat_SolverClausesDecrement( Msat_Solver_t * p );
extern void                Msat_SolverClausesIncrementL( Msat_Solver_t * p );
extern void                Msat_SolverClausesDecrementL( Msat_Solver_t * p );
extern void                Msat_SolverVarBumpActivity( Msat_Solver_t * p, Msat_Lit_t Lit );
extern void                Msat_SolverClaBumpActivity( Msat_Solver_t * p, Msat_Clause_t * pC );
extern int                 Msat_SolverEnqueue( Msat_Solver_t * p, Msat_Lit_t Lit, Msat_Clause_t * pC );
extern double              Msat_SolverProgressEstimate( Msat_Solver_t * p );
/*=== satSolverSearch.c ===========================================================*/
extern int                 Msat_SolverAssume( Msat_Solver_t * p, Msat_Lit_t Lit );
extern Msat_Clause_t *     Msat_SolverPropagate( Msat_Solver_t * p );
extern void                Msat_SolverCancelUntil( Msat_Solver_t * p, int Level );
extern Msat_Type_t         Msat_SolverSearch( Msat_Solver_t * p, int nConfLimit, int nLearnedLimit, int nBackTrackLimit, Msat_SearchParams_t * pPars );
/*=== satQueue.c ===========================================================*/
extern Msat_Queue_t *      Msat_QueueAlloc( int nVars );
extern void                Msat_QueueFree( Msat_Queue_t * p );
extern int                 Msat_QueueReadSize( Msat_Queue_t * p );
extern void                Msat_QueueInsert( Msat_Queue_t * p, int Lit );
extern int                 Msat_QueueExtract( Msat_Queue_t * p );
extern void                Msat_QueueClear( Msat_Queue_t * p );
/*=== satOrder.c ===========================================================*/
extern Msat_Order_t *      Msat_OrderAlloc( Msat_Solver_t * pSat );
extern void                Msat_OrderSetBounds( Msat_Order_t * p, int nVarsMax );
extern void                Msat_OrderClean( Msat_Order_t * p, Msat_IntVec_t * vCone );
extern int                 Msat_OrderCheck( Msat_Order_t * p );
extern void                Msat_OrderFree( Msat_Order_t * p );
extern int                 Msat_OrderVarSelect( Msat_Order_t * p );
extern void                Msat_OrderVarAssigned( Msat_Order_t * p, int Var );
extern void                Msat_OrderVarUnassigned( Msat_Order_t * p, int Var );
extern void                Msat_OrderUpdate( Msat_Order_t * p, int Var );
/*=== satClause.c ===========================================================*/
extern int                 Msat_ClauseCreate( Msat_Solver_t * p, Msat_IntVec_t * vLits, int  fLearnt, Msat_Clause_t ** pClause_out );
extern Msat_Clause_t *     Msat_ClauseCreateFake( Msat_Solver_t * p, Msat_IntVec_t * vLits );
extern Msat_Clause_t *     Msat_ClauseCreateFakeLit( Msat_Solver_t * p, Msat_Lit_t Lit );
extern int                 Msat_ClauseReadLearned( Msat_Clause_t * pC );
extern int                 Msat_ClauseReadSize( Msat_Clause_t * pC );
extern int *               Msat_ClauseReadLits( Msat_Clause_t * pC );
extern int                 Msat_ClauseReadMark( Msat_Clause_t * pC );
extern void                Msat_ClauseSetMark( Msat_Clause_t * pC, int  fMark );
extern int                 Msat_ClauseReadNum( Msat_Clause_t * pC );
extern void                Msat_ClauseSetNum( Msat_Clause_t * pC, int Num );
extern int                 Msat_ClauseReadTypeA( Msat_Clause_t * pC );
extern void                Msat_ClauseSetTypeA( Msat_Clause_t * pC, int  fTypeA );
extern int                 Msat_ClauseIsLocked( Msat_Solver_t * p, Msat_Clause_t * pC );
extern float               Msat_ClauseReadActivity( Msat_Clause_t * pC );
extern void                Msat_ClauseWriteActivity( Msat_Clause_t * pC, float Num );
extern void                Msat_ClauseFree( Msat_Solver_t * p, Msat_Clause_t * pC, int  fRemoveWatched );
extern int                 Msat_ClausePropagate( Msat_Clause_t * pC, Msat_Lit_t Lit, int * pAssigns, Msat_Lit_t * pLit_out );
extern int                 Msat_ClauseSimplify( Msat_Clause_t * pC, int * pAssigns );
extern void                Msat_ClauseCalcReason( Msat_Solver_t * p, Msat_Clause_t * pC, Msat_Lit_t Lit, Msat_IntVec_t * vLits_out );
extern void                Msat_ClauseRemoveWatch( Msat_ClauseVec_t * vClauses, Msat_Clause_t * pC );
extern void                Msat_ClausePrint( Msat_Clause_t * pC );
extern void                Msat_ClausePrintSymbols( Msat_Clause_t * pC );
extern void                Msat_ClauseWriteDimacs( FILE * pFile, Msat_Clause_t * pC, int  fIncrement );
extern unsigned            Msat_ClauseComputeTruth( Msat_Solver_t * p, Msat_Clause_t * pC );
/*=== satSort.c ===========================================================*/
extern void                Msat_SolverSortDB( Msat_Solver_t * p );
/*=== satClauseVec.c ===========================================================*/
extern Msat_ClauseVec_t *  Msat_ClauseVecAlloc( int nCap );
extern void                Msat_ClauseVecFree( Msat_ClauseVec_t * p );
extern Msat_Clause_t **    Msat_ClauseVecReadArray( Msat_ClauseVec_t * p );
extern int                 Msat_ClauseVecReadSize( Msat_ClauseVec_t * p );
extern void                Msat_ClauseVecGrow( Msat_ClauseVec_t * p, int nCapMin );
extern void                Msat_ClauseVecShrink( Msat_ClauseVec_t * p, int nSizeNew );
extern void                Msat_ClauseVecClear( Msat_ClauseVec_t * p );
extern void                Msat_ClauseVecPush( Msat_ClauseVec_t * p, Msat_Clause_t * Entry );
extern Msat_Clause_t *     Msat_ClauseVecPop( Msat_ClauseVec_t * p );
extern void                Msat_ClauseVecWriteEntry( Msat_ClauseVec_t * p, int i, Msat_Clause_t * Entry );
extern Msat_Clause_t *     Msat_ClauseVecReadEntry( Msat_ClauseVec_t * p, int i );

/*=== satMem.c ===========================================================*/
// fixed-size-block memory manager
extern Msat_MmFixed_t *    Msat_MmFixedStart( int nEntrySize );
extern void                Msat_MmFixedStop( Msat_MmFixed_t * p, int fVerbose );
extern char *              Msat_MmFixedEntryFetch( Msat_MmFixed_t * p );
extern void                Msat_MmFixedEntryRecycle( Msat_MmFixed_t * p, char * pEntry );
extern void                Msat_MmFixedRestart( Msat_MmFixed_t * p );
extern int                 Msat_MmFixedReadMemUsage( Msat_MmFixed_t * p );
// flexible-size-block memory manager
extern Msat_MmFlex_t *     Msat_MmFlexStart();
extern void                Msat_MmFlexStop( Msat_MmFlex_t * p, int fVerbose );
extern char *              Msat_MmFlexEntryFetch( Msat_MmFlex_t * p, int nBytes );
extern int                 Msat_MmFlexReadMemUsage( Msat_MmFlex_t * p );
// hierarchical memory manager
extern Msat_MmStep_t *     Msat_MmStepStart( int nSteps );
extern void                Msat_MmStepStop( Msat_MmStep_t * p, int fVerbose );
extern char *              Msat_MmStepEntryFetch( Msat_MmStep_t * p, int nBytes );
extern void                Msat_MmStepEntryRecycle( Msat_MmStep_t * p, char * pEntry, int nBytes );
extern int                 Msat_MmStepReadMemUsage( Msat_MmStep_t * p );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_HEADER_END

#endif
