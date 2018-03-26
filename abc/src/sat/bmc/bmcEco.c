/**CFile****************************************************************

  FileName    [bmcEco.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based bounded model checking.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: bmcEco.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bmc.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satSolver.h"
#include "aig/gia/giaAig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes miter for ECO with given root node and fanins.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Bmc_EcoMiter( Gia_Man_t * pGold, Gia_Man_t * pOld, Vec_Int_t * vFans )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pRoot = Gia_ObjFanin0( Gia_ManPo(pOld, Gia_ManPoNum(pOld)-1) ); // fanin of the last PO
    Gia_Obj_t * pObj;
    int i, NewPi, Miter;
    assert( Gia_ManCiNum(pGold) == Gia_ManCiNum(pOld) );
    assert( Gia_ManCoNum(pGold) == Gia_ManCoNum(pOld) - 1 );
    assert( Gia_ObjIsAnd(pRoot) );
    // create the miter
    pNew = Gia_ManStart( 3 * Gia_ManObjNum(pGold) );
    pNew->pName = Abc_UtilStrsav( pGold->pName );
    Gia_ManHashAlloc( pNew );
    // copy gold
    Gia_ManConst0(pGold)->Value = 0;
    Gia_ManForEachCi( pGold, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    NewPi = Gia_ManAppendCi( pNew );
    Gia_ManForEachAnd( pGold, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachCo( pGold, pObj, i )
        pObj->Value = Gia_ObjFanin0Copy( pObj );
    // create onset
    Gia_ManConst0(pOld)->Value = 0;
    Gia_ManForEachCi( pOld, pObj, i )
        pObj->Value = Gia_ManCi(pGold, i)->Value;
    Gia_ManForEachAnd( pOld, pObj, i )
        if ( pObj == pRoot )
            pObj->Value = NewPi;
        else
            pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachCo( pOld, pObj, i )
        pObj->Value = Gia_ObjFanin0Copy( pObj );
    Gia_ManForEachCo( pGold, pObj, i )
        Gia_ManAppendCo( pNew, Gia_ManHashXor(pNew, pObj->Value, Gia_ManCo(pOld, i)->Value) );
    // create offset
    Gia_ManForEachAnd( pOld, pObj, i )
        if ( pObj == pRoot )
            pObj->Value = Abc_LitNot(NewPi);
        else
            pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachCo( pOld, pObj, i )
        pObj->Value = Gia_ObjFanin0Copy( pObj );
    Miter = 0;
    Gia_ManForEachCo( pGold, pObj, i )
        Miter = Gia_ManHashOr( pNew, Miter, Gia_ManHashXor(pNew, pObj->Value, Gia_ManCo(pOld, i)->Value) );
    Gia_ManAppendCo( pNew, Miter );
    // add outputs for the nodes
    Gia_ManForEachObjVec( vFans, pOld, pObj, i )
        Gia_ManAppendCo( pNew, pObj->Value );
    // cleanup
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    assert( Gia_ManPiNum(pNew) == Gia_ManCiNum(pGold) + 1 );
    assert( Gia_ManPoNum(pNew) == Gia_ManCoNum(pGold) + 1 + Vec_IntSize(vFans) );
    return pNew;
}

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

  Synopsis    [Solve the enumeration problem.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bmc_EcoSolve( sat_solver * pSat, int Root, Vec_Int_t * vVars )
{
    int nBTLimit = 1000000;
    Vec_Int_t * vLits   = Vec_IntAlloc( Vec_IntSize(vVars) );
    int status, i, Div, iVar, nFinal, * pFinal, nIter = 0, RetValue = 0;
    int pLits[2], nVars = sat_solver_nvars( pSat );
    sat_solver_setnvars( pSat, nVars + 1 );
    pLits[0] = Abc_Var2Lit( Root, 0 );  // F = 1
    pLits[1] = Abc_Var2Lit( nVars, 0 ); // iNewLit
    while ( 1 ) 
    {
        // find onset minterm
        status = sat_solver_solve( pSat, pLits, pLits + 2, nBTLimit, 0, 0, 0 );
        if ( status == l_Undef )
            { RetValue = -1; break; }
        if ( status == l_False )
            { RetValue = 1; break; }
        assert( status == l_True );
        // collect divisor literals
        Vec_IntClear( vLits );
        Vec_IntPush( vLits, Abc_LitNot(pLits[0]) ); // F = 0
        Vec_IntForEachEntry( vVars, Div, i )
            Vec_IntPush( vLits, sat_solver_var_literal(pSat, Div) );
        // check against offset
        status = sat_solver_solve( pSat, Vec_IntArray(vLits), Vec_IntArray(vLits) + Vec_IntSize(vLits), nBTLimit, 0, 0, 0 );
        if ( status == l_Undef )
            { RetValue = -1; break; }
        if ( status == l_True )
            break;
        assert( status == l_False );
        // compute cube and add clause
        nFinal = sat_solver_final( pSat, &pFinal );
        Vec_IntClear( vLits );
        Vec_IntPush( vLits, Abc_LitNot(pLits[1]) ); // NOT(iNewLit)
        printf( "Cube %d : ", nIter );
        for ( i = 0; i < nFinal; i++ )
        {
            if ( pFinal[i] == pLits[0] )
                continue;
            Vec_IntPush( vLits, pFinal[i] );
            iVar = Vec_IntFind( vVars, Abc_Lit2Var(pFinal[i]) );   assert( iVar >= 0 );
            printf( "%s%d ", Abc_LitIsCompl(pFinal[i]) ? "+":"-", iVar );
        }
        printf( "\n" );
        status = sat_solver_addclause( pSat, Vec_IntArray(vLits), Vec_IntArray(vLits) + Vec_IntSize(vLits) );
        assert( status );
        nIter++;
    }
//    assert( status == l_True );
    Vec_IntFree( vLits );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Compute the patch function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bmc_EcoPatch( Gia_Man_t * p, int nIns, int nOuts )
{
    int i, Lit, RetValue, Root;
    Gia_Obj_t * pObj;
    Vec_Int_t * vVars;
    // generate CNF and solver
    Cnf_Dat_t * pCnf = Cnf_DeriveGiaRemapped( p );
    sat_solver * pSat = sat_solver_new();
    sat_solver_setnvars( pSat, pCnf->nVars );
    // add timeframe clauses
    for ( i = 0; i < pCnf->nClauses; i++ )
        if ( !sat_solver_addclause( pSat, pCnf->pClauses[i], pCnf->pClauses[i+1] ) )
            assert( 0 );
    // add equality constraints
    assert( Gia_ManPoNum(p) == nOuts + 1 + nIns );
    Gia_ManForEachPo( p, pObj, i )
    {
        if ( i == nOuts )
            break;
        Lit = Abc_Var2Lit( pCnf->pVarNums[Gia_ObjId(p, pObj)], 1 ); // neg lit
        RetValue = sat_solver_addclause( pSat, &Lit, &Lit + 1 );
        assert( RetValue );
    }
    // add inequality constraint
    pObj = Gia_ManPo( p, nOuts );
    Lit = Abc_Var2Lit( pCnf->pVarNums[Gia_ObjId(p, pObj)], 0 ); // pos lit
    RetValue = sat_solver_addclause( pSat, &Lit, &Lit + 1 );
    assert( RetValue );
    // simplify the SAT solver
    RetValue = sat_solver_simplify( pSat );
    assert( RetValue );
    // collect input variables
    vVars = Vec_IntAlloc( nIns );
    Gia_ManForEachPo( p, pObj, i )
        if ( i >= nOuts + 1 )
            Vec_IntPush( vVars, pCnf->pVarNums[Gia_ObjId(p, pObj)] );
    assert( Vec_IntSize(vVars) == nIns );
    // derive the root variable
    pObj = Gia_ManPi( p, Gia_ManPiNum(p) - 1 );
    Root = pCnf->pVarNums[Gia_ObjId(p, pObj)];
    // solve the problem
    RetValue = Bmc_EcoSolve( pSat, Root, vVars );
    Vec_IntFree( vVars );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Tests the ECO miter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bmc_EcoMiterTest()
{
    char * pFileGold = "eco_gold.aig";
    char * pFileOld =  "eco_old.aig";
    Vec_Int_t * vFans;
    FILE * pFile;
    Gia_Man_t * pMiter;
    Gia_Obj_t * pObj;
    Gia_Man_t * pGold;
    Gia_Man_t * pOld;
    int i, RetValue;
    // check that the files exist
    pFile = fopen( pFileGold, "r" );
    if ( pFile == NULL )
    {
        printf( "File \"%s\" does not exist.\n", pFileGold );
        return;
    }
    fclose( pFile );
    pFile = fopen( pFileOld, "r" );
    if ( pFile == NULL )
    {
        printf( "File \"%s\" does not exist.\n", pFileOld );
        return;
    }
    fclose( pFile );
    // read files
    pGold = Gia_AigerRead( pFileGold, 0, 0, 0 );
    pOld  = Gia_AigerRead( pFileOld, 0, 0, 0 );
    // create ECO miter
    vFans = Vec_IntAlloc( Gia_ManCiNum(pOld) );
    Gia_ManForEachCi( pOld, pObj, i )
        Vec_IntPush( vFans, Gia_ObjId(pOld, pObj) );
    pMiter = Bmc_EcoMiter( pGold, pOld, vFans );
    Vec_IntFree( vFans );
    Gia_AigerWrite( pMiter, "eco_miter.aig", 0, 0 );
    // find the patch
    RetValue = Bmc_EcoPatch( pMiter, Gia_ManCiNum(pGold), Gia_ManCoNum(pGold) );
    if ( RetValue == 1 )
        printf( "Patch is computed.\n" );
    if ( RetValue == 0 )
        printf( "Cannot be patched.\n" );
    if ( RetValue == -1 )
        printf( "Resource limit exceeded.\n" );
    Gia_ManStop( pMiter );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

