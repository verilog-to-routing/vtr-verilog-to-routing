/**CFile****************************************************************

  FileName    [ifTest.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [FPGA mapping based on priority cuts.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 21, 2006.]

  Revision    [$Id: ifTest.c,v 1.00 2006/11/21 00:00:00 alanmi Exp $]

***********************************************************************/

#include "if.h"
#include "aig/gia/gia.h"

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

// do nothing

#else // pthreads are used

static inline word * Gia_ParTestObj( Gia_Man_t * p, int Id )         { return (word *)p->pData + Id * p->iData; }
static inline void   Gia_ParTestAlloc( Gia_Man_t * p, int nWords )   { assert( !p->pData ); p->pData = (unsigned *)ABC_ALLOC(word, Gia_ManObjNum(p) * nWords); p->iData = nWords; }
static inline void   Gia_ParTestFree( Gia_Man_t * p )                { ABC_FREE( p->pData ); p->iData = 0; }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
 
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ParComputeSignature( Gia_Man_t * p, int nWords )
{
    Gia_Obj_t * pObj;
    word * pData, Sign = 0;
    int i, k;
    Gia_ManForEachCo( p, pObj, k )
    {
        pData = Gia_ParTestObj( p, Gia_ObjId(p, pObj) );
        for ( i = 0; i < p->iData; i++ )
            Sign ^= pData[i];
    }
    Abc_TtPrintHexRev( stdout, &Sign, 6 );
}
 
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ParTestSimulateInit( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    word * pData;
    int i, k;
    Gia_ManForEachCi( p, pObj, k )
    {
        pData = Gia_ParTestObj( p, Gia_ObjId(p, pObj) );
        for ( i = 0; i < p->iData; i++ )
            pData[i] = Gia_ManRandomW( 0 );
    }
}
void Gia_ParTestSimulateObj( Gia_Man_t * p, int Id )
{
    Gia_Obj_t * pObj = Gia_ManObj( p, Id );
    word * pData, * pData0, * pData1;
    int i;
    if ( Gia_ObjIsAnd(pObj) )
    {
        pData  = Gia_ParTestObj( p, Id );
        pData0 = Gia_ParTestObj( p, Gia_ObjFaninId0(pObj, Id) );
        pData1 = Gia_ParTestObj( p, Gia_ObjFaninId1(pObj, Id) );
        if ( Gia_ObjFaninC0(pObj) )
        {
            if ( Gia_ObjFaninC1(pObj) )
                for ( i = 0; i < p->iData; i++ )
                    pData[i] = ~(pData0[i] | pData1[i]);
            else 
                for ( i = 0; i < p->iData; i++ )
                    pData[i] = ~pData0[i] & pData1[i];
        }
        else 
        {
            if ( Gia_ObjFaninC1(pObj) )
                for ( i = 0; i < p->iData; i++ )
                    pData[i] = pData0[i] & ~pData1[i];
            else 
                for ( i = 0; i < p->iData; i++ )
                    pData[i] = pData0[i] & pData1[i];
        }
    }
    else if ( Gia_ObjIsCo(pObj) )
    {
        pData  = Gia_ParTestObj( p, Id );
        pData0 = Gia_ParTestObj( p, Gia_ObjFaninId0(pObj, Id) );
        if ( Gia_ObjFaninC0(pObj) )
            for ( i = 0; i < p->iData; i++ )
                pData[i] = ~pData0[i];
        else 
            for ( i = 0; i < p->iData; i++ )
                pData[i] = pData0[i];
    }
    else if ( Gia_ObjIsCi(pObj) )
    {
    }
    else if ( Gia_ObjIsConst0(pObj) )
    {
        pData = Gia_ParTestObj( p, Id );
        for ( i = 0; i < p->iData; i++ )
            pData[i] = 0;
    }
    else assert( 0 );
}
void Gia_ParTestSimulate( Gia_Man_t * p, int nWords )
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManRandom( 1 );
    Gia_ParTestAlloc( p, nWords );
    Gia_ParTestSimulateInit( p );
    Gia_ManForEachObj( p, pObj, i )
        Gia_ParTestSimulateObj( p, i );
//    Gia_ParComputeSignature( p, nWords ); printf( "   " );
    Gia_ParTestFree( p );
}
  

