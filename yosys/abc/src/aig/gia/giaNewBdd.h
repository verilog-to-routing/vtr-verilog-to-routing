/**CFile****************************************************************

  FileName    [giaNewBdd.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Implementation of transduction method.]

  Author      [Yukio Miyasaka]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 2023.]

  Revision    [$Id: giaNewBdd.h,v 1.00 2023/05/10 00:00:00 Exp $]

***********************************************************************/

#ifndef ABC__aig__gia__giaNewBdd_h
#define ABC__aig__gia__giaNewBdd_h

#include <cstdlib>
#include <limits>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cmath>

ABC_NAMESPACE_CXX_HEADER_START

namespace NewBdd {

  typedef unsigned short     var;
  typedef int                bvar;
  typedef unsigned           lit;
  typedef unsigned short     ref;
  typedef unsigned long long size;
  typedef unsigned           edge;
  typedef unsigned           uniq;
  typedef unsigned           cac;
  static inline var  VarMax()                     { return std::numeric_limits<var>::max();  }
  static inline bvar BvarMax()                    { return std::numeric_limits<bvar>::max(); }
  static inline lit  LitMax()                     { return std::numeric_limits<lit>::max();  }
  static inline ref  RefMax()                     { return std::numeric_limits<ref>::max();  }
  static inline size SizeMax()                    { return std::numeric_limits<size>::max(); }
  static inline uniq UniqHash(lit Arg0, lit Arg1) { return Arg0 + 4256249 * Arg1;            }
  static inline cac  CacHash(lit Arg0, lit Arg1)  { return Arg0 + 4256249 * Arg1;            }

  static inline void fatal_error(const char* message) {
    std::cerr << message << std::endl;
    std::abort();
  }

  class Cache {
  private:
    cac    nSize;
    cac    nMax;
    cac    Mask;
    size   nLookups;
    size   nHits;
    size   nThold;
    double HitRate;
    int    nVerbose;
    std::vector<lit> vCache;

  public:
    Cache(int nCacheSizeLog, int nCacheMaxLog, int nVerbose): nVerbose(nVerbose) {
      if(nCacheMaxLog < nCacheSizeLog)
        fatal_error("nCacheMax must not be smaller than nCacheSize");
      nMax = (cac)1 << nCacheMaxLog;
      if(!(nMax << 1))
        fatal_error("Memout (nCacheMax) in init");
      nSize = (cac)1 << nCacheSizeLog;
      if(nVerbose)
        std::cout << "Allocating " << nSize << " cache entries" << std::endl;
      vCache.resize(nSize * 3);
      Mask = nSize - 1;
      nLookups = 0;
      nHits = 0;
      nThold = (nSize == nMax)? SizeMax(): nSize;
      HitRate = 1;
    }
    ~Cache() {
      if(nVerbose)
        std::cout << "Free " << nSize << " cache entries" << std::endl;
    }
    inline lit Lookup(lit x, lit y) {
      nLookups++;
      if(nLookups > nThold) {
        double NewHitRate = (double)nHits / nLookups;
        if(nVerbose >= 2)
          std::cout << "Cache Hits: " << std::setw(10) << nHits << ", "
                    << "Lookups: " << std::setw(10) << nLookups << ", "
                    << "Rate: " << std::setw(10) << NewHitRate
                    << std::endl;
        if(NewHitRate > HitRate)
          Resize();
        if(nSize == nMax)
          nThold = SizeMax();
        else {
          nThold <<= 1;
          if(!nThold)
            nThold = SizeMax();
        }
        HitRate = NewHitRate;
      }
      cac i = (CacHash(x, y) & Mask) * 3;
      if(vCache[i] == x && vCache[i + 1] == y) {
        if(nVerbose >= 3)
          std::cout << "Cache hit: "
                    << "x = " << std::setw(10) << x << ", "
                    << "y = " << std::setw(10) << y << ", "
                    << "z = " << std::setw(10) << vCache[i + 2] << ", "
                    << "hash = " << std::hex << (CacHash(x, y) & Mask) << std::dec
                    << std::endl;
        nHits++;
        return vCache[i + 2];
      }
      return LitMax();
    }
    inline void Insert(lit x, lit y, lit z) {
      cac i = (CacHash(x, y) & Mask) * 3;
      vCache[i] = x;
      vCache[i + 1] = y;
      vCache[i + 2] = z;
      if(nVerbose >= 3)
        std::cout << "Cache ent: "
                  << "x = " << std::setw(10) << x << ", "
                  << "y = " << std::setw(10) << y << ", "
                  << "z = " << std::setw(10) << z << ", "
                  << "hash = " << std::hex << (CacHash(x, y) & Mask) << std::dec
                  << std::endl;
    }
    inline void Clear() {
      std::fill(vCache.begin(), vCache.end(), 0);
    }
    void Resize() {
      cac nSizeOld = nSize;
      nSize <<= 1;
      if(nVerbose >= 2)
        std::cout << "Reallocating " << nSize << " cache entries" << std::endl;
      vCache.resize(nSize * 3);
      Mask = nSize - 1;
      for(cac j = 0; j < nSizeOld; j++) {
        cac i = j * 3;
        if(vCache[i] || vCache[i + 1]) {
          cac hash = (CacHash(vCache[i], vCache[i + 1]) & Mask) * 3;
          vCache[hash] = vCache[i];
          vCache[hash + 1] = vCache[i + 1];
          vCache[hash + 2] = vCache[i + 2];
          if(nVerbose >= 3)
            std::cout << "Cache mov: "
                      << "x = " << std::setw(10) << vCache[i] << ", "
                      << "y = " << std::setw(10) << vCache[i + 1] << ", "
                      << "z = " << std::setw(10) << vCache[i + 2] << ", "
                      << "hash = " << std::hex << (CacHash(vCache[i], vCache[i + 1]) & Mask) << std::dec
                      << std::endl;
        }
      }
    }
  };

