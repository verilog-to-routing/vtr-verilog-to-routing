#pragma once

#include <utility>
#include <list>
#include <map>
#include <algorithm>

#include "rrrParameter.h"
#include "rrrUtils.h"

ABC_NAMESPACE_CXX_HEADER_START

namespace rrr {

  class AndNetwork {
  private:
    // aliases
    using itr = std::list<int>::iterator;
    using citr = std::list<int>::const_iterator;
    using ritr = std::list<int>::reverse_iterator;
    using critr = std::list<int>::const_reverse_iterator;
    using Callback = std::function<void(Action const &)>;

    // network data
    int nNodes; // number of allocated nodes
    std::vector<int> vPis;
    std::list<int> lInts; // internal nodes in topological order
    std::set<int> sInts; // internal nodes as a set
    std::vector<int> vPos;
    std::vector<std::vector<int>> vvFaninEdges; // complementable edges, no duplicated fanins allowed (including complements), and nodes without fanins are treated as const-1
    std::vector<int> vRefs; // reference count (number of fanouts)

    // mark for network traversal
    bool fLockTrav;
    unsigned iTrav;
    std::vector<unsigned> vTrav;

    // flag used during constant propagation
    bool fPropagating;

    // callback functions
    std::vector<Callback> vCallbacks;

    // network backups
    std::vector<AndNetwork> vBackups;

    // conversion between node and edge
    int  Node2Edge(int id, bool c) const { return (id << 1) + (int)c; }
    int  Edge2Node(int f)          const { return f >> 1;             }
    bool EdgeIsCompl(int f)        const { return f & 1;              }

    // other private functions
    int  CreateNode();
    void SortInts(itr it);
    unsigned StartTraversal(int n = 1);
    void EndTraversal();
    void ForEachTfiRec(int id, std::function<void(int)> const &func);
    void TakenAction(Action const &action) const;

  public:
    // constructors
    AndNetwork();
    AndNetwork(AndNetwork const &x);
    AndNetwork &operator=(AndNetwork const &x);

    // initialization APIs (should not be called after optimization has started)
    void Clear();
    void Reserve(int nReserve);
    int  AddPi();
    int  AddAnd(int id0, int id1, bool c0, bool c1);
    int  AddPo(int id, bool c);
    template <typename Ntk, typename Reader>
    void Read(Ntk *pFrom, Reader &reader);

    // network properties
    bool UseComplementedEdges() const;
    int  GetNumNodes() const; // number of allocated nodes (max id + 1)
    int  GetNumPis() const;
    int  GetNumInts() const;
    int  GetNumPos() const;
    int  GetConst0() const;
    int  GetPi(int idx) const;
    int  GetPo(int idx) const;
    std::vector<int> GetPis() const;
    std::vector<int> GetInts() const;
    std::vector<int> GetPisInts() const;
    std::vector<int> GetPos() const;
    
    // node properties
    bool IsPi(int id) const;
    bool IsInt(int id) const;
    bool IsPo(int id) const;
    NodeType GetNodeType(int id) const;
    bool IsPoDriver(int id) const;
    int  GetPiIndex(int id) const;
    int  GetIntIndex(int id) const;
    int  GetPoIndex(int id) const;
    int  GetNumFanins(int id) const;
    int  GetNumFanouts(int id) const;
    int  GetFanin(int id, int idx) const;
    bool GetCompl(int id, int idx) const;
    int  FindFanin(int id, int fi) const;
    bool IsReconvergent(int id);
    std::vector<int> GetNeighbors(int id, bool fPis, int nHops);
    template <template <typename...> typename Container, typename... Ts, template <typename...> typename Container2, typename... Ts2>
    bool IsReachable(Container<Ts...> const &srcs, Container2<Ts2...> const &dsts);
    template <template <typename...> typename Container, typename... Ts, template <typename...> typename Container2, typename... Ts2>
    std::vector<int> GetInners(Container<Ts...> const &srcs, Container2<Ts2...> const &dsts);
    std::set<int> GetExtendedFanins(int id);

    // network traversal
    void ForEachPi(std::function<void(int)> const &func) const;
    void ForEachPiIdx(std::function<void(int, int)> const &func) const; // func(index, id)
    void ForEachInt(std::function<void(int)> const &func) const;
    void ForEachIntReverse(std::function<void(int)> const &func) const;
    void ForEachPiInt(std::function<void(int)> const &func) const;
    void ForEachPo(std::function<void(int)> const &func) const;
    template <typename Func>
    void ForEachPoDriver(Func const &func) const;
    template <typename Func>
    void ForEachFanin(int id, Func const &func) const;
    template <typename Func>
    void ForEachFaninIdx(int id, Func const &func) const; // func(index of fi in fanin list of id, fi[, c])
    template <typename Func>
    void ForEachFanout(int id, bool fPos, Func const &func) const;
    template <typename Func>
    void ForEachFanoutRidx(int id, bool fPos, Func const &func) const; // func(fo[, c], index of id in fanin list of fo)
    void ForEachTfi(int id, bool fPis, std::function<void(int)> const &func);
    template <template <typename...> typename Container, typename... Ts>
    void ForEachTfiEnd(int id, Container<Ts...> const &ends, std::function<void(int)> const &func);
    void ForEachTfiUpdate(int id, bool fPis, std::function<bool(int)> const &func);
    template <template <typename...> typename Container, typename... Ts>
    void ForEachTfisUpdate(Container<Ts...> const &ids, bool fPis, std::function<bool(int)> const &func);
    void ForEachTfo(int id, bool fPos, std::function<void(int)> const &func);
    void ForEachTfoReverse(int id, bool fPos, std::function<void(int)> const &func);
    void ForEachTfoUpdate(int id, bool fPos, std::function<bool(int)> const &func);
    template <template <typename...> typename Container, typename... Ts>
    void ForEachTfos(Container<Ts...> const &ids, bool fPos, std::function<void(int)> const &func);
    template <template <typename...> typename Container, typename... Ts>
    void ForEachTfosUpdate(Container<Ts...> const &ids, bool fPos, std::function<bool(int)> const &func);

    // extraction
    template <template <typename...> typename Container, typename... Ts>
    AndNetwork *Extract(Container<Ts...> const &ids, std::vector<int> const &vInputs, std::vector<int> const &vOutputs);

    // actions
    void RemoveFanin(int id, int idx);
    void RemoveUnused(int id, bool fRecursive = false, bool fSweeping = false);
    void RemoveBuffer(int id);
    void RemoveConst(int id);
    void AddFanin(int id, int fi, bool c);
    void TrivialCollapse(int id);
    void TrivialDecompose(int id);
    template <typename Func>
    void SortFanins(int id, Func const &cost);
    std::pair<std::vector<int>, std::vector<bool>> Insert(AndNetwork *pNtk, std::vector<int> const &vInputs, std::vector<bool> const &vCompls, std::vector<int> const &vOutputs);

