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

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "satSolver2.h"

ABC_NAMESPACE_IMPL_START

#define SAT_USE_PROOF_LOGGING

//=================================================================================================
// Debug:
 
//#define VERBOSEDEBUG

// For derivation output (verbosity level 2)
#define L_IND    "%-*d"
#define L_ind    solver2_dlevel(s)*2+2,solver2_dlevel(s)
#define L_LIT    "%sx%d"
#define L_lit(p) lit_sign(p)?"~":"", (lit_var(p))
static void printlits(lit* begin, lit* end)
{
    int i;
    for (i = 0; i < end - begin; i++)
        Abc_Print(1,L_LIT" ",L_lit(begin[i]));
}

//=================================================================================================
// Random numbers:


// Returns a random float 0 <= x < 1. Seed must never be 0.
static inline double drand(double* seed) {
    int q;
    *seed *= 1389796;
    q = (int)(*seed / 2147483647);
    *seed -= (double)q * 2147483647;
    return *seed / 2147483647; }


// Returns a random integer 0 <= x < size. Seed must never be 0.
static inline int irand(double* seed, int size) {
    return (int)(drand(seed) * size); }

//=================================================================================================
// Variable datatype + minor functions:

//static const int var0  = 1;
static const int var1  = 0;
static const int varX  = 3;

struct varinfo2_t
{
//    unsigned val    :  2;  // variable value 
    unsigned pol    :  1;  // last polarity
    unsigned partA  :  1;  // partA variable
    unsigned tag    :  4;  // conflict analysis tags
//    unsigned lev    : 24;  // variable level
};

int    var_is_assigned(sat_solver2* s, int v)            { return s->assigns[v] != varX; }
int    var_is_partA (sat_solver2* s, int v)              { return s->vi[v].partA;        }
void   var_set_partA(sat_solver2* s, int v, int partA)   { s->vi[v].partA = partA;       }

//static inline int     var_level     (sat_solver2* s, int v)            { return s->vi[v].lev; }
static inline int     var_level     (sat_solver2* s, int v)            { return s->levels[v];  }
//static inline int     var_value     (sat_solver2* s, int v)            { return s->vi[v].val; }
static inline int     var_value     (sat_solver2* s, int v)            { return s->assigns[v]; }
static inline int     var_polar     (sat_solver2* s, int v)            { return s->vi[v].pol; }

//static inline void    var_set_level (sat_solver2* s, int v, int lev)   { s->vi[v].lev = lev;  }
static inline void    var_set_level (sat_solver2* s, int v, int lev)   { s->levels[v] = lev;  }
//static inline void    var_set_value (sat_solver2* s, int v, int val)   { s->vi[v].val = val;  }
static inline void    var_set_value (sat_solver2* s, int v, int val)   { s->assigns[v] = val; }
static inline void    var_set_polar (sat_solver2* s, int v, int pol)   { s->vi[v].pol = pol;  }

// check if the literal is false under current assumptions
static inline int     solver2_lit_is_false( sat_solver2* s, int Lit )  { return var_value(s, lit_var(Lit)) == !lit_sign(Lit); }



// variable tags
static inline int     var_tag       (sat_solver2* s, int v)            { return s->vi[v].tag; }
static inline void    var_set_tag   (sat_solver2* s, int v, int tag)   {
    assert( tag > 0 && tag < 16 );
    if ( s->vi[v].tag == 0 )
        veci_push( &s->tagged, v );
    s->vi[v].tag = tag;
}
static inline void    var_add_tag   (sat_solver2* s, int v, int tag)   {
    assert( tag > 0 && tag < 16 );
    if ( s->vi[v].tag == 0 )
        veci_push( &s->tagged, v );
    s->vi[v].tag |= tag;
}
static inline void    solver2_clear_tags(sat_solver2* s, int start)    {
    int i, * tagged = veci_begin(&s->tagged);
    for (i = start; i < veci_size(&s->tagged); i++)
        s->vi[tagged[i]].tag = 0;
    veci_resize(&s->tagged,start);
}

// level marks
static inline int     var_lev_mark (sat_solver2* s, int v)             {
    return (veci_begin(&s->trail_lim)[var_level(s,v)] & 0x80000000) > 0;
}
static inline void    var_lev_set_mark (sat_solver2* s, int v)         {
    int level = var_level(s,v);
    assert( level < veci_size(&s->trail_lim) );
    veci_begin(&s->trail_lim)[level] |= 0x80000000;
    veci_push(&s->mark_levels, level);
}
static inline void    solver2_clear_marks(sat_solver2* s)              {
    int i, * mark_levels = veci_begin(&s->mark_levels);
    for (i = 0; i < veci_size(&s->mark_levels); i++)
        veci_begin(&s->trail_lim)[mark_levels[i]] &= 0x7FFFFFFF;
    veci_resize(&s->mark_levels,0);
}

//=================================================================================================
// Clause datatype + minor functions:

//static inline int     var_reason    (sat_solver2* s, int v)      { return (s->reasons[v]&1) ? 0 : s->reasons[v] >> 1;                   }
//static inline int     lit_reason    (sat_solver2* s, int l)      { return (s->reasons[lit_var(l)&1]) ? 0 : s->reasons[lit_var(l)] >> 1; }
//static inline clause* var_unit_clause(sat_solver2* s, int v)           { return (s->reasons[v]&1) ? clause2_read(s, s->reasons[v] >> 1) : NULL;  }
//static inline void    var_set_unit_clause(sat_solver2* s, int v, cla i){ assert(i && !s->reasons[v]); s->reasons[v] = (i << 1) | 1;             }
static inline int     var_reason    (sat_solver2* s, int v)            { return s->reasons[v];           }
static inline int     lit_reason    (sat_solver2* s, int l)            { return s->reasons[lit_var(l)];  }
static inline clause* var_unit_clause(sat_solver2* s, int v)           { return clause2_read(s, s->units[v]);                                          }
static inline void    var_set_unit_clause(sat_solver2* s, int v, cla i){ 
    assert(v >= 0 && v < s->size && !s->units[v]); s->units[v] = i; s->nUnits++; 
}

#define clause_foreach_var( p, var, i, start )        \
    for ( i = start; (i < (int)(p)->size) && ((var) = lit_var((p)->lits[i])); i++ )

//=================================================================================================
// Simple helpers:

static inline int     solver2_dlevel(sat_solver2* s)       { return veci_size(&s->trail_lim); }
static inline veci*   solver2_wlist(sat_solver2* s, lit l) { return &s->wlists[l];            }

//=================================================================================================
// Proof logging:

static inline void proof_chain_start( sat_solver2* s, clause* c )
{
    if ( !s->fProofLogging )
        return;
    if ( s->pInt2 )
        s->tempInter = Int2_ManChainStart( s->pInt2, c );
    if ( s->pPrf2 )
        Prf_ManChainStart( s->pPrf2, c );
    if ( s->pPrf1 )
    {
        int ProofId = clause2_proofid(s, c, 0);
        assert( (ProofId >> 2) > 0 );
        veci_resize( &s->temp_proof, 0 );
        veci_push( &s->temp_proof, 0 );
        veci_push( &s->temp_proof, 0 );
        veci_push( &s->temp_proof, ProofId );
    }
}

static inline void proof_chain_resolve( sat_solver2* s, clause* cls, int Var )
{
    if ( !s->fProofLogging )
        return;
    if ( s->pInt2 )
    {
        clause* c = cls ? cls : var_unit_clause( s, Var );
        s->tempInter = Int2_ManChainResolve( s->pInt2, c, s->tempInter, var_is_partA(s,Var) );
    }
    if ( s->pPrf2 )
    {
        clause* c = cls ? cls : var_unit_clause( s, Var );
        Prf_ManChainResolve( s->pPrf2, c );
    }
    if ( s->pPrf1 )
    {
        clause* c = cls ? cls : var_unit_clause( s, Var );
        int ProofId = clause2_proofid(s, c, var_is_partA(s,Var));
        assert( (ProofId >> 2) > 0 );
        veci_push( &s->temp_proof, ProofId );
    }
}

static inline int proof_chain_stop( sat_solver2* s )
{
    if ( !s->fProofLogging )
        return 0;
    if ( s->pInt2 )
    {
        int h = s->tempInter; 
        s->tempInter = -1; 
        return h;
    }
    if ( s->pPrf2 )
        Prf_ManChainStop( s->pPrf2 );
    if ( s->pPrf1 )
    {
        extern void Proof_ClauseSetEnts( Vec_Set_t* p, int h, int nEnts );
        int h = Vec_SetAppend( s->pPrf1, veci_begin(&s->temp_proof), veci_size(&s->temp_proof) );
        Proof_ClauseSetEnts( s->pPrf1, h, veci_size(&s->temp_proof) - 2 );
        return h;
    }
    return 0;
}

//=================================================================================================
// Variable order functions:

