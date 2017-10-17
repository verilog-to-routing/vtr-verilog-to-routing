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
 
#ifndef __IF_H__
#define __IF_H__

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "vec.h"
#include "mem.h"

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
#define IF_COST_MAX          ((1<<14)-1)

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
typedef struct If_Lib_t_     If_Lib_t;
typedef struct If_Obj_t_     If_Obj_t;
typedef struct If_Cut_t_     If_Cut_t;
typedef struct If_Set_t_     If_Set_t;

// parameters
struct If_Par_t_
{
    // user-controlable paramters
    int                nLutSize;      // the LUT size
    int                nCutsMax;      // the max number of cuts
    int                nFlowIters;    // the number of iterations of area recovery
    int                nAreaIters;    // the number of iterations of area recovery
    float              DelayTarget;   // delay target
    int                fPreprocess;   // preprossing
    int                fArea;         // area-oriented mapping
    int                fFancy;        // a fancy feature
    int                fExpRed;       // expand/reduce of the best cuts
    int                fLatchPaths;   // reset timing on latch paths
    int                fSeqMap;       // sequential mapping
    int                fVerbose;      // the verbosity flag
    // internal parameters
    int                fAreaOnly;     // area only mode
    int                fTruth;        // truth table computation enabled
    int                fUsePerm;      // use permutation (delay info)
    int                fUseBdds;      // use local BDDs as a cost function
    int                fUseSops;      // use local SOPs as a cost function
    int                fUseCnfs;      // use local CNFs as a cost function
    int                fUseMv;        // use local MV-SOPs as a cost function
    int                nLatches;      // the number of latches in seq mapping
    int                fLiftLeaves;   // shift the leaves for seq mapping
    If_Lib_t *         pLutLib;       // the LUT library
    float *            pTimesArr;     // arrival times
    float *            pTimesReq;     // required times
    int (* pFuncCost)  (If_Cut_t *);  // procedure to compute the user's cost of a cut
    int (* pFuncUser)  (If_Man_t *, If_Obj_t *, If_Cut_t *); //  procedure called for each cut when cut computation is finished
    void *             pReoMan;       // reordering manager
};

// the LUT library
struct If_Lib_t_
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
    // mapping parameters
    If_Par_t *         pPars;
    // mapping nodes
    If_Obj_t *         pConst1;       // the constant 1 node
    Vec_Ptr_t *        vCis;          // the primary inputs
    Vec_Ptr_t *        vCos;          // the primary outputs
    Vec_Ptr_t *        vObjs;         // all objects
    Vec_Ptr_t *        vMapped;       // objects used in the mapping
    Vec_Ptr_t *        vTemp;         // temporary array
    int                nObjs[IF_VOID];// the number of objects by type
    // various data
    int                nLevelMax;     // the max number of AIG levels
    float              fEpsilon;      // epsilon used for comparison
    float              RequiredGlo;   // global required times
    float              RequiredGlo2;  // global required times
    float              AreaGlo;       // global area
    int                nNets;         // the sum total of fanins of all LUTs in the mapping
    int                nCutsUsed;     // the number of cuts currently used
    int                nCutsMerged;   // the total number of cuts merged
    unsigned *         puTemp[4];     // used for the truth table computation
    int                SortMode;      // one of the three sorting modes
    int                fNextRound;    // set to 1 after the first round
    int                nChoices;      // the number of choice nodes
    // sequential mapping
    Vec_Ptr_t *        vLatchOrder;   // topological ordering of latches
    Vec_Int_t *        vLags;         // sequentail lags of all nodes
    int                nAttempts;     // the number of attempts in binary search
    int                nMaxIters;     // the maximum number of iterations
    int                Period;        // the current value of the clock period (for seq mapping)
    // memory management
    int                nTruthWords;   // the size of the truth table if allocated
    int                nPermWords;    // the size of the permutation array (in words)
    int                nObjBytes;     // the size of the object
    int                nCutBytes;     // the size of the cut
    int                nSetBytes;     // the size of the cut set
    Mem_Fixed_t *      pMemObj;       // memory manager for objects (entrysize = nEntrySize)
    Mem_Fixed_t *      pMemSet;       // memory manager for sets of cuts (entrysize = nCutSize*(nCutsMax+1))
    If_Set_t *         pMemCi;        // memory for CI cutsets
    If_Set_t *         pMemAnd;       // memory for AND cutsets
    If_Set_t *         pFreeList;     // the list of free cutsets
};

