/**CFile****************************************************************

  FileName    [ssw.h] 

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Inductive prover with constraints.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 1, 2008.]

  Revision    [$Id: ssw.h,v 1.00 2008/09/01 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__aig__ssw__ssw_h
#define ABC__aig__ssw__ssw_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

// choicing parameters
typedef struct Ssw_Pars_t_ Ssw_Pars_t;
struct Ssw_Pars_t_
{
    int              nPartSize;     // size of the partition
    int              nOverSize;     // size of the overlap between partitions
    int              nFramesK;      // the induction depth
    int              nFramesAddSim; // the number of additional frames to simulate
    int              fConstrs;      // treat the last nConstrs POs as seq constraints
    int              fMergeFull;    // enables full merge when constraints are used
    int              nMaxLevs;      // the max number of levels of nodes to consider
    int              nBTLimit;      // conflict limit at a node
    int              nBTLimitGlobal;// conflict limit for multiple runs
    int              nMinDomSize;   // min clock domain considered for optimization
    int              nItersStop;    // stop after the given number of iterations
    int              fDumpSRInit;   // dumps speculative reduction
    int              nResimDelta;   // the number of nodes to resimulate
    int              nStepsMax;     // (scorr only) the max number of induction steps
    int              TimeLimit;     // time out in seconds
    int              fPolarFlip;    // uses polarity adjustment
    int              fLatchCorr;    // perform register correspondence
    int              fConstCorr;    // perform constant correspondence
    int              fOutputCorr;   // perform 'PO correspondence'
    int              fSemiFormal;   // enable semiformal filtering
//    int              fUniqueness;   // enable uniqueness constraints
    int              fDynamic;      // enable dynamic addition of constraints
    int              fLocalSim;     // enable local simulation simulation
    int              fPartSigCorr;  // uses partial signal correspondence
    int              nIsleDist;     // extends islands by the given distance
    int              fScorrGia;     // new signal correspondence implementation
    int              fUseCSat;      // new SAT solver using when fScorrGia is selected
    int              fVerbose;      // verbose stats
    int              fFlopVerbose;  // verbose printout of redundant flops
    int              fEquivDump;    // enables dumping equivalences
    int              fEquivDump2;   // enables dumping equivalences
    int              fStopWhenGone; // stop when PO output is not a candidate constant
    // optimized latch correspondence
    int              fLatchCorrOpt; // perform register correspondence (optimized)
    int              nSatVarMax;    // max number of SAT vars before recycling SAT solver (optimized latch corr only)
    int              nRecycleCalls; // calls to perform before recycling SAT solver (optimized latch corr only)
    // optimized signal correspondence
    int              nSatVarMax2;   // max number of SAT vars before recycling SAT solver (optimized latch corr only)
    int              nRecycleCalls2;// calls to perform before recycling SAT solver (optimized latch corr only)
    // internal parameters
    int              nIters;        // the number of iterations performed
    int              nConflicts;    // the total number of conflicts performed
    // callback
    void *           pData;
    void *           pFunc;
};

typedef struct Ssw_RarPars_t_ Ssw_RarPars_t;
struct Ssw_RarPars_t_
{
    int              nFrames;
    int              nWords;
    int              nBinSize;
    int              nRounds;
    int              nRestart;
    int              nRandSeed;
    int              TimeOut;
    int              TimeOutGap;
    int              fSolveAll;
    int              fSetLastState;
    int              fVerbose;
    int              fNotVerbose;
    int              fSilent;
    int              fDropSatOuts;
    int              fMiter;
    int              fUseCex;
    int              fLatchOnly;
    int              fUseFfGrouping;
    int              nSolved;
    Abc_Cex_t *      pCex;
    int(*pFuncOnFail)(int,Abc_Cex_t*); // called for a failed output in MO mode
};

typedef struct Ssw_Sml_t_ Ssw_Sml_t; // sequential simulation manager

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== sswBmc.c ==========================================================*/
extern int           Ssw_BmcDynamic( Aig_Man_t * pAig, int nFramesMax, int nConfLimit, int fVerbose, int * piFrame );
/*=== sswConstr.c ==========================================================*/
extern int           Ssw_ManSetConstrPhases( Aig_Man_t * p, int nFrames, Vec_Int_t ** pvInits );
/*=== sswCore.c ==========================================================*/
extern void          Ssw_ManSetDefaultParams( Ssw_Pars_t * p );
extern void          Ssw_ManSetDefaultParamsLcorr( Ssw_Pars_t * p );
extern Aig_Man_t *   Ssw_SignalCorrespondence( Aig_Man_t * pAig, Ssw_Pars_t * pPars );
extern Aig_Man_t *   Ssw_LatchCorrespondence( Aig_Man_t * pAig, Ssw_Pars_t * pPars );
/*=== sswIslands.c ==========================================================*/
extern int           Ssw_SecWithSimilarityPairs( Aig_Man_t * p0, Aig_Man_t * p1, Vec_Int_t * vPairs, Ssw_Pars_t * pPars );
extern int           Ssw_SecWithSimilarity( Aig_Man_t * p0, Aig_Man_t * p1, Ssw_Pars_t * pPars );
/*=== sswMiter.c ===================================================*/
/*=== sswPart.c ==========================================================*/
extern Aig_Man_t *   Ssw_SignalCorrespondencePart( Aig_Man_t * pAig, Ssw_Pars_t * pPars );
/*=== sswPairs.c ===================================================*/
extern int           Ssw_MiterStatus( Aig_Man_t * p, int fVerbose );
extern int           Ssw_SecWithPairs( Aig_Man_t * pAig1, Aig_Man_t * pAig2, Vec_Int_t * vIds1, Vec_Int_t * vIds2, Ssw_Pars_t * pPars );
extern int           Ssw_SecGeneral( Aig_Man_t * pAig1, Aig_Man_t * pAig2, Ssw_Pars_t * pPars );
extern int           Ssw_SecGeneralMiter( Aig_Man_t * pMiter, Ssw_Pars_t * pPars );
/*=== sswRarity.c ===================================================*/
extern void          Ssw_RarSetDefaultParams( Ssw_RarPars_t * p );
extern int           Ssw_RarSignalFilter( Aig_Man_t * pAig, Ssw_RarPars_t * pPars );
extern int           Ssw_RarSimulate( Aig_Man_t * pAig, Ssw_RarPars_t * pPars );
/*=== sswSim.c ===================================================*/
extern Ssw_Sml_t *   Ssw_SmlSimulateComb( Aig_Man_t * pAig, int nWords );
extern Ssw_Sml_t *   Ssw_SmlSimulateSeq( Aig_Man_t * pAig, int nPref, int nFrames, int nWords );
extern void          Ssw_SmlUnnormalize( Ssw_Sml_t * p );
extern void          Ssw_SmlStop( Ssw_Sml_t * p );
extern int           Ssw_SmlNumFrames( Ssw_Sml_t * p );
extern int           Ssw_SmlNumWordsTotal( Ssw_Sml_t * p );
extern unsigned *    Ssw_SmlSimInfo( Ssw_Sml_t * p, Aig_Obj_t * pObj );
extern int           Ssw_SmlObjsAreEqualWord( Ssw_Sml_t * p, Aig_Obj_t * pObj0, Aig_Obj_t * pObj1 );
extern void          Ssw_SmlInitializeSpecial( Ssw_Sml_t * p, Vec_Int_t * vInit );
extern int           Ssw_SmlCheckNonConstOutputs( Ssw_Sml_t * p );
extern Vec_Ptr_t *   Ssw_SmlSimDataPointers( Ssw_Sml_t * p );


ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

