/**CFile****************************************************************

  FileName    [rewire_miaig.cpp]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Re-wiring.]

  Synopsis    []

  Author      [Jiun-Hao Chen]
  
  Affiliation [National Taiwan University]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: rewire_miaig.cpp,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "rewire_rar.h"
#include "rewire_miaig.h"

#ifdef RW_ABC
ABC_NAMESPACE_IMPL_START
#endif // RW_ABC

#ifdef RW_ABC
Gia_Man_t *Gia_ManRewireInt(Gia_Man_t *pGia, int nIters, float levelGrowRatio, int nExpands, int nGrowth, int nDivs, int nFaninMax, int nTimeOut, int nMode, int nMappedMode, int nDist, int nSeed, int fVerbose) {
    Random_Num(nSeed == 0 ? Abc_Random(0) % 10 : nSeed);

    Rewire::Miaig pNtkMiaig(pGia);
    Rewire::Miaig pNew = pNtkMiaig.rewire(nIters, levelGrowRatio, nExpands, nGrowth, nDivs, nFaninMax, nTimeOut, nMode, nMappedMode, nDist, fVerbose);
    pNew.setName(Gia_ManName(pGia));

    return pNew.toGia();
}

Abc_Ntk_t *Abc_ManRewireInt(Abc_Ntk_t *pNtk, int nIters, float levelGrowRatio, int nExpands, int nGrowth, int nDivs, int nFaninMax, int nTimeOut, int nMode, int nMappedMode, int nDist, int nSeed, int fVerbose) {
    Random_Num(nSeed == 0 ? Abc_Random(0) % 10 : nSeed);

    int fMapped = nMode == 1;
    Rewire::Miaig pNtkMiaig(pNtk);
    Rewire::Miaig pNew = pNtkMiaig.rewire(nIters, levelGrowRatio, nExpands, nGrowth, nDivs, nFaninMax, nTimeOut, fMapped, nMappedMode, nDist, fVerbose);
    pNew.setName(Abc_NtkName(pNtk));
    if (nMode == 2) {
        pNew.countTransistors(1, nMappedMode);
    }

    return pNew.toNtk(nMode >= 1);
}

Mini_Aig_t *MiniAig_ManRewireInt(Mini_Aig_t *pAig, int nIters, float levelGrowRatio, int nExpands, int nGrowth, int nDivs, int nFaninMax, int nTimeOut, int nMode, int nMappedMode, int nDist, int nSeed, int fVerbose) {
    Random_Num(nSeed == 0 ? Abc_Random(0) % 10 : nSeed);

    Rewire::Miaig pNtkMiaig(pAig);
    Rewire::Miaig pNew = pNtkMiaig.rewire(nIters, levelGrowRatio, nExpands, nGrowth, nDivs, nFaninMax, nTimeOut, nMode, nMappedMode, nDist, fVerbose);

    return pNew.toMiniAig();
}
#endif // RW_ABC

namespace Rewire {

void Miaig::create(int nIns, int nOuts, int nObjsAlloc) {
    assert(1 + nIns + nOuts <= nObjsAlloc);
    release();

    _data = (Miaig_Data *)calloc(sizeof(Miaig_Data), 1);

    if (_data) {
        _data->nIns = nIns;
        _data->nOuts = nOuts;
        _data->nObjs = 1 + nIns; // const0 + nIns
        _data->nObjsAlloc = nObjsAlloc;
        _data->nWords = Tt_WordNum(nIns);
        _data->nTravIds = 0;
        _data->pTravIds = (int *)calloc(sizeof(int), _data->nObjsAlloc);
        _data->pCopy = (int *)calloc(sizeof(int), _data->nObjsAlloc);
        _data->pRefs = (int *)calloc(sizeof(int), _data->nObjsAlloc);
        _data->vOrder = Vi_Alloc(1000);
        _data->vOrderF = Vi_Alloc(1000);
        _data->vOrderF2 = Vi_Alloc(1000);
        _data->vTfo = Vi_Alloc(1000);
        _data->pvFans = (vi *)calloc(sizeof(vi), _data->nObjsAlloc);
        _data->pLevel = nullptr;
        _data->pTable = nullptr;
        _data->refcount = 1;
        _refcount = &_data->refcount;
    }
}

void Miaig::setName(char *pName) {
    if (_data) {
        if (_data->pName) {
            free(_data->pName);
        }
        _data->pName = strdup(pName);
    }
}

void Miaig::print(void) {
    int i, k, iLit;
    printf("\nAIG printout:\n");
    printf("Const0\n");
    Miaig_ForEachInput(i)
        printf("Pi%d\n", i);
    Miaig_ForEachNode(i) {
        printf("Node%d {", i);
        Miaig_ForEachObjFanin(i, iLit, k)
            printf(" %d", iLit);
        printf(" }\n");
    }
    Miaig_ForEachOutput(i) {
        printf("Po%d ", i);
        Miaig_ForEachObjFanin(i, iLit, k)
            printf(" %d", iLit);
        printf("\n");
    }
}

#ifdef RW_ABC
int Miaig::fromGia(Gia_Man_t *pGia) {
    Gia_Obj_t *pObj;
    int *pCopy = (int *)calloc(sizeof(int), Gia_ManObjNum(pGia)); // obj2obj
    // printf("[INFO] Converting GIA into MIAIG\n");
    // printf("[INFO] PI/PO/OBJ = %d/%d/%d\n", Gia_ManPiNum(pGia), Gia_ManPoNum(pGia), Gia_ManObjNum(pGia));
    create(Gia_ManPiNum(pGia), Gia_ManPoNum(pGia), Gia_ManObjNum(pGia));
    int i;
    pCopy[0] = Gia_ObjId(pGia, Gia_ManConst0(pGia));
    Miaig_ForEachInput(i) {
        pCopy[i] = i;
    }
    Gia_ManForEachAnd(pGia, pObj, i) {
        pCopy[i] = appendObj();
        assert(pCopy[i] == i);
        appendFanin(pCopy[i], Rw_Lit2LitV(pCopy, Gia_Obj2Lit(pGia, Gia_ObjChild1(pObj))));
        appendFanin(pCopy[i], Rw_Lit2LitV(pCopy, Gia_Obj2Lit(pGia, Gia_ObjChild0(pObj))));
    }
    Gia_ManForEachPo(pGia, pObj, i)
        appendFanin(appendObj(), Rw_Lit2LitV(pCopy, Gia_Obj2Lit(pGia, Gia_ObjChild0(pObj))));
    free(pCopy);
    return 1;
}

int Miaig::fromMiniAig(Mini_Aig_t *pMiniAig) {
    int *pCopy = (int *)calloc(sizeof(int), Mini_AigNodeNum(pMiniAig)); // obj2obj
    create(Mini_AigPiNum(pMiniAig), Mini_AigPoNum(pMiniAig), Mini_AigNodeNum(pMiniAig));
    int i;
    Miaig_ForEachInput(i)
        pCopy[i] = i;  
    Mini_AigForEachAnd( pMiniAig, i ) {
        pCopy[i] = appendObj();
        assert( pCopy[i] == i );
        appendFanin(pCopy[i], Rw_Lit2LitV(pCopy, Mini_AigNodeFanin0(pMiniAig, i)));
        appendFanin(pCopy[i], Rw_Lit2LitV(pCopy, Mini_AigNodeFanin1(pMiniAig, i)));
    }
    Mini_AigForEachPo( pMiniAig, i )
        appendFanin(appendObj(), Rw_Lit2LitV(pCopy, Mini_AigNodeFanin0(pMiniAig, i)));
    free(pCopy);
    return 1;
}

Gia_Man_t *Miaig::toGia(void) {
    int i, k, iLit, And2 = countAnd2();
    Gia_Man_t *pGia = Gia_ManStart(1 + nIns() + And2 + nOuts()), *pTemp;
    pGia->pName = Abc_UtilStrsav( _data->pName );
    Gia_ManHashAlloc(pGia);
    memset(_data->pCopy, 0, sizeof(int) * nObjs());
    Miaig_ForEachInput(i)
        objCopy(i) = Gia_ManAppendCi(pGia);
    Miaig_ForEachNode(i) {
        assert(objFaninNum(i) > 0);
        Miaig_ForEachObjFanin(i, iLit, k) {
            if (k == 0)
                objCopy(i) = Rw_Lit2LitL(_data->pCopy, iLit);
            else 
                objCopy(i) = Gia_ManHashAnd(pGia, objCopy(i), Rw_Lit2LitL(_data->pCopy, iLit));
        }
    }
    Miaig_ForEachOutput(i) {
        assert(objFaninNum(i) == 1);
        Miaig_ForEachObjFanin(i, iLit, k)
            Gia_ManAppendCo(pGia, Rw_Lit2LitL(_data->pCopy, iLit));
    }
    // assert(Gia_ManObjNum(pGia) == (1 + nIns() + nOuts() + And2));
    pGia = Gia_ManCleanup(pTemp = pGia);
    Gia_ManStop(pTemp);
    return pGia;
}

Mini_Aig_t *Miaig::toMiniAig(void) {
    int i, k, iLit;
    Mini_Aig_t * pMini = Mini_AigStart();
    memset(_data->pCopy, 0, sizeof(int) * nObjs());
    Miaig_ForEachInput(i)
        objCopy(i) = Mini_AigCreatePi(pMini);
    Miaig_ForEachNode(i) {
        assert(objFaninNum(i) > 0);
        Miaig_ForEachObjFanin(i, iLit, k)
        if (k == 0)
            objCopy(i) = Rw_Lit2LitL(_data->pCopy, iLit);
        else
            objCopy(i) = Mini_AigAnd(pMini, objCopy(i), Rw_Lit2LitL(_data->pCopy, iLit));
    }
    Miaig_ForEachOutput(i) {
        assert(objFaninNum(i) == 1);
        Miaig_ForEachObjFanin(i, iLit, k)
            Mini_AigCreatePo(pMini, Rw_Lit2LitL(_data->pCopy, iLit));
    }
    assert(pMini->nSize == 2 * (1 + nIns() + nOuts() + countAnd2()));
    return pMini;    
}

Abc_Ntk_t *Miaig::toNtk(int fMapped) {
    Abc_Ntk_t *pNtk;
    if (_data->pNtkMapped && fMapped) {
        pNtk = Abc_ManRewireNtkFromMiniMapping(Vi_Array(_data->pNtkMapped));
        ABC_FREE(pNtk->pName);
        Abc_NtkSetName(pNtk, Abc_UtilStrsav(_data->pName));
        return pNtk; 
    }
    Gia_Man_t *pGia = toGia();
    pNtk = Gia_ManRewirePut(pGia);
    Gia_ManStop(pGia);
    return pNtk;
}

vi *moveVecToVi(Vec_Int_t *v) {
    vi *p = (vi *)malloc(sizeof(vi));
    p->size = Vec_IntSize(v);
    p->cap = Vec_IntCap(v);
    p->ptr = Vec_IntArray(v);
    free(v);
    return p;
}
#endif // RW_ABC

// technology mapping
float Miaig::countTransistors(int reset, int nMappedMode) {
    if (!reset && _data->nTransistor) return _data->nTransistor;
#ifdef RW_ABC
    float area = 0;
    Abc_Ntk_t *pNtkMapped = NULL, *pNtkMappedTemp = NULL;
    if (nMappedMode == 0) {        // amap
        Abc_Ntk_t *pNtk = toNtk();
        pNtkMapped = Abc_ManRewireMapAmap(pNtk);
        Abc_NtkDelete(pNtk);
    } else if (nMappedMode == 1) { // &nf
        Gia_Man_t *pGia = toGia();
        pNtkMapped = Gia_ManRewireMapNf(pGia);
        Gia_ManStop(pGia);
    } else if (nMappedMode == 2) { // &simap
        Abc_Ntk_t *pNtk = toNtk();
        pNtkMapped = Abc_ManRewireMapAmap(pNtk);
        area = Abc_NtkGetMappedArea(pNtkMapped);
        Gia_Man_t *pGia = toGia();
        while ((pNtkMappedTemp = Gia_ManRewireMapSimap(pGia, area - 2, 0, 40))) {
            area -= 2;
            Abc_NtkDelete(pNtkMapped);
            pNtkMapped = pNtkMappedTemp;
        }
        Gia_ManStop(pGia);
    }
    if (pNtkMapped) {
        area = Abc_NtkGetMappedArea(pNtkMapped);
        Vec_Int_t *vMapping = Abc_ManRewireNtkWriteMiniMapping(pNtkMapped);
        _data->pNtkMapped = moveVecToVi(vMapping);
        Abc_NtkDelete(pNtkMapped);
    }
#else
    float area = countAnd2(reset, 0);
#endif // RW_ABC

    return _data->nTransistor = area;
}

// topological collection
void Miaig::topoCollect_rec(int iObj) {
    int i, iLit;
    if (objTravId(iObj) == nTravIds())
        return;
    objTravId(iObj) = nTravIds();
    Vi_PushOrder(_data->vOrder, iObj);
    Miaig_ForEachObjFanin(iObj, iLit, i)
        topoCollect_rec(Abc_Lit2Var(iLit));
}

vi *Miaig::topoCollect(void) {
    int i;
    nTravIds()++;
    Vi_Shrink(_data->vOrder, 0);
    Miaig_ForEachConstInput(i)
        objTravId(i) = nTravIds();
    Miaig_ForEachOutput(i)
        topoCollect_rec(Abc_Lit2Var(objFanin0(i)));
    return _data->vOrder;
}

// initializeLevels
int Miaig::initializeLevels_rec(int iObj) {
    int i, iLit;
    if (objLevel(iObj) != -1)
        return objLevel(iObj);
    int level = -1;
    Miaig_ForEachObjFanin(iObj, iLit, i) {
        level = Abc_MaxInt(initializeLevels_rec(Abc_Lit2Var(iLit)), level);
    }
    return objLevel(iObj) = level + 1;
}

void Miaig::initializeLevels(void) {
    if (_data->pLevel) return;
    _data->pLevel = (int *)malloc(sizeof(int) * nObjs());

    int i;
    Miaig_ForEachObj(i) {
        objLevel(i) = -1;
    }
    Miaig_ForEachConstInput(i) {
        objLevel(i) = 0;
    }
    Miaig_ForEachOutput(i) {
        objLevel(i) = initializeLevels_rec(Abc_Lit2Var(objFanin0(i)));
    }
}

// distance computation
void Miaig::initializeDists(void) {
    if (_data->pDist) return;
    _data->pDist = (int *)malloc(sizeof(int) * nObjs());
}

void Miaig::markDistanceN_rec(int iObj, int n, int limit) {
    int i, iLit;
    if (objDist(iObj) >= 0 && objDist(iObj) <= n)
        return;
    objDist(iObj) = n;
    if (n == limit)
        return;
    Miaig_ForEachObjFanin(iObj, iLit, i)
        markDistanceN_rec(Abc_Lit2Var(iLit), n + 1, limit);
}

void Miaig::markDistanceN(int iObj, int n) {
    int i, j, k, iLit;
    memset(_data->pDist, 0xFF, sizeof(int) * nObjs());
    markDistanceN_rec(iObj, 0, n);
    for (i = 0; i < n; ++i) {
        Miaig_ForEachNode(j) {
            if (objDist(j) >= 0) continue;
            int minDist = nObjs();
            Miaig_ForEachObjFanin(j, iLit, k) {
                if (objDist(Abc_Lit2Var(iLit)) == -1) continue;
                minDist = Abc_MinInt(minDist, objDist(Abc_Lit2Var(iLit)));
            }
            if (minDist != nObjs()) {
                markDistanceN_rec(j, minDist + 1, n);
            }
        }
    }
}

// reference counting
void Miaig::refObj(int iObj) {
    int k, iLit;
    Miaig_ForEachObjFanin(iObj, iLit, k)
        objRef(Abc_Lit2Var(iLit))++;
}

void Miaig::derefObj(int iObj) {
    int k, iLit;
    Miaig_ForEachObjFanin(iObj, iLit, k)
        objRef(Abc_Lit2Var(iLit))--;
}

void Miaig::derefObj_rec(int iObj, int iLitSkip) {
    int k, iLit;
    Miaig_ForEachObjFanin(iObj, iLit, k) { 
        if (iLit != iLitSkip && --objRef(Abc_Lit2Var(iLit)) == 0 && objIsNode(Abc_Lit2Var(iLit))) {
            derefObj_rec(Abc_Lit2Var(iLit), -1);
            Vi_Fill(_data->pvFans + Abc_Lit2Var(iLit), 1, 0);
            refObj(Abc_Lit2Var(iLit));
        }
    }
}

void Miaig::initializeRefs(void) {
    int i;
    memset(_data->pRefs, 0, sizeof(int) * _data->nObjs);
    Miaig_ForEachNodeOutput(i)
        refObj(i);
}

void Miaig::verifyRefs(void) {
    int i;
    Miaig_ForEachNodeOutput(i)
        derefObj(i);
    for (i = 0; i < _data->nObjs; i++)
        if (objRef(i))
            printf("Ref count of node %d is incorrect (%d).\n", i, objRef(i));
    initializeRefs();
}

// this procedure marks Const0, PIs, POs, and used nodes with the current trav ID
void Miaig::markDfs_rec(int iObj) {
    int i, iLit;
    if (objTravId(iObj) == nTravIds())
        return;
    objTravId(iObj) = nTravIds();
    Miaig_ForEachObjFanin(iObj, iLit, i)
        markDfs_rec(Abc_Lit2Var(iLit));
}

int Miaig::markDfs(void) {
    int i, nUnused = 0;
    nTravIds()++;
    Miaig_ForEachConstInput(i)
        objTravId(i) = nTravIds();
    Miaig_ForEachOutput(i)
        markDfs_rec(Abc_Lit2Var(objFanin0(i)));
    Miaig_ForEachOutput(i)
        objTravId(i) = nTravIds();
    Miaig_ForEachNode(i)
        nUnused += (int)(objTravId(i) != nTravIds());
    return nUnused;
}

// simple duplicator (optionally removes unused nodes)
Miaig Miaig::dup(int fRemDangle, int fMapped) {
    // Miaig pNew = Maig_Alloc(nIns(), nOuts(), nObjs());
    Miaig pNew(nIns(), nOuts(), nObjs());
    memset(_data->pCopy, 0, sizeof(int) * nObjs());
    int i, k, iLit; // obj2obj
    if (fRemDangle)
        markDfs();
    Miaig_ForEachInput(i)
        objCopy(i) = i;
    Miaig_ForEachNodeOutput(i) {
        assert(objFaninNum(i) > 0);
        if (fRemDangle && objTravId(i) != nTravIds())
            continue;
        objCopy(i) = pNew.appendObj();
        Miaig_ForEachObjFanin(i, iLit, k)
            pNew.appendFanin(objCopy(i), Rw_Lit2LitV(_data->pCopy, iLit));
    }
    if (_data->pNtkMapped && fMapped)
        pNew._data->pNtkMapped = Vi_Dup(_data->pNtkMapped);
    return pNew;
}

// duplicator to restore the topological order
// (the input AIG can have "hidden" internal nodes listed after primary outputs)
void Miaig::dupDfs_rec(Miaig &pNew, int iObj) {
    int i, iLit;
    // 1. return if current node is already marked
    if (objCopy(iObj) >= 0)
        return;
    // 2. create fanins for a given node
    Miaig_ForEachObjFanin(iObj, iLit, i)
        dupDfs_rec(pNew, Abc_Lit2Var(iLit));
    assert(objCopy(iObj) < 0); // combinational loop catching
    assert(objFaninNum(iObj) > 0);
    // 3. create current node
    objCopy(iObj) = pNew.appendObj();
    // 4. append newly created fanins to the current node
    Miaig_ForEachObjFanin(iObj, iLit, i)
        pNew.appendFanin(objCopy(iObj), Rw_Lit2LitV(_data->pCopy, iLit));
}

Miaig Miaig::dupDfs(void) {
    Miaig pNew(nIns(), nOuts(), nObjsAlloc());
    // 1. the array is filled with -1 to distinct visited nodes from unvisited
    memset(_data->pCopy, 0xFF, sizeof(int) * nObjsAlloc());
    int i; // obj2obj
    // for each primary input we mark it with it's index
    Miaig_ForEachConstInput(i)
        objCopy(i) = i;
    // 2. for each primary output we call recursive function for it's fanin
    Miaig_ForEachOutput(i)
        dupDfs_rec(pNew, Abc_Lit2Var(objFanin0(i)));
    // 3. for each primary output append it's fanin
    Miaig_ForEachOutput(i)
        pNew.appendFanin(pNew.appendObj(), Rw_Lit2LitV(_data->pCopy, objFanin0(i)));
    return pNew;
}

// reduces multi-input and-gate represented by an array of fanin literals
void Miaig::reduceFanins(vi *v) {
    assert(Vi_Size(v) > 0);
    Vi_SelectSort(v);

    if (Vi_Read(v, 0) == 0) {
        Vi_Shrink(v, 1);
        return;
    }
    while (Vi_Read(v, 0) == 1)
        Vi_Drop(v, 0);
    if (Vi_Size(v) == 0) {
        Vi_Push(v, 1);
        return;
    }
    if (Vi_Size(v) == 1)
        return;
    int i, iLit, iPrev = Vi_Read(v, 0);
    Vi_ForEachEntryStart(v, iLit, i, 1) {
        if ((iPrev ^ iLit) == 1) {
            Vi_Fill(v, 1, 0);
            return;
        }
        if (iPrev == iLit)
            Vi_Drop(v, i--);
        else
            iPrev = iLit;
    }
}

int Miaig::hashTwo(int l0, int l1, int TableSize) {
    unsigned Key = 0;
    Key += Abc_Lit2Var(l0) * 7937;
    Key += Abc_Lit2Var(l1) * 2971;
    Key += Abc_LitIsCompl(l0) * 911;
    Key += Abc_LitIsCompl(l1) * 353;
    return Key % TableSize;
}

int *Miaig::hashLookup(int *pTable, int l0, int l1, int TableSize) {
    int Key = hashTwo(l0, l1, TableSize);
    for (; pTable[3 * Key]; Key = (Key + 1) % TableSize)
        if (pTable[3 * Key] == l0 && pTable[3 * Key + 1] == l1)
            return pTable + 3 * Key + 2;
    assert(pTable[3 * Key] == 0);
    assert(pTable[3 * Key + 1] == 0);
    assert(pTable[3 * Key + 2] == 0);
    pTable[3 * Key] = l0;
    pTable[3 * Key + 1] = l1;
    return pTable + 3 * Key + 2;
}

// this duplicator creates two-input nodes, propagates constants, and does structural hashing
int Miaig::buildNode(int l0, int l1, int fCprop, int fStrash) {
    if (fCprop) {
        if (l0 == 0 || l1 == 0 || (l0 ^ l1) == 1) return 0;
        if (l0 == l1 || l1 == 1) return l0;
        if (l0 == 1) return l1;
    }
    int *pPlace = NULL;
    if (fStrash) {
        pPlace = hashLookup(_data->pTable, Abc_MinInt(l0, l1), Abc_MaxInt(l0, l1), _data->TableSize);
        if (*pPlace)
            return *pPlace;
    }
    int iObj = appendObj();
    appendFanin(iObj, Abc_MinInt(l0, l1));
    appendFanin(iObj, Abc_MaxInt(l0, l1));
    return pPlace ? (*pPlace = Abc_Var2Lit(iObj, 0)) : Abc_Var2Lit(iObj, 0);
}

int Miaig::buildNodeBalance_rec(Miaig &pNew, vi *vFanins, int begin, int end, int fCprop, int fStrash) {
    int length = end - begin + 1;
    if (length == 1) {
        return Rw_Lit2LitL(_data->pCopy, Vi_Read(vFanins, begin));
    } else if (length == 2) {
        return pNew.buildNode(Rw_Lit2LitL(_data->pCopy, Vi_Read(vFanins, begin)), Rw_Lit2LitL(_data->pCopy, Vi_Read(vFanins, end)), fCprop, fStrash);
    }
    int middle = begin + length / 2;
    int iLit0 = buildNodeBalance_rec(pNew, vFanins, begin, middle - 1, fCprop, fStrash);
    int iLit1 = buildNodeBalance_rec(pNew, vFanins, middle, end, fCprop, fStrash);
    return pNew.buildNode(iLit0, iLit1, fCprop, fStrash);
}

int Miaig::buildNodeBalance(Miaig &pNew, vi *vFanins, int fCprop, int fStrash) {
    return buildNodeBalance_rec(pNew, vFanins, 0, Vi_Size(vFanins) - 1, fCprop, fStrash);
}

int Miaig::buildNodeCascade(Miaig &pNew, vi *vFanins, int fCprop, int fStrash) {
    int iLit = -1;
    for (int i = 0; i < Vi_Size(vFanins); ++i) {
        if (i == 0)
            iLit = Rw_Lit2LitL(_data->pCopy, Vi_Read(vFanins, i));
        else
            iLit = pNew.buildNode(iLit, Rw_Lit2LitL(_data->pCopy, Vi_Read(vFanins, i)), fCprop, fStrash);
    }
    return iLit;
}

Miaig Miaig::dupStrash(int fCprop, int fStrash, int fCascade) {
    int i, nObjsAlloc = 1 + nIns() + nOuts() + countAnd2();
    Miaig pNew(nIns(), nOuts(), nObjsAlloc);
    memset(_data->pCopy, 0, sizeof(int) * nObjs()); // obj2lit
    if (fStrash) {
        assert(pNew._data->pTable == NULL);
        pNew._data->TableSize = Abc_PrimeCudd(3 * countAnd2());
        pNew._data->pTable = (int *)calloc(sizeof(int), 3 * pNew._data->TableSize);
    }
    Miaig_ForEachInput(i)
        objCopy(i) = Abc_Var2Lit(i, 0);
    Miaig_ForEachNode(i) {
        assert(objFaninNum(i) > 0);
        if (fCascade)
            objCopy(i) = buildNodeCascade(pNew, _data->pvFans + i, fCprop, fStrash);
        else
            objCopy(i) = buildNodeBalance(pNew, _data->pvFans + i, fCprop, fStrash);
    }
    Miaig_ForEachOutput(i)
        pNew.appendFanin(pNew.appendObj(), Rw_Lit2LitL(_data->pCopy, objFanin0(i)));
    return pNew.dup(1);
}

// this duplicator converts two-input-node AIG into multi-input-node AIG
int *Miaig::createStops(void) {
    int i, *pStop = (int *)calloc(sizeof(int), nObjs());
    Miaig_ForEachConstInput(i)
        pStop[i] = 2;
    Miaig_ForEachNode(i) {
        assert(objFaninNum(i) == 2);
        int iLit0 = objFanin0(i);
        int iLit1 = objFanin1(i);
        pStop[Abc_Lit2Var(iLit0)] += 1 + Abc_LitIsCompl(iLit0);
        pStop[Abc_Lit2Var(iLit1)] += 1 + Abc_LitIsCompl(iLit1);
    }
    Miaig_ForEachOutput(i)
        pStop[Abc_Lit2Var(objFanin0(i))] += 2;
    return pStop;
}

void Miaig::collectSuper_rec(int iLit, int *pStop, vi *vSuper) {
    if (pStop[Abc_Lit2Var(iLit)] > 1)
        Vi_Push(vSuper, Rw_Lit2LitL(_data->pCopy, iLit));
    else {
        assert(Abc_LitIsCompl(iLit) == 0);
        collectSuper_rec(objFanin0(Abc_Lit2Var(iLit)), pStop, vSuper);
        collectSuper_rec(objFanin1(Abc_Lit2Var(iLit)), pStop, vSuper);
    }
}

Miaig Miaig::dupMulti(int nFaninMax_, int nGrowth) {
    Miaig pNew(nIns(), nOuts(), nObjs());
    int *pStop = createStops();
    int i, k, iLit;
    vi *vArray = Vi_Alloc(100);
    assert(nFaninMax_ >= 2 && nGrowth >= 1);
    memset(_data->pCopy, 0, sizeof(int) * nObjs()); // obj2lit
    Miaig_ForEachConstInput(i)
        objCopy(i) = Abc_Var2Lit(i, 0);
    Miaig_ForEachNode(i) {
        if (pStop[i] == 1)
            continue;
        assert(pStop[i] > 1); // no dangling
        Vi_Shrink(vArray, 0);
        collectSuper_rec(objFanin0(i), pStop, vArray);
        collectSuper_rec(objFanin1(i), pStop, vArray);
        assert(Vi_Size(vArray) > 1);
        reduceFanins(vArray);
        assert(Vi_Size(vArray) > 0);
        if (Vi_Size(vArray) == 1)
            objCopy(i) = Vi_Read(vArray, 0);
        else {
            int nFaninMaxLocal = 2 + (Random_Num(0) % (nFaninMax_ - 1));
            int nGrowthLocal = 1 + (Random_Num(0) % nGrowth);
            assert(nFaninMaxLocal >= 2 && nFaninMaxLocal <= nFaninMax_);
            assert(nGrowthLocal >= 1 && nGrowthLocal <= nGrowth);

            if (Vi_Size(vArray) > nFaninMaxLocal)
                Vi_Randomize(vArray);
            // create a cascade of nodes
            while (Vi_Size(vArray) > nFaninMaxLocal) {
                int iObj = pNew.appendObj();
                vi *vFanins = pNew._data->pvFans + iObj;
                assert(vFanins->ptr == NULL);
                vFanins->cap = nFaninMaxLocal + nGrowthLocal;
                vFanins->ptr = (int *)malloc(sizeof(int) * vFanins->cap);
                Vi_ForEachEntryStop(vArray, iLit, k, nFaninMaxLocal)
                    pNew.appendFanin(iObj, iLit);
                assert(Vi_Space(vFanins) == nGrowthLocal);
                for (k = 0; k < nFaninMaxLocal; k++)
                    Vi_Drop(vArray, 0);
                Vi_Push(vArray, Abc_Var2Lit(iObj, 0));
            }
            // create the last node
            int iObj = pNew.appendObj();
            vi *vFanins = pNew._data->pvFans + iObj;
            assert(vFanins->ptr == NULL);
            vFanins->cap = Vi_Size(vArray) + nGrowthLocal;
            vFanins->ptr = (int *)malloc(sizeof(int) * vFanins->cap);
            Vi_ForEachEntry(vArray, iLit, k)
                pNew.appendFanin(iObj, iLit);
            assert(Vi_Space(vFanins) == nGrowthLocal);
            objCopy(i) = Abc_Var2Lit(iObj, 0);
        }
    }
    Miaig_ForEachOutput(i)
        pNew.appendFanin(pNew.appendObj(), Rw_Lit2LitL(_data->pCopy, objFanin0(i)));
    Vi_Free(vArray);
    free(pStop);
    return pNew;
}

// compute truth table of the node
void Miaig::truthSimNode(int i) {
    int k, iLit;
    Miaig_ForEachObjFanin(i, iLit, k) { 
        if (k == 0) Tt_DupC(objTruth(i, objType(i)), objTruth(Abc_Lit2Var(iLit), objType(Abc_Lit2Var(iLit))), Abc_LitIsCompl(iLit), nWords());
        else Tt_Sharp(objTruth(i, objType(i)), objTruth(Abc_Lit2Var(iLit), objType(Abc_Lit2Var(iLit))), Abc_LitIsCompl(iLit), nWords());
    }
}

// compute truth table of the node using a subset of its current fanin
word *Miaig::truthSimNodeSubset(int i, int m) {
    int k, iLit, Counter = 0;
    assert(m > 0);
    Miaig_ForEachObjFanin(i, iLit, k) {
        if ((m >> k) & 1) { // fanin is included in the subset
            if (Counter++ == 0)
                Tt_DupC(_data->pProd, objTruth(Abc_Lit2Var(iLit), 0), Abc_LitIsCompl(iLit), nWords());
            else
                Tt_Sharp(_data->pProd, objTruth(Abc_Lit2Var(iLit), 0), Abc_LitIsCompl(iLit), nWords());
        }
    }
    assert(Counter == Tt_BitCount16(m));
    return _data->pProd;
}

word *Miaig::truthSimNodeSubset2(int i, vi *vFanins, int nFanins) {
    int k, iLit;
    Vi_ForEachEntryStop(vFanins, iLit, k, nFanins) {
        if (k == 0) Tt_DupC(_data->pProd, objTruth(Abc_Lit2Var(iLit), 0), Abc_LitIsCompl(iLit), nWords());
        else Tt_Sharp(_data->pProd, objTruth(Abc_Lit2Var(iLit), 0), Abc_LitIsCompl(iLit), nWords());
    }
    return _data->pProd;
}

void Miaig::initializeTruth(void) {
    int i;
    if (_data->pTruths[0])
        return;
    _data->pTruths[0] = (word *)calloc(sizeof(word), 3 * nWords() * nObjs());
    _data->pTruths[1] = _data->pTruths[0] + 1 * nWords() * nObjs();
    _data->pTruths[2] = _data->pTruths[0] + 2 * nWords() * nObjs();
    _data->pCare = (word *)calloc(sizeof(word), nWords());
    _data->pProd = (word *)calloc(sizeof(word), nWords());
    float MemMB = 8.0 * nWords() * (3 * nObjs() + 2) / (1 << 20);
    if (MemMB > 100.0)
        printf("Allocated %d truth tables of %d-variable functions (%.2f MB),\n", 3 * nObjs() + 2, nIns(), MemMB);
    nTravIds()++;
    Miaig_ForEachInput(i)
        Tt_ElemInit(objTruth(i, 0), i - 1, nWords());
    Miaig_ForEachNodeOutput(i)
        truthSimNode(i);
    Miaig_ForEachOutput(i)
        assert(objFaninNum(i) == 1);
    Miaig_ForEachOutput(i)
        Tt_Dup(objTruth(i, 2), objTruth(i, 0), nWords());
}

void Miaig::truthUpdate(vi *vTfo) {
    int i, iTemp, nFails = 0;
    nTravIds()++;
    Vi_ForEachEntry(vTfo, iTemp, i) {
        truthSimNode(iTemp);
        if (objIsPo(iTemp) && !Tt_Equal(objTruth(iTemp, 2), objTruth(iTemp, 0), nWords()))
            printf("Verification failed at output %d.\n", iTemp - (nObjs() - nOuts())), nFails++;
    }
    if (nFails)
        printf("Verification failed for %d outputs after updating node %d.\n", nFails, Vi_Read(vTfo, 0));
}

int Miaig::computeTfo_rec(int iObj) {
    int k, iLit, Value = 0;
    if (objTravId(iObj) == nTravIds())
        return 1;
    if (objTravId(iObj) == nTravIds() - 1)
        return 0;
    Miaig_ForEachObjFanin(iObj, iLit, k) {
        Value |= computeTfo_rec(Abc_Lit2Var(iLit));
    }
    objTravId(iObj) = nTravIds() - 1 + Value;
    if (Value) Vi_Push(_data->vTfo, iObj);
    return Value;
}

vi *Miaig::computeTfo(int iObj) {
    int i;
    assert(objIsNode(iObj));
    nTravIds() += 2;
    objTravId(iObj) = nTravIds();
    Vi_Fill(_data->vTfo, 1, iObj);
    Miaig_ForEachConstInput(i)
        objTravId(i) = nTravIds() - 1;
    Miaig_ForEachOutput(i)
        computeTfo_rec(i);
    Miaig_ForEachNode(i) {
        computeTfo_rec(i);
    }
    return _data->vTfo;
}

word *Miaig::computeCareSet(int iObj) {
    vi *vTfo = computeTfo(iObj);
    int i, iTemp;
    Tt_Not(objTruth(iObj, 1), objTruth(iObj, 0), nWords());
    Tt_Clear(_data->pCare, nWords());
    Vi_ForEachEntryStart(vTfo, iTemp, i, 1) {
        truthSimNode(iTemp);
        if (objIsPo(iTemp))
            Tt_OrXor(_data->pCare, objTruth(iTemp, 0), objTruth(iTemp, 1), nWords());
    }
    return _data->pCare;
}

// adds fanin pair to storage
void Miaig::addPair(vi *vPair, int iFan1, int iFan2) {
    assert(iFan1 < iFan2);
    int i, *pArray = Vi_Array(vPair);
    assert(Vi_Size(vPair) % 3 == 0);
    for (i = 0; i < Vi_Size(vPair); i += 3)
        if (pArray[i] == iFan1 && pArray[i + 1] == iFan2)
            break;
    if (i == Vi_Size(vPair)) {
        Vi_Push(vPair, iFan1);
        Vi_Push(vPair, iFan2);
        Vi_Push(vPair, 1);
        pArray = Vi_Array(vPair);
    }
    pArray[i + 2]++;
    //printf( "Adding pair (%d, %d)\n", iFan1, iFan2 );
}

// find fanin pair that appears most often
int Miaig::findPair(vi *vPair) {
    int iBest = -1, BestCost = 0;
    int i, *pArray = Vi_Array(vPair);
    assert(Vi_Size(vPair) % 3 == 0);
    for (i = 0; i < Vi_Size(vPair); i += 3)
        if (BestCost < pArray[i + 2]) {
            BestCost = pArray[i + 2];
            iBest = i;
        }
    //if ( iBest >= 0 ) printf( "Extracting pair (%d, %d) used %d times.\n", pArray[iBest], pArray[iBest+1], pArray[iBest+2] );
    return iBest;
}

// updates one fanin array by replacing the pair with a new literal (iLit)
int Miaig::updateFanins(vi *vFans, int iFan1, int iFan2, int iLit) {
    int f1, f2, iFan1_, iFan2_;
    Vi_ForEachEntry(vFans, iFan1_, f1) if (iFan1_ == iFan1)
        Vi_ForEachEntryStart(vFans, iFan2_, f2, f1 + 1) if (iFan2_ == iFan2) {
        assert(f1 < f2);
        Vi_Drop(vFans, f2);
        Vi_Drop(vFans, f1);
        Vi_Push(vFans, iLit);
        return 1;
    }
    return 0;
}

// updates the network by extracting one pair
void Miaig::extractBest(vi *vPairs) {
    int i, iObj = appendObj();
    int Counter = 0, *pArray = Vi_Array(vPairs);
    int iBest = findPair(vPairs);
    assert(iBest >= 0);
    //printf( "Creating node %d with fanins (%d, %d).\n", iObj, pArray[iBest], pArray[iBest+1] );
    assert(Vi_Size(_data->pvFans + iObj) == 0);
    appendFanin(iObj, pArray[iBest]);
    appendFanin(iObj, pArray[iBest + 1]);
    Miaig_ForEachNode(i)
        Counter += updateFanins(_data->pvFans + i, pArray[iBest], pArray[iBest + 1], Abc_Var2Lit(iObj, 0));
    assert(Counter == pArray[iBest + 2]);
}

// find the set of all pairs that appear more than once
vi *Miaig::findPairs(word *pSto, int nWords) {
    vi *vPairs = Vi_Alloc(30);
    int i, f1, f2, iFan1, iFan2;
    Miaig_ForEachNode(i) {
        vi *vFans = _data->pvFans + i;
        Vi_ForEachEntry(vFans, iFan1, f1) {
            word *pRowFan1 = pSto + iFan1 * nWords;
            Vi_ForEachEntryStart(vFans, iFan2, f2, f1 + 1) {
                if (Tt_GetBit(pRowFan1, iFan2)) addPair(vPairs, iFan1, iFan2);
                else Tt_SetBit(pRowFan1, iFan2);
            }
        }
    }
    return vPairs;
}

// extract shared fanin pairs and return the number of pairs extracted
int Miaig::findShared(int nNewNodesMax) {
    if (nObjs() + nNewNodesMax > nObjsAlloc()) {
        nObjsAlloc() = nObjs() + nNewNodesMax;
        _data->pCopy = (int *)realloc((void *)_data->pCopy, sizeof(int) * nObjsAlloc());
        _data->pvFans = (vi *)realloc((void *)_data->pvFans, sizeof(vi) * nObjsAlloc());
        memset(_data->pCopy + nObjs(), 0, sizeof(int) * (nObjsAlloc() - nObjs()));
        memset(_data->pvFans + nObjs(), 0, sizeof(vi) * (nObjsAlloc() - nObjs()));
    }
    assert(sizeof(word) == 8);
    int i, nWords = (2 * nObjsAlloc() + 63) / 64; // how many words are needed to have a bitstring with one bit for each literal
    int nBytesAll = sizeof(word) * nWords * 2 * nObjsAlloc();
    word *pSto = (word *)malloc(nBytesAll);
    vi *vPairs;
    for (i = 0; i < nNewNodesMax; i++) {
        memset(pSto, 0, nBytesAll);
        nObjs() -= i;
        vPairs = findPairs(pSto, nWords);
        nObjs() += i;
        if (Vi_Size(vPairs) > 0)
            extractBest(vPairs);
        int Size = Vi_Size(vPairs);
        Vi_Free(vPairs);
        if (Size == 0)
            break;
    }
    free(pSto);
    return i;
}

int Miaig::checkConst(int iObj, word *pCare, int fVerbose) {
    word *pFunc = objTruth(iObj, 0);
    if (!Tt_IntersectC(pCare, pFunc, 0, nWords())) {
        derefObj_rec(iObj, -1);
        Vi_Fill(_data->pvFans + iObj, 1, 0); // const0
        refObj(iObj);
        truthUpdate(_data->vTfo);
        if (fVerbose) printf("Detected Const0 at node %d.\n", iObj);
        return 1;
    }
    if (!Tt_IntersectC(pCare, pFunc, 1, nWords())) {
        derefObj_rec(iObj, -1);
        Vi_Fill(_data->pvFans + iObj, 1, 1); // const1
        refObj(iObj);
        truthUpdate(_data->vTfo);
        if (fVerbose) printf("Detected Const1 at node %d.\n", iObj);
        return 1;
    }
    return 0;
}

int Miaig::expandOne(int iObj, int nAddedMax, int nDist, int nExpandableLevel, int fVerbose) {
    int i, k, n, iLit, nAdded = 0;
    word *pCare = computeCareSet(iObj);
    assert(nAddedMax > 0);
    assert(nAddedMax <= Vi_Space(_data->pvFans + iObj));
    // mark node's fanins
    Miaig_ForEachObjFanin(iObj, iLit, k)
        objTravId(Abc_Lit2Var(iLit)) = nTravIds();
    // compute the onset
    word *pOnset = objTruth(iObj, 0);
    Tt_Sharp(pOnset, pCare, 0, nWords());
    // create a random order of fanin candidates
    Vi_Shrink(_data->vOrderF, 0);
    if (nDist) markDistanceN(iObj, nDist);
    Miaig_ForEachInputNode(i) {
        if (nDist && objDist(i) < 0 && !objIsPi(i)) continue;
        // if (nExpandableLevel && objLevel(i) - objLevel(iObj) > nExpandableLevel) continue;
        if (objTravId(i) != nTravIds() && (objIsPi(i) || (objFaninNum(i) > 1 && objRef(i) > 0))) // this node is NOT in the TFO
            Vi_Push(_data->vOrderF, i);
    }
    Vi_Randomize(_data->vOrderF);

    int *pOrderF = Vi_Array(_data->vOrderF);
    std::stable_sort(pOrderF, pOrderF + Vi_Size(_data->vOrderF), [&](int a, int b) {
        return objLevel(Abc_Lit2Var(a)) > objLevel(Abc_Lit2Var(b));
    });
    std::stable_sort(pOrderF, pOrderF + Vi_Size(_data->vOrderF), [&](int a, int b) {
        if (objLevel(Abc_Lit2Var(a)) == 0 || objLevel(Abc_Lit2Var(b)) == 0) {
            return false;
        }
        return objRef(Abc_Lit2Var(a)) < objRef(Abc_Lit2Var(b));
    });

    // iterate through candidate fanins (nodes that are not in the TFO of iObj)
    Vi_ForEachEntry(_data->vOrderF, i, k) {
        assert(objTravId(i) != nTravIds());
        // new fanin can be added if its offset does not intersect with the node's onset
        for (n = 0; n < 2; n++)
            if (!Tt_IntersectC(pOnset, objTruth(i, 0), !n, nWords())) {
                if (fVerbose) printf("Adding node %d fanin %d\n", iObj, Abc_Var2Lit(i, n));
                appendFanin(iObj, Abc_Var2Lit(i, n));
                objRef(i)++;
                nAdded++;
                break;
            }
        if (nAdded == nAddedMax)
            break;
    }
    //printf( "Updating TFO of node %d:  ", iObj );  Vi_Print(_data->vTfo);
    truthUpdate(_data->vTfo);
    //assert( objFaninNum(iObj) <= nFaninMax );
    return nAdded;
}

int Miaig::reduceOne(int iObj, int fOnlyConst, int fOnlyBuffer, int fHeuristic, int fVerbose) {
    int n, k, iLit, nFans = objFaninNum(iObj);
    word *pCare = computeCareSet(iObj);
    if (checkConst(iObj, pCare, fVerbose))
        return nFans;
    if (fOnlyConst)
        return 0;
    if (nFans == 1)
        return 0;
    // if one fanin can be used, take it
    word *pFunc = objTruth(iObj, 0);
    Miaig_ForEachObjFanin(iObj, iLit, k) {
        Tt_DupC(_data->pProd, objTruth(Abc_Lit2Var(iLit), 0), Abc_LitIsCompl(iLit), nWords());
        if (Tt_EqualOnCare(pCare, pFunc, _data->pProd, nWords())) {
            derefObj(iObj);
            Vi_Fill(_data->pvFans + iObj, 1, iLit);
            refObj(iObj);
            truthUpdate(_data->vTfo);
            if (fVerbose) printf("Reducing node %d fanin count from %d to %d.\n", iObj, nFans, objFaninNum(iObj));
            return nFans - 1;
        }
    }
    if (fOnlyBuffer)
        return 0;
    Vi_Shrink(_data->vOrderF, 0);
    Miaig_ForEachObjFanin(iObj, iLit, k)
        Vi_Push(_data->vOrderF, iLit);
    Vi_Randomize(_data->vOrderF);

    if (fHeuristic) {
        int *pOrderF = Vi_Array(_data->vOrderF);
        std::stable_sort(pOrderF, pOrderF + Vi_Size(_data->vOrderF), [&](int a, int b) {
            return objLevel(Abc_Lit2Var(a)) < objLevel(Abc_Lit2Var(b));
        });
        std::stable_sort(pOrderF, pOrderF + Vi_Size(_data->vOrderF), [&](int a, int b) {
            if (objLevel(Abc_Lit2Var(a)) == 0 || objLevel(Abc_Lit2Var(b)) == 0) {
                return false;
            }
            return objRef(Abc_Lit2Var(a)) > objRef(Abc_Lit2Var(b));
        });
    }

    assert(Vi_Size(_data->vOrderF) == nFans);
    // try to remove fanins starting from the end of the list
    for (n = Vi_Size(_data->vOrderF) - 1; n >= 0; n--) {
        int iFanin = Vi_Drop(_data->vOrderF, n);
        word *pProd = truthSimNodeSubset2(iObj, _data->vOrderF, Vi_Size(_data->vOrderF));
        if (!Tt_EqualOnCare(pCare, pFunc, pProd, nWords()))
            Vi_Push(_data->vOrderF, iFanin);
    }
    assert(Vi_Size(_data->vOrderF) >= 1);
    // update the node if it is reduced
    if (Vi_Size(_data->vOrderF) < nFans) {
        derefObj(iObj);
        Vi_Shrink(_data->pvFans + iObj, 0);
        Vi_ForEachEntry(_data->vOrderF, iLit, k)
            Vi_PushOrder(_data->pvFans + iObj, iLit);
        refObj(iObj);
        truthUpdate(_data->vTfo);
        if (fVerbose) printf("Reducing node %d fanin count from %d to %d.\n", iObj, nFans, objFaninNum(iObj));
        return nFans - Vi_Size(_data->vOrderF);
    }
    return 0;
}

int Miaig::expandThenReduceOne(int iNode, int nFaninAddLimit, int nDist, int nExpandableLevel, int fVerbose) {
    expandOne(iNode, Abc_MinInt(Vi_Space(_data->pvFans + iNode), nFaninAddLimit), nDist, nExpandableLevel, fVerbose);
    reduceOne(iNode, 0, 0, 0, fVerbose);
    return 0;
}

vi *Miaig::createRandomOrder(void) {
    int i;
    Vi_Shrink(_data->vOrder, 0);
    Miaig_ForEachNode(i)
        Vi_Push(_data->vOrder, i);
    Vi_Randomize(_data->vOrder);
    return _data->vOrder;
}

Miaig Miaig::expand(int nFaninAddLimitAll, int nDist, int nExpandableLevel, int fVerbose) {
    int i, iNode, nAdded = 0;
    assert(nFaninAddLimitAll > 0);
    vi *vOrder = createRandomOrder();

    initializeTruth();
    initializeRefs();
    initializeLevels();
    if (nDist) initializeDists();
    Vi_ForEachEntry(vOrder, iNode, i) {
        nAdded += expandOne(iNode, Abc_MinInt(Vi_Space(_data->pvFans + iNode), nFaninAddLimitAll - nAdded), nDist, nExpandableLevel, fVerbose);
        if (nAdded >= nFaninAddLimitAll)
            break;
    }
    assert(nAdded <= nFaninAddLimitAll);
    verifyRefs();
    return dupDfs();
}

// perform shared logic extraction
Miaig Miaig::share(int nNewNodesMax) {
    Miaig pCopy = dup(0);
    int nNewNodes = pCopy.findShared(nNewNodesMax);
    if (nNewNodes == 0)
        return pCopy;
    // temporarily create "hidden" nodes for DFS duplicator
    pCopy.nObjs() -= nNewNodes;
    Miaig pNew = pCopy.dupDfs();
    pCopy.nObjs() += nNewNodes;
    return pNew;
}

Miaig Miaig::reduce(int fVerbose) {
    int i, iNode;
    vi *vOrder = topoCollect();

    initializeTruth();
    initializeRefs();
    initializeLevels();
    // works best for final
    Vi_ForEachEntry(vOrder, iNode, i)
        reduceOne(iNode, 0, 0, 1, fVerbose);
    verifyRefs();
    return dupStrash(1, 1, 1);
}

Miaig Miaig::expandThenReduce(int nFaninAddLimit, int nDist, int nExpandableLevel, int fVerbose) {
    Miaig pTemp;
    int i, iNode;
    vi *vOrder = topoCollect();

    initializeTruth();
    initializeRefs();
    initializeLevels();
    if (nDist) initializeDists();
    Vi_ForEachEntry(vOrder, iNode, i) {
        expandThenReduceOne(iNode, nFaninAddLimit, nDist, nExpandableLevel, fVerbose);
    }
    verifyRefs();
    return dupDfs().dupStrash(1, 1, 1);
}

Miaig Miaig::expandShareReduce(int nFaninAddLimitAll, int nDivs, int nDist, int nExpandableLevel, int nVerbose) {
    // expand
    Miaig pNew = expand(nFaninAddLimitAll, nDist, nExpandableLevel, nVerbose);
    // share
    pNew = pNew.share(nDivs == -1 ? pNew.nObjs() : nDivs);
    // reduce
    pNew = pNew.reduce(nVerbose);
    return pNew;
}

void randomAddBest(std::vector<Miaig> &pBests, Miaig pNew, int nBestSave) {
    if (pBests.size() < nBestSave) {
        pBests.push_back(pNew);
    } else {
        int iNum = Random_Num(0) % nBestSave;
        pBests[iNum] = pNew;
    }
}

Miaig randomRead(std::vector<Miaig> &pBests) {
    return pBests[Random_Num(0) % pBests.size()];
}

Miaig Miaig::rewire(int nIters, float levelGrowRatio, int nExpands, int nGrowth, int nDivs, int nFaninMax, int nTimeOut, int nMode, int nMappedMode, int nDist, int nVerbose) {
    const int nRootSave = 8;
    const int nBestSave = 4;
    int nRestart = 5000;
    std::vector<Miaig> pRoots = {this->dup(0)};
    std::vector<Miaig> pBests = {this->dup(0)};
    iword clkStart = Time_Clock();
    Miaig pNew;
    Miaig pRoot = pRoots[0];
    Miaig pBest = this->dup(0);
    float (Miaig::*Miaig_ObjectiveFunction)(int, int) = (nMode == 0) ? &Miaig::countAnd2 : &Miaig::countTransistors;
    int maxLevel = levelGrowRatio != 0 ? this->countLevel() * levelGrowRatio : 0;
    int nExpandableLevel = maxLevel ? maxLevel - this->countLevel() : 0;

    float PrevBest = ((&pBest)->*Miaig_ObjectiveFunction)(1, nMappedMode);
    int iterNotImproveAfterRestart = 0;
    if (nVerbose && maxLevel) printf("Max level         : %5d\n", maxLevel);
    if (nVerbose) printf("Initial target    : %5g (AND2 = %5g Level = %3d)\n", PrevBest, this->countAnd2(1), this->countLevel());
    for (int i = 0; nIters ? i < nIters : 1; i++) {
        if (nVerbose) printf("\rIteration %7d : %5g -> ", i + 1, ((&pRoot)->*Miaig_ObjectiveFunction)(0, nMappedMode));
        if (nTimeOut && nTimeOut < 1.0 * (Time_Clock() - clkStart) / CLOCKS_PER_SEC) break;
        pNew = pRoot.dupMulti(nFaninMax, nGrowth);

        if (i % 2 == 0) {
            pNew = pNew.expandThenReduce(nGrowth, nDist, nExpandableLevel, nVerbose > 1);
        }
        pNew = pNew.expandShareReduce(nExpands, nDivs, nDist, nExpandableLevel, nVerbose > 1);

        ++iterNotImproveAfterRestart;
        // report
        float rootTarget = ((&pRoot)->*Miaig_ObjectiveFunction)(0, nMappedMode);
        float newTarget = ((&pNew)->*Miaig_ObjectiveFunction)(1, nMappedMode);
        if (maxLevel ? pNew.countLevel() > maxLevel : 0) {
        } else if (PrevBest > newTarget) {
            if (nVerbose) printf("%5g (AND2 = %5g Level = %3d) ", newTarget, pNew.countAnd2(), pNew.countLevel());
            if (nVerbose) Time_PrintEndl("Elapsed time", Time_Clock() - clkStart);
            PrevBest = newTarget;
            pBests = {pNew.dup(0), pNew.dup(0)};
            pBest = pNew.dup(0, 1);
            iterNotImproveAfterRestart = 0;
        } else if (PrevBest == newTarget) {
            randomAddBest(pBests, pNew.dup(0), nBestSave);
        }
        // compare
        if (maxLevel ? pNew.countLevel() > maxLevel : 0) {
        } else if (rootTarget < newTarget) {
            if (iterNotImproveAfterRestart > nRestart) {
                pNew = randomRead(pBests).dupMulti(nFaninMax, nGrowth);
                pNew = pNew.expand(nExpands, nDist, nExpandableLevel, nVerbose > 1);
                pNew = pNew.share(nDivs == -1 ? pNew.nObjs() : nDivs);
                pNew = pNew.dupStrash(1, 1, 0);
                pRoots = {pNew};
                iterNotImproveAfterRestart = 0;
            } else if (rootTarget * 1.05 > newTarget) {
                pRoots = {pNew};
            }
        } else if (rootTarget == newTarget) {
            randomAddBest(pRoots, pNew, nRootSave);
        } else {
            pRoots = {pNew};
        }
        pRoot = randomRead(pRoots);
    }
    if (nVerbose) Time_PrintEndl("Total solving time", Time_Clock() - clkStart);
    return pBest;
}

} // namespace Rewire

#ifdef RW_ABC
ABC_NAMESPACE_IMPL_END
#endif // RW_ABC
