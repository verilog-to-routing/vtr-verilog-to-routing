/**CFile****************************************************************

  FileName    [AbcGlucose.cpp]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT solver Glucose 3.0 by Gilles Audemard and Laurent Simon.]

  Synopsis    [Interface to Glucose.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 6, 2017.]

  Revision    [$Id: AbcGlucose.cpp,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sat/glucose/System.h"
#include "sat/glucose/ParseUtils.h"
#include "sat/glucose/Options.h"
#include "sat/glucose/Dimacs.h"
#include "sat/glucose/SimpSolver.h"

#include "sat/glucose/AbcGlucose.h"

#include "base/abc/abc.h"
#include "aig/gia/gia.h"
#include "sat/cnf/cnf.h"
#include "misc/extra/extra.h"

ABC_NAMESPACE_IMPL_START

using namespace Gluco;

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define USE_SIMP_SOLVER 1

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

#ifdef USE_SIMP_SOLVER
    
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gluco::SimpSolver * glucose_solver_start()
{
    SimpSolver * S = new SimpSolver;
    S->setIncrementalMode();
    return S;
}

void glucose_solver_stop(Gluco::SimpSolver* S)
{
    delete S;
}

void glucose_solver_reset(Gluco::SimpSolver* S)
{
    S->reset();
}

int glucose_solver_addclause(Gluco::SimpSolver* S, int * plits, int nlits)
{
    vec<Lit> lits;
    for ( int i = 0; i < nlits; i++,plits++)
    {
        // note: Glucose uses the same var->lit conventiaon as ABC
        while ((*plits)/2 >= S->nVars()) S->newVar();
        assert((*plits)/2 < S->nVars()); // NOTE: since we explicitely use new function bmc_add_var
        Lit p;
        p.x = *plits;
        lits.push(p);
    }
    return S->addClause(lits); // returns 0 if the problem is UNSAT
}

void glucose_solver_setcallback(Gluco::SimpSolver* S, void * pman, int(*pfunc)(void*, int, int*))
{
    S->pCnfMan = pman;
    S->pCnfFunc = pfunc;
    S->nCallConfl = 1000;
}

int glucose_solver_solve(Gluco::SimpSolver* S, int * plits, int nlits)
{
    vec<Lit> lits;
    for (int i=0;i<nlits;i++,plits++)
    {
        Lit p;
        p.x = *plits;
        lits.push(p);
    }
    Gluco::lbool res = S->solveLimited(lits, 0);
    return (res == l_True ? 1 : res == l_False ? -1 : 0);
}

int glucose_solver_addvar(Gluco::SimpSolver* S)
{
    S->newVar();
    return S->nVars() - 1;
}

int glucose_solver_read_cex_varvalue(Gluco::SimpSolver* S, int ivar)
{
    return S->model[ivar] == l_True;
}

void glucose_solver_setstop(Gluco::SimpSolver* S, int * pstop)
{
    S->pstop = pstop;
}


/**Function*************************************************************

  Synopsis    [Wrapper APIs to calling from ABC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bmcg_sat_solver * bmcg_sat_solver_start() 
{
    return (bmcg_sat_solver *)glucose_solver_start();
}
void bmcg_sat_solver_stop(bmcg_sat_solver* s)
{
    glucose_solver_stop((Gluco::SimpSolver*)s);
}
void bmcg_sat_solver_reset(bmcg_sat_solver* s)
{
    glucose_solver_reset((Gluco::SimpSolver*)s);
}


int bmcg_sat_solver_addclause(bmcg_sat_solver* s, int * plits, int nlits)
{
    return glucose_solver_addclause((Gluco::SimpSolver*)s,plits,nlits);
}

void bmcg_sat_solver_setcallback(bmcg_sat_solver* s, void * pman, int(*pfunc)(void*, int, int*))
{
    glucose_solver_setcallback((Gluco::SimpSolver*)s,pman,pfunc);
}

int bmcg_sat_solver_solve(bmcg_sat_solver* s, int * plits, int nlits)
{
    return glucose_solver_solve((Gluco::SimpSolver*)s,plits,nlits);
}

int bmcg_sat_solver_final(bmcg_sat_solver* s, int ** ppArray)
{
    *ppArray = (int *)(Lit *)((Gluco::SimpSolver*)s)->conflict;
    return ((Gluco::SimpSolver*)s)->conflict.size();
}

int bmcg_sat_solver_addvar(bmcg_sat_solver* s)
{
    return glucose_solver_addvar((Gluco::SimpSolver*)s);
}

void bmcg_sat_solver_set_nvars( bmcg_sat_solver* s, int nvars )
{
    int i;
    for ( i = bmcg_sat_solver_varnum(s); i < nvars; i++ )
        bmcg_sat_solver_addvar(s);
}

int bmcg_sat_solver_eliminate( bmcg_sat_solver* s, int turn_off_elim )
{
//    return 1; 
    return ((Gluco::SimpSolver*)s)->eliminate(turn_off_elim != 0);
}

int bmcg_sat_solver_var_is_elim( bmcg_sat_solver* s, int v )
{
//    return 0; 
    return ((Gluco::SimpSolver*)s)->isEliminated(v);
}

void bmcg_sat_solver_var_set_frozen( bmcg_sat_solver* s, int v, int freeze )
{
    ((Gluco::SimpSolver*)s)->setFrozen(v, freeze != 0);
}

int bmcg_sat_solver_elim_varnum(bmcg_sat_solver* s)
{
//    return 0; 
    return ((Gluco::SimpSolver*)s)->eliminated_vars;
}

int bmcg_sat_solver_read_cex_varvalue(bmcg_sat_solver* s, int ivar)
{
    return glucose_solver_read_cex_varvalue((Gluco::SimpSolver*)s, ivar);
}

void bmcg_sat_solver_set_stop(bmcg_sat_solver* s, int * pstop)
{
    glucose_solver_setstop((Gluco::SimpSolver*)s, pstop);
}

abctime bmcg_sat_solver_set_runtime_limit(bmcg_sat_solver* s, abctime Limit)
{
    abctime nRuntimeLimit = ((Gluco::SimpSolver*)s)->nRuntimeLimit;
    ((Gluco::SimpSolver*)s)->nRuntimeLimit = Limit;
    return nRuntimeLimit;
}

void bmcg_sat_solver_set_conflict_budget(bmcg_sat_solver* s, int Limit)
{
    if ( Limit > 0 ) 
        ((Gluco::SimpSolver*)s)->setConfBudget( (int64_t)Limit );
    else 
        ((Gluco::SimpSolver*)s)->budgetOff();
}

int bmcg_sat_solver_varnum(bmcg_sat_solver* s)
{
    return ((Gluco::SimpSolver*)s)->nVars();
}
int bmcg_sat_solver_clausenum(bmcg_sat_solver* s)
{
    return ((Gluco::SimpSolver*)s)->nClauses();
}
int bmcg_sat_solver_learntnum(bmcg_sat_solver* s)
{
    return ((Gluco::SimpSolver*)s)->nLearnts();
}
int bmcg_sat_solver_conflictnum(bmcg_sat_solver* s)
{
    return ((Gluco::SimpSolver*)s)->conflicts;
}

int bmcg_sat_solver_minimize_assumptions( bmcg_sat_solver * s, int * plits, int nlits, int pivot )
{
    vec<int>*array = &((Gluco::SimpSolver*)s)->user_vec;
    int i, nlitsL, nlitsR, nresL, nresR, status;
    assert( pivot >= 0 );
//    assert( nlits - pivot >= 2 );
    assert( nlits - pivot >= 1 );
    if ( nlits - pivot == 1 )
    {
        // since the problem is UNSAT, we try to solve it without assuming the last literal
        // if the result is UNSAT, the last literal can be dropped; otherwise, it is needed
        status = bmcg_sat_solver_solve( s, plits, pivot );
        return status != GLUCOSE_UNSAT; // return 1 if the problem is not UNSAT
    }
    assert( nlits - pivot >= 2 );
    nlitsL = (nlits - pivot) / 2;
    nlitsR = (nlits - pivot) - nlitsL;
    assert( nlitsL + nlitsR == nlits - pivot );
    // solve with these assumptions
    status = bmcg_sat_solver_solve( s, plits, pivot + nlitsL );
    if ( status == GLUCOSE_UNSAT ) // these are enough
        return bmcg_sat_solver_minimize_assumptions( s, plits, pivot + nlitsL, pivot );
    // these are not enough
    // solve for the right lits
//    nResL = nLitsR == 1 ? 1 : sat_solver_minimize_assumptions( s, pLits + nLitsL, nLitsR, nConfLimit );
    nresL = nlitsR == 1 ? 1 : bmcg_sat_solver_minimize_assumptions( s, plits, nlits, pivot + nlitsL );
    // swap literals
    array->clear();
    for ( i = 0; i < nlitsL; i++ )
        array->push(plits[pivot + i]);
    for ( i = 0; i < nresL; i++ )
        plits[pivot + i] = plits[pivot + nlitsL + i];
    for ( i = 0; i < nlitsL; i++ )
        plits[pivot + nresL + i] = (*array)[i];
    // solve with these assumptions
    status = bmcg_sat_solver_solve( s, plits, pivot + nresL );
    if ( status == GLUCOSE_UNSAT ) // these are enough
        return nresL;
    // solve for the left lits
//    nResR = nLitsL == 1 ? 1 : sat_solver_minimize_assumptions( s, pLits + nResL, nLitsL, nConfLimit );
    nresR = nlitsL == 1 ? 1 : bmcg_sat_solver_minimize_assumptions( s, plits, pivot + nresL + nlitsL, pivot + nresL );
    return nresL + nresR;
}

int bmcg_sat_solver_add_and( bmcg_sat_solver * s, int iVar, int iVar0, int iVar1, int fCompl0, int fCompl1, int fCompl )
{
    int Lits[3];

    Lits[0] = Abc_Var2Lit( iVar, !fCompl );
    Lits[1] = Abc_Var2Lit( iVar0, fCompl0 );
    if ( !bmcg_sat_solver_addclause( s, Lits, 2 ) )
        return 0;

    Lits[0] = Abc_Var2Lit( iVar, !fCompl );
    Lits[1] = Abc_Var2Lit( iVar1, fCompl1 );
    if ( !bmcg_sat_solver_addclause( s, Lits, 2 ) )
        return 0;

    Lits[0] = Abc_Var2Lit( iVar,   fCompl );
    Lits[1] = Abc_Var2Lit( iVar0, !fCompl0 );
    Lits[2] = Abc_Var2Lit( iVar1, !fCompl1 );
    if ( !bmcg_sat_solver_addclause( s, Lits, 3 ) )
        return 0;

    return 1;
}

#else

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gluco::Solver * glucose_solver_start()
{
    Solver * S = new Solver;
    S->setIncrementalMode();
    return S;
}

void glucose_solver_stop(Gluco::Solver* S)
{
    delete S;
}

int glucose_solver_addclause(Gluco::Solver* S, int * plits, int nlits)
{
    vec<Lit> lits;
    for ( int i = 0; i < nlits; i++,plits++)
    {
        // note: Glucose uses the same var->lit conventiaon as ABC
        while ((*plits)/2 >= S->nVars()) S->newVar();
        assert((*plits)/2 < S->nVars()); // NOTE: since we explicitely use new function bmc_add_var
        Lit p;
        p.x = *plits;
        lits.push(p);
    }
    return S->addClause(lits); // returns 0 if the problem is UNSAT
}

void glucose_solver_setcallback(Gluco::Solver* S, void * pman, int(*pfunc)(void*, int, int*))
{
    S->pCnfMan = pman;
    S->pCnfFunc = pfunc;
    S->nCallConfl = 1000;
}

int glucose_solver_solve(Gluco::Solver* S, int * plits, int nlits)
{
    vec<Lit> lits;
    for (int i=0;i<nlits;i++,plits++)
    {
        Lit p;
        p.x = *plits;
        lits.push(p);
    }
    Gluco::lbool res = S->solveLimited(lits);
    return (res == l_True ? 1 : res == l_False ? -1 : 0);
}

int glucose_solver_addvar(Gluco::Solver* S)
{
    S->newVar();
    return S->nVars() - 1;
}

int glucose_solver_read_cex_varvalue(Gluco::Solver* S, int ivar)
{
    return S->model[ivar] == l_True;
}

void glucose_solver_setstop(Gluco::Solver* S, int * pstop)
{
    S->pstop = pstop;
}


/**Function*************************************************************

  Synopsis    [Wrapper APIs to calling from ABC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bmcg_sat_solver * bmcg_sat_solver_start() 
{
    return (bmcg_sat_solver *)glucose_solver_start();
}
void bmcg_sat_solver_stop(bmcg_sat_solver* s)
{
    glucose_solver_stop((Gluco::Solver*)s);
}

int bmcg_sat_solver_addclause(bmcg_sat_solver* s, int * plits, int nlits)
{
    return glucose_solver_addclause((Gluco::Solver*)s,plits,nlits);
}

void bmcg_sat_solver_setcallback(bmcg_sat_solver* s, void * pman, int(*pfunc)(void*, int, int*))
{
    glucose_solver_setcallback((Gluco::Solver*)s,pman,pfunc);
}

int bmcg_sat_solver_solve(bmcg_sat_solver* s, int * plits, int nlits)
{
    return glucose_solver_solve((Gluco::Solver*)s,plits,nlits);
}

int bmcg_sat_solver_final(bmcg_sat_solver* s, int ** ppArray)
{
    *ppArray = (int *)(Lit *)((Gluco::Solver*)s)->conflict;
    return ((Gluco::Solver*)s)->conflict.size();
}

int bmcg_sat_solver_addvar(bmcg_sat_solver* s)
{
    return glucose_solver_addvar((Gluco::Solver*)s);
}

void bmcg_sat_solver_set_nvars( bmcg_sat_solver* s, int nvars )
{
    int i;
    for ( i = bmcg_sat_solver_varnum(s); i < nvars; i++ )
        bmcg_sat_solver_addvar(s);
}

int bmcg_sat_solver_eliminate( bmcg_sat_solver* s, int turn_off_elim )
{
    return 1; 
//    return ((Gluco::SimpSolver*)s)->eliminate(turn_off_elim != 0);
}

int bmcg_sat_solver_var_is_elim( bmcg_sat_solver* s, int v )
{
    return 0; 
//    return ((Gluco::SimpSolver*)s)->isEliminated(v);
}

void bmcg_sat_solver_var_set_frozen( bmcg_sat_solver* s, int v, int freeze )
{
//    ((Gluco::SimpSolver*)s)->setFrozen(v, freeze);
}

int bmcg_sat_solver_elim_varnum(bmcg_sat_solver* s)
{
    return 0;
//    return ((Gluco::SimpSolver*)s)->eliminated_vars;
}

int bmcg_sat_solver_read_cex_varvalue(bmcg_sat_solver* s, int ivar)
{
    return glucose_solver_read_cex_varvalue((Gluco::Solver*)s, ivar);
}

void bmcg_sat_solver_set_stop(bmcg_sat_solver* s, int * pstop)
{
    glucose_solver_setstop((Gluco::Solver*)s, pstop);
}

abctime bmcg_sat_solver_set_runtime_limit(bmcg_sat_solver* s, abctime Limit)
{
    abctime nRuntimeLimit = ((Gluco::Solver*)s)->nRuntimeLimit;
    ((Gluco::Solver*)s)->nRuntimeLimit = Limit;
    return nRuntimeLimit;
}

void bmcg_sat_solver_set_conflict_budget(bmcg_sat_solver* s, int Limit)
{
    if ( Limit > 0 ) 
        ((Gluco::Solver*)s)->setConfBudget( (int64_t)Limit );
    else 
        ((Gluco::Solver*)s)->budgetOff();
}

int bmcg_sat_solver_varnum(bmcg_sat_solver* s)
{
    return ((Gluco::Solver*)s)->nVars();
}
int bmcg_sat_solver_clausenum(bmcg_sat_solver* s)
{
    return ((Gluco::Solver*)s)->nClauses();
}
int bmcg_sat_solver_learntnum(bmcg_sat_solver* s)
{
    return ((Gluco::Solver*)s)->nLearnts();
}
int bmcg_sat_solver_conflictnum(bmcg_sat_solver* s)
{
    return ((Gluco::Solver*)s)->conflicts;
}

int bmcg_sat_solver_minimize_assumptions( bmcg_sat_solver * s, int * plits, int nlits, int pivot )
{
    vec<int>*array = &((Gluco::Solver*)s)->user_vec;
    int i, nlitsL, nlitsR, nresL, nresR, status;
    assert( pivot >= 0 );
//    assert( nlits - pivot >= 2 );
    assert( nlits - pivot >= 1 );
    if ( nlits - pivot == 1 )
    {
        // since the problem is UNSAT, we try to solve it without assuming the last literal
        // if the result is UNSAT, the last literal can be dropped; otherwise, it is needed
        status = bmcg_sat_solver_solve( s, plits, pivot );
        return status != GLUCOSE_UNSAT; // return 1 if the problem is not UNSAT
    }
    assert( nlits - pivot >= 2 );
    nlitsL = (nlits - pivot) / 2;
    nlitsR = (nlits - pivot) - nlitsL;
    assert( nlitsL + nlitsR == nlits - pivot );
    // solve with these assumptions
    status = bmcg_sat_solver_solve( s, plits, pivot + nlitsL );
    if ( status == GLUCOSE_UNSAT ) // these are enough
        return bmcg_sat_solver_minimize_assumptions( s, plits, pivot + nlitsL, pivot );
    // these are not enough
    // solve for the right lits
//    nResL = nLitsR == 1 ? 1 : sat_solver_minimize_assumptions( s, pLits + nLitsL, nLitsR, nConfLimit );
    nresL = nlitsR == 1 ? 1 : bmcg_sat_solver_minimize_assumptions( s, plits, nlits, pivot + nlitsL );
    // swap literals
    array->clear();
    for ( i = 0; i < nlitsL; i++ )
        array->push(plits[pivot + i]);
    for ( i = 0; i < nresL; i++ )
        plits[pivot + i] = plits[pivot + nlitsL + i];
    for ( i = 0; i < nlitsL; i++ )
        plits[pivot + nresL + i] = (*array)[i];
    // solve with these assumptions
    status = bmcg_sat_solver_solve( s, plits, pivot + nresL );
    if ( status == GLUCOSE_UNSAT ) // these are enough
        return nresL;
    // solve for the left lits
//    nResR = nLitsL == 1 ? 1 : sat_solver_minimize_assumptions( s, pLits + nResL, nLitsL, nConfLimit );
    nresR = nlitsL == 1 ? 1 : bmcg_sat_solver_minimize_assumptions( s, plits, pivot + nresL + nlitsL, pivot + nresL );
    return nresL + nresR;
}

#endif


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void glucose_print_stats(SimpSolver& s, abctime clk)
{
    double cpu_time = (double)(unsigned)clk / CLOCKS_PER_SEC;
    double mem_used = memUsed();
    printf("c restarts              : %d (%d conflicts on average)\n",         (int)s.starts, s.starts > 0 ? (int)(s.conflicts/s.starts) : 0);
    printf("c blocked restarts      : %d (multiple: %d) \n",                   (int)s.nbstopsrestarts, (int)s.nbstopsrestartssame);
    printf("c last block at restart : %d\n",                                   (int)s.lastblockatrestart);
    printf("c nb ReduceDB           : %-12d\n",                                (int)s.nbReduceDB);
    printf("c nb removed Clauses    : %-12d\n",                                (int)s.nbRemovedClauses);
    printf("c nb learnts DL2        : %-12d\n",                                (int)s.nbDL2);
    printf("c nb learnts size 2     : %-12d\n",                                (int)s.nbBin);
    printf("c nb learnts size 1     : %-12d\n",                                (int)s.nbUn);
    printf("c conflicts             : %-12d  (%.0f /sec)\n",                   (int)s.conflicts,    s.conflicts   /cpu_time);
    printf("c decisions             : %-12d  (%4.2f %% random) (%.0f /sec)\n", (int)s.decisions,    (float)s.rnd_decisions*100 / (float)s.decisions, s.decisions   /cpu_time);
    printf("c propagations          : %-12d  (%.0f /sec)\n",                   (int)s.propagations, s.propagations/cpu_time);
    printf("c conflict literals     : %-12d  (%4.2f %% deleted)\n",            (int)s.tot_literals, (s.max_literals - s.tot_literals)*100 / (double)s.max_literals);
    printf("c nb reduced Clauses    : %-12d\n", (int)s.nbReducedClauses);
    if (mem_used != 0) printf("Memory used           : %.2f MB\n", mem_used);
    //printf("c CPU time              : %.2f sec\n", cpu_time);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Glucose_ReadDimacs( char * pFileName, SimpSolver& s )
{
    vec<Lit> * lits = &s.user_lits;
    char * pBuffer = Extra_FileReadContents( pFileName );
    char * pTemp; int fComp, Var, VarMax = 0;
    lits->clear();
    for ( pTemp = pBuffer; *pTemp; pTemp++ )
    {
        if ( *pTemp == 'c' || *pTemp == 'p' ) {
            while ( *pTemp != '\n' )
                pTemp++;
            continue;
        }
        while ( *pTemp == ' ' || *pTemp == '\t' || *pTemp == '\r' || *pTemp == '\n' )
            pTemp++;
        fComp = 0;
        if ( *pTemp == '-' )
            fComp = 1, pTemp++;
        if ( *pTemp == '+' )
            pTemp++;
        Var = atoi( pTemp );
        if ( Var == 0 ) {
            if ( lits->size() > 0 ) {
                s.addVar( VarMax );
                s.addClause(*lits);
                lits->clear();
            }
        }
        else {
            Var--;
            VarMax = Abc_MaxInt( VarMax, Var );
            lits->push( toLit(Abc_Var2Lit(Var, fComp)) );
        }
        while ( *pTemp >= '0' && *pTemp <= '9' )
            pTemp++;
    }
    ABC_FREE( pBuffer );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Glucose_SolveCnf( char * pFileName, Glucose_Pars * pPars )
{
    abctime clk = Abc_Clock();

    SimpSolver  S;
    S.verbosity = pPars->verb;
    S.setConfBudget( pPars->nConfls > 0 ? (int64_t)pPars->nConfls : -1 );

//    gzFile in = gzopen(pFilename, "rb");
//    parse_DIMACS(in, S);
//    gzclose(in);
    Glucose_ReadDimacs( pFileName, S );

    if ( pPars->verb )
    {
        printf("c ============================[ Problem Statistics ]=============================\n");
        printf("c |                                                                             |\n");
        printf("c |  Number of variables:  %12d                                         |\n", S.nVars());
        printf("c |  Number of clauses:    %12d                                         |\n", S.nClauses());
    }
    
    if ( pPars->pre ) 
    {
        S.eliminate(true);
        printf( "c Simplication removed %d variables and %d clauses.  ", S.eliminated_vars, S.eliminated_clauses );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    }

    vec<Lit> dummy;
    lbool ret = S.solveLimited(dummy, 0);
    if ( pPars->verb ) glucose_print_stats(S, Abc_Clock() - clk);
    printf(ret == l_True ? "SATISFIABLE" : ret == l_False ? "UNSATISFIABLE" : "INDETERMINATE");
    Abc_PrintTime( 1, "      Time", Abc_Clock() - clk );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Glucose_SolverFromAig( Gia_Man_t * p, SimpSolver& s )
{
    abctime clk = Abc_Clock();
    vec<Lit> * lits = &s.user_lits;
    Cnf_Dat_t * pCnf = (Cnf_Dat_t *)Mf_ManGenerateCnf( p, 8 /*nLutSize*/, 0 /*fCnfObjIds*/, 1/*fAddOrCla*/, 0, 0/*verbose*/ );
    for ( int i = 0; i < pCnf->nClauses; i++ )
    {
        lits->clear();
        for ( int * pLit = pCnf->pClauses[i]; pLit < pCnf->pClauses[i+1]; pLit++ )
            lits->push( toLit(*pLit) ), s.addVar( *pLit >> 1 );
        s.addClause(*lits);
    }
    Vec_Int_t * vCnfIds = Vec_IntAllocArrayCopy(pCnf->pVarNums,pCnf->nVars);
    printf( "CNF stats: Vars = %6d. Clauses = %7d. Literals = %8d. ", pCnf->nVars, pCnf->nClauses, pCnf->nLiterals );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    Cnf_DataFree(pCnf);
    return vCnfIds;
}

