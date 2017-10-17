/**CFile****************************************************************

  FileName    [retInt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Retiming package.]

  Synopsis    [Internal declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - Oct 31, 2006.]

  Revision    [$Id: retInt.h,v 1.00 2006/10/31 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__opt__ret__retInt_h
#define ABC__opt__ret__retInt_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "base/abc/abc.h"

ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== retArea.c ========================================================*/
extern int         Abc_NtkRetimeMinArea( Abc_Ntk_t * pNtk, int fForwardOnly, int fBackwardOnly, int fUseOldNames, int fVerbose );
/*=== retCore.c ========================================================*/
extern int         Abc_NtkRetime( Abc_Ntk_t * pNtk, int Mode, int nDelayLim, int fForwardOnly, int fBackwardOnly, int fOneStep, int fUseOldNames, int fVerbose );
/*=== retDelay.c ========================================================*/
extern int         Abc_NtkRetimeMinDelay( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkCopy, int nDelayLim, int nIterLimit, int fForward, int fVerbose );
/*=== retDirect.c ========================================================*/
extern int         Abc_NtkRetimeIncremental( Abc_Ntk_t * pNtk, int nDelayLim, int fForward, int fMinDelay, int fOneStep, int fUseOldNames, int fVerbose );
extern void        Abc_NtkRetimeShareLatches( Abc_Ntk_t * pNtk, int fInitial );
extern int         Abc_NtkRetimeNodeIsEnabled( Abc_Obj_t * pObj, int fForward );
extern void        Abc_NtkRetimeNode( Abc_Obj_t * pObj, int fForward, int fInitial );
extern st__table *  Abc_NtkRetimePrepareLatches( Abc_Ntk_t * pNtk );
extern int         Abc_NtkRetimeFinalizeLatches( Abc_Ntk_t * pNtk, st__table * tLatches, int nIdMaxStart, int fUseOldNames );
/*=== retFlow.c ========================================================*/
extern void        Abc_NtkMaxFlowTest( Abc_Ntk_t * pNtk );
extern Vec_Ptr_t * Abc_NtkMaxFlow( Abc_Ntk_t * pNtk, int fForward, int fVerbose );
/*=== retInit.c ========================================================*/
extern Vec_Int_t * Abc_NtkRetimeInitialValues( Abc_Ntk_t * pNtkSat, Vec_Int_t * vValues, int fVerbose );
extern int         Abc_ObjSopSimulate( Abc_Obj_t * pObj );
extern void        Abc_NtkRetimeTranferToCopy( Abc_Ntk_t * pNtk );
extern void        Abc_NtkRetimeTranferFromCopy( Abc_Ntk_t * pNtk );
extern Vec_Int_t * Abc_NtkRetimeCollectLatchValues( Abc_Ntk_t * pNtk );
extern void        Abc_NtkRetimeInsertLatchValues( Abc_Ntk_t * pNtk, Vec_Int_t * vValues );
extern Abc_Ntk_t * Abc_NtkRetimeBackwardInitialStart( Abc_Ntk_t * pNtk );
extern void        Abc_NtkRetimeBackwardInitialFinish( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkNew, Vec_Int_t * vValuesOld, int fVerbose );
/*=== retLvalue.c ========================================================*/
extern int         Abc_NtkRetimeLValue( Abc_Ntk_t * pNtk, int nIterLimit, int fVerbose );



ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


