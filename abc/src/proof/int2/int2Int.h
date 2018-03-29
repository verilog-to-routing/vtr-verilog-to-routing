/**CFile****************************************************************

  FileName    [int2Int.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Interpolation engine.]

  Synopsis    [Internal declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - Dec 1, 2013.]

  Revision    [$Id: int2Int.h,v 1.00 2013/12/01 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__Gia__int2__intInt_h
#define ABC__Gia__int2__intInt_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "aig/gia/gia.h"
#include "sat/bsat/satSolver.h"
#include "sat/cnf/cnf.h"
#include "int2.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

// interpolation manager
typedef struct Int2_Man_t_ Int2_Man_t;
struct Int2_Man_t_
{
    // parameters
    Int2_ManPars_t * pPars;        // parameters
    // GIA managers
    Gia_Man_t *      pGia;         // original manager
    Gia_Man_t *      pGiaPref;     // prefix manager
    Gia_Man_t *      pGiaSuff;     // suffix manager
    // subset of the manager
    Vec_Int_t *      vSuffCis;     // suffix CIs
    Vec_Int_t *      vSuffCos;     // suffix COs
    Vec_Int_t *      vPrefCos;     // suffix POs
    Vec_Int_t *      vStack;       // temporary stack
    // preimages
    Vec_Int_t *      vImageOne;    // latest preimage
    Vec_Int_t *      vImagesAll;   // cumulative preimage
    // variable maps
    Vec_Ptr_t *      vMapFrames;   // mapping of GIA IDs into frame IDs
    Vec_Int_t *      vMapPref;     // mapping of flop inputs into SAT variables
    Vec_Int_t *      vMapSuff;     // mapping of flop outputs into SAT variables
    // initial minimization
    Vec_Int_t *      vAssign;      // assignment of PIs in pGiaSuff
    Vec_Int_t *      vPrio;        // priority of PIs in pGiaSuff
    // SAT solving
    sat_solver *     pSatPref;     // prefix solver
    sat_solver *     pSatSuff;     // suffix solver
    // runtime
    abctime          timeSatPref;
    abctime          timeSatSuff;
    abctime          timeOther;
    abctime          timeTotal;
};

static inline Int2_Man_t * Int2_ManCreate( Gia_Man_t * pGia, Int2_ManPars_t * pPars )
{
    Int2_Man_t * p;
    p = ABC_CALLOC( Int2_Man_t, 1 );
    p->pPars       = pPars;
    p->pGia        = pGia;
    p->pGiaPref    = Gia_ManStart( 10000 );
    // perform structural hashing
    Gia_ManHashAlloc( pFrames );
    // subset of the manager
    p->vSuffCis    = Vec_IntAlloc( Gia_ManCiNum(pGia) );
    p->vSuffCos    = Vec_IntAlloc( Gia_ManCoNum(pGia) );
    p->vPrefCos    = Vec_IntAlloc( Gia_ManCoNum(pGia) );
    p->vStack      = Vec_IntAlloc( 10000 );
    // preimages
    p->vImageOne   = Vec_IntAlloc( 1000 );
    p->vImagesAll  = Vec_IntAlloc( 1000 );
    // variable maps
    p->vMapFrames  = Vec_PtrAlloc( 100 );
    p->vMapPref    = Vec_IntAlloc( Gia_ManRegNum(pGia) );
    p->vMapSuff    = Vec_IntAlloc( Gia_ManRegNum(pGia) );
    // initial minimization
    p->vAssign     = Vec_IntAlloc( Gia_ManCiNum(pGia) );
    p->vPrio       = Vec_IntAlloc( Gia_ManCiNum(pGia) );
    return p;
}
static inline void Int2_ManStop( Int2_Man_t * p )
{
    // GIA managers
    Gia_ManStopP( &p->pGiaPref );
    Gia_ManStopP( &p->pGiaSuff );
    // subset of the manager
    Vec_IntFreeP( &p->vSuffCis );
    Vec_IntFreeP( &p->vSuffCos );
    Vec_IntFreeP( &p->vPrefCos );
    Vec_IntFreeP( &p->vStack );
    // preimages
    Vec_IntFreeP( &p->vImageOne );
    Vec_IntFreeP( &p->vImagesAll );
    // variable maps
    Vec_VecFree( (Vec_Vec_t *)p->vMapFrames );
    Vec_IntFreeP( &p->vMapPref );
    Vec_IntFreeP( &p->vMapSuff );
    // initial minimization
    Vec_IntFreeP( &p->vAssign );
    Vec_IntFreeP( &p->vPrio );
    // SAT solving
    if ( p->pSatPref )
        sat_solver_delete( p->pSatPref );
    if ( p->timeSatSuff )
        sat_solver_delete( p->pSatSuff );
    ABC_FREE( p );
}

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== int2Bmc.c =============================================================*/
extern int           Int2_ManCheckInit( Gia_Man_t * p );
extern Gia_Man_t *   Int2_ManDupInit( Gia_Man_t * p, int fVerbose );
extern sat_solver *  Int2_ManSetupBmcSolver( Gia_Man_t * p, int nFrames );
extern void          Int2_ManCreateFrames( Int2_Man_t * p, int iFrame, Vec_Int_t * vPrefCos );
extern int           Int2_ManCheckBmc( Int2_Man_t * p, Vec_Int_t * vCube );

/*=== int2Refine.c =============================================================*/
extern Vec_Int_t *   Int2_ManRefineCube( Gia_Man_t * p, Vec_Int_t * vAssign, Vec_Int_t * vPrio );

/*=== int2Util.c ============================================================*/
extern Gia_Man_t *   Int2_ManProbToGia( Gia_Man_t * p, Vec_Int_t * vSop );


ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

