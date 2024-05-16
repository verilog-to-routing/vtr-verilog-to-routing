/**CFile****************************************************************

  FileName    [giaDeep.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Experiments with synthesis.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaDeep.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "base/main/main.h"
#include "base/cmd/cmd.h"

#ifdef _MSC_VER
#define unlink _unlink
#else
#include <unistd.h>
#endif

#ifdef ABC_USE_PTHREADS

#ifdef _WIN32
#include "../lib/pthread.h"
#else
#include <pthread.h>
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

  Synopsis    [Processing on a single core.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_StochProcessOne( Gia_Man_t * p, char * pScript, int Rand, int TimeSecs )
{
    Gia_Man_t * pNew;
    char FileName[100], Command[1000];
    sprintf( FileName, "%06x.aig", Rand );
    Gia_AigerWrite( p, FileName, 0, 0, 0 );
    sprintf( Command, "./abc -q \"&read %s; %s; &write %s\"", FileName, pScript, FileName );
    if ( system( (char *)Command ) )    
    {
        fprintf( stderr, "The following command has returned non-zero exit status:\n" );
        fprintf( stderr, "\"%s\"\n", (char *)Command );
        fprintf( stderr, "Sorry for the inconvenience.\n" );
        fflush( stdout );
        unlink( FileName );
        return Gia_ManDup(p);
    }    
    pNew = Gia_AigerRead( FileName, 0, 0, 0 );
    unlink( FileName );
    if ( pNew && Gia_ManAndNum(pNew) < Gia_ManAndNum(p) )
        return pNew;
    Gia_ManStopP( &pNew );
    return Gia_ManDup(p);
}
void Gia_StochProcessArray( Vec_Ptr_t * vGias, char * pScript, int TimeSecs, int fVerbose )
{
    Gia_Man_t * pGia, * pNew; int i;
    Vec_Int_t * vRands = Vec_IntAlloc( Vec_PtrSize(vGias) ); 
    Abc_Random(1);
    for ( i = 0; i < Vec_PtrSize(vGias); i++ )
        Vec_IntPush( vRands, Abc_Random(0) % 0x1000000 );
    Vec_PtrForEachEntry( Gia_Man_t *, vGias, pGia, i ) 
    {
        pNew = Gia_StochProcessOne( pGia, pScript, Vec_IntEntry(vRands, i), TimeSecs );
        Gia_ManStop( pGia );
        Vec_PtrWriteEntry( vGias, i, pNew );
    }
    Vec_IntFree( vRands );
}

/**Function*************************************************************

  Synopsis    [Processing on a many cores.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
#ifndef ABC_USE_PTHREADS

void Gia_StochProcess( Vec_Ptr_t * vGias, char * pScript, int nProcs, int TimeSecs, int fVerbose )
{
    Gia_StochProcessArray( vGias, pScript, TimeSecs, fVerbose );
}

#else // pthreads are used


#define PAR_THR_MAX 100
typedef struct Gia_StochThData_t_
{
    Vec_Ptr_t *  vGias;
    char *       pScript;
    int          Index;
    int          Rand;
    int          nTimeOut;
    int          fWorking;
} Gia_StochThData_t;

void * Gia_StochWorkerThread( void * pArg )
{
    Gia_StochThData_t * pThData = (Gia_StochThData_t *)pArg;
    volatile int * pPlace = &pThData->fWorking;
    Gia_Man_t * pGia, * pNew;
    while ( 1 )
    {
        while ( *pPlace == 0 );
        assert( pThData->fWorking );
        if ( pThData->Index == -1 )
        {
            pthread_exit( NULL );
            assert( 0 );
            return NULL;
        }
        pGia = (Gia_Man_t *)Vec_PtrEntry( pThData->vGias, pThData->Index );
        pNew = Gia_StochProcessOne( pGia, pThData->pScript, pThData->Rand, pThData->nTimeOut );
        Gia_ManStop( pGia );
        Vec_PtrWriteEntry( pThData->vGias, pThData->Index, pNew );
        pThData->fWorking = 0;
    }
    assert( 0 );
    return NULL;
}

void Gia_StochProcess( Vec_Ptr_t * vGias, char * pScript, int nProcs, int TimeSecs, int fVerbose )
{
    Gia_StochThData_t ThData[PAR_THR_MAX];
    pthread_t WorkerThread[PAR_THR_MAX];
    int i, k, status;
    if ( fVerbose )
        printf( "Running concurrent synthesis with %d processes.\n", nProcs );
    fflush( stdout );
    if ( nProcs < 2 )
        return Gia_StochProcessArray( vGias, pScript, TimeSecs, fVerbose );
    // subtract manager thread
    nProcs--;
    assert( nProcs >= 1 && nProcs <= PAR_THR_MAX );
    // start threads
    Abc_Random(1);
    for ( i = 0; i < nProcs; i++ )
    {
        ThData[i].vGias    = vGias;
        ThData[i].pScript  = pScript;
        ThData[i].Index    = -1;
        ThData[i].Rand     = Abc_Random(0) % 0x1000000;
        ThData[i].nTimeOut = TimeSecs;
        ThData[i].fWorking = 0;
        status = pthread_create( WorkerThread + i, NULL, Gia_StochWorkerThread, (void *)(ThData + i) );  assert( status == 0 );
    }
    // look at the threads
    for ( k = 0; k < Vec_PtrSize(vGias); k++ )
    {
        for ( i = 0; i < nProcs; i++ )
        {
            if ( ThData[i].fWorking )
                continue;
            ThData[i].Index = k;
            ThData[i].fWorking = 1;   
            break;
        }
        if ( i == nProcs )
            k--;
    }
    // wait till threads finish
    for ( i = 0; i < nProcs; i++ )
        if ( ThData[i].fWorking )
            i = -1;
    // stop threads
    for ( i = 0; i < nProcs; i++ )
    {
        assert( !ThData[i].fWorking );
        // stop
        ThData[i].Index = -1;
        ThData[i].fWorking = 1;
    }
}

#endif // pthreads are used


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManDupMapping( Gia_Man_t * pNew, Gia_Man_t * p )
{
    Gia_Obj_t * pObj, * pFanin; int i, k;
    Vec_Int_t * vMapping = p->vMapping ? Vec_IntAlloc( Vec_IntSize(p->vMapping) ) : NULL;
    if ( p->vMapping == NULL )
        return;
    Vec_IntFill( vMapping, Gia_ManObjNum(p), 0 );
    Gia_ManForEachLut( p, i )
    {
        pObj = Gia_ManObj( p, i );
        Vec_IntWriteEntry( vMapping, Abc_Lit2Var(pObj->Value), Vec_IntSize(vMapping) );
        Vec_IntPush( vMapping, Gia_ObjLutSize(p, i) );
        Gia_LutForEachFaninObj( p, i, pFanin, k )
            Vec_IntPush( vMapping, Abc_Lit2Var(pFanin->Value)  );
        Vec_IntPush( vMapping, Abc_Lit2Var(pObj->Value) );
    }
    pNew->vMapping = vMapping;
}
Gia_Man_t * Gia_ManDupWithMapping( Gia_Man_t * pGia )
{
    Gia_Man_t * pCopy = Gia_ManDup(pGia);
    Gia_ManDupMapping( pCopy, pGia );
    return pCopy;
}
void Gia_ManStochSynthesis( Vec_Ptr_t * vAigs, char * pScript )
{
    Gia_Man_t * pGia, * pNew; int i;
    Vec_PtrForEachEntry( Gia_Man_t *, vAigs, pGia, i )
    {
        Gia_Man_t * pCopy = Gia_ManDupWithMapping(pGia);
        Abc_FrameUpdateGia( Abc_FrameGetGlobalFrame(), pGia );
        if ( Abc_FrameIsBatchMode() )
        {
            if ( Cmd_CommandExecute(Abc_FrameGetGlobalFrame(), pScript) )
            {
                Abc_Print( 1, "Something did not work out with the command \"%s\".\n", pScript );
                return;
            }
        }
        else
        {
            Abc_FrameSetBatchMode( 1 );
            if ( Cmd_CommandExecute(Abc_FrameGetGlobalFrame(), pScript) )
            {
                Abc_Print( 1, "Something did not work out with the command \"%s\".\n", pScript );
                Abc_FrameSetBatchMode( 0 );
                return;
            }
            Abc_FrameSetBatchMode( 0 );
        }
        pNew = Abc_FrameReadGia(Abc_FrameGetGlobalFrame());
        if ( Gia_ManHasMapping(pNew) && Gia_ManHasMapping(pCopy) )
        {
            if ( Gia_ManLutNum(pNew) < Gia_ManLutNum(pCopy) )
            {
                Gia_ManStop( pCopy );
                pCopy = Gia_ManDupWithMapping( pNew );
            }
        }
        else
        {
            if ( Gia_ManAndNum(pNew) < Gia_ManAndNum(pCopy) )
            {
                Gia_ManStop( pCopy );
                pCopy = Gia_ManDup( pNew );
            }
        }
        Vec_PtrWriteEntry( vAigs, i, pCopy );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCollectNodes_rec( Gia_Man_t * p, int iObj, Vec_Int_t * vAnds )
{
    Gia_Obj_t * pObj;
    if ( Gia_ObjUpdateTravIdCurrentId( p, iObj ) )
        return;
    pObj = Gia_ManObj( p, iObj );
    if ( Gia_ObjIsCi(pObj) || iObj == 0 )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ManCollectNodes_rec( p, Gia_ObjFaninId0(pObj, iObj), vAnds );
    Gia_ManCollectNodes_rec( p, Gia_ObjFaninId1(pObj, iObj), vAnds );
    Vec_IntPush( vAnds, iObj );
}
void Gia_ManCollectNodes( Gia_Man_t * p, Vec_Int_t * vCis, Vec_Int_t * vAnds, Vec_Int_t * vCos )
{
    int i, iObj;
    if ( !Gia_ManHasMapping(p) )
        return;
    Vec_IntClear( vAnds );
    Gia_ManIncrementTravId( p );
    Vec_IntForEachEntry( vCis, iObj, i )
        Gia_ObjSetTravIdCurrentId( p, iObj );
    Vec_IntForEachEntry( vCos, iObj, i )
        Gia_ManCollectNodes_rec( p, iObj, vAnds );
}
Gia_Man_t * Gia_ManDupDivideOne( Gia_Man_t * p, Vec_Int_t * vCis, Vec_Int_t * vAnds, Vec_Int_t * vCos )
{
    Vec_Int_t * vMapping; int i;
    Gia_Man_t * pNew; Gia_Obj_t * pObj; 
    pNew = Gia_ManStart( 1+Vec_IntSize(vCis)+Vec_IntSize(vAnds)+Vec_IntSize(vCos) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    Gia_ManFillValue(p);
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachObjVec( vCis, p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachObjVec( vAnds, p, pObj, i )
        pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachObjVec( vCos, p, pObj, i )
        Gia_ManAppendCo( pNew, pObj->Value );
    assert( Gia_ManCiNum(pNew) > 0 && Gia_ManCoNum(pNew) > 0 );
    if ( !Gia_ManHasMapping(p) )
        return pNew;
    vMapping = Vec_IntAlloc( 4*Gia_ManObjNum(pNew) );
    Vec_IntFill( vMapping, Gia_ManObjNum(pNew), 0 );
    Gia_ManForEachObjVec( vAnds, p, pObj, i )
    {
        Gia_Obj_t * pFanin; int k; 
        int iObj = Gia_ObjId(p, pObj);
        if ( !Gia_ObjIsLut(p, iObj) )
            continue;
        Vec_IntWriteEntry( vMapping, Abc_Lit2Var(pObj->Value), Vec_IntSize(vMapping) );
        Vec_IntPush( vMapping, Gia_ObjLutSize(p, iObj) );
        Gia_LutForEachFaninObj( p, iObj, pFanin, k )
            Vec_IntPush( vMapping, Abc_Lit2Var(pFanin->Value)  );
        Vec_IntPush( vMapping, Abc_Lit2Var(pObj->Value) );
    }
    pNew->vMapping = vMapping;
    return pNew;
}
Vec_Ptr_t * Gia_ManDupDivide( Gia_Man_t * p, Vec_Wec_t * vCis, Vec_Wec_t * vAnds, Vec_Wec_t * vCos, char * pScript, int nProcs, int TimeOut )
{
    Vec_Ptr_t * vAigs = Vec_PtrAlloc( Vec_WecSize(vCis) );  int i;
    for ( i = 0; i < Vec_WecSize(vCis); i++ )
    {
        Gia_ManCollectNodes( p, Vec_WecEntry(vCis, i), Vec_WecEntry(vAnds, i), Vec_WecEntry(vCos, i) );
        Vec_PtrPush( vAigs, Gia_ManDupDivideOne(p, Vec_WecEntry(vCis, i), Vec_WecEntry(vAnds, i), Vec_WecEntry(vCos, i)) );
    }
    //Gia_ManStochSynthesis( vAigs, pScript );
    Gia_StochProcess( vAigs, pScript, nProcs, TimeOut, 0 );
    return vAigs;
}
Gia_Man_t * Gia_ManDupStitch( Gia_Man_t * p, Vec_Wec_t * vCis, Vec_Wec_t * vAnds, Vec_Wec_t * vCos, Vec_Ptr_t * vAigs, int fHash )
{
    Gia_Man_t * pGia, * pNew;
    Gia_Obj_t * pObj; int i, k;
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManCleanValue( p );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    if ( fHash )
        Gia_ManHashAlloc( pNew );
    Vec_PtrForEachEntry( Gia_Man_t *, vAigs, pGia, i )
    {
        Vec_Int_t * vCi = Vec_WecEntry( vCis, i );
        Vec_Int_t * vCo = Vec_WecEntry( vCos, i );
        Gia_ManCleanValue( pGia );
        Gia_ManConst0(pGia)->Value = 0;
        Gia_ManForEachObjVec( vCi, p, pObj, k )
            Gia_ManCi(pGia, k)->Value = pObj->Value;
        if ( fHash )
            Gia_ManForEachAnd( pGia, pObj, k )
                pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) ); 
        else
            Gia_ManForEachAnd( pGia, pObj, k )
                pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) ); 
        Gia_ManForEachObjVec( vCo, p, pObj, k )
            pObj->Value = Gia_ObjFanin0Copy(Gia_ManCo(pGia, k));
    }
    Gia_ManForEachCo( p, pObj, i )
        Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    if ( fHash )
    {
        pNew = Gia_ManCleanup( pGia = pNew );
        Gia_ManStop( pGia );
    }
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    return pNew;
}
Gia_Man_t * Gia_ManDupStitchMap( Gia_Man_t * p, Vec_Wec_t * vCis, Vec_Wec_t * vAnds, Vec_Wec_t * vCos, Vec_Ptr_t * vAigs )
{
    Vec_Int_t * vMapping; int n;
    Gia_Man_t * pGia, * pNew = Gia_ManDupStitch( p, vCis, vAnds, vCos, vAigs, !Gia_ManHasMapping(p) ); 
    if ( !Gia_ManHasMapping(p) )
        return pNew;
    vMapping = Vec_IntAlloc( Vec_IntSize(p->vMapping) );
    Vec_IntFill( vMapping, Gia_ManObjNum(pNew), 0 );
    Vec_PtrForEachEntry( Gia_Man_t *, vAigs, pGia, n )
    {
        Gia_Obj_t * pFanin; int iObj, k; 
        //printf( "Gia %d has %d Luts\n", n, Gia_ManLutNum(pGia) );

        Gia_ManForEachLut( pGia, iObj )
        {
            Gia_Obj_t * pObj = Gia_ManObj( pGia, iObj );
            Vec_IntWriteEntry( vMapping, Abc_Lit2Var(pObj->Value), Vec_IntSize(vMapping) );
            Vec_IntPush( vMapping, Gia_ObjLutSize(pGia, iObj) );
            Gia_LutForEachFaninObj( pGia, iObj, pFanin, k )
                Vec_IntPush( vMapping, Abc_Lit2Var(pFanin->Value)  );
            Vec_IntPush( vMapping, Abc_Lit2Var(pObj->Value) );
        }
    }
    pNew->vMapping = vMapping;
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Wec_t * Gia_ManStochNodes( Gia_Man_t * p, int nMaxSize, int Seed )
{
    Vec_Wec_t * vRes = Vec_WecAlloc( 100 ); 
    Vec_Int_t * vPart = Vec_WecPushLevel( vRes );
    int i, iStart = Seed % Gia_ManCoNum(p);
    //Gia_ManLevelNum( p );
    Gia_ManIncrementTravId( p );
    for ( i = 0; i < Gia_ManCoNum(p); i++ )
    {
        Gia_Obj_t * pObj = Gia_ManCo( p, (iStart+i) % Gia_ManCoNum(p) );
        if ( Vec_IntSize(vPart) > nMaxSize )
            vPart = Vec_WecPushLevel( vRes );
        Gia_ManCollectNodes_rec( p, Gia_ObjFaninId0p(p, pObj), vPart );
    }
    if ( Vec_IntSize(vPart) == 0 )
        Vec_WecShrink( vRes, Vec_WecSize(vRes)-1 );
    //Vec_WecPrint( vRes, 0 );
    return vRes;
}
Vec_Wec_t * Gia_ManStochInputs( Gia_Man_t * p, Vec_Wec_t * vAnds )
{
    Vec_Wec_t * vRes = Vec_WecAlloc( 100 ); 
    Vec_Int_t * vLevel; Gia_Obj_t * pObj; int i, k, iObj, iFan, f;
    Vec_WecForEachLevel( vAnds, vLevel, i )
    {
        Vec_Int_t * vVec = Vec_WecPushLevel( vRes );
        assert( Vec_IntSize(vVec) == 0 );
        Gia_ManIncrementTravId( p );
        Vec_IntForEachEntry( vLevel, iObj, k )
            Gia_ObjSetTravIdCurrentId( p, iObj );
        if ( Gia_ManHasMapping(p) )
        {
            Vec_IntForEachEntry( vLevel, iObj, k )
                if ( Gia_ObjIsLut(p, iObj) )
                    Gia_LutForEachFanin( p, iObj, iFan, f )
                        if ( !Gia_ObjUpdateTravIdCurrentId(p, iFan) )
                            Vec_IntPush( vVec, iFan );
        }
        else
        {
            Gia_ManForEachObjVec( vLevel, p, pObj, k )
            {
                iObj = Gia_ObjFaninId0p(p, pObj);
                if ( !Gia_ObjUpdateTravIdCurrentId(p, iObj) )
                    Vec_IntPush( vVec, iObj );
                iObj = Gia_ObjFaninId1p(p, pObj);
                if ( !Gia_ObjUpdateTravIdCurrentId(p, iObj) )
                    Vec_IntPush( vVec, iObj );
            }
        }
        assert( Vec_IntSize(vVec) > 0 );
    }
    return vRes;
}
Vec_Wec_t * Gia_ManStochOutputs( Gia_Man_t * p, Vec_Wec_t * vAnds )
{
    Vec_Wec_t * vRes = Vec_WecAlloc( 100 ); 
    Vec_Int_t * vLevel; Gia_Obj_t * pObj; int i, k, iObj, iFan, f;
    if ( Gia_ManHasMapping(p) )
    {
        Gia_ManSetLutRefs( p );
        Vec_WecForEachLevel( vAnds, vLevel, i )
        {
            Vec_Int_t * vVec = Vec_WecPushLevel( vRes );
            assert( Vec_IntSize(vVec) == 0 );
            Vec_IntForEachEntry( vLevel, iObj, k )
                if ( Gia_ObjIsLut(p, iObj) )
                    Gia_LutForEachFanin( p, iObj, iFan, f )
                        Gia_ObjLutRefDecId( p, iFan );
            Vec_IntForEachEntry( vLevel, iObj, k )
                if ( Gia_ObjIsLut(p, iObj) )
                    if ( Gia_ObjLutRefNumId(p, iObj) )
                        Vec_IntPush( vVec, iObj );
            Vec_IntForEachEntry( vLevel, iObj, k )
                if ( Gia_ObjIsLut(p, iObj) )
                    Gia_LutForEachFanin( p, iObj, iFan, f )
                        Gia_ObjLutRefIncId( p, iFan );
            assert( Vec_IntSize(vVec) > 0 );
        }
    }
    else
    {
        Gia_ManCreateRefs( p );
        Vec_WecForEachLevel( vAnds, vLevel, i )
        {
            Vec_Int_t * vVec = Vec_WecPushLevel( vRes );
            Gia_ManForEachObjVec( vLevel, p, pObj, k )
            {
                Gia_ObjRefDecId( p, Gia_ObjFaninId0p(p, pObj) );
                Gia_ObjRefDecId( p, Gia_ObjFaninId1p(p, pObj) );
            }
            Gia_ManForEachObjVec( vLevel, p, pObj, k )
                if ( Gia_ObjRefNum(p, pObj) )
                    Vec_IntPush( vVec, Gia_ObjId(p, pObj) );
            Gia_ManForEachObjVec( vLevel, p, pObj, k )
            {
                Gia_ObjRefIncId( p, Gia_ObjFaninId0p(p, pObj) );
                Gia_ObjRefIncId( p, Gia_ObjFaninId1p(p, pObj) );
            }
        }
    }
    return vRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManStochSyn( int nMaxSize, int nIters, int TimeOut, int Seed, int fVerbose, char * pScript, int nProcs )
{
    abctime nTimeToStop  = TimeOut ? Abc_Clock() + TimeOut * CLOCKS_PER_SEC : 0;
    abctime clkStart     = Abc_Clock();
    int fMapped          = Gia_ManHasMapping(Abc_FrameReadGia(Abc_FrameGetGlobalFrame()));
    int nLutEnd, nLutBeg = fMapped ? Gia_ManLutNum(Abc_FrameReadGia(Abc_FrameGetGlobalFrame())) : 0;
    int i, nEnd, nBeg    = Gia_ManAndNum(Abc_FrameReadGia(Abc_FrameGetGlobalFrame()));
    Abc_Random(1);
    for ( i = 0; i < 10+Seed; i++ )
        Abc_Random(0);
    if ( fVerbose )
    printf( "Running %d iterations of script \"%s\".\n", nIters, pScript );
    for ( i = 0; i < nIters; i++ )
    {
        abctime clk = Abc_Clock();
        Gia_Man_t * pGia  = Gia_ManDupWithMapping( Abc_FrameReadGia(Abc_FrameGetGlobalFrame()) );
        Vec_Wec_t * vAnds = Gia_ManStochNodes( pGia, nMaxSize, Abc_Random(0) & 0x7FFFFFFF );
        Vec_Wec_t * vIns  = Gia_ManStochInputs( pGia, vAnds );
        Vec_Wec_t * vOuts = Gia_ManStochOutputs( pGia, vAnds );
        Vec_Ptr_t * vAigs = Gia_ManDupDivide( pGia, vIns, vAnds, vOuts, pScript, nProcs, TimeOut );
        Gia_Man_t * pNew  = Gia_ManDupStitchMap( pGia, vIns, vAnds, vOuts, vAigs );
        int fMapped = Gia_ManHasMapping(pGia) && Gia_ManHasMapping(pNew);
        Abc_FrameUpdateGia( Abc_FrameGetGlobalFrame(), pNew );
        if ( fVerbose )
        printf( "Iteration %3d : Using %3d partitions. Reducing %6d to %6d %s.  ", 
            i, Vec_PtrSize(vAigs), fMapped ? Gia_ManLutNum(pGia) : Gia_ManAndNum(pGia), 
                                   fMapped ? Gia_ManLutNum(pNew) : Gia_ManAndNum(pNew),
                                   fMapped ? "LUTs" : "ANDs" ); 
        if ( fVerbose )
        Abc_PrintTime( 0, "Time", Abc_Clock() - clk );
        Gia_ManStop( pGia );
        Vec_PtrFreeFunc( vAigs, (void (*)(void *)) Gia_ManStop );
        Vec_WecFree( vAnds );
        Vec_WecFree( vIns );
        Vec_WecFree( vOuts );
        if ( nTimeToStop && Abc_Clock() > nTimeToStop )
        {
            printf( "Runtime limit (%d sec) is reached after %d iterations.\n", TimeOut, i );
            break;
        }
    }
    fMapped &= Gia_ManHasMapping(Abc_FrameReadGia(Abc_FrameGetGlobalFrame()));
    nLutEnd  = fMapped ? Gia_ManLutNum(Abc_FrameReadGia(Abc_FrameGetGlobalFrame())) : 0;
    nEnd     = Gia_ManAndNum(Abc_FrameReadGia(Abc_FrameGetGlobalFrame()));
    if ( fVerbose )
    printf( "Cumulatively reduced %d %s after %d iterations.  ", 
        fMapped ? nLutBeg - nLutEnd : nBeg - nEnd, fMapped ? "LUTs" : "ANDs", nIters );
    if ( fVerbose )
    Abc_PrintTime( 0, "Total time", Abc_Clock() - clkStart );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

