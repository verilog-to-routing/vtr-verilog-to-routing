/**CFile****************************************************************

  FileName    [giaNewTt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Implementation of transduction method.]

  Author      [Yukio Miyasaka]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 2023.]

  Revision    [$Id: giaNewTt.h,v 1.00 2023/05/10 00:00:00 Exp $]

***********************************************************************/

#ifndef ABC__aig__gia__giaNewTt_h
#define ABC__aig__gia__giaNewTt_h

#include <limits>
#include <iomanip>
#include <iostream>
#include <vector>
#include <bitset>

ABC_NAMESPACE_CXX_HEADER_START

namespace NewTt {

  typedef int                bvar;
  typedef unsigned           lit;
  typedef unsigned short     ref;
  typedef unsigned long long size;
  
  static inline bvar BvarMax() { return std::numeric_limits<bvar>::max(); }
  static inline lit  LitMax()  { return std::numeric_limits<lit>::max();  }
  static inline ref  RefMax()  { return std::numeric_limits<ref>::max();  }
  static inline size SizeMax() { return std::numeric_limits<size>::max(); }

  struct Param {
    int  nObjsAllocLog;
    int  nObjsMaxLog;
    int  nVerbose;
    bool fCountOnes;
    int  nGbc;
    int  nReo; // dummy
    Param() {
      nObjsAllocLog = 15;
      nObjsMaxLog   = 20;
      nVerbose      = 0;
      fCountOnes    = false;
      nGbc          = 0;
      nReo          = BvarMax();
    }
  };

  class Man {
  private:
    typedef unsigned long long word;
    typedef std::bitset<64> bsw;
    static inline int ww() { return 64; } // word width
    static inline int lww() { return 6; } // log word width
    static inline word one() {return 0xffffffffffffffffull; }
    static inline word vars(int i) {
      static const word vars[] = {0xaaaaaaaaaaaaaaaaull,
                                  0xccccccccccccccccull,
                                  0xf0f0f0f0f0f0f0f0ull,
                                  0xff00ff00ff00ff00ull,
                                  0xffff0000ffff0000ull,
                                  0xffffffff00000000ull};
      return vars[i];
    }
    static inline word ones(int i) {
      static const word ones[] = {0x0000000000000001ull,
                                  0x0000000000000003ull,
                                  0x000000000000000full,
                                  0x00000000000000ffull,
                                  0x000000000000ffffull,
                                  0x00000000ffffffffull,
                                  0xffffffffffffffffull};
      return ones[i];
    }

  private:
    int  nVars;
    bvar nObjs;
    bvar nObjsAlloc;
    bvar nObjsMax;
    size nSize;
    size nTotalSize;
    std::vector<word> vVals;
    std::vector<bvar> vDeads;
    std::vector<ref>  vRefs;
    int nGbc;
    int nVerbose;

  public:
    inline lit  Bvar2Lit(bvar a)          const { return (lit)a << 1;        }
    inline bvar Lit2Bvar(lit x)           const { return (bvar)(x >> 1);     }
    inline lit  IthVar(int v)             const { return ((lit)v + 1) << 1;  }
    inline lit  LitNot(lit x)             const { return x ^ (lit)1;         }
    inline lit  LitNotCond(lit x, bool c) const { return x ^ (lit)c;         }
    inline bool LitIsCompl(lit x)         const { return x & (lit)1;         }
    inline ref  Ref(lit x)                const { return vRefs[Lit2Bvar(x)]; }
    inline lit  Const0()                  const { return (lit)0;             }
    inline lit  Const1()                  const { return (lit)1;             }
    inline bool IsConst0(lit x)           const {
      bvar a = Lit2Bvar(x);
      word c = LitIsCompl(x)? one(): 0;
      for(size j = 0; j < nSize; j++)
        if(vVals[nSize * a + j] ^ c)
          return false;
      return true;
    }
    inline bool IsConst1(lit x)           const {
      bvar a = Lit2Bvar(x);
      word c = LitIsCompl(x)? one(): 0;
      for(size j = 0; j < nSize; j++)
        if(~(vVals[nSize * a + j] ^ c))
          return false;
      return true;
    }
    inline bool LitIsEq(lit x, lit y)     const {
      if(x == y)
        return true;
      if(x == LitMax() || y == LitMax())
        return false;
      bvar xvar = Lit2Bvar(x);
      bvar yvar = Lit2Bvar(y);
      word c = LitIsCompl(x) ^ LitIsCompl(y)? one(): 0;
      for(size j = 0; j < nSize; j++)
        if(vVals[nSize * xvar + j] ^ vVals[nSize * yvar + j] ^ c)
          return false;
      return true;
    }
    inline size OneCount(lit x)           const {
      bvar a = Lit2Bvar(x);
      size count = 0;
      if(nVars > 6) {
        for(size j = 0; j < nSize; j++)
          count += bsw(vVals[nSize * a + j]).count();
      } else
        count = bsw(vVals[nSize * a] & ones(nVars)).count();
      return LitIsCompl(x)? ((size)1 << nVars) - count: count;
    }

