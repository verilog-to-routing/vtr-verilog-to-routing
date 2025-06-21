/**CFile****************************************************************

  FileName    [sswPart.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Inductive prover with constraints.]

  Synopsis    [Partitioned signal correspondence.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 1, 2008.]

  Revision    [$Id: sswPart.c,v 1.00 2008/09/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sswInt.h"
#include "aig/ioa/ioa.h"
#include "aig/gia/giaAig.h"
#include "proof/cec/cec.h"

#ifdef ABC_USE_PTHREADS

#ifdef _WIN32
#include "../lib/pthread.h"
#else
#include <pthread.h>
#include <unistd.h>
#endif

#ifdef __cplusplus
#include <atomic>
using namespace std;
#else
#include <stdatomic.h>
#include <stdbool.h>
#endif

#endif


ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performing SAT sweeping for the array of AIGs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_SignalCorrespondenceArray1( Vec_Ptr_t * vGias, Ssw_Pars_t * pPars )
{
    Gia_Man_t * pGia; int i;
    Cec_ParCor_t CorPars, * pCorPars = &CorPars;
    Cec_ManCorSetDefaultParams( pCorPars );
    pCorPars->nBTLimit  = pPars->nBTLimit;
    pCorPars->fVerbose  = pPars->fVerbose;
    pCorPars->fUseCSat  = 1; 
    Vec_PtrForEachEntry( Gia_Man_t *, vGias, pGia, i )
        if ( Gia_ManPiNum(pGia) > 0 )
            Cec_ManLSCorrespondenceClasses( pGia, pCorPars );
}

/**Function*************************************************************

  Synopsis    [Performing SAT sweeping for the array of AIGs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
#ifndef ABC_USE_PTHREADS

void Ssw_SignalCorrespondenceArray( Vec_Ptr_t * vGias, Ssw_Pars_t * pPars )
{
    Ssw_SignalCorrespondenceArray1( vGias, pPars );
}

#else // pthreads are used


#define PAR_THR_MAX 100
typedef struct Par_ScorrThData_t_
{
    Cec_ParCor_t CorPars;
    Gia_Man_t *  p;
    int *        pMap;
    int          iThread;
    int          nTimeOut;
    atomic_bool  fWorking;
} Par_ScorrThData_t;

void * Ssw_GiaWorkerThread( void * pArg )
{
    struct timespec pause_duration;
    pause_duration.tv_sec = 0;
    pause_duration.tv_nsec = 10000000L; // 10 milliseconds

    Par_ScorrThData_t * pThData = (Par_ScorrThData_t *)pArg;
    while ( 1 )
    {
        while ( !atomic_load_explicit((atomic_bool *)&pThData->fWorking, memory_order_acquire) )
            nanosleep(&pause_duration, NULL);
        if ( pThData->p == NULL )
        {
            pthread_exit( NULL );
            assert( 0 );
            return NULL;
        }
        Cec_ManLSCorrespondenceClasses( pThData->p, &pThData->CorPars );
        atomic_store_explicit(&pThData->fWorking, false, memory_order_release);
    }
    assert( 0 );
    return NULL;
}

void Ssw_SignalCorrespondenceArray( Vec_Ptr_t * vGias, Ssw_Pars_t * pPars )
{
    //abctime clkTotal = Abc_Clock();
    Par_ScorrThData_t ThData[PAR_THR_MAX];
    pthread_t WorkerThread[PAR_THR_MAX];
    int i, status, nProcs = pPars->nProcs;
    Vec_Ptr_t * vStack;
    Cec_ParCor_t CorPars, * pCorPars = &CorPars;
    Cec_ManCorSetDefaultParams( pCorPars );
    if ( pPars->fVerbose )
        printf( "Running concurrent &scorr with %d processes.\n", nProcs );
    fflush( stdout );
    if ( pPars->nProcs < 2 )
        return Ssw_SignalCorrespondenceArray1( vGias, pPars );
    // subtract manager thread
    nProcs--;
    assert( nProcs >= 1 && nProcs <= PAR_THR_MAX );
    // start threads
    for ( i = 0; i < nProcs; i++ )
    {
        ThData[i].CorPars  = *pCorPars;
        ThData[i].iThread  = i;
        atomic_store_explicit(&ThData[i].fWorking, false, memory_order_release);
        status = pthread_create( WorkerThread + i, NULL, Ssw_GiaWorkerThread, (void *)(ThData + i) );  assert( status == 0 );
    }

    struct timespec pause_duration;
    pause_duration.tv_sec = 0;
    pause_duration.tv_nsec = 10000000L; // 10 milliseconds

    // look at the threads
    vStack = Vec_PtrDup( vGias );
    while ( Vec_PtrSize(vStack) > 0 )
    {
        for ( i = 0; i < nProcs; i++ )
        {
            if ( atomic_load_explicit(&ThData[i].fWorking, memory_order_acquire) )
                continue;
            ThData[i].p = (Gia_Man_t*)Vec_PtrPop( vStack );
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
        ThData[i].p = NULL;
        atomic_store_explicit(&ThData[i].fWorking, true, memory_order_release);
    }

    // Join threads
    for ( i = 0; i < nProcs; i++ )
        pthread_join( WorkerThread[i], NULL );
}

#endif // pthreads are used


/**Function*************************************************************

  Synopsis    [Performs partitioned sequential SAT sweeping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Ssw_SignalCorrespondencePart( Aig_Man_t * pAig, Ssw_Pars_t * pPars )
{
    int fPrintParts = 1;
    char Buffer[100];
    Aig_Man_t * pTemp, * pNew;
    Vec_Ptr_t * vResult;
    Vec_Int_t * vPart;
    int * pMapBack;
    int i, nCountPis, nCountRegs;
    int nClasses, nPartSize, fVerbose;
    abctime clk = Abc_Clock();
    if ( pPars->fConstrs )
    {
        Abc_Print( 1, "Cannot use partitioned computation with constraints.\n" );
        return NULL;
    }
    // save parameters
    nPartSize = pPars->nPartSize; pPars->nPartSize = 0;
    fVerbose  = pPars->fVerbose;  pPars->fVerbose  = 0;
    // generate partitions
    if ( pAig->vClockDoms )
    {
        // divide large clock domains into separate partitions
        vResult = Vec_PtrAlloc( 100 );
        Vec_PtrForEachEntry( Vec_Int_t *, (Vec_Ptr_t *)pAig->vClockDoms, vPart, i )
        {
            if ( nPartSize && Vec_IntSize(vPart) > nPartSize )
                Aig_ManPartDivide( vResult, vPart, nPartSize, pPars->nOverSize );
            else
                Vec_PtrPush( vResult, Vec_IntDup(vPart) );
        }
    }
    else
        vResult = Aig_ManRegPartitionSimple( pAig, nPartSize, pPars->nOverSize );
//    vResult = Aig_ManPartitionSmartRegisters( pAig, nPartSize, 0 ); 
//    vResult = Aig_ManRegPartitionSmart( pAig, nPartSize );
    if ( fPrintParts )
    {
        // print partitions
        Abc_Print( 1, "Simple partitioning. %d partitions are saved:\n", Vec_PtrSize(vResult) );
        Vec_PtrForEachEntry( Vec_Int_t *, vResult, vPart, i )
        {
//            extern void Ioa_WriteAiger( Aig_Man_t * pMan, char * pFileName, int fWriteSymbols, int fCompact );
            sprintf( Buffer, "part%03d.aig", i );
            pTemp = Aig_ManRegCreatePart( pAig, vPart, &nCountPis, &nCountRegs, NULL );
            Ioa_WriteAiger( pTemp, Buffer, 0, 0 );
            Abc_Print( 1, "part%03d.aig : Reg = %4d. PI = %4d. (True = %4d. Regs = %4d.) And = %5d.\n",
                i, Vec_IntSize(vPart), Aig_ManCiNum(pTemp)-Vec_IntSize(vPart), nCountPis, nCountRegs, Aig_ManNodeNum(pTemp) );
            Aig_ManStop( pTemp );
        }
    }

    // perform SSW with partitions
    Aig_ManReprStart( pAig, Aig_ManObjNumMax(pAig) );
    Vec_PtrForEachEntry( Vec_Int_t *, vResult, vPart, i )
    {
        pTemp = Aig_ManRegCreatePart( pAig, vPart, &nCountPis, &nCountRegs, &pMapBack );
        Aig_ManSetRegNum( pTemp, pTemp->nRegs );
        // create the projection of 1-hot registers
        if ( pAig->vOnehots )
            pTemp->vOnehots = Aig_ManRegProjectOnehots( pAig, pTemp, pAig->vOnehots, fVerbose );
        // run SSW
        if (nCountPis>0) {
            pNew = Ssw_SignalCorrespondence( pTemp, pPars );
            nClasses = Aig_TransferMappedClasses( pAig, pTemp, pMapBack );
            if ( fVerbose )
                Abc_Print( 1, "%3d : Reg = %4d. PI = %4d. (True = %4d. Regs = %4d.) And = %5d. It = %3d. Cl = %5d.\n",
                    i, Vec_IntSize(vPart), Aig_ManCiNum(pTemp)-Vec_IntSize(vPart), nCountPis, nCountRegs, Aig_ManNodeNum(pTemp), pPars->nIters, nClasses );
            Aig_ManStop( pNew );
        }
        Aig_ManStop( pTemp );
        ABC_FREE( pMapBack );
    }
    // remap the AIG
    pNew = Aig_ManDupRepr( pAig, 0 );
    Aig_ManSeqCleanup( pNew );
//    Aig_ManPrintStats( pAig );
//    Aig_ManPrintStats( pNew );
    Vec_VecFree( (Vec_Vec_t *)vResult );
    pPars->nPartSize = nPartSize;
    pPars->fVerbose = fVerbose;
    if ( fVerbose )
    {
        ABC_PRT( "Total time", Abc_Clock() - clk );
    }
    return pNew;
}


/**Function*************************************************************

  Synopsis    [Performs partitioned sequential SAT sweeping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Ssw_SignalCorrespondencePart2( Aig_Man_t * pAig, Ssw_Pars_t * pPars )
{
    int fPrintParts = 1;
    //char Buffer[100];
    Aig_Man_t * pTemp, * pNew;
    Vec_Ptr_t * vAigs;    
    Vec_Ptr_t * vGias;
    Vec_Ptr_t * vMaps;
    Vec_Ptr_t * vResult;
    Vec_Int_t * vPart;
    int * pMapBack = NULL;
    int i, nCountPis, nCountRegs;
    int nClasses, nPartSize, fVerbose;
    abctime clk = Abc_Clock();
    if ( pPars->fConstrs )
    {
        Abc_Print( 1, "Cannot use partitioned computation with constraints.\n" );
        return NULL;
    }
    // save parameters
    nPartSize = pPars->nPartSize; pPars->nPartSize = 0;
    fVerbose  = pPars->fVerbose;  pPars->fVerbose  = 0;
    // generate partitions
    if ( pAig->vClockDoms )
    {
        // divide large clock domains into separate partitions
        vResult = Vec_PtrAlloc( 100 );
        Vec_PtrForEachEntry( Vec_Int_t *, (Vec_Ptr_t *)pAig->vClockDoms, vPart, i )
        {
            if ( nPartSize && Vec_IntSize(vPart) > nPartSize )
                Aig_ManPartDivide( vResult, vPart, nPartSize, pPars->nOverSize );
            else
                Vec_PtrPush( vResult, Vec_IntDup(vPart) );
        }
    }
    else
        vResult = Aig_ManRegPartitionSimple( pAig, nPartSize, pPars->nOverSize );
//    vResult = Aig_ManPartitionSmartRegisters( pAig, nPartSize, 0 ); 
//    vResult = Aig_ManRegPartitionSmart( pAig, nPartSize );
    // collect partitions
    vAigs = Vec_PtrAlloc( 100 );
    vGias = Vec_PtrAlloc( 100 );
    vMaps = Vec_PtrAlloc( 100 );
    if ( fPrintParts )
        Abc_Print( 1, "Simple partitioning. %d partitions are saved:\n", Vec_PtrSize(vResult) );
    Vec_PtrForEachEntry( Vec_Int_t *, vResult, vPart, i )
    {
        pTemp = Aig_ManRegCreatePart( pAig, vPart, &nCountPis, &nCountRegs, &pMapBack );
        Aig_ManSetRegNum( pTemp, pTemp->nRegs );
        Vec_PtrPush( vAigs, pTemp );
        Vec_PtrPush( vGias, Gia_ManFromAigSimple(pTemp) );
        Vec_PtrPush( vMaps, pMapBack );
        //sprintf( Buffer, "part%03d.aig", i );
        //Ioa_WriteAiger( pTemp, Buffer, 0, 0 );
        if ( fPrintParts )
            Abc_Print( 1, "part%03d.aig : Reg = %4d. PI = %4d. (True = %4d. Regs = %4d.) And = %5d.\n",
                i, Vec_IntSize(vPart), Aig_ManCiNum(pTemp)-Vec_IntSize(vPart), nCountPis, nCountRegs, Aig_ManNodeNum(pTemp) );
    }
    // solve partitions
    Ssw_SignalCorrespondenceArray( vGias, pPars );
    // collect the results
    Aig_ManReprStart( pAig, Aig_ManObjNumMax(pAig) );
    Vec_PtrForEachEntry( Vec_Int_t *, vResult, vPart, i )
    {
        int * pMapBack = (int *)Vec_PtrEntry( vMaps, i );
        Gia_Man_t * pGia = (Gia_Man_t *)Vec_PtrEntry( vGias, i );
        Aig_Man_t * pTemp2 = Gia_ManToAigSimple( pGia );
        pTemp = (Aig_Man_t *)Vec_PtrEntry( vAigs, i );
        Gia_ManReprToAigRepr2( pTemp2, pGia );
        // remap back
        nClasses = Aig_TransferMappedClasses( pAig, pTemp2, pMapBack );
        if ( fVerbose )
            Abc_Print( 1, "%3d : Reg = %4d. PI = %4d. (True = %4d. Regs = %4d.) And = %5d. It = %3d. Cl = %5d.\n",
                i, Vec_IntSize(vPart), Aig_ManCiNum(pTemp)-Vec_IntSize(vPart), 0, 0, Aig_ManNodeNum(pTemp), 0, nClasses );
        Aig_ManStop( pTemp );
        Aig_ManStop( pTemp2 );
        Gia_ManStop( pGia );
        ABC_FREE( pMapBack );
    }
    Vec_PtrFree( vAigs );
    Vec_PtrFree( vGias );
    Vec_PtrFree( vMaps );

    // remap the AIG
    pNew = Aig_ManDupRepr( pAig, 0 );
    Aig_ManSeqCleanup( pNew );
//    Aig_ManPrintStats( pAig );
//    Aig_ManPrintStats( pNew );
    Vec_VecFree( (Vec_Vec_t *)vResult );
    pPars->nPartSize = nPartSize;
    pPars->fVerbose = fVerbose;
    if ( fVerbose )
    {
        ABC_PRT( "Total time", Abc_Clock() - clk );
    }
    return pNew;
}
void Gia_ManRestoreNodeMapping( Aig_Man_t * pAig, Gia_Man_t * pGia )
{
    Aig_Obj_t * pObjAig; int i;
    assert( Gia_ManObjNum(pGia) == Aig_ManObjNum(pAig) );
    Aig_ManForEachObj( pAig, pObjAig, i )
        pObjAig->iData = Abc_Var2Lit(i, 0);
}
Gia_Man_t * Gia_SignalCorrespondencePart( Gia_Man_t * p, Cec_ParCor_t * pPars )
{
    Gia_Man_t * pRes = NULL;
    Aig_Man_t * pNew = NULL;
    Aig_Man_t * pAig = Gia_ManToAigSimple(p);
    Ssw_Pars_t SswPars, * pSswPars = &SswPars;
    assert( pPars->nProcs > 0 );
    assert( pPars->nPartSize > 0 );
    Ssw_ManSetDefaultParams( pSswPars ); 
    pSswPars->nBTLimit  = pPars->nBTLimit;
    pSswPars->nProcs    = pPars->nProcs;
    pSswPars->nPartSize = pPars->nPartSize;
    pSswPars->fVerbose  = pPars->fVerbose;
    pNew = Ssw_SignalCorrespondencePart2( pAig, pSswPars );
    Gia_ManRestoreNodeMapping( pAig, p );
    Gia_ManReprFromAigRepr2( pAig, p );
    pRes = Gia_ManFromAigSimple(pNew);
    Aig_ManStop( pNew );
    Aig_ManStop( pAig );
    return pRes;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END