    // Network cleanup
    void Propagate(int id = -1); // all nodes unless specified
    void Sweep(bool fPropagate = true);

    // save & load
    int  Save(int slot = -1); // slot is assigned automatically unless specified
    void Load(int slot);
    void PopBack(); // deletes the last entry of backups

    // misc
    void AddCallback(Callback const &callback);
    void Print() const;
  };

  /* {{{ Private functions */

  inline int AndNetwork::CreateNode() {
    // TODO: reuse already allocated but dead nodes? or perform garbage collection?
    vvFaninEdges.emplace_back();
    vRefs.push_back(0);
    assert(!check_int_max(nNodes));
    return nNodes++;
  }

  void AndNetwork::SortInts(itr it) {
    ForEachFanin(*it, [&](int fi) {
      itr it2 = std::find(it, lInts.end(), fi);
      if(it2 != lInts.end()) {
        lInts.erase(it2);
        it2 = lInts.insert(it, fi);
        SortInts(it2);
      }
    });
  }

  inline unsigned AndNetwork::StartTraversal(int n) {
    assert(!fLockTrav);
    fLockTrav = true;
    do {
      for(int i = 0; i < n; i++) {
        iTrav++;
        if(iTrav == 0) {
          vTrav.clear();
          break;
        }
      }
    } while(iTrav == 0);
    vTrav.resize(nNodes);
    return iTrav - n + 1;
  }
  
  inline void AndNetwork::EndTraversal() {
    assert(fLockTrav);
    fLockTrav = false;
  }

  void AndNetwork::ForEachTfiRec(int id, std::function<void(int)> const &func) {
    for(int fi_edge: vvFaninEdges[id]) {
      int fi = Edge2Node(fi_edge);
      if(vTrav[fi] == iTrav) {
        continue;
      }
      func(fi);
      vTrav[fi] = iTrav;
      ForEachTfiRec(fi, func);
    }
  }

  inline void AndNetwork::TakenAction(Action const &action) const {
    for(Callback const &callback: vCallbacks) {
      callback(action);
    }
  }

  /* }}} */

  /* {{{ Constructors */

  AndNetwork::AndNetwork() :
    nNodes(0),
    fLockTrav(false),
    iTrav(0),
    fPropagating(false) {
    // add constant node
    vvFaninEdges.emplace_back();
    vRefs.push_back(0);
    nNodes++;
  }

  AndNetwork::AndNetwork(AndNetwork const &x) :
    fLockTrav(false),
    iTrav(0),
    fPropagating(false) {
    nNodes       = x.nNodes;
    vPis         = x.vPis;
    lInts        = x.lInts;
    sInts        = x.sInts;
    vPos         = x.vPos;
    vvFaninEdges = x.vvFaninEdges;
    vRefs        = x.vRefs;
  }
  
  AndNetwork &AndNetwork::operator=(AndNetwork const &x) {
    nNodes       = x.nNodes;
    vPis         = x.vPis;
    lInts        = x.lInts;
    sInts        = x.sInts;
    vPos         = x.vPos;
    vvFaninEdges = x.vvFaninEdges;
    vRefs        = x.vRefs;
    return *this;
  }
  
  /* }}} */

  /* {{{ Initialization APIs */

  void AndNetwork::Clear() {
    nNodes = 0;
    vPis.clear();
    lInts.clear();
    sInts.clear();
    vPos.clear();
    vvFaninEdges.clear();
    vRefs.clear();
    fLockTrav = false;
    iTrav = 0;
    vTrav.clear();
    fPropagating = false;
    vCallbacks.clear();
    vBackups.clear();
    // add constant node
    vvFaninEdges.emplace_back();
    vRefs.push_back(0);
    nNodes++;
  }

  void AndNetwork::Reserve(int nReserve) {
    vvFaninEdges.reserve(nReserve);
    vRefs.reserve(nReserve);
  }
  
  inline int AndNetwork::AddPi() {
    vPis.push_back(nNodes);
    vvFaninEdges.emplace_back();
    vRefs.push_back(0);
    assert(!check_int_max(nNodes));
    return nNodes++;
  }
  
  inline int AndNetwork::AddAnd(int id0, int id1, bool c0, bool c1) {
    assert(id0 < nNodes);
    assert(id1 < nNodes);
    assert(id0 != id1);
    lInts.push_back(nNodes);
    sInts.insert(nNodes);
    vRefs[id0]++;
    vRefs[id1]++;
    vvFaninEdges.emplace_back(std::initializer_list<int>({Node2Edge(id0, c0), Node2Edge(id1, c1)}));
    vRefs.push_back(0);
    assert(!check_int_max(nNodes));
    return nNodes++;
  }

  inline int AndNetwork::AddPo(int id, bool c) {
    assert(id < nNodes);
    vPos.push_back(nNodes);
    vRefs[id]++;
    vvFaninEdges.emplace_back(std::initializer_list<int>({Node2Edge(id, c)}));
    vRefs.push_back(0);
    assert(!check_int_max(nNodes));
    return nNodes++;
  }

  template <typename Ntk, typename Reader>
  void AndNetwork::Read(Ntk *pFrom, Reader &reader) {
    Clear();
    reader(pFrom, this);
  }
  
  /* }}} */
  
  /* {{{ Network properties */
  
  inline bool AndNetwork::UseComplementedEdges() const {
    return true;
  }
  
  inline int AndNetwork::GetNumNodes() const {
    return nNodes;
  }

  inline int AndNetwork::GetNumPis() const {
    return int_size(vPis);
  }
  
  inline int AndNetwork::GetNumInts() const {
    return int_size(lInts);
  }
  
  inline int AndNetwork::GetNumPos() const {
    return int_size(vPos);
  }

  inline int AndNetwork::GetConst0() const {
    return 0;
  }
  
  inline int AndNetwork::GetPi(int idx) const {
    return vPis[idx];
  }

  inline int AndNetwork::GetPo(int idx) const {
    return vPos[idx];
  }

  inline std::vector<int> AndNetwork::GetPis() const {
    return vPis;
  }

  inline std::vector<int> AndNetwork::GetInts() const {
    return std::vector<int>(lInts.begin(), lInts.end());
  }
  
  inline std::vector<int> AndNetwork::GetPisInts() const {
    std::vector<int> vPisInts = vPis;
    vPisInts.insert(vPisInts.end(), lInts.begin(), lInts.end());
    return vPisInts;
  }
  
  inline std::vector<int> AndNetwork::GetPos() const {
    return vPos;
  }
  
  /* }}} */

  /* {{{ Node properties */
  
