/**************************************************************************************************
MiniSat -- Copyright (c) 2005, Niklas Sorensson
http://www.cs.chalmers.se/Cs/Research/FormalMethods/MiniSat/

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/
// Modified to compile with MS Visual Studio 6.0 by Alan Mishchenko

#ifndef satSolver_h
#define satSolver_h

#ifdef _WIN32
#define inline __inline // compatible with MS VS 6.0
#endif

#include "satVec.h"
#include "satMem.h"

//=================================================================================================
// Simple types:

// does not work for c++
//typedef int                   bool;
#ifndef __cplusplus
#ifndef bool
#define bool int
#endif
#endif

static const bool  true      = 1;
static const bool  false     = 0;

typedef int                lit;
typedef char               lbool;

#ifndef SINT64
#define SINT64

#ifdef _WIN32
typedef signed __int64     sint64;   // compatible with MS VS 6.0
#else
typedef long long          sint64;
#endif

#endif

static const int   var_Undef = -1;
static const lit   lit_Undef = -2;

static const lbool l_Undef   =  0;
static const lbool l_True    =  1;
static const lbool l_False   = -1;

static inline lit  toLit    (int v)        { return v + v; }
static inline lit  toLitCond(int v, int c) { return v + v + (c != 0); }
static inline lit  lit_neg  (lit l)        { return l ^ 1; }
static inline int  lit_var  (lit l)        { return l >> 1; }
static inline int  lit_sign (lit l)        { return l & 1; }
static inline int  lit_print(lit l)        { return lit_sign(l)? -lit_var(l)-1 : lit_var(l)+1; }
static inline lit  lit_read (int s)        { return s > 0 ? toLit(s-1) : lit_neg(toLit(-s-1)); }


//=================================================================================================
// Public interface:

struct sat_solver_t;
typedef struct sat_solver_t sat_solver;

extern sat_solver* sat_solver_new(void);
extern void        sat_solver_delete(sat_solver* s);

extern bool        sat_solver_addclause(sat_solver* s, lit* begin, lit* end);
extern bool        sat_solver_simplify(sat_solver* s);
extern int         sat_solver_solve(sat_solver* s, lit* begin, lit* end, sint64 nConfLimit, sint64 nInsLimit, sint64 nConfLimitGlobal, sint64 nInsLimitGlobal);

extern int         sat_solver_nvars(sat_solver* s);
extern int         sat_solver_nclauses(sat_solver* s);
extern int         sat_solver_nconflicts(sat_solver* s);

extern void        sat_solver_setnvars(sat_solver* s,int n);

struct stats_t
{
    sint64   starts, decisions, propagations, inspects, conflicts;
    sint64   clauses, clauses_literals, learnts, learnts_literals, max_literals, tot_literals;
};
typedef struct stats_t stats;

extern void        Sat_SolverWriteDimacs( sat_solver * p, char * pFileName, lit* assumptionsBegin, lit* assumptionsEnd, int incrementVars );
extern void        Sat_SolverPrintStats( FILE * pFile, sat_solver * p );
extern int *       Sat_SolverGetModel( sat_solver * p, int * pVars, int nVars );
extern void        Sat_SolverDoubleClauses( sat_solver * p, int iVar );

// trace recording
extern void        Sat_SolverTraceStart( sat_solver * pSat, char * pName );
extern void        Sat_SolverTraceStop( sat_solver * pSat );
extern void        Sat_SolverTraceWrite( sat_solver * pSat, int * pBeg, int * pEnd, int fRoot );

// clause storage
extern void        sat_solver_store_alloc( sat_solver * s );
extern void        sat_solver_store_write( sat_solver * s, char * pFileName );
extern void        sat_solver_store_free( sat_solver * s );
extern void        sat_solver_store_mark_roots( sat_solver * s );
extern void        sat_solver_store_mark_clauses_a( sat_solver * s );
extern void *      sat_solver_store_release( sat_solver * s ); 

//=================================================================================================
// Solver representation:

struct clause_t;
typedef struct clause_t clause;

struct sat_solver_t
{
    int      size;          // nof variables
    int      cap;           // size of varmaps
    int      qhead;         // Head index of queue.
    int      qtail;         // Tail index of queue.

    // clauses
    vecp     clauses;       // List of problem constraints. (contains: clause*)
    vecp     learnts;       // List of learnt clauses. (contains: clause*)

    // activities
    double   var_inc;       // Amount to bump next variable with.
    double   var_decay;     // INVERSE decay factor for variable activity: stores 1/decay. 
    float    cla_inc;       // Amount to bump next clause with.
    float    cla_decay;     // INVERSE decay factor for clause activity: stores 1/decay.

    vecp*    wlists;        // 
    double*  activity;      // A heuristic measurement of the activity of a variable.
    lbool*   assigns;       // Current values of variables.
    int*     orderpos;      // Index in variable order.
    clause** reasons;       //
    int*     levels;        //
    lit*     trail;

    clause*  binary;        // A temporary binary clause
    lbool*   tags;          //
    veci     tagged;        // (contains: var)
    veci     stack;         // (contains: var)

    veci     order;         // Variable order. (heap) (contains: var)
    veci     trail_lim;     // Separator indices for different decision levels in 'trail'. (contains: int)
    veci     model;         // If problem is solved, this vector contains the model (contains: lbool).

    int      root_level;    // Level of first proper decision.
    int      simpdb_assigns;// Number of top-level assignments at last 'simplifyDB()'.
    int      simpdb_props;  // Number of propagations before next 'simplifyDB()'.
    double   random_seed;
    double   progress_estimate;
    int      verbosity;     // Verbosity level. 0=silent, 1=some progress report, 2=everything

    stats    stats;

    sint64   nConfLimit;    // external limit on the number of conflicts
    sint64   nInsLimit;     // external limit on the number of implications

    veci     act_vars;      // variables whose activity has changed
    double*  factors;       // the activity factors
    int      nRestarts;     // the number of local restarts
 
    Sat_MmStep_t * pMem;

    int      fSkipSimplify; // set to one to skip simplification of the clause database

    // clause store
    void *   pStore;

    // trace recording
    FILE *   pFile;
    int      nClauses;
    int      nRoots;
};

#endif
