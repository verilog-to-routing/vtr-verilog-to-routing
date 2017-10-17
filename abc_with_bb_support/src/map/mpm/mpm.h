/**CFile****************************************************************

  FileName    [mpm.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Configurable technology mapper.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 1, 2013.]

  Revision    [$Id: mpm.h,v 1.00 2013/06/01 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__map__mpm__h
#define ABC__map__mpm__h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

#define MPM_VAR_MAX  12  

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Mpm_LibLut_t_ Mpm_LibLut_t;
struct Mpm_LibLut_t_
{
    char *           pName;                                     // the name of the LUT library
    int              LutMax;                                    // the maximum LUT size 
    int              fVarPinDelays;                             // set to 1 if variable pin delays are specified
    int              pLutAreas[MPM_VAR_MAX+1];                  // the areas of LUTs
    int              pLutDelays[MPM_VAR_MAX+1][MPM_VAR_MAX+1];  // the delays of LUTs
};

typedef struct Mpm_Par_t_ Mpm_Par_t;
struct Mpm_Par_t_
{
    Mpm_LibLut_t *   pLib;
    void *           pScl;
    int              nNumCuts;
    int              DelayTarget;
    int              fUseGates;
    int              fUseTruth;
    int              fUseDsd;
    int              fCutMin;
    int              fOneRound;
    int              fDeriveLuts;
    int              fMap4Cnf;
    int              fMap4Aig;
    int              fMap4Gates;
    int              fVerbose;
    int              fVeryVerbose;
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== mpmCore.c ===========================================================*/
extern void           Mpm_ManSetParsDefault( Mpm_Par_t * p );
/*=== mpmLib.c ===========================================================*/
extern Mpm_LibLut_t * Mpm_LibLutSetSimple( int nLutSize );
extern void           Mpm_LibLutFree( Mpm_LibLut_t * pLib );

ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

