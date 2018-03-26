/**CFile****************************************************************

  FileName    [saigLoc.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Sequential AIG package.]

  Synopsis    [K-step induction for one property only.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: saigLoc.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "saig.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satSolver.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns 1 if two state are equal.]

  Description [Array vState contains indexes of CNF variables for each
  flop in the first N time frames (0 < i < k, i < N, k < N).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManStatesAreEqual( sat_solver * pSat, Vec_Int_t * vState, int nRegs, int i, int k )
{
    int * pStateI = (int *)Vec_IntArray(vState) + nRegs * i;
    int * pStateK = (int *)Vec_IntArray(vState) + nRegs * k;
    int v;
    assert( i && k && i < k );
    assert( nRegs * k <= Vec_IntSize(vState) );
    // check if the states are available
    for ( v = 0; v < nRegs; v++ )
        if ( pStateI[v] >= 0 && pStateK[v] == -1 )
            return 0;
/*
    printf( "\nchecking uniqueness\n" );
    printf( "%3d : ", i );
    for ( v = 0; v < nRegs; v++ )
        printf( "%d", sat_solver_var_value(pSat, pStateI[v]) );
    printf( "\n" );

    printf( "%3d : ", k );
    for ( v = 0; v < nRegs; v++ )
        printf( "%d", sat_solver_var_value(pSat, pStateK[v]) );
    printf( "\n" );
*/
    for ( v = 0; v < nRegs; v++ )
        if ( pStateI[v] >= 0 )
        {
            if ( sat_solver_var_value(pSat, pStateI[v]) != sat_solver_var_value(pSat, pStateK[v]) )
                return 0;
        }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Add uniqueness constraint.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManAddUniqueness( sat_solver * pSat, Vec_Int_t * vState, int nRegs, int i, int k, int * pnSatVarNum, int * pnClauses, int fVerbose )
{
    int * pStateI = (int *)Vec_IntArray(vState) + nRegs * i;
    int * pStateK = (int *)Vec_IntArray(vState) + nRegs * k;
    int v, iVars, nSatVarsOld, RetValue, * pClause;
    assert( i && k && i < k );
    assert( nRegs * k <= Vec_IntSize(vState) );
    // check if the states are available
    for ( v = 0; v < nRegs; v++ )
        if ( pStateI[v] >= 0 && pStateK[v] == -1 )
        {
            if ( fVerbose )
                printf( "Cannot constrain an incomplete state.\n" );
            return 0;
        }
    // add XORs
    nSatVarsOld = *pnSatVarNum;
    for ( v = 0; v < nRegs; v++ )
        if ( pStateI[v] >= 0 )
        {
            *pnClauses += 4;
            RetValue = Cnf_DataAddXorClause( pSat, pStateI[v], pStateK[v], (*pnSatVarNum)++ );
            if ( RetValue == 0 )
            {
                if ( fVerbose )
                    printf( "SAT solver became UNSAT after adding a uniqueness constraint.\n" );
                return 1;
            }
        }
    // add OR clause
    (*pnClauses)++;
    iVars = 0;
    pClause = ABC_ALLOC( int, nRegs );
    for ( v = nSatVarsOld; v < *pnSatVarNum; v++ )
        pClause[iVars++] = toLitCond( v, 0 );
    assert( iVars <= nRegs );
    RetValue = sat_solver_addclause( pSat, pClause, pClause + iVars );
    ABC_FREE( pClause );
    if ( RetValue == 0 )
    {
        if ( fVerbose )
            printf( "SAT solver became UNSAT after adding a uniqueness constraint.\n" );
        return 1;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Performs induction by unrolling timeframes backward.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManInduction( Aig_Man_t * p, int nTimeOut, int nFramesMax, int nConfMax, int fUnique, int fUniqueAll, int fGetCex, int fVerbose, int fVeryVerbose )
{
    sat_solver * pSat;
    Aig_Man_t * pAigPart = NULL;
    Cnf_Dat_t * pCnfPart = NULL;
    Vec_Int_t * vTopVarNums, * vState, * vTopVarIds = NULL;
    Vec_Ptr_t * vTop, * vBot;
    Aig_Obj_t * pObjPi, * pObjPiCopy, * pObjPo;
    int i, k, f, Lits[2], status = -1, RetValue, nSatVarNum, nConfPrev;
    int nOldSize, iReg, iLast, fAdded, nConstrs = 0, nClauses = 0;
    abctime clk, nTimeToStop = nTimeOut ? nTimeOut * CLOCKS_PER_SEC + Abc_Clock() : 0;
    assert( fUnique == 0 || fUniqueAll == 0 );
    assert( Saig_ManPoNum(p) == 1 );
    Aig_ManSetCioIds( p );

    // start the top by including the PO
    vBot = Vec_PtrAlloc( 100 );
    vTop = Vec_PtrAlloc( 100 );
    vState = Vec_IntAlloc( 1000 );
    Vec_PtrPush( vTop, Aig_ManCo(p, 0) );
    // start the array of CNF variables
    vTopVarNums = Vec_IntAlloc( 100 );
    // start the solver
    pSat = sat_solver_new();
    sat_solver_setnvars( pSat, 1000 );

    // set runtime limit
    if ( nTimeToStop )
        sat_solver_set_runtime_limit( pSat, nTimeToStop );

    // iterate backward unrolling
    RetValue = -1;
    nSatVarNum = 0;
    if ( fVerbose )
        printf( "Induction parameters: FramesMax = %5d. ConflictMax = %6d.\n", nFramesMax, nConfMax );
    for ( f = 0; ; f++ )
    { 
        if ( f > 0 )
        {
            Aig_ManStop( pAigPart );
            Cnf_DataFree( pCnfPart );
        }
        clk = Abc_Clock();
        // get the bottom
        Aig_SupportNodes( p, (Aig_Obj_t **)Vec_PtrArray(vTop), Vec_PtrSize(vTop), vBot );
        // derive AIG for the part between top and bottom
        pAigPart = Aig_ManDupSimpleDfsPart( p, vBot, vTop );
        // convert it into CNF
        pCnfPart = Cnf_Derive( pAigPart, Aig_ManCoNum(pAigPart) );
        Cnf_DataLift( pCnfPart, nSatVarNum );
        nSatVarNum += pCnfPart->nVars;
        nClauses   += pCnfPart->nClauses;

        // remember top frame var IDs
        if ( fGetCex && vTopVarIds == NULL )
        {
            vTopVarIds = Vec_IntStartFull( Aig_ManCiNum(p) );
            Aig_ManForEachCi( p, pObjPi, i )
            {
                if ( pObjPi->pData == NULL )
                    continue;
                pObjPiCopy = (Aig_Obj_t *)pObjPi->pData;
                assert( Aig_ObjIsCi(pObjPiCopy) );
                if ( Saig_ObjIsPi(p, pObjPi) )
                    Vec_IntWriteEntry( vTopVarIds, Aig_ObjCioId(pObjPi) + Saig_ManRegNum(p), pCnfPart->pVarNums[Aig_ObjId(pObjPiCopy)] );
                else if ( Saig_ObjIsLo(p, pObjPi) )
                    Vec_IntWriteEntry( vTopVarIds, Aig_ObjCioId(pObjPi) - Saig_ManPiNum(p), pCnfPart->pVarNums[Aig_ObjId(pObjPiCopy)] );
                else assert( 0 );
            }
        }

        // stitch variables of top and bot
        assert( Aig_ManCoNum(pAigPart)-1 == Vec_IntSize(vTopVarNums) );
        Aig_ManForEachCo( pAigPart, pObjPo, i )
        {
            if ( i == 0 )
            {
                // do not perform inductive strengthening
//                if ( f > 0 )
//                    continue;
                // add topmost literal
                Lits[0] = toLitCond( pCnfPart->pVarNums[pObjPo->Id], f>0 );
                if ( !sat_solver_addclause( pSat, Lits, Lits+1 ) )
                    assert( 0 );
                nClauses++;
                continue;
            }
            Lits[0] = toLitCond( Vec_IntEntry(vTopVarNums, i-1), 0 );
            Lits[1] = toLitCond( pCnfPart->pVarNums[pObjPo->Id], 1 );
            if ( !sat_solver_addclause( pSat, Lits, Lits+2 ) )
                assert( 0 );
            Lits[0] = toLitCond( Vec_IntEntry(vTopVarNums, i-1), 1 );
            Lits[1] = toLitCond( pCnfPart->pVarNums[pObjPo->Id], 0 );
            if ( !sat_solver_addclause( pSat, Lits, Lits+2 ) )
                assert( 0 );
            nClauses += 2;
        }
        // add CNF to the SAT solver
        for ( i = 0; i < pCnfPart->nClauses; i++ )
            if ( !sat_solver_addclause( pSat, pCnfPart->pClauses[i], pCnfPart->pClauses[i+1] ) )
                break;
        if ( i < pCnfPart->nClauses )
        {
//            printf( "SAT solver became UNSAT after adding clauses.\n" );
            RetValue = 1;
            break;
        }

        // create new set of POs to derive new top
        Vec_PtrClear( vTop );
        Vec_PtrPush( vTop, Aig_ManCo(p, 0) );
        Vec_IntClear( vTopVarNums );
        nOldSize = Vec_IntSize(vState);
        Vec_IntFillExtra( vState, nOldSize + Aig_ManRegNum(p), -1 );
        Vec_PtrForEachEntry( Aig_Obj_t *, vBot, pObjPi, i )
        {
            assert( Aig_ObjIsCi(pObjPi) );
            if ( Saig_ObjIsLo(p, pObjPi) )
            {
                pObjPiCopy = (Aig_Obj_t *)pObjPi->pData;
                assert( pObjPiCopy != NULL );
                Vec_PtrPush( vTop, Saig_ObjLoToLi(p, pObjPi) );
                Vec_IntPush( vTopVarNums, pCnfPart->pVarNums[pObjPiCopy->Id] );

                iReg = pObjPi->CioId - Saig_ManPiNum(p);
                assert( iReg >= 0 && iReg < Aig_ManRegNum(p) );
                Vec_IntWriteEntry( vState, nOldSize+iReg, pCnfPart->pVarNums[pObjPiCopy->Id] );
            }
        } 
        assert( Vec_IntSize(vState)%Aig_ManRegNum(p) == 0 );
        iLast = Vec_IntSize(vState)/Aig_ManRegNum(p);
        if ( fUniqueAll )
        {
            for ( i = 1; i < iLast-1; i++ )
            {
                nConstrs++;
                if ( fVeryVerbose )
                    printf( "Adding constaint for state %2d and state %2d.\n", i, iLast-1 );
                if ( Saig_ManAddUniqueness( pSat, vState, Aig_ManRegNum(p), i, iLast-1, &nSatVarNum, &nClauses, fVerbose ) )
                    break;
            }
            if ( i < iLast-1 )
            {
                RetValue = 1;
                break;
            }
        }

nextrun:
        fAdded = 0;
        // run the SAT solver
        nConfPrev = pSat->stats.conflicts;
        status = sat_solver_solve( pSat, NULL, NULL, (ABC_INT64_T)nConfMax, 0, 0, 0 );
        if ( fVerbose )
        {
            printf( "Frame %4d : PI =%5d. PO =%5d. AIG =%5d. Var =%7d. Clau =%7d. Conf =%7d. ",
                f, Aig_ManCiNum(pAigPart), Aig_ManCoNum(pAigPart), Aig_ManNodeNum(pAigPart), 
                nSatVarNum, nClauses, (int)pSat->stats.conflicts-nConfPrev );
            ABC_PRT( "Time", Abc_Clock() - clk );
        }
        if ( status == l_Undef )
            break;
        if ( status == l_False )
        {
            RetValue = 1;
            break;
        }
        assert( status == l_True );
        // the problem is SAT - add more clauses
        if ( fVeryVerbose )
        {
            Vec_IntForEachEntry( vState, iReg, i )
            {
                if ( i && (i % Aig_ManRegNum(p)) == 0 )
                    printf( "\n" );
                if ( (i % Aig_ManRegNum(p)) == 0 )
                    printf( "       State %3d : ", i/Aig_ManRegNum(p) );
                printf( "%c", (iReg >= 0) ? ('0' + sat_solver_var_value(pSat, iReg)) : 'x' );
            }
            printf( "\n" );
        }
        if ( nFramesMax && f == nFramesMax - 1 )
        {
            // derive counter-example
            assert( status == l_True );
            if ( fGetCex )
            {
                int VarNum, iBit = 0;
                Abc_Cex_t * pCex = Abc_CexAlloc( Aig_ManRegNum(p)-1, Saig_ManPiNum(p), 1 );
                pCex->iFrame = 0;
                pCex->iPo = 0;
                Vec_IntForEachEntryStart( vTopVarIds, VarNum, i, 1 )
                {
                    if ( VarNum >= 0 && sat_solver_var_value( pSat, VarNum ) )
                        Abc_InfoSetBit( pCex->pData, iBit );
                    iBit++;
                }
                assert( iBit == pCex->nBits );
                Abc_CexFree( p->pSeqModel );
                p->pSeqModel = pCex;
            }
            break;
        }
        if ( fUnique )
        {
            for ( i = 1; i < iLast; i++ )
            {
                for ( k = i+1; k < iLast; k++ )
                {
                    if ( !Saig_ManStatesAreEqual( pSat, vState, Aig_ManRegNum(p), i, k ) )
                        continue;
                    nConstrs++;
                    fAdded = 1;
                    if ( fVeryVerbose )
                        printf( "Adding constaint for state %2d and state %2d.\n", i, k );
                    if ( Saig_ManAddUniqueness( pSat, vState, Aig_ManRegNum(p), i, k, &nSatVarNum, &nClauses, fVerbose ) )
                        break;
                }
                if ( k < iLast )
                    break;
            }
            if ( i < iLast )
            {
                RetValue = 1;
                break;
            }
        }
        if ( fAdded )
            goto nextrun;
    }
    if ( fVerbose )
    {
        if ( nTimeToStop && Abc_Clock() >= nTimeToStop )
            printf( "Timeout (%d sec) was reached during iteration %d.\n", nTimeOut, f+1 );
        else if ( status == l_Undef )
            printf( "Conflict limit (%d) was reached during iteration %d.\n", nConfMax, f+1 );
        else if ( fUnique || fUniqueAll )
            printf( "Completed %d interations and added %d uniqueness constraints.\n", f+1, nConstrs );
        else
            printf( "Completed %d interations.\n", f+1 );
    }
    // cleanup
    sat_solver_delete( pSat );
    Aig_ManStop( pAigPart );
    Cnf_DataFree( pCnfPart );
    Vec_IntFree( vTopVarNums );
    Vec_PtrFree( vTop );
    Vec_PtrFree( vBot );
    Vec_IntFree( vState );
    Vec_IntFreeP( &vTopVarIds );
    return RetValue;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

