#pragma once

#include <map>
#include <iterator>
#include <random>
#include <numeric>

#include "rrrParameter.h"
#include "rrrUtils.h"

ABC_NAMESPACE_CXX_HEADER_START

namespace rrr {

  // it is assumed that removing redundancy never degrades the cost in this optimizer
  template <typename Ntk, typename Ana>
  class Optimizer {
  private:
    // aliases
    using itr = std::vector<int>::iterator;
    using citr = std::vector<int>::const_iterator;
    using ritr = std::vector<int>::reverse_iterator;
    using critr = std::vector<int>::const_reverse_iterator;

    // pointer to network
    Ntk *pNtk;
    
    // parameters
    int nVerbose;
    std::function<double(Ntk *)> CostFunction;
    int nSortTypeOriginal;
    int nSortType;
    int nFlow;
    int nDistance;
    bool fCompatible;
    bool fGreedy;
    seconds nTimeout; // assigned upon Run
    std::string strVerbosePrefix;

    // data
    Ana ana;
    std::mt19937 rng;
    std::vector<int> vTmp;
    std::map<int, std::set<int>> mapNewFanins;
    time_point start;

    // fanin sorting data
    std::vector<int> vRandPiOrder;
    std::vector<double> vRandCosts;

    // marks
    int target;
    std::vector<bool> vTfoMarks;

    // print
    template<typename... Args>
    void Print(int nVerboseLevel, Args... args);
    
    // callback
    void ActionCallback(Action const &action);

    // topology
    void MarkTfo(int id);

    // time
    bool Timeout();

    // sort fanins
    void SetRandPiOrder();
    void SetRandCosts();
    void SortFanins(int id);
    void SortFanins();

    // reduce fanins
    bool ReduceFanins(int id, bool fRemoveUnused = false);
    bool ReduceFaninsOneRandom(int id, bool fRemoveUnused = false);

    // reduce
    bool Reduce();
    void ReduceRandom();

    // remove redundancy
    void RemoveRedundancy();
    void RemoveRedundancyRandom();

    // addition
    template <typename T>
    T SingleAdd(int id, T begin, T end);
    int  MultiAdd(int id, std::vector<int> const &vCands, int nMax = 0);

    // resub
    void SingleResub(int id, std::vector<int> const &vCands);
    void MultiResub(int id, std::vector<int> const &vCands, int nMax = 0);

    // resub stop
    bool SingleResubStop(int id, std::vector<int> const &vCands);
    bool MultiResubStop(int id, std::vector<int> const &vCands, int nMax = 0);

    // multi-target resub
    bool MultiTargetResub(std::vector<int> vTargets, int nMax = 0);
    
    // apply
    void ApplyReverseTopologically(std::function<void(int)> const &func);
    void ApplyRandomlyStop(std::function<bool(int)> const &func);
    void ApplyCombinationRandomlyStop(int k, std::function<bool(std::vector<int> const &)> const &func);
    void ApplyCombinationSampledRandomlyStop(int k, int nSamples, std::function<bool(std::vector<int> const &)> const &func);
    
  public:
    // constructors
    Optimizer(Parameter const *pPar, std::function<double(Ntk *)> CostFunction);
    void UpdateNetwork(Ntk *pNtk_, bool fSame = false);

    // run
    void Run(int iSeed = 0, seconds nTimeout_ = 0);
    
  };

  /* {{{ Print */
  
  template <typename Ntk, typename Ana>
  template <typename... Args>
  inline void Optimizer<Ntk, Ana>::Print(int nVerboseLevel, Args... args) {
    if(nVerbose > nVerboseLevel) {
      std::cout << strVerbosePrefix;
      for(int i = 0; i < nVerboseLevel; i++) {
        std::cout << "\t";
      }
      PrintNext(std::cout, args...);
      std::cout << std::endl;
    }
  }
  
  /* }}} */

  /* {{{ Callback */
  
  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::ActionCallback(Action const &action) {
    if(nVerbose > 2) {
      std::stringstream ss = GetActionDescription(action);
      std::string str;
      std::getline(ss, str);
      Print(2, str);
      while(std::getline(ss, str)) {
        Print(3, str);
      }
    }
    switch(action.type) {
    case REMOVE_FANIN:
      if(action.id != target) {
        target = -1;
      }
      break;
    case REMOVE_UNUSED:
      break;
    case REMOVE_BUFFER:
    case REMOVE_CONST:
      if(action.id == target) {
        target = -1;
      }
      break;
    case ADD_FANIN:
      if(action.id != target) {
        target = -1;
      }
      break;
    case TRIVIAL_COLLAPSE:
      break;
    case TRIVIAL_DECOMPOSE:
      target = -1;
      break;
    case SORT_FANINS:
      break;
    case SAVE:
      break;
    case LOAD:
      //target = -1; // this is not always needed, so do it manually
      break;
    case POP_BACK:
      break;
    default:
      assert(0);
    }
  }

