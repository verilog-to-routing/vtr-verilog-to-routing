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

#ifndef ABC_USE_PTHREADS

int Cec_GiaProveTest( Gia_Man_t * p, int nProcs, int nTimeOut, int fVerbose, int fVeryVerbose, int fSilent ) { return -1; }

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
int Cec_GiaProveOne( Gia_Man_t * p, int iEngine, int nTimeOut, Abc_Cex_t ** ppCex )
{
    abctime clk = Abc_Clock();
    //abctime clkStop = nTimeOut * CLOCKS_PER_SEC + Abc_Clock();
    printf( "Calling engine %d with timeout %d sec.\n", iEngine, nTimeOut );
    if ( iEngine == 0 )
    {
    }
    else if ( iEngine == 1 )
    {
    }    
    else if ( iEngine == 2 )
    {
    }    
    else if ( iEngine == 3 )
    {
    }
    else assert( 0 );
    //while ( Abc_Clock() < clkStop );
    printf( "Engine %d finished.   ", iEngine );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );    
    return 0;
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
    Abc_Cex_t * pCex;
    int         iEngine;
    int         fWorking;
    int         nTimeOut;
    int         Result;
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
        pThData->Result = Cec_GiaProveOne( pThData->p, pThData->iEngine, pThData->nTimeOut, &pThData->pCex );
        pThData->fWorking = 0;
    }
    assert( 0 );
    return NULL;
}
int Cec_GiaProveTest( Gia_Man_t * p, int nProcs, int nTimeOut, int fVerbose, int fVeryVerbose, int fSilent )
{
    abctime clkTotal = Abc_Clock();
    Par_ThData_t ThData[PAR_THR_MAX];
    pthread_t WorkerThread[PAR_THR_MAX];
    int i, status, RetValue = -1;
    Abc_CexFreeP( &p->pCexComb );
    Abc_CexFreeP( &p->pCexSeq );        
    if ( fVerbose )
        printf( "Solving verification problem with the following parameters:\n" );
    if ( fVerbose )
        printf( "Processes = %d   TimeOut = %d sec   Verbose = %d.\n", nProcs, nTimeOut, fVerbose );
    fflush( stdout );
    if ( nProcs == 1 )
        return -1;
    // subtract manager thread
    nProcs--;
    assert( (nProcs == 4 || nProcs == 8) && nProcs <= PAR_THR_MAX );
    // start threads
    for ( i = 0; i < nProcs; i++ )
    {
        ThData[i].p        = Gia_ManDup(p);
        ThData[i].pCex     = NULL;
        ThData[i].iEngine  = i;
        ThData[i].nTimeOut = nTimeOut;
        ThData[i].fWorking = 0;
        ThData[i].Result   = -1;
        status = pthread_create( WorkerThread + i, NULL,Cec_GiaProveWorkerThread, (void *)(ThData + i) );  assert( status == 0 );
    }

    for ( i = 0; i < nProcs; i++ )
        ThData[i].fWorking = 1;
    
    // wait till threads finish
    for ( i = 0; i < nProcs; i++ )
        if ( ThData[i].fWorking )
            i = -1;

    // stop threads
    for ( i = 0; i < nProcs; i++ )
    {
        assert( !ThData[i].fWorking );
        // cleanup
        Gia_ManStopP( &ThData[i].p );        
        if ( !p->pCexSeq && ThData[i].pCex )
            p->pCexSeq = Abc_CexDup( ThData[i].pCex, -1 );
        Abc_CexFreeP( &ThData[i].pCex );
        // stop
        ThData[i].p = NULL;
        ThData[i].fWorking = 1;
    }
    if ( !fSilent )
    {
        if ( RetValue == 0 )
            printf( "Problem is SAT " );
        else if ( RetValue == 1 )
            printf( "Problem is UNSAT " );
        else if ( RetValue == -1 )
            printf( "Problem is UNDECIDED " );
        else assert( 0 );
        printf( ".  " );
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

