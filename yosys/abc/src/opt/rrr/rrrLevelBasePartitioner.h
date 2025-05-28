#pragma once

#include <utility>
#include <map>
#include <tuple>
#include <random>

#include "rrrParameter.h"
#include "rrrUtils.h"

ABC_NAMESPACE_CXX_HEADER_START

namespace rrr {

  template <typename Ntk>
  class LevelBasePartitioner {
  private:
    // pointer to network
    Ntk *pNtk;

    // parameters
    int nVerbose;
    int nPartitionSize;
    int nPartitionSizeMin;
    std::string strVerbosePrefix;

    // data
    int nMaxLevel;
    std::vector<int> vLevels;
    std::map<Ntk *, std::tuple<std::set<int>, std::vector<int>, std::vector<bool>, std::vector<int>>> mSubNtk2Io;
    std::set<int> sBlocked;
    std::vector<bool> vFailed;

    // print
    template<typename... Args>
    void Print(int nVerboseLevel, Args... args);
    
    // subroutines
    void ComputeLevel();
    std::vector<int> GetIOI(int id, int nLevels);
    Ntk *ExtractIOI(int id);
    
  public:
    // constructors
    LevelBasePartitioner(Parameter const *pPar);
    void UpdateNetwork(Ntk *pNtk);

    // APIs
    Ntk *Extract(int iSeed);
    void Insert(Ntk *pSubNtk);
  };

  /* {{{ Print */
  