  /* }}} */

  /* {{{ Topology */
  
  template <typename Ntk, typename Ana>
  inline void Optimizer<Ntk, Ana>::MarkTfo(int id) {
    // includes id itself
    if(id == target) {
      return;
    }
    target = id;
    vTfoMarks.clear();
    vTfoMarks.resize(pNtk->GetNumNodes());
    vTfoMarks[id] = true;
    pNtk->ForEachTfo(id, false, [&](int fo) {
      vTfoMarks[fo] = true;
    });
  }

  /* }}} */

  /* {{{ Time */
  
  template <typename Ntk, typename Ana>
  inline bool Optimizer<Ntk, Ana>::Timeout() {
    if(nTimeout) {
      time_point current = GetCurrentTime();
      if(DurationInSeconds(start, current) > nTimeout) {
        return true;
      }
    }
    return false;
  }

  /* }}} */

  /* {{{ Sort fanins */

  template <typename Ntk, typename Ana>
  inline void Optimizer<Ntk, Ana>::SetRandPiOrder() {
    if(int_size(vRandPiOrder) != pNtk->GetNumPis()) {
      vRandPiOrder.clear();
      vRandPiOrder.resize(pNtk->GetNumPis());
      std::iota(vRandPiOrder.begin(), vRandPiOrder.end(), 0);
      std::shuffle(vRandPiOrder.begin(), vRandPiOrder.end(), rng);      
    }    
  }

  template <typename Ntk, typename Ana>
  inline void Optimizer<Ntk, Ana>::SetRandCosts() {
    std::uniform_real_distribution<> dis(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max());
    while(int_size(vRandCosts) < pNtk->GetNumNodes()) {
      vRandCosts.push_back(dis(rng));
    }
  }

