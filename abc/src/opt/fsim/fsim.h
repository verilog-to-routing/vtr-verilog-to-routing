/**CFile****************************************************************

  FileName    [fsim.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Fast sequential AIG simulator.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: fsim.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__aig__fsim__fsim_h
#define ABC__aig__fsim__fsim_h


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

typedef struct Fsim_Man_t_ Fsim_Man_t;

// simulation parameters
typedef struct Fsim_ParSim_t_ Fsim_ParSim_t;
struct Fsim_ParSim_t_
{
    // user-controlled parameters
    int             nWords;       // the number of machine words
    int             nIters;       // the number of timeframes
    int             TimeLimit;    // time limit in seconds
    int             fCheckMiter;  // check if miter outputs are non-zero
    int             fVerbose;     // enables verbose output
    // internal parameters
    int             fCompressAig; // compresses internal data
};

// switching estimation parameters
typedef struct Fsim_ParSwitch_t_ Fsim_ParSwitch_t;
struct Fsim_ParSwitch_t_
{
    // user-controlled parameters
    int             nWords;       // the number of machine words
    int             nIters;       // the number of timeframes
    int             nPref;        // the number of first timeframes to skip
    int             nRandPiNum;   // PI trans prob (0=1/2; 1=1/4; 2=1/8, etc)
    int             fProbOne;     // collect probability of one
    int             fProbTrans;   // collect probatility of switching
    int             fVerbose;     // enables verbose output
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== fsimCore.c ==========================================================*/
extern void           Fsim_ManSetDefaultParamsSim( Fsim_ParSim_t * p );
extern void           Fsim_ManSetDefaultParamsSwitch( Fsim_ParSwitch_t * p );
/*=== fsimSim.c ==========================================================*/
extern int            Fsim_ManSimulate( Aig_Man_t * pAig, Fsim_ParSim_t * pPars );
/*=== fsimSwitch.c ==========================================================*/
extern Vec_Int_t *    Fsim_ManSwitchSimulate( Aig_Man_t * pAig, Fsim_ParSwitch_t * pPars );
/*=== fsimTsim.c ==========================================================*/
extern Vec_Ptr_t *    Fsim_ManTerSimulate( Aig_Man_t * pAig, int fVerbose );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

