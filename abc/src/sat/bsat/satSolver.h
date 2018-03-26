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

#ifndef ABC__sat__bsat__satSolver_h
#define ABC__sat__bsat__satSolver_h


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "satVec.h"
#include "satClause.h"
#include "misc/util/utilDouble.h"

ABC_NAMESPACE_HEADER_START

//=================================================================================================
// Public interface:

struct sat_solver_t;
typedef struct sat_solver_t sat_solver;

extern sat_solver* sat_solver_new(void);
extern sat_solver* zsat_solver_new_seed(double seed);
extern void        sat_solver_delete(sat_solver* s);

extern int         sat_solver_addclause(sat_solver* s, lit* begin, lit* end);
extern int         sat_solver_clause_new(sat_solver* s, lit* begin, lit* end, int learnt);
extern int         sat_solver_simplify(sat_solver* s);
extern int         sat_solver_solve(sat_solver* s, lit* begin, lit* end, ABC_INT64_T nConfLimit, ABC_INT64_T nInsLimit, ABC_INT64_T nConfLimitGlobal, ABC_INT64_T nInsLimitGlobal);
extern int         sat_solver_solve_internal(sat_solver* s);
extern int         sat_solver_solve_lexsat(sat_solver* s, int * pLits, int nLits);
extern int         sat_solver_minimize_assumptions( sat_solver* s, int * pLits, int nLits, int nConfLimit );
extern int         sat_solver_minimize_assumptions2( sat_solver* s, int * pLits, int nLits, int nConfLimit );
extern int         sat_solver_push(sat_solver* s, int p);
extern void        sat_solver_pop(sat_solver* s);
extern void        sat_solver_set_resource_limits(sat_solver* s, ABC_INT64_T nConfLimit, ABC_INT64_T nInsLimit, ABC_INT64_T nConfLimitGlobal, ABC_INT64_T nInsLimitGlobal);
extern void        sat_solver_restart( sat_solver* s );
extern void        zsat_solver_restart_seed( sat_solver* s, double seed );
extern void        sat_solver_rollback( sat_solver* s );

extern int         sat_solver_nvars(sat_solver* s);
extern int         sat_solver_nclauses(sat_solver* s);
extern int         sat_solver_nconflicts(sat_solver* s);
extern double      sat_solver_memory(sat_solver* s);
extern int         sat_solver_count_assigned(sat_solver* s);

extern int         sat_solver_addvar(sat_solver* s);
extern void        sat_solver_setnvars(sat_solver* s,int n);
extern int         sat_solver_get_var_value(sat_solver* s, int v);
extern void        sat_solver_set_var_activity(sat_solver* s, int * pVars, int nVars);

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

//struct clause_t;
//typedef struct clause_t clause;

struct varinfo_t;
typedef struct varinfo_t varinfo;

struct sat_solver_t
{
    int         size;          // nof variables
    int         cap;           // size of varmaps
    int         qhead;         // Head index of queue.
    int         qtail;         // Tail index of queue.

    // clauses
    Sat_Mem_t   Mem;
    int         hLearnts;      // the first learnt clause
    int         hBinary;       // the special binary clause
    clause *    binary;
    veci*       wlists;        // watcher lists

    // rollback
    int         iVarPivot;     // the pivot for variables
    int         iTrailPivot;   // the pivot for trail
    int         hProofPivot;   // the pivot for proof records

    // activities
    int         VarActType;
    int         ClaActType;
    word        var_inc;       // Amount to bump next variable with.
    word        var_inc2;      // Amount to bump next variable with.
    word        var_decay;     // INVERSE decay factor for variable activity: stores 1/decay. 
    word*       activity;      // A heuristic measurement of the activity of a variable.
    word*       activity2;     // backup variable activity
    unsigned    cla_inc;       // Amount to bump next clause with.
    unsigned    cla_decay;     // INVERSE decay factor for clause activity: stores 1/decay.
    veci        act_clas;      // contain clause activities

    char *      pFreqs;        // how many times this variable was assigned a value
    int         nVarUsed;

//    varinfo *   vi;            // variable information
    int*        levels;        //
    char*       assigns;       // Current values of variables.
    char*       polarity;      //
    char*       tags;          //
    char*       loads;         //

    int*        orderpos;      // Index in variable order.
    int*        reasons;       //
    lit*        trail;
    veci        tagged;        // (contains: var)
    veci        stack;         // (contains: var)

