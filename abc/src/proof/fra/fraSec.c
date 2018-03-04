/**CFile****************************************************************

  FileName    [fraSec.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [New FRAIG package.]

  Synopsis    [Performs SEC based on seq sweeping.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 30, 2007.]

  Revision    [$Id: fraSec.c,v 1.00 2007/06/30 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fra.h"
#include "aig/ioa/ioa.h"
#include "proof/int/int.h"
#include "proof/ssw/ssw.h"
#include "aig/saig/saig.h"
#include "bdd/bbr/bbr.h"
#include "proof/pdr/pdr.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_SecSetDefaultParams( Fra_Sec_t * p )
{
    memset( p, 0, sizeof(Fra_Sec_t) );
    p->fTryComb          =       1;  // try CEC call as a preprocessing step
    p->fTryBmc           =       1;  // try BMC call as a preprocessing step 
    p->nFramesMax        =       4;  // the max number of frames used for induction
    p->nBTLimit          =    1000;  // conflict limit at a node during induction 
    p->nBTLimitGlobal    = 5000000;  // global conflict limit during induction
    p->nBTLimitInter     =   10000;  // conflict limit during interpolation
    p->nBddVarsMax       =     150;  // the limit on the number of registers in BDD reachability
    p->nBddMax           =   50000;  // the limit on the number of BDD nodes 
    p->nBddIterMax       = 1000000;  // the limit on the number of BDD iterations
    p->fPhaseAbstract    =       0;  // enables phase abstraction
    p->fRetimeFirst      =       1;  // enables most-forward retiming at the beginning
    p->fRetimeRegs       =       1;  // enables min-register retiming at the beginning
    p->fFraiging         =       1;  // enables fraiging at the beginning
    p->fInduction        =       1;  // enables the use of induction (signal correspondence)
    p->fInterpolation    =       1;  // enables interpolation
    p->fInterSeparate    =       0;  // enables interpolation for each outputs separately
    p->fReachability     =       1;  // enables BDD based reachability
    p->fReorderImage     =       1;  // enables variable reordering during image computation
    p->fStopOnFirstFail  =       1;  // enables stopping after first output of a miter has failed to prove
    p->fUseNewProver     =       0;  // enables new prover
    p->fUsePdr           =       1;  // enables PDR
    p->nPdrTimeout       =      60;  // enabled PDR timeout
    p->fSilent           =       0;  // disables all output
    p->fVerbose          =       0;  // enables verbose reporting of statistics
    p->fVeryVerbose      =       0;  // enables very verbose reporting  
    p->TimeLimit         =       0;  // enables the timeout
    // internal parameters
    p->fReportSolution   =       0;  // enables specialized format for reporting solution
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_FraigSec( Aig_Man_t * p, Fra_Sec_t * pParSec, Aig_Man_t ** ppResult )
{
    Ssw_Pars_t Pars2, * pPars2 = &Pars2;
    Fra_Ssw_t Pars, * pPars = &Pars;
    Fra_Sml_t * pSml;
    Aig_Man_t * pNew, * pTemp;
    int nFrames, RetValue, nIter;
    abctime clk, clkTotal = Abc_Clock();
    int TimeOut = 0;
    int fLatchCorr = 0;
    float TimeLeft = 0.0;
    pParSec->nSMnumber = -1;

    // try the miter before solving
    pNew = Aig_ManDupSimple( p );
    RetValue = Fra_FraigMiterStatus( pNew );
    if ( RetValue >= 0 )
        goto finish;

    // prepare parameters
    memset( pPars, 0, sizeof(Fra_Ssw_t) );
    pPars->fLatchCorr  = fLatchCorr;
    pPars->fVerbose = pParSec->fVeryVerbose;
    if ( pParSec->fVerbose )
    {
        printf( "Original miter:       Latches = %5d. Nodes = %6d.\n", 
            Aig_ManRegNum(pNew), Aig_ManNodeNum(pNew) );
    }
//Aig_ManDumpBlif( pNew, "after.blif", NULL, NULL );

    // perform sequential cleanup
clk = Abc_Clock();
    if ( pNew->nRegs )
    pNew = Aig_ManReduceLaches( pNew, 0 );
    if ( pNew->nRegs )
    pNew = Aig_ManConstReduce( pNew, 0, -1, -1, 0, 0 );
    if ( pParSec->fVerbose )
    {
        printf( "Sequential cleanup:   Latches = %5d. Nodes = %6d. ", 
            Aig_ManRegNum(pNew), Aig_ManNodeNum(pNew) );
ABC_PRT( "Time", Abc_Clock() - clk );
    }
    RetValue = Fra_FraigMiterStatus( pNew );
    if ( RetValue >= 0 )
        goto finish;

    // perform phase abstraction
clk = Abc_Clock();
    if ( pParSec->fPhaseAbstract )
    {
        extern Aig_Man_t * Saig_ManPhaseAbstractAuto( Aig_Man_t * p, int fVerbose );
        pNew->nTruePis = Aig_ManCiNum(pNew) - Aig_ManRegNum(pNew); 
        pNew->nTruePos = Aig_ManCoNum(pNew) - Aig_ManRegNum(pNew); 
        pNew = Saig_ManPhaseAbstractAuto( pTemp = pNew, 0 );
        Aig_ManStop( pTemp );
        if ( pParSec->fVerbose )
        {
            printf( "Phase abstraction:    Latches = %5d. Nodes = %6d. ", 
                Aig_ManRegNum(pNew), Aig_ManNodeNum(pNew) );
ABC_PRT( "Time", Abc_Clock() - clk );
        }
    }

    // perform forward retiming
    if ( pParSec->fRetimeFirst && pNew->nRegs )
    {
clk = Abc_Clock();
//    pNew = Rtm_ManRetime( pTemp = pNew, 1, 1000, 0 );
    pNew = Saig_ManRetimeForward( pTemp = pNew, 100, 0 );
    Aig_ManStop( pTemp );
    if ( pParSec->fVerbose )
    {
        printf( "Forward retiming:     Latches = %5d. Nodes = %6d. ", 
            Aig_ManRegNum(pNew), Aig_ManNodeNum(pNew) );
ABC_PRT( "Time", Abc_Clock() - clk );
    }
    }
     
    // run latch correspondence
clk = Abc_Clock();
    if ( pNew->nRegs )
    {
    pNew = Aig_ManDupOrdered( pTemp = pNew );
//    pNew = Aig_ManDupDfs( pTemp = pNew );
    Aig_ManStop( pTemp );
/*
    if ( RetValue == -1 && pParSec->TimeLimit )
    {
        TimeLeft = (float)pParSec->TimeLimit - ((float)(Abc_Clock()-clkTotal)/(float)(CLOCKS_PER_SEC));
        TimeLeft = Abc_MaxInt( TimeLeft, 0.0 );
        if ( TimeLeft == 0.0 )
        {
            if ( !pParSec->fSilent )
                printf( "Runtime limit exceeded.\n" );
            RetValue = -1;
            TimeOut = 1;
            goto finish;
        }
    }
*/

