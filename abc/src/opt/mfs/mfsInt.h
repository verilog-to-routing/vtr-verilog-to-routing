/**CFile****************************************************************

  FileName    [mfsInt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [The good old minimization with complete don't-cares.]

  Synopsis    [Internal declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: mfsInt.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__opt__mfs__mfsInt_h
#define ABC__opt__mfs__mfsInt_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "base/abc/abc.h"
#include "mfs.h"
#include "aig/aig/aig.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satSolver.h"
#include "sat/bsat/satStore.h"
#include "bool/bdc/bdc.h"
#include "aig/gia/gia.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START


#define MFS_FANIN_MAX   12

typedef struct Mfs_Man_t_ Mfs_Man_t;
struct Mfs_Man_t_
{
    // input data
    Mfs_Par_t *         pPars;
    Abc_Ntk_t *         pNtk;
    Aig_Man_t *         pCare;
    Vec_Ptr_t *         vSuppsInv;
    int                 nFaninMax;
    // intermeditate data for the node
    Vec_Ptr_t *         vRoots;    // the roots of the window
    Vec_Ptr_t *         vSupp;     // the support of the window
    Vec_Ptr_t *         vNodes;    // the internal nodes of the window
    Vec_Ptr_t *         vDivs;     // the divisors of the node
    Vec_Int_t *         vDivLits;  // the SAT literals of divisor nodes
    Vec_Int_t *         vProjVarsCnf; // the projection variables
    Vec_Int_t *         vProjVarsSat; // the projection variables
    // intermediate simulation data
    Vec_Ptr_t *         vDivCexes; // the counter-example for dividors
    int                 nDivWords; // the number of words
    int                 nCexes;    // the numbe rof current counter-examples
    int                 nSatCalls; 
    int                 nSatCexes;
/*
    // intermediate AIG data
    Gia_Man_t *         pGia;      // replica of the AIG in the new package
//    Gia_Obj_t **        pSat2Gia;  // mapping of PO SAT var into internal GIA nodes
    Tas_Man_t *         pTas;      // the SAT solver
    Vec_Int_t *         vCex;      // the counter-example
    Vec_Ptr_t *         vGiaLits;  // literals given as assumptions
*/
    // used for bidecomposition
    Vec_Int_t *         vTruth;
    Bdc_Man_t *         pManDec;
    int                 nNodesDec;
    int                 nNodesGained;
    int                 nNodesGainedLevel;
    // solving data
    Aig_Man_t *         pAigWin;   // window AIG with constraints
    Cnf_Dat_t *         pCnf;      // the CNF for the window
    sat_solver *        pSat;      // the SAT solver used 
    Int_Man_t *         pMan;      // interpolation manager;
    Vec_Int_t *         vMem;      // memory for intermediate SOPs
    Vec_Vec_t *         vLevels;   // levelized structure for updating
    Vec_Ptr_t *         vMfsFanins;   // the new set of fanins
    int                 nTotConfLim; // total conflict limit
    int                 nTotConfLevel; // total conflicts on this level
    // switching activity
    Vec_Int_t *         vProbs; 
    // the result of solving
    int                 nFanins;   // the number of fanins
    int                 nWords;    // the number of words
    int                 nCares;    // the number of care minterms
    unsigned            uCare[(MFS_FANIN_MAX<=5)?1:1<<(MFS_FANIN_MAX-5)];  // the computed care-set
    // performance statistics
    int                 nTryRemoves; // number of fanin removals
    int                 nTryResubs;  // number of resubstitutions
    int                 nRemoves;    // number of fanin removals
    int                 nResubs;     // number of resubstitutions
    int                 nNodesTried;
    int                 nNodesResub;
    int                 nMintsCare;
    int                 nMintsTotal;
    int                 nNodesBad;
    int                 nTotalDivs;
    int                 nTimeOuts;
    int                 nTimeOutsLevel;
    int                 nDcMints;
    int                 nMaxDivs;
    double              dTotalRatios;
    // node/edge stats
    int                 nTotalNodesBeg;
    int                 nTotalNodesEnd;
    int                 nTotalEdgesBeg;
    int                 nTotalEdgesEnd;
    float               TotalSwitchingBeg;
    float               TotalSwitchingEnd;
    // statistics
    abctime             timeWin;
    abctime             timeDiv;
    abctime             timeAig;
    abctime             timeGia;
    abctime             timeCnf;
    abctime             timeSat;
    abctime             timeInt;
    abctime             timeTotal;
};

static inline float Abc_MfsObjProb( Mfs_Man_t * p, Abc_Obj_t * pObj ) { return (p->vProbs && pObj->Id < Vec_IntSize(p->vProbs))? Abc_Int2Float(Vec_IntEntry(p->vProbs,pObj->Id)) : 0.0; }

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== mfsDiv.c ==========================================================*/
extern Vec_Ptr_t *      Abc_MfsComputeDivisors( Mfs_Man_t * p, Abc_Obj_t * pNode, int nLevDivMax );
/*=== mfsInter.c ==========================================================*/
extern sat_solver *     Abc_MfsCreateSolverResub( Mfs_Man_t * p, int * pCands, int nCands, int fInvert );
extern Hop_Obj_t *      Abc_NtkMfsInterplate( Mfs_Man_t * p, int * pCands, int nCands );
extern int              Abc_NtkMfsInterplateEval( Mfs_Man_t * p, int * pCands, int nCands );
/*=== mfsMan.c ==========================================================*/
extern Mfs_Man_t *      Mfs_ManAlloc( Mfs_Par_t * pPars );
extern void             Mfs_ManStop( Mfs_Man_t * p );
extern void             Mfs_ManClean( Mfs_Man_t * p );
/*=== mfsResub.c ==========================================================*/
extern void             Abc_NtkMfsPrintResubStats( Mfs_Man_t * p );
extern int              Abc_NtkMfsEdgeSwapEval( Mfs_Man_t * p, Abc_Obj_t * pNode );
extern int              Abc_NtkMfsEdgePower( Mfs_Man_t * p, Abc_Obj_t * pNode );
extern int              Abc_NtkMfsResubNode( Mfs_Man_t * p, Abc_Obj_t * pNode );
extern int              Abc_NtkMfsResubNode2( Mfs_Man_t * p, Abc_Obj_t * pNode );
/*=== mfsSat.c ==========================================================*/
extern int              Abc_NtkMfsSolveSat( Mfs_Man_t * p, Abc_Obj_t * pNode );
extern int              Abc_NtkAddOneHotness( Mfs_Man_t * p );
/*=== mfsStrash.c ==========================================================*/
extern Aig_Man_t *      Abc_NtkConstructAig( Mfs_Man_t * p, Abc_Obj_t * pNode );
extern double           Abc_NtkConstraintRatio( Mfs_Man_t * p, Abc_Obj_t * pNode );
/*=== mfsWin.c ==========================================================*/
extern Vec_Ptr_t *      Abc_MfsComputeRoots( Abc_Obj_t * pNode, int nWinTfoMax, int nFanoutLimit );

/*=== mfsGia.c ==========================================================*/
extern void             Abc_NtkMfsConstructGia( Mfs_Man_t * p );
extern void             Abc_NtkMfsDeconstructGia( Mfs_Man_t * p );
extern int              Abc_NtkMfsTryResubOnceGia( Mfs_Man_t * p, int * pCands, int nCands );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

