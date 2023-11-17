/**CFile****************************************************************

  FileName    [rsbInt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based optimization using internal don't-cares.]

  Synopsis    [Internal declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: rsbInt.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__opt_sfmInt__h
#define ABC__opt_sfmInt__h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "misc/vec/vec.h"
#include "sat/bsat/satSolver.h"
#include "misc/util/utilNam.h"
#include "map/scl/sclLib.h"
#include "map/scl/sclCon.h"
#include "misc/st/st.h"
#include "map/mio/mio.h"
#include "base/abc/abc.h"
#include "misc/util/utilTruth.h"
#include "sfm.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START

#define SFM_FANIN_MAX 12
#define SFM_WORDS_MAX ((SFM_FANIN_MAX>6) ? (1<<(SFM_FANIN_MAX-6)) : 1)
#define SFM_SAT_UNDEC 0x1234567812345678
#define SFM_SAT_SAT   0x8765432187654321

#define SFM_SUPP_MAX  8
#define SFM_WORD_MAX ((SFM_SUPP_MAX>6) ? (1<<(SFM_SUPP_MAX-6)) : 1)
#define SFM_WIN_MAX   1000
#define SFM_DEC_MAX   4
#define SFM_SIM_WORDS 8

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Sfm_Fun_t_ Sfm_Fun_t; 
typedef struct Sfm_Lib_t_ Sfm_Lib_t; 
typedef struct Sfm_Tim_t_ Sfm_Tim_t;
typedef struct Sfm_Mit_t_ Sfm_Mit_t;

struct Sfm_Ntk_t_
{
    // parameters
    Sfm_Par_t *       pPars;       // parameters
    // objects
    int               nPis;        // PI count (PIs should be first objects)
    int               nPos;        // PO count (POs should be last objects)
    int               nNodes;      // internal nodes
    int               nObjs;       // total objects
    int               nLevelMax;   // maximum level
    // user data
    Vec_Str_t *       vFixed;      // persistent objects
    Vec_Str_t *       vEmpty;      // transparent objects
    Vec_Wrd_t *       vTruths;     // truth tables
    Vec_Wec_t         vFanins;     // fanins
    Vec_Int_t *       vStarts;     // offsets
    Vec_Wrd_t *       vTruths2;    // truth tables (large)
    // attributes
    Vec_Wec_t         vFanouts;    // fanouts
    Vec_Int_t         vLevels;     // logic level
    Vec_Int_t         vLevelsR;    // logic level
    Vec_Int_t         vCounts;     // fanin counters
    Vec_Int_t         vId2Var;     // ObjId -> SatVar
    Vec_Int_t         vVar2Id;     // SatVar -> ObjId
    Vec_Wec_t *       vCnfs;       // CNFs
    Vec_Int_t *       vCover;      // temporary
    // traversal IDs
    Vec_Int_t         vTravIds;    // traversal IDs
    Vec_Int_t         vTravIds2;   // traversal IDs
    int               nTravIds;    // traversal IDs
    int               nTravIds2;   // traversal IDs
    // window
    int               iPivotNode;  // window pivot
    Vec_Int_t *       vNodes;      // internal
    Vec_Int_t *       vDivs;       // divisors
    Vec_Int_t *       vRoots;      // roots
    Vec_Int_t *       vTfo;        // TFO (excluding iNode)
    // SAT solving
    sat_solver *      pSat;        // SAT solver
    int               nSatVars;    // the number of variables
    int               nTryRemoves; // number of fanin removals
    int               nTryImproves;// number of node improvements
    int               nTryResubs;  // number of resubstitutions
    int               nRemoves;    // number of fanin removals
    int               nImproves;   // number of node improvements
    int               nResubs;     // number of resubstitutions
    // counter-examples
    int               nCexes;      // number of CEXes
    Vec_Wrd_t *       vDivCexes;   // counter-examples
    // intermediate data
    Vec_Int_t *       vOrder;      // object order
    Vec_Int_t *       vDivVars;    // divisor SAT variables
    Vec_Int_t *       vDivIds;     // divisors indexes
    Vec_Int_t *       vLits;       // literals
    Vec_Int_t *       vValues;     // SAT variable values
    Vec_Wec_t *       vClauses;    // CNF clauses for the node
    Vec_Int_t *       vFaninMap;   // mapping fanins into their SAT vars
    word              TtElems[SFM_FANIN_MAX][SFM_WORDS_MAX];
    word *            pTtElems[SFM_FANIN_MAX];
    word              pTruth[SFM_WORDS_MAX];
    word              pCube[SFM_WORDS_MAX];
    // nodes
    int               nTotalNodesBeg;
    int               nTotalEdgesBeg;
    int               nTotalNodesEnd;
    int               nTotalEdgesEnd;
    int               nNodesTried;
    int               nTotalDivs;
    int               nSatCalls;
    int               nTimeOuts;
    int               nMaxDivs;
    // runtime
    abctime           timeWin;
    abctime           timeDiv;
    abctime           timeCnf;
    abctime           timeSat;
    abctime           timeOther;
    abctime           timeTotal;
//    abctime           time1;
};

static inline int  Sfm_NtkPiNum( Sfm_Ntk_t * p )                        { return p->nPis;                                   }
static inline int  Sfm_NtkPoNum( Sfm_Ntk_t * p )                        { return p->nPos;                                   }
static inline int  Sfm_NtkNodeNum( Sfm_Ntk_t * p )                      { return p->nObjs - p->nPis - p->nPos;              }

static inline int  Sfm_ObjIsPi( Sfm_Ntk_t * p, int i )                  { return i < p->nPis;                               }
static inline int  Sfm_ObjIsPo( Sfm_Ntk_t * p, int i )                  { return i + p->nPos >= p->nObjs;                   }
static inline int  Sfm_ObjIsNode( Sfm_Ntk_t * p, int i )                { return i >= p->nPis && i + p->nPos < p->nObjs;    }
static inline int  Sfm_ObjIsFixed( Sfm_Ntk_t * p, int i )               { return Vec_StrEntry(p->vFixed, i);                }
static inline int  Sfm_ObjAddsLevelArray( Vec_Str_t * p, int i )        { return p == NULL || Vec_StrEntry(p, i) == 0;      }
static inline int  Sfm_ObjAddsLevel( Sfm_Ntk_t * p, int i )             { return Sfm_ObjAddsLevelArray(p->vEmpty, i);       }

static inline Vec_Int_t * Sfm_ObjFiArray( Sfm_Ntk_t * p, int i )        { return Vec_WecEntry(&p->vFanins, i);              }
static inline Vec_Int_t * Sfm_ObjFoArray( Sfm_Ntk_t * p, int i )        { return Vec_WecEntry(&p->vFanouts, i);             }

static inline int  Sfm_ObjFaninNum( Sfm_Ntk_t * p, int i )              { return Vec_IntSize(Sfm_ObjFiArray(p, i));         }
static inline int  Sfm_ObjFanoutNum( Sfm_Ntk_t * p, int i )             { return Vec_IntSize(Sfm_ObjFoArray(p, i));         }

static inline int  Sfm_ObjRefIncrement( Sfm_Ntk_t * p, int iObj )       { return ++Sfm_ObjFoArray(p, iObj)->nSize;          } 
static inline int  Sfm_ObjRefDecrement( Sfm_Ntk_t * p, int iObj )       { return --Sfm_ObjFoArray(p, iObj)->nSize;          } 

static inline int  Sfm_ObjFanin( Sfm_Ntk_t * p, int i, int k )          { return Vec_IntEntry(Sfm_ObjFiArray(p, i), k);     }
static inline int  Sfm_ObjFanout( Sfm_Ntk_t * p, int i, int k )         { return Vec_IntEntry(Sfm_ObjFoArray(p, i), k);     }

static inline int  Sfm_ObjSatVar( Sfm_Ntk_t * p, int iObj )             { assert(Vec_IntEntry(&p->vId2Var, iObj) > 0); return Vec_IntEntry(&p->vId2Var, iObj);  }
static inline void Sfm_ObjSetSatVar( Sfm_Ntk_t * p, int iObj, int Num ) { assert(Vec_IntEntry(&p->vId2Var, iObj) == -1); Vec_IntWriteEntry(&p->vId2Var, iObj, Num);  Vec_IntWriteEntry(&p->vVar2Id, Num, iObj);  }
static inline void Sfm_ObjCleanSatVar( Sfm_Ntk_t * p, int Num )         { int iObj = Vec_IntEntry(&p->vVar2Id, Num); assert(Vec_IntEntry(&p->vId2Var, iObj) > 0); Vec_IntWriteEntry(&p->vId2Var, iObj, -1);  Vec_IntWriteEntry(&p->vVar2Id, Num, -1); }
static inline void Sfm_NtkCleanVars( Sfm_Ntk_t * p )                    { int i; for ( i = 1; i < p->nSatVars; i++ )  if ( Vec_IntEntry(&p->vVar2Id, i) != -1 ) Sfm_ObjCleanSatVar( p, i ); }

static inline int  Sfm_ObjLevel( Sfm_Ntk_t * p, int iObj )              { return Vec_IntEntry( &p->vLevels, iObj );                         }
static inline void Sfm_ObjSetLevel( Sfm_Ntk_t * p, int iObj, int Lev )  { Vec_IntWriteEntry( &p->vLevels, iObj, Lev );                      }

static inline int  Sfm_ObjLevelR( Sfm_Ntk_t * p, int iObj )             { return Vec_IntEntry( &p->vLevelsR, iObj );                        }
static inline void Sfm_ObjSetLevelR( Sfm_Ntk_t * p, int iObj, int Lev ) { Vec_IntWriteEntry( &p->vLevelsR, iObj, Lev );                     }

static inline int  Sfm_ObjUpdateFaninCount( Sfm_Ntk_t * p, int iObj )   { return Vec_IntAddToEntry(&p->vCounts, iObj, -1);                  }
static inline void Sfm_ObjResetFaninCount( Sfm_Ntk_t * p, int iObj )    { Vec_IntWriteEntry(&p->vCounts, iObj, Sfm_ObjFaninNum(p, iObj)-1); }

extern void        Kit_DsdPrintFromTruth( unsigned * pTruth, int nVars );

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

#define Sfm_NtkForEachPi( p, i )                 for ( i = 0; i < p->nPis; i++ )
#define Sfm_NtkForEachPo( p, i )                 for ( i = p->nObjs - p->nPos; i < p->nObjs; i++ )
#define Sfm_NtkForEachNode( p, i )               for ( i = p->nPis; i + p->nPos < p->nObjs; i++ )
#define Sfm_NtkForEachNodeReverse( p, i )        for ( i = p->nObjs - p->nPos - 1; i >= p->nPis; i-- )
#define Sfm_ObjForEachFanin( p, Node, Fan, i )   for ( i = 0; i < Sfm_ObjFaninNum(p, Node)  && ((Fan = Sfm_ObjFanin(p, Node, i)), 1);  i++ )
#define Sfm_ObjForEachFanout( p, Node, Fan, i )  for ( i = 0; i < Sfm_ObjFanoutNum(p, Node) && ((Fan = Sfm_ObjFanout(p, Node, i)), 1); i++ )

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== sfmCnf.c ==========================================================*/
extern void         Sfm_PrintCnf( Vec_Str_t * vCnf );
extern int          Sfm_TruthToCnf( word Truth, word * pTruth, int nVars, Vec_Int_t * vCover, Vec_Str_t * vCnf );
extern Vec_Wec_t *  Sfm_CreateCnf( Sfm_Ntk_t * p );
extern void         Sfm_TranslateCnf( Vec_Wec_t * vRes, Vec_Str_t * vCnf, Vec_Int_t * vFaninMap, int iPivotVar );
/*=== sfmCore.c ==========================================================*/
/*=== sfmLib.c ==========================================================*/
extern int          Sfm_LibFindComplInputGate( Vec_Wrd_t * vFuncs, int iGate, int nFanins, int iFanin, int * piFaninNew );
extern Sfm_Lib_t *  Sfm_LibPrepare( int nVars, int fTwo, int fDelay, int fVerbose, int fLibVerbose );
extern void         Sfm_LibPrint( Sfm_Lib_t * p );
extern void         Sfm_LibStop( Sfm_Lib_t * p );
extern int          Sfm_LibFindAreaMatch( Sfm_Lib_t * p, word * pTruth, int nFanins, int * piObj );
extern int          Sfm_LibFindDelayMatches( Sfm_Lib_t * p, word * pTruth, int * pFanins, int nFanins, Vec_Ptr_t * vGates, Vec_Ptr_t * vFans );
extern int          Sfm_LibImplementSimple( Sfm_Lib_t * p, word * pTruth, int * pFanins, int nFanins, Vec_Int_t * vGates, Vec_Wec_t * vFanins );
extern int          Sfm_LibImplementGatesArea( Sfm_Lib_t * p, int * pFanins, int nFanins, int iObj, Vec_Int_t * vGates, Vec_Wec_t * vFanins );
extern int          Sfm_LibImplementGatesDelay( Sfm_Lib_t * p, int * pFanins, Mio_Gate_t * pGateB, Mio_Gate_t * pGateT, char * pFansB, char * pFansT, Vec_Int_t * vGates, Vec_Wec_t * vFanins );
/*=== sfmNtk.c ==========================================================*/
extern Sfm_Ntk_t *  Sfm_ConstructNetwork( Vec_Wec_t * vFanins, int nPis, int nPos );
extern void         Sfm_NtkPrepare( Sfm_Ntk_t * p );
extern void         Sfm_NtkUpdate( Sfm_Ntk_t * p, int iNode, int f, int iFaninNew, word uTruth, word * pTruth );
/*=== sfmSat.c ==========================================================*/
extern int          Sfm_NtkWindowToSolver( Sfm_Ntk_t * p );
extern word         Sfm_ComputeInterpolant( Sfm_Ntk_t * p );
extern word         Sfm_ComputeInterpolant2( Sfm_Ntk_t * p );
/*=== sfmTim.c ==========================================================*/
extern Sfm_Tim_t *  Sfm_TimStart( Mio_Library_t * pLib, Scl_Con_t * pExt, Abc_Ntk_t * pNtk, int DeltaCrit );
extern void         Sfm_TimStop( Sfm_Tim_t * p );
extern int          Sfm_TimReadNtkDelay( Sfm_Tim_t * p );
extern int          Sfm_TimReadObjDelay( Sfm_Tim_t * p, int iObj );
extern void         Sfm_TimUpdateTiming( Sfm_Tim_t * p, Vec_Int_t * vTimeNodes );
extern int          Sfm_TimSortArrayByArrival( Sfm_Tim_t * p, Vec_Int_t * vNodes, int iPivot );
extern int          Sfm_TimPriorityNodes( Sfm_Tim_t * p, Vec_Int_t * vCands, int Window );
extern int          Sfm_TimNodeIsNonCritical( Sfm_Tim_t * p, Abc_Obj_t * pPivot, Abc_Obj_t * pNode );
extern int          Sfm_TimEvalRemapping( Sfm_Tim_t * p, Vec_Int_t * vFanins, Vec_Int_t * vMap, Mio_Gate_t * pGate1, char * pFans1, Mio_Gate_t * pGate2, char * pFans2 );
/*=== sfmMit.c ==========================================================*/
extern Sfm_Mit_t *  Sfm_MitStart( Mio_Library_t * pLib, SC_Lib * pScl, Scl_Con_t * pExt, Abc_Ntk_t * pNtk, int DeltaCrit );
extern void         Sfm_MitStop( Sfm_Mit_t * p );
extern int          Sfm_MitReadNtkDelay( Sfm_Mit_t * p );
extern int          Sfm_MitReadNtkMinSlack( Sfm_Mit_t * p );
extern int          Sfm_MitReadObjDelay( Sfm_Mit_t * p, int iObj );
extern void         Sfm_MitTransferLoad( Sfm_Mit_t * p, Abc_Obj_t * pNew, Abc_Obj_t * pOld );
extern void         Sfm_MitTimingGrow( Sfm_Mit_t * p );
extern void         Sfm_MitUpdateLoad( Sfm_Mit_t * p, Vec_Int_t * vTimeNodes, int fAdd );
extern void         Sfm_MitUpdateTiming( Sfm_Mit_t * p, Vec_Int_t * vTimeNodes );
extern int          Sfm_MitSortArrayByArrival( Sfm_Mit_t * p, Vec_Int_t * vNodes, int iPivot );
extern int          Sfm_MitPriorityNodes( Sfm_Mit_t * p, Vec_Int_t * vCands, int Window );
extern int          Sfm_MitNodeIsNonCritical( Sfm_Mit_t * p, Abc_Obj_t * pPivot, Abc_Obj_t * pNode );
extern int          Sfm_MitEvalRemapping( Sfm_Mit_t * p, Vec_Int_t * vMffc, Abc_Obj_t * pObj, Vec_Int_t * vFanins, Vec_Int_t * vMap, Mio_Gate_t * pGate1, char * pFans1, Mio_Gate_t * pGate2, char * pFans2 );
/*=== sfmWin.c ==========================================================*/
extern int          Sfm_ObjMffcSize( Sfm_Ntk_t * p, int iObj );
extern int          Sfm_NtkCreateWindow( Sfm_Ntk_t * p, int iNode, int fVerbose );

ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

