/**CFile****************************************************************

  FileName    [acbPar.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchical word-level netlist.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - July 21, 2015.]

  Revision    [$Id: acbPar.h,v 1.00 2014/11/29 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__base__acb__acbPar_h
#define ABC__base__acb__acbPar_h

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

typedef struct Acb_Par_t_ Acb_Par_t;
struct Acb_Par_t_
{
    int             nLutSize;      // LUT size
    int             nTfoLevMax;    // the maximum fanout levels
    int             nTfiLevMax;    // the maximum fanin levels
    int             nFanoutMax;    // the maximum number of fanouts
    int             nWinNodeMax;   // the maximum number of nodes in the window
    int             nGrowthLevel;  // the maximum allowed growth in level
    int             nBTLimit;      // the maximum number of conflicts in one SAT run
    int             nNodesMax;     // the maximum number of nodes to try
    int             fUseAshen;     // user Ashenhurst decomposition
    int             iNodeOne;      // one particular node to try
    int             fArea;         // performs optimization for area
    int             fMoreEffort;   // performs high-affort minimization
    int             fVerbose;      // enable basic stats
    int             fVeryVerbose;  // enable detailed stats
};


/*=== acbAbc.c =============================================================*/
extern void   Acb_ParSetDefault( Acb_Par_t * pPars );


ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