  template <typename Ntk>
  template <typename... Args>
  inline void LevelBasePartitioner<Ntk>::Print(int nVerboseLevel, Args... args) {
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

  /* {{{ Subroutines */

  template <typename Ntk>
  void LevelBasePartitioner<Ntk>::ComputeLevel() {
    nMaxLevel = 0;
    vLevels.clear();
    vLevels.resize(pNtk->GetNumNodes());
    pNtk->ForEachInt([&](int id) {
      pNtk->ForEachFanin(id, [&](int fi) {
        if(vLevels[id] < vLevels[fi]) {
          vLevels[id] = vLevels[fi];
        }
      });
      vLevels[id] += 1;
      if(nMaxLevel < vLevels[id]) {
        nMaxLevel = vLevels[id];
      }
    });
  }

  template <typename Ntk>
  std::vector<int> LevelBasePartitioner<Ntk>::GetIOI(int id, int nLevels) {
    std::vector<int> vNodes, vNodes2;
    vNodes.push_back(id);
    pNtk->ForEachTfiUpdate(id, false, [&](int fi) {
      if(vLevels[fi] < vLevels[id] - nLevels) {
        return false;
      }
      vNodes.push_back(fi);
      return true;
    });
    pNtk->ForEachTfosUpdate(vNodes, false, [&](int fo) {
      if(vLevels[fo] > vLevels[id] + nLevels) {
        return false;
      }
      vNodes2.push_back(fo);
      return true;
    });
    vNodes.clear();
    pNtk->ForEachTfisUpdate(vNodes2, false, [&](int fi) {
      if(vLevels[fi] < vLevels[id] - nLevels) {
        return false;
      }
      vNodes.push_back(fi);
      return true;
    });
    return vNodes;
  }

  template <typename Ntk>
  Ntk *LevelBasePartitioner<Ntk>::ExtractIOI(int id) {
    // collect IOI nodes
    assert(!sBlocked.count(id));
    int nLevels = 1;
    std::vector<int> vNodes = GetIOI(id, nLevels);
    Print(1, "level", NS(), nLevels, ":", "size =", int_size(vNodes));
    std::vector<int> vNodesNew = GetIOI(id, nLevels + 1);
    Print(1, "level", NS(), nLevels + 1, ":", "size =", int_size(vNodesNew));
    // gradually increase level until it hits partition size limit
    while(int_size(vNodesNew) < nPartitionSize) {
      if(vLevels[id] - nLevels < 1 && vLevels[id] + nLevels >= nMaxLevel) { // already maximum
        break;
      }
      vNodes = vNodesNew;
      nLevels++;
      vNodesNew = GetIOI(id, nLevels + 1);
      Print(1, "level", NS(), nLevels + 1, ":", "size =", int_size(vNodesNew));
    }
    std::set<int> sNodes(vNodes.begin(), vNodes.end());
    // remove nodes that are already blocked
    for(std::set<int>::iterator it = sNodes.begin(); it != sNodes.end();) {
      if(sBlocked.count(*it)) {
        it = sNodes.erase(it);
      } else {
        it++;
      }
    }
    Print(1, "checking:", "size =", int_size(sNodes));
    // get partition IO
    std::set<int> sInputs, sOutputs;
    for(int id: sNodes) {
      pNtk->ForEachFanin(id, [&](int fi) {
        if(!sNodes.count(fi)) {
          sInputs.insert(fi);
        }
      });
      bool fOutput = false;
      pNtk->ForEachFanout(id, true, [&](int fo) {
        if(!sNodes.count(fo)) {
          fOutput = true;
        }
      });
      if(fOutput) {
        sOutputs.insert(id);
      }
    }
    Print(2, "nodes:", sNodes);
    Print(2, "inputs:", sInputs);
    Print(2, "outputs:", sOutputs);
    if(int_size(sNodes) < nPartitionSizeMin) {
      return NULL;
    }
    // check loops and just give up if any
    std::set<int> sFanouts;
    for(int id: sOutputs) {
      pNtk->ForEachFanout(id, false, [&](int fo) {
        if(!sNodes.count(fo)) {
          sFanouts.insert(fo);
        }
      });
    }
    if(pNtk->IsReachable(sFanouts, sInputs)) {
      return NULL;
    }
    for(auto const &entry: mSubNtk2Io) {
      if(!pNtk->IsReachable(sOutputs, std::get<1>(entry.second))) {
        continue;
      }
      if(!pNtk->IsReachable(std::get<3>(entry.second), sInputs)) {
        continue;
      }
      return NULL;
    }
    // extract by inputs, internals, and outputs (no checks needed in ntk side)
    std::vector<int> vInputs(sInputs.begin(), sInputs.end());
    std::vector<int> vOutputs(sOutputs.begin(), sOutputs.end());
    Ntk *pSubNtk = pNtk->Extract(sNodes, vInputs, vOutputs);
    // return subntk to be modified, while remember IOs
    for(int id: sNodes) {
      sBlocked.insert(id);
    }
    mSubNtk2Io.emplace(pSubNtk, std::make_tuple(std::move(sNodes), std::move(vInputs), std::vector<bool>(vInputs.size()), std::move(vOutputs)));
    return pSubNtk;
  }
  
  /* }}} */

  /* {{{ Constructors */
  
  template <typename Ntk>
  LevelBasePartitioner<Ntk>::LevelBasePartitioner(Parameter const *pPar) :
    nVerbose(pPar->nPartitionerVerbose),
    nPartitionSize(pPar->nPartitionSize),
    nPartitionSizeMin(pPar->nPartitionSizeMin) {
  }

  template <typename Ntk>
  void LevelBasePartitioner<Ntk>::UpdateNetwork(Ntk *pNtk_) {
    pNtk = pNtk_;
    assert(mSubNtk2Io.empty());
    assert(sBlocked.empty());
    vFailed.clear();
    vLevels.clear();
  }

  /* }}} */

  /* {{{ APIs */
  
  template <typename Ntk>
  Ntk *LevelBasePartitioner<Ntk>::Extract(int iSeed) {
    // pick a center node from candidates that do not belong to any other ongoing partitions
    vFailed.resize(pNtk->GetNumNodes());
    if(vLevels.empty()) {
      ComputeLevel();
    }
    std::mt19937 rng(iSeed);
    std::vector<int> vInts = pNtk->GetInts();
    std::shuffle(vInts.begin(), vInts.end(), rng);
    for(int i = 0; i < int_size(vInts); i++) {
      int id = vInts[i];
      if(vFailed[id]) {
        continue;
      }
      if(!sBlocked.count(id)) {
        Print(0, "try partitioning with node", id, "(", i, "/", int_size(vInts), ")");
        Ntk *pSubNtk = ExtractIOI(id);
        if(pSubNtk) {
          return pSubNtk;
        }
      }
      vFailed[id] = true;
    }
    return NULL;
  }

  template <typename Ntk>
  void LevelBasePartitioner<Ntk>::Insert(Ntk *pSubNtk) {
    for(int i: std::get<0>(mSubNtk2Io[pSubNtk])) {
      sBlocked.erase(i);
    }
    std::pair<std::vector<int>, std::vector<bool>> vNewSignals = pNtk->Insert(pSubNtk, std::get<1>(mSubNtk2Io[pSubNtk]), std::get<2>(mSubNtk2Io[pSubNtk]), std::get<3>(mSubNtk2Io[pSubNtk]));
    std::vector<int> &vOldOutputs = std::get<3>(mSubNtk2Io[pSubNtk]);
    std::vector<int> &vNewOutputs = vNewSignals.first;
    std::vector<bool> &vNewCompls = vNewSignals.second;
    // need to remap updated outputs that are used as inputs in other partitions
    std::map<int, int> mOutput2Idx;
    for(int idx = 0; idx < int_size(vOldOutputs); idx++) {
      mOutput2Idx[vOldOutputs[idx]] = idx;
    }
    for(auto &entry: mSubNtk2Io) {
      if(entry.first != pSubNtk) {
        std::vector<int> &vInputs = std::get<1>(entry.second);
        std::vector<bool> &vCompls = std::get<2>(entry.second);
        for(int i = 0; i < int_size(vInputs); i++) {
          if(mOutput2Idx.count(vInputs[i])) {
            int idx = mOutput2Idx[vInputs[i]];
            vInputs[i] = vNewOutputs[idx];
            vCompls[i] = vCompls[i] ^ vNewCompls[idx];
          }
        }
      }
    }
    delete pSubNtk;
    mSubNtk2Io.erase(pSubNtk);
    vFailed.clear(); // clear, there isn't really a way to track
    vLevels.clear();
  }

  /* }}} */
  
}

ABC_NAMESPACE_CXX_HEADER_END
