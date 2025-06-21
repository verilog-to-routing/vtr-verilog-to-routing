/**CFile****************************************************************

  FileName    [cgt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Clock gating package.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cgt.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__aig__cgt__cgt_h
#define ABC__aig__cgt__cgt_h


/* 
    The algorithm implemented in this package is based on the paper:
    A. Hurst. "Automatic synthesis of clock gating logic with controlled 
    netlist perturbation", DAC 2008.
*/

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

typedef struct Cgt_Par_t_ Cgt_Par_t;
struct Cgt_Par_t_
{
    int          nLevelMax;      // the max number of levels to look for clock-gates
    int          nCandMax;       // the max number of candidates at each node
    int          nOdcMax;        // the max number of ODC levels to consider
    int          nConfMax;       // the max number of conflicts at a node
    int          nVarsMin;       // the min number of variables to recycle the SAT solver
    int          nFlopsMin;      // the min number of flops needed to recycle the SAT solver
    int          fAreaOnly;      // derive clock gating to minimize area
    int          fVerbose;       // verbosity flag
    int          fVeryVerbose;   // verbosity flag
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== cgtCore.c ==========================================================*/
extern void            Cgt_SetDefaultParams( Cgt_Par_t * p );
extern Vec_Vec_t *     Cgt_ClockGatingCandidates( Aig_Man_t * pAig, Aig_Man_t * pCare, Cgt_Par_t * pPars, Vec_Int_t * vUseful );
extern Vec_Vec_t *     Cgt_ClockGatingInt( Aig_Man_t * pAig, Aig_Man_t * pCare, Cgt_Par_t * pPars, Vec_Int_t * vUseful );
extern Aig_Man_t *     Cgt_ClockGating( Aig_Man_t * pAig, Aig_Man_t * pCare, Cgt_Par_t * pPars );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

