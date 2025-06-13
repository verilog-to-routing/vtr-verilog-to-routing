/**CFile****************************************************************

  FileName    [resInt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Resynthesis package.]

  Synopsis    [Internal declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 15, 2007.]

  Revision    [$Id: resInt.h,v 1.00 2007/01/15 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__opt__res__resInt_h
#define ABC__opt__res__resInt_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "res.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Res_Win_t_ Res_Win_t;
struct Res_Win_t_
{
    // windowing parameters
    Abc_Obj_t *      pNode;        // the node in the center
    int              nWinTfiMax;   // the fanin levels
    int              nWinTfoMax;   // the fanout levels
    int              nLevDivMax;   // the maximum divisor level
    // internal windowing parameters
    int              nFanoutLimit; // the limit on the fanout count of a TFO node (if more, the node is treated as a root)
    int              nLevTfiMinus; // the number of additional levels to search from TFO below the level of leaves
    // derived windowing parameters
    int              nLevLeafMin;  // the minimum level of a leaf
    int              nLevTravMin;  // the minimum level to search from TFO
    int              nDivsPlus;    // the number of additional divisors
    // the window data
    Vec_Ptr_t *      vRoots;       // outputs of the window
    Vec_Ptr_t *      vLeaves;      // inputs of the window
    Vec_Ptr_t *      vBranches;    // side nodes of the window
    Vec_Ptr_t *      vNodes;       // internal nodes of the window
    Vec_Ptr_t *      vDivs;        // candidate divisors of the node
    // temporary data
    Vec_Vec_t *      vMatrix;      // TFI nodes below the given node
};

typedef struct Res_Sim_t_ Res_Sim_t;
struct Res_Sim_t_
{
    Abc_Ntk_t *      pAig;         // AIG for simulation
    int              nTruePis;     // the number of true PIs of the window
    int              fConst0;      // the node can be replaced by constant 0
    int              fConst1;      // the node can be replaced by constant 0
    // simulation parameters
    int              nWords;       // the number of simulation words
    int              nPats;        // the number of patterns
    int              nWordsIn;     // the number of simulation words in the input patterns
    int              nPatsIn;      // the number of patterns in the input patterns 
    int              nBytesIn;     // the number of bytes in the input patterns
    int              nWordsOut;    // the number of simulation words in the output patterns
    int              nPatsOut;     // the number of patterns in the output patterns 
    // simulation info
    Vec_Ptr_t *      vPats;        // input simulation patterns
    Vec_Ptr_t *      vPats0;       // input simulation patterns
    Vec_Ptr_t *      vPats1;       // input simulation patterns
    Vec_Ptr_t *      vOuts;        // output simulation info
    int              nPats0;       // the number of 0-patterns accumulated
    int              nPats1;       // the number of 1-patterns accumulated
    // resub candidates
    Vec_Vec_t *      vCands;       // resubstitution candidates
    // statistics
    abctime          timeSat;
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== resDivs.c ==========================================================*/
extern void          Res_WinDivisors( Res_Win_t * p, int nLevDivMax );
extern void          Res_WinSweepLeafTfo_rec( Abc_Obj_t * pObj, int nLevelLimit );
extern int           Res_WinVisitMffc( Abc_Obj_t * pNode );
/*=== resFilter.c ==========================================================*/
extern int           Res_FilterCandidates( Res_Win_t * pWin, Abc_Ntk_t * pAig, Res_Sim_t * pSim, Vec_Vec_t * vResubs, Vec_Vec_t * vResubsW, int nFaninsMax, int fArea );
extern int           Res_FilterCandidatesArea( Res_Win_t * pWin, Abc_Ntk_t * pAig, Res_Sim_t * pSim, Vec_Vec_t * vResubs, Vec_Vec_t * vResubsW, int nFaninsMax );
/*=== resSat.c ==========================================================*/
extern void *        Res_SatProveUnsat( Abc_Ntk_t * pAig, Vec_Ptr_t * vFanins );
extern int           Res_SatSimulate( Res_Sim_t * p, int nPats, int fOnSet );
/*=== resSim.c ==========================================================*/
extern Res_Sim_t *   Res_SimAlloc( int nWords );
extern void          Res_SimFree( Res_Sim_t * p );
extern int           Res_SimPrepare( Res_Sim_t * p, Abc_Ntk_t * pAig, int nTruePis, int fVerbose );
/*=== resStrash.c ==========================================================*/
extern Abc_Ntk_t *   Res_WndStrash( Res_Win_t * p );
/*=== resWnd.c ==========================================================*/
extern void          Res_UpdateNetwork( Abc_Obj_t * pObj, Vec_Ptr_t * vFanins, Hop_Obj_t * pFunc, Vec_Vec_t * vLevels );
/*=== resWnd.c ==========================================================*/
extern Res_Win_t *   Res_WinAlloc();
extern void          Res_WinFree( Res_Win_t * p );
extern int           Res_WinIsTrivial( Res_Win_t * p );
extern int           Res_WinCompute( Abc_Obj_t * pNode, int nWinTfiMax, int nWinTfoMax, Res_Win_t * p );




ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

