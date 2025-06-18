/****************************************************************************************[Solver.h]
 Glucose -- Copyright (c) 2009, Gilles Audemard, Laurent Simon
                CRIL - Univ. Artois, France
                LRI  - Univ. Paris Sud, France
 
Glucose sources are based on MiniSat (see below MiniSat copyrights). Permissions and copyrights of
Glucose are exactly the same as Minisat on which it is based on. (see below).

---------------
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2010, Niklas Sorensson

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

#ifndef Glucose_Solver_h
#define Glucose_Solver_h

#include "sat/glucose2/Vec.h"
#include "sat/glucose2/Heap.h"
#include "sat/glucose2/Alg.h"
#include "sat/glucose2/Options.h"
#include "sat/glucose2/SolverTypes.h"
#include "sat/glucose2/BoundedQueue.h"
#include "sat/glucose2/Constants.h"

#include "sat/glucose2/CGlucose.h"

ABC_NAMESPACE_CXX_HEADER_START

namespace Gluco2 {

//=================================================================================================
// Solver -- the main class:

class Solver {
public:

    int SolverType; // ABC identifies Glucose's type as 0

    // Constructor/Destructor:
    //
    Solver();
    virtual ~Solver();

    // ABC callbacks
    void * pCnfMan;                      // external CNF manager
    int(*pCnfFunc)(void * p, int, int*); // external callback. messages: 0: unsat; 1: sat; -1: still working
    int nCallConfl;                      // callback will be called every this number of conflicts
    bool terminate_search_early;         // used to stop the solver early if it as instructed by an external caller
    int * pstop;                         // another callback
    uint64_t nRuntimeLimit;              // runtime limit
    vec<int> user_vec;
    vec<Lit> user_lits;

    // circuit-based solving
    int jftr;
    void sat_solver_set_var_fanin_lit(int, int, int);  
    void sat_solver_start_new_round();  
    void sat_solver_mark_cone(int);  
    void sat_solver_set_jftr(int);
    int  sat_solver_jftr();
    void sat_solver_reset();

    // Problem specification:
    //
    Var     newVar    (bool polarity = true, bool dvar = true); // Add a new variable with parameters specifying variable mode.
    void    addVar    (Var v);                                  // Add enough variables to make sure there is variable v.

    bool    addClause (const vec<Lit>& ps);                     // Add a clause to the solver. 
    bool    addEmptyClause();                                   // Add the empty clause, making the solver contradictory.
    bool    addClause (Lit p);                                  // Add a unit clause to the solver. 
    bool    addClause (Lit p, Lit q);                           // Add a binary clause to the solver. 
    bool    addClause (Lit p, Lit q, Lit r);                    // Add a ternary clause to the solver. 
    bool    addClause_(      vec<Lit>& ps);                     // Add a clause to the solver without making superflous internal copy. Will
                                                                // change the passed vector 'ps'.

    // Solving:
    //
    bool    simplify     ();                        // Removes already satisfied clauses.
    bool    solve        (const vec<Lit>& assumps); // Search for a model that respects a given set of assumptions.
    lbool   solveLimited (const vec<Lit>& assumps); // Search for a model that respects a given set of assumptions (With resource constraints).
    bool    solve        ();                        // Search without assumptions.
    bool    solve        (Lit p);                   // Search for a model that respects a single assumption.
    bool    solve        (Lit p, Lit q);            // Search for a model that respects two assumptions.
    bool    solve        (Lit p, Lit q, Lit r);     // Search for a model that respects three assumptions.
    bool    okay         () const;                  // FALSE means solver is in a conflicting state

    void    toDimacs     (FILE* f, const vec<Lit>& assumps);            // Write CNF to file in DIMACS-format.
    void    toDimacs     (const char *file, const vec<Lit>& assumps);
    void    toDimacs     (FILE* f, Clause& c, vec<Var>& map, Var& max);
    void printLit(Lit l);
    void printClause(CRef c);
    void printInitialClause(CRef c);
    // Convenience versions of 'toDimacs()':
    void    toDimacs     (const char* file);
    void    toDimacs     (const char* file, Lit p);
    void    toDimacs     (const char* file, Lit p, Lit q);
    void    toDimacs     (const char* file, Lit p, Lit q, Lit r);
    
    // Variable mode:
    // 
    void    setPolarity    (Var v, bool b); // Declare which polarity the decision heuristic should use for a variable. Requires mode 'polarity_user'.
    void    setDecisionVar (Var v, bool b, bool use_oheap = true); // Declare if a variable should be eligible for selection in the decision heuristic.

    // Read state:
    //
    lbool   value      (Var x) const;       // The current value of a variable.
    lbool   value      (Lit p) const;       // The current value of a literal.
    lbool   modelValue (Var x) const;       // The value of a variable in the last model. The last call to solve must have been satisfiable.
    lbool   modelValue (Lit p) const;       // The value of a literal in the last model. The last call to solve must have been satisfiable.
    int     nAssigns   ()      const;       // The current number of assigned literals.
    int     nClauses   ()      const;       // The current number of original clauses.
    int     nLearnts   ()      const;       // The current number of learnt clauses.
    int     nVars      ()      const;       // The current number of variables.
    int     nFreeVars  ()      const;
    int *   getCex     ()      const;
    int     level      (Var x) const;       // moved level() to public to compile "struct JustOrderLt" -- alanmi

    // Incremental mode
    void setIncrementalMode();
    void initNbInitialVars(int nb);
    void printIncrementalStats();

    // Resource contraints:
    //
    void    setConfBudget(int64_t x);
    void    setPropBudget(int64_t x);
    void    budgetOff();
    void    interrupt();          // Trigger a (potentially asynchronous) interruption of the solver.
    void    clearInterrupt();     // Clear interrupt indicator flag.

    // Memory managment:
    //
    virtual void reset();
    virtual void garbageCollect(); // virtuality causes segfault for some reason
    void    checkGarbage(double gf);
    void    checkGarbage();




    // Extra results: (read-only member variable)
    //
    vec<lbool> model;             // If problem is satisfiable, this vector contains the model (if any).
    vec<Lit>   conflict;          // If problem is unsatisfiable (possibly under assumptions),
                                  // this vector represent the final conflict clause expressed in the assumptions.

    // Mode of operation:
    //
    int       verbosity;
    int       verbEveryConflicts;
    int       showModel;
    // Constants For restarts
    double    K;
    double    R;
    double    sizeLBDQueue;
    double    sizeTrailQueue;

    // Constants for reduce DB
    int firstReduceDB;
    int incReduceDB;
    int specialIncReduceDB;
    unsigned int lbLBDFrozenClause;

    // Constant for reducing clause
    int lbSizeMinimizingClause;
    unsigned int lbLBDMinimizingClause;

    double    var_decay;
    double    clause_decay;
    double    random_var_freq;
    double    random_seed;
    int       ccmin_mode;         // Controls conflict clause minimization (0=none, 1=basic, 2=deep).
    int       phase_saving;       // Controls the level of phase saving (0=none, 1=limited, 2=full).
    bool      rnd_pol;            // Use random polarities for branching heuristics.
    bool      rnd_init_act;       // Initialize variable activities with a small random value.
    double    garbage_frac;       // The fraction of wasted memory allowed before a garbage collection is triggered.

    // Certified UNSAT ( Thanks to Marijn Heule)
    FILE*               certifiedOutput;
    bool                certifiedUNSAT;

    
    // Statistics: (read-only member variable)
    //
    int64_t nbRemovedClauses,nbReducedClauses,nbDL2,nbBin,nbUn,nbReduceDB,solves, starts, decisions, rnd_decisions, propagations, conflicts,conflictsRestarts,nbstopsrestarts,nbstopsrestartssame,lastblockatrestart;
    int64_t dec_vars, clauses_literals, learnts_literals, max_literals, tot_literals;

protected:
    long curRestart;
    // Helper structures:
    //
    struct VarData { CRef reason; int level; };
    static inline VarData mkVarData(CRef cr, int l){ VarData d = {cr, l}; return d; }

    struct Watcher {
        CRef cref;
        Lit  blocker;
        Watcher(CRef cr, Lit p) : cref(cr), blocker(p) {}
        bool operator==(const Watcher& w) const { return cref == w.cref; }
        bool operator!=(const Watcher& w) const { return cref != w.cref; }
    };

    struct WatcherDeleted
    {
        const ClauseAllocator& ca;
        WatcherDeleted(const ClauseAllocator& _ca) : ca(_ca) {}
        bool operator()(const Watcher& w) const { return ca[w.cref].mark() == 1; }
    };

    struct VarOrderLt {
        const vec<double>&  activity;
        bool operator () (Var x, Var y) const { return activity[x] > activity[y]; }
        VarOrderLt(const vec<double>&  act) : activity(act) { }
    };


    // Solver state:
    //
    int lastIndexRed;
    bool                ok;               // If FALSE, the constraints are already unsatisfiable. No part of the solver state may be used!
    double              cla_inc;          // Amount to bump next clause with.
    vec<double>         activity;         // A heuristic measurement of the activity of a variable.
    double              var_inc;          // Amount to bump next variable with.
    OccLists<Lit, vec<Watcher>, WatcherDeleted>
                        watches;          // 'watches[lit]' is a list of constraints watching 'lit' (will go there if literal becomes true).
    OccLists<Lit, vec<Watcher>, WatcherDeleted>
                        watchesBin;          // 'watches[lit]' is a list of constraints watching 'lit' (will go there if literal becomes true).
    vec<CRef>           clauses;          // List of problem clauses.
    vec<CRef>           learnts;          // List of learnt clauses.

    vec<lbool>          assigns;          // The current assignments.
    vec<char>           polarity;         // The preferred polarity of each variable.
    vec<char>           decision;         // Declares if a variable is eligible for selection in the decision heuristic.
    vec<Lit>            trail;            // Assignment stack; stores all assigments made in the order they were made.
    vec<int>            nbpos;
    vec<int>            trail_lim;        // Separator indices for different decision levels in 'trail'.
    vec<VarData>        vardata;          // Stores reason and level for each variable.
    int                 qhead;            // Head of queue (as index into the trail -- no more explicit propagation queue in MiniSat).
    int                 simpDB_assigns;   // Number of top-level assignments since last execution of 'simplify()'.
    int64_t             simpDB_props;     // Remaining number of propagations that must be made before next execution of 'simplify()'.
    vec<Lit>            assumptions;      // Current set of assumptions provided to solve by the user.
    Heap<VarOrderLt>    order_heap;       // A priority queue of variables ordered with respect to the variable activity.
    double              progress_estimate;// Set by 'search()'.
    bool                remove_satisfied; // Indicates whether possibly inefficient linear scan for satisfied clauses should be performed in 'simplify'.
    vec<unsigned int> permDiff;      // permDiff[var] contains the current conflict number... Used to count the number of  LBD
    
#ifdef UPDATEVARACTIVITY
    // UPDATEVARACTIVITY trick (see competition'09 companion paper)
    vec<Lit> lastDecisionLevel; 
#endif

    ClauseAllocator     ca;

    int nbclausesbeforereduce;            // To know when it is time to reduce clause database
    
    bqueue<unsigned int> trailQueue,lbdQueue; // Bounded queues for restarts.
    float sumLBD; // used to compute the global average of LBD. Restarts...
    int sumAssumptions;


    // Temporaries (to reduce allocation overhead). Each variable is prefixed by the method in which it is
    // used, exept 'seen' wich is used in several places.
    //
    vec<char>           seen;
    vec<Lit>            analyze_stack;
    vec<Lit>            analyze_toclear;
    vec<Lit>            add_tmp;
    unsigned int  MYFLAG;


    double              max_learnts;
    double              learntsize_adjust_confl;
    int                 learntsize_adjust_cnt;

    // Resource contraints:
    //
    int64_t             conflict_budget;    // -1 means no budget.
    int64_t             propagation_budget; // -1 means no budget.
    bool                asynch_interrupt;


    // Variables added for incremental mode
    int incremental; // Use incremental SAT Solver
    int nbVarsInitialFormula; // nb VAR in formula without assumptions (incremental SAT)
    double totalTime4Sat,totalTime4Unsat;
    int nbSatCalls,nbUnsatCalls;
    vec<int> assumptionPositions,initialPositions;


    // Main internal methods:
    //
    void     insertVarOrder   (Var x);                                                 // Insert a variable in the decision order priority queue.
    Lit      pickBranchLit    ();                                                      // Return the next decision variable.
    void     newDecisionLevel ();                                                      // Begins a new decision level.
    void     uncheckedEnqueue (Lit p, CRef from = CRef_Undef);                         // Enqueue a literal. Assumes value of literal is undefined.
    bool     enqueue          (Lit p, CRef from = CRef_Undef);                         // Test if fact 'p' contradicts current state, enqueue otherwise.
    CRef     propagate        ();                                                      // Perform unit propagation. Returns possibly conflicting clause.
    void     cancelUntil      (int level);                                             // Backtrack until a certain level.
    void     analyze          (CRef confl, vec<Lit>& out_learnt, vec<Lit> & selectors, int& out_btlevel,unsigned int &nblevels,unsigned int &szWithoutSelectors);    // (bt = backtrack)
    void     analyzeFinal     (Lit p, vec<Lit>& out_conflict);                         // COULD THIS BE IMPLEMENTED BY THE ORDINARIY "analyze" BY SOME REASONABLE GENERALIZATION?
    bool     litRedundant     (Lit p, uint32_t abstract_levels);                       // (helper method for 'analyze()')
    lbool    search           (int nof_conflicts);                                     // Search for a given number of conflicts.
    lbool    solve_           ();                                                      // Main solve method (assumptions given in 'assumptions').
    void     reduceDB         ();                                                      // Reduce the set of learnt clauses.
    void     removeSatisfied  (vec<CRef>& cs);                                         // Shrink 'cs' to contain only non-satisfied clauses.
    void     rebuildOrderHeap ();

    // Maintaining Variable/Clause activity:
    //
    void     varDecayActivity ();                      // Decay all variables with the specified factor. Implemented by increasing the 'bump' value instead.
    void     varBumpActivity  (Var v, double inc);     // Increase a variable with the current 'bump' value.
    void     varBumpActivity  (Var v);                 // Increase a variable with the current 'bump' value.
    void     claDecayActivity ();                      // Decay all clauses with the specified factor. Implemented by increasing the 'bump' value instead.
    void     claBumpActivity  (Clause& c);             // Increase a clause with the current 'bump' value.

    // Operations on clauses:
    //
    void     attachClause     (CRef cr);               // Attach a clause to watcher lists.
    void     detachClause     (CRef cr, bool strict = false); // Detach a clause to watcher lists.
    void     removeClause     (CRef cr);               // Detach and free a clause.
    bool     locked           (const Clause& c) const; // Returns TRUE if a clause is a reason for some implication in the current state.
    bool     satisfied        (const Clause& c) const; // Returns TRUE if a clause is satisfied in the current state.

    unsigned int computeLBD(const vec<Lit> & lits,int end=-1);
    unsigned int computeLBD(const Clause &c);
    void minimisationWithBinaryResolution(vec<Lit> &out_learnt);

    void     relocAll         (ClauseAllocator& to);

    // Misc:
    //
    int      decisionLevel    ()      const; // Gives the current decisionlevel.
    uint32_t abstractLevel    (Var x) const; // Used to represent an abstraction of sets of decision levels.
    CRef     reason           (Var x) const;
    double   progressEstimate ()      const; // DELETE THIS ?? IT'S NOT VERY USEFUL ...
    bool     withinBudget     ()      const;
    inline bool isSelector(Var v) {return (incremental && v>nbVarsInitialFormula);}

    // Static helpers:
    //

    // Returns a random float 0 <= x < 1. Seed must never be 0.
    static inline double drand(double& seed) {
        seed *= 1389796;
        int q = (int)(seed / 2147483647);
        seed -= (double)q * 2147483647;
        return seed / 2147483647; }

    // Returns a random integer 0 <= x < size. Seed must never be 0.
    static inline int irand(double& seed, int size) {
        return (int)(drand(seed) * size); }


    // circuit-based solver
protected:
    void uncheckedEnqueue2(Lit p, CRef from = CRef_Undef);
    bool heap_rescale;

    void addJwatch( Var host, Var member, int index );
    //void delJwatch( Var member );

    struct NodeData { Lit lit0; Lit lit1; unsigned sort:30; unsigned dir:1; unsigned now:1; };
    static inline NodeData mkNodeData(){ NodeData w; w.lit0 = toLit(~0); w.lit1 = toLit(~0); w.sort = 0; w.dir = 0; w.now = 0; return w; }
    vec<NodeData> var2NodeData;
    //vec<Lit> var2FaninLits; // (~0): undefine
    vec<unsigned> var2TravId;
    vec<Lit> var2Fanout0, var2FanoutN;//, var2FanoutP;
    CRef itpc; // the interpreted clause of a gate 
    void inplace_sort( Var v );

    bool isTwoFanin  ( Var v ) const ; // this var has two fanins 
    bool isAND       ( Var v ) const { return getFaninVar0(v) < getFaninVar1(v); }
    bool isJReason   ( Var v ) const { return isTwoFanin(v) && ( l_False == value(v) || (!isAND(v) && l_Undef != value(v)) ); }
    Lit  getFaninLit0( Var v ) const { return var2NodeData[ v ].lit0; }
    Lit  getFaninLit1( Var v ) const { return var2NodeData[ v ].lit1; }
    bool getFaninC0  ( Var v ) const { return sign(getFaninLit0(v)); }
    bool getFaninC1  ( Var v ) const { return sign(getFaninLit1(v)); }
    Var  getFaninVar0( Var v ) const { return  var(getFaninLit0(v)); }
    Var  getFaninVar1( Var v ) const { return  var(getFaninLit1(v)); }
    Lit  getFaninPlt0( Var v ) const { return mkLit(getFaninVar0(v), 1 == polarity[getFaninVar0(v)]); }
    Lit  getFaninPlt1( Var v ) const { return mkLit(getFaninVar1(v), 1 == polarity[getFaninVar1(v)]); }
    Lit  maxActiveLit(Lit lit0, Lit lit1) const { return activity[var(lit0)] < activity[var(lit1)]? lit1: lit0; }

    Lit  gateJustFanin(Var v) const ; // l_Undef=satisfied, 0/1 = fanin0/fanin1 requires justify
    void gateAddJwatch(Var v,int index);
    CRef gatePropagateCheck( Var v, Var t );
    CRef gatePropagateCheckThis( Var v );
    CRef gatePropagateCheckFanout( Var v, Lit lfo );
    void setItpcSize( int sz ); // sz <= 3

    // directly call by original glucose functions 
    void updateJustActivity( Var v );
    void ResetJustData(bool fCleanMemory);
    Lit  pickJustLit( int& index );
    void justCheck();
    void pushJustQueue(Var v, int index);
    void restoreJustQueue(int level); // call with cancelUntil
    void gateClearJwatch( Var v, int backtrack_level );

    CRef gatePropagate( Lit p );

    CRef interpret( Var v, Var t );
    CRef castCRef( Lit p );  // interpret a gate into a clause 
    CRef getConfClause( CRef r );

    CRef Var2CRef( Var  v  ) const { return v  |  (1<<(sizeof(CRef)*8-1)); }
    Var  CRef2Var( CRef cr ) const { return cr & ~(1<<(sizeof(CRef)*8-1)); }
    bool isGateCRef( CRef cr ) const { return CRef_Undef != cr && 0 != (cr & (1<<(sizeof(CRef)*8-1))); }

    unsigned travId_prev, travId;

    //Heap<JustOrderLt> jheap;
    int jhead;

    struct JustKey {
        typedef double Key;
        typedef Var Data;
        typedef int Attr;
        Key  _key;
        Data _data;
        Attr _attr;
        Data data() const { return _data; }
        Key   key() const { return _key; }
        Attr attr() const { return _attr; }
        JustKey():_key(0),_data(0),_attr(0){}
        JustKey( const Key& nkey, const Data& ndata, const Attr& nattr ): _key(nkey), _data(ndata), _attr(nattr) {}
    };
    struct JustOrderLt2 {
        const Solver * pS;
        bool operator () (const JustKey& x, const JustKey& y) const {
            if( x.key() != y.key() ) return x.key() > y.key();
            if( pS->level( x.data() ) != pS->level( y.data() ) )
                return pS->level( x.data() ) < pS->level( y.data() );
            return x.data() > y.data();
        }
        JustOrderLt2(const Solver * _pS) : pS(_pS) { }
    };
    Heap2<JustOrderLt2, JustKey> jheap;
    vec<int> jlevel;
    vec<int> jnext;

    int nSkipMark;
    void loadJust_rec( Var v );
    void loadJust();
    vec<Var> vMarked;
public:
    void markTill( Var v0, int nlim );
    void markApprox( Var v0, Var v1, int nlim );

    void prelocate( int var_num );
    void setVarFaninLits( Var v, Lit lit1, Lit lit2 );
    void setVarFaninLits( int v, int lit1, int lit2 ){ setVarFaninLits( Var(v), toLit(lit1), toLit(lit2) ); }
    //void delVarFaninLits( Var v);

    void setNewRound(){ travId ++ ; }
    void markCone( Var v );
    void setJust( int njftr ){ jftr = njftr; }
    bool isRoundWatch( Var v ) const { return travId==var2TravId[v]; }
    void justReset(){ jftr = 0; reset(); }


    //const JustData& getJustData(int v) const { return jdata[v]; }
    double varActivity(int v) const { return activity[v];}
    //double justActivity(int v) const { return jdata[v].act_fanin;}
    int varPolarity(int v){ return polarity[v]? 1: 0;}
    vec<Lit> JustModel; // model obtained by justification enabled

    int justUsage() const ;
    int solveLimited( int * , int nlits );
};


//=================================================================================================
// Implementation of inline methods:

inline CRef Solver::reason(Var x) const { return vardata[x].reason; }
inline int  Solver::level (Var x) const { return vardata[x].level; }

inline void Solver::insertVarOrder(Var x) {
    #ifdef CGLUCOSE_EXP
    if (!justUsage() && !order_heap.inHeap(x) && decision[x]) order_heap.insert(x); 
    #else
    if (!order_heap.inHeap(x) && decision[x]) order_heap.insert(x); 
    #endif
}

inline void Solver::varDecayActivity() { var_inc *= (1 / var_decay); }
inline void Solver::varBumpActivity(Var v) { varBumpActivity(v, var_inc); }
inline void Solver::varBumpActivity(Var v, double inc) {
    if ( (activity[v] += inc) > 1e100 ) {
        heap_rescale = 1;
        // Rescale:
        for (int i = 0; i < nVars(); i++)
            activity[i] *= 1e-100;
        var_inc *= 1e-100; }

    // Update order_heap with respect to new activity:
    if (!justUsage() && order_heap.inHeap(v))
        order_heap.decrease(v); 
}

inline void Solver::claDecayActivity() { cla_inc *= (1 / clause_decay); }
inline void Solver::claBumpActivity (Clause& c) {
        if ( (c.activity() += cla_inc) > 1e20 ) {
            // Rescale:
            for (int i = 0; i < learnts.size(); i++)
                ca[learnts[i]].activity() *= (float)1e-20;
            cla_inc *= 1e-20; } }

inline void Solver::checkGarbage(void){ checkGarbage(garbage_frac); }
inline void Solver::checkGarbage(double gf){
    if (ca.wasted() > ca.size() * gf)
        garbageCollect();}

// NOTE: enqueue does not set the ok flag! (only public methods do)
inline bool     Solver::enqueue         (Lit p, CRef from)      { return value(p) != l_Undef ? value(p) != l_False : (uncheckedEnqueue(p, from), true); }
inline bool     Solver::addClause       (const vec<Lit>& ps)    { ps.copyTo(add_tmp); return addClause_(add_tmp); }
inline bool     Solver::addEmptyClause  ()                      { add_tmp.clear(); return addClause_(add_tmp); }
inline bool     Solver::addClause       (Lit p)                 { add_tmp.clear(); add_tmp.push(p); return addClause_(add_tmp); }
inline bool     Solver::addClause       (Lit p, Lit q)          { add_tmp.clear(); add_tmp.push(p); add_tmp.push(q); return addClause_(add_tmp); }
inline bool     Solver::addClause       (Lit p, Lit q, Lit r)   { add_tmp.clear(); add_tmp.push(p); add_tmp.push(q); add_tmp.push(r); return addClause_(add_tmp); }
inline bool     Solver::locked          (const Clause& c) const { 
   #ifdef CGLUCOSE_EXP

    if(c.size()>2) 
     return value(c[0]) == l_True && reason(var(c[0])) != CRef_Undef && !isGateCRef(reason(var(c[0]))) && ca.lea(reason(var(c[0]))) == &c; 
   return 
     (value(c[0]) == l_True && reason(var(c[0])) != CRef_Undef && !isGateCRef(reason(var(c[0]))) && ca.lea(reason(var(c[0]))) == &c)
     || 
     (value(c[1]) == l_True && reason(var(c[1])) != CRef_Undef && !isGateCRef(reason(var(c[1]))) && ca.lea(reason(var(c[1]))) == &c);

   #else
   
   if(c.size()>2) 
     return value(c[0]) == l_True && reason(var(c[0])) != CRef_Undef && ca.lea(reason(var(c[0]))) == &c; 
   return 
     (value(c[0]) == l_True && reason(var(c[0])) != CRef_Undef && ca.lea(reason(var(c[0]))) == &c)
     || 
     (value(c[1]) == l_True && reason(var(c[1])) != CRef_Undef && ca.lea(reason(var(c[1]))) == &c);

   #endif
 }
inline void     Solver::newDecisionLevel()                      {trail_lim.push(trail.size());}

inline int      Solver::decisionLevel ()      const   { return trail_lim.size(); }
inline uint32_t Solver::abstractLevel (Var x) const   { return 1 << (level(x) & 31); }
inline lbool    Solver::value         (Var x) const   { return assigns[x]; }
inline lbool    Solver::value         (Lit p) const   { return assigns[var(p)] ^ sign(p); }
inline lbool    Solver::modelValue    (Var x) const   { return model[x]; }
inline lbool    Solver::modelValue    (Lit p) const   { return model[var(p)] ^ sign(p); }
inline int      Solver::nAssigns      ()      const   { return trail.size(); }
inline int      Solver::nClauses      ()      const   { return clauses.size(); }
inline int      Solver::nLearnts      ()      const   { return learnts.size(); }
inline int      Solver::nVars         ()      const   { return vardata.size(); }
inline int      Solver::nFreeVars     ()      const   { return (int)dec_vars - (trail_lim.size() == 0 ? trail.size() : trail_lim[0]); }
inline int *    Solver::getCex        ()      const   { return (int*) &JustModel[0]; }
inline void     Solver::setPolarity   (Var v, bool b) { polarity[v] = b; }
inline void     Solver::setDecisionVar(Var v, bool b, bool use_oheap) 
{ 
    if      ( b && !decision[v]) dec_vars++;
    else if (!b &&  decision[v]) dec_vars--;

    decision[v] = b;
    if( use_oheap ) insertVarOrder(v);
}
inline void     Solver::setConfBudget(int64_t x){ conflict_budget    = conflicts    + x; }
inline void     Solver::setPropBudget(int64_t x){ propagation_budget = propagations + x; }
inline void     Solver::interrupt(){ asynch_interrupt = true; }
inline void     Solver::clearInterrupt(){ asynch_interrupt = false; }
inline void     Solver::budgetOff(){ conflict_budget = propagation_budget = -1; }
inline bool     Solver::withinBudget() const {
    return !asynch_interrupt &&
           (conflict_budget    < 0 || conflicts < (uint64_t)conflict_budget) &&
           (propagation_budget < 0 || propagations < (uint64_t)propagation_budget); }

// FIXME: after the introduction of asynchronous interrruptions the solve-versions that return a
// pure bool do not give a safe interface. Either interrupts must be possible to turn off here, or
// all calls to solve must return an 'lbool'. I'm not yet sure which I prefer.
inline bool     Solver::solve         ()                    { budgetOff(); assumptions.clear(); return solve_() == l_True; }
inline bool     Solver::solve         (Lit p)               { budgetOff(); assumptions.clear(); assumptions.push(p); return solve_() == l_True; }
inline bool     Solver::solve         (Lit p, Lit q)        { budgetOff(); assumptions.clear(); assumptions.push(p); assumptions.push(q); return solve_() == l_True; }
inline bool     Solver::solve         (Lit p, Lit q, Lit r) { budgetOff(); assumptions.clear(); assumptions.push(p); assumptions.push(q); assumptions.push(r); return solve_() == l_True; }
inline bool     Solver::solve         (const vec<Lit>& assumps){ budgetOff(); assumps.copyTo(assumptions); return solve_() == l_True; }
inline lbool    Solver::solveLimited  (const vec<Lit>& assumps){ assumps.copyTo(assumptions); return solve_(); }
inline bool     Solver::okay          ()      const   { return ok; }

inline void     Solver::toDimacs      (const char* file){ vec<Lit> as; toDimacs(file, as); }
inline void     Solver::toDimacs      (const char* file, Lit p){ vec<Lit> as; as.push(p); toDimacs(file, as); }
inline void     Solver::toDimacs      (const char* file, Lit p, Lit q){ vec<Lit> as; as.push(p); as.push(q); toDimacs(file, as); }
inline void     Solver::toDimacs      (const char* file, Lit p, Lit q, Lit r){ vec<Lit> as; as.push(p); as.push(q); as.push(r); toDimacs(file, as); }

inline void     Solver::addVar(Var v) { while (v >= nVars()) newVar(); }

inline void     Solver::sat_solver_set_var_fanin_lit(int v, int lit0, int lit1) { setVarFaninLits( Var(v), toLit(lit0), toLit(lit1) ); }
inline void     Solver::sat_solver_start_new_round()  { setNewRound(); }
inline void     Solver::sat_solver_mark_cone(int v) { markCone(v); }
inline void     Solver::sat_solver_set_jftr( int njftr ){ setJust(njftr); }
inline int      Solver::sat_solver_jftr(){ return jftr; }
inline void     Solver::sat_solver_reset(){ justReset();  }
inline int      Solver::solveLimited( int * lit0, int nlits ){
    assumptions.clear();
    for(int i = 0; i < nlits; i ++)
        assumptions.push(toLit(lit0[i]));
    lbool res = solve_();
    return res == l_True ? 1 : (res == l_False ? -1 : 0);
}
//=================================================================================================
// Debug etc:


inline void Solver::printLit(Lit l)
{
    printf("%s%d:%c", sign(l) ? "-" : "", var(l)+1, value(l) == l_True ? '1' : (value(l) == l_False ? '0' : 'X'));
}


inline void Solver::printClause(CRef cr)
{
  Clause &c = ca[cr];
    for (int i = 0; i < c.size(); i++){
        printLit(c[i]);
        printf(" ");
    }
}

inline void Solver::printInitialClause(CRef cr)
{
  Clause &c = ca[cr];
    for (int i = 0; i < c.size(); i++){
      if(!isSelector(var(c[i]))) {
    printLit(c[i]);
        printf(" ");
      }
    }
}


//=================================================================================================
}


ABC_NAMESPACE_CXX_HEADER_END

#include "sat/glucose2/CGlucoseCore.h"

#endif