// priority cut
struct If_Cut_t_
{
    float              Delay;         // delay of the cut
    float              Area;          // area (or area-flow) of the cut
    float              AveRefs;       // the average number of leaf references
    unsigned           uSign;         // cut signature
    unsigned           Cost    : 14;  // the user's cost of the cut
    unsigned           fCompl  :  1;  // the complemented attribute 
    unsigned           fUser   :  1;  // using the user's area and delay
    unsigned           nLimit  :  8;  // the maximum number of leaves
    unsigned           nLeaves :  8;  // the number of leaves
    int *              pLeaves;       // array of fanins
    char *             pPerm;         // permutation
    unsigned *         pTruth;        // the truth table
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
    unsigned           Level   : 22;  // logic level of the node
    int                Id;            // integer ID
    int                nRefs;         // the number of references
    int                nVisits;       // the number of visits to this node
    int                nVisitsCopy;   // the number of visits to this node
    If_Obj_t *         pFanin0;       // the first fanin 
    If_Obj_t *         pFanin1;       // the second fanin
    If_Obj_t *         pEquiv;        // the choice node
    float              EstRefs;       // estimated reference counter
    float              Required;      // required time of the onde
    float              LValue;        // sequential arrival time of the node
    void *             pCopy;         // used for object duplication
    If_Set_t *         pCutSet;       // the pointer to the cutset
    If_Cut_t           CutBest;       // the best cut selected 
};

static inline If_Obj_t * If_Regular( If_Obj_t * p )                          { return (If_Obj_t *)((unsigned long)(p) & ~01);  }
static inline If_Obj_t * If_Not( If_Obj_t * p )                              { return (If_Obj_t *)((unsigned long)(p) ^  01);  }
static inline If_Obj_t * If_NotCond( If_Obj_t * p, int c )                   { return (If_Obj_t *)((unsigned long)(p) ^ (c));  }
static inline int        If_IsComplement( If_Obj_t * p )                     { return (int )(((unsigned long)p) & 01);         }

static inline int        If_ManCiNum( If_Man_t * p )                         { return p->nObjs[IF_CI];               }
static inline int        If_ManCoNum( If_Man_t * p )                         { return p->nObjs[IF_CO];               }
static inline int        If_ManAndNum( If_Man_t * p )                        { return p->nObjs[IF_AND];              }
static inline int        If_ManObjNum( If_Man_t * p )                        { return Vec_PtrSize(p->vObjs);         }

static inline If_Obj_t * If_ManConst1( If_Man_t * p )                        { return p->pConst1;                              }
static inline If_Obj_t * If_ManCi( If_Man_t * p, int i )                     { return (If_Obj_t *)Vec_PtrEntry( p->vCis, i );  }
static inline If_Obj_t * If_ManCo( If_Man_t * p, int i )                     { return (If_Obj_t *)Vec_PtrEntry( p->vCos, i );  }
static inline If_Obj_t * If_ManLi( If_Man_t * p, int i )                     { return (If_Obj_t *)Vec_PtrEntry( p->vCos, If_ManCoNum(p) - p->pPars->nLatches + i );  }
static inline If_Obj_t * If_ManLo( If_Man_t * p, int i )                     { return (If_Obj_t *)Vec_PtrEntry( p->vCis, If_ManCiNum(p) - p->pPars->nLatches + i );  }
static inline If_Obj_t * If_ManObj( If_Man_t * p, int i )                    { return (If_Obj_t *)Vec_PtrEntry( p->vObjs, i ); }

static inline int        If_ObjIsConst1( If_Obj_t * pObj )                   { return pObj->Type == IF_CONST1;       }
static inline int        If_ObjIsCi( If_Obj_t * pObj )                       { return pObj->Type == IF_CI;           }
static inline int        If_ObjIsCo( If_Obj_t * pObj )                       { return pObj->Type == IF_CO;           }
static inline int        If_ObjIsPi( If_Obj_t * pObj )                       { return If_ObjIsCi(pObj) && pObj->pFanin0 == NULL; }
static inline int        If_ObjIsLatch( If_Obj_t * pObj )                    { return If_ObjIsCi(pObj) && pObj->pFanin0 != NULL; }
static inline int        If_ObjIsAnd( If_Obj_t * pObj )                      { return pObj->Type == IF_AND;          }

