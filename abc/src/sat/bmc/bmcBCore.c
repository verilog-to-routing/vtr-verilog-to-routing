/**CFile****************************************************************

  FileName    [bmcBCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based bounded model checking.]

  Synopsis    [Performs recording of BMC core.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: bmcBCore.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bmc.h"
#include "sat/bsat/satSolver.h"
#include "sat/bsat/satStore.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Collect pivot variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Bmc_ManBCoreReadPivots( char * pName )
{
    int Num;
    Vec_Int_t * vPivots = Vec_IntAlloc( 100 );
    FILE * pFile = fopen( pName, "r" );
    while ( fscanf( pFile, "%d", &Num ) == 1 )
        Vec_IntPush( vPivots, Num );
    fclose( pFile );
    return vPivots;
}
Vec_Int_t * Bmc_ManBCoreCollectPivots( Gia_Man_t * p, char * pName, Vec_Int_t * vVarMap )
{
    Gia_Obj_t * pObj;
    int i, iVar, iFrame;
    Vec_Int_t * vPivots = Vec_IntAlloc( 100 );
    Vec_Int_t * vVars = Bmc_ManBCoreReadPivots( pName );
    Gia_ManForEachObj( p, pObj, i )
        pObj->fMark0 = 0;
    Vec_IntForEachEntry( vVars, iVar, i )
        if ( iVar > 0 && iVar < Gia_ManObjNum(p) )
            Gia_ManObj( p, iVar )->fMark0 = 1;
        else
            printf( "Cannot find object with Id %d in the given AIG.\n", iVar );
    Vec_IntForEachEntryDouble( vVarMap, iVar, iFrame, i )
        if ( Gia_ManObj( p, iVar )->fMark0 )
            Vec_IntPush( vPivots, Abc_Lit2Var(i) );
    Gia_ManForEachObj( p, pObj, i )
        pObj->fMark0 = 0;
    Vec_IntFree( vVars );
    return vPivots;
}

/**Function*************************************************************

  Synopsis    [Collect (Id; Frame) pairs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Bmc_ManBCoreAssignVar( Gia_Man_t * p, Gia_Obj_t * pObj, int f, Vec_Int_t * vNodes )
{
    pObj->Value = Abc_Lit2Var(Vec_IntSize(vNodes));
    Vec_IntPush( vNodes, Gia_ObjId(p, pObj) );
    Vec_IntPush( vNodes, f );
//    printf( "Obj %3d  Frame %3d  --->  Var %3d    ", Gia_ObjId(p, pObj), f, pObj->Value );
//    Gia_ObjPrint( p, pObj );
}
void Bmc_ManBCoreCollect_rec( Gia_Man_t * p, int Id, int f, Vec_Int_t * vNodes, Vec_Int_t * vRootsNew )
{
    Gia_Obj_t * pObj;
    if ( Gia_ObjIsTravIdCurrentId(p, Id) )
        return;
    Gia_ObjSetTravIdCurrentId(p, Id);
    pObj = Gia_ManObj( p, Id );
    Bmc_ManBCoreAssignVar( p, pObj, f, vNodes );
    if ( Gia_ObjIsPi(p, pObj) )
        return;
    if ( Gia_ObjIsRo(p, pObj) )
    {
        Vec_IntPush( vRootsNew, Gia_ObjId(p, Gia_ObjRoToRi(p, pObj)) );
        return;
    }
    assert( Gia_ObjIsAnd(pObj) );
    Bmc_ManBCoreCollect_rec( p, Gia_ObjFaninId0p(p, pObj), f, vNodes, vRootsNew );
    Bmc_ManBCoreCollect_rec( p, Gia_ObjFaninId1p(p, pObj), f, vNodes, vRootsNew );
}
Vec_Int_t * Bmc_ManBCoreCollect( Gia_Man_t * p, int iFrame, int iOut, sat_solver * pSat )
{
    Gia_Obj_t * pObj; 
    int f, i, iObj, nNodesOld;
    Vec_Int_t * vNodes  = Vec_IntAlloc( 100 );
    Vec_Int_t * vRoots  = Vec_IntAlloc( 100 );
    Vec_Int_t * vRoots2 = Vec_IntAlloc( 100 );
    assert( iFrame >= 0 && iOut >= 0 );
    // add first variables
    Vec_IntPush( vNodes, -1 );
    Vec_IntPush( vNodes, -1 );
    Bmc_ManBCoreAssignVar( p, Gia_ManPo(p, iOut), iFrame, vNodes );
    // start with root node
    Vec_IntPush( vRoots, Gia_ObjId(p, Gia_ManPo(p, iOut)) );
    // iterate through time frames
    for ( f = iFrame; f >= 0; f-- )
    {
        Gia_ManIncrementTravId( p );
        // add constant node
        Gia_ObjSetTravIdCurrent( p, Gia_ManConst0(p) );
        Bmc_ManBCoreAssignVar( p, Gia_ManConst0(p), f, vNodes );
        sat_solver_add_const( pSat, Gia_ManConst0(p)->Value, 1 );
        // collect nodes in this timeframe
        Vec_IntClear( vRoots2 );
        nNodesOld = Vec_IntSize(vNodes);
        Gia_ManForEachObjVec( vRoots, p, pObj, i )
            Bmc_ManBCoreCollect_rec( p, Gia_ObjFaninId0p(p, pObj), f, vNodes, vRoots2 );
        if ( f == iFrame )
        {
            // add the final clause
            pObj = Gia_ManPo(p, iOut);
            assert( pObj->Value == 1 );
            assert( Gia_ObjFanin0(pObj)->Value == 3 );
//            sat_solver_add_const( pSat, Gia_ObjFanin0(pObj)->Value, Gia_ObjFaninC0(pObj) );
            sat_solver_add_constraint( pSat, Gia_ObjFanin0(pObj)->Value, pObj->Value, Gia_ObjFaninC0(pObj) );
        }
        else
        {
            // connect current RIs to previous ROs
            Gia_ManForEachObjVec( vRoots, p, pObj, i )
                sat_solver_add_buffer( pSat, pObj->Value, Gia_ObjFanin0(pObj)->Value, Gia_ObjFaninC0(pObj) );
        }
        Gia_ManForEachObjVec( vRoots2, p, pObj, i )
            pObj->Value = Gia_ObjRiToRo(p, pObj)->Value;
        // add nodes of this timeframe
        Vec_IntForEachEntryStart( vNodes, iObj, i, nNodesOld )
        {
            pObj = Gia_ManObj(p, iObj); i++;
            if ( Gia_ObjIsCi(pObj) )
                continue;
            assert( Gia_ObjIsAnd(pObj) );
            sat_solver_add_and( pSat, pObj->Value, Gia_ObjFanin0(pObj)->Value, Gia_ObjFanin1(pObj)->Value, Gia_ObjFaninC0(pObj), Gia_ObjFaninC1(pObj), 0 );
        }
        // collect constant node
        ABC_SWAP( Vec_Int_t *, vRoots, vRoots2 );
    }
    // add constraint variables for the init state
    Gia_ManForEachObjVec( vRoots, p, pObj, i )
    {
        sat_solver_add_constraint( pSat, pObj->Value, Abc_Lit2Var(Vec_IntSize(vNodes)), 1 );
        pObj = Gia_ObjRiToRo(p, pObj);
        Bmc_ManBCoreAssignVar( p, pObj, -1, vNodes );
    }
    Vec_IntFree( vRoots );
    Vec_IntFree( vRoots2 );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bmc_ManBCorePerform( Gia_Man_t * p, Bmc_BCorePar_t * pPars )
{
    clock_t clk = clock();
    Intp_Man_t * pManProof;
    Vec_Int_t * vVarMap, * vCore;
    sat_solver * pSat;
    FILE * pFile;
    void * pSatCnf; 
    int RetValue;
    // create SAT solver
    pSat = sat_solver_new();
    sat_solver_store_alloc( pSat ); 
    sat_solver_setnvars( pSat, 1000 );
    sat_solver_set_runtime_limit( pSat, pPars->nTimeOut ? pPars->nTimeOut * CLOCKS_PER_SEC + Abc_Clock(): 0 );
    vVarMap = Bmc_ManBCoreCollect( p, pPars->iFrame, pPars->iOutput, pSat );
    sat_solver_store_mark_roots( pSat ); 
    // create pivot variables
    if ( pPars->pFilePivots )
    {
        Vec_Int_t * vPivots = Bmc_ManBCoreCollectPivots(p, pPars->pFilePivots, vVarMap);
        sat_solver_set_pivot_variables( pSat, Vec_IntArray(vPivots), Vec_IntSize(vPivots) );
        Vec_IntReleaseArray( vPivots );
        Vec_IntFree( vPivots );
    }
    // solve the problem
    RetValue = sat_solver_solve( pSat, NULL, NULL, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
    if ( RetValue == l_Undef )
    {
        Vec_IntFree( vVarMap );
        sat_solver_delete( pSat );
        printf( "Timeout of conflict limit is reached.\n" );
        return;
    }
    if ( RetValue == l_True )
    {
        Vec_IntFree( vVarMap );
        sat_solver_delete( pSat );
        printf( "The BMC problem is SAT.\n" );
        return;
    }
    if ( pPars->fVerbose )
    {
        printf( "SAT solver returned UNSAT after %7d conflicts.      ", (int)pSat->stats.conflicts );
        Abc_PrintTime( 1, "Time", clock() - clk );
    }
    assert( RetValue == l_False );
    pSatCnf = sat_solver_store_release( pSat ); 
//    Sto_ManDumpClauses( (Sto_Man_t *)pSatCnf, "cnf_store.txt" );
    // derive the UNSAT core
    clk = clock();
    pManProof = Intp_ManAlloc();
    vCore = (Vec_Int_t *)Intp_ManUnsatCore( pManProof, (Sto_Man_t *)pSatCnf, 1, pPars->fVerbose );
    Intp_ManFree( pManProof );
    if ( pPars->fVerbose )
    {
        printf( "UNSAT core contains %d (out of %d) learned clauses.   ", Vec_IntSize(vCore), sat_solver_nconflicts(pSat) );
        Abc_PrintTime( 1, "Time", clock() - clk );
    }
    // write the problem
    Vec_IntSort( vCore, 0 );
    pFile = pPars->pFileProof ? fopen( pPars->pFileProof, "wb" ) : stdout;
    Intp_ManUnsatCorePrintForBmc( pFile, (Sto_Man_t *)pSatCnf, vCore, vVarMap );
    if ( pFile != stdout )
        fclose( pFile );
    // cleanup
    Sto_ManFree( (Sto_Man_t *)pSatCnf );
    Vec_IntFree( vVarMap );
    Vec_IntFree( vCore );
    sat_solver_delete( pSat );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

