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

#ifndef __FRA_H__
#define __FRA_H__

#ifdef __cplusplus
extern "C" {
#endif 

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "vec.h"
#include "aig.h"
#include "dar.h"
#include "satSolver.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Fra_Par_t_   Fra_Par_t;
typedef struct Fra_Cla_t_   Fra_Cla_t;
typedef struct Fra_Man_t_   Fra_Man_t;

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
    int              nFramesK;          // the number of timeframes to unroll
    int              fRewrite;          // use rewriting for constraint reduction
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
    // simulation info
    unsigned *       pSimWords;         // memory for simulation information
    int              nSimWords;         // the number of simulation words
    // counter example storage
    int              nPatWords;         // the number of words in the counter example
    unsigned *       pPatWords;         // the counter example
    int *            pPatScores;        // the scores of each pattern
    // equivalence classes 
    Fra_Cla_t *      pCla;              // representation of (candidate) equivalent nodes
    // equivalence checking
    sat_solver *     pSat;              // SAT solver
    int              nSatVars;          // the number of variables currently used
    Vec_Ptr_t *      vPiVars;           // the PIs of the cone used 
    sint64           nBTLimitGlobal;    // resource limit
    sint64           nInsLimitGlobal;   // resource limit
    Vec_Ptr_t **     pMemFanins;        // the arrays of fanins for some FRAIG nodes
    int *            pMemSatNums;       // the array of SAT numbers for some FRAIG nodes
    int              nSizeAlloc;        // allocated size of the arrays
    Vec_Ptr_t *      vTimeouts;         // the nodes, for which equivalence checking timed out
    // statistics
    int              nSimRounds;
    int              nNodesMiter;
    int              nLitsZero;
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
    // runtime
    int              timeSim;
    int              timeTrav;
    int              timeRwr;
    int              timeSat;
    int              timeSatUnsat;
    int              timeSatSat;
    int              timeSatFail;
    int              timeRef;
    int              timeTotal;
    int              time1;
    int              time2;
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

static inline unsigned *   Fra_ObjSim( Aig_Obj_t * pObj )   { return ((Fra_Man_t *)pObj->pData)->pSimWords + ((Fra_Man_t *)pObj->pData)->nSimWords * pObj->Id;   }
static inline unsigned     Fra_ObjRandomSim()               { return (rand() << 24) ^ (rand() << 12) ^ rand();                                                   }

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

////////////////////////////////////////////////////////////////////////
///                         ITERATORS                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== fraClass.c ========================================================*/
extern Fra_Cla_t *         Fra_ClassesStart( Aig_Man_t * pAig );
extern void                Fra_ClassesStop( Fra_Cla_t * p );
extern void                Fra_ClassesCopyReprs( Fra_Cla_t * p, Vec_Ptr_t * vFailed );
extern void                Fra_ClassesPrint( Fra_Cla_t * p, int fVeryVerbose );
extern void                Fra_ClassesPrepare( Fra_Cla_t * p );
extern int                 Fra_ClassesRefine( Fra_Cla_t * p );
extern int                 Fra_ClassesRefine1( Fra_Cla_t * p );
extern int                 Fra_ClassesCountLits( Fra_Cla_t * p );
extern int                 Fra_ClassesCountPairs( Fra_Cla_t * p );
extern void                Fra_ClassesTest( Fra_Cla_t * p, int Id1, int Id2 );
/*=== fraCnf.c ========================================================*/
extern void                Fra_NodeAddToSolver( Fra_Man_t * p, Aig_Obj_t * pOld, Aig_Obj_t * pNew );
/*=== fraCore.c ========================================================*/
extern Aig_Man_t *         Fra_FraigPerform( Aig_Man_t * pManAig, Fra_Par_t * pPars );
extern Aig_Man_t *         Fra_FraigChoice( Aig_Man_t * pManAig );
extern void                Fra_FraigSweep( Fra_Man_t * pManAig );
extern int                 Fra_FraigMiterStatus( Aig_Man_t * p );
/*=== fraDfs.c ========================================================*/
/*=== fraInd.c ========================================================*/
extern Aig_Man_t *         Fra_FraigInduction( Aig_Man_t * p, int nFramesK, int fRewrite, int fVerbose, int * pnIter );
/*=== fraMan.c ========================================================*/
extern void                Fra_ParamsDefault( Fra_Par_t * pParams );
extern void                Fra_ParamsDefaultSeq( Fra_Par_t * pParams );
extern Fra_Man_t *         Fra_ManStart( Aig_Man_t * pManAig, Fra_Par_t * pParams );
extern void                Fra_ManClean( Fra_Man_t * p );
extern Aig_Man_t *         Fra_ManPrepareComb( Fra_Man_t * p );
extern void                Fra_ManFinalizeComb( Fra_Man_t * p );
extern void                Fra_ManStop( Fra_Man_t * p );
extern void                Fra_ManPrint( Fra_Man_t * p );
/*=== fraSat.c ========================================================*/
extern int                 Fra_NodesAreEquiv( Fra_Man_t * p, Aig_Obj_t * pOld, Aig_Obj_t * pNew );
extern int                 Fra_NodeIsConst( Fra_Man_t * p, Aig_Obj_t * pNew );
/*=== fraSec.c ========================================================*/
extern int                 Fra_FraigSec( Aig_Man_t * p, int nFrames, int fVerbose, int fVeryVerbose );
/*=== fraSim.c ========================================================*/
extern int                 Fra_NodeHasZeroSim( Aig_Obj_t * pObj );
extern int                 Fra_NodeCompareSims( Aig_Obj_t * pObj0, Aig_Obj_t * pObj1 );
extern unsigned            Fra_NodeHashSims( Aig_Obj_t * pObj );
extern void                Fra_SavePattern( Fra_Man_t * p );
extern void                Fra_Simulate( Fra_Man_t * p, int fInit );
extern void                Fra_Resimulate( Fra_Man_t * p );
extern int                 Fra_CheckOutputSims( Fra_Man_t * p );

#ifdef __cplusplus
}
#endif

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