  template <typename Ntk, typename Ana>
  inline void Optimizer<Ntk, Ana>::SortFanins(int id) {
    switch(nSortType) {
    case 0: // no sorting
      break;
    case 1: // prioritize internals
      pNtk->SortFanins(id, [&](int i, int j) {
        return !pNtk->IsPi(i) && pNtk->IsPi(j);
      });
      break;
    case 2: // prioritize internals with (reversely) sorted PIs
      pNtk->SortFanins(id, [&](int i, int j) {
        if(pNtk->IsPi(i) && pNtk->IsPi(j)) {
          return pNtk->GetPiIndex(i) > pNtk->GetPiIndex(j);
        }
        return pNtk->IsPi(j);
      });
      break;
    case 3: // prioritize internals with random PI order
      SetRandPiOrder();
      pNtk->SortFanins(id, [&](int i, int j) {
        if(pNtk->IsPi(i) && pNtk->IsPi(j)) {
          return vRandPiOrder[pNtk->GetPiIndex(i)] > vRandPiOrder[pNtk->GetPiIndex(j)];
        }
        return pNtk->IsPi(j);
      });
      break;
    case 4: // smaller fanout takes larger cost
      pNtk->SortFanins(id, [&](int i, int j) {
        return pNtk->GetNumFanouts(i) < pNtk->GetNumFanouts(j);
      });
      break;
    case 5: // fanout + PI
      pNtk->SortFanins(id, [&](int i, int j) {
        if(pNtk->IsPi(i) && !pNtk->IsPi(j)) {
          return false;
        }
        if(!pNtk->IsPi(i) && pNtk->IsPi(j)) {
          return true;
        }
        return pNtk->GetNumFanouts(i) < pNtk->GetNumFanouts(j);
      });
      break;
    case 6: // fanout + sorted PI
      pNtk->SortFanins(id, [&](int i, int j) {
        if(pNtk->IsPi(i) && pNtk->IsPi(j)) {
          return pNtk->GetPiIndex(i) > pNtk->GetPiIndex(j);
        }
        if(pNtk->IsPi(i)) {
          return false;
        }
        if(pNtk->IsPi(j)) {
          return true;
        }
        return pNtk->GetNumFanouts(i) < pNtk->GetNumFanouts(j);
      });
      break;
    case 7: // fanout + random PI
      SetRandPiOrder();
      pNtk->SortFanins(id, [&](int i, int j) {
        if(pNtk->IsPi(i) && pNtk->IsPi(j)) {
          return vRandPiOrder[pNtk->GetPiIndex(i)] > vRandPiOrder[pNtk->GetPiIndex(j)];
        }
        if(pNtk->IsPi(i)) {
          return false;
        }
        if(pNtk->IsPi(j)) {
          return true;
        }
        return pNtk->GetNumFanouts(i) < pNtk->GetNumFanouts(j);
      });
      break;
    case 8: // reverse topological order + PI
      pNtk->SortFanins(id, [&](int i, int j) {
        if(pNtk->IsPi(i) && pNtk->IsPi(j)) {
          return false;
        }
        if(pNtk->IsPi(i)) {
          return false;
        }
        if(pNtk->IsPi(j)) {
          return true;
        }
        return pNtk->GetIntIndex(i) > pNtk->GetIntIndex(j);
      });
      break;
    case 9: // reverse topological order + sorted PI
      pNtk->SortFanins(id, [&](int i, int j) {
        if(pNtk->IsPi(i) && pNtk->IsPi(j)) {
          return pNtk->GetPiIndex(i) > pNtk->GetPiIndex(j);
        }
        if(pNtk->IsPi(i)) {
          return false;
        }
        if(pNtk->IsPi(j)) {
          return true;
        }
        return pNtk->GetIntIndex(i) > pNtk->GetIntIndex(j);
      });
      break;
    case 10: // reverse topological order + random PI
      SetRandPiOrder();
      pNtk->SortFanins(id, [&](int i, int j) {
        if(pNtk->IsPi(i) && pNtk->IsPi(j)) {
          return vRandPiOrder[pNtk->GetPiIndex(i)] > vRandPiOrder[pNtk->GetPiIndex(j)];
        }
        if(pNtk->IsPi(i)) {
          return false;
        }
        if(pNtk->IsPi(j)) {
          return true;
        }
        return pNtk->GetIntIndex(i) > pNtk->GetIntIndex(j);
      });
      break;
    case 11: // topo + fanout + PI
      pNtk->SortFanins(id, [&](int i, int j) {
        if(pNtk->IsPi(i) && !pNtk->IsPi(j)) {
          return false;
        }
        if(!pNtk->IsPi(i) && pNtk->IsPi(j)) {
          return true;
        }
        if(pNtk->GetNumFanouts(i) > pNtk->GetNumFanouts(j)) {
          return false;
        }
        if(pNtk->GetNumFanouts(i) < pNtk->GetNumFanouts(j)) {
          return true;
        }
        if(pNtk->IsPi(i) && pNtk->IsPi(j)) {
          return false;
        }
        return pNtk->GetIntIndex(i) > pNtk->GetIntIndex(j);
      });
      break;
    case 12: // topo + fanout + sorted PI
      pNtk->SortFanins(id, [&](int i, int j) {
        if(pNtk->IsPi(i) && pNtk->IsPi(j)) {
          return pNtk->GetPiIndex(i) > pNtk->GetPiIndex(j);
        }
        if(pNtk->IsPi(i)) {
          return false;
        }
        if(pNtk->IsPi(j)) {
          return true;
        }
        if(pNtk->GetNumFanouts(i) > pNtk->GetNumFanouts(j)) {
          return false;
        }
        if(pNtk->GetNumFanouts(i) < pNtk->GetNumFanouts(j)) {
          return true;
        }
        return pNtk->GetIntIndex(i) > pNtk->GetIntIndex(j);
      });
      break;
    case 13: // topo + fanout + random PI
      SetRandPiOrder();
      pNtk->SortFanins(id, [&](int i, int j) {
        if(pNtk->IsPi(i) && pNtk->IsPi(j)) {
          return vRandPiOrder[pNtk->GetPiIndex(i)] > vRandPiOrder[pNtk->GetPiIndex(j)];
        }
        if(pNtk->IsPi(i)) {
          return false;
        }
        if(pNtk->IsPi(j)) {
          return true;
        }
        if(pNtk->GetNumFanouts(i) > pNtk->GetNumFanouts(j)) {
          return false;
        }
        if(pNtk->GetNumFanouts(i) < pNtk->GetNumFanouts(j)) {
          return true;
        }
        return pNtk->GetIntIndex(i) > pNtk->GetIntIndex(j);
      });
      break;
    case 14: // random order
      SetRandCosts();
      pNtk->SortFanins(id, [&](int i, int j) {
        return vRandCosts[i] > vRandCosts[j];
      });
      break;
    case 15: // random + PI
      SetRandCosts();
      pNtk->SortFanins(id, [&](int i, int j) {
        if(pNtk->IsPi(i) && !pNtk->IsPi(j)) {
          return false;
        }
        if(!pNtk->IsPi(i) && pNtk->IsPi(j)) {
          return true;
        }
        return vRandCosts[i] > vRandCosts[j];
      });
      break;
    case 16: // random + fanout
      SetRandCosts();
      pNtk->SortFanins(id, [&](int i, int j) {
        if(pNtk->GetNumFanouts(i) > pNtk->GetNumFanouts(j)) {
          return false;
        }
        if(pNtk->GetNumFanouts(i) < pNtk->GetNumFanouts(j)) {
          return true;
        }
        return vRandCosts[i] > vRandCosts[j];
      });
      break;
    case 17: // random + fanout + PI
      SetRandCosts();
      pNtk->SortFanins(id, [&](int i, int j) {
        if(pNtk->IsPi(i) && !pNtk->IsPi(j)) {
          return false;
        }
        if(!pNtk->IsPi(i) && pNtk->IsPi(j)) {
          return true;
        }
        if(pNtk->GetNumFanouts(i) > pNtk->GetNumFanouts(j)) {
          return false;
        }
        if(pNtk->GetNumFanouts(i) < pNtk->GetNumFanouts(j)) {
          return true;
        }
        return vRandCosts[i] > vRandCosts[j];
      });
      break;
    default:
      assert(0);
    }
  }
  
