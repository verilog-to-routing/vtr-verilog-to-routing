/**CFile****************************************************************

  FileName    [sfmSat.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based optimization using internal don't-cares.]

  Synopsis    [SAT-based procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: sfmSat.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sfmInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static word s_Truths6[6] = {
    ABC_CONST(0xAAAAAAAAAAAAAAAA),
    ABC_CONST(0xCCCCCCCCCCCCCCCC),
    ABC_CONST(0xF0F0F0F0F0F0F0F0),
    ABC_CONST(0xFF00FF00FF00FF00),
    ABC_CONST(0xFFFF0000FFFF0000),
    ABC_CONST(0xFFFFFFFF00000000)
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Converts a window into a SAT solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sfm_NtkWindowToSolver( Sfm_Ntk_t * p )
{
    // p->vOrder contains all variables in the window in a good order
    // p->vDivs is a subset of nodes in p->vOrder used as divisor candidates
    // p->vTfo contains TFO of the node (does not include node)
    // p->vRoots contains roots of the TFO of the node (may include node)
    Vec_Int_t * vClause;
    int RetValue, iNode = -1, iFanin, i, k;
    abctime clk = Abc_Clock();
//    if ( p->pSat )
//        printf( "%d  ", p->pSat->stats.learnts );
    sat_solver_restart( p->pSat );
    sat_solver_setnvars( p->pSat, 1 + Vec_IntSize(p->vOrder) + Vec_IntSize(p->vTfo) + Vec_IntSize(p->vRoots) + 10 );
    // create SAT variables
    Sfm_NtkCleanVars( p );
    p->nSatVars = 1;
    Vec_IntForEachEntry( p->vOrder, iNode, i )
        Sfm_ObjSetSatVar( p, iNode, p->nSatVars++ );
    // collect divisor variables
    Vec_IntClear( p->vDivVars );
    Vec_IntForEachEntry( p->vDivs, iNode, i )
        Vec_IntPush( p->vDivVars, Sfm_ObjSatVar(p, iNode) );
    // add CNF clauses for the TFI
    Vec_IntForEachEntry( p->vOrder, iNode, i )
    {
        if ( Sfm_ObjIsPi(p, iNode) )
            continue;
        // collect fanin variables
        Vec_IntClear( p->vFaninMap );
        Sfm_ObjForEachFanin( p, iNode, iFanin, k )
            Vec_IntPush( p->vFaninMap, Sfm_ObjSatVar(p, iFanin) );
        Vec_IntPush( p->vFaninMap, Sfm_ObjSatVar(p, iNode) );
        // generate CNF 
        Sfm_TranslateCnf( p->vClauses, (Vec_Str_t *)Vec_WecEntry(p->vCnfs, iNode), p->vFaninMap, -1 );
        // add clauses
        Vec_WecForEachLevel( p->vClauses, vClause, k )
        {
            if ( Vec_IntSize(vClause) == 0 )
                break;
            RetValue = sat_solver_addclause( p->pSat, Vec_IntArray(vClause), Vec_IntArray(vClause) + Vec_IntSize(vClause) );
            if ( RetValue == 0 )
                return 0;
        }
    }
    if ( Vec_IntSize(p->vTfo) > 0 )
    {
        assert( p->pPars->nTfoLevMax > 0 );
        assert( Vec_IntSize(p->vRoots) > 0 );
        assert( Vec_IntEntry(p->vTfo, 0) != p->iPivotNode );
        // collect variables of root nodes
        Vec_IntClear( p->vLits );
        Vec_IntForEachEntry( p->vRoots, iNode, i )
            Vec_IntPush( p->vLits, Sfm_ObjSatVar(p, iNode) );
        // assign new variables to the TFO nodes
        Vec_IntForEachEntry( p->vTfo, iNode, i )
        {
            Sfm_ObjCleanSatVar( p, Sfm_ObjSatVar(p, iNode) );
            Sfm_ObjSetSatVar( p, iNode, p->nSatVars++ );
        }
        // add CNF clauses for the TFO
        Vec_IntForEachEntry( p->vTfo, iNode, i )
        {
            assert( Sfm_ObjIsNode(p, iNode) );
            // collect fanin variables
            Vec_IntClear( p->vFaninMap );
            Sfm_ObjForEachFanin( p, iNode, iFanin, k )
                Vec_IntPush( p->vFaninMap, Sfm_ObjSatVar(p, iFanin) );
            Vec_IntPush( p->vFaninMap, Sfm_ObjSatVar(p, iNode) );
            // generate CNF 
            Sfm_TranslateCnf( p->vClauses, (Vec_Str_t *)Vec_WecEntry(p->vCnfs, iNode), p->vFaninMap, Sfm_ObjSatVar(p, p->iPivotNode) );
            // add clauses
            Vec_WecForEachLevel( p->vClauses, vClause, k )
            {
                if ( Vec_IntSize(vClause) == 0 )
                    break;
                RetValue = sat_solver_addclause( p->pSat, Vec_IntArray(vClause), Vec_IntArray(vClause) + Vec_IntSize(vClause) );
                if ( RetValue == 0 )
                    return 0;
            }
        }
        // create XOR clauses for the roots
        Vec_IntForEachEntry( p->vRoots, iNode, i )
        {
            sat_solver_add_xor( p->pSat, Vec_IntEntry(p->vLits, i), Sfm_ObjSatVar(p, iNode), p->nSatVars++, 0 );
            Vec_IntWriteEntry( p->vLits, i, Abc_Var2Lit(p->nSatVars-1, 0) );
        }
        // make OR clause for the last nRoots variables
        RetValue = sat_solver_addclause( p->pSat, Vec_IntArray(p->vLits), Vec_IntArray(p->vLits) + Vec_IntSize(p->vLits) );
        if ( RetValue == 0 )
            return 0;
    }
    // finalize
    RetValue = sat_solver_simplify( p->pSat );
    p->timeCnf += Abc_Clock() - clk;
    return RetValue;
} 

/**Function*************************************************************

  Synopsis    [Takes SAT solver and returns interpolant.]

  Description [If interpolant does not exist, records difference variables.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
word Sfm_ComputeInterpolant( Sfm_Ntk_t * p )
{
    word * pSign, uCube, uTruth = 0;
    int status, i, Div, iVar, nFinal, * pFinal, nIter = 0;
    int pLits[2], nVars = sat_solver_nvars( p->pSat );
    sat_solver_setnvars( p->pSat, nVars + 1 );
    pLits[0] = Abc_Var2Lit( Sfm_ObjSatVar(p, p->iPivotNode), 0 ); // F = 1
    pLits[1] = Abc_Var2Lit( nVars, 0 ); // iNewLit
    while ( 1 ) 
    {
        // find onset minterm
        p->nSatCalls++;
        status = sat_solver_solve( p->pSat, pLits, pLits + 2, p->pPars->nBTLimit, 0, 0, 0 );
        if ( status == l_Undef )
            return SFM_SAT_UNDEC;
        if ( status == l_False )
            return uTruth;
        assert( status == l_True );
        // remember variable values
        Vec_IntClear( p->vValues );
        Vec_IntForEachEntry( p->vDivVars, iVar, i )
            Vec_IntPush( p->vValues, sat_solver_var_value(p->pSat, iVar) );
        // collect divisor literals
        Vec_IntClear( p->vLits );
        Vec_IntPush( p->vLits, Abc_LitNot(pLits[0]) ); // F = 0
        Vec_IntForEachEntry( p->vDivIds, Div, i )
            Vec_IntPush( p->vLits, sat_solver_var_literal(p->pSat, Div) );
        // check against offset
        p->nSatCalls++;
        status = sat_solver_solve( p->pSat, Vec_IntArray(p->vLits), Vec_IntArray(p->vLits) + Vec_IntSize(p->vLits), p->pPars->nBTLimit, 0, 0, 0 );
        if ( status == l_Undef )
            return SFM_SAT_UNDEC;
        if ( status == l_True )
            break;
        assert( status == l_False );
        // compute cube and add clause
        nFinal = sat_solver_final( p->pSat, &pFinal );
        uCube = ~(word)0;
        Vec_IntClear( p->vLits );
        Vec_IntPush( p->vLits, Abc_LitNot(pLits[1]) ); // NOT(iNewLit)
        for ( i = 0; i < nFinal; i++ )
        {
            if ( pFinal[i] == pLits[0] )
                continue;
            Vec_IntPush( p->vLits, pFinal[i] );
            iVar = Vec_IntFind( p->vDivIds, Abc_Lit2Var(pFinal[i]) );   assert( iVar >= 0 );
            uCube &= Abc_LitIsCompl(pFinal[i]) ? s_Truths6[iVar] : ~s_Truths6[iVar];
        }
        uTruth |= uCube;
        status = sat_solver_addclause( p->pSat, Vec_IntArray(p->vLits), Vec_IntArray(p->vLits) + Vec_IntSize(p->vLits) );
        assert( status );
        nIter++;
    }
    assert( status == l_True );
    // store the counter-example
    Vec_IntForEachEntry( p->vDivVars, iVar, i )
        if ( Vec_IntEntry(p->vValues, i) ^ sat_solver_var_value(p->pSat, iVar) ) // insert 1
        {
            pSign = Vec_WrdEntryP( p->vDivCexes, i );
            assert( !Abc_InfoHasBit( (unsigned *)pSign, p->nCexes) );
            Abc_InfoXorBit( (unsigned *)pSign, p->nCexes );
        }
    p->nCexes++;
    return SFM_SAT_SAT;
}

/**Function*************************************************************

  Synopsis    [Checks resubstitution.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sfm_ComputeInterpolantCheck( Sfm_Ntk_t * p )
{
    int iNode = 3;
    int iDiv0 = 6;
    int iDiv1 = 7;
    word uTruth;
//    int i;
//    Sfm_NtkForEachNode( p, i )
    {
        Sfm_NtkCreateWindow( p, iNode, 1 );
        Sfm_NtkWindowToSolver( p );

        // collect SAT variables of divisors
        Vec_IntClear( p->vDivIds );
        Vec_IntPush( p->vDivIds, Sfm_ObjSatVar(p, iDiv0) );
        Vec_IntPush( p->vDivIds, Sfm_ObjSatVar(p, iDiv1) );

        uTruth = Sfm_ComputeInterpolant( p );

        if ( uTruth == SFM_SAT_SAT )
            printf( "The problem is SAT.\n" );
        else if ( uTruth == SFM_SAT_UNDEC )
            printf( "The problem is UNDEC.\n" );
        else
            Kit_DsdPrintFromTruth( (unsigned *)&uTruth, 2 ); printf( "\n" );
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

