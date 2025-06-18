#pragma once

#include "rrrParameter.h"
#include "rrrUtils.h"

ABC_NAMESPACE_CXX_HEADER_START

namespace rrr {

  template <typename Ntk, typename Sim, typename Sol>
  class Analyzer {
  private:
    // pointer to network
    Ntk *pNtk;

    // parameters
    bool nVerbose;

    // data
    Sim sim;
    Sol sol;

  public:
    // constructors
    Analyzer(Parameter const *pPar);
    void UpdateNetwork(Ntk *pNtk_, bool fSame);

    // checks
    bool CheckRedundancy(int id, int idx);
    bool CheckFeasibility(int id, int fi, bool c);
  };

  /* {{{ Constructors */
  
  template <typename Ntk, typename Sim, typename Sol>
  Analyzer<Ntk, Sim, Sol>::Analyzer(Parameter const *pPar) :
    pNtk(NULL),
    nVerbose(pPar->nAnalyzerVerbose),
    sim(pPar),
    sol(pPar) {
  }
  
  template <typename Ntk, typename Sim, typename Sol>
  void Analyzer<Ntk, Sim, Sol>::UpdateNetwork(Ntk *pNtk_, bool fSame) {
    pNtk = pNtk_;
    sim.UpdateNetwork(pNtk, fSame);
    sol.UpdateNetwork(pNtk, fSame);
  }

  /* }}} */

  /* {{{ Checks */
  
  template <typename Ntk, typename Sim, typename Sol>
  bool Analyzer<Ntk, Sim, Sol>::CheckRedundancy(int id, int idx) {
    if(sim.CheckRedundancy(id, idx)) {
      if(nVerbose) {
        std::cout << "node " << id << " fanin " << (pNtk->GetCompl(id, idx)? "!": "") << pNtk->GetFanin(id, idx) << " index " << idx << " seems redundant" << std::endl;
      }
      SatResult r = sol.CheckRedundancy(id, idx);
      if(r == UNSAT) {
        if(nVerbose) {
          std::cout << "node " << id << " fanin " << (pNtk->GetCompl(id, idx)? "!": "") << pNtk->GetFanin(id, idx) << " index " << idx << " is redundant" << std::endl;
        }
        return true;
      }
      if(r == SAT) {
        if(nVerbose) {
          std::cout << "node " << id << " fanin " << (pNtk->GetCompl(id, idx)? "!": "") << pNtk->GetFanin(id, idx) << " index " << idx << " is NOT redundant" << std::endl;
        }
        sim.AddCex(sol.GetCex());
      }
    }
    return false;
  }
  
  template <typename Ntk, typename Sim, typename Sol>
  bool Analyzer<Ntk, Sim, Sol>::CheckFeasibility(int id, int fi, bool c) {
    if(sim.CheckFeasibility(id, fi, c)) {
      if(nVerbose) {
        std::cout << "node " << id << " fanin " << (c? "!": "") << fi << " seems feasible" << std::endl;
      }
      SatResult r = sol.CheckFeasibility(id, fi, c);
      if(r == UNSAT) {
        if(nVerbose) {
          std::cout << "node " << id << " fanin " << (c? "!": "") << fi << " is feasible" << std::endl;
        }
        return true;
      }
      if(r == SAT) {
        if(nVerbose) {
          std::cout << "node " << id << " fanin " << (c? "!": "") << fi << " is NOT feasible" << std::endl;
        }
        sim.AddCex(sol.GetCex());
      }
    }
    return false;
  }

  /* }}} */
  
}

ABC_NAMESPACE_CXX_HEADER_END