static inline void order_update(sat_solver2* s, int v) // updateorder
{
    int*      orderpos = s->orderpos;
    int*      heap     = veci_begin(&s->order);
    int       i        = orderpos[v];
    int       x        = heap[i];
    int       parent   = (i - 1) / 2;
    assert(s->orderpos[v] != -1);
    while (i != 0 && s->activity[x] > s->activity[heap[parent]]){
        heap[i]           = heap[parent];
        orderpos[heap[i]] = i;
        i                 = parent;
        parent            = (i - 1) / 2;
    }
    heap[i]     = x;
    orderpos[x] = i;
}
static inline void order_assigned(sat_solver2* s, int v)
{
}
static inline void order_unassigned(sat_solver2* s, int v) // undoorder
{
    int* orderpos = s->orderpos;
    if (orderpos[v] == -1){
        orderpos[v] = veci_size(&s->order);
        veci_push(&s->order,v);
        order_update(s,v);
    }
}
static inline int  order_select(sat_solver2* s, float random_var_freq) // selectvar
{
    int*      heap     = veci_begin(&s->order);
    int*      orderpos = s->orderpos;
    // Random decision:
    if (drand(&s->random_seed) < random_var_freq){
        int next = irand(&s->random_seed,s->size);
        assert(next >= 0 && next < s->size);
        if (var_value(s, next) == varX)
            return next;
    }
    // Activity based decision:
    while (veci_size(&s->order) > 0){
        int    next  = heap[0];
        int    size  = veci_size(&s->order)-1;
        int    x     = heap[size];
        veci_resize(&s->order,size);
        orderpos[next] = -1;
        if (size > 0){
            unsigned act   = s->activity[x];
            int      i     = 0;
            int      child = 1;
            while (child < size){
                if (child+1 < size && s->activity[heap[child]] < s->activity[heap[child+1]])
                    child++;
                assert(child < size);
                if (act >= s->activity[heap[child]])
                    break;
                heap[i]           = heap[child];
                orderpos[heap[i]] = i;
                i                 = child;
                child             = 2 * child + 1;
            }
            heap[i]           = x;
            orderpos[heap[i]] = i;
        }
        if (var_value(s, next) == varX)
            return next;
    }
    return var_Undef;
}


//=================================================================================================
// Activity functions:

#ifdef USE_FLOAT_ACTIVITY2

static inline void act_var_rescale(sat_solver2* s)  {
    double* activity = s->activity;
    int i;
    for (i = 0; i < s->size; i++)
        activity[i] *= 1e-100;
    s->var_inc *= 1e-100;
}
static inline void act_clause2_rescale(sat_solver2* s) {
    static abctime Total = 0;
    float * act_clas = (float *)veci_begin(&s->act_clas);
    int i;
    abctime clk = Abc_Clock();
    for (i = 0; i < veci_size(&s->act_clas); i++)
        act_clas[i] *= (float)1e-20;
    s->cla_inc *= (float)1e-20;

    Total += Abc_Clock() - clk;
    Abc_Print(1, "Rescaling...   Cla inc = %10.3f  Conf = %10d   ", s->cla_inc,  s->stats.conflicts );
    Abc_PrintTime( 1, "Time", Total );
}
static inline void act_var_bump(sat_solver2* s, int v) {
    s->activity[v] += s->var_inc;
    if (s->activity[v] > 1e100)
        act_var_rescale(s);
    if (s->orderpos[v] != -1)
        order_update(s,v);
}
static inline void act_clause2_bump(sat_solver2* s, clause*c) {
    float * act_clas = (float *)veci_begin(&s->act_clas);
    assert( c->Id < veci_size(&s->act_clas) );
    act_clas[c->Id] += s->cla_inc;
    if (act_clas[c->Id] > (float)1e20)
        act_clause2_rescale(s);
}
static inline void act_var_decay(sat_solver2* s)    { s->var_inc *= s->var_decay; }
static inline void act_clause2_decay(sat_solver2* s) { s->cla_inc *= s->cla_decay; }

#else

static inline void act_var_rescale(sat_solver2* s) {
    unsigned* activity = s->activity;
    int i;
    for (i = 0; i < s->size; i++)
        activity[i] >>= 19;
    s->var_inc >>= 19;
    s->var_inc = Abc_MaxInt( s->var_inc, (1<<4) );
}
static inline void act_clause2_rescale(sat_solver2* s) {
//    static abctime Total = 0;
//    abctime clk = Abc_Clock();
    int i;
    unsigned * act_clas = (unsigned *)veci_begin(&s->act_clas);
    for (i = 0; i < veci_size(&s->act_clas); i++)
        act_clas[i] >>= 14;
    s->cla_inc >>= 14;
    s->cla_inc = Abc_MaxInt( s->cla_inc, (1<<10) );
//    Total += Abc_Clock() - clk;
//    Abc_Print(1, "Rescaling...   Cla inc = %5d  Conf = %10d   ", s->cla_inc,  s->stats.conflicts );
//    Abc_PrintTime( 1, "Time", Total );
}
static inline void act_var_bump(sat_solver2* s, int v) {
    s->activity[v] += s->var_inc;
    if (s->activity[v] & 0x80000000)
        act_var_rescale(s);
    if (s->orderpos[v] != -1)
        order_update(s,v);
}
static inline void act_clause2_bump(sat_solver2* s, clause*c) {
    unsigned * act_clas = (unsigned *)veci_begin(&s->act_clas);
    int Id = clause_id(c);
    assert( Id >= 0 && Id < veci_size(&s->act_clas) );
    act_clas[Id] += s->cla_inc;
    if (act_clas[Id] & 0x80000000)
        act_clause2_rescale(s);
}
static inline void act_var_decay(sat_solver2* s)    { s->var_inc += (s->var_inc >>  4); }
static inline void act_clause2_decay(sat_solver2* s) { s->cla_inc += (s->cla_inc >> 10); }

#endif

//=================================================================================================
// Clause functions:

static inline int sat_clause_compute_lbd( sat_solver2* s, clause* c )
{
    int i, lev, minl = 0, lbd = 0;
    for (i = 0; i < (int)c->size; i++) {
        lev = var_level(s, lit_var(c->lits[i]));
        if ( !(minl & (1 << (lev & 31))) ) {
            minl |= 1 << (lev & 31);
            lbd++;
        }
    }
    return lbd;
}

static int clause2_create_new(sat_solver2* s, lit* begin, lit* end, int learnt, int proof_id )
{
    clause* c;
    int h, size = end - begin;
    assert(size < 1 || begin[0] >= 0);
    assert(size < 2 || begin[1] >= 0);
    assert(size < 1 || lit_var(begin[0]) < s->size);
    assert(size < 2 || lit_var(begin[1]) < s->size);
    // create new clause
    h = Sat_MemAppend( &s->Mem, begin, size, learnt, 1 );
    assert( !(h & 1) );
    c = clause2_read( s, h );
    if (learnt)
    {
        if ( s->pPrf1 )
            assert( proof_id );
        c->lbd = sat_clause_compute_lbd( s, c );
        assert( clause_id(c) == veci_size(&s->act_clas) );
        if ( s->pPrf1 || s->pInt2 )
            veci_push(&s->claProofs, proof_id);
//        veci_push(&s->act_clas, (1<<10));
        veci_push(&s->act_clas, 0);
        if ( size > 2 )
            act_clause2_bump( s,c );
        s->stats.learnts++;
        s->stats.learnts_literals += size;
        // remember the last one
        s->hLearntLast = h;
    }
    else
    {
        assert( clause_id(c) == (int)s->stats.clauses );
        s->stats.clauses++;
        s->stats.clauses_literals += size;
    }
    // watch the clause
    if ( size > 1 )
    {
        veci_push(solver2_wlist(s,lit_neg(begin[0])),h);
        veci_push(solver2_wlist(s,lit_neg(begin[1])),h);
    }
    return h;
}

//=================================================================================================
// Minor (solver) functions:

static inline int solver2_enqueue(sat_solver2* s, lit l, cla from)
{
    int v = lit_var(l);
#ifdef VERBOSEDEBUG
    Abc_Print(1,L_IND"enqueue("L_LIT")\n", L_ind, L_lit(l));
#endif
    if (var_value(s, v) != varX)
        return var_value(s, v) == lit_sign(l);
    else
    {  // New fact -- store it.
#ifdef VERBOSEDEBUG
        Abc_Print(1,L_IND"bind("L_LIT")\n", L_ind, L_lit(l));
#endif
        var_set_value( s, v, lit_sign(l) );
        var_set_level( s, v, solver2_dlevel(s) );
        s->reasons[v] = from;  //  = from << 1;
        s->trail[s->qtail++] = l;
        order_assigned(s, v);
        return true;
    }
}

static inline int solver2_assume(sat_solver2* s, lit l)
{
    assert(s->qtail == s->qhead);
    assert(var_value(s, lit_var(l)) == varX);
#ifdef VERBOSEDEBUG
    Abc_Print(1,L_IND"assume("L_LIT")  ", L_ind, L_lit(l));
    Abc_Print(1, "act = %.20f\n", s->activity[lit_var(l)] );
#endif
    veci_push(&s->trail_lim,s->qtail);
    return solver2_enqueue(s,l,0);
}

static void solver2_canceluntil(sat_solver2* s, int level) {
    int      bound;
    int      lastLev;
    int      c, x;

    if (solver2_dlevel(s) <= level)
        return;
    assert( solver2_dlevel(s) > 0 );

    bound   = (veci_begin(&s->trail_lim))[level];
    lastLev = (veci_begin(&s->trail_lim))[veci_size(&s->trail_lim)-1];

    for (c = s->qtail-1; c >= bound; c--)
    {
        x = lit_var(s->trail[c]);
        var_set_value(s, x, varX);
        s->reasons[x] = 0;
        s->units[x] = 0; // temporary?
        if ( c < lastLev )
            var_set_polar(s, x, !lit_sign(s->trail[c]));
    }

    for (c = s->qhead-1; c >= bound; c--)
        order_unassigned(s,lit_var(s->trail[c]));

    s->qhead = s->qtail = bound;
    veci_resize(&s->trail_lim,level);
}

static void solver2_canceluntil_rollback(sat_solver2* s, int NewBound) {
    int      c, x;
   
    assert( solver2_dlevel(s) == 0 );
    assert( s->qtail == s->qhead );
    assert( s->qtail >= NewBound );

    for (c = s->qtail-1; c >= NewBound; c--) 
    {
        x = lit_var(s->trail[c]);
        var_set_value(s, x, varX);
        s->reasons[x] = 0;
        s->units[x] = 0; // temporary?
    }

    for (c = s->qhead-1; c >= NewBound; c--)
        order_unassigned(s,lit_var(s->trail[c]));

    s->qhead = s->qtail = NewBound;
}


static void solver2_record(sat_solver2* s, veci* cls, int proof_id)
{
    lit* begin = veci_begin(cls);
    lit* end   = begin + veci_size(cls);
    cla  Cid   = clause2_create_new(s,begin,end,1, proof_id);
    assert(veci_size(cls) > 0);
    if ( veci_size(cls) == 1 )
    {
        if ( s->fProofLogging )
            var_set_unit_clause(s, lit_var(begin[0]), Cid);
        Cid = 0;
    }
    solver2_enqueue(s, begin[0], Cid);
}


static double solver2_progress(sat_solver2* s)
{
    int i;
    double progress = 0.0, F = 1.0 / s->size;
    for (i = 0; i < s->size; i++)
        if (var_value(s, i) != varX)
            progress += pow(F, var_level(s, i));
    return progress / s->size;
}

//=================================================================================================
// Major methods:

/*_________________________________________________________________________________________________
|
|  analyzeFinal : (p : Lit)  ->  [void]
|  
|  Description:
|    Specialized analysis procedure to express the final conflict in terms of assumptions.
|    Calculates the (possibly empty) set of assumptions that led to the assignment of 'p', and
|    stores the result in 'out_conflict'.
|________________________________________________________________________________________________@*/
/*
void Solver::analyzeFinal(clause* confl, bool skip_first)
{
    // -- NOTE! This code is relatively untested. Please report bugs!
    conflict.clear();
    if (root_level == 0) return;

    vec<char>& seen  = analyze_seen;
    for (int i = skip_first ? 1 : 0; i < confl->size(); i++){
        Var x = var((*confl)[i]);
        if (level[x] > 0)
            seen[x] = 1;
    }

    int start = (root_level >= trail_lim.size()) ? trail.size()-1 : trail_lim[root_level];
    for (int i = start; i >= trail_lim[0]; i--){
        Var     x = var(trail[i]);
        if (seen[x]){
            GClause r = reason[x];
            if (r == Gclause2_NULL){
                assert(level[x] > 0);
                conflict.push(~trail[i]);
            }else{
                if (r.isLit()){
                    Lit p = r.lit();
                    if (level[var(p)] > 0)
                        seen[var(p)] = 1;
                }else{
                    Clause& c = *r.clause();
                    for (int j = 1; j < c.size(); j++)
                        if (level[var(c[j])] > 0)
                            seen[var(c[j])] = 1;
                }
            }
            seen[x] = 0;
        }
    }
}
*/

static int solver2_analyze_final(sat_solver2* s, clause* conf, int skip_first)
{
    int i, j, x;//, start;
    veci_resize(&s->conf_final,0);
    if ( s->root_level == 0 )
        return s->hProofLast;
    proof_chain_start( s, conf );
    assert( veci_size(&s->tagged) == 0 );
    clause_foreach_var( conf, x, i, skip_first ){
        if ( var_level(s,x) )
            var_set_tag(s, x, 1);
        else
            proof_chain_resolve( s, NULL, x );
    }
    assert( s->root_level >= veci_size(&s->trail_lim) );
//    start = (s->root_level >= veci_size(&s->trail_lim))? s->qtail-1 : (veci_begin(&s->trail_lim))[s->root_level];
    for (i = s->qtail-1; i >= (veci_begin(&s->trail_lim))[0]; i--){
        x = lit_var(s->trail[i]);
        if (var_tag(s,x)){
            clause* c = clause2_read(s, var_reason(s,x));
            if (c){
                proof_chain_resolve( s, c, x );
                clause_foreach_var( c, x, j, 1 ){
                    if ( var_level(s,x) )
                        var_set_tag(s, x, 1);
                    else
                        proof_chain_resolve( s, NULL, x );
                }
            }else {
                assert( var_level(s,x) );
                veci_push(&s->conf_final,lit_neg(s->trail[i]));
            }
        }
    }
    solver2_clear_tags(s,0);
    return proof_chain_stop( s );
}

static int solver2_lit_removable_rec(sat_solver2* s, int v)
{
    // tag[0]: True if original conflict clause literal.
    // tag[1]: Processed by this procedure
    // tag[2]: 0=non-removable, 1=removable

    clause* c;
    int i, x;

    // skip visited
    if (var_tag(s,v) & 2)
        return (var_tag(s,v) & 4) > 0;

    // skip decisions on a wrong level
    c = clause2_read(s, var_reason(s,v));
    if ( c == NULL ){
        var_add_tag(s,v,2);
        return 0;
    }

    clause_foreach_var( c, x, i, 1 ){
        if (var_tag(s,x) & 1)
            solver2_lit_removable_rec(s, x);
        else{
            if (var_level(s,x) == 0 || var_tag(s,x) == 6) continue;     // -- 'x' checked before, found to be removable (or belongs to the toplevel)
            if (var_tag(s,x) == 2 || !var_lev_mark(s,x) || !solver2_lit_removable_rec(s, x))
            {  // -- 'x' checked before, found NOT to be removable, or it belongs to a wrong level, or cannot be removed
                var_add_tag(s,v,2);
                return 0;
            }
        }
    }
    if ( s->fProofLogging && (var_tag(s,v) & 1) )
        veci_push(&s->min_lit_order, v );
    var_add_tag(s,v,6);
    return 1;
}

static int solver2_lit_removable(sat_solver2* s, int x)
{
    clause* c;
    int i, top;
    if ( !var_reason(s,x) )
        return 0;
    if ( var_tag(s,x) & 2 )
    {
        assert( var_tag(s,x) & 1 );
        return 1;
    }
    top = veci_size(&s->tagged);
    veci_resize(&s->stack,0);
    veci_push(&s->stack, x << 1);
    while (veci_size(&s->stack))
    {
        x = veci_pop(&s->stack);
        if ( s->fProofLogging )
        {
            if ( x & 1 ){
                if ( var_tag(s,x >> 1) & 1 )
                    veci_push(&s->min_lit_order, x >> 1 );
                continue;
            }
            veci_push(&s->stack, x ^ 1 );
        }
        x >>= 1;
        c = clause2_read(s, var_reason(s,x));
        clause_foreach_var( c, x, i, 1 ){
            if (var_tag(s,x) || !var_level(s,x))
                continue;
            if (!var_reason(s,x) || !var_lev_mark(s,x)){
                solver2_clear_tags(s, top);
                return 0;
            }
            veci_push(&s->stack, x << 1);
            var_set_tag(s, x, 2);
        }
    }
    return 1;
}

static void solver2_logging_order(sat_solver2* s, int x)
{
    clause* c;
    int i;
    if ( (var_tag(s,x) & 4) )
        return;
    var_add_tag(s, x, 4);
    veci_resize(&s->stack,0);
    veci_push(&s->stack,x << 1);
    while (veci_size(&s->stack))
    {
        x = veci_pop(&s->stack);
        if ( x & 1 ){
            veci_push(&s->min_step_order, x >> 1 );
            continue;
        }
        veci_push(&s->stack, x ^ 1 );
        x >>= 1;
        c = clause2_read(s, var_reason(s,x));
//        if ( !c )
//            Abc_Print(1, "solver2_logging_order(): Error in conflict analysis!!!\n" );
        clause_foreach_var( c, x, i, 1 ){
            if ( !var_level(s,x) || (var_tag(s,x) & 1) )
                continue;
            veci_push(&s->stack, x << 1);
            var_add_tag(s, x, 4);
        }
    }
}

static void solver2_logging_order_rec(sat_solver2* s, int x)
{
    clause* c;
    int i, y;
    if ( (var_tag(s,x) & 8) )
        return;
    c = clause2_read(s, var_reason(s,x));
    clause_foreach_var( c, y, i, 1 )
        if ( var_level(s,y) && (var_tag(s,y) & 1) == 0 )
            solver2_logging_order_rec(s, y);
    var_add_tag(s, x, 8);
    veci_push(&s->min_step_order, x);
}