  template <typename Ntk, typename Ana>
  inline void Optimizer<Ntk, Ana>::SortFanins() {
    pNtk->ForEachInt([&](int id) {
      SortFanins(id);
    });
  }
  
  /* }}} */
  
  /* {{{ Reduce fanins */
  
  template <typename Ntk, typename Ana>
  inline bool Optimizer<Ntk, Ana>::ReduceFanins(int id, bool fRemoveUnused) {
    assert(pNtk->GetNumFanouts(id) > 0);
    bool fRemoved = false;
    for(int idx = 0; idx < pNtk->GetNumFanins(id); idx++) {
      // skip fanins that were just added
      if(mapNewFanins.count(id)) {
        int fi = pNtk->GetFanin(id, idx);
        if(mapNewFanins[id].count(fi)) {
          continue;
        }
      }
      // reduce
      if(ana.CheckRedundancy(id, idx)) {
        int fi = pNtk->GetFanin(id, idx);
        pNtk->RemoveFanin(id, idx);
        fRemoved = true;
        idx--;
        if(fRemoveUnused && pNtk->GetNumFanouts(fi) == 0) {
          pNtk->RemoveUnused(fi, true);
        }
      }
    }
    return fRemoved;
  }

  /*
  template <typename Ntk, typename Ana>
  inline bool Optimizer<Ntk, Ana>::ReduceFaninsOneRandom(int id, bool fRemoveUnused) {
    assert(pNtk->GetNumFanouts(id) > 0);
    // generate random order
    vTmp.resize(pNtk->GetNumFanins(id));
    std::iota(vTmp.begin(), vTmp.end(), 0);
    std::shuffle(vTmp.begin(), vTmp.end(), rng);
    for(int idx: vTmp) {
      // skip fanins that were just added
      if(mapNewFanins.count(id)) {
        int fi = pNtk->GetFanin(id, idx);
        if(mapNewFanins[id].count(fi)) {
          continue;
        }
      }
      // reduce
      if(ana.CheckRedundancy(id, idx)) {
        int fi = pNtk->GetFanin(id, idx);
        pNtk->RemoveFanin(id, idx);
        if(fRemoveUnused && pNtk->GetNumFanouts(fi) == 0) {
          pNtk->RemoveUnused(fi, true);
        }
        return true;
      }
    }
    return false;
  }
  */
  
  // TODO: add a method to minimize the size of fanins (check singles, pairs, trios, and so on)?
  
  /* }}} */
  
  /* {{{ Reduce */

  template <typename Ntk, typename Ana>
  bool Optimizer<Ntk, Ana>::Reduce() {
    bool fReduced = false;
    std::vector<int> vInts = pNtk->GetInts();
    for(critr it = vInts.rbegin(); it != vInts.rend(); it++) {
      if(!pNtk->IsInt(*it)) {
        continue;
      }
      if(pNtk->GetNumFanouts(*it) == 0) {
        pNtk->RemoveUnused(*it);
        continue;
      }
      fReduced |= ReduceFanins(*it);
      if(pNtk->GetNumFanins(*it) <= 1) {
        pNtk->Propagate(*it);
      }
    }
    return fReduced;
  }

  /*
  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::ReduceRandom() {
    pNtk->Sweep(false);
    std::vector<int> vInts = pNtk->GetInts();
    std::shuffle(vInts.begin(), vInts.end(), rng);
    for(citr it = vInts.begin(); it != vInts.end(); it++) {
      if(!pNtk->IsInt(*it)) {
        continue;
      }
      if(pNtk->GetNumFanouts(*it) == 0) {
        pNtk->RemoveUnused(*it);
        continue;
      }
      ReduceFanins(*it, true);
      if(pNtk->GetNumFanins(*it) <= 1) {
        pNtk->Propagate(*it);
      }
    }
  }
  */

  /* }}} */

  /* {{{ Redundancy removal */
  
  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::RemoveRedundancy() {
    if(fCompatible) {
      while(Reduce()) {
        SortFanins();
      }
      return;
    }
    std::vector<int> vInts = pNtk->GetInts();
    for(critr it = vInts.rbegin(); it != vInts.rend();) {
      if(!pNtk->IsInt(*it)) {
        it++;
        continue;
      }
      if(pNtk->GetNumFanouts(*it) == 0) {
        pNtk->RemoveUnused(*it);
        it++;
        continue;
      }
      SortFanins(*it);
      bool fReduced = ReduceFanins(*it);
      if(pNtk->GetNumFanins(*it) <= 1) {
        pNtk->Propagate(*it);
      }
      if(fReduced) {
        it = vInts.rbegin();
      } else {
        it++;
      }
    }
  }

