/**CFile****************************************************************

  FileName    [selectionStrategy.hpp]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Using Exact Synthesis with the SAT-based Local Improvement Method (eSLIM).]

  Synopsis    [Procedures for selecting subcircuits.]

  Author      [Franz-Xaver Reichl]
  
  Affiliation [University of Freiburg]

  Date        [Ver. 1.0. Started - March 2025.]

  Revision    [$Id: selectionStrategy.hpp,v 1.00 2025/03/17 00:00:00 Exp $]

***********************************************************************/

#ifndef ABC__OPT__ESLIM__SELECTIONSTRATEGY_h
#define ABC__OPT__ESLIM__SELECTIONSTRATEGY_h

#include <random>
#include <queue>

#include "misc/util/abc_namespaces.h"
#include "misc/vec/vec.h"
#include "aig/gia/gia.h"

#include "utils.hpp"

ABC_NAMESPACE_CXX_HEADER_START

namespace eSLIM {

  template<typename T>
  class SelectionStrategy {
    public:
      Subcircuit getSubcircuit(); 
      bool getStatus() {return status;};
      void setSeed(int seed);

    protected:
      SelectionStrategy(Gia_Man_t*& gia_man, const eSLIMConfig& cfg, eSLIMLog& log);
      unsigned int getUniformRandomNumber(unsigned int lower, unsigned int upper); // requires lower < upper
      bool getRandomBool();
      int getSubcircuitIO(Vec_Int_t* subcircuit, Vec_Int_t* io);
      // The resulting map assigns inputs of the subcircuit to outputs of the subcircuit s.t. each associated input is reachable from the output
      std::unordered_map<int, std::unordered_set<int>> computeForbiddenPairs(const Subcircuit& subcir);

      Gia_Man_t*& gia_man;
      const eSLIMConfig& cfg;
      eSLIMLog& log;

    private:

      bool filterSubcircuit(const Subcircuit& subcir);
      void forbiddenPairsRec(Gia_Obj_t * pObj, int input, int min_level, std::unordered_map<int, std::unordered_set<int>>& pairs, const std::unordered_map<int,int>& out_ids );

      bool status = true; //set to false if selection fails too often
      static constexpr unsigned int min_size = 2;

      std::mt19937 rng;
      std::bernoulli_distribution bdist;
      std::uniform_int_distribution<std::mt19937::result_type> udistr;

  };

  template<typename T>
  class randomizedBFS : public SelectionStrategy<T> {
    private:
      randomizedBFS(Gia_Man_t*& gia_man, const eSLIMConfig& cfg, eSLIMLog& log);
      Subcircuit findSubcircuit();
      int selectRoot();
      void checkCandidate(Subcircuit& subcircuit, std::queue<int>& candidate, std::queue<int>& rejected, bool add);
      const int nPis;
      const int nPos;
    friend SelectionStrategy<T>;
    friend T;
  };

  class randomizedBFSFP : public randomizedBFS<randomizedBFSFP> {
    public :
      randomizedBFSFP(Gia_Man_t*& gia_man, const eSLIMConfig& cfg, eSLIMLog& log);
    private:
      Subcircuit getSubcircuitImpl(); 
      bool filterSubcircuitImpl(const Subcircuit& subcir);

    friend SelectionStrategy<randomizedBFSFP>;
  };

  class randomizedBFSnoFP : public randomizedBFS<randomizedBFSnoFP> {
    public:
      randomizedBFSnoFP(Gia_Man_t*& gia_man, const eSLIMConfig& cfg, eSLIMLog& log);
    private:
      Subcircuit getSubcircuitImpl() {return findSubcircuit();}; 
      bool filterSubcircuitImpl(const Subcircuit& subcir);
      bool filterSubcircuitRec(Gia_Obj_t * pObj, unsigned int min_level);

    friend SelectionStrategy<randomizedBFSnoFP>;
  };


  template<typename T>
  SelectionStrategy<T>::SelectionStrategy(Gia_Man_t*& gia_man, const eSLIMConfig& cfg, eSLIMLog& log)
                      : gia_man(gia_man), cfg(cfg), log(log), rng(std::random_device()()), bdist(cfg.expansion_probability) {
  }

  template<typename T>
  void SelectionStrategy<T>::setSeed(int seed) {
    rng.seed(seed);
  }

  template<typename T>
  unsigned int SelectionStrategy<T>::getUniformRandomNumber(unsigned int lower, unsigned int upper) {
    udistr.param(std::uniform_int_distribution<std::mt19937::result_type>::param_type(lower, upper));
    return udistr(rng);
  }

  template<typename T>
  bool SelectionStrategy<T>::getRandomBool() {
    return bdist(rng);
  }

  template<typename T>
  randomizedBFS<T>::randomizedBFS(Gia_Man_t*& gia_man, const eSLIMConfig& cfg, eSLIMLog& log)
                  : SelectionStrategy<T>(gia_man, cfg, log), nPis(Gia_ManPiNum(gia_man)), nPos(Gia_ManPoNum(gia_man)) {
  }

  inline randomizedBFSFP::randomizedBFSFP(Gia_Man_t*& gia_man, const eSLIMConfig& cfg, eSLIMLog& log)
                        : randomizedBFS<randomizedBFSFP>(gia_man, cfg, log){
  }

  inline Subcircuit randomizedBFSFP::getSubcircuitImpl() {
    Subcircuit subcir = findSubcircuit();
    subcir.forbidden_pairs = computeForbiddenPairs(subcir);
    return subcir;
  }

  inline randomizedBFSnoFP::randomizedBFSnoFP(Gia_Man_t*& gia_man, const eSLIMConfig& cfg, eSLIMLog& log)
                          : randomizedBFS<randomizedBFSnoFP>(gia_man, cfg, log) {
  }

  inline bool randomizedBFSFP::filterSubcircuitImpl(const Subcircuit& subcir) {
    return true;
  }
}

ABC_NAMESPACE_CXX_HEADER_END

#include "selectionStrategy.tpp"

#endif