  struct Param {
    int    nObjsAllocLog;
    int    nObjsMaxLog;
    int    nUniqueSizeLog;
    double UniqueDensity;
    int    nCacheSizeLog;
    int    nCacheMaxLog;
    int    nCacheVerbose;
    bool   fCountOnes;
    int    nGbc;
    bvar   nReo;
    double MaxGrowth;
    bool   fReoVerbose;
    int    nVerbose;
    std::vector<var> *pVar2Level;
    Param() {
      nObjsAllocLog  = 20;
      nObjsMaxLog    = 25;
      nUniqueSizeLog = 10;
      UniqueDensity  = 4;
      nCacheSizeLog  = 15;
      nCacheMaxLog   = 20;
      nCacheVerbose  = 0;
      fCountOnes     = false;
      nGbc           = 0;
      nReo           = BvarMax();
      MaxGrowth      = 1.2;
      fReoVerbose    = false;
      nVerbose       = 0;
      pVar2Level     = NULL;
    }
  };

  class Man {
  private:
    var    nVars;
    bvar   nObjs;
    bvar   nObjsAlloc;
    bvar   nObjsMax;
    bvar   RemovedHead;
    int    nGbc;
    bvar   nReo;
    double MaxGrowth;
    bool   fReoVerbose;
    int    nVerbose;
    std::vector<var>    vVars;
    std::vector<var>    Var2Level;
    std::vector<var>    Level2Var;
    std::vector<lit>    vObjs;
    std::vector<bvar>   vNexts;
    std::vector<bool>   vMarks;
    std::vector<ref>    vRefs;
    std::vector<edge>   vEdges;
    std::vector<double> vOneCounts;
    std::vector<uniq>   vUniqueMasks;
    std::vector<bvar>   vUniqueCounts;
    std::vector<bvar>   vUniqueTholds;
    std::vector<std::vector<bvar> > vvUnique;
    Cache *cache;