static int solver2_analyze(sat_solver2* s, clause* c, veci* learnt)
{
    int cnt      = 0;
    lit p        = 0;
    int x, ind   = s->qtail-1;
    int proof_id = 0;
    lit* lits,* vars, i, j, k;
    assert( veci_size(&s->tagged) == 0 );
    // tag[0] - visited by conflict analysis (afterwards: literals of the learned clause)
    // tag[1] - visited by solver2_lit_removable() with success
    // tag[2] - visited by solver2_logging_order()

    proof_chain_start( s, c );
    veci_push(learnt,lit_Undef);
    while ( 1 ){
        assert(c != 0);
        if (c->lrn)
            act_clause2_bump(s,c);
        clause_foreach_var( c, x, j, (int)(p > 0) ){
            assert(x >= 0 && x < s->size);
            if ( var_tag(s, x) )
                continue;
            if ( var_level(s,x) == 0 )
            {
                proof_chain_resolve( s, NULL, x );
                continue;
            }
            var_set_tag(s, x, 1);
            act_var_bump(s,x);
            if (var_level(s,x) == solver2_dlevel(s))
                cnt++;
            else
                veci_push(learnt,c->lits[j]);
        }

        while (!var_tag(s, lit_var(s->trail[ind--])));

        p = s->trail[ind+1];
        c = clause2_read(s, lit_reason(s,p));
        cnt--;
        if ( cnt == 0 )
            break;
        proof_chain_resolve( s, c, lit_var(p) );
    }
    *veci_begin(learnt) = lit_neg(p);

    // mark levels
    assert( veci_size(&s->mark_levels) == 0 );
    lits = veci_begin(learnt);
    for (i = 1; i < veci_size(learnt); i++)
        var_lev_set_mark(s, lit_var(lits[i]));

    // simplify (full)
    veci_resize(&s->min_lit_order, 0);
    for (i = j = 1; i < veci_size(learnt); i++){
//        if (!solver2_lit_removable( s,lit_var(lits[i])))
        if (!solver2_lit_removable_rec(s,lit_var(lits[i]))) // changed to vars!!!
            lits[j++] = lits[i];
    }

    // record the proof
    if ( s->fProofLogging )
    {
        // collect additional clauses to resolve
        veci_resize(&s->min_step_order, 0);
        vars = veci_begin(&s->min_lit_order);
        for (i = 0; i < veci_size(&s->min_lit_order); i++)
//            solver2_logging_order(s, vars[i]);
            solver2_logging_order_rec(s, vars[i]);

        // add them in the reverse order
        vars = veci_begin(&s->min_step_order);
        for (i = veci_size(&s->min_step_order); i > 0; ) { i--;
            c = clause2_read(s, var_reason(s,vars[i]));
            proof_chain_resolve( s, c, vars[i] );
            clause_foreach_var(c, x, k, 1)
                if ( var_level(s,x) == 0 )
                    proof_chain_resolve( s, NULL, x );
        }
        proof_id = proof_chain_stop( s );
    }

    // unmark levels
    solver2_clear_marks( s );

    // update size of learnt + statistics
    veci_resize(learnt,j);
    s->stats.tot_literals += j;

    // clear tags
    solver2_clear_tags(s,0);

#ifdef DEBUG
    for (i = 0; i < s->size; i++)
        assert(!var_tag(s, i));
#endif

#ifdef VERBOSEDEBUG
    Abc_Print(1,L_IND"Learnt {", L_ind);
    for (i = 0; i < veci_size(learnt); i++) Abc_Print(1," "L_LIT, L_lit(lits[i]));
#endif
    if (veci_size(learnt) > 1){
        lit tmp;
        int max_i = 1;
        int max   = var_level(s, lit_var(lits[1]));
        for (i = 2; i < veci_size(learnt); i++)
            if (max < var_level(s, lit_var(lits[i]))) {
                max = var_level(s, lit_var(lits[i]));
                max_i = i;
            }

        tmp         = lits[1];
        lits[1]     = lits[max_i];
        lits[max_i] = tmp;
    }
#ifdef VERBOSEDEBUG
    {
        int lev = veci_size(learnt) > 1 ? var_level(s, lit_var(lits[1])) : 0;
        Abc_Print(1," } at level %d\n", lev);
    }
#endif
    return proof_id;
}

clause* solver2_propagate(sat_solver2* s)
{
    clause* c, * confl  = NULL;
    veci* ws;
    lit* lits, false_lit, p, * stop, * k;
    cla* begin,* end, *i, *j;
    int Lit;

    while (confl == 0 && s->qtail - s->qhead > 0){

        p  = s->trail[s->qhead++];
        ws = solver2_wlist(s,p);
        begin = (cla*)veci_begin(ws);
        end   = begin + veci_size(ws);

        s->stats.propagations++;
        for (i = j = begin; i < end; ){
            c = clause2_read(s,*i);
            lits = c->lits;

            // Make sure the false literal is data[1]:
            false_lit = lit_neg(p);
            if (lits[0] == false_lit){
                lits[0] = lits[1];
                lits[1] = false_lit;
            }
            assert(lits[1] == false_lit);

            // If 0th watch is true, then clause is already satisfied.
            if (var_value(s, lit_var(lits[0])) == lit_sign(lits[0]))
                *j++ = *i;
            else{
                // Look for new watch:
                stop = lits + c->size;
                for (k = lits + 2; k < stop; k++){
                    if (var_value(s, lit_var(*k)) != !lit_sign(*k)){
                        lits[1] = *k;
                        *k = false_lit;
                        veci_push(solver2_wlist(s,lit_neg(lits[1])),*i);
                        goto WatchFound; }
                }

                // Did not find watch -- clause is unit under assignment:
                Lit = lits[0];
                if ( s->fProofLogging && solver2_dlevel(s) == 0 )
                {
                    int k, x, proof_id, Cid, Var = lit_var(Lit);
                    int fLitIsFalse = (var_value(s, Var) == !lit_sign(Lit));
                    // Log production of top-level unit clause:
                    proof_chain_start( s, c );
                    clause_foreach_var( c, x, k, 1 ){
                        assert( var_level(s, x) == 0 );
                        proof_chain_resolve( s, NULL, x );
                    }
                    proof_id = proof_chain_stop( s );
                    // get a new clause
                    Cid = clause2_create_new( s, &Lit, &Lit + 1, 1, proof_id );
                    assert( (var_unit_clause(s, Var) == NULL) != fLitIsFalse );
                    // if variable already has unit clause, it must be with the other polarity 
                    // in this case, we should derive the empty clause here
                    if ( var_unit_clause(s, Var) == NULL )
                        var_set_unit_clause(s, Var, Cid);
                    else{
                        // Empty clause derived:
                        proof_chain_start( s, clause2_read(s,Cid) );
                        proof_chain_resolve( s, NULL, Var );
                        proof_id = proof_chain_stop( s );
                        s->hProofLast = proof_id;
//                        clause2_create_new( s, &Lit, &Lit, 1, proof_id );
                    }
                }

                *j++ = *i;
                // Clause is unit under assignment:
                if ( c->lrn )
                    c->lbd = sat_clause_compute_lbd(s, c);
                if (!solver2_enqueue(s,Lit, *i)){
                    confl = clause2_read(s,*i++);
                    // Copy the remaining watches:
                    while (i < end)
                        *j++ = *i++;
                }
            }
WatchFound: i++;
        }
        s->stats.inspects += j - (int*)veci_begin(ws);
        veci_resize(ws,j - (int*)veci_begin(ws));
    }

    return confl;
}

int sat_solver2_simplify(sat_solver2* s)
{
    assert(solver2_dlevel(s) == 0);
    return (solver2_propagate(s) == NULL);
}