/**Function*************************************************************

  Synopsis    [Assigns references.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManCreateFaninCounts( Gia_Man_t * p )  
{
    Vec_Int_t * vCounts;
    Gia_Obj_t * pObj; int i;
    vCounts = Vec_IntAlloc( Gia_ManObjNum(p) );
    Gia_ManForEachObj( p, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) )
            Vec_IntPush( vCounts, 2 );
        else if ( Gia_ObjIsCo(pObj) )
            Vec_IntPush( vCounts, 1 );
        else
            Vec_IntPush( vCounts, 0 );
    }
    assert( Vec_IntSize(vCounts) == Gia_ManObjNum(p) );
    return vCounts;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
#define PAR_THR_MAX 100
typedef struct Par_ThData_t_
{
    Gia_Man_t * p;
    int         Id;
    int         Status;
} Par_ThData_t;
void * Gia_ParWorkerThread( void * pArg )
{
    Par_ThData_t * pThData = (Par_ThData_t *)pArg;
    volatile int * pPlace = &pThData->Status;
    while ( 1 )
    {
        while ( *pPlace == 0 );
        assert( pThData->Status == 1 );
        if ( pThData->Id == -1 )
        {
	        pthread_exit( NULL );
            assert( 0 );
            return NULL;
        }
        assert( pThData->Id >= 0 );
        Gia_ParTestSimulateObj( pThData->p, pThData->Id );
        pThData->Status = 0;
    }
	assert( 0 );
	return NULL;
}
void Gia_ParTestSimulate2( Gia_Man_t * p, int nWords, int nProcs )
{
	pthread_t WorkerThread[PAR_THR_MAX];
    Par_ThData_t ThData[PAR_THR_MAX];
    Vec_Int_t * vStack, * vFanins;
    int i, k, iFan, status, nCountFanins;
    assert( nProcs <= PAR_THR_MAX );
    Gia_ManRandom( 1 );
    Gia_ParTestAlloc( p, nWords );
    Gia_ParTestSimulateInit( p );
    // start the stack
    vStack = Vec_IntAlloc( 1000 );
    Vec_IntForEachEntryReverse( p->vCis, iFan, i )
        Vec_IntPush( vStack, iFan );
    Vec_IntPush( vStack, 0 );
    Gia_ManStaticFanoutStart( p );
    vFanins = Gia_ManCreateFaninCounts( p );
    nCountFanins = Vec_IntSum(vFanins);
    // start the threads
    for ( i = 0; i < nProcs; i++ )
    {
        ThData[i].p = p;
        ThData[i].Id = -1;
        ThData[i].Status = 0;
        status = pthread_create( WorkerThread + i, NULL, Gia_ParWorkerThread, (void *)(ThData + i) );  assert( status == 0 );
    }
    while ( nCountFanins > 0 || Vec_IntSize(vStack) > 0 )
    {
        for ( i = 0; i < nProcs; i++ )
        {
            if ( ThData[i].Status )
                continue;
            assert( ThData[i].Status == 0 );
            if ( ThData[i].Id >= 0 )
            {
                Gia_ObjForEachFanoutStaticId( p, ThData[i].Id, iFan, k )
                {
                    assert( Vec_IntEntry(vFanins, iFan) > 0 );
                    if ( Vec_IntAddToEntry(vFanins, iFan, -1) == 0 )
                        Vec_IntPush( vStack, iFan );
                    assert( nCountFanins > 0 );
                    nCountFanins--;
                }
                ThData[i].Id = -1;
            }
            if ( Vec_IntSize(vStack) > 0 )
            {
                ThData[i].Id = Vec_IntPop( vStack );
                ThData[i].Status = 1;
            }
        }
    }
    Vec_IntForEachEntry( vFanins, iFan, k )
        if ( iFan != 0 )
        {
            printf( "%d -> %d    ", k, iFan );
            Gia_ObjPrint( p, Gia_ManObj(p, k) );
        }
//    assert( Vec_IntSum(vFanins) == 0 );
    // stop the threads
    while ( 1 )
    {
        for ( i = 0; i < nProcs; i++ )
            if ( ThData[i].Status )
                break;
        if ( i == nProcs )
            break;
    }
    for ( i = 0; i < nProcs; i++ )
    {
        assert( ThData[i].Status == 0 );
        ThData[i].Id = -1;
        ThData[i].Status = 1;
    }
    Gia_ManStaticFanoutStop( p );
    Vec_IntFree( vStack );
    Vec_IntFree( vFanins );
//    Gia_ParComputeSignature( p, nWords ); printf( "   " );
    Gia_ParTestFree( p );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ParTest( Gia_Man_t * p, int nWords, int nProcs )
{
    abctime clk;
    printf( "Trying with %d words and %d threads.  ", nWords, nProcs );
    printf( "Memory usage = %.2f MB\n", (8.0*nWords*Gia_ManObjNum(p))/(1<<20) );
    
    clk = Abc_Clock();
    Gia_ParTestSimulate( p, nWords );
    Abc_PrintTime( 1, "Regular time", Abc_Clock() - clk );

    clk = Abc_Clock();
    Gia_ParTestSimulate2( p, nWords, nProcs );
    Abc_PrintTime( 1, "Special time", Abc_Clock() - clk );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif // pthreads are used


ABC_NAMESPACE_IMPL_END

