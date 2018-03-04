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
#include "sat/cnf/cnf.h"
#include "sat/bsat/satSolver.h"
#include "misc/util/utilTruth.h"
//#include "bdd/cudd/cuddInt.h"

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

int Cec_GiaSplitTest( Gia_Man_t * p, int nProcs, int nTimeOut, int nIterMax, int LookAhead, int fVerbose, int fVeryVerbose, int fSilent ) { return -1; }

#else // pthreads are used

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

#if 0 // BDD code

/**Function*************************************************************

  Synopsis    [Permute primary inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdManager * Gia_ManBuildBdd( Gia_Man_t * p, Vec_Ptr_t ** pvNodes, int nSkip )
{
    abctime clk = Abc_Clock();
    DdManager * dd;
    DdNode * bBdd, * bBdd0, * bBdd1;
    Vec_Ptr_t * vNodes;
    Gia_Obj_t * pObj;
    int i;
    vNodes = Vec_PtrStart( Gia_ManObjNum(p) );
    dd = Cudd_Init( Gia_ManPiNum(p), 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
//    Cudd_AutodynEnable( dd,  CUDD_REORDER_SYMM_SIFT );
    bBdd = Cudd_ReadLogicZero(dd); Cudd_Ref( bBdd );
    Vec_PtrWriteEntry( vNodes, 0, bBdd );  
    Gia_ManForEachPi( p, pObj, i )
    {
        bBdd = i > nSkip ? Cudd_bddIthVar(dd, i) : Cudd_ReadLogicZero(dd); Cudd_Ref( bBdd );
        Vec_PtrWriteEntry( vNodes, Gia_ObjId(p, pObj), bBdd );
    }
    Gia_ManForEachAnd( p, pObj, i )
    {
        bBdd0 = Cudd_NotCond( (DdNode *)Vec_PtrEntry(vNodes, Gia_ObjFaninId0(pObj, i)), Gia_ObjFaninC0(pObj) );
        bBdd1 = Cudd_NotCond( (DdNode *)Vec_PtrEntry(vNodes, Gia_ObjFaninId1(pObj, i)), Gia_ObjFaninC1(pObj) );
        bBdd = Cudd_bddAnd( dd, bBdd0, bBdd1 ); Cudd_Ref( bBdd );
        Vec_PtrWriteEntry( vNodes, Gia_ObjId(p, pObj), bBdd );
        if ( i % 10 == 0 )
            printf( "%d ", i );
//        if ( i == 3000 )
//            break;
    }
    printf( "\n" );
    Gia_ManForEachPo( p, pObj, i )
    {
        bBdd = Cudd_NotCond( (DdNode *)Vec_PtrEntry(vNodes, Gia_ObjFaninId0(pObj, Gia_ObjId(p, pObj))), Gia_ObjFaninC0(pObj) );  Cudd_Ref( bBdd );
        Vec_PtrWriteEntry( vNodes, Gia_ObjId(p, pObj), bBdd );
    }
    if ( bBdd == Cudd_ReadLogicZero(dd) )
        printf( "Equivalent!\n" );
    else
        printf( "Not tquivalent!\n" );
    if ( pvNodes )
        *pvNodes = vNodes;
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    return dd;
}
void Gia_ManDerefBdd( DdManager * dd, Vec_Ptr_t * vNodes )
{
    DdNode * bBdd;
    int i;
    Vec_PtrForEachEntry( DdNode *, vNodes, bBdd, i )
        if ( bBdd )
            Cudd_RecursiveDeref( dd, bBdd );
    if ( Cudd_CheckZeroRef(dd) > 0 )
        printf( "The number of referenced nodes = %d\n", Cudd_CheckZeroRef(dd) );
    Cudd_PrintInfo( dd, stdout );
    Cudd_Quit( dd );
}
void Gia_ManBuildBddTest( Gia_Man_t * p )
{
    Vec_Ptr_t * vNodes;
    DdManager * dd = Gia_ManBuildBdd( p, &vNodes, 50 );
    Gia_ManDerefBdd( dd, vNodes );
}

#endif // BDD code

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_GiaSplitExplore( Gia_Man_t * p )
{
    Gia_Obj_t * pObj, * pFan0, * pFan1;
    int i, Counter = 0;
    assert( p->pMuxes == NULL );
    ABC_FREE( p->pRefs );
    Gia_ManCreateRefs( p ); 
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( !Gia_ObjRecognizeExor(pObj, &pFan0, &pFan1) )
            continue;
        if ( Gia_ObjRefNum(p, Gia_ObjFanin0(pObj)) > 1 && 
             Gia_ObjRefNum(p, Gia_ObjFanin1(pObj)) > 1 )
             continue;
        printf( "%5d : ", Counter++ );
        printf( "%2d %2d    ", Gia_ObjRefNum(p, Gia_Regular(pFan0)),  Gia_ObjRefNum(p, Gia_Regular(pFan1)) );
        printf( "%2d %2d  \n", Gia_ObjRefNum(p, Gia_ObjFanin0(pObj)), Gia_ObjRefNum(p, Gia_ObjFanin1(pObj)) );
    }
}

/**Function*************************************************************

  Synopsis    [Find cofactoring variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Gia_PermuteSpecialOrder( Gia_Man_t * p )
{
    Vec_Int_t * vPerm;
    Gia_Obj_t * pObj;
    int i, * pOrder;
    Gia_ManCreateRefs( p );
    vPerm = Vec_IntAlloc( Gia_ManPiNum(p) );
    Gia_ManForEachPi( p, pObj, i )
        Vec_IntPush( vPerm, Gia_ObjRefNum(p, pObj) );
    pOrder = Abc_QuickSortCost( Vec_IntArray(vPerm), Vec_IntSize(vPerm), 1 );
    Vec_IntFree( vPerm );
    return pOrder;
}
Gia_Man_t * Gia_PermuteSpecial( Gia_Man_t * p )
{
    Gia_Man_t * pNew;
    Vec_Int_t * vPerm;
    int * pOrder = Gia_PermuteSpecialOrder( p );
    vPerm = Vec_IntAllocArray( pOrder, Gia_ManPiNum(p) );
    pNew = Gia_ManDupPerm( p, vPerm );
    Vec_IntFree( vPerm );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Find cofactoring variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_SplitCofVar2( Gia_Man_t * p, int * pnFanouts, int * pnCost )
{
    Gia_Obj_t * pObj;
    int i, iBest = -1, CostBest = -1;
    if ( p->pRefs == NULL )
        Gia_ManCreateRefs( p );
    Gia_ManForEachPi( p, pObj, i )
        if ( CostBest < Gia_ObjRefNum(p, pObj) )
            iBest = i, CostBest = Gia_ObjRefNum(p, pObj);
    assert( iBest >= 0 );
    *pnFanouts = Gia_ObjRefNum(p, Gia_ManPi(p, iBest));
    *pnCost = -1;
    return iBest;
}
int Gia_SplitCofVar( Gia_Man_t * p, int LookAhead, int * pnFanouts, int * pnCost )
{
    Gia_Man_t * pPart;
    int Cost0, Cost1, CostBest = ABC_INFINITY;
    int * pOrder, i, iBest = -1;
    if ( LookAhead == 1 )
        return Gia_SplitCofVar2( p, pnFanouts, pnCost );
    pOrder = Gia_PermuteSpecialOrder( p );
    LookAhead = Abc_MinInt( LookAhead, Gia_ManPiNum(p) );
    for ( i = 0; i < LookAhead; i++ )
    {
        pPart = Gia_ManDupCofactorVar( p, pOrder[i], 0 );
        Cost0 = Gia_ManAndNum(pPart);
        Gia_ManStop( pPart );

        pPart = Gia_ManDupCofactorVar( p, pOrder[i], 1 );
        Cost1 = Gia_ManAndNum(pPart);
        Gia_ManStop( pPart );

        if ( CostBest > Cost0 + Cost1 )
            CostBest = Cost0 + Cost1, iBest = pOrder[i];

/*
        pPart = Gia_ManDupExist( p, pOrder[i] );
        printf( "%2d : Var = %4d  Refs = %3d  %6d %6d -> %6d    %6d -> %6d\n", 
            i, pOrder[i], Gia_ObjRefNum(p, Gia_ManPi(p, pOrder[i])), 
            Cost0, Cost1, Cost0+Cost1, Gia_ManAndNum(p), Gia_ManAndNum(pPart) );
        Gia_ManStop( pPart );

        printf( "%2d : Var = %4d  Refs = %3d  %6d %6d -> %6d\n", 
            i, pOrder[i], Gia_ObjRefNum(p, Gia_ManPi(p, pOrder[i])), 
            Cost0, Cost1, Cost0+Cost1 );
*/
    }
    ABC_FREE( pOrder );
    assert( iBest >= 0 );
    *pnFanouts = Gia_ObjRefNum(p, Gia_ManPi(p, iBest));
    *pnCost = CostBest;
    return iBest;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Cec_SplitDeriveModel( Gia_Man_t * p, Cnf_Dat_t * pCnf, sat_solver * pSat )
{
    Abc_Cex_t * pCex;
    Gia_Obj_t * pObj;
    int i, iLit, * pModel;
    pModel = ABC_CALLOC( int, Gia_ManPiNum(p) );
    Gia_ManForEachPi( p, pObj, i )
        pModel[i] = sat_solver_var_value(pSat, pCnf->pVarNums[Gia_ObjId(p, pObj)]);
    if ( p->vCofVars )
        Vec_IntForEachEntry( p->vCofVars, iLit, i )
            pModel[Abc_Lit2Var(iLit)] = !Abc_LitIsCompl(iLit);
    pCex = Abc_CexCreate( 0, Gia_ManPiNum(p), pModel, 0, 0, 0 );
    ABC_FREE( pModel );
    return pCex;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Cnf_Dat_t * Cec_GiaDeriveGiaRemapped( Gia_Man_t * p )
{
    Cnf_Dat_t * pCnf;
    Aig_Man_t * pAig = Gia_ManToAigSimple( p );
    pAig->nRegs = 0;
    pCnf = Cnf_Derive( pAig, 0 );//Aig_ManCoNum(pAig) );
    Aig_ManStop( pAig );
    return pCnf;
}
static inline sat_solver * Cec_GiaDeriveSolver( Gia_Man_t * p, Cnf_Dat_t * pCnf, int nTimeOut )
{
    sat_solver * pSat;
    int i;
    pSat = sat_solver_new();
    sat_solver_setnvars( pSat, pCnf->nVars );
    for ( i = 0; i < pCnf->nClauses; i++ )
        if ( !sat_solver_addclause( pSat, pCnf->pClauses[i], pCnf->pClauses[i+1] ) )
        {
            // the problem is UNSAT
            sat_solver_delete( pSat );
            return NULL;
        }
    sat_solver_set_runtime_limit( pSat, nTimeOut ? nTimeOut * CLOCKS_PER_SEC + Abc_Clock(): 0 );
    return pSat;
}
static inline int Cnf_GiaSolveOne( Gia_Man_t * p, Cnf_Dat_t * pCnf, int nTimeOut, int * pnVars, int * pnConfs )
{
    int status;
    sat_solver * pSat = Cec_GiaDeriveSolver( p, pCnf, nTimeOut );
    if ( pSat == NULL )
    {
        *pnVars = 0;
        *pnConfs = 0;
        return 1;
    }
    status   = sat_solver_solve( pSat, NULL, NULL, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
    *pnVars  = sat_solver_nvars( pSat );
    *pnConfs = sat_solver_nconflicts( pSat );
    if ( status == l_True )
        p->pCexComb = Cec_SplitDeriveModel( p, pCnf, pSat );
    sat_solver_delete( pSat );
    if ( status == l_Undef )
        return -1;
    if ( status == l_False )
        return 1;
    return 0;
}
static inline void Cec_GiaSplitClean( Vec_Ptr_t * vStack )
{
    Gia_Man_t * pNew;
    int i;
    Vec_PtrForEachEntry( Gia_Man_t *, vStack, pNew, i )
        Gia_ManStop( pNew );
    Vec_PtrFree( vStack );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_GiaSplitPrint( int nIter, int Depth, int nVars, int nConfs, int fStatus, double Prog, abctime clk )
{
    printf( "%4d : ",             nIter );
    printf( "Depth =%3d  ",       Depth );
    printf( "SatVar =%7d  ",      nVars );
    printf( "SatConf =%7d   ",    nConfs );
    printf( "%s   ",              fStatus ? (fStatus == 1 ? "UNSAT    " : "UNDECIDED") : "SAT      " );
    printf( "Solved %8.4f %%   ", 100*Prog );
    Abc_PrintTime( 1, "Time", clk );
    //ABC_PRTr( "Time", Abc_Clock()-clk );
    fflush( stdout );
}
void Cec_GiaSplitPrintRefs( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i;
    if ( p->pRefs == NULL )
        Gia_ManCreateRefs( p ); 
    Gia_ManForEachPi( p, pObj, i )
        printf( "%d ", Gia_ObjRefNum(p, pObj) );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cec_GiaSplitTest2( Gia_Man_t * p, int nProcs, int nTimeOut, int nIterMax, int LookAhead, int fVerbose, int fVeryVerbose, int fSilent )
{
    abctime clkTotal = Abc_Clock();
    Vec_Ptr_t * vStack;
    Cnf_Dat_t * pCnf;
    int nSatVars, nSatConfs;
    int nIter, status, RetValue = -1;
    double Progress = 0;
    // check the problem
    pCnf = Cec_GiaDeriveGiaRemapped( p );
    status = Cnf_GiaSolveOne( p, pCnf, nTimeOut, &nSatVars, &nSatConfs );
    Cnf_DataFree( pCnf );
    if ( fVerbose )
        Cec_GiaSplitPrint( 0, 0, nSatVars, nSatConfs, status, Progress, Abc_Clock() - clkTotal );
    if ( status == 0 )
    {
		if ( !fSilent )
        printf( "The problem is SAT without cofactoring.\n" );
        return 0;
    }
    if ( status == 1 )
    {
		if ( !fSilent )
        printf( "The problem is UNSAT without cofactoring.\n" );
        return 1;
    }
    assert( status == -1 );
    // create local copy
    vStack = Vec_PtrAlloc( 1000 );
    Vec_PtrPush( vStack, Gia_ManDup(p) );
    // start with the current problem
    for ( nIter = 1; Vec_PtrSize(vStack) > 0; nIter++ )
    {
        // get the last AIG
        Gia_Man_t * pLast = (Gia_Man_t *)Vec_PtrPop( vStack );
        // determine cofactoring variable
        int Depth = 1 + (pLast->vCofVars ? Vec_IntSize(pLast->vCofVars) : 0);
        int nFanouts, Cost, iVar  = Gia_SplitCofVar( pLast, LookAhead, &nFanouts, &Cost );
        // cofactor
        Gia_Man_t * pPart = Gia_ManDupCofactorVar( pLast, iVar, 0 );
        if ( pLast->vCofVars == NULL )
            pLast->vCofVars = Vec_IntAlloc( 100 );
        // print results
        if ( fVeryVerbose )
        {
//            Cec_GiaSplitPrintRefs( pLast );
            printf( "Var = %5d. Fanouts = %5d. Cost = %8d.  AndBefore = %6d.  AndAfter = %6d.\n", 
                iVar, nFanouts, Cost, Gia_ManAndNum(pLast), Gia_ManAndNum(pPart) );
//            Cec_GiaSplitPrintRefs( pPart );
        }
        // create variable
        pPart->vCofVars = Vec_IntAlloc( Vec_IntSize(pLast->vCofVars) + 1 );
        Vec_IntAppend( pPart->vCofVars, pLast->vCofVars );
        Vec_IntPush( pPart->vCofVars, Abc_Var2Lit(iVar, 1) );
        // solve the problem
        pCnf = Cec_GiaDeriveGiaRemapped( pPart );
        status = Cnf_GiaSolveOne( pPart, pCnf, nTimeOut, &nSatVars, &nSatConfs );
        Cnf_DataFree( pCnf );
        if ( status == 1 )
            Progress += 1.0 / pow(2, Depth);
        if ( fVerbose ) 
            Cec_GiaSplitPrint( nIter, Depth, nSatVars, nSatConfs, status, Progress, Abc_Clock() - clkTotal );
        if ( status == 0 ) // SAT
        {
            p->pCexComb = pPart->pCexComb;  pPart->pCexComb = NULL;
            Gia_ManStop( pLast );
            Gia_ManStop( pPart );
            RetValue = 0;
            break;
        }
        if ( status == 1 ) // UNSAT
            Gia_ManStop( pPart );
        else               // UNDEC
            Vec_PtrPush( vStack, pPart );
        // cofactor
        pPart = Gia_ManDupCofactorVar( pLast, iVar, 1 );
        // create variable
        pPart->vCofVars = Vec_IntAlloc( Vec_IntSize(pLast->vCofVars) + 1 );
        Vec_IntAppend( pPart->vCofVars, pLast->vCofVars );
        Vec_IntPush( pPart->vCofVars, Abc_Var2Lit(iVar, 0) );
        Gia_ManStop( pLast );
        // solve the problem
        pCnf = Cec_GiaDeriveGiaRemapped( pPart );
        status = Cnf_GiaSolveOne( pPart, pCnf, nTimeOut, &nSatVars, &nSatConfs );
        Cnf_DataFree( pCnf );
        if ( status == 1 )
            Progress += 1.0 / pow(2, Depth);
        if ( fVerbose )
            Cec_GiaSplitPrint( nIter, Depth, nSatVars, nSatConfs, status, Progress, Abc_Clock() - clkTotal );
        if ( status == 0 ) // SAT
        {
            p->pCexComb = pPart->pCexComb;  pPart->pCexComb = NULL;
            Gia_ManStop( pPart );
            RetValue = 0;
            break;
        }
        if ( status == 1 ) // UNSAT
            Gia_ManStop( pPart );
        else               // UNDEC
            Vec_PtrPush( vStack, pPart );
        if ( nIterMax && nIter >= nIterMax )
            break;
    }
    if ( Vec_PtrSize(vStack) == 0 )
        RetValue = 1;
    // finish
    Cec_GiaSplitClean( vStack );
	if ( !fSilent )
	{
		if ( RetValue == 0 )
			printf( "Problem is SAT " );
		else if ( RetValue == 1 )
			printf( "Problem is UNSAT " );
		else if ( RetValue == -1 )
			printf( "Problem is UNDECIDED " );
		else assert( 0 );
		printf( "after %d case-splits.  ", nIter );
		Abc_PrintTime( 1, "Time", Abc_Clock() - clkTotal );
		fflush( stdout );
	}
    return RetValue;
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
    Cnf_Dat_t * pCnf;
    int         iThread;
    int         nTimeOut;
    int         fWorking;
    int         Result;
    int         nVars;
    int         nConfs;
} Par_ThData_t;
void * Cec_GiaSplitWorkerThread( void * pArg )
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
        pThData->Result = Cnf_GiaSolveOne( pThData->p, pThData->pCnf, pThData->nTimeOut, &pThData->nVars, &pThData->nConfs );
        pThData->fWorking = 0;
    }
	assert( 0 );
	return NULL;
}
int Cec_GiaSplitTestInt( Gia_Man_t * p, int nProcs, int nTimeOut, int nIterMax, int LookAhead, int fVerbose, int fVeryVerbose, int fSilent )
{
    abctime clkTotal = Abc_Clock();
    Par_ThData_t ThData[PAR_THR_MAX];
	pthread_t WorkerThread[PAR_THR_MAX];
    Vec_Ptr_t * vStack;
    Cnf_Dat_t * pCnf;
    double Progress = 0;
    int i, status, nSatVars, nSatConfs;
    int nIter = 0, RetValue = -1, fWorkToDo = 1;
    Abc_CexFreeP( &p->pCexComb );
    if ( fVerbose )
        printf( "Solving CEC problem by cofactoring with the following parameters:\n" );
    if ( fVerbose )
        printf( "Processes = %d   TimeOut = %d sec   MaxIter = %d   LookAhead = %d   Verbose = %d.\n", nProcs, nTimeOut, nIterMax, LookAhead, fVerbose );
    fflush( stdout );
    if ( nProcs == 1 )
        return Cec_GiaSplitTest2( p, nProcs, nTimeOut, nIterMax, LookAhead, fVerbose, fVeryVerbose, fSilent );
    // subtract manager thread
    nProcs--;
    assert( nProcs >= 1 && nProcs <= PAR_THR_MAX );
    // check the problem
    pCnf = Cec_GiaDeriveGiaRemapped( p );
    status = Cnf_GiaSolveOne( p, pCnf, nTimeOut, &nSatVars, &nSatConfs );
    Cnf_DataFree( pCnf );
    if ( fVerbose && status != -1 )
        Cec_GiaSplitPrint( 0, 0, nSatVars, nSatConfs, status, Progress, Abc_Clock() - clkTotal );
    if ( status == 0 )
    {
		if ( !fSilent )
        printf( "The problem is SAT without cofactoring.\n" );
        return 0;
    }
    if ( status == 1 )
    {
		if ( !fSilent )
        printf( "The problem is UNSAT without cofactoring.\n" );
        return 1;
    }
    assert( status == -1 );
    // create local copy
    vStack = Vec_PtrAlloc( 1000 );
    Vec_PtrPush( vStack, Gia_ManDup(p) );
    // start threads
    for ( i = 0; i < nProcs; i++ )
    {
        ThData[i].p        = NULL;
        ThData[i].pCnf     = NULL;
        ThData[i].iThread  = i;
        ThData[i].nTimeOut = nTimeOut;
        ThData[i].fWorking = 0;
        ThData[i].Result   = -1;
        ThData[i].nVars    = -1;
        ThData[i].nConfs   = -1;
        status = pthread_create( WorkerThread + i, NULL,Cec_GiaSplitWorkerThread, (void *)(ThData + i) );  assert( status == 0 );
    }
    // look at the threads
    while ( fWorkToDo )
    {
        fWorkToDo = (int)(Vec_PtrSize(vStack) > 0);
        for ( i = 0; i < nProcs; i++ )
        {
            // check if this thread is working
            if ( ThData[i].fWorking )
            {
                fWorkToDo = 1;
                continue;
            }
            // check if this thread has recently finished
            if ( ThData[i].p != NULL )
            {
                Gia_Man_t * pLast = ThData[i].p;
                int Depth = pLast->vCofVars ? Vec_IntSize(pLast->vCofVars) : 0;
                if ( pLast->vCofVars == NULL )
                    pLast->vCofVars = Vec_IntAlloc( 100 );
                if ( fVerbose )
                    Cec_GiaSplitPrint( i+1, Depth, ThData[i].nVars, ThData[i].nConfs, ThData[i].Result, Progress, Abc_Clock() - clkTotal );
                if ( ThData[i].Result == 0 ) // SAT
                {
                    p->pCexComb = pLast->pCexComb;  pLast->pCexComb = NULL;
                    RetValue = 0;
                    goto finish;
                }
                if ( ThData[i].Result == -1 ) // UNDEC
                {
                    // determine cofactoring variable
                    int nFanouts, Cost, iVar = Gia_SplitCofVar( pLast, LookAhead, &nFanouts, &Cost );
                    // cofactor
                    Gia_Man_t * pPart = Gia_ManDupCofactorVar( pLast, iVar, 0 );
                    pPart->vCofVars = Vec_IntAlloc( Vec_IntSize(pLast->vCofVars) + 1 );
                    Vec_IntAppend( pPart->vCofVars, pLast->vCofVars );
                    Vec_IntPush( pPart->vCofVars, Abc_Var2Lit(iVar, 1) );
                    Vec_PtrPush( vStack, pPart );
                    // print results
                    if ( fVeryVerbose )
                    {
//                        Cec_GiaSplitPrintRefs( pLast );
                        printf( "Var = %5d. Fanouts = %5d. Cost = %8d.  AndBefore = %6d.  AndAfter = %6d.\n", 
                            iVar, nFanouts, Cost, Gia_ManAndNum(pLast), Gia_ManAndNum(pPart) );
//                        Cec_GiaSplitPrintRefs( pPart );
                    }
                    // cofactor
                    pPart = Gia_ManDupCofactorVar( pLast, iVar, 1 );
                    pPart->vCofVars = Vec_IntAlloc( Vec_IntSize(pLast->vCofVars) + 1 );
                    Vec_IntAppend( pPart->vCofVars, pLast->vCofVars );
                    Vec_IntPush( pPart->vCofVars, Abc_Var2Lit(iVar, 1) );
                    Vec_PtrPush( vStack, pPart );
                    // keep working
                    fWorkToDo = 1;
                    nIter++;
                }
                else
                    Progress += 1.0 / pow(2, Depth);
                Gia_ManStopP( &ThData[i].p );
                if ( ThData[i].pCnf == NULL )
                    continue;
                Cnf_DataFree( ThData[i].pCnf );
                ThData[i].pCnf = NULL;
            }
            if ( Vec_PtrSize(vStack) == 0 )
                continue;
            // start a new thread
            assert( ThData[i].p == NULL );
            ThData[i].p = (Gia_Man_t*)Vec_PtrPop( vStack );
            ThData[i].pCnf = Cec_GiaDeriveGiaRemapped( ThData[i].p );
            ThData[i].fWorking = 1;
        }
        if ( nIterMax && nIter >= nIterMax )
            break;
    }
    if ( !fWorkToDo )
        RetValue = 1;
finish:
    // wait till threads finish
    for ( i = 0; i < nProcs; i++ )
        if ( ThData[i].fWorking )
            i = 0;
    // stop threads
    for ( i = 0; i < nProcs; i++ )
    {
        assert( !ThData[i].fWorking );
        // cleanup
        Gia_ManStopP( &ThData[i].p );
        if ( ThData[i].pCnf == NULL )
            continue;
        Cnf_DataFree( ThData[i].pCnf );
        ThData[i].pCnf = NULL;
        // stop
        ThData[i].p = NULL;
        ThData[i].fWorking = 1;
    }
    // finish
    Cec_GiaSplitClean( vStack );
	if ( !fSilent )
	{
		if ( RetValue == 0 )
			printf( "Problem is SAT " );
		else if ( RetValue == 1 )
			printf( "Problem is UNSAT " );
		else if ( RetValue == -1 )
			printf( "Problem is UNDECIDED " );
		else assert( 0 );
		printf( "after %d case-splits.  ", nIter );
		Abc_PrintTime( 1, "Time", Abc_Clock() - clkTotal );
		fflush( stdout );
	}
    return RetValue;
}
int Cec_GiaSplitTest( Gia_Man_t * p, int nProcs, int nTimeOut, int nIterMax, int LookAhead, int fVerbose, int fVeryVerbose, int fSilent )
{
    Abc_Cex_t * pCex = NULL;
    Gia_Man_t * pOne;
    Gia_Obj_t * pObj;
    int i, RetValue1, fOneUndef = 0, RetValue = -1;
    Abc_CexFreeP( &p->pCexComb );
    Gia_ManForEachPo( p, pObj, i )
    {
        pOne = Gia_ManDupOutputGroup( p, i, i+1 );
        if ( fVerbose )
            printf( "\nSolving output %d:\n", i );
        RetValue1 = Cec_GiaSplitTestInt( pOne, nProcs, nTimeOut, nIterMax, LookAhead,  fVerbose, fVeryVerbose, fSilent );
        Gia_ManStop( pOne );
        // collect the result
        if ( RetValue1 == 0 && RetValue == -1 )
        {
            pCex = pOne->pCexComb; pOne->pCexComb = NULL;
            pCex->iPo = i;
            RetValue = 0;
        }
        if ( RetValue1 == -1 )
            fOneUndef = 1;
    }
    if ( RetValue == -1 )
        RetValue = fOneUndef ? -1 : 1;
    else
        p->pCexComb = pCex;
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Print stats about cofactoring variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_GiaPrintCofStats( Gia_Man_t * p )
{
    Gia_Man_t * pCof0, * pCof1;
    Gia_Obj_t * pObj, * pFan0, * pFan1, * pCtrl;
    Vec_Int_t * vMarks;
    int i, Count = 0;
    vMarks = Vec_IntStart( Gia_ManObjNum(p) );
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( !Gia_ObjIsMuxType(pObj) )
            continue;
        if ( Gia_ObjRecognizeExor(pObj, &pFan0, &pFan1) )
            continue;
        pCtrl = Gia_ObjRecognizeMux( pObj, &pFan1, &pFan0 );
        pCtrl = Gia_Regular(pCtrl);
        Vec_IntAddToEntry( vMarks, Gia_ObjId(p, pCtrl), 1 );
    }
    printf( "The AIG with %d candidate nodes (PI+AND) has %d unique MUX control drivers:\n", 
        Gia_ManCandNum(p), Vec_IntCountPositive(vMarks) );
    Gia_ManLevelNum( p );
    Gia_ManForEachCand( p, pObj, i )
    {
        if ( !Vec_IntEntry(vMarks, i) )
            continue;
        pCof0 = Gia_ManDupCofactorObj( p, i, 0 );
        pCof1 = Gia_ManDupCofactorObj( p, i, 1 );
        printf( "%6d :   ",          Count++ );
        printf( "Obj = %6d   ",      i );
        printf( "MUX refs = %5d   ", Vec_IntEntry(vMarks, i) );
        printf( "Level = %5d   ",    Gia_ObjLevelId(p, i) );
        printf( "Cof0 = %7d   ",     Gia_ManAndNum(pCof0) );
        printf( "Cof1 = %7d   ",     Gia_ManAndNum(pCof1) );
        printf( "\n" );
        Gia_ManStop( pCof0 );
        Gia_ManStop( pCof1 );
    }
    Vec_IntFree( vMarks );
}
void Cec_GiaPrintCofStats2( Gia_Man_t * p )
{
    Gia_Man_t * pCof0, * pCof1;
    Gia_Obj_t * pObj;
    int i;
    Gia_ManLevelNum( p );
    Gia_ManCreateRefs( p );
    Gia_ManForEachPi( p, pObj, i )
    {
        pCof0 = Gia_ManDupCofactorVar( p, i, 0 );
        pCof1 = Gia_ManDupCofactorVar( p, i, 1 );
        printf( "PI %5d :   ",   i );
        printf( "Refs = %5d   ", Gia_ObjRefNum(p, pObj) );
        printf( "Cof0 = %7d   ", Gia_ManAndNum(pCof0) );
        printf( "Cof1 = %7d   ", Gia_ManAndNum(pCof1) );
        printf( "\n" );
        Gia_ManStop( pCof0 );
        Gia_ManStop( pCof1 );
    }
}

#endif // pthreads are used

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

