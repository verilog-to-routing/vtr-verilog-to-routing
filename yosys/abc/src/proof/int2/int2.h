/**CFile****************************************************************

  FileName    [int2.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Interpolation engine.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - Dec 1, 2013.]

  Revision    [$Id: int2.h,v 1.00 2013/12/01 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__aig__int2__int_h
#define ABC__aig__int2__int_h


/* 
    The interpolation algorithm implemented here was introduced in the papers:
    K. L. McMillan. Interpolation and SAT-based model checking. CAV’03, pp. 1-13.
    C.-Y. Wu et al. A CEX-Guided Interpolant Generation Algorithm for 
    SAT-based Model Checking. DAC'13.
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
typedef struct Int2_ManPars_t_ Int2_ManPars_t;
struct Int2_ManPars_t_
{
    int     nBTLimit;       // limit on the number of conflicts
    int     nFramesS;       // the starting number timeframes
    int     nFramesMax;     // the max number timeframes to unroll
    int     nSecLimit;      // time limit in seconds
    int     nFramesK;       // the number of timeframes to use in induction
    int     fRewrite;       // use additional rewriting to simplify timeframes
    int     fTransLoop;     // add transition into the init state under new PI var
    int     fDropInvar;     // dump inductive invariant into file
    int     fVerbose;       // print verbose statistics
    int     iFrameMax;      // the time frame reached
    char *  pFileName;      // file name to dump interpolant
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== intCore.c ==========================================================*/
extern void       Int2_ManSetDefaultParams( Int2_ManPars_t * p );
extern int        Int2_ManPerformInterpolation( Gia_Man_t * p, Int2_ManPars_t * pPars );




ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

