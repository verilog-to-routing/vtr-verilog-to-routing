/**CFile****************************************************************

  FileName    [bmcBmcG.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based bounded model checking.]

  Synopsis    [New BMC package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - July 20, 2017.]

  Revision    [$Id: bmcBmcG.c,v 1.00 2017/07/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bmc.h"
#include "sat/cnf/cnf.h"
#include "sat/glucose/AbcGlucose.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define PAR_THR_MAX 100

typedef struct Bmcg_Man_t_ Bmcg_Man_t;
struct Bmcg_Man_t_
{
    Bmc_AndPar_t *    pPars;               // parameters
    Gia_Man_t *       pGia;                // user's AIG
    Gia_Man_t *       pFrames;             // unfolded AIG (pFrames->vCopies point to pClean)
    Gia_Man_t *       pClean;              // incremental AIG (pClean->Value point to pFrames)
    Vec_Ptr_t         vGia2Fr;             // copies of GIA in each timeframe
    Vec_Int_t         vFr2Sat;             // mapping of objects in pFrames into SAT variables
    Vec_Int_t         vCiMap;              // maps CIs of pFrames into CIs/frames of GIA
    bmcg_sat_solver * pSats[PAR_THR_MAX];  // concurrent SAT solvers
    int               nSatVars;            // number of SAT variables used
    int               nOldFrPis;           // number of primary inputs
    int               nOldFrPos;           // number of primary output
    int               fStopNow;            // signal when it is time to stop
    abctime           timeUnf;             // runtime of unfolding
    abctime           timeCnf;             // runtime of CNF generation
    abctime           timeSmp;             // runtime of CNF simplification
    abctime           timeSat;             // runtime of the solvers
    abctime           timeOth;             // other runtime
};

static inline int * Bmcg_ManCopies( Bmcg_Man_t * p, int f ) { return (int*)Vec_PtrEntry(&p->vGia2Fr, f); }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Bmcg_Man_t * Bmcg_ManStart( Gia_Man_t * pGia, Bmc_AndPar_t * pPars )
{
    Bmcg_Man_t * p = ABC_CALLOC( Bmcg_Man_t, 1 ); 
    int i, Lit = Abc_Var2Lit( 0, 1 );
//    opts.conf_limit = pPars->nConfLimit;
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
    for ( i = 0; i < p->pPars->nProcs; i++ )
    {
        p->pSats[i] = bmcg_sat_solver_start();  
//        p->pSats[i]->SolverType = i;
        bmcg_sat_solver_addvar( p->pSats[i] );
        bmcg_sat_solver_addclause( p->pSats[i], &Lit, 1 );  
        bmcg_sat_solver_set_stop( p->pSats[i], &p->fStopNow );
    }
    p->nSatVars = 1;
    return p;
}
void Bmcg_ManStop( Bmcg_Man_t * p )
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
            bmcg_sat_solver_stop( p->pSats[i] );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    [Incremental unfolding.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bmcg_ManUnfold_rec( Bmcg_Man_t * p, int iObj, int f )
{
    Gia_Obj_t * pObj; 
    int iLit = 0, * pCopies = Bmcg_ManCopies( p, f );
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
            iLit = Bmcg_ManUnfold_rec( p, Gia_ObjFaninId0p(p->pGia, pObj), f-1 );
            iLit = Abc_LitNotCond( iLit, Gia_ObjFaninC0(pObj) );
        }
    }
    else if ( Gia_ObjIsAnd(pObj) )
    {
        iLit = Bmcg_ManUnfold_rec( p, Gia_ObjFaninId0(pObj, iObj), f );
        iLit = Abc_LitNotCond( iLit, Gia_ObjFaninC0(pObj) );
        if ( iLit > 0 )
        {
            int iNew;
            iNew = Bmcg_ManUnfold_rec( p, Gia_ObjFaninId1(pObj, iObj), f );
            iNew = Abc_LitNotCond( iNew, Gia_ObjFaninC1(pObj) );
            iLit = Gia_ManHashAnd( p->pFrames, iLit, iNew );
        }
    }
    else assert( 0 );
    return (pCopies[iObj] = iLit);
}
int Bmcg_ManCollect_rec( Bmcg_Man_t * p, int iObj )
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
        int iLit0 = Bmcg_ManCollect_rec( p, Gia_ObjFaninId0(pObj, iObj) );
        int iLit1 = Bmcg_ManCollect_rec( p, Gia_ObjFaninId1(pObj, iObj) );
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
Gia_Man_t * Bmcg_ManUnfold( Bmcg_Man_t * p, int f, int nFramesAdd )
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
        pCopies = Bmcg_ManCopies( p, f+k );
        //memset( pCopies, 0xFF, sizeof(int)*Gia_ManObjNum(p->pGia) );  
        pCopies[0] = 0;
        Gia_ManForEachPo( p->pGia, pObj, i )
        {
            iLitFrame = Bmcg_ManUnfold_rec( p, Gia_ObjFaninId0p(p->pGia, pObj), f+k );
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
        iLitClean = Bmcg_ManCollect_rec( p, Gia_ObjFaninId0p(p->pFrames, pObj) );
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
Cnf_Dat_t * Bmcg_ManAddNewCnf( Bmcg_Man_t * p, int f, int nFramesAdd )
{
    abctime clk = Abc_Clock();
    Gia_Man_t * pNew = Bmcg_ManUnfold( p, f, nFramesAdd );
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
void Bmcg_ManPrintFrame( Bmcg_Man_t * p, int f, int nClauses, int Solver, abctime clkStart )
{
    int fUnfinished = 0;
    if ( !p->pPars->fVerbose )
        return;
    Abc_Print( 1, "%4d %s : ", f,   fUnfinished ? "-" : "+" );
//    Abc_Print( 1, "Var =%8.0f.  ",  (double)p->nSatVars ); 
//    Abc_Print( 1, "Cla =%9.0f.  ",  (double)nClauses );  
    Abc_Print( 1, "Var =%8.0f.  ",  (double)(bmcg_sat_solver_varnum(p->pSats[0])-bmcg_sat_solver_elim_varnum(p->pSats[0])) ); 
    Abc_Print( 1, "Cla =%9.0f.  ",  (double)bmcg_sat_solver_clausenum(p->pSats[0]) );  
    Abc_Print( 1, "Learn =%9.0f.  ",(double)bmcg_sat_solver_learntnum(p->pSats[0]) );  
    Abc_Print( 1, "Conf =%9.0f.  ", (double)bmcg_sat_solver_conflictnum(p->pSats[0]) );  
    if ( p->pPars->nProcs > 1 )
        Abc_Print( 1, "S = %3d. ",  Solver );
    Abc_Print( 1, "%4.0f MB",       1.0*((int)Gia_ManMemory(p->pFrames) + Vec_IntMemory(&p->vFr2Sat))/(1<<20) );
    Abc_Print( 1, "%9.2f sec  ",    (float)(Abc_Clock() - clkStart)/(float)(CLOCKS_PER_SEC) );
    printf( "\n" );
    fflush( stdout );
}
void Bmcg_ManPrintTime( Bmcg_Man_t * p )
{
    abctime clkTotal = p->timeUnf + p->timeCnf + p->timeSmp + p->timeSat + p->timeOth;
    if ( !p->pPars->fVerbose )
        return;
    ABC_PRTP( "Unfolding     ", p->timeUnf,  clkTotal );
    ABC_PRTP( "CNF generation", p->timeCnf,  clkTotal );
    ABC_PRTP( "CNF simplify  ", p->timeSmp,  clkTotal );
    ABC_PRTP( "SAT solving   ", p->timeSat,  clkTotal );
    ABC_PRTP( "Other         ", p->timeOth,  clkTotal );
    ABC_PRTP( "TOTAL         ", clkTotal  ,  clkTotal );
}
Abc_Cex_t * Bmcg_ManGenerateCex( Bmcg_Man_t * p, int i, int f, int s )
{
    Abc_Cex_t * pCex = Abc_CexMakeTriv( Gia_ManRegNum(p->pGia), Gia_ManPiNum(p->pGia), Gia_ManPoNum(p->pGia), f*Gia_ManPoNum(p->pGia)+i );
    Gia_Obj_t * pObj;  int k;
    Gia_ManForEachPi( p->pFrames, pObj, k )
    {
        int iSatVar = Vec_IntEntry( &p->vFr2Sat, Gia_ObjId(p->pFrames, pObj) );
        if ( iSatVar > 0 && bmcg_sat_solver_read_cex_varvalue(p->pSats[s], iSatVar) ) // 1 bit
        {
            int iCiId   = Vec_IntEntry( &p->vCiMap, 2*k+0 );
            int iFrame  = Vec_IntEntry( &p->vCiMap, 2*k+1 );
            Abc_InfoSetBit( pCex->pData, Gia_ManRegNum(p->pGia) + iFrame * Gia_ManPiNum(p->pGia) + iCiId );
        }
    }
    return pCex;
}
void Bmcg_ManAddCnf( Bmcg_Man_t * p, bmcg_sat_solver * pSat, Cnf_Dat_t * pCnf )
{
    int i, iSatVar;
    abctime clk = Abc_Clock();
    bmcg_sat_solver_set_nvars( pSat, p->nSatVars );
    if ( p->pPars->fUseEliminate )
    {
        for ( i = p->nOldFrPis; i < Gia_ManPiNum(p->pFrames); i++ )
        {
            Gia_Obj_t * pObj = Gia_ManPi( p->pFrames, i );
            int iSatVar = Vec_IntEntry( &p->vFr2Sat, Gia_ObjId(p->pFrames, pObj) );
            if ( iSatVar > 0 )
                bmcg_sat_solver_var_set_frozen( pSat, iSatVar, 1 );
        }
        for ( i = p->nOldFrPos; i < Gia_ManPoNum(p->pFrames); i++ )
        {
            Gia_Obj_t * pObj = Gia_ManPo( p->pFrames, i );
            int iSatVar = Vec_IntEntry( &p->vFr2Sat, Gia_ObjId(p->pFrames, pObj) );
            if ( iSatVar > 0 )
                bmcg_sat_solver_var_set_frozen( pSat, iSatVar, 1 );
        }
        p->nOldFrPis = Gia_ManPiNum(p->pFrames);
        p->nOldFrPos = Gia_ManPoNum(p->pFrames);
    }
    for ( i = 0; i < pCnf->nClauses; i++ )
        if ( !bmcg_sat_solver_addclause( pSat, pCnf->pClauses[i], pCnf->pClauses[i+1]-pCnf->pClauses[i] ) )
            assert( 0 );
    if ( !p->pPars->fUseEliminate )
        return;
    bmcg_sat_solver_eliminate( pSat, 0 );
    Vec_IntForEachEntry( &p->vFr2Sat, iSatVar, i )
        if ( iSatVar > 0 && bmcg_sat_solver_var_is_elim(pSat, iSatVar) )
            Vec_IntWriteEntry( &p->vFr2Sat, i, -1 );
    p->timeSmp += Abc_Clock() - clk;
}
int Bmcg_ManPerformOne( Gia_Man_t * pGia, Bmc_AndPar_t * pPars )
{
    abctime clkStart = Abc_Clock();
    Bmcg_Man_t * p = Bmcg_ManStart( pGia, pPars );
    int f, k = 0, i = Gia_ManPoNum(pGia), status, RetValue = -1, nClauses = 0;
    Abc_CexFreeP( &pGia->pCexSeq );
    for ( f = 0; !pPars->nFramesMax || f < pPars->nFramesMax; f += pPars->nFramesAdd )
    {
        Cnf_Dat_t * pCnf = Bmcg_ManAddNewCnf( p, f, pPars->nFramesAdd );
        if ( pCnf == NULL )
        {
            Bmcg_ManPrintFrame( p, f, nClauses, -1, clkStart );
            if( pPars->pFuncOnFrameDone )
                for ( k = 0; k < pPars->nFramesAdd; k++ )
                for ( i = 0; i < Gia_ManPoNum(pGia); i++ )
                    pPars->pFuncOnFrameDone(f+k, i, 0);
            continue;
        }
        nClauses += pCnf->nClauses;
        Bmcg_ManAddCnf( p, p->pSats[0], pCnf );
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
                status = bmcg_sat_solver_solve( p->pSats[0], &iLit, 1 );
                p->timeSat += Abc_Clock() - clk;
                if ( status == -1 ) // unsat
                {
                    if ( i == Gia_ManPoNum(pGia)-1 )
                        Bmcg_ManPrintFrame( p, f+k, nClauses, -1, clkStart );
                    if( pPars->pFuncOnFrameDone)
                        pPars->pFuncOnFrameDone(f+k, i, 0);
                    continue;
                }
                if ( status == 1 ) // sat
                {
                    RetValue = 0;
                    pPars->iFrame = f+k;
                    pGia->pCexSeq = Bmcg_ManGenerateCex( p, i, f+k, 0 );
                    pPars->nFailOuts++;
                    Bmcg_ManPrintFrame( p, f+k, nClauses, -1, clkStart );
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
    p->timeOth = Abc_Clock() - clkStart - p->timeUnf - p->timeCnf - p->timeSmp - p->timeSat;
    if ( RetValue == -1 && !pPars->fNotVerbose )
        printf( "No output failed in %d frames.  ", f + (k < pPars->nFramesAdd ? k+1 : 0) );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clkStart );
    Bmcg_ManPrintTime( p );
    Bmcg_ManStop( p );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bmcg_ManPerform( Gia_Man_t * pGia, Bmc_AndPar_t * pPars ) 
{ 
    pPars->nProcs = 1;
    return Bmcg_ManPerformOne( pGia, pPars );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

