#pragma once

#include "rrrParameter.h"
#include "rrrUtils.h"
#include "rrrBddManager.h"

ABC_NAMESPACE_CXX_HEADER_START

namespace rrr {

  template <typename Ntk>
  class BddMspfAnalyzer {
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
    std::vector<lit> vFs;
    std::vector<lit> vGs;
    std::vector<std::vector<lit>> vvCs;
    bool fUpdate;
    std::vector<bool> vUpdates;
    std::vector<bool> vGUpdates;
    std::vector<bool> vCUpdates;
    std::vector<bool> vVisits;
    
    // backups
    std::vector<BddMspfAnalyzer> vBackups;

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
    bool SimulateNode(int id, std::vector<lit> &v) const;
    void Simulate();

    // PF computation
    bool ComputeReconvergentG(int id);
    bool ComputeG(int id);
    void ComputeC(int id);
    bool ComputeCDebug(int id);
    void MspfNode(int id);
    void Mspf(int id = -1, bool fC = true);

    // save & load
    void Save(int slot);
    void Load(int slot);
    void PopBack();
    
  public:
    // constructors
    BddMspfAnalyzer();
    BddMspfAnalyzer(Parameter const *pPar);
    ~BddMspfAnalyzer();
    void UpdateNetwork(Ntk *pNtk_, bool fSame);

    // checks
    bool CheckRedundancy(int id, int idx);
    bool CheckFeasibility(int id, int fi, bool c);
  };
  
  /* {{{ BDD utils */
  
  template <typename Ntk>
  inline void BddMspfAnalyzer<Ntk>::IncRef(lit x) const {
    if(x != LitMax) {
      pBdd->IncRef(x);
    }
  }
  
  template <typename Ntk>
  inline void BddMspfAnalyzer<Ntk>::DecRef(lit x) const {
    if(x != LitMax) {
      pBdd->DecRef(x);
    }
  }
  
  template <typename Ntk>
  inline void BddMspfAnalyzer<Ntk>::Assign(lit &x, lit y) const {
    DecRef(x);
    x = y;
    IncRef(x);
  }