// procedure below does not work because glucose_solver_addclause() expects Solver
Vec_Int_t * Glucose_SolverFromAig2( Gia_Man_t * p, SimpSolver& S ) 
{
    Cnf_Dat_t * pCnf = (Cnf_Dat_t *)Mf_ManGenerateCnf( p, 8 /*nLutSize*/, 0 /*fCnfObjIds*/, 1/*fAddOrCla*/, 0, 0/*verbose*/ );
    for ( int i = 0; i < pCnf->nClauses; i++ )
        if ( !glucose_solver_addclause( &S, pCnf->pClauses[i], pCnf->pClauses[i+1]-pCnf->pClauses[i] ) )
            assert( 0 );
    Vec_Int_t * vCnfIds = Vec_IntAllocArrayCopy(pCnf->pVarNums,pCnf->nVars);
    //printf( "CNF stats: Vars = %6d. Clauses = %7d. Literals = %8d. ", pCnf->nVars, pCnf->nClauses, pCnf->nLiterals );
    //Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    Cnf_DataFree(pCnf);
    return vCnfIds;
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Str_t * Glucose_GenerateCubes( bmcg_sat_solver * pSat[2], Vec_Int_t * vCiSatVars, Vec_Int_t * vVar2Index, int CubeLimit )
{
    int fCreatePrime = 1;
    int nCubes, nSupp = Vec_IntSize(vCiSatVars);
    Vec_Str_t * vSop  = Vec_StrAlloc( 1000 );
    Vec_Int_t * vLits = Vec_IntAlloc( nSupp );
    Vec_Str_t * vCube = Vec_StrAlloc( nSupp + 4 );
    Vec_StrFill( vCube, nSupp, '-' );
    Vec_StrPrintF( vCube, " 1\n\0" );
    for ( nCubes = 0; !CubeLimit || nCubes < CubeLimit; nCubes++ )
    {
        int * pFinal, nFinal, iVar, i, k = 0;
        // generate onset minterm
        int status = bmcg_sat_solver_solve( pSat[1], NULL, 0 );
        if ( status == GLUCOSE_UNSAT )
            break;
        assert( status == GLUCOSE_SAT );
        Vec_IntClear( vLits );
        Vec_IntForEachEntry( vCiSatVars, iVar, i )
            Vec_IntPush( vLits, Abc_Var2Lit(iVar, !bmcg_sat_solver_read_cex_varvalue(pSat[1], iVar)) );
        // expand against offset
        if ( fCreatePrime )
        {
            nFinal = bmcg_sat_solver_minimize_assumptions( pSat[0], Vec_IntArray(vLits), Vec_IntSize(vLits), 0 );
            Vec_IntShrink( vLits, nFinal );
            pFinal = Vec_IntArray( vLits );
            for ( i = 0; i < nFinal; i++ )
                pFinal[i] = Abc_LitNot(pFinal[i]);
        }
        else
        {
            status = bmcg_sat_solver_solve( pSat[0], Vec_IntArray(vLits), Vec_IntSize(vLits) );
            assert( status == GLUCOSE_UNSAT );
            nFinal = bmcg_sat_solver_final( pSat[0], &pFinal );
        }
        // print cube
        Vec_StrFill( vCube, nSupp, '-' );
        for ( i = 0; i < nFinal; i++ )
        {
            int Index = Vec_IntEntry(vVar2Index, Abc_Lit2Var(pFinal[i]));
            if ( Index == -1 )
                continue;
            pFinal[k++] = pFinal[i];
            assert( Index >= 0 && Index < nSupp );
            Vec_StrWriteEntry( vCube, Index, (char)('0' + Abc_LitIsCompl(pFinal[i])) );
        }
        nFinal = k;
        Vec_StrAppend( vSop, Vec_StrArray(vCube) );
        //printf( "%s\n", Vec_StrArray(vCube) );
        // add blocking clause
        if ( !bmcg_sat_solver_addclause( pSat[1], pFinal, nFinal ) )
            break;
    }
    Vec_IntFree( vLits );
    Vec_StrFree( vCube );
    Vec_StrPush( vSop, '\0' );
    return vSop;
}
Vec_Str_t * bmcg_sat_solver_sop( Gia_Man_t * p, int CubeLimit )
{
    bmcg_sat_solver * pSat[2] = { bmcg_sat_solver_start(), bmcg_sat_solver_start() };

    // generate CNF for the on-set and off-set
    Cnf_Dat_t * pCnf = (Cnf_Dat_t *)Mf_ManGenerateCnf( p, 8 /*nLutSize*/, 0 /*fCnfObjIds*/, 0/*fAddOrCla*/, 0, 0/*verbose*/ );
    int i, n, nVars = Gia_ManCiNum(p), Lit;//, Count = 0;
    int iFirstVar = pCnf->nVars - nVars;
    assert( Gia_ManCoNum(p) == 1 );
    for ( n = 0; n < 2; n++ )
    {
        bmcg_sat_solver_set_nvars( pSat[n], pCnf->nVars );
        Lit = Abc_Var2Lit( 1, !n );  // output variable is 1
        for ( i = 0; i < pCnf->nClauses; i++ )
            if ( !bmcg_sat_solver_addclause( pSat[n], pCnf->pClauses[i], pCnf->pClauses[i+1]-pCnf->pClauses[i] ) )
                assert( 0 );
        if ( !bmcg_sat_solver_addclause( pSat[n], &Lit, 1 ) )
        {
            Vec_Str_t * vSop = Vec_StrAlloc( 10 );
            Vec_StrPrintF( vSop, " %d\n\0", !n );
            Cnf_DataFree( pCnf );
            return vSop;
        }
    }
    Cnf_DataFree( pCnf );

    // collect cube vars and map SAT vars into them
    Vec_Int_t * vVars = Vec_IntAlloc( 100 ); 
    Vec_Int_t * vVarMap = Vec_IntStartFull( iFirstVar + nVars ); 
    for ( i = 0; i < nVars; i++ )
    {
        Vec_IntPush( vVars, iFirstVar+i );
        Vec_IntWriteEntry( vVarMap, iFirstVar+i, i );
    }

    Vec_Str_t * vSop = Glucose_GenerateCubes( pSat, vVars, vVarMap, CubeLimit );
    Vec_IntFree( vVarMap );
    Vec_IntFree( vVars );

    bmcg_sat_solver_stop( pSat[0] );
    bmcg_sat_solver_stop( pSat[1] );
    return vSop;
}
void bmcg_sat_solver_print_sop( Gia_Man_t * p )
{
    Vec_Str_t * vSop = bmcg_sat_solver_sop( p, 0 );
    printf( "%s", Vec_StrArray(vSop) );
    Vec_StrFree( vSop );
}
void bmcg_sat_solver_print_sop_lit( Gia_Man_t * p, int Lit )
{
    Vec_Int_t * vCisUsed = Vec_IntAlloc( 100 );
    int i, ObjId, iNode = Abc_Lit2Var( Lit );
    Gia_ManCollectCis( p, &iNode, 1, vCisUsed );
    Vec_IntSort( vCisUsed, 0 );
    Vec_IntForEachEntry( vCisUsed, ObjId, i )
        Vec_IntWriteEntry( vCisUsed, i, Gia_ManIdToCioId(p, ObjId) );
    Vec_IntPrint( vCisUsed );
    Gia_Man_t * pNew = Gia_ManDupConeSupp( p, Lit, vCisUsed );
    Vec_IntFree( vCisUsed );
    bmcg_sat_solver_print_sop( pNew );
    Gia_ManStop( pNew );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Computing d-literals.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
#define Gia_CubeForEachVar( pCube, Value, i )                                                      \
    for ( i = 0; (pCube[i] != ' ') && (Value = pCube[i]); i++ )           
#define Gia_SopForEachCube( pSop, nFanins, pCube )                                                 \
    for ( pCube = (pSop); *pCube; pCube += (nFanins) + 3 )

void bmcg_sat_generate_dvars( Vec_Int_t * vCiVars, Vec_Str_t * vSop, Vec_Int_t * vDLits )
{
    int i, Lit, Counter, nCubes = 0;
    char Value, * pCube, * pSop = Vec_StrArray( vSop );
    Vec_Int_t * vCounts = Vec_IntStart( 2*Vec_IntSize(vCiVars) );
    Vec_IntClear( vDLits );
    Gia_SopForEachCube( pSop, Vec_IntSize(vCiVars), pCube )
    {
        nCubes++;
        Gia_CubeForEachVar( pCube, Value, i )
        {
            if ( Value == '1' )
                Vec_IntAddToEntry( vCounts, 2*i, 1 );
            else if ( Value == '0' )
                Vec_IntAddToEntry( vCounts, 2*i+1, 1 );
        }
    }
    Vec_IntForEachEntry( vCounts, Counter, Lit )
        if ( Counter == nCubes )
            Vec_IntPush( vDLits, Abc_Var2Lit(Vec_IntEntry(vCiVars, Abc_Lit2Var(Lit)), Abc_LitIsCompl(Lit)) );
    Vec_IntSort( vDLits, 0 );
    Vec_IntFree( vCounts );
}

/**Function*************************************************************

  Synopsis    [Performs SAT-based quantification.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int bmcg_sat_solver_quantify2( Gia_Man_t * p, int iLit, int fHash, int(*pFuncCiToKeep)(void *, int), void * pData, Vec_Int_t * vDLits )
{
    int fSynthesize = 0;
    extern Gia_Man_t * Abc_SopSynthesizeOne( char * pSop, int fClp );
    Gia_Man_t * pMan, * pNew, * pTemp;  Vec_Str_t * vSop;
    int i, CiId, ObjId, Res, nCubes = 0, nNodes, Count = 0, iNode = Abc_Lit2Var(iLit);
    Vec_Int_t * vCisUsed = Vec_IntAlloc( 100 );
    Gia_ManCollectCis( p, &iNode, 1, vCisUsed );
    Vec_IntSort( vCisUsed, 0 );
    if ( vDLits ) Vec_IntClear( vDLits );
    if ( iLit < 2 )
        return iLit;
    // remap into CI Ids
    Vec_IntForEachEntry( vCisUsed, ObjId, i )
        Vec_IntWriteEntry( vCisUsed, i, Gia_ManIdToCioId(p, ObjId) );
    // duplicate cone
    pNew = Gia_ManDupConeSupp( p, iLit, vCisUsed );
    assert( Gia_ManCiNum(pNew) == Vec_IntSize(vCisUsed) );
    nNodes = Gia_ManAndNum(pNew);

    // perform quantification one CI at a time
    assert( pFuncCiToKeep );
    Vec_IntForEachEntry( vCisUsed, CiId, i )
        if ( !pFuncCiToKeep( pData, CiId ) )
        {
            //printf( "Quantifying %d.\n", CiId );
            pNew = Gia_ManDupExist( pTemp = pNew, i );
            Gia_ManStop( pTemp );
            Count++;
        }
    if ( Gia_ManPoIsConst(pNew, 0) )
    {
        int RetValue = Gia_ManPoIsConst1(pNew, 0);
        Vec_IntFree( vCisUsed );
        Gia_ManStop( pNew );
        return RetValue;
    }

    if ( fSynthesize )
    {
        vSop = bmcg_sat_solver_sop( pNew, 0 );
        Gia_ManStop( pNew );
        pMan = Abc_SopSynthesizeOne( Vec_StrArray(vSop), 1 );
        nCubes = Vec_StrCountEntry(vSop, '\n');
        if ( vDLits )
        {
            // convert into object IDs
            Vec_Int_t * vCisObjs = Vec_IntAlloc( Vec_IntSize(vCisUsed) );
            Vec_IntForEachEntry( vCisUsed, CiId, i )
                Vec_IntPush( vCisObjs, CiId + 1 );
            bmcg_sat_generate_dvars( vCisObjs, vSop, vDLits );
            Vec_IntFree( vCisObjs );
        }
        Vec_StrFree( vSop );

        if ( Gia_ManPoIsConst(pMan, 0) )
        {
            int RetValue = Gia_ManPoIsConst1(pMan, 0);
            Vec_IntFree( vCisUsed );
            Gia_ManStop( pMan );
            return RetValue;
        }
    }
    else
    {
        pMan = pNew;
    }

    Res = Gia_ManDupConeBack( p, pMan, vCisUsed );

    // report the result
    //printf( "Performed quantification with %5d nodes, %3d keep-vars, %3d quant-vars, resulting in %5d cubes and %5d nodes. ", 
    //    nNodes, Vec_IntSize(vCisUsed) - Count, Count, nCubes, Gia_ManAndNum(pMan) );
    //Abc_PrintTime( 1, "Time", Abc_Clock() - clkAll );

    Vec_IntFree( vCisUsed );
    Gia_ManStop( pMan );
    return Res;
}


/**Function*************************************************************

  Synopsis    [Performs SAT-based quantification.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManSatAndCollect_rec( Gia_Man_t * p, int iObj, Vec_Int_t * vObjsUsed, Vec_Int_t * vCiVars )
{
    Gia_Obj_t * pObj; int iVar;
    if ( (iVar = Gia_ObjCopyArray(p, iObj)) >= 0 )
        return iVar;
    pObj = Gia_ManObj( p, iObj );
    assert( Gia_ObjIsCand(pObj) );
    if ( Gia_ObjIsAnd(pObj) )
    {
        Gia_ManSatAndCollect_rec( p, Gia_ObjFaninId0(pObj, iObj), vObjsUsed, vCiVars );
        Gia_ManSatAndCollect_rec( p, Gia_ObjFaninId1(pObj, iObj), vObjsUsed, vCiVars );
    }
    iVar = Vec_IntSize( vObjsUsed );
    Vec_IntPush( vObjsUsed, iObj );
    Gia_ObjSetCopyArray( p, iObj, iVar );
    if ( vCiVars && Gia_ObjIsCi(pObj) )
        Vec_IntPush( vCiVars, iVar );
    return iVar;
}                             
void Gia_ManQuantLoadCnf( Gia_Man_t * p, Vec_Int_t * vObjsUsed, bmcg_sat_solver * pSats[] )
{
    Gia_Obj_t * pObj; int i;
    bmcg_sat_solver_reset( pSats[0] );
    if ( pSats[1] )
    bmcg_sat_solver_reset( pSats[1] );
    bmcg_sat_solver_set_nvars( pSats[0], Vec_IntSize(vObjsUsed) );
    if ( pSats[1] )
    bmcg_sat_solver_set_nvars( pSats[1], Vec_IntSize(vObjsUsed) );
    Gia_ManForEachObjVec( vObjsUsed, p, pObj, i ) 
        if ( Gia_ObjIsAnd(pObj) )
        {
            int iObj  = Gia_ObjId( p, pObj );
            int iVar  = Gia_ObjCopyArray(p, iObj);
            int iVar0 = Gia_ObjCopyArray(p, Gia_ObjFaninId0(pObj, iObj));
            int iVar1 = Gia_ObjCopyArray(p, Gia_ObjFaninId1(pObj, iObj));
            bmcg_sat_solver_add_and( pSats[0], iVar, iVar0, iVar1, Gia_ObjFaninC0(pObj), Gia_ObjFaninC1(pObj), 0 );
            if ( pSats[1] )
            bmcg_sat_solver_add_and( pSats[1], iVar, iVar0, iVar1, Gia_ObjFaninC0(pObj), Gia_ObjFaninC1(pObj), 0 );
        }
        else if ( Gia_ObjIsConst0(pObj) )
        {
            int Lit = Abc_Var2Lit( Gia_ObjCopyArray(p, 0), 1 );
            int RetValue = bmcg_sat_solver_addclause( pSats[0], &Lit, 1 );
            assert( RetValue );
            if ( pSats[1] )
            bmcg_sat_solver_addclause( pSats[1], &Lit, 1 );
            assert( Lit == 1 );
        }
}
int Gia_ManFactorSop( Gia_Man_t * p, Vec_Int_t * vCiObjIds, Vec_Str_t * vSop, int fHash )
{
    extern Gia_Man_t * Abc_SopSynthesizeOne( char * pSop, int fClp );
    Gia_Man_t * pMan = Abc_SopSynthesizeOne( Vec_StrArray(vSop), 1 );
    Gia_Obj_t * pObj; int i, Result;
    assert( Gia_ManPiNum(pMan) == Vec_IntSize(vCiObjIds) );
    Gia_ManConst0(pMan)->Value = 0;
    Gia_ManForEachPi( pMan, pObj, i )
        pObj->Value = Abc_Var2Lit( Vec_IntEntry(vCiObjIds, i), 0 );
    Gia_ManForEachAnd( pMan, pObj, i )
        if ( fHash )
            pObj->Value = Gia_ManHashAnd( p, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else
            pObj->Value = Gia_ManAppendAnd( p, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    pObj = Gia_ManPo(pMan, 0);
    Result = Gia_ObjFanin0Copy(pObj);
    Gia_ManStop( pMan );
    return Result;
}
int bmcg_sat_solver_quantify( bmcg_sat_solver * pSats[], Gia_Man_t * p, int iLit, int fHash, int(*pFuncCiToKeep)(void *, int), void * pData, Vec_Int_t * vDLits )
{
    Vec_Int_t * vObjsUsed = Vec_IntAlloc( 100 ); // GIA objs
    Vec_Int_t * vCiVars = Vec_IntAlloc( 100 );   // CI SAT vars
    Vec_Int_t * vVarMap = NULL; Vec_Str_t * vSop = NULL; 
    int i, iVar, iVarLast, Lit, RetValue, Count = 0, Result = -1;
    if ( vDLits ) Vec_IntClear( vDLits );
    if ( iLit < 2 )
        return iLit;
    if ( Vec_IntSize(&p->vCopies) < Gia_ManObjNum(p) )
        Vec_IntFillExtra( &p->vCopies, Gia_ManObjNum(p), -1 );
    // assign variable number 0 to const0 node
    iVar = Vec_IntSize(vObjsUsed); 
    Vec_IntPush( vObjsUsed, 0 );
    Gia_ObjSetCopyArray( p, 0, iVar );
    assert( iVar == 0 );    

    // collect other variables
    iVarLast = Gia_ManSatAndCollect_rec( p, Abc_Lit2Var(iLit), vObjsUsed, vCiVars );
    Gia_ManQuantLoadCnf( p, vObjsUsed, pSats );

    // check constants
    Lit = Abc_Var2Lit( iVarLast, !Abc_LitIsCompl(iLit) ); 
    RetValue = bmcg_sat_solver_addclause( pSats[0], &Lit, 1 ); // offset
    if ( !RetValue || bmcg_sat_solver_solve(pSats[0], NULL, 0) == GLUCOSE_UNSAT )
    {
        Result = 1;
        goto cleanup;
    }
    Lit = Abc_Var2Lit( iVarLast, Abc_LitIsCompl(iLit) );
    RetValue = bmcg_sat_solver_addclause( pSats[1], &Lit, 1 ); // onset
    if ( !RetValue || bmcg_sat_solver_solve(pSats[1], NULL, 0) == GLUCOSE_UNSAT )
    {
        Result = 0;
        goto cleanup;
    }
/*
    // reorder CI SAT variables to have keep-vars first
    Vec_Int_t * vCiVars2 = Vec_IntAlloc( 100 );   // CI SAT vars
    Vec_IntForEachEntry( vCiVars, iVar, i )
    {
        Gia_Obj_t * pObj = Gia_ManObj( p, Vec_IntEntry(vObjsUsed, iVar) );
        assert( Gia_ObjIsCi(pObj) );
        if ( pFuncCiToKeep(pData, Gia_ObjCioId(pObj)) )
            Vec_IntPush( vCiVars2, iVar );
    }
    Vec_IntForEachEntry( vCiVars, iVar, i )
    {
        Gia_Obj_t * pObj = Gia_ManObj( p, Vec_IntEntry(vObjsUsed, iVar) );
        assert( Gia_ObjIsCi(pObj) );
        if ( !pFuncCiToKeep(pData, Gia_ObjCioId(pObj)) )
            Vec_IntPush( vCiVars2, iVar );
    }
    ABC_SWAP( Vec_Int_t *, vCiVars2, vCiVars );
    Vec_IntFree( vCiVars2 );
*/
    // map CI SAT variables into their indexes used in the cubes
    vVarMap = Vec_IntStartFull( Vec_IntSize(vObjsUsed) );
    Vec_IntForEachEntry( vCiVars, iVar, i )
    {
        Gia_Obj_t * pObj = Gia_ManObj( p, Vec_IntEntry(vObjsUsed, iVar) );
        assert( Gia_ObjIsCi(pObj) );
        if ( pFuncCiToKeep(pData, Gia_ObjCioId(pObj)) )
            Vec_IntWriteEntry( vVarMap, iVar, i ), Count++;
    }
    if ( Count == 0 || Count == Vec_IntSize(vCiVars) )
    {
        Result = Count == 0 ? 1 : iLit;
        goto cleanup;
    }
    // generate cubes
    vSop = Glucose_GenerateCubes( pSats, vCiVars, vVarMap, 0 );
    //printf( "%s", Vec_StrArray(vSop) );
    // convert into object IDs
    Vec_IntForEachEntry( vCiVars, iVar, i )
        Vec_IntWriteEntry( vCiVars, i, Vec_IntEntry(vObjsUsed, iVar) );
    // generate unate variables
    if ( vDLits )
        bmcg_sat_generate_dvars( vCiVars, vSop, vDLits );
    // convert into an AIG
    RetValue = Gia_ManAndNum(p);
    Result = Gia_ManFactorSop( p, vCiVars, vSop, fHash );

    // report the result
//    printf( "Performed quantification with %5d nodes, %3d keep-vars, %3d quant-vars, resulting in %5d cubes and %5d nodes. ", 
//        Vec_IntSize(vObjsUsed), Count, Vec_IntSize(vCiVars) - Count, Vec_StrCountEntry(vSop, '\n'), Gia_ManAndNum(p)-RetValue );
//    Abc_PrintTime( 1, "Time", Abc_Clock() - clkAll );

cleanup:
    Vec_IntForEachEntry( vObjsUsed, iVar, i )
        Gia_ObjSetCopyArray( p, iVar, -1 );
    Vec_IntFree( vObjsUsed );
    Vec_IntFree( vCiVars );
    Vec_IntFreeP( &vVarMap );
    Vec_StrFreeP( &vSop );
    return Result;
}
int Gia_ManCiIsToKeep( void * pData, int i )
{
    return i % 5 != 0;
}
void Glucose_QuantifyAigTest( Gia_Man_t * p )
{
    bmcg_sat_solver * pSats[3] = { bmcg_sat_solver_start(), bmcg_sat_solver_start(), bmcg_sat_solver_start() };

    abctime clk1 = Abc_Clock();
    int iRes1 = bmcg_sat_solver_quantify( pSats, p, Gia_ObjFaninLit0p(p, Gia_ManPo(p, 0)), 0, Gia_ManCiIsToKeep, NULL, NULL );
    abctime clk1d = Abc_Clock()-clk1;

    abctime clk2 = Abc_Clock();
    int iRes2 = bmcg_sat_solver_quantify2( p, Gia_ObjFaninLit0p(p, Gia_ManPo(p, 0)), 0, Gia_ManCiIsToKeep, NULL, NULL );
    abctime clk2d = Abc_Clock()-clk2;

    Abc_PrintTime( 1, "Time1", clk1d );
    Abc_PrintTime( 1, "Time2", clk2d );

    if ( bmcg_sat_solver_equiv_overlap_check( pSats[2], p, iRes1, iRes2, 1 ) )
        printf( "Verification passed.\n" );
    else
        printf( "Verification FAILED.\n" );

    Gia_ManAppendCo( p, iRes1 );
    Gia_ManAppendCo( p, iRes2 );

    bmcg_sat_solver_stop( pSats[0] );
    bmcg_sat_solver_stop( pSats[1] );
    bmcg_sat_solver_stop( pSats[2] );
}
int bmcg_sat_solver_quantify_test( bmcg_sat_solver * pSats[], Gia_Man_t * p, int iLit, int fHash, int(*pFuncCiToKeep)(void *, int), void * pData, Vec_Int_t * vDLits )
{
    extern int Gia_ManQuantExist( Gia_Man_t * p, int iLit, int(*pFuncCiToKeep)(void *, int), void * pData );
    int Res1 = Gia_ManQuantExist( p, iLit, pFuncCiToKeep, pData );
    int Res2 = bmcg_sat_solver_quantify2( p, iLit, 1, pFuncCiToKeep, pData, NULL );

    bmcg_sat_solver * pSat = bmcg_sat_solver_start();
    if ( bmcg_sat_solver_equiv_overlap_check( pSat, p, Res1, Res2, 1 ) )
        printf( "Verification passed.\n" );
    else
    {
        printf( "Verification FAILED.\n" );
        bmcg_sat_solver_print_sop_lit( p, Res1 );
        bmcg_sat_solver_print_sop_lit( p, Res2 );
        printf( "\n" );
    }
    return Res1;
}

/**Function*************************************************************

  Synopsis    [Checks equivalence or intersection of two nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int bmcg_sat_solver_equiv_overlap_check( bmcg_sat_solver * pSat, Gia_Man_t * p, int iLit0, int iLit1, int fEquiv )
{
    bmcg_sat_solver * pSats[2] = { pSat, NULL };
    Vec_Int_t * vObjsUsed = Vec_IntAlloc( 100 ); 
    int i, iVar, iSatVar[2], iSatLit[2], Lits[2], status;
    if ( Vec_IntSize(&p->vCopies) < Gia_ManObjNum(p) )
        Vec_IntFillExtra( &p->vCopies, Gia_ManObjNum(p), -1 );

    // assign const0 variable number 0
    iVar = Vec_IntSize(vObjsUsed);
    Vec_IntPush( vObjsUsed, 0 );
    Gia_ObjSetCopyArray( p, 0, iVar );
    assert( iVar == 0 );

    iSatVar[0] = Gia_ManSatAndCollect_rec( p, Abc_Lit2Var(iLit0), vObjsUsed, NULL );
    iSatVar[1] = Gia_ManSatAndCollect_rec( p, Abc_Lit2Var(iLit1), vObjsUsed, NULL );

    iSatLit[0] = Abc_Var2Lit( iSatVar[0], Abc_LitIsCompl(iLit0) );
    iSatLit[1] = Abc_Var2Lit( iSatVar[1], Abc_LitIsCompl(iLit1) );
    Gia_ManQuantLoadCnf( p, vObjsUsed, pSats );
    Vec_IntForEachEntry( vObjsUsed, iVar, i )
        Gia_ObjSetCopyArray( p, iVar, -1 );
    Vec_IntFree( vObjsUsed );

    if ( fEquiv )
    {
        Lits[0] = iSatLit[0];
        Lits[1] = Abc_LitNot(iSatLit[1]);
        status  = bmcg_sat_solver_solve( pSats[0], Lits, 2 );
        if ( status == GLUCOSE_UNSAT )
        {
            Lits[0] = Abc_LitNot(iSatLit[0]);
            Lits[1] = iSatLit[1];
            status  = bmcg_sat_solver_solve( pSats[0], Lits, 2 );
        }
        return status == GLUCOSE_UNSAT;
    }
    else
    {
        Lits[0] = iSatLit[0];
        Lits[1] = iSatLit[1];
        status  = bmcg_sat_solver_solve( pSats[0], Lits, 2 );
        return status == GLUCOSE_SAT;
    }
}
void Glucose_CheckTwoNodesTest( Gia_Man_t * p )
{
    int n, Res;
    bmcg_sat_solver * pSat = bmcg_sat_solver_start();
    for ( n = 0; n < 2; n++ )
    {
        Res = bmcg_sat_solver_equiv_overlap_check( 
            pSat, p, 
            Gia_ObjFaninLit0p(p, Gia_ManPo(p, 0)), 
            Gia_ObjFaninLit0p(p, Gia_ManPo(p, 1)), 
            n );
        bmcg_sat_solver_reset( pSat );
        printf( "%s %s.\n", n ? "Equivalence" : "Overlap", Res ? "holds" : "fails" );
    }
    bmcg_sat_solver_stop( pSat );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Glucose_SolveAig(Gia_Man_t * p, Glucose_Pars * pPars)
{  
    abctime clk = Abc_Clock();

    SimpSolver S;
    S.verbosity = pPars->verb;
    S.verbEveryConflicts = 50000;
    S.showModel = false;
    //S.verbosity = 2;
    S.setConfBudget( pPars->nConfls > 0 ? (int64_t)pPars->nConfls : -1 );

    S.parsing = 1;
    Vec_Int_t * vCnfIds = Glucose_SolverFromAig(p,S);
    S.parsing = 0;

    if (pPars->verb)
    {
        printf("c ============================[ Problem Statistics ]=============================\n");
        printf("c |                                                                             |\n");
        printf("c |  Number of variables:  %12d                                         |\n", S.nVars());
        printf("c |  Number of clauses:    %12d                                         |\n", S.nClauses());
    }

    if (pPars->pre) 
    {
        S.eliminate(true);
        printf( "c Simplication removed %d variables and %d clauses.  ", S.eliminated_vars, S.eliminated_clauses );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    }
    
    vec<Lit> dummy;
    lbool ret = S.solveLimited(dummy, 0);

    if ( pPars->verb ) glucose_print_stats(S, Abc_Clock() - clk);
    printf(ret == l_True ? "SATISFIABLE" : ret == l_False ? "UNSATISFIABLE" : "INDETERMINATE");
    Abc_PrintTime( 1, "      Time", Abc_Clock() - clk );

    // port counterexample
    if (ret == l_True)
    {
        Gia_Obj_t * pObj;  int i;
        p->pCexComb = Abc_CexAlloc(0,Gia_ManCiNum(p),1);
        Gia_ManForEachCi( p, pObj, i )
        {
            assert(Vec_IntEntry(vCnfIds,Gia_ObjId(p, pObj))!=-1);
            if (S.model[Vec_IntEntry(vCnfIds,Gia_ObjId(p, pObj))] == l_True)
                Abc_InfoSetBit( p->pCexComb->pData, i);
        }
    }
    Vec_IntFree(vCnfIds);
    return (ret == l_True ? 10 : ret == l_False ? 20 : 0);
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END
