/**CFile****************************************************************

  FileName    [cadicalSolver.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT solver CaDiCaL by Armin Biere, University of Freiburg]

  Synopsis    [https://github.com/arminbiere/cadical]

  Author      [Integrated into ABC by Yukio Miyasaka]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cadicalSolver.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC_SAT_CADICAL_SOLVER_H_
#define ABC_SAT_CADICAL_SOLVER_H_

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "aig/gia/gia.h"
#include "sat/cnf/cnf.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct cadical_solver_ cadical_solver;
struct cadical_solver_
{
  void* p;
  int nVars;
  Vec_Int_t* vAssumptions;
  Vec_Int_t* vCore;
};


////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

extern cadical_solver*  cadical_solver_new(void);
extern void             cadical_solver_delete(cadical_solver* s);
extern int              cadical_solver_addclause(cadical_solver* s, int* begin, int* end);
extern int              cadical_solver_solve(cadical_solver* s, int* begin, int* end, ABC_INT64_T nConfLimit, ABC_INT64_T nInsLimit, ABC_INT64_T nConfLimitGlobal, ABC_INT64_T nInsLimitGlobal);
extern int              cadical_solver_final(cadical_solver* s, int** ppArray);
extern int              cadical_solver_nvars(cadical_solver* s);
extern int              cadical_solver_addvar(cadical_solver* s);
extern void             cadical_solver_setnvars(cadical_solver* s,int n);
extern int              cadical_solver_get_var_value(cadical_solver* s, int v);
extern Vec_Int_t *      cadical_solve_cnf( Cnf_Dat_t * pCnf, char * pArgs, int nConfs, int nTimeLimit, int fSat, int fUnsat, int fPrintCex, int fVerbose );

ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
