// C-language header for MiniSat 1.14p

#ifndef ABC__sat__psat__m114p_h
#define ABC__sat__psat__m114p_h


#include "m114p_types.h"

ABC_NAMESPACE_HEADER_START


// SAT solver APIs
extern M114p_Solver_t M114p_SolverNew( int fRecordProof );
extern void           M114p_SolverDelete( M114p_Solver_t s );
extern void           M114p_SolverPrintStats( M114p_Solver_t s, double Time );
extern void           M114p_SolverSetVarNum( M114p_Solver_t s, int nVars );
extern int            M114p_SolverAddClause( M114p_Solver_t s, lit* begin, lit* end );
extern int            M114p_SolverSimplify( M114p_Solver_t s );
extern int            M114p_SolverSolve( M114p_Solver_t s, lit* begin, lit* end, int nConfLimit );
extern int            M114p_SolverGetConflictNum( M114p_Solver_t s );

// proof status APIs
extern int            M114p_SolverProofIsReady( M114p_Solver_t s );
extern void           M114p_SolverProofSave( M114p_Solver_t s, char * pFileName );
extern int            M114p_SolverProofClauseNum( M114p_Solver_t s );

// proof traversal APIs
extern int            M114p_SolverGetFirstRoot( M114p_Solver_t s, int ** ppLits );
extern int            M114p_SolverGetNextRoot( M114p_Solver_t s, int ** ppLits );
extern int            M114p_SolverGetFirstChain( M114p_Solver_t s, int ** ppClauses, int ** ppVars );
extern int            M114p_SolverGetNextChain( M114p_Solver_t s, int ** ppClauses, int ** ppVars );

// iterator over the root clauses (should be called first)
#define M114p_SolverForEachRoot( s, ppLits, nLits, i )                           \
    for ( i = 0, nLits = M114p_SolverGetFirstRoot(s, ppLits); nLits;             \
          i++, nLits = M114p_SolverGetNextRoot(s, ppLits) )

// iterator over the learned clauses (should be called after iterating over roots)
#define M114p_SolverForEachChain( s, ppClauses, ppVars, nVars, i )               \
    for ( i = 0, nVars = M114p_SolverGetFirstChain(s, ppClauses, ppVars); nVars; \
          i++, nVars = M114p_SolverGetNextChain(s, ppClauses, ppVars) )



ABC_NAMESPACE_HEADER_END

#endif