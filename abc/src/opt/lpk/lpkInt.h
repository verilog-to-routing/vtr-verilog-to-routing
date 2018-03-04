/**CFile****************************************************************

  FileName    [lpkInt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Fast Boolean matching for LUT structures.]

  Synopsis    [Internal declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: lpkInt.h,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__opt__lpk__lpkInt_h
#define ABC__opt__lpk__lpkInt_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "base/abc/abc.h"
#include "bool/kit/kit.h"
#include "map/if/if.h"
#include "lpk.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START
 

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

#define LPK_SIZE_MAX             24     // the largest size of the function
#define LPK_CUTS_MAX            512     // the largest number of cuts considered

typedef struct Lpk_Man_t_ Lpk_Man_t;
typedef struct Lpk_Cut_t_ Lpk_Cut_t;

struct Lpk_Cut_t_
{
    unsigned     nLeaves       : 6;     // (L) the number of leaves
    unsigned     nNodes        : 6;     // (M) the number of nodes
    unsigned     nNodesDup     : 6;     // (Q) nodes outside of MFFC
    unsigned     nLuts         : 6;     // (N) the number of LUTs to try
    unsigned     unused        : 6;     // unused
    unsigned     fHasDsd       : 1;     // set to 1 if the cut has structural DSD (and so cannot be used)
    unsigned     fMark         : 1;     // multipurpose mark
    unsigned     uSign[2];              // the signature
    float        Weight;                // the weight of the cut: (M - Q)/N(V)   (the larger the better)
    int          Gain;                  // the gain achieved using this cut
    int          pLeaves[LPK_SIZE_MAX]; // the leaves of the cut
    int          pNodes[LPK_SIZE_MAX];  // the nodes of the cut
};

struct Lpk_Man_t_
{
    // parameters
    Lpk_Par_t *  pPars;                 // the set of parameters
    // current representation
    Abc_Ntk_t *  pNtk;                  // the network
    Abc_Obj_t *  pObj;                  // the node to resynthesize 
    // cut representation
    int          nMffc;                 // the size of MFFC of the node
    int          nCuts;                 // the total number of cuts    
    int          nCutsMax;              // the largest possible number of cuts
    int          nEvals;                // the number of good cuts
    Lpk_Cut_t    pCuts[LPK_CUTS_MAX];   // the storage for cuts
    int          pEvals[LPK_CUTS_MAX];  // the good cuts
    // visited nodes 
    Vec_Vec_t *  vVisited;
    // mapping manager
    If_Man_t *   pIfMan;
    Vec_Int_t *  vCover; 
    Vec_Vec_t *  vLevels;
    // temporary variables
    int          fCofactoring;          // working in the cofactoring mode
    int          fCalledOnce;           // limits the depth of MUX cofactoring
    int          nCalledSRed;           // the number of called to SRed
    int          pRefs[LPK_SIZE_MAX];   // fanin reference counters 
    int          pCands[LPK_SIZE_MAX];  // internal nodes pointing only to the leaves
    Vec_Ptr_t *  vLeaves;
    // truth table representation
    Vec_Ptr_t *  vTtElems;              // elementary truth tables
    Vec_Ptr_t *  vTtNodes;              // storage for temporary truth tables of the nodes 
    Vec_Int_t *  vMemory;
    Vec_Int_t *  vBddDir;
    Vec_Int_t *  vBddInv;
    unsigned     puSupps[32];           // the supports of the cofactors
    unsigned *   ppTruths[5][16];
    // variable sets
    Vec_Int_t *  vSets[8];
    Kit_DsdMan_t* pDsdMan;
    // statistics
    int          nNodesTotal;           // total number of nodes
    int          nNodesOver;            // nodes with cuts over the limit 
    int          nCutsTotal;            // total number of cuts
    int          nCutsUseful;           // useful cuts 
    int          nGainTotal;            // the gain in LUTs
    int          nChanges;              // the number of changed nodes
    int          nBenefited;            // the number of gainful that benefited from decomposition
    int          nMuxes;
    int          nDsds;
    int          nTotalNets;
    int          nTotalNets2;
    int          nTotalNodes;
    int          nTotalNodes2;
    // counter of non-DSD blocks
    int          nBlocks[17];
    // runtime
    abctime      timeCuts;
    abctime      timeTruth;
    abctime      timeSupps;
    abctime      timeTruth2;
    abctime      timeTruth3;
    abctime      timeEval;
    abctime      timeMap;
    abctime      timeOther;
    abctime      timeTotal;
    // runtime of eval
    abctime      timeEvalMuxAn;
    abctime      timeEvalMuxSp;
    abctime      timeEvalDsdAn;
    abctime      timeEvalDsdSp;
 
};


// internal representation of the function to be decomposed
typedef struct Lpk_Fun_t_ Lpk_Fun_t;
struct Lpk_Fun_t_
{
    Vec_Ptr_t *  vNodes;           // the array of leaves and decomposition nodes
    unsigned     Id         :  7;  // the ID of this node
    unsigned     nVars      :  5;  // the number of variables
    unsigned     nLutK      :  4;  // the number of LUT inputs
    unsigned     nAreaLim   :  5;  // the area limit (the largest allowed)
    unsigned     nDelayLim  :  9;  // the delay limit (the largest allowed)
    unsigned     fSupports  :  1;  // supports of cofactors were precomputed
    unsigned     fMark      :  1;  // marks the MUX-based dec
    unsigned     uSupp;            // the support of this component
    unsigned     puSupps[32];      // the supports of the cofactors
    char         pDelays[16];      // the delays of the inputs
    char         pFanins[16];      // the fanins of this function
    unsigned     pTruth[0];        // the truth table (contains room for three truth tables)    
};

// preliminary decomposition result
typedef struct Lpk_Res_t_ Lpk_Res_t;
struct Lpk_Res_t_
{
    int          nBSVars;          // the number of bound set variables
    unsigned     BSVars;           // the bound set
    int          nCofVars;         // the number of cofactoring variables
    char         pCofVars[4];      // the cofactoring variables
    int          nSuppSizeS;       // support size of the smaller (decomposed) function 
    int          nSuppSizeL;       // support size of the larger (composition) function
    int          DelayEst;         // estimated delay of the decomposition
    int          AreaEst;          // estimated area of the decomposition
    int          Variable;         // variable in MUX decomposition
    int          Polarity;         // polarity in MUX decomposition
};

static inline int        Lpk_LutNumVars( int nLutsLim, int nLutK ) { return  nLutsLim * (nLutK - 1) + 1;                                            }
static inline int        Lpk_LutNumLuts( int nVarsMax, int nLutK ) { return (nVarsMax - 1) / (nLutK - 1) + (int)((nVarsMax - 1) % (nLutK - 1) > 0); }
static inline unsigned * Lpk_FunTruth( Lpk_Fun_t * p, int Num )    { assert( Num < 3 ); return p->pTruth + Kit_TruthWordNum(p->nVars) * Num;        }

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                           ITERATORS                              ///
////////////////////////////////////////////////////////////////////////

#define Lpk_CutForEachLeaf( pNtk, pCut, pObj, i )                                        \
    for ( i = 0; (i < (int)(pCut)->nLeaves) && (((pObj) = Abc_NtkObj(pNtk, (pCut)->pLeaves[i])), 1); i++ )
#define Lpk_CutForEachNode( pNtk, pCut, pObj, i )                                        \
    for ( i = 0; (i < (int)(pCut)->nNodes) && (((pObj) = Abc_NtkObj(pNtk, (pCut)->pNodes[i])), 1); i++ )
#define Lpk_CutForEachNodeReverse( pNtk, pCut, pObj, i )                                 \
    for ( i = (int)(pCut)->nNodes - 1; (i >= 0) && (((pObj) = Abc_NtkObj(pNtk, (pCut)->pNodes[i])), 1); i-- )
#define Lpk_SuppForEachVar( Supp, Var )\
    for ( Var = 0; Var < 16; Var++ )\
        if ( !(Supp & (1<<Var)) ) {} else

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== lpkAbcDec.c ============================================================*/
extern Abc_Obj_t *    Lpk_Decompose( Lpk_Man_t * pMan, Abc_Ntk_t * pNtk, Vec_Ptr_t * vLeaves, unsigned * pTruth, unsigned * puSupps, int nLutK, int AreaLim, int DelayLim );
/*=== lpkAbcDsd.c ============================================================*/
extern Lpk_Res_t *    Lpk_DsdAnalize( Lpk_Man_t * pMan, Lpk_Fun_t * p, int nShared );
extern Lpk_Fun_t *    Lpk_DsdSplit( Lpk_Man_t * pMan, Lpk_Fun_t * p, char * pCofVars, int nCofVars, unsigned uBoundSet );
/*=== lpkAbcMux.c ============================================================*/
extern Lpk_Res_t *    Lpk_MuxAnalize( Lpk_Man_t * pMan, Lpk_Fun_t * p );
extern Lpk_Fun_t *    Lpk_MuxSplit( Lpk_Man_t * pMan, Lpk_Fun_t * p, int Var, int Pol );
/*=== lpkAbcUtil.c ============================================================*/
extern Lpk_Fun_t *    Lpk_FunAlloc( int nVars );
extern void           Lpk_FunFree( Lpk_Fun_t * p );
extern Lpk_Fun_t *    Lpk_FunCreate( Abc_Ntk_t * pNtk, Vec_Ptr_t * vLeaves, unsigned * pTruth, int nLutK, int AreaLim, int DelayLim );
extern Lpk_Fun_t *    Lpk_FunDup( Lpk_Fun_t * p, unsigned * pTruth );
extern int            Lpk_FunSuppMinimize( Lpk_Fun_t * p );
extern void           Lpk_FunComputeCofSupps( Lpk_Fun_t * p );
extern int            Lpk_SuppDelay( unsigned uSupp, char * pDelays );
extern int            Lpk_SuppToVars( unsigned uBoundSet, char * pVars );