static inline If_Obj_t * If_ObjFanin0( If_Obj_t * pObj )                     { return pObj->pFanin0;                 }
static inline If_Obj_t * If_ObjFanin1( If_Obj_t * pObj )                     { return pObj->pFanin1;                 }
static inline int        If_ObjFaninC0( If_Obj_t * pObj )                    { return pObj->fCompl0;                 }
static inline int        If_ObjFaninC1( If_Obj_t * pObj )                    { return pObj->fCompl1;                 }
static inline void *     If_ObjCopy( If_Obj_t * pObj )                       { return pObj->pCopy;                   }
static inline void       If_ObjSetCopy( If_Obj_t * pObj, void * pCopy )      { pObj->pCopy = pCopy;                  }
static inline void       If_ObjSetChoice( If_Obj_t * pObj, If_Obj_t * pEqu ) { pObj->pEquiv = pEqu;                  }

static inline If_Cut_t * If_ObjCutBest( If_Obj_t * pObj )                    { return &pObj->CutBest;                }
static inline unsigned   If_ObjCutSign( unsigned ObjId )                     { return (1 << (ObjId % 31));           }

static inline float      If_ObjArrTime( If_Obj_t * pObj )                    { return If_ObjCutBest(pObj)->Delay;    }
static inline void       If_ObjSetArrTime( If_Obj_t * pObj, float ArrTime )  { If_ObjCutBest(pObj)->Delay = ArrTime; }

static inline float      If_ObjLValue( If_Obj_t * pObj )                     { return pObj->LValue;                  }
static inline void       If_ObjSetLValue( If_Obj_t * pObj, float LValue )    { pObj->LValue = LValue;                }

static inline void *     If_CutData( If_Cut_t * pCut )                       { return *(void **)pCut;                }
static inline void       If_CutSetData( If_Cut_t * pCut, void * pData )      { *(void **)pCut = pData;               }

static inline int        If_CutLeaveNum( If_Cut_t * pCut )                   { return pCut->nLeaves;                             }
static inline unsigned * If_CutTruth( If_Cut_t * pCut )                      { return pCut->pTruth;                              }
static inline int        If_CutTruthWords( int nVarsMax )                    { return nVarsMax <= 5 ? 1 : (1 << (nVarsMax - 5)); }
static inline int        If_CutPermWords( int nVarsMax )                     { return nVarsMax / sizeof(int) + ((nVarsMax % sizeof(int)) > 0); }

static inline float      If_CutLutArea( If_Man_t * p, If_Cut_t * pCut )      { return pCut->fUser? (float)pCut->Cost : (p->pPars->pLutLib? p->pPars->pLutLib->pLutAreas[pCut->nLeaves] : (float)1.0); }

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
    Vec_PtrForEachEntry( p->vCis, pObj, i )
// iterator over the primary outputs
#define If_ManForEachCo( p, pObj, i )                                          \
    Vec_PtrForEachEntry( p->vCos, pObj, i )
// iterator over the primary inputs
#define If_ManForEachPi( p, pObj, i )                                          \
    Vec_PtrForEachEntryStop( p->vCis, pObj, i, If_ManCiNum(p) - p->pPars->nLatches )
// iterator over the primary outputs
#define If_ManForEachPo( p, pObj, i )                                          \
    Vec_PtrForEachEntryStop( p->vCos, pObj, i, If_ManCoNum(p) - p->pPars->nLatches )
// iterator over the latches 
#define If_ManForEachLatchInput( p, pObj, i )                                  \
    Vec_PtrForEachEntryStart( p->vCos, pObj, i, If_ManCoNum(p) - p->pPars->nLatches )
#define If_ManForEachLatchOutput( p, pObj, i )                                 \
    Vec_PtrForEachEntryStart( p->vCis, pObj, i, If_ManCiNum(p) - p->pPars->nLatches )
