/**CFile****************************************************************

  FileName    [sfmMit.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based optimization using internal don't-cares.]

  Synopsis    [Timing manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: sfmMit.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sfmInt.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

struct Sfm_Mit_t_
{
    int      Value1;
    int      Value2;
    int      Value3;
    int      Value4;
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Temporary place holders.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sfm_Mit_t *  Sfm_MitStart( Mio_Library_t * pLib, SC_Lib * pScl, Scl_Con_t * pExt, Abc_Ntk_t * pNtk, int DeltaCrit ) { return NULL; }
void         Sfm_MitStop( Sfm_Mit_t * p )                                                           {}
int          Sfm_MitReadNtkDelay( Sfm_Mit_t * p )                                                   { return 0;}
int          Sfm_MitReadNtkMinSlack( Sfm_Mit_t * p )                                                { return 0;}
int          Sfm_MitReadObjDelay( Sfm_Mit_t * p, int iObj )                                         { return 0;}
void         Sfm_MitTransferLoad( Sfm_Mit_t * p, Abc_Obj_t * pNew, Abc_Obj_t * pOld )               {};
void         Sfm_MitTimingGrow( Sfm_Mit_t * p )                                                     {};
void         Sfm_MitUpdateLoad( Sfm_Mit_t * p, Vec_Int_t * vTimeNodes, int fAdd )                   {}
void         Sfm_MitUpdateTiming( Sfm_Mit_t * p, Vec_Int_t * vTimeNodes )                           {}
int          Sfm_MitSortArrayByArrival( Sfm_Mit_t * p, Vec_Int_t * vNodes, int iPivot )             { return 0;}
int          Sfm_MitPriorityNodes( Sfm_Mit_t * p, Vec_Int_t * vCands, int Window )                  { return 0;}
int          Sfm_MitNodeIsNonCritical( Sfm_Mit_t * p, Abc_Obj_t * pPivot, Abc_Obj_t * pNode )       { return 0;}
int          Sfm_MitEvalRemapping( Sfm_Mit_t * p, Vec_Int_t * vMffc, Abc_Obj_t * pObj, Vec_Int_t * vFanins, Vec_Int_t * vMap, Mio_Gate_t * pGate1, char * pFans1, Mio_Gate_t * pGate2, char * pFans2 ) { return 0;}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

