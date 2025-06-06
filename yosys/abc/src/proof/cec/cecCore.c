/**CFile****************************************************************

  FileName    [cecCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Combinational equivalence checking.]

  Synopsis    [Core procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cecCore.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cecInt.h"

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
void Cec_ManSatSetDefaultParams( Cec_ParSat_t * p )
{
    memset( p, 0, sizeof(Cec_ParSat_t) );
    p->SolverType     =      -1;  // SAT solver type
    p->nBTLimit       =     100;  // conflict limit at a node
    p->nSatVarMax     =    2000;  // the max number of SAT variables
    p->nCallsRecycle  =     200;  // calls to perform before recycling SAT solver
    p->fNonChrono     =       1;  // use non-chronological backtracling (for circuit SAT only)
    p->fPolarFlip     =       1;  // flops polarity of variables
    p->fCheckMiter    =       0;  // the circuit is the miter
//    p->fFirstStop     =       0;  // stop on the first sat output
    p->fLearnCls      =       0;  // perform clause learning
    p->fVerbose       =       0;  // verbose stats
}  

/**Function************  *************************************************

  Synopsis    [This procedure sets default parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_ManSimSetDefaultParams( Cec_ParSim_t * p )
{
    memset( p, 0, sizeof(Cec_ParSim_t) );
    p->nWords         =      31;  // the number of simulation words
    p->nFrames        =     100;  // the number of simulation frames
    p->nRounds        =      20;  // the max number of simulation rounds
    p->nNonRefines    =       3;  // the max number of rounds without refinement
    p->TimeLimit      =       0;  // the runtime limit in seconds
    p->fCheckMiter    =       0;  // the circuit is the miter
//    p->fFirstStop     =       0;  // stop on the first sat output
    p->fDualOut       =       0;  // miter with separate outputs
    p->fConstCorr     =       0;  // consider only constants
    p->fSeqSimulate   =       0;  // performs sequential simulation
    p->fVeryVerbose   =       0;  // verbose stats
    p->fVerbose       =       0;  // verbose stats
} 

/**Function************  *************************************************

  Synopsis    [This procedure sets default parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_ManSmfSetDefaultParams( Cec_ParSmf_t * p )
{
    memset( p, 0, sizeof(Cec_ParSmf_t) );
    p->nWords         =      31;  // the number of simulation words
    p->nRounds        =     200;  // the number of simulation rounds
    p->nFrames        =     200;  // the max number of time frames
    p->nNonRefines    =       3;  // the max number of rounds without refinement
    p->nMinOutputs    =       0;  // the min outputs to accumulate
    p->nBTLimit       =     100;  // conflict limit at a node
    p->TimeLimit      =       0;  // the runtime limit in seconds
    p->fDualOut       =       0;  // miter with separate outputs
    p->fCheckMiter    =       0;  // the circuit is the miter
//    p->fFirstStop     =       0;  // stop on the first sat output
    p->fVerbose       =       0;  // verbose stats
} 

/**Function************  *************************************************

  Synopsis    [This procedure sets default parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_ManFraSetDefaultParams( Cec_ParFra_t * p )
{
    memset( p, 0, sizeof(Cec_ParFra_t) );
    p->nWords         =      15;  // the number of simulation words
    p->nRounds        =      15;  // the number of simulation rounds
    p->TimeLimit      =       0;  // the runtime limit in seconds
    p->nItersMax      =      10;  // the maximum number of iterations of SAT sweeping
    p->nBTLimit       =     100;  // conflict limit at a node
    p->nLevelMax      =       0;  // restriction on the level of nodes to be swept
    p->nDepthMax      =       1;  // the depth in terms of steps of speculative reduction
    p->fRewriting     =       0;  // enables AIG rewriting
    p->fCheckMiter    =       0;  // the circuit is the miter
//    p->fFirstStop     =       0;  // stop on the first sat output
    p->fDualOut       =       0;  // miter with separate outputs
    p->fColorDiff     =       0;  // miter with separate outputs
    p->fSatSweeping   =       0;  // enable SAT sweeping
    p->fUseCones      =       0;  // use cones
    p->fVeryVerbose   =       0;  // verbose stats
    p->fVerbose       =       0;  // verbose stats
    p->iOutFail       =      -1;  // the failed output
} 

/**Function*************************************************************

  Synopsis    [This procedure sets default parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_ManCecSetDefaultParams( Cec_ParCec_t * p )
{
    memset( p, 0, sizeof(Cec_ParCec_t) );
    p->nBTLimit       =    1000;  // conflict limit at a node
    p->TimeLimit      =       0;  // the runtime limit in seconds
//    p->fFirstStop     =       0;  // stop on the first sat output
    p->fUseSmartCnf   =       0;  // use smart CNF computation
    p->fRewriting     =       0;  // enables AIG rewriting
    p->fVeryVerbose   =       0;  // verbose stats
    p->fVerbose       =       0;  // verbose stats
    p->iOutFail       =      -1;  // the number of failed output
}  

/**Function*************************************************************

  Synopsis    [This procedure sets default parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_ManCorSetDefaultParams( Cec_ParCor_t * p )
{
    memset( p, 0, sizeof(Cec_ParCor_t) );
    p->nWords         =      15;  // the number of simulation words
    p->nRounds        =      15;  // the number of simulation rounds
    p->nFrames        =       1;  // the number of time frames
    p->nBTLimit       =     100;  // conflict limit at a node
    p->nLevelMax      =      -1;  // (scorr only) the max number of levels
    p->nStepsMax      =      -1;  // (scorr only) the max number of induction steps
    p->fLatchCorr     =       0;  // consider only latch outputs
    p->fConstCorr     =       0;  // consider only constants
    p->fUseRings      =       1;  // combine classes into rings
    p->fUseCSat       =       1;  // use circuit-based solver
//    p->fFirstStop     =       0;  // stop on the first sat output
    p->fUseSmartCnf   =       0;  // use smart CNF computation
    p->fVeryVerbose   =       0;  // verbose stats
    p->fVerbose       =       0;  // verbose stats
}  

/**Function*************************************************************

  Synopsis    [This procedure sets default parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_ManChcSetDefaultParams( Cec_ParChc_t * p )
{
    memset( p, 0, sizeof(Cec_ParChc_t) );
    p->nWords         =      15;  // the number of simulation words
    p->nRounds        =      15;  // the number of simulation rounds
    p->nBTLimit       =    1000;  // conflict limit at a node
    p->fUseRings      =       1;  // use rings
    p->fUseCSat       =       0;  // use circuit-based solver
    p->fVeryVerbose   =       0;  // verbose stats
    p->fVerbose       =       0;  // verbose stats
}  

/**Function*************************************************************

  Synopsis    [Core procedure for SAT sweeping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Cec_ManSatSolving( Gia_Man_t * pAig, Cec_ParSat_t * pPars, int f0Proved )
{
    Gia_Man_t * pNew;
    Cec_ManPat_t * pPat;
    pPat = Cec_ManPatStart();
    if ( pPars->SolverType == -1 )
        Cec_ManSatSolve( pPat, pAig, pPars, NULL, NULL, NULL, f0Proved );
    else
        CecG_ManSatSolve( pPat, pAig, pPars, f0Proved );
//    pNew = Gia_ManDupDfsSkip( pAig );
    pNew = Gia_ManCleanup( pAig );
    Cec_ManPatStop( pPat );
    pNew->vSeqModelVec = pAig->vSeqModelVec;
    pAig->vSeqModelVec = NULL;
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Core procedure for simulation.]

  Description [Returns 1 if refinement has happened.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cec_ManSimulationOne( Gia_Man_t * pAig, Cec_ParSim_t * pPars )
{
    Cec_ManSim_t * pSim;
    int RetValue = 0;
    abctime clkTotal = Abc_Clock();
    pSim = Cec_ManSimStart( pAig, pPars );
    if ( (pAig->pReprs == NULL && (RetValue = Cec_ManSimClassesPrepare( pSim, -1 ))) ||
         (RetValue == 0 &&        (RetValue = Cec_ManSimClassesRefine( pSim ))) )
        Abc_Print( 1, "The number of failed outputs of the miter = %6d. (Words = %4d. Frames = %4d.)\n", 
            pSim->nOuts, pPars->nWords, pPars->nFrames );
    if ( pPars->fVerbose )
        Abc_PrintTime( 1, "Time", Abc_Clock() - clkTotal );
    Cec_ManSimStop( pSim );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Core procedure for simulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_ManSimulation( Gia_Man_t * pAig, Cec_ParSim_t * pPars )
{
    int r, nLitsOld, nLitsNew, nCountNoRef = 0, fStop = 0;
    Gia_ManRandom( 1 );
    if ( pPars->fSeqSimulate )
        Abc_Print( 1, "Performing rounds of random simulation of %d frames with %d words.\n", 
            pPars->nRounds, pPars->nFrames, pPars->nWords );
    nLitsOld = Gia_ManEquivCountLits( pAig );
    for ( r = 0; r < pPars->nRounds; r++ )
    {
        if ( Cec_ManSimulationOne( pAig, pPars ) )
        {
            fStop = 1;
            break;
        }
        // decide when to stop
        nLitsNew = Gia_ManEquivCountLits( pAig );
        if ( nLitsOld == 0 || nLitsOld > nLitsNew )
        {
            nLitsOld = nLitsNew;
            nCountNoRef = 0;
        }
        else if ( ++nCountNoRef == pPars->nNonRefines )
        {
            r++;
            break;
        }
        assert( nLitsOld == nLitsNew );
    }
//    if ( pPars->fVerbose )
    if ( r == pPars->nRounds || fStop )
        Abc_Print( 1, "Random simulation is stopped after %d rounds.\n", r );
    else
        Abc_Print( 1, "Random simulation saturated after %d rounds.\n", r );
    if ( pPars->fCheckMiter )
    {
        int nNonConsts = Cec_ManCountNonConstOutputs( pAig );
        if ( nNonConsts )
            Abc_Print( 1, "The number of POs that are not const-0 candidates = %d.\n", nNonConsts );
    }
}

/**Function*************************************************************

  Synopsis    [Core procedure for SAT sweeping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Cec_ManSatSweeping( Gia_Man_t * pAig, Cec_ParFra_t * pPars, int fSilent )
{
    int fOutputResult = 0;
    Cec_ParSat_t ParsSat, * pParsSat = &ParsSat;
    Cec_ParSim_t ParsSim, * pParsSim = &ParsSim;
    Gia_Man_t * pIni, * pSrm, * pTemp;
    Cec_ManFra_t * p;
    Cec_ManSim_t * pSim;
    Cec_ManPat_t * pPat;
    int i, fTimeOut = 0, nMatches = 0;
    abctime clk, clk2, clkTotal = Abc_Clock();
    if ( pPars->fVerbose )
        printf( "Simulating %d words for %d rounds. SAT solving with %d conflicts.\n", pPars->nWords, pPars->nRounds, pPars->nBTLimit );

    // duplicate AIG and transfer equivalence classes
    Gia_ManRandom( 1 );
    pIni = Gia_ManDup(pAig);
    pIni->pReprs = pAig->pReprs; pAig->pReprs = NULL;
    pIni->pNexts = pAig->pNexts; pAig->pNexts = NULL;
    if ( pPars->fUseOrigIds )
    {
        Gia_ManOrigIdsInit( pIni );
        Vec_IntFreeP( &pAig->vIdsEquiv );
        pAig->vIdsEquiv = Vec_IntAlloc( 1000 );
    }
    if ( pAig->vSimsPi )
    {
        pIni->vSimsPi = Vec_WrdDup(pAig->vSimsPi); 
        pIni->nSimWords = pAig->nSimWords;
    }

    // prepare the managers
    // SAT sweeping
    p = Cec_ManFraStart( pIni, pPars );
    if ( pPars->fDualOut )
        pPars->fColorDiff = 1;
    // simulation
    Cec_ManSimSetDefaultParams( pParsSim );
    pParsSim->nWords      = Abc_MaxInt(2*pAig->nSimWords, pPars->nWords);
    pParsSim->nFrames     = pPars->nRounds;
    pParsSim->fCheckMiter = pPars->fCheckMiter;
    pParsSim->fDualOut    = pPars->fDualOut;
    pParsSim->fVerbose    = pPars->fVerbose;
    pSim = Cec_ManSimStart( pIni, pParsSim );
    // SAT solving
    Cec_ManSatSetDefaultParams( pParsSat );
    pParsSat->nBTLimit = pPars->nBTLimit;
    pParsSat->fVerbose = pPars->fVeryVerbose;
    // simulation patterns
    pPat = Cec_ManPatStart();
    //pPat->fVerbose = pPars->fVeryVerbose;

    // start equivalence classes
clk = Abc_Clock();
    if ( p->pAig->pReprs == NULL )
    {
        if ( Cec_ManSimClassesPrepare(pSim, -1) || (!p->pAig->nSimWords && Cec_ManSimClassesRefine(pSim)) )
        {
            Gia_ManStop( p->pAig );
            p->pAig = NULL;
            goto finalize;
        }
    }
p->timeSim += Abc_Clock() - clk;
    // perform solving
    for ( i = 1; i <= pPars->nItersMax; i++ )
    {
        clk2 = Abc_Clock();
        nMatches = 0;
        if ( pPars->fDualOut )
        {
            nMatches = Gia_ManEquivSetColors( p->pAig, pPars->fVeryVerbose );
//            p->pAig->pIso = Cec_ManDetectIsomorphism( p->pAig );
//            Gia_ManEquivTransform( p->pAig, 1 );
        }
        pSrm = Cec_ManFraSpecReduction( p ); 

//        Gia_AigerWrite( pSrm, "gia_srm.aig", 0, 0, 0 );

        if ( pPars->fVeryVerbose )
            Gia_ManPrintStats( pSrm, NULL );
        if ( Gia_ManCoNum(pSrm) == 0 )
        {
            Gia_ManStop( pSrm );
            if ( p->pPars->fVerbose )
                Abc_Print( 1, "Considered all available candidate equivalences.\n" );
            if ( pPars->fDualOut && Gia_ManAndNum(p->pAig) > 0 )
            {
                if ( pPars->fColorDiff )
                {
                    if ( p->pPars->fVerbose )
                        Abc_Print( 1, "Switching into reduced mode.\n" );
                    pPars->fColorDiff = 0;
                }
                else
                {
                    if ( p->pPars->fVerbose )
                        Abc_Print( 1, "Switching into normal mode.\n" );
                    pPars->fDualOut = 0;
                }
                continue;
            }
            break;
        }
clk = Abc_Clock();
        if ( pPars->fRunCSat )
            Cec_ManSatSolveCSat( pPat, pSrm, pParsSat ); 
        else
            Cec_ManSatSolve( pPat, pSrm, pParsSat, p->pAig->vIdsOrig, p->vXorNodes, pAig->vIdsEquiv, 0 ); 
p->timeSat += Abc_Clock() - clk;
        if ( Cec_ManFraClassesUpdate( p, pSim, pPat, pSrm ) )
        {
            Gia_ManStop( pSrm );
            Gia_ManStop( p->pAig );
            p->pAig = NULL;
            goto finalize;
        }
        Gia_ManStop( pSrm );

        // update the manager
        pSim->pAig = p->pAig = Gia_ManEquivReduceAndRemap( pTemp = p->pAig, 0, pParsSim->fDualOut );
        if ( p->pAig == NULL )
        {
            p->pAig = pTemp;
            break;
        }
        Gia_ManStop( pTemp );
        if ( p->pPars->fVerbose )
        {
            Abc_Print( 1, "%3d : P =%7d. D =%7d. F =%6d. M = %7d. And =%8d. ", 
                i, p->nAllProved, p->nAllDisproved, p->nAllFailed, nMatches, Gia_ManAndNum(p->pAig) );
            Abc_PrintTime( 1, "Time", Abc_Clock() - clk2 );
        }
        if ( Gia_ManAndNum(p->pAig) == 0 )
        {
            if ( p->pPars->fVerbose )
                Abc_Print( 1, "Network after reduction is empty.\n" );
            break;
        }
        // check resource limits
        if ( p->pPars->TimeLimit && (Abc_Clock() - clkTotal)/CLOCKS_PER_SEC >= p->pPars->TimeLimit )
        {
            fTimeOut = 1;
            break;
        }
//        if ( p->nAllFailed && !p->nAllProved && !p->nAllDisproved )
        if ( p->nAllFailed > p->nAllProved + p->nAllDisproved )
        {
            if ( pParsSat->nBTLimit >= 10001 )
                break;
            if ( pPars->fSatSweeping )
            {
                if ( p->pPars->fVerbose )
                    Abc_Print( 1, "Exceeded the limit on the number of conflicts (%d).\n", pParsSat->nBTLimit );
                break;
            }
            pParsSat->nBTLimit *= 10;
            if ( p->pPars->fVerbose )
            {
                if ( p->pPars->fVerbose )
                    Abc_Print( 1, "Increasing conflict limit to %d.\n", pParsSat->nBTLimit );
                if ( fOutputResult )
                {
                    Gia_AigerWrite( p->pAig, "gia_cec_temp.aig", 0, 0, 0 );
                    Abc_Print( 1,"The result is written into file \"%s\".\n", "gia_cec_temp.aig" );
                }
            }
        }
        if ( pPars->fDualOut && pPars->fColorDiff && (Gia_ManAndNum(p->pAig) < 100000 || p->nAllProved + p->nAllDisproved < 10) )
        {
            if ( p->pPars->fVerbose )
                Abc_Print( 1, "Switching into reduced mode.\n" );
            pPars->fColorDiff = 0;
        }
//        if ( pPars->fDualOut && Gia_ManAndNum(p->pAig) < 20000 )
        else if ( pPars->fDualOut && (Gia_ManAndNum(p->pAig) < 20000 || p->nAllProved + p->nAllDisproved < 10) )
        {
            if ( p->pPars->fVerbose )
                Abc_Print( 1, "Switching into normal mode.\n" );
            pPars->fColorDiff = 0;
            pPars->fDualOut = 0;
        }
    }
finalize:
    if ( pPars->fVerbose )
        printf( "Performed %d SAT calls: P = %d  D = %d  F = %d\n", 
            p->nAllProvedS + p->nAllDisprovedS + p->nAllFailedS, p->nAllProvedS, p->nAllDisprovedS, p->nAllFailedS );
    if ( p->pPars->fVerbose && p->pAig )
    {
        Abc_Print( 1, "NBeg = %d. NEnd = %d. (Gain = %6.2f %%).  RBeg = %d. REnd = %d. (Gain = %6.2f %%).\n", 
            Gia_ManAndNum(pAig), Gia_ManAndNum(p->pAig), 
            100.0*(Gia_ManAndNum(pAig)-Gia_ManAndNum(p->pAig))/(Gia_ManAndNum(pAig)?Gia_ManAndNum(pAig):1), 
            Gia_ManRegNum(pAig), Gia_ManRegNum(p->pAig), 
            100.0*(Gia_ManRegNum(pAig)-Gia_ManRegNum(p->pAig))/(Gia_ManRegNum(pAig)?Gia_ManRegNum(pAig):1) );
        Abc_PrintTimeP( 1, "Sim ", p->timeSim, Abc_Clock() - (int)clkTotal );
        Abc_PrintTimeP( 1, "Sat ", p->timeSat-pPat->timeTotalSave, Abc_Clock() - (int)clkTotal );
        Abc_PrintTimeP( 1, "Pat ", p->timePat+pPat->timeTotalSave, Abc_Clock() - (int)clkTotal );
        Abc_PrintTime( 1, "Time", (int)(Abc_Clock() - clkTotal) );
    }

    pTemp = p->pAig; p->pAig = NULL;
    if ( pTemp == NULL && pSim->iOut >= 0 )
    {
        if ( !fSilent )
        Abc_Print( 1, "Disproved at least one output of the miter (zero-based number %d).\n", pSim->iOut );
        pPars->iOutFail = pSim->iOut;
    }
    else if ( pSim->pCexes && !fSilent )
        Abc_Print( 1, "Disproved %d outputs of the miter.\n", pSim->nOuts );
    if ( fTimeOut && !fSilent )
        Abc_Print( 1, "Timed out after %d seconds.\n", (int)((double)Abc_Clock() - clkTotal)/CLOCKS_PER_SEC );

    pAig->pCexComb = pSim->pCexComb; pSim->pCexComb = NULL;
    Cec_ManSimStop( pSim );
    Cec_ManPatStop( pPat );
    Cec_ManFraStop( p );
    if ( pTemp ) ABC_FREE( pTemp->pReprs );
    if ( pTemp ) ABC_FREE( pTemp->pNexts );
    return pTemp;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

