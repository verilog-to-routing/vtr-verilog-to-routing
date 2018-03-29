/**CFile****************************************************************

  FileName    [lpk.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Fast Boolean matching for LUT structures.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: lpk.h,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__opt__lpk__lpk_h
#define ABC__opt__lpk__lpk_h


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

typedef struct Lpk_Par_t_ Lpk_Par_t;
struct Lpk_Par_t_
{
    // user-controlled parameters
    int               nLutsMax;      // (N) the maximum number of LUTs in the structure
    int               nLutsOver;     // (Q) the maximum number of LUTs not in the MFFC
    int               nVarsShared;   // (S) the maximum number of shared variables (crossbars)
    int               nGrowthLevel;  // (L) the maximum increase in the node level after resynthesis
    int               fSatur;        // iterate till saturation
    int               fZeroCost;     // accept zero-cost replacements
    int               fFirst;        // use root node and first cut only
    int               fOldAlgo;      // use old algorithm
    int               fVerbose;      // the verbosiness flag
    int               fVeryVerbose;  // additional verbose info printout
    // internal parameters
    int               nLutSize;      // (K) the LUT size (determined by the input network)
    int               nVarsMax;      // (V) the largest number of variables: V = N * (K-1) + 1
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                           ITERATORS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== lpkCore.c ========================================================*/
extern int     Lpk_Resynthesize( Abc_Ntk_t * pNtk, Lpk_Par_t * pPars );




ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

