/**CFile****************************************************************

  FileName    [intInt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Interpolation engine.]

  Synopsis    [Internal declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 24, 2008.]

  Revision    [$Id: intInt.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__aig__int__intInt_h
#define ABC__aig__int__intInt_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "aig/saig/saig.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satSolver.h"
#include "sat/bsat/satStore.h"
#include "int.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

// interpolation manager
typedef struct Inter_Man_t_ Inter_Man_t;
struct Inter_Man_t_
{
    // AIG manager
    Aig_Man_t *      pAig;         // the original AIG manager
    Aig_Man_t *      pAigTrans;    // the transformed original AIG manager
    Cnf_Dat_t *      pCnfAig;      // CNF for the original manager
    // interpolant
    Aig_Man_t *      pInter;       // the current interpolant
    Cnf_Dat_t *      pCnfInter;    // CNF for the current interplant
    // timeframes
    Aig_Man_t *      pFrames;      // the timeframes      
    Cnf_Dat_t *      pCnfFrames;   // CNF for the timeframes 
    // other data
    Vec_Int_t *      vVarsAB;      // the variables participating in 
    // temporary place for the new interpolant
    Aig_Man_t *      pInterNew;
    Vec_Ptr_t *      vInters;
    // parameters
    int              nFrames;      // the number of timeframes
    int              nConfCur;     // the current number of conflicts
    int              nConfLimit;   // the limit on the number of conflicts
    int              fVerbose;     // the verbosiness flag
    char *           pFileName;
    // runtime
    abctime          timeRwr;
    abctime          timeCnf;
    abctime          timeSat;
    abctime          timeInt;
    abctime          timeEqu;
    abctime          timeOther;
    abctime          timeTotal;
};

// containment checking manager
typedef struct Inter_Check_t_ Inter_Check_t;

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== intCheck.c ============================================================*/
extern Inter_Check_t * Inter_CheckStart( Aig_Man_t * pTrans, int nFramesK );
extern void            Inter_CheckStop( Inter_Check_t * p );
extern int             Inter_CheckPerform( Inter_Check_t * p, Cnf_Dat_t * pCnf, abctime nTimeNewOut );

/*=== intContain.c ============================================================*/
extern int             Inter_ManCheckContainment( Aig_Man_t * pNew, Aig_Man_t * pOld );
extern int             Inter_ManCheckEquivalence( Aig_Man_t * pNew, Aig_Man_t * pOld );
extern int             Inter_ManCheckInductiveContainment( Aig_Man_t * pTrans, Aig_Man_t * pInter, int nSteps, int fBackward );

/*=== intCtrex.c ============================================================*/
extern void *          Inter_ManGetCounterExample( Aig_Man_t * pAig, int nFrames, int fVerbose );

/*=== intDup.c ============================================================*/
extern Aig_Man_t *     Inter_ManStartInitState( int nRegs );
extern Aig_Man_t *     Inter_ManStartDuplicated( Aig_Man_t * p );
extern Aig_Man_t *     Inter_ManStartOneOutput( Aig_Man_t * p, int fAddFirstPo );

/*=== intFrames.c ============================================================*/
extern Aig_Man_t *     Inter_ManFramesInter( Aig_Man_t * pAig, int nFrames, int fAddRegOuts, int fUseTwoFrames );

/*=== intMan.c ============================================================*/
extern Inter_Man_t *   Inter_ManCreate( Aig_Man_t * pAig, Inter_ManParams_t * pPars );
extern void            Inter_ManClean( Inter_Man_t * p );
extern void            Inter_ManStop( Inter_Man_t * p, int fProved );

/*=== intM114.c ============================================================*/
extern int             Inter_ManPerformOneStep( Inter_Man_t * p, int fUseBias, int fUseBackward, abctime nTimeNewOut );

/*=== intM114p.c ============================================================*/
#ifdef ABC_USE_LIBRARIES
extern int             Inter_ManPerformOneStepM114p( Inter_Man_t * p, int fUsePudlak, int fUseOther );
#endif

/*=== intUtil.c ============================================================*/
extern int             Inter_ManCheckInitialState( Aig_Man_t * p );
extern int             Inter_ManCheckAllStates( Aig_Man_t * p );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