  /*
  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::RemoveRedundancyRandom() {
    pNtk->Sweep(false);
    std::vector<int> vInts = pNtk->GetInts();
    std::shuffle(vInts.begin(), vInts.end(), rng);
    for(citr it = vInts.begin(); it != vInts.end();) {
      if(!pNtk->IsInt(*it)) {
        it++;
        continue;
      }
      if(pNtk->GetNumFanouts(*it) == 0) {
        pNtk->RemoveUnused(*it);
        it++;
        continue;
      }
      bool fReduced = ReduceFaninsOneRandom(*it, true);
      if(pNtk->GetNumFanins(*it) <= 1) {
        pNtk->Propagate(*it);
      }
      if(fReduced) {
        std::shuffle(vInts.begin(), vInts.end(), rng);
        it = vInts.begin();
      } else {
        it++;
      }
    }
  }
  */

  /* }}} */

  /* {{{ Addition */

  template <typename Ntk, typename Ana>
  template <typename T>
  T Optimizer<Ntk, Ana>::SingleAdd(int id, T begin, T end) {
    MarkTfo(id);
    pNtk->ForEachFanin(id, [&](int fi) {
      vTfoMarks[fi] = true;
    });
    T it = begin;
    for(; it != end; it++) {
      if(!pNtk->IsInt(*it) && !pNtk->IsPi(*it)) {
        continue;
      }
      if(vTfoMarks[*it]) {
        continue;
      }
      if(ana.CheckFeasibility(id, *it, false)) {
        pNtk->AddFanin(id, *it, false);
      } else if(pNtk->UseComplementedEdges() && ana.CheckFeasibility(id, *it, true)) {
        pNtk->AddFanin(id, *it, true);
      } else {
        continue;
      }
      mapNewFanins[id].insert(*it);
      break;
    }
    pNtk->ForEachFanin(id, [&](int fi) {
      vTfoMarks[fi] = false;
    });
    return it;
  }

  template <typename Ntk, typename Ana>
  int Optimizer<Ntk, Ana>::MultiAdd(int id, std::vector<int> const &vCands, int nMax) {
    MarkTfo(id);
    pNtk->ForEachFanin(id, [&](int fi) {
      vTfoMarks[fi] = true;
    });
    int nAdded = 0;
    for(int cand: vCands) {
      if(!pNtk->IsInt(cand) && !pNtk->IsPi(cand)) {
        continue;
      }
      if(vTfoMarks[cand]) {
        continue;
      }
      if(ana.CheckFeasibility(id, cand, false)) {
        pNtk->AddFanin(id, cand, false);
      } else if(pNtk->UseComplementedEdges() && ana.CheckFeasibility(id, cand, true)) {
        pNtk->AddFanin(id, cand, true);
      } else {
        continue;
      }
      mapNewFanins[id].insert(cand);
      nAdded++;
      if(nAdded == nMax) {
        break;
      }
    }
    pNtk->ForEachFanin(id, [&](int fi) {
      vTfoMarks[fi] = false;
    });
    return nAdded;
  }

  /* }}} */
  
  /* {{{ Resub */

  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::SingleResub(int id, std::vector<int> const &vCands) {
    // NOTE: this assumes trivial collapse/decompose does not change cost
    // let us assume the node is not trivially redundant for now
    assert(pNtk->GetNumFanouts(id) != 0);
    assert(pNtk->GetNumFanins(id) > 1);
    // collapse
    pNtk->TrivialCollapse(id);
    // save if wanted
    int slot = -2;
    if(fGreedy) {
      slot = pNtk->Save();
    }
    double dCost = CostFunction(pNtk);
    // main loop
    for(citr it = vCands.begin(); it != vCands.end(); it++) {
      if(Timeout()) {
        break;
      }
      if(!pNtk->IsInt(id)) {
        break;
      }
      it = SingleAdd<citr>(id, it, vCands.end());
      if(it == vCands.end()) {
        break;
      }
      Print(1, "cand", *it, "(", int_distance(vCands.begin(), it) + 1, "/", int_size(vCands), "):", "cost", "=", dCost);
      RemoveRedundancy();
      mapNewFanins.clear();
      double dNewCost = CostFunction(pNtk);
      Print(2, "cost:", dCost, "->", dNewCost);
      if(fGreedy) {
        if(dNewCost <= dCost) {
          pNtk->Save(slot);
          dCost = dNewCost;
        } else {
          pNtk->Load(slot);
        }
      } else {
        dCost = dNewCost;
      }
    }
    if(pNtk->IsInt(id)) {
      pNtk->TrivialDecompose(id);
      SortFanins();
      if(fCompatible) {
        RemoveRedundancy();
      }
    }
    if(fGreedy) {
      pNtk->PopBack();
    }
  }

  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::MultiResub(int id, std::vector<int> const &vCands, int nMax) {
    // NOTE: this assumes trivial collapse/decompose does not change cost
    // let us assume the node is not trivially redundant for now
    assert(pNtk->GetNumFanouts(id) != 0);
    assert(pNtk->GetNumFanins(id) > 1);
    // save if wanted
    int slot = -2;
    if(fGreedy) {
      slot = pNtk->Save();
    }
    double dCost = CostFunction(pNtk);
    // collapse
    pNtk->TrivialCollapse(id);
    // resub
    MultiAdd(id, vCands, nMax);
    RemoveRedundancy();
    mapNewFanins.clear();
    // TODO: we could quit here if nothing has been removed
    RemoveRedundancy();
    double dNewCost = CostFunction(pNtk);
    Print(1, "cost:", dCost, "->", dNewCost);
    if(fGreedy && dNewCost > dCost) {
      pNtk->Load(slot);
    }
    if(pNtk->IsInt(id)) {
      pNtk->TrivialDecompose(id);
      SortFanins();
      if(fCompatible) {
        RemoveRedundancy();
      }
    }
    if(fGreedy) {
      pNtk->PopBack();
    }
  }
  
