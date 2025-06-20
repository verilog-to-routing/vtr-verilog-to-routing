/**CFile****************************************************************

  FileName    [kissatSolver.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT solver Kissat by Armin Biere, University of Freiburg]

  Synopsis    [https://github.com/arminbiere/kissat]

  Author      [Integrated into ABC by Yukio Miyasaka]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: kissatSolver.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC_SAT_KISSAT_SOLVER_H_
#define ABC_SAT_KISSAT_SOLVER_H_

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

typedef struct kissat_solver_ kissat_solver;
struct kissat_solver_
{
  void* p;
  int nVars;
};


////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

extern kissat_solver*  kissat_solver_new(void);
extern void            kissat_solver_delete(kissat_solver* s);
extern int             kissat_solver_addclause(kissat_solver* s, int* begin, int* end);
extern int             kissat_solver_solve(kissat_solver* s, int* begin, int* end, ABC_INT64_T nConfLimit, ABC_INT64_T nInsLimit, ABC_INT64_T nConfLimitGlobal, ABC_INT64_T nInsLimitGlobal);
extern int             kissat_solver_nvars(kissat_solver* s);
extern int             kissat_solver_addvar(kissat_solver* s);
extern void            kissat_solver_setnvars(kissat_solver* s,int n);
extern int             kissat_solver_get_var_value(kissat_solver* s, int v);
extern Vec_Int_t *     kissat_solve_cnf( Cnf_Dat_t * pCnf, char * pArgs, int nConfs, int nTimeLimit, int fSat, int fUnsat, int fPrintCex, int fVerbose );

ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
