#pragma once

#include "rrrParameter.h"
#include "rrrUtils.h"
#include "rrrBddManager.h"

ABC_NAMESPACE_CXX_HEADER_START

namespace rrr {

  template <typename Ntk>
  class BddAnalyzer {
  private:
    // aliases
    using lit = int;
    static constexpr lit LitMax = 0xffffffff;

    // pointer to network
    Ntk *pNtk;
    
    // parameters
    int nVerbose;
    
    // data
    NewBdd::Man *pBdd;
    int target;
    std::vector<lit> vFs;
    std::vector<lit> vGs;
    std::vector<std::vector<lit>> vvCs;
    bool fResim;
    std::vector<bool> vUpdates;
    std::vector<bool> vGUpdates;
    std::vector<bool> vCUpdates;

    // backups
    std::vector<BddAnalyzer> vBackups;

    // BDD utils
    void IncRef(lit x) const;
    void DecRef(lit x) const;
    void Assign(lit &x, lit y) const;
    void CopyVec(std::vector<lit> &x, std::vector<lit> const &y) const;
    void DelVec(std::vector<lit> &v) const;
    void CopyVecVec(std::vector<std::vector<lit>> &x, std::vector<std::vector<lit>> const &y) const;
    void DelVecVec(std::vector<std::vector<lit>> &v) const;
    lit  Xor(lit x, lit y) const;

    // callback
    void ActionCallback(Action const &action);

    // allocation
    void Allocate();

    // simulation
    void SimulateNode(int id, std::vector<lit> &v) const;
    void Simulate();
    
    // CSPF computation
    bool ComputeG(int id);
    void ComputeC(int id);
    void CspfNode(int id);
    void Cspf(int id = -1, bool fC = true);

    // save & load
    void Save(int slot);
    void Load(int slot);
    void PopBack();

  public:
    // constructors
    BddAnalyzer();
    BddAnalyzer(Parameter const *pPar);
    ~BddAnalyzer();
    void UpdateNetwork(Ntk *pNtk_, bool fSame);

    // checks
    bool CheckRedundancy(int id, int idx);
    bool CheckFeasibility(int id, int fi, bool c);
  };
  
  /* {{{ BDD utils */
  
  template <typename Ntk>
  inline void BddAnalyzer<Ntk>::IncRef(lit x) const {
    if(x != LitMax) {
      pBdd->IncRef(x);
    }
  }
  
  template <typename Ntk>
  inline void BddAnalyzer<Ntk>::DecRef(lit x) const {
    if(x != LitMax) {
      pBdd->DecRef(x);
    }
  }
  
  template <typename Ntk>
  inline void BddAnalyzer<Ntk>::Assign(lit &x, lit y) const {
    DecRef(x);
    x = y;
    IncRef(x);
  }

  template <typename Ntk>
  inline void BddAnalyzer<Ntk>::CopyVec(std::vector<lit> &x, std::vector<lit> const &y) const {
    for(size_t i = y.size(); i < x.size(); i++) {
      DecRef(x[i]);
    }
    x.resize(y.size(), LitMax);
    for(size_t i = 0; i < y.size(); i++) {
      if(x[i] != y[i]) {
        DecRef(x[i]);
        x[i] = y[i];
        IncRef(x[i]);
      }
    }
  }

  template <typename Ntk>
  inline void BddAnalyzer<Ntk>::DelVec(std::vector<lit> &v) const {
    for(lit x: v) {
      DecRef(x);
    }
    v.clear();
  }

  template <typename Ntk>
  inline void BddAnalyzer<Ntk>::CopyVecVec(std::vector<std::vector<lit>> &x, std::vector<std::vector<lit>> const &y) const {
    for(size_t i = y.size(); i < x.size(); i++) {
      DelVec(x[i]);
    }
    x.resize(y.size());
    for(size_t i = 0; i < y.size(); i++) {
      CopyVec(x[i], y[i]);
    }
  }

  template <typename Ntk>
  inline void BddAnalyzer<Ntk>::DelVecVec(std::vector<std::vector<lit>> &v) const {
    for(size_t i = 0; i < v.size(); i++) {
      DelVec(v[i]);
    }
    v.clear();
  }
  
