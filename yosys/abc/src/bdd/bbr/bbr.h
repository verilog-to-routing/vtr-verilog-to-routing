/**CFile****************************************************************

  FileName    [bbr.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [BDD-based reachability analysis.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: bbr.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__aig__bbr__bbr_h
#define ABC__aig__bbr__bbr_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "aig/aig/aig.h"
#include "aig/saig/saig.h"
#include "bdd/cudd/cuddInt.h"

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

static inline DdNode * Aig_ObjGlobalBdd( Aig_Obj_t * pObj )  { return (DdNode *)pObj->pData; }

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== bbrImage.c ==========================================================*/
typedef struct Bbr_ImageTree_t_  Bbr_ImageTree_t;
extern Bbr_ImageTree_t * Bbr_bddImageStart( 
    DdManager * dd, DdNode * bCare,
    int nParts, DdNode ** pbParts,
    int nVars, DdNode ** pbVars, int nBddMax, int fVerbose );
extern DdNode *    Bbr_bddImageCompute( Bbr_ImageTree_t * pTree, DdNode * bCare );
extern void        Bbr_bddImageTreeDelete( Bbr_ImageTree_t * pTree );
extern DdNode *    Bbr_bddImageRead( Bbr_ImageTree_t * pTree );
typedef struct Bbr_ImageTree2_t_  Bbr_ImageTree2_t;
extern Bbr_ImageTree2_t * Bbr_bddImageStart2( 
    DdManager * dd, DdNode * bCare,
    int nParts, DdNode ** pbParts,
    int nVars, DdNode ** pbVars, int fVerbose );
extern DdNode *    Bbr_bddImageCompute2( Bbr_ImageTree2_t * pTree, DdNode * bCare );
extern void        Bbr_bddImageTreeDelete2( Bbr_ImageTree2_t * pTree );
extern DdNode *    Bbr_bddImageRead2( Bbr_ImageTree2_t * pTree );
/*=== bbrNtbdd.c ==========================================================*/
extern void        Aig_ManFreeGlobalBdds( Aig_Man_t * p, DdManager * dd );
extern int         Aig_ManSizeOfGlobalBdds( Aig_Man_t * p );
extern DdManager * Aig_ManComputeGlobalBdds( Aig_Man_t * p, int nBddSizeMax, int fDropInternal, int fReorder, int fVerbose );
/*=== bbrReach.c ==========================================================*/
extern int         Aig_ManVerifyUsingBdds( Aig_Man_t * p, Saig_ParBbr_t * pPars );
extern void        Bbr_ManSetDefaultParams( Saig_ParBbr_t * p );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

