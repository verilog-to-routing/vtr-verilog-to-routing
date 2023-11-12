/**CFile****************************************************************

  FileName    [AbcGlucose.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT solver Glucose 3.0 by Gilles Audemard and Laurent Simon.]

  Synopsis    [Interface to Glucose.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 6, 2017.]

  Revision    [$Id: AbcGlucose.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC_SAT_GLUCOSE_H_
#define ABC_SAT_GLUCOSE_H_

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "aig/gia/gia.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

#define GLUCOSE_UNSAT -1
#define GLUCOSE_SAT    1
#define GLUCOSE_UNDEC  0


ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Glucose_Pars_ Glucose_Pars;
struct Glucose_Pars_ {
    int pre;     // preprocessing
    int verb;    // verbosity
    int cust;    // customizable
    int nConfls; // conflict limit (0 = no limit)
};

static inline Glucose_Pars Glucose_CreatePars(int p, int v, int c, int nConfls)
{
    Glucose_Pars pars;
    pars.pre     = p;
    pars.verb    = v;
    pars.cust    = c;
    pars.nConfls = nConfls;
    return pars;
}

typedef void bmcg_sat_solver;

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

extern bmcg_sat_solver * bmcg_sat_solver_start();
extern void              bmcg_sat_solver_stop( bmcg_sat_solver* s );
extern void              bmcg_sat_solver_reset( bmcg_sat_solver* s );
extern int               bmcg_sat_solver_addclause( bmcg_sat_solver* s, int * plits, int nlits );
extern void              bmcg_sat_solver_setcallback( bmcg_sat_solver* s, void * pman, int(*pfunc)(void*, int, int*) );
extern int               bmcg_sat_solver_solve( bmcg_sat_solver* s, int * plits, int nlits );
extern int               bmcg_sat_solver_final( bmcg_sat_solver* s, int ** ppArray );
extern int               bmcg_sat_solver_addvar( bmcg_sat_solver* s );
extern void              bmcg_sat_solver_set_nvars( bmcg_sat_solver* s, int nvars );
extern int               bmcg_sat_solver_eliminate( bmcg_sat_solver* s, int turn_off_elim );
extern int               bmcg_sat_solver_var_is_elim( bmcg_sat_solver* s, int v );
extern void              bmcg_sat_solver_var_set_frozen( bmcg_sat_solver* s, int v, int freeze );
extern int               bmcg_sat_solver_elim_varnum(bmcg_sat_solver* s);
extern int *             bmcg_sat_solver_read_cex( bmcg_sat_solver* s );
extern int               bmcg_sat_solver_read_cex_varvalue( bmcg_sat_solver* s, int );
extern void              bmcg_sat_solver_set_stop( bmcg_sat_solver* s, int * pstop );
extern abctime           bmcg_sat_solver_set_runtime_limit( bmcg_sat_solver* s, abctime Limit );
extern void              bmcg_sat_solver_set_conflict_budget( bmcg_sat_solver* s, int Limit );
extern int               bmcg_sat_solver_varnum( bmcg_sat_solver* s );
extern int               bmcg_sat_solver_clausenum( bmcg_sat_solver* s );
extern int               bmcg_sat_solver_learntnum( bmcg_sat_solver* s );
extern int               bmcg_sat_solver_conflictnum( bmcg_sat_solver* s );
extern int               bmcg_sat_solver_minimize_assumptions( bmcg_sat_solver * s, int * plits, int nlits, int pivot );
extern int               bmcg_sat_solver_add_and( bmcg_sat_solver * s, int iVar, int iVar0, int iVar1, int fCompl0, int fCompl1, int fCompl );
extern int               bmcg_sat_solver_add_xor( bmcg_sat_solver * s, int iVarA, int iVarB, int iVarC, int fCompl );
extern int               bmcg_sat_solver_quantify( bmcg_sat_solver * s[], Gia_Man_t * p, int iLit, int fHash, int(*pFuncCiToKeep)(void *, int), void * pData, Vec_Int_t * vDLits );
extern int               bmcg_sat_solver_equiv_overlap_check( bmcg_sat_solver * s, Gia_Man_t * p, int iLit0, int iLit1, int fEquiv );
extern Vec_Str_t *       bmcg_sat_solver_sop( Gia_Man_t * p, int CubeLimit );
extern int               bmcg_sat_solver_jftr( bmcg_sat_solver * s );
extern void              bmcg_sat_solver_set_jftr( bmcg_sat_solver * s, int jftr );
extern void              bmcg_sat_solver_set_var_fanin_lit( bmcg_sat_solver * s, int var, int lit0, int lit1 );
extern void              bmcg_sat_solver_start_new_round( bmcg_sat_solver * s );
extern void              bmcg_sat_solver_mark_cone( bmcg_sat_solver * s, int var );


extern void              Glucose_SolveCnf( char * pFilename, Glucose_Pars * pPars, int fDumpCnf );
extern int               Glucose_SolveAig( Gia_Man_t * p, Glucose_Pars * pPars );

ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

