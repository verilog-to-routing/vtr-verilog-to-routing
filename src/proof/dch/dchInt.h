/**CFile****************************************************************

  FileName    [dchInt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Choice computation for tech-mapping.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 29, 2008.]

  Revision    [$Id: dchInt.h,v 1.00 2008/07/29 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__aig__dch__dchInt_h
#define ABC__aig__dch__dchInt_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "aig/aig/aig.h"
#include "sat/bsat/satSolver.h"
#include "dch.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

// equivalence classes
typedef struct Dch_Cla_t_ Dch_Cla_t;

// choicing manager
typedef struct Dch_Man_t_ Dch_Man_t;
struct Dch_Man_t_
{
    // parameters
    Dch_Pars_t *     pPars;          // choicing parameters
    // AIGs used in the package
//    Vec_Ptr_t *      vAigs;          // user-given AIGs
    Aig_Man_t *      pAigTotal;      // intermediate AIG
    Aig_Man_t *      pAigFraig;      // final AIG
    // equivalence classes
    Dch_Cla_t *      ppClasses;      // equivalence classes of nodes
    Aig_Obj_t **     pReprsProved;   // equivalences proved
    // SAT solving
    sat_solver *     pSat;           // recyclable SAT solver
    int              nSatVars;       // the counter of SAT variables
    int *            pSatVars;       // mapping of each node into its SAT var
    Vec_Ptr_t *      vUsedNodes;     // nodes whose SAT vars are assigned
    int              nRecycles;      // the number of times SAT solver was recycled
    int              nCallsSince;    // the number of calls since the last recycle
    Vec_Ptr_t *      vFanins;        // fanins of the CNF node
    Vec_Ptr_t *      vSimRoots;      // the roots of cand const 1 nodes to simulate
    Vec_Ptr_t *      vSimClasses;    // the roots of cand equiv classes to simulate
    // solver cone size
    int              nConeThis;
    int              nConeMax;
    // SAT calls statistics
    int              nSatCalls;      // the number of SAT calls
    int              nSatProof;      // the number of proofs
    int              nSatFailsReal;  // the number of timeouts
    int              nSatCallsUnsat; // the number of unsat SAT calls
    int              nSatCallsSat;   // the number of sat SAT calls
    // choice node statistics
    int              nLits;          // the number of lits in the cand equiv classes
    int              nReprs;         // the number of proved equivalent pairs
    int              nEquivs;        // the number of final equivalences
    int              nChoices;       // the number of final choice nodes
    // runtime stats
    abctime          timeSimInit;    // simulation and class computation
    abctime          timeSimSat;     // simulation of the counter-examples
    abctime          timeSat;        // solving SAT
    abctime          timeSatSat;     // sat
    abctime          timeSatUnsat;   // unsat
    abctime          timeSatUndec;   // undecided
    abctime          timeChoice;     // choice computation
    abctime          timeOther;      // other runtime
    abctime          timeTotal;      // total runtime
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

static inline int  Dch_ObjSatNum( Dch_Man_t * p, Aig_Obj_t * pObj )             { return p->pSatVars[pObj->Id]; }
static inline void Dch_ObjSetSatNum( Dch_Man_t * p, Aig_Obj_t * pObj, int Num ) { p->pSatVars[pObj->Id] = Num;  }

static inline Aig_Obj_t * Dch_ObjFraig( Aig_Obj_t * pObj )                       { return (Aig_Obj_t *)pObj->pData;  }
static inline void        Dch_ObjSetFraig( Aig_Obj_t * pObj, Aig_Obj_t * pNode ) { pObj->pData = pNode; }

static inline int  Dch_ObjIsConst1Cand( Aig_Man_t * pAig, Aig_Obj_t * pObj ) 
{
    return Aig_ObjRepr(pAig, pObj) == Aig_ManConst1(pAig);
}
static inline void Dch_ObjSetConst1Cand( Aig_Man_t * pAig, Aig_Obj_t * pObj ) 
{
    assert( !Dch_ObjIsConst1Cand( pAig, pObj ) );
    Aig_ObjSetRepr( pAig, pObj, Aig_ManConst1(pAig) );
}

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== dchAig.c ===================================================*/
/*=== dchChoice.c ===================================================*/
extern int           Dch_DeriveChoiceCountReprs( Aig_Man_t * pAig );
extern int           Dch_DeriveChoiceCountEquivs( Aig_Man_t * pAig );
extern Aig_Man_t *   Dch_DeriveChoiceAig( Aig_Man_t * pAig, int fSkipRedSupps );
/*=== dchClass.c =================================================*/
extern Dch_Cla_t *   Dch_ClassesStart( Aig_Man_t * pAig );
extern void          Dch_ClassesSetData( Dch_Cla_t * p, void * pManData,
                         unsigned (*pFuncNodeHash)(void *,Aig_Obj_t *),
                         int (*pFuncNodeIsConst)(void *,Aig_Obj_t *),
                         int (*pFuncNodesAreEqual)(void *,Aig_Obj_t *, Aig_Obj_t *) );
extern void          Dch_ClassesStop( Dch_Cla_t * p );
extern int           Dch_ClassesLitNum( Dch_Cla_t * p );
extern Aig_Obj_t **  Dch_ClassesReadClass( Dch_Cla_t * p, Aig_Obj_t * pRepr, int * pnSize );
extern void          Dch_ClassesPrint( Dch_Cla_t * p, int fVeryVerbose );
extern void          Dch_ClassesPrepare( Dch_Cla_t * p, int fLatchCorr, int nMaxLevs );
extern int           Dch_ClassesRefine( Dch_Cla_t * p );
extern int           Dch_ClassesRefineOneClass( Dch_Cla_t * p, Aig_Obj_t * pRepr, int fRecursive );
extern void          Dch_ClassesCollectOneClass( Dch_Cla_t * p, Aig_Obj_t * pRepr, Vec_Ptr_t * vRoots );
extern void          Dch_ClassesCollectConst1Group( Dch_Cla_t * p, Aig_Obj_t * pObj, int nNodes, Vec_Ptr_t * vRoots );
extern int           Dch_ClassesRefineConst1Group( Dch_Cla_t * p, Vec_Ptr_t * vRoots, int fRecursive );
/*=== dchCnf.c ===================================================*/
extern void          Dch_CnfNodeAddToSolver( Dch_Man_t * p, Aig_Obj_t * pObj );
/*=== dchMan.c ===================================================*/
extern Dch_Man_t *   Dch_ManCreate( Aig_Man_t * pAig, Dch_Pars_t * pPars );
extern void          Dch_ManStop( Dch_Man_t * p );
extern void          Dch_ManSatSolverRecycle( Dch_Man_t * p );
/*=== dchSat.c ===================================================*/
extern int           Dch_NodesAreEquiv( Dch_Man_t * p, Aig_Obj_t * pObj1, Aig_Obj_t * pObj2 );
/*=== dchSim.c ===================================================*/
extern Dch_Cla_t *   Dch_CreateCandEquivClasses( Aig_Man_t * pAig, int nWords, int fVerbose );
/*=== dchSimSat.c ===================================================*/
extern void          Dch_ManResimulateCex( Dch_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pRepr );
extern void          Dch_ManResimulateCex2( Dch_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pRepr );
/*=== dchSweep.c ===================================================*/
extern void          Dch_ManSweep( Dch_Man_t * p );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

