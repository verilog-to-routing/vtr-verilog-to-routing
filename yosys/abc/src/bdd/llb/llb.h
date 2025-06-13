/**CFile****************************************************************

  FileName    [llb.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [BDD-based reachability.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 8, 2010.]

  Revision    [$Id: llb.h,v 1.00 2010/05/08 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__aig__llb__llb_h
#define ABC__aig__llb__llb_h

 
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
  
typedef struct Gia_ParLlb_t_ Gia_ParLlb_t;
struct Gia_ParLlb_t_
{
    int         nBddMax;       // maximum BDD size
    int         nIterMax;      // maximum iteration count
    int         nClusterMax;   // maximum cluster size
    int         nHintDepth;    // the number of times to cofactor
    int         HintFirst;     // the number of first hint to use
    int         fUseFlow;      // use flow computation
    int         nVolumeMax;    // the largest volume
    int         nVolumeMin;    // the smallest volume
    int         nPartValue;    // partitioning value
    int         fBackward;     // enable backward reachability
    int         fReorder;      // enable dynamic variable reordering
    int         fIndConstr;    // extract inductive constraints
    int         fUsePivots;    // use internal pivot variables
    int         fCluster;      // use partition clustering
    int         fSchedule;     // use cluster scheduling
    int         fDumpReached;  // dump reached states into a file
    int         fVerbose;      // print verbose information
    int         fVeryVerbose;  // print dependency matrices
    int         fSilent;       // do not print any infomation
    int         fSkipReach;    // skip reachability (preparation phase only)
    int         fSkipOutCheck; // does not check the property output
    int         TimeLimit;     // time limit for one reachability run
    int         TimeLimitGlo;  // time limit for all reachability runs
    // internal parameters
    abctime     TimeTarget;    // the time to stop
    int         iFrame;        // explored up to this frame
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== llbCore.c ==========================================================*/
extern void     Llb_ManSetDefaultParams( Gia_ParLlb_t * pPars );
/*=== llb4Nonlin.c ==========================================================*/
extern int      Llb_Nonlin4CoreReach( Aig_Man_t * pAig, Gia_ParLlb_t * pPars );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

