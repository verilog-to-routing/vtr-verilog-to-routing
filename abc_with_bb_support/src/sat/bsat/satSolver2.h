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

#ifndef ABC__sat__bsat__satSolver2_h
#define ABC__sat__bsat__satSolver2_h
 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "satVec.h"
#include "satClause.h"
#include "misc/vec/vecSet.h"
#include "satProof2.h"

ABC_NAMESPACE_HEADER_START

//#define USE_FLOAT_ACTIVITY2

//=================================================================================================
// Public interface:

struct sat_solver2_t;
typedef struct sat_solver2_t sat_solver2;
typedef struct Int2_Man_t_ Int2_Man_t;

extern sat_solver2* sat_solver2_new(void);
extern void         sat_solver2_delete(sat_solver2* s);

extern int          sat_solver2_addclause(sat_solver2* s, lit* begin, lit* end, int Id);
extern int          sat_solver2_simplify(sat_solver2* s);
extern int          sat_solver2_solve(sat_solver2* s, lit* begin, lit* end, ABC_INT64_T nConfLimit, ABC_INT64_T nInsLimit, ABC_INT64_T nConfLimitGlobal, ABC_INT64_T nInsLimitGlobal);
extern void         sat_solver2_rollback(sat_solver2* s);
extern void         sat_solver2_reducedb(sat_solver2* s);
extern double       sat_solver2_memory( sat_solver2* s, int fAll );
extern double       sat_solver2_memory_proof( sat_solver2* s );

extern void         sat_solver2_setnvars(sat_solver2* s,int n);

extern void         Sat_Solver2WriteDimacs( sat_solver2 * p, char * pFileName, lit* assumptionsBegin, lit* assumptionsEnd, int incrementVars );
extern void         Sat_Solver2PrintStats( FILE * pFile, sat_solver2 * p );
extern int *        Sat_Solver2GetModel( sat_solver2 * p, int * pVars, int nVars );
extern void         Sat_Solver2DoubleClauses( sat_solver2 * p, int iVar );

// global variables
extern int          var_is_assigned(sat_solver2* s, int v);
extern int          var_is_partA   (sat_solver2* s, int v);
extern void         var_set_partA  (sat_solver2* s, int v, int partA);

// proof-based APIs
extern void *       Sat_ProofCore( sat_solver2 * s );
extern void *       Sat_ProofInterpolant( sat_solver2 * s, void * pGloVars );
extern word *       Sat_ProofInterpolantTruth( sat_solver2 * s, void * pGloVars );
extern void         Sat_ProofCheck( sat_solver2 * s );

// interpolation APIs
extern Int2_Man_t * Int2_ManStart( sat_solver2 * pSat, int * pGloVars, int nGloVars );
extern void         Int2_ManStop( Int2_Man_t * p );
extern int          Int2_ManChainStart( Int2_Man_t * p, clause * c );
extern int          Int2_ManChainResolve( Int2_Man_t * p, clause * c, int iLit, int varA );
extern void *       Int2_ManReadInterpolant( sat_solver2 * s );


//=================================================================================================
// Solver representation:

struct varinfo_t;
typedef struct varinfo2_t varinfo2;

struct sat_solver2_t
{
    int             size;           // nof variables
    int             cap;            // size of varmaps
    int             qhead;          // Head index of queue.
    int             qtail;          // Tail index of queue.

    int             root_level;     // Level of first proper decision.
    double          random_seed;
    double          progress_estimate;
    int             verbosity;      // Verbosity level. 0=silent, 1=some progress report, 2=everything    // activities

#ifdef USE_FLOAT_ACTIVITY2
    double          var_inc;        // Amount to bump next variable with.
    double          var_decay;      // INVERSE decay factor for variable activity: stores 1/decay. 
    float           cla_inc;        // Amount to bump next clause with.
    float           cla_decay;      // INVERSE decay factor for clause activity: stores 1/decay.
    double*         activity;       // A heuristic measurement of the activity of a variable.
#else
    int             var_inc;        // Amount to bump next variable with.
    int             var_inc2;       // Amount to bump next variable with.
    int             cla_inc;        // Amount to bump next clause with.
    unsigned*       activity;       // A heuristic measurement of the activity of a variable
    unsigned*       activity2;      // backup variable activity
#endif

    int             nUnits;         // the total number of unit clauses
    int             nof_learnts;    // the number of clauses to trigger reduceDB
    int             nLearntMax;     // enables using reduce DB method
    int             nLearntStart;   // starting learned clause limit
    int             nLearntDelta;   // delta of learned clause limit
    int             nLearntRatio;   // ratio percentage of learned clauses
    int             nDBreduces;     // number of DB reductions
    int             fNotUseRandom;  // do not allow random decisions with a fixed probability
    int             fSkipSimplify;  // set to one to skip simplification of the clause database
    int             fProofLogging;  // enable proof-logging
    int             fVerbose;

    // clauses
    Sat_Mem_t       Mem;
    veci*           wlists;         // watcher lists (for each literal)
    veci            act_clas;       // clause activities
    veci            claProofs;      // clause proofs

