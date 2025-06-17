/**CFile****************************************************************

  FileName    [utilPth.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Generic interface to pthreads.]

  Synopsis    [Generic interface to pthreads.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 3, 2024.]

  Revision    [$Id: utilPth.c,v 1.00 2024/08/03 00:00:00 alanmi Exp $]

***********************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#ifdef ABC_USE_PTHREADS

#ifdef _WIN32
#include "../lib/pthread.h"
#else
#include <pthread.h>
#endif

#ifdef __cplusplus
#include <atomic>
using namespace std;
#else
#include <stdatomic.h>
#include <stdbool.h>
#endif

#endif

#include "misc/vec/vec.h"

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

#ifndef ABC_USE_PTHREADS

void Util_ProcessThreads( int (*pUserFunc)(void *), void * vData, int nProcs, int TimeOut, int fVerbose )
{
    void * pData; int i;
    Vec_PtrForEachEntry( void *, (Vec_Ptr_t *)vData, pData, i )
        pUserFunc( pData );
}

#else // pthreads are used

#define PAR_THR_MAX 100
typedef struct Util_ThData_t_
{
    void *       pUserData;
    int        (*pUserFunc)(void *);
    int          iThread;
    int          nTimeOut;
    atomic_bool  fWorking;
} Util_ThData_t;

void * Util_Thread( void * pArg )
{
    struct timespec pause_duration;
    pause_duration.tv_sec = 0;
    pause_duration.tv_nsec = 10000000L; // 10 milliseconds

    Util_ThData_t * pThData = (Util_ThData_t *)pArg;
    while ( 1 )
    {
        while ( !atomic_load_explicit((atomic_bool *)&pThData->fWorking, memory_order_acquire) )
            nanosleep(&pause_duration, NULL);
        if ( pThData->pUserData == NULL )
        {
            pthread_exit( NULL );
            assert( 0 );
            return NULL;
        }
        pThData->pUserFunc( pThData->pUserData );
        atomic_store_explicit(&pThData->fWorking, false, memory_order_release);
    }
    assert( 0 );
    return NULL;
}

void Util_ProcessThreads( int (*pUserFunc)(void *), void * vData, int nProcs, int TimeOut, int fVerbose )
{
    //abctime clkStart = Abc_Clock();
    Util_ThData_t ThData[PAR_THR_MAX];
    pthread_t WorkerThread[PAR_THR_MAX];
    Vec_Ptr_t * vStack = NULL;
    int i, status;
    fflush( stdout );
    if ( nProcs <= 2 ) {
        void * pData; int i;
        Vec_PtrForEachEntry( void *, (Vec_Ptr_t *)vData, pData, i )
            pUserFunc( pData );
        return;
    }
    // subtract manager thread
    nProcs--;
    assert( nProcs >= 1 && nProcs <= PAR_THR_MAX );
    // start threads
    for ( i = 0; i < nProcs; i++ )
    {
        ThData[i].pUserData = NULL;
        ThData[i].pUserFunc = pUserFunc;
        ThData[i].iThread   = i;
        ThData[i].nTimeOut  = TimeOut;
        atomic_store_explicit(&ThData[i].fWorking, false, memory_order_release);
        status = pthread_create( WorkerThread + i, NULL, Util_Thread, (void *)(ThData + i) );  assert( status == 0 );
    }

    struct timespec pause_duration;
    pause_duration.tv_sec = 0;
    pause_duration.tv_nsec = 10000000L; // 10 milliseconds

    // look at the threads
    vStack = Vec_PtrDup( (Vec_Ptr_t *)vData );
    while ( Vec_PtrSize(vStack) > 0 )
    {
        for ( i = 0; i < nProcs; i++ )
        {
            if ( atomic_load_explicit(&ThData[i].fWorking, memory_order_acquire) )
                continue;
            ThData[i].pUserData = Vec_PtrPop( vStack );
            atomic_store_explicit(&ThData[i].fWorking, true, memory_order_release);
            break;
        }
    }
    Vec_PtrFree( vStack );    
    
    // wait till threads finish
    for ( i = 0; i < nProcs; i++ )
    {
        if ( atomic_load_explicit(&ThData[i].fWorking, memory_order_acquire) )
            i = -1; // Start from the beginning again
        nanosleep(&pause_duration, NULL);
    }

    // stop threads
    for ( i = 0; i < nProcs; i++ )
    {
        ThData[i].pUserData = NULL;
        atomic_store_explicit(&ThData[i].fWorking, true, memory_order_release);
    }

    // Join threads
    for ( i = 0; i < nProcs; i++ )
        pthread_join( WorkerThread[i], NULL );

    //if ( fVerbose )
    //    Abc_PrintTime( 1, "Time", Abc_Clock() - clkStart );    
}

#endif // pthreads are used


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

