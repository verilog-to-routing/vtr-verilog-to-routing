/**CFile****************************************************************

  FileName    [fxu.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [External declarations of fast extract for unate covers.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fxu.h,v 1.0 2003/02/01 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef __FXU_H__
#define __FXU_H__

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "vec.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

#ifndef __cplusplus
#ifndef bool
#define bool int
#endif
#endif

typedef struct FxuDataStruct   Fxu_Data_t;

// structure for the FX input/output data 
struct FxuDataStruct
{
    // user specified parameters
    bool              fOnlyS;           // set to 1 to have only single-cube divs
    bool              fOnlyD;           // set to 1 to have only double-cube divs
    bool              fUse0;            // set to 1 to have 0-weight also extracted
    bool              fUseCompl;        // set to 1 to have complement taken into account
    bool              fVerbose;         // set to 1 to have verbose output
    int               nNodesExt;        // the number of divisors to extract
    int               nPairsMax;        // the maximum number of cube pairs to consider
    // the input information
    Vec_Ptr_t *       vSops;            // the SOPs for each node in the network
    Vec_Ptr_t *       vFanins;          // the fanins of each node in the network
    // output information
    Vec_Ptr_t *       vSopsNew;         // the SOPs for each node in the network after extraction
    Vec_Ptr_t *       vFaninsNew;       // the fanins of each node in the network after extraction
    // the SOP manager
    Extra_MmFlex_t *  pManSop;
    // statistics   
    int               nNodesOld;        // the old number of nodes
    int               nNodesNew;        // the number of divisors actually extracted
};

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/*===== fxu.c ==========================================================*/
extern int          Fxu_FastExtract( Fxu_Data_t * pData );

#ifdef __cplusplus
}
#endif

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

