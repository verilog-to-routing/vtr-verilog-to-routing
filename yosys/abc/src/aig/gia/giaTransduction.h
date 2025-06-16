/**CFile****************************************************************

  FileName    [giaTransduction.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Implementation of transduction method.]

  Author      [Yukio Miyasaka]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 2023.]

  Revision    [$Id: giaTransduction.h,v 1.00 2023/05/10 00:00:00 Exp $]

***********************************************************************/

#ifndef ABC__aig__gia__giaTransduction_h
#define ABC__aig__gia__giaTransduction_h

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <list>
#include <set>
#include <algorithm>
#include <cassert>
#include <iterator>

#include "gia.h"

ABC_NAMESPACE_CXX_HEADER_START

namespace Transduction {
  
enum PfState {none, cspf, mspf};

template <class Man, class lit, lit LitMax>
class ManUtil {
protected:
  Man *man;
  inline void IncRef(lit x) const {
    if(x != LitMax)
      man->IncRef(x);
  }
  inline void DecRef(lit x) const {
    if(x != LitMax)
      man->DecRef(x);
  }
  inline void Update(lit &x, lit y) const {
    DecRef(x);
    x = y;
    IncRef(x);
  }
  inline void DelVec(std::vector<lit> &v) const {
    for(unsigned i = 0; i < v.size(); i++)
      DecRef(v[i]);
    v.clear();
  }
  inline void DelVec(std::vector<std::vector<lit> > &v) const {
    for(unsigned i = 0; i < v.size(); i++)
      DelVec(v[i]);
    v.clear();
  }
  inline void CopyVec(std::vector<lit> &v, std::vector<lit> const &u) const {
    DelVec(v);
    v = u;
    for(unsigned i = 0; i < v.size(); i++)
      IncRef(v[i]);
  }
  inline void CopyVec(std::vector<std::vector<lit> > &v, std::vector<std::vector<lit> > const &u) const {
    for(unsigned i = u.size(); i < v.size(); i++)
      DelVec(v[i]);
    v.resize(u.size());
    for(unsigned i = 0; i < v.size(); i++)
      CopyVec(v[i], u[i]);
  }
  inline bool LitVecIsEq(std::vector<lit> const &v, std::vector<lit> const &u) const {
    if(v.size() != u.size())
      return false;
    for(unsigned i = 0; i < v.size(); i++)
      if(!man->LitIsEq(v[i], u[i]))
        return false;
    return true;
  }
  inline bool LitVecIsEq(std::vector<std::vector<lit> > const &v, std::vector<std::vector<lit> > const &u) const {
    if(v.size() != u.size())
      return false;
    for(unsigned i = 0; i < v.size(); i++)
      if(!LitVecIsEq(v[i], u[i]))
        return false;
    return true;
  }
  inline lit Xor(lit x, lit y) const {
    lit f = man->And(x, man->LitNot(y));
    man->IncRef(f);
    lit g = man->And(man->LitNot(x), y);
    man->IncRef(g);
    lit r = man->Or(f, g);
    man->DecRef(f);
    man->DecRef(g);
    return r;
  }
};

template <class Man, class lit, lit LitMax>
class TransductionBackup: ManUtil<Man, lit, LitMax> {
private:
  int nObjsAlloc;
  PfState state;
  std::list<int> vObjs;
  std::vector<std::vector<int> > vvFis;
  std::vector<std::vector<int> > vvFos;
  std::vector<int> vLevels;
  std::vector<int> vSlacks;
  std::vector<std::vector<int> > vvFiSlacks;
  std::vector<lit> vFs;
  std::vector<lit> vGs;
  std::vector<std::vector<lit> > vvCs;
  std::vector<bool> vUpdates;
  std::vector<bool> vPfUpdates;
  std::vector<bool> vFoConeShared;
  template <class Man_, class Param, class lit_, lit_ LitMax_>
  friend class Transduction;

public:
  ~TransductionBackup() {
    if(this->man) {
      this->DelVec(vFs);
      this->DelVec(vGs);
      this->DelVec(vvCs);
    }
  }
};

template <class Man, class Param, class lit, lit LitMax>
class Transduction: ManUtil<Man, lit, LitMax> {
private:
  int  nVerbose;
  int  nSortType;
  bool fLevel;
  int  nObjsAlloc;
  int  nMaxLevels;
  PfState state;
  std::vector<int> vPis;
  std::vector<int> vPos;
  std::list<int> vObjs;
  std::vector<std::vector<int> > vvFis;
  std::vector<std::vector<int> > vvFos;
  std::vector<int> vLevels;
  std::vector<int> vSlacks;
  std::vector<std::vector<int> > vvFiSlacks;
  std::vector<lit> vFs;
  std::vector<lit> vGs;
  std::vector<std::vector<lit> > vvCs;
  std::vector<bool> vUpdates;
  std::vector<bool> vPfUpdates;
  std::vector<bool> vFoConeShared;
  std::vector<lit> vPoFs;