  inline bool AndNetwork::IsPi(int id) const {
    return GetNumFanins(id) == 0 && std::find(vPis.begin(), vPis.end(), id) != vPis.end();
  }

  inline bool AndNetwork::IsInt(int id) const {
    return sInts.count(id);
  }

  inline bool AndNetwork::IsPo(int id) const {
    return GetNumFanouts(id) == 0 && std::find(vPos.begin(), vPos.end(), id) != vPos.end();
  }
  
  inline NodeType AndNetwork::GetNodeType(int id) const {
    if(IsPi(id)) {
      return PI;
    }
    if(IsPo(id)) {
      return PO;
    }
    return AND;
  }

  inline bool AndNetwork::IsPoDriver(int id) const {
    for(int po: vPos) {
      if(GetFanin(po, 0) == id) {
        return true;
      }
    }
    return false;
  }

  inline int AndNetwork::GetPiIndex(int id) const {
    assert(IsPi(id));
    assert(check_int_size(vPis));
    std::vector<int>::const_iterator it = std::find(vPis.begin(), vPis.end(), id);
    assert(it != vPis.end());
    return std::distance(vPis.begin(), it);
  }
  
  inline int AndNetwork::GetIntIndex(int id) const {
    assert(check_int_size(lInts));
    int index = 0;
    citr it = lInts.begin();
    for(; it != lInts.end(); it++) {
      if(*it == id) {
        break;
      }
      index++;
    }
    assert(it != lInts.end());
    return index;
  }

  inline int AndNetwork::GetPoIndex(int id) const {
    assert(IsPo(id));
    assert(check_int_size(vPos));
    std::vector<int>::const_iterator it = std::find(vPos.begin(), vPos.end(), id);
    assert(it != vPos.end());
    return std::distance(vPos.begin(), it);
  }
  
  inline int AndNetwork::GetNumFanins(int id) const {
    return int_size(vvFaninEdges[id]);
  }

  inline int AndNetwork::GetNumFanouts(int id) const {
    return vRefs[id];
  }

  inline int AndNetwork::GetFanin(int id, int idx) const {
    return Edge2Node(vvFaninEdges[id][idx]);
  }

  inline bool AndNetwork::GetCompl(int id, int idx) const {
    return EdgeIsCompl(vvFaninEdges[id][idx]);
  }

  inline int AndNetwork::FindFanin(int id, int fi) const {
    for(int idx = 0; idx < GetNumFanins(id); idx++) {
      if(GetFanin(id, idx) == fi) {
        return idx;
      }
    }
    return -1;
  }
  
  inline bool AndNetwork::IsReconvergent(int id) {
    if(GetNumFanouts(id) <= 1) {
      return false;
    }
    unsigned iTravStart = StartTraversal(GetNumFanouts(id));
    int idx = 0;
    ForEachFanout(id, false, [&](int fo) {
      vTrav[fo] = iTravStart + idx;
      idx++;
    });
    if(idx <= 1) {
      // less than two fanouts excluding POs
      EndTraversal();
      return false;
    }
    citr it = lInts.begin();
    while(vTrav[*it] < iTravStart && it != lInts.end()) {
      it++;
    }
    it++;
    for(; it != lInts.end(); it++) {
      for(int fi_edge: vvFaninEdges[*it]) {
        int fi = Edge2Node(fi_edge);
        if(vTrav[fi] >= iTravStart) {
          if(vTrav[*it] >= iTravStart && vTrav[*it] != vTrav[fi]) {
            EndTraversal();
            return true;
          }
          vTrav[*it] = vTrav[fi];
        }
      }
    }
    EndTraversal();
    return false;
  }

  inline std::vector<int> AndNetwork::GetNeighbors(int id, bool fPis, int nHops) {
    StartTraversal();
    vTrav[id] = iTrav;
    std::vector<int> vPrevs, vNexts;
    vNexts.push_back(id);
    for(int i = 0; i < nHops; i++) {
      vPrevs.swap(vNexts);
      for(int id: vPrevs) {
        ForEachFanin(id, [&](int fi) {
          if(vTrav[fi] != iTrav) {
            vNexts.push_back(fi);
            vTrav[fi] = iTrav;
          }
        });
        ForEachFanout(id, false, [&](int fo) {
          if(vTrav[fo] != iTrav) {
            vNexts.push_back(fo);
            vTrav[fo] = iTrav;
          }
        });
      }
      vPrevs.clear();
    }
    vTrav[id] = 0;
    std::vector<int> v;
    if(fPis) {
      ForEachPiInt([&](int id) {
        if(vTrav[id] == iTrav) {
          v.push_back(id);
        }
      });
    } else {
      ForEachInt([&](int id) {
        if(vTrav[id] == iTrav) {
          v.push_back(id);
        }
      });
    }
    EndTraversal();
    return v;
  }

  template <template <typename...> typename Container, typename... Ts, template <typename...> typename Container2, typename... Ts2>
  inline bool AndNetwork::IsReachable(Container<Ts...> const &srcs, Container2<Ts2...> const &dsts) {
    if(srcs.empty() || dsts.empty()) {
      return false;
    }
    // mark destinations
    unsigned iTravStart = StartTraversal(2);
    for(int id: dsts) {
      vTrav[id] = iTravStart;
    }
    // mark sources
    for(int id: srcs) {
      if(vTrav[id] == iTravStart) {
        EndTraversal();
        return true;
      }
      vTrav[id] = iTrav;
    }
    // find the first source
    citr it = lInts.begin();
    while(vTrav[*it] != iTrav && it != lInts.end()) {
      it++;
    }
    // check if sources are reachable to destinations
    for(; it != lInts.end(); it++) {
      if(vTrav[*it] == iTrav) {
        continue;
      }
      for(int fi_edge: vvFaninEdges[*it]) {
        if(vTrav[Edge2Node(fi_edge)] == iTrav) {
          if(vTrav[*it] == iTravStart) {
            EndTraversal();
            return true;
          }
          vTrav[*it] = iTrav;
          break;
        }
      }
    }
    for(int po: vPos) {
      if(vTrav[po] == iTrav) {
        continue;
      }
      if(vTrav[GetFanin(po, 0)] == iTrav) {
        if(vTrav[po] == iTravStart) {
          EndTraversal();
          return true;
        }
        vTrav[po] = iTrav;
      }
    }
    EndTraversal();
    return false;
  }