  public:
    inline lit  Bvar2Lit(bvar a)          const { return (lit)a << 1;                                       }
    inline lit  Bvar2Lit(bvar a, bool c)  const { return ((lit)a << 1) ^ (lit)c;                            }
    inline bvar Lit2Bvar(lit x)           const { return (bvar)(x >> 1);                                    }
    inline var  VarOfBvar(bvar a)         const { return vVars[a];                                          }
    inline lit  ThenOfBvar(bvar a)        const { return vObjs[Bvar2Lit(a)];                                }
    inline lit  ElseOfBvar(bvar a)        const { return vObjs[Bvar2Lit(a, true)];                          }
    inline ref  RefOfBvar(bvar a)         const { return vRefs[a];                                          }
    inline lit  Const0()                  const { return (lit)0;                                            }
    inline lit  Const1()                  const { return (lit)1;                                            }
    inline bool IsConst0(lit x)           const { return x == Const0();                                     }
    inline bool IsConst1(lit x)           const { return x == Const1();                                     }
    inline lit  IthVar(var v)             const { return Bvar2Lit((bvar)v + 1);                             }
    inline lit  LitRegular(lit x)         const { return x & ~(lit)1;                                       }
    inline lit  LitIrregular(lit x)       const { return x | (lit)1;                                        }
    inline lit  LitNot(lit x)             const { return x ^ (lit)1;                                        }
    inline lit  LitNotCond(lit x, bool c) const { return x ^ (lit)c;                                        }
    inline bool LitIsCompl(lit x)         const { return x & (lit)1;                                        }
    inline bool LitIsEq(lit x, lit y)     const { return x == y;                                            }
    inline var  Var(lit x)                const { return vVars[Lit2Bvar(x)];                                }
    inline var  Level(lit x)              const { return Var2Level[Var(x)];                                 }
    inline lit  Then(lit x)               const { return LitNotCond(vObjs[LitRegular(x)], LitIsCompl(x));   }
    inline lit  Else(lit x)               const { return LitNotCond(vObjs[LitIrregular(x)], LitIsCompl(x)); }
    inline ref  Ref(lit x)                const { return vRefs[Lit2Bvar(x)];                                }
    inline double OneCount(lit x)         const {
      if(vOneCounts.empty())
        fatal_error("fCountOnes was not set");
      if(LitIsCompl(x))
        return std::pow(2.0, nVars) - vOneCounts[Lit2Bvar(x)];
      return vOneCounts[Lit2Bvar(x)];
    }

  public:
    inline void IncRef(lit x)              { if(!vRefs.empty() && Ref(x) != RefMax()) vRefs[Lit2Bvar(x)]++; }
    inline void DecRef(lit x)              { if(!vRefs.empty() && Ref(x) != RefMax()) vRefs[Lit2Bvar(x)]--; }

  private:
    inline bool Mark(lit x)               const { return vMarks[Lit2Bvar(x)];                               }
    inline edge Edge(lit x)               const { return vEdges[Lit2Bvar(x)];                               }
    inline void SetMark(lit x)                  { vMarks[Lit2Bvar(x)] = true;                               }
    inline void ResetMark(lit x)                { vMarks[Lit2Bvar(x)] = false;                              }
    inline void IncEdge(lit x)                  { vEdges[Lit2Bvar(x)]++;                                    }
    inline void DecEdge(lit x)                  { vEdges[Lit2Bvar(x)]--;                                    }
    inline bool MarkOfBvar(bvar a)        const { return vMarks[a];                                         }
    inline edge EdgeOfBvar(bvar a)        const { return vEdges[a];                                         }
    inline void SetVarOfBvar(bvar a, var v)     { vVars[a] = v;                                             }
    inline void SetThenOfBvar(bvar a, lit x)    { vObjs[Bvar2Lit(a)] = x;                                   }
    inline void SetElseOfBvar(bvar a, lit x)    { vObjs[Bvar2Lit(a, true)] = x;                             }
    inline void SetMarkOfBvar(bvar a)           { vMarks[a] = true;                                         }
    inline void ResetMarkOfBvar(bvar a)         { vMarks[a] = false;                                        }
    inline void RemoveBvar(bvar a)              {
      var v = VarOfBvar(a);
      SetVarOfBvar(a, VarMax());
      std::vector<bvar>::iterator q = vvUnique[v].begin() + (UniqHash(ThenOfBvar(a), ElseOfBvar(a)) & vUniqueMasks[v]);
      for(; *q; q = vNexts.begin() + *q)
        if(*q == a)
          break;
      bvar next = vNexts[*q];
      vNexts[*q] = RemovedHead;
      RemovedHead = *q;
      *q = next;
      vUniqueCounts[v]--;
    }