// iterator over all objects, including those currently not used
#define If_ManForEachObj( p, pObj, i )                                         \
    Vec_PtrForEachEntry( p->vObjs, pObj, i )
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
//#define If_CutForEachLeaf( p, pCut, pLeaf, i )                                 \
//    for ( i = 0; (i < (int)(pCut)->nLeaves) && ((pLeaf) = If_ManObj(p, p->pPars->fLiftLeaves? (pCut)->pLeaves[i] >> 8 : (pCut)->pLeaves[i])); i++ )
// iterator over the leaves of the sequential cut
#define If_CutForEachLeafSeq( p, pCut, pLeaf, Shift, i )                       \
    for ( i = 0; (i < (int)(pCut)->nLeaves) && ((pLeaf) = If_ManObj(p, (pCut)->pLeaves[i] >> 8)) && (((Shift) = ((pCut)->pLeaves[i] & 255)) >= 0); i++ )

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== ifCore.c ===========================================================*/
extern int             If_ManPerformMapping( If_Man_t * p );
extern int             If_ManPerformMappingComb( If_Man_t * p );
/*=== ifCut.c ============================================================*/
extern float           If_CutAreaDerefed( If_Man_t * p, If_Cut_t * pCut, int nLevels );
extern float           If_CutAreaRefed( If_Man_t * p, If_Cut_t * pCut, int nLevels );
extern float           If_CutDeref( If_Man_t * p, If_Cut_t * pCut, int nLevels );
extern float           If_CutRef( If_Man_t * p, If_Cut_t * pCut, int nLevels );
extern void            If_CutPrint( If_Man_t * p, If_Cut_t * pCut );
extern void            If_CutPrintTiming( If_Man_t * p, If_Cut_t * pCut );
extern float           If_CutFlow( If_Man_t * p, If_Cut_t * pCut );
extern float           If_CutAverageRefs( If_Man_t * p, If_Cut_t * pCut );
extern int             If_CutFilter( If_Set_t * pCutSet, If_Cut_t * pCut );
extern void            If_CutSort( If_Man_t * p, If_Set_t * pCutSet, If_Cut_t * pCut );
extern int             If_CutMerge( If_Cut_t * pCut0, If_Cut_t * pCut1, If_Cut_t * pCut );
extern void            If_CutLift( If_Cut_t * pCut );
extern void            If_CutCopy( If_Man_t * p, If_Cut_t * pCutDest, If_Cut_t * pCutSrc );
extern void            If_ManSortCuts( If_Man_t * p, int Mode );
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
extern void            If_ObjPerformMappingAnd( If_Man_t * p, If_Obj_t * pObj, int Mode, int fPreprocess );
extern void            If_ObjPerformMappingChoice( If_Man_t * p, If_Obj_t * pObj, int Mode, int fPreprocess );
extern int             If_ManPerformMappingRound( If_Man_t * p, int nCutsUsed, int Mode, int fPreprocess, char * pLabel );
/*=== ifReduce.c ==========================================================*/
extern void            If_ManImproveMapping( If_Man_t * p );
/*=== ifSeq.c =============================================================*/
extern int             If_ManPerformMappingSeq( If_Man_t * p );
/*=== ifTime.c ============================================================*/
extern float           If_CutDelay( If_Man_t * p, If_Cut_t * pCut );
extern void            If_CutPropagateRequired( If_Man_t * p, If_Cut_t * pCut, float Required );
/*=== ifTruth.c ===========================================================*/
extern void            If_CutComputeTruth( If_Man_t * p, If_Cut_t * pCut, If_Cut_t * pCut0, If_Cut_t * pCut1, int fCompl0, int fCompl1 );
/*=== ifUtil.c ============================================================*/
extern void            If_ManCleanNodeCopy( If_Man_t * p );
extern void            If_ManCleanCutData( If_Man_t * p );
extern void            If_ManCleanMarkV( If_Man_t * p );
extern float           If_ManDelayMax( If_Man_t * p, int fSeq );
extern void            If_ManComputeRequired( If_Man_t * p );
extern float           If_ManScanMapping( If_Man_t * p );
extern float           If_ManScanMappingSeq( If_Man_t * p );
extern void            If_ManResetOriginalRefs( If_Man_t * p );
extern int             If_ManCrossCut( If_Man_t * p );

#ifdef __cplusplus
}
#endif

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