  template <template <typename...> typename Container, typename... Ts, template <typename...> typename Container2, typename... Ts2>
  inline std::vector<int> AndNetwork::GetInners(Container<Ts...> const &srcs, Container2<Ts2...> const &dsts) {
    // this includes sources and destinations that are connected
    if(srcs.empty() || dsts.empty()) {
      return std::vector<int>();
    }
    unsigned iTravStart = StartTraversal(4);
    unsigned iDst = iTravStart;
    unsigned iTfo = iTravStart + 1;
    unsigned iInner = iTravStart + 2;
    // mark destinations (to prevent nodes between destinations to sources being included)
    for(int id: dsts) {
      vTrav[id] = iDst;
    }
    // mark TFOs of sources until destinations, which will be marekd as inner
    for(int id: srcs) {
      if(vTrav[id] == iDst) {
        vTrav[id] = iInner;
      } else {
        vTrav[id] = iTfo;
      }
    }
    citr it = lInts.begin();
    while(vTrav[*it] != iTfo && it != lInts.end()) {
      it++;
    }
    for(; it != lInts.end(); it++) {
      if(vTrav[*it] >= iTfo) { // TFO or inner
        continue;
      }
      for(int fi_edge: vvFaninEdges[*it]) {
        if(vTrav[Edge2Node(fi_edge)] == iTfo) {
          if(vTrav[*it] == iDst) {
            vTrav[*it] = iInner;
          } else {
            vTrav[*it] = iTfo;
          }
          break;
        }
      }
    }
    // traverse TFIs of connected destinations
    std::vector<int> vInners;
    for(int id: dsts) {
      if(vTrav[id] == iInner) {
        vInners.push_back(id);
        vTrav[id] = iTrav;
        ForEachTfiRec(id, [&](int fi) {
          if(vTrav[fi] == iTfo || vTrav[fi] == iInner) {
            vInners.push_back(fi);
          }
        });
      }
    }
    EndTraversal();
    return vInners;
  }

  inline std::set<int> AndNetwork::GetExtendedFanins(int id) {
    // go to the root of trivially collapsable nodes
    while(GetNumFanouts(id) == 1) {
      int id_new = -1;
      ForEachFanout(id, false, [&](int fo, bool c) {
        if(!c) {
          id_new = fo;
        }
      });
      if(id_new != -1) {
        id = id_new;
      } else {
        break;
      }
    }
    // emulate trivial collapse
    std::vector<int> vFaninEdges = vvFaninEdges[id];
    for(int idx = 0; idx < int_size(vFaninEdges);) {
      int fi_edge = vFaninEdges[idx];
      int fi = Edge2Node(fi_edge);
      bool c = EdgeIsCompl(fi_edge);
      if(!IsPi(fi) && !c && vRefs[fi] == 1) {
        std::vector<int>::iterator it = vFaninEdges.begin() + idx;
        it = vFaninEdges.erase(it);
        vFaninEdges.insert(it, vvFaninEdges[fi].begin(), vvFaninEdges[fi].end());
      } else {
        idx++;
      }
    }
    // create set
    std::set<int> sFanins;
    for(int fi_edge: vFaninEdges) {
      sFanins.insert(Edge2Node(fi_edge));
    }
    return sFanins;
  }

  /* }}} */

  /* {{{ Network traversal */

  inline void AndNetwork::ForEachPi(std::function<void(int)> const &func) const {
    for(int pi: vPis) {
      func(pi);
    }
  }

  inline void AndNetwork::ForEachPiIdx(std::function<void(int, int)> const &func) const {
    for(int idx = 0; idx < GetNumPis(); idx++) {
      func(idx, GetPi(idx));
    }
  }

  inline void AndNetwork::ForEachInt(std::function<void(int)> const &func) const {
    for(int id: lInts) {
      func(id);
    }
  }

  inline void AndNetwork::ForEachIntReverse(std::function<void(int)> const &func) const {
    for(critr it = lInts.rbegin(); it != lInts.rend(); it++) {
      func(*it);
    }
  }

  inline void AndNetwork::ForEachPiInt(std::function<void(int)> const &func) const {
    for(int pi: vPis) {
      func(pi);
    }
    for(int id: lInts) {
      func(id);
    }
  }
  
  inline void AndNetwork::ForEachPo(std::function<void(int)> const &func) const {
    for(int po: vPos) {
      func(po);
    }
  }

  template <typename Func>
  inline void AndNetwork::ForEachPoDriver(Func const &func) const {
    static_assert(is_invokable<Func, int>::value || is_invokable<Func, int, bool>::value, "for each edge function format error");
    for(int po: vPos) {
      if constexpr(is_invokable<Func, int>::value) {
        func(GetFanin(po, 0));
      } else if constexpr(is_invokable<Func, int, bool>::value) {
        func(GetFanin(po, 0), GetCompl(po, 0));
      }
    }
  }

  template <typename Func>
  inline void AndNetwork::ForEachFanin(int id, Func const &func) const {
    static_assert(is_invokable<Func, int>::value || is_invokable<Func, int, bool>::value, "for each edge function format error");
    for(int fi_edge: vvFaninEdges[id]) {
      if constexpr(is_invokable<Func, int>::value) {
        func(Edge2Node(fi_edge));
      } else if constexpr(is_invokable<Func, int, bool>::value) {
        func(Edge2Node(fi_edge), EdgeIsCompl(fi_edge));
      }
    }
  }

  template <typename Func>
  inline void AndNetwork::ForEachFaninIdx(int id, Func const &func) const {
    static_assert(is_invokable<Func, int, int>::value || is_invokable<Func, int, int, bool>::value, "for each edge function format error");
    for(int idx = 0; idx < GetNumFanins(id); idx++) {
      if constexpr(is_invokable<Func, int, int>::value) {
        func(idx, GetFanin(id, idx));
      } else if constexpr(is_invokable<Func, int, int, bool>::value) {
        func(idx, GetFanin(id, idx), GetCompl(id, idx));
      }
    }
  }
  
  template <typename Func>
  inline void AndNetwork::ForEachFanout(int id, bool fPos, Func const &func) const {
    static_assert(is_invokable<Func, int>::value || is_invokable<Func, int, bool>::value, "for each edge function format error");
    if(vRefs[id] == 0) {
      return;
    }
    citr it = lInts.begin();
    if(IsInt(id)) {
      it = std::find(it, lInts.end(), id);
      assert(it != lInts.end());
      it++;
    }
    int nRefs = vRefs[id];
    for(; nRefs != 0 && it != lInts.end(); it++) {
      int idx = FindFanin(*it, id);
      if(idx >= 0) {
        if constexpr(is_invokable<Func, int>::value) {
          func(*it);
        } else if constexpr(is_invokable<Func, int, bool>::value) {
          func(*it, GetCompl(*it, idx));
        }
        nRefs--;
      }
    }
    if(fPos && nRefs != 0) {
      for(int po: vPos) {
        if(GetFanin(po, 0) == id) {
          if constexpr(is_invokable<Func, int>::value) {
            func(po);
          } else if constexpr(is_invokable<Func, int, bool>::value) {
            func(po, GetCompl(po, 0));
          }
          nRefs--;
          if(nRefs == 0) {
            break;
          }
        }
      }
    }
    assert(!fPos || nRefs == 0);
  }

