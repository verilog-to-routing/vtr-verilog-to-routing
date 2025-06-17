/**CFile****************************************************************

  FileName    [giaTranStoch.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Implementation of transduction method.]

  Author      [Yukio Miyasaka]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 2023.]

  Revision    [$Id: giaTranStoch.c,v 1.00 2023/05/10 00:00:00 Exp $]

***********************************************************************/

#include <base/abc/abc.h>
#include <aig/aig/aig.h>
#include <opt/dar/dar.h>
#include <aig/gia/gia.h>
#include <aig/gia/giaAig.h>
#include <base/main/main.h>
#include <base/main/mainInt.h>
#include <map/mio/mio.h>
#include <opt/sfm/sfm.h>
#include <opt/fxu/fxu.h>

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

extern Abc_Ntk_t * Abc_NtkFromAigPhase( Aig_Man_t * pMan );
extern Abc_Ntk_t * Abc_NtkIf( Abc_Ntk_t * pNtk, If_Par_t * pPars );
extern int Abc_NtkPerformMfs( Abc_Ntk_t * pNtk, Sfm_Par_t * pPars );
extern Aig_Man_t * Abc_NtkToDar( Abc_Ntk_t * pNtk, int fExors, int fRegisters );
extern int Abc_NtkFxPerform( Abc_Ntk_t * pNtk, int nNewNodesMax, int nLitCountMax, int fCanonDivs, int fVerbose, int fVeryVerbose );

Abc_Ntk_t * Gia_ManTranStochPut( Gia_Man_t * pGia ) {
  Abc_Ntk_t * pNtk;
  Aig_Man_t * pMan = Gia_ManToAig( pGia, 0 );
  pNtk = Abc_NtkFromAigPhase( pMan );
  Aig_ManStop( pMan );
  return pNtk;
}
Abc_Ntk_t * Gia_ManTranStochIf( Abc_Ntk_t * pNtk ) {
  If_Par_t Pars, * pPars = &Pars;
  If_ManSetDefaultPars( pPars );
  pPars->pLutLib = (If_LibLut_t *)Abc_FrameReadLibLut();
  pPars->nLutSize = pPars->pLutLib->LutMax;
  return Abc_NtkIf( pNtk, pPars );
}
void Gia_ManTranStochMfs2( Abc_Ntk_t * pNtk ) {
  Sfm_Par_t Pars, * pPars = &Pars;
  Sfm_ParSetDefault( pPars );
  Abc_NtkPerformMfs( pNtk, pPars );
}
Gia_Man_t * Gia_ManTranStochGet( Abc_Ntk_t * pNtk ) {
  Gia_Man_t * pGia;
  Aig_Man_t * pAig = Abc_NtkToDar( pNtk, 0, 1 );
  pGia = Gia_ManFromAig( pAig );
  Aig_ManStop( pAig );
  return pGia;
}
void Gia_ManTranStochFx( Abc_Ntk_t * pNtk ) {
  Fxu_Data_t Params, * p = &Params;
  Abc_NtkSetDefaultFxParams( p );
  Abc_NtkFxPerform( pNtk, p->nNodesExt, p->LitCountMax, p->fCanonDivs, p->fVerbose, p->fVeryVerbose );
  Abc_NtkFxuFreeInfo( p );
}
Gia_Man_t * Gia_ManTranStochRefactor( Gia_Man_t * pGia ) {
  Gia_Man_t * pNew;
  Aig_Man_t * pAig, * pTemp;
  Dar_RefPar_t Pars, * pPars = &Pars;
  Dar_ManDefaultRefParams( pPars );
  pPars->fUseZeros = 1;
  pAig = Gia_ManToAig( pGia, 0 );
  Dar_ManRefactor( pAig, pPars );
  pAig = Aig_ManDupDfs( pTemp = pAig );
  Aig_ManStop( pTemp );
  pNew = Gia_ManFromAig( pAig );
  Aig_ManStop( pAig );
  return pNew;
}

struct Gia_ManTranStochParam {
  Gia_Man_t * pStart;
  int nSeed;
  int nHops;
  int nRestarts;
  int nSeedBase;
  int fMspf;
  int fMerge;
  int fResetHop;
  int fZeroCostHop;
  int fRefactor;
  int fTruth;
  int fNewLine;
  Gia_Man_t * pExdc;
  int nVerbose;

#ifdef ABC_USE_PTHREADS
  int nSp;
  int nIte;
  Gia_Man_t * pRes;
  int fWorking;
  pthread_mutex_t * mutex;
#endif
};

typedef struct Gia_ManTranStochParam Gia_ManTranStochParam;

