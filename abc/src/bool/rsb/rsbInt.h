/**CFile****************************************************************

  FileName    [rsbInt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Truth-table based resubstitution.]

  Synopsis    [Internal declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: rsbInt.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__bool_RsbInt_h
#define ABC__bool_RsbInt_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "misc/vec/vec.h"
#include "misc/util/utilTruth.h"
#include "bool/kit/kit.h"
#include "rsb.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

// truth table computation manager
struct Rsb_Man_t_
{
    // parameters
    int                nLeafMax;     // the max number of leaves of a cut
    int                nDivMax;      // the max number of divisors to collect
    int                nDecMax;      // the max number of decompositions
    int                fVerbose;     // verbosity level
    // decomposition
    Vec_Wrd_t *        vCexes;       // counter-examples
    Vec_Int_t *        vDecPats;     // decomposition patterns
    Vec_Int_t *        vFanins;      // the result of decomposition
    Vec_Int_t *        vFaninsOld;   // original fanins
    Vec_Int_t *        vTries;       // intermediate
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== rsbMan.c ==========================================================*/

ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