  private:
    void SetMark_rec(lit x) {
      if(x < 2 || Mark(x))
        return;
      SetMark(x);
      SetMark_rec(Then(x));
      SetMark_rec(Else(x));
    }
    void ResetMark_rec(lit x) {
      if(x < 2 || !Mark(x))
        return;
      ResetMark(x);
      ResetMark_rec(Then(x));
      ResetMark_rec(Else(x));
    }
    bvar CountNodes_rec(lit x) {
      if(x < 2 || Mark(x))
        return 0;
      SetMark(x);
      return 1 + CountNodes_rec(Then(x)) + CountNodes_rec(Else(x));
    }
    void CountEdges_rec(lit x) {
      if(x < 2)
        return;
      IncEdge(x);
      if(Mark(x))
        return;
      SetMark(x);
      CountEdges_rec(Then(x));
      CountEdges_rec(Else(x));
    }
    void CountEdges() {
      vEdges.resize(nObjsAlloc);
      for(bvar a = (bvar)nVars + 1; a < nObjs; a++)
        if(RefOfBvar(a))
          CountEdges_rec(Bvar2Lit(a));
      for(bvar a = 1; a <= (bvar)nVars; a++)
        vEdges[a]++;
      for(bvar a = (bvar)nVars + 1; a < nObjs; a++)
        if(RefOfBvar(a))
          ResetMark_rec(Bvar2Lit(a));
    }

  public:
    bool Resize() {
      if(nObjsAlloc == nObjsMax)
        return false;
      lit nObjsAllocLit = (lit)nObjsAlloc << 1;
      if(nObjsAllocLit > (lit)BvarMax())
        nObjsAlloc = BvarMax();
      else
        nObjsAlloc = (bvar)nObjsAllocLit;
      if(nVerbose >= 2)
        std::cout << "Reallocating " << nObjsAlloc << " nodes" << std::endl;
      vVars.resize(nObjsAlloc);
      vObjs.resize((lit)nObjsAlloc * 2);
      vNexts.resize(nObjsAlloc);
      vMarks.resize(nObjsAlloc);
      if(!vRefs.empty())
        vRefs.resize(nObjsAlloc);
      if(!vEdges.empty())
        vEdges.resize(nObjsAlloc);
      if(!vOneCounts.empty())
        vOneCounts.resize(nObjsAlloc);
      return true;
    }
    void ResizeUnique(var v) {
      uniq nUniqueSize, nUniqueSizeOld;
      nUniqueSize = nUniqueSizeOld = vvUnique[v].size();
      nUniqueSize <<= 1;
      if(!nUniqueSize) {
        vUniqueTholds[v] = BvarMax();
        return;
      }
      if(nVerbose >= 2)
        std::cout << "Reallocating " << nUniqueSize << " unique table entries for Var " << v << std::endl;
      vvUnique[v].resize(nUniqueSize);
      vUniqueMasks[v] = nUniqueSize - 1;
      for(uniq i = 0; i < nUniqueSizeOld; i++) {
        std::vector<bvar>::iterator q, tail, tail1, tail2;
        q = tail1 = vvUnique[v].begin() + i;
        tail2 = q + nUniqueSizeOld;
        while(*q) {
          uniq hash = UniqHash(ThenOfBvar(*q), ElseOfBvar(*q)) & vUniqueMasks[v];
          if(hash == i)
            tail = tail1;
          else
            tail = tail2;
          if(tail != q)
            *tail = *q, *q = 0;
          q = vNexts.begin() + *tail;
          if(tail == tail1)
            tail1 = q;
          else
            tail2 = q;
        }
      }
      vUniqueTholds[v] <<= 1;
      if((lit)vUniqueTholds[v] > (lit)BvarMax())
        vUniqueTholds[v] = BvarMax();
    }
    bool Gbc() {
      if(nVerbose >= 2)
        std::cout << "Garbage collect" << std::endl;
      if(!vEdges.empty()) {
        for(bvar a = (bvar)nVars + 1; a < nObjs; a++)
          if(!EdgeOfBvar(a) && VarOfBvar(a) != VarMax())
            RemoveBvar(a);
      } else {
        for(bvar a = (bvar)nVars + 1; a < nObjs; a++)
          if(RefOfBvar(a))
            SetMark_rec(Bvar2Lit(a));
        for(bvar a = (bvar)nVars + 1; a < nObjs; a++)
          if(!MarkOfBvar(a) && VarOfBvar(a) != VarMax())
            RemoveBvar(a);
        for(bvar a = (bvar)nVars + 1; a < nObjs; a++)
          if(RefOfBvar(a))
            ResetMark_rec(Bvar2Lit(a));
      }
      cache->Clear();
      return RemovedHead;
    }

