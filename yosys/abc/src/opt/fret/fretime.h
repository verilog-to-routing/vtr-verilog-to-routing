/**CFile****************************************************************

  FileName    [fretime.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Flow-based retiming package.]

  Synopsis    [Header file for retiming package.]

  Author      [Aaron Hurst]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2008.]

  Revision    [$Id: fretime.h,v 1.00 2008/01/01 00:00:00 ahurst Exp $]

***********************************************************************/

#if !defined(RETIME_H_)
#define RETIME_H_


#include "base/abc/abc.h"
#include "misc/vec/vec.h"


ABC_NAMESPACE_HEADER_START


// #define IGNORE_TIMING
// #define DEBUG_PRINT_FLOWS
// #define DEBUG_VISITED
// #define DEBUG_PREORDER
#define DEBUG_CHECK
// #define DEBUG_PRINT_LEVELS

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define MAX_DIST 30000

// flags in Flow_Data structure...
#define VISITED_E       0x001
#define VISITED_R       0x002
#define VISITED  (VISITED_E | VISITED_R)
#define FLOW            0x004
#define CROSS_BOUNDARY  0x008
#define BLOCK           0x010
#define INIT_0          0x020
#define INIT_1          0x040
#define INIT_CARE (INIT_0 | INIT_1)
#define CONSERVATIVE    0x080
#define BLOCK_OR_CONS (BLOCK | CONSERVATIVE)
#define BIAS_NODE       0x100


#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))


typedef struct Flow_Data_t_ {
  unsigned int mark : 16;

  union {
    Abc_Obj_t   *pred;
    /* unsigned int var; */
    Abc_Obj_t   *pInitObj;
    Abc_Obj_t   *pCopy;
    Vec_Ptr_t   *vNodes;
  };

  unsigned int e_dist : 16;
  unsigned int r_dist : 16;
} Flow_Data_t;

// useful macros for manipulating Flow_Data structure...
#define FDATA( x )     (pManMR->pDataArray+Abc_ObjId(x))
#define FSET( x, y )   FDATA(x)->mark |= y
#define FUNSET( x, y ) FDATA(x)->mark &= ~y
#define FTEST( x, y )  (FDATA(x)->mark & y)
#define FTIMEEDGES( x )  &(pManMR->vTimeEdges[Abc_ObjId( x )])

typedef struct NodeLag_T_ {
  int id;
  int lag;
} NodeLag_t;

typedef struct InitConstraint_t_ {
  Abc_Obj_t *pBiasNode;

  Vec_Int_t  vNodes;
  Vec_Int_t  vLags;

} InitConstraint_t;

typedef struct MinRegMan_t_ {
 
  // problem description:
  int         maxDelay;
  int        fComputeInitState, fGuaranteeInitState, fBlockConst;
  int         nNodes, nLatches;
  int        fForwardOnly, fBackwardOnly;
  int        fConservTimingOnly;
  int         nMaxIters;
  int        fVerbose;
  Abc_Ntk_t  *pNtk;

  int         nPreRefine;

  // problem state
  int        fIsForward;
  int        fSinkDistTerminate;
  int         nExactConstraints, nConservConstraints;
  int         fSolutionIsDc;
  int         constraintMask;
  int         iteration, subIteration;
  Vec_Int_t  *vLags;
  
  // problem data
  Vec_Int_t   *vSinkDistHist;
  Flow_Data_t *pDataArray;
  Vec_Ptr_t   *vTimeEdges;
  Vec_Ptr_t   *vExactNodes;
  Vec_Ptr_t   *vInitConstraints;
  Abc_Ntk_t   *pInitNtk;
  Vec_Ptr_t   *vNodes; // re-useable struct
  
  NodeLag_t   *pInitToOrig;
  int          sizeInitToOrig;
  
} MinRegMan_t ;

extern MinRegMan_t *pManMR;

#define vprintf if (pManMR->fVerbose) printf 

static inline void FSETPRED(Abc_Obj_t *pObj, Abc_Obj_t *pPred) {
  assert(!Abc_ObjIsLatch(pObj)); // must preserve field to maintain init state linkage
  FDATA(pObj)->pred = pPred;
}
static inline Abc_Obj_t * FGETPRED(Abc_Obj_t *pObj) {
  return FDATA(pObj)->pred;
}

/*=== fretMain.c ==========================================================*/

Abc_Ntk_t *    Abc_FlowRetime_MinReg( Abc_Ntk_t * pNtk, int fVerbose,
                                      int fComputeInitState, int fGuaranteeInitState, int fBlockConst,
                                      int fForward, int fBackward, int nMaxIters,
                                      int maxDelay, int fFastButConservative);

void print_node(Abc_Obj_t *pObj);

void Abc_ObjBetterTransferFanout( Abc_Obj_t * pFrom, Abc_Obj_t * pTo, int complement );

int  Abc_FlowRetime_PushFlows( Abc_Ntk_t * pNtk, int fVerbose );
int Abc_FlowRetime_IsAcrossCut( Abc_Obj_t *pCur, Abc_Obj_t *pNext );
void Abc_FlowRetime_ClearFlows( int fClearAll );

int  Abc_FlowRetime_GetLag( Abc_Obj_t *pObj );
void Abc_FlowRetime_SetLag( Abc_Obj_t *pObj, int lag );

void Abc_FlowRetime_UpdateLags( );

void Abc_ObjPrintNeighborhood( Abc_Obj_t *pObj, int depth );

Abc_Ntk_t * Abc_FlowRetime_NtkSilentRestrash( Abc_Ntk_t * pNtk, int fCleanup );

/*=== fretFlow.c ==========================================================*/

int  dfsplain_e( Abc_Obj_t *pObj, Abc_Obj_t *pPred );
int  dfsplain_r( Abc_Obj_t *pObj, Abc_Obj_t *pPred );

void dfsfast_preorder( Abc_Ntk_t *pNtk );
int  dfsfast_e( Abc_Obj_t *pObj, Abc_Obj_t *pPred );
int  dfsfast_r( Abc_Obj_t *pObj, Abc_Obj_t *pPred );

/*=== fretInit.c ==========================================================*/

void Abc_FlowRetime_PrintInitStateInfo( Abc_Ntk_t * pNtk );

void Abc_FlowRetime_InitState( Abc_Ntk_t * pNtk );

void Abc_FlowRetime_UpdateForwardInit( Abc_Ntk_t * pNtk );
void Abc_FlowRetime_UpdateBackwardInit( Abc_Ntk_t * pNtk );

void Abc_FlowRetime_SetupBackwardInit( Abc_Ntk_t * pNtk );
int  Abc_FlowRetime_SolveBackwardInit( Abc_Ntk_t * pNtk );

void Abc_FlowRetime_ConstrainInit(  );
void Abc_FlowRetime_AddInitBias(  );
void Abc_FlowRetime_RemoveInitBias(  );

/*=== fretTime.c ==========================================================*/

void Abc_FlowRetime_InitTiming( Abc_Ntk_t *pNtk );
void Abc_FlowRetime_FreeTiming( Abc_Ntk_t *pNtk );

int Abc_FlowRetime_RefineConstraints( );

void Abc_FlowRetime_ConstrainConserv( Abc_Ntk_t * pNtk );
void Abc_FlowRetime_ConstrainExact( Abc_Obj_t * pObj );
void Abc_FlowRetime_ConstrainExactAll( Abc_Ntk_t * pNtk );



ABC_NAMESPACE_HEADER_END

#endif