  template <typename Func>
  inline void AndNetwork::ForEachFanoutRidx(int id, bool fPos, Func const &func) const {
    static_assert(is_invokable<Func, int, int>::value || is_invokable<Func, int, bool, int>::value, "for each edge function format error");
    if(vRefs[id] == 0) {
      return;
    }
    citr it = lInts.begin();
    if(IsInt(id)) {
      it = std::find(it, lInts.end(), id);
      assert(it != lInts.end());
      it++;
    }
    int nRefs = vRefs[id];
    for(; nRefs != 0 && it != lInts.end(); it++) {
      int idx = FindFanin(*it, id);
      if(idx >= 0) {
        if constexpr(is_invokable<Func, int, int>::value) {
          func(*it, idx);
        } else if constexpr(is_invokable<Func, int, bool, int>::value) {
          func(*it, GetCompl(*it, idx), idx);
        }
        nRefs--;
      }
    }
    if(fPos && nRefs != 0) {
      for(int po: vPos) {
        if(GetFanin(po, 0) == id) {
          if constexpr(is_invokable<Func, int, int>::value) {
            func(po, 0);
          } else if constexpr(is_invokable<Func, int, bool, int>::value) {
            func(po, GetCompl(po, 0), 0);
          }
          nRefs--;
          if(nRefs == 0) {
            break;
          }
        }
      }
    }
    assert(!fPos || nRefs == 0);
  }

  inline void AndNetwork::ForEachTfi(int id, bool fPis, std::function<void(int)> const &func) {
    // this does not include id itself
    StartTraversal();
    if(!fPis) {
      for(int pi: vPis) {
        vTrav[pi] = iTrav;
      }
    }
    ForEachTfiRec(id, func);
    EndTraversal();
  }

  template <template <typename...> typename Container, typename... Ts>
  inline void AndNetwork::ForEachTfiEnd(int id, Container<Ts...> const &ends, std::function<void(int)> const &func) {
    // this does not include id itself
    StartTraversal();
    for(int end: ends) {
      vTrav[end] = iTrav;
    }
    ForEachTfiRec(id, func);
    EndTraversal();
  }

  inline void AndNetwork::ForEachTfiUpdate(int id, bool fPis, std::function<bool(int)> const &func) {
    if(GetNumFanins(id) == 0) {
      return;
    }
    StartTraversal();
    for(int fi_edge: vvFaninEdges[id]) {
      vTrav[Edge2Node(fi_edge)] = iTrav;
    }
    critr it = std::find(lInts.rbegin(), lInts.rend(), id);
    assert(it != lInts.rend());
    it++;
    for(; it != lInts.rend(); it++) {
      if(vTrav[*it] == iTrav) {
        if(func(*it)) {
          for(int fi_edge: vvFaninEdges[*it]) {
            vTrav[Edge2Node(fi_edge)] = iTrav;
          }
        }
      }
    }
    if(fPis) {
      for(int pi: vPis) {
        if(vTrav[pi] == iTrav) {
          func(pi);
        }
      }
    }
    EndTraversal();
  }

  template <template <typename...> typename Container, typename... Ts>
  inline void AndNetwork::ForEachTfisUpdate(Container<Ts...> const &ids, bool fPis, std::function<bool(int)> const &func) {
    // this includes ids themselves
    StartTraversal();
    for(int id: ids) {
      vTrav[id] = iTrav;
    }
    critr it = lInts.rbegin();
    while(vTrav[*it] != iTrav && it != lInts.rend()) {
      it++;
    }
    for(; it != lInts.rend(); it++) {
      if(vTrav[*it] == iTrav) {
        if(func(*it)) {
          for(int fi_edge: vvFaninEdges[*it]) {
            vTrav[Edge2Node(fi_edge)] = iTrav;
          }
        }
      }
    }
    if(fPis) {
      for(int pi: vPis) {
        if(vTrav[pi] == iTrav) {
          func(pi);
        }
      }
    }
    EndTraversal();
  }

  inline void AndNetwork::ForEachTfo(int id, bool fPos, std::function<void(int)> const &func) {
    // this does not include id itself
    if(vRefs[id] == 0) {
      return;
    }
    StartTraversal();
    vTrav[id] = iTrav;
    citr it = std::find(lInts.begin(), lInts.end(), id);
    assert(it != lInts.end());
    it++;
    for(; it != lInts.end(); it++) {
      for(int fi_edge: vvFaninEdges[*it]) {
        if(vTrav[Edge2Node(fi_edge)] == iTrav) {
          func(*it);
          vTrav[*it] = iTrav;
          break;
        }
      }
    }
    if(fPos) {
      for(int po: vPos) {
        if(vTrav[GetFanin(po, 0)] == iTrav) {
          func(po);
          vTrav[po] = iTrav;
        }
      }
    }
    EndTraversal();
  }

  inline void AndNetwork::ForEachTfoReverse(int id, bool fPos, std::function<void(int)> const &func) {
    // this does not include id itself
    if(vRefs[id] == 0) {
      return;
    }
    StartTraversal();
    vTrav[id] = iTrav;
    citr it = std::find(lInts.begin(), lInts.end(), id);
    assert(it != lInts.end());
    it++;
    for(; it != lInts.end(); it++) {
      for(int fi_edge: vvFaninEdges[*it]) {
        if(vTrav[Edge2Node(fi_edge)] == iTrav) {
          vTrav[*it] = iTrav;
          break;
        }
      }
    }
    if(fPos) {
      for(int po: vPos) {
        if(vTrav[GetFanin(po, 0)] == iTrav) {
          vTrav[po] = iTrav;
        }
      }
    }
    EndTraversal(); // release here so func can call IsReconvergent
    unsigned iTravTfo = iTrav;
    if(fPos) {
      // use reverse order even for POs
      for(std::vector<int>::const_reverse_iterator it = vPos.rbegin(); it != vPos.rend(); it++) {
        assert(vTrav[*it] <= iTravTfo); // make sure func does not touch vTrav of preceding nodes
        if(vTrav[*it] == iTravTfo) {
          func(*it);
        }
      }
    }
    for(critr it = lInts.rbegin(); *it != id; it++) {
      assert(vTrav[*it] <= iTravTfo); // make sure func does not touch vTrav of preceding nodes
      if(vTrav[*it] == iTravTfo) {
        func(*it);
      }
    }
  }