static lbool solver2_search(sat_solver2* s, ABC_INT64_T nof_conflicts)
{
    double  random_var_freq = s->fNotUseRandom ? 0.0 : 0.02;

    ABC_INT64_T  conflictC       = 0;
    veci    learnt_clause;
    int     proof_id;

    assert(s->root_level == solver2_dlevel(s));

    s->stats.starts++;
//    s->var_decay = (float)(1 / 0.95   );
//    s->cla_decay = (float)(1 / 0.999);
    veci_new(&learnt_clause);

    for (;;){
        clause* confl = solver2_propagate(s);
        if (confl != 0){
            // CONFLICT
            int blevel;

#ifdef VERBOSEDEBUG
            Abc_Print(1,L_IND"**CONFLICT**\n", L_ind);
#endif
            s->stats.conflicts++; conflictC++;
            if (solver2_dlevel(s) <= s->root_level){
                proof_id = solver2_analyze_final(s, confl, 0);
                if ( s->pPrf1 )
                    assert( proof_id > 0 );
                s->hProofLast = proof_id;
                veci_delete(&learnt_clause);
                return l_False;
            }

            veci_resize(&learnt_clause,0);
            proof_id = solver2_analyze(s, confl, &learnt_clause);
            blevel = veci_size(&learnt_clause) > 1 ? var_level(s, lit_var(veci_begin(&learnt_clause)[1])) : s->root_level;
            blevel = s->root_level > blevel ? s->root_level : blevel;
            solver2_canceluntil(s,blevel);
            solver2_record(s,&learnt_clause, proof_id);
            // if (learnt_clause.size() == 1) level[var(learnt_clause[0])] = 0;    
            // (this is ugly (but needed for 'analyzeFinal()') -- in future versions, we will backtrack past the 'root_level' and redo the assumptions)
            if ( learnt_clause.size == 1 )
                var_set_level( s, lit_var(learnt_clause.ptr[0]), 0 );
            act_var_decay(s);
            act_clause2_decay(s);

        }else{
            // NO CONFLICT
            int next;

            if ((nof_conflicts >= 0 && conflictC >= nof_conflicts) || (s->nRuntimeLimit && (s->stats.conflicts & 63) == 0 && Abc_Clock() > s->nRuntimeLimit)){
                // Reached bound on number of conflicts:
                s->progress_estimate = solver2_progress(s);
                solver2_canceluntil(s,s->root_level);
                veci_delete(&learnt_clause);
                return l_Undef; }

            if ( (s->nConfLimit && s->stats.conflicts > s->nConfLimit) ||
//                 (s->nInsLimit  && s->stats.inspects  > s->nInsLimit) )
                 (s->nInsLimit  && s->stats.propagations > s->nInsLimit) )
            {
                // Reached bound on number of conflicts:
                s->progress_estimate = solver2_progress(s);
                solver2_canceluntil(s,s->root_level);
                veci_delete(&learnt_clause);
                return l_Undef;
            }

//            if (solver2_dlevel(s) == 0 && !s->fSkipSimplify)
                // Simplify the set of problem clauses:
//                sat_solver2_simplify(s);

            // Reduce the set of learnt clauses:
//            if (s->nLearntMax > 0 && s->stats.learnts >= (unsigned)s->nLearntMax)
//                sat_solver2_reducedb(s);

            // New variable decision:
            s->stats.decisions++;
            next = order_select(s,(float)random_var_freq);

            if (next == var_Undef){
                // Model found:
                int i;
                for (i = 0; i < s->size; i++)
                {
                    assert( var_value(s,i) != varX );
                    s->model[i] = (var_value(s,i)==var1 ? l_True : l_False);
                }
                solver2_canceluntil(s,s->root_level);
                veci_delete(&learnt_clause);
                return l_True;
            }

            if ( var_polar(s, next) ) // positive polarity
                solver2_assume(s,toLit(next));
            else
                solver2_assume(s,lit_neg(toLit(next)));
        }
    }

    return l_Undef; // cannot happen
}

//=================================================================================================
// External solver functions:

sat_solver2* sat_solver2_new(void)
{
    sat_solver2* s = (sat_solver2 *)ABC_CALLOC( char, sizeof(sat_solver2) );

#ifdef USE_FLOAT_ACTIVITY2
    s->var_inc        = 1;
    s->cla_inc        = 1;
    s->var_decay      = (float)(1 / 0.95  );
    s->cla_decay      = (float)(1 / 0.999 );
//    s->cla_decay      = 1;
//    s->var_decay      = 1;
#else
    s->var_inc        = (1 <<  5);
    s->cla_inc        = (1 << 11);
#endif
    s->random_seed    = 91648253;

#ifdef SAT_USE_PROOF_LOGGING
    s->fProofLogging  =  1;
#else
    s->fProofLogging  =  0;
#endif
    s->fSkipSimplify  =  1;
    s->fNotUseRandom  =  0;
    s->fVerbose       =  0;

    s->nLearntStart   = LEARNT_MAX_START_DEFAULT;  // starting learned clause limit
    s->nLearntDelta   = LEARNT_MAX_INCRE_DEFAULT;  // delta of learned clause limit
    s->nLearntRatio   = LEARNT_MAX_RATIO_DEFAULT;  // ratio of learned clause limit
    s->nLearntMax     = s->nLearntStart;

    // initialize vectors
    veci_new(&s->order);
    veci_new(&s->trail_lim);
    veci_new(&s->tagged);
    veci_new(&s->stack);
    veci_new(&s->temp_clause);
    veci_new(&s->temp_proof);
    veci_new(&s->conf_final);
    veci_new(&s->mark_levels);
    veci_new(&s->min_lit_order);
    veci_new(&s->min_step_order);
//    veci_new(&s->learnt_live);
    Sat_MemAlloc_( &s->Mem, 14 );
    veci_new(&s->act_clas);  
    // proof-logging
    veci_new(&s->claProofs);
//    s->pPrf1 = Vec_SetAlloc( 20 );
    s->tempInter = -1;

    // initialize clause pointers
    s->hLearntLast            = -1; // the last learnt clause 
    s->hProofLast             = -1; // the last proof ID
    // initialize rollback
    s->iVarPivot              =  0; // the pivot for variables
    s->iTrailPivot            =  0; // the pivot for trail
    s->hProofPivot            =  1; // the pivot for proof records
    return s;
}


void sat_solver2_setnvars(sat_solver2* s,int n)
{
    int var;

    if (s->cap < n){
        int old_cap = s->cap;
        while (s->cap < n) s->cap = s->cap*2+1;

        s->wlists    = ABC_REALLOC(veci,     s->wlists,   s->cap*2);
        s->vi        = ABC_REALLOC(varinfo2, s->vi,       s->cap);
        s->levels    = ABC_REALLOC(int,      s->levels,   s->cap);
        s->assigns   = ABC_REALLOC(char,     s->assigns,  s->cap);
        s->trail     = ABC_REALLOC(lit,      s->trail,    s->cap);
        s->orderpos  = ABC_REALLOC(int,      s->orderpos, s->cap);
        s->reasons   = ABC_REALLOC(cla,      s->reasons,  s->cap);
        if ( s->fProofLogging )
        s->units     = ABC_REALLOC(cla,      s->units,    s->cap);
#ifdef USE_FLOAT_ACTIVITY2
        s->activity  = ABC_REALLOC(double,   s->activity, s->cap);
#else
        s->activity  = ABC_REALLOC(unsigned, s->activity, s->cap);
        s->activity2 = ABC_REALLOC(unsigned, s->activity2,s->cap);
#endif
        s->model     = ABC_REALLOC(int,      s->model,    s->cap);
        memset( s->wlists + 2*old_cap, 0, 2*(s->cap-old_cap)*sizeof(vecp) );
    }

    for (var = s->size; var < n; var++){
        assert(!s->wlists[2*var].size);
        assert(!s->wlists[2*var+1].size);
        if ( s->wlists[2*var].ptr == NULL )
            veci_new(&s->wlists[2*var]);
        if ( s->wlists[2*var+1].ptr == NULL )
            veci_new(&s->wlists[2*var+1]);
        *((int*)s->vi + var) = 0; //s->vi[var].val = varX;
        s->levels  [var] = 0;
        s->assigns [var] = varX;
        s->reasons [var] = 0;
        if ( s->fProofLogging )
        s->units   [var] = 0;
#ifdef USE_FLOAT_ACTIVITY2
        s->activity[var] = 0;
#else
        s->activity[var] = (1<<10);
#endif
        s->model   [var] = 0;
        // does not hold because variables enqueued at top level will not be reinserted in the heap
        // assert(veci_size(&s->order) == var); 
        s->orderpos[var] = veci_size(&s->order);
        veci_push(&s->order,var);
        order_update(s, var);
    }
    s->size = n > s->size ? n : s->size;
}

void sat_solver2_delete(sat_solver2* s)
{
    int fVerify = 0;
    if ( fVerify )
    {
        veci * pCore = (veci *)Sat_ProofCore( s );
//        Abc_Print(1, "UNSAT core contains %d clauses (%6.2f %%).\n", veci_size(pCore), 100.0*veci_size(pCore)/veci_size(&s->clauses) );
        veci_delete( pCore );
        ABC_FREE( pCore );
//        if ( s->fProofLogging )
//            Sat_ProofCheck( s );
    }

    // report statistics
//    Abc_Print(1, "Used %6.2f MB for proof-logging.   Unit clauses = %d.\n", 1.0 * Vec_ReportMemory(&s->Proofs) / (1<<20), s->nUnits );

    // delete vectors
    veci_delete(&s->order);
    veci_delete(&s->trail_lim);
    veci_delete(&s->tagged);
    veci_delete(&s->stack);
    veci_delete(&s->temp_clause);
    veci_delete(&s->temp_proof);
    veci_delete(&s->conf_final);
    veci_delete(&s->mark_levels);
    veci_delete(&s->min_lit_order);
    veci_delete(&s->min_step_order);
//    veci_delete(&s->learnt_live);
    veci_delete(&s->act_clas);
    veci_delete(&s->claProofs);
//    veci_delete(&s->clauses);
//    veci_delete(&s->lrns);
    Sat_MemFree_( &s->Mem );
//    veci_delete(&s->proofs);
    Vec_SetFree( s->pPrf1 );
    Prf_ManStop( s->pPrf2 );
    Int2_ManStop( s->pInt2 );

    // delete arrays
    if (s->vi != 0){
        int i;
        if ( s->wlists )
            for (i = 0; i < s->cap*2; i++)
                veci_delete(&s->wlists[i]);
        ABC_FREE(s->wlists   );
        ABC_FREE(s->vi       );
        ABC_FREE(s->levels   );
        ABC_FREE(s->assigns  );
        ABC_FREE(s->trail    );
        ABC_FREE(s->orderpos );
        ABC_FREE(s->reasons  );
        ABC_FREE(s->units    );
        ABC_FREE(s->activity );
        ABC_FREE(s->activity2);
        ABC_FREE(s->model    );
    }
    ABC_FREE(s);

//    Abc_PrintTime( 1, "Time", Time );
}


