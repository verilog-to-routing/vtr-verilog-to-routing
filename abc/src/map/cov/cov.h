/**CFile****************************************************************

  FileName    [cov.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Mapping into network of SOPs/ESOPs.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
   
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cov.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__map__cov__cov_h
#define ABC__map__cov__cov_h

#include "base/abc/abc.h"
#include "covInt.h"


ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Cov_Man_t_  Cov_Man_t;
typedef struct Cov_Obj_t_  Cov_Obj_t;

// storage for node information
struct Cov_Obj_t_
{
    Min_Cube_t *      pCover[3];    // pos/neg/esop
    Vec_Int_t *       vSupp;        // computed support (all nodes except CIs)
};

// storage for additional information
struct Cov_Man_t_
{
    // general characteristics
    int               nFaninMax;    // the number of vars
    int               nCubesMax;    // the limit on the number of cubes in the intermediate covers
    int               nWords;       // the number of words
    Vec_Int_t *       vFanCounts;   // fanout counts
    Vec_Ptr_t *       vObjStrs;     // object structures
    void *            pMemory;      // memory for the internal data strctures
    Min_Man_t *       pManMin;      // the cube manager
    int               fUseEsop;     // enables ESOPs
    int               fUseSop;      // enables SOPs
    // arrays to map local variables
    Vec_Int_t *       vComTo0;      // mapping of common variables into first fanin
    Vec_Int_t *       vComTo1;      // mapping of common variables into second fanin
    Vec_Int_t *       vPairs0;      // the first var in each pair of common vars
    Vec_Int_t *       vPairs1;      // the second var in each pair of common vars
    Vec_Int_t *       vTriv0;       // trival support of the first node
    Vec_Int_t *       vTriv1;       // trival support of the second node
    // statistics
    int               nSupps;       // supports created
    int               nSuppsMax;    // the maximum number of supports
    int               nBoundary;    // the boundary size
    int               nNodes;       // the number of nodes processed
};

static inline Cov_Obj_t *  Abc_ObjGetStr( Abc_Obj_t * pObj )                       { return (Cov_Obj_t *)Vec_PtrEntry(((Cov_Man_t *)pObj->pNtk->pManCut)->vObjStrs, pObj->Id); }

static inline void         Abc_ObjSetSupp( Abc_Obj_t * pObj, Vec_Int_t * vVec )    { Abc_ObjGetStr(pObj)->vSupp = vVec;   }
static inline Vec_Int_t *  Abc_ObjGetSupp( Abc_Obj_t * pObj )                      { return Abc_ObjGetStr(pObj)->vSupp;   }

static inline void         Abc_ObjSetCover2( Abc_Obj_t * pObj, Min_Cube_t * pCov ) { Abc_ObjGetStr(pObj)->pCover[2] = pCov; }
static inline Min_Cube_t * Abc_ObjGetCover2( Abc_Obj_t * pObj )                    { return Abc_ObjGetStr(pObj)->pCover[2]; }

static inline void         Abc_ObjSetCover( Abc_Obj_t * pObj, Min_Cube_t * pCov, int Pol ) { Abc_ObjGetStr(pObj)->pCover[Pol] = pCov; }
static inline Min_Cube_t * Abc_ObjGetCover( Abc_Obj_t * pObj, int Pol )                    { return Abc_ObjGetStr(pObj)->pCover[Pol]; }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== covBuild.c ==========================================================*/
extern Abc_Ntk_t * Abc_NtkCovDerive( Cov_Man_t * p, Abc_Ntk_t * pNtk );
extern Abc_Ntk_t * Abc_NtkCovDeriveClean( Cov_Man_t * p, Abc_Ntk_t * pNtk );
extern Abc_Ntk_t * Abc_NtkCovDeriveRegular( Cov_Man_t * p, Abc_Ntk_t * pNtk );
/*=== covCore.c ===========================================================*/
extern Abc_Ntk_t * Abc_NtkSopEsopCover( Abc_Ntk_t * pNtk, int nFaninMax, int nCubesMax, int fUseEsop, int fUseSop, int fUseInvs, int fVerbose );
/*=== covMan.c ============================================================*/
extern Cov_Man_t * Cov_ManAlloc( Abc_Ntk_t * pNtk, int nFaninMax, int nCubesMax );
extern void        Cov_ManFree( Cov_Man_t * p );
extern void        Abc_NodeCovDropData( Cov_Man_t * p, Abc_Obj_t * pObj );
/*=== covTest.c ===========================================================*/
extern Abc_Ntk_t * Abc_NtkCovTestSop( Abc_Ntk_t * pNtk );


ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



