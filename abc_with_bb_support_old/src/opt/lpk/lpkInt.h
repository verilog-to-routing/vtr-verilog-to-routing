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
 
#ifndef __LPK_INT_H__
#define __LPK_INT_H__

#ifdef __cplusplus
extern "C" {
#endif 

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "abc.h"
#include "kit.h"
#include "if.h"
#include "lpk.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

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
    // truth table representation
    Vec_Ptr_t *  vTtElems;              // elementary truth tables
    Vec_Ptr_t *  vTtNodes;              // storage for temporary truth tables of the nodes 
    // variable sets
    Vec_Int_t *  vSets[8];
    Kit_DsdMan_t * pDsdMan;
    // statistics
    int          nNodesTotal;           // total number of nodes
    int          nNodesOver;            // nodes with cuts over the limit 
    int          nCutsTotal;            // total number of cuts
    int          nCutsUseful;           // useful cuts 
    int          nGainTotal;            // the gain in LUTs
    int          nChanges;              // the number of changed nodes
    int          nBenefited;            // the number of gainful that benefited from decomposition
    int          nTotalNets;
    int          nTotalNets2;
    // counter of non-DSD blocks
    int          nBlocks[17];
    // rutime
    int          timeCuts;
    int          timeTruth;
    int          timeEval;
    int          timeMap;
    int          timeOther;
    int          timeTotal;
};

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

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== lpkCut.c =========================================================*/
extern unsigned *     Lpk_CutTruth( Lpk_Man_t * p, Lpk_Cut_t * pCut );
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

#ifdef __cplusplus
}
#endif

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

