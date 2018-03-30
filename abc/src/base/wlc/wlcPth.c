/**CFile****************************************************************

  FileName    [wlcPth.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Verilog parser.]

  Synopsis    [Abstraction for word-level networks.]

  Author      [Yen-Sheng Ho, Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: wlcPth.c $]

***********************************************************************/

#include "wlc.h"
#include "sat/bmc/bmc.h"

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

extern Abc_Ntk_t *   Abc_NtkFromAigPhase( Aig_Man_t * pAig );
extern int           Abc_NtkDarBmc3( Abc_Ntk_t * pAbcNtk, Saig_ParBmc_t * pBmcPars, int fOrDecomp );
extern int           Wla_ManShrinkAbs( Wla_Man_t * pWla, int nFrames, int RunId );

static volatile int  g_nRunIds = 0;             // the number of the last prover instance
int Wla_CallBackToStop( int RunId ) { assert( RunId <= g_nRunIds ); return RunId < g_nRunIds; }
int Wla_GetGlobalRunId() { return g_nRunIds; }

#ifndef ABC_USE_PTHREADS

void Wla_ManJoinThread( Wla_Man_t * pWla, int RunId ) {}
void Wla_ManConcurrentBmc3( Wla_Man_t * pWla, Aig_Man_t * pAig, Abc_Cex_t ** ppCex ) {}

#else // pthreads are used

// information given to the thread
typedef struct Bmc3_ThData_t_
{
    Wla_Man_t *  pWla;
    Aig_Man_t *  pAig;
    Abc_Cex_t ** ppCex;
    int          RunId;
    int          fVerbose;
} Bmc3_ThData_t;

// mutext to control access to shared variables
extern pthread_mutex_t g_mutex;

void Wla_ManJoinThread( Wla_Man_t * pWla, int RunId )
{
    int status;
    if ( RunId == g_nRunIds )
    {
        status = pthread_mutex_lock(&g_mutex);  assert( status == 0 );
        ++g_nRunIds;
        status = pthread_mutex_unlock(&g_mutex);  assert( status == 0 );
    }

    status = pthread_join( *(pthread_t *)(pWla->pThread), NULL );
    assert( status == 0 );
    ABC_FREE( pWla->pThread );
    pWla->pThread = NULL;
}
    
void * Wla_Bmc3Thread ( void * pArg )
{
    int status;
    int RetValue = -1;
    int nFramesNoChangeLim = 10;
    Bmc3_ThData_t * pData = (Bmc3_ThData_t *)pArg;
    Abc_Ntk_t * pAbcNtk = Abc_NtkFromAigPhase( pData->pAig );
    Saig_ParBmc_t BmcPars, *pBmcPars = &BmcPars;
    Saig_ParBmcSetDefaultParams( pBmcPars );
    pBmcPars->pFuncStop = Wla_CallBackToStop;
    pBmcPars->RunId = pData->RunId;

    if ( pData->pWla->pPars->fShrinkAbs )
        pBmcPars->nFramesMax = pData->pWla->iCexFrame + nFramesNoChangeLim;

    RetValue = Abc_NtkDarBmc3( pAbcNtk, pBmcPars, 0 );

    if ( RetValue == 0 )
    {
        assert( pAbcNtk->pSeqModel );
        *(pData->ppCex) = pAbcNtk->pSeqModel;
        pAbcNtk->pSeqModel = NULL;

        if ( pData->fVerbose )
            Abc_Print( 1, "Bmc3 found CEX. RunId=%d.\n", pData->RunId );

        status = pthread_mutex_lock(&g_mutex);  assert( status == 0 );
        ++g_nRunIds;
        status = pthread_mutex_unlock(&g_mutex);  assert( status == 0 );
    }
    else if ( RetValue == -1 )
    {
        if ( pData->RunId < g_nRunIds && pData->fVerbose )
            Abc_Print( 1, "Bmc3 was cancelled. RunId=%d.\n", pData->RunId );

        if ( pData->pWla->nIters > 1 && pData->RunId == g_nRunIds )
        {
            RetValue = Wla_ManShrinkAbs( pData->pWla, pData->pWla->iCexFrame + nFramesNoChangeLim, pData->RunId );
            pData->pWla->iCexFrame += nFramesNoChangeLim; 
            
            if ( RetValue == 1 )
            {
                pData->pWla->fNewAbs = 1;
                status = pthread_mutex_lock(&g_mutex);  assert( status == 0 );
                ++g_nRunIds;
                status = pthread_mutex_unlock(&g_mutex);  assert( status == 0 );
            }
        }
    }

    // free memory
    Abc_NtkDelete( pAbcNtk );
    Aig_ManStop( pData->pAig );
    ABC_FREE( pData );

    // quit this thread
    pthread_exit( NULL );
    assert(0);
    return NULL;
}

void Wla_ManConcurrentBmc3( Wla_Man_t * pWla, Aig_Man_t * pAig, Abc_Cex_t ** ppCex )
{
    int status;
    Bmc3_ThData_t * pData;

    assert( pWla->pThread == NULL );
    pWla->pThread = (void *)ABC_CALLOC( pthread_t, 1 );

    pData = ABC_CALLOC( Bmc3_ThData_t, 1 );
    pData->pWla = pWla;
    pData->pAig = pAig;
    pData->ppCex = ppCex;
    pData->RunId = g_nRunIds;
    pData->fVerbose = pWla->pPars->fVerbose;

    status = pthread_create( (pthread_t *)pWla->pThread, NULL, Wla_Bmc3Thread, pData );
    assert( status == 0 );
}

#endif // pthreads are used

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

