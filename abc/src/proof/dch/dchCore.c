/**CFile****************************************************************

  FileName    [dchCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Choice computation for tech-mapping.]

  Synopsis    [The core procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 29, 2008.]

  Revision    [$Id: dchCore.c,v 1.00 2008/07/29 00:00:00 alanmi Exp $]

***********************************************************************/

#include "dchInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [This procedure sets default parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dch_ManSetDefaultParams( Dch_Pars_t * p )
{
    memset( p, 0, sizeof(Dch_Pars_t) );
    p->nWords         =     8;  // the number of simulation words
    p->nBTLimit       =  1000;  // conflict limit at a node
    p->nSatVarMax     =  5000;  // the max number of SAT variables
    p->fSynthesis     =     1;  // derives three snapshots
    p->fPolarFlip     =     1;  // uses polarity adjustment
    p->fSimulateTfo   =     1;  // simulate TFO
    p->fPower         =     0;  // power-aware rewriting
    p->fLightSynth    =     0;  // uses lighter version of synthesis
    p->fSkipRedSupp   =     0;  // skips choices with redundant structural support
    p->fVerbose       =     0;  // verbose stats
    p->nNodesAhead    =  1000;  // the lookahead in terms of nodes
    p->nCallsRecycle  =   100;  // calls to perform before recycling SAT solver
}

/**Function*************************************************************

  Synopsis    [Returns verbose parameter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dch_ManReadVerbose( Dch_Pars_t * p )
{
    return p->fVerbose;
}

/**Function*************************************************************

  Synopsis    [Performs computation of AIGs with choices.]

  Description [Takes several AIGs and performs choicing.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Dch_ComputeChoices( Aig_Man_t * pAig, Dch_Pars_t * pPars )
{
    Dch_Man_t * p;
    Aig_Man_t * pResult;
    abctime clk, clk2 = Abc_Clock(), clkTotal = Abc_Clock();
    // reset random numbers
    Aig_ManRandom(1);
    // start the choicing manager
    p = Dch_ManCreate( pAig, pPars );
    // compute candidate equivalence classes
clk = Abc_Clock(); 
    p->ppClasses = Dch_CreateCandEquivClasses( pAig, pPars->nWords, pPars->fVerbose );
p->timeSimInit = Abc_Clock() - clk;
//    Dch_ClassesPrint( p->ppClasses, 0 );
    p->nLits = Dch_ClassesLitNum( p->ppClasses );
    // perform SAT sweeping
    Dch_ManSweep( p );
    // free memory ahead of time
p->timeTotal = Abc_Clock() - clkTotal;
    Dch_ManStop( p );
    if ( pPars->fVerbose ) 
        Abc_PrintTime( 1, "Old choice computation time", Abc_Clock() - clk2 );
    // create choices
    ABC_FREE( pAig->pTable );
    pResult = Dch_DeriveChoiceAig( pAig, pPars->fSkipRedSupp );
    // count the number of representatives
    if ( pPars->fVerbose ) 
        Abc_Print( 1, "STATS:  Ands:%8d  ->%8d.  Reprs:%7d  ->%7d.  Choices =%7d.\n", 
               Aig_ManNodeNum(pAig), 
               Aig_ManNodeNum(pResult), 
               Dch_DeriveChoiceCountReprs( pAig ),
               Dch_DeriveChoiceCountEquivs( pResult ),
               Aig_ManChoiceNum( pResult ) );
    return pResult;
}

/**Function*************************************************************

  Synopsis    [Performs computation of AIGs with choices.]

  Description [Takes several AIGs and performs choicing.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dch_ComputeEquivalences( Aig_Man_t * pAig, Dch_Pars_t * pPars )
{
    Dch_Man_t * p;
    abctime clk, clkTotal = Abc_Clock();
    // reset random numbers
    Aig_ManRandom(1);
    // start the choicing manager
    p = Dch_ManCreate( pAig, pPars );
    // compute candidate equivalence classes
clk = Abc_Clock(); 
    p->ppClasses = Dch_CreateCandEquivClasses( pAig, pPars->nWords, pPars->fVerbose );
p->timeSimInit = Abc_Clock() - clk;
//    Dch_ClassesPrint( p->ppClasses, 0 );
    p->nLits = Dch_ClassesLitNum( p->ppClasses );
    // perform SAT sweeping
    Dch_ManSweep( p );
    // free memory ahead of time
p->timeTotal = Abc_Clock() - clkTotal;
    Dch_ManStop( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