  private:
    inline lit UniqueCreateInt(var v, lit x1, lit x0) {
      std::vector<bvar>::iterator p, q;
      p = q = vvUnique[v].begin() + (UniqHash(x1, x0) & vUniqueMasks[v]);
      for(; *q; q = vNexts.begin() + *q)
        if(VarOfBvar(*q) == v && ThenOfBvar(*q) == x1 && ElseOfBvar(*q) == x0)
          return Bvar2Lit(*q);
      bvar next = *p;
      if(nObjs < nObjsAlloc)
        *p = nObjs++;
      else if(RemovedHead)
        *p = RemovedHead, RemovedHead = vNexts[*p];
      else
        return LitMax();
      SetVarOfBvar(*p, v);
      SetThenOfBvar(*p, x1);
      SetElseOfBvar(*p, x0);
      vNexts[*p] = next;
      if(!vOneCounts.empty())
        vOneCounts[*p] = OneCount(x1) / 2 + OneCount(x0) / 2;
      if(nVerbose >= 3) {
        std::cout << "Create node " << std::setw(10) << *p << ": "
                  << "Var = " << std::setw(6) << v << ", "
                  << "Then = " << std::setw(10) << x1 << ", "
                  << "Else = " << std::setw(10) << x0;
        if(!vOneCounts.empty())
          std::cout << ", Ones = " << std::setw(10) << vOneCounts[*q];
        std::cout << std::endl;
      }
      vUniqueCounts[v]++;
      if(vUniqueCounts[v] > vUniqueTholds[v]) {
        bvar a = *p;
        ResizeUnique(v);
        return Bvar2Lit(a);
      }
      return Bvar2Lit(*p);
    }
    inline lit UniqueCreate(var v, lit x1, lit x0) {
      if(x1 == x0)
        return x1;
      lit x;
      while(true) {
        if(!LitIsCompl(x0))
          x = UniqueCreateInt(v, x1, x0);
        else
          x = UniqueCreateInt(v, LitNot(x1), LitNot(x0));
        if(x == LitMax()) {
          bool fRemoved = false;
          if(nGbc > 1)
            fRemoved = Gbc();
          if(!Resize() && !fRemoved && (nGbc != 1 || !Gbc()))
            fatal_error("Memout (node)");
        } else
          break;
      }
      return LitIsCompl(x0)? LitNot(x): x;
    }
    lit And_rec(lit x, lit y) {
      if(x == 0 || y == 1)
        return x;
      if(x == 1 || y == 0)
        return y;
      if(Lit2Bvar(x) == Lit2Bvar(y))
        return (x == y)? x: 0;
      if(x > y)
        std::swap(x, y);
      lit z = cache->Lookup(x, y);
      if(z != LitMax())
        return z;
      var v;
      lit x0, x1, y0, y1;
      if(Level(x) < Level(y))
        v = Var(x), x1 = Then(x), x0 = Else(x), y0 = y1 = y;
      else if(Level(x) > Level(y))
        v = Var(y), x0 = x1 = x, y1 = Then(y), y0 = Else(y);
      else
        v = Var(x), x1 = Then(x), x0 = Else(x), y1 = Then(y), y0 = Else(y);
      lit z1 = And_rec(x1, y1);
      IncRef(z1);
      lit z0 = And_rec(x0, y0);
      IncRef(z0);
      z = UniqueCreate(v, z1, z0);
      DecRef(z1);
      DecRef(z0);
      cache->Insert(x, y, z);
      return z;
    }

