/**CFile****************************************************************

  FileName    [cecSplit.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Combinational equivalence checking.]

  Synopsis    [Cofactoring for combinational miters.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cecSplit.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include <math.h>
#include "aig/gia/gia.h"
#include "aig/gia/giaAig.h"

#include "sat/bmc/bmc.h"
#include "proof/pdr/pdr.h"
#include "proof/cec/cec.h"
#include "proof/ssw/ssw.h"


#ifdef ABC_USE_PTHREADS

#ifdef _WIN32
#include "../lib/pthread.h"
#else
#include <pthread.h>
#include <unistd.h>
#endif

#endif

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern int Ssw_RarSimulateGia( Gia_Man_t * p, Ssw_RarPars_t * pPars );
extern int Bmcg_ManPerform( Gia_Man_t * pGia, Bmc_AndPar_t * pPars );

#ifndef ABC_USE_PTHREADS

int Cec_GiaProveTest( Gia_Man_t * p, int nProcs, int nTimeOut, int nTimeOut2, int nTimeOut3, int fVerbose, int fVeryVerbose, int fSilent ) { return -1; }

#else // pthreads are used

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cec_GiaProveOne( Gia_Man_t * p, int iEngine, int nTimeOut, int fVerbose )
{
    abctime clk = Abc_Clock();   
    int RetValue = -1;
    //abctime clkStop = nTimeOut * CLOCKS_PER_SEC + Abc_Clock();
    if ( fVerbose )
    printf( "Calling engine %d with timeout %d sec.\n", iEngine, nTimeOut );
    Abc_CexFreeP( &p->pCexSeq );
    if ( iEngine == 0 )
    {
        Ssw_RarPars_t Pars, * pPars = &Pars;
        Ssw_RarSetDefaultParams( pPars );
        pPars->TimeOut = nTimeOut;
        pPars->fSilent = 1;
        RetValue = Ssw_RarSimulateGia( p, pPars );
    }
    else if ( iEngine == 1 )
    {
        Saig_ParBmc_t Pars, * pPars = &Pars;
        Saig_ParBmcSetDefaultParams( pPars );
        pPars->nTimeOut = nTimeOut;
        pPars->fSilent  = 1;
        Aig_Man_t * pAig = Gia_ManToAigSimple( p );
        RetValue = Saig_ManBmcScalable( pAig, pPars );
        p->pCexSeq = pAig->pSeqModel; pAig->pSeqModel = NULL;
        Aig_ManStop( pAig );                 
    }
    else if ( iEngine == 2 )
    {
        Pdr_Par_t Pars, * pPars = &Pars;
        Pdr_ManSetDefaultParams( pPars );
        pPars->nTimeOut = nTimeOut;
        pPars->fSilent  = 1;
        Aig_Man_t * pAig = Gia_ManToAigSimple( p );
        RetValue = Pdr_ManSolve( pAig, pPars );
        p->pCexSeq = pAig->pSeqModel; pAig->pSeqModel = NULL;
        Aig_ManStop( pAig );                
    }        
    else if ( iEngine == 3 )
    {
        Saig_ParBmc_t Pars, * pPars = &Pars;
        Saig_ParBmcSetDefaultParams( pPars );
        pPars->fUseGlucose = 1;
        pPars->nTimeOut    = nTimeOut;
        pPars->fSilent     = 1;
        Aig_Man_t * pAig = Gia_ManToAigSimple( p );
        RetValue = Saig_ManBmcScalable( pAig, pPars );
        p->pCexSeq = pAig->pSeqModel; pAig->pSeqModel = NULL;
        Aig_ManStop( pAig );                
    }
    else if ( iEngine == 4 )
    {
        Pdr_Par_t Pars, * pPars = &Pars;
        Pdr_ManSetDefaultParams( pPars );
        pPars->fUseAbs  = 1;
        pPars->nTimeOut = nTimeOut;
        pPars->fSilent  = 1;
        Aig_Man_t * pAig = Gia_ManToAigSimple( p );
        RetValue = Pdr_ManSolve( pAig, pPars );
        p->pCexSeq = pAig->pSeqModel; pAig->pSeqModel = NULL;
        Aig_ManStop( pAig );                
    }
    else if ( iEngine == 5 )
    {
        Bmc_AndPar_t Pars, * pPars = &Pars;
        memset( pPars, 0, sizeof(Bmc_AndPar_t) );
        pPars->nProcs        =        1;  // the number of parallel solvers        
        pPars->nFramesAdd    =        1;  // the number of additional frames
        pPars->fNotVerbose   =        1;  // silent
        pPars->nTimeOut      = nTimeOut;  // timeout in seconds
        RetValue = Bmcg_ManPerform( p, pPars );
    }
    else assert( 0 );
    //while ( Abc_Clock() < clkStop );
    if ( fVerbose ) {
        printf( "Engine %d finished and %ssolved the problem.   ", iEngine, RetValue != -1 ? "    " : "not " );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    }
    return RetValue;
}
Gia_Man_t * Cec_GiaScorrOld( Gia_Man_t * p )
{
    Ssw_Pars_t Pars, * pPars = &Pars;
    Ssw_ManSetDefaultParams( pPars );
    Aig_Man_t * pAig  = Gia_ManToAigSimple( p );
    Aig_Man_t * pAig2 = Ssw_SignalCorrespondence( pAig, pPars );
    Gia_Man_t * pGia2 = Gia_ManFromAigSimple( pAig2 );
    Aig_ManStop( pAig2 );  
    Aig_ManStop( pAig );
    return pGia2;     
}
Gia_Man_t * Cec_GiaScorrNew( Gia_Man_t * p )
{
    Cec_ParCor_t Pars, * pPars = &Pars;
    Cec_ManCorSetDefaultParams( pPars );
    pPars->nBTLimit   = 100;
    pPars->nLevelMax  = 100;
    pPars->fVerbose   = 0;
    pPars->fUseCSat   = 1;
    return Cec_ManLSCorrespondence( p, pPars ); 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
#define PAR_THR_MAX 8
typedef struct Par_ThData_t_
{
    Gia_Man_t * p;
    int         iEngine;
    int         fWorking;
    int         nTimeOut;
    int         Result;
    int         fVerbose;
} Par_ThData_t;
void * Cec_GiaProveWorkerThread( void * pArg )
{
    Par_ThData_t * pThData = (Par_ThData_t *)pArg;
    volatile int * pPlace = &pThData->fWorking;
    while ( 1 )
    {
        while ( *pPlace == 0 );
        assert( pThData->fWorking );
        if ( pThData->p == NULL )
        {
            pthread_exit( NULL );
            assert( 0 );
            return NULL;
        }
        pThData->Result = Cec_GiaProveOne( pThData->p, pThData->iEngine, pThData->nTimeOut, pThData->fVerbose );
        pThData->fWorking = 0;
    }
    assert( 0 );
    return NULL;
}
void Cec_GiaInitThreads( Par_ThData_t * ThData, int nProcs, Gia_Man_t * p, int nTimeOut, int fVerbose, pthread_t * WorkerThread )
{
    int i, status;
    assert( nProcs <= PAR_THR_MAX );
    for ( i = 0; i < nProcs; i++ )
    {
        ThData[i].p        = Gia_ManDup(p);
        ThData[i].iEngine  = i;
        ThData[i].nTimeOut = nTimeOut;
        ThData[i].fWorking = 0;
        ThData[i].Result   = -1;
        ThData[i].fVerbose = fVerbose;
        if ( !WorkerThread )
            continue;
        status = pthread_create( WorkerThread + i, NULL,Cec_GiaProveWorkerThread, (void *)(ThData + i) );  assert( status == 0 );
    }
    for ( i = 0; i < nProcs; i++ )
        ThData[i].fWorking = 1;
}
int Cec_GiaWaitThreads( Par_ThData_t * ThData, int nProcs, Gia_Man_t * p, int RetValue, int * pRetEngine )
{
    int i;
    for ( i = 0; i < nProcs; i++ )
    {
        if ( RetValue == -1 && !ThData[i].fWorking && ThData[i].Result != -1 ) {
            RetValue = ThData[i].Result;
            *pRetEngine = i;
            if ( !p->pCexSeq && ThData[i].p->pCexSeq )
                p->pCexSeq = Abc_CexDup( ThData[i].p->pCexSeq, -1 );
        }
        if ( ThData[i].fWorking )
            i = -1;
    }
    return RetValue;
}
    
int Cec_GiaProveTest( Gia_Man_t * p, int nProcs, int nTimeOut, int nTimeOut2, int nTimeOut3, int fVerbose, int fVeryVerbose, int fSilent )
{
    abctime clkScorr = 0, clkTotal = Abc_Clock();
    Par_ThData_t ThData[PAR_THR_MAX];
    pthread_t WorkerThread[PAR_THR_MAX];
    int i, RetValue = -1, RetEngine = -2;
    Abc_CexFreeP( &p->pCexComb );
    Abc_CexFreeP( &p->pCexSeq );        
    if ( !fSilent && fVerbose )
        printf( "Solving verification problem with the following parameters:\n" );
    if ( !fSilent && fVerbose )
        printf( "Processes = %d   TimeOut = %d sec   Verbose = %d.\n", nProcs, nTimeOut, fVerbose );
    fflush( stdout );

    assert( nProcs == 3 || nProcs == 5 );
    Cec_GiaInitThreads( ThData, nProcs, p, nTimeOut, fVerbose, WorkerThread );

    // meanwhile, perform scorr
    Gia_Man_t * pScorr = Cec_GiaScorrNew( p );
    clkScorr = Abc_Clock() - clkTotal;
    if ( Gia_ManAndNum(pScorr) == 0 )
        RetValue = 1, RetEngine = -1;
    
    RetValue = Cec_GiaWaitThreads( ThData, nProcs, p, RetValue, &RetEngine );
    if ( RetValue == -1 )
    {
        abctime clkScorr2, clkStart = Abc_Clock();
        if ( !fSilent && fVerbose ) {
            printf( "Reduced the miter from %d to %d nodes. ", Gia_ManAndNum(p), Gia_ManAndNum(pScorr) );
            Abc_PrintTime( 1, "Time", clkScorr );
        }
        Cec_GiaInitThreads( ThData, nProcs, pScorr, nTimeOut2, fVerbose, NULL );

        // meanwhile, perform scorr
        if ( Gia_ManAndNum(pScorr) < 100000 )
        {
            Gia_Man_t * pScorr2 = Cec_GiaScorrOld( pScorr );
            clkScorr2 = Abc_Clock() - clkStart;
            if ( Gia_ManAndNum(pScorr2) == 0 )
                RetValue = 1;
        
            RetValue = Cec_GiaWaitThreads( ThData, nProcs, p, RetValue, &RetEngine );      
            if ( RetValue == -1 )
            {
                if ( !fSilent && fVerbose ) {
                    printf( "Reduced the miter from %d to %d nodes. ", Gia_ManAndNum(pScorr), Gia_ManAndNum(pScorr2) );
                    Abc_PrintTime( 1, "Time", clkScorr2 );
                }
                Cec_GiaInitThreads( ThData, nProcs, pScorr2, nTimeOut3, fVerbose, NULL );

                RetValue = Cec_GiaWaitThreads( ThData, nProcs, p, RetValue, &RetEngine );
                // do something else      
            }
            Gia_ManStop( pScorr2 );   
        }
    }
    Gia_ManStop( pScorr );    

    // stop threads
    for ( i = 0; i < nProcs; i++ )
    {
        ThData[i].p = NULL;
        ThData[i].fWorking = 1;
    }
    if ( !fSilent )
    {
        printf( "Problem \"%s\" is ", p->pSpec );
        if ( RetValue == 0 )
            printf( "SAT (solved by %d).", RetEngine );
        else if ( RetValue == 1 )
            printf( "UNSAT (solved by %d).", RetEngine );
        else if ( RetValue == -1 )
            printf( "UNDECIDED." );
        else assert( 0 );
        printf( "   " );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clkTotal );
        fflush( stdout );
    }
    return RetValue;
}

#endif // pthreads are used

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

