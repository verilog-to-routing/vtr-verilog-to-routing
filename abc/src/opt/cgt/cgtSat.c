/**CFile****************************************************************

  FileName    [cgtSat.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Clock gating package.]

  Synopsis    [Checking implications using SAT.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cgtSat.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cgtInt.h"

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
int Cgt_CheckImplication( Cgt_Man_t * p, Aig_Obj_t * pGate, Aig_Obj_t * pMiter )
{
    int nBTLimit = p->pPars->nConfMax;
    int pLits[2], RetValue;
    abctime clk;
    p->nCalls++;

    // sanity checks
    assert( p->pSat && p->pCnf );
    assert( !Aig_IsComplement(pMiter) );
    assert( Aig_Regular(pGate) != pMiter );

    // solve under assumptions
    // G => !M -- true     G & M -- false
    pLits[0] = toLitCond( p->pCnf->pVarNums[Aig_Regular(pGate)->Id], Aig_IsComplement(pGate) );
    pLits[1] = toLitCond( p->pCnf->pVarNums[pMiter->Id], 0 );

clk = Abc_Clock();
    RetValue = sat_solver_solve( p->pSat, pLits, pLits + 2, (ABC_INT64_T)nBTLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
p->timeSat += Abc_Clock() - clk;
    if ( RetValue == l_False )
    {
p->timeSatUnsat += Abc_Clock() - clk;
        pLits[0] = lit_neg( pLits[0] );
        pLits[1] = lit_neg( pLits[1] );
        RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 2 );
        assert( RetValue );
        sat_solver_compress( p->pSat );
        p->nCallsUnsat++;
        return 1;
    }
    else if ( RetValue == l_True )
    {
p->timeSatSat += Abc_Clock() - clk;
        p->nCallsSat++;
        return 0;
    }
    else // if ( RetValue1 == l_Undef )
    {
p->timeSatUndec += Abc_Clock() - clk;
        p->nCallsUndec++;
        return -1;
    }
    return -2;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

