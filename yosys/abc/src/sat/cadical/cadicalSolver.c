/**CFile****************************************************************

  FileName    [cadicalSolver.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT solver CaDiCaL by Armin Biere, University of Freiburg]

  Synopsis    [https://github.com/arminbiere/cadical]

  Author      [Integrated into ABC by Yukio Miyasaka]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cadicalSolver.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ccadical.h"
#include "cadicalSolver.h"

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
cadical_solver* cadical_solver_new(void) {
  cadical_solver* s = (cadical_solver*)malloc(sizeof(cadical_solver));
  s->p = (void*)ccadical_init();
  s->nVars = 0;
  s->vAssumptions = NULL;
  s->vCore = NULL;
  return s;
}

/**Function*************************************************************

  Synopsis    [delete solver]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void cadical_solver_delete(cadical_solver* s) {
  ccadical_release((CCaDiCaL*)s->p);
  if(s->vAssumptions) {
    Vec_IntFree(s->vAssumptions);
  }
  if(s->vCore) {
    Vec_IntFree(s->vCore);
  }
  free(s);
}

/**Function*************************************************************

  Synopsis    [add clause]

  Description [cadical takes x and -x as a literal for a variable x > 0,
               where 0 is an indicator of the end of a clause.
               since variables start from 0 in abc, a variable v is
               translated into v + 1 in cadical.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int cadical_solver_addclause(cadical_solver* s, int* begin, int* end) {
  for(;begin != end; begin++) {
    if(*begin & 1) {
      ccadical_add((CCaDiCaL*)s->p, -(1 + ((*begin) >> 1)));
    } else {
      ccadical_add((CCaDiCaL*)s->p,   1 + ((*begin) >> 1) );
    }
  }
  ccadical_add((CCaDiCaL*)s->p, 0);
  return !ccadical_is_inconsistent((CCaDiCaL*)s->p);
}

/**Function*************************************************************

  Synopsis    [solve with resource limits]

  Description [assumptions and inspection limits are not supported.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int cadical_solver_solve(cadical_solver* s, int* begin, int* end, ABC_INT64_T nConfLimit, ABC_INT64_T nInsLimit, ABC_INT64_T nConfLimitGlobal, ABC_INT64_T nInsLimitGlobal) {
  // inspection limits are not supported
  assert(nInsLimit == 0);
  assert(nInsLimitGlobal == 0);
  // set conflict limits
  if(nConfLimit)
    ccadical_limit((CCaDiCaL*)s->p, "conflicts", nConfLimit);
  if(nConfLimitGlobal && (nConfLimit == 0 || nConfLimit > nConfLimitGlobal))
    ccadical_limit((CCaDiCaL*)s->p, "conflicts", nConfLimitGlobal);
  // assumptions
  if(begin != end) {
    // save
    if(s->vAssumptions == NULL) {
      s->vAssumptions = Vec_IntAllocArrayCopy(begin, end - begin);
    } else {
      Vec_IntClear(s->vAssumptions);
      Vec_IntGrow(s->vAssumptions, end - begin);
      Vec_IntPushArray(s->vAssumptions, begin, end - begin);
    }
    // assume
    for(;begin != end; begin++) {
      if(*begin & 1) {
        ccadical_assume((CCaDiCaL*)s->p, -(1 + ((*begin) >> 1)));
      } else {
        ccadical_assume((CCaDiCaL*)s->p,   1 + ((*begin) >> 1) );
      }
    }
  }
  // solve
  int res = ccadical_solve((CCaDiCaL*)s->p);
  // translate this cadical return value into a corresponding ABC status value
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

  Synopsis    [get unsat core]

  Description [following minisat, return number of literals in core,
               which consists of responsible assumptions, negated.
               array will be freed by solver.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int cadical_solver_final(cadical_solver* s, int** ppArray) {
  int v, i;
  if(s->vCore == NULL) {
    s->vCore = Vec_IntAlloc(Vec_IntSize(s->vAssumptions));
  } else {
    Vec_IntClear(s->vCore);
  }
  Vec_IntForEachEntry(s->vAssumptions, v, i) {
    int failed;
    if(v & 1) {
      failed = ccadical_failed((CCaDiCaL*)s->p, -(1 + (v >> 1)));
    } else {
      failed = ccadical_failed((CCaDiCaL*)s->p,   1 + (v >> 1) );
    }
    if(failed) {
      Vec_IntPush(s->vCore, Abc_LitNot(v));
    }
  }
  *ppArray = Vec_IntArray(s->vCore);
  return Vec_IntSize(s->vCore);
}

/**Function*************************************************************

  Synopsis    [get number of variables]

  Description [emulated using "nVars".]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int cadical_solver_nvars(cadical_solver* s) {
  return s->nVars;
}

/**Function*************************************************************

  Synopsis    [add new variable]

  Description [emulated using "nVars".]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int cadical_solver_addvar(cadical_solver* s) {
  return s->nVars++;
}

/**Function*************************************************************

  Synopsis    [set number of variables]

  Description [not only emulate with "nVars" but also reserve memory.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void cadical_solver_setnvars(cadical_solver* s,int n) {
  s->nVars = n;
  ccadical_reserve((CCaDiCaL*)s->p, n);
}

/**Function*************************************************************

  Synopsis    [get value of variable]

  Description [cadical returns x (true) or -x (false) for a variable x.
               note a variable v was translated into v + 1 in cadical.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int cadical_solver_get_var_value(cadical_solver* s, int v) {
  return ccadical_val((CCaDiCaL*)s->p, v + 1) > 0;
}


/**Function*************************************************************

  Synopsis    [Solves the given CNF using cadical.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * cadical_solve_cnf( Cnf_Dat_t * pCnf, char * pArgs, int nConfs, int nTimeLimit, int fSat, int fUnsat, int fPrintCex, int fVerbose )
{
    abctime clk = Abc_Clock();
    Vec_Int_t * vRes = NULL;
    int i, * pBeg, * pEnd, RetValue;    
    if ( fVerbose )
        printf( "CNF stats: Vars = %6d. Clauses = %7d. Literals = %8d. ", pCnf->nVars, pCnf->nClauses, pCnf->nLiterals );
    cadical_solver *pSat = cadical_solver_new();
    cadical_solver_setnvars(pSat, pCnf->nVars);
    assert(cadical_solver_nvars(pSat) == pCnf->nVars);
    Cnf_CnfForClause( pCnf, pBeg, pEnd, i ) {
        if ( !cadical_solver_addclause(pSat, pBeg, pEnd) )
        {
            assert( 0 ); // if it happens, can return 1 (unsatisfiable)
            return NULL;
        }    
    }
    RetValue = cadical_solver_solve(pSat, NULL, NULL, 0, 0, 0, 0);
    if ( RetValue == 1 )
      printf( "Result: Satisfiable.  " );
    else if ( RetValue == -1 )
      printf( "Result: Unsatisfiable.  " );
    else
      printf( "Result: Undecided.  " );
    if ( RetValue == 1 ) {
        vRes = Vec_IntAlloc( pCnf->nVars );
        for ( i = 0; i < pCnf->nVars; i++ )
          Vec_IntPush( vRes, cadical_solver_get_var_value(pSat, i) );
    }
    cadical_solver_delete(pSat);
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    return vRes;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END