//    pNew = Fra_FraigLatchCorrespondence( pTemp = pNew, 0, 1000, 1, pParSec->fVeryVerbose, &nIter, TimeLeft );
//Aig_ManDumpBlif( pNew, "ex.blif", NULL, NULL );
    Ssw_ManSetDefaultParamsLcorr( pPars2 );
    pNew = Ssw_LatchCorrespondence( pTemp = pNew, pPars2 );
    nIter = pPars2->nIters;

    // prepare parameters for scorr
    Ssw_ManSetDefaultParams( pPars2 );

    if ( pTemp->pSeqModel )
    {
        if ( !Saig_ManVerifyCex( pTemp, pTemp->pSeqModel ) )
            printf( "Fra_FraigSec(): Counter-example verification has FAILED.\n" );
        if ( Saig_ManPiNum(p) != Saig_ManPiNum(pTemp) )
            printf( "The counter-example is invalid because of phase abstraction.\n" );
        else
        {
        ABC_FREE( p->pSeqModel );
        p->pSeqModel = Abc_CexDup( pTemp->pSeqModel, Aig_ManRegNum(p) );
        ABC_FREE( pTemp->pSeqModel );
        }
    }
    if ( pNew == NULL )
    {
        if ( p->pSeqModel )
        {
            RetValue = 0;
            if ( !pParSec->fSilent )
            {
                printf( "Networks are NOT EQUIVALENT after simulation.   " );
ABC_PRT( "Time", Abc_Clock() - clkTotal );
            }
            if ( pParSec->fReportSolution && !pParSec->fRecursive )
            {
            printf( "SOLUTION: FAIL       " );
ABC_PRT( "Time", Abc_Clock() - clkTotal );
            }
            Aig_ManStop( pTemp );
            return RetValue;
        }
        pNew = pTemp;
        RetValue = -1;
        TimeOut = 1;
        goto finish;
    }
    Aig_ManStop( pTemp );

    if ( pParSec->fVerbose )
    {
        printf( "Latch-corr (I=%3d):   Latches = %5d. Nodes = %6d. ", 
            nIter, Aig_ManRegNum(pNew), Aig_ManNodeNum(pNew) );
ABC_PRT( "Time", Abc_Clock() - clk );
    }
    }
