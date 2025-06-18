/**CFile****************************************************************

  FileName    [cadicalTest.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName []

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cadicalTest.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cadicalSolver.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
void cadical_solver_test() {
  int RetValue;
  int Lits[3];
  // test 1
  {
    cadical_solver *pSat = cadical_solver_new();
    int a = cadical_solver_addvar(pSat);
    int b = cadical_solver_addvar(pSat);
    int c = cadical_solver_addvar(pSat);
    assert(cadical_solver_nvars(pSat) == 3);
    Lits[0] = Abc_Var2Lit(a, 0);
    Lits[1] = Abc_Var2Lit(b, 0);
    Lits[2] = Abc_Var2Lit(c, 0);
    printf("adding (a, b, c)\n");
    RetValue = cadical_solver_addclause(pSat, Lits, Lits + 3);
    assert(RetValue);
    Lits[0] = Abc_Var2Lit(a, 0);
    Lits[1] = Abc_Var2Lit(b, 1);
    printf("adding (a, !b)\n");
    RetValue = cadical_solver_addclause(pSat, Lits, Lits + 2);
    assert(RetValue);
    Lits[0] = Abc_Var2Lit(a, 1);
    printf("adding (!a)\n");
    RetValue = cadical_solver_addclause(pSat, Lits, Lits + 1);
    assert(RetValue);
    RetValue = cadical_solver_solve(pSat, NULL, NULL, 0, 0, 0, 0);
    printf("solved: %d\n", RetValue);
    assert(RetValue == 1);
    int a_val = cadical_solver_get_var_value(pSat, a);
    int b_val = cadical_solver_get_var_value(pSat, b);
    int c_val = cadical_solver_get_var_value(pSat, c);
    printf("a = %d, b = %d, c = %d\n", a_val, b_val, c_val);
    assert(a_val == 0);
    assert(b_val == 0);
    assert(c_val == 1);
    cadical_solver_delete(pSat);
    printf("test 1 passed\n");
  }
  // test 2
  {
    cadical_solver *pSat = cadical_solver_new();
    cadical_solver_setnvars(pSat, 2);
    assert(cadical_solver_nvars(pSat) == 2);
    Lits[0] = Abc_Var2Lit(0, 0);
    Lits[1] = Abc_Var2Lit(1, 0);
    printf("adding (x0, x1)\n");
    RetValue = cadical_solver_addclause(pSat, Lits, Lits + 2);
    assert(RetValue);
    Lits[0] = Abc_Var2Lit(0, 0);
    Lits[1] = Abc_Var2Lit(1, 1);
    printf("adding (x0, !x1)\n");
    RetValue = cadical_solver_addclause(pSat, Lits, Lits + 2);
    assert(RetValue);
    Lits[0] = Abc_Var2Lit(0, 1);
    Lits[1] = Abc_Var2Lit(1, 1);
    printf("adding (!x0, !x1)\n");
    RetValue = cadical_solver_addclause(pSat, Lits, Lits + 2);
    assert(RetValue);
    RetValue = cadical_solver_solve(pSat, NULL, NULL, 0, 0, 0, 0);
    printf("solved: %d\n", RetValue);
    assert(RetValue == 1);
    printf("x0 = %d, x1 = %d\n", cadical_solver_get_var_value(pSat, 0), cadical_solver_get_var_value(pSat, 1));
    assert(cadical_solver_get_var_value(pSat, 0) == 1);
    assert(cadical_solver_get_var_value(pSat, 1) == 0);
    cadical_solver_delete(pSat);
    printf("test 2 passed\n");
  }
  // test 3
  {
    cadical_solver *pSat = cadical_solver_new();
    cadical_solver_setnvars(pSat, 3);
    assert(cadical_solver_nvars(pSat) == 3);
    Lits[0] = Abc_Var2Lit(0, 1);
    Lits[1] = Abc_Var2Lit(1, 0);
    Lits[2] = Abc_Var2Lit(2, 1);
    printf("adding (!x0, x1, !x2)\n");
    RetValue = cadical_solver_addclause(pSat, Lits, Lits + 3);
    assert(RetValue);
    Lits[0] = Abc_Var2Lit(0, 0);
    printf("adding (x0)\n");
    RetValue = cadical_solver_addclause(pSat, Lits, Lits + 1);
    assert(RetValue);
    Lits[0] = Abc_Var2Lit(1, 1);
    printf("adding (!x1)\n");
    RetValue = cadical_solver_addclause(pSat, Lits, Lits + 1);
    assert(RetValue);
    Lits[0] = Abc_Var2Lit(2, 0);
    printf("adding (x2)\n");
    RetValue = cadical_solver_addclause(pSat, Lits, Lits + 1);
    RetValue = cadical_solver_solve(pSat, NULL, NULL, 0, 0, 0, 0);
    printf("solved: %d\n", RetValue);
    assert(RetValue == -1);
    cadical_solver_delete(pSat);
    printf("test 3 passed\n");
  }
  // test 4
  {
    cadical_solver *pSat = cadical_solver_new();
    cadical_solver_setnvars(pSat, 3);
    assert(cadical_solver_nvars(pSat) == 3);
    Lits[0] = Abc_Var2Lit(0, 1);
    Lits[1] = Abc_Var2Lit(1, 0);
    Lits[2] = Abc_Var2Lit(2, 1);
    printf("adding (!x0, x1, !x2)\n");
    RetValue = cadical_solver_addclause(pSat, Lits, Lits + 3);
    assert(RetValue);
    Lits[0] = Abc_Var2Lit(0, 0);
    printf("adding (x0)\n");
    RetValue = cadical_solver_addclause(pSat, Lits, Lits + 1);
    assert(RetValue);
    Lits[0] = Abc_Var2Lit(1, 1);
    printf("assume (!x1)\n");
    RetValue = cadical_solver_solve(pSat, Lits, Lits + 1, 0, 0, 0, 0);
    printf("solved: %d\n", RetValue);
    assert(RetValue == 1);
    printf("x0 = %d, x1 = %d, x2 = %d\n", cadical_solver_get_var_value(pSat, 0), cadical_solver_get_var_value(pSat, 1), cadical_solver_get_var_value(pSat, 2));
    assert(cadical_solver_get_var_value(pSat, 0) == 1);
    assert(cadical_solver_get_var_value(pSat, 1) == 0);
    assert(cadical_solver_get_var_value(pSat, 2) == 0);
    cadical_solver_delete(pSat);
    printf("test 4 passed\n");
  }
  // test 5
  {
    cadical_solver *pSat = cadical_solver_new();
    cadical_solver_setnvars(pSat, 3);
    assert(cadical_solver_nvars(pSat) == 3);
    Lits[0] = Abc_Var2Lit(0, 1);
    Lits[1] = Abc_Var2Lit(1, 0);
    Lits[2] = Abc_Var2Lit(2, 1);
    printf("adding (!x0, x1, !x2)\n");
    RetValue = cadical_solver_addclause(pSat, Lits, Lits + 3);
    assert(RetValue);
    Lits[0] = Abc_Var2Lit(0, 1);
    Lits[1] = Abc_Var2Lit(1, 1);
    Lits[2] = Abc_Var2Lit(2, 1);
    printf("adding (!x0, !x1, !x2)\n");
    RetValue = cadical_solver_addclause(pSat, Lits, Lits + 3);
    assert(RetValue);
    Lits[0] = Abc_Var2Lit(0, 0);
    printf("adding (x0)\n");
    RetValue = cadical_solver_addclause(pSat, Lits, Lits + 1);
    assert(RetValue);
    Lits[0] = Abc_Var2Lit(1, 0);
    Lits[1] = Abc_Var2Lit(2, 0);
    printf("assume (x1, x2)\n");
    RetValue = cadical_solver_solve(pSat, Lits, Lits + 2, 0, 0, 0, 0);
    printf("solved: %d\n", RetValue);
    assert(RetValue == -1);
    int *pCore;
    int nSize = cadical_solver_final(pSat, &pCore);
    printf("core: ");
    for(int i = 0; i < nSize; i++) {
      if(i) {
        printf(", ");
      }
      if(Abc_LitIsCompl(pCore[i])) {
        printf("!");
      }
      printf("x%d", Abc_Lit2Var(pCore[i]));
    }
    printf("\n");
    int neg_x2_in_core = 0;
    for(int i = 0; i < nSize; i++) {
      if(Abc_LitIsCompl(pCore[i]) &&  Abc_Lit2Var(pCore[i]) == 2) {
        neg_x2_in_core = 1;
      }
    }
    assert(neg_x2_in_core);
    cadical_solver_delete(pSat);
    printf("test 5 passed\n");
  }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END
