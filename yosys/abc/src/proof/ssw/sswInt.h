/**CFile****************************************************************

  FileName    [sswInt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Inductive prover with constraints.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 1, 2008.]

  Revision    [$Id: sswInt.h,v 1.00 2008/09/01 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__aig__ssw__sswInt_h
#define ABC__aig__ssw__sswInt_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "aig/saig/saig.h"
#include "sat/bsat/satSolver.h"
#include "ssw.h"
#include "aig/ioa/ioa.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Ssw_Man_t_ Ssw_Man_t; // signal correspondence manager
typedef struct Ssw_Frm_t_ Ssw_Frm_t; // unrolled frames manager
typedef struct Ssw_Sat_t_ Ssw_Sat_t; // SAT solver manager
typedef struct Ssw_Cla_t_ Ssw_Cla_t; // equivalence classe manager

struct Ssw_Man_t_
{
    // parameters
    Ssw_Pars_t *     pPars;          // parameters
    int              nFrames;        // for quick lookup
    // AIGs used in the package
    Aig_Man_t *      pAig;           // user-given AIG
    Aig_Man_t *      pFrames;        // final AIG
    Aig_Obj_t **     pNodeToFrames;  // mapping of AIG nodes into FRAIG nodes
    // equivalence classes
    Ssw_Cla_t *      ppClasses;      // equivalence classes of nodes
    int              fRefined;       // is set to 1 when refinement happens
    // SAT solving 
    Ssw_Sat_t *      pMSatBmc;       // SAT manager for base case
    Ssw_Sat_t *      pMSat;          // SAT manager for inductive case
    // SAT solving (latch corr only)
    Vec_Ptr_t *      vSimInfo;       // simulation information for the framed PIs
    int              nPatterns;      // the number of patterns saved
    int              nSimRounds;     // the number of simulation rounds performed
    int              nCallsCount;    // the number of calls in this round
    int              nCallsDelta;    // the number of calls to skip
    int              nCallsSat;      // the number of SAT calls in this round
    int              nCallsUnsat;    // the number of UNSAT calls in this round
    int              nRecycleCalls;  // the number of calls since last recycling
    int              nRecycles;      // the number of time SAT solver was recycled
    int              nRecyclesTotal; // the number of time SAT solver was recycled
    int              nVarsMax;       // the maximum variables in the solver
    int              nCallsMax;      // the maximum number of SAT calls
    // uniqueness
    Vec_Ptr_t *      vCommon;        // the set of common variables in the logic cones
    int              iOutputLit;     // the output literal of the uniqueness constraint
    Vec_Int_t *      vDiffPairs;     // is set to 1 if reg pair can be diff
    int              nUniques;       // the number of uniqueness constraints used 
    int              nUniquesAdded;  // useful uniqueness constraints
    int              nUniquesUseful; // useful uniqueness constraints
    // dynamic constraint addition
    int              nSRMiterMaxId;  // max ID after which the last frame begins
    Vec_Ptr_t *      vNewLos;        // new time frame LOs of to constrain
    Vec_Int_t *      vNewPos;        // new time frame POs of to add constraints
    int *            pVisited;       // flags to label visited nodes in each frame
    int              nVisCounter;    // the traversal ID
    // sequential simulation
    Ssw_Sml_t *      pSml;           // the simulator
    int              iNodeStart;     // the first node considered
    int              iNodeLast;      // the last node considered
    Vec_Ptr_t *      vResimConsts;   // resimulation constants
    Vec_Ptr_t *      vResimClasses;  // resimulation classes
    Vec_Int_t *      vInits;         // the init values of primary inputs under constraints
    // counter example storage
    int              nPatWords;      // the number of words in the counter example
    unsigned *       pPatWords;      // the counter example
    // constraints
    int              nConstrTotal;   // the number of total constraints
    int              nConstrReduced; // the number of reduced constraints
    int              nStrangers;     // the number of strange situations
    // SAT calls statistics
    int              nSatCalls;      // the number of SAT calls
    int              nSatProof;      // the number of proofs
    int              nSatFailsReal;  // the number of timeouts
    int              nSatCallsUnsat; // the number of unsat SAT calls
    int              nSatCallsSat;   // the number of sat SAT calls
    // node/register/lit statistics
    int              nLitsBeg;
    int              nLitsEnd;
    int              nNodesBeg;
    int              nNodesEnd;
    int              nRegsBeg;
    int              nRegsEnd;
    // equiv statistis
    int              nConesTotal;
    int              nConesConstr;
    int              nEquivsTotal;
    int              nEquivsConstr;
    int              nNodesBegC;
    int              nNodesEndC;
    int              nRegsBegC;
    int              nRegsEndC;
    // runtime stats
    abctime          timeBmc;        // bounded model checking
    abctime          timeReduce;     // speculative reduction
    abctime          timeMarkCones;  // marking the cones not to be refined
    abctime          timeSimSat;     // simulation of the counter-examples
    abctime          timeSat;        // solving SAT
    abctime          timeSatSat;     // sat
    abctime          timeSatUnsat;   // unsat
    abctime          timeSatUndec;   // undecided
    abctime          timeOther;      // other runtime
    abctime          timeTotal;      // total runtime
};

// internal SAT manager
struct Ssw_Sat_t_
{
    Aig_Man_t *      pAig;           // the AIG manager
    int              fPolarFlip;     // flips polarity 
    sat_solver *     pSat;           // recyclable SAT solver
    int              nSatVars;       // the counter of SAT variables
    Vec_Int_t *      vSatVars;       // mapping of each node into its SAT var
    Vec_Ptr_t *      vFanins;        // fanins of the CNF node
    Vec_Ptr_t *      vUsedPis;       // the PIs with SAT variables 
    int              nSolverCalls;   // the total number of SAT calls
};

// internal frames manager
struct Ssw_Frm_t_
{
    Aig_Man_t *      pAig;           // user-given AIG
    int              nObjs;          // offset in terms of AIG nodes
    int              nFrames;        // the number of frames in current unrolling
    Aig_Man_t *      pFrames;        // unrolled AIG
    Vec_Ptr_t *      vAig2Frm;       // mapping of AIG nodes into frame nodes
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

static inline int  Ssw_ObjSatNum( Ssw_Sat_t * p, Aig_Obj_t * pObj )             { return Vec_IntGetEntry( p->vSatVars, pObj->Id );  }
static inline void Ssw_ObjSetSatNum( Ssw_Sat_t * p, Aig_Obj_t * pObj, int Num ) { Vec_IntSetEntry(p->vSatVars, pObj->Id, Num);      }

static inline int  Ssw_ObjIsConst1Cand( Aig_Man_t * pAig, Aig_Obj_t * pObj ) 
{
    return Aig_ObjRepr(pAig, pObj) == Aig_ManConst1(pAig);
}
static inline void Ssw_ObjSetConst1Cand( Aig_Man_t * pAig, Aig_Obj_t * pObj ) 
{
    assert( !Ssw_ObjIsConst1Cand( pAig, pObj ) );
    Aig_ObjSetRepr( pAig, pObj, Aig_ManConst1(pAig) );
}

static inline Aig_Obj_t * Ssw_ObjFrame( Ssw_Man_t * p, Aig_Obj_t * pObj, int i )                       { return p->pNodeToFrames[p->nFrames*pObj->Id + i];  }
static inline void        Ssw_ObjSetFrame( Ssw_Man_t * p, Aig_Obj_t * pObj, int i, Aig_Obj_t * pNode ) { p->pNodeToFrames[p->nFrames*pObj->Id + i] = pNode; }

static inline Aig_Obj_t * Ssw_ObjChild0Fra( Ssw_Man_t * p, Aig_Obj_t * pObj, int i ) { assert( !Aig_IsComplement(pObj) ); return Aig_ObjFanin0(pObj)? Aig_NotCond(Ssw_ObjFrame(p, Aig_ObjFanin0(pObj), i), Aig_ObjFaninC0(pObj)) : NULL;  }
static inline Aig_Obj_t * Ssw_ObjChild1Fra( Ssw_Man_t * p, Aig_Obj_t * pObj, int i ) { assert( !Aig_IsComplement(pObj) ); return Aig_ObjFanin1(pObj)? Aig_NotCond(Ssw_ObjFrame(p, Aig_ObjFanin1(pObj), i), Aig_ObjFaninC1(pObj)) : NULL;  }

static inline Aig_Obj_t * Ssw_ObjFrame_( Ssw_Frm_t * p, Aig_Obj_t * pObj, int i )                       { return (Aig_Obj_t *)Vec_PtrGetEntry( p->vAig2Frm, p->nObjs*i+pObj->Id );     }
static inline void        Ssw_ObjSetFrame_( Ssw_Frm_t * p, Aig_Obj_t * pObj, int i, Aig_Obj_t * pNode ) { Vec_PtrSetEntry( p->vAig2Frm, p->nObjs*i+pObj->Id, pNode );     }

static inline Aig_Obj_t * Ssw_ObjChild0Fra_( Ssw_Frm_t * p, Aig_Obj_t * pObj, int i ) { assert( !Aig_IsComplement(pObj) ); return Aig_ObjFanin0(pObj)? Aig_NotCond(Ssw_ObjFrame_(p, Aig_ObjFanin0(pObj), i), Aig_ObjFaninC0(pObj)) : NULL;  }
static inline Aig_Obj_t * Ssw_ObjChild1Fra_( Ssw_Frm_t * p, Aig_Obj_t * pObj, int i ) { assert( !Aig_IsComplement(pObj) ); return Aig_ObjFanin1(pObj)? Aig_NotCond(Ssw_ObjFrame_(p, Aig_ObjFanin1(pObj), i), Aig_ObjFaninC1(pObj)) : NULL;  }

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== sswAig.c ===================================================*/
extern Ssw_Frm_t *   Ssw_FrmStart( Aig_Man_t * pAig );
extern void          Ssw_FrmStop( Ssw_Frm_t * p );
extern Aig_Man_t *   Ssw_FramesWithClasses( Ssw_Man_t * p );
extern Aig_Man_t *   Ssw_SpeculativeReduction( Ssw_Man_t * p );
/*=== sswBmc.c ===================================================*/
/*=== sswClass.c =================================================*/
extern Ssw_Cla_t *   Ssw_ClassesStart( Aig_Man_t * pAig );
extern void          Ssw_ClassesSetData( Ssw_Cla_t * p, void * pManData,
                         unsigned (*pFuncNodeHash)(void *,Aig_Obj_t *),
                         int (*pFuncNodeIsConst)(void *,Aig_Obj_t *),
                         int (*pFuncNodesAreEqual)(void *,Aig_Obj_t *, Aig_Obj_t *) );
