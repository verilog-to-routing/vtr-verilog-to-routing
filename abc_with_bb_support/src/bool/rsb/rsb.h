/**CFile****************************************************************

  FileName    [rsb.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Truth-table based resubstitution.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: rsb.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__bool_Rsb_h
#define ABC__bool_Rsb_h


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

typedef struct Rsb_Man_t_ Rsb_Man_t;

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== rsbMan.c ==========================================================*/
extern Rsb_Man_t *  Rsb_ManAlloc( int nLeafMax, int nDivMax, int nDecMax, int fVerbose );
extern void         Rsb_ManFree( Rsb_Man_t * p );
extern Vec_Int_t *  Rsb_ManGetFanins( Rsb_Man_t * p );
extern Vec_Int_t *  Rsb_ManGetFaninsOld( Rsb_Man_t * p );
/*=== rsbDec6.c ==========================================================*/
extern int          Rsb_ManPerformResub6( Rsb_Man_t * p, int nVars, word uTruth, Vec_Wrd_t * vDivTruths, word * puTruth0, word * puTruth1, int fVerbose );


ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

