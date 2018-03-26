/**CFile****************************************************************

  FileName    [fxu.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [External declarations of fast extract for unate covers.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fxu.h,v 1.0 2003/02/01 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__opt__fxu__fxu_h
#define ABC__opt__fxu__fxu_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "misc/vec/vec.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct FxuDataStruct   Fxu_Data_t;

// structure for the FX input/output data 
struct FxuDataStruct
{
    // user specified parameters
    int               fOnlyS;           // set to 1 to have only single-cube divs
    int               fOnlyD;           // set to 1 to have only double-cube divs
    int               fUse0;            // set to 1 to have 0-weight also extracted
    int               fUseCompl;        // set to 1 to have complement taken into account
    int               fVerbose;         // set to 1 to have verbose output
    int               fVeryVerbose;     // set to 1 to have more verbose output
    int               nNodesExt;        // the number of divisors to extract
    int               nSingleMax;       // the max number of single-cube divisors to consider
    int               nPairsMax;        // the max number of double-cube divisors to consider
    int               WeightMin;        // the min weight of a divisor to extract
    int               LitCountMax;      // the max literal count of a divisor to consider
    int               fCanonDivs;       // use only canonical divisors (AND/XOR/MUX) 
    // the input information
    Vec_Ptr_t *       vSops;            // the SOPs for each node in the network
    Vec_Ptr_t *       vFanins;          // the fanins of each node in the network
    // output information
    Vec_Ptr_t *       vSopsNew;         // the SOPs for each node in the network after extraction
    Vec_Ptr_t *       vFaninsNew;       // the fanins of each node in the network after extraction
    // the SOP manager
    Mem_Flex_t *      pManSop;
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
extern void   Abc_NtkSetDefaultFxParams( Fxu_Data_t * p );
extern int    Abc_NtkFastExtract( Abc_Ntk_t * pNtk, Fxu_Data_t * p );
extern void   Abc_NtkFxuFreeInfo( Fxu_Data_t * p );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

