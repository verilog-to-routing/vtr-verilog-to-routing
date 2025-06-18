/**CFile****************************************************************

  FileName    [kissatSolver.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT solver Kissat by Armin Biere, University of Freiburg]

  Synopsis    [https://github.com/arminbiere/kissat]

  Author      [Integrated into ABC by Yukio Miyasaka]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: kissatSolver.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "kissat.h"
#include "kissatSolver.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [allocate solver]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
kissat_solver* kissat_solver_new(void) {
  kissat_solver* s = (kissat_solver*)malloc(sizeof(kissat_solver));
  s->p = (void*)kissat_init();
  s->nVars = 0;
  return s;
}

/**Function*************************************************************

  Synopsis    [delete solver]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void kissat_solver_delete(kissat_solver* s) {
  kissat_release((kissat*)s->p);
  free(s);
}

/**Function*************************************************************

  Synopsis    [add clause]

  Description [kissat takes x and -x as a literal for a variable x > 0,
               where 0 is an indicator of the end of a clause.
               since variables start from 0 in abc, a variable v is
               translated into v + 1 in kissat.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int kissat_solver_addclause(kissat_solver* s, int* begin, int* end) {
  for(;begin != end; begin++) {
    if(*begin & 1) {
      kissat_add((kissat*)s->p, -(1 + ((*begin) >> 1)));
    } else {
      kissat_add((kissat*)s->p,   1 + ((*begin) >> 1) );
    }
  }
  kissat_add((kissat*)s->p, 0);
  return !kissat_is_inconsistent((kissat*)s->p);
}

/**Function*************************************************************

  Synopsis    [solve with resource limits]

  Description [assumptions and inspection limits are not supported.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int kissat_solver_solve(kissat_solver* s, int* begin, int* end, ABC_INT64_T nConfLimit, ABC_INT64_T nInsLimit, ABC_INT64_T nConfLimitGlobal, ABC_INT64_T nInsLimitGlobal) {
  // assumptions are not supported
  assert(begin == end);
  // inspection limits are not supported
  assert(nInsLimit == 0);
  assert(nInsLimitGlobal == 0);
  // set conflict limits
  if(nConfLimit)
    kissat_set_conflict_limit((kissat*)s->p, nConfLimit);
  if(nConfLimitGlobal && (nConfLimit == 0 || nConfLimit > nConfLimitGlobal))
    kissat_set_conflict_limit((kissat*)s->p, nConfLimitGlobal);
  // solve
  int res = kissat_solve((kissat*)s->p);
  // translate this kissat return value into a corresponding ABC status value
  switch(res) {
  case  0: // UNDETERMINED
    return 0;
  case 10: // SATISFIABLE
    return 1;
  case 20: // UNSATISFIABLE
    return -1;
  default:
    assert(0);
  }
  return 0;
}

/**Function*************************************************************

  Synopsis    [get number of variables]

  Description [emulated using "nVars".]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int kissat_solver_nvars(kissat_solver* s) {
  return s->nVars;
}

/**Function*************************************************************

  Synopsis    [add new variable]

  Description [emulated using "nVars".]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int kissat_solver_addvar(kissat_solver* s) {
  return s->nVars++;
}

/**Function*************************************************************

  Synopsis    [set number of variables]

  Description [not only emulate with "nVars" but also reserve memory.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void kissat_solver_setnvars(kissat_solver* s,int n) {
  s->nVars = n;
  kissat_reserve((kissat*)s->p, n);
}

/**Function*************************************************************

  Synopsis    [get value of variable]

  Description [kissat returns x (true) or -x (false) for a variable x.
               note a variable v was translated into v + 1 in kissat.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int kissat_solver_get_var_value(kissat_solver* s, int v) {
  return kissat_value((kissat*)s->p, v + 1) > 0;
}


/**Function*************************************************************

  Synopsis    [Solves the given CNF using kissat.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * kissat_solve_cnf( Cnf_Dat_t * pCnf, char * pArgs, int nConfs, int nTimeLimit, int fSat, int fUnsat, int fPrintCex, int fVerbose )
{
    abctime clk = Abc_Clock();
    Vec_Int_t * vRes = NULL;
    int i, * pBeg, * pEnd, RetValue;    
    if ( fVerbose )
        printf( "CNF stats: Vars = %6d. Clauses = %7d. Literals = %8d. ", pCnf->nVars, pCnf->nClauses, pCnf->nLiterals );
    kissat_solver *pSat = kissat_solver_new();
    kissat_solver_setnvars(pSat, pCnf->nVars);
    assert(kissat_solver_nvars(pSat) == pCnf->nVars);
    Cnf_CnfForClause( pCnf, pBeg, pEnd, i ) {
        if ( !kissat_solver_addclause(pSat, pBeg, pEnd) )
        {
            assert( 0 ); // if it happens, can return 1 (unsatisfiable)
            return NULL;
        }    
    }
    RetValue = kissat_solver_solve(pSat, NULL, NULL, 0, 0, 0, 0);
    if ( RetValue == 1 )
      printf( "Result: Satisfiable.  " );
    else if ( RetValue == -1 )
      printf( "Result: Unsatisfiable.  " );
    else
      printf( "Result: Undecided.  " );
    if ( RetValue == 1 ) {
        vRes = Vec_IntAlloc( pCnf->nVars );
        for ( i = 0; i < pCnf->nVars; i++ )
          Vec_IntPush( vRes, kissat_solver_get_var_value(pSat, i) );
    }
    kissat_solver_delete(pSat);
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    return vRes;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END