  private:
    bvar Swap(var i) {
      var v1 = Level2Var[i];
      var v2 = Level2Var[i + 1];
      bvar f = 0;
      bvar diff = 0;
      for(std::vector<bvar>::iterator p = vvUnique[v1].begin(); p != vvUnique[v1].end(); p++) {
        std::vector<bvar>::iterator q = p;
        while(*q) {
          if(!EdgeOfBvar(*q)) {
            SetVarOfBvar(*q, VarMax());
            bvar next = vNexts[*q];
            vNexts[*q] = RemovedHead;
            RemovedHead = *q;
            *q = next;
            vUniqueCounts[v1]--;
            continue;
          }
          lit f1 = ThenOfBvar(*q);
          lit f0 = ElseOfBvar(*q);
          if(Var(f1) == v2 || Var(f0) == v2) {
            DecEdge(f1);
            if(Var(f1) == v2 && !Edge(f1))
              DecEdge(Then(f1)), DecEdge(Else(f1)), diff--;
            DecEdge(f0);
            if(Var(f0) == v2 && !Edge(f0))
              DecEdge(Then(f0)), DecEdge(Else(f0)), diff--;
            bvar next = vNexts[*q];
            vNexts[*q] = f;
            f = *q;
            *q = next;
            vUniqueCounts[v1]--;
            continue;
          }
          q = vNexts.begin() + *q;
        }
      }
      while(f) {
        lit f1 = ThenOfBvar(f);
        lit f0 = ElseOfBvar(f);
        lit f00, f01, f10, f11;
        if(Var(f1) == v2)
          f11 = Then(f1), f10 = Else(f1);
        else
          f10 = f11 = f1;
        if(Var(f0) == v2)
          f01 = Then(f0), f00 = Else(f0);
        else
          f00 = f01 = f0;
        if(f11 == f01)
          f1 = f11;
        else {
          f1 = UniqueCreate(v1, f11, f01);
          if(!Edge(f1))
            IncEdge(f11), IncEdge(f01), diff++;
        }
        IncEdge(f1);
        IncRef(f1);
        if(f10 == f00)
          f0 = f10;
        else {
          f0 = UniqueCreate(v1, f10, f00);
          if(!Edge(f0))
            IncEdge(f10), IncEdge(f00), diff++;
        }
        IncEdge(f0);
        DecRef(f1);
        SetVarOfBvar(f, v2);
        SetThenOfBvar(f, f1);
        SetElseOfBvar(f, f0);
        std::vector<bvar>::iterator q = vvUnique[v2].begin() + (UniqHash(f1, f0) & vUniqueMasks[v2]);
        lit next = vNexts[f];
        vNexts[f] = *q;
        *q = f;
        vUniqueCounts[v2]++;
        f = next;
      }
      Var2Level[v1] = i + 1;
      Var2Level[v2] = i;
      Level2Var[i] = v2;
      Level2Var[i + 1] = v1;
      return diff;
    }
    void Sift() {
      bvar count = CountNodes();
      std::vector<var> sift_order(nVars);
      for(var v = 0; v < nVars; v++)
        sift_order[v] = v;
      for(var i = 0; i < nVars; i++) {
        var max_j = i;
        for(var j = i + 1; j < nVars; j++)
          if(vUniqueCounts[sift_order[j]] > vUniqueCounts[sift_order[max_j]])
            max_j = j;
        if(max_j != i)
          std::swap(sift_order[max_j], sift_order[i]);
      }
      for(var v = 0; v < nVars; v++) {
        bvar lev = Var2Level[sift_order[v]];
        bool UpFirst = lev < (bvar)(nVars / 2);
        bvar min_lev = lev;
        bvar min_diff = 0;
        bvar diff = 0;
        bvar thold = count * (MaxGrowth - 1);
        if(fReoVerbose)
          std::cout << "Sift " << sift_order[v] << " : Level = " << lev << " Count = " << count << " Thold = " << thold << std::endl;
        if(UpFirst) {
          lev--;
          for(; lev >= 0; lev--) {
            diff += Swap(lev);
            if(fReoVerbose)
              std::cout << "\tSwap " << lev << " : Diff = " << diff << " Thold = " << thold << std::endl;
            if(diff < min_diff)
              min_lev = lev, min_diff = diff, thold = (count + diff) * (MaxGrowth - 1);
            else if(diff > thold) {
              lev--;
              break;
            }
          }
          lev++;
        }
        for(; lev < (bvar)nVars - 1; lev++) {
          diff += Swap(lev);
          if(fReoVerbose)
            std::cout << "\tSwap " << lev << " : Diff = " << diff << " Thold = " << thold << std::endl;
          if(diff <= min_diff)
            min_lev = lev + 1, min_diff = diff, thold = (count + diff) * (MaxGrowth - 1);
          else if(diff > thold) {
            lev++;
            break;
          }
        }
        lev--;
        if(UpFirst) {
          for(; lev >= min_lev; lev--) {
            diff += Swap(lev);
            if(fReoVerbose)
              std::cout << "\tSwap " << lev << " : Diff = " << diff << " Thold = " << thold << std::endl;
          }
        } else {
          for(; lev >= 0; lev--) {
            diff += Swap(lev);
            if(fReoVerbose)
              std::cout << "\tSwap " << lev << " : Diff = " << diff << " Thold = " << thold << std::endl;
            if(diff <= min_diff)
              min_lev = lev, min_diff = diff, thold = (count + diff) * (MaxGrowth - 1);
            else if(diff > thold) {
              lev--;
              break;
            }
          }
          lev++;
          for(; lev < min_lev; lev++) {
            diff += Swap(lev);
            if(fReoVerbose)
              std::cout << "\tSwap " << lev << " : Diff = " << diff << " Thold = " << thold << std::endl;
          }
        }
        count += min_diff;
        if(fReoVerbose)
          std::cout << "Sifted " << sift_order[v] << " : Level = " << min_lev << " Count = " << count << " Thold = " << thold << std::endl;
      }
    }