  /* }}} */

  /* {{{ Resub stop */

  template <typename Ntk, typename Ana>
  bool Optimizer<Ntk, Ana>::SingleResubStop(int id, std::vector<int> const &vCands) {
    // stop whenever structure has changed without increasing cost and return true
    // return false if no candidates are effective
    // NOTE: this assumes trivial collapse/decompose does not change cost
    // let us assume the node is not trivially redundant for now
    assert(pNtk->GetNumFanouts(id) != 0);
    assert(pNtk->GetNumFanins(id) > 1);
    // save before trivial collapse
    int slot0 = pNtk->Save();
    // collapse
    pNtk->TrivialCollapse(id);
    // save after trivial collapse
    int slot = pNtk->Save();
    double dCost = CostFunction(pNtk);
    // remember fanins
    std::set<int> sFanins = pNtk->GetExtendedFanins(id);
    Print(2, "extended fanins:", sFanins);
    // main loop
    for(citr it = vCands.begin(); it != vCands.end(); it++) {
      if(Timeout()) {
        break;
      }
      assert(pNtk->IsInt(id));
      it = SingleAdd<citr>(id, it, vCands.end());
      if(it == vCands.end()) {
        break;
      }
      Print(1, "cand", *it, "(", int_distance(vCands.begin(), it) + 1, "/", int_size(vCands), "):", "cost", "=", dCost);
      RemoveRedundancy();
      mapNewFanins.clear();
      double dNewCost = CostFunction(pNtk);
      Print(2, "cost:", dCost, "->", dNewCost);
      if(dNewCost <= dCost) {
        bool fChanged = false;
        if(dNewCost < dCost || !pNtk->IsInt(id)) {
          fChanged = true;
        } else {
          std::set<int> sNewFanins = pNtk->GetExtendedFanins(id);
          Print(2, "new extended fanins:", sNewFanins);
          if(sFanins != sNewFanins) {
            fChanged = true;
          }
        }
        if(fChanged) {
          if(pNtk->IsInt(id)) {
            pNtk->TrivialDecompose(id);
          }
          pNtk->PopBack(); // slot
          pNtk->PopBack(); // slot0
          return true;
        }
      }
      pNtk->Load(slot);
    }
    pNtk->PopBack(); // slot
    pNtk->Load(slot0);
    pNtk->PopBack(); // slot0
    return false;
  }

  template <typename Ntk, typename Ana>
  bool Optimizer<Ntk, Ana>::MultiResubStop(int id, std::vector<int> const &vCands, int nMax) {
    // if structure has changed without increasing cost, return true
    // otherwise, return false
    // NOTE: this assumes trivial collapse/decompose does not change cost
    // let us assume the node is not trivially redundant for now
    assert(pNtk->GetNumFanouts(id) != 0);
    assert(pNtk->GetNumFanins(id) > 1);
    // save
    int slot = pNtk->Save();
    double dCost = CostFunction(pNtk);
    // remember fanins
    std::set<int> sFanins = pNtk->GetExtendedFanins(id);
    Print(2, "extended fanins:", sFanins);
    // collapse
    pNtk->TrivialCollapse(id);
    // resub
    MultiAdd(id, vCands, nMax);
    RemoveRedundancy();
    mapNewFanins.clear();
    // TODO: we could quit here if nothing has been removed
    RemoveRedundancy();
    double dNewCost = CostFunction(pNtk);
    Print(1, "cost:", dCost, "->", dNewCost);
    if(!fGreedy || dNewCost <= dCost) {
      bool fChanged = false;
      if(dNewCost < dCost || !pNtk->IsInt(id)) {
        fChanged = true;
      } else {
        std::set<int> sNewFanins = pNtk->GetExtendedFanins(id);
        Print(2, "new extended fanins:", sNewFanins);
        if(sFanins != sNewFanins) {
          fChanged = true;
        }
      }
      if(fChanged) {
        if(pNtk->IsInt(id)) {
          pNtk->TrivialDecompose(id);
        }
        pNtk->PopBack();
        return true;
      }
    }
    pNtk->Load(slot);
    pNtk->PopBack();
    return false;
  }
  