int sat_solver2_addclause(sat_solver2* s, lit* begin, lit* end, int Id)
{
    cla Cid;
    lit *i,*j,*iFree = NULL;
    int maxvar, count, temp;
    assert( solver2_dlevel(s) == 0 );
    assert( begin < end );
    assert( Id != 0 );

    // copy clause into storage
    veci_resize( &s->temp_clause, 0 );
    for ( i = begin; i < end; i++ )
        veci_push( &s->temp_clause, *i );
    begin = veci_begin( &s->temp_clause );
    end = begin + veci_size( &s->temp_clause );

    // insertion sort
    maxvar = lit_var(*begin);
    for (i = begin + 1; i < end; i++){
        lit l = *i;
        maxvar = lit_var(l) > maxvar ? lit_var(l) : maxvar;
        for (j = i; j > begin && *(j-1) > l; j--)
            *j = *(j-1);
        *j = l;
    }
    sat_solver2_setnvars(s,maxvar+1);


    // delete duplicates
    for (i = j = begin + 1; i < end; i++)
    {
        if ( *(i-1) == lit_neg(*i) ) // tautology
            return clause2_create_new( s, begin, end, 0, 0 ); // add it anyway, to preserve proper clause count       
        if ( *(i-1) != *i )
            *j++ = *i;
    }
    end = j;
    assert( begin < end );

    // coount the number of 0-literals
    count = 0;
    for ( i = begin; i < end; i++ )
    {
        // make sure all literals are unique
        assert( i == begin || lit_var(*(i-1)) != lit_var(*i) );
        // consider the value of this literal
        if ( var_value(s, lit_var(*i)) == lit_sign(*i) ) // this clause is always true
            return clause2_create_new( s, begin, end, 0, 0 ); // add it anyway, to preserve proper clause count       
        if ( var_value(s, lit_var(*i)) == varX ) // unassigned literal
            iFree = i;
        else
            count++; // literal is 0
    }
    assert( count < end-begin ); // the clause cannot be UNSAT

    // swap variables to make sure the clause is watched using unassigned variable
    temp   = *iFree;
    *iFree = *begin;
    *begin = temp;

    // create a new clause
    Cid = clause2_create_new( s, begin, end, 0, 0 );
    if ( Id )
        clause2_set_id( s, Cid, Id );

    // handle unit clause
    if ( count+1 == end-begin )
    {
        if ( s->fProofLogging )
        {
            if ( count == 0 ) // original learned clause
            {
                var_set_unit_clause( s, lit_var(begin[0]), Cid );
                if ( !solver2_enqueue(s,begin[0],0) )
                    assert( 0 );
            }
            else
            {
                // Log production of top-level unit clause:
                int x, k, proof_id, CidNew;
                clause* c = clause2_read(s, Cid);
                proof_chain_start( s, c );
                clause_foreach_var( c, x, k, 1 )
                    proof_chain_resolve( s, NULL, x );
                proof_id = proof_chain_stop( s );
                // generate a new clause
                CidNew = clause2_create_new( s, begin, begin+1, 1, proof_id );
                var_set_unit_clause( s, lit_var(begin[0]), CidNew );
                if ( !solver2_enqueue(s,begin[0],Cid) )
                    assert( 0 );
            }
        }
    }
    return Cid;
}


double luby2(double y, int x)
{
    int size, seq;
    for (size = 1, seq = 0; size < x+1; seq++, size = 2*size + 1);
    while (size-1 != x){
        size = (size-1) >> 1;
        seq--;
        x = x % size;
    }
    return pow(y, (double)seq);
}

void luby2_test()
{
    int i;
    for ( i = 0; i < 20; i++ )
        Abc_Print(1, "%d ", (int)luby2(2,i) );
    Abc_Print(1, "\n" );
}


// updates clauses, watches, units, and proof
void sat_solver2_reducedb(sat_solver2* s)
{
    static abctime TimeTotal = 0;
    Sat_Mem_t * pMem = &s->Mem;
    clause * c = NULL;
    int nLearnedOld = veci_size(&s->act_clas);
    int * act_clas = veci_begin(&s->act_clas);
    int * pPerm, * pSortValues, nCutoffValue, * pClaProofs;
    int i, j, k, Id, nSelected;//, LastSize = 0;
    int Counter, CounterStart;
    abctime clk = Abc_Clock();
    static int Count = 0;
    Count++;
    assert( s->nLearntMax );
    s->nDBreduces++;
//    printf( "Calling reduceDB with %d clause limit and parameters (%d %d %d).\n", s->nLearntMax, s->nLearntStart, s->nLearntDelta, s->nLearntRatio );

/*
    // find the new limit
    s->nLearntMax = s->nLearntMax * 11 / 10;
    // preserve 1/10 of last clauses
    CounterStart  = s->stats.learnts - (s->nLearntMax / 10);
    // preserve 1/10 of most active clauses
    pSortValues = veci_begin(&s->act_clas);
    pPerm = Abc_MergeSortCost( pSortValues, nLearnedOld );
    assert( pSortValues[pPerm[0]] <= pSortValues[pPerm[nLearnedOld-1]] );
    nCutoffValue = pSortValues[pPerm[nLearnedOld*9/10]];
    ABC_FREE( pPerm );
//    nCutoffValue = ABC_INFINITY;
*/


    // find the new limit
    s->nLearntMax = s->nLearntStart + s->nLearntDelta * s->nDBreduces;
    // preserve 1/20 of last clauses
    CounterStart  = nLearnedOld - (s->nLearntMax / 20);
    // preserve 3/4 of most active clauses
    nSelected = nLearnedOld*s->nLearntRatio/100;
    // create sorting values
    pSortValues = ABC_ALLOC( int, nLearnedOld );
    Sat_MemForEachLearned( pMem, c, i, k )
    {
        Id = clause_id(c);
        pSortValues[Id] = (((7 - Abc_MinInt(c->lbd, 7)) << 28) | (act_clas[Id] >> 4));
//        pSortValues[Id] = act_clas[Id];
        assert( pSortValues[Id] >= 0 );
    }
    // find non-decreasing permutation
    pPerm = Abc_MergeSortCost( pSortValues, nLearnedOld );
    assert( pSortValues[pPerm[0]] <= pSortValues[pPerm[nLearnedOld-1]] );
    nCutoffValue = pSortValues[pPerm[nLearnedOld-nSelected]];
    ABC_FREE( pPerm );
//    nCutoffValue = ABC_INFINITY;

    // count how many clauses satisfy the condition
    Counter = j = 0;
    Sat_MemForEachLearned( pMem, c, i, k )
    {
        assert( c->mark == 0 );
        if ( Counter++ > CounterStart || clause_size(c) < 2 || pSortValues[clause_id(c)] >= nCutoffValue || s->reasons[lit_var(c->lits[0])] == Sat_MemHand(pMem, i, k) )
        {
        }
        else
            j++;
        if ( j >= nLearnedOld / 6 )
            break;
    }
    if ( j < nLearnedOld / 6 )
    {
        ABC_FREE( pSortValues );
        return;
    }

    // mark learned clauses to remove
    Counter = j = 0;
    pClaProofs = veci_size(&s->claProofs) ? veci_begin(&s->claProofs) : NULL;
    Sat_MemForEachLearned( pMem, c, i, k )
    {
        assert( c->mark == 0 );
        if ( Counter++ > CounterStart || clause_size(c) < 2 || pSortValues[clause_id(c)] >= nCutoffValue || s->reasons[lit_var(c->lits[0])] == Sat_MemHand(pMem, i, k) )
        {
            pSortValues[j] = pSortValues[clause_id(c)];
            if ( pClaProofs ) 
                pClaProofs[j] = pClaProofs[clause_id(c)];
            if ( s->pPrf2 )
                Prf_ManAddSaved( s->pPrf2, clause_id(c), j );
            j++;
        }
        else // delete
        {
            c->mark = 1;
            s->stats.learnts_literals -= clause_size(c);
            s->stats.learnts--;
        }
    }
    ABC_FREE( pSortValues );
    if ( s->pPrf2 )
        Prf_ManCompact( s->pPrf2, j );
//    if ( j == nLearnedOld )
//        return;

    assert( s->stats.learnts == (unsigned)j );
    assert( Counter == nLearnedOld );
    veci_resize(&s->act_clas,j);
    if ( veci_size(&s->claProofs) )
        veci_resize(&s->claProofs,j);

    // update ID of each clause to be its new handle
    Counter = Sat_MemCompactLearned( pMem, 0 );
    assert( Counter == (int)s->stats.learnts );

    // update reasons
    for ( i = 0; i < s->size; i++ )
    {
        if ( !s->reasons[i] ) // no reason
            continue;
        if ( clause_is_lit(s->reasons[i]) ) // 2-lit clause
            continue;
        if ( !clause_learnt_h(pMem, s->reasons[i]) ) // problem clause
            continue;
        assert( c->lrn );
        c = clause2_read( s, s->reasons[i] );
        assert( c->mark == 0 );
        s->reasons[i] = clause_id(c); // updating handle here!!!
    }

    // update watches
    for ( i = 0; i < s->size*2; i++ )
    {
        int * pArray = veci_begin(&s->wlists[i]);
        for ( j = k = 0; k < veci_size(&s->wlists[i]); k++ )
        {
            if ( clause_is_lit(pArray[k]) ) // 2-lit clause
                pArray[j++] = pArray[k];
            else if ( !clause_learnt_h(pMem, pArray[k]) ) // problem clause
                pArray[j++] = pArray[k];
            else 
            {
                assert( c->lrn );
                c = clause2_read(s, pArray[k]);
                if ( !c->mark ) // useful learned clause
                   pArray[j++] = clause_id(c); // updating handle here!!!
            }
        }
        veci_resize(&s->wlists[i],j);
    }

    // compact units
    if ( s->fProofLogging )
        for ( i = 0; i < s->size; i++ )
            if ( s->units[i] && clause_learnt_h(pMem, s->units[i]) )
            {
                assert( c->lrn );
                c = clause2_read( s, s->units[i] );
                assert( c->mark == 0 );
                s->units[i] = clause_id(c);
            }

    // perform final move of the clauses
    Counter = Sat_MemCompactLearned( pMem, 1 );
    assert( Counter == (int)s->stats.learnts );

    // compact proof (compacts 'proofs' and update 'claProofs')
    if ( s->pPrf1 )
    {
        extern int Sat_ProofReduce( Vec_Set_t * vProof, void * pRoots, int hProofPivot );
        s->hProofPivot = Sat_ProofReduce( s->pPrf1, &s->claProofs, s->hProofPivot );
    }

    // report the results
    TimeTotal += Abc_Clock() - clk;
    if ( s->fVerbose )
    {
        Abc_Print(1, "reduceDB: Keeping %7d out of %7d clauses (%5.2f %%)  ",
            s->stats.learnts, nLearnedOld, 100.0 * s->stats.learnts / nLearnedOld );
        Abc_PrintTime( 1, "Time", TimeTotal );
    }
}

