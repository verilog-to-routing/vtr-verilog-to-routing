/**CFile****************************************************************

  FileName    [res.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Resynthesis package.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 15, 2007.]

  Revision    [$Id: res.h,v 1.00 2007/01/15 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef __RES_H__
#define __RES_H__

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

typedef struct Res_Par_t_ Res_Par_t;
struct Res_Par_t_
{
    // general parameters
    int           nWindow;       // window size
    int           nGrowthLevel;  // the maximum allowed growth in level after one iteration of resynthesis
    int           nSimWords;     // the number of simulation words 
    int           nCands;        // the number of candidates to try
    int           fArea;         // performs optimization for area
    int           fDelay;        // performs optimization for delay
    int           fVerbose;      // enable basic stats
    int           fVeryVerbose;  // enable detailed stats
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== resCore.c ==========================================================*/
extern int        Abc_NtkResynthesize( Abc_Ntk_t * pNtk, Res_Par_t * pPars );


#ifdef __cplusplus
}
#endif

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