  template <typename Ntk>
  inline int BddAnalyzer<Ntk>::Xor(lit x, lit y) const {
    lit f = pBdd->And(x, pBdd->LitNot(y));
    pBdd->IncRef(f);
    lit g = pBdd->And(pBdd->LitNot(x), y);
    pBdd->IncRef(g);
    lit r = pBdd->Or(f, g);
    pBdd->DecRef(f);
    pBdd->DecRef(g);
    return r;
  }
  
  /* }}} */

  /* {{{ Callback */

  template <typename Ntk>
  void BddAnalyzer<Ntk>::ActionCallback(Action const &action) {
    switch(action.type) {
    case REMOVE_FANIN:
      vUpdates[action.id] = true;
      vGUpdates[action.fi] = true;
      DecRef(vvCs[action.id][action.idx]);
      vvCs[action.id].erase(vvCs[action.id].begin() + action.idx);
      break;
    case REMOVE_UNUSED:
      if(vGUpdates[action.id] || vCUpdates[action.id]) {
        for(int fi: action.vFanins) {
          vGUpdates[fi] = true;
        }
      }
      Assign(vFs[action.id], LitMax);
      Assign(vGs[action.id], LitMax);
      DelVec(vvCs[action.id]);
      break;
    case REMOVE_BUFFER:
      if(vUpdates[action.id]) {
        for(int fo: action.vFanouts) {
          vUpdates[fo] = true;
          vCUpdates[fo] = true;
        }
      }
      if(vGUpdates[action.id] || vCUpdates[action.id]) {
        vGUpdates[action.fi] = true;
      }
      Assign(vFs[action.id], LitMax);
      Assign(vGs[action.id], LitMax);
      DelVec(vvCs[action.id]);
      break;
    case REMOVE_CONST:
      if(vUpdates[action.id]) {
        for(int fo: action.vFanouts) {
          vUpdates[fo] = true;
          vCUpdates[fo] = true;
        }
      }
      for(int fi: action.vFanins) {
        vGUpdates[fi] = true;
      }
      Assign(vFs[action.id], LitMax);
      Assign(vGs[action.id], LitMax);
      DelVec(vvCs[action.id]);
      break;
    case ADD_FANIN:
      vUpdates[action.id] = true;
      vCUpdates[action.id] = true;
      vvCs[action.id].insert(vvCs[action.id].begin() + action.idx, LitMax);
      break;
    case TRIVIAL_COLLAPSE: {
      if(vGUpdates[action.fi] || vCUpdates[action.fi]) {
        vCUpdates[action.id] = true;
      }
      std::vector<lit>::iterator it = vvCs[action.id].begin() + action.idx;
      DecRef(*it);
      it = vvCs[action.id].erase(it);
      vvCs[action.id].insert(it,  vvCs[action.fi].begin(), vvCs[action.fi].end());
      vvCs[action.fi].clear();
      Assign(vFs[action.fi], LitMax);
      Assign(vGs[action.fi], LitMax);
      break;
    }
    case TRIVIAL_DECOMPOSE: {
      Allocate();
      SimulateNode(action.fi, vFs);
      assert(vGs[action.fi] == LitMax);
      Assign(vGs[action.fi], vGs[action.id]);
      std::vector<lit>::iterator it = vvCs[action.id].begin() + action.idx;
      assert(vvCs[action.fi].empty());
      vvCs[action.fi].insert(vvCs[action.fi].begin(), it, vvCs[action.id].end());
      vvCs[action.id].erase(it, vvCs[action.id].end());
      assert(vvCs[action.id].size() == action.idx);
      vvCs[action.id].resize(action.idx + 1, LitMax);
      Assign(vvCs[action.id][action.idx], vGs[action.fi]);
      vUpdates[action.fi] = false;
      vGUpdates[action.fi] = false;
      vCUpdates[action.fi] = vCUpdates[action.id];
      break;
    }
    case SORT_FANINS: {
      std::vector<lit> vCs = vvCs[action.id];
      vvCs[action.id].clear();
      for(int index: action.vIndices) {
        vvCs[action.id].push_back(vCs[index]);
      }
      if(!fResim && target != -1 && target != action.id) {
        pNtk->ForEachTfo(target, false, [&](int fo) {
          if(fResim) {
            return;
          }
          if(fo == action.id) {
            fResim = true;
          }
        });
      }
      vCUpdates[action.id] = true;
      break;
    }
    case SAVE:
      Save(action.idx);
      break;
    case LOAD:
      Load(action.idx);
      break;
    case POP_BACK:
      PopBack();
      break;
    default:
      assert(0);
    }
  }
  