    // rollback
    int             iVarPivot;      // the pivot for variables
    int             iTrailPivot;    // the pivot for trail
    int             hProofPivot;    // the pivot for proof records

    // internal state
    varinfo2 *      vi;             // variable information
    int*            levels;         // 
    char*           assigns;        // 
    lit*            trail;          // sequence of assignment and implications
    int*            orderpos;       // Index in variable order.
    cla*            reasons;        // reason clauses
    cla*            units;          // unit clauses
    int*            model;          // If problem is solved, this vector contains the model (contains: lbool).

    veci            tagged;         // (contains: var)
    veci            stack;          // (contains: var)
    veci            order;          // Variable order. (heap) (contains: var)
    veci            trail_lim;      // Separator indices for different decision levels in 'trail'. (contains: int)
    veci            temp_clause;    // temporary storage for a CNF clause
    veci            conf_final;     // If problem is unsatisfiable (possibly under assumptions),
                                    // this vector represent the final conflict clause expressed in the assumptions.
    veci            mark_levels;    // temporary storage for labeled levels
    veci            min_lit_order;  // ordering of removable literals
    veci            min_step_order; // ordering of resolution steps
    veci            learnt_live;    // remaining clauses after reduce DB

    // proof logging
    Vec_Set_t *     pPrf1;          // sequence of proof records
    veci            temp_proof;     // temporary place to store proofs
    int             hLearntLast;    // in proof-logging mode, the ID of the final conflict clause (conf_final)
    int             hProofLast;     // in proof-logging mode, the ID of the final conflict clause (conf_final)
    Prf_Man_t *     pPrf2;          // another proof manager
    double          dPrfMemory;     // memory used by the proof-logger
    Int2_Man_t *    pInt2;          // interpolation manager
    int             tempInter;      // temporary storage for the interpolant

    // statistics
    stats_t         stats;
    ABC_INT64_T     nConfLimit;     // external limit on the number of conflicts
    ABC_INT64_T     nInsLimit;      // external limit on the number of implications
    abctime         nRuntimeLimit;  // external limit on runtime
};

static inline clause * clause2_read( sat_solver2 * s, cla h )                  { return Sat_MemClauseHand( &s->Mem, h ); }
static inline int      clause2_proofid(sat_solver2* s, clause* c, int varA)    { return c->lrn ? (veci_begin(&s->claProofs)[clause_id(c)]<<2) | (varA<<1) : (clause_id(c)<<2) | (varA<<1) | 1; }

// these two only work after creating a clause before the solver is called
static inline int   clause2_is_partA (sat_solver2* s, int h)                   { return clause2_read(s, h)->partA;       }
static inline void  clause2_set_partA(sat_solver2* s, int h, int partA)        { clause2_read(s, h)->partA = partA;      }
static inline int   clause2_id(sat_solver2* s, int h)                          { return clause_id(clause2_read(s, h));   }
static inline void  clause2_set_id(sat_solver2* s, int h, int id)              { clause_set_id(clause2_read(s, h), id);  }

//=================================================================================================
// Public APIs:

static inline int sat_solver2_nvars(sat_solver2* s)
{
    return s->size;
}

static inline int sat_solver2_nclauses(sat_solver2* s)
{
    return (int)s->stats.clauses;
}

static inline int sat_solver2_nlearnts(sat_solver2* s)
{
    return (int)s->stats.learnts;
}

static inline int sat_solver2_nconflicts(sat_solver2* s)
{
    return (int)s->stats.conflicts;
}

static inline int sat_solver2_var_value( sat_solver2* s, int v ) 
{
    assert( v >= 0 && v < s->size );
    return (int)(s->model[v] == l_True);
}
static inline int sat_solver2_var_literal( sat_solver2* s, int v ) 
{
    assert( v >= 0 && v < s->size );
    return toLitCond( v, s->model[v] != l_True );
}
static inline void sat_solver2_act_var_clear(sat_solver2* s) 
{
    int i;
    for (i = 0; i < s->size; i++)
        s->activity[i] = 0;//.0;
    s->var_inc = 1.0;
}

static inline int sat_solver2_final(sat_solver2* s, int ** ppArray)
{
    *ppArray = s->conf_final.ptr;
    return s->conf_final.size;
}

static inline abctime sat_solver2_set_runtime_limit(sat_solver2* s, abctime Limit)
{
    abctime temp = s->nRuntimeLimit;
    s->nRuntimeLimit = Limit;
    return temp;
}

static inline int sat_solver2_set_learntmax(sat_solver2* s, int nLearntMax)
{
    int temp = s->nLearntMax;
    s->nLearntMax = nLearntMax;
    return temp;
}

static inline void sat_solver2_bookmark(sat_solver2* s)
{
    assert( s->qhead == s->qtail );
    s->iVarPivot    = s->size;
    s->iTrailPivot  = s->qhead;
    if ( s->pPrf1 )
        s->hProofPivot  = Vec_SetHandCurrent(s->pPrf1);
    Sat_MemBookMark( &s->Mem );
    if ( s->activity2 )
    {
        s->var_inc2 = s->var_inc;
        memcpy( s->activity2, s->activity, sizeof(unsigned) * s->iVarPivot );
    }
}