// reverses to the previously bookmarked point
void sat_solver2_rollback( sat_solver2* s )
{
    Sat_Mem_t * pMem = &s->Mem;
    int i, k, j;
    static int Count = 0;
    Count++;
    assert( s->iVarPivot >= 0 && s->iVarPivot <= s->size );
    assert( s->iTrailPivot >= 0 && s->iTrailPivot <= s->qtail );
    assert( s->pPrf1 == NULL || (s->hProofPivot >= 1 && s->hProofPivot <= Vec_SetHandCurrent(s->pPrf1)) );
    // reset implication queue
    solver2_canceluntil_rollback( s, s->iTrailPivot );
    // update order 
    if ( s->iVarPivot < s->size )
    { 
        if ( s->activity2 )
        {
            s->var_inc = s->var_inc2;
            memcpy( s->activity, s->activity2, sizeof(unsigned) * s->iVarPivot );
        }
        veci_resize(&s->order, 0);
        for ( i = 0; i < s->iVarPivot; i++ )
        {
            if ( var_value(s, i) != varX )
                continue;
            s->orderpos[i] = veci_size(&s->order);
            veci_push(&s->order,i);
            order_update(s, i);
        }
    }
    // compact watches
    for ( i = 0; i < s->iVarPivot*2; i++ )
    {
        cla* pArray = veci_begin(&s->wlists[i]);
        for ( j = k = 0; k < veci_size(&s->wlists[i]); k++ )
            if ( Sat_MemClauseUsed(pMem, pArray[k]) )
                pArray[j++] = pArray[k];
        veci_resize(&s->wlists[i],j);
    }
    // reset watcher lists
    for ( i = 2*s->iVarPivot; i < 2*s->size; i++ )
        s->wlists[i].size = 0;

    // reset clause counts
    s->stats.clauses = pMem->BookMarkE[0];
    s->stats.learnts = pMem->BookMarkE[1];
    // rollback clauses
    Sat_MemRollBack( pMem );

    // resize learned arrays
    veci_resize(&s->act_clas,  s->stats.learnts);
    if ( s->pPrf1 ) 
    {
        veci_resize(&s->claProofs, s->stats.learnts);
        Vec_SetShrink(s->pPrf1, s->hProofPivot); 
        // some weird bug here, which shows only on 64-bits!
        // temporarily, perform more general proof reduction
//        Sat_ProofReduce( s->pPrf1, &s->claProofs, s->hProofPivot );
    }
    assert( s->pPrf2 == NULL );
//    if ( s->pPrf2 )
//        Prf_ManShrink( s->pPrf2, s->stats.learnts );

    // initialize other vars
    s->size = s->iVarPivot;
    if ( s->size == 0 )
    {
    //    s->size                   = 0;
    //    s->cap                    = 0;
        s->qhead                  = 0;
        s->qtail                  = 0;
#ifdef USE_FLOAT_ACTIVITY2
        s->var_inc                = 1;
        s->cla_inc                = 1;
        s->var_decay              = (float)(1 / 0.95  );
        s->cla_decay              = (float)(1 / 0.999 );
#else
        s->var_inc                = (1 <<  5);
        s->cla_inc                = (1 << 11);
#endif
        s->root_level             = 0;
        s->random_seed            = 91648253;
        s->progress_estimate      = 0;
        s->verbosity              = 0;

        s->stats.starts           = 0;
        s->stats.decisions        = 0;
        s->stats.propagations     = 0;
        s->stats.inspects         = 0;
        s->stats.conflicts        = 0;
        s->stats.clauses          = 0;
        s->stats.clauses_literals = 0;
        s->stats.learnts          = 0;
        s->stats.learnts_literals = 0;
        s->stats.tot_literals     = 0;
        // initialize clause pointers
        s->hLearntLast            = -1; // the last learnt clause 
        s->hProofLast             = -1; // the last proof ID

        // initialize rollback
        s->iVarPivot              =  0; // the pivot for variables
        s->iTrailPivot            =  0; // the pivot for trail
        s->hProofPivot            =  1; // the pivot for proof records
    }
}

// returns memory in bytes used by the SAT solver
double sat_solver2_memory( sat_solver2* s, int fAll )
{
    int i;
    double Mem = sizeof(sat_solver2);
    if ( fAll )
        for (i = 0; i < s->cap*2; i++)
            Mem += s->wlists[i].cap * sizeof(int);
    Mem += s->cap * sizeof(veci);     // ABC_FREE(s->wlists   );
    Mem += s->act_clas.cap * sizeof(int);
    Mem += s->claProofs.cap * sizeof(int);
//    Mem += s->cap * sizeof(char);     // ABC_FREE(s->polarity );
//    Mem += s->cap * sizeof(char);     // ABC_FREE(s->tags     );
    Mem += s->cap * sizeof(varinfo2); // ABC_FREE(s->vi       );
    Mem += s->cap * sizeof(int);      // ABC_FREE(s->levels   );
    Mem += s->cap * sizeof(char);     // ABC_FREE(s->assigns  );
#ifdef USE_FLOAT_ACTIVITY2
    Mem += s->cap * sizeof(double);   // ABC_FREE(s->activity );
#else
    Mem += s->cap * sizeof(unsigned); // ABC_FREE(s->activity );
    if ( s->activity2 )
    Mem += s->cap * sizeof(unsigned); // ABC_FREE(s->activity2);
#endif
    Mem += s->cap * sizeof(lit);      // ABC_FREE(s->trail    );
    Mem += s->cap * sizeof(int);      // ABC_FREE(s->orderpos );
    Mem += s->cap * sizeof(int);      // ABC_FREE(s->reasons  );
    Mem += s->cap * sizeof(int);      // ABC_FREE(s->units    );
    Mem += s->cap * sizeof(int);      // ABC_FREE(s->model    );
    Mem += s->tagged.cap * sizeof(int);
    Mem += s->stack.cap * sizeof(int);
    Mem += s->order.cap * sizeof(int);
    Mem += s->trail_lim.cap * sizeof(int);
    Mem += s->temp_clause.cap * sizeof(int);
    Mem += s->conf_final.cap * sizeof(int);
    Mem += s->mark_levels.cap * sizeof(int);
    Mem += s->min_lit_order.cap * sizeof(int);
    Mem += s->min_step_order.cap * sizeof(int);
    Mem += s->temp_proof.cap * sizeof(int);
    Mem += Sat_MemMemoryAll( &s->Mem );
//    Mem += Vec_ReportMemory( s->pPrf1 );
    return Mem;
}
double sat_solver2_memory_proof( sat_solver2* s )
{
    double Mem = s->dPrfMemory;
    if ( s->pPrf1 )
        Mem += Vec_ReportMemory( s->pPrf1 );
    return Mem;
}


