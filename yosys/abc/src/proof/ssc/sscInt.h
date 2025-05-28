/**CFile****************************************************************

  FileName    [sscInt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Choice computation for tech-mapping.]

  Synopsis    [Internal declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 29, 2008.]

  Revision    [$Id: sscInt.h,v 1.00 2008/07/29 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__aig__ssc__sscInt_h
#define ABC__aig__ssc__sscInt_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "aig/gia/gia.h"
#include "sat/bsat/satSolver.h"
#include "ssc.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

// choicing manager
typedef struct Ssc_Man_t_ Ssc_Man_t;
struct Ssc_Man_t_
{
    // user data
    Ssc_Pars_t *     pPars;          // choicing parameters
    Gia_Man_t *      pAig;           // subject AIG
    Gia_Man_t *      pCare;          // care set AIG
    // internal data
    Gia_Man_t *      pFraig;         // resulting AIG
    sat_solver *     pSat;           // recyclable SAT solver
    Vec_Int_t *      vId2Var;        // mapping of each node into its SAT var
    Vec_Int_t *      vVar2Id;        // mapping of each SAT var into its node
    Vec_Int_t *      vPivot;         // one SAT pattern
    int              nSatVarsPivot;  // the number of variables for constraints
    int              nSatVars;       // the current number of variables  
    // temporary storage
    Vec_Int_t *      vFront;         // supergate fanins
    Vec_Int_t *      vFanins;        // supergate fanins
    Vec_Int_t *      vPattern;       // counter-example
    Vec_Int_t *      vDisPairs;      // disproved pairs
    // SAT calls statistics
    int              nSimRounds;     // the number of simulation rounds
    int              nRecycles;      // the number of times SAT solver was recycled
    int              nCallsSince;    // the number of calls since the last recycle
    int              nSatCalls;      // the number of SAT calls
    int              nSatCallsUnsat; // the number of unsat SAT calls
    int              nSatCallsSat;   // the number of sat SAT calls
    int              nSatCallsUndec; // the number of undec SAT calls
    // runtime stats
    abctime          timeSimInit;    // simulation and class computation
    abctime          timeSimSat;     // simulation of the counter-examples
    abctime          timeCnfGen;     // generation of CNF
    abctime          timeSat;        // total SAT time
    abctime          timeSatSat;     // sat
    abctime          timeSatUnsat;   // unsat
    abctime          timeSatUndec;   // undecided
    abctime          timeOther;      // other runtime
    abctime          timeTotal;      // total runtime
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

static inline int    Ssc_ObjSatVar( Ssc_Man_t * p, int iObj )             { return Vec_IntEntry(p->vId2Var, iObj);     }
static inline void   Ssc_ObjSetSatVar( Ssc_Man_t * p, int iObj, int Num ) { Vec_IntWriteEntry(p->vId2Var, iObj, Num);  Vec_IntWriteEntry(p->vVar2Id, Num, iObj);  }
static inline void   Ssc_ObjCleanSatVar( Ssc_Man_t * p, int Num )         { Vec_IntWriteEntry(p->vId2Var, Vec_IntEntry(p->vVar2Id, Num), Num);  Vec_IntWriteEntry(p->vVar2Id, Num, 0);                        }

static inline int    Ssc_ObjFraig( Ssc_Man_t * p, Gia_Obj_t * pObj )      { return pObj->Value;           }
static inline void   Ssc_ObjSetFraig( Gia_Obj_t * pObj, int iNode )       { pObj->Value = iNode;          }

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== sscClass.c =================================================*/
extern void          Ssc_GiaClassesInit( Gia_Man_t * p );
extern int           Ssc_GiaClassesRefine( Gia_Man_t * p );
extern void          Ssc_GiaClassesCheckPairs( Gia_Man_t * p, Vec_Int_t * vDisPairs );
extern int           Ssc_GiaSimClassRefineOneBit( Gia_Man_t * p, int i );
/*=== sscCnf.c ===================================================*/
extern void          Ssc_CnfNodeAddToSolver( Ssc_Man_t * p, Gia_Obj_t * pObj );
/*=== sscCore.c ==================================================*/
/*=== sscSat.c ===================================================*/
extern void          Ssc_ManSatSolverRecycle( Ssc_Man_t * p );
extern void          Ssc_ManStartSolver( Ssc_Man_t * p );
extern Vec_Int_t *   Ssc_ManFindPivotSat( Ssc_Man_t * p );
extern int           Ssc_ManCheckEquivalence( Ssc_Man_t * p, int iRepr, int iObj, int fCompl );
/*=== sscSim.c ===================================================*/
extern void          Ssc_GiaResetPiPattern( Gia_Man_t * p, int nWords );
extern void          Ssc_GiaRandomPiPattern( Gia_Man_t * p, int nWords, Vec_Int_t * vPivot );
extern int           Ssc_GiaTransferPiPattern( Gia_Man_t * pAig, Gia_Man_t * pCare, Vec_Int_t * vPivot );
extern void          Ssc_GiaSavePiPattern( Gia_Man_t * p, Vec_Int_t * vPat );
extern void          Ssc_GiaSimRound( Gia_Man_t * p );
extern Vec_Int_t *   Ssc_GiaFindPivotSim( Gia_Man_t * p );
extern int           Ssc_GiaEstimateCare( Gia_Man_t * p, int nWords );
/*=== sscUtil.c ===================================================*/
extern Gia_Man_t *   Ssc_GenerateOneHot( int nVars );


ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