void Gia_ManTranStochLock( Gia_ManTranStochParam * p ) {
#ifdef ABC_USE_PTHREADS
  if ( p->fWorking )
    pthread_mutex_lock( p->mutex );
#endif
}
void Gia_ManTranStochUnlock( Gia_ManTranStochParam * p ) {
#ifdef ABC_USE_PTHREADS
  if ( p->fWorking )
    pthread_mutex_unlock( p->mutex );
#endif
}
  
Gia_Man_t * Gia_ManTranStochOpt1( Gia_ManTranStochParam * p, Gia_Man_t * pOld ) {
  Gia_Man_t * pGia, * pNew;
  int i = 0, n;
  pGia = Gia_ManDup( pOld );
  do {
    n = Gia_ManAndNum( pGia );
    if ( p->fTruth )
      pNew = Gia_ManTransductionTt( pGia, (p->fMerge? 8: 7), p->fMspf, p->nSeed++, 0, 0, 0, 0, p->pExdc, p->fNewLine, p->nVerbose > 0? p->nVerbose - 1: 0 );
    else
      pNew = Gia_ManTransductionBdd( pGia, (p->fMerge? 8: 7), p->fMspf, p->nSeed++, 0, 0, 0, 0, p->pExdc, p->fNewLine, p->nVerbose > 0? p->nVerbose - 1: 0 );
    Gia_ManStop( pGia );
    pGia = pNew;
    if ( p->fRefactor ) {
      pNew = Gia_ManTranStochRefactor( pGia );
      Gia_ManStop( pGia );
      pGia = pNew;
    } else {
      Gia_ManTranStochLock( p );
      pNew = Gia_ManCompress2( pGia, 1, 0 );
      Gia_ManTranStochUnlock( p );
      Gia_ManStop( pGia );
      pGia = pNew;
    }
    if ( p->nVerbose )
      printf( "*                ite %d : #nodes = %5d\n", i, Gia_ManAndNum( pGia ) );
    i++;
  } while ( n > Gia_ManAndNum( pGia ) );
  return pGia;
}

Gia_Man_t * Gia_ManTranStochOpt2( Gia_ManTranStochParam * p ) {
  int i, n = Gia_ManAndNum( p->pStart );
  Gia_Man_t * pGia, * pBest, * pNew;
  Abc_Ntk_t * pNtk, * pNtkRes;
  pGia = Gia_ManDup( p->pStart );
  pBest = Gia_ManDup( pGia );
  for ( i = 0; 1; i++ ) {
    pNew = Gia_ManTranStochOpt1( p, pGia );
    Gia_ManStop( pGia );
    pGia = pNew;
    if ( n > Gia_ManAndNum( pGia ) ) {
      n = Gia_ManAndNum( pGia );
      Gia_ManStop( pBest );
      pBest = Gia_ManDup( pGia );
      if ( p->fResetHop )
        i = 0;
    }
    if ( i == p->nHops )
      break;
    if ( p->fZeroCostHop ) {
      pNew = Gia_ManTranStochRefactor( pGia );
      Gia_ManStop( pGia );
      pGia = pNew;
    } else {
      Gia_ManTranStochLock( p );
      pNtk = Gia_ManTranStochPut( pGia );
      Gia_ManTranStochUnlock( p );
      Gia_ManStop( pGia );
      pNtkRes = Gia_ManTranStochIf( pNtk );
      Abc_NtkDelete( pNtk );
      pNtk = pNtkRes;
      Gia_ManTranStochMfs2( pNtk );
      Gia_ManTranStochLock( p );
      pNtkRes = Abc_NtkStrash( pNtk, 0, 1, 0 );
      Gia_ManTranStochUnlock( p );
      Abc_NtkDelete( pNtk );
      pNtk = pNtkRes;
      pGia = Gia_ManTranStochGet( pNtk );
      Abc_NtkDelete( pNtk );
    }
    if ( p->nVerbose )
      printf( "*         hop %d        : #nodes = %5d\n", i, Gia_ManAndNum( pGia ) );
  }
  Gia_ManStop( pGia );
  return pBest;
}

Gia_Man_t * Gia_ManTranStochOpt3( Gia_ManTranStochParam * p ) {
  int i, n = Gia_ManAndNum( p->pStart );
  Gia_Man_t * pBest, * pNew;
  pBest = Gia_ManDup( p->pStart );
  for ( i = 0; i <= p->nRestarts; i++ ) {
    p->nSeed = 1234 * (i + p->nSeedBase);
    pNew = Gia_ManTranStochOpt2( p );
    if ( p->nRestarts && p->nVerbose )
      printf( "*  res %2d              : #nodes = %5d\n", i, Gia_ManAndNum( pNew ) );
    if ( n > Gia_ManAndNum( pNew ) ) {
      n = Gia_ManAndNum( pNew );
      Gia_ManStop( pBest );
      pBest = pNew;
    } else {
      Gia_ManStop( pNew );
    }
  }
  return pBest;
}