    veci        order;         // Variable order. (heap) (contains: var)
    veci        trail_lim;     // Separator indices for different decision levels in 'trail'. (contains: int)
//    veci        model;         // If problem is solved, this vector contains the model (contains: lbool).
    int *       model;         // If problem is solved, this vector contains the model (contains: lbool).
    veci        conf_final;    // If problem is unsatisfiable (possibly under assumptions),
                               // this vector represent the final conflict clause expressed in the assumptions.

    int         root_level;    // Level of first proper decision.
    int         simpdb_assigns;// Number of top-level assignments at last 'simplifyDB()'.
    int         simpdb_props;  // Number of propagations before next 'simplifyDB()'.
    double      random_seed;
    double      progress_estimate;
    int         verbosity;     // Verbosity level. 0=silent, 1=some progress report, 2=everything
    int         fVerbose;
    int         fPrintClause;

    stats_t     stats;
    int         nLearntMax;    // max number of learned clauses
    int         nLearntStart;  // starting learned clause limit
    int         nLearntDelta;  // delta of learned clause limit
    int         nLearntRatio;  // ratio percentage of learned clauses
    int         nDBreduces;    // number of DB reductions

    ABC_INT64_T nConfLimit;    // external limit on the number of conflicts
    ABC_INT64_T nInsLimit;     // external limit on the number of implications
    abctime     nRuntimeLimit; // external limit on runtime

    veci        act_vars;      // variables whose activity has changed
    double*     factors;       // the activity factors
    int         nRestarts;     // the number of local restarts
    int         nCalls;        // the number of local restarts
    int         nCalls2;       // the number of local restarts
    veci        unit_lits;     // variables whose activity has changed
    veci        pivot_vars;    // pivot variables

    int         fSkipSimplify; // set to one to skip simplification of the clause database
    int         fNotUseRandom; // do not allow random decisions with a fixed probability
    int         fNoRestarts;   // disables periodic restarts

    int *       pGlobalVars;   // for experiments with global vars during interpolation
    // clause store
    void *      pStore;
    int         fSolved;

    // trace recording
    FILE *      pFile;
    int         nClauses;
    int         nRoots;

    veci        temp_clause;    // temporary storage for a CNF clause

    // assignment storage
    veci        user_vars;      // variable IDs
    veci        user_values;    // values of these variables

    // CNF loading
    void *      pCnfMan;           // external CNF manager
    int(*pCnfFunc)(void * p, int); // external callback

    // termination callback
    int         RunId;          // SAT id in this run
    int(*pFuncStop)(int);       // callback to terminate
};

static inline clause * clause_read( sat_solver * s, cla h )          
{ 
    return Sat_MemClauseHand( &s->Mem, h );      
}

static inline int sat_solver_var_value( sat_solver* s, int v )
{
    assert( v >= 0 && v < s->size );
    return (int)(s->model[v] == l_True);
}
static inline int sat_solver_var_literal( sat_solver* s, int v )
{
    assert( v >= 0 && v < s->size );
    return toLitCond( v, s->model[v] != l_True );
}
static inline void sat_solver_flip_print_clause( sat_solver* s )
{
    s->fPrintClause ^= 1;
}
static inline void sat_solver_act_var_clear(sat_solver* s) 
{
    int i;
    if ( s->VarActType == 0 )
    {
        for (i = 0; i < s->size; i++)
            s->activity[i] = (1 << 10);
        s->var_inc = (1 << 5);
    }
    else if ( s->VarActType == 1 )
    {
        for (i = 0; i < s->size; i++)
            s->activity[i] = 0;
        s->var_inc = 1;
    }
    else if ( s->VarActType == 2 )
    {
        for (i = 0; i < s->size; i++)
            s->activity[i] = Xdbl_Const1();
        s->var_inc = Xdbl_Const1();
    }
    else assert(0);
}
static inline void sat_solver_compress(sat_solver* s) 
{
    if ( s->qtail != s->qhead )
    {
        int RetValue = sat_solver_simplify(s);
        assert( RetValue != 0 );
        (void) RetValue;
    }
}
static inline void sat_solver_delete_p( sat_solver ** ps )
{
    if ( *ps )
        sat_solver_delete( *ps );
    *ps = NULL;
}
static inline void sat_solver_clean_polarity(sat_solver* s, int * pVars, int nVars )
{
    int i;
    for ( i = 0; i < nVars; i++ )
        s->polarity[pVars[i]] = 0;
}
static inline void sat_solver_set_polarity(sat_solver* s, int * pVars, int nVars )
{
    int i;
    for ( i = 0; i < s->size; i++ )
        s->polarity[i] = 0;
    for ( i = 0; i < nVars; i++ )
        s->polarity[pVars[i]] = 1;
}
static inline void sat_solver_set_literal_polarity(sat_solver* s, int * pLits, int nLits )
{
    int i;
    for ( i = 0; i < nLits; i++ )
        s->polarity[Abc_Lit2Var(pLits[i])] = !Abc_LitIsCompl(pLits[i]);
}

