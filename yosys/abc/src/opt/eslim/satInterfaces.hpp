/**CFile****************************************************************

  FileName    [satInterfaces.hpp]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Using Exact Synthesis with the SAT-based Local Improvement Method (eSLIM).]

  Synopsis    [Interface to SAT solvers.]

  Author      [Franz-Xaver Reichl]
  
  Affiliation [University of Freiburg]

  Date        [Ver. 1.0. Started - March 2025.]

  Revision    [$Id: satInterfaces.hpp,v 1.00 2025/03/17 00:00:00 Exp $]

***********************************************************************/

#ifndef ABC__OPT__ESLIM__SATINTERFACE_h
#define ABC__OPT__ESLIM__SATINTERFACE_h

#include <vector>
#include <chrono>
#include <iostream>

#include "misc/util/abc_namespaces.h"
#include "misc/vec/vec.h"
#include "sat/kissat/kissat.h"
#include "sat/cadical/cadical.hpp"

ABC_NAMESPACE_CXX_HEADER_START

namespace eSLIM {

  class CadicalSolver {
    public:
     void addClause(int * pLits, int nLits );
     void assume(const std::vector<int>& assumptions);
     int solve(double timeout);
     int solve();   
     Vec_Int_t* getModelVec();
     double getRunTime() const;

    private:
     int val(int variable);
     double last_runtime;
     CaDiCaL::Solver solver;
   
    public: 
     class TimeoutTerminator : public CaDiCaL::Terminator {
      public:
       TimeoutTerminator(double max_runtime);
       bool terminate();
   
      private:
       double max_runtime; //in seconds
       std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();
     };
  };

  class KissatSolver {
    public:
      KissatSolver();
     ~KissatSolver();
     void init(int max_var);
     void addClause(int * pLits, int nLits );
     int solve();
     // int solve(int timeout);
     Vec_Int_t* getModelVec();
   
    private:
   
     int max_var = 0;
     int val(int variable);
     kissat * solver = nullptr;
  };

  inline CadicalSolver::TimeoutTerminator::TimeoutTerminator(double max_runtime) : max_runtime(max_runtime) {}

  inline bool CadicalSolver::TimeoutTerminator::terminate() {
    auto current_time = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = current_time - start;
    return elapsed_seconds.count() > max_runtime;
  }

  inline double CadicalSolver::getRunTime() const {
    return last_runtime;
  }

  inline void CadicalSolver::addClause(int * pLits, int nLits ) {
    for ( int i = 0; i < nLits; i++ ) {
      if (pLits[i] == 0) {
        continue;;
      }
      solver.add(Abc_LitIsCompl(pLits[i]) ? -Abc_Lit2Var(pLits[i]) :  Abc_Lit2Var(pLits[i]));
    }
    solver.add(0);
  }

  inline void CadicalSolver::assume(const std::vector<int>& assumptions) {
    for (auto& l: assumptions) {
      solver.assume(l);
    }
  }

  inline int CadicalSolver::solve() {
    std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();
    int status = solver.solve();
    last_runtime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    return status;
  }

  inline int CadicalSolver::solve(double timeout) {
    std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();
    TimeoutTerminator terminator(timeout);
    solver.connect_terminator(&terminator);
    int status = solver.solve();
    last_runtime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    if (solver.state() == CaDiCaL::INVALID) {
      std::cerr<<"Solver is in invalid state"<<std::endl;
      return -1;
    }
    solver.disconnect_terminator();
    return status;
  }

  inline Vec_Int_t* CadicalSolver::getModelVec() {
    Vec_Int_t* assignment = Vec_IntAlloc( solver.vars() );
    for (int v = 1; v <= solver.vars(); v++) {
      Vec_IntSetEntryFull( assignment, Abc_AbsInt(val(v)), val(v) > 0 );
    }
    return assignment;
  }

  inline int CadicalSolver::val(int variable) {
    auto l = solver.val(variable);
    auto v = abs(l);
    if (v == variable) {
      return l;
    } else {
      return variable;
    }
  }

  inline KissatSolver::KissatSolver() {
    solver = kissat_init();
  }
  
  inline KissatSolver::~KissatSolver() {
    kissat_release(solver);
  }
  
  inline void KissatSolver::init(int max_var) {
    this->max_var = max_var;
    kissat_reserve(solver, max_var);
  }

  inline void KissatSolver::addClause(int * pLits, int nLits ) {
    for ( int i = 0; i < nLits; i++ ) {
      if (pLits[i] == 0) {
        continue;
      }
      kissat_add (solver, Abc_LitIsCompl(pLits[i]) ? -Abc_Lit2Var(pLits[i]) :  Abc_Lit2Var(pLits[i]));
    }
    kissat_add (solver, 0);
  }
  
  inline int KissatSolver::solve() {
    int status = kissat_solve(solver);
    return status;
  }

  inline int KissatSolver::val(int variable) {
    int l = kissat_value (solver, variable);
    auto v = abs(l);
    if (v == variable) {
      return l;
    } else {
      return variable;
    }
  }

  inline Vec_Int_t* KissatSolver::getModelVec() {
    Vec_Int_t* assignment = Vec_IntAlloc( max_var );
    for (int v = 1; v <= max_var; v++) {
      Vec_IntSetEntryFull( assignment, Abc_AbsInt(val(v)), val(v) > 0 );
    }
    return assignment;
  }
}

ABC_NAMESPACE_CXX_HEADER_END

#endif