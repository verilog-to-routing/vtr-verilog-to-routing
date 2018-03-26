/**CFile****************************************************************

  FileName    [fra.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [[New FRAIG package.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 30, 2007.]

  Revision    [$Id: fra.h,v 1.00 2007/06/30 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__aig__fra__fra_h
#define ABC__aig__fra__fra_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "misc/vec/vec.h"
#include "aig/aig/aig.h"
#include "opt/dar/dar.h"
#include "sat/bsat/satSolver.h"
#include "aig/ioa/ioa.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START
 

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Fra_Par_t_   Fra_Par_t;
typedef struct Fra_Ssw_t_   Fra_Ssw_t;
typedef struct Fra_Sec_t_   Fra_Sec_t;
typedef struct Fra_Man_t_   Fra_Man_t;
typedef struct Fra_Cla_t_   Fra_Cla_t;
typedef struct Fra_Sml_t_   Fra_Sml_t;
typedef struct Fra_Bmc_t_   Fra_Bmc_t;

// FRAIG parameters
struct Fra_Par_t_
{
    int              nSimWords;         // the number of words in the simulation info
    double           dSimSatur;         // the ratio of refined classes when saturation is reached
    int              fPatScores;        // enables simulation pattern scoring
    int              MaxScore;          // max score after which resimulation is used
    double           dActConeRatio;     // the ratio of cone to be bumped
    double           dActConeBumpMax;   // the largest bump in activity
    int              fChoicing;         // enables choicing
    int              fSpeculate;        // use speculative reduction
    int              fProve;            // prove the miter outputs
    int              fVerbose;          // verbose output
    int              fDoSparse;         // skip sparse functions
    int              fConeBias;         // bias variables in the cone (good for unsat runs)
    int              nBTLimitNode;      // conflict limit at a node
    int              nBTLimitMiter;     // conflict limit at an output
    int              nLevelMax;         // the max level to consider seriously
    int              nFramesP;          // the number of timeframes to in the prefix
    int              nFramesK;          // the number of timeframes to unroll
    int              nMaxImps;          // the maximum number of implications to consider
    int              nMaxLevs;          // the maximum number of levels to consider
    int              fRewrite;          // use rewriting for constraint reduction
    int              fLatchCorr;        // computes latch correspondence only
    int              fUseImps;          // use implications
    int              fUse1Hot;          // use one-hotness conditions
    int              fWriteImps;        // record implications
    int              fDontShowBar;      // does not show progressbar during fraiging
};

// seq SAT sweeping parameters
struct Fra_Ssw_t_
{
    int              nPartSize;         // size of the partition
    int              nOverSize;         // size of the overlap between partitions
    int              nFramesP;          // number of frames in the prefix
    int              nFramesK;          // number of frames for induction (1=simple) 
    int              nMaxImps;          // max implications to consider
    int              nMaxLevs;          // max levels to consider
    int              nMinDomSize;       // min clock domain considered for optimization
    int              fUseImps;          // use implications  
    int              fRewrite;          // enable rewriting of the specualatively reduced model
    int              fFraiging;         // enable comb SAT sweeping as preprocessing 
    int              fLatchCorr;        // perform register correspondence
    int              fWriteImps;        // write implications into a file
    int              fUse1Hot;          // use one-hotness constraints
    int              fVerbose;          // enable verbose output 
    int              fSilent;           // disable any output 
    int              nIters;            // the number of iterations performed
    float            TimeLimit;         // the runtime budget for this call
};

// SEC parametesr
struct Fra_Sec_t_
{
    int              fTryComb;          // try CEC call as a preprocessing step
    int              fTryBmc;           // try BMC call as a preprocessing step 
    int              nFramesMax;        // the max number of frames used for induction
    int              nBTLimit;          // the conflict limit at a node
    int              nBTLimitGlobal;    // the global conflict limit
    int              nBTLimitInter;     // the conflict limit for interpolation
    int              nBddVarsMax;       // the state space limit for BDD reachability
    int              nBddMax;           // the max number of BDD nodes
    int              nBddIterMax;       // the limit on the number of BDD iterations
    int              nPdrTimeout;       // the timeout for PDR in the end
    int              fPhaseAbstract;    // enables phase abstraction
    int              fRetimeFirst;      // enables most-forward retiming at the beginning
    int              fRetimeRegs;       // enables min-register retiming at the beginning
    int              fFraiging;         // enables fraiging at the beginning
    int              fInduction;        // enable the use of induction
    int              fInterpolation;    // enables interpolation
    int              fInterSeparate;    // enables interpolation for each outputs separately
    int              fReachability;     // enables BDD based reachability
    int              fReorderImage;     // enables BDD reordering during image computation
    int              fStopOnFirstFail;  // enables stopping after first output of a miter has failed to prove
    int              fUseNewProver;     // the new prover
    int              fUsePdr;           // the PDR
    int              fSilent;           // disables all output
    int              fVerbose;          // enables verbose reporting of statistics
    int              fVeryVerbose;      // enables very verbose reporting  
    int              TimeLimit;         // enables the timeout
    int              fReadUnsolved;     // inserts the unsolved model back
    int              nSMnumber;         // the number of model written
    // internal parameters
    int              fRecursive;        // set to 1 when SEC is called recursively
    int              fReportSolution;   // enables report solution in a special form
};

// FRAIG equivalence classes
struct Fra_Cla_t_
{
    Aig_Man_t *      pAig;              // the original AIG manager
    Aig_Obj_t **     pMemRepr;          // pointers to representatives of each node
    Vec_Ptr_t *      vClasses;          // equivalence classes
    Vec_Ptr_t *      vClasses1;         // equivalence class of Const1 node
    Vec_Ptr_t *      vClassesTemp;      // temporary storage for new classes
    Aig_Obj_t **     pMemClasses;       // memory allocated for equivalence classes
    Aig_Obj_t **     pMemClassesFree;   // memory allocated for equivalence classes to be used
    Vec_Ptr_t *      vClassOld;         // old equivalence class after splitting
    Vec_Ptr_t *      vClassNew;         // new equivalence class(es) after splitting
    int              nPairs;            // the number of pairs of nodes
    int              fRefinement;       // set to 1 when refinement has happened
    Vec_Int_t *      vImps;             // implications
    // procedures used for class refinement
    int (*pFuncNodeHash)     (Aig_Obj_t *, int);         // returns has key of the node
    int (*pFuncNodeIsConst)  (Aig_Obj_t *);              // returns 1 if the node is a constant
    int (*pFuncNodesAreEqual)(Aig_Obj_t *, Aig_Obj_t *); // returns 1 if nodes are equal up to a complement
};

// simulation manager
struct Fra_Sml_t_
{
    Aig_Man_t *      pAig;              // the original AIG manager
    int              nPref;             // the number of times frames in the prefix
    int              nFrames;           // the number of times frames 
    int              nWordsFrame;       // the number of words in each time frame
    int              nWordsTotal;       // the total number of words at a node
    int              nWordsPref;        // the number of word in the prefix
    int              fNonConstOut;      // have seen a non-const-0 output during simulation
    int              nSimRounds;        // statistics
    int              timeSim;           // statistics
    unsigned         pData[0];          // simulation data for the nodes
};

// FRAIG manager
struct Fra_Man_t_
{
    // high-level data    
    Fra_Par_t *      pPars;             // parameters governing fraiging
    // AIG managers
    Aig_Man_t *      pManAig;           // the starting AIG manager
    Aig_Man_t *      pManFraig;         // the final AIG manager
    // mapping AIG into FRAIG
    int              nFramesAll;        // the number of timeframes used
    Aig_Obj_t **     pMemFraig;         // memory allocated for points to the fraig nodes
    int              nSizeAlloc;        // allocated size of the arrays for timeframe nodes
    // equivalence classes 
    Fra_Cla_t *      pCla;              // representation of (candidate) equivalent nodes
    // simulation info
    Fra_Sml_t *      pSml;              // simulation manager
    // bounded model checking manager
    Fra_Bmc_t *      pBmc;
    // counter example storage
    int              nPatWords;         // the number of words in the counter example
    unsigned *       pPatWords;         // the counter example
    Vec_Int_t *      vCex;
    // one-hotness conditions
    Vec_Int_t *      vOneHots;          
    // satisfiability solving
    sat_solver *     pSat;              // SAT solver
    int              nSatVars;          // the number of variables currently used
    Vec_Ptr_t *      vPiVars;           // the PIs of the cone used 
    ABC_INT64_T           nBTLimitGlobal;    // resource limit
    ABC_INT64_T           nInsLimitGlobal;   // resource limit
    Vec_Ptr_t **     pMemFanins;        // the arrays of fanins for some FRAIG nodes
    int *            pMemSatNums;       // the array of SAT numbers for some FRAIG nodes
    int              nMemAlloc;         // allocated size of the arrays for FRAIG varnums and fanins
    Vec_Ptr_t *      vTimeouts;         // the nodes, for which equivalence checking timed out
    // statistics
    int              nSimRounds;
    int              nNodesMiter;
    int              nLitsBeg;
    int              nLitsEnd;
    int              nNodesBeg;
    int              nNodesEnd;
    int              nRegsBeg;
    int              nRegsEnd;
    int              nSatCalls;
    int              nSatCallsSat;
    int              nSatCallsUnsat;
    int              nSatProof;
    int              nSatFails;
    int              nSatFailsReal;
    int              nSpeculs;   
    int              nChoices;
    int              nChoicesFake;
    int              nSatCallsRecent;
    int              nSatCallsSkipped;
    // runtime
    abctime          timeSim;
    abctime          timeTrav;
    abctime          timeRwr;
    abctime          timeSat;
    abctime          timeSatUnsat;
    abctime          timeSatSat;
    abctime          timeSatFail;
    abctime          timeRef;
    abctime          timeTotal;
    abctime          time1;
    abctime          time2;
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

static inline unsigned *   Fra_ObjSim( Fra_Sml_t * p, int Id )                           { return p->pData + p->nWordsTotal * Id; }
static inline unsigned     Fra_ObjRandomSim()                                            { return Aig_ManRandom(0);               }

static inline Aig_Obj_t *  Fra_ObjFraig( Aig_Obj_t * pObj, int i )                       { return ((Fra_Man_t *)pObj->pData)->pMemFraig[((Fra_Man_t *)pObj->pData)->nFramesAll*pObj->Id + i];  }
static inline void         Fra_ObjSetFraig( Aig_Obj_t * pObj, int i, Aig_Obj_t * pNode ) { ((Fra_Man_t *)pObj->pData)->pMemFraig[((Fra_Man_t *)pObj->pData)->nFramesAll*pObj->Id + i] = pNode; }

static inline Vec_Ptr_t *  Fra_ObjFaninVec( Aig_Obj_t * pObj )                           { return ((Fra_Man_t *)pObj->pData)->pMemFanins[pObj->Id];      }
static inline void         Fra_ObjSetFaninVec( Aig_Obj_t * pObj, Vec_Ptr_t * vFanins )   { ((Fra_Man_t *)pObj->pData)->pMemFanins[pObj->Id] = vFanins;   }

static inline int          Fra_ObjSatNum( Aig_Obj_t * pObj )                             { return ((Fra_Man_t *)pObj->pData)->pMemSatNums[pObj->Id];     }
static inline void         Fra_ObjSetSatNum( Aig_Obj_t * pObj, int Num )                 { ((Fra_Man_t *)pObj->pData)->pMemSatNums[pObj->Id] = Num;      }

static inline Aig_Obj_t *  Fra_ClassObjRepr( Aig_Obj_t * pObj )                          { return ((Fra_Man_t *)pObj->pData)->pCla->pMemRepr[pObj->Id];  }
static inline void         Fra_ClassObjSetRepr( Aig_Obj_t * pObj, Aig_Obj_t * pNode )    { ((Fra_Man_t *)pObj->pData)->pCla->pMemRepr[pObj->Id] = pNode; }

static inline Aig_Obj_t *  Fra_ObjChild0Fra( Aig_Obj_t * pObj, int i ) { assert( !Aig_IsComplement(pObj) ); return Aig_ObjFanin0(pObj)? Aig_NotCond(Fra_ObjFraig(Aig_ObjFanin0(pObj),i), Aig_ObjFaninC0(pObj)) : NULL;  }
static inline Aig_Obj_t *  Fra_ObjChild1Fra( Aig_Obj_t * pObj, int i ) { assert( !Aig_IsComplement(pObj) ); return Aig_ObjFanin1(pObj)? Aig_NotCond(Fra_ObjFraig(Aig_ObjFanin1(pObj),i), Aig_ObjFaninC1(pObj)) : NULL;  }

static inline int          Fra_ImpLeft( int Imp )                                        { return Imp & 0xFFFF;         }
static inline int          Fra_ImpRight( int Imp )                                       { return Imp >> 16;            }
static inline int          Fra_ImpCreate( int Left, int Right )                          { return (Right << 16) | Left; }

////////////////////////////////////////////////////////////////////////
///                         ITERATORS                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== fraCec.c ========================================================*/
extern int                 Fra_FraigSat( Aig_Man_t * pMan, ABC_INT64_T nConfLimit, ABC_INT64_T nInsLimit, int nLearnedStart, int nLearnedDelta, int nLearnedPerce, int fFlipBits, int fAndOuts, int fNewSolver, int fVerbose );
extern int                 Fra_FraigCec( Aig_Man_t ** ppAig, int nConfLimit, int fVerbose );
extern int                 Fra_FraigCecPartitioned( Aig_Man_t * pMan1, Aig_Man_t * pMan2, int nConfLimit, int nPartSize, int fSmart, int fVerbose );
/*=== fraClass.c ========================================================*/
extern int                 Fra_BmcNodeIsConst( Aig_Obj_t * pObj );
extern int                 Fra_BmcNodesAreEqual( Aig_Obj_t * pObj0, Aig_Obj_t * pObj1 );
extern void                Fra_BmcStop( Fra_Bmc_t * p );
extern void                Fra_BmcPerform( Fra_Man_t * p, int nPref, int nDepth );
extern void                Fra_BmcPerformSimple( Aig_Man_t * pAig, int nFrames, int nBTLimit, int fRewrite, int fVerbose );
/*=== fraClass.c ========================================================*/
extern Fra_Cla_t *         Fra_ClassesStart( Aig_Man_t * pAig );
extern void                Fra_ClassesStop( Fra_Cla_t * p );
extern void                Fra_ClassesCopyReprs( Fra_Cla_t * p, Vec_Ptr_t * vFailed );
extern void                Fra_ClassesPrint( Fra_Cla_t * p, int fVeryVerbose );
extern void                Fra_ClassesPrepare( Fra_Cla_t * p, int fLatchCorr, int nMaxLevs );
extern int                 Fra_ClassesRefine( Fra_Cla_t * p );
extern int                 Fra_ClassesRefine1( Fra_Cla_t * p, int fRefineNewClass, int * pSkipped );
extern int                 Fra_ClassesCountLits( Fra_Cla_t * p );
extern int                 Fra_ClassesCountPairs( Fra_Cla_t * p );
extern void                Fra_ClassesTest( Fra_Cla_t * p, int Id1, int Id2 );
extern void                Fra_ClassesLatchCorr( Fra_Man_t * p );
extern void                Fra_ClassesPostprocess( Fra_Cla_t * p );
extern void                Fra_ClassesSelectRepr( Fra_Cla_t * p );
extern Aig_Man_t *         Fra_ClassesDeriveAig( Fra_Cla_t * p, int nFramesK );
/*=== fraCnf.c ========================================================*/
extern void                Fra_CnfNodeAddToSolver( Fra_Man_t * p, Aig_Obj_t * pOld, Aig_Obj_t * pNew );
/*=== fraCore.c ========================================================*/
extern void                Fra_FraigSweep( Fra_Man_t * pManAig );
extern int                 Fra_FraigMiterStatus( Aig_Man_t * p );
extern int                 Fra_FraigMiterAssertedOutput( Aig_Man_t * p );
extern Aig_Man_t *         Fra_FraigPerform( Aig_Man_t * pManAig, Fra_Par_t * pPars );
extern Aig_Man_t *         Fra_FraigChoice( Aig_Man_t * pManAig, int nConfMax, int nLevelMax );
extern Aig_Man_t *         Fra_FraigEquivence( Aig_Man_t * pManAig, int nConfMax, int fProve );
/*=== fraHot.c ========================================================*/
extern Vec_Int_t *         Fra_OneHotCompute( Fra_Man_t * p, Fra_Sml_t * pSim );
extern void                Fra_OneHotAssume( Fra_Man_t * p, Vec_Int_t * vOneHots );
extern void                Fra_OneHotCheck( Fra_Man_t * p, Vec_Int_t * vOneHots );
extern int                 Fra_OneHotRefineUsingCex( Fra_Man_t * p, Vec_Int_t * vOneHots );
extern int                 Fra_OneHotCount( Fra_Man_t * p, Vec_Int_t * vOneHots );
extern void                Fra_OneHotEstimateCoverage( Fra_Man_t * p, Vec_Int_t * vOneHots );
extern Aig_Man_t *         Fra_OneHotCreateExdc( Fra_Man_t * p, Vec_Int_t * vOneHots );
extern void                Fra_OneHotAddKnownConstraint( Fra_Man_t * p, Vec_Ptr_t * vOnehots );
/*=== fraImp.c ========================================================*/
extern Vec_Int_t *         Fra_ImpDerive( Fra_Man_t * p, int nImpMaxLimit, int nImpUseLimit, int fLatchCorr );
extern void                Fra_ImpAddToSolver( Fra_Man_t * p, Vec_Int_t * vImps, int * pSatVarNums );
extern int                 Fra_ImpCheckForNode( Fra_Man_t * p, Vec_Int_t * vImps, Aig_Obj_t * pNode, int Pos );
extern int                 Fra_ImpRefineUsingCex( Fra_Man_t * p, Vec_Int_t * vImps );
extern void                Fra_ImpCompactArray( Vec_Int_t * vImps );
extern double              Fra_ImpComputeStateSpaceRatio( Fra_Man_t * p );
extern int                 Fra_ImpVerifyUsingSimulation( Fra_Man_t * p );
extern void                Fra_ImpRecordInManager( Fra_Man_t * p, Aig_Man_t * pNew );
/*=== fraInd.c ========================================================*/
extern Aig_Man_t *         Fra_FraigInduction( Aig_Man_t * p, Fra_Ssw_t * pPars );
/*=== fraIndVer.c =====================================================*/
extern int                 Fra_InvariantVerify( Aig_Man_t * p, int nFrames, Vec_Int_t * vClauses, Vec_Int_t * vLits );
/*=== fraLcr.c ========================================================*/
extern Aig_Man_t *         Fra_FraigLatchCorrespondence( Aig_Man_t * pAig, int nFramesP, int nConfMax, int fProve, int fVerbose, int * pnIter, float TimeLimit );
/*=== fraMan.c ========================================================*/
extern void                Fra_ParamsDefault( Fra_Par_t * pParams );
extern void                Fra_ParamsDefaultSeq( Fra_Par_t * pParams );
extern Fra_Man_t *         Fra_ManStart( Aig_Man_t * pManAig, Fra_Par_t * pParams );
extern void                Fra_ManClean( Fra_Man_t * p, int nNodesMax );
extern Aig_Man_t *         Fra_ManPrepareComb( Fra_Man_t * p );
extern void                Fra_ManFinalizeComb( Fra_Man_t * p );
extern void                Fra_ManStop( Fra_Man_t * p );
extern void                Fra_ManPrint( Fra_Man_t * p );
/*=== fraSat.c ========================================================*/
extern int                 Fra_NodesAreEquiv( Fra_Man_t * p, Aig_Obj_t * pOld, Aig_Obj_t * pNew );
extern int                 Fra_NodesAreImp( Fra_Man_t * p, Aig_Obj_t * pOld, Aig_Obj_t * pNew, int fComplL, int fComplR );
extern int                 Fra_NodesAreClause( Fra_Man_t * p, Aig_Obj_t * pOld, Aig_Obj_t * pNew, int fComplL, int fComplR );
extern int                 Fra_NodeIsConst( Fra_Man_t * p, Aig_Obj_t * pNew );
/*=== fraSec.c ========================================================*/
extern void                Fra_SecSetDefaultParams( Fra_Sec_t * p );
extern int                 Fra_FraigSec( Aig_Man_t * p, Fra_Sec_t * pParSec, Aig_Man_t ** ppResult );
/*=== fraSim.c ========================================================*/
extern int                 Fra_SmlNodeHash( Aig_Obj_t * pObj, int nTableSize );
extern int                 Fra_SmlNodeIsConst( Aig_Obj_t * pObj );
extern int                 Fra_SmlNodesAreEqual( Aig_Obj_t * pObj0, Aig_Obj_t * pObj1 );
extern int                 Fra_SmlNodeNotEquWeight( Fra_Sml_t * p, int Left, int Right );
extern int                 Fra_SmlNodeCountOnes( Fra_Sml_t * p, Aig_Obj_t * pObj );
extern int                 Fra_SmlCheckOutput( Fra_Man_t * p );
extern void                Fra_SmlSavePattern( Fra_Man_t * p );
extern void                Fra_SmlSimulate( Fra_Man_t * p, int fInit );
extern void                Fra_SmlResimulate( Fra_Man_t * p );
extern Fra_Sml_t *         Fra_SmlStart( Aig_Man_t * pAig, int nPref, int nFrames, int nWordsFrame );
extern void                Fra_SmlStop( Fra_Sml_t * p );
extern Fra_Sml_t *         Fra_SmlSimulateComb( Aig_Man_t * pAig, int nWords, int fCheckMiter );
extern Fra_Sml_t *         Fra_SmlSimulateCombGiven( Aig_Man_t * pAig, char * pFileName, int fCheckMiter, int fVerbose );
extern Fra_Sml_t *         Fra_SmlSimulateSeq( Aig_Man_t * pAig, int nPref, int nFrames, int nWords, int fCheckMiter );
extern Abc_Cex_t *         Fra_SmlGetCounterExample( Fra_Sml_t * p );
extern Abc_Cex_t *         Fra_SmlCopyCounterExample( Aig_Man_t * pAig, Aig_Man_t * pFrames, int * pModel );

ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