/*
    if ( RetValue == -1 && pParSec->TimeLimit )
    {
        TimeLeft = (float)pParSec->TimeLimit - ((float)(Abc_Clock()-clkTotal)/(float)(CLOCKS_PER_SEC));
        TimeLeft = Abc_MaxInt( TimeLeft, 0.0 );
        if ( TimeLeft == 0.0 )
        {
            if ( !pParSec->fSilent )
                printf( "Runtime limit exceeded.\n" );
            RetValue = -1;
            TimeOut = 1;
            goto finish;
        }
    }
*/
    // perform fraiging
    if ( pParSec->fFraiging )
    {
clk = Abc_Clock();
    pNew = Fra_FraigEquivence( pTemp = pNew, 100, 0 );
    Aig_ManStop( pTemp );
    if ( pParSec->fVerbose )
    {
        printf( "Fraiging:             Latches = %5d. Nodes = %6d. ", 
            Aig_ManRegNum(pNew), Aig_ManNodeNum(pNew) );
ABC_PRT( "Time", Abc_Clock() - clk );
    }
    }

    if ( pNew->nRegs == 0 )
        RetValue = Fra_FraigCec( &pNew, 100000, 0 );

    RetValue = Fra_FraigMiterStatus( pNew );
    if ( RetValue >= 0 )
        goto finish;