  public:
    Man(int nVars_, Param p) {
      nVerbose = p.nVerbose;
      // parameter sanity check
      if(p.nObjsMaxLog < p.nObjsAllocLog)
        fatal_error("nObjsMax must not be smaller than nObjsAlloc");
      if(nVars_ >= (int)VarMax())
        fatal_error("Memout (nVars) in init");
      nVars = nVars_;
      lit nObjsMaxLit = (lit)1 << p.nObjsMaxLog;
      if(!nObjsMaxLit)
        fatal_error("Memout (nObjsMax) in init");
      if(nObjsMaxLit > (lit)BvarMax())
        nObjsMax = BvarMax();
      else
        nObjsMax = (bvar)nObjsMaxLit;
      lit nObjsAllocLit = (lit)1 << p.nObjsAllocLog;
      if(!nObjsAllocLit)
        fatal_error("Memout (nObjsAlloc) in init");
      if(nObjsAllocLit > (lit)BvarMax())
        nObjsAlloc = BvarMax();
      else
        nObjsAlloc = (bvar)nObjsAllocLit;
      if(nObjsAlloc <= (bvar)nVars)
        fatal_error("nObjsAlloc must be larger than nVars");
      uniq nUniqueSize = (uniq)1 << p.nUniqueSizeLog;
      if(!nUniqueSize)
        fatal_error("Memout (nUniqueSize) in init");
      // allocation
      if(nVerbose)
        std::cout << "Allocating " << nObjsAlloc << " nodes and " << nVars << " x " << nUniqueSize << " unique table entries" << std::endl;
      vVars.resize(nObjsAlloc);
      vObjs.resize((lit)nObjsAlloc * 2);
      vNexts.resize(nObjsAlloc);
      vMarks.resize(nObjsAlloc);
      vvUnique.resize(nVars);
      vUniqueMasks.resize(nVars);
      vUniqueCounts.resize(nVars);
      vUniqueTholds.resize(nVars);
      for(var v = 0; v < nVars; v++) {
        vvUnique[v].resize(nUniqueSize);
        vUniqueMasks[v] = nUniqueSize - 1;
        if((lit)(nUniqueSize * p.UniqueDensity) > (lit)BvarMax())
          vUniqueTholds[v] = BvarMax();
        else
          vUniqueTholds[v] = (bvar)(nUniqueSize * p.UniqueDensity);
      }
      if(p.fCountOnes) {
        if(nVars > 1023)
          fatal_error("nVars must be less than 1024 to count ones");
        vOneCounts.resize(nObjsAlloc);
      }
      // set up cache
      cache = new Cache(p.nCacheSizeLog, p.nCacheMaxLog, p.nCacheVerbose);
      // create nodes for variables
      nObjs = 1;
      vVars[0] = VarMax();
      for(var v = 0; v < nVars; v++)
        UniqueCreateInt(v, 1, 0);
      // set up variable order
      Var2Level.resize(nVars);
      Level2Var.resize(nVars);
      for(var v = 0; v < nVars; v++) {
        if(p.pVar2Level)
          Var2Level[v] = (*p.pVar2Level)[v];
        else
          Var2Level[v] = v;
        Level2Var[Var2Level[v]] = v;
      }
      // set other parameters
      RemovedHead = 0;
      nGbc = p.nGbc;
      nReo = p.nReo;
      MaxGrowth = p.MaxGrowth;
      fReoVerbose = p.fReoVerbose;
      if(nGbc || nReo != BvarMax())
        vRefs.resize(nObjsAlloc);
    }
    ~Man() {
      if(nVerbose) {
        std::cout << "Free " << nObjsAlloc << " nodes (" << nObjs << " live nodes)" << std::endl;
        std::cout << "Free {";
        std::string delim;
        for(var v = 0; v < nVars; v++) {
          std::cout << delim << vvUnique[v].size();
          delim = ", ";
        }
        std::cout << "} unique table entries" << std::endl;
        if(!vRefs.empty())
          std::cout << "Free " << vRefs.size() << " refs" << std::endl;
      }
      delete cache;
    }
    void Reorder() {
      if(nVerbose >= 2)
        std::cout << "Reorder" << std::endl;
      int nGbc_ = nGbc;
      nGbc = 0;
      CountEdges();
      Sift();
      vEdges.clear();
      cache->Clear();
      nGbc = nGbc_;
    }
    inline lit And(lit x, lit y) {
      if(nObjs > nReo) {
        Reorder();
        while(nReo < nObjs) {
          nReo <<= 1;
          if((lit)nReo > (lit)BvarMax())
            nReo = BvarMax();
        }
      }
      return And_rec(x, y);
    }
    inline lit Or(lit x, lit y) {
      return LitNot(And(LitNot(x), LitNot(y)));
    }

