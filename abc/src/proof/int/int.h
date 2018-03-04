/**CFile****************************************************************

  FileName    [int.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Interpolation engine.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 24, 2008.]

  Revision    [$Id: int.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__aig__int__int_h
#define ABC__aig__int__int_h


/* 
    The interpolation algorithm implemented here was introduced in the paper:
    K. L. McMillan. Interpolation and SAT-based model checking. CAV’03, pp. 1-13.
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

// simulation manager
typedef struct Inter_ManParams_t_ Inter_ManParams_t;
struct Inter_ManParams_t_
{
    int  nBTLimit;      // limit on the number of conflicts
    int  nFramesMax;    // the max number timeframes to unroll
    int  nSecLimit;     // time limit in seconds
    int  nFramesK;      // the number of timeframes to use in induction
    int  fRewrite;      // use additional rewriting to simplify timeframes
    int  fTransLoop;    // add transition into the init state under new PI var
    int  fUsePudlak;    // use Pudluk interpolation procedure
    int  fUseOther;     // use other undisclosed option
    int  fUseMiniSat;   // use MiniSat-1.14p instead of internal proof engine
    int  fCheckKstep;   // check using K-step induction
    int  fUseBias;      // bias decisions to global variables
    int  fUseBackward;  // perform backward interpolation
    int  fUseSeparate;  // solve each output separately
    int  fUseTwoFrames; // create the OR of two last timeframes
    int  fDropSatOuts;  // replace by 1 the solved outputs
    int  fDropInvar;    // dump inductive invariant into file
    int  fVerbose;      // print verbose statistics
    int  iFrameMax;     // the time frame reached
    char * pFileName;   // file name to dump interpolant
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== intCore.c ==========================================================*/
extern void       Inter_ManSetDefaultParams( Inter_ManParams_t * p );
extern int        Inter_ManPerformInterpolation( Aig_Man_t * pAig, Inter_ManParams_t * pPars, int * piFrame );




ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