  public:
    inline void IncRef(lit x) { if(!vRefs.empty() && Ref(x) != RefMax()) vRefs[Lit2Bvar(x)]++; }
    inline void DecRef(lit x) { if(!vRefs.empty() && Ref(x) != RefMax()) vRefs[Lit2Bvar(x)]--; }

  public:
    bool Resize() {
      if(nObjsAlloc == nObjsMax)
        return false;
      lit nObjsAllocLit = (lit)nObjsAlloc << 1;
      if(nObjsAllocLit > (lit)BvarMax())
        nObjsAlloc = BvarMax();
      else
        nObjsAlloc = (bvar)nObjsAllocLit;
      nTotalSize = nTotalSize << 1;
      if(nVerbose >= 2)
        std::cout << "Reallocating " << nObjsAlloc << " nodes" << std::endl;
      vVals.resize(nTotalSize);
      if(!vRefs.empty())
        vRefs.resize(nObjsAlloc);
      return true;
    }
    bool Gbc() {
      if(nVerbose >= 2)
        std::cout << "Garbage collect" << std::endl;
      for(bvar a = nVars + 1; a < nObjs; a++)
        if(!vRefs[a])
          vDeads.push_back(a);
      return vDeads.size();
    }

  public:
    Man(int nVars, Param p): nVars(nVars) {
      if(p.nObjsMaxLog < p.nObjsAllocLog)
        throw std::invalid_argument("nObjsMax must not be smaller than nObjsAlloc");
      if(nVars >= lww())
        nSize = 1ull << (nVars - lww());
      else
        nSize = 1;
      if(!nSize)
        throw std::length_error("Memout (nVars) in init");
      if(!(nSize << p.nObjsMaxLog))
        throw std::length_error("Memout (nObjsMax) in init");
      lit nObjsMaxLit = (lit)1 << p.nObjsMaxLog;
      if(!nObjsMaxLit)
        throw std::length_error("Memout (nObjsMax) in init");
      if(nObjsMaxLit > (lit)BvarMax())
        nObjsMax = BvarMax();
      else
        nObjsMax = (bvar)nObjsMaxLit;
      lit nObjsAllocLit = (lit)1 << p.nObjsAllocLog;
      if(!nObjsAllocLit)
        throw std::length_error("Memout (nObjsAlloc) in init");
      if(nObjsAllocLit > (lit)BvarMax())
        nObjsAlloc = BvarMax();
      else
        nObjsAlloc = (bvar)nObjsAllocLit;
      if(nObjsAlloc <= (bvar)nVars)
        throw std::invalid_argument("nObjsAlloc must be larger than nVars");
      nTotalSize = nSize << p.nObjsAllocLog;
      vVals.resize(nTotalSize);
      if(p.fCountOnes && nVars > 63)
        throw std::length_error("nVars must be less than 64 to count ones");
      nObjs = 1;
      for(int i = 0; i < 6 && i < nVars; i++) {
        for(size j = 0; j < nSize; j++)
          vVals[nSize * nObjs + j] = vars(i);
        nObjs++;
      }
      for(int i = 0; i < nVars - 6; i++) {
        for(size j = 0; j < nSize; j += (2ull << i))
          for(size k = 0; k < (1ull << i); k++)
            vVals[nSize * nObjs + j + k] = one();
        nObjs++;
      }
      nVerbose = p.nVerbose;
      nGbc = p.nGbc;
      if(nGbc || p.nReo != BvarMax())
        vRefs.resize(nObjsAlloc);
    }
    inline lit And(lit x, lit y) {
      bvar xvar = Lit2Bvar(x);
      bvar yvar = Lit2Bvar(y);
      word xcompl = LitIsCompl(x)? one(): 0;
      word ycompl = LitIsCompl(y)? one(): 0;
      unsigned j;
      if(nObjs >= nObjsAlloc && vDeads.empty()) {
        bool fRemoved = false;
        if(nGbc > 1)
          fRemoved = Gbc();
        if(!Resize() && !fRemoved && (nGbc != 1 || !Gbc()))
          throw std::length_error("Memout (node)");
      }
      bvar zvar;
      if(nObjs < nObjsAlloc)
        zvar = nObjs++;
      else
        zvar = vDeads.back(), vDeads.resize(vDeads.size() - 1);
      for(j = 0; j < nSize; j++)
        vVals[nSize * zvar + j] = (vVals[nSize * xvar + j] ^ xcompl) & (vVals[nSize * yvar + j] ^ ycompl);
      return zvar << 1;
    }
    inline lit Or(lit x, lit y) {
      return LitNot(And(LitNot(x), LitNot(y)));
    }
    void Reorder() {} // dummy

  public:
    void SetRef(std::vector<lit> const &vLits) {
      vRefs.clear();
      vRefs.resize(nObjsAlloc);
      for(size_t i = 0; i < vLits.size(); i++)
        IncRef(vLits[i]);
    }
    void TurnOffReo() {
      if(!nGbc)
        vRefs.clear();
    }
    void PrintNode(lit x) const {
      bvar a = Lit2Bvar(x);
      word c = LitIsCompl(x)? one(): 0;
      for(size j = 0; j < nSize; j++)
        std::cout << bsw(vVals[nSize * a + j] ^ c);
      std::cout << std::endl;
    }
  };

}

ABC_NAMESPACE_CXX_HEADER_END

#endif
