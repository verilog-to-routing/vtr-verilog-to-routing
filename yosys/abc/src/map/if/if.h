/**CFile****************************************************************

  FileName    [if.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [FPGA mapping based on priority cuts.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 21, 2006.]

  Revision    [$Id: if.h,v 1.00 2006/11/21 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__map__if__if_h
#define ABC__map__if__if_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "misc/vec/vec.h"
#include "misc/mem/mem.h"
#include "misc/tim/tim.h"
#include "misc/util/utilNam.h"
#include "misc/vec/vecMem.h"
#include "misc/util/utilTruth.h"
#include "opt/dau/dau.h"
#include "misc/vec/vecHash.h"
#include "misc/vec/vecWec.h"
#include "map/if/acd/ac_wrapper.h"

ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////
 
// the maximum size of LUTs used for mapping (should be the same as FPGA_MAX_LUTSIZE defined in "fpga.h"!!!)
#define IF_MAX_LUTSIZE       32
// the largest possible number of LUT inputs when funtionality of the LUTs are computed
#define IF_MAX_FUNC_LUTSIZE  15
// a very large number
#define IF_INFINITY          100000000  
// the largest possible user cut cost
#define IF_COST_MAX          4095 // ((1<<12)-1)

#define IF_BIG_CHAR ((char)120)

// object types
typedef enum { 
    IF_NONE,     // 0: non-existent object
    IF_CONST1,   // 1: constant 1 
    IF_CI,       // 2: combinational input
    IF_CO,       // 3: combinational output
    IF_AND,      // 4: AND node
    IF_VOID      // 5: unused object
} If_Type_t;

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct If_Man_t_     If_Man_t;
typedef struct If_Par_t_     If_Par_t;
typedef struct If_Obj_t_     If_Obj_t;
typedef struct If_Cut_t_     If_Cut_t;
typedef struct If_Set_t_     If_Set_t;
typedef struct If_LibLut_t_  If_LibLut_t;
typedef struct If_LibBox_t_  If_LibBox_t;
typedef struct If_DsdMan_t_  If_DsdMan_t;
typedef struct Ifn_Ntk_t_    Ifn_Ntk_t;

typedef struct Ifif_Par_t_   Ifif_Par_t;
struct Ifif_Par_t_
{
    int                nLutSize;      // the LUT size
    If_LibLut_t *      pLutLib;       // the LUT library
    float              pLutDelays[IF_MAX_LUTSIZE];  // pin-to-pin delays of the max LUT
    float              DelayWire;     // wire delay
    int                nDegree;       // structure degree 
    int                fCascade;      // cascade
    int                fVerbose;      // verbose
    int                fVeryVerbose;  // verbose
};

// parameters
struct If_Par_t_
{
    // user-controlable parameters
    int                nLutSize;      // the LUT size
    int                nCutsMax;      // the max number of cuts
    int                nFlowIters;    // the number of iterations of area recovery
    int                nAreaIters;    // the number of iterations of area recovery
    int                nGateSize;     // the max size of the AND/OR gate to map into
    int                nNonDecLimit;  // the max size of non-dec nodes
    float              DelayTarget;   // delay target
    float              Epsilon;       // value used in comparison floating point numbers
    int                nRelaxRatio;   // delay relaxation ratio
    int                nStructType;   // type of the structure
    int                nAndDelay;     // delay of AND-gate in LUT library units
    int                nAndArea;      // area of AND-gate in LUT library units
    int                nLutDecSize;   // the LUT size for decomposition
    int                fPreprocess;   // preprossing
    int                fArea;         // area-oriented mapping
    int                fFancy;        // a fancy feature
    int                fExpRed;       // expand/reduce of the best cuts
    int                fLatchPaths;   // reset timing on latch paths
    int                fLut6Filter;   // uses filtering of 6-LUT functions
    int                fEdge;         // uses edge-based cut selection heuristics
    int                fPower;        // uses power-aware cut selection heuristics
    int                fCutMin;       // performs cut minimization by removing functionally reducdant variables
    int                fDelayOpt;     // special delay optimization
    int                fDelayOptLut;  // delay optimization for LUTs
    int                fDsdBalance;   // special delay optimization
    int                fUserRecLib;   // use recorded library
    int                fUserSesLib;   // use SAT-based synthesis
    int                fBidec;        // use bi-decomposition
    int                fUse34Spec;    // use specialized matching
    int                fUseBat;       // use one specialized feature
    int                fUseBuffs;     // use buffers to decouple outputs
    int                fEnableCheck07;// enable additional checking
    int                fEnableCheck08;// enable additional checking
    int                fEnableCheck10;// enable additional checking
    int                fEnableCheck75;// enable additional checking
    int                fEnableCheck75u;// enable additional checking
    int                fUseDsd;       // compute DSD of the cut functions
    int                fUseDsdTune;   // use matching based on precomputed manager
    int                fUseCofVars;   // use cofactoring variables
    int                fUseAndVars;   // use bi-decomposition
    int                fUseTtPerm;    // compute truth tables of the cut functions
    int                fUseCheck1;    // compute truth tables of the cut functions
    int                fUseCheck2;    // compute truth tables of the cut functions
    int                fDeriveLuts;   // enables deriving LUT structures
    int                fDoAverage;    // optimize average rather than maximum level
    int                fHashMapping;  // perform AIG hashing after mapping
    int                fUserLutDec;   // perform Boolean decomposition during mapping
    int                fUserLut2D;    // perform Boolean decomposition during mapping
    int                fVerbose;      // the verbosity flag
    int                fVerboseTrace; // the verbosity flag
    char *             pLutStruct;    // LUT structure
    int                fEnableStructN;// LUT structure using a new method
    float              WireDelay;     // wire delay
    // internal parameters
    int                fSkipCutFilter;// skip cut filter
    int                fAreaOnly;     // area only mode
    int                fTruth;        // truth table computation enabled
    int                fUsePerm;      // use permutation (delay info)
    int                fUseBdds;      // use local BDDs as a cost function
    int                fUseSops;      // use local SOPs as a cost function
    int                fUseCnfs;      // use local CNFs as a cost function
    int                fUseMv;        // use local MV-SOPs as a cost function
    int                nLatchesCi;    // the number of latches among the CIs
    int                nLatchesCo;    // the number of latches among the COs
    int                nLatchesCiBox; // the number of white box outputs among the CIs
    int                nLatchesCoBox; // the number of white box inputs among the COs
    int                fLiftLeaves;   // shift the leaves for seq mapping
    int                fUseCoAttrs;   // use CO attributes
    float              DelayTargetNew;// new delay target
    float              FinalDelay;    // final delay after mapping
    float              FinalArea;     // final area after mapping
    If_LibLut_t *      pLutLib;       // the LUT library
    float *            pTimesArr;     // arrival times
    float *            pTimesReq;     // required times
    int (* pFuncCost)  (If_Man_t *, If_Cut_t *);  // procedure to compute the user's cost of a cut
    int (* pFuncUser)  (If_Man_t *, If_Obj_t *, If_Cut_t *);            //  procedure called for each cut when cut computation is finished
    int (* pFuncCell)  (If_Man_t *, unsigned *, int, int, char *);      //  procedure called for cut functions
    int (* pFuncCell2) (If_Man_t *, word *, int, Vec_Str_t *, char **); //  procedure called for cut functions
    int (* pFuncWrite) (If_Man_t *);                                    //  procedure called for cut functions
    void *             pReoMan;       // reordering manager
};

// the LUT library
struct If_LibLut_t_
{
    char *             pName;         // the name of the LUT library
    int                LutMax;        // the maximum LUT size 
    int                fVarPinDelays; // set to 1 if variable pin delays are specified
    float              pLutAreas[IF_MAX_LUTSIZE+1]; // the areas of LUTs
    float              pLutDelays[IF_MAX_LUTSIZE+1][IF_MAX_LUTSIZE+1];// the delays of LUTs
};

// manager
struct If_Man_t_
{
    char *             pName;
    // mapping parameters
    If_Par_t *         pPars;
    // mapping nodes
    If_Obj_t *         pConst1;       // the constant 1 node
    Vec_Ptr_t *        vCis;          // the primary inputs
    Vec_Ptr_t *        vCos;          // the primary outputs
    Vec_Ptr_t *        vObjs;         // all objects
    Vec_Ptr_t *        vObjsRev;      // reverse topological order of objects
    Vec_Ptr_t *        vTemp;         // temporary array
    int                nObjs[IF_VOID];// the number of objects by type
    // various data
    int                nLevelMax;     // the max number of AIG levels
    float              fEpsilon;      // epsilon used for comparison
    float              RequiredGlo;   // global required times
    float              RequiredGlo2;  // global required times
    float              AreaGlo;       // global area
    int                nNets;         // the sum total of fanins of all LUTs in the mapping
    float              dPower;        // the sum total of switching activities of all LUTs in the mapping
    int                nCutsUsed;     // the number of cuts currently used
    int                nCutsMerged;   // the total number of cuts merged
    unsigned *         puTemp[4];     // used for the truth table computation
    word *             puTempW;       // used for the truth table computation
    int                SortMode;      // one of the three sorting modes
    int                fNextRound;    // set to 1 after the first round
    int                nChoices;      // the number of choice nodes
    Vec_Int_t *        vSwitching;    // switching activity of each node
    int                pPerm[3][IF_MAX_LUTSIZE]; // permutations
    unsigned           uSharedMask;   // mask of shared variables
    int                nShared;       // the number of shared variables
    int                fReqTimeWarn;  // warning about exceeding required times was printed
    // SOP balancing
    Vec_Int_t *        vCover;        // used to compute ISOP
    Vec_Int_t *        vArray;        // intermediate storage
    Vec_Wrd_t *        vAnds;         // intermediate storage
    Vec_Wrd_t *        vOrGate;       // intermediate storage
    Vec_Wrd_t *        vAndGate;      // intermediate storage
    // sequential mapping
    Vec_Ptr_t *        vLatchOrder;   // topological ordering of latches
    Vec_Int_t *        vLags;         // sequentail lags of all nodes
    int                nAttempts;     // the number of attempts in binary search
    int                nMaxIters;     // the maximum number of iterations
    int                Period;        // the current value of the clock period (for seq mapping)
    // memory management
    int                nTruth6Words[IF_MAX_FUNC_LUTSIZE+1];  // the size of the truth table if allocated
    int                nPermWords;    // the size of the permutation array (in words)
    int                nObjBytes;     // the size of the object
    int                nCutBytes;     // the size of the cut
    int                nSetBytes;     // the size of the cut set
    Mem_Fixed_t *      pMemObj;       // memory manager for objects (entrysize = nEntrySize)
    Mem_Fixed_t *      pMemSet;       // memory manager for sets of cuts (entrysize = nCutSize*(nCutsMax+1))
    If_Set_t *         pMemCi;        // memory for CI cutsets
    If_Set_t *         pMemAnd;       // memory for AND cutsets
    If_Set_t *         pFreeList;     // the list of free cutsets
    int                nSmallSupp;    // the small support
    int                nCutsTotal;
    int                nCutsUseless[32];
    int                nCutsCount[32];
    int                nCutsCountAll;
    int                nCutsUselessAll;
    int                nCuts5, nCuts5a;
    If_DsdMan_t *      pIfDsdMan;     // DSD manager
    Vec_Mem_t *        vTtMem[IF_MAX_FUNC_LUTSIZE+1];   // truth table memory and hash table
    Vec_Wec_t *        vTtIsops[IF_MAX_FUNC_LUTSIZE+1]; // mapping of truth table into DSD
    Vec_Int_t *        vTtDsds[IF_MAX_FUNC_LUTSIZE+1];  // mapping of truth table into DSD
    Vec_Str_t *        vTtPerms[IF_MAX_FUNC_LUTSIZE+1]; // mapping of truth table into permutations
    Vec_Str_t *        vTtVars[IF_MAX_FUNC_LUTSIZE+1];  // mapping of truth table into selected vars
    Vec_Int_t *        vTtDecs[IF_MAX_FUNC_LUTSIZE+1];  // mapping of truth table into decomposition pattern
    Vec_Int_t *        vTtOccurs[IF_MAX_FUNC_LUTSIZE+1];// truth table occurange counters
    Hash_IntMan_t *    vPairHash;     // hashing pairs of truth tables
    Vec_Int_t *        vPairRes;      // resulting truth table
    Vec_Str_t *        vPairPerms;    // resulting permutation
    Vec_Mem_t *        vTtMem6;
    char               pCanonPerm[IF_MAX_LUTSIZE];
    unsigned           uCanonPhase;
    int                nCacheHits;
    int                nCacheMisses;
    abctime            timeCache[6];
    int                nBestCutSmall[2];
    int                nCountNonDec[2];
    Vec_Int_t *        vCutData;      // cut data storage
    int                pArrTimeProfile[IF_MAX_FUNC_LUTSIZE];
    Vec_Ptr_t *        vVisited;
    void *             pUserMan;
    Vec_Int_t *        vDump;
    int                pDumpIns[16];
    Vec_Str_t *        vMarks;
    Vec_Int_t *        vVisited2;

    // timing manager
    Tim_Man_t *        pManTim;
    Vec_Int_t *        vCoAttrs;      // CO attributes   0=optimize; 1=keep; 2=relax
    // hash table for functions
    int                nTableSize[2];    // hash table size
    int                nTableEntries[2]; // hash table entries
    void **            pHashTable[2];    // hash table bins
    Mem_Fixed_t *      pMemEntries;      // memory manager for hash table entries
    // statistics 
//    abctime                timeTruth;
};

// priority cut
struct If_Cut_t_
{
    float              Area;          // area (or area-flow) of the cut
    float              Edge;          // the edge flow
    float              Power;         // the power flow
    float              Delay;         // delay of the cut
    int                iCutFunc;      // TT ID of the cut
    int                uMaskFunc;     // polarity bitmask
    unsigned           uSign;         // cut signature
    unsigned           Cost    : 12;  // the user's cost of the cut (related to IF_COST_MAX)
    unsigned           fCompl  :  1;  // the complemented attribute 
    unsigned           fUser   :  1;  // using the user's area and delay
    unsigned           fUseless:  1;  // cannot be used in the mapping
    unsigned           fAndCut :  1;  // matched with AND gate
    unsigned           nLimit  :  8;  // the maximum number of leaves
    unsigned           nLeaves :  8;  // the number of leaves
    unsigned           decDelay: 16;  // pin-to-pin decomposition delay
    int                pLeaves[0];
};

// set of priority cut
struct If_Set_t_
{
    short              nCutsMax;      // the max number of cuts
    short              nCuts;         // the current number of cuts
    If_Set_t *         pNext;         // next cutset in the free list
    If_Cut_t **        ppCuts;        // the array of pointers to the cuts
};

// node extension
struct If_Obj_t_
{
    unsigned           Type    :  4;  // object
    unsigned           fCompl0 :  1;  // complemented attribute
    unsigned           fCompl1 :  1;  // complemented attribute
    unsigned           fPhase  :  1;  // phase of the node
    unsigned           fRepr   :  1;  // representative of the equivalence class
    unsigned           fMark   :  1;  // multipurpose mark
    unsigned           fVisit  :  1;  // multipurpose mark
    unsigned           fSpec   :  1;  // multipurpose mark
    unsigned           fDriver :  1;  // multipurpose mark
    unsigned           fSkipCut:  1;  // multipurpose mark
    unsigned           Level   : 19;  // logic level of the node
    int                Id;            // integer ID
    int                IdPio;         // integer ID of PIs/POs
    int                nRefs;         // the number of references
    int                nVisits;       // the number of visits to this node
    int                nVisitsCopy;   // the number of visits to this node
    If_Obj_t *         pFanin0;       // the first fanin 
    If_Obj_t *         pFanin1;       // the second fanin
    If_Obj_t *         pEquiv;        // the choice node
    float              EstRefs;       // estimated reference counter
    float              Required;      // required time of the onde
    float              LValue;        // sequential arrival time of the node
    union{
    void *             pCopy;         // used for object duplication
    int                iCopy;
    };
    
    If_Set_t *         pCutSet;       // the pointer to the cutset
    If_Cut_t           CutBest;       // the best cut selected 
};

typedef struct If_Box_t_ If_Box_t;
struct If_Box_t_
{
    char *             pName;
    char               fSeq;
    char               fBlack;
    char               fOuter;
    char               fUnused;
    int                Id;
    int                nPis;
    int                nPos;
    int *              pDelays;
};

struct If_LibBox_t_
{
    int                nBoxes;
    Vec_Ptr_t *        vBoxes;
};

static inline If_Obj_t * If_Regular( If_Obj_t * p )                          { return (If_Obj_t *)((ABC_PTRUINT_T)(p) & ~01);  }
static inline If_Obj_t * If_Not( If_Obj_t * p )                              { return (If_Obj_t *)((ABC_PTRUINT_T)(p) ^  01);  }
static inline If_Obj_t * If_NotCond( If_Obj_t * p, int c )                   { return (If_Obj_t *)((ABC_PTRUINT_T)(p) ^ (c));  }
static inline int        If_IsComplement( If_Obj_t * p )                     { return (int )(((ABC_PTRUINT_T)p) & 01);         }

static inline int        If_ManCiNum( If_Man_t * p )                         { return p->nObjs[IF_CI];               }
static inline int        If_ManCoNum( If_Man_t * p )                         { return p->nObjs[IF_CO];               }
static inline int        If_ManAndNum( If_Man_t * p )                        { return p->nObjs[IF_AND];              }
static inline int        If_ManObjNum( If_Man_t * p )                        { return Vec_PtrSize(p->vObjs);         }

static inline If_Obj_t * If_ManConst1( If_Man_t * p )                        { return p->pConst1;                              }
static inline If_Obj_t * If_ManCi( If_Man_t * p, int i )                     { return (If_Obj_t *)Vec_PtrEntry( p->vCis, i );  }
static inline If_Obj_t * If_ManCo( If_Man_t * p, int i )                     { return (If_Obj_t *)Vec_PtrEntry( p->vCos, i );  }
static inline If_Obj_t * If_ManLi( If_Man_t * p, int i )                     { return (If_Obj_t *)Vec_PtrEntry( p->vCos, If_ManCoNum(p) - p->pPars->nLatchesCo + i );  }
static inline If_Obj_t * If_ManLo( If_Man_t * p, int i )                     { return (If_Obj_t *)Vec_PtrEntry( p->vCis, If_ManCiNum(p) - p->pPars->nLatchesCi + i );  }
static inline If_Obj_t * If_ManObj( If_Man_t * p, int i )                    { return (If_Obj_t *)Vec_PtrEntry( p->vObjs, i ); }

static inline int        If_ObjIsConst1( If_Obj_t * pObj )                   { return pObj->Type == IF_CONST1;       }
static inline int        If_ObjIsCi( If_Obj_t * pObj )                       { return pObj->Type == IF_CI;           }
static inline int        If_ObjIsCo( If_Obj_t * pObj )                       { return pObj->Type == IF_CO;           }
static inline int        If_ObjIsTerm( If_Obj_t * pObj )                     { return pObj->Type == IF_CI || pObj->Type == IF_CO; }
static inline int        If_ObjIsLatch( If_Obj_t * pObj )                    { return If_ObjIsCi(pObj) && pObj->pFanin0 != NULL;  }
static inline int        If_ObjIsAnd( If_Obj_t * pObj )                      { return pObj->Type == IF_AND;          }

static inline int        If_ObjId( If_Obj_t * pObj )                         { return pObj->Id;                      }
static inline If_Obj_t * If_ObjFanin0( If_Obj_t * pObj )                     { return pObj->pFanin0;                 }
static inline If_Obj_t * If_ObjFanin1( If_Obj_t * pObj )                     { return pObj->pFanin1;                 }
static inline int        If_ObjFaninC0( If_Obj_t * pObj )                    { return pObj->fCompl0;                 }
static inline int        If_ObjFaninC1( If_Obj_t * pObj )                    { return pObj->fCompl1;                 }
static inline void *     If_ObjCopy( If_Obj_t * pObj )                       { return pObj->pCopy;                   }
static inline int        If_ObjLevel( If_Obj_t * pObj )                      { return pObj->Level;                   }
static inline void       If_ObjSetLevel( If_Obj_t * pObj, int Level )        { pObj->Level = Level;                  }
static inline void       If_ObjSetCopy( If_Obj_t * pObj, void * pCopy )      { pObj->pCopy = pCopy;                  }
static inline void       If_ObjSetChoice( If_Obj_t * pObj, If_Obj_t * pEqu ) { pObj->pEquiv = pEqu;                  }

static inline int        If_CutLeaveNum( If_Cut_t * pCut )                   { return pCut->nLeaves;                             }
static inline int *      If_CutLeaves( If_Cut_t * pCut )                     { return pCut->pLeaves;                             }
static inline If_Obj_t * If_CutLeaf( If_Man_t * p, If_Cut_t * pCut, int i )  { assert(i >= 0 && i < (int)pCut->nLeaves); return If_ManObj(p, pCut->pLeaves[i]);                         }
static inline unsigned   If_CutSuppMask( If_Cut_t * pCut )                   { return (~(unsigned)0) >> (32-pCut->nLeaves);      }
static inline int        If_CutTruthWords( int nVarsMax )                    { return nVarsMax <= 5 ? 2 : (1 << (nVarsMax - 5)); }
static inline int        If_CutPermWords( int nVarsMax )                     { return nVarsMax / sizeof(int) + ((nVarsMax % sizeof(int)) > 0); }
static inline int        If_CutLeafBit( If_Cut_t * pCut, int i )             { return (pCut->uMaskFunc >> i) & 1;                }
static inline char *     If_CutPerm( If_Cut_t * pCut )                       { return (char *)(pCut->pLeaves + pCut->nLeaves);   }
static inline void       If_CutCopy( If_Man_t * p, If_Cut_t * pDst, If_Cut_t * pSrc ) { memcpy( pDst, pSrc, (size_t)p->nCutBytes );      }
static inline void       If_CutSetup( If_Man_t * p, If_Cut_t * pCut        ) { memset(pCut, 0, (size_t)p->nCutBytes); pCut->nLimit = p->pPars->nLutSize; }

static inline If_Cut_t * If_ObjCutBest( If_Obj_t * pObj )                    { return &pObj->CutBest;                }
static inline unsigned   If_ObjCutSign( unsigned ObjId )                     { return (1 << (ObjId % 31));           }
static inline unsigned   If_ObjCutSignCompute( If_Cut_t * p )                { unsigned s = 0; int i; for ( i = 0; i < If_CutLeaveNum(p); i++ ) s |= If_ObjCutSign(p->pLeaves[i]); return s; }

static inline float      If_ObjArrTime( If_Obj_t * pObj )                    { return If_ObjCutBest(pObj)->Delay;    }
static inline void       If_ObjSetArrTime( If_Obj_t * pObj, float ArrTime )  { If_ObjCutBest(pObj)->Delay = ArrTime; }

static inline float      If_ObjLValue( If_Obj_t * pObj )                     { return pObj->LValue;                  }
static inline void       If_ObjSetLValue( If_Obj_t * pObj, float LValue )    { pObj->LValue = LValue;                }

static inline void *     If_CutData( If_Cut_t * pCut )                       { return *(void **)pCut;                }
static inline void       If_CutSetData( If_Cut_t * pCut, void * pData )      { *(void **)pCut = pData;               }

static inline int        If_CutDataInt( If_Cut_t * pCut )                    { return *(int *)pCut;                  }
static inline void       If_CutSetDataInt( If_Cut_t * pCut, int Data )       { *(int *)pCut = Data;                  }

static inline int        If_CutTruthLit( If_Cut_t * pCut )                   { assert( pCut->iCutFunc >= 0 ); return pCut->iCutFunc;             }
static inline int        If_CutTruthIsCompl( If_Cut_t * pCut )               { assert( pCut->iCutFunc >= 0 ); return Abc_LitIsCompl(pCut->iCutFunc);                               }
static inline word *     If_CutTruthWR( If_Man_t * p, If_Cut_t * pCut )      { return p->vTtMem[pCut->nLeaves] ? Vec_MemReadEntry(p->vTtMem[pCut->nLeaves], Abc_Lit2Var(pCut->iCutFunc)) : NULL;  }
static inline unsigned * If_CutTruthUR( If_Man_t * p, If_Cut_t * pCut)       { return (unsigned *)If_CutTruthWR(p, pCut);                        }
static inline word *     If_CutTruthW( If_Man_t * p, If_Cut_t * pCut )       { assert( pCut->iCutFunc >= 0 ); Abc_TtCopy( p->puTempW, If_CutTruthWR(p, pCut), p->nTruth6Words[pCut->nLeaves], If_CutTruthIsCompl(pCut) ); return p->puTempW;  }
static inline unsigned * If_CutTruth( If_Man_t * p, If_Cut_t * pCut )        { return (unsigned *)If_CutTruthW(p, pCut);                         }

static inline int        If_CutDsdLit( If_Man_t * p, If_Cut_t * pCut )       { return Abc_Lit2LitL( Vec_IntArray(p->vTtDsds[pCut->nLeaves]), If_CutTruthLit(pCut) );               }
static inline int        If_CutDsdIsCompl( If_Man_t * p, If_Cut_t * pCut )   { return Abc_LitIsCompl( If_CutDsdLit(p, pCut) );                                                     }
static inline char *     If_CutDsdPerm( If_Man_t * p, If_Cut_t * pCut )      { return Vec_StrEntryP( p->vTtPerms[pCut->nLeaves], Abc_Lit2Var(pCut->iCutFunc) * Abc_MaxInt(6, pCut->nLeaves) );           }

static inline float      If_CutLutArea( If_Man_t * p, If_Cut_t * pCut )      { return pCut->fAndCut ? p->pPars->nAndArea : (pCut->fUser? (float)pCut->Cost : (p->pPars->pLutLib? p->pPars->pLutLib->pLutAreas[pCut->nLeaves] : (float)1.0));  }
static inline float      If_CutLutDelay( If_LibLut_t * p, int Size, int iPin )  { return p ? (p->fVarPinDelays ? p->pLutDelays[Size][iPin] : p->pLutDelays[Size][0]) : 1.0;                              }


////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

#define IF_MIN(a,b)      (((a) < (b))? (a) : (b))
#define IF_MAX(a,b)      (((a) > (b))? (a) : (b))

// the small and large numbers (min/max float are 1.17e-38/3.40e+38)
#define IF_FLOAT_LARGE   ((float)1.0e+20)
#define IF_FLOAT_SMALL   ((float)1.0e-20)
#define IF_INT_LARGE     (10000000)

// iterator over the primary inputs
#define If_ManForEachCi( p, pObj, i )                                          \
    Vec_PtrForEachEntry( If_Obj_t *, p->vCis, pObj, i )
// iterator over the primary outputs
#define If_ManForEachCo( p, pObj, i )                                          \
    Vec_PtrForEachEntry( If_Obj_t *, p->vCos, pObj, i )
// iterator over the primary inputs
#define If_ManForEachPi( p, pObj, i )                                          \
    Vec_PtrForEachEntryStop( If_Obj_t *, p->vCis, pObj, i, If_ManCiNum(p) - p->pPars->nLatchesCi - p->pPars->nLatchesCiBox )
// iterator over the primary outputs
#define If_ManForEachPo( p, pObj, i )                                          \
    Vec_PtrForEachEntryStartStop( If_Obj_t *, p->vCos, pObj, i, p->pPars->nLatchesCoBox, If_ManCoNum(p) - p->pPars->nLatchesCo )
// iterator over the latches 
#define If_ManForEachLatchInput( p, pObj, i )                                  \
    Vec_PtrForEachEntryStart( If_Obj_t *, p->vCos, pObj, i, If_ManCoNum(p) - p->pPars->nLatchesCo )
#define If_ManForEachLatchOutput( p, pObj, i )                                 \
    Vec_PtrForEachEntryStartStop( If_Obj_t *, p->vCis, pObj, i, If_ManCiNum(p) - p->pPars->nLatchesCi - p->pPars->nLatchesCiBox, If_ManCiNum(p) - p->pPars->nLatchesCiBox )
// iterator over all objects in topological order
#define If_ManForEachObj( p, pObj, i )                                         \
    Vec_PtrForEachEntry( If_Obj_t *, p->vObjs, pObj, i )
// iterator over all objects in reverse topological order
#define If_ManForEachObjReverse( p, pObj, i )                                  \
    Vec_PtrForEachEntry( If_Obj_t *, p->vObjsRev, pObj, i )
// iterator over logic nodes 
#define If_ManForEachNode( p, pObj, i )                                        \
    If_ManForEachObj( p, pObj, i ) if ( pObj->Type != IF_AND ) {} else
// iterator over cuts of the node
#define If_ObjForEachCut( pObj, pCut, i )                                      \
    for ( i = 0; (i < (pObj)->pCutSet->nCuts) && ((pCut) = (pObj)->pCutSet->ppCuts[i]); i++ )
// iterator over the leaves of the cut
#define If_CutForEachLeaf( p, pCut, pLeaf, i )                                 \
    for ( i = 0; (i < (int)(pCut)->nLeaves) && ((pLeaf) = If_ManObj(p, (pCut)->pLeaves[i])); i++ )
#define If_CutForEachLeafReverse( p, pCut, pLeaf, i )                                 \
    for ( i = (int)(pCut)->nLeaves - 1; (i >= 0) && ((pLeaf) = If_ManObj(p, (pCut)->pLeaves[i])); i-- )
//#define If_CutForEachLeaf( p, pCut, pLeaf, i )                                 \ \\prevent multiline comment
//    for ( i = 0; (i < (int)(pCut)->nLeaves) && ((pLeaf) = If_ManObj(p, p->pPars->fLiftLeaves? (pCut)->pLeaves[i] >> 8 : (pCut)->pLeaves[i])); i++ )
// iterator over the leaves of the sequential cut
#define If_CutForEachLeafSeq( p, pCut, pLeaf, Shift, i )                       \
    for ( i = 0; (i < (int)(pCut)->nLeaves) && ((pLeaf) = If_ManObj(p, (pCut)->pLeaves[i] >> 8)) && (((Shift) = ((pCut)->pLeaves[i] & 255)) >= 0); i++ )

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== ifCore.c ===========================================================*/
extern void            If_ManSetDefaultPars( If_Par_t * pPars );
extern int             If_ManPerformMapping( If_Man_t * p );
extern int             If_ManPerformMappingComb( If_Man_t * p );
extern void            If_ManComputeSwitching( If_Man_t * p );
/*=== ifCut.c ============================================================*/
extern int             If_CutVerifyCuts( If_Set_t * pCutSet, int fOrdered );
extern int             If_CutFilter( If_Set_t * pCutSet, If_Cut_t * pCut, int fSaveCut0 );
extern void            If_CutSort( If_Man_t * p, If_Set_t * pCutSet, If_Cut_t * pCut );
extern void            If_CutOrder( If_Cut_t * pCut );
extern int             If_CutMergeOrdered( If_Man_t * p, If_Cut_t * pCut0, If_Cut_t * pCut1, If_Cut_t * pCut );
extern int             If_CutMerge( If_Man_t * p, If_Cut_t * pCut0, If_Cut_t * pCut1, If_Cut_t * pCut );
extern int             If_CutCheck( If_Cut_t * pCut );
extern void            If_CutPrint( If_Cut_t * pCut );
extern void            If_CutPrintTiming( If_Man_t * p, If_Cut_t * pCut );
extern void            If_CutLift( If_Cut_t * pCut );
extern void            If_CutCopy( If_Man_t * p, If_Cut_t * pCutDest, If_Cut_t * pCutSrc );
extern float           If_CutAreaFlow( If_Man_t * p, If_Cut_t * pCut );
extern float           If_CutEdgeFlow( If_Man_t * p, If_Cut_t * pCut );
extern float           If_CutPowerFlow( If_Man_t * p, If_Cut_t * pCut, If_Obj_t * pRoot );
extern float           If_CutAverageRefs( If_Man_t * p, If_Cut_t * pCut );
extern float           If_CutAreaDeref( If_Man_t * p, If_Cut_t * pCut );
extern float           If_CutAreaRef( If_Man_t * p, If_Cut_t * pCut );
extern float           If_CutAreaDerefed( If_Man_t * p, If_Cut_t * pCut );
extern float           If_CutAreaRefed( If_Man_t * p, If_Cut_t * pCut );
extern float           If_CutEdgeDeref( If_Man_t * p, If_Cut_t * pCut );
extern float           If_CutEdgeRef( If_Man_t * p, If_Cut_t * pCut );
extern float           If_CutEdgeDerefed( If_Man_t * p, If_Cut_t * pCut );
extern float           If_CutEdgeRefed( If_Man_t * p, If_Cut_t * pCut );
extern float           If_CutPowerDeref( If_Man_t * p, If_Cut_t * pCut, If_Obj_t * pRoot );
extern float           If_CutPowerRef( If_Man_t * p, If_Cut_t * pCut, If_Obj_t * pRoot );
extern float           If_CutPowerDerefed( If_Man_t * p, If_Cut_t * pCut, If_Obj_t * pRoot );
extern float           If_CutPowerRefed( If_Man_t * p, If_Cut_t * pCut, If_Obj_t * pRoot );
/*=== ifDec.c =============================================================*/
extern word            If_CutPerformDerive07( If_Man_t * p, unsigned * pTruth, int nVars, int nLeaves, char * pStr );
extern int             If_CutPerformCheck07( If_Man_t * p, unsigned * pTruth, int nVars, int nLeaves, char * pStr );
extern int             If_CutPerformCheck08( If_Man_t * p, unsigned * pTruth, int nVars, int nLeaves, char * pStr );
extern int             If_CutPerformCheck10( If_Man_t * p, unsigned * pTruth, int nVars, int nLeaves, char * pStr );
extern int             If_CutPerformCheck16( If_Man_t * p, unsigned * pTruth, int nVars, int nLeaves, char * pStr );
extern int             If_CutPerformCheckXX( If_Man_t * p, unsigned * pTruth, int nVars, int nLeaves, char * pStr );
extern int             If_CutPerformCheck45( If_Man_t * p, unsigned * pTruth, int nVars, int nLeaves, char * pStr );
extern int             If_CutPerformCheck54( If_Man_t * p, unsigned * pTruth, int nVars, int nLeaves, char * pStr );
extern int             If_CutPerformCheck75( If_Man_t * p, unsigned * pTruth, int nVars, int nLeaves, char * pStr );
extern float           If_CutDelayLutStruct( If_Man_t * p, If_Cut_t * pCut, char * pStr, float WireDelay );
// extern int             If_CutPerformAcd( If_Man_t * p, unsigned nVars, int lutSize, unsigned * pdelay, int use_late_arrival, unsigned * cost );
extern int             If_CluCheckExt( void * p, word * pTruth, int nVars, int nLutLeaf, int nLutRoot, 
                           char * pLut0, char * pLut1, word * pFunc0, word * pFunc1 );
