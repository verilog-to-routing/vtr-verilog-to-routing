/**CFile****************************************************************

  FileName    [abs.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Abstraction package.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abs.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__proof_abs__Abs_h
#define ABC__proof_abs__Abs_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "aig/gia/gia.h"
#include "aig/gia/giaAig.h"
#include "aig/saig/saig.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

// abstraction parameters
typedef struct Abs_Par_t_ Abs_Par_t;
struct Abs_Par_t_
{
    int            nFramesMax;         // maximum frames
    int            nFramesStart;       // starting frame 
    int            nFramesPast;        // overlap frames
    int            nConfLimit;         // conflict limit
    int            nLearnedMax;        // max number of learned clauses
    int            nLearnedStart;      // max number of learned clauses
    int            nLearnedDelta;      // delta increase of learned clauses
    int            nLearnedPerce;      // percentage of clauses to leave
    int            nTimeOut;           // timeout in seconds
    int            nRatioMin;          // stop when less than this % of object is unabstracted
    int            nRatioMin2;         // stop when less than this % of object is unabstracted during refinement
    int            nRatioMax;          // restart when the number of abstracted object is more than this
    int            fUseTermVars;       // use terminal variables
    int            fUseRollback;       // use rollback to the starting number of frames
    int            fPropFanout;        // propagate fanout implications
    int            fAddLayer;          // refinement strategy by adding layers
    int            fNewRefine;         // uses new refinement heuristics
    int            fUseSkip;           // skip proving intermediate timeframes
    int            fUseSimple;         // use simple CNF construction
    int            fSkipHash;          // skip hashing CNF while unrolling
    int            fUseFullProof;      // use full proof for UNSAT cores
    int            fDumpVabs;          // dumps the abstracted model
    int            fDumpMabs;          // dumps the original AIG with abstraction map
    int            fCallProver;        // calls the prover
    int            fSimpProver;        // calls simplification before prover
    char *         pFileVabs;          // dumps the abstracted model into this file
    int            fVerbose;           // verbose flag
    int            fVeryVerbose;       // print additional information
    int            iFrame;             // the number of frames covered
    int            iFrameProved;       // the number of frames proved
    int            nFramesNoChange;    // the number of last frames without changes
    int            nFramesNoChangeLim; // the number of last frames without changes to dump abstraction
};

// old abstraction parameters
typedef struct Gia_ParAbs_t_ Gia_ParAbs_t;
struct Gia_ParAbs_t_
{
    int            Algo;               // the algorithm to be used
    int            nFramesMax;         // timeframes for PBA
    int            nConfMax;           // conflicts for PBA
    int            fDynamic;           // dynamic unfolding for PBA
    int            fConstr;            // use constraints
    int            nFramesBmc;         // timeframes for BMC
    int            nConfMaxBmc;        // conflicts for BMC
    int            nStableMax;         // the number of stable frames to quit
    int            nRatio;             // ratio of flops to quit
    int            TimeOut;            // approximate timeout in seconds
    int            TimeOutVT;          // approximate timeout in seconds
    int            nBobPar;            // Bob's parameter
    int            fUseBdds;           // use BDDs to refine abstraction
    int            fUseDprove;         // use 'dprove' to refine abstraction
    int            fUseStart;          // use starting frame
    int            fVerbose;           // verbose output
    int            fVeryVerbose;       // printing additional information
    int            Status;             // the problem status
    int            nFramesDone;        // the number of frames covered
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

static inline int         Ga2_ObjOffset( Gia_Man_t * p, Gia_Obj_t * pObj )          { return Vec_IntEntry(p->vMapping, Gia_ObjId(p, pObj));                                                         }
static inline int         Ga2_ObjLeaveNum( Gia_Man_t * p, Gia_Obj_t * pObj )        { return Vec_IntEntry(p->vMapping, Ga2_ObjOffset(p, pObj));                                                     }
static inline int *       Ga2_ObjLeavePtr( Gia_Man_t * p, Gia_Obj_t * pObj )        { return Vec_IntEntryP(p->vMapping, Ga2_ObjOffset(p, pObj) + 1);                                                }
static inline unsigned    Ga2_ObjTruth( Gia_Man_t * p, Gia_Obj_t * pObj )           { return (unsigned)Vec_IntEntry(p->vMapping, Ga2_ObjOffset(p, pObj) + Ga2_ObjLeaveNum(p, pObj) + 1);            }
static inline int         Ga2_ObjRefNum( Gia_Man_t * p, Gia_Obj_t * pObj )          { return (unsigned)Vec_IntEntry(p->vMapping, Ga2_ObjOffset(p, pObj) + Ga2_ObjLeaveNum(p, pObj) + 2);            }
static inline Vec_Int_t * Ga2_ObjLeaves( Gia_Man_t * p, Gia_Obj_t * pObj )          { static Vec_Int_t v; v.nSize = Ga2_ObjLeaveNum(p, pObj), v.pArray = Ga2_ObjLeavePtr(p, pObj); return &v;       }

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== abs.c =========================================================*/
/*=== absDup.c =========================================================*/
extern Gia_Man_t *       Gia_ManDupAbsFlops( Gia_Man_t * p, Vec_Int_t * vFlopClasses );
extern Gia_Man_t *       Gia_ManDupAbsGates( Gia_Man_t * p, Vec_Int_t * vGateClasses );
extern void              Gia_ManGlaCollect( Gia_Man_t * p, Vec_Int_t * vGateClasses, Vec_Int_t ** pvPis, Vec_Int_t ** pvPPis, Vec_Int_t ** pvFlops, Vec_Int_t ** pvNodes );
extern void              Gia_ManPrintFlopClasses( Gia_Man_t * p );
extern void              Gia_ManPrintObjClasses( Gia_Man_t * p );
extern void              Gia_ManPrintGateClasses( Gia_Man_t * p );
/*=== absGla.c =========================================================*/
extern int               Gia_ManPerformGla( Gia_Man_t * p, Abs_Par_t * pPars );
/*=== absGlaOld.c =========================================================*/
extern int               Gia_ManPerformGlaOld( Gia_Man_t * p, Abs_Par_t * pPars, int fStartVta );
/*=== absIter.c =========================================================*/
extern Gia_Man_t *       Gia_ManShrinkGla( Gia_Man_t * p, int nFrameMax, int nTimeOut, int fUsePdr, int fUseSat, int fUseBdd, int fVerbose );
/*=== absPth.c =========================================================*/
extern void              Gia_GlaProveAbsracted( Gia_Man_t * p, int fSimpProver, int fVerbose );
extern void              Gia_GlaProveCancel( int fVerbose );
extern int               Gia_GlaProveCheck( int fVerbose );
/*=== absVta.c =========================================================*/
extern int               Gia_VtaPerform( Gia_Man_t * pAig, Abs_Par_t * pPars );
/*=== absUtil.c =========================================================*/
extern void              Abs_ParSetDefaults( Abs_Par_t * p );
extern Vec_Int_t *       Gia_VtaConvertToGla( Gia_Man_t * p, Vec_Int_t * vVta );
extern Vec_Int_t *       Gia_VtaConvertFromGla( Gia_Man_t * p, Vec_Int_t * vGla, int nFrames );
extern Vec_Int_t *       Gia_FlaConvertToGla( Gia_Man_t * p, Vec_Int_t * vFla );
extern Vec_Int_t *       Gia_GlaConvertToFla( Gia_Man_t * p, Vec_Int_t * vGla );
extern int               Gia_GlaCountFlops( Gia_Man_t * p, Vec_Int_t * vGla );
extern int               Gia_GlaCountNodes( Gia_Man_t * p, Vec_Int_t * vGla );