/*=== lpkCut.c =========================================================*/
extern unsigned *     Lpk_CutTruth( Lpk_Man_t * p, Lpk_Cut_t * pCut, int fInv );
extern int            Lpk_NodeCuts( Lpk_Man_t * p );
/*=== lpkMap.c =========================================================*/
extern Lpk_Man_t *    Lpk_ManStart( Lpk_Par_t * pPars );
extern void           Lpk_ManStop( Lpk_Man_t * p );
/*=== lpkMap.c =========================================================*/
extern If_Obj_t *     Lpk_MapPrime( Lpk_Man_t * p, unsigned * pTruth, int nVars, If_Obj_t ** ppLeaves );
extern If_Obj_t *     Lpk_MapTree_rec( Lpk_Man_t * p, Kit_DsdNtk_t * pNtk, If_Obj_t ** ppLeaves, int iLit, If_Obj_t * pResult );
/*=== lpkMulti.c =======================================================*/
extern If_Obj_t *     Lpk_MapTreeMulti( Lpk_Man_t * p, unsigned * pTruth, int nLeaves, If_Obj_t ** ppLeaves );
/*=== lpkMux.c =========================================================*/
extern If_Obj_t *     Lpk_MapTreeMux_rec( Lpk_Man_t * p, unsigned * pTruth, int nVars, If_Obj_t ** ppLeaves );
extern If_Obj_t *     Lpk_MapSuppRedDec_rec( Lpk_Man_t * p, unsigned * pTruth, int nVars, If_Obj_t ** ppLeaves );
/*=== lpkSets.c =========================================================*/
extern unsigned       Lpk_MapSuppRedDecSelect( Lpk_Man_t * p, unsigned * pTruth, int nVars, int * piVar, int * piVarReused );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