  /* }}} */

  /* {{{ Multi-target resub */

  template <typename Ntk, typename Ana>
  bool Optimizer<Ntk, Ana>::MultiTargetResub(std::vector<int> vTargets, int nMax) {
    // save
    int slot = pNtk->Save();
    double dCost = CostFunction(pNtk);
    // remove targets that are trivially collapsed together
    for(int id: vTargets) {
      if(pNtk->IsInt(id)) {
        pNtk->TrivialCollapse(id);
      }
    }
    for(itr it = vTargets.begin(); it != vTargets.end();) {
      if(!pNtk->IsInt(*it)) {
        it = vTargets.erase(it);
      } else {
        it++;
      }
    }
    // add while remembering extended fanins
    Print(1, "targets:", vTargets);
    std::vector<std::set<int>> vsFanins;
    for(int id: vTargets) {
      // get candidates
      std::vector<int> vCands;
      if(nDistance) {
        vCands = pNtk->GetNeighbors(id, true, nDistance);
      } else {
        vCands = pNtk->GetPisInts();
      }
      std::shuffle(vCands.begin(), vCands.end(), rng);
      // remember fanins
      std::set<int> sFanins = pNtk->GetExtendedFanins(id);
      Print(2, "extended fanins:", sFanins);
      vsFanins.push_back(std::move(sFanins));
      // add
      MultiAdd(id, vCands, nMax);
    }
    // reduce
    RemoveRedundancy();
    mapNewFanins.clear();
    // TODO: we could quit here if nothing has been removed
    RemoveRedundancy();
    double dNewCost = CostFunction(pNtk);
    Print(1, "cost:", dCost, "->", dNewCost);
    if(!fGreedy || dNewCost <= dCost) {
      bool fChanged = false;
      if(dNewCost < dCost) {
        fChanged = true;
      } else {
        for(int id: vTargets) {
          if(!pNtk->IsInt(id)) {
            fChanged = true;
            break;
          }
        }
        for(int i = 0; !fChanged && i < int_size(vTargets); i++) {
          std::set<int> sNewFanins = pNtk->GetExtendedFanins(vTargets[i]);
          Print(2, "new extended fanins:", sNewFanins);
          if(vsFanins[i] != sNewFanins) {
            fChanged = true;
          }
        }
      }
      if(fChanged) {
        for(int id: vTargets) {
          if(pNtk->IsInt(id)) {
            pNtk->TrivialDecompose(id);
          }
        }
        pNtk->PopBack();
        return true;
      }
    }
    pNtk->Load(slot);
    pNtk->PopBack();
    return false;
  }
  
  /* }}} */
  
  /* {{{ Apply */

  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::ApplyReverseTopologically(std::function<void(int)> const &func) {
    std::vector<int> vInts = pNtk->GetInts();
    for(critr it = vInts.rbegin(); it != vInts.rend(); it++) {
      if(Timeout()) {
        break;
      }
      if(!pNtk->IsInt(*it)) {
        continue;
      }
      Print(0, "node", *it, "(", int_distance(vInts.crbegin(), it) + 1, "/", int_size(vInts), "):", "cost", "=", CostFunction(pNtk));
      func(*it);
    }
  }

  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::ApplyRandomlyStop(std::function<bool(int)> const &func) {
    std::vector<int> vInts = pNtk->GetInts();
    std::shuffle(vInts.begin(), vInts.end(), rng);
    for(citr it = vInts.begin(); it != vInts.end(); it++) {
      if(Timeout()) {
        break;
      }
      if(!pNtk->IsInt(*it)) {
        continue;
      }
      Print(0, "node", *it, "(", int_distance(vInts.cbegin(), it) + 1, "/", int_size(vInts), "):", "cost", "=", CostFunction(pNtk));
      if(func(*it)) {
        break;
      }
    }
  }

  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::ApplyCombinationRandomlyStop(int k, std::function<bool(std::vector<int> const &)> const &func) {
    std::vector<int> vInts = pNtk->GetInts();
    std::shuffle(vInts.begin(), vInts.end(), rng); // order is decided here, so it's not truely exhaustive
    int nTried = 0;
    int nCombs = k * (k - 1) / 2;
    ForEachCombinationStop(int_size(vInts), k, [&](std::vector<int> const &vIdxs) {
      Print(0, "comb", vIdxs, "(", ++nTried, "/", nCombs, ")");
      assert(int_size(vIdxs) == k);
      if(Timeout()) {
        return true;
      }
      std::vector<int> vTargets(k);
      for(int i = 0; i < k; i++) {
        vTargets[i] = vInts[vIdxs[i]];
      }
      return func(vTargets);
    });
  }
  
  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::ApplyCombinationSampledRandomlyStop(int k, int nSamples, std::function<bool(std::vector<int> const &)> const &func) {
    std::vector<int> vInts = pNtk->GetInts();
    for(int i = 0; i < nSamples; i++) {
      if(Timeout()) {
        break;
      }
      std::set<int> sIdxs;
      while(int_size(sIdxs) < k) {
        int idx = rng() % pNtk->GetNumInts();
        sIdxs.insert(idx);
      }
      std::vector<int> vIdxs(sIdxs.begin(), sIdxs.end());
      std::shuffle(vIdxs.begin(), vIdxs.end(), rng);
      Print(0, "comb", vIdxs, "(", i + 1, "/", nSamples, ")");
      std::vector<int> vTargets(k);
      for(int i = 0; i < k; i++) {
        vTargets[i] = vInts[vIdxs[i]];
      }
      if(func(vTargets)) {
        break;
      }
    }
  }
  
