/**CFile****************************************************************

  FileName    [absPth.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Abstraction package.]

  Synopsis    [Interface to pthreads.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: absPth.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#include "abs.h"
#include "proof/pdr/pdr.h"
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

#ifndef ABC_USE_PTHREADS

void Gia_GlaProveAbsracted( Gia_Man_t * p, int fSimpProver, int fVerbose ) {}
void Gia_GlaProveCancel( int fVerbose )                                    {}
int  Gia_GlaProveCheck( int fVerbose )                                     { return 0; }

#else // pthreads are used

// information given to the thread
typedef struct Abs_ThData_t_
{
    Aig_Man_t * pAig;
    int         fVerbose;
    int         RunId;
} Abs_ThData_t;

// mutext to control access to shared variables
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile int g_nRunIds = 0;             // the number of the last prover instance
static volatile int g_fAbstractionProved = 0;  // set to 1 when prover successed to prove

// call back procedure for PDR
int Abs_CallBackToStop( int RunId ) { assert( RunId <= g_nRunIds ); return RunId < g_nRunIds; }

// test procedure to replace PDR
int Pdr_ManSolve_test( Aig_Man_t * pAig, Pdr_Par_t * pPars, Abc_Cex_t ** ppCex )
{
    char * p = ABC_ALLOC( char, 111 );
    while ( 1 )
    {
        if ( pPars->pFuncStop && pPars->pFuncStop(pPars->RunId) )
            break;
    }
    ABC_FREE( p );
    return -1;
}

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Create one thread]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Abs_ProverThread( void * pArg )
{
    Abs_ThData_t * pThData = (Abs_ThData_t *)pArg;
    Pdr_Par_t Pars, * pPars = &Pars;
    int RetValue, status;
    // call PDR
    Pdr_ManSetDefaultParams( pPars );
    pPars->fSilent   = 1;
    pPars->RunId     = pThData->RunId;
    pPars->pFuncStop = Abs_CallBackToStop;
    RetValue = Pdr_ManSolve( pThData->pAig, pPars );
    // update the result
    if ( RetValue == 1 )
    {
        status = pthread_mutex_lock(&g_mutex);  assert( status == 0 );
        g_fAbstractionProved = 1;
        status = pthread_mutex_unlock(&g_mutex);  assert( status == 0 );
    }
    // quit this thread
    if ( pThData->fVerbose )
    {
        if ( RetValue == 1 )
            Abc_Print( 1, "Proved abstraction %d.\n", pThData->RunId );
        else if ( RetValue == 0 )
            Abc_Print( 1, "Disproved abstraction %d.\n", pThData->RunId );
        else if ( RetValue == -1 )
            Abc_Print( 1, "Cancelled abstraction %d.\n", pThData->RunId );
        else assert( 0 );
    }
    // free memory
    Aig_ManStop( pThData->pAig );
    ABC_FREE( pThData );
    // quit this thread
    pthread_exit( NULL );
    assert(0);
    return NULL;
}
void Gia_GlaProveAbsracted( Gia_Man_t * pGia, int fSimpProver, int fVerbose )
{
    extern Aig_Man_t * Dar_ManRwsat( Aig_Man_t * pAig, int fBalance, int fVerbose );
    Abs_ThData_t * pThData;
    Ssw_Pars_t Pars, * pPars = &Pars;
    Aig_Man_t * pAig, * pTemp;
    Gia_Man_t * pAbs;
    pthread_t ProverThread;
    int status;
    // disable verbosity
//    fVerbose = 0;
    // create abstraction 
    assert( pGia->vGateClasses != NULL );
    pAbs = Gia_ManDupAbsGates( pGia, pGia->vGateClasses );
    Gia_ManCleanValue( pGia );
    pAig = Gia_ManToAigSimple( pAbs );
    Gia_ManStop( pAbs );
    // simplify abstraction
    if ( fSimpProver )
    {
        Ssw_ManSetDefaultParams( pPars );
        pPars->nFramesK = 4;
        pAig = Ssw_SignalCorrespondence( pTemp = pAig, pPars );
//printf( "\n" );
//Aig_ManPrintStats( pTemp );
//Aig_ManPrintStats( pAig );
        Aig_ManStop( pTemp );
    }
    // synthesize abstraction
//    pAig = Dar_ManRwsat( pTemp = pAig, 0, 0 ); 
//    Aig_ManStop( pTemp );
    // reset the proof 
    status = pthread_mutex_lock(&g_mutex);  assert( status == 0 );
    g_fAbstractionProved = 0;
    status = pthread_mutex_unlock(&g_mutex);  assert( status == 0 );
    // collect thread data
    pThData = ABC_CALLOC( Abs_ThData_t, 1 );
    pThData->pAig = pAig;
    pThData->fVerbose = fVerbose;
    status = pthread_mutex_lock(&g_mutex);  assert( status == 0 );
    pThData->RunId = ++g_nRunIds;
    status = pthread_mutex_unlock(&g_mutex);  assert( status == 0 );
    // create thread
    if ( fVerbose )  Abc_Print( 1, "\nTrying to prove abstraction %d.\n", pThData->RunId );
    status = pthread_create( &ProverThread, NULL, Abs_ProverThread, pThData );
    assert( status == 0 );
}
void Gia_GlaProveCancel( int fVerbose )
{
    int status;
    status = pthread_mutex_lock(&g_mutex);  assert( status == 0 );
    g_nRunIds++;
    status = pthread_mutex_unlock(&g_mutex);  assert( status == 0 );
}
int Gia_GlaProveCheck( int fVerbose )
{
    int status;
    if ( g_fAbstractionProved == 0 )
        return 0;
    status = pthread_mutex_lock(&g_mutex);  assert( status == 0 );
    g_fAbstractionProved = 0;
    status = pthread_mutex_unlock(&g_mutex);  assert( status == 0 );
    return 1;
}

#endif // pthreads are used

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