// find the clause in the watcher lists
/*
int sat_solver2_find_clause( sat_solver2* s, int Hand, int fVerbose )
{
    int i, k, Found = 0;
    if ( Hand >= s->clauses.size )
        return 1;
    for ( i = 0; i < s->size*2; i++ )
    {
        cla* pArray = veci_begin(&s->wlists[i]);
        for ( k = 0; k < veci_size(&s->wlists[i]); k++ )
            if ( (pArray[k] >> 1) == Hand )
            {
                if ( fVerbose )
                Abc_Print(1, "Clause found in list %d at position %d.\n", i, k );
                Found = 1;
                break;
            }
    }
    if ( Found == 0 )
    {
        if ( fVerbose )
        Abc_Print(1, "Clause with handle %d is not found.\n", Hand );
    }
    return Found;
}
*/
/*
// verify that all problem clauses are satisfied
void sat_solver2_verify( sat_solver2* s )
{
    clause * c;
    int i, k, v, Counter = 0;
    clause_foreach_entry( &s->clauses, c, i, 1 )
    {
        for ( k = 0; k < (int)c->size; k++ )
        {
            v = lit_var(c->lits[k]);
            if ( sat_solver2_var_value(s, v) ^ lit_sign(c->lits[k]) )
                break;
        }
        if ( k == (int)c->size )
        {
            Abc_Print(1, "Clause %d is not satisfied.   ", c->Id );
            clause_print_( c );
            sat_solver2_find_clause( s, clause_handle(&s->clauses, c), 1 );
            Counter++;
        }
    }
    if ( Counter != 0 )
        Abc_Print(1, "Verification failed!\n" );
//    else
//        Abc_Print(1, "Verification passed.\n" );
}
*/

// checks how many watched clauses are satisfied by 0-level assignments
// (this procedure may be called before setting up a new bookmark for rollback)
int sat_solver2_check_watched( sat_solver2* s )
{
    clause * c;
    int i, k, j, m;
    int claSat[2] = {0};
    // update watches
    for ( i = 0; i < s->size*2; i++ )
    {
        int * pArray = veci_begin(&s->wlists[i]);
        for ( m = k = 0; k < veci_size(&s->wlists[i]); k++ )
        {
            c = clause2_read(s, pArray[k]);
            for ( j = 0; j < (int)c->size; j++ )
                if ( var_value(s, lit_var(c->lits[j])) == lit_sign(c->lits[j]) ) // true lit
                    break;
            if ( j == (int)c->size )
            {
                pArray[m++] = pArray[k];
                continue;
            }
            claSat[c->lrn]++;
        }
        veci_resize(&s->wlists[i],m);
        if ( m == 0 )
        {
//            s->wlists[i].cap = 0;
//            s->wlists[i].size = 0;
//            ABC_FREE( s->wlists[i].ptr );
        }
    }
//    printf( "\nClauses = %d  (Sat = %d).  Learned = %d  (Sat = %d).\n",
//        s->stats.clauses, claSat[0], s->stats.learnts, claSat[1] );
    return 0;
}

int sat_solver2_solve(sat_solver2* s, lit* begin, lit* end, ABC_INT64_T nConfLimit, ABC_INT64_T nInsLimit, ABC_INT64_T nConfLimitGlobal, ABC_INT64_T nInsLimitGlobal)
{
    int restart_iter = 0;
    ABC_INT64_T  nof_conflicts;
    lbool status = l_Undef;
    int proof_id;
    lit * i;

    s->hLearntLast = -1;
    s->hProofLast = -1;

    // set the external limits
//    s->nCalls++;
//    s->nRestarts  = 0;
    s->nConfLimit = 0;
    s->nInsLimit  = 0;
    if ( nConfLimit )
        s->nConfLimit = s->stats.conflicts + nConfLimit;
    if ( nInsLimit )
//        s->nInsLimit = s->stats.inspects + nInsLimit;
        s->nInsLimit = s->stats.propagations + nInsLimit;
    if ( nConfLimitGlobal && (s->nConfLimit == 0 || s->nConfLimit > nConfLimitGlobal) )
        s->nConfLimit = nConfLimitGlobal;
    if ( nInsLimitGlobal && (s->nInsLimit == 0 || s->nInsLimit > nInsLimitGlobal) )
        s->nInsLimit = nInsLimitGlobal;

/*
    // Perform assumptions:
    root_level = assumps.size();
    for (int i = 0; i < assumps.size(); i++){
        Lit p = assumps[i];
        assert(var(p) < nVars());
        if (!solver2_assume(p)){
            GClause r = reason[var(p)];
            if (r != Gclause2_NULL){
                clause* confl;
                if (r.isLit()){
                    confl = propagate_tmpbin;
                    (*confl)[1] = ~p;
                    (*confl)[0] = r.lit();
                }else
                    confl = r.clause();
                analyzeFinal(confl, true);
                conflict.push(~p);
            }else
                conflict.clear(),
                conflict.push(~p);
            cancelUntil(0);
            return false; }
        clause* confl = propagate();
        if (confl != NULL){
            analyzeFinal(confl), assert(conflict.size() > 0);
            cancelUntil(0);
            return false; }
    }
    assert(root_level == decisionLevel());
*/

    // Perform assumptions:
    s->root_level = end - begin;
    for ( i = begin; i < end; i++ )
    {
        lit p = *i;
        assert(lit_var(p) < s->size);
        veci_push(&s->trail_lim,s->qtail);
        if (!solver2_enqueue(s,p,0))
        {
            clause* r = clause2_read(s, lit_reason(s,p));
            if (r != NULL)
            {
                clause* confl = r;
                proof_id = solver2_analyze_final(s, confl, 1);
                veci_push(&s->conf_final, lit_neg(p));
            }
            else
            {
//                assert( 0 );
//                r = var_unit_clause( s, lit_var(p) );
//                assert( r != NULL );
//                proof_id = clause2_proofid(s, r, 0);
                proof_id = -1; // the only case when ProofId is not assigned (conflicting assumptions)
                veci_resize(&s->conf_final,0);
                veci_push(&s->conf_final, lit_neg(p));
                // the two lines below are a bug fix by Siert Wieringa 
                if (var_level(s, lit_var(p)) > 0)
                    veci_push(&s->conf_final, p);
            }
            s->hProofLast = proof_id;
            solver2_canceluntil(s, 0);
            return l_False;
        }
        else
        {
            clause* confl = solver2_propagate(s);
            if (confl != NULL){
                proof_id = solver2_analyze_final(s, confl, 0);
                assert(s->conf_final.size > 0);
                s->hProofLast = proof_id;
                solver2_canceluntil(s, 0);
                return l_False;
            }
        }
    }
    assert(s->root_level == solver2_dlevel(s));

    if (s->verbosity >= 1){
        Abc_Print(1,"==================================[MINISAT]===================================\n");
        Abc_Print(1,"| Conflicts |     ORIGINAL     |              LEARNT              | Progress |\n");
        Abc_Print(1,"|           | Clauses Literals |   Limit Clauses Literals  Lit/Cl |          |\n");
        Abc_Print(1,"==============================================================================\n");
    }

    while (status == l_Undef){
        if (s->verbosity >= 1)
        {
            Abc_Print(1,"| %9.0f | %7.0f %8.0f | %7.0f %7.0f %8.0f %7.1f | %6.3f %% |\n",
                (double)s->stats.conflicts,
                (double)s->stats.clauses,
                (double)s->stats.clauses_literals,
                (double)s->nLearntMax,
                (double)s->stats.learnts,
                (double)s->stats.learnts_literals,
                (s->stats.learnts == 0)? 0.0 : (double)s->stats.learnts_literals / s->stats.learnts,
                s->progress_estimate*100);
            fflush(stdout);
        }
        if ( s->nRuntimeLimit && Abc_Clock() > s->nRuntimeLimit )
            break;
        // reduce the set of learnt clauses
        if ( s->nLearntMax && veci_size(&s->act_clas) >= s->nLearntMax && s->pPrf2 == NULL )
            sat_solver2_reducedb(s);
        // perform next run
        nof_conflicts = (ABC_INT64_T)( 100 * luby2(2, restart_iter++) );
        status = solver2_search(s, nof_conflicts);
        // quit the loop if reached an external limit
        if ( s->nConfLimit && s->stats.conflicts > s->nConfLimit )
            break;
        if ( s->nInsLimit  && s->stats.propagations > s->nInsLimit )
            break;
    }
    if (s->verbosity >= 1)
        Abc_Print(1,"==============================================================================\n");

    solver2_canceluntil(s,0);
//    assert( s->qhead == s->qtail );
//    if ( status == l_True )
//        sat_solver2_verify( s );
    return status;
}

void * Sat_ProofCore( sat_solver2 * s )
{
    extern void * Proof_DeriveCore( Vec_Set_t * vProof, int hRoot );
    if ( s->pPrf1 )
        return Proof_DeriveCore( s->pPrf1, s->hProofLast );
    if ( s->pPrf2 )
    {
        s->dPrfMemory = Abc_MaxDouble( s->dPrfMemory, Prf_ManMemory(s->pPrf2) );
        return Prf_ManUnsatCore( s->pPrf2 );
    }
    return NULL;
}

ABC_NAMESPACE_IMPL_END