  /* }}} */
  
  /* {{{ Constructors */
  
  template <typename Ntk, typename Ana>
  Optimizer<Ntk, Ana>::Optimizer(Parameter const *pPar, std::function<double(Ntk *)> CostFunction) :
    pNtk(NULL),
    nVerbose(pPar->nOptimizerVerbose),
    CostFunction(CostFunction),
    nSortTypeOriginal(pPar->nSortType),
    nSortType(pPar->nSortType),
    nFlow(pPar->nOptimizerFlow),
    nDistance(pPar->nDistance),
    fCompatible(pPar->fUseBddCspf),
    fGreedy(pPar->fGreedy),
    ana(pPar),
    target(-1) {
  }
  
  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::UpdateNetwork(Ntk *pNtk_, bool fSame) {
    pNtk = pNtk_;
    target = -1;
    pNtk->AddCallback(std::bind(&Optimizer<Ntk, Ana>::ActionCallback, this, std::placeholders::_1));
    ana.UpdateNetwork(pNtk, fSame);
  }
  
  /* }}} */

  /* {{{ Run */

  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::Run(int iSeed, seconds nTimeout_) {
    rng.seed(iSeed);
    vRandPiOrder.clear();
    vRandCosts.clear();
    if(nSortTypeOriginal < 0) {
      nSortType = rng() % 18;
      Print(0, "sorttype =", nSortType);
    }
    nTimeout = nTimeout_;
    start = GetCurrentTime();
    switch(nFlow) {
    case 0:
      RemoveRedundancy();
      ApplyReverseTopologically([&](int id) {
        std::vector<int> vCands;
        if(nDistance) {
          vCands = pNtk->GetNeighbors(id, true, nDistance);
        } else {
          vCands = pNtk->GetPisInts();
        }
        SingleResub(id, vCands);
      });
      break;
    case 1:
      RemoveRedundancy();
      ApplyReverseTopologically([&](int id) {
        std::vector<int> vCands;
        if(nDistance) {
          vCands = pNtk->GetNeighbors(id, true, nDistance);
        } else {
          vCands = pNtk->GetPisInts();
        }
        MultiResub(id, vCands);
      });
      break;
    case 2: {
      RemoveRedundancy();
      double dCost = CostFunction(pNtk);
      while(true) {
        ApplyReverseTopologically([&](int id) {
          std::vector<int> vCands;
          if(nDistance) {
            vCands = pNtk->GetNeighbors(id, true, nDistance);
          } else {
            vCands = pNtk->GetPisInts();
          }
          SingleResub(id, vCands);
        });
        ApplyReverseTopologically([&](int id) {
          std::vector<int> vCands;
          if(nDistance) {
            vCands = pNtk->GetNeighbors(id, true, nDistance);
          } else {
            vCands = pNtk->GetPisInts();
            // vCands = pNtk->GetInts();
          }
          MultiResub(id, vCands);
        });
        double dNewCost = CostFunction(pNtk);
        if(dNewCost < dCost) {
          dCost = dNewCost;
        } else {
          break;
        }
      }
      break;
    }
    case 3: {
      RemoveRedundancy();
      std::vector<int> vCands;
      if(!nDistance) {
        vCands = pNtk->GetPisInts();
        std::shuffle(vCands.begin(), vCands.end(), rng);
      }
      ApplyRandomlyStop([&](int id) {
        if(nDistance) {
          vCands = pNtk->GetNeighbors(id, true, nDistance);
          std::shuffle(vCands.begin(), vCands.end(), rng);
        }
        if(rng() & 1) {
          return SingleResubStop(id, vCands);
        } else {
          return MultiResubStop(id, vCands);
        }
      });
      break;
    }
    case 4: {
      RemoveRedundancy();
      ApplyCombinationSampledRandomlyStop(3, 100, [&](std::vector<int> const &vTargets) {
        return MultiTargetResub(vTargets);
      });
      break;
    }
    default:
      assert(0);
    }
  }
  
  /* }}} */
  
}

ABC_NAMESPACE_CXX_HEADER_END
