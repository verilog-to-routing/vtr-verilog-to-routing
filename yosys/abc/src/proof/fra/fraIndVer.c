/**CFile****************************************************************

  FileName    [fraIndVer.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [New FRAIG package.]

  Synopsis    [Verification of the inductive invariant.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 30, 2007.]

  Revision    [$Id: fraIndVer.c,v 1.00 2007/06/30 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fra.h"
#include "sat/cnf/cnf.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Verifies the inductive invariant.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_InvariantVerify( Aig_Man_t * pAig, int nFrames, Vec_Int_t * vClauses, Vec_Int_t * vLits )
{
    Cnf_Dat_t * pCnf;
    sat_solver * pSat;
    int * pStart;
    int RetValue, Beg, End, i, k;
    int CounterBase = 0, CounterInd = 0;
    abctime clk = Abc_Clock();

    if ( nFrames != 1 )
    {
        printf( "Invariant verification: Can only verify for K = 1\n" );
        return 1;
    }

    // derive CNF
    pCnf = Cnf_DeriveSimple( pAig, Aig_ManCoNum(pAig) );
/*
    // add the property
    {
        Aig_Obj_t * pObj;
        int Lits[1];

        pObj = Aig_ManCo( pAig, 0 );
        Lits[0] = toLitCond( pCnf->pVarNums[pObj->Id], 1 ); 

        Vec_IntPush( vLits, Lits[0] );
        Vec_IntPush( vClauses, Vec_IntSize(vLits) );
        printf( "Added the target property to the set of clauses to be inductively checked.\n" );
    }
*/
    // derive initialized frames for the base case
    pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, nFrames, 1 );
    // check clauses in the base case
    Beg = 0;
    pStart = Vec_IntArray( vLits );
    Vec_IntForEachEntry( vClauses, End, i )
    {
        // complement the literals
        for ( k = Beg; k < End; k++ )
            pStart[k] = lit_neg(pStart[k]);
        RetValue = sat_solver_solve( pSat, pStart + Beg, pStart + End, 0, 0, 0, 0 );
        for ( k = Beg; k < End; k++ )
            pStart[k] = lit_neg(pStart[k]);
        Beg = End;
        if ( RetValue == l_False )
            continue;
//        printf( "Clause %d failed the base case.\n", i );
        CounterBase++;
    }
    sat_solver_delete( pSat );

    // derive initialized frames for the base case
    pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, nFrames + 1, 0 );
    assert( pSat->size == 2 * pCnf->nVars );
    // add clauses to the first frame
    Beg = 0;
    pStart = Vec_IntArray( vLits );
    Vec_IntForEachEntry( vClauses, End, i )
    {
        RetValue = sat_solver_addclause( pSat, pStart + Beg, pStart + End );
        Beg = End;
        if ( RetValue == 0 )
        {
            Cnf_DataFree( pCnf );
            sat_solver_delete( pSat );
            printf( "Invariant verification: SAT solver is unsat after adding a clause.\n" );
            return 0;
        }
    }
    // simplify the solver
    if ( pSat->qtail != pSat->qhead )
    {
        RetValue = sat_solver_simplify(pSat);
        assert( RetValue != 0 );
        assert( pSat->qtail == pSat->qhead );
    }

    // check clauses in the base case
    Beg = 0;
    pStart = Vec_IntArray( vLits );
    Vec_IntForEachEntry( vClauses, End, i )
    {
        // complement the literals
        for ( k = Beg; k < End; k++ )
        {
            pStart[k] += 2 * pCnf->nVars;
            pStart[k] = lit_neg(pStart[k]);
        }
        RetValue = sat_solver_solve( pSat, pStart + Beg, pStart + End, 0, 0, 0, 0 );
        for ( k = Beg; k < End; k++ )
        {
            pStart[k] = lit_neg(pStart[k]);
            pStart[k] -= 2 * pCnf->nVars;
        }
        Beg = End;
        if ( RetValue == l_False )
            continue;
//        printf( "Clause %d failed the inductive case.\n", i );
        CounterInd++;
    }
    sat_solver_delete( pSat );
    Cnf_DataFree( pCnf );
    if ( CounterBase )
        printf( "Invariant verification: %d clauses (out of %d) FAILED the base case.\n", CounterBase, Vec_IntSize(vClauses) );
    if ( CounterInd )
        printf( "Invariant verification: %d clauses (out of %d) FAILED the inductive case.\n", CounterInd, Vec_IntSize(vClauses) );
    if ( CounterBase || CounterInd )
        return 0;
    printf( "Invariant verification: %d clauses verified correctly.  ", Vec_IntSize(vClauses) );
    ABC_PRT( "Time", Abc_Clock() - clk );
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

