/**CFile****************************************************************

  FileName    [aigRepar.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [Interpolation-based reparametrization.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aigRepar.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satSolver2.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Adds buffer to the solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Aig_ManInterAddBuffer( sat_solver2 * pSat, int iVarA, int iVarB, int fCompl, int fMark )
{
    lit Lits[2];
    int Cid;
    assert( iVarA >= 0 && iVarB >= 0 );

    Lits[0] = toLitCond( iVarA, 0 );
    Lits[1] = toLitCond( iVarB, !fCompl );
    Cid = sat_solver2_addclause( pSat, Lits, Lits + 2, 0 );
    if ( fMark )
        clause2_set_partA( pSat, Cid, 1 );

    Lits[0] = toLitCond( iVarA, 1 );
    Lits[1] = toLitCond( iVarB, fCompl );
    Cid = sat_solver2_addclause( pSat, Lits, Lits + 2, 0 );
    if ( fMark )
        clause2_set_partA( pSat, Cid, 1 );
}

/**Function*************************************************************

  Synopsis    [Adds constraints for the two-input AND-gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Aig_ManInterAddXor( sat_solver2 * pSat, int iVarA, int iVarB, int iVarC, int fCompl, int fMark )
{
    lit Lits[3];
    int Cid;
    assert( iVarA >= 0 && iVarB >= 0 && iVarC >= 0 );

    Lits[0] = toLitCond( iVarA, !fCompl );
    Lits[1] = toLitCond( iVarB, 1 );
    Lits[2] = toLitCond( iVarC, 1 );
    Cid = sat_solver2_addclause( pSat, Lits, Lits + 3, 0 );
    if ( fMark )
        clause2_set_partA( pSat, Cid, 1 );

    Lits[0] = toLitCond( iVarA, !fCompl );
    Lits[1] = toLitCond( iVarB, 0 );
    Lits[2] = toLitCond( iVarC, 0 );
    Cid = sat_solver2_addclause( pSat, Lits, Lits + 3, 0 );
    if ( fMark )
        clause2_set_partA( pSat, Cid, 1 );

    Lits[0] = toLitCond( iVarA, fCompl );
    Lits[1] = toLitCond( iVarB, 1 );
    Lits[2] = toLitCond( iVarC, 0 );
    Cid = sat_solver2_addclause( pSat, Lits, Lits + 3, 0 );
    if ( fMark )
        clause2_set_partA( pSat, Cid, 1 );

    Lits[0] = toLitCond( iVarA, fCompl );
    Lits[1] = toLitCond( iVarB, 0 );
    Lits[2] = toLitCond( iVarC, 1 );
    Cid = sat_solver2_addclause( pSat, Lits, Lits + 3, 0 );
    if ( fMark )
        clause2_set_partA( pSat, Cid, 1 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManInterTest( Aig_Man_t * pMan, int fVerbose )
{
    sat_solver2 * pSat;
//    Aig_Man_t * pInter;
    word * pInter;
    Vec_Int_t * vVars;
    Cnf_Dat_t * pCnf;
    Aig_Obj_t * pObj;
    int Lit, Cid, Var, status, i;
    clock_t clk = clock();
    assert( Aig_ManRegNum(pMan) == 0 );
    assert( Aig_ManCoNum(pMan) == 1 );

    // derive CNFs
    pCnf = Cnf_Derive( pMan, 1 );

    // start the solver
    pSat = sat_solver2_new();
    sat_solver2_setnvars( pSat, 2*pCnf->nVars+1 );
    // set A-variables (all used except PI/PO)
    Aig_ManForEachObj( pMan, pObj, i )
    {
        if ( pCnf->pVarNums[pObj->Id] < 0 )
            continue;
        if ( !Aig_ObjIsCi(pObj) && !Aig_ObjIsCo(pObj) )
            var_set_partA( pSat, pCnf->pVarNums[pObj->Id], 1 );
    }

    // add clauses of A
    for ( i = 0; i < pCnf->nClauses; i++ )
    {
        Cid = sat_solver2_addclause( pSat, pCnf->pClauses[i], pCnf->pClauses[i+1], 0 );
        clause2_set_partA( pSat, Cid, 1 );
    }

    // add clauses of B
    Cnf_DataLift( pCnf, pCnf->nVars );
    for ( i = 0; i < pCnf->nClauses; i++ )
        sat_solver2_addclause( pSat, pCnf->pClauses[i], pCnf->pClauses[i+1], 0 );
    Cnf_DataLift( pCnf, -pCnf->nVars );

    // add PI equality clauses
    vVars = Vec_IntAlloc( Aig_ManCoNum(pMan)+1 );
    Aig_ManForEachCi( pMan, pObj, i )
    {
        if ( Aig_ObjRefs(pObj) == 0 )
            continue;
        Var = pCnf->pVarNums[pObj->Id];
        Aig_ManInterAddBuffer( pSat, Var, pCnf->nVars + Var, 0, 0 );
        Vec_IntPush( vVars, Var );
    }

    // add an XOR clause in the end
    Var = pCnf->pVarNums[Aig_ManCo(pMan,0)->Id];
    Aig_ManInterAddXor( pSat, Var, pCnf->nVars + Var, 2*pCnf->nVars, 0, 0 );
    Vec_IntPush( vVars, Var );

    // solve the problem
    Lit = toLitCond( 2*pCnf->nVars, 0 );
    status = sat_solver2_solve( pSat, &Lit, &Lit + 1, 0, 0, 0, 0 );
    assert( status == l_False );
    Sat_Solver2PrintStats( stdout, pSat );

    // derive interpolant
//    pInter = Sat_ProofInterpolant( pSat, vVars );
//    Aig_ManPrintStats( pInter );
//    Aig_ManDumpBlif( pInter, "int.blif", NULL, NULL );
//pInter = Sat_ProofInterpolantTruth( pSat, vVars );
    pInter = NULL;
//    Extra_PrintHex( stdout, pInter, Vec_IntSize(vVars) ); printf( "\n" );

    // clean up
//    Aig_ManStop( pInter );
    ABC_FREE( pInter );

    Vec_IntFree( vVars );
    Cnf_DataFree( pCnf );
    sat_solver2_delete( pSat );
    ABC_PRT( "Total interpolation time", clock() - clk );
}

/**Function*************************************************************

  Synopsis    [Duplicates AIG while mapping PIs into the given array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManAppend( Aig_Man_t * pBase, Aig_Man_t * pNew )
{
    Aig_Obj_t * pObj;
    int i;
    assert( Aig_ManCoNum(pNew) == 1 );
    assert( Aig_ManCiNum(pNew) == Aig_ManCiNum(pBase) );
    // create the PIs
    Aig_ManCleanData( pNew );
    Aig_ManConst1(pNew)->pData = Aig_ManConst1(pBase);
    Aig_ManForEachCi( pNew, pObj, i )
        pObj->pData = Aig_IthVar(pBase, i);
    // duplicate internal nodes
    Aig_ManForEachNode( pNew, pObj, i )
        pObj->pData = Aig_And( pBase, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    // add one PO to base
    pObj = Aig_ManCo( pNew, 0 );
    Aig_ObjCreateCo( pBase, Aig_ObjChild0Copy(pObj) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Aig_ManInterRepar( Aig_Man_t * pMan, int fVerbose )
{
    Aig_Man_t * pAigTemp, * pInter, * pBase = NULL;
    sat_solver2 * pSat;
    Vec_Int_t * vVars;
    Cnf_Dat_t * pCnf, * pCnfInter;
    Aig_Obj_t * pObj;
    int nOuts = Aig_ManCoNum(pMan);
    int ShiftP[2], ShiftCnf[2], ShiftOr[2], ShiftAssume;
    int Cid, Lit, status, i, k, c;
    clock_t clk = clock();
    assert( Aig_ManRegNum(pMan) == 0 );

    // derive CNFs
    pCnf = Cnf_Derive( pMan, nOuts );

    // start the solver
    pSat = sat_solver2_new();
    sat_solver2_setnvars( pSat, 4*pCnf->nVars + 6*nOuts );
    // vars: pGlobal + (p0 + A1 + A2 + or0) + (p1 + B1 + B2 + or1) + pAssume;
    ShiftP[0] = nOuts;
    ShiftP[1] = 2*pCnf->nVars + 3*nOuts;
    ShiftCnf[0] = ShiftP[0] + nOuts;
    ShiftCnf[1] = ShiftP[1] + nOuts;
    ShiftOr[0] = ShiftCnf[0] + 2*pCnf->nVars;
    ShiftOr[1] = ShiftCnf[1] + 2*pCnf->nVars;
    ShiftAssume = ShiftOr[1] + nOuts;
    assert( ShiftAssume + nOuts == pSat->size );

    // mark variables of A
    for ( i = ShiftCnf[0]; i < ShiftP[1]; i++ )
        var_set_partA( pSat, i, 1 );

    // add clauses of A, then B
    vVars = Vec_IntAlloc( 2*nOuts );
    for ( k = 0; k < 2; k++ )
    {
        // copy A1
        Cnf_DataLift( pCnf, ShiftCnf[k] );
        for ( i = 0; i < pCnf->nClauses; i++ )
        {
            Cid = sat_solver2_addclause( pSat, pCnf->pClauses[i], pCnf->pClauses[i+1], 0 );
            clause2_set_partA( pSat, Cid, k==0 );
        }
        // add equality p[k] == A1/B1
        Aig_ManForEachCo( pMan, pObj, i )
            Aig_ManInterAddBuffer( pSat, ShiftP[k] + i, pCnf->pVarNums[pObj->Id], k==1, k==0 );
        // copy A2
        Cnf_DataLift( pCnf, pCnf->nVars );
        for ( i = 0; i < pCnf->nClauses; i++ )
        {
            Cid = sat_solver2_addclause( pSat, pCnf->pClauses[i], pCnf->pClauses[i+1], 0 );
            clause2_set_partA( pSat, Cid, k==0 );
        }
        // add comparator (!p[k] ^ A2/B2) == or[k]
        Vec_IntClear( vVars );
        Aig_ManForEachCo( pMan, pObj, i )
        {
            Aig_ManInterAddXor( pSat, ShiftP[k] + i, pCnf->pVarNums[pObj->Id], ShiftOr[k] + i, k==1, k==0 );
            Vec_IntPush( vVars, toLitCond(ShiftOr[k] + i, 1) );
        }
        Cid = sat_solver2_addclause( pSat, Vec_IntArray(vVars), Vec_IntArray(vVars) + Vec_IntSize(vVars), 0 );
        clause2_set_partA( pSat, Cid, k==0 );
        // return to normal
        Cnf_DataLift( pCnf, -ShiftCnf[k]-pCnf->nVars );
    }
    // add clauses to constrain p[0] and p[1]
    for ( k = 0; k < nOuts; k++ )
        Aig_ManInterAddXor( pSat, ShiftP[0] + k, ShiftP[1] + k, ShiftAssume + k, 0, 0 );

    // start the interpolant
    pBase = Aig_ManStart( 1000 );
    pBase->pName = Abc_UtilStrsav( "repar" );
    for ( k = 0; k < 2*nOuts; k++ )
        Aig_IthVar(pBase, i);

    // start global variables (pGlobal and p[0])
    Vec_IntClear( vVars );
    for ( k = 0; k < 2*nOuts; k++ )
        Vec_IntPush( vVars, k );

    // perform iterative solving
    // vars: pGlobal + (p0 + A1 + A2 + or0) + (p1 + B1 + B2 + or1) + pAssume;
    for ( k = 0; k < nOuts; k++ )
    {
        // swap k-th variables
        int Temp = Vec_IntEntry( vVars, k );
        Vec_IntWriteEntry( vVars, k, Vec_IntEntry(vVars, nOuts+k) );
        Vec_IntWriteEntry( vVars, nOuts+k, Temp );

        // solve incrementally 
        Lit = toLitCond( ShiftAssume + k, 1 ); // XOR output is 0 ==> p1 == p2
        status = sat_solver2_solve( pSat, &Lit, &Lit + 1, 0, 0, 0, 0 );
        assert( status == l_False );
        Sat_Solver2PrintStats( stdout, pSat );

        // derive interpolant
        pInter = (Aig_Man_t *)Sat_ProofInterpolant( pSat, vVars );
        Aig_ManPrintStats( pInter );
        // make sure interpolant does not depend on useless vars
        Aig_ManForEachCi( pInter, pObj, i )
            assert( i <= k || Aig_ObjRefs(pObj) == 0 );

        // simplify
        pInter = Dar_ManRwsat( pAigTemp = pInter, 1, 0 );
        Aig_ManStop( pAigTemp );

        // add interpolant to the solver
        pCnfInter = Cnf_Derive( pInter, 1 );
        Cnf_DataLift( pCnfInter, pSat->size );
        sat_solver2_setnvars( pSat, pSat->size + 2*pCnfInter->nVars );
        for ( i = 0; i < pCnfInter->nVars; i++ )
            var_set_partA( pSat, pSat->size-2*pCnfInter->nVars + i, 1 );
        for ( c = 0; c < 2; c++ )
        {
            if ( c == 1 )
                Cnf_DataLift( pCnfInter, pCnfInter->nVars );
            // add to A
            for ( i = 0; i < pCnfInter->nClauses; i++ )
            {
                Cid = sat_solver2_addclause( pSat, pCnfInter->pClauses[i], pCnfInter->pClauses[i+1], 0 );
                clause2_set_partA( pSat, Cid, c==0 );
            }
            // connect to the inputs
            Aig_ManForEachCi( pInter, pObj, i )
                if ( i <= k )
                    Aig_ManInterAddBuffer( pSat, i, pCnf->pVarNums[pObj->Id], 0, c==0 );
            // connect to the outputs
            pObj = Aig_ManCo(pInter, 0);
            Aig_ManInterAddBuffer( pSat, ShiftP[c] + k, pCnf->pVarNums[pObj->Id], 0, c==0 );
            if ( c == 1 )
                Cnf_DataLift( pCnfInter, -pCnfInter->nVars );
        }
        Cnf_DataFree( pCnfInter );

        // accumulate
        Aig_ManAppend( pBase, pInter );
        Aig_ManStop( pInter );

        // update global variables
        Temp = Vec_IntEntry( vVars, k );
        Vec_IntWriteEntry( vVars, k, Vec_IntEntry(vVars, nOuts+k) );
        Vec_IntWriteEntry( vVars, nOuts+k, Temp );
    }

    Vec_IntFree( vVars );
    Cnf_DataFree( pCnf );
    sat_solver2_delete( pSat );
    ABC_PRT( "Reparameterization time", clock() - clk );
    return pBase;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