  public:
    void SetRef(std::vector<lit> const &vLits) {
      vRefs.clear();
      vRefs.resize(nObjsAlloc);
      for(size_t i = 0; i < vLits.size(); i++)
        IncRef(vLits[i]);
    }
    void RemoveRefIfUnused() {
      if(!nGbc && nReo == BvarMax())
        vRefs.clear();
    }
    void TurnOnReo(int nReo_ = 0, std::vector<lit> const *vLits = NULL) {
      if(nReo_)
        nReo = nReo_;
      else
        nReo = nObjs << 1;
      if((lit)nReo > (lit)BvarMax())
        nReo = BvarMax();
      if(vRefs.empty()) {
        if(vLits)
          SetRef(*vLits);
        else
          vRefs.resize(nObjsAlloc);
      }
    }
    void TurnOffReo() {
      nReo = BvarMax();
    }
    var GetNumVars() const {
      return nVars;
    }
    void GetOrdering(std::vector<int> &Var2Level_) {
      Var2Level_.resize(nVars);
      for(var v = 0; v < nVars; v++)
        Var2Level_[v] = Var2Level[v];
    }
    bvar CountNodes() {
      bvar count = 1;
      if(!vEdges.empty()) {
        for(bvar a = 1; a < nObjs; a++)
          if(EdgeOfBvar(a))
            count++;
        return count;
      }
      for(bvar a = 1; a <= (bvar)nVars; a++) {
        count++;
        SetMarkOfBvar(a);
      }
      for(bvar a = (bvar)nVars + 1; a < nObjs; a++)
        if(RefOfBvar(a))
          count += CountNodes_rec(Bvar2Lit(a));
      for(bvar a = 1; a <= (bvar)nVars; a++)
        ResetMarkOfBvar(a);
      for(bvar a = (bvar)nVars + 1; a < nObjs; a++)
        if(RefOfBvar(a))
          ResetMark_rec(Bvar2Lit(a));
      return count;
    }
    bvar CountNodes(std::vector<lit> const &vLits) {
      bvar count = 1;
      for(size_t i = 0; i < vLits.size(); i++)
        count += CountNodes_rec(vLits[i]);
      for(size_t i = 0; i < vLits.size(); i++)
        ResetMark_rec(vLits[i]);
      return count;
    }
    void PrintStats() {
      bvar nRemoved = 0;
      bvar a = RemovedHead;
      while(a)
        a = vNexts[a], nRemoved++;
      bvar nLive = 1;
      for(var v = 0; v < nVars; v++)
        nLive += vUniqueCounts[v];
      std::cout << "ref: " << std::setw(10) << (vRefs.empty()? 0: CountNodes()) << ", "
                << "used: " << std::setw(10) << nObjs << ", "
                << "live: " << std::setw(10) << nLive << ", "
                << "dead: " << std::setw(10) << nRemoved << ", "
                << "alloc: " << std::setw(10) << nObjsAlloc
                << std::endl;
    }
  };

}

ABC_NAMESPACE_CXX_HEADER_END

#endif