  inline void AndNetwork::ForEachTfoUpdate(int id, bool fPos, std::function<bool(int)> const &func) {
    // this does not include id itself
    if(vRefs[id] == 0) {
      return;
    }
    StartTraversal();
    vTrav[id] = iTrav;
    citr it = std::find(lInts.begin(), lInts.end(), id);
    assert(it != lInts.end());
    it++;
    for(; it != lInts.end(); it++) {
      for(int fi_edge: vvFaninEdges[*it]) {
        if(vTrav[Edge2Node(fi_edge)] == iTrav) {
          if(func(*it)) {
            vTrav[*it] = iTrav;
          }
          break;
        }
      }
    }
    if(fPos) {
      for(int po: vPos) {
        if(vTrav[GetFanin(po, 0)] == iTrav) {
          if(func(po)) {
            vTrav[po] = iTrav;
          }
        }
      }
    }
    EndTraversal();
  }

  template <template <typename...> typename Container, typename... Ts>
  inline void AndNetwork::ForEachTfos(Container<Ts...> const &ids, bool fPos, std::function<void(int)> const &func) {
    // this includes ids themselves
    StartTraversal();
    for(int id: ids) {
      vTrav[id] = iTrav;
    }
    citr it = lInts.begin();
    while(vTrav[*it] != iTrav && it != lInts.end()) {
      it++;
    }
    for(; it != lInts.end(); it++) {
      if(vTrav[*it] == iTrav) {
        func(*it);
      } else {
        for(int fi_edge: vvFaninEdges[*it]) {
          if(vTrav[Edge2Node(fi_edge)] == iTrav) {
            func(*it);
            vTrav[*it] = iTrav;
            break;
          }
        }
      }
    }
    if(fPos) {
      for(int po: vPos) {
        if(vTrav[po] == iTrav || vTrav[GetFanin(po, 0)] == iTrav) {
          func(po);
          vTrav[po] = iTrav;
        }
      }
    }
    EndTraversal();
  }
  
  template <template <typename...> typename Container, typename... Ts>
  inline void AndNetwork::ForEachTfosUpdate(Container<Ts...> const &ids, bool fPos, std::function<bool(int)> const &func) {
    // this includes ids themselves
    StartTraversal();
    for(int id: ids) {
      vTrav[id] = iTrav;
    }
    citr it = lInts.begin();
    while(vTrav[*it] != iTrav && it != lInts.end()) {
      it++;
    }
    for(; it != lInts.end(); it++) {
      if(vTrav[*it] == iTrav) {
        if(!func(*it)) {
          vTrav[*it] = 0;
        }
      } else {
        for(int fi_edge: vvFaninEdges[*it]) {
          if(vTrav[Edge2Node(fi_edge)] == iTrav) {
            if(func(*it)) {
              vTrav[*it] = iTrav;
            }
            break;
          }
        }
      }
    }
    if(fPos) {
      for(int po: vPos) {
        if(vTrav[po] == iTrav) {
          if(!func(po)) {
            vTrav[po] = 0;
          }
        } else if(vTrav[GetFanin(po, 0)] == iTrav) {
          if(func(po)) {
            vTrav[po] = iTrav;
          }
        }
      }
    }
    EndTraversal();
  }

  /* }}} */

  /* {{{ Extraction */

  template <template <typename...> typename Container, typename... Ts>
  AndNetwork *AndNetwork::Extract(Container<Ts...> const &ids, std::vector<int> const &vInputs, std::vector<int> const &vOutputs) {
    AndNetwork *pNtk = new AndNetwork;
    pNtk->Reserve(int_size(vInputs) + int_size(ids) + int_size(vOutputs));
    std::map<int, int> m;
    m[GetConst0()] = pNtk->GetConst0();
    for(int id: vInputs) {
      m[id] = pNtk->AddPi();
    }
    StartTraversal();
    for(int id: ids) {
      vTrav[id] = iTrav;
    }
    ForEachInt([&](int id) {
      if(vTrav[id] == iTrav) {
        m[id] = pNtk->CreateNode();
        pNtk->lInts.push_back(m[id]);
        pNtk->sInts.insert(m[id]);
        pNtk->vvFaninEdges[m[id]].resize(GetNumFanins(id));
        ForEachFaninIdx(id, [&](int idx, int fi, bool c) {
          assert(m.count(fi));
          pNtk->vvFaninEdges[m[id]][idx] = pNtk->Node2Edge(m[fi], c);
          pNtk->vRefs[m[fi]]++;
        });
      }
    });
    EndTraversal();
    for(int id: vOutputs) {
      assert(m.count(id));
      pNtk->AddPo(m[id], false);
    }
    return pNtk;
  }
  
  /* }}} */
  
  /* {{{ Actions */
  
  void AndNetwork::RemoveFanin(int id, int idx) {
    Action action;
    action.type = REMOVE_FANIN;
    action.id = id;
    action.idx = idx;
    int fi = GetFanin(id, idx);
    bool c = GetCompl(id, idx);
    action.fi = fi;
    action.c = c;
    vRefs[fi]--;
    vvFaninEdges[id].erase(vvFaninEdges[id].begin() + idx);
    TakenAction(action);
  }

  void AndNetwork::RemoveUnused(int id, bool fRecursive, bool fSweeping) {
    assert(vRefs[id] == 0);
    Action action;
    action.type = REMOVE_UNUSED;
    action.id = id;
    ForEachFanin(id, [&](int fi) {
      action.vFanins.push_back(fi);
      vRefs[fi]--;
    });
    vvFaninEdges[id].clear();
    if(!fSweeping) {
      itr it = std::find(lInts.begin(), lInts.end(), id);
      lInts.erase(it);
    }
    sInts.erase(id);
    TakenAction(action);
    if(fRecursive) {
      for(int fi: action.vFanins) {
        if(vRefs[fi] == 0 && IsInt(fi)) {
          RemoveUnused(fi, fRecursive, fSweeping);
        }
      }
    }
  }

  void AndNetwork::RemoveBuffer(int id) {
    assert(GetNumFanins(id) == 1);
    assert(!fPropagating || fLockTrav);
    int fi = GetFanin(id, 0);
    bool c = GetCompl(id, 0);
    // check if it is buffering constant
    if(fi == GetConst0()) {
      RemoveConst(id);
      return;
    }
    // remove if substitution would lead to duplication with the same polarity
    ForEachFanoutRidx(id, false, [&](int fo, bool foc, int idx) {
      int idx2 = FindFanin(fo, fi);
      if(idx2 != -1 && GetCompl(fo, idx2) == (c ^ foc)) {
        RemoveFanin(fo, idx);
        if(fPropagating && GetNumFanins(fo) == 1) {
          vTrav[fo] = iTrav;
        }
      }
    });
    // substitute node with fanin or const-0
    Action action;
    action.type = REMOVE_BUFFER;
    action.id = id;
    action.fi = fi;
    action.c = c;
    ForEachFanoutRidx(id, true, [&](int fo, bool foc, int idx) {
      action.vFanouts.push_back(fo);
      int idx2 = FindFanin(fo, fi);
      if(idx2 != -1) { // substitute with const-0 in case of duplication
        assert(GetCompl(fo, idx2) != (c ^ foc)); // of a different polarity
        vRefs[GetConst0()]++;
        vvFaninEdges[fo][idx] = Node2Edge(GetConst0(), 0);
        if(fPropagating) {
          vTrav[fo] = iTrav;
        }
      } else { // otherwise, substitute with fanin
        vvFaninEdges[fo][idx] = Node2Edge(fi, c ^ foc);
        vRefs[fi]++;
      }
    });
    // remove node
    vRefs[id] = 0;
    vRefs[fi]--;
    vvFaninEdges[id].clear();
    if(!fPropagating) {
      itr it = std::find(lInts.begin(), lInts.end(), id);
      lInts.erase(it);
    }
    sInts.erase(id);
    TakenAction(action);
  }