  /* }}} */

  /* {{{ Allocation */

  template <typename Ntk>
  void BddAnalyzer<Ntk>::Allocate() {
    int nNodes = pNtk->GetNumNodes();
    vFs.resize(nNodes, LitMax);
    vGs.resize(nNodes, LitMax);
    vvCs.resize(nNodes);
    vUpdates.resize(nNodes);
    vGUpdates.resize(nNodes);
    vCUpdates.resize(nNodes);
  }

  /* }}} */
  
  /* {{{ Simulation */
  
  template <typename Ntk>
  inline void BddAnalyzer<Ntk>::SimulateNode(int id, std::vector<lit> &v) const {
    if(nVerbose) {
      std::cout << "simulating node " << id << std::endl;
    }
    Assign(v[id], pBdd->Const1());
    pNtk->ForEachFanin(id, [&](int fi, bool c) {
      Assign(v[id], pBdd->And(v[id], pBdd->LitNotCond(v[fi], c)));
    });
  }
  
  template <typename Ntk>
  void BddAnalyzer<Ntk>::Simulate() {
    if(nVerbose) {
      std::cout << "symbolic simulation with BDD" << std::endl;
    }
    pNtk->ForEachInt([&](int id) {
      if(vUpdates[id]) {
        lit x = vFs[id];
        IncRef(x);
        SimulateNode(id, vFs);
        DecRef(x);
        if(!pBdd->LitIsEq(x, vFs[id])) {
          pNtk->ForEachFanout(id, false, [&](int fo) {
            vUpdates[fo] = true;
            vCUpdates[fo] = true;
          });
        }
        vUpdates[id] = false;
      }
    });
  }

  /* }}} */

  /* {{{ CSPF computation */
  
  template <typename Ntk>
  inline bool BddAnalyzer<Ntk>::ComputeG(int id) {
    if(pNtk->GetNumFanouts(id) == 0) {
      if(pBdd->IsConst1(vGs[id])) {
        return false;
      }
      Assign(vGs[id], pBdd->Const1());
      return true;
    }
    lit x = pBdd->Const1();
    IncRef(x);
    pNtk->ForEachFanoutRidx(id, true, [&](int fo, int idx) {
      Assign(x, pBdd->And(x, vvCs[fo][idx]));
    });
    if(pBdd->LitIsEq(vGs[id], x)) {
      DecRef(x);
      return false;
    }
    Assign(vGs[id], x);
    DecRef(x);
    return true;
  }

  template <typename Ntk>
  inline void BddAnalyzer<Ntk>::ComputeC(int id) {
    int nFanins = pNtk->GetNumFanins(id);
    assert(vvCs[id].size() == nFanins);
    if(pBdd->IsConst1(vGs[id])) {
      for(int idx = 0; idx < nFanins; idx++) {
        if(!pBdd->IsConst1(vvCs[id][idx])) {
          Assign(vvCs[id][idx], pBdd->Const1());
          int fi = pNtk->GetFanin(id, idx);
          vGUpdates[fi] = true;
        }
      }
      return;
    }
    for(int idx = 0; idx < nFanins; idx++) {
      lit x = pBdd->Const1();
      IncRef(x);
      for(int idx2 = idx + 1; idx2 < nFanins; idx2++) {
        int fi = pNtk->GetFanin(id, idx2);
        bool c = pNtk->GetCompl(id, idx2);
        Assign(x, pBdd->And(x, pBdd->LitNotCond(vFs[fi], c)));
      }
      Assign(x, pBdd->Or(pBdd->LitNot(x), vGs[id]));
      if(!pBdd->LitIsEq(vvCs[id][idx], x)) {
        Assign(vvCs[id][idx], x);
        int fi = pNtk->GetFanin(id, idx);
        vGUpdates[fi] = true;
      }
      DecRef(x);
    }
  }