/*
    if ( RetValue == -1 && pParSec->TimeLimit )
    {
        TimeLeft = (float)pParSec->TimeLimit - ((float)(Abc_Clock()-clkTotal)/(float)(CLOCKS_PER_SEC));
        TimeLeft = Abc_MaxInt( TimeLeft, 0.0 );
        if ( TimeLeft == 0.0 )
        {
            if ( !pParSec->fSilent )
                printf( "Runtime limit exceeded.\n" );
            RetValue = -1;
            TimeOut = 1;
            goto finish;
        }
    }
*/
    // perform min-area retiming
    if ( pParSec->fRetimeRegs && pNew->nRegs )
    {
//    extern Aig_Man_t * Saig_ManRetimeMinArea( Aig_Man_t * p, int nMaxIters, int fForwardOnly, int fBackwardOnly, int fInitial, int fVerbose );
clk = Abc_Clock();
    pNew->nTruePis = Aig_ManCiNum(pNew) - Aig_ManRegNum(pNew); 
    pNew->nTruePos = Aig_ManCoNum(pNew) - Aig_ManRegNum(pNew); 
//        pNew = Rtm_ManRetime( pTemp = pNew, 1, 1000, 0 );
    pNew = Saig_ManRetimeMinArea( pTemp = pNew, 1000, 0, 0, 1, 0 );
    Aig_ManStop( pTemp );
    pNew = Aig_ManDupOrdered( pTemp = pNew );
    Aig_ManStop( pTemp );
    if ( pParSec->fVerbose )
    {
    printf( "Min-reg retiming:     Latches = %5d. Nodes = %6d. ", 
        Aig_ManRegNum(pNew), Aig_ManNodeNum(pNew) );
ABC_PRT( "Time", Abc_Clock() - clk );
    }
    }

    // perform seq sweeping while increasing the number of frames
    RetValue = Fra_FraigMiterStatus( pNew );
    if ( RetValue == -1 && pParSec->fInduction )
    for ( nFrames = 1; nFrames <= pParSec->nFramesMax; nFrames *= 2 )
    {
/*
        if ( RetValue == -1 && pParSec->TimeLimit )
        {
            TimeLeft = (float)pParSec->TimeLimit - ((float)(Abc_Clock()-clkTotal)/(float)(CLOCKS_PER_SEC));
            TimeLeft = Abc_MaxInt( TimeLeft, 0.0 );
            if ( TimeLeft == 0.0 )
            {
                if ( !pParSec->fSilent )
                    printf( "Runtime limit exceeded.\n" );
                RetValue = -1;
                TimeOut = 1;
                goto finish;
            }
        }
*/ 

clk = Abc_Clock();
        pPars->nFramesK = nFrames;
        pPars->TimeLimit = TimeLeft;
        pPars->fSilent = pParSec->fSilent;
//        pNew = Fra_FraigInduction( pTemp = pNew, pPars );

        pPars2->nFramesK = nFrames;
        pPars2->nBTLimit = pParSec->nBTLimit;
        pPars2->nBTLimitGlobal = pParSec->nBTLimitGlobal;
//        pPars2->nBTLimit = 1000 * nFrames;

        if ( RetValue == -1 && pPars2->nConflicts > pPars2->nBTLimitGlobal )
        {
            if ( !pParSec->fSilent )
                printf( "Global conflict limit (%d) exceeded.\n", pPars2->nBTLimitGlobal );
            RetValue = -1;
            TimeOut = 1;
            goto finish;
        }

        Aig_ManSetRegNum( pNew, pNew->nRegs );
//        pNew = Ssw_SignalCorrespondence( pTemp = pNew, pPars2 );
        if ( Aig_ManRegNum(pNew) > 0 )
            pNew = Ssw_SignalCorrespondence( pTemp = pNew, pPars2 );
        else
            pNew = Aig_ManDupSimpleDfs( pTemp = pNew );

        if ( pNew == NULL )
        {
            pNew = pTemp;
            RetValue = -1;
            TimeOut = 1;
            goto finish;
        }

//        printf( "Total conflicts = %d.\n", pPars2->nConflicts );

        Aig_ManStop( pTemp );
        RetValue = Fra_FraigMiterStatus( pNew );
        if ( pParSec->fVerbose )
        { 
            printf( "K-step (K=%2d,I=%3d):  Latches = %5d. Nodes = %6d. ", 
                nFrames, pPars2->nIters, Aig_ManRegNum(pNew), Aig_ManNodeNum(pNew) );
ABC_PRT( "Time", Abc_Clock() - clk );
        }
        if ( RetValue != -1 )
            break;

        // perform retiming
//        if ( pParSec->fRetimeFirst && pNew->nRegs )
        if ( pNew->nRegs )
        {
//        extern Aig_Man_t * Saig_ManRetimeMinArea( Aig_Man_t * p, int nMaxIters, int fForwardOnly, int fBackwardOnly, int fInitial, int fVerbose );
clk = Abc_Clock();
        pNew->nTruePis = Aig_ManCiNum(pNew) - Aig_ManRegNum(pNew); 
        pNew->nTruePos = Aig_ManCoNum(pNew) - Aig_ManRegNum(pNew); 
//        pNew = Rtm_ManRetime( pTemp = pNew, 1, 1000, 0 );
        pNew = Saig_ManRetimeMinArea( pTemp = pNew, 1000, 0, 0, 1, 0 );
        Aig_ManStop( pTemp );
        pNew = Aig_ManDupOrdered( pTemp = pNew );
        Aig_ManStop( pTemp );
        if ( pParSec->fVerbose )
        {
            printf( "Min-reg retiming:     Latches = %5d. Nodes = %6d. ", 
                Aig_ManRegNum(pNew), Aig_ManNodeNum(pNew) );
ABC_PRT( "Time", Abc_Clock() - clk );
        }
        }  

        if ( pNew->nRegs )
            pNew = Aig_ManConstReduce( pNew, 0, -1, -1, 0, 0 );

        // perform rewriting
clk = Abc_Clock();
        pNew = Aig_ManDupOrdered( pTemp = pNew );
        Aig_ManStop( pTemp );
//        pNew = Dar_ManRewriteDefault( pTemp = pNew );
        pNew = Dar_ManCompress2( pTemp = pNew, 1, 0, 1, 0, 0 ); 
        Aig_ManStop( pTemp );
        if ( pParSec->fVerbose )
        {
            printf( "Rewriting:            Latches = %5d. Nodes = %6d. ", 
                Aig_ManRegNum(pNew), Aig_ManNodeNum(pNew) );
ABC_PRT( "Time", Abc_Clock() - clk );
        } 

        // perform sequential simulation
        if ( pNew->nRegs )
        {
clk = Abc_Clock();
        pSml = Fra_SmlSimulateSeq( pNew, 0, 128 * nFrames, 1 + 16/(1+Aig_ManNodeNum(pNew)/1000), 1  ); 
        if ( pParSec->fVerbose )
        {
            printf( "Seq simulation  :     Latches = %5d. Nodes = %6d. ", 
                Aig_ManRegNum(pNew), Aig_ManNodeNum(pNew) );
ABC_PRT( "Time", Abc_Clock() - clk );
        }
        if ( pSml->fNonConstOut )
        {
            pNew->pSeqModel = Fra_SmlGetCounterExample( pSml );
            // transfer to the original manager
            if ( Saig_ManPiNum(p) != Saig_ManPiNum(pNew) )
                printf( "The counter-example is invalid because of phase abstraction.\n" );
            else
            {
            ABC_FREE( p->pSeqModel );
            p->pSeqModel = Abc_CexDup( pNew->pSeqModel, Aig_ManRegNum(p) );
            ABC_FREE( pNew->pSeqModel );
            }

            Fra_SmlStop( pSml );
            Aig_ManStop( pNew );
            RetValue = 0;
            if ( !pParSec->fSilent )
            {
                printf( "Networks are NOT EQUIVALENT after simulation.   " );
ABC_PRT( "Time", Abc_Clock() - clkTotal );
            }
            if ( pParSec->fReportSolution && !pParSec->fRecursive )
            {
            printf( "SOLUTION: FAIL       " );
ABC_PRT( "Time", Abc_Clock() - clkTotal );
            }
            return RetValue;
        }
        Fra_SmlStop( pSml );
        }
    }

    // get the miter status
    RetValue = Fra_FraigMiterStatus( pNew );

    // try interplation
