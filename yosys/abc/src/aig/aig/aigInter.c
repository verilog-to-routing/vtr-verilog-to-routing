/**CFile****************************************************************

  FileName    [aigInter.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [Interpolate two AIGs.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aigInter.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satStore.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern abctime timeCnf;
extern abctime timeSat;
extern abctime timeInt;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManInterFast( Aig_Man_t * pManOn, Aig_Man_t * pManOff, int fVerbose )
{
    sat_solver * pSat;
    Cnf_Dat_t * pCnfOn, * pCnfOff;
    Aig_Obj_t * pObj, * pObj2;
    int Lits[3], status, i;
//    abctime clk = Abc_Clock();

    assert( Aig_ManCiNum(pManOn) == Aig_ManCiNum(pManOff) );
    assert( Aig_ManCoNum(pManOn) == Aig_ManCoNum(pManOff) );

    // derive CNFs
    pManOn->nRegs = Aig_ManCoNum(pManOn);
    pCnfOn  = Cnf_Derive( pManOn, Aig_ManCoNum(pManOn) );
    pManOn->nRegs = 0;

    pManOff->nRegs = Aig_ManCoNum(pManOn);
    pCnfOff = Cnf_Derive( pManOff, Aig_ManCoNum(pManOff) );
    pManOff->nRegs = 0;

//    pCnfOn  = Cnf_DeriveSimple( pManOn, Aig_ManCoNum(pManOn) );
//    pCnfOff = Cnf_DeriveSimple( pManOff, Aig_ManCoNum(pManOn) );
    Cnf_DataLift( pCnfOff, pCnfOn->nVars );

    // start the solver
    pSat = sat_solver_new();
    sat_solver_setnvars( pSat, pCnfOn->nVars + pCnfOff->nVars );

    // add clauses of A
    for ( i = 0; i < pCnfOn->nClauses; i++ )
    {
        if ( !sat_solver_addclause( pSat, pCnfOn->pClauses[i], pCnfOn->pClauses[i+1] ) )
        {
            Cnf_DataFree( pCnfOn );
            Cnf_DataFree( pCnfOff );
            sat_solver_delete( pSat );
            return;
        }
    }

    // add clauses of B
    for ( i = 0; i < pCnfOff->nClauses; i++ )
    {
        if ( !sat_solver_addclause( pSat, pCnfOff->pClauses[i], pCnfOff->pClauses[i+1] ) )
        {
            Cnf_DataFree( pCnfOn );
            Cnf_DataFree( pCnfOff );
            sat_solver_delete( pSat );
            return;
        }
    }

    // add PI clauses
    // collect the common variables
    Aig_ManForEachCi( pManOn, pObj, i )
    {
        pObj2 = Aig_ManCi( pManOff, i );

        Lits[0] = toLitCond( pCnfOn->pVarNums[pObj->Id], 0 );
        Lits[1] = toLitCond( pCnfOff->pVarNums[pObj2->Id], 1 );
        if ( !sat_solver_addclause( pSat, Lits, Lits+2 ) )
            assert( 0 );
        Lits[0] = toLitCond( pCnfOn->pVarNums[pObj->Id], 1 );
        Lits[1] = toLitCond( pCnfOff->pVarNums[pObj2->Id], 0 );
        if ( !sat_solver_addclause( pSat, Lits, Lits+2 ) )
            assert( 0 );
    }
    status = sat_solver_simplify( pSat );
    assert( status != 0 );

    // solve incremental SAT problems
    Aig_ManForEachCo( pManOn, pObj, i )
    {
        pObj2 = Aig_ManCo( pManOff, i );

        Lits[0] = toLitCond( pCnfOn->pVarNums[pObj->Id], 0 );
        Lits[1] = toLitCond( pCnfOff->pVarNums[pObj2->Id], 0 );
        status = sat_solver_solve( pSat, Lits, Lits+2, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
        if ( status != l_False )
            printf( "The incremental SAT problem is not UNSAT.\n" );
    }
    Cnf_DataFree( pCnfOn );
    Cnf_DataFree( pCnfOff );
    sat_solver_delete( pSat );
//    ABC_PRT( "Fast interpolation time", Abc_Clock() - clk );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Aig_ManInter( Aig_Man_t * pManOn, Aig_Man_t * pManOff, int fRelation, int fVerbose )
{
    void * pSatCnf = NULL;
    Inta_Man_t * pManInter; 
    Aig_Man_t * pRes;
    sat_solver * pSat;
    Cnf_Dat_t * pCnfOn, * pCnfOff;
    Vec_Int_t * vVarsAB;
    Aig_Obj_t * pObj, * pObj2;
    int Lits[3], status, i;
    abctime clk;
    int iLast = -1; // Suppress "might be used uninitialized"

    assert( Aig_ManCiNum(pManOn) == Aig_ManCiNum(pManOff) );

clk = Abc_Clock();
    // derive CNFs
//    pCnfOn  = Cnf_Derive( pManOn, 0 );
//    pCnfOff = Cnf_Derive( pManOff, 0 );
    pCnfOn  = Cnf_DeriveSimple( pManOn, 0 );
    pCnfOff = Cnf_DeriveSimple( pManOff, 0 );
    Cnf_DataLift( pCnfOff, pCnfOn->nVars );
timeCnf += Abc_Clock() - clk;

clk = Abc_Clock();
    // start the solver
    pSat = sat_solver_new();
    sat_solver_store_alloc( pSat );
    sat_solver_setnvars( pSat, pCnfOn->nVars + pCnfOff->nVars );

    // add clauses of A
    for ( i = 0; i < pCnfOn->nClauses; i++ )
    {
        if ( !sat_solver_addclause( pSat, pCnfOn->pClauses[i], pCnfOn->pClauses[i+1] ) )
        {
            Cnf_DataFree( pCnfOn );
            Cnf_DataFree( pCnfOff );
            sat_solver_delete( pSat );
            return NULL;
        }
    }
    sat_solver_store_mark_clauses_a( pSat );

    // update the last clause
    if ( fRelation )
    {
        extern int sat_solver_store_change_last( sat_solver * pSat );
        iLast = sat_solver_store_change_last( pSat );
    }

    // add clauses of B
    for ( i = 0; i < pCnfOff->nClauses; i++ )
    {
        if ( !sat_solver_addclause( pSat, pCnfOff->pClauses[i], pCnfOff->pClauses[i+1] ) )
        {
            Cnf_DataFree( pCnfOn );
            Cnf_DataFree( pCnfOff );
            sat_solver_delete( pSat );
            return NULL;
        }
    }

    // add PI clauses
    // collect the common variables
    vVarsAB = Vec_IntAlloc( Aig_ManCiNum(pManOn) );
    if ( fRelation )
        Vec_IntPush( vVarsAB, iLast );

    Aig_ManForEachCi( pManOn, pObj, i )
    {
        Vec_IntPush( vVarsAB, pCnfOn->pVarNums[pObj->Id] );
        pObj2 = Aig_ManCi( pManOff, i );

        Lits[0] = toLitCond( pCnfOn->pVarNums[pObj->Id], 0 );
        Lits[1] = toLitCond( pCnfOff->pVarNums[pObj2->Id], 1 );
        if ( !sat_solver_addclause( pSat, Lits, Lits+2 ) )
            assert( 0 );
        Lits[0] = toLitCond( pCnfOn->pVarNums[pObj->Id], 1 );
        Lits[1] = toLitCond( pCnfOff->pVarNums[pObj2->Id], 0 );
        if ( !sat_solver_addclause( pSat, Lits, Lits+2 ) )
            assert( 0 );
    }
    Cnf_DataFree( pCnfOn );
    Cnf_DataFree( pCnfOff );
    sat_solver_store_mark_roots( pSat );

/*
    status = sat_solver_simplify(pSat);
    if ( status == 0 )
    {
        Vec_IntFree( vVarsAB );
        Cnf_DataFree( pCnfOn );
        Cnf_DataFree( pCnfOff );
        sat_solver_delete( pSat );
        return NULL;
    }
*/

    // solve the problem
    status = sat_solver_solve( pSat, NULL, NULL, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
timeSat += Abc_Clock() - clk;
    if ( status == l_False )
    {
        pSatCnf = sat_solver_store_release( pSat );
//        printf( "unsat\n" );
    }
    else if ( status == l_True )
    {
//        printf( "sat\n" );
    }
    else
    {
//        printf( "undef\n" );
    }
    sat_solver_delete( pSat );
    if ( pSatCnf == NULL )
    {
        printf( "The SAT problem is not unsat.\n" );
        Vec_IntFree( vVarsAB );
        return NULL;
    }

    // create the resulting manager
clk = Abc_Clock();
    pManInter = Inta_ManAlloc();
    pRes = (Aig_Man_t *)Inta_ManInterpolate( pManInter, (Sto_Man_t *)pSatCnf, 0, vVarsAB, fVerbose );
    Inta_ManFree( pManInter );
timeInt += Abc_Clock() - clk;
/*
    // test UNSAT core computation
    {
        Intp_Man_t * pManProof;
        Vec_Int_t * vCore;
        pManProof = Intp_ManAlloc();
        vCore = Intp_ManUnsatCore( pManProof, pSatCnf, 0 );
        Intp_ManFree( pManProof );
        Vec_IntFree( vCore );
    }
*/
    Vec_IntFree( vVarsAB );
    Sto_ManFree( (Sto_Man_t *)pSatCnf );

//    Ioa_WriteAiger( pRes, "inter2.aig", 0, 0 );
    return pRes;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

