/**CFile****************************************************************

  FileName    [bmcBmci.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based bounded model checking.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: bmcBmci.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bmc.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satStore.h"
#include "aig/gia/giaAig.h"

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
static inline Cnf_Dat_t * Cnf_DeriveGiaRemapped( Gia_Man_t * p )
{
    Cnf_Dat_t * pCnf;
    Aig_Man_t * pAig = Gia_ManToAigSimple( p );
    pAig->nRegs = 0;
    pCnf = Cnf_Derive( pAig, Aig_ManCoNum(pAig) );
    Aig_ManStop( pAig );
    return pCnf;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Cnf_DataLiftGia( Cnf_Dat_t * p, Gia_Man_t * pGia, int nVarsPlus )
{
    Gia_Obj_t * pObj;
    int v;
    Gia_ManForEachObj( pGia, pObj, v )
        if ( p->pVarNums[Gia_ObjId(pGia, pObj)] >= 0 )
            p->pVarNums[Gia_ObjId(pGia, pObj)] += nVarsPlus;
    for ( v = 0; v < p->nLiterals; v++ )
        p->pClauses[0][v] += 2*nVarsPlus;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bmc_BmciUnfold( Gia_Man_t * pNew, Gia_Man_t * p, Vec_Int_t * vFFLits, int fPiReuse )
{
    Gia_Obj_t * pObj;
    int i;
    assert( Gia_ManRegNum(p) == Vec_IntSize(vFFLits) );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachRo( p, pObj, i )
        pObj->Value = Vec_IntEntry(vFFLits, i);
    Gia_ManForEachPi( p, pObj, i )
        pObj->Value = fPiReuse ? Gia_ObjToLit(pNew, Gia_ManPi(pNew, Gia_ManPiNum(pNew)-Gia_ManPiNum(p)+i)) : Gia_ManAppendCi(pNew);
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachRi( p, pObj, i )
        Vec_IntWriteEntry( vFFLits, i, Gia_ObjFanin0Copy(pObj) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bmc_BmciPart_rec( Gia_Man_t * pNew, Vec_Int_t * vSatMap, int iIdNew, Gia_Man_t * pPart, Vec_Int_t * vPartMap, Vec_Int_t * vCopies )
{
    Gia_Obj_t * pObj = Gia_ManObj( pNew, iIdNew ); 
    int iLitPart0, iLitPart1, iRes;
    if ( Vec_IntEntry(vCopies, iIdNew) )
        return Vec_IntEntry(vCopies, iIdNew);
    if ( Vec_IntEntry(vSatMap, iIdNew) >= 0 || Gia_ObjIsCi(pObj) )
    {
        Vec_IntPush( vPartMap, iIdNew );
        iRes = Gia_ManAppendCi(pPart);
        Vec_IntWriteEntry( vCopies, iIdNew, iRes );
        return iRes;
    }
    assert( Gia_ObjIsAnd(pObj) );
    iLitPart0 = Bmc_BmciPart_rec( pNew, vSatMap, Gia_ObjFaninId0(pObj, iIdNew), pPart, vPartMap, vCopies );
    iLitPart1 = Bmc_BmciPart_rec( pNew, vSatMap, Gia_ObjFaninId1(pObj, iIdNew), pPart, vPartMap, vCopies );
    iLitPart0 = Abc_LitNotCond( iLitPart0, Gia_ObjFaninC0(pObj) );
    iLitPart1 = Abc_LitNotCond( iLitPart1, Gia_ObjFaninC1(pObj) );
    Vec_IntPush( vPartMap, iIdNew );
    iRes = Gia_ManAppendAnd( pPart, iLitPart0, iLitPart1 );
    Vec_IntWriteEntry( vCopies, iIdNew, iRes );
    return iRes;
}
Gia_Man_t * Bmc_BmciPart( Gia_Man_t * pNew, Vec_Int_t * vSatMap, Vec_Int_t * vMiters, Vec_Int_t * vPartMap, Vec_Int_t * vCopies )
{
    Gia_Man_t * pPart;
    int i, iLit, iLitPart;
    Vec_IntFill( vCopies, Gia_ManObjNum(pNew), 0 );
    Vec_IntFillExtra( vSatMap, Gia_ManObjNum(pNew), -1 );
    pPart = Gia_ManStart( 1000 );
    pPart->pName = Abc_UtilStrsav( pNew->pName );
    Vec_IntClear( vPartMap );
    Vec_IntPush( vPartMap, 0 );
    Vec_IntForEachEntry( vMiters, iLit, i )
    {
        if ( iLit == -1 )
            continue;
        assert( iLit >= 2 );
        iLitPart = Bmc_BmciPart_rec( pNew, vSatMap, Abc_Lit2Var(iLit), pPart, vPartMap, vCopies );
        Gia_ManAppendCo( pPart, Abc_LitNotCond(iLitPart, Abc_LitIsCompl(iLit)) );
        Vec_IntPush( vPartMap, -1 );
    }
    assert( Gia_ManPoNum(pPart) > 0 );
    assert( Vec_IntSize(vPartMap) == Gia_ManObjNum(pPart) );
    return pPart;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bmc_BmciPerform( Gia_Man_t * p, Vec_Int_t * vInit0, Vec_Int_t * vInit1, int nFrames, int nWords, int nTimeOut, int fVerbose )
{
    int nSatVars = 1;
    Vec_Int_t * vLits0, * vLits1, * vMiters, * vSatMap, * vPartMap, * vCopies;
    Gia_Man_t * pNew, * pPart;
    Gia_Obj_t * pObj;
    Cnf_Dat_t * pCnf;
    sat_solver * pSat;
    int iVar0, iVar1, iLit, iLit0, iLit1;
    int i, f, status, nChanges, nMiters, RetValue = 1;
    assert( Vec_IntSize(vInit0) == Gia_ManRegNum(p) );
    assert( Vec_IntSize(vInit1) == Gia_ManRegNum(p) );

    // start the SAT solver
    pSat = sat_solver_new();
    sat_solver_set_runtime_limit( pSat, nTimeOut ? nTimeOut * CLOCKS_PER_SEC + Abc_Clock(): 0 );

    pNew = Gia_ManStart( 10000 );
    pNew->pName = Abc_UtilStrsav( p->pName );
    Gia_ManHashAlloc( pNew );
    Gia_ManConst0(p)->Value = 0;

    vLits0 = Vec_IntAlloc( Gia_ManRegNum(p) );
    Vec_IntForEachEntry( vInit0, iLit, i )
        Vec_IntPush( vLits0, iLit < 2 ? iLit : Gia_ManAppendCi(pNew) );

    vLits1 = Vec_IntAlloc( Gia_ManRegNum(p) );
    Vec_IntForEachEntry( vInit1, iLit, i )
        Vec_IntPush( vLits1, iLit < 2 ? iLit : Gia_ManAppendCi(pNew) );

    vMiters  = Vec_IntAlloc( 1000 );
    vSatMap  = Vec_IntAlloc( 1000 );
    vPartMap = Vec_IntAlloc( 1000 );
    vCopies  = Vec_IntAlloc( 1000 );
    for ( f = 0; f < nFrames; f++ )
    {
        abctime clk = Abc_Clock();
        Bmc_BmciUnfold( pNew, p, vLits0, 0 );
        Bmc_BmciUnfold( pNew, p, vLits1, 1 );
        assert( Vec_IntSize(vLits0) == Vec_IntSize(vLits1) );
        nMiters  = 0;
        Vec_IntClear( vMiters );
        Vec_IntForEachEntryTwo( vLits0, vLits1, iLit0, iLit1, i )
            if ( (iLit0 >= 2 || iLit1 >= 2) && iLit0 != iLit1 )
                Vec_IntPush( vMiters, Gia_ManHashXor(pNew, iLit0, iLit1) ), nMiters++;
            else
                Vec_IntPush( vMiters, -1 );
        assert( Vec_IntSize(vMiters) == Gia_ManRegNum(p) );
        if ( Vec_IntSum(vMiters) + Vec_IntSize(vLits1) == 0 )
        {
            if ( fVerbose )
                printf( "Reached a fixed point after %d frames.  \n", f+1 );
            break;
        }
        // create new part
        pPart = Bmc_BmciPart( pNew, vSatMap, vMiters, vPartMap, vCopies );
        pCnf = Cnf_DeriveGiaRemapped( pPart );
        Cnf_DataLiftGia( pCnf, pPart, nSatVars );
        nSatVars += pCnf->nVars;
        sat_solver_setnvars( pSat, nSatVars );
        for ( i = 0; i < pCnf->nClauses; i++ )
            if ( !sat_solver_addclause( pSat, pCnf->pClauses[i], pCnf->pClauses[i+1] ) )
                assert( 0 );
        // stitch the clauses
        Gia_ManForEachPi( pPart, pObj, i )
        {
            iVar0 = pCnf->pVarNums[Gia_ObjId(pPart, pObj)];
            iVar1 = Vec_IntEntry( vSatMap, Vec_IntEntry(vPartMap, Gia_ObjId(pPart, pObj)) );
            if ( iVar1 == -1 )
                continue;
            sat_solver_add_buffer( pSat, iVar0, iVar1, 0 );
        }
        // transfer variables
        Gia_ManForEachCand( pPart, pObj, i )
            if ( pCnf->pVarNums[i] >= 0 )
            {
                assert( Gia_ObjIsCi(pObj) || Vec_IntEntry( vSatMap, Vec_IntEntry(vPartMap, i) ) == -1 );
                Vec_IntWriteEntry( vSatMap, Vec_IntEntry(vPartMap, i), pCnf->pVarNums[i] );
            }
        Cnf_DataFree( pCnf );
        Gia_ManStop( pPart );
        // perform runs
        nChanges = 0;
        Vec_IntForEachEntry( vMiters, iLit, i )
        {
            if ( iLit == -1 )
                continue;
            assert( Vec_IntEntry(vSatMap, Abc_Lit2Var(iLit)) > 0 );
            iLit = Abc_Lit2LitV( Vec_IntArray(vSatMap), iLit );
            status = sat_solver_solve( pSat, &iLit, &iLit + 1, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
            if ( status == l_True )
            {
                nChanges++;
                continue;
            }
            if ( status == l_Undef )
            {
                printf( "Timeout reached after %d seconds.  \n", nTimeOut );
                RetValue = 0;
                goto cleanup;
            }
            assert( status == l_False );
            iLit0 = Vec_IntEntry( vLits0, i );
            iLit1 = Vec_IntEntry( vLits1, i );
            assert( (iLit0 >= 2 || iLit1 >= 2) && iLit0 != iLit1 );
            if ( iLit1 >= 2 )
                Vec_IntWriteEntry( vLits1, i, iLit0 );
            else
                Vec_IntWriteEntry( vLits0, i, iLit1 );
            iLit0 = Vec_IntEntry( vLits0, i );
            iLit1 = Vec_IntEntry( vLits1, i );
            assert( iLit0 == iLit1 );
        }
        if ( fVerbose )
        {
            printf( "Frame %4d : ",     f+1 );
            printf( "Vars =%7d  ",      nSatVars );
            printf( "Clause =%10d  ",   sat_solver_nclauses(pSat) );
            printf( "Conflict =%10d  ", sat_solver_nconflicts(pSat) );
            printf( "AIG =%7d  ",       Gia_ManAndNum(pNew) );
            printf( "Miters =%5d  ",    nMiters );
            printf( "SAT =%5d  ",       nChanges );
            Abc_PrintTime( 1, "Time",   Abc_Clock() - clk );
        }
        if ( nChanges == 0 )
        {
            printf( "Reached a fixed point after %d frames.  \n", f+1 );
            break;
        }
    }
cleanup:

    sat_solver_delete( pSat );
    Gia_ManStopP( &pNew );
    Vec_IntFree( vLits0 );
    Vec_IntFree( vLits1 );
    Vec_IntFree( vMiters );
    Vec_IntFree( vSatMap );
    Vec_IntFree( vPartMap );
    Vec_IntFree( vCopies );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManBmciTest( Gia_Man_t * p, Vec_Int_t * vInit, int nFrames, int nWords, int nTimeOut, int fSim, int fVerbose )
{
    Vec_Int_t * vInit0 = Vec_IntStart( Gia_ManRegNum(p) );
    Bmc_BmciPerform( p, vInit, vInit0, nFrames, nWords, nTimeOut, fVerbose );
    Vec_IntFree( vInit0 );
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

