/**CFile****************************************************************

  FileName    [absRef.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Abstraction package.]

  Synopsis    [Refinement manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: absRef.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__proof_abs__AbsRef_h
#define ABC__proof_abs__AbsRef_h


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


////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

typedef struct Rnm_Obj_t_ Rnm_Obj_t; // refinement object
struct Rnm_Obj_t_
{
    unsigned        Value     :  1;  // binary value
    unsigned        fVisit    :  1;  // visited object
    unsigned        fVisitJ   :  1;  // justified visited object
    unsigned        fPPi      :  1;  // PPI object
    unsigned        Prio      : 24;  // priority (0 - highest)
};

typedef struct Rnm_Man_t_ Rnm_Man_t; // refinement manager
struct Rnm_Man_t_
{
    // user data
    Gia_Man_t *     pGia;            // working AIG manager (it is completely owned by this package)
    Abc_Cex_t *     pCex;            // counter-example
    Vec_Int_t *     vMap;            // mapping of CEX inputs into objects (PI + PPI, in any order)
    int             fPropFanout;     // propagate fanouts
    int             fVerbose;        // verbose flag
    int             nRefId;          // refinement ID
    // traversing data
    Vec_Int_t *     vObjs;           // internal objects used in value propagation
    // filtering of selected objects
    Vec_Str_t *     vCounts;         // fanin counters
    Vec_Int_t *     vFanins;         // fanins
/*
    // SAT solver
    sat_solver2 *   pSat;            // incremental SAT solver
    Vec_Int_t *     vSatVars;        // SAT variables
    Vec_Int_t *     vSat2Ids;        // mapping of SAT variables into object IDs
    Vec_Int_t *     vIsopMem;        // memory for ISOP computation
*/
    // internal data
    Rnm_Obj_t *     pObjs;           // refinement objects
    int             nObjs;           // the number of used objects
    int             nObjsAlloc;      // the number of allocated objects
    int             nObjsFrame;      // the number of used objects in each frame
    int             nCalls;          // total number of calls
    int             nRefines;        // total refined objects
    int             nVisited;        // visited during justification
    // statistics  
    abctime         timeFwd;         // forward propagation
    abctime         timeBwd;         // backward propagation
    abctime         timeVer;         // ternary simulation
    abctime         timeTotal;       // other time
};

// accessing the refinement object
static inline Rnm_Obj_t * Rnm_ManObj( Rnm_Man_t * p, Gia_Obj_t * pObj, int f )  
{ 
    assert( Gia_ObjIsConst0(pObj) || pObj->Value );
    assert( (int)pObj->Value < p->nObjsFrame );
    assert( f >= 0 && f <= p->pCex->iFrame ); 
    return p->pObjs + f * p->nObjsFrame + pObj->Value;  
}
static inline void  Rnm_ManSetRefId( Rnm_Man_t * p, int RefId )               { p->nRefId = RefId; }

static inline int   Rnm_ObjCount( Rnm_Man_t * p, Gia_Obj_t * pObj )           { return Vec_StrEntry( p->vCounts, Gia_ObjId(p->pGia, pObj) );                           }
static inline void  Rnm_ObjSetCount( Rnm_Man_t * p, Gia_Obj_t * pObj, int c ) { Vec_StrWriteEntry( p->vCounts, Gia_ObjId(p->pGia, pObj), (char)c );                    }
static inline int   Rnm_ObjAddToCount( Rnm_Man_t * p, Gia_Obj_t * pObj )      { int c = Rnm_ObjCount(p, pObj); if ( c < 16 )  Rnm_ObjSetCount(p, pObj, c+1); return c; }

static inline int   Rnm_ObjIsJust( Rnm_Man_t * p, Gia_Obj_t * pObj )          { return Gia_ObjIsConst0(pObj) || (pObj->Value && Rnm_ManObj(p, pObj, 0)->fVisitJ);      }

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== absRef.c ===========================================================*/
extern Rnm_Man_t *  Rnm_ManStart( Gia_Man_t * pGia );
extern void         Rnm_ManStop( Rnm_Man_t * p, int fProfile );
extern double       Rnm_ManMemoryUsage( Rnm_Man_t * p );
extern Vec_Int_t *  Rnm_ManRefine( Rnm_Man_t * p, Abc_Cex_t * pCex, Vec_Int_t * vMap, int fPropFanout, int fNewRefinement, int fVerbose );
/*=== absRefSelected.c ===========================================================*/
extern Vec_Int_t *  Rnm_ManFilterSelected( Rnm_Man_t * p, Vec_Int_t * vOldPPis );
extern Vec_Int_t *  Rnm_ManFilterSelectedNew( Rnm_Man_t * p, Vec_Int_t * vOldPPis );


ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