  bool fNewLine;
  abctime startclk;

private: // Helper functions
  inline bool AllFalse(std::vector<bool> const &v) const {
    for(std::list<int>::const_iterator it = vObjs.begin(); it != vObjs.end(); it++)
      if(v[*it])
        return false;
    return true;
  }

public: // Counting
  inline int CountGates() const {
    return vObjs.size();
  }
  inline int CountWires() const {
    int count = 0;
    for(std::list<int>::const_iterator it = vObjs.begin(); it != vObjs.end(); it++)
      count += vvFis[*it].size();
    return count;
  }
  inline int CountNodes() const {
    return CountWires() - CountGates();
  }
  inline int CountLevels() const {
    int count = 0;
    for(unsigned i = 0; i < vPos.size(); i++)
      count = std::max(count, vLevels[vvFis[vPos[i]][0] >> 1]);
    return count;
  }
  inline void Print(std::string str, bool nl) const {
    if(!fNewLine)
      std::cout << "\33[2K\r";
    std::cout << str;
    if(fNewLine || nl)
      std::cout << std::endl;
    else
      std::cout << std::flush;
  }
  inline void PrintStats(std::string prefix, bool nl, int prefix_size = 0) const {
    if(!prefix_size)
      prefix_size = prefix.size();
    std::stringstream ss;
    ss << std::left << std::setw(prefix_size) << prefix << ": " << std::right;
    ss << "#nodes = " << std::setw(5) << CountNodes();
    if(fLevel)
      ss << ", #level = " << std::setw(5) << CountLevels();
    ss << ", elapsed = " << std::setprecision(2) << std::fixed << std::setw(8) << 1.0 * (Abc_Clock() - startclk) / CLOCKS_PER_SEC << "s";
    Print(ss.str(), nl);
  }
  inline void PrintPfHeader(std::string prefix, int block, int block_i0) {
    std::stringstream ss;
    ss << "\t\t" << prefix;
    if(block_i0 != -1)
      ss << " (blocking Wire " << block_i0 << " -> " << block << ")";
    else if(block != -1)
      ss << " (blocking Gate " << block << ")";
    Print(ss.str(), true);
  }

private: // MIAIG
  void SortObjs_rec(std::list<int>::iterator const &it) {
    for(unsigned j = 0; j < vvFis[*it].size(); j++) {
      int i0 = vvFis[*it][j] >> 1;
      if(!vvFis[i0].empty()) {
        std::list<int>::iterator it_i0 = std::find(it, vObjs.end(), i0);
        if(it_i0 != vObjs.end()) {
          // if(nVerbose > 6)
          //   std::cout << "\t\t\t\t\t\tMove " << i0 << " before " << *it << std::endl;
          vObjs.erase(it_i0);
          it_i0 = vObjs.insert(it, i0);
          SortObjs_rec(it_i0);
        }
      }
    }
  }
  inline void Connect(int i, int f, bool fSort = false, bool fUpdate = true, lit c = LitMax) {
    int i0 = f >> 1;
    if(nVerbose > 5) {
      std::stringstream ss;
      ss << "\t\t\t\t\tadd Wire " << std::setw(5) << i0 << "(" << (f & 1) << ")"
         << " -> " << std::setw(5) << i;
      Print(ss.str(), true);
    }
    assert(std::find(vvFis[i].begin(), vvFis[i].end(), f) == vvFis[i].end());
    vvFis[i].push_back(f);
    vvFos[i0].push_back(i);
    if(fUpdate)
      vUpdates[i] = true;
    this->IncRef(c);
    vvCs[i].push_back(c);
    if(fSort && !vvFos[i].empty() && !vvFis[i0].empty()) {
      std::list<int>::iterator it = find(vObjs.begin(), vObjs.end(), i);
      std::list<int>::iterator it_i0 = find(it, vObjs.end(), i0);
      if(it_i0 != vObjs.end()) {
        // if(nVerbose > 6)
        //   std::cout << "\t\t\t\t\t\tMove " << i0 << " before " << *it << std::endl;
        vObjs.erase(it_i0);
        it_i0 = vObjs.insert(it, i0);
        SortObjs_rec(it_i0);
      }
    }
  }
  inline void Disconnect(int i, int i0, unsigned j, bool fUpdate = true, bool fPfUpdate = true) {
    if(nVerbose > 5) {
      std::stringstream ss;
      ss << "\t\t\t\t\tremove Wire " << std::setw(5) << i0 << "(" << (vvFis[i][j] & 1) << ")"
         << " -> " << std::setw(5) << i;
      Print(ss.str(), true);
    }
    vvFos[i0].erase(std::find(vvFos[i0].begin(), vvFos[i0].end(), i));
    vvFis[i].erase(vvFis[i].begin() + j);
    this->DecRef(vvCs[i][j]);
    vvCs[i].erase(vvCs[i].begin() + j);
    if(fUpdate)
      vUpdates[i] = true;
    if(fPfUpdate)
      vPfUpdates[i0] = true;
  }
  inline int Remove(int i, bool fPfUpdate = true) {
    if(nVerbose > 5) {
      std::stringstream ss;
      ss << "\t\t\t\t\tremove Gate " << std::setw(5) << i;
      Print(ss.str(), true);
    }
    assert(vvFos[i].empty());
    for(unsigned j = 0; j < vvFis[i].size(); j++) {
      int i0 = vvFis[i][j] >> 1;
      vvFos[i0].erase(std::find(vvFos[i0].begin(), vvFos[i0].end(), i));
      if(fPfUpdate)
        vPfUpdates[i0] = true;
    }
    int count = vvFis[i].size();
    vvFis[i].clear();
    this->DecRef(vFs[i]);
    this->DecRef(vGs[i]);
    vFs[i] = vGs[i] = LitMax;
    this->DelVec(vvCs[i]);
    vUpdates[i] = vPfUpdates[i] = false;
    return count;
  }
  inline int FindFi(int i, int i0) const {
    for(unsigned j = 0; j < vvFis[i].size(); j++)
      if((vvFis[i][j] >> 1) == i0)
        return j;
    return -1;
  }
  inline int Replace(int i, int f, bool fUpdate = true) {
    if(nVerbose > 4) {
      std::stringstream ss;
      ss << "\t\t\t\treplace Gate " << std::setw(5) << i
         << " by Node " << std::setw(5) << (f >> 1) << "(" << (f & 1) << ")";
      Print(ss.str(), true);
    }
    assert(i != (f >> 1));
    int count = 0;
    for(unsigned j = 0; j < vvFos[i].size(); j++) {
      int k = vvFos[i][j];
      int l = FindFi(k, i);
      assert(l >= 0);
      int fc = f ^ (vvFis[k][l] & 1);
      if(find(vvFis[k].begin(), vvFis[k].end(), fc) != vvFis[k].end()) {
        this->DecRef(vvCs[k][l]);
        vvCs[k].erase(vvCs[k].begin() + l);
        vvFis[k].erase(vvFis[k].begin() + l);
        count++;
      } else {
        vvFis[k][l] = f ^ (vvFis[k][l] & 1);
        vvFos[f >> 1].push_back(k);
      }
      if(fUpdate)
        vUpdates[k] = true;
    }
    vvFos[i].clear();
    vPfUpdates[f >> 1] = true;
    return count + Remove(i);
  }
  int ReplaceByConst(int i, bool c) {
    if(nVerbose > 4) {
      std::stringstream ss;
      ss << "\t\t\t\treplace Gate " << std::setw(5) << i << " by Const " << c;
      Print(ss.str(), true);
    }
    int count = 0;
    for(unsigned j = 0; j < vvFos[i].size(); j++) {
      int k = vvFos[i][j];
      int l = FindFi(k, i);
      assert(l >= 0);
      bool fc = c ^ (vvFis[k][l] & 1);
      this->DecRef(vvCs[k][l]);
      vvCs[k].erase(vvCs[k].begin() + l);
      vvFis[k].erase(vvFis[k].begin() + l);
      if(fc) {
        if(vvFis[k].size() == 1)
          count += Replace(k, vvFis[k][0]);
        else
          vUpdates[k] = true;
      } else
        count += ReplaceByConst(k, 0);
    }
    count += vvFos[i].size();
    vvFos[i].clear();
    return count + Remove(i);
  }
  inline void NewGate(int &pos) {
    while(pos != nObjsAlloc && (!vvFis[pos].empty() || !vvFos[pos].empty()))
      pos++;
    if(nVerbose > 5) {
      std::stringstream ss;
      ss << "\t\t\t\t\tcreate Gate " << std::setw(5) << pos;
      Print(ss.str(), true);
    }
    if(pos == nObjsAlloc) {
      nObjsAlloc++;
      vvFis.resize(nObjsAlloc);
      vvFos.resize(nObjsAlloc);
      if(fLevel) {
        vLevels.resize(nObjsAlloc);
        vSlacks.resize(nObjsAlloc);
        vvFiSlacks.resize(nObjsAlloc);
      }
      vFs.resize(nObjsAlloc, LitMax);
      vGs.resize(nObjsAlloc, LitMax);
      vvCs.resize(nObjsAlloc);
      vUpdates.resize(nObjsAlloc);
      vPfUpdates.resize(nObjsAlloc);
    }
  }
  void MarkFiCone_rec(std::vector<bool> &vMarks, int i) const {
    if(vMarks[i])
      return;
    vMarks[i] = true;
    for(unsigned j = 0; j < vvFis[i].size(); j++)
      MarkFiCone_rec(vMarks, vvFis[i][j] >> 1);
  }
  void MarkFoCone_rec(std::vector<bool> &vMarks, int i) const {
    if(vMarks[i])
      return;
    vMarks[i] = true;
    for(unsigned j = 0; j < vvFos[i].size(); j++)
      MarkFoCone_rec(vMarks, vvFos[i][j]);
  }
  bool IsFoConeShared_rec(std::vector<int> &vVisits, int i, int visitor) const {
    if(vVisits[i] == visitor)
      return false;
    if(vVisits[i])
      return true;
    vVisits[i] = visitor;
    for(unsigned j = 0; j < vvFos[i].size(); j++)
      if(IsFoConeShared_rec(vVisits, vvFos[i][j], visitor))
        return true;
    return false;
  }
  inline bool IsFoConeShared(int i) const {
    std::vector<int> vVisits(nObjsAlloc);
    for(unsigned j = 0; j < vvFos[i].size(); j++)
      if(IsFoConeShared_rec(vVisits, vvFos[i][j], j + 1))
        return true;
    return false;
  }

private: // Level calculation
  inline void add(std::vector<bool> &a, unsigned i) {
    if(a.size() <= i) {
      a.resize(i + 1);
      a[i] = true;
      return;
    }
    for(; i < a.size() && a[i]; i++)
      a[i] = false;
    if(i == a.size())
      a.resize(i + 1);
    a[i] = true;
  }
  inline bool balanced(std::vector<bool> const &a) {
    for(unsigned i = 0; i < a.size() - 1; i++)
      if(a[i])
        return false;
    return true;
  }
  inline bool noexcess(std::vector<bool> const &a, unsigned i) {
    if(a.size() <= i)
      return false;
    for(unsigned j = i; j < a.size(); j++)
      if(!a[j])
        return true;
    for(unsigned j = 0; j < i; j++)
      if(a[j])
        return false;
    return true;
  }
  inline void ComputeLevel() {
    for(std::list<int>::iterator it = vObjs.begin(); it != vObjs.end(); it++) {
      if(vvFis[*it].size() == 2)
        vLevels[*it] = std::max(vLevels[vvFis[*it][0] >> 1], vLevels[vvFis[*it][1] >> 1]) + 1;
      else {
        std::vector<bool> lev;
        for(unsigned j = 0; j < vvFis[*it].size(); j++)
          add(lev, vLevels[vvFis[*it][j] >> 1]);
        if(balanced(lev))
          vLevels[*it] = (int)lev.size() - 1;
        else
          vLevels[*it] = (int)lev.size();
      }
    }
    if(nMaxLevels == -1)
      nMaxLevels = CountLevels();
    for(unsigned i = 0; i < vPos.size(); i++) {
      vvFiSlacks[vPos[i]].resize(1);
      vvFiSlacks[vPos[i]][0] = nMaxLevels - vLevels[vvFis[vPos[i]][0] >> 1];
    }
    for(std::list<int>::reverse_iterator it = vObjs.rbegin(); it != vObjs.rend(); it++) {
      vSlacks[*it] = nMaxLevels;
      for(unsigned j = 0; j < vvFos[*it].size(); j++) {
        int k = vvFos[*it][j];
        int l = FindFi(k, *it);
        assert(l >= 0);
        vSlacks[*it] = std::min(vSlacks[*it], vvFiSlacks[k][l]);
      }
      vvFiSlacks[*it].resize(vvFis[*it].size());
      for(unsigned j = 0; j < vvFis[*it].size(); j++)
        vvFiSlacks[*it][j] = vSlacks[*it] + vLevels[*it] - 1 - vLevels[vvFis[*it][j] >> 1];
    }
  }

private: // Cost function
  inline void ShufflePis(int seed) {
    srand(seed);
    for(int i = (int)vPis.size() - 1; i > 0; i--)
      std::swap(vPis[i], vPis[rand() % (i + 1)]);
  }
  inline bool CostCompare(int a, int b) const { // return (cost(a) > cost(b))
    int a0 = a >> 1;
    int b0 = b >> 1;
    if(vvFis[a0].empty() && vvFis[b0].empty())
      return std::find(find(vPis.begin(), vPis.end(), a0), vPis.end(), b0) != vPis.end();
    if(vvFis[a0].empty() && !vvFis[b0].empty())
      return false;
    if(!vvFis[a0].empty() && vvFis[b0].empty())
      return true;
    if(vvFos[a0].size() > vvFos[b0].size())
      return false;
    if(vvFos[a0].size() < vvFos[b0].size())
      return true;
    bool ac = a & 1;
    bool bc = b & 1;
    switch(nSortType) {
    case 0:
      return std::find(find(vObjs.begin(), vObjs.end(), a0), vObjs.end(), b0) == vObjs.end();
    case 1:
      return this->man->OneCount(this->man->LitNotCond(vFs[a0], ac)) < this->man->OneCount(this->man->LitNotCond(vFs[b0], bc));
    case 2:
      return this->man->OneCount(vFs[a0]) < this->man->OneCount(vFs[b0]);
    case 3:
      return this->man->OneCount(this->man->LitNot(vFs[a0])) < this->man->OneCount(vFs[b0]); // pseudo random
    default:
      return false; // no sorting
    }
  }
  inline bool SortFis(int i) {
    if(nVerbose > 5) {
      std::stringstream ss;
      ss << "\t\t\t\tsort FIs Gate " << std::setw(5) << i;
      Print(ss.str(), true);
    }
    bool fSort = false;
    for(int p = 1; p < (int)vvFis[i].size(); p++) {
      int f = vvFis[i][p];
      lit c = vvCs[i][p];
      int q = p - 1;
      for(; q >= 0 && CostCompare(f, vvFis[i][q]); q--) {
        vvFis[i][q + 1] = vvFis[i][q];
        vvCs[i][q + 1] = vvCs[i][q];
      }
      if(q + 1 != p) {
        fSort = true;
        vvFis[i][q + 1] = f;
        vvCs[i][q + 1] = c;
      }
    }
    if(nVerbose > 5)
      for(unsigned j = 0; j < vvFis[i].size(); j++) {
        std::stringstream ss;
        ss << "\t\t\t\t\tFI " << j << " = "
           << std::setw(5) << (vvFis[i][j] >> 1) << "(" << (vvFis[i][j] & 1) << ")";
        Print(ss.str(), true);
      }
    return fSort;
  }

private: // Symbolic simulation
  inline lit LitFi(int i, int j) const {
    int i0 = vvFis[i][j] >> 1;
    bool c0 = vvFis[i][j] & 1;
    return this->man->LitNotCond(vFs[i0], c0);
  }
  inline lit LitFi(int i, int j, std::vector<lit> const &vFs_) const {
    int i0 = vvFis[i][j] >> 1;
    bool c0 = vvFis[i][j] & 1;
    return this->man->LitNotCond(vFs_[i0], c0);
  }
  inline void Build(int i, std::vector<lit> &vFs_) const {
    if(nVerbose > 6) {
      std::stringstream ss;
      ss << "\t\t\t\tbuilding Gate" << std::setw(5) << i;
      Print(ss.str(), nVerbose > 7);
    }
    this->Update(vFs_[i], this->man->Const1());
    for(unsigned j = 0; j < vvFis[i].size(); j++)
      this->Update(vFs_[i], this->man->And(vFs_[i], LitFi(i, j, vFs_)));
  }
  inline void Build(bool fPfUpdate = true) {
    if(nVerbose > 6)
      Print("\t\t\tBuild", true);
    for(std::list<int>::iterator it = vObjs.begin(); it != vObjs.end(); it++)
      if(vUpdates[*it]) {
        lit x = vFs[*it];
        this->IncRef(x);
        Build(*it, vFs);
        this->DecRef(x);
        if(!this->man->LitIsEq(x, vFs[*it]))
          for(unsigned j = 0; j < vvFos[*it].size(); j++)
            vUpdates[vvFos[*it][j]] = true;
      }
    if(fPfUpdate)
      for(std::list<int>::iterator it = vObjs.begin(); it != vObjs.end(); it++)
        vPfUpdates[*it] = vPfUpdates[*it] || vUpdates[*it];
    for(std::list<int>::iterator it = vObjs.begin(); it != vObjs.end(); it++)
      vUpdates[*it] = false;
    assert(AllFalse(vUpdates));
  }
  void BuildFoConeCompl(int i, std::vector<lit> &vPoFsCompl) const {
    if(nVerbose > 6) {
      std::stringstream ss;
      ss << "\t\t\tBuild with complemented Gate " << std::setw(5) << i;
      Print(ss.str(), true);
    }
    std::vector<lit> vFsCompl;
    this->CopyVec(vFsCompl, vFs);
    vFsCompl[i] = this->man->LitNot(vFs[i]);
    std::vector<bool> vUpdatesCompl(nObjsAlloc);
    for(unsigned j = 0; j < vvFos[i].size(); j++)
      vUpdatesCompl[vvFos[i][j]] = true;
    for(std::list<int>::const_iterator it = vObjs.begin(); it != vObjs.end(); it++)
      if(vUpdatesCompl[*it]) {
        Build(*it, vFsCompl);
        if(!this->man->LitIsEq(vFsCompl[*it], vFs[*it]))
          for(unsigned j = 0; j < vvFos[*it].size(); j++)
            vUpdatesCompl[vvFos[*it][j]] = true;
      }
    for(unsigned j = 0; j < vPos.size(); j++)
      this->Update(vPoFsCompl[j], LitFi(vPos[j], 0, vFsCompl));
    this->DelVec(vFsCompl);
  }

private: // CSPF
  inline int RemoveRedundantFis(int i, int block_i0 = -1, unsigned j = 0) {
    int count = 0;
    for(; j < vvFis[i].size(); j++) {
      if(block_i0 == (vvFis[i][j] >> 1))
        continue;
      lit x = this->man->Const1();
      this->IncRef(x);
      for(unsigned jj = 0; jj < vvFis[i].size(); jj++)
        if(j != jj)
          this->Update(x, this->man->And(x, LitFi(i, jj)));
      this->Update(x, this->man->Or(this->man->LitNot(x), vGs[i]));
      this->Update(x, this->man->Or(x, LitFi(i, j)));
      this->DecRef(x);
      if(this->man->IsConst1(x)) {
        int i0 = vvFis[i][j] >> 1;
        if(nVerbose > 4) {
          std::stringstream ss;
          ss << "\t\t\t\t[Rrfi] remove Wire " << std::setw(5) << i0 << "(" << (vvFis[i][j] & 1) << ")"
             << " -> " << std::setw(5) << i;
          Print(ss.str(), true);
        }
        Disconnect(i, i0, j--);
        count++;
      }
    }
    return count;
  }
  inline void CalcG(int i) {
    this->Update(vGs[i], this->man->Const1());
    for(unsigned j = 0; j < vvFos[i].size(); j++) {
      int k = vvFos[i][j];
      int l = FindFi(k, i);
      assert(l >= 0);
      this->Update(vGs[i], this->man->And(vGs[i], vvCs[k][l]));
    }
  }
  inline int CalcC(int i) {
    int count = 0;
    for(unsigned j = 0; j < vvFis[i].size(); j++) {
      lit x = this->man->Const1();
      this->IncRef(x);
      for(unsigned jj = j + 1; jj < vvFis[i].size(); jj++)
        this->Update(x, this->man->And(x, LitFi(i, jj)));
      this->Update(x, this->man->Or(this->man->LitNot(x), vGs[i]));
      int i0 = vvFis[i][j] >> 1;
      if(this->man->IsConst1(this->man->Or(x, LitFi(i, j)))) {
        if(nVerbose > 4) {
          std::stringstream ss;
          ss << "\t\t\t\t[Cspf] remove Wire " << std::setw(5) << i0 << "(" << (vvFis[i][j] & 1) << ")"
             << " -> " << std::setw(5) << i;
          Print(ss.str(), true);
        }
        Disconnect(i, i0, j--);
        count++;
      } else if(!this->man->LitIsEq(vvCs[i][j], x)) {
        this->Update(vvCs[i][j], x);
        vPfUpdates[i0] = true;
      }
      this->DecRef(x);
    }
    return count;
  }
  int Cspf(bool fSortRemove, int block = -1, int block_i0 = -1) {
    if(nVerbose > 3)
      PrintPfHeader("Cspf", block, block_i0);
    if(state != cspf)
      for(std::list<int>::iterator it = vObjs.begin(); it != vObjs.end(); it++)
        vPfUpdates[*it] = true;
    state = cspf;
    int count = 0;
    for(std::list<int>::reverse_iterator it = vObjs.rbegin(); it != vObjs.rend();) {
      if(vvFos[*it].empty()) {
        if(nVerbose > 3) {
          std::stringstream ss;
          ss << "\t\t\tremove unused Gate " << std::setw(5) << *it;
          Print(ss.str(), nVerbose > 4);
        }
        count += Remove(*it);
        it = std::list<int>::reverse_iterator(vObjs.erase(--(it.base())));
        continue;
      }
      if(!vPfUpdates[*it]) {
        it++;
        continue;
      }
      if(nVerbose > 3) {
        std::stringstream ss;
        ss << "\t\t\tprocessing Gate " << std::setw(5) << *it
           << " (" << std::setw(5) << std::distance(vObjs.rbegin(), it) + 1
           << "/" << std::setw(5) << vObjs.size() << ")";
        Print(ss.str(), nVerbose > 4);
      }
      CalcG(*it);
      if(fSortRemove) {
        if(*it != block)
          SortFis(*it), count += RemoveRedundantFis(*it);
        else if(block_i0 != -1)
          count += RemoveRedundantFis(*it, block_i0);
      }
      count += CalcC(*it);
      vPfUpdates[*it] = false;
      assert(!vvFis[*it].empty());
      if(vvFis[*it].size() == 1) {
        count += Replace(*it, vvFis[*it][0]);
        it = std::list<int>::reverse_iterator(vObjs.erase(--(it.base())));
        continue;
      }
      it++;
    }
    Build(false);
    assert(AllFalse(vPfUpdates));
    if(fLevel)
      ComputeLevel();
    return count;
  }

private: // MSPF
  inline bool MspfCalcG(int i) {
    lit g = vGs[i];
    this->IncRef(g);
    std::vector<lit> vPoFsCompl(vPos.size(), LitMax);
    BuildFoConeCompl(i, vPoFsCompl);
    this->Update(vGs[i], this->man->Const1());
    for(unsigned j = 0; j < vPos.size(); j++) {
      lit x = this->man->LitNot(this->Xor(vPoFs[j], vPoFsCompl[j]));
      this->IncRef(x);
      this->Update(x, this->man->Or(x, vvCs[vPos[j]][0]));
      this->Update(vGs[i], this->man->And(vGs[i], x));
      this->DecRef(x);
    }
    this->DelVec(vPoFsCompl);
    this->DecRef(g);
    return !this->man->LitIsEq(vGs[i], g);
  }
  inline int MspfCalcC(int i, int block_i0 = -1) {
    for(unsigned j = 0; j < vvFis[i].size(); j++) {
      lit x = this->man->Const1();
      this->IncRef(x);
      for(unsigned jj = 0; jj < vvFis[i].size(); jj++)
        if(j != jj)
          this->Update(x, this->man->And(x, LitFi(i, jj)));
      this->Update(x, this->man->Or(this->man->LitNot(x), vGs[i]));
      int i0 = vvFis[i][j] >> 1;
      if(i0 != block_i0 && this->man->IsConst1(this->man->Or(x, LitFi(i, j)))) {
        if(nVerbose > 4) {
          std::stringstream ss;
          ss << "\t\t\t\t[Mspf] remove Wire " << std::setw(5) << i0 << "(" << (vvFis[i][j] & 1) << ")"
             << " -> " << std::setw(5) << i;
          Print(ss.str(), true);
        }
        Disconnect(i, i0, j);
        this->DecRef(x);
        return RemoveRedundantFis(i, block_i0, j) + 1;
      } else if(!this->man->LitIsEq(vvCs[i][j], x)) {
        this->Update(vvCs[i][j], x);
        vPfUpdates[i0] = true;
      }
      this->DecRef(x);
    }
    return 0;
  }
  int Mspf(bool fSort, int block = -1, int block_i0 = -1) {
    if(nVerbose > 3)
      PrintPfHeader("Mspf", block, block_i0);
    assert(AllFalse(vUpdates));
    vFoConeShared.resize(nObjsAlloc);
    if(state != mspf)
      for(std::list<int>::iterator it = vObjs.begin(); it != vObjs.end(); it++)
        vPfUpdates[*it] = true;
    state = mspf;
    int count = 0;
    int restarts = 0;
    for(std::list<int>::reverse_iterator it = vObjs.rbegin(); it != vObjs.rend();) {
      if(vvFos[*it].empty()) {
        if(nVerbose > 3) {
          std::stringstream ss;
          ss << "\t\t\tremove unused Gate " << std::setw(5) << *it;
          Print(ss.str(), nVerbose > 4);
        }
        count += Remove(*it);
        it = std::list<int>::reverse_iterator(vObjs.erase(--(it.base())));
        continue;
      }
      if(!vFoConeShared[*it] && !vPfUpdates[*it] && (vvFos[*it].size() == 1 || !IsFoConeShared(*it))) {
        it++;
        continue;
      }
      if(nVerbose > 3) {
        std::stringstream ss;
        ss << "\t\t\tprocessing Gate " << std::setw(5) << *it
           << " (" << std::setw(5) << std::distance(vObjs.rbegin(), it) + 1
           << "/" << std::setw(5) << vObjs.size() << ")"
           << ", #restarts = " << std::setw(3) << restarts;
        Print(ss.str(), nVerbose > 4);
      }
      if(vvFos[*it].size() == 1 || !IsFoConeShared(*it)) {
        if(vFoConeShared[*it]) {
          vFoConeShared[*it] = false;
          lit g = vGs[*it];
          this->IncRef(g);
          CalcG(*it);
          this->DecRef(g);
          if(g == vGs[*it] && !vPfUpdates[*it]) {
            it++;
            continue;
          }
        } else
          CalcG(*it);
      } else {
        vFoConeShared[*it] = true;
        if(!MspfCalcG(*it) && !vPfUpdates[*it]) {
          it++;
          continue;
        }
        bool IsConst1 = this->man->IsConst1(this->man->Or(vGs[*it], vFs[*it]));
        bool IsConst0 = IsConst1? false: this->man->IsConst1(this->man->Or(vGs[*it], this->man->LitNot(vFs[*it])));
        if(IsConst1 || IsConst0) {
          count += ReplaceByConst(*it, (int)IsConst1);
          vObjs.erase(--(it.base()));
          Build();
          it = vObjs.rbegin();
          restarts++;
          continue;
        }
      }
      if(fSort && block != *it)
        SortFis(*it);
      if(int diff = (block == *it)? MspfCalcC(*it, block_i0): MspfCalcC(*it)) {
        count += diff;
        assert(!vvFis[*it].empty());
        if(vvFis[*it].size() == 1) {
          count += Replace(*it, vvFis[*it][0]);
          vObjs.erase(--(it.base()));
        }
        Build();
        it = vObjs.rbegin();
        restarts++;
        continue;
      }
      vPfUpdates[*it] = false;
      it++;
    }
    assert(AllFalse(vUpdates));
    assert(AllFalse(vPfUpdates));
    if(fLevel)
      ComputeLevel();
    return count;
  }

private: // Merge/decompose one
  inline int TrivialMergeOne(int i) {
    int count = 0;
    std::vector<int> vFisOld = vvFis[i];
    std::vector<lit> vCsOld = vvCs[i];
    vvFis[i].clear();
    vvCs[i].clear();
    for(unsigned j = 0; j < vFisOld.size(); j++) {
      int i0 = vFisOld[j] >> 1;
      int c0 = vFisOld[j] & 1;
      if(vvFis[i0].empty() || vvFos[i0].size() > 1 || c0) {
        if(nVerbose > 3) {
          std::stringstream ss;
          ss << "\t\t[TrivialMerge] adding Wire " << std::setw(5) << i0 << "(" << c0 << ")"
             << " -> " << std::setw(5) << i;
          Print(ss.str(), true);
        }
        vvFis[i].push_back(vFisOld[j]);
        vvCs[i].push_back(vCsOld[j]);
        continue;
      }
      vPfUpdates[i] = vPfUpdates[i] | vPfUpdates[i0];
      vvFos[i0].erase(find(vvFos[i0].begin(), vvFos[i0].end(), i));
      count++;
      typename std::vector<int>::iterator itfi = vFisOld.begin() + j;
      typename std::vector<lit>::iterator itc = vCsOld.begin() + j;
      for(unsigned jj = 0; jj < vvFis[i0].size(); jj++) {
        int f = vvFis[i0][jj];
        std::vector<int>::iterator it = find(vvFis[i].begin(), vvFis[i].end(), f);
        if(it == vvFis[i].end()) {
          vvFos[f >> 1].push_back(i);
          itfi = vFisOld.insert(itfi, f);
          itc = vCsOld.insert(itc, vvCs[i0][jj]);
          this->IncRef(*itc);
          itfi++;
          itc++;
          count--;
        } else {
          assert(state == none);
        }
      }
      count += Remove(i0, false);
      vObjs.erase(find(vObjs.begin(), vObjs.end(), i0));
      vFisOld.erase(itfi);
      this->DecRef(*itc);
      vCsOld.erase(itc);
      j--;
    }
    return count;
  }
  inline int TrivialDecomposeOne(std::list<int>::iterator const &it, int &pos) {
    if(nVerbose > 2) {
      std::stringstream ss;
      ss << "\tTrivialDecompose Gate " << std::setw(5) << *it;
      Print(ss.str() , nVerbose > 3);
    }
    assert(vvFis[*it].size() > 2);
    int count = 2 - vvFis[*it].size();
    while(vvFis[*it].size() > 2) {
      int f0 = vvFis[*it].back();
      lit c0 = vvCs[*it].back();
      Disconnect(*it, f0 >> 1, vvFis[*it].size() - 1, false, false);
      int f1 = vvFis[*it].back();
      lit c1 = vvCs[*it].back();
      Disconnect(*it, f1 >> 1, vvFis[*it].size() - 1, false, false);
      NewGate(pos);
      Connect(pos, f1, false, false, c1);
      Connect(pos, f0, false, false, c0);
      if(!vPfUpdates[*it]) {
        if(state == cspf)
          this->Update(vGs[pos], vGs[*it]);
        else if(state == mspf) {
          lit x = this->man->Const1();
          this->IncRef(x);
          for(unsigned j = 0; j < vvFis[*it].size(); j++)
            this->Update(x, this->man->And(x, LitFi(*it, j)));
          this->Update(vGs[pos], this->man->Or(this->man->LitNot(x), vGs[*it]));
          this->DecRef(x);
        }
      }
      Connect(*it, pos << 1, false, false, vGs[pos]);
      vObjs.insert(it, pos);
      Build(pos, vFs);
    }
    return count;
  }
  inline int BalancedDecomposeOne(std::list<int>::iterator const &it, int &pos) {
    if(nVerbose > 2) {
      std::stringstream ss;
      ss << "\tBalancedDecompose Gate " << std::setw(5) << *it;
      Print(ss.str(), nVerbose > 3);
    }
    assert(fLevel);
    assert(vvFis[*it].size() > 2);
    for(int p = 1; p < (int)vvFis[*it].size(); p++) {
      int f = vvFis[*it][p];
      lit c = vvCs[*it][p];
      int q = p - 1;
      for(; q >= 0 && vLevels[f >> 1] > vLevels[vvFis[*it][q] >> 1]; q--) {
        vvFis[*it][q + 1] = vvFis[*it][q];
        vvCs[*it][q + 1] = vvCs[*it][q];
      }
      if(q + 1 != p) {
        vvFis[*it][q + 1] = f;
        vvCs[*it][q + 1] = c;
      }
    }
    int count = 2 - vvFis[*it].size();
    while(vvFis[*it].size() > 2) {
      int f0 = vvFis[*it].back();
      lit c0 = vvCs[*it].back();
      Disconnect(*it, f0 >> 1, vvFis[*it].size() - 1, false, false);
      int f1 = vvFis[*it].back();
      lit c1 = vvCs[*it].back();
      Disconnect(*it, f1 >> 1, vvFis[*it].size() - 1, false, false);
      NewGate(pos);
      Connect(pos, f1, false, false, c1);
      Connect(pos, f0, false, false, c0);
      Connect(*it, pos << 1, false, false);
      Build(pos, vFs);
      vLevels[pos] = std::max(vLevels[f0 >> 1], vLevels[f1 >> 1]) + 1;
      vObjs.insert(it, pos);
      int f = vvFis[*it].back();
      lit c = vvCs[*it].back();
      int q = (int)vvFis[*it].size() - 2;
      for(; q >= 0 && vLevels[f >> 1] > vLevels[vvFis[*it][q] >> 1]; q--) {
        vvFis[*it][q + 1] = vvFis[*it][q];
        vvCs[*it][q + 1] = vvCs[*it][q];
      }
      if(q + 1 != (int)vvFis[*it].size() - 1) {
        vvFis[*it][q + 1] = f;
        vvCs[*it][q + 1] = c;
      }
    }
    vPfUpdates[*it] = true;
    return count;
  }

public: // Merge/decompose
  int TrivialMerge() {
    int count = 0;
    for(std::list<int>::reverse_iterator it = vObjs.rbegin(); it != vObjs.rend();) {
      count += TrivialMergeOne(*it);
      it++;
    }
    return count;
  }
  int TrivialDecompose() {
    int count = 0;
    int pos = vPis.size() + 1;
    for(std::list<int>::iterator it = vObjs.begin(); it != vObjs.end(); it++)
      if(vvFis[*it].size() > 2)
        count += TrivialDecomposeOne(it, pos);
    return count;
  }
  int Decompose() {
    int count = 0;
    int pos = vPis.size() + 1;
    for(std::list<int>::iterator it = vObjs.begin(); it != vObjs.end(); it++) {
      std::set<int> s1(vvFis[*it].begin(), vvFis[*it].end());
      assert(s1.size() == vvFis[*it].size());
      std::list<int>::iterator it2 = it;
      for(it2++; it2 != vObjs.end(); it2++) {
        std::set<int> s2(vvFis[*it2].begin(), vvFis[*it2].end());
        std::set<int> s;
        std::set_intersection(s1.begin(), s1.end(), s2.begin(), s2.end(), std::inserter(s, s.begin()));
        if(s.size() > 1) {
          if(s == s1) {
            if(s == s2) {
              if(nVerbose > 2) {
                std::stringstream ss;
                ss << "\treplace Gate " << std::setw(5) << *it2
                   << " by Gate " << std::setw(5) << *it;
                Print(ss.str() , nVerbose > 3);
              }
              count += Replace(*it2, *it << 1, false);
              it2 = vObjs.erase(it2);
              it2--;
            } else {
              if(nVerbose > 2) {
                std::stringstream ss;
                ss << "\tdecompose Gate " << std::setw(5) << *it2
                   << " by Gate " << std::setw(5) << *it;
                Print(ss.str() , nVerbose > 3);
              }
              for(std::set<int>::iterator it3 = s.begin(); it3 != s.end(); it3++) {
                unsigned j = find(vvFis[*it2].begin(), vvFis[*it2].end(), *it3) - vvFis[*it2].begin();
                Disconnect(*it2, *it3 >> 1, j, false);
              }
              count += s.size();
              if(std::find(vvFis[*it2].begin(), vvFis[*it2].end(), *it << 1) == vvFis[*it2].end()) {
                Connect(*it2, *it << 1, false, false);
                count--;
              }
              vPfUpdates[*it2] = true;
            }
            continue;
          }
          if(s == s2) {
            it = vObjs.insert(it, *it2);
            vObjs.erase(it2);
          } else {
            NewGate(pos);
            if(nVerbose > 2) {
              std::stringstream ss;
              ss << "\tdecompose Gate " << std::setw(5) << *it
                 << " and " << std::setw(5) << *it2
                 << " by a new Gate " << std::setw(5) <<  pos;
              Print(ss.str() , nVerbose > 3);
            }
            if(nVerbose > 4) {
              std::stringstream ss;
              ss << "\t\tIntersection:";
              for(std::set<int>::iterator it3 = s.begin(); it3 != s.end(); it3++)
                ss << " " << (*it3 >> 1) << "(" << (*it3 & 1) << ")";
              Print(ss.str(), true);
            }
            for(std::set<int>::iterator it3 = s.begin(); it3 != s.end(); it3++)
              Connect(pos, *it3, false, false);
            count -= s.size();
            it = vObjs.insert(it, pos);
            Build(pos, vFs);
            vPfUpdates[*it] = true;
          }
          s1 = s;
          it2 = it;
        }
      }
      if(vvFis[*it].size() > 2)
        count += TrivialDecomposeOne(it, pos);
    }
    return count;
  }

private: // Save/load
  inline void Save(TransductionBackup<Man, lit, LitMax> &b) const {
    b.man = this->man;
    b.nObjsAlloc = nObjsAlloc;
    b.state = state;
    b.vObjs = vObjs;
    b.vvFis = vvFis;
    b.vvFos = vvFos;
    b.vLevels = vLevels;
    b.vSlacks = vSlacks;
    b.vvFiSlacks = vvFiSlacks;
    this->CopyVec(b.vFs, vFs);
    this->CopyVec(b.vGs, vGs);
    this->CopyVec(b.vvCs, vvCs);
    b.vUpdates = vUpdates;
    b.vPfUpdates = vPfUpdates;
    b.vFoConeShared = vFoConeShared;
  }
  inline void Load(TransductionBackup<Man, lit, LitMax> const &b) {
    nObjsAlloc = b.nObjsAlloc;
    state = b.state;
    vObjs = b.vObjs;
    vvFis = b.vvFis;
    vvFos = b.vvFos;
    vLevels = b.vLevels;
    vSlacks = b.vSlacks;
    vvFiSlacks = b.vvFiSlacks;
    this->CopyVec(vFs, b.vFs);
    this->CopyVec(vGs, b.vGs);
    this->CopyVec(vvCs, b.vvCs);
    vUpdates = b.vUpdates;
    vPfUpdates = b.vPfUpdates;
    vFoConeShared = b.vFoConeShared;
  }

private: // Connectable condition
  inline bool TryConnect(int i, int i0, bool c0) {
    int f = (i0 << 1) ^ (int)c0;
    if(find(vvFis[i].begin(), vvFis[i].end(), f) == vvFis[i].end()) {
      lit x = this->man->Or(this->man->LitNot(vFs[i]), vGs[i]);
      this->IncRef(x);
      if(this->man->IsConst1(this->man->Or(x, this->man->LitNotCond(vFs[i0], c0)))) {
        this->DecRef(x);
        if(nVerbose > 3) {
          std::stringstream ss;
          ss << "\t\t[TryConnect] adding Wire " << std::setw(5) << i0 << "(" << c0 << ")"
             << " -> " << std::setw(5) << i;
          Print(ss.str(), true);
        }
        Connect(i, f, true);
        return true;
      }
      this->DecRef(x);
    }
    return false;
  }

public: // Resubs
  int Resub(bool fMspf) {
    int count = fMspf? Mspf(true): Cspf(true);
    int nodes = CountNodes();
    TransductionBackup<Man, lit, LitMax> b;
    Save(b);
    int count_ = count;
    std::list<int> targets = vObjs;
    for(std::list<int>::reverse_iterator it = targets.rbegin(); it != targets.rend(); it++) {
      if(nVerbose > 1) {
        std::stringstream ss;
        ss << "[Resub] processing Gate " << std::setw(5) << *it
           << " (" << std::setw(5) << std::distance(targets.rbegin(), it) + 1
           << "/" << std::setw(5) << targets.size() << ")";
        PrintStats(ss.str(), nVerbose > 2);
      }
      if(vvFos[*it].empty())
        continue;
      count += TrivialMergeOne(*it);
      std::vector<bool> lev;
      if(fLevel) {
        for(unsigned j = 0; j < vvFis[*it].size(); j++)
          add(lev, vLevels[vvFis[*it][j] >> 1]);
        if((int)lev.size() > vLevels[*it] + vSlacks[*it]) {
          Load(b);
          count = count_;
          continue;
        }
        lev.resize(vLevels[*it] + vSlacks[*it]);
      }
      bool fConnect = false;
      std::vector<bool> vMarks(nObjsAlloc);
      MarkFoCone_rec(vMarks, *it);
      std::list<int> targets2 = vObjs;
      for(std::list<int>::iterator it2 = targets2.begin(); it2 != targets2.end(); it2++) {
        if(fLevel && (int)lev.size() > vLevels[*it] + vSlacks[*it])
          break;
        if(!vMarks[*it2] && !vvFos[*it2].empty())
          if(!fLevel || noexcess(lev, vLevels[*it2]))
            if(TryConnect(*it, *it2, false) || TryConnect(*it, *it2, true)) {
              fConnect = true;
              count--;
              if(fLevel)
                add(lev, vLevels[*it2]);
            }
      }
      if(fConnect) {
        if(fMspf) {
          Build();
          count += Mspf(true, *it);
        } else {
          vPfUpdates[*it] = true;
          count += Cspf(true, *it);
        }
        if(!vvFos[*it].empty()) {
          vPfUpdates[*it] = true;
          count += fMspf? Mspf(true): Cspf(true);
        }
      }
      if(nodes < CountNodes()) {
        Load(b);
        count = count_;
        continue;
      }
      if(!vvFos[*it].empty() && vvFis[*it].size() > 2) {
        std::list<int>::iterator it2 = find(vObjs.begin(), vObjs.end(), *it);
        int pos = nObjsAlloc;
        if(fLevel)
          count += BalancedDecomposeOne(it2, pos) + (fMspf? Mspf(true): Cspf(true));
        else
          count += TrivialDecomposeOne(it2, pos);
      }
      nodes = CountNodes();
      Save(b);
      count_ = count;
    }
    if(nVerbose)
      PrintStats("Resub", true, 11);
    return count;
  }
  int ResubMono(bool fMspf) {
    int count = fMspf? Mspf(true): Cspf(true);
    std::list<int> targets = vObjs;
    for(std::list<int>::reverse_iterator it = targets.rbegin(); it != targets.rend(); it++) {
      if(nVerbose > 1) {
        std::stringstream ss;
        ss << "[ResubMono] processing Gate " << std::setw(5) << *it
           << " (" << std::setw(5) << std::distance(targets.rbegin(), it) + 1
           << "/" << std::setw(5) << targets.size() << ")";
        PrintStats(ss.str(), nVerbose > 2);
      }
      if(vvFos[*it].empty())
        continue;
      count += TrivialMergeOne(*it);
      TransductionBackup<Man, lit, LitMax> b;
      Save(b);
      int count_ = count;
      for(unsigned i = 0; i < vPis.size(); i++) {
        if(vvFos[*it].empty())
          break;
        if(nVerbose > 2) {
          std::stringstream ss;
          ss << "\ttrying a new fanin PI " << std::setw(2) << i;
          PrintStats(ss.str(), nVerbose > 3);
        }
        if(TryConnect(*it, vPis[i], false) || TryConnect(*it, vPis[i], true)) {
          count--;
          int diff;
          if(fMspf) {
            Build();
            diff = Mspf(true, *it, vPis[i]);
          } else {
            vPfUpdates[*it] = true;
            diff = Cspf(true, *it, vPis[i]);
          }
          if(diff) {
            count += diff;
            if(!vvFos[*it].empty()) {
              vPfUpdates[*it] = true;
              count += fMspf? Mspf(true): Cspf(true);
            }
            if(fLevel && CountLevels() > nMaxLevels) {
              Load(b);
              count = count_;
            } else {
              Save(b);
              count_ = count;
            }
          } else {
            Load(b);
            count = count_;
          }
        }
      }
      if(vvFos[*it].empty())
        continue;
      std::vector<bool> vMarks(nObjsAlloc);
      MarkFoCone_rec(vMarks, *it);
      std::list<int> targets2 = vObjs;
      for(std::list<int>::iterator it2 = targets2.begin(); it2 != targets2.end(); it2++) {
        if(vvFos[*it].empty())
          break;
        if(nVerbose > 2) {
          std::stringstream ss;
          ss << "\ttrying a new fanin Gate " << std::setw(5) << *it2
             << " (" << std::setw(5) << std::distance(targets2.begin(), it2) + 1
             << "/" << std::setw(5) << targets2.size() << ")";
          PrintStats(ss.str(), nVerbose > 3);
        }
        if(!vMarks[*it2] && !vvFos[*it2].empty())
          if(TryConnect(*it, *it2, false) || TryConnect(*it, *it2, true)) {
            count--;
            int diff;
            if(fMspf) {
              Build();
              diff = Mspf(true, *it, *it2);
            } else {
              vPfUpdates[*it] = true;
              diff = Cspf(true, *it, *it2);
            }
            if(diff) {
              count += diff;
              if(!vvFos[*it].empty()) {
                vPfUpdates[*it] = true;
                count += fMspf? Mspf(true): Cspf(true);
              }
              if(fLevel && CountLevels() > nMaxLevels) {
                Load(b);
                count = count_;
              } else {
                Save(b);
                count_ = count;
              }
            } else {
              Load(b);
              count = count_;
            }
          }
      }
      if(vvFos[*it].empty())
        continue;
      if(vvFis[*it].size() > 2) {
        std::list<int>::iterator it2 = find(vObjs.begin(), vObjs.end(), *it);
        int pos = nObjsAlloc;
        if(fLevel)
          count += BalancedDecomposeOne(it2, pos) + (fMspf? Mspf(true): Cspf(true));
        else
          count += TrivialDecomposeOne(it2, pos);
      }
    }
    if(nVerbose)
      PrintStats("ResubMono", true, 11);
    return count;
  }
  int ResubShared(bool fMspf) {
    int count = fMspf? Mspf(true): Cspf(true);
    std::list<int> targets = vObjs;
    for(std::list<int>::reverse_iterator it = targets.rbegin(); it != targets.rend(); it++) {
      if(nVerbose > 1) {
        std::stringstream ss;
        ss << "[ResubShared] processing Gate " << std::setw(5) << *it
           << " (" << std::setw(5) << std::distance(targets.rbegin(), it) + 1
           << "/" << std::setw(5) << targets.size() << ")";
        PrintStats(ss.str(), nVerbose > 2);
      }
      if(vvFos[*it].empty())
        continue;
      count += TrivialMergeOne(*it);
      bool fConnect = false;
      for(unsigned i = 0; i < vPis.size(); i++)
        if(TryConnect(*it, vPis[i], false) || TryConnect(*it, vPis[i], true)) {
          fConnect |= true;
          count--;
        }
      std::vector<bool> vMarks(nObjsAlloc);
      MarkFoCone_rec(vMarks, *it);
      for(std::list<int>::iterator it2 = targets.begin(); it2 != targets.end(); it2++)
        if(!vMarks[*it2] && !vvFos[*it2].empty())
          if(TryConnect(*it, *it2, false) || TryConnect(*it, *it2, true)) {
            fConnect |= true;
            count--;
          }
      if(fConnect) {
        if(fMspf) {
          Build();
          count += Mspf(true, *it);
        } else {
          vPfUpdates[*it] = true;
          count += Cspf(true, *it);
        }
        if(!vvFos[*it].empty()) {
          vPfUpdates[*it] = true;
          count += fMspf? Mspf(true): Cspf(true);
        }
      }
    }
    count += Decompose();
    if(nVerbose)
      PrintStats("ResubShared", true, 11);
    return count;
  }

public: // Optimization scripts
  int RepeatResub(bool fMono, bool fMspf) {
    int count = 0;
    while(int diff = fMono? ResubMono(fMspf): Resub(fMspf))
      count += diff;
    return count;
  }
  int RepeatInner(bool fMspf, bool fInner) {
    int count = 0;
    while(int diff = RepeatResub(true, fMspf) + RepeatResub(false, fMspf)) {
      count += diff;
      if(!fInner)
        break;
    }
    return count;
  }
  int RepeatOuter(bool fMspf, bool fInner, bool fOuter) {
    int count = 0;
    while(int diff = fMspf? RepeatInner(false, fInner) + RepeatInner(true, fInner): RepeatInner(false, fInner)) {
      count += diff;
      if(!fOuter)
        break;
    }
    return count;
  }
  int RepeatAll(bool fFirstMerge, bool fMspfMerge, bool fMspfResub, bool fInner, bool fOuter) {
    TransductionBackup<Man, lit, LitMax> b;
    Save(b);
    int count = 0;
    int diff = 0;
    if(fFirstMerge)
      diff = ResubShared(fMspfMerge);
    diff += RepeatOuter(fMspfResub, fInner, fOuter);
    if(diff > 0) {
      count = diff;
      Save(b);
      diff = 0;
    }
    while(true) {
      diff += ResubShared(fMspfMerge) + RepeatOuter(fMspfResub, fInner, fOuter);
      if(diff > 0) {
        count += diff;
        Save(b);
        diff = 0;
      } else {
        Load(b);
        break;
      }
    }
    return count;
  }

public: // Cspf/mspf
  int Cspf() {
    return Cspf(true);
  }
  int Mspf() {
    return Mspf(true);
  }

private: // Setup
  void ImportAig(Gia_Man_t *pGia) {
    int i;
    Gia_Obj_t *pObj;
    nObjsAlloc = Gia_ManObjNum(pGia);
    vvFis.resize(nObjsAlloc);
    vvFos.resize(nObjsAlloc);
    if(fLevel) {
      vLevels.resize(nObjsAlloc);
      vSlacks.resize(nObjsAlloc);
      vvFiSlacks.resize(nObjsAlloc);
    }
    vFs.resize(nObjsAlloc, LitMax);
    vGs.resize(nObjsAlloc, LitMax);
    vvCs.resize(nObjsAlloc);
    vUpdates.resize(nObjsAlloc);
    vPfUpdates.resize(nObjsAlloc);
    std::vector<int> v(Gia_ManObjNum(pGia), -1);
    int nObjs = 0;
    v[Gia_ObjId(pGia, Gia_ManConst0(pGia))] = nObjs << 1;
    nObjs++;
    Gia_ManForEachCi(pGia, pObj, i) {
      v[Gia_ObjId(pGia, pObj)] = nObjs << 1;
      vPis.push_back(nObjs);
      nObjs++;
    }
    Gia_ManForEachAnd(pGia, pObj, i) {
      int id = Gia_ObjId(pGia, pObj);
      if(nVerbose > 5) {
        std::stringstream ss;
        ss << "\t\t\t\timport Gate " << std::setw(5) << id;
        Print(ss.str(), true);
      }
      int i0 = Gia_ObjId(pGia, Gia_ObjFanin0(pObj));
      int i1 = Gia_ObjId(pGia, Gia_ObjFanin1(pObj));
      int c0 = Gia_ObjFaninC0(pObj);
      int c1 = Gia_ObjFaninC1(pObj);
      if(i0 == i1) {
        if(c0 == c1)
          v[id] = v[i0] ^ c0;
        else
          v[id] = 0;
      } else {
        Connect(nObjs , v[i0] ^ c0);
        Connect(nObjs, v[i1] ^ c1);
        v[id] = nObjs << 1;
        vObjs.push_back(nObjs);
        nObjs++;
      }
    }
    Gia_ManForEachCo(pGia, pObj, i) {
      if(nVerbose > 5) {
        std::stringstream ss;
        ss << "\t\t\t\timport PO " << std::setw(2) << i;
        Print(ss.str(), true);
      }
      int i0 = Gia_ObjId(pGia, Gia_ObjFanin0(pObj));
      int c0 = Gia_ObjFaninC0(pObj);
      Connect(nObjs, v[i0] ^ c0);
      vPos.push_back(nObjs);
      nObjs++;
    }
  }
  void Aig2Bdd(Gia_Man_t *pGia, std::vector<lit> &vNodes) {
    if(nVerbose > 6)
      Print("\t\t\tBuild Exdc", true);
    int i;
    Gia_Obj_t *pObj;
    std::vector<int> vCounts(pGia->nObjs);
    Gia_ManStaticFanoutStart(pGia);
    Gia_ManForEachAnd(pGia, pObj, i)
      vCounts[Gia_ObjId(pGia, pObj)] = Gia_ObjFanoutNum(pGia, pObj);
    Gia_ManStaticFanoutStop(pGia);
    std::vector<lit> nodes(pGia->nObjs);
    nodes[Gia_ObjId(pGia, Gia_ManConst0(pGia))] = this->man->Const0();
    Gia_ManForEachCi(pGia, pObj, i)
      nodes[Gia_ObjId(pGia, pObj)] = this->man->IthVar(i);
    Gia_ManForEachAnd(pGia, pObj, i) {
      int id = Gia_ObjId(pGia, pObj);
      if(nVerbose > 6) {
        std::stringstream ss;
        ss << "\t\t\t\tbuilding Exdc (" << i << " / " << Gia_ManAndNum(pGia) << ")";
        Print(ss.str(), nVerbose > 7);
      }
      int i0 = Gia_ObjId(pGia, Gia_ObjFanin0(pObj));
      int i1 = Gia_ObjId(pGia, Gia_ObjFanin1(pObj));
      bool c0 = Gia_ObjFaninC0(pObj);
      bool c1 = Gia_ObjFaninC1(pObj);
      nodes[id] = this->man->And(this->man->LitNotCond(nodes[i0], c0), this->man->LitNotCond(nodes[i1], c1));
      this->IncRef(nodes[id]);
      vCounts[i0]--;
      if(!vCounts[i0])
        this->DecRef(nodes[i0]);
      vCounts[i1]--;
      if(!vCounts[i1])
        this->DecRef(nodes[i1]);
    }
    Gia_ManForEachCo(pGia, pObj, i) {
      int i0 = Gia_ObjId(pGia, Gia_ObjFanin0(pObj));
      bool c0 = Gia_ObjFaninC0(pObj);
      vNodes.push_back(this->man->LitNotCond(nodes[i0], c0));
    }
  }
  void RemoveConstOutputs() {
    bool fRemoved = false;
    for(unsigned i = 0; i < vPos.size(); i++) {
      int i0 = vvFis[vPos[i]][0] >> 1;
      lit c = vvCs[vPos[i]][0];
      if(i0) {
        if(this->man->IsConst1(this->man->Or(LitFi(vPos[i], 0), c))) {
          if(nVerbose > 3) {
            std::stringstream ss;
            ss << "\t\tPO " << std::setw(2) << i << " is Const 1";
            Print(ss.str(), true);
          }
          Disconnect(vPos[i], i0, 0, false, false);
          Connect(vPos[i], 1, false, false, c);
          fRemoved |= vvFos[i0].empty();
        } else if(this->man->IsConst1(this->man->Or(this->man->LitNot(LitFi(vPos[i], 0)), c))) {
          if(nVerbose > 3) {
            std::stringstream ss;
            ss << "\t\tPO " << std::setw(2) << i << " is Const 0";
            Print(ss.str(), true);
          }
          Disconnect(vPos[i], i0, 0, false, false);
          Connect(vPos[i], 0, false, false, c);
          fRemoved |= vvFos[i0].empty();
        }
      }
    }
    if(fRemoved) {
      for(std::list<int>::reverse_iterator it = vObjs.rbegin(); it != vObjs.rend();) {
        if(vvFos[*it].empty()) {
          if(nVerbose > 3) {
            std::stringstream ss;
            ss << "\t\tremove unused Gate " << std::setw(5) << *it;
            Print(ss.str(), true);
          }
          Remove(*it, false);
          it = std::list<int>::reverse_iterator(vObjs.erase(--(it.base())));
          continue;
        }
        it++;
      }
    }
  }

public: // Constructor
  Transduction(Gia_Man_t *pGia, int nVerbose, bool fNewLine, int nSortType, int nPiShuffle, bool fLevel, Gia_Man_t *pExdc, Param &p): nVerbose(nVerbose), nSortType(nSortType), fLevel(fLevel), fNewLine(fNewLine) {
    startclk = Abc_Clock();
    p.nGbc = 1;
    p.nReo = 4000;
    if(nSortType && nSortType < 4)
      p.fCountOnes = true;
    this->man = new Man(Gia_ManCiNum(pGia), p);
    ImportAig(pGia);
    this->Update(vFs[0], this->man->Const0());
    for(unsigned i = 0; i < vPis.size(); i++)
      this->Update(vFs[i + 1], this->man->IthVar(i));
    nMaxLevels = -1;
    Build(false);
    this->man->Reorder();
    this->man->TurnOffReo();
    if(pExdc) {
      std::vector<lit> vExdc;
      Aig2Bdd(pExdc, vExdc);
      for(unsigned i = 0; i < vPos.size(); i++)
        vvCs[vPos[i]][0] = vExdc.size() == 1? vExdc[0]: vExdc[i];
    } else
      for(unsigned i = 0; i < vPos.size(); i++)
        this->Update(vvCs[vPos[i]][0], this->man->Const0());
    RemoveConstOutputs();
    vPoFs.resize(vPos.size(), LitMax);
    for(unsigned i = 0; i < vPos.size(); i++)
      this->Update(vPoFs[i], LitFi(vPos[i], 0));
    state = none;
    if(nPiShuffle)
      ShufflePis(nPiShuffle);
    if(fLevel)
      ComputeLevel();
    if(nVerbose)
      PrintStats("Init", true, 11);
  }
  ~Transduction() {
    if(nVerbose)
      PrintStats("End", true, 11);
    this->DelVec(vFs);
    this->DelVec(vGs);
    this->DelVec(vvCs);
    this->DelVec(vPoFs);
    //assert(this->man->CountNodes() == (int)vPis.size() + 1);
    //assert(!this->man->Ref(this->man->Const0()));
    delete this->man;
  }

public:  // Output
  Gia_Man_t *GenerateAig() const {
    Gia_Man_t * pGia, *pTemp;
    pGia = Gia_ManStart(1 + vPis.size() + CountNodes() + vPos.size());
    Gia_ManHashAlloc(pGia);
    std::vector<int> values(nObjsAlloc);
    values[0] = Gia_ManConst0Lit();
    for(unsigned i = 0; i < vPis.size(); i++)
      values[i + 1] = Gia_ManAppendCi(pGia);
    for(std::list<int>::const_iterator it = vObjs.begin(); it != vObjs.end(); it++) {
      assert(vvFis[*it].size() > 1);
      int i0 = vvFis[*it][0] >> 1;
      int i1 = vvFis[*it][1] >> 1;
      int c0 = vvFis[*it][0] & 1;
      int c1 = vvFis[*it][1] & 1;
      int r = Gia_ManHashAnd(pGia, Abc_LitNotCond(values[i0], c0), Abc_LitNotCond(values[i1], c1));
      for(unsigned i = 2; i < vvFis[*it].size(); i++) {
        int ii = vvFis[*it][i] >> 1;
        int ci = vvFis[*it][i] & 1;
        r = Gia_ManHashAnd(pGia, r, Abc_LitNotCond(values[ii], ci));
      }
      values[*it] = r;
    }
    for(unsigned i = 0; i < vPos.size(); i++) {
      int i0 = vvFis[vPos[i]][0] >> 1;
      int c0 = vvFis[vPos[i]][0] & 1;
      Gia_ManAppendCo(pGia, Abc_LitNotCond(values[i0], c0));
    }
    pGia = Gia_ManCleanup(pTemp = pGia);
    Gia_ManStop(pTemp);
    return pGia;
  }

public: // Debug and print
  PfState State() const {
    return state;
  }
  bool BuildDebug() {
    for(std::list<int>::iterator it = vObjs.begin(); it != vObjs.end(); it++)
      vUpdates[*it] = true;
    std::vector<lit> vFsOld;
    CopyVec(vFsOld, vFs);
    Build(false);
    bool r = LitVecIsEq(vFsOld, vFs);
    DelVec(vFsOld);
    return r;
  }
  bool CspfDebug() {
    std::vector<lit> vGsOld;
    this->CopyVec(vGsOld, vGs);
    std::vector<std::vector<lit> > vvCsOld;
    this->CopyVec(vvCsOld, vvCs);
    state = none;
    Cspf(false);
    bool r = this->LitVecIsEq(vGsOld, vGs) && this->LitVecIsEq(vvCsOld, vvCs);
    this->DelVec(vGsOld);
    this->DelVec(vvCsOld);
    return r;
  }
  bool MspfDebug() {
    std::vector<lit> vGsOld;
    this->CopyVec(vGsOld, vGs);
    std::vector<std::vector<lit> > vvCsOld;
    this->CopyVec(vvCsOld, vvCs);
    state = none;
    Mspf(false);
    bool r = this->LitVecIsEq(vGsOld, vGs) && this->LitVecIsEq(vvCsOld, vvCs);
    this->DelVec(vGsOld);
    this->DelVec(vvCsOld);
    return r;
  }
  bool Verify() const {
    for(unsigned j = 0; j < vPos.size(); j++) {
      lit x = this->Xor(LitFi(vPos[j], 0), vPoFs[j]);
      this->IncRef(x);
      this->Update(x, this->man->And(x, this->man->LitNot(vvCs[vPos[j]][0])));
      this->DecRef(x);
      if(!this->man->IsConst0(x))
        return false;
    }
    return true;
  }
  void PrintObjs() const {
    for(std::list<int>::const_iterator it = vObjs.begin(); it != vObjs.end(); it++) {
      std::cout << "Gate " << *it << ":";
      if(fLevel)
        std::cout << " Level = " << vLevels[*it] << ", Slack = " << vSlacks[*it];
      std::cout << std::endl;
      std::string delim = "";
      std::cout << "\tFis: ";
      for(unsigned j = 0; j < vvFis[*it].size(); j++) {
        std::cout << delim << (vvFis[*it][j] >> 1) << "(" << (vvFis[*it][j] & 1) << ")";
        delim = ", ";
      }
      std::cout << std::endl;
      delim = "";
      std::cout << "\tFos: ";
      for(unsigned j = 0; j < vvFos[*it].size(); j++) {
        std::cout << delim << vvFos[*it][j];
        delim = ", ";
      }
      std::cout << std::endl;
    }
  }
};

}

ABC_NAMESPACE_CXX_HEADER_END

#endif
