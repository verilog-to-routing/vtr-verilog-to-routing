/**CFile****************************************************************

  FileName    [sswSat.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Inductive prover with constraints.]

  Synopsis    [Calls to the SAT solver.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 1, 2008.]

  Revision    [$Id: sswSat.c,v 1.00 2008/09/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sswInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Runs equivalence test for the two nodes.]

  Description [Both nodes should be regular and different from each other.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_NodesAreEquiv( Ssw_Man_t * p, Aig_Obj_t * pOld, Aig_Obj_t * pNew )
{
    int nBTLimit = p->pPars->nBTLimit;
    int pLits[3], nLits, RetValue, RetValue1;
    abctime clk;//, status;
    p->nSatCalls++;
    p->pMSat->nSolverCalls++;

    // sanity checks
    assert( !Aig_IsComplement(pOld) );
    assert( !Aig_IsComplement(pNew) );
    assert( pOld != pNew );
    assert( p->pMSat != NULL );

    // if the nodes do not have SAT variables, allocate them
    Ssw_CnfNodeAddToSolver( p->pMSat, pOld );
    Ssw_CnfNodeAddToSolver( p->pMSat, pNew );

    // solve under assumptions
    // A = 1; B = 0     OR     A = 1; B = 1 
    nLits = 2;
    pLits[0] = toLitCond( Ssw_ObjSatNum(p->pMSat,pOld), 0 );
    pLits[1] = toLitCond( Ssw_ObjSatNum(p->pMSat,pNew), pOld->fPhase == pNew->fPhase );
    if ( p->iOutputLit > -1 )
        pLits[nLits++] = p->iOutputLit;
    if ( p->pPars->fPolarFlip )
    {
        if ( pOld->fPhase )  pLits[0] = lit_neg( pLits[0] );
        if ( pNew->fPhase )  pLits[1] = lit_neg( pLits[1] );
    }
//Sat_SolverWriteDimacs( p->pSat, "temp.cnf", pLits, pLits + 2, 1 );

    if ( p->pMSat->pSat->qtail != p->pMSat->pSat->qhead )
    {
        RetValue = sat_solver_simplify(p->pMSat->pSat);
        //assert( RetValue != 0 );
    }

clk = Abc_Clock();
    RetValue1 = sat_solver_solve( p->pMSat->pSat, pLits, pLits + nLits, 
        (ABC_INT64_T)nBTLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
p->timeSat += Abc_Clock() - clk;
    if ( RetValue1 == l_False )
    {
p->timeSatUnsat += Abc_Clock() - clk;
        if ( nLits == 2 )
        {
            pLits[0] = lit_neg( pLits[0] );
            pLits[1] = lit_neg( pLits[1] );
            RetValue = sat_solver_addclause( p->pMSat->pSat, pLits, pLits + 2 );
            assert( RetValue );
/*
            if ( p->pMSat->pSat->qtail != p->pMSat->pSat->qhead )
            {
                RetValue = sat_solver_simplify(p->pMSat->pSat);
                assert( RetValue != 0 );
            }
*/
        }
        p->nSatCallsUnsat++;
    }
    else if ( RetValue1 == l_True )
    {
p->timeSatSat += Abc_Clock() - clk;
        p->nSatCallsSat++;
        return 0;
    }
    else // if ( RetValue1 == l_Undef )
    {
p->timeSatUndec += Abc_Clock() - clk;
        p->nSatFailsReal++;
        return -1;
    }

    // if the old node was constant 0, we already know the answer
    if ( pOld == Aig_ManConst1(p->pFrames) )
    {
        p->nSatProof++;
        return 1;
    }

    // solve under assumptions
    // A = 0; B = 1     OR     A = 0; B = 0 
    nLits = 2;
    pLits[0] = toLitCond( Ssw_ObjSatNum(p->pMSat,pOld), 1 );
    pLits[1] = toLitCond( Ssw_ObjSatNum(p->pMSat,pNew), pOld->fPhase ^ pNew->fPhase );
    if ( p->iOutputLit > -1 )
        pLits[nLits++] = p->iOutputLit;
    if ( p->pPars->fPolarFlip )
    {
        if ( pOld->fPhase )  pLits[0] = lit_neg( pLits[0] );
        if ( pNew->fPhase )  pLits[1] = lit_neg( pLits[1] );
    }

    if ( p->pMSat->pSat->qtail != p->pMSat->pSat->qhead )
    {
        RetValue = sat_solver_simplify(p->pMSat->pSat);
        //assert( RetValue != 0 );
    }