/*=== absRpm.c =========================================================*/
extern Gia_Man_t *       Abs_RpmPerform( Gia_Man_t * p, int nCutMax, int fVerbose, int fVeryVerbose );
/*=== absRpmOld.c =========================================================*/
extern Gia_Man_t *       Abs_RpmPerformOld( Gia_Man_t * p, int fVerbose );
extern void              Gia_ManAbsSetDefaultParams( Gia_ParAbs_t * p );
extern Vec_Int_t *       Saig_ManCexAbstractionFlops( Aig_Man_t * p, Gia_ParAbs_t * pPars );

/*=== absOldCex.c ==========================================================*/
extern Vec_Int_t *       Saig_ManCbaFilterFlops( Aig_Man_t * pAig, Abc_Cex_t * pAbsCex, Vec_Int_t * vFlopClasses, Vec_Int_t * vAbsFfsToAdd, int nFfsToSelect );
extern Abc_Cex_t *       Saig_ManCbaFindCexCareBits( Aig_Man_t * pAig, Abc_Cex_t * pCex, int nInputs, int fVerbose );
/*=== absOldRef.c ==========================================================*/
extern int               Gia_ManCexAbstractionRefine( Gia_Man_t * pGia, Abc_Cex_t * pCex, int nFfToAddMax, int fTryFour, int fSensePath, int fVerbose );
/*=== absOldSat.c ==========================================================*/
extern Vec_Int_t *       Saig_ManExtendCounterExampleTest3( Aig_Man_t * pAig, int iFirstFlopPi, Abc_Cex_t * pCex, int fVerbose );
/*=== absOldSim.c ==========================================================*/
extern Vec_Int_t *       Saig_ManExtendCounterExampleTest2( Aig_Man_t * p, int iFirstPi, Abc_Cex_t * pCex, int fVerbose );


ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

