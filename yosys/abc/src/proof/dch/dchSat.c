/**CFile****************************************************************

  FileName    [dchSat.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Choice computation for tech-mapping.]

  Synopsis    [Calls to the SAT solver.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 29, 2008.]

  Revision    [$Id: dchSat.c,v 1.00 2008/07/29 00:00:00 alanmi Exp $]

***********************************************************************/

#include "dchInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Runs equivalence test for the two nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dch_NodesAreEquiv( Dch_Man_t * p, Aig_Obj_t * pOld, Aig_Obj_t * pNew )
{
    int nBTLimit = p->pPars->nBTLimit;
    int pLits[2], RetValue, RetValue1, status;
    abctime clk;
    p->nSatCalls++;

    // sanity checks
    assert( !Aig_IsComplement(pNew) );
    assert( !Aig_IsComplement(pOld) );
    assert( pNew != pOld );

    p->nCallsSince++;  // experiment with this!!!
    
    // check if SAT solver needs recycling
    if ( p->pSat == NULL || 
        (p->pPars->nSatVarMax && 
         p->nSatVars > p->pPars->nSatVarMax && 
         p->nCallsSince > p->pPars->nCallsRecycle) )
        Dch_ManSatSolverRecycle( p );

    // if the nodes do not have SAT variables, allocate them
    Dch_CnfNodeAddToSolver( p, pOld );
    Dch_CnfNodeAddToSolver( p, pNew );

    // propage unit clauses
    if ( p->pSat->qtail != p->pSat->qhead )
    {
        status = sat_solver_simplify(p->pSat);
        assert( status != 0 );
        assert( p->pSat->qtail == p->pSat->qhead );
    }

    // solve under assumptions
    // A = 1; B = 0     OR     A = 1; B = 1 
    pLits[0] = toLitCond( Dch_ObjSatNum(p,pOld), 0 );
    pLits[1] = toLitCond( Dch_ObjSatNum(p,pNew), pOld->fPhase == pNew->fPhase );
    if ( p->pPars->fPolarFlip )
    {
        if ( pOld->fPhase )  pLits[0] = lit_neg( pLits[0] );
        if ( pNew->fPhase )  pLits[1] = lit_neg( pLits[1] );
    }
//Sat_SolverWriteDimacs( p->pSat, "temp.cnf", pLits, pLits + 2, 1 );
clk = Abc_Clock();
    RetValue1 = sat_solver_solve( p->pSat, pLits, pLits + 2, 
        (ABC_INT64_T)nBTLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
p->timeSat += Abc_Clock() - clk;
    if ( RetValue1 == l_False )
    {
p->timeSatUnsat += Abc_Clock() - clk;
        pLits[0] = lit_neg( pLits[0] );
        pLits[1] = lit_neg( pLits[1] );
        RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 2 );
        assert( RetValue );
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
    if ( pOld == Aig_ManConst1(p->pAigFraig) )
    {
        p->nSatProof++;
        return 1;
    }

    // solve under assumptions
    // A = 0; B = 1     OR     A = 0; B = 0 
    pLits[0] = toLitCond( Dch_ObjSatNum(p,pOld), 1 );
    pLits[1] = toLitCond( Dch_ObjSatNum(p,pNew), pOld->fPhase ^ pNew->fPhase );
    if ( p->pPars->fPolarFlip )
    {
        if ( pOld->fPhase )  pLits[0] = lit_neg( pLits[0] );
        if ( pNew->fPhase )  pLits[1] = lit_neg( pLits[1] );
    }
clk = Abc_Clock();
    RetValue1 = sat_solver_solve( p->pSat, pLits, pLits + 2, 
        (ABC_INT64_T)nBTLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
p->timeSat += Abc_Clock() - clk;
    if ( RetValue1 == l_False )
    {
p->timeSatUnsat += Abc_Clock() - clk;
        pLits[0] = lit_neg( pLits[0] );
        pLits[1] = lit_neg( pLits[1] );
        RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 2 );
        assert( RetValue );
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


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