  template <typename Ntk>
  void BddAnalyzer<Ntk>::CspfNode(int id) {
    if(!vGUpdates[id] && !vCUpdates[id]) {
      return;
    }
    if(vGUpdates[id]) {
      if(nVerbose) {
        std::cout << "computing node " << id << " G " << std::endl;
      }
      if(ComputeG(id)) {
        vCUpdates[id] = true;
      }
      vGUpdates[id] = false;
    }
    if(vCUpdates[id]) {
      if(nVerbose) {
        std::cout << "computing node " << id << " C " << std::endl;
      }
      ComputeC(id);
      vCUpdates[id] = false;
    }
  }
  
  template <typename Ntk>
  void BddAnalyzer<Ntk>::Cspf(int id, bool fC) {
    if(id != -1) {
      pNtk->ForEachTfoReverse(id, false, [&](int fo) {
        CspfNode(fo);
      });
      bool fCUpdate = vCUpdates[id];
      if(!fC) {
        vCUpdates[id] = false;
      }
      CspfNode(id);
      if(!fC) {
        vCUpdates[id] = fCUpdate;
      }
    } else {
      pNtk->ForEachIntReverse([&](int id) {
        CspfNode(id);
      });
    }
  }

  /* }}} */

  /* {{{ Save & load */

  template <typename Ntk>
  void BddAnalyzer<Ntk>::Save(int slot) {
    if(slot >= int_size(vBackups)) {
      vBackups.resize(slot + 1);
    }
    vBackups[slot].target = target;
    CopyVec(vBackups[slot].vFs, vFs);
    CopyVec(vBackups[slot].vGs, vGs);
    CopyVecVec(vBackups[slot].vvCs, vvCs);
    vBackups[slot].vUpdates = vUpdates;
    vBackups[slot].vGUpdates = vGUpdates;
    vBackups[slot].vCUpdates = vCUpdates;
  }

  template <typename Ntk>
  void BddAnalyzer<Ntk>::Load(int slot) {
    assert(slot < int_size(vBackups));
    target = vBackups[slot].target;
    CopyVec(vFs, vBackups[slot].vFs);
    CopyVec(vGs, vBackups[slot].vGs);
    CopyVecVec(vvCs, vBackups[slot].vvCs);
    vUpdates = vBackups[slot].vUpdates;
    vGUpdates = vBackups[slot].vGUpdates;
    vCUpdates = vBackups[slot].vCUpdates;
  }

  template <typename Ntk>
  void BddAnalyzer<Ntk>::PopBack() {
    assert(!vBackups.empty());
    DelVec(vBackups.back().vFs);
    DelVec(vBackups.back().vGs);
    DelVecVec(vBackups.back().vvCs);
    vBackups.pop_back();
  }

  /* }}} Save & load end */
  
  /* {{{ Constructors */

  template <typename Ntk>
  BddAnalyzer<Ntk>::BddAnalyzer() :
    pNtk(NULL),
    nVerbose(0),
    pBdd(NULL),
    target(-1),
    fResim(false) {
  }
  
  template <typename Ntk>
  BddAnalyzer<Ntk>::BddAnalyzer(Parameter const *pPar) :
    pNtk(NULL),
    nVerbose(pPar->nAnalyzerVerbose),
    pBdd(NULL),
    target(-1),
    fResim(false) {
  }