extern void          Ssw_ClassesStop( Ssw_Cla_t * p );
extern Aig_Man_t *   Ssw_ClassesReadAig( Ssw_Cla_t * p );
extern Vec_Ptr_t *   Ssw_ClassesGetRefined( Ssw_Cla_t * p );
extern void          Ssw_ClassesClearRefined( Ssw_Cla_t * p );
extern int           Ssw_ClassesCand1Num( Ssw_Cla_t * p );
extern int           Ssw_ClassesClassNum( Ssw_Cla_t * p );
extern int           Ssw_ClassesLitNum( Ssw_Cla_t * p );
extern Aig_Obj_t **  Ssw_ClassesReadClass( Ssw_Cla_t * p, Aig_Obj_t * pRepr, int * pnSize );
extern void          Ssw_ClassesCollectClass( Ssw_Cla_t * p, Aig_Obj_t * pRepr, Vec_Ptr_t * vClass );
extern void          Ssw_ClassesCheck( Ssw_Cla_t * p );
extern void          Ssw_ClassesPrint( Ssw_Cla_t * p, int fVeryVerbose );
extern void          Ssw_ClassesRemoveNode( Ssw_Cla_t * p, Aig_Obj_t * pObj );
extern Ssw_Cla_t *   Ssw_ClassesPrepare( Aig_Man_t * pAig, int nFramesK, int fLatchCorr, int fConstCorr, int fOutputCorr, int nMaxLevs, int fVerbose );
extern Ssw_Cla_t *   Ssw_ClassesPrepareSimple( Aig_Man_t * pAig, int fLatchCorr, int nMaxLevs );
extern Ssw_Cla_t *   Ssw_ClassesPrepareFromReprs( Aig_Man_t * pAig );
extern Ssw_Cla_t *   Ssw_ClassesPrepareTargets( Aig_Man_t * pAig );
extern Ssw_Cla_t *   Ssw_ClassesPreparePairs( Aig_Man_t * pAig, Vec_Int_t ** pvClasses );
extern Ssw_Cla_t *   Ssw_ClassesPreparePairsSimple( Aig_Man_t * pMiter, Vec_Int_t * vPairs );
extern int           Ssw_ClassesRefine( Ssw_Cla_t * p, int fRecursive );
extern int           Ssw_ClassesRefineGroup( Ssw_Cla_t * p, Vec_Ptr_t * vReprs, int fRecursive );
extern int           Ssw_ClassesRefineOneClass( Ssw_Cla_t * p, Aig_Obj_t * pRepr, int fRecursive );
extern int           Ssw_ClassesRefineConst1Group( Ssw_Cla_t * p, Vec_Ptr_t * vRoots, int fRecursive );
extern int           Ssw_ClassesRefineConst1( Ssw_Cla_t * p, int fRecursive );
extern int           Ssw_ClassesPrepareRehash( Ssw_Cla_t * p, Vec_Ptr_t * vCands, int fConstCorr );
/*=== sswCnf.c ===================================================*/
extern Ssw_Sat_t *   Ssw_SatStart( int fPolarFlip );
extern void          Ssw_SatStop( Ssw_Sat_t * p );
extern void          Ssw_CnfNodeAddToSolver( Ssw_Sat_t * p, Aig_Obj_t * pObj );
extern int           Ssw_CnfGetNodeValue( Ssw_Sat_t * p, Aig_Obj_t * pObjFraig );
/*=== sswConstr.c ===================================================*/
extern int           Ssw_ManSweepBmcConstr( Ssw_Man_t * p );
extern int           Ssw_ManSweepConstr( Ssw_Man_t * p );
extern void          Ssw_ManRefineByConstrSim( Ssw_Man_t * p );
/*=== sswCore.c ===================================================*/
extern Aig_Man_t *   Ssw_SignalCorrespondenceRefine( Ssw_Man_t * p );
/*=== sswDyn.c ===================================================*/
extern void          Ssw_ManLoadSolver( Ssw_Man_t * p, Aig_Obj_t * pRepr, Aig_Obj_t * pObj );
extern int           Ssw_ManSweepDyn( Ssw_Man_t * p );
/*=== sswLcorr.c ==========================================================*/
extern int           Ssw_ManSweepLatch( Ssw_Man_t * p );
/*=== sswMan.c ===================================================*/
extern Ssw_Man_t *   Ssw_ManCreate( Aig_Man_t * pAig, Ssw_Pars_t * pPars );
extern void          Ssw_ManCleanup( Ssw_Man_t * p );
extern void          Ssw_ManStop( Ssw_Man_t * p );
/*=== sswSat.c ===================================================*/
extern int           Ssw_NodesAreEquiv( Ssw_Man_t * p, Aig_Obj_t * pOld, Aig_Obj_t * pNew );
extern int           Ssw_NodesAreConstrained( Ssw_Man_t * p, Aig_Obj_t * pOld, Aig_Obj_t * pNew );
extern int           Ssw_NodeIsConstrained( Ssw_Man_t * p, Aig_Obj_t * pPoObj );
/*=== sswSemi.c ===================================================*/
extern int           Ssw_FilterUsingSemi( Ssw_Man_t * pMan, int fCheckTargets, int nConfMax, int fVerbose );
/*=== sswSim.c ===================================================*/
extern unsigned      Ssw_SmlObjHashWord( Ssw_Sml_t * p, Aig_Obj_t * pObj );
extern int           Ssw_SmlObjIsConstWord( Ssw_Sml_t * p, Aig_Obj_t * pObj );
extern int           Ssw_SmlObjsAreEqualWord( Ssw_Sml_t * p, Aig_Obj_t * pObj0, Aig_Obj_t * pObj1 );
extern int           Ssw_SmlObjIsConstBit( void * p, Aig_Obj_t * pObj );
extern int           Ssw_SmlObjsAreEqualBit( void * p, Aig_Obj_t * pObj0, Aig_Obj_t * pObj1 );
extern void          Ssw_SmlAssignRandomFrame( Ssw_Sml_t * p, Aig_Obj_t * pObj, int iFrame );
extern Ssw_Sml_t *   Ssw_SmlStart( Aig_Man_t * pAig, int nPref, int nFrames, int nWordsFrame );
extern void          Ssw_SmlClean( Ssw_Sml_t * p );
extern void          Ssw_SmlStop( Ssw_Sml_t * p );
extern void          Ssw_SmlObjAssignConst( Ssw_Sml_t * p, Aig_Obj_t * pObj, int fConst1, int iFrame );
extern void          Ssw_SmlObjSetWord( Ssw_Sml_t * p, Aig_Obj_t * pObj, unsigned Word, int iWord, int iFrame );
extern void          Ssw_SmlAssignDist1Plus( Ssw_Sml_t * p, unsigned * pPat );
extern void          Ssw_SmlSimulateOne( Ssw_Sml_t * p );
extern void          Ssw_SmlSimulateOneFrame( Ssw_Sml_t * p );
extern void          Ssw_SmlSimulateOneDyn_rec( Ssw_Sml_t * p, Aig_Obj_t * pObj, int f, int * pVisited, int nVisCounter );
extern void          Ssw_SmlResimulateSeq( Ssw_Sml_t * p );
/*=== sswSimSat.c ===================================================*/
extern void          Ssw_ManResimulateBit( Ssw_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pRepr );
extern void          Ssw_ManResimulateWord( Ssw_Man_t * p, Aig_Obj_t * pCand, Aig_Obj_t * pRepr, int f );
/*=== sswSweep.c ===================================================*/
extern int           Ssw_ManGetSatVarValue( Ssw_Man_t * p, Aig_Obj_t * pObj, int f );
extern void          Ssw_SmlSavePatternAig( Ssw_Man_t * p, int f );
extern int           Ssw_ManSweepNode( Ssw_Man_t * p, Aig_Obj_t * pObj, int f, int fBmc, Vec_Int_t * vPairs );
extern int           Ssw_ManSweepBmc( Ssw_Man_t * p );
extern int           Ssw_ManSweep( Ssw_Man_t * p );
/*=== sswUnique.c ===================================================*/
extern void          Ssw_UniqueRegisterPairInfo( Ssw_Man_t * p );
extern int           Ssw_ManUniqueOne( Ssw_Man_t * p, Aig_Obj_t * pRepr, Aig_Obj_t * pObj, int fVerbose );
extern int           Ssw_ManUniqueAddConstraint( Ssw_Man_t * p, Vec_Ptr_t * vCommon, int f1, int f2 );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