static inline int sat_solver_final(sat_solver* s, int ** ppArray)
{
    *ppArray = s->conf_final.ptr;
    return s->conf_final.size;
}

static inline abctime sat_solver_set_runtime_limit(sat_solver* s, abctime Limit)
{
    abctime nRuntimeLimit = s->nRuntimeLimit;
    s->nRuntimeLimit = Limit;
    return nRuntimeLimit;
}

static inline int sat_solver_set_random(sat_solver* s, int fNotUseRandom)
{
    int fNotUseRandomOld = s->fNotUseRandom;
    s->fNotUseRandom = fNotUseRandom;
    return fNotUseRandomOld;
}

static inline void sat_solver_bookmark(sat_solver* s)
{
    assert( s->qhead == s->qtail );
    s->iVarPivot    = s->size;
    s->iTrailPivot  = s->qhead;
    Sat_MemBookMark( &s->Mem );
    if ( s->activity2 )
    {
        s->var_inc2 = s->var_inc;
        memcpy( s->activity2, s->activity, sizeof(word) * s->iVarPivot );
    }
}
static inline void sat_solver_set_pivot_variables( sat_solver* s, int * pPivots, int nPivots )
{
    s->pivot_vars.cap = nPivots;
    s->pivot_vars.size = nPivots;
    s->pivot_vars.ptr = pPivots;
}
static inline int sat_solver_count_usedvars(sat_solver* s)
{
    int i, nVars = 0;
    for ( i = 0; i < s->size; i++ )
        if ( s->pFreqs[i] )
        {
            s->pFreqs[i] = 0;
            nVars++;
        }
    return nVars;
}
static inline void sat_solver_set_runid( sat_solver *s, int id )               
{ 
    s->RunId      = id;  
}
static inline void sat_solver_set_stop_func( sat_solver *s, int (*fnct)(int) ) 
{ 
    s->pFuncStop = fnct; 
}

