/**CFile****************************************************************

  FileName    [intM114p.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Interpolation engine.]

  Synopsis    [Intepolation using interfaced to MiniSat-1.14p.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 24, 2008.]

  Revision    [$Id: intM114p.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "intInt.h"
#include "sat/psat/m114p.h"

#ifdef ABC_USE_LIBRARIES

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns the SAT solver for one interpolation run.]

  Description [pInter is the previous interpolant. pAig is one time frame.
  pFrames is the unrolled time frames.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
M114p_Solver_t Inter_ManDeriveSatSolverM114p( 
    Aig_Man_t * pInter, Cnf_Dat_t * pCnfInter, 
    Aig_Man_t * pAig, Cnf_Dat_t * pCnfAig, 
    Aig_Man_t * pFrames, Cnf_Dat_t * pCnfFrames, 
    Vec_Int_t ** pvMapRoots, Vec_Int_t ** pvMapVars )
{
    M114p_Solver_t pSat;
    Aig_Obj_t * pObj, * pObj2;
    int i, Lits[2];

    // sanity checks
    assert( Aig_ManRegNum(pInter) == 0 );
    assert( Aig_ManRegNum(pAig) > 0 );
    assert( Aig_ManRegNum(pFrames) == 0 );
    assert( Aig_ManCoNum(pInter) == 1 );
    assert( Aig_ManCoNum(pFrames) == 1 );
    assert( Aig_ManCiNum(pInter) == Aig_ManRegNum(pAig) );
//    assert( (Aig_ManCiNum(pFrames) - Aig_ManRegNum(pAig)) % Saig_ManPiNum(pAig) == 0 );

    // prepare CNFs
    Cnf_DataLift( pCnfAig,   pCnfFrames->nVars );
    Cnf_DataLift( pCnfInter, pCnfFrames->nVars + pCnfAig->nVars );

    *pvMapRoots = Vec_IntAlloc( 10000 );
    *pvMapVars = Vec_IntAlloc( 0 );
    Vec_IntFill( *pvMapVars, pCnfInter->nVars + pCnfAig->nVars + pCnfFrames->nVars, -1 );
    for ( i = 0; i < pCnfFrames->nVars; i++ )
        Vec_IntWriteEntry( *pvMapVars, i, -2 );

    // start the solver
    pSat = M114p_SolverNew( 1 );
    M114p_SolverSetVarNum( pSat, pCnfInter->nVars + pCnfAig->nVars + pCnfFrames->nVars );

    // add clauses of A
    // interpolant
    for ( i = 0; i < pCnfInter->nClauses; i++ )
    {
        Vec_IntPush( *pvMapRoots, 0 );
        if ( !M114p_SolverAddClause( pSat, pCnfInter->pClauses[i], pCnfInter->pClauses[i+1] ) )
            assert( 0 );
    }
    // connector clauses
    Aig_ManForEachCi( pInter, pObj, i )
    {
        pObj2 = Saig_ManLo( pAig, i );
        Lits[0] = toLitCond( pCnfInter->pVarNums[pObj->Id], 0 );
        Lits[1] = toLitCond( pCnfAig->pVarNums[pObj2->Id], 1 );
        Vec_IntPush( *pvMapRoots, 0 );
        if ( !M114p_SolverAddClause( pSat, Lits, Lits+2 ) )
            assert( 0 );
        Lits[0] = toLitCond( pCnfInter->pVarNums[pObj->Id], 1 );
        Lits[1] = toLitCond( pCnfAig->pVarNums[pObj2->Id], 0 );
        Vec_IntPush( *pvMapRoots, 0 );
        if ( !M114p_SolverAddClause( pSat, Lits, Lits+2 ) )
            assert( 0 );
    }
    // one timeframe
    for ( i = 0; i < pCnfAig->nClauses; i++ )
    {
        Vec_IntPush( *pvMapRoots, 0 );
        if ( !M114p_SolverAddClause( pSat, pCnfAig->pClauses[i], pCnfAig->pClauses[i+1] ) )
            assert( 0 );
    }
    // connector clauses
    Aig_ManForEachCi( pFrames, pObj, i )
    { 
        if ( i == Aig_ManRegNum(pAig) )
            break;
//        Vec_IntPush( vVarsAB, pCnfFrames->pVarNums[pObj->Id] );
        Vec_IntWriteEntry( *pvMapVars, pCnfFrames->pVarNums[pObj->Id], i );

        pObj2 = Saig_ManLi( pAig, i );
        Lits[0] = toLitCond( pCnfFrames->pVarNums[pObj->Id], 0 );
        Lits[1] = toLitCond( pCnfAig->pVarNums[pObj2->Id], 1 );
        Vec_IntPush( *pvMapRoots, 0 );
        if ( !M114p_SolverAddClause( pSat, Lits, Lits+2 ) )
            assert( 0 );
        Lits[0] = toLitCond( pCnfFrames->pVarNums[pObj->Id], 1 );
        Lits[1] = toLitCond( pCnfAig->pVarNums[pObj2->Id], 0 );
        Vec_IntPush( *pvMapRoots, 0 );
        if ( !M114p_SolverAddClause( pSat, Lits, Lits+2 ) )
            assert( 0 );
    }
    // add clauses of B
    for ( i = 0; i < pCnfFrames->nClauses; i++ )
    {
        Vec_IntPush( *pvMapRoots, 1 );
        if ( !M114p_SolverAddClause( pSat, pCnfFrames->pClauses[i], pCnfFrames->pClauses[i+1] ) )
        {
//            assert( 0 );
            break;
        }
    }
    // return clauses to the original state
    Cnf_DataLift( pCnfAig, -pCnfFrames->nVars );
    Cnf_DataLift( pCnfInter, -pCnfFrames->nVars -pCnfAig->nVars );
    return pSat;
}


/**Function*************************************************************

  Synopsis    [Performs one resolution step.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Inter_ManResolveM114p( Vec_Int_t * vResolvent, int * pLits, int nLits, int iVar )
{
    int i, k, iLit = -1, fFound = 0;
    // find the variable in the clause
    for ( i = 0; i < vResolvent->nSize; i++ )
        if ( lit_var(vResolvent->pArray[i]) == iVar )
        {
            iLit = vResolvent->pArray[i];
            vResolvent->pArray[i] = vResolvent->pArray[--vResolvent->nSize];
            break;
        }
    assert( iLit != -1 );
    // add other variables
    for ( i = 0; i < nLits; i++ )
    {
        if ( lit_var(pLits[i]) == iVar )
        {
            assert( iLit == lit_neg(pLits[i]) );
            fFound = 1;
            continue;
        }
        // check if this literal appears
        for ( k = 0; k < vResolvent->nSize; k++ )
            if ( vResolvent->pArray[k] == pLits[i] )
                break;
        if ( k < vResolvent->nSize )
            continue;
        // add this literal
        Vec_IntPush( vResolvent, pLits[i] );
    }
    assert( fFound );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Computes interpolant using MiniSat-1.14p.]

  Description [Assumes that the solver returned UNSAT and proof
  logging was enabled. Array vMapRoots maps number of each root clause 
  into 0 (clause of A) or 1 (clause of B). Array vMapVars maps each SAT
  solver variable into -1 (var of A), -2 (var of B), and <num> (var of C),
  where <num> is the var's 0-based number in the ordering of C variables.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Inter_ManInterpolateM114pPudlak( M114p_Solver_t s, Vec_Int_t * vMapRoots, Vec_Int_t * vMapVars )
{
    Aig_Man_t * p;
    Aig_Obj_t * pInter, * pInter2, * pVar;
    Vec_Ptr_t * vInters;
    Vec_Int_t * vLiterals, * vClauses, * vResolvent;
    int * pLitsNext, nLitsNext, nOffset, iLit;
    int * pLits, * pClauses, * pVars;
    int nLits, nVars, i, k, v, iVar;
    assert( M114p_SolverProofIsReady(s) );
    vInters = Vec_PtrAlloc( 1000 );

    vLiterals = Vec_IntAlloc( 10000 );
    vClauses = Vec_IntAlloc( 1000 );
    vResolvent = Vec_IntAlloc( 100 );

    // create elementary variables
    p = Aig_ManStart( 10000 );
    Vec_IntForEachEntry( vMapVars, iVar, i )
        if ( iVar >= 0 )
            Aig_IthVar(p, iVar);
    // process root clauses
    M114p_SolverForEachRoot( s, &pLits, nLits, i )
    {
        if ( Vec_IntEntry(vMapRoots, i) == 1 ) // clause of B
            pInter = Aig_ManConst1(p);
        else // clause of A
            pInter = Aig_ManConst0(p);
        Vec_PtrPush( vInters, pInter );

        // save the root clause
        Vec_IntPush( vClauses, Vec_IntSize(vLiterals) );
        Vec_IntPush( vLiterals, nLits );
        for ( v = 0; v < nLits; v++ )
            Vec_IntPush( vLiterals, pLits[v] );
    }
    assert( Vec_PtrSize(vInters) == Vec_IntSize(vMapRoots) );

    // process learned clauses
    M114p_SolverForEachChain( s, &pClauses, &pVars, nVars, i )
    {
        pInter = Vec_PtrEntry( vInters, pClauses[0] );

        // initialize the resolvent
        nOffset = Vec_IntEntry( vClauses, pClauses[0] );
        nLitsNext = Vec_IntEntry( vLiterals, nOffset );
        pLitsNext = Vec_IntArray(vLiterals) + nOffset + 1;
        Vec_IntClear( vResolvent );
        for ( v = 0; v < nLitsNext; v++ )
            Vec_IntPush( vResolvent, pLitsNext[v] );

        for ( k = 0; k < nVars; k++ )
        {
            iVar = Vec_IntEntry( vMapVars, pVars[k] );
            pInter2 = Vec_PtrEntry( vInters, pClauses[k+1] );

            // resolve it with the next clause
            nOffset = Vec_IntEntry( vClauses, pClauses[k+1] );
            nLitsNext = Vec_IntEntry( vLiterals, nOffset );
            pLitsNext = Vec_IntArray(vLiterals) + nOffset + 1;
            Inter_ManResolveM114p( vResolvent, pLitsNext, nLitsNext, pVars[k] );

            if ( iVar == -1 ) // var of A
                pInter = Aig_Or( p, pInter, pInter2 );
            else if ( iVar == -2 ) // var of B
                pInter = Aig_And( p, pInter, pInter2 );
            else // var of C
            {
                // check polarity of the pivot variable in the clause
                for ( v = 0; v < nLitsNext; v++ )
                    if ( lit_var(pLitsNext[v]) == pVars[k] )
                        break;
                assert( v < nLitsNext );
                pVar = Aig_NotCond( Aig_IthVar(p, iVar), lit_sign(pLitsNext[v]) );
                pInter = Aig_Mux( p, pVar, pInter, pInter2 );
            }
        }
        Vec_PtrPush( vInters, pInter );

        // store the resulting clause
        Vec_IntPush( vClauses, Vec_IntSize(vLiterals) );
        Vec_IntPush( vLiterals, Vec_IntSize(vResolvent) );
        Vec_IntForEachEntry( vResolvent, iLit, v )
            Vec_IntPush( vLiterals, iLit );
    }
    assert( Vec_PtrSize(vInters) == M114p_SolverProofClauseNum(s) );
    assert( Vec_IntSize(vResolvent) == 0 ); // the empty clause
    Vec_PtrFree( vInters );
    Vec_IntFree( vLiterals );
    Vec_IntFree( vClauses );
    Vec_IntFree( vResolvent );
    Aig_ObjCreateCo( p, pInter );
    Aig_ManCleanup( p );
    return p;
}


/**Function*************************************************************

  Synopsis    [Computes interpolant using MiniSat-1.14p.]

  Description [Assumes that the solver returned UNSAT and proof
  logging was enabled. Array vMapRoots maps number of each root clause 
  into 0 (clause of A) or 1 (clause of B). Array vMapVars maps each SAT
  solver variable into -1 (var of A), -2 (var of B), and <num> (var of C),
  where <num> is the var's 0-based number in the ordering of C variables.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Inter_ManpInterpolateM114( M114p_Solver_t s, Vec_Int_t * vMapRoots, Vec_Int_t * vMapVars )
{
    Aig_Man_t * p;
    Aig_Obj_t * pInter, * pInter2, * pVar;
    Vec_Ptr_t * vInters;
    int * pLits, * pClauses, * pVars;
    int nLits, nVars, i, k, iVar;
    int nClauses;

    nClauses = M114p_SolverProofClauseNum(s);

    assert( M114p_SolverProofIsReady(s) );

    vInters = Vec_PtrAlloc( 1000 );
    // process root clauses
    p = Aig_ManStart( 10000 );
    M114p_SolverForEachRoot( s, &pLits, nLits, i )
    {
        if ( Vec_IntEntry(vMapRoots, i) == 1 ) // clause of B
            pInter = Aig_ManConst1(p);
        else // clause of A
        {
            pInter = Aig_ManConst0(p);
            for ( k = 0; k < nLits; k++ )
            {
                iVar = Vec_IntEntry( vMapVars, lit_var(pLits[k]) );
                if ( iVar < 0 ) // var of A or B
                    continue;
                // this is a variable of C
                pVar = Aig_NotCond( Aig_IthVar(p, iVar), lit_sign(pLits[k]) );
                pInter = Aig_Or( p, pInter, pVar );
            }
        }
        Vec_PtrPush( vInters, pInter );
    } 
//    assert( Vec_PtrSize(vInters) == Vec_IntSize(vMapRoots) );

    // process learned clauses
    M114p_SolverForEachChain( s, &pClauses, &pVars, nVars, i )
    {
        pInter = Vec_PtrEntry( vInters, pClauses[0] );
        for ( k = 0; k < nVars; k++ )
        {
            iVar = Vec_IntEntry( vMapVars, pVars[k] );
            pInter2 = Vec_PtrEntry( vInters, pClauses[k+1] );
            if ( iVar == -1 ) // var of A
                pInter = Aig_Or( p, pInter, pInter2 );
            else // var of B or C
                pInter = Aig_And( p, pInter, pInter2 );
        }
        Vec_PtrPush( vInters, pInter );
    }

    assert( Vec_PtrSize(vInters) == M114p_SolverProofClauseNum(s) );
    Vec_PtrFree( vInters );
    Aig_ObjCreateCo( p, pInter );
    Aig_ManCleanup( p );
    assert( Aig_ManCheck(p) );
    return p;
}


/**Function*************************************************************

  Synopsis    [Performs one SAT run with interpolation.]

  Description [Returns 1 if proven. 0 if failed. -1 if undecided.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Inter_ManPerformOneStepM114p( Inter_Man_t * p, int fUsePudlak, int fUseOther )
{
    M114p_Solver_t pSat;
    Vec_Int_t * vMapRoots, * vMapVars;
    clock_t clk;
    int status, RetValue;
    assert( p->pInterNew == NULL );
    // derive the SAT solver
    pSat = Inter_ManDeriveSatSolverM114p( p->pInter, p->pCnfInter, 
        p->pAigTrans, p->pCnfAig, p->pFrames, p->pCnfFrames, 
        &vMapRoots, &vMapVars );
    // solve the problem
clk = clock();
    status = M114p_SolverSolve( pSat, NULL, NULL, 0 );
    p->nConfCur = M114p_SolverGetConflictNum( pSat );
p->timeSat += clock() - clk;
    if ( status == 0 )
    {
        RetValue = 1;
//        Inter_ManpInterpolateM114Report( pSat, vMapRoots, vMapVars );

clk = clock();
        if ( fUsePudlak )
            p->pInterNew = Inter_ManInterpolateM114pPudlak( pSat, vMapRoots, vMapVars );
        else
            p->pInterNew = Inter_ManpInterpolateM114( pSat, vMapRoots, vMapVars );
p->timeInt += clock() - clk;
    }
    else if ( status == 1 )
    {
        RetValue = 0;
    }
    else
    {
        RetValue = -1;
    }
    M114p_SolverDelete( pSat );
    Vec_IntFree( vMapRoots );
    Vec_IntFree( vMapVars );
    return RetValue;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END

#endif


