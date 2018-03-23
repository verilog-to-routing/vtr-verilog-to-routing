/**CFile****************************************************************

  FileName    [intCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Interpolation engine.]

  Synopsis    [Core procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 24, 2008.]

  Revision    [$Id: intCore.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "intInt.h"
#include "sat/bmc/bmc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////`
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [This procedure sets default values of interpolation parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Inter_ManSetDefaultParams( Inter_ManParams_t * p )
{ 
    memset( p, 0, sizeof(Inter_ManParams_t) );
    p->nBTLimit      = 0;     // limit on the number of conflicts
    p->nFramesMax    = 0;     // the max number timeframes to unroll
    p->nSecLimit     = 0;     // time limit in seconds
    p->nFramesK      = 1;     // the number of timeframes to use in induction
    p->fRewrite      = 0;     // use additional rewriting to simplify timeframes
    p->fTransLoop    = 0;     // add transition into the init state under new PI var
    p->fUsePudlak    = 0;     // use Pudluk interpolation procedure
    p->fUseOther     = 0;     // use other undisclosed option
    p->fUseMiniSat   = 0;     // use MiniSat-1.14p instead of internal proof engine
    p->fCheckKstep   = 1;     // check using K-step induction
    p->fUseBias      = 0;     // bias decisions to global variables
    p->fUseBackward  = 0;     // perform backward interpolation
    p->fUseSeparate  = 0;     // solve each output separately
    p->fUseTwoFrames = 0;     // create OR of two last timeframes
    p->fDropSatOuts  = 0;     // replace by 1 the solved outputs
    p->fVerbose      = 0;     // print verbose statistics
    p->iFrameMax     =-1;
}

/**Function*************************************************************

  Synopsis    [Interplates while the number of conflicts is not exceeded.]

  Description [Returns 1 if proven. 0 if failed. -1 if undecided.]
               
  SideEffects [Does not check the property in 0-th frame.]

  SeeAlso     []

***********************************************************************/
int Inter_ManPerformInterpolation( Aig_Man_t * pAig, Inter_ManParams_t * pPars, int * piFrame )
{
    extern int Inter_ManCheckInductiveContainment( Aig_Man_t * pTrans, Aig_Man_t * pInter, int nSteps, int fBackward );
    Inter_Man_t * p;
    Inter_Check_t * pCheck = NULL;
    Aig_Man_t * pAigTemp;
    int s, i, RetValue, Status;
    abctime clk, clk2, clkTotal = Abc_Clock(), timeTemp = 0;
    abctime nTimeNewOut = pPars->nSecLimit ? pPars->nSecLimit * CLOCKS_PER_SEC + Abc_Clock() : 0;

    // enable ORing of the interpolants, if containment check is performed inductively with K > 1
    if ( pPars->nFramesK > 1 )
        pPars->fTransLoop = 1;

    // sanity checks
    assert( Saig_ManRegNum(pAig) > 0 );
    assert( Saig_ManPiNum(pAig) > 0 );
    assert( Saig_ManPoNum(pAig)-Saig_ManConstrNum(pAig) == 1 );
    if ( pPars->fVerbose && Saig_ManConstrNum(pAig) )
        printf( "Performing interpolation with %d constraints...\n", Saig_ManConstrNum(pAig) );

    if ( Inter_ManCheckInitialState(pAig) )
    {
        *piFrame = -1;
        printf( "Property trivially fails in the initial state.\n" );
        return 0;
    }
/*
    if ( Inter_ManCheckAllStates(pAig) )
    {
        printf( "Property trivially holds in all states.\n" );
        return 1;
    }
*/
    // create interpolation manager
    // can perform SAT sweeping and/or rewriting of this AIG...
    p = Inter_ManCreate( pAig, pPars );
    if ( pPars->fTransLoop )
        p->pAigTrans = Inter_ManStartOneOutput( pAig, 0 );
    else
        p->pAigTrans = Inter_ManStartDuplicated( pAig );
    // derive CNF for the transformed AIG
clk = Abc_Clock();
    p->pCnfAig = Cnf_Derive( p->pAigTrans, Aig_ManRegNum(p->pAigTrans) ); 
p->timeCnf += Abc_Clock() - clk;    
    if ( pPars->fVerbose )
    { 
        printf( "AIG: PI/PO/Reg = %d/%d/%d. And = %d. Lev = %d.  CNF: Var/Cla = %d/%d.\n",
            Saig_ManPiNum(pAig), Saig_ManPoNum(pAig), Saig_ManRegNum(pAig), 
            Aig_ManAndNum(pAig), Aig_ManLevelNum(pAig),
            p->pCnfAig->nVars, p->pCnfAig->nClauses );
    }
 
    // derive interpolant
    *piFrame = -1;
    p->nFrames = 1;
    for ( s = 0; ; s++ )
    {
        Cnf_Dat_t * pCnfInter2;

clk2 = Abc_Clock();
        // initial state
        if ( pPars->fUseBackward )
            p->pInter = Inter_ManStartOneOutput( pAig, 1 );
        else
            p->pInter = Inter_ManStartInitState( Aig_ManRegNum(pAig) );
        assert( Aig_ManCoNum(p->pInter) == 1 );
clk = Abc_Clock();
        p->pCnfInter = Cnf_Derive( p->pInter, 0 );  
p->timeCnf += Abc_Clock() - clk;    
        // timeframes
        p->pFrames = Inter_ManFramesInter( pAig, p->nFrames, pPars->fUseBackward, pPars->fUseTwoFrames );
clk = Abc_Clock();
        if ( pPars->fRewrite )
        {
            p->pFrames = Dar_ManRwsat( pAigTemp = p->pFrames, 1, 0 );
            Aig_ManStop( pAigTemp );
//        p->pFrames = Fra_FraigEquivence( pAigTemp = p->pFrames, 100, 0 );
//        Aig_ManStop( pAigTemp );
        }
p->timeRwr += Abc_Clock() - clk;
        // can also do SAT sweeping on the timeframes...
clk = Abc_Clock();
        if ( pPars->fUseBackward )
            p->pCnfFrames = Cnf_Derive( p->pFrames, Aig_ManCoNum(p->pFrames) );  
        else
//            p->pCnfFrames = Cnf_Derive( p->pFrames, 0 );  
            p->pCnfFrames = Cnf_DeriveSimple( p->pFrames, 0 );  
p->timeCnf += Abc_Clock() - clk;    
        // report statistics
        if ( pPars->fVerbose )
        {
            printf( "Step = %2d. Frames = 1 + %d. And = %5d. Lev = %5d.  ", 
                s+1, p->nFrames, Aig_ManNodeNum(p->pFrames), Aig_ManLevelNum(p->pFrames) );
            ABC_PRT( "Time", Abc_Clock() - clk2 );
        }


        //////////////////////////////////////////
        // start containment checking
        if ( !(pPars->fTransLoop || pPars->fUseBackward || pPars->nFramesK > 1) )
        {
            pCheck = Inter_CheckStart( p->pAigTrans, pPars->nFramesK );
            // try new containment check for the initial state
clk = Abc_Clock();
            pCnfInter2 = Cnf_Derive( p->pInter, 1 );  
p->timeCnf += Abc_Clock() - clk;    
clk = Abc_Clock();
            RetValue = Inter_CheckPerform( pCheck, pCnfInter2, nTimeNewOut );
p->timeEqu += Abc_Clock() - clk;
//            assert( RetValue == 0 );
            Cnf_DataFree( pCnfInter2 );
            if ( p->vInters )
                Vec_PtrPush( p->vInters, Aig_ManDupSimple(p->pInter) );
        }
        //////////////////////////////////////////

        // iterate the interpolation procedure
        for ( i = 0; ; i++ )
        {
            if ( pPars->nFramesMax && p->nFrames + i >= pPars->nFramesMax )
            { 
                if ( pPars->fVerbose )
                    printf( "Reached limit (%d) on the number of timeframes.\n", pPars->nFramesMax );
                p->timeTotal = Abc_Clock() - clkTotal;
                Inter_ManStop( p, 0 );
                Inter_CheckStop( pCheck );
                return -1;
            }

            // perform interpolation
            clk = Abc_Clock();
#ifdef ABC_USE_LIBRARIES
            if ( pPars->fUseMiniSat )
            {
                assert( !pPars->fUseBackward );
                RetValue = Inter_ManPerformOneStepM114p( p, pPars->fUsePudlak, pPars->fUseOther );
            }
            else 
#endif
                RetValue = Inter_ManPerformOneStep( p, pPars->fUseBias, pPars->fUseBackward, nTimeNewOut );

            if ( pPars->fVerbose )
            {
                printf( "   I = %2d. Bmc =%3d. IntAnd =%6d. IntLev =%5d. Conf =%6d.  ", 
                    i+1, i + 1 + p->nFrames, Aig_ManNodeNum(p->pInter), Aig_ManLevelNum(p->pInter), p->nConfCur );
                ABC_PRT( "Time", Abc_Clock() - clk );
            }
            // remember the number of timeframes completed
            pPars->iFrameMax = i - 1 + p->nFrames;
            if ( RetValue == 0 ) // found a (spurious?) counter-example
            {
                if ( i == 0 ) // real counterexample
                {
                    if ( pPars->fVerbose )
                        printf( "Found a real counterexample in frame %d.\n", p->nFrames );
                    p->timeTotal = Abc_Clock() - clkTotal;
                    *piFrame = p->nFrames;
//                    pAig->pSeqModel = (Abc_Cex_t *)Inter_ManGetCounterExample( pAig, p->nFrames+1, pPars->fVerbose );
                    {
                        int RetValue;
                        Saig_ParBmc_t ParsBmc, * pParsBmc = &ParsBmc;
                        Saig_ParBmcSetDefaultParams( pParsBmc );
                        pParsBmc->nConfLimit = 100000000;
                        pParsBmc->nStart     = p->nFrames;
                        pParsBmc->fVerbose   = pPars->fVerbose;
                        RetValue = Saig_ManBmcScalable( pAig, pParsBmc );
                        if ( RetValue == 1 )
                            printf( "Error: The problem should be SAT but it is UNSAT.\n" );
                        else if ( RetValue == -1 )
                            printf( "Error: The problem timed out.\n" );
                    }
                    Inter_ManStop( p, 0 );
                    Inter_CheckStop( pCheck );
                    return 0;
                }
                // likely spurious counter-example
                p->nFrames += i;
                Inter_ManClean( p ); 
                break;
            }
            else if ( RetValue == -1 ) 
            {
                if ( pPars->nSecLimit && Abc_Clock() > nTimeNewOut ) // timed out
                {
                    if ( pPars->fVerbose )
                        printf( "Reached timeout (%d seconds).\n",  pPars->nSecLimit );
                }
                else
                {
                    assert( p->nConfCur >= p->nConfLimit );
                    if ( pPars->fVerbose )
                        printf( "Reached limit (%d) on the number of conflicts.\n", p->nConfLimit );
                }
                p->timeTotal = Abc_Clock() - clkTotal;
                Inter_ManStop( p, 0 );
                Inter_CheckStop( pCheck );
                return -1;
            }
            assert( RetValue == 1 ); // found new interpolant
            // compress the interpolant
clk = Abc_Clock();
            if ( p->pInterNew )
            {
                // save the timeout value
                p->pInterNew->Time2Quit = nTimeNewOut;
//                Ioa_WriteAiger( p->pInterNew, "interpol.aig", 0, 0 );
                p->pInterNew = Dar_ManRwsat( pAigTemp = p->pInterNew, 1, 0 );
//                p->pInterNew = Dar_ManRwsat( pAigTemp = p->pInterNew, 0, 0 );
                Aig_ManStop( pAigTemp );
                if ( p->pInterNew == NULL )
                {
                    printf( "Reached timeout (%d seconds) during rewriting.\n",  pPars->nSecLimit );
                    p->timeTotal = Abc_Clock() - clkTotal;
                    Inter_ManStop( p, 1 );
                    Inter_CheckStop( pCheck );
                    return -1;
                }
            }
p->timeRwr += Abc_Clock() - clk;

            // check if interpolant is trivial
            if ( p->pInterNew == NULL || Aig_ObjChild0(Aig_ManCo(p->pInterNew,0)) == Aig_ManConst0(p->pInterNew) )
            { 
//                printf( "interpolant is constant 0\n" );
                if ( pPars->fVerbose )
                    printf( "The problem is trivially true for all states.\n" );
                p->timeTotal = Abc_Clock() - clkTotal;
                Inter_ManStop( p, 1 );
                Inter_CheckStop( pCheck );
                return 1;
            }

            // check containment of interpolants
clk = Abc_Clock();
            if ( pPars->fCheckKstep ) // k-step unique-state induction
            {
                if ( Aig_ManCiNum(p->pInterNew) == Aig_ManCiNum(p->pInter) )
                {
                    if ( pPars->fTransLoop || pPars->fUseBackward || pPars->nFramesK > 1 )
                    {
clk2 = Abc_Clock();
                        Status = Inter_ManCheckInductiveContainment( p->pAigTrans, p->pInterNew, Abc_MinInt(i + 1, pPars->nFramesK), pPars->fUseBackward );
timeTemp = Abc_Clock() - clk2;
                    }
                    else
                    {   // new containment check
clk2 = Abc_Clock();
                        pCnfInter2 = Cnf_Derive( p->pInterNew, 1 );  
p->timeCnf += Abc_Clock() - clk2;
timeTemp = Abc_Clock() - clk2;
            
                        Status = Inter_CheckPerform( pCheck, pCnfInter2, nTimeNewOut );
                        Cnf_DataFree( pCnfInter2 );
                        if ( p->vInters )
                            Vec_PtrPush( p->vInters, Aig_ManDupSimple(p->pInterNew) );
                    }
                }
                else
                    Status = 0;
            }
            else // combinational containment
            {
                if ( Aig_ManCiNum(p->pInterNew) == Aig_ManCiNum(p->pInter) )
                    Status = Inter_ManCheckContainment( p->pInterNew, p->pInter );
                else
                    Status = 0;
            }
p->timeEqu += Abc_Clock() - clk - timeTemp;
            if ( Status ) // contained
            {
                if ( pPars->fVerbose )
                    printf( "Proved containment of interpolants.\n" );
                p->timeTotal = Abc_Clock() - clkTotal;
                Inter_ManStop( p, 1 );
                Inter_CheckStop( pCheck );
                return 1;
            }
            if ( pPars->nSecLimit && Abc_Clock() > nTimeNewOut )
            {
                printf( "Reached timeout (%d seconds).\n",  pPars->nSecLimit );
                p->timeTotal = Abc_Clock() - clkTotal;
                Inter_ManStop( p, 1 );
                Inter_CheckStop( pCheck );
                return -1;
            }
            // save interpolant and convert it into CNF
            if ( pPars->fTransLoop )
            {
                Aig_ManStop( p->pInter );
                p->pInter = p->pInterNew; 
            }
            else
            {
                if ( pPars->fUseBackward )
                {
                    p->pInter = Aig_ManCreateMiter( pAigTemp = p->pInter, p->pInterNew, 2 );
                    Aig_ManStop( pAigTemp );
                    Aig_ManStop( p->pInterNew );
                    // compress the interpolant
clk = Abc_Clock();
                    p->pInter = Dar_ManRwsat( pAigTemp = p->pInter, 1, 0 );
                    Aig_ManStop( pAigTemp );
p->timeRwr += Abc_Clock() - clk;
                }
                else // forward with the new containment checking (using only the frontier)
                {
                    Aig_ManStop( p->pInter );
                    p->pInter = p->pInterNew; 
                }
            }
            p->pInterNew = NULL;
            Cnf_DataFree( p->pCnfInter );
clk = Abc_Clock();
            p->pCnfInter = Cnf_Derive( p->pInter, 0 );  
p->timeCnf += Abc_Clock() - clk;
        }

        // start containment checking
        Inter_CheckStop( pCheck );
    }
    assert( 0 );
    return RetValue;
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

