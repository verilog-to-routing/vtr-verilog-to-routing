/**CFile****************************************************************

  FileName    [ssc.h] 

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT sweeping under constraints.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 29, 2008.]

  Revision    [$Id: ssc.h,v 1.00 2008/07/29 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__aig__ssc__ssc_h
#define ABC__aig__ssc__ssc_h


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

// choicing parameters
typedef struct Ssc_Pars_t_ Ssc_Pars_t;
struct Ssc_Pars_t_
{
    int              nWords;        // the number of simulation words
    int              nBTLimit;      // conflict limit at a node
    int              nSatVarMax;    // the max number of SAT variables
    int              nCallsRecycle; // calls to perform before recycling SAT solver
    int              fAppend;       // append constraints to the resulting AIG
    int              fVerbose;      // verbose stats
    int              fVerify;       // enable internal verification
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== sscCore.c ==========================================================*/
extern void          Ssc_ManSetDefaultParams( Ssc_Pars_t * p );
extern Gia_Man_t *   Ssc_PerformSweeping( Gia_Man_t * pAig, Gia_Man_t * pCare, Ssc_Pars_t * pPars );
extern Gia_Man_t *   Ssc_PerformSweepingConstr( Gia_Man_t * p, Ssc_Pars_t * pPars );


ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

