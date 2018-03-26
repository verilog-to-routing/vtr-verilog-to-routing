/**CFile****************************************************************

  FileName    [msat.h]

  PackageName [A C version of SAT solver MINISAT, originally developed 
  in C++ by Niklas Een and Niklas Sorensson, Chalmers University of 
  Technology, Sweden: http://www.cs.chalmers.se/~een/Satzoo.]

  Synopsis    [External definitions of the solver.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: msat.h,v 1.6 2004/05/12 06:30:20 satrajit Exp $]

***********************************************************************/

#ifndef ABC__sat__msat__msat_h
#define ABC__sat__msat__msat_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct Msat_Solver_t_      Msat_Solver_t;

// the vector of intergers and of clauses
typedef struct Msat_IntVec_t_      Msat_IntVec_t;
typedef struct Msat_ClauseVec_t_   Msat_ClauseVec_t;
typedef struct Msat_VarHeap_t_     Msat_VarHeap_t;

// the return value of the solver
typedef enum { MSAT_FALSE = -1, MSAT_UNKNOWN = 0, MSAT_TRUE = 1 } Msat_Type_t;

// representation of variables and literals
// the literal (l) is the variable (v) and the sign (s)
// s = 0 the variable is positive
// s = 1 the variable is negative
#define MSAT_VAR2LIT(v,s) (2*(v)+(s)) 
#define MSAT_LITNOT(l)    ((l)^1)
#define MSAT_LITSIGN(l)   ((l)&1)     
#define MSAT_LIT2VAR(l)   ((l)>>1)

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DECLARATIONS                        ///
////////////////////////////////////////////////////////////////////////