clk = Abc_Clock();
    Aig_ManSetRegNum( pNew, Aig_ManRegNum(pNew) );
    if ( pParSec->fInterpolation && RetValue == -1 && Aig_ManRegNum(pNew) > 0 )
    {
        Inter_ManParams_t Pars, * pPars = &Pars;
        int Depth;
        ABC_FREE( pNew->pSeqModel );
        Inter_ManSetDefaultParams( pPars );
//        pPars->nBTLimit = 100;
        pPars->nBTLimit = pParSec->nBTLimitInter;
        pPars->fVerbose = pParSec->fVeryVerbose;
        if ( Saig_ManPoNum(pNew) == 1 )
        {
            RetValue = Inter_ManPerformInterpolation( pNew, pPars, &Depth );
        }
        else if ( pParSec->fInterSeparate )
        {
            Abc_Cex_t * pCex = NULL;
            Aig_Man_t * pTemp, * pAux;
            Aig_Obj_t * pObjPo;
            int i, Counter = 0;
            Saig_ManForEachPo( pNew, pObjPo, i )
            { 
                if ( Aig_ObjFanin0(pObjPo) == Aig_ManConst1(pNew) )
                    continue;
                if ( pPars->fVerbose )
                    printf( "Solving output %2d (out of %2d):\n", i, Saig_ManPoNum(pNew) );
                pTemp = Aig_ManDupOneOutput( pNew, i, 1 );
                pTemp = Aig_ManScl( pAux = pTemp, 1, 1, 0, -1, -1, 0, 0 );
                Aig_ManStop( pAux );
                if ( Saig_ManRegNum(pTemp) > 0 )
                {
                    RetValue = Inter_ManPerformInterpolation( pTemp, pPars, &Depth );
                    if ( pTemp->pSeqModel )
                    {
                        pCex = p->pSeqModel = Abc_CexDup( pTemp->pSeqModel, Aig_ManRegNum(p) );
                        pCex->iPo = i;
                        Aig_ManStop( pTemp );
                        break;
                    }
                    // if solved, remove the output
                    if ( RetValue == 1 )
                    {
                        Aig_ObjPatchFanin0( pNew, pObjPo, Aig_ManConst0(pNew) );
    //                    printf( "Output %3d : Solved ", i );
                    }
                    else
                    {
                        Counter++;
    //                    printf( "Output %3d : Undec  ", i );
                    }
                }
                else
                    Counter++;
//                Aig_ManPrintStats( pTemp );
                Aig_ManStop( pTemp );
                printf( "Solving output %3d (out of %3d) using interpolation.\r", i, Saig_ManPoNum(pNew) );
            }
            Aig_ManCleanup( pNew );
            if ( pCex == NULL )
            {
                printf( "Interpolation left %d (out of %d) outputs unsolved              \n", Counter, Saig_ManPoNum(pNew) );
                if ( Counter )
                    RetValue = -1;
            }
            pNew = Aig_ManDupUnsolvedOutputs( pTemp = pNew, 1 );
            Aig_ManStop( pTemp );
            pNew = Aig_ManScl( pTemp = pNew, 1, 1, 0, -1, -1, 0, 0 );
            Aig_ManStop( pTemp );
        }
        else
        {
            Aig_Man_t * pNewOrpos = Saig_ManDupOrpos( pNew );
            RetValue = Inter_ManPerformInterpolation( pNewOrpos, pPars, &Depth );
            if ( pNewOrpos->pSeqModel )
            {
                Abc_Cex_t * pCex;
                pCex = pNew->pSeqModel = pNewOrpos->pSeqModel; pNewOrpos->pSeqModel = NULL;
                pCex->iPo = Saig_ManFindFailedPoCex( pNew, pNew->pSeqModel );
            }
            Aig_ManStop( pNewOrpos );
        }

        if ( pParSec->fVerbose )
        {
        if ( RetValue == 1 )
            printf( "Property proved using interpolation.  " );
        else if ( RetValue == 0 )
            printf( "Property DISPROVED in frame %d using interpolation.  ", Depth );
        else if ( RetValue == -1 )
            printf( "Property UNDECIDED after interpolation.  " );
        else
            assert( 0 ); 
ABC_PRT( "Time", Abc_Clock() - clk );
        }
    }

    // try reachability analysis
    if ( pParSec->fReachability && RetValue == -1 && Aig_ManRegNum(pNew) > 0 && Aig_ManRegNum(pNew) < pParSec->nBddVarsMax )
    {
        Saig_ParBbr_t Pars, * pPars = &Pars;
        Bbr_ManSetDefaultParams( pPars );
        pPars->TimeLimit     = 0;
        pPars->nBddMax       = pParSec->nBddMax;
        pPars->nIterMax      = pParSec->nBddIterMax;
        pPars->fPartition    = 1;
        pPars->fReorder      = 1;
        pPars->fReorderImage = 1;
        pPars->fVerbose      = 0;
        pPars->fSilent       = pParSec->fSilent;
        pNew->nTruePis = Aig_ManCiNum(pNew) - Aig_ManRegNum(pNew); 
        pNew->nTruePos = Aig_ManCoNum(pNew) - Aig_ManRegNum(pNew); 
        RetValue = Aig_ManVerifyUsingBdds( pNew, pPars );
    }

    // try PDR
    if ( pParSec->fUsePdr && RetValue == -1 && Aig_ManRegNum(pNew) > 0 )
    {
        Pdr_Par_t Pars, * pPars = &Pars;
        Pdr_ManSetDefaultParams( pPars );
        pPars->nTimeOut = pParSec->nPdrTimeout;
        pPars->fVerbose = pParSec->fVerbose;
        if ( pParSec->fVerbose )
            printf( "Running property directed reachability...\n" );
        RetValue = Pdr_ManSolve( pNew, pPars );
        if ( pNew->pSeqModel )
            pNew->pSeqModel->iPo = Saig_ManFindFailedPoCex( pNew, pNew->pSeqModel );
    }

