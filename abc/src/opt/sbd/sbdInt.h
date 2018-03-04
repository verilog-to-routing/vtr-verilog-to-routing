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
 
#ifndef ABC__opt_sbdInt__h
#define ABC__opt_sbdInt__h


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
#include "misc/util/utilTruth.h"
#include "map/scl/sclLib.h"
#include "map/scl/sclCon.h"
#include "bool/kit/kit.h"
#include "misc/st/st.h"
#include "map/mio/mio.h"
#include "base/abc/abc.h"
#include "sbd.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START

#define SBD_SAT_UNDEC 0x1234567812345678
#define SBD_SAT_SAT   0x8765432187654321

#define SBD_LUTS_MAX    2
#define SBD_SIZE_MAX    4
#define SBD_DIV_MAX    10
#define SBD_FVAR_MAX  100

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Sbd_Sto_t_ Sbd_Sto_t;
typedef struct Sbd_Srv_t_ Sbd_Srv_t; 

typedef struct Sbd_Str_t_ Sbd_Str_t;
struct Sbd_Str_t_
{
    int               fLut;                 // LUT or SEL
    int               nVarIns;              // input count
    int               VarIns[SBD_DIV_MAX];  // input vars
    word              Res;                  // result of solving
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== sbdCut.c ==========================================================*/
extern Sbd_Sto_t *  Sbd_StoAlloc( Gia_Man_t * pGia, Vec_Int_t * vMirrors, int nLutSize, int nCutSize, int nCutNum, int fCutMin, int fVerbose );
extern void         Sbd_StoFree( Sbd_Sto_t * p );
extern int          Sbd_StoObjRefs( Sbd_Sto_t * p, int iObj );
extern void         Sbd_StoRefObj( Sbd_Sto_t * p, int iObj, int iMirror );
extern void         Sbd_StoDerefObj( Sbd_Sto_t * p, int iObj );
extern void         Sbd_StoComputeCutsConst0( Sbd_Sto_t * p, int iObj );
extern void         Sbd_StoComputeCutsObj( Sbd_Sto_t * p, int iObj, int Delay, int Level );
extern void         Sbd_StoComputeCutsCi( Sbd_Sto_t * p, int iObj, int Delay, int Level );
extern int          Sbd_StoComputeCutsNode( Sbd_Sto_t * p, int iObj );
extern void         Sbd_StoSaveBestDelayCut( Sbd_Sto_t * p, int iObj, int * pCut );
extern int          Sbd_StoObjBestCut( Sbd_Sto_t * p, int iObj, int nSize, int * pLeaves );
/*=== sbdCut2.c ==========================================================*/
extern Sbd_Srv_t *  Sbd_ManCutServerStart( Gia_Man_t * pGia, Vec_Int_t * vMirrors, 
                                   Vec_Int_t * vLutLevs, Vec_Int_t * vLevs, Vec_Int_t * vRefs, 
                                   int nLutSize, int nCutSize, int nCutNum, int fVerbose );
extern void         Sbd_ManCutServerStop( Sbd_Srv_t * p );
extern int          Sbd_ManCutServerFirst( Sbd_Srv_t * p, int iObj, int * pLeaves );
/*=== sbdWin.c ==========================================================*/
extern word         Sbd_ManSolve( sat_solver * pSat, int PivotVar, int FreeVar, Vec_Int_t * vDivSet, Vec_Int_t * vDivVars, Vec_Int_t * vDivValues, Vec_Int_t * vTemp );
extern sat_solver * Sbd_ManSatSolver( sat_solver * pSat, Gia_Man_t * p, Vec_Int_t * vMirrors, int Pivot, Vec_Int_t * vWinObjs, Vec_Int_t * vObj2Var, Vec_Int_t * vTfo, Vec_Int_t * vRoots, int fQbf );
extern int          Sbd_ManCollectConstants( sat_solver * pSat, int nCareMints[2], int PivotVar, word * pVarSims[], Vec_Int_t * vInds );
extern int          Sbd_ManCollectConstantsNew( sat_solver * pSat, Vec_Int_t * vDivVars, int nConsts, int PivotVar, word * pOnset, word * pOffset );
/*=== sbdPath.c ==========================================================*/
extern Vec_Bit_t *  Sbc_ManCriticalPath( Gia_Man_t * p );
/*=== sbdQbf.c ==========================================================*/
extern int          Sbd_ProblemSolve( 
                                       Gia_Man_t * p, Vec_Int_t * vMirrors, 
                                       int Pivot, Vec_Int_t * vWinObjs, Vec_Int_t * vObj2Var, 
                                       Vec_Int_t * vTfo, Vec_Int_t * vRoots, 
                                       Vec_Int_t * vDivSet, int nStrs, Sbd_Str_t * pStr0 
                                    );

ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