  void AndNetwork::RemoveConst(int id) {
    assert(GetNumFanins(id) == 0 || FindFanin(id, GetConst0()) != -1);
    assert(!fPropagating || fLockTrav);
    bool c = (GetNumFanins(id) == 0);
    // just remove immediately if polarity is true but not PO
    ForEachFanoutRidx(id, false, [&](int fo, bool foc, int idx) {
      if(c ^ foc) {
        assert(!IsPo(fo));
        RemoveFanin(fo, idx);
        if(fPropagating && GetNumFanins(fo) <= 1) {
          vTrav[fo] = iTrav;
        }
      }
    });
    // substitute with constant
    Action action;
    action.type = REMOVE_CONST;
    action.id = id;
    ForEachFanoutRidx(id, true, [&](int fo, bool foc, int idx) {
      action.vFanouts.push_back(fo);
      vRefs[GetConst0()]++;
      vvFaninEdges[fo][idx] = Node2Edge(GetConst0(), c ^ foc);
      if(fPropagating) {
        vTrav[fo] = iTrav;
      }
    });
    // remove node
    vRefs[id] = 0;
    ForEachFanin(id, [&](int fi) {
      vRefs[fi]--;
      action.vFanins.push_back(fi);
    });
    vvFaninEdges[id].clear();
    if(!fPropagating) {
      itr it = std::find(lInts.begin(), lInts.end(), id);
      lInts.erase(it);
    }
    sInts.erase(id);
    TakenAction(action);
  }

  void AndNetwork::AddFanin(int id, int fi, bool c) {
    assert(FindFanin(id, fi) == -1); // no duplication
    assert(fi != GetConst0() || !c); // no const-1
    Action action;
    action.type = ADD_FANIN;
    action.id = id;
    action.idx = GetNumFanins(id);
    action.fi = fi;
    action.c = c;
    itr it = std::find(lInts.begin(), lInts.end(), id);
    itr it2 = std::find(it, lInts.end(), fi);
    if(it2 != lInts.end()) {
      lInts.erase(it2);
      it2 = lInts.insert(it, fi);
      SortInts(it2);
    }
    vRefs[fi]++;
    vvFaninEdges[id].push_back(Node2Edge(fi, c));
    TakenAction(action);
  }

  void AndNetwork::TrivialCollapse(int id) {
    for(int idx = 0; idx < GetNumFanins(id);) {
      int fi_edge = vvFaninEdges[id][idx];
      int fi = Edge2Node(fi_edge);
      bool c = EdgeIsCompl(fi_edge);
      if(!IsPi(fi) && !c && vRefs[fi] == 1) {
        Action action;
        action.type = TRIVIAL_COLLAPSE;
        action.id = id;
        action.idx = idx;
        action.fi = fi;
        action.c = c;
        std::vector<int>::iterator it = vvFaninEdges[id].begin() + idx;
        it = vvFaninEdges[id].erase(it);
        vvFaninEdges[id].insert(it, vvFaninEdges[fi].begin(), vvFaninEdges[fi].end());
        ForEachFanin(fi, [&](int fi) {
          action.vFanins.push_back(fi);
        });
        // remove collapsed fanin
        vRefs[fi] = 0;
        vvFaninEdges[fi].clear();
        lInts.erase(std::find(lInts.begin(), lInts.end(), fi));
        sInts.erase(fi);
        TakenAction(action);
      } else {
        idx++;
      }
    }
  }

  void AndNetwork::TrivialDecompose(int id) {
    while(GetNumFanins(id) > 2) {
      Action action;
      action.type = TRIVIAL_DECOMPOSE;
      action.id = id;
      action.idx = GetNumFanins(id) - 2;
      int new_fi = CreateNode();
      action.fi = new_fi;
      int fi_edge1 = vvFaninEdges[id].back();
      vvFaninEdges[id].pop_back();
      int fi_edge0 = vvFaninEdges[id].back();
      vvFaninEdges[id].pop_back();
      vvFaninEdges[new_fi].push_back(fi_edge0);
      action.vFanins.push_back(Edge2Node(fi_edge0));
      vvFaninEdges[new_fi].push_back(fi_edge1);
      action.vFanins.push_back(Edge2Node(fi_edge1));
      vvFaninEdges[id].push_back(Node2Edge(new_fi, false));
      vRefs[new_fi]++;
      itr it = std::find(lInts.begin(), lInts.end(), id);
      lInts.insert(it, new_fi);
      sInts.insert(new_fi);
      TakenAction(action);
    }
  }

  template <typename Func>
  void AndNetwork::SortFanins(int id, Func const &comp) {
    static_assert(is_invokable<Func, int, int>::value || is_invokable<Func, int, bool, int, bool>::value, "fanin cost function format error");
    std::vector<int> vFaninEdges = vvFaninEdges[id];
    std::sort(vvFaninEdges[id].begin(), vvFaninEdges[id].end(), [&](int i, int j) {
      if constexpr(is_invokable<Func, int, int>::value) {
        return comp(Edge2Node(i), Edge2Node(j));
      } else if constexpr(is_invokable<Func, int, bool, int, bool>::value) {
        return comp(Edge2Node(i), EdgeIsCompl(i), Edge2Node(j), EdgeIsCompl(j));
      }
    });
    if(vFaninEdges == vvFaninEdges[id]) {
      return;
    }
    Action action;
    action.type = SORT_FANINS;
    action.id = id;
    assert(check_int_size(vFaninEdges));
    for(int fanin_edge: vvFaninEdges[id]) {
      std::vector<int>::const_iterator it = std::find(vFaninEdges.begin(), vFaninEdges.end(), fanin_edge);
      action.vIndices.push_back(std::distance(vFaninEdges.cbegin(), it));
    }
    TakenAction(action);
  }