  template <typename Ntk>
  inline void BddMspfAnalyzer<Ntk>::CopyVec(std::vector<lit> &x, std::vector<lit> const &y) const {
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
  inline void BddMspfAnalyzer<Ntk>::DelVec(std::vector<lit> &v) const {
    for(lit x: v) {
      DecRef(x);
    }
    v.clear();
  }

  template <typename Ntk>
  inline void BddMspfAnalyzer<Ntk>::CopyVecVec(std::vector<std::vector<lit>> &x, std::vector<std::vector<lit>> const &y) const {
    for(size_t i = y.size(); i < x.size(); i++) {
      DelVec(x[i]);
    }
    x.resize(y.size());
    for(size_t i = 0; i < y.size(); i++) {
      CopyVec(x[i], y[i]);
    }
  }

  template <typename Ntk>
  inline void BddMspfAnalyzer<Ntk>::DelVecVec(std::vector<std::vector<lit>> &v) const {
    for(size_t i = 0; i < v.size(); i++) {
      DelVec(v[i]);
    }
    v.clear();
  }
  
  template <typename Ntk>
  inline typename BddMspfAnalyzer<Ntk>::lit BddMspfAnalyzer<Ntk>::Xor(lit x, lit y) const {
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
  void BddMspfAnalyzer<Ntk>::ActionCallback(Action const &action) {
    switch(action.type) {
    case REMOVE_FANIN:
      fUpdate = true;
      std::fill(vVisits.begin(), vVisits.end(), false);
      vUpdates[action.id] = true;
      vCUpdates[action.id] = true;
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
      Assign(vGs[action.id], LitMax);
      DelVec(vvCs[action.id]);
      break;
    case REMOVE_BUFFER:
      if(vUpdates[action.id]) {
        fUpdate = true;
        for(int fo: action.vFanouts) {
          vUpdates[fo] = true;
          vCUpdates[fo] = true;
        }
      }
      if(vGUpdates[action.id] || vCUpdates[action.id]) {
        vGUpdates[action.fi] = true;
      }
      Assign(vGs[action.id], LitMax);
      DelVec(vvCs[action.id]);
      break;
    case REMOVE_CONST:
      if(vUpdates[action.id]) {
        fUpdate = true;
        for(int fo: action.vFanouts) {
          vUpdates[fo] = true;
          vCUpdates[fo] = true;
        }
      }
      for(int fi: action.vFanins) {
        vGUpdates[fi] = true;
      }
      break;
    case ADD_FANIN:
      fUpdate = true;
      std::fill(vVisits.begin(), vVisits.end(), false);
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
      std::vector<lit>::iterator it = vvCs[action.id].begin() + action.idx;
      assert(vvCs[action.fi].empty());
      vvCs[action.fi].insert(vvCs[action.fi].begin(), it, vvCs[action.id].end());
      vvCs[action.id].erase(it, vvCs[action.id].end());
      assert(vvCs[action.id].size() == action.idx);
      if(!vGUpdates[action.id] && !vCUpdates[action.id]) {
        // recompute here only when updates are unlikely to happen
        if(pBdd->IsConst1(vGs[action.id])) {
          Assign(vGs[action.fi], pBdd->Const1());
        } else {
          lit x = pBdd->Const1();
          IncRef(x);
          for(int idx2 = 0; idx2 < action.idx; idx2++) {
            int fi = pNtk->GetFanin(action.id, idx2);
            bool c = pNtk->GetCompl(action.id, idx2);
            Assign(x, pBdd->And(x, pBdd->LitNotCond(vFs[fi], c)));
          }
          Assign(vGs[action.fi], pBdd->Or(pBdd->LitNot(x), vGs[action.id]));
          DecRef(x);
        }
      } else {
        // otherwise mark the node for future update
        vCUpdates[action.id] = true;
      }
      vvCs[action.id].resize(action.idx + 1, LitMax);
      Assign(vvCs[action.id][action.idx], vGs[action.fi]);
      vUpdates[action.fi] = false;
      vGUpdates[action.fi] = false;
      vCUpdates[action.fi] = false;
      break;
    }
    case SORT_FANINS: {
      std::vector<lit> vCs = vvCs[action.id];
      vvCs[action.id].clear();
      for(int index: action.vIndices) {
        vvCs[action.id].push_back(vCs[index]);
      }
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
  void BddMspfAnalyzer<Ntk>::Allocate() {
    int nNodes = pNtk->GetNumNodes();
    vFs.resize(nNodes, LitMax);
    vGs.resize(nNodes, LitMax);
    vvCs.resize(nNodes);
    vUpdates.resize(nNodes);
    vGUpdates.resize(nNodes);
    vCUpdates.resize(nNodes);
    vVisits.resize(nNodes);
  }

  /* }}} */
  
  /* {{{ Simulation */
  
  template <typename Ntk>
  inline bool BddMspfAnalyzer<Ntk>::SimulateNode(int id, std::vector<lit> &v) const {
    if(nVerbose) {
      std::cout << "simulating node " << id << std::endl;
    }
    lit x = pBdd->Const1();
    IncRef(x);
    pNtk->ForEachFanin(id, [&](int fi, bool c) {
      Assign(x, pBdd->And(x, pBdd->LitNotCond(v[fi], c)));
    });
    if(pBdd->LitIsEq(x, v[id])) {
      DecRef(x);
      return false;
    }
    Assign(v[id], x);
    DecRef(x);
    return true;
  }
  
  template <typename Ntk>
  void BddMspfAnalyzer<Ntk>::Simulate() {
    if(nVerbose) {
      std::cout << "symbolic simulation with BDD" << std::endl;
    }
    pNtk->ForEachInt([&](int id) {
      if(vUpdates[id]) {
        if(SimulateNode(id, vFs)) {
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

  /* {{{ MSPF computation */

  template <typename pNtk>
  inline bool BddMspfAnalyzer<pNtk>::ComputeReconvergentG(int id) {
    std::vector<lit> v;
    CopyVec(v, vFs);
    v[id] = pBdd->LitNot(v[id]);
    pNtk->ForEachTfoUpdate(id, false, [&](int fo) {
      return SimulateNode(fo, v);
    });
    lit x = pBdd->Const1();
    IncRef(x);
    pNtk->ForEachPoDriver([&](int fi) {
      lit y = Xor(vFs[fi], v[fi]);
      IncRef(y);
      Assign(x, pBdd->And(x, pBdd->LitNot(y)));
      DecRef(y);
    });
    if(pBdd->LitIsEq(vGs[id], x)) {
      DecRef(x);
      DelVec(v);
      return false;
    }
    Assign(vGs[id], x);
    DecRef(x);
    DelVec(v);
    return true;
  }
  
  template <typename pNtk>
  inline bool BddMspfAnalyzer<pNtk>::ComputeG(int id) {
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

  template <typename pNtk>
  inline void BddMspfAnalyzer<pNtk>::ComputeC(int id) {
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
      for(int idx2 = 0; idx2 < nFanins; idx2++) {
        if(idx2 != idx) {
          int fi = pNtk->GetFanin(id, idx2);
          bool c = pNtk->GetCompl(id, idx2);
          Assign(x, pBdd->And(x, pBdd->LitNotCond(vFs[fi], c)));
        }
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
  
  template <typename pNtk>
  inline bool BddMspfAnalyzer<pNtk>::ComputeCDebug(int id) {
    bool fUpdated = false;
    int nFanins = pNtk->GetNumFanins(id);
    assert(vvCs[id].size() == nFanins);
    if(pBdd->IsConst1(vGs[id])) {
      for(int idx = 0; idx < nFanins; idx++) {
        if(!pBdd->IsConst1(vvCs[id][idx])) {
          Assign(vvCs[id][idx], pBdd->Const1());
          int fi = pNtk->GetFanin(id, idx);
          vGUpdates[fi] = true;
          fUpdated = true;
        }
      }
      return fUpdated;
    }
    for(int idx = 0; idx < nFanins; idx++) {
      lit x = pBdd->Const1();
      IncRef(x);
      for(int idx2 = 0; idx2 < nFanins; idx2++) {
        if(idx2 != idx) {
          int fi = pNtk->GetFanin(id, idx2);
          bool c = pNtk->GetCompl(id, idx2);
          Assign(x, pBdd->And(x, pBdd->LitNotCond(vFs[fi], c)));
        }
      }
      Assign(x, pBdd->Or(pBdd->LitNot(x), vGs[id]));
      if(!pBdd->LitIsEq(vvCs[id][idx], x)) {
        Assign(vvCs[id][idx], x);
        int fi = pNtk->GetFanin(id, idx);
        vGUpdates[fi] = true;
        fUpdated = true;
      }
      DecRef(x);
    }
    return fUpdated;
  }

  template <typename pNtk>
  void BddMspfAnalyzer<pNtk>::MspfNode(int id) {
    if(vGUpdates[id]) {
      if(pNtk->IsReconvergent(id)) {
        if(nVerbose) {
          std::cout << "computing reconvergent node " << id << " G " << std::endl;
        }
        if(ComputeReconvergentG(id)) {
          vCUpdates[id] = true;
        }
      } else {
        if(nVerbose) {
          std::cout << "computing node " << id << " G " << std::endl;
        }
        if(ComputeG(id)) {
          vCUpdates[id] = true;
        }
      }
      vGUpdates[id] = false;
    } else if(!vVisits[id] && pNtk->IsReconvergent(id)) {
      if(nVerbose) {
        std::cout << "computing unvisited reconvergent node " << id << " G " << std::endl;
      }
      if(ComputeReconvergentG(id)) {
        vCUpdates[id] = true;
      }
    }
    if(vCUpdates[id]) {
      if(nVerbose) {
        std::cout << "computing node " << id << " C " << std::endl;
      }
      ComputeC(id);
      vCUpdates[id] = false;
    }
    vVisits[id] = true;
  }

  template <typename pNtk>
  void BddMspfAnalyzer<pNtk>::Mspf(int id, bool fC) {
    if(id != -1) {
      pNtk->ForEachTfoReverse(id, false, [&](int fo) {
        MspfNode(fo);
      });
      bool fCUpdate = vCUpdates[id];
      if(!fC) {
        vCUpdates[id] = false;
      }
      MspfNode(id);
      if(!fC) {
        vCUpdates[id] = fCUpdate;
      }
    } else {
      pNtk->ForEachIntReverse([&](int id) {
        MspfNode(id);
      });
    }
  }

  /* }}} */

  /* {{{ Save & load */

  template <typename Ntk>
  void BddMspfAnalyzer<Ntk>::Save(int slot) {
    if(slot >= int_size(vBackups)) {
      vBackups.resize(slot + 1);
    }
    CopyVec(vBackups[slot].vFs, vFs);
    CopyVec(vBackups[slot].vGs, vGs);
    CopyVecVec(vBackups[slot].vvCs, vvCs);
    vBackups[slot].fUpdate = fUpdate;
    vBackups[slot].vUpdates = vUpdates;
    vBackups[slot].vGUpdates = vGUpdates;
    vBackups[slot].vCUpdates = vCUpdates;
    vBackups[slot].vVisits = vVisits;
  }

  template <typename Ntk>
  void BddMspfAnalyzer<Ntk>::Load(int slot) {
    assert(slot < int_size(vBackups));
    CopyVec(vFs, vBackups[slot].vFs);
    CopyVec(vGs, vBackups[slot].vGs);
    CopyVecVec(vvCs, vBackups[slot].vvCs);
    fUpdate = vBackups[slot].fUpdate;
    vUpdates = vBackups[slot].vUpdates;
    vGUpdates = vBackups[slot].vGUpdates;
    vCUpdates = vBackups[slot].vCUpdates;
    vVisits = vBackups[slot].vVisits;
  }

  template <typename Ntk>
  void BddMspfAnalyzer<Ntk>::PopBack() {
    assert(!vBackups.empty());
    DelVec(vBackups.back().vFs);
    DelVec(vBackups.back().vGs);
    DelVecVec(vBackups.back().vvCs);
    vBackups.pop_back();
  }

  /* }}} */
  
  /* {{{ Constructor */

  template <typename Ntk>
  BddMspfAnalyzer<Ntk>::BddMspfAnalyzer() :
    pNtk(NULL),
    nVerbose(0),
    pBdd(NULL) {
  }
  
  template <typename Ntk>
  BddMspfAnalyzer<Ntk>::BddMspfAnalyzer(Parameter const *pPar) :
    pNtk(NULL),
    nVerbose(pPar->nAnalyzerVerbose),
    pBdd(NULL),
    fUpdate(false) {
  }

  template <typename Ntk>
  void BddMspfAnalyzer<Ntk>::UpdateNetwork(Ntk *pNtk_, bool fSame) {
    // clear
    while(!vBackups.empty()) {
      PopBack();
    }
    DelVec(vFs);
    DelVec(vGs);
    DelVecVec(vvCs);
    pNtk = pNtk_;
    fUpdate = false;
    vUpdates.clear();
    vGUpdates.clear();
    vCUpdates.clear();
    vVisits.clear();
    // alloc
    bool fUseReo = false;
    if(!pBdd || pBdd->GetNumVars() != pNtk->GetNumPis()) {
      // need to reset manager
      delete pBdd;
      NewBdd::Param Par;
      Par.nObjsMaxLog = 25;
      Par.nCacheMaxLog = 20;
      Par.fCountOnes = false;
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
    pNtk->AddCallback(std::bind(&BddMspfAnalyzer<Ntk>::ActionCallback, this, std::placeholders::_1));
  }
  
  template <typename Ntk>
  BddMspfAnalyzer<Ntk>::~BddMspfAnalyzer() {
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
  bool BddMspfAnalyzer<Ntk>::CheckRedundancy(int id, int idx) {
    if(fUpdate) {
      Simulate();
      fUpdate = false;
    }
    Mspf(id);
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
  bool BddMspfAnalyzer<Ntk>::CheckFeasibility(int id, int fi, bool c) {
    if(fUpdate) {
      Simulate();
      fUpdate = false;
    }
    Mspf(id, false);
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