finish:
    // report the miter
    if ( RetValue == 1 )
    {
        if ( !pParSec->fSilent )
        {
            printf( "Networks are equivalent.  " );
ABC_PRT( "Time", Abc_Clock() - clkTotal );
        }
        if ( pParSec->fReportSolution && !pParSec->fRecursive )
        {
        printf( "SOLUTION: PASS       " );
ABC_PRT( "Time", Abc_Clock() - clkTotal );
        }
    }
    else if ( RetValue == 0 )
    {
        if ( pNew->pSeqModel == NULL )
        {
            int i;
            // if the CEX is not derives, it is because tricial CEX should be assumed
            pNew->pSeqModel = Abc_CexAlloc( Aig_ManRegNum(pNew), pNew->nTruePis, 1 );
            // if the CEX does not work, we need to change PIs to 1 because 
            // the only way it can happen is when a PO is equal to a PI...
            if ( Saig_ManFindFailedPoCex( pNew, pNew->pSeqModel ) == -1 )
                for ( i = 0; i < pNew->nTruePis; i++ )
                    Abc_InfoSetBit( pNew->pSeqModel->pData, i );
        }
        if ( !pParSec->fSilent )
        {
            printf( "Networks are NOT EQUIVALENT.  " );
ABC_PRT( "Time", Abc_Clock() - clkTotal );
        }
        if ( pParSec->fReportSolution && !pParSec->fRecursive )
        {
        printf( "SOLUTION: FAIL       " );
ABC_PRT( "Time", Abc_Clock() - clkTotal );
        }
    }
    else 
    {
        ///////////////////////////////////
        // save intermediate result
        extern void Abc_FrameSetSave1( void * pAig );
        Abc_FrameSetSave1( Aig_ManDupSimple(pNew) );
        ///////////////////////////////////
        if ( !pParSec->fSilent )
        {
            printf( "Networks are UNDECIDED.   " );
ABC_PRT( "Time", Abc_Clock() - clkTotal );
        }
        if ( pParSec->fReportSolution && !pParSec->fRecursive )
        {
        printf( "SOLUTION: UNDECIDED  " );
ABC_PRT( "Time", Abc_Clock() - clkTotal );
        }
        if ( !TimeOut && !pParSec->fSilent )
        {
            static int Counter = 1;
            char pFileName[1000];
            pParSec->nSMnumber = Counter;
            sprintf( pFileName, "sm%02d.aig", Counter++ );
            Ioa_WriteAiger( pNew, pFileName, 0, 0 );
            printf( "The unsolved reduced miter is written into file \"%s\".\n", pFileName );
        }
    }
    if ( pNew->pSeqModel )
    {
        if ( Saig_ManPiNum(p) != Saig_ManPiNum(pNew) )
            printf( "The counter-example is invalid because of phase abstraction.\n" );
        else
        {
        ABC_FREE( p->pSeqModel );
        p->pSeqModel = Abc_CexDup( pNew->pSeqModel, Aig_ManRegNum(p) );
        ABC_FREE( pNew->pSeqModel );
        }
    }
    if ( ppResult != NULL )
        *ppResult = Aig_ManDupSimpleDfs( pNew );
    if ( pNew )
        Aig_ManStop( pNew );
    return RetValue;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

