/**CFile****************************************************************

  FileName    [sfm.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based optimization using internal don't-cares.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: sfm.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__opt_sfm__h
#define ABC__opt_sfm__h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "misc/vec/vecWec.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Sfm_Ntk_t_ Sfm_Ntk_t;
typedef struct Sfm_Par_t_ Sfm_Par_t;
struct Sfm_Par_t_
{
    int             nTfoLevMax;    // the maximum fanout levels
    int             nTfiLevMax;    // the maximum fanin levels
    int             nFanoutMax;    // the maximum number of fanouts
    int             nDepthMax;     // the maximum depth to try
    int             nVarMax;       // the maximum variable count
    int             nMffcMin;      // the minimum MFFC size
    int             nMffcMax;      // the maximum MFFC size
    int             nDecMax;       // the maximum number of decompositions
    int             nWinSizeMax;   // the maximum window size
    int             nGrowthLevel;  // the maximum allowed growth in level
    int             nBTLimit;      // the maximum number of conflicts in one SAT run
    int             nNodesMax;     // the maximum number of nodes to try
    int             iNodeOne;      // one particular node to try
    int             nFirstFixed;   // the number of first nodes to be treated as fixed
    int             nTimeWin;      // the size of timing window in percents
    int             DeltaCrit;     // delay delta in picoseconds
    int             DelAreaRatio;  // delay/area tradeoff (how many ps we trade for a unit of area)
    int             fRrOnly;       // perform redundance removal
    int             fArea;         // performs optimization for area
    int             fAreaRev;      // performs optimization for area in reverse order
    int             fMoreEffort;   // performs high-affort minimization
    int             fUseAndOr;     // enable internal detection of AND/OR gates
    int             fZeroCost;     // enable zero-cost replacement
    int             fUseSim;       // enable simulation
    int             fUseDcs;       // enable deriving don't-cares
    int             fPrintDecs;    // enable printing decompositions
    int             fAllBoxes;     // enable preserving all boxes
    int             fLibVerbose;   // enable library stats
    int             fDelayVerbose; // enable delay stats
    int             fVerbose;      // enable basic stats
    int             fVeryVerbose;  // enable detailed stats
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== sfmCnf.c ==========================================================*/
/*=== sfmCore.c ==========================================================*/
extern void         Sfm_ParSetDefault( Sfm_Par_t * pPars );
extern int          Sfm_NtkPerform( Sfm_Ntk_t * p, Sfm_Par_t * pPars );
/*=== sfmNtk.c ==========================================================*/
extern Sfm_Ntk_t *  Sfm_NtkConstruct( Vec_Wec_t * vFanins, int nPis, int nPos, Vec_Str_t * vFixed, Vec_Str_t * vEmpty, Vec_Wrd_t * vTruths, Vec_Int_t * vStarts, Vec_Wrd_t * vTruths2 );
extern void         Sfm_NtkFree( Sfm_Ntk_t * p );
extern Vec_Int_t *  Sfm_NodeReadFanins( Sfm_Ntk_t * p, int i );
extern word *       Sfm_NodeReadTruth( Sfm_Ntk_t * p, int i );
extern int          Sfm_NodeReadFixed( Sfm_Ntk_t * p, int i );
extern int          Sfm_NodeReadUsed( Sfm_Ntk_t * p, int i );
/*=== sfmWin.c ==========================================================*/
extern Vec_Int_t *  Sfm_NtkDfs( Sfm_Ntk_t * p, Vec_Wec_t * vGroups, Vec_Int_t * vGroupMap, Vec_Int_t * vBoxesLeft, int fAllBoxes );


ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

