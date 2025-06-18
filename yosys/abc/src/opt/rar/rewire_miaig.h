/**CFile****************************************************************

  FileName    [rewire_miaig.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Re-wiring.]

  Synopsis    []

  Author      [Jiun-Hao Chen]
  
  Affiliation [National Taiwan University]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: rewire_miaig.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef REWIRE_MIAIG_H
#define REWIRE_MIAIG_H

#define RW_ABC

#ifdef RW_ABC
#include "base/abc/abc.h"
#include "aig/miniaig/miniaig.h"
#include "rewire_map.h"
#else
#ifdef _WIN32
typedef unsigned __int64 word;   // 32-bit windows
#else
typedef long long unsigned word; // other platforms
#endif
#ifdef _WIN32
typedef __int64 iword;   // 32-bit windows
#else
typedef long long iword; // other platforms
#endif
#endif // RW_ABC
#include "rewire_vec.h"
#include "rewire_tt.h"
#include "rewire_time.h"


#include <vector>
#include <algorithm>

#ifdef RW_ABC
ABC_NAMESPACE_CXX_HEADER_START
#endif // RW_ABC

namespace Rewire {

#if RW_THREADS
// exchange-add operation for atomic operations on reference counters
#if defined __riscv && !defined __riscv_atomic
// riscv target without A extension
static inline int RW_XADD(int *addr, int delta) {
    int tmp = *addr;
    *addr += delta;
    return tmp;
}
#elif defined __INTEL_COMPILER && !(defined WIN32 || defined _WIN32)
// atomic increment on the linux version of the Intel(tm) compiler
#define RW_XADD(addr, delta)    \
    (int)_InterlockedExchangeAdd( \
        const_cast<void *>(reinterpret_cast<volatile void *>(addr)), delta)
#elif defined __GNUC__
#if defined __clang__ && __clang_major__ >= 3 && !defined __ANDROID__ && !defined __EMSCRIPTEN__ && !defined(__CUDACC__)
#ifdef __ATOMIC_ACQ_REL
#define RW_XADD(addr, delta) \
    __c11_atomic_fetch_add((_Atomic(int) *)(addr), delta, __ATOMIC_ACQ_REL)
#else
#define RW_XADD(addr, delta) \
    __atomic_fetch_add((_Atomic(int) *)(addr), delta, 4)
#endif
#else
#if defined __ATOMIC_ACQ_REL && !defined __clang__
// version for gcc >= 4.7
#define RW_XADD(addr, delta)                                     \
    (int)__atomic_fetch_add((unsigned *)(addr), (unsigned)(delta), \
                            __ATOMIC_ACQ_REL)
#else
#define RW_XADD(addr, delta) \
    (int)__sync_fetch_and_add((unsigned *)(addr), (unsigned)(delta))
#endif
#endif
#elif defined _MSC_VER && !defined RC_INVOKED
#define RW_XADD(addr, delta) \
    (int)_InterlockedExchangeAdd((long volatile *)addr, delta)
#else
// thread-unsafe branch
static inline int RW_XADD(int *addr, int delta) {
    int tmp = *addr;
    *addr += delta;
    return tmp;
}
#endif
#else  // RW_THREADS
static inline int RW_XADD(int *addr, int delta) {
    int tmp = *addr;
    *addr += delta;
    return tmp;
}
#endif // RW_THREADS

#define Miaig_CustomForEachConstInput(p, i)         for (i = 0; i <= p->nIns; i++)
#define Miaig_CustomForEachInput(p, i)              for (i = 1; i <= p->nIns; i++)
#define Miaig_CustomForEachNode(p, i)               for (i = 1 + p->nIns; i < p->nObjs - p->nOuts; i++)
#define Miaig_CustomForEachNodeReverse(p, i)        for (i = p->nObjs - p->nOuts - 1; i > 1 + p->nIns; i--)
#define Miaig_CustomForEachInputNode(p, i)          for (i = 1; i < p->nObjs - p->nOuts; i++)
#define Miaig_CustomForEachNodeStart(p, i, s)       for (i = s; i < p->nObjs - p->nOuts; i++)
#define Miaig_CustomForEachOutput(p, i)             for (i = p->nObjs - p->nOuts; i < p->nObjs; i++)
#define Miaig_CustomForEachNodeOutput(p, i)         for (i = 1 + p->nIns; i < p->nObjs; i++)
#define Miaig_CustomForEachNodeOutputStart(p, i, s) for (i = s; i < p->nObjs; i++)
#define Miaig_CustomForEachObj(p, i)                for (i = 0; i < p->nObjs; i++)
#define Miaig_CustomForEachObjFanin(p, i, iLit, k)  Vi_ForEachEntry(&p->pvFans[i], iLit, k)

#define Miaig_ForEachConstInput(i)         for (i = 0; i <= _data->nIns; i++)
#define Miaig_ForEachInput(i)              for (i = 1; i <= _data->nIns; i++)
#define Miaig_ForEachNode(i)               for (i = 1 + _data->nIns; i < _data->nObjs - _data->nOuts; i++)
#define Miaig_ForEachNodeReverse(i)        for (i = _data->nObjs - p->nOuts - 1; i > 1 + _data->nIns; i--)
#define Miaig_ForEachInputNode(i)          for (i = 1; i < _data->nObjs - _data->nOuts; i++)
#define Miaig_ForEachNodeStart(i, s)       for (i = s; i < _data->nObjs - _data->nOuts; i++)
#define Miaig_ForEachOutput(i)             for (i = _data->nObjs - _data->nOuts; i < _data->nObjs; i++)
#define Miaig_ForEachNodeOutput(i)         for (i = 1 + _data->nIns; i < _data->nObjs; i++)
#define Miaig_ForEachNodeOutputStart(i, s) for (i = s; i < _data->nObjs; i++)
#define Miaig_ForEachObj(i)                for (i = 0; i < _data->nObjs; i++)
#define Miaig_ForEachObjFanin(i, iLit, k)  Vi_ForEachEntry(&_data->pvFans[i], iLit, k)

static inline int Rw_Lit2LitV(int *pMapV2V, int Lit) {
    assert(Lit >= 0);
    return Abc_Var2Lit(pMapV2V[Abc_Lit2Var(Lit)], Abc_LitIsCompl(Lit));
}

static inline int Rw_Lit2LitL(int *pMapV2L, int Lit) {
    assert(Lit >= 0);
    return Abc_LitNotCond(pMapV2L[Abc_Lit2Var(Lit)], Abc_LitIsCompl(Lit));
}

struct Miaig_Data {
    char *pName;           // network name
    int refcount;          // Reference counter
    int nIns;              // primary inputs
    int nOuts;             // primary outputs
    int nObjs;             // all objects
    int nObjsAlloc;        // allocated space
    int nWords;            // the truth table size
    int nTravIds;          // traversal ID counter
    int *pTravIds;         // traversal IDs
    int *pCopy;            // temp copy
    int *pRefs;            // reference counters
    int *pLevel;           // levels
    int *pDist;            // distances
    word *pTruths[3];      // truth tables
    word *pCare;           // careset
    word *pProd;           // product
    vi *vOrder;            // node order
    vi *vOrderF;           // fanin order
    vi *vOrderF2;          // fanin order
    vi *vTfo;              // transitive fanout cone
    vi *pvFans;            // the array of objects' fanins
    int *pTable;           // structural hashing table
    int TableSize;         // the size of the hash table
    float nTransistor;     // objective value
    vi *pNtkMapped;        // mapped network
};

class Miaig {
public:
    Miaig(void);
    ~Miaig(void);
    Miaig(const Miaig &m);
    Miaig &operator=(const Miaig &m);
    Miaig(int nIns, int nOuts, int nObjsAlloc);
#ifdef RW_ABC
    Miaig(Gia_Man_t *pGia);
    Miaig(Abc_Ntk_t *pNtk);
    Miaig(Mini_Aig_t *pMiniAig);
#endif // RW_ABC

public:
    void addref(void);
    void release(void);

private:
    void create(int nIns, int nOuts, int nObjsAlloc);

public:
    int fromGia(Gia_Man_t *pGia);
    int fromMiniAig(Mini_Aig_t *pMiniAig);

public:
    int &nIns(void);
    int &nOuts(void);
    int &nObjs(void);
    int &nObjsAlloc(void);
    int objIsPi(int i);
    int objIsPo(int i);
    int objIsNode(int i);
    void print(void);
    int appendObj(void);
    void appendFanin(int i, int iLit);
    int objFaninNum(int i);
    int objFanin0(int i);
    int objFanin1(int i);
    int &objLevel(int i);
    int &objRef(int i);
    int &objTravId(int i);
    int &objCopy(int i);
    int &objDist(int i);
    int &nTravIds(void);
    word *objTruth(int i, int n);
    int objType(int i);
    int nWords(void);
    void refObj(int iObj);
    void derefObj(int iObj);
    void derefObj_rec(int iObj, int iLitSkip);
    void setName(char *pName);

private:
    int initializeLevels_rec(int iObj);
    void initializeLevels(void);
    void initializeRefs(void);
    void verifyRefs(void);
    void initializeTruth(void);
    void initializeDists(void);

private:
    void markDfs_rec(int iObj);
    int markDfs(void);
    void markDistanceN_rec(int iObj, int n, int limit);
    void markDistanceN(int Obj, int n);
    void topoCollect_rec(int iObj);
    vi *topoCollect(void);
    void reduceFanins(vi *v);
    int *createStops(void);
    void collectSuper_rec(int iLit, int *pStop, vi *vSuper);
    int checkConst(int iObj, word *pCare, int fVerbose);
    void truthSimNode(int i);
    word *truthSimNodeSubset(int i, int m);
    word *truthSimNodeSubset2(int i, vi *vFanins, int nFanins);
    void truthUpdate(vi *vTfo);
    int computeTfo_rec(int iObj);
    vi *computeTfo(int iObj);
    word *computeCareSet(int iObj);
    vi *createRandomOrder(void);
    void addPair(vi *vPair, int iFan1, int iFan2);
    int findPair(vi *vPair);
    int updateFanins(vi *vFans, int iFan1, int iFan2, int iLit);
    void extractBest(vi *vPairs);
    vi *findPairs(word *pSto, int nWords);
    int findShared(int nNewNodesMax);
    int hashTwo(int l0, int l1, int TableSize);
    int *hashLookup(int *pTable, int l0, int l1, int TableSize);

public:
    float countAnd2(int reset = 0, int fDummy = 0);
    // 0: amap 1: &nf 2: &simap
    float countTransistors(int reset = 0, int nMode = 0);
    int countLevel(void);

private:
    void dupDfs_rec(Miaig &pNew, int iObj);
    int buildNodeBalance_rec(Miaig &pNew, vi *vFanins, int begin, int end, int fCprop, int fStrash);

private:
    int buildNode(int l0, int l1, int fCprop, int fStrash);
    int buildNodeBalance(Miaig &pNew, vi *vFanins, int fCprop, int fStrash);
    int buildNodeCascade(Miaig &pNew, vi *vFanins, int fCprop, int fStrash);

private:
    int expandOne(int iObj, int nAddedMax, int nDist, int nExpandableLevel, int fVerbose);
    int reduceOne(int iObj, int fOnlyConst, int fOnlyBuffer, int fHeuristic, int fVerbose);
    int expandThenReduceOne(int iNode, int nFaninAddLimit, int nDist, int nExpandableLevel, int fVerbose);

public:
    Miaig dup(int fRemDangle, int fMapped = 0);
    Miaig dupDfs(void);
    Miaig dupStrash(int fCprop, int fStrash, int fCascade);
    Miaig dupMulti(int nFaninMax_, int nGrowth);
    Miaig expand(int nFaninAddLimitAll, int nDist, int nExpandableLevel, int nVerbose);
    Miaig share(int nNewNodesMax);
    Miaig reduce(int fVerbose);
    Miaig expandThenReduce(int nFaninAddLimit, int nDist, int nExpandableLevel, int fVerbose);
    Miaig expandShareReduce(int nFaninAddLimitAll, int nDivs, int nDist, int nExpandableLevel, int nVerbose);
    Miaig rewire(int nIters, float levelGrowRatio, int nExpands, int nGrowth, int nDivs, int nFaninMax, int nTimeOut, int nMode, int nMappedMode, int nDist, int nVerbose);
    #ifdef RW_ABC
    Gia_Man_t *toGia(void);
    Abc_Ntk_t *toNtk(int fMapped = 0);
    Mini_Aig_t *toMiniAig(void);
#endif // RW_ABC

private:
    int *_refcount;
    Miaig_Data *_data;
};

inline Miaig::Miaig(void)
    : _refcount(nullptr), _data(nullptr) {
}

inline Miaig::Miaig(int nIns, int nOuts, int nObjsAlloc)
    : Miaig() {
    create(nIns, nOuts, nObjsAlloc);
}

#ifdef RW_ABC
inline Miaig::Miaig(Gia_Man_t *pGia) : Miaig() {
    fromGia(pGia);
}

inline Miaig::Miaig(Abc_Ntk_t *pNtk) : Miaig() {
    Mini_Aig_t *pMiniAig = Abc_ManRewireMiniAigFromNtk(pNtk);
    fromMiniAig(pMiniAig);
    Mini_AigStop(pMiniAig);
}

inline Miaig::Miaig(Mini_Aig_t *pMiniAig) : Miaig() {
    fromMiniAig(pMiniAig);
}
#endif // RW_ABC

inline Miaig::~Miaig(void) {
    release();
}

inline Miaig::Miaig(const Miaig &m)
    : _refcount(m._refcount), _data(m._data) {
    addref();
}

inline Miaig &Miaig::operator=(const Miaig &m) {
    if (this == &m) {
        return *this;
    }

    if (m._refcount) {
        RW_XADD(m._refcount, 1);
    }

    release();

    _refcount = m._refcount;
    _data = m._data;

    return *this;
}

inline void Miaig::addref(void) {
    if (_refcount) {
        RW_XADD(_refcount, 1);
    }
}

inline void Miaig::release(void) {
    if (_refcount && RW_XADD(_refcount, -1) == 1) {
        if (_data) {
            if (_data->pName) free(_data->pName);
            for (int i = 0; i < _data->nObjsAlloc; ++i)
                if (_data->pvFans[i].ptr)
                    free(_data->pvFans[i].ptr);
            free(_data->pvFans);
            Vi_Free(_data->vOrder);
            Vi_Free(_data->vOrderF);
            Vi_Free(_data->vOrderF2);
            Vi_Free(_data->vTfo);
            free(_data->pTravIds);
            free(_data->pCopy);
            free(_data->pRefs);
            free(_data->pTruths[0]);
            if (_data->pCare) free(_data->pCare);
            if (_data->pProd) free(_data->pProd);
            if (_data->pLevel) free(_data->pLevel);
            if (_data->pDist) free(_data->pDist);
            if (_data->pTable) free(_data->pTable);
            if (_data->pNtkMapped) Vi_Free(_data->pNtkMapped);
            delete _data;
        }
    }

    _data = nullptr;
    _refcount = nullptr;
}

inline int &Miaig::nIns(void) {
    return _data->nIns;
}

inline int &Miaig::nOuts(void) {
    return _data->nOuts;
}

inline int &Miaig::nObjs(void) {
    return _data->nObjs;
}

inline int &Miaig::nObjsAlloc(void) {
    return _data->nObjsAlloc;
}

inline int Miaig::objIsPi(int i) {
    return i > 0 && i <= _data->nIns;
}

inline int Miaig::objIsPo(int i) {
    return i >= _data->nObjs - _data->nOuts;
}

inline int Miaig::objIsNode(int i) {
    return i > _data->nIns && i < _data->nObjs - _data->nOuts;
}

inline int Miaig::appendObj(void) {
    assert(_data->nObjs < _data->nObjsAlloc);
    return _data->nObjs++;
}

inline void Miaig::appendFanin(int i, int iLit) {
    Vi_PushOrder(_data->pvFans + i, iLit);
}

inline int Miaig::objFaninNum(int i) {
    return Vi_Size(_data->pvFans + i);
}

inline int Miaig::objFanin0(int i) {
    return Vi_Read(_data->pvFans + i, 0);
}

inline int Miaig::objFanin1(int i) {
    assert(objFaninNum(i) == 2);
    return Vi_Read(_data->pvFans + i, 1);
}

inline int &Miaig::objLevel(int i) {
    return _data->pLevel[i];
}

inline int &Miaig::objRef(int i) {
    return _data->pRefs[i];
}

inline int &Miaig::objTravId(int i) {
    return _data->pTravIds[i];
}

inline int &Miaig::objCopy(int i) {
    return _data->pCopy[i];
}

inline int &Miaig::objDist(int i) {
    return _data->pDist[i];
}

inline int &Miaig::nTravIds(void) {
    return _data->nTravIds;
}

inline int Miaig::nWords(void) {
    return _data->nWords;
}

inline float Miaig::countAnd2(int reset, int fDummy) {
    int i, Counter = 0;
    Miaig_ForEachNode(i) {
        Counter += objFaninNum(i) - 1;
    }
    return Counter;
}

inline int Miaig::countLevel(void) {
    initializeLevels();
    int i, Level = -1;
    Miaig_ForEachOutput(i) {
        Level = Abc_MaxInt(Level, objLevel(i));
    }
    return Level;
}

inline word *Miaig::objTruth(int i, int n) {
    return _data->pTruths[n] + nWords() * i;
}

inline int Miaig::objType(int i) {
    return objTravId(i) == nTravIds();
}

} // namespace Rewire

#ifdef RW_ABC
ABC_NAMESPACE_CXX_HEADER_END
#endif // RW_ABC

#endif // REWIRE_MIAIG_H