clk = Abc_Clock();
    RetValue1 = sat_solver_solve( p->pMSat->pSat, pLits, pLits + nLits, 
        (ABC_INT64_T)nBTLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
p->timeSat += Abc_Clock() - clk;
    if ( RetValue1 == l_False )
    {
p->timeSatUnsat += Abc_Clock() - clk;
        if ( nLits == 2 )
        {
            pLits[0] = lit_neg( pLits[0] );
            pLits[1] = lit_neg( pLits[1] );
            RetValue = sat_solver_addclause( p->pMSat->pSat, pLits, pLits + 2 );
            assert( RetValue );
/*
            if ( p->pMSat->pSat->qtail != p->pMSat->pSat->qhead )
            {
                RetValue = sat_solver_simplify(p->pMSat->pSat);
                assert( RetValue != 0 );
            }
*/
        }
        p->nSatCallsUnsat++;
    }
    else if ( RetValue1 == l_True )
    {
p->timeSatSat += Abc_Clock() - clk;
        p->nSatCallsSat++;
        return 0;
    }
    else // if ( RetValue1 == l_Undef )
    {
p->timeSatUndec += Abc_Clock() - clk;
        p->nSatFailsReal++;
        return -1;
    }
    // return SAT proof
    p->nSatProof++;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Constrains two nodes to be equivalent in the SAT solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_NodesAreConstrained( Ssw_Man_t * p, Aig_Obj_t * pOld, Aig_Obj_t * pNew )
{
    int pLits[2], RetValue, fComplNew;
    Aig_Obj_t * pTemp;

    // sanity checks
    assert( Aig_Regular(pOld) != Aig_Regular(pNew) );
    assert( p->pPars->fConstrs || Aig_ObjPhaseReal(pOld) == Aig_ObjPhaseReal(pNew) );

    // move constant to the old node
    if ( Aig_Regular(pNew) == Aig_ManConst1(p->pFrames) )
    {
        assert( Aig_Regular(pOld) != Aig_ManConst1(p->pFrames) );
        pTemp = pOld;
        pOld  = pNew;
        pNew  = pTemp;
    }

    // move complement to the new node
    if ( Aig_IsComplement(pOld) )
    {
        pOld = Aig_Regular(pOld);
        pNew = Aig_Not(pNew);
    }
    assert( p->pMSat != NULL );

    // if the nodes do not have SAT variables, allocate them
    Ssw_CnfNodeAddToSolver( p->pMSat, pOld );
    Ssw_CnfNodeAddToSolver( p->pMSat, Aig_Regular(pNew) );

    // transform the new node
    fComplNew = Aig_IsComplement( pNew );
    pNew = Aig_Regular( pNew );

    // consider the constant 1 case
    if ( pOld == Aig_ManConst1(p->pFrames) )
    {
        // add constraint A = 1  ---->  A
        pLits[0] = toLitCond( Ssw_ObjSatNum(p->pMSat,pNew), fComplNew );
        if ( p->pPars->fPolarFlip )
        {
            if ( pNew->fPhase )  pLits[0] = lit_neg( pLits[0] );
        }
        RetValue = sat_solver_addclause( p->pMSat->pSat, pLits, pLits + 1 );
        assert( RetValue );
    }
    else
    {
        // add constraint A = B  ---->  (A v !B)(!A v B)

        // (A v !B)
        pLits[0] = toLitCond( Ssw_ObjSatNum(p->pMSat,pOld), 0 );
        pLits[1] = toLitCond( Ssw_ObjSatNum(p->pMSat,pNew), !fComplNew );
        if ( p->pPars->fPolarFlip )
        {
            if ( pOld->fPhase )  pLits[0] = lit_neg( pLits[0] );
            if ( pNew->fPhase )  pLits[1] = lit_neg( pLits[1] );
        }
        pLits[0] = lit_neg( pLits[0] );
        pLits[1] = lit_neg( pLits[1] );
        RetValue = sat_solver_addclause( p->pMSat->pSat, pLits, pLits + 2 );
        assert( RetValue );

        // (!A v B)
        pLits[0] = toLitCond( Ssw_ObjSatNum(p->pMSat,pOld), 1 );
        pLits[1] = toLitCond( Ssw_ObjSatNum(p->pMSat,pNew), fComplNew);
        if ( p->pPars->fPolarFlip )
        {
            if ( pOld->fPhase )  pLits[0] = lit_neg( pLits[0] );
            if ( pNew->fPhase )  pLits[1] = lit_neg( pLits[1] );
        }
        pLits[0] = lit_neg( pLits[0] );
        pLits[1] = lit_neg( pLits[1] );
        RetValue = sat_solver_addclause( p->pMSat->pSat, pLits, pLits + 2 );
        assert( RetValue );
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Constrains one node in the SAT solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_NodeIsConstrained( Ssw_Man_t * p, Aig_Obj_t * pPoObj )
{
    int RetValue, Lit;
    Ssw_CnfNodeAddToSolver( p->pMSat, Aig_ObjFanin0(pPoObj) );
    // add constraint A = 1  ---->  A
    Lit = toLitCond( Ssw_ObjSatNum(p->pMSat,Aig_ObjFanin0(pPoObj)), !Aig_ObjFaninC0(pPoObj) );
    if ( p->pPars->fPolarFlip )
    {
        if ( Aig_ObjFanin0(pPoObj)->fPhase )  Lit = lit_neg( Lit );
    }
    RetValue = sat_solver_addclause( p->pMSat->pSat, &Lit, &Lit + 1 );
    assert( RetValue );
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