#ifdef ABC_USE_PTHREADS
void * Gia_ManTranStochWorkerThread( void * pArg ) {
  Gia_ManTranStochParam * p = (Gia_ManTranStochParam *)pArg;
  volatile int * pPlace = &p->fWorking;
  while ( 1 ) {
    while ( *pPlace == 0 );
    assert( p->fWorking );
    if ( p->pStart == NULL ) {
      pthread_exit( NULL );
      assert( 0 );
      return NULL;
    }
    p->nSeed = 1234 * (p->nIte + p->nSeedBase);
    p->pRes = Gia_ManTranStochOpt2( p );
    p->fWorking = 0;
  }
  assert( 0 );
  return NULL;
}
#endif

Gia_Man_t * Gia_ManTranStoch( Gia_Man_t * pGia, int nRestarts, int nHops, int nSeedBase, int fMspf, int fMerge, int fResetHop, int fZeroCostHop, int fRefactor, int fTruth, int fSingle, int fOriginalOnly, int fNewLine, Gia_Man_t * pExdc, int nThreads, int nVerbose ) {
  int i, j = 0;
  Gia_Man_t * pNew, * pBest, * pStart;
  Abc_Ntk_t * pNtk, * pNtkRes; Vec_Ptr_t * vpStarts;
  Gia_ManTranStochParam Par, *p = &Par;
  p->nRestarts = nRestarts;
  p->nHops = nHops;
  p->nSeedBase = nSeedBase;
  p->fMspf = fMspf;
  p->fMerge = fMerge;
  p->fResetHop = fResetHop;
  p->fZeroCostHop = fZeroCostHop;
  p->fRefactor = fRefactor;
  p->fTruth = fTruth;
  p->fNewLine = fNewLine;
  p->pExdc = pExdc;
  p->nVerbose = nVerbose;
#ifdef ABC_USE_PTHREADS
  p->fWorking = 0;
#endif
  // setup start points
  vpStarts = Vec_PtrAlloc( 4 );
  Vec_PtrPush( vpStarts, Gia_ManDup( pGia ) );
  if ( !fOriginalOnly ) {
    { // &put; collapse; st; &get;
      pNtk = Gia_ManTranStochPut( pGia );
      pNtkRes = Abc_NtkCollapse( pNtk, ABC_INFINITY, 0, 1, 0, 0, 0 );
      Abc_NtkDelete( pNtk );
      pNtk = pNtkRes;
      pNtkRes = Abc_NtkStrash( pNtk, 0, 1, 0 );
      Abc_NtkDelete( pNtk );
      pNtk = pNtkRes;
      pNew = Gia_ManTranStochGet( pNtk );
      Abc_NtkDelete( pNtk );
      Vec_PtrPush( vpStarts, pNew );
    }
    { // &ttopt;
      pNew = Gia_ManTtopt( pGia, Gia_ManCiNum( pGia ), Gia_ManCoNum( pGia ), 100 );
      Vec_PtrPush( vpStarts, pNew );
    }
    { // &put; collapse; sop; fx; 
      pNtk = Gia_ManTranStochPut( pGia );
      pNtkRes = Abc_NtkCollapse( pNtk, ABC_INFINITY, 0, 1, 0, 0, 0 );
      Abc_NtkDelete( pNtk );
      pNtk = pNtkRes;
      Abc_NtkToSop( pNtk, -1, ABC_INFINITY );
      Gia_ManTranStochFx( pNtk );
      pNtkRes = Abc_NtkStrash( pNtk, 0, 1, 0 );
      Abc_NtkDelete( pNtk );
      pNtk = pNtkRes;
      pNew = Gia_ManTranStochGet( pNtk );
      Abc_NtkDelete( pNtk );
      Vec_PtrPush( vpStarts, pNew );
    }
  }
  if ( fSingle ) {
    pBest = (Gia_Man_t *)Vec_PtrEntry( vpStarts, 0 );
    for ( i = 1; i < Vec_PtrSize( vpStarts ); i++ ) {
      pStart = (Gia_Man_t *)Vec_PtrEntry( vpStarts, i );
      if ( Gia_ManAndNum( pStart ) < Gia_ManAndNum( pBest ) ) {
        Gia_ManStop( pBest );
        pBest = pStart;
        j = i;
      } else {
        Gia_ManStop( pStart );
      }
    }
    Vec_PtrClear( vpStarts );
    Vec_PtrPush( vpStarts, pBest );
  }
  // optimize
  pBest = Gia_ManDup( pGia );
  if ( nThreads == 1 ) {
    Vec_PtrForEachEntry( Gia_Man_t *, vpStarts, pStart, i ) {
      if ( p->nVerbose )
        printf( "*begin starting point %d: #nodes = %5d\n", i + j, Gia_ManAndNum( pStart ) );
      p->pStart = pStart;
      pNew = Gia_ManTranStochOpt3( p );
      if ( p->nVerbose )
        printf( "*end   starting point %d: #nodes = %5d\n", i + j, Gia_ManAndNum( pNew ) );
      if ( Gia_ManAndNum( pBest ) > Gia_ManAndNum( pNew ) ) {
        Gia_ManStop( pBest );
        pBest = pNew;
      } else {
        Gia_ManStop( pNew );
      }
      Gia_ManStop( pStart );
    }
  } else {
#ifdef ABC_USE_PTHREADS
    static pthread_mutex_t mutex;
    int k, status, nIte, fAssigned, fWorking;
    Gia_ManTranStochParam ThData[100];
    pthread_t WorkerThread[100];
    p->pRes = NULL;
    p->mutex = &mutex;
    if ( p->nVerbose )
      p->nVerbose--;
    for ( i = 0; i < nThreads; i++ ) {
      ThData[i] = *p;
      status = pthread_create( WorkerThread + i, NULL, Gia_ManTranStochWorkerThread, (void *)(ThData + i) );
      assert( status == 0 );
    }
    Vec_PtrForEachEntry( Gia_Man_t *, vpStarts, pStart, k ) {
      for ( nIte = 0; nIte <= p->nRestarts; nIte++ ) {
        fAssigned = 0;
        while ( !fAssigned ) {
          for ( i = 0; i < nThreads; i++ ) {
            if ( ThData[i].fWorking )
              continue;
            if ( ThData[i].pRes != NULL ) {
              if( nVerbose )
                printf( "*sp %d res %4d        : #nodes = %5d\n", ThData[i].nSp, ThData[i].nIte, Gia_ManAndNum( ThData[i].pRes ) );
              if ( Gia_ManAndNum( pBest ) > Gia_ManAndNum( ThData[i].pRes ) ) {
                Gia_ManStop( pBest );
                pBest = ThData[i].pRes;
              } else {
                Gia_ManStop( ThData[i].pRes );
              }
              ThData[i].pRes = NULL;
            }
            ThData[i].nSp = j + k;
            ThData[i].nIte = nIte;
            ThData[i].pStart = pStart;
            ThData[i].fWorking = 1;
            fAssigned = 1;
            break;
          }
        }
      }
    }
    fWorking = 1;
    while ( fWorking ) {
      fWorking = 0;
      for ( i = 0; i < nThreads; i++ ) {
        if( ThData[i].fWorking ) {
          fWorking = 1;
          continue;
        }
        if ( ThData[i].pRes != NULL ) {
          if( nVerbose )
            printf( "*sp %d res %4d        : #nodes = %5d\n", ThData[i].nSp, ThData[i].nIte, Gia_ManAndNum( ThData[i].pRes ) );
          if ( Gia_ManAndNum( pBest ) > Gia_ManAndNum( ThData[i].pRes ) ) {
            Gia_ManStop( pBest );
            pBest = ThData[i].pRes;
          } else {
            Gia_ManStop( ThData[i].pRes );
          }
          ThData[i].pRes = NULL;
        }
      }
    }
    for ( i = 0; i < nThreads; i++ ) {
      ThData[i].pStart = NULL;
      ThData[i].fWorking = 1;
    }
#else
    printf( "ERROR: pthread is off" );
#endif
    Vec_PtrForEachEntry( Gia_Man_t *, vpStarts, pStart, i )
      Gia_ManStop( pStart );
  }
  if ( nVerbose )
    printf( "best: %d\n", Gia_ManAndNum( pBest ) );
  Vec_PtrFree( vpStarts );
  ABC_FREE( pBest->pName );
  ABC_FREE( pBest->pSpec );
  pBest->pName = Abc_UtilStrsav( pGia->pName );
  pBest->pSpec = Abc_UtilStrsav( pGia->pSpec );
  return pBest;
}

ABC_NAMESPACE_IMPL_END
