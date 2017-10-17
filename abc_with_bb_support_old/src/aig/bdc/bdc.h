/**CFile****************************************************************

  FileName    [bdc.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Truth-table-based bi-decomposition engine.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 30, 2007.]

  Revision    [$Id: bdc.h,v 1.00 2007/01/30 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef __BDC_H__
#define __BDC_H__
 
#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Bdc_Man_t_ Bdc_Man_t;
typedef struct Bdc_Par_t_ Bdc_Par_t;
struct Bdc_Par_t_
{
    // general parameters
    int           nVarsMax;      // the maximum support
    int           fVerbose;      // enable basic stats
    int           fVeryVerbose;  // enable detailed stats
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== bdcCore.c ==========================================================*/
extern Bdc_Man_t * Bdc_ManAlloc( Bdc_Par_t * pPars );
extern void        Bdc_ManFree( Bdc_Man_t * p );
extern int         Bdc_ManDecompose( Bdc_Man_t * p, unsigned * puFunc, unsigned * puCare, int nVars, Vec_Ptr_t * vDivs, int nNodesLimit );


#ifdef __cplusplus
}
#endif

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