static inline int sat_solver2_add_const( sat_solver2 * pSat, int iVar, int fCompl, int fMark, int Id )
{
    lit Lits[1];
    int Cid;
    assert( iVar >= 0 );

    Lits[0] = toLitCond( iVar, fCompl );
    Cid = sat_solver2_addclause( pSat, Lits, Lits + 1, Id );
    if ( fMark )
        clause2_set_partA( pSat, Cid, 1 );
    return 1;
}
static inline int sat_solver2_add_buffer( sat_solver2 * pSat, int iVarA, int iVarB, int fCompl, int fMark, int Id )
{
    lit Lits[2];
    int Cid;
    assert( iVarA >= 0 && iVarB >= 0 );

    Lits[0] = toLitCond( iVarA, 0 );
    Lits[1] = toLitCond( iVarB, !fCompl );
    Cid = sat_solver2_addclause( pSat, Lits, Lits + 2, Id );
    if ( fMark )
        clause2_set_partA( pSat, Cid, 1 );

    Lits[0] = toLitCond( iVarA, 1 );
    Lits[1] = toLitCond( iVarB, fCompl );
    Cid = sat_solver2_addclause( pSat, Lits, Lits + 2, Id );
    if ( fMark )
        clause2_set_partA( pSat, Cid, 1 );
    return 2;
}
static inline int sat_solver2_add_and( sat_solver2 * pSat, int iVar, int iVar0, int iVar1, int fCompl0, int fCompl1, int fMark, int Id )
{
    lit Lits[3];
    int Cid;

    Lits[0] = toLitCond( iVar, 1 );
    Lits[1] = toLitCond( iVar0, fCompl0 );
    Cid = sat_solver2_addclause( pSat, Lits, Lits + 2, Id );
    if ( fMark )
        clause2_set_partA( pSat, Cid, 1 );

    Lits[0] = toLitCond( iVar, 1 );
    Lits[1] = toLitCond( iVar1, fCompl1 );
    Cid = sat_solver2_addclause( pSat, Lits, Lits + 2, Id );
    if ( fMark )
        clause2_set_partA( pSat, Cid, 1 );

    Lits[0] = toLitCond( iVar, 0 );
    Lits[1] = toLitCond( iVar0, !fCompl0 );
    Lits[2] = toLitCond( iVar1, !fCompl1 );
    Cid = sat_solver2_addclause( pSat, Lits, Lits + 3, Id );
    if ( fMark )
        clause2_set_partA( pSat, Cid, 1 );
    return 3;
}
static inline int sat_solver2_add_xor( sat_solver2 * pSat, int iVarA, int iVarB, int iVarC, int fCompl, int fMark, int Id )
{
    lit Lits[3];
    int Cid;
    assert( iVarA >= 0 && iVarB >= 0 && iVarC >= 0 );

    Lits[0] = toLitCond( iVarA, !fCompl );
    Lits[1] = toLitCond( iVarB, 1 );
    Lits[2] = toLitCond( iVarC, 1 );
    Cid = sat_solver2_addclause( pSat, Lits, Lits + 3, Id );
    if ( fMark )
        clause2_set_partA( pSat, Cid, 1 );

    Lits[0] = toLitCond( iVarA, !fCompl );
    Lits[1] = toLitCond( iVarB, 0 );
    Lits[2] = toLitCond( iVarC, 0 );
    Cid = sat_solver2_addclause( pSat, Lits, Lits + 3, Id );
    if ( fMark )
        clause2_set_partA( pSat, Cid, 1 );

    Lits[0] = toLitCond( iVarA, fCompl );
    Lits[1] = toLitCond( iVarB, 1 );
    Lits[2] = toLitCond( iVarC, 0 );
    Cid = sat_solver2_addclause( pSat, Lits, Lits + 3, Id );
    if ( fMark )
        clause2_set_partA( pSat, Cid, 1 );

    Lits[0] = toLitCond( iVarA, fCompl );
    Lits[1] = toLitCond( iVarB, 0 );
    Lits[2] = toLitCond( iVarC, 1 );
    Cid = sat_solver2_addclause( pSat, Lits, Lits + 3, Id );
    if ( fMark )
        clause2_set_partA( pSat, Cid, 1 );
    return 4;
}
static inline int sat_solver2_add_constraint( sat_solver2 * pSat, int iVar, int iVar2, int fCompl, int fMark, int Id )
{
    lit Lits[2];
    int Cid;
    assert( iVar >= 0 );

    Lits[0] = toLitCond( iVar, fCompl );
    Lits[1] = toLitCond( iVar2, 0 );
    Cid = sat_solver2_addclause( pSat, Lits, Lits + 2, Id );
    if ( fMark )
        clause2_set_partA( pSat, Cid, 1 );

    Lits[0] = toLitCond( iVar, fCompl );
    Lits[1] = toLitCond( iVar2, 1 );
    Cid = sat_solver2_addclause( pSat, Lits, Lits + 2, Id );
    if ( fMark )
        clause2_set_partA( pSat, Cid, 1 );
    return 2;
}


ABC_NAMESPACE_HEADER_END

#endif
