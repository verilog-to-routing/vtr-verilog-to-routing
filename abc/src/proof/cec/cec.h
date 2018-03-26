/**CFile****************************************************************

  FileName    [cec.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Combinational equivalence checking.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cec.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__aig__cec__cec_h
#define ABC__aig__cec__cec_h


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

// dynamic SAT parameters
typedef struct Cec_ParSat_t_ Cec_ParSat_t;
struct Cec_ParSat_t_
{
    int              nBTLimit;      // conflict limit at a node
    int              nSatVarMax;    // the max number of SAT variables
    int              nCallsRecycle; // calls to perform before recycling SAT solver
    int              fNonChrono;    // use non-chronological backtracling (for circuit SAT only)
    int              fPolarFlip;    // flops polarity of variables
    int              fCheckMiter;   // the circuit is the miter
//    int              fFirstStop;    // stop on the first sat output
    int              fLearnCls;     // perform clause learning
    int              fVerbose;      // verbose stats
};

// simulation parameters
typedef struct Cec_ParSim_t_ Cec_ParSim_t;
struct Cec_ParSim_t_ 
{
    int              nWords;        // the number of simulation words
    int              nFrames;       // the number of simulation frames
    int              nRounds;       // the number of simulation rounds
    int              nNonRefines;   // the max number of rounds without refinement
    int              TimeLimit;     // the runtime limit in seconds
    int              fDualOut;      // miter with separate outputs
    int              fCheckMiter;   // the circuit is the miter
//    int              fFirstStop;    // stop on the first sat output
    int              fSeqSimulate;  // performs sequential simulation
    int              fLatchCorr;    // consider only latch outputs
    int              fConstCorr;    // consider only constants
    int              fVeryVerbose;  // verbose stats
    int              fVerbose;      // verbose stats
};

// semiformal parameters
typedef struct Cec_ParSmf_t_ Cec_ParSmf_t;
struct Cec_ParSmf_t_
{
    int              nWords;        // the number of simulation words
    int              nRounds;       // the number of simulation rounds
    int              nFrames;       // the max number of time frames
    int              nNonRefines;   // the max number of rounds without refinement
    int              nMinOutputs;   // the min outputs to accumulate
    int              nBTLimit;      // conflict limit at a node
    int              TimeLimit;     // the runtime limit in seconds
    int              fDualOut;      // miter with separate outputs
    int              fCheckMiter;   // the circuit is the miter
//    int              fFirstStop;    // stop on the first sat output
    int              fVerbose;      // verbose stats
};

// combinational SAT sweeping parameters
typedef struct Cec_ParFra_t_ Cec_ParFra_t;
struct Cec_ParFra_t_
{
    int              nWords;        // the number of simulation words
    int              nRounds;       // the number of simulation rounds
    int              nItersMax;     // the maximum number of iterations of SAT sweeping
    int              nBTLimit;      // conflict limit at a node
    int              TimeLimit;     // the runtime limit in seconds
    int              nLevelMax;     // restriction on the level nodes to be swept
    int              nDepthMax;     // the depth in terms of steps of speculative reduction
    int              fRewriting;    // enables AIG rewriting
    int              fCheckMiter;   // the circuit is the miter
//    int              fFirstStop;    // stop on the first sat output
    int              fDualOut;      // miter with separate outputs
    int              fColorDiff;    // miter with separate outputs
    int              fSatSweeping;  // enable SAT sweeping
    int              fRunCSat;      // enable another solver
    int              fUseCones;     // use cones
    int              fUseOrigIds;   // enable recording of original IDs
    int              fVeryVerbose;  // verbose stats
    int              fVerbose;      // verbose stats
    int              iOutFail;      // the failed output
};

// combinational equivalence checking parameters
typedef struct Cec_ParCec_t_ Cec_ParCec_t;
struct Cec_ParCec_t_
{
    int              nBTLimit;      // conflict limit at a node
    int              TimeLimit;     // the runtime limit in seconds
//    int              fFirstStop;    // stop on the first sat output
    int              fUseSmartCnf;  // use smart CNF computation
    int              fRewriting;    // enables AIG rewriting
    int              fNaive;        // performs naive SAT-based checking
    int              fSilent;       // print no messages
    int              fVeryVerbose;  // verbose stats
    int              fVerbose;      // verbose stats
    int              iOutFail;      // the number of failed output
};

// sequential register correspodence parameters
typedef struct Cec_ParCor_t_ Cec_ParCor_t;
struct Cec_ParCor_t_
{
    int              nWords;        // the number of simulation words
    int              nRounds;       // the number of simulation rounds
    int              nFrames;       // the number of time frames
    int              nPrefix;       // the number of time frames in the prefix
    int              nBTLimit;      // conflict limit at a node
    int              nLevelMax;     // (scorr only) the max number of levels
    int              nStepsMax;     // (scorr only) the max number of induction steps
    int              fLatchCorr;    // consider only latch outputs
    int              fConstCorr;    // consider only constants
    int              fUseRings;     // use rings
    int              fMakeChoices;  // use equilvaences as choices
    int              fUseCSat;      // use circuit-based solver
//    int              fFirstStop;    // stop on the first sat output
    int              fUseSmartCnf;  // use smart CNF computation
    int              fStopWhenGone; // quit when PO is not a candidate constant
    int              fVerboseFlops; // verbose stats
    int              fVeryVerbose;  // verbose stats
    int              fVerbose;      // verbose stats
    // callback
    void *           pData;
    void *           pFunc;
};

// sequential register correspodence parameters
typedef struct Cec_ParChc_t_ Cec_ParChc_t;
struct Cec_ParChc_t_
{
    int              nWords;        // the number of simulation words
    int              nRounds;       // the number of simulation rounds
    int              nBTLimit;      // conflict limit at a node
    int              fUseRings;     // use rings
    int              fUseCSat;      // use circuit-based solver
    int              fVeryVerbose;  // verbose stats
    int              fVerbose;      // verbose stats
};

// sequential synthesis parameters
typedef struct Cec_ParSeq_t_ Cec_ParSeq_t;
struct Cec_ParSeq_t_
{
    int              fUseLcorr;     // enables latch correspondence
    int              fUseScorr;     // enables signal correspondence
    int              nBTLimit;      // (scorr/lcorr) conflict limit at a node
    int              nFrames;       // (scorr/lcorr) the number of timeframes
    int              nLevelMax;     // (scorr only) the max number of levels
    int              fConsts;       // (scl only) merging constants
    int              fEquivs;       // (scl only) merging equivalences
    int              fUseMiniSat;   // enables MiniSat in lcorr/scorr
    int              nMinDomSize;   // the size of minimum clock domain
    int              fVeryVerbose;  // verbose stats
    int              fVerbose;      // verbose stats
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== cecCec.c ==========================================================*/
extern int           Cec_ManVerify( Gia_Man_t * p, Cec_ParCec_t * pPars );
extern int           Cec_ManVerifyTwo( Gia_Man_t * p0, Gia_Man_t * p1, int fVerbose );
extern int           Cec_ManVerifySimple( Gia_Man_t * p );
/*=== cecChoice.c ==========================================================*/
extern Gia_Man_t *   Cec_ManChoiceComputation( Gia_Man_t * pAig, Cec_ParChc_t * pPars );
/*=== cecCorr.c ==========================================================*/
extern int           Cec_ManLSCorrespondenceClasses( Gia_Man_t * pAig, Cec_ParCor_t * pPars );
extern Gia_Man_t *   Cec_ManLSCorrespondence( Gia_Man_t * pAig, Cec_ParCor_t * pPars );
/*=== cecCore.c ==========================================================*/
extern void          Cec_ManSatSetDefaultParams( Cec_ParSat_t * p );
extern void          Cec_ManSimSetDefaultParams( Cec_ParSim_t * p );
extern void          Cec_ManSmfSetDefaultParams( Cec_ParSmf_t * p );
extern void          Cec_ManFraSetDefaultParams( Cec_ParFra_t * p );
extern void          Cec_ManCecSetDefaultParams( Cec_ParCec_t * p );
extern void          Cec_ManCorSetDefaultParams( Cec_ParCor_t * p );
extern void          Cec_ManChcSetDefaultParams( Cec_ParChc_t * p );
extern Gia_Man_t *   Cec_ManSatSweeping( Gia_Man_t * pAig, Cec_ParFra_t * pPars, int fSilent );
extern Gia_Man_t *   Cec_ManSatSolving( Gia_Man_t * pAig, Cec_ParSat_t * pPars );
extern void          Cec_ManSimulation( Gia_Man_t * pAig, Cec_ParSim_t * pPars );
/*=== cecSeq.c ==========================================================*/
extern int           Cec_ManSeqResimulateCounter( Gia_Man_t * pAig, Cec_ParSim_t * pPars, Abc_Cex_t * pCex );
extern int           Cec_ManSeqSemiformal( Gia_Man_t * pAig, Cec_ParSmf_t * pPars );
extern int           Cec_ManCheckNonTrivialCands( Gia_Man_t * pAig );
/*=== cecSynth.c ==========================================================*/
extern int           Cec_SeqReadMinDomSize( Cec_ParSeq_t * p );
extern int           Cec_SeqReadVerbose( Cec_ParSeq_t * p );
extern void          Cec_SeqSynthesisSetDefaultParams( Cec_ParSeq_t * pPars );
extern int           Cec_SequentialSynthesisPart( Gia_Man_t * p, Cec_ParSeq_t * pPars );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

