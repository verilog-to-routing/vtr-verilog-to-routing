/**CFile****************************************************************

  FileName    [giaTtopt.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Truth-table-based logic synthesis.]

  Author      [Yukio Miyasaka]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaTtopt.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#ifdef _WIN32
#ifndef __MINGW32__
#pragma warning(disable : 4786) // warning C4786: identifier was truncated to '255' characters in the browser information
#endif
#endif

#include <vector>
#include <algorithm>
#include <cassert>
#include <bitset>

#include "gia.h"
#include "misc/vec/vecHash.h"

ABC_NAMESPACE_IMPL_START

namespace Ttopt {

class TruthTable {
public:
  static const int ww; // word width
  static const int lww; // log word width
  typedef std::bitset<64> bsw;

  int nInputs;
  int nSize;
  int nTotalSize;
  int nOutputs;
  std::vector<word> t;

  std::vector<std::vector<int> > vvIndices;
  std::vector<std::vector<int> > vvRedundantIndices;
  std::vector<int> vLevels;

  std::vector<std::vector<word> > savedt;
  std::vector<std::vector<std::vector<int> > > vvIndicesSaved;
  std::vector<std::vector<std::vector<int> > > vvRedundantIndicesSaved;
  std::vector<std::vector<int> > vLevelsSaved;

  static const word ones[];
  static const word swapmask[];

  TruthTable(int nInputs, int nOutputs): nInputs(nInputs), nOutputs(nOutputs) {
    srand(0xABC);
    if(nInputs >= lww) {
      nSize = 1 << (nInputs - lww);
      nTotalSize = nSize * nOutputs;
      t.resize(nTotalSize);
    } else {
      nSize = 0;
      nTotalSize = ((1 << nInputs) * nOutputs + ww - 1) / ww;
      t.resize(nTotalSize);
    }
    vLevels.resize(nInputs);
    for(int i = 0; i < nInputs; i++) {
      vLevels[i] = i;
    }
  }

  virtual void Save(unsigned i) {
    if(savedt.size() < i + 1) {
      savedt.resize(i + 1);
      vLevelsSaved.resize(i + 1);
    }
    savedt[i] = t;
    vLevelsSaved[i] = vLevels;
  }

  virtual void Load(unsigned i) {
    assert(i < savedt.size());
    t = savedt[i];
    vLevels = vLevelsSaved[i];
  }

  virtual void SaveIndices(unsigned i) {
    if(vvIndicesSaved.size() < i + 1) {
      vvIndicesSaved.resize(i + 1);
      vvRedundantIndicesSaved.resize(i + 1);
    }
    vvIndicesSaved[i] = vvIndices;
    vvRedundantIndicesSaved[i] = vvRedundantIndices;
  }

  virtual void LoadIndices(unsigned i) {
    vvIndices = vvIndicesSaved[i];
    vvRedundantIndices = vvRedundantIndicesSaved[i];
  }

  word GetValue(int index_lev, int lev) {
    assert(index_lev >= 0);
    assert(nInputs - lev <= lww);
    int logwidth = nInputs - lev;
    int index = index_lev >> (lww - logwidth);
    int pos = (index_lev % (1 << (lww - logwidth))) << logwidth;
    return (t[index] >> pos) & ones[logwidth];
  }

  int IsEq(int index1, int index2, int lev, bool fCompl = false) {
    assert(index1 >= 0);
    assert(index2 >= 0);
    int logwidth = nInputs - lev;
    bool fEq = true;
    if(logwidth > lww) {
      int nScopeSize = 1 << (logwidth - lww);
      for(int i = 0; i < nScopeSize && (fEq || fCompl); i++) {
        fEq &= (t[nScopeSize * index1 + i] == t[nScopeSize * index2 + i]);
        fCompl &= (t[nScopeSize * index1 + i] == ~t[nScopeSize * index2 + i]);
      }
    } else {
      word value = GetValue(index1, lev) ^ GetValue(index2, lev);
      fEq &= !value;
      fCompl &= !(value ^ ones[logwidth]);
    }
    return 2 * fCompl + fEq;
  }

  bool Imply(int index1, int index2, int lev) {
    assert(index1 >= 0);
    assert(index2 >= 0);
    int logwidth = nInputs - lev;
    if(logwidth > lww) {
      int nScopeSize = 1 << (logwidth - lww);
      for(int i = 0; i < nScopeSize; i++) {
        if(t[nScopeSize * index1 + i] & ~t[nScopeSize * index2 + i]) {
          return false;
        }
      }
      return true;
    }
    return !(GetValue(index1, lev) & (GetValue(index2, lev) ^ ones[logwidth]));
  }

  int BDDNodeCountLevel(int lev) {
    return vvIndices[lev].size() - vvRedundantIndices[lev].size();
  }

  int BDDNodeCount() {
    int count = 1; // const node
    for(int i = 0; i < nInputs; i++) {
      count += BDDNodeCountLevel(i);
    }
    return count;
  }

  int BDDFind(int index, int lev) {
    int logwidth = nInputs - lev;
    if(logwidth > lww) {
      int nScopeSize = 1 << (logwidth - lww);
      bool fZero = true;
      bool fOne = true;
      for(int i = 0; i < nScopeSize && (fZero || fOne); i++) {
        word value = t[nScopeSize * index + i];
        fZero &= !value;
        fOne &= !(~value);
      }
      if(fZero || fOne) {
        return -2 ^ (int)fOne;
      }
      for(unsigned j = 0; j < vvIndices[lev].size(); j++) {
        int index2 = vvIndices[lev][j];
        bool fEq = true;
        bool fCompl = true;
        for(int i = 0; i < nScopeSize && (fEq || fCompl); i++) {
          fEq &= (t[nScopeSize * index + i] == t[nScopeSize * index2 + i]);
          fCompl &= (t[nScopeSize * index + i] == ~t[nScopeSize * index2 + i]);
        }
        if(fEq || fCompl) {
          return (j << 1) ^ (int)fCompl;
        }
      }
    } else {
      word value = GetValue(index, lev);
      if(!value) {
        return -2;
      }
      if(!(value ^ ones[logwidth])) {
        return -1;
      }
      for(unsigned j = 0; j < vvIndices[lev].size(); j++) {
        int index2 = vvIndices[lev][j];
        word value2 = value ^ GetValue(index2, lev);
        if(!(value2)) {
          return j << 1;
        }
        if(!(value2 ^ ones[logwidth])) {
          return (j << 1) ^ 1;
        }
      }
    }
    return -3;
  }

  virtual int BDDBuildOne(int index, int lev) {
    int r = BDDFind(index, lev);
    if(r >= -2) {
      return r;
    }
    vvIndices[lev].push_back(index);
    return (vvIndices[lev].size() - 1) << 1;
  }

  virtual void BDDBuildStartup() {
    vvIndices.clear();
    vvIndices.resize(nInputs);
    vvRedundantIndices.clear();
    vvRedundantIndices.resize(nInputs);
    for(int i = 0; i < nOutputs; i++) {
      BDDBuildOne(i, 0);
    }
  }

  virtual void BDDBuildLevel(int lev) {
    for(unsigned i = 0; i < vvIndices[lev-1].size(); i++) {
      int index = vvIndices[lev-1][i];
      int cof0 = BDDBuildOne(index << 1, lev);
      int cof1 = BDDBuildOne((index << 1) ^ 1, lev);
      if(cof0 == cof1) {
        vvRedundantIndices[lev-1].push_back(index);
      }
    }
  }

  virtual int BDDBuild() {
    BDDBuildStartup();
    for(int i = 1; i < nInputs; i++) {
      BDDBuildLevel(i);
    }
    return BDDNodeCount();
  }

  virtual int BDDRebuild(int lev) {
    vvIndices[lev].clear();
    vvIndices[lev+1].clear();
    for(int i = lev; i < lev + 2; i++) {
      if(!i) {
        for(int j = 0; j < nOutputs; j++) {
          BDDBuildOne(j, 0);
        }
      } else {
        vvRedundantIndices[i-1].clear();
        BDDBuildLevel(i);
      }
    }
    if(lev < nInputs - 2) {
      vvRedundantIndices[lev+1].clear();
      for(unsigned i = 0; i < vvIndices[lev+1].size(); i++) {
        int index = vvIndices[lev+1][i];
        if(IsEq(index << 1, (index << 1) ^ 1, lev + 2)) {
          vvRedundantIndices[lev+1].push_back(index);
        }
      }
    }
    return BDDNodeCount();
  }

  virtual void Swap(int lev) {
    assert(lev < nInputs - 1);
    std::vector<int>::iterator it0 = std::find(vLevels.begin(), vLevels.end(), lev);
    std::vector<int>::iterator it1 = std::find(vLevels.begin(), vLevels.end(), lev + 1);
    std::swap(*it0, *it1);
    if(nInputs - lev - 1 > lww) {
      int nScopeSize = 1 << (nInputs - lev - 2 - lww);
      for(int i = nScopeSize; i < nTotalSize; i += (nScopeSize << 2)) {
        for(int j = 0; j < nScopeSize; j++) {
          std::swap(t[i + j], t[i + nScopeSize + j]);
        }
      }
    } else if(nInputs - lev - 1 == lww) {
      for(int i = 0; i < nTotalSize; i += 2) {
        t[i+1] ^= t[i] >> (ww / 2);
        t[i] ^= t[i+1] << (ww / 2);
        t[i+1] ^= t[i] >> (ww / 2);
      }
    } else {
      for(int i = 0; i < nTotalSize; i++) {
        int d = nInputs - lev - 2;
        int shamt = 1 << d;
        t[i] ^= (t[i] >> shamt) & swapmask[d];
        t[i] ^= (t[i] & swapmask[d]) << shamt;
        t[i] ^= (t[i] >> shamt) & swapmask[d];
      }
    }
  }

  void SwapIndex(int &index, int d) {
    if((index >> d) % 4 == 1) {
      index += 1 << d;
    } else if((index >> d) % 4 == 2) {
      index -= 1 << d;
    }
  }

  virtual int BDDSwap(int lev) {
    Swap(lev);
    for(int i = lev + 2; i < nInputs; i++) {
      for(unsigned j = 0; j < vvIndices[i].size(); j++) {
        SwapIndex(vvIndices[i][j], i - (lev + 2));
      }
    }
    // swapping vvRedundantIndices is unnecessary for node counting
    return BDDRebuild(lev);
  }

  int SiftReo() {
    int best = BDDBuild();
    Save(0);
    SaveIndices(0);
    std::vector<int> vars(nInputs);
    int i;
    for(i = 0; i < nInputs; i++) {
      vars[i] = i;
    }
    std::vector<unsigned> vCounts(nInputs);
    for(i = 0; i < nInputs; i++) {
      vCounts[i] = BDDNodeCountLevel(vLevels[i]);
    }
    for(i = 1; i < nInputs; i++) {
      int j = i;
      while(j > 0 && vCounts[vars[j-1]] < vCounts[vars[j]]) {
        std::swap(vars[j], vars[j-1]);
        j--;
      }
    }
    bool turn = true;
    unsigned j;
    for(j = 0; j < vars.size(); j++) {
      int var = vars[j];
      bool updated = false;
      int lev = vLevels[var];
      for(int i = lev; i < nInputs - 1; i++) {
        int count = BDDSwap(i);
        if(best > count) {
          best = count;
          updated = true;
          Save(turn);
          SaveIndices(turn);
        }
      }
      if(lev) {
        Load(!turn);
        LoadIndices(!turn);
        for(int i = lev - 1; i >= 0; i--) {
          int count = BDDSwap(i);
          if(best > count) {
            best = count;
            updated = true;
            Save(turn);
            SaveIndices(turn);
          }
        }
      }
      turn ^= updated;
      Load(!turn);
      LoadIndices(!turn);
    }
    return best;
  }

  void Reo(std::vector<int> vLevelsNew) {
    for(int i = 0; i < nInputs; i++) {
      int var = std::find(vLevelsNew.begin(), vLevelsNew.end(), i) - vLevelsNew.begin();
      int lev = vLevels[var];
      if(lev < i) {
        for(int j = lev; j < i; j++) {
          Swap(j);
        }
      } else if(lev > i) {
        for(int j = lev - 1; j >= i; j--) {
          Swap(j);
        }
      }
    }
    assert(vLevels == vLevelsNew);
  }

  int RandomSiftReo(int nRound) {
    int best = SiftReo();
    Save(2);
    for(int i = 0; i < nRound; i++) {
      std::vector<int> vLevelsNew(nInputs);
      int j;
      for(j = 0; j < nInputs; j++) {
        vLevelsNew[j] = j;
      }
      for(j = nInputs - 1; j > 0; j--) {
        int d = rand() % j;
        std::swap(vLevelsNew[j], vLevelsNew[d]);
      }
      Reo(vLevelsNew);
      int r = SiftReo();
      if(best > r) {
        best = r;
        Save(2);
      }
    }
    Load(2);
    return best;
  }

  int BDDGenerateAigRec(Gia_Man_t *pNew, std::vector<int> const &vInputs, std::vector<std::vector<int> > &vvNodes, int index, int lev) {
    int r = BDDFind(index, lev);
    if(r >= 0) {
      return vvNodes[lev][r >> 1] ^ (r & 1);
    }
    if(r >= -2) {
      return r + 2;
    }
    int cof0 = BDDGenerateAigRec(pNew, vInputs, vvNodes, index << 1, lev + 1);
    int cof1 = BDDGenerateAigRec(pNew, vInputs, vvNodes, (index << 1) ^ 1, lev + 1);
    if(cof0 == cof1) {
      return cof0;
    }
    int node;
    if(Imply(index << 1, (index << 1) ^ 1, lev + 1)) {
      node = Gia_ManHashOr(pNew, Gia_ManHashAnd(pNew, vInputs[lev], cof1), cof0);
    } else if(Imply((index << 1) ^ 1, index << 1, lev + 1)) {
      node = Gia_ManHashOr(pNew, Gia_ManHashAnd(pNew, vInputs[lev] ^ 1, cof0), cof1);
    } else {
      node = Gia_ManHashMux(pNew, vInputs[lev], cof1, cof0);
    }
    vvIndices[lev].push_back(index);
    vvNodes[lev].push_back(node);
    return node;
  }

  virtual void BDDGenerateAig(Gia_Man_t *pNew, Vec_Int_t *vSupp) {
    vvIndices.clear();
    vvIndices.resize(nInputs);
    std::vector<std::vector<int> > vvNodes(nInputs);
    std::vector<int> vInputs(nInputs);
    int i;
    for(i = 0; i < nInputs; i++) {
      vInputs[vLevels[i]] = Vec_IntEntry(vSupp, nInputs - i - 1) << 1;
    }
    for(i = 0; i < nOutputs; i++) {
      int node = BDDGenerateAigRec(pNew, vInputs, vvNodes, i, 0);
      Gia_ManAppendCo(pNew, node);
    }
  }
};

const int TruthTable::ww = 64;
const int TruthTable::lww = 6;

const word TruthTable::ones[7] = {ABC_CONST(0x0000000000000001),
                                  ABC_CONST(0x0000000000000003),
                                  ABC_CONST(0x000000000000000f),
                                  ABC_CONST(0x00000000000000ff),
                                  ABC_CONST(0x000000000000ffff),
                                  ABC_CONST(0x00000000ffffffff),
                                  ABC_CONST(0xffffffffffffffff)};
  
const word TruthTable::swapmask[5] = {ABC_CONST(0x2222222222222222),
                                      ABC_CONST(0x0c0c0c0c0c0c0c0c),
                                      ABC_CONST(0x00f000f000f000f0),
                                      ABC_CONST(0x0000ff000000ff00),
                                      ABC_CONST(0x00000000ffff0000)};
  
class TruthTableReo : public TruthTable {
public:
  bool fBuilt;
  std::vector<std::vector<int> > vvChildren;
  std::vector<std::vector<std::vector<int> > > vvChildrenSaved;

  TruthTableReo(int nInputs, int nOutputs): TruthTable(nInputs, nOutputs) {
    fBuilt = false;
  }

  void Save(unsigned i) {
    if(vLevelsSaved.size() < i + 1) {
      vLevelsSaved.resize(i + 1);
    }
    vLevelsSaved[i] = vLevels;
  }

  void Load(unsigned i) {
    assert(i < vLevelsSaved.size());
    vLevels = vLevelsSaved[i];
  }

  void SaveIndices(unsigned i) {
    TruthTable::SaveIndices(i);
    if(vvChildrenSaved.size() < i + 1) {
      vvChildrenSaved.resize(i + 1);
    }
    vvChildrenSaved[i] = vvChildren;
  }

  void LoadIndices(unsigned i) {
    TruthTable::LoadIndices(i);
    vvChildren = vvChildrenSaved[i];
  }

  void BDDBuildStartup() {
    vvChildren.clear();
    vvChildren.resize(nInputs);
    TruthTable::BDDBuildStartup();
  }

  void BDDBuildLevel(int lev) {
    for(unsigned i = 0; i < vvIndices[lev-1].size(); i++) {
      int index = vvIndices[lev-1][i];
      int cof0 = BDDBuildOne(index << 1, lev);
      int cof1 = BDDBuildOne((index << 1) ^ 1, lev);
      vvChildren[lev-1].push_back(cof0);
      vvChildren[lev-1].push_back(cof1);
      if(cof0 == cof1) {
        vvRedundantIndices[lev-1].push_back(index);
      }
    }
  }

  int BDDBuild() {
    if(fBuilt) {
      return BDDNodeCount();
    }
    fBuilt = true;
    BDDBuildStartup();
    for(int i = 1; i < nInputs + 1; i++) {
      BDDBuildLevel(i);
    }
    return BDDNodeCount();
  }

  int BDDRebuildOne(int index, int cof0, int cof1, int lev, Hash_IntMan_t *unique, std::vector<int> &vChildrenLow) {
    if(cof0 < 0 && cof0 == cof1) {
      return cof0;
    }
    bool fCompl = cof0 & 1;
    if(fCompl) {
      cof0 ^= 1;
      cof1 ^= 1;
    }
    int *place = Hash_Int2ManLookup(unique, cof0, cof1);
    if(*place) {
      return (Hash_IntObjData2(unique, *place) << 1) ^ (int)fCompl;
    }
    vvIndices[lev].push_back(index);
    Hash_Int2ManInsert(unique, cof0, cof1, vvIndices[lev].size() - 1);
    vChildrenLow.push_back(cof0);
    vChildrenLow.push_back(cof1);
    if(cof0 == cof1) {
      vvRedundantIndices[lev].push_back(index);
    }
    return ((vvIndices[lev].size() - 1) << 1) ^ (int)fCompl;
  }

  int BDDRebuild(int lev) {
    vvRedundantIndices[lev].clear();
    vvRedundantIndices[lev+1].clear();
    std::vector<int> vChildrenHigh;
    std::vector<int> vChildrenLow;
    Hash_IntMan_t *unique = Hash_IntManStart(2 * vvIndices[lev+1].size());
    vvIndices[lev+1].clear();
    for(unsigned i = 0; i < vvIndices[lev].size(); i++) {
      int index = vvIndices[lev][i];
      int cof0index = vvChildren[lev][i+i] >> 1;
      int cof1index = vvChildren[lev][i+i+1] >> 1;
      bool cof0c = vvChildren[lev][i+i] & 1;
      bool cof1c = vvChildren[lev][i+i+1] & 1;
      int cof00, cof01, cof10, cof11;
      if(cof0index < 0) {
        cof00 = -2 ^ (int)cof0c;
        cof01 = -2 ^ (int)cof0c;
      } else {
        cof00 = vvChildren[lev+1][cof0index+cof0index] ^ (int)cof0c;
        cof01 = vvChildren[lev+1][cof0index+cof0index+1] ^ (int)cof0c;
      }
      if(cof1index < 0) {
        cof10 = -2 ^ (int)cof1c;
        cof11 = -2 ^ (int)cof1c;
      } else {
        cof10 = vvChildren[lev+1][cof1index+cof1index] ^ (int)cof1c;
        cof11 = vvChildren[lev+1][cof1index+cof1index+1] ^ (int)cof1c;
      }
      int newcof0 = BDDRebuildOne(index << 1, cof00, cof10, lev + 1, unique, vChildrenLow);
      int newcof1 = BDDRebuildOne((index << 1) ^ 1, cof01, cof11, lev + 1, unique, vChildrenLow);
      vChildrenHigh.push_back(newcof0);
      vChildrenHigh.push_back(newcof1);
      if(newcof0 == newcof1) {
        vvRedundantIndices[lev].push_back(index);
      }
    }
    Hash_IntManStop(unique);
    vvChildren[lev] = vChildrenHigh;
    vvChildren[lev+1] = vChildrenLow;
    return BDDNodeCount();
  }

  void Swap(int lev) {
    assert(lev < nInputs - 1);
    std::vector<int>::iterator it0 = std::find(vLevels.begin(), vLevels.end(), lev);
    std::vector<int>::iterator it1 = std::find(vLevels.begin(), vLevels.end(), lev + 1);
    std::swap(*it0, *it1);
    BDDRebuild(lev);
  }

  int BDDSwap(int lev) {
    Swap(lev);
    return BDDNodeCount();
  }

  virtual void BDDGenerateAig(Gia_Man_t *pNew, Vec_Int_t *vSupp) {
    abort();
  }
};

class TruthTableRewrite : public TruthTable {
public:
  TruthTableRewrite(int nInputs, int nOutputs): TruthTable(nInputs, nOutputs) {}

  void SetValue(int index_lev, int lev, word value) {
    assert(index_lev >= 0);
    assert(nInputs - lev <= lww);
    int logwidth = nInputs - lev;
    int index = index_lev >> (lww - logwidth);
    int pos = (index_lev % (1 << (lww - logwidth))) << logwidth;
    t[index] &= ~(ones[logwidth] << pos);
    t[index] ^= value << pos;
  }

  void CopyFunc(int index1, int index2, int lev, bool fCompl) {
    assert(index1 >= 0);
    int logwidth = nInputs - lev;
    if(logwidth > lww) {
      int nScopeSize = 1 << (logwidth - lww);
      if(!fCompl) {
        if(index2 < 0) {
          for(int i = 0; i < nScopeSize; i++) {
            t[nScopeSize * index1 + i] = 0;
          }
        } else {
          for(int i = 0; i < nScopeSize; i++) {
            t[nScopeSize * index1 + i] = t[nScopeSize * index2 + i];
          }
        }
      } else {
        if(index2 < 0) {
          for(int i = 0; i < nScopeSize; i++) {
            t[nScopeSize * index1 + i] = ones[lww];
          }
        } else {
          for(int i = 0; i < nScopeSize; i++) {
            t[nScopeSize * index1 + i] = ~t[nScopeSize * index2 + i];
          }
        }
      }
    } else {
      word value = 0;
      if(index2 >= 0) {
        value = GetValue(index2, lev);
      }
      if(fCompl) {
        value ^= ones[logwidth];
      }
      SetValue(index1, lev, value);
    }
  }

  void ShiftToMajority(int index, int lev) {
    assert(index >= 0);
    int logwidth = nInputs - lev;
    int count = 0;
    if(logwidth > lww) {
      int nScopeSize = 1 << (logwidth - lww);
      for(int i = 0; i < nScopeSize; i++) {
        count += bsw(t[nScopeSize * index + i]).count();
      }
    } else {
      count = bsw(GetValue(index, lev)).count();
    }
    bool majority = count > (1 << (logwidth - 1));
    CopyFunc(index, -1, lev, majority);
  }
};

class TruthTableCare : public TruthTableRewrite {
public:
  std::vector<word> originalt;
  std::vector<word> caret;
  std::vector<word> care;

  std::vector<std::vector<std::pair<int, int> > > vvMergedIndices;

  std::vector<std::vector<word> > savedcare;
  std::vector<std::vector<std::vector<std::pair<int, int> > > > vvMergedIndicesSaved;

  TruthTableCare(int nInputs, int nOutputs): TruthTableRewrite(nInputs, nOutputs) {
    if(nSize) {
      care.resize(nSize);
    } else {
      care.resize(1);
    }
  }

  void Save(unsigned i) {
    TruthTable::Save(i);
    if(savedcare.size() < i + 1) {
      savedcare.resize(i + 1);
    }
    savedcare[i] = care;
  }

  void Load(unsigned i) {
    TruthTable::Load(i);
    care = savedcare[i];
  }

  void SaveIndices(unsigned i) {
    TruthTable::SaveIndices(i);
    if(vvMergedIndicesSaved.size() < i + 1) {
      vvMergedIndicesSaved.resize(i + 1);
    }
    vvMergedIndicesSaved[i] = vvMergedIndices;
  }

  void LoadIndices(unsigned i) {
    TruthTable::LoadIndices(i);
    vvMergedIndices = vvMergedIndicesSaved[i];
  }

  void Swap(int lev) {
    TruthTable::Swap(lev);
    if(nInputs - lev - 1 > lww) {
      int nScopeSize = 1 << (nInputs - lev - 2 - lww);
      for(int i = nScopeSize; i < nSize; i += (nScopeSize << 2)) {
        for(int j = 0; j < nScopeSize; j++) {
          std::swap(care[i + j], care[i + nScopeSize + j]);
        }
      }
    } else if(nInputs - lev - 1 == lww) {
      for(int i = 0; i < nSize; i += 2) {
        care[i+1] ^= care[i] >> (ww / 2);
        care[i] ^= care[i+1] << (ww / 2);
        care[i+1] ^= care[i] >> (ww / 2);
      }
    } else {
      for(int i = 0; i < nSize || (i == 0 && !nSize); i++) {
        int d = nInputs - lev - 2;
        int shamt = 1 << d;
        care[i] ^= (care[i] >> shamt) & swapmask[d];
        care[i] ^= (care[i] & swapmask[d]) << shamt;
        care[i] ^= (care[i] >> shamt) & swapmask[d];
      }
    }
  }

  void RestoreCare() {
    caret.clear();
    if(nSize) {
      for(int i = 0; i < nOutputs; i++) {
        caret.insert(caret.end(), care.begin(), care.end());
      }
    } else {
      caret.resize(nTotalSize);
      for(int i = 0; i < nOutputs; i++) {
        int padding = i * (1 << nInputs);
        caret[padding / ww] |= care[0] << (padding % ww);
      }
    }
  }

  word GetCare(int index_lev, int lev) {
    assert(index_lev >= 0);
    assert(nInputs - lev <= lww);
    int logwidth = nInputs - lev;
    int index = index_lev >> (lww - logwidth);
    int pos = (index_lev % (1 << (lww - logwidth))) << logwidth;
    return (caret[index] >> pos) & ones[logwidth];
  }

  void CopyFuncMasked(int index1, int index2, int lev, bool fCompl) {
    assert(index1 >= 0);
    assert(index2 >= 0);
    int logwidth = nInputs - lev;
    if(logwidth > lww) {
      int nScopeSize = 1 << (logwidth - lww);
      for(int i = 0; i < nScopeSize; i++) {
        word value = t[nScopeSize * index2 + i];
        if(fCompl) {
          value = ~value;
        }
        word cvalue = caret[nScopeSize * index2 + i];
        t[nScopeSize * index1 + i] &= ~cvalue;
        t[nScopeSize * index1 + i] |= cvalue & value;
      }
    } else {
      word one = ones[logwidth];
      word value1 = GetValue(index1, lev);
      word value2 = GetValue(index2, lev);
      if(fCompl) {
        value2 ^= one;
      }
      word cvalue = GetCare(index2, lev);
      value1 &= cvalue ^ one;
      value1 |= cvalue & value2;
      SetValue(index1, lev, value1);
    }
  }

  bool IsDC(int index, int lev) {
    if(nInputs - lev > lww) {
      int nScopeSize = 1 << (nInputs - lev - lww);
      for(int i = 0; i < nScopeSize; i++) {
        if(caret[nScopeSize * index + i]) {
          return false;
        }
      }
    } else if(GetCare(index, lev)) {
      return false;
    }
    return true;
  }

  int Include(int index1, int index2, int lev, bool fCompl) {
    assert(index1 >= 0);
    assert(index2 >= 0);
    int logwidth = nInputs - lev;
    bool fEq = true;
    if(logwidth > lww) {
      int nScopeSize = 1 << (logwidth - lww);
      for(int i = 0; i < nScopeSize && (fEq || fCompl); i++) {
        word cvalue = caret[nScopeSize * index2 + i];
        if(~caret[nScopeSize * index1 + i] & cvalue) {
          return 0;
        }
        word value = t[nScopeSize * index1 + i] ^ t[nScopeSize * index2 + i];
        fEq &= !(value & cvalue);
        fCompl &= !(~value & cvalue);
      }
    } else {
      word cvalue = GetCare(index2, lev);
      if((GetCare(index1, lev) ^ ones[logwidth]) & cvalue) {
        return 0;
      }
      word value = GetValue(index1, lev) ^ GetValue(index2, lev);
      fEq &= !(value & cvalue);
      fCompl &= !((value ^ ones[logwidth]) & cvalue);
    }
    return 2 * fCompl + fEq;
  }

  int Intersect(int index1, int index2, int lev, bool fCompl, bool fEq = true) {
    assert(index1 >= 0);
    assert(index2 >= 0);
    int logwidth = nInputs - lev;
    if(logwidth > lww) {
      int nScopeSize = 1 << (logwidth - lww);
      for(int i = 0; i < nScopeSize && (fEq || fCompl); i++) {
        word value = t[nScopeSize * index1 + i] ^ t[nScopeSize * index2 + i];
        word cvalue = caret[nScopeSize * index1 + i] & caret[nScopeSize * index2 + i];
        fEq &= !(value & cvalue);
        fCompl &= !(~value & cvalue);
      }
    } else {
      word value = GetValue(index1, lev) ^ GetValue(index2, lev);
      word cvalue = GetCare(index1, lev) & GetCare(index2, lev);
      fEq &= !(value & cvalue);
      fCompl &= !((value ^ ones[logwidth]) & cvalue);
    }
    return 2 * fCompl + fEq;
  }

  void MergeCare(int index1, int index2, int lev) {
    assert(index1 >= 0);
    assert(index2 >= 0);
    int logwidth = nInputs - lev;
    if(logwidth > lww) {
      int nScopeSize = 1 << (logwidth - lww);
      for(int i = 0; i < nScopeSize; i++) {
        caret[nScopeSize * index1 + i] |= caret[nScopeSize * index2 + i];
      }
    } else {
      word value = GetCare(index2, lev);
      int index = index1 >> (lww - logwidth);
      int pos = (index1 % (1 << (lww - logwidth))) << logwidth;
      caret[index] |= value << pos;
    }
  }

  void Merge(int index1, int index2, int lev, bool fCompl) {
    MergeCare(index1, index2, lev);
    vvMergedIndices[lev].push_back(std::make_pair((index1 << 1) ^ (int)fCompl, index2));
  }

  int BDDBuildOne(int index, int lev) {
    int r = BDDFind(index, lev);
    if(r >= -2) {
      if(r >= 0) {
        Merge(vvIndices[lev][r >> 1], index, lev, r & 1);
      }
      return r;
    }
    vvIndices[lev].push_back(index);
    return (vvIndices[lev].size() - 1) << 1;
  }

  void CompleteMerge() {
    for(int i = nInputs - 1; i >= 0; i--) {
      for(std::vector<std::pair<int, int> >::reverse_iterator it = vvMergedIndices[i].rbegin(); it != vvMergedIndices[i].rend(); it++) {
        CopyFunc((*it).second, (*it).first >> 1, i, (*it).first & 1);
      }
    }
  }

  void BDDBuildStartup() {
    RestoreCare();
    vvIndices.clear();
    vvIndices.resize(nInputs);
    vvRedundantIndices.clear();
    vvRedundantIndices.resize(nInputs);
    vvMergedIndices.clear();
    vvMergedIndices.resize(nInputs);
    for(int i = 0; i < nOutputs; i++) {
      if(!IsDC(i, 0)) {
        BDDBuildOne(i, 0);
      }
    }
  }

  virtual void BDDRebuildByMerge(int lev) {
    for(unsigned i = 0; i < vvMergedIndices[lev].size(); i++) {
      std::pair<int, int> &p = vvMergedIndices[lev][i];
      MergeCare(p.first >> 1, p.second, lev);
    }
  }

  int BDDRebuild(int lev) {
    RestoreCare();
    int i;
    for(i = lev; i < nInputs; i++) {
      vvIndices[i].clear();
      vvMergedIndices[i].clear();
      if(i) {
        vvRedundantIndices[i-1].clear();
      }
    }
    for(i = 0; i < lev; i++) {
      BDDRebuildByMerge(i);
    }
    for(i = lev; i < nInputs; i++) {
      if(!i) {
        for(int j = 0; j < nOutputs; j++) {
          if(!IsDC(j, 0)) {
            BDDBuildOne(j, 0);
          }
        }
      } else {
        BDDBuildLevel(i);
      }
    }
    return BDDNodeCount();
  }

  int BDDSwap(int lev) {
    Swap(lev);
    return BDDRebuild(lev);
  }

  void OptimizationStartup() {
    originalt = t;
    RestoreCare();
    vvIndices.clear();
    vvIndices.resize(nInputs);
    vvMergedIndices.clear();
    vvMergedIndices.resize(nInputs);
    for(int i = 0; i < nOutputs; i++) {
      if(!IsDC(i, 0)) {
        BDDBuildOne(i, 0);
      } else {
        ShiftToMajority(i, 0);
      }
    }
  }

  virtual void Optimize() {
    OptimizationStartup();
    for(int i = 1; i < nInputs; i++) {
      for(unsigned j = 0; j < vvIndices[i-1].size(); j++) {
        int index = vvIndices[i-1][j];
        BDDBuildOne(index << 1, i);
        BDDBuildOne((index << 1) ^ 1, i);
      }
    }
    CompleteMerge();
  }
};

class TruthTableLevelTSM : public TruthTableCare {
public:
  TruthTableLevelTSM(int nInputs, int nOutputs): TruthTableCare(nInputs, nOutputs) {}

  int BDDFindTSM(int index, int lev) {
    int logwidth = nInputs - lev;
    if(logwidth > lww) {
      int nScopeSize = 1 << (logwidth - lww);
      bool fZero = true;
      bool fOne = true;
      for(int i = 0; i < nScopeSize && (fZero || fOne); i++) {
        word value = t[nScopeSize * index + i];
        word cvalue = caret[nScopeSize * index + i];
        fZero &= !(value & cvalue);
        fOne &= !(~value & cvalue);
      }
      if(fZero || fOne) {
        return -2 ^ (int)fOne;
      }
      for(unsigned j = 0; j < vvIndices[lev].size(); j++) {
        int index2 = vvIndices[lev][j];
        bool fEq = true;
        bool fCompl = true;
        for(int i = 0; i < nScopeSize && (fEq || fCompl); i++) {
          word value = t[nScopeSize * index + i] ^ t[nScopeSize * index2 + i];
          word cvalue = caret[nScopeSize * index + i] & caret[nScopeSize * index2 + i];
          fEq &= !(value & cvalue);
          fCompl &= !(~value & cvalue);
        }
        if(fEq || fCompl) {
          return (index2 << 1) ^ (int)!fEq;
        }
      }
    } else {
      word one = ones[logwidth];
      word value = GetValue(index, lev);
      word cvalue = GetCare(index, lev);
      if(!(value & cvalue)) {
        return -2;
      }
      if(!((value ^ one) & cvalue)) {
        return -1;
      }
      for(unsigned j = 0; j < vvIndices[lev].size(); j++) {
        int index2 = vvIndices[lev][j];
        word value2 = value ^ GetValue(index2, lev);
        word cvalue2 = cvalue & GetCare(index2, lev);
        if(!(value2 & cvalue2)) {
          return index2 << 1;
        }
        if(!((value2 ^ one) & cvalue2)) {
          return (index2 << 1) ^ 1;
        }
      }
    }
    return -3;
  }

  int BDDBuildOne(int index, int lev) {
    int r = BDDFindTSM(index, lev);
    if(r >= -2) {
      if(r >= 0) {
        CopyFuncMasked(r >> 1, index, lev, r & 1);
        Merge(r >> 1, index, lev, r & 1);
      } else {
        vvMergedIndices[lev].push_back(std::make_pair(r, index));
      }
      return r;
    }
    vvIndices[lev].push_back(index);
    return index << 1;
  }

  int BDDBuild() {
    TruthTable::Save(3);
    int r = TruthTable::BDDBuild();
    TruthTable::Load(3);
    return r;
  }

  void BDDRebuildByMerge(int lev) {
    for(unsigned i = 0; i < vvMergedIndices[lev].size(); i++) {
      std::pair<int, int> &p = vvMergedIndices[lev][i];
      if(p.first >= 0) {
        CopyFuncMasked(p.first >> 1, p.second, lev, p.first & 1);
        MergeCare(p.first >> 1, p.second, lev);
      }
    }
  }

  int BDDRebuild(int lev) {
    TruthTable::Save(3);
    int r = TruthTableCare::BDDRebuild(lev);
    TruthTable::Load(3);
    return r;
  }
};

}

Gia_Man_t * Gia_ManTtopt( Gia_Man_t * p, int nIns, int nOuts, int nRounds )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    Vec_Int_t * vSupp;
    word v;
    word * pTruth;
    int i, g, k, nInputs;
    Gia_ManLevelNum( p );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManForEachCi( p, pObj, k )
        Gia_ManAppendCi( pNew );
    Gia_ObjComputeTruthTableStart( p, nIns );
    Gia_ManHashStart( pNew );
    for ( g = 0; g < Gia_ManCoNum(p); g += nOuts )
    {
        vSupp = Gia_ManCollectSuppNew( p, g, nOuts );
        nInputs = Vec_IntSize( vSupp );
        Ttopt::TruthTableReo tt( nInputs, nOuts );
        for ( k = 0; k < nOuts; k++ )
        {
            pObj = Gia_ManCo( p, g+k );
            pTruth = Gia_ObjComputeTruthTableCut( p, Gia_ObjFanin0(pObj), vSupp );
            if ( nInputs >= 6 )
                for ( i = 0; i < tt.nSize; i++ )
                    tt.t[i + tt.nSize * k] = Gia_ObjFaninC0(pObj)? ~pTruth[i]: pTruth[i];
            else
            {
                i = k * (1 << nInputs);
                v = (Gia_ObjFaninC0(pObj)? ~pTruth[0]: pTruth[0]) & tt.ones[nInputs];
                tt.t[i / tt.ww] |= v << (i % tt.ww);
            }
        }
        tt.RandomSiftReo( nRounds );
        Ttopt::TruthTable tt2( nInputs, nOuts );
        tt2.t = tt.t;
        tt2.Reo( tt.vLevels );
        tt2.BDDGenerateAig( pNew, vSupp );
        Vec_IntFree( vSupp );
    }
    Gia_ObjComputeTruthTableStop( p );
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    return pNew;
}

Gia_Man_t * Gia_ManTtoptCare( Gia_Man_t * p, int nIns, int nOuts, int nRounds, char * pFileName, int nRarity )
{
    int fVerbose = 0;
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    Vec_Int_t * vSupp;
    word v;
    word * pTruth, * pCare;
    int i, g, k, nInputs;
    Vec_Wrd_t * vSimI = Vec_WrdReadBin( pFileName, fVerbose );
    Gia_ManLevelNum( p );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManForEachCi( p, pObj, k )
        Gia_ManAppendCi( pNew );
    Gia_ObjComputeTruthTableStart( p, nIns );
    Gia_ManHashStart( pNew );
    for ( g = 0; g < Gia_ManCoNum(p); g += nOuts )
    {
        vSupp = Gia_ManCollectSuppNew( p, g, nOuts );
        nInputs = Vec_IntSize( vSupp );
        Ttopt::TruthTableLevelTSM tt( nInputs, nOuts );
        for ( k = 0; k < nOuts; k++ )
        {
            pObj = Gia_ManCo( p, g+k );
            pTruth = Gia_ObjComputeTruthTableCut( p, Gia_ObjFanin0(pObj), vSupp );
            if ( nInputs >= 6 )
                for ( i = 0; i < tt.nSize; i++ )
                    tt.t[i + tt.nSize * k] = Gia_ObjFaninC0(pObj)? ~pTruth[i]: pTruth[i];
            else
            {
                i = k * (1 << nInputs);
                v = (Gia_ObjFaninC0(pObj)? ~pTruth[0]: pTruth[0]) & tt.ones[nInputs];
                tt.t[i / tt.ww] |= v << (i % tt.ww);
            }
        }
        i = 1 << Vec_IntSize( vSupp );
        pCare = Gia_ManCountFraction( p, vSimI, vSupp, nRarity, fVerbose, &i );
        tt.care[0] = pCare[0];
        for ( i = 1; i < tt.nSize; i++ )
            tt.care[i] = pCare[i];
        ABC_FREE( pCare );
        tt.RandomSiftReo( nRounds );
        tt.Optimize();
        tt.BDDGenerateAig( pNew, vSupp );
        Vec_IntFree( vSupp );
    }
    Gia_ObjComputeTruthTableStop( p );
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    Vec_WrdFreeP( &vSimI );
    return pNew;
}

ABC_NAMESPACE_IMPL_END

