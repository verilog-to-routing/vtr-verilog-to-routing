/**CFile****************************************************************

  FileName    [bmcBmcS.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based bounded model checking.]

  Synopsis    [New BMC package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - July 20, 2017.]

  Revision    [$Id: bmcBmcS.c,v 1.00 2017/07/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bmc.h"
#include "sat/cnf/cnf.h"
#include "sat/satoko/satoko.h"


//#define ABC_USE_EXT_SOLVERS 1

#ifdef ABC_USE_EXT_SOLVERS
    #include "extsat/bmc/bmcApi.h"
    #define l_Undef -1
    #define l_True   1
    #define l_False  0
#else
    #define l_Undef  0
    #define l_True   1
    #define l_False -1
    #define bmc_sat_solver                     satoko_t
    #define bmc_sat_solver_start(type)         satoko_create()
    #define bmc_sat_solver_stop                satoko_destroy
    #define bmc_sat_solver_addclause           satoko_add_clause
    #define bmc_sat_solver_addvar(s)           satoko_add_variable(s, 0)
    #define bmc_sat_solver_solve               satoko_solve_assumptions
    #define bmc_sat_solver_read_cex_varvalue   satoko_read_cex_varvalue
    #define bmc_sat_solver_setstop             satoko_set_stop
#endif



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

#define PAR_THR_MAX 100

typedef struct Bmcs_Man_t_ Bmcs_Man_t;
struct Bmcs_Man_t_
{
    Bmc_AndPar_t *    pPars;               // parameters
    Gia_Man_t *       pGia;                // user's AIG
    Gia_Man_t *       pFrames;             // unfolded AIG (pFrames->vCopies point to pClean)
    Gia_Man_t *       pClean;              // incremental AIG (pClean->Value point to pFrames)
    Vec_Ptr_t         vGia2Fr;             // copies of GIA in each timeframe
    Vec_Int_t         vFr2Sat;             // mapping of objects in pFrames into SAT variables
    Vec_Int_t         vCiMap;              // maps CIs of pFrames into CIs/frames of GIA
    bmc_sat_solver *  pSats[PAR_THR_MAX];  // concurrent SAT solvers
    int               nSatVars;            // number of SAT variables used
    int               nSatVarsOld;         // number of SAT variables used
    int               fStopNow;            // signal when it is time to stop
    abctime           timeUnf;             // runtime of unfolding
    abctime           timeCnf;             // runtime of CNF generation
    abctime           timeSat;             // runtime of the solvers
    abctime           timeOth;             // other runtime
};

//static inline int * Bmcs_ManCopies( Bmcs_Man_t * p, int f ) { return (int*)Vec_PtrEntry(&p->vGia2Fr, f % Vec_PtrSize(&p->vGia2Fr)); }
static inline int * Bmcs_ManCopies( Bmcs_Man_t * p, int f ) { return (int*)Vec_PtrEntry(&p->vGia2Fr, f); }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Incremental unfolding.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bmc_SuperBuildTents_rec( Gia_Man_t * p, int iObj, Vec_Int_t * vIns, Vec_Int_t * vCuts, Vec_Int_t * vFlops, Vec_Int_t * vObjs, Vec_Int_t * vRankIns, Vec_Int_t * vRankCuts, int Rank )
{
    Gia_Obj_t * pObj;
    if ( iObj == 0 )
        return;
    if ( Gia_ObjIsTravIdCurrentId(p, iObj) )
        return;
    Gia_ObjSetTravIdCurrentId(p, iObj);
    pObj = Gia_ManObj( p, iObj );
    if ( pObj->fMark0 )
    {
        if ( !pObj->fMark1 )
            return;
        Vec_IntPush( vCuts, iObj );
        Vec_IntPush( vRankCuts, Rank );
        pObj->fMark1 = 1;
        return;
    }
    pObj->fMark0 = 1;
    if ( Gia_ObjIsPi(p, pObj) )
    {
        Vec_IntPush( vIns, iObj );
        Vec_IntPush( vRankIns, Rank );
        pObj->fMark1 = 1;
        return;
    }
    if ( Gia_ObjIsCi(pObj) )
    {
        Vec_IntPush( vFlops, iObj );
        return;
    }
    assert( Gia_ObjIsAnd(pObj) );
    Bmc_SuperBuildTents_rec( p, Gia_ObjFaninId0(pObj, iObj), vIns, vCuts, vFlops, vObjs, vRankIns, vRankCuts, Rank );
    Bmc_SuperBuildTents_rec( p, Gia_ObjFaninId1(pObj, iObj), vIns, vCuts, vFlops, vObjs, vRankIns, vRankCuts, Rank );
    Vec_IntPush( vObjs, iObj );
}
Gia_Man_t * Bmc_SuperBuildTents( Gia_Man_t * p, Vec_Int_t ** pvMap )
{
    Vec_Int_t * vIns      = Vec_IntAlloc( 1000 );
    Vec_Int_t * vCuts     = Vec_IntAlloc( 1000 );
    Vec_Int_t * vFlops    = Vec_IntAlloc( 1000 );
    Vec_Int_t * vObjs     = Vec_IntAlloc( 1000 );
    Vec_Int_t * vLimIns   = Vec_IntAlloc( 1000 );
    Vec_Int_t * vLimCuts  = Vec_IntAlloc( 1000 );
    Vec_Int_t * vLimFlops = Vec_IntAlloc( 1000 );
    Vec_Int_t * vLimObjs  = Vec_IntAlloc( 1000 );
    Vec_Int_t * vRankIns  = Vec_IntAlloc( 1000 );
    Vec_Int_t * vRankCuts = Vec_IntAlloc( 1000 );
    Vec_Int_t * vMap      = Vec_IntAlloc( 1000 );
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;
    int i, r, Entry, Rank, iPrev, iThis = 0;
    // collect internal nodes
    Gia_ManForEachPo( p, pObj, i )
        Vec_IntPush( vFlops, Gia_ObjId(p, pObj) );
    Gia_ManCleanMark01( p );
    for ( Rank = 0; iThis < Vec_IntEntryLast(vFlops); Rank++ )
    {
        Vec_IntPush( vLimIns,   Vec_IntSize(vIns)   );
        Vec_IntPush( vLimCuts,  Vec_IntSize(vCuts)  );
        Vec_IntPush( vLimFlops, Vec_IntSize(vFlops) );
        Vec_IntPush( vLimObjs,  Vec_IntSize(vObjs)  );
        iPrev = iThis;
        iThis = Vec_IntEntryLast(vFlops);
        Vec_IntForEachEntryStartStop( vFlops, Entry, i, iPrev, iThis )
        {
            Gia_ManIncrementTravId( p );
            Bmc_SuperBuildTents_rec( p, Gia_ObjFaninId0(Gia_ManObj(p, iPrev), iPrev), vIns, vCuts, vFlops, vObjs, vRankIns, vRankCuts, Rank );
        }
    }
    Gia_ManCleanMark01( p );
    Vec_IntPush( vLimIns,   Vec_IntSize(vIns)   );
    Vec_IntPush( vLimCuts,  Vec_IntSize(vCuts)  );
    Vec_IntPush( vLimFlops, Vec_IntSize(vFlops) );
    Vec_IntPush( vLimObjs,  Vec_IntSize(vObjs)  );
    // create new GIA
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManFillValue( p );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachObjVec( vIns, p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManForEachObjVec( vCuts, p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    for ( r = Rank; r >= 0; r-- )
    {
        Vec_IntForEachEntryStartStop( vFlops, Entry, i, Vec_IntEntry(vLimFlops, r), Vec_IntEntry(vLimFlops, r+1) )
        {
            pObj = Gia_ManObj(p, Entry);
            pObj->Value = Gia_ObjFanin0Copy(pObj);
        }
        Vec_IntForEachEntryStartStop( vObjs, Entry, i, Vec_IntEntry(vLimObjs, r), Vec_IntEntry(vLimObjs, r+1) )
        {
            pObj = Gia_ManObj(p, Entry);
            pObj->Value = Gia_ObjFanin0Copy(pObj);
        }
    }
    Gia_ManForEachPo( p, pObj, i )
        Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManForEachObjVec( vCuts, p, pObj, i )
        Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManSetRegNum( pNew, Vec_IntSize(vCuts) );

    // create map
    Vec_IntForEachEntryTwo( vIns, vRankIns, Entry, Rank, i )
        Vec_IntPushTwo( vMap, Entry, Rank );
    Vec_IntForEachEntryTwo( vCuts, vRankCuts, Entry, Rank, i )
        Vec_IntPushTwo( vMap, Entry, Rank );

    Vec_IntFree( vIns   );
    Vec_IntFree( vCuts  );
    Vec_IntFree( vFlops );
    Vec_IntFree( vObjs  );

    Vec_IntFree( vLimIns   );
    Vec_IntFree( vLimCuts  );
    Vec_IntFree( vLimFlops );
    Vec_IntFree( vLimObjs  );

    Vec_IntFree( vRankIns  );
    Vec_IntFree( vRankCuts  );

    if ( pvMap )
        *pvMap = vMap;
    else
        Vec_IntFree( vMap );

    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}



/**Function*************************************************************

  Synopsis    [Count tents.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCountTents_rec( Gia_Man_t * p, int iObj, Vec_Int_t * vRoots )
{
    Gia_Obj_t * pObj;
    if ( Gia_ObjIsTravIdCurrentId(p, iObj) )
        return;
    Gia_ObjSetTravIdCurrentId(p, iObj);
    pObj = Gia_ManObj( p, iObj );
    if ( Gia_ObjIsAnd(pObj) )
    {
        Gia_ManCountTents_rec( p, Gia_ObjFaninId0(pObj, iObj), vRoots );
        Gia_ManCountTents_rec( p, Gia_ObjFaninId1(pObj, iObj), vRoots );
    }
    else if ( Gia_ObjIsRo(p, pObj) )
        Vec_IntPush( vRoots, Gia_ObjFaninId0p(p, Gia_ObjRoToRi(p, pObj)) );
    else if ( !Gia_ObjIsPi(p, pObj) )
        assert( 0 );
}
int Gia_ManCountTents( Gia_Man_t * p )
{
    Vec_Int_t * vRoots;
    Gia_Obj_t * pObj;  
    int t, i, iObj, nSizeCurr = 0;
    assert( Gia_ManPoNum(p) > 0 );
    Gia_ManIncrementTravId( p );
    Gia_ObjSetTravIdCurrentId( p, 0 );
    vRoots = Vec_IntAlloc( 100 );
    Gia_ManForEachPo( p, pObj, i )
        Vec_IntPush( vRoots, Gia_ObjFaninId0p(p, pObj) );
    for ( t = 0; nSizeCurr < Vec_IntSize(vRoots); t++ )
    {
        int nSizePrev = nSizeCurr;
        nSizeCurr = Vec_IntSize(vRoots);
        Vec_IntForEachEntryStartStop( vRoots, iObj, i, nSizePrev, nSizeCurr )
            Gia_ManCountTents_rec( p, iObj, vRoots );
    }
    Vec_IntFree( vRoots );
    return t;
}

/**Function*************************************************************

  Synopsis    [Count tents.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCountRanks_rec( Gia_Man_t * p, int iObj, Vec_Int_t * vRoots, Vec_Int_t * vRanks, Vec_Int_t * vCands, int Rank )
{
    Gia_Obj_t * pObj;
    if ( Gia_ObjIsTravIdCurrentId(p, iObj) )
    {
        if ( Vec_IntEntry(vRanks, iObj) < Rank )
            Vec_IntWriteEntry( vCands, iObj, 1 );
        return;
    }
    Gia_ObjSetTravIdCurrentId(p, iObj);
    Vec_IntWriteEntry( vRanks, iObj, Rank );
    pObj = Gia_ManObj( p, iObj );
    if ( Gia_ObjIsAnd(pObj) )
    {
        Gia_ManCountRanks_rec( p, Gia_ObjFaninId0(pObj, iObj), vRoots, vRanks, vCands, Rank );
        Gia_ManCountRanks_rec( p, Gia_ObjFaninId1(pObj, iObj), vRoots, vRanks, vCands, Rank );
    }
    else if ( Gia_ObjIsRo(p, pObj) )
        Vec_IntPush( vRoots, Gia_ObjFaninId0p(p, Gia_ObjRoToRi(p, pObj)) );
    else if ( !Gia_ObjIsPi(p, pObj) )
        assert( 0 );
}
int Gia_ManCountRanks( Gia_Man_t * p )
{
    Vec_Int_t * vRoots;
    Vec_Int_t * vRanks = Vec_IntStartFull( Gia_ManObjNum(p) );
    Vec_Int_t * vCands = Vec_IntStart( Gia_ManObjNum(p) );
    Gia_Obj_t * pObj;  
    int t, i, iObj, nSizeCurr = 0;
    assert( Gia_ManPoNum(p) > 0 );
    Gia_ManIncrementTravId( p );
    Gia_ObjSetTravIdCurrentId( p, 0 );
    vRoots = Vec_IntAlloc( 100 );
    Gia_ManForEachPo( p, pObj, i )
        Vec_IntPush( vRoots, Gia_ObjFaninId0p(p, pObj) );
    for ( t = 0; nSizeCurr < Vec_IntSize(vRoots); t++ )
    {
        int nSizePrev = nSizeCurr;
        nSizeCurr = Vec_IntSize(vRoots);
        Vec_IntForEachEntryStartStop( vRoots, iObj, i, nSizePrev, nSizeCurr )
            Gia_ManCountRanks_rec( p, iObj, vRoots, vRanks, vCands, t );
    }
    Vec_IntWriteEntry( vCands, 0, 0 );
    printf( "Tents = %6d.   Cands = %6d.  %10.2f %%\n", t, Vec_IntSum(vCands), 100.0*Vec_IntSum(vCands)/Gia_ManCandNum(p) );
    Vec_IntFree( vRoots );
    Vec_IntFree( vRanks );
    Vec_IntFree( vCands );
    return t;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Bmcs_Man_t * Bmcs_ManStart( Gia_Man_t * pGia, Bmc_AndPar_t * pPars )
{
    Bmcs_Man_t * p = ABC_CALLOC( Bmcs_Man_t, 1 ); 
    int i, Lit = Abc_Var2Lit( 0, 1 );
    satoko_opts_t opts;
    satoko_default_opts(&opts);
    opts.conf_limit = pPars->nConfLimit;
    assert( Gia_ManRegNum(pGia) > 0 );
    p->pPars   = pPars;
    p->pGia    = pGia;
    p->pFrames = Gia_ManStart( 3*Gia_ManObjNum(pGia) );  Gia_ManHashStart(p->pFrames);
    p->pClean  = NULL;
//    Vec_PtrFill( &p->vGia2Fr, Gia_ManCountTents(pGia)+1, NULL );
//    for ( i = 0; i < Vec_PtrSize(&p->vGia2Fr); i++ )
//        Vec_PtrWriteEntry( &p->vGia2Fr, i, ABC_FALLOC(int, Gia_ManObjNum(pGia)) );
    Vec_PtrGrow( &p->vGia2Fr, 1000 );  
    Vec_IntGrow( &p->vFr2Sat, 3*Gia_ManCiNum(pGia) );  
    Vec_IntPush( &p->vFr2Sat, 0 );
    Vec_IntGrow( &p->vCiMap, 3*Gia_ManCiNum(pGia) );
    for ( i = 0; i < pPars->nProcs; i++ )
    {
        // modify parameters to get different SAT solvers
        opts.f_rst = 0.8 - i * 0.05;
        opts.b_rst = 1.4 - i * 0.05;
        opts.garbage_max_ratio = (float) 0.3 + i * 0.05;
        // create SAT solvers
        p->pSats[i] = bmc_sat_solver_start( i );  
#ifdef ABC_USE_EXT_SOLVERS
        p->pSats[i]->SolverType = i;
#else
        satoko_configure(p->pSats[i], &opts);
#endif
        bmc_sat_solver_addvar( p->pSats[i] );
        bmc_sat_solver_addclause( p->pSats[i], &Lit, 1 );  
        bmc_sat_solver_setstop( p->pSats[i], &p->fStopNow );
    }
    p->nSatVars = 1;
    return p;
}
void Bmcs_ManStop( Bmcs_Man_t * p )
{
    int i;
    Gia_ManStopP( &p->pFrames );
    Gia_ManStopP( &p->pClean );
    Vec_PtrFreeData( &p->vGia2Fr );
    Vec_PtrErase( &p->vGia2Fr );
    Vec_IntErase( &p->vFr2Sat );
    Vec_IntErase( &p->vCiMap );
    for ( i = 0; i < p->pPars->nProcs; i++ )
        if ( p->pSats[i] ) 
            bmc_sat_solver_stop( p->pSats[i] );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    [Incremental unfolding.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bmcs_ManUnfold_rec( Bmcs_Man_t * p, int iObj, int f )
{
    Gia_Obj_t * pObj; 
    int iLit = 0, * pCopies = Bmcs_ManCopies( p, f );
    if ( pCopies[iObj] >= 0 )
        return pCopies[iObj];
    pObj = Gia_ManObj( p->pGia, iObj );
    if ( Gia_ObjIsCi(pObj) )
    {
        if ( Gia_ObjIsPi(p->pGia, pObj) )
        {
            Vec_IntPushTwo( &p->vCiMap, Gia_ObjCioId(pObj), f );
            iLit = Gia_ManAppendCi( p->pFrames );
        }
        else if ( f > 0 )
        {
            pObj = Gia_ObjRoToRi( p->pGia, pObj );
            iLit = Bmcs_ManUnfold_rec( p, Gia_ObjFaninId0p(p->pGia, pObj), f-1 );
            iLit = Abc_LitNotCond( iLit, Gia_ObjFaninC0(pObj) );
        }
    }
    else if ( Gia_ObjIsAnd(pObj) )
    {
        iLit = Bmcs_ManUnfold_rec( p, Gia_ObjFaninId0(pObj, iObj), f );
        iLit = Abc_LitNotCond( iLit, Gia_ObjFaninC0(pObj) );
        if ( iLit > 0 )
        {
            int iNew;
            iNew = Bmcs_ManUnfold_rec( p, Gia_ObjFaninId1(pObj, iObj), f );
            iNew = Abc_LitNotCond( iNew, Gia_ObjFaninC1(pObj) );
            iLit = Gia_ManHashAnd( p->pFrames, iLit, iNew );
        }
    }
    else assert( 0 );
    return (pCopies[iObj] = iLit);
}
int Bmcs_ManCollect_rec( Bmcs_Man_t * p, int iObj )
{
    Gia_Obj_t * pObj; 
    int iSatVar, iLitClean = Gia_ObjCopyArray( p->pFrames, iObj );
    if ( iLitClean >= 0 )
        return iLitClean;
    pObj = Gia_ManObj( p->pFrames, iObj );
    iSatVar = Vec_IntEntry( &p->vFr2Sat, iObj );
    if ( iSatVar > 0 || Gia_ObjIsCi(pObj) )
        iLitClean = Gia_ManAppendCi( p->pClean );
    else if ( Gia_ObjIsAnd(pObj) )
    {
        int iLit0 = Bmcs_ManCollect_rec( p, Gia_ObjFaninId0(pObj, iObj) );
        int iLit1 = Bmcs_ManCollect_rec( p, Gia_ObjFaninId1(pObj, iObj) );
        iLit0 = Abc_LitNotCond( iLit0, Gia_ObjFaninC0(pObj) );
        iLit1 = Abc_LitNotCond( iLit1, Gia_ObjFaninC1(pObj) );
        iLitClean = Gia_ManAppendAnd( p->pClean, iLit0, iLit1 );
    }
    else assert( 0 );
    assert( !Abc_LitIsCompl(iLitClean) );
    Gia_ManObj( p->pClean, Abc_Lit2Var(iLitClean) )->Value = iObj;
    Gia_ObjSetCopyArray( p->pFrames, iObj, iLitClean );
    return iLitClean;
}
Gia_Man_t * Bmcs_ManUnfold( Bmcs_Man_t * p, int f, int nFramesAdd )
{
    Gia_Man_t * pNew = NULL; Gia_Obj_t * pObj;
    int i, k, iLitFrame, iLitClean, fTrivial = 1;
    int * pCopies, nFrameObjs = Gia_ManObjNum(p->pFrames);
    assert( Gia_ManPoNum(p->pFrames) == f * Gia_ManPoNum(p->pGia) );
    for ( k = 0; k < nFramesAdd; k++ )
    {
        // unfold this timeframe
        Vec_PtrPush( &p->vGia2Fr, ABC_FALLOC(int, Gia_ManObjNum(p->pGia)) );
        assert( Vec_PtrSize(&p->vGia2Fr) == f+k+1 );
        pCopies = Bmcs_ManCopies( p, f+k );
        //memset( pCopies, 0xFF, sizeof(int)*Gia_ManObjNum(p->pGia) );  
        pCopies[0] = 0;
        Gia_ManForEachPo( p->pGia, pObj, i )
        {
            iLitFrame = Bmcs_ManUnfold_rec( p, Gia_ObjFaninId0p(p->pGia, pObj), f+k );
            iLitFrame = Abc_LitNotCond( iLitFrame, Gia_ObjFaninC0(pObj) );
            pCopies[Gia_ObjId(p->pGia, pObj)] = Gia_ManAppendCo( p->pFrames, iLitFrame );
            fTrivial &= (iLitFrame == 0);
        }
    }
    if ( fTrivial )
        return NULL;
    // create a clean copy of the new nodes of this timeframe
    Vec_IntFillExtra( &p->vFr2Sat, Gia_ManObjNum(p->pFrames), -1 );
    Vec_IntFillExtra( &p->pFrames->vCopies, Gia_ManObjNum(p->pFrames), -1 );
    //assert( Vec_IntCountEntry(&p->pFrames->vCopies, -1) == Vec_IntSize(&p->pFrames->vCopies) );
    Gia_ManStopP( &p->pClean );
    p->pClean = Gia_ManStart( Gia_ManObjNum(p->pFrames) - nFrameObjs + 1000 );
    Gia_ObjSetCopyArray( p->pFrames, 0, 0 );
    for ( k = 0; k < nFramesAdd; k++ )
    for ( i = 0; i < Gia_ManPoNum(p->pGia); i++ )
    {
        pObj = Gia_ManCo( p->pFrames, (f+k) * Gia_ManPoNum(p->pGia) + i );
        iLitClean = Bmcs_ManCollect_rec( p, Gia_ObjFaninId0p(p->pFrames, pObj) );
        iLitClean = Abc_LitNotCond( iLitClean, Gia_ObjFaninC0(pObj) );
        iLitClean = Gia_ManAppendCo( p->pClean, iLitClean );
        Gia_ManObj( p->pClean, Abc_Lit2Var(iLitClean) )->Value = Gia_ObjId(p->pFrames, pObj);
        Gia_ObjSetCopyArray( p->pFrames, Gia_ObjId(p->pFrames, pObj), iLitClean );
    }
    pNew = p->pClean; p->pClean = NULL;
    Gia_ManForEachObj( pNew, pObj, i )
        Gia_ObjSetCopyArray( p->pFrames, pObj->Value, -1 );
    return pNew;
}
Cnf_Dat_t * Bmcs_ManAddNewCnf( Bmcs_Man_t * p, int f, int nFramesAdd )
{
    abctime clk = Abc_Clock();
    Gia_Man_t * pNew = Bmcs_ManUnfold( p, f, nFramesAdd );
    Cnf_Dat_t * pCnf;
    Gia_Obj_t * pObj; 
    int i, iVar, * pMap;
    p->timeUnf += Abc_Clock() - clk;
    if ( pNew == NULL )
        return NULL;
    clk = Abc_Clock();
    pCnf = (Cnf_Dat_t *) Mf_ManGenerateCnf( pNew, 8, 1, 0, 0, 0 );
    pMap = ABC_FALLOC( int, Gia_ManObjNum(pNew) );
    pMap[0] = 0;
    Gia_ManForEachObj1( pNew, pObj, i )
    {
        if ( pCnf->pObj2Count[i] <= 0 && !Gia_ObjIsCi(pObj) )
            continue;
        iVar = Vec_IntEntry( &p->vFr2Sat, pObj->Value );
        if ( iVar == -1 )
            Vec_IntWriteEntry( &p->vFr2Sat, pObj->Value, (iVar = p->nSatVars++) );
        pMap[i] = iVar;
    }
    Gia_ManStop( pNew );
    for ( i = 0; i < pCnf->nLiterals; i++ )
        pCnf->pClauses[0][i] = Abc_Lit2LitV( pMap, pCnf->pClauses[0][i] );
    ABC_FREE( pMap );
    p->timeCnf += Abc_Clock() - clk;
    return pCnf;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bmcs_ManPrintFrame( Bmcs_Man_t * p, int f, int nClauses, int Solver, abctime clkStart )
{
    int fUnfinished = 0;
    if ( !p->pPars->fVerbose )
        return;
    Abc_Print( 1, "%4d %s : ", f,   fUnfinished ? "-" : "+" );
#ifndef ABC_USE_EXT_SOLVERS
    Abc_Print( 1, "Var =%8.0f.  ",  (double)satoko_varnum(p->pSats[0]) ); 
    Abc_Print( 1, "Cla =%9.0f.  ",  (double)satoko_clausenum(p->pSats[0]) );  
    Abc_Print( 1, "Learn =%9.0f.  ",(double)satoko_learntnum(p->pSats[0]) );  
    Abc_Print( 1, "Conf =%9.0f.  ", (double)satoko_conflictnum(p->pSats[0]) );  
#else
    Abc_Print( 1, "Var =%8.0f.  ",  (double)p->nSatVars ); 
    Abc_Print( 1, "Cla =%9.0f.  ",  (double)nClauses );  
#endif
    if ( p->pPars->nProcs > 1 )
        Abc_Print( 1, "S = %3d. ",  Solver );
    Abc_Print( 1, "%4.0f MB",       1.0*((int)Gia_ManMemory(p->pFrames) + Vec_IntMemory(&p->vFr2Sat))/(1<<20) );
    Abc_Print( 1, "%9.2f sec  ",    (float)(Abc_Clock() - clkStart)/(float)(CLOCKS_PER_SEC) );
    printf( "\n" );
    fflush( stdout );
}
void Bmcs_ManPrintTime( Bmcs_Man_t * p )
{
    abctime clkTotal = p->timeUnf + p->timeCnf + p->timeSat + p->timeOth;
    if ( !p->pPars->fVerbose )
        return;
    ABC_PRTP( "Unfolding     ", p->timeUnf,  clkTotal );
    ABC_PRTP( "CNF generation", p->timeCnf,  clkTotal );
    ABC_PRTP( "SAT solving   ", p->timeSat,  clkTotal );
    ABC_PRTP( "Other         ", p->timeOth,  clkTotal );
    ABC_PRTP( "TOTAL         ", clkTotal  ,  clkTotal );
}
Abc_Cex_t * Bmcs_ManGenerateCex( Bmcs_Man_t * p, int i, int f, int s )
{
    Abc_Cex_t * pCex = Abc_CexMakeTriv( Gia_ManRegNum(p->pGia), Gia_ManPiNum(p->pGia), Gia_ManPoNum(p->pGia), f*Gia_ManPoNum(p->pGia)+i );
    Gia_Obj_t * pObj;  int k;
    Gia_ManForEachPi( p->pFrames, pObj, k )
    {
        int iSatVar = Vec_IntEntry( &p->vFr2Sat, Gia_ObjId(p->pFrames, pObj) );
        if ( iSatVar > 0 && bmc_sat_solver_read_cex_varvalue(p->pSats[s], iSatVar) ) // 1 bit
        {
            int iCiId   = Vec_IntEntry( &p->vCiMap, 2*k+0 );
            int iFrame  = Vec_IntEntry( &p->vCiMap, 2*k+1 );
            Abc_InfoSetBit( pCex->pData, Gia_ManRegNum(p->pGia) + iFrame * Gia_ManPiNum(p->pGia) + iCiId );
        }
    }
    return pCex;
}
void Bmcs_ManAddCnf( Bmcs_Man_t * p, bmc_sat_solver * pSat, Cnf_Dat_t * pCnf )
{
    int i;
    for ( i = p->nSatVarsOld; i < p->nSatVars; i++ )
        bmc_sat_solver_addvar( pSat );
    for ( i = 0; i < pCnf->nClauses; i++ )
        if ( !bmc_sat_solver_addclause( pSat, pCnf->pClauses[i], pCnf->pClauses[i+1]-pCnf->pClauses[i] ) )
            assert( 0 );
}
int Bmcs_ManPerformOne( Gia_Man_t * pGia, Bmc_AndPar_t * pPars )
{
    abctime clkStart = Abc_Clock();
    Bmcs_Man_t * p = Bmcs_ManStart( pGia, pPars );
    int f, k = 0, i = Gia_ManPoNum(pGia), status, RetValue = -1, nClauses = 0;
    Abc_CexFreeP( &pGia->pCexSeq );
    for ( f = 0; !pPars->nFramesMax || f < pPars->nFramesMax; f += pPars->nFramesAdd )
    {
        Cnf_Dat_t * pCnf = Bmcs_ManAddNewCnf( p, f, pPars->nFramesAdd );
        if ( pCnf == NULL )
        {
            Bmcs_ManPrintFrame( p, f, nClauses, -1, clkStart );
            if( pPars->pFuncOnFrameDone)
                for ( k = 0; k < pPars->nFramesAdd; k++ )
                for ( i = 0; i < Gia_ManPoNum(pGia); i++ )
                    pPars->pFuncOnFrameDone(f+k, i, 0);
            continue;
        }
        nClauses += pCnf->nClauses;
        Bmcs_ManAddCnf( p, p->pSats[0], pCnf );
        p->nSatVarsOld = p->nSatVars;
        Cnf_DataFree( pCnf );
        assert( Gia_ManPoNum(p->pFrames) == (f + pPars->nFramesAdd) * Gia_ManPoNum(pGia) );
        for ( k = 0; k < pPars->nFramesAdd; k++ )
        {
            for ( i = 0; i < Gia_ManPoNum(pGia); i++ )
            {
                abctime clk = Abc_Clock();
                int iObj = Gia_ObjId( p->pFrames, Gia_ManCo(p->pFrames, (f+k) * Gia_ManPoNum(pGia) + i) );
                int iLit = Abc_Var2Lit( Vec_IntEntry(&p->vFr2Sat, iObj), 0 );
                if ( pPars->nTimeOut && (Abc_Clock() - clkStart)/CLOCKS_PER_SEC >= pPars->nTimeOut )
                    break;
                status = bmc_sat_solver_solve( p->pSats[0], &iLit, 1 );
                p->timeSat += Abc_Clock() - clk;
                if ( status == l_False ) // unsat
                {
                    if ( i == Gia_ManPoNum(pGia)-1 )
                        Bmcs_ManPrintFrame( p, f+k, nClauses, -1, clkStart );
                    if( pPars->pFuncOnFrameDone)
                        pPars->pFuncOnFrameDone(f+k, i, 0);
                    continue;
                }
                if ( status == l_True ) // sat
                {
                    RetValue = 0;
                    pPars->iFrame = f+k;
                    pGia->pCexSeq = Bmcs_ManGenerateCex( p, i, f+k, 0 );
                    pPars->nFailOuts++;
                    Bmcs_ManPrintFrame( p, f+k, nClauses, -1, clkStart );
                    if ( !pPars->fNotVerbose )
                    {
                        int nOutDigits = Abc_Base10Log( Gia_ManPoNum(pGia) );
                        Abc_Print( 1, "Output %*d was asserted in frame %2d (solved %*d out of %*d outputs).  ",  
                            nOutDigits, i, f+k, nOutDigits, pPars->nFailOuts, nOutDigits, Gia_ManPoNum(pGia) );
                        fflush( stdout );
                    }
                    if( pPars->pFuncOnFrameDone)
                        pPars->pFuncOnFrameDone(f+k, i, 1);
                }
                break;
            }
            if ( i < Gia_ManPoNum(pGia) || f+k == pPars->nFramesMax-1 )
                break;
        }
        if ( k < pPars->nFramesAdd )
            break;
    }
    p->timeOth = Abc_Clock() - clkStart - p->timeUnf - p->timeCnf - p->timeSat;
    if ( RetValue == -1 && !pPars->fNotVerbose )
        printf( "No output failed in %d frames.  ", f + (k < pPars->nFramesAdd ? k+1 : 0) );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clkStart );
    Bmcs_ManPrintTime( p );
    Bmcs_ManStop( p );
    return RetValue;
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
#ifndef ABC_USE_PTHREADS

int Bmcs_ManPerformMulti( Gia_Man_t * pGia, Bmc_AndPar_t * pPars ) { return Bmcs_ManPerformOne(pGia, pPars); }

#else // pthreads are used


typedef struct Par_ThData_t_
{
    bmc_sat_solver *  pSat;
    int         iLit;
    int         iThread;
    int         fWorking;
    int         status;
} Par_ThData_t;

void * Bmcs_ManWorkerThread( void * pArg )
{
    Par_ThData_t * pThData = (Par_ThData_t *)pArg;
    volatile int * pPlace = &pThData->fWorking;
    while ( 1 )
    {
        while ( *pPlace == 0 );
        assert( pThData->fWorking );
        if ( pThData->pSat == NULL )
        {
            pthread_exit( NULL );
            assert( 0 );
            return NULL;
        }

        pThData->status = bmc_sat_solver_solve( pThData->pSat, &pThData->iLit, 1 );

        //printf( "Thread %d finished with status %d\n", pThData->iThread, pThData->status );

        pThData->fWorking = 0;
    }
    assert( 0 );
    return NULL;
}

int Bmcs_ManPerform_Solve( Bmcs_Man_t * p, int iLit, pthread_t * WorkerThread, Par_ThData_t * ThData, int nProcs, int * pSolver )
{
    int i, status = -1;
    // set new problem
    for ( i = 0; i < nProcs; i++ )
    {
        ThData[i].iLit = iLit;
        assert( ThData[i].fWorking == 0 );
    }
    // start solvers on a new problem
    for ( i = 0; i < nProcs; i++ )
        ThData[i].fWorking = 1;
    // check if any of the solvers finished
    while ( i == nProcs )
    {
        for ( i = 0; i < nProcs; i++ )
        {
            if ( ThData[i].fWorking )
                continue;
            // set stop request
            p->fStopNow = 1;
            // remember status
            status = ThData[i].status;
            //printf( "Solver %d returned status %d.\n", i, status );
            *pSolver = i;
            break;
        }
    }
    // wait till threads finish
    while ( i < nProcs )
    {
        for ( i = 0; i < nProcs; i++ )
            if ( ThData[i].fWorking )
                break;
    }
    for ( i = 0; i < nProcs; i++ )
    {
        ThData[i].iLit = -1;
        assert( ThData[i].fWorking == 0 );
    }
    // reset stop request
    p->fStopNow = 0;
    return status;
}

int Bmcs_ManPerformMulti( Gia_Man_t * pGia, Bmc_AndPar_t * pPars )
{
    abctime clkStart = Abc_Clock();
    pthread_t WorkerThread[PAR_THR_MAX];
    Par_ThData_t ThData[PAR_THR_MAX];
    Bmcs_Man_t * p = Bmcs_ManStart( pGia, pPars );
    int f, k = 0, i = Gia_ManPoNum(pGia), status, RetValue = -1, nClauses = 0, Solver = 0;
    Abc_CexFreeP( &pGia->pCexSeq );
    // start threads
    for ( i = 0; i < pPars->nProcs; i++ )
    {
        ThData[i].pSat     = p->pSats[i];
        ThData[i].iLit     = -1;
        ThData[i].iThread  =  i;
        ThData[i].fWorking =  0;
        ThData[i].status   = -1;
        status = pthread_create( WorkerThread + i, NULL, Bmcs_ManWorkerThread, (void *)(ThData + i) );  assert( status == 0 );
    }
    // solve properties in each timeframe
    for ( f = 0; !pPars->nFramesMax || f < pPars->nFramesMax; f += pPars->nFramesAdd )
    {
        Cnf_Dat_t * pCnf = Bmcs_ManAddNewCnf( p, f, pPars->nFramesAdd );
        if ( pCnf == NULL )
        {
            Bmcs_ManPrintFrame( p, f, nClauses, 0, clkStart );
            if( pPars->pFuncOnFrameDone )
                for ( k = 0; k < pPars->nFramesAdd; k++ )
                for ( i = 0; i < Gia_ManPoNum(pGia); i++ )
                    pPars->pFuncOnFrameDone(f+k, i, 0);
            continue;
        }
        // load CNF into solvers
        nClauses += pCnf->nClauses;
        for ( i = 0; i < pPars->nProcs; i++ )
            Bmcs_ManAddCnf( p, p->pSats[i], pCnf );
        p->nSatVarsOld = p->nSatVars;
        Cnf_DataFree( pCnf );
        // solve outputs
        assert( Gia_ManPoNum(p->pFrames) == (f + pPars->nFramesAdd) * Gia_ManPoNum(pGia) );
        for ( k = 0; k < pPars->nFramesAdd; k++ )
        {
            for ( i = 0; i < Gia_ManPoNum(pGia); i++ )
            {
                abctime clk = Abc_Clock();
                int iObj = Gia_ObjId( p->pFrames, Gia_ManCo(p->pFrames, (f+k) * Gia_ManPoNum(pGia) + i) );
                int iLit = Abc_Var2Lit( Vec_IntEntry(&p->vFr2Sat, iObj), 0 );
                if ( pPars->nTimeOut && (Abc_Clock() - clkStart)/CLOCKS_PER_SEC >= pPars->nTimeOut )
                    break;
                status = Bmcs_ManPerform_Solve( p, iLit, WorkerThread, ThData, pPars->nProcs, &Solver );
                p->timeSat += Abc_Clock() - clk;
                if ( status == l_False ) // unsat
                {
                    if ( i == Gia_ManPoNum(pGia)-1 )
                        Bmcs_ManPrintFrame( p, f+k, nClauses, Solver, clkStart );
                    if( pPars->pFuncOnFrameDone )
                        pPars->pFuncOnFrameDone(f+k, i, 0);
                    continue;
                }
                if ( status == l_True ) // sat
                {
                    RetValue = 0;
                    pPars->iFrame = f+k;
                    pGia->pCexSeq = Bmcs_ManGenerateCex( p, i, f+k, Solver );
                    pPars->nFailOuts++;
                    Bmcs_ManPrintFrame( p, f+k, nClauses, Solver, clkStart );
                    if ( !pPars->fNotVerbose )
                    {
                        int nOutDigits = Abc_Base10Log( Gia_ManPoNum(pGia) );
                        Abc_Print( 1, "Output %*d was asserted in frame %2d (solved %*d out of %*d outputs).  ",  
                            nOutDigits, i, f+k, nOutDigits, pPars->nFailOuts, nOutDigits, Gia_ManPoNum(pGia) );
                        fflush( stdout );
                    }
                    if( pPars->pFuncOnFrameDone )
                        pPars->pFuncOnFrameDone(f+k, i, 1);
                }
                break;
            }
            if ( i < Gia_ManPoNum(pGia) || f+k == pPars->nFramesMax-1 )
                break;
        }
        if ( k < pPars->nFramesAdd )
            break;
    }
    // stop threads
    for ( i = 0; i < pPars->nProcs; i++ )
    {
        assert( !ThData[i].fWorking );
        ThData[i].pSat = NULL;
        ThData[i].fWorking = 1;
    }
    p->timeOth = Abc_Clock() - clkStart - p->timeUnf - p->timeCnf - p->timeSat;
    if ( RetValue == -1 && !pPars->fNotVerbose )
        printf( "No output failed in %d frames.  ", f + (k < pPars->nFramesAdd ? k+1 : 0) );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clkStart );
    Bmcs_ManPrintTime( p );
    Bmcs_ManStop( p );
    return RetValue;
}

#endif // pthreads are used


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bmcs_ManPerform( Gia_Man_t * pGia, Bmc_AndPar_t * pPars ) 
{ 
    assert( pPars->nProcs < PAR_THR_MAX );
    if ( pPars->nProcs == 1 )
        return Bmcs_ManPerformOne( pGia, pPars );
    else
        return Bmcs_ManPerformMulti( pGia, pPars );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

