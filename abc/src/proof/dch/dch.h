/**CFile****************************************************************

  FileName    [dch.h] 

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Choice computation for tech-mapping.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 29, 2008.]

  Revision    [$Id: dch.h,v 1.00 2008/07/29 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__aig__dch__dch_h
#define ABC__aig__dch__dch_h


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
typedef struct Dch_Pars_t_ Dch_Pars_t;
struct Dch_Pars_t_
{
    int              nWords;        // the number of simulation words
    int              nBTLimit;      // conflict limit at a node
    int              nSatVarMax;    // the max number of SAT variables
    int              fSynthesis;    // set to 1 to perform synthesis
    int              fPolarFlip;    // uses polarity adjustment
    int              fSimulateTfo;  // uses simulation of TFO classes
    int              fPower;        // uses power-aware rewriting
    int              fUseGia;       // uses GIA package 
    int              fUseCSat;      // uses circuit-based solver
    int              fLightSynth;   // uses lighter version of synthesis
    int              fSkipRedSupp;  // skip choices with redundant support vars
    int              fVerbose;      // verbose stats
    abctime          timeSynth;     // synthesis runtime
    int              nNodesAhead;   // the lookahead in terms of nodes
    int              nCallsRecycle; // calls to perform before recycling SAT solver
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== dchAig.c ==========================================================*/
extern Aig_Man_t *   Dch_DeriveTotalAig( Vec_Ptr_t * vAigs );
/*=== dchCore.c ==========================================================*/
extern void          Dch_ManSetDefaultParams( Dch_Pars_t * p );
extern int           Dch_ManReadVerbose( Dch_Pars_t * p );
extern Aig_Man_t *   Dch_ComputeChoices( Aig_Man_t * pAig, Dch_Pars_t * pPars );
extern void          Dch_ComputeEquivalences( Aig_Man_t * pAig, Dch_Pars_t * pPars );
/*=== dchScript.c ==========================================================*/
extern Aig_Man_t *   Dar_ManChoiceNew( Aig_Man_t * pAig, Dch_Pars_t * pPars );


ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