static inline int sat_solver_add_const( sat_solver * pSat, int iVar, int fCompl )
{
    lit Lits[1];
    int Cid;
    assert( iVar >= 0 );

    Lits[0] = toLitCond( iVar, fCompl );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 1 );
    assert( Cid );
    return 1;
}
static inline int sat_solver_add_buffer( sat_solver * pSat, int iVarA, int iVarB, int fCompl )
{
    lit Lits[2];
    int Cid;
    assert( iVarA >= 0 && iVarB >= 0 );

    Lits[0] = toLitCond( iVarA, 0 );
    Lits[1] = toLitCond( iVarB, !fCompl );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 2 );
    if ( Cid == 0 )
        return 0;
    assert( Cid );

    Lits[0] = toLitCond( iVarA, 1 );
    Lits[1] = toLitCond( iVarB, fCompl );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 2 );
    if ( Cid == 0 )
        return 0;
    assert( Cid );
    return 2;
}
static inline int sat_solver_add_buffer_enable( sat_solver * pSat, int iVarA, int iVarB, int iVarEn, int fCompl )
{
    lit Lits[3];
    int Cid;
    assert( iVarA >= 0 && iVarB >= 0 && iVarEn >= 0 );

    Lits[0] = toLitCond( iVarA, 0 );
    Lits[1] = toLitCond( iVarB, !fCompl );
    Lits[2] = toLitCond( iVarEn, 1 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 3 );
    assert( Cid );

    Lits[0] = toLitCond( iVarA, 1 );
    Lits[1] = toLitCond( iVarB, fCompl );
    Lits[2] = toLitCond( iVarEn, 1 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 3 );
    assert( Cid );
    return 2;
}
static inline int sat_solver_add_and( sat_solver * pSat, int iVar, int iVar0, int iVar1, int fCompl0, int fCompl1, int fCompl )
{
    lit Lits[3];
    int Cid;

    Lits[0] = toLitCond( iVar, !fCompl );
    Lits[1] = toLitCond( iVar0, fCompl0 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 2 );
    assert( Cid );

    Lits[0] = toLitCond( iVar, !fCompl );
    Lits[1] = toLitCond( iVar1, fCompl1 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 2 );
    assert( Cid );

    Lits[0] = toLitCond( iVar, fCompl );
    Lits[1] = toLitCond( iVar0, !fCompl0 );
    Lits[2] = toLitCond( iVar1, !fCompl1 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 3 );
    assert( Cid );
    return 3;
}
static inline int sat_solver_add_xor( sat_solver * pSat, int iVarA, int iVarB, int iVarC, int fCompl )
{
    lit Lits[3];
    int Cid;
    assert( iVarA >= 0 && iVarB >= 0 && iVarC >= 0 );

    Lits[0] = toLitCond( iVarA, !fCompl );
    Lits[1] = toLitCond( iVarB, 1 );
    Lits[2] = toLitCond( iVarC, 1 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 3 );
    assert( Cid );

    Lits[0] = toLitCond( iVarA, !fCompl );
    Lits[1] = toLitCond( iVarB, 0 );
    Lits[2] = toLitCond( iVarC, 0 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 3 );
    assert( Cid );

    Lits[0] = toLitCond( iVarA, fCompl );
    Lits[1] = toLitCond( iVarB, 1 );
    Lits[2] = toLitCond( iVarC, 0 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 3 );
    assert( Cid );

    Lits[0] = toLitCond( iVarA, fCompl );
    Lits[1] = toLitCond( iVarB, 0 );
    Lits[2] = toLitCond( iVarC, 1 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 3 );
    assert( Cid );
    return 4;
}
static inline int sat_solver_add_mux( sat_solver * pSat, int iVarZ, int iVarC, int iVarT, int iVarE, int iComplC, int iComplT, int iComplE, int iComplZ )
{
    lit Lits[3];
    int Cid;
    assert( iVarC >= 0 && iVarT >= 0 && iVarE >= 0 && iVarZ >= 0 );

    Lits[0] = toLitCond( iVarC, 1 ^ iComplC );
    Lits[1] = toLitCond( iVarT, 1 ^ iComplT );
    Lits[2] = toLitCond( iVarZ, 0 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 3 );
    assert( Cid );

    Lits[0] = toLitCond( iVarC, 1 ^ iComplC );
    Lits[1] = toLitCond( iVarT, 0 ^ iComplT );
    Lits[2] = toLitCond( iVarZ, 1 ^ iComplZ );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 3 );
    assert( Cid );

    Lits[0] = toLitCond( iVarC, 0 ^ iComplC );
    Lits[1] = toLitCond( iVarE, 1 ^ iComplE );
    Lits[2] = toLitCond( iVarZ, 0 ^ iComplZ );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 3 );
    assert( Cid );

    Lits[0] = toLitCond( iVarC, 0 ^ iComplC );
    Lits[1] = toLitCond( iVarE, 0 ^ iComplE );
    Lits[2] = toLitCond( iVarZ, 1 ^ iComplZ );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 3 );
    assert( Cid );

    if ( iVarT == iVarE )
        return 4;

    Lits[0] = toLitCond( iVarT, 0 ^ iComplT );
    Lits[1] = toLitCond( iVarE, 0 ^ iComplE );
    Lits[2] = toLitCond( iVarZ, 1 ^ iComplZ );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 3 );
    assert( Cid );

    Lits[0] = toLitCond( iVarT, 1 ^ iComplT );
    Lits[1] = toLitCond( iVarE, 1 ^ iComplE );
    Lits[2] = toLitCond( iVarZ, 0 ^ iComplZ );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 3 );
    assert( Cid );
    return 6;
}
static inline int sat_solver_add_mux41( sat_solver * pSat, int iVarZ, int iVarC0, int iVarC1, int iVarD0, int iVarD1, int iVarD2, int iVarD3 )
{
    lit Lits[4];
    int Cid;
    assert( iVarC0 >= 0 && iVarC1 >= 0 && iVarD0 >= 0 && iVarD1 >= 0 && iVarD2 >= 0 && iVarD3 >= 0 && iVarZ >= 0 );

    Lits[0] = toLitCond( iVarD0, 1 );
    Lits[1] = toLitCond( iVarC0, 0 );
    Lits[2] = toLitCond( iVarC1, 0 );
    Lits[3] = toLitCond( iVarZ,  0 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 4 );
    assert( Cid );

    Lits[0] = toLitCond( iVarD1, 1 );
    Lits[1] = toLitCond( iVarC0, 1 );
    Lits[2] = toLitCond( iVarC1, 0 );
    Lits[3] = toLitCond( iVarZ,  0 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 4 );
    assert( Cid );

    Lits[0] = toLitCond( iVarD2, 1 );
    Lits[1] = toLitCond( iVarC0, 0 );
    Lits[2] = toLitCond( iVarC1, 1 );
    Lits[3] = toLitCond( iVarZ,  0 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 4 );
    assert( Cid );

    Lits[0] = toLitCond( iVarD3, 1 );
    Lits[1] = toLitCond( iVarC0, 1 );
    Lits[2] = toLitCond( iVarC1, 1 );
    Lits[3] = toLitCond( iVarZ,  0 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 4 );
    assert( Cid );


    Lits[0] = toLitCond( iVarD0, 0 );
    Lits[1] = toLitCond( iVarC0, 0 );
    Lits[2] = toLitCond( iVarC1, 0 );
    Lits[3] = toLitCond( iVarZ,  1 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 4 );
    assert( Cid );

    Lits[0] = toLitCond( iVarD1, 0 );
    Lits[1] = toLitCond( iVarC0, 1 );
    Lits[2] = toLitCond( iVarC1, 0 );
    Lits[3] = toLitCond( iVarZ,  1 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 4 );
    assert( Cid );

    Lits[0] = toLitCond( iVarD2, 0 );
    Lits[1] = toLitCond( iVarC0, 0 );
    Lits[2] = toLitCond( iVarC1, 1 );
    Lits[3] = toLitCond( iVarZ,  1 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 4 );
    assert( Cid );

    Lits[0] = toLitCond( iVarD3, 0 );
    Lits[1] = toLitCond( iVarC0, 1 );
    Lits[2] = toLitCond( iVarC1, 1 );
    Lits[3] = toLitCond( iVarZ,  1 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 4 );
    assert( Cid );
    return 8;
}
static inline int sat_solver_add_xor_and( sat_solver * pSat, int iVarF, int iVarA, int iVarB, int iVarC )
{
    // F = (a (+) b) * c
    lit Lits[4];
    int Cid;
    assert( iVarF >= 0 && iVarA >= 0 && iVarB >= 0 && iVarC >= 0 );

    Lits[0] = toLitCond( iVarF, 1 );
    Lits[1] = toLitCond( iVarA, 1 );
    Lits[2] = toLitCond( iVarB, 1 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 3 );
    assert( Cid );

    Lits[0] = toLitCond( iVarF, 1 );
    Lits[1] = toLitCond( iVarA, 0 );
    Lits[2] = toLitCond( iVarB, 0 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 3 );
    assert( Cid );

    Lits[0] = toLitCond( iVarF, 1 );
    Lits[1] = toLitCond( iVarC, 0 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 2 );
    assert( Cid );

    Lits[0] = toLitCond( iVarF, 0 );
    Lits[1] = toLitCond( iVarA, 1 );
    Lits[2] = toLitCond( iVarB, 0 );
    Lits[3] = toLitCond( iVarC, 1 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 4 );
    assert( Cid );

    Lits[0] = toLitCond( iVarF, 0 );
    Lits[1] = toLitCond( iVarA, 0 );
    Lits[2] = toLitCond( iVarB, 1 );
    Lits[3] = toLitCond( iVarC, 1 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 4 );
    assert( Cid );
    return 5;
}
static inline int sat_solver_add_constraint( sat_solver * pSat, int iVar, int iVar2, int fCompl )
{
    lit Lits[2];
    int Cid;
    assert( iVar >= 0 );

    Lits[0] = toLitCond( iVar, fCompl );
    Lits[1] = toLitCond( iVar2, 0 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 2 );
    assert( Cid );

    Lits[0] = toLitCond( iVar, fCompl );
    Lits[1] = toLitCond( iVar2, 1 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 2 );
    assert( Cid );
    return 2;
}

static inline int sat_solver_add_half_sorter( sat_solver * pSat, int iVarA, int iVarB, int iVar0, int iVar1 )
{
    lit Lits[3];
    int Cid;

    Lits[0] = toLitCond( iVarA, 0 );
    Lits[1] = toLitCond( iVar0, 1 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 2 );
    assert( Cid );

    Lits[0] = toLitCond( iVarA, 0 );
    Lits[1] = toLitCond( iVar1, 1 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 2 );
    assert( Cid );

    Lits[0] = toLitCond( iVarB, 0 );
    Lits[1] = toLitCond( iVar0, 1 );
    Lits[2] = toLitCond( iVar1, 1 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 3 );
    assert( Cid );
    return 3;
}


ABC_NAMESPACE_HEADER_END

#endif
