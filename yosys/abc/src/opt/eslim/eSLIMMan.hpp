/**CFile****************************************************************

  FileName    [eSLIMMan.hpp]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Using Exact Synthesis with the SAT-based Local Improvement Method (eSLIM).]

  Synopsis    [eSLIM manager.]

  Author      [Franz-Xaver Reichl]
  
  Affiliation [University of Freiburg]

  Date        [Ver. 1.0. Started - March 2025.]

  Revision    [$Id: eSLIMMan.hpp,v 1.00 2025/03/17 00:00:00 Exp $]

***********************************************************************/

#ifndef ABC__OPT__ESLIM__ESLIMMAN_h
#define ABC__OPT__ESLIM__ESLIMMAN_h

#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "misc/util/abc_namespaces.h"
#include "misc/vec/vec.h"
#include "aig/gia/gia.h"
#include "misc/util/utilTruth.h" // ioResub.h depends on utilTruth.h
#include "base/io/ioResub.h"
#include "aig/miniaig/miniaig.h"

#include "utils.hpp"
#include "selectionStrategy.hpp"


ABC_NAMESPACE_CXX_HEADER_START

namespace eSLIM {

  template<class SynthesisEngine, class RelationEngine, class SelectionEngine>
  class eSLIM_Man {

    public:
      static Gia_Man_t* applyeSLIM(Gia_Man_t * p, const eSLIMConfig& cfg, eSLIMLog& log);
    private:

      eSLIM_Man(Gia_Man_t * gia_man, const eSLIMConfig& cfg, eSLIMLog& log);
      Gia_Man_t* getCircuit();

      void minimize();
      void findReplacement();
      Mini_Aig_t* findMinimumAig(const Subcircuit& subcir);

      Vec_Wrd_t* getSimsIn(Abc_RData_t* relation);
      Vec_Wrd_t* getSimsOut(Abc_RData_t* relation);
      word getAllFalseBehaviour(const Subcircuit& subcir);
      bool getAllFalseBehaviourRec(Gia_Obj_t * pObj);

      std::pair<int, Mini_Aig_t*> reduce( Vec_Wrd_t* vSimsDiv, Vec_Wrd_t* vSimsOut, const std::unordered_map<int, std::unordered_set<int>>& forbidden_pairs,
                                          int nVars, int nDivs, int nOuts, int initial_size);
      Mini_Aig_t* computeReplacement( SynthesisEngine& syn_man, int size);
      double getDynamicTimeout(int size);

      void insertReplacement(Mini_Aig_t* replacement, const Subcircuit& subcir);
      std::vector<int> processReplacement(Gia_Man_t* gia_man, Gia_Man_t* pNew, const Subcircuit& subcir, Mini_Aig_t* replacement, std::vector<int>&& to_process, std::vector<int>& replacement_values);
      Vec_Int_t * processEncompassing(Gia_Man_t* gia_man, Gia_Man_t* pNew,  Vec_Int_t* to_process);
      int getInsertionLiteral(Gia_Man_t* gia_man, const Subcircuit& subcir, Mini_Aig_t* replacement, const std::vector<int>& replacement_values, int fanin_lit);

      Gia_Man_t* gia_man = nullptr; 
      const eSLIMConfig& cfg;
      eSLIMLog& log;
      SelectionEngine subcircuit_selection;
      
      double relation_generation_time ;
      double synthesis_time ;

      bool stopeSLIM=false;

  };

  template <typename Y, typename R, typename S>
  inline Gia_Man_t* eSLIM_Man<Y, R, S>::getCircuit() {
    return gia_man;
  }
}

ABC_NAMESPACE_CXX_HEADER_END

#include "eSLIMMan.tpp"

#endif