/*=== satRead.c ============================================================*/
extern int              Msat_SolverParseDimacs( FILE * pFile, Msat_Solver_t ** p, int fVerbose );
/*=== satSolver.c ===========================================================*/
// adding vars, clauses, simplifying the database, and solving
extern int              Msat_SolverAddVar( Msat_Solver_t * p, int Level );
extern int              Msat_SolverAddClause( Msat_Solver_t * p, Msat_IntVec_t * pLits );
extern int              Msat_SolverSimplifyDB( Msat_Solver_t * p );
extern int              Msat_SolverSolve( Msat_Solver_t * p, Msat_IntVec_t * pVecAssumps, int nBackTrackLimit, int nTimeLimit );
// printing stats, assignments, and clauses
extern void             Msat_SolverPrintStats( Msat_Solver_t * p );
extern void             Msat_SolverPrintAssignment( Msat_Solver_t * p );
extern void             Msat_SolverPrintClauses( Msat_Solver_t * p );
extern void             Msat_SolverWriteDimacs( Msat_Solver_t * p, char * pFileName );
// access to the solver internal data
extern int              Msat_SolverReadVarNum( Msat_Solver_t * p );
extern int              Msat_SolverReadClauseNum( Msat_Solver_t * p );
extern int              Msat_SolverReadVarAllocNum( Msat_Solver_t * p );
extern int *            Msat_SolverReadAssignsArray( Msat_Solver_t * p );
extern int *            Msat_SolverReadModelArray( Msat_Solver_t * p );
extern unsigned         Msat_SolverReadTruth( Msat_Solver_t * p );
extern int              Msat_SolverReadBackTracks( Msat_Solver_t * p );
extern int              Msat_SolverReadInspects( Msat_Solver_t * p );
extern void             Msat_SolverSetVerbosity( Msat_Solver_t * p, int fVerbose );
extern void             Msat_SolverSetProofWriting( Msat_Solver_t * p, int fProof );
extern void             Msat_SolverSetVarTypeA( Msat_Solver_t * p, int Var );
extern void             Msat_SolverSetVarMap( Msat_Solver_t * p, Msat_IntVec_t * vVarMap );
extern void             Msat_SolverMarkLastClauseTypeA( Msat_Solver_t * p );
extern void             Msat_SolverMarkClausesStart( Msat_Solver_t * p );
extern float *          Msat_SolverReadFactors( Msat_Solver_t * p );
// returns the solution after incremental solving
extern int              Msat_SolverReadSolutions( Msat_Solver_t * p );
extern int *            Msat_SolverReadSolutionsArray( Msat_Solver_t * p );
extern Msat_ClauseVec_t *  Msat_SolverReadAdjacents( Msat_Solver_t * p );
extern Msat_IntVec_t *  Msat_SolverReadConeVars( Msat_Solver_t * p );
extern Msat_IntVec_t *  Msat_SolverReadVarsUsed( Msat_Solver_t * p );
/*=== satSolverSearch.c ===========================================================*/
extern void             Msat_SolverRemoveLearned( Msat_Solver_t * p );
extern void             Msat_SolverRemoveMarked( Msat_Solver_t * p );
/*=== satSolverApi.c ===========================================================*/
// allocation, cleaning, and freeing the solver
extern Msat_Solver_t *  Msat_SolverAlloc( int nVars, double dClaInc, double dClaDecay, double dVarInc, double dVarDecay, int fVerbose );
extern void             Msat_SolverResize( Msat_Solver_t * pMan, int nVarsAlloc );
extern void             Msat_SolverClean( Msat_Solver_t * p, int nVars );
extern void             Msat_SolverPrepare( Msat_Solver_t * pSat, Msat_IntVec_t * vVars );
extern void             Msat_SolverFree( Msat_Solver_t * p );
/*=== satVec.c ===========================================================*/
extern Msat_IntVec_t *  Msat_IntVecAlloc( int nCap );
extern Msat_IntVec_t *  Msat_IntVecAllocArray( int * pArray, int nSize );
extern Msat_IntVec_t *  Msat_IntVecAllocArrayCopy( int * pArray, int nSize );
extern Msat_IntVec_t *  Msat_IntVecDup( Msat_IntVec_t * pVec );
extern Msat_IntVec_t *  Msat_IntVecDupArray( Msat_IntVec_t * pVec );
extern void             Msat_IntVecFree( Msat_IntVec_t * p );
extern void             Msat_IntVecFill( Msat_IntVec_t * p, int nSize, int Entry );
extern int *            Msat_IntVecReleaseArray( Msat_IntVec_t * p );
extern int *            Msat_IntVecReadArray( Msat_IntVec_t * p );
extern int              Msat_IntVecReadSize( Msat_IntVec_t * p );
extern int              Msat_IntVecReadEntry( Msat_IntVec_t * p, int i );
extern int              Msat_IntVecReadEntryLast( Msat_IntVec_t * p );
extern void             Msat_IntVecWriteEntry( Msat_IntVec_t * p, int i, int Entry );
extern void             Msat_IntVecGrow( Msat_IntVec_t * p, int nCapMin );
extern void             Msat_IntVecShrink( Msat_IntVec_t * p, int nSizeNew );
extern void             Msat_IntVecClear( Msat_IntVec_t * p );
extern void             Msat_IntVecPush( Msat_IntVec_t * p, int Entry );
extern int              Msat_IntVecPushUnique( Msat_IntVec_t * p, int Entry );
extern void             Msat_IntVecPushUniqueOrder( Msat_IntVec_t * p, int Entry, int fIncrease );
extern int              Msat_IntVecPop( Msat_IntVec_t * p );
extern void             Msat_IntVecSort( Msat_IntVec_t * p, int fReverse );
/*=== satHeap.c ===========================================================*/
extern Msat_VarHeap_t * Msat_VarHeapAlloc();
extern void             Msat_VarHeapSetActivity( Msat_VarHeap_t * p, double * pActivity );
extern void             Msat_VarHeapStart( Msat_VarHeap_t * p, int * pVars, int nVars, int nVarsAlloc );
extern void             Msat_VarHeapGrow( Msat_VarHeap_t * p, int nSize );
extern void             Msat_VarHeapStop( Msat_VarHeap_t * p );
extern void             Msat_VarHeapPrint( FILE * pFile, Msat_VarHeap_t * p );
extern void             Msat_VarHeapCheck( Msat_VarHeap_t * p );
extern void             Msat_VarHeapCheckOne( Msat_VarHeap_t * p, int iVar );
extern int              Msat_VarHeapContainsVar( Msat_VarHeap_t * p, int iVar );
extern void             Msat_VarHeapInsert( Msat_VarHeap_t * p, int iVar );  
extern void             Msat_VarHeapUpdate( Msat_VarHeap_t * p, int iVar );  
extern void             Msat_VarHeapDelete( Msat_VarHeap_t * p, int iVar );  
extern double           Msat_VarHeapReadMaxWeight( Msat_VarHeap_t * p );
extern int              Msat_VarHeapCountNodes( Msat_VarHeap_t * p, double WeightLimit );  
extern int              Msat_VarHeapReadMax( Msat_VarHeap_t * p );  
extern int              Msat_VarHeapGetMax( Msat_VarHeap_t * p );  
 


ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
