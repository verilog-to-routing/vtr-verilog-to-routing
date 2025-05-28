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
  class Partitioner {
  private:
    // pointer to network
    Ntk *pNtk;

    // parameters
    int nVerbose;
    int nPartitionSize;
    int nPartitionSizeMin;
    std::string strVerbosePrefix;

    // data
    std::map<Ntk *, std::tuple<std::set<int>, std::vector<int>, std::vector<bool>, std::vector<int>>> mSubNtk2Io;
    std::set<int> sBlocked;
    std::vector<bool> vFailed;

    // print
    template<typename... Args>
    void Print(int nVerboseLevel, Args... args);
    
    // subroutines
    void GetIo(std::set<int> const &sNodes, std::set<int> &sInputs, std::set<int> &sOutputs);
    std::set<int> GetFanouts(std::set<int> const &sNodes, std::set<int> const &sOutputs);
    std::set<int> GetUnblockedNeighborsAndInners(int id, int nRadius);
    Ntk *ExtractDisjoint(int id);
    
  public:
    // constructors
    Partitioner(Parameter const *pPar);
    void UpdateNetwork(Ntk *pNtk);

    // APIs
    Ntk *Extract(int iSeed);
    void Insert(Ntk *pSubNtk);
  };

  /* {{{ Print */
  
  template <typename Ntk>
  template <typename... Args>
  inline void Partitioner<Ntk>::Print(int nVerboseLevel, Args... args) {
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
  void Partitioner<Ntk>::GetIo(std::set<int> const &sNodes, std::set<int> &sInputs, std::set<int> &sOutputs) {
    sInputs.clear();
    sOutputs.clear();
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
  }

  template <typename Ntk>
  std::set<int> Partitioner<Ntk>::GetFanouts(std::set<int> const &sNodes, std::set<int> const &sOutputs) {
    std::set<int> sFanouts;
    for(int id: sOutputs) {
      pNtk->ForEachFanout(id, false, [&](int fo) {
        if(!sNodes.count(fo)) {
          sFanouts.insert(fo);
        }
      });
    }
    return sFanouts;
  }

  template <typename Ntk>
  std::set<int> Partitioner<Ntk>::GetUnblockedNeighborsAndInners(int id, int nRadius) {
    // return empty set on failure
    // get neighbors
    std::vector<int> vNodes = pNtk->GetNeighbors(id, false, nRadius);
    vNodes.push_back(id);
    std::set<int> sNodes(vNodes.begin(), vNodes.end());
    Print(2, "radius", NS(), nRadius, ":", "size =", int_size(sNodes));
    // remove nodes that are already blocked
    for(std::set<int>::iterator it = sNodes.begin(); it != sNodes.end();) {
      if(sBlocked.count(*it)) {
        it = sNodes.erase(it);
      } else {
        it++;
      }
    }
    Print(2, "unblocked:", "size =", int_size(sNodes));
    if(int_size(sNodes) > nPartitionSize) {
      // too large
      return std::set<int>();
    }
    // get tentative partition IO
    std::set<int> sInputs, sOutputs;
    GetIo(sNodes, sInputs, sOutputs);
    Print(3, "nodes:", sNodes);
    Print(3, "inputs:", sInputs);
    Print(3, "outputs:", sOutputs);
    // include inner nodes
    std::set<int> sFanouts = GetFanouts(sNodes, sOutputs);
    std::vector<int> vInners = pNtk->GetInners(sFanouts, sInputs);
    while(!vInners.empty()) {
      Print(2, "inner size =", int_size(vInners));
      // check overlap
      for(int id: vInners) {
        if(sBlocked.count(id)) {
          // overlapped
          return std::set<int>();     
        }
      }
      // expand
      sNodes.insert(vInners.begin(), vInners.end());
      Print(2, "expanded:", "size =", int_size(sNodes));
      if(int_size(sNodes) > nPartitionSize) {
        // too large
        return std::set<int>();
      }
      GetIo(sNodes, sInputs, sOutputs);
      Print(3, "nodes:", sNodes);
      Print(3, "inputs:", sInputs);
      Print(3, "outputs:", sOutputs);
      // recompute fanouts
      sFanouts = GetFanouts(sNodes, sOutputs);
      vInners = pNtk->GetInners(sFanouts, sInputs);
    }
    return sNodes;
  }
  
  template <typename Ntk>
  Ntk *Partitioner<Ntk>::ExtractDisjoint(int id) {
    // collect neighbor
    assert(!sBlocked.count(id));
    int nRadius = 1;
    std::set<int> sNodes = GetUnblockedNeighborsAndInners(id, nRadius);
    Print(1, "radius", NS(), nRadius, ":", "size =", int_size(sNodes));
    if(sNodes.empty()) {
      return NULL;
    }
    std::set<int> sNodesNew = GetUnblockedNeighborsAndInners(id, nRadius + 1);
    Print(1, "radius", NS(), nRadius + 1, ":", "size =", int_size(sNodesNew));
    // gradually increase radius until it hits partition size limit
    while(!sNodesNew.empty()) {
      if(int_size(sNodes) == int_size(sNodesNew)) { // likely already maximum
        break;
      }
      sNodes = sNodesNew;
      nRadius++;
      sNodesNew = GetUnblockedNeighborsAndInners(id, nRadius + 1);
      Print(1, "radius", NS(), nRadius + 1, ":", "size =", int_size(sNodesNew));
    }
    if(int_size(sNodes) < nPartitionSizeMin) {
      // too small
      return NULL;
    }
    std::set<int> sInputs, sOutputs;
    GetIo(sNodes, sInputs, sOutputs);
    /* fall back method by removing TFI of outputs reachable to inputs
    if(!vInners.empty()) {
      for(int id: sOutputs) {
        if(!sNodes.count(id)) { // already removed
          continue;
        }
        std::set<int> sFanouts = GetFanouts(sNodes, sOutputs);
        if(pNtk->IsReachable(sFanouts, sInputs)) {
          Print(2, "node", id, "is reachable");
          sNodes.erase(id);
          pNtk->ForEachTfiEnd(id, sInputs, [&](int fi) {
            int r = sNodes.erase(fi);
            assert(r);
          });
          Print(1, "shrinking:", "size =", int_size(sNodes));
          if(int_size(sNodes) < nPartitionSizeMin) {
            // too small
            return NULL;
          }
          // recompute inputs
          sInputs.clear();
          for(int id: sNodes) {
            pNtk->ForEachFanin(id, [&](int fi) {
              if(!sNodes.count(fi)) {
                sInputs.insert(fi);
              }
            });
          }
          Print(3, "nodes:", sNodes);
          Print(3, "inputs:", sInputs);
        }
      }
      // recompute outputs
      for(std::set<int>::iterator it = sOutputs.begin(); it != sOutputs.end();) {
        if(!sNodes.count(*it)) {
          it = sOutputs.erase(it);
        } else {
          it++;
        }
      }
      Print(3, "new outputs:", sOutputs);
    }
    */
    // ensure outputs of both partitions do not reach each other's inputs at the same time
    for(auto const &entry: mSubNtk2Io) {
      if(!pNtk->IsReachable(sOutputs, std::get<1>(entry.second))) {
        continue;
      }
      if(!pNtk->IsReachable(std::get<3>(entry.second), sInputs)) {
        continue;
      }
      // if(nVerbose) {
      //   std::cout << "POTENTIAL LOOPS" << std::endl;
      // }
      return NULL;
    }
    assert(int_size(sNodes) <= nPartitionSize);
    assert(int_size(sNodes) >= nPartitionSizeMin);
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
  Partitioner<Ntk>::Partitioner(Parameter const *pPar) :
    nVerbose(pPar->nPartitionerVerbose),
    nPartitionSize(pPar->nPartitionSize),
    nPartitionSizeMin(pPar->nPartitionSizeMin) {
  }

  template <typename Ntk>
  void Partitioner<Ntk>::UpdateNetwork(Ntk *pNtk_) {
    pNtk = pNtk_;
    assert(mSubNtk2Io.empty());
    assert(sBlocked.empty());
    vFailed.clear();
  }

  /* }}} */

  /* {{{ APIs */
  
  template <typename Ntk>
  Ntk *Partitioner<Ntk>::Extract(int iSeed) {
    // pick a center node from candidates that do not belong to any other ongoing partitions
    vFailed.resize(pNtk->GetNumNodes());
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
        Ntk *pSubNtk = ExtractDisjoint(id);
        if(pSubNtk) {
          return pSubNtk;
        }
      }
      vFailed[id] = true;
    }
    return NULL;
  }

  template <typename Ntk>
  void Partitioner<Ntk>::Insert(Ntk *pSubNtk) {
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
  }

  /* }}} */
  
}

ABC_NAMESPACE_CXX_HEADER_END