  template <typename Ntk>
  void BddAnalyzer<Ntk>::UpdateNetwork(Ntk *pNtk_, bool fSame) {
    // clear
    while(!vBackups.empty()) {
      PopBack();
    }
    DelVec(vFs);
    DelVec(vGs);
    DelVecVec(vvCs);
    pNtk = pNtk_;
    target = -1;
    fResim = false;
    vUpdates.clear();
    vGUpdates.clear();
    vCUpdates.clear();
    // alloc
    bool fUseReo = false;
    if(!pBdd || pBdd->GetNumVars() != pNtk->GetNumPis()) {
      // need to reset manager
      delete pBdd;
      NewBdd::Param Par;
      Par.nObjsMaxLog = 25;
      Par.nCacheMaxLog = 20;
      Par.fCountOnes = true;
      Par.nGbc = 1;
      Par.nReo = 4000;
      pBdd = new NewBdd::Man(pNtk->GetNumPis(), Par);
      fUseReo = true;
    } else if(!fSame) {
      // turning on reordering if network function changed
      pBdd->TurnOnReo();
      fUseReo = true;
    }
    Allocate();
    // prepare
    Assign(vFs[0], pBdd->Const0());
    int idx = 0;
    pNtk->ForEachPi([&](int id) {
      Assign(vFs[id], pBdd->IthVar(idx));
      idx++;
    });
    pNtk->ForEachInt([&](int id) {
      vUpdates[id] = true;
    });
    Simulate();
    if(fUseReo) {
      pBdd->Reorder();
      pBdd->TurnOffReo();
    }
    pNtk->ForEachInt([&](int id) {
      vvCs[id].resize(pNtk->GetNumFanins(id), LitMax);
    });
    pNtk->ForEachPo([&](int id) {
      vvCs[id].resize(1, LitMax);
      Assign(vvCs[id][0], pBdd->Const0());
      int fi = pNtk->GetFanin(id, 0);
      vGUpdates[fi]  = true;
    });
    pNtk->AddCallback(std::bind(&BddAnalyzer<Ntk>::ActionCallback, this, std::placeholders::_1));
  }
  
  template <typename Ntk>
  BddAnalyzer<Ntk>::~BddAnalyzer() {
    while(!vBackups.empty()) {
      PopBack();
    }
    DelVec(vFs);
    DelVec(vGs);
    DelVecVec(vvCs);
    if(pBdd) {
      pBdd->PrintStats();
    }
    delete pBdd;
  }

  /* }}} */

  /* {{{ Checks */
  
  template <typename Ntk>
  bool BddAnalyzer<Ntk>::CheckRedundancy(int id, int idx) {
    if(fResim) {
      Simulate();
    }
    Cspf(id);
    switch(pNtk->GetNodeType(id)) {
    case AND: {
      int fi = pNtk->GetFanin(id, idx);
      bool c = pNtk->GetCompl(id, idx);
      lit x = pBdd->Or(pBdd->LitNotCond(vFs[fi], c), vvCs[id][idx]);
      if(pBdd->IsConst1(x)) {
        if(nVerbose) {
          std::cout << "node " << id << " fanin " << (pNtk->GetCompl(id, idx)? "!": "") << pNtk->GetFanin(id, idx) << " index " << idx << " is redundant" << std::endl;
        }
        return true;
      }
      break;
    }
    default:
      assert(0);
    }
    if(nVerbose) {
      std::cout << "node " << id << " fanin " << (pNtk->GetCompl(id, idx)? "!": "") << pNtk->GetFanin(id, idx) << " index " << idx << " is NOT redundant" << std::endl;
    }
    return false;
  }
  
  template <typename Ntk>
  bool BddAnalyzer<Ntk>::CheckFeasibility(int id, int fi, bool c) {
    if(target != id) { // simualte if there has been update in non-tfo of this node
      Simulate();
      target = id;
    }
    Cspf(id, false);
    switch(pNtk->GetNodeType(id)) {
    case AND: {
      lit x = pBdd->Or(pBdd->LitNot(vFs[id]), vGs[id]);
      IncRef(x);
      lit y = pBdd->Or(x, pBdd->LitNotCond(vFs[fi], c));
      DecRef(x);
      if(pBdd->IsConst1(y)) {
        if(nVerbose) {
          std::cout << "node " << id << " fanin " << (c? "!": "") << fi << " is feasible" << std::endl;
        }
        return true;
      }
      break;
    }
    default:
      assert(0);
    }
    if(nVerbose) {
      std::cout << "node " << id << " fanin " << (c? "!": "") << fi << " is NOT feasible" << std::endl;
    }
    return false;
  }

  /* }}} */
  
}

ABC_NAMESPACE_CXX_HEADER_END
