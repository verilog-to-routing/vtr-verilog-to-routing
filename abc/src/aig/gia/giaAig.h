/**CFile****************************************************************

  FileName    [giaAig.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaAig.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__aig__gia__giaAig_h
#define ABC__aig__gia__giaAig_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "aig/aig/aig.h"
#include "gia.h"

ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////
 
////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== giaAig.c =============================================================*/
extern Gia_Man_t *         Gia_ManFromAig( Aig_Man_t * p );
extern Gia_Man_t *         Gia_ManFromAigChoices( Aig_Man_t * p );
extern Gia_Man_t *         Gia_ManFromAigSimple( Aig_Man_t * p );
extern Gia_Man_t *         Gia_ManFromAigSwitch( Aig_Man_t * p );
extern Aig_Man_t *         Gia_ManToAig( Gia_Man_t * p, int fChoices );
extern Aig_Man_t *         Gia_ManToAigSkip( Gia_Man_t * p, int nOutDelta );
extern Aig_Man_t *         Gia_ManToAigSimple( Gia_Man_t * p );
extern void                Gia_ManReprToAigRepr( Aig_Man_t * pAig, Gia_Man_t * pGia );
extern void                Gia_ManReprToAigRepr2( Aig_Man_t * pAig, Gia_Man_t * pGia );
extern void                Gia_ManReprFromAigRepr( Aig_Man_t * pAig, Gia_Man_t * pGia );
extern void                Gia_ManReprFromAigRepr2( Aig_Man_t * pAig, Gia_Man_t * pGia );
extern Gia_Man_t *         Gia_ManCompress2( Gia_Man_t * p, int fUpdateLevel, int fVerbose );
extern Gia_Man_t *         Gia_ManPerformDch( Gia_Man_t * p, void * pPars );
extern Gia_Man_t *         Gia_ManAbstraction( Gia_Man_t * p, Vec_Int_t * vFlops );
extern void                Gia_ManSeqCleanupClasses( Gia_Man_t * p, int fConst, int fEquiv, int fVerbose );
extern int                 Gia_ManSolveSat( Gia_Man_t * p );


ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