extern int             If_CluCheckExt3( void * p, word * pTruth, int nVars, int nLutLeaf, int nLutLeaf2, int nLutRoot, 
                           char * pLut0, char * pLut1, char * pLut2, word * pFunc0, word * pFunc1, word * pFunc2 );
extern int             If_CluCheckXXExt( void * p, word * pTruth, int nVars, int nLutLeaf, int nLutRoot, 
                           char * pLut0, char * pLut1, word * pFunc0, word * pFunc1 );
extern int             If_MatchCheck1( If_Man_t * p, unsigned * pTruth, int nVars, int nLeaves, char * pStr );
extern int             If_MatchCheck2( If_Man_t * p, unsigned * pTruth, int nVars, int nLeaves, char * pStr );
/*=== ifDelay.c =============================================================*/
extern int             If_CutDelaySop( If_Man_t * p, If_Cut_t * pCut );
extern int             If_CutSopBalanceEvalInt( Vec_Int_t * vCover, int * pTimes, int * pFaninLits, Vec_Int_t * vAig, int * piRes, int nSuppAll, int * pArea );
extern int             If_CutSopBalanceEval( If_Man_t * p, If_Cut_t * pCut, Vec_Int_t * vAig );
extern int             If_CutSopBalancePinDelaysInt( Vec_Int_t * vCover, int * pTimes, word * pFaninRes, int nSuppAll, word * pRes );
extern int             If_CutSopBalancePinDelays( If_Man_t * p, If_Cut_t * pCut, char * pPerm );
extern int             If_CutLutBalanceEval( If_Man_t * p, If_Cut_t * pCut );
extern int             If_CutLutBalancePinDelays( If_Man_t * p, If_Cut_t * pCut, char * pPerm );
extern int             If_LutDecEval( If_Man_t * p, If_Cut_t * pCut, If_Obj_t * pObj, int optDelay, int fFirst );
extern int             If_Lut2DecEval( If_Man_t * p, If_Cut_t * pCut, If_Obj_t * pObj, int optDelay, int fFirst );
extern int             If_LutDecReEval( If_Man_t * p, If_Cut_t * pCut );
extern float           If_LutDecPinRequired( If_Man_t * p, If_Cut_t * pCut, int i, float required );
/*=== ifDsd.c =============================================================*/
extern If_DsdMan_t *   If_DsdManAlloc( int nVars, int nLutSize );
extern void            If_DsdManAllocIsops( If_DsdMan_t * p, int nLutSize );
extern void            If_DsdManPrint( If_DsdMan_t * p, char * pFileName, int Number, int Support, int fOccurs, int fTtDump, int fVerbose );
extern void            If_DsdManTune( If_DsdMan_t * p, int LutSize, int fFast, int fAdd, int fSpec, int fVerbose );
extern void            Id_DsdManTuneStr( If_DsdMan_t * p, char * pStruct, int nConfls, int nProcs, int nInputs, int fVerbose );
extern void            If_DsdManFree( If_DsdMan_t * p, int fVerbose );
extern void            If_DsdManSave( If_DsdMan_t * p, char * pFileName );
extern If_DsdMan_t *   If_DsdManLoad( char * pFileName );
extern void            If_DsdManMerge( If_DsdMan_t * p, If_DsdMan_t * pNew );
extern void            If_DsdManCleanOccur( If_DsdMan_t * p, int fVerbose );
extern void            If_DsdManCleanMarks( If_DsdMan_t * p, int fVerbose );
extern void            If_DsdManInvertMarks( If_DsdMan_t * p, int fVerbose );
extern If_DsdMan_t *   If_DsdManFilter( If_DsdMan_t * p, int Limit );
extern int             If_DsdManCompute( If_DsdMan_t * p, word * pTruth, int nLeaves, unsigned char * pPerm, char * pLutStruct );
extern char *          If_DsdManFileName( If_DsdMan_t * p );
extern int             If_DsdManVarNum( If_DsdMan_t * p );
extern int             If_DsdManObjNum( If_DsdMan_t * p );
extern int             If_DsdManLutSize( If_DsdMan_t * p );
extern int             If_DsdManTtBitNum( If_DsdMan_t * p );
extern int             If_DsdManPermBitNum( If_DsdMan_t * p );
extern void            If_DsdManSetLutSize( If_DsdMan_t * p, int nLutSize );
extern int             If_DsdManSuppSize( If_DsdMan_t * p, int iDsd );
extern int             If_DsdManCheckDec( If_DsdMan_t * p, int iDsd );
extern int             If_DsdManReadMark( If_DsdMan_t * p, int iDsd );
extern void            If_DsdManSetNewAsUseless( If_DsdMan_t * p );
extern word *          If_DsdManGetFuncConfig( If_DsdMan_t * p, int iDsd );
extern char *          If_DsdManGetCellStr( If_DsdMan_t * p );
extern unsigned        If_DsdManCheckXY( If_DsdMan_t * p, int iDsd, int LutSize, int fDerive, unsigned uMaskNot, int fHighEffort, int fVerbose );
extern int             If_CutDsdBalanceEval( If_Man_t * p, If_Cut_t * pCut, Vec_Int_t * vAig );
extern int             If_CutDsdBalancePinDelays( If_Man_t * p, If_Cut_t * pCut, char * pPerm );
extern void            Id_DsdManTuneThresh( If_DsdMan_t * p, int fUnate, int fThresh, int fThreshHeuristic, int fVerbose );
/*=== ifLib.c =============================================================*/
extern If_LibLut_t *   If_LibLutRead( char * FileName );
extern If_LibLut_t *   If_LibLutDup( If_LibLut_t * p );
extern void            If_LibLutFree( If_LibLut_t * pLutLib );
extern void            If_LibLutPrint( If_LibLut_t * pLutLib );
extern int             If_LibLutDelaysAreDiscrete( If_LibLut_t * pLutLib );
extern int             If_LibLutDelaysAreDifferent( If_LibLut_t * pLutLib );
extern If_LibLut_t *   If_LibLutSetSimple( int nLutSize );
extern float           If_LibLutFastestPinDelay( If_LibLut_t * p );
extern float           If_LibLutSlowestPinDelay( If_LibLut_t * p );
/*=== ifLibBox.c =============================================================*/
extern If_LibBox_t *   If_LibBoxStart();
extern void            If_LibBoxFree( If_LibBox_t * p );
extern int             If_LibBoxNum( If_LibBox_t * p );
extern If_Box_t *      If_LibBoxReadBox( If_LibBox_t * p, int Id );
extern If_Box_t *      If_LibBoxFindBox( If_LibBox_t * p, char * pName );
extern void            If_LibBoxAdd( If_LibBox_t * p, If_Box_t * pBox );
extern If_LibBox_t *   If_LibBoxRead( char * pFileName );
extern If_LibBox_t *   If_LibBoxRead2( char * pFileName );
extern void            If_LibBoxPrint( FILE * pFile, If_LibBox_t * p );
extern void            If_LibBoxWrite( char * pFileName, If_LibBox_t * p );
extern int             If_LibBoxLoad( char * pFileName );
extern If_Box_t *      If_BoxStart( char * pName, int Id, int nPis, int nPos, int fSeq, int fBlack, int fOuter );
/*=== ifMan.c =============================================================*/
extern If_Man_t *      If_ManStart( If_Par_t * pPars );
extern void            If_ManRestart( If_Man_t * p );
extern void            If_ManStop( If_Man_t * p );
extern If_Obj_t *      If_ManCreateCi( If_Man_t * p );
extern If_Obj_t *      If_ManCreateCo( If_Man_t * p, If_Obj_t * pDriver );
extern If_Obj_t *      If_ManCreateAnd( If_Man_t * p, If_Obj_t * pFan0, If_Obj_t * pFan1 );
extern If_Obj_t *      If_ManCreateXor( If_Man_t * p, If_Obj_t * pFan0, If_Obj_t * pFan1 );
extern If_Obj_t *      If_ManCreateMux( If_Man_t * p, If_Obj_t * pFan0, If_Obj_t * pFan1, If_Obj_t * pCtrl );
extern void            If_ManCreateChoice( If_Man_t * p, If_Obj_t * pRepr );
extern void            If_ManSetupCutTriv( If_Man_t * p, If_Cut_t * pCut, int ObjId );
extern void            If_ManSetupCiCutSets( If_Man_t * p );
extern If_Set_t *      If_ManSetupNodeCutSet( If_Man_t * p, If_Obj_t * pObj );
extern void            If_ManDerefNodeCutSet( If_Man_t * p, If_Obj_t * pObj );
extern void            If_ManDerefChoiceCutSet( If_Man_t * p, If_Obj_t * pObj );
extern void            If_ManSetupSetAll( If_Man_t * p, int nCrossCut );
/*=== ifMap.c =============================================================*/
extern int *           If_CutArrTimeProfile( If_Man_t * p, If_Cut_t * pCut );
extern void            If_ObjPerformMappingAnd( If_Man_t * p, If_Obj_t * pObj, int Mode, int fPreprocess, int fFirst );
extern void            If_ObjPerformMappingChoice( If_Man_t * p, If_Obj_t * pObj, int Mode, int fPreprocess );
extern int             If_ManPerformMappingRound( If_Man_t * p, int nCutsUsed, int Mode, int fPreprocess, int fFirst, char * pLabel );
/*=== ifReduce.c ==========================================================*/
extern void            If_ManImproveMapping( If_Man_t * p );
/*=== ifSat.c ==========================================================*/
extern void *          If_ManSatBuildXY( int nLutSize );
extern void *          If_ManSatBuildXYZ( int nLutSize );
extern void            If_ManSatUnbuild( void * p );
extern int             If_ManSatCheckXY( void * pSat, int nLutSize, word * pTruth, int nVars, unsigned uSet, word * pTBound, word * pTFree, Vec_Int_t * vLits );
extern unsigned        If_ManSatCheckXYall( void * pSat, int nLutSize, word * pTruth, int nVars, Vec_Int_t * vLits );
/*=== ifSeq.c =============================================================*/
extern int             If_ManPerformMappingSeq( If_Man_t * p );
/*=== ifTime.c ============================================================*/
extern float           If_CutDelay( If_Man_t * p, If_Obj_t * pObj, If_Cut_t * pCut );
extern void            If_CutPropagateRequired( If_Man_t * p, If_Obj_t * pObj, If_Cut_t * pCut, float Required );
extern float           If_ManDelayMax( If_Man_t * p, int fSeq );
extern void            If_ManComputeRequired( If_Man_t * p );
/*=== ifTruth.c ===========================================================*/
extern void            If_CutRotatePins( If_Man_t * p, If_Cut_t * pCut );
extern int             If_CutComputeTruth( If_Man_t * p, If_Cut_t * pCut, If_Cut_t * pCut0, If_Cut_t * pCut1, int fCompl0, int fCompl1 );
extern int             If_CutComputeTruthPerm( If_Man_t * p, If_Cut_t * pCut, If_Cut_t * pCut0, If_Cut_t * pCut1, int fCompl0, int fCompl1 );
extern Vec_Mem_t *     If_DeriveHashTable6( int nVars, word Truth );
extern int             If_CutCheckTruth6( If_Man_t * p, If_Cut_t * pCut );
/*=== ifTune.c ===========================================================*/
extern Ifn_Ntk_t *     Ifn_NtkParse( char * pStr );
extern int             Ifn_NtkTtBits( char * pStr );
extern int             Ifn_NtkMatch( Ifn_Ntk_t * p, word * pTruth, int nVars, int nConfls, int fVerbose, int fVeryVerbose, word * pConfig );
extern void            Ifn_NtkPrint( Ifn_Ntk_t * p );
extern int             Ifn_NtkLutSizeMax( Ifn_Ntk_t * p );
extern int             Ifn_NtkInputNum( Ifn_Ntk_t * p );
extern void *          If_ManSatBuildFromCell( char * pStr, Vec_Int_t ** pvPiVars, Vec_Int_t ** pvPoVars, Ifn_Ntk_t ** ppNtk );
extern int             If_ManSatFindCofigBits( void * pSat, Vec_Int_t * vPiVars, Vec_Int_t * vPoVars, word * pTruth, int nVars, word Perm, int nInps, Vec_Int_t * vValues );
extern int             If_ManSatDeriveGiaFromBits( void * pNew, Ifn_Ntk_t * p, word * pTtData, Vec_Int_t * vLeaves, Vec_Int_t * vValues );
extern void *          If_ManDeriveGiaFromCells( void * p );
/*=== ifUtil.c ============================================================*/
extern void            If_ManCleanNodeCopy( If_Man_t * p );
extern void            If_ManCleanCutData( If_Man_t * p );
extern void            If_ManCleanMarkV( If_Man_t * p );
extern float           If_ManScanMapping( If_Man_t * p );
extern float           If_ManScanMappingDirect( If_Man_t * p );
extern float           If_ManScanMappingSeq( If_Man_t * p );
extern void            If_ManResetOriginalRefs( If_Man_t * p );
extern int             If_ManCrossCut( If_Man_t * p );

extern Vec_Ptr_t *     If_ManReverseOrder( If_Man_t * p );
extern void            If_ManMarkMapping( If_Man_t * p );
extern Vec_Ptr_t *     If_ManCollectMappingDirect( If_Man_t * p );
extern Vec_Int_t *     If_ManCollectMappingInt( If_Man_t * p );

extern int             If_ManCountSpecialPos( If_Man_t * p );
extern void            If_CutTraverse( If_Man_t * p, If_Obj_t * pRoot, If_Cut_t * pCut, Vec_Ptr_t * vNodes );
extern void            If_ObjPrint( If_Obj_t * pObj );

extern int             acd_evaluate( word * pTruth, unsigned nVars, int lutSize, unsigned *pdelay, unsigned *cost, int try_no_late_arrival );
extern int             acd_decompose( word * pTruth, unsigned nVars, int lutSize, unsigned *pdelay, unsigned char *decomposition );
extern int             acd2_evaluate( word * pTruth, unsigned nVars, int lutSize, unsigned *pdelay, unsigned *cost, int try_no_late_arrival );
extern int             acd2_decompose( word * pTruth, unsigned nVars, int lutSize, unsigned *pdelay, unsigned char *decomposition );

ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

