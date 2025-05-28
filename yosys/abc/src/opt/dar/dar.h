/**CFile****************************************************************

  FileName    [dar.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware AIG rewriting.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: dar.h,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__aig__dar__dar_h
#define ABC__aig__dar__dar_h


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

typedef struct Dar_RwrPar_t_            Dar_RwrPar_t;
typedef struct Dar_RefPar_t_            Dar_RefPar_t;

struct Dar_RwrPar_t_  
{
    int              nCutsMax;       // the maximum number of cuts to try
    int              nSubgMax;       // the maximum number of subgraphs to try
    int              nMinSaved;      // the minumum number of nodes saved
    int              fFanout;        // support fanout representation
    int              fUpdateLevel;   // update level 
    int              fUseZeros;      // performs zero-cost replacement
    int              fPower;         // enables power-aware rewriting
    int              fRecycle;       // enables cut recycling
    int              fVerbose;       // enables verbose output
    int              fVeryVerbose;   // enables very verbose output
};

struct Dar_RefPar_t_  
{
    int              nMffcMin;       // the min MFFC size for which refactoring is used
    int              nLeafMax;       // the max number of leaves of a cut
    int              nCutsMax;       // the max number of cuts to consider  
    int              fExtend;        // extends the cut below MFFC
    int              fUpdateLevel;   // updates the level after each move
    int              fUseZeros;      // perform zero-cost replacements
    int              fVerbose;       // verbosity level
    int              fVeryVerbose;   // enables very verbose output
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                             ITERATORS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== darLib.c ========================================================*/
extern void            Dar_LibStart();
extern void            Dar_LibStop();
extern void            Dar_LibPrepare( int nSubgraphs );
extern int             Dar_LibReturnClass( unsigned uTruth );
/*=== darBalance.c ========================================================*/
extern Aig_Man_t *     Dar_ManBalance( Aig_Man_t * p, int fUpdateLevel );
extern Aig_Man_t *     Dar_ManBalanceXor( Aig_Man_t * pAig, int fExor, int fUpdateLevel, int fVerbose );
extern void            Dar_BalancePrintStats( Aig_Man_t * p );
/*=== darCore.c ========================================================*/
extern void            Dar_ManDefaultRwrParams( Dar_RwrPar_t * pPars );
extern int             Dar_ManRewrite( Aig_Man_t * pAig, Dar_RwrPar_t * pPars );
extern Aig_MmFixed_t * Dar_ManComputeCuts( Aig_Man_t * pAig, int nCutsMax, int fSkipTtMin, int fVerbose );
/*=== darRefact.c ========================================================*/
extern void            Dar_ManDefaultRefParams( Dar_RefPar_t * pPars );
extern int             Dar_ManRefactor( Aig_Man_t * pAig, Dar_RefPar_t * pPars );
/*=== darScript.c ========================================================*/
extern Aig_Man_t *     Dar_ManRewriteDefault( Aig_Man_t * pAig );
extern Aig_Man_t *     Dar_ManRwsat( Aig_Man_t * pAig, int fBalance, int fVerbose );
extern Aig_Man_t *     Dar_ManCompress( Aig_Man_t * pAig, int fBalance, int fUpdateLevel, int fPower, int fVerbose );
extern Aig_Man_t *     Dar_ManCompress2( Aig_Man_t * pAig, int fBalance, int fUpdateLevel, int fFanout, int fPower, int fVerbose );
extern Aig_Man_t *     Dar_ManChoice( Aig_Man_t * pAig, int fBalance, int fUpdateLevel, int fConstruct, int nConfMax, int nLevelMax, int fVerbose );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