  std::pair<std::vector<int>, std::vector<bool>> AndNetwork::Insert(AndNetwork *pNtk, std::vector<int> const &vInputs, std::vector<bool> const &vCompls, std::vector<int> const &vOutputs) {
    Reserve(nNodes + pNtk->GetNumInts());
    std::map<int, std::pair<int, bool>> m;
    m[pNtk->GetConst0()] = std::make_pair(GetConst0(), false);
    assert(pNtk->GetNumPis() == int_size(vInputs));
    assert(vInputs.size() == vCompls.size());
    for(int i = 0; i < pNtk->GetNumPis(); i++) {
      assert(IsInt(vInputs[i]) || IsPi(vInputs[i]));
      m[pNtk->GetPi(i)] = std::make_pair(vInputs[i], vCompls[i]);
    }
    pNtk->ForEachInt([&](int id) {
      int id2 = CreateNode();
      lInts.push_back(id2);
      sInts.insert(id2);
      vvFaninEdges[id2].resize(pNtk->GetNumFanins(id));
      pNtk->ForEachFaninIdx(id, [&](int idx, int fi, bool c) {
        assert(m.count(fi));
        vvFaninEdges[id2][idx] = Node2Edge(m[fi].first, c ^ m[fi].second);
        vRefs[m[fi].first]++;
      });
      m[id] = std::make_pair(id2, false);
    });
    assert(pNtk->GetNumPos() == int_size(vOutputs));
    std::vector<int> vNewOutputs(pNtk->GetNumPos());
    std::vector<bool> vNewCompls(pNtk->GetNumPos());
    for(int i = 0; i < pNtk->GetNumPos(); i++) {
      int id = vOutputs[i];
      int po = pNtk->GetPo(i);
      assert(m.count(pNtk->GetFanin(po, 0)));
      int fi = m[pNtk->GetFanin(po, 0)].first;
      bool c = pNtk->GetCompl(po, 0) ^ m[pNtk->GetFanin(po, 0)].second;
      assert(id != fi);
      vNewOutputs[i] = fi;
      vNewCompls[i] = c;
      // remove if substitution would lead to duplication with the same polarity
      ForEachFanoutRidx(id, false, [&](int fo, bool foc, int idx) {
        int idx2 = FindFanin(fo, fi);
        if(idx2 != -1 && GetCompl(fo, idx2) == (c ^ foc)) {
          RemoveFanin(fo, idx);
        }
      });
      ForEachFanoutRidx(id, true, [&](int fo, bool foc, int idx) {
        int idx2 = FindFanin(fo, fi);
        if(idx2 != -1) { // substitute with const-0 in case of duplication
          assert(GetCompl(fo, idx2) != (c ^ foc)); // of a different polarity
          vRefs[GetConst0()]++;
          vvFaninEdges[fo][idx] = Node2Edge(GetConst0(), 0);
        } else { // otherwise, substitute with fanin
          vvFaninEdges[fo][idx] = Node2Edge(fi, c ^ foc);
          vRefs[fi]++;
          // sort internal nodes
          itr it = std::find(lInts.begin(), lInts.end(), id);
          itr it2 = std::find(it, lInts.end(), fi);
          if(it2 != lInts.end()) {
            lInts.erase(it2);
            it2 = lInts.insert(it, fi);
            SortInts(it2);
          }
        }
      });
      vRefs[id] = 0;
    }
    Action action;
    action.type = INSERT;
    action.vFanins = vInputs;
    action.vFanouts = vOutputs;
    TakenAction(action);
    for(int id: vOutputs) {
      RemoveUnused(id, true);
    }
    return std::make_pair(std::move(vNewOutputs), std::move(vNewCompls));
  }

  /* }}} */

  /* {{{ Network cleanup */
  
  void AndNetwork::Propagate(int id) {
    StartTraversal();
    itr it;
    if(id == -1) {
      ForEachInt([&](int id) {
        if(GetNumFanins(id) <= 1 || FindFanin(id, GetConst0()) != -1) {
          vTrav[id] = iTrav;
        }
      });
      it = lInts.begin();
      while(vTrav[*it] != iTrav && it != lInts.end()) {
        it++;
      }
    } else {
      vTrav[id] = iTrav;
      it = std::find(lInts.begin(), lInts.end(), id);
    }
    fPropagating = true;
    while(it != lInts.end()) {
      if(vTrav[*it] == iTrav) {
        if(GetNumFanins(*it) == 1) {
          RemoveBuffer(*it);
        } else {
          RemoveConst(*it);
        }
        it = lInts.erase(it);
      } else {
        it++;
      }
    }
    fPropagating = false;
    EndTraversal();
  }

  void AndNetwork::Sweep(bool fPropagate) {
    if(fPropagate) {
      Propagate();
    }
    for(ritr it = lInts.rbegin(); it != lInts.rend();) {
      if(vRefs[*it] == 0) {
        RemoveUnused(*it, false, true);
        it = ritr(lInts.erase(--it.base()));
      } else {
        it++;
      }
    }
  }
  
  /* }}} */

  /* {{{ Save & load */

  int AndNetwork::Save(int slot) {
    Action action;
    action.type = SAVE;
    if(slot < 0) {
      slot = int_size(vBackups);
      vBackups.push_back(*this);
      assert(check_int_size(vBackups));
    } else {
      assert(slot < int_size(vBackups));
      vBackups[slot] = *this;
    }
    action.idx = slot;
    TakenAction(action);
    return slot;
  }

  void AndNetwork::Load(int slot) {
    assert(slot >= 0);
    assert(slot < int_size(vBackups));
    Action action;
    action.type = LOAD;
    action.idx = slot;
    *this = vBackups[slot];
    TakenAction(action);
  }

  void AndNetwork::PopBack() {
    assert(!vBackups.empty());
    Action action;
    action.type = POP_BACK;
    action.idx = int_size(vBackups) - 1;
    vBackups.pop_back();
    TakenAction(action);
  }

  /* }}} */
  
  /* {{{ Misc */

  void AndNetwork::AddCallback(Callback const &callback) {
    vCallbacks.push_back(callback);
  }
  
  void AndNetwork::Print() const {
    std::cout << "inputs: " << vPis << std::endl;
    ForEachInt([&](int id) {
      std::cout << "node " << id << ": ";
      PrintComplementedEdges(std::bind(&AndNetwork::ForEachFanin<std::function<void(int, bool)>>, this, id, std::placeholders::_1));
      std::cout << " (ref = " << vRefs[id] << ")";
      std::cout << std::endl;
    });
    std::cout << "outputs: ";
    PrintComplementedEdges(std::bind(&AndNetwork::ForEachPoDriver<std::function<void(int, bool)>>, this, std::placeholders::_1));
    std::cout << std::endl;
  }

  /* }}} */

}

ABC_NAMESPACE_CXX_HEADER_END
