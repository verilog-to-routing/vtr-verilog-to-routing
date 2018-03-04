/**CFile****************************************************************

  FileName    [msatActivity.c]

  PackageName [A C version of SAT solver MINISAT, originally developed 
  in C++ by Niklas Een and Niklas Sorensson, Chalmers University of 
  Technology, Sweden: http://www.cs.chalmers.se/~een/Satzoo.]

  Synopsis    [Procedures controlling activity of variables and clauses.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: msatActivity.c,v 1.0 2004/01/01 1:00:00 alanmi Exp $]

***********************************************************************/

#include "msatInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_SolverVarBumpActivity( Msat_Solver_t * p, Msat_Lit_t Lit )
{
    Msat_Var_t Var;
    if ( p->dVarDecay < 0 ) // (negative decay means static variable order -- don't bump)
        return;
    Var = MSAT_LIT2VAR(Lit);
    p->pdActivity[Var] += p->dVarInc;
//    p->pdActivity[Var] += p->dVarInc * p->pFactors[Var];
    if ( p->pdActivity[Var] > 1e100 )
        Msat_SolverVarRescaleActivity( p );
    Msat_OrderUpdate( p->pOrder, Var );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_SolverVarDecayActivity( Msat_Solver_t * p )
{
    if ( p->dVarDecay >= 0 )
        p->dVarInc *= p->dVarDecay;
}

/**Function*************************************************************

  Synopsis    [Divide all variable activities by 1e100.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_SolverVarRescaleActivity( Msat_Solver_t * p )
{
    int i;
    for ( i = 0; i < p->nVars; i++ )
        p->pdActivity[i] *= 1e-100;
    p->dVarInc *= 1e-100;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_SolverClaBumpActivity( Msat_Solver_t * p, Msat_Clause_t * pC )
{
    float Activ;
    Activ = Msat_ClauseReadActivity(pC);
    if ( Activ + p->dClaInc > 1e20 )
    {
        Msat_SolverClaRescaleActivity( p );
        Activ = Msat_ClauseReadActivity( pC );
    }
    Msat_ClauseWriteActivity( pC, Activ + (float)p->dClaInc );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_SolverClaDecayActivity( Msat_Solver_t * p )
{
    p->dClaInc *= p->dClaDecay;
}

/**Function*************************************************************

  Synopsis    [Divide all constraint activities by 1e20.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_SolverClaRescaleActivity( Msat_Solver_t * p )
{
    Msat_Clause_t ** pLearned;
    int nLearned, i;
    float Activ;
    nLearned = Msat_ClauseVecReadSize( p->vLearned );
    pLearned = Msat_ClauseVecReadArray( p->vLearned );
    for ( i = 0; i < nLearned; i++ )
    {
        Activ = Msat_ClauseReadActivity( pLearned[i] );
        Msat_ClauseWriteActivity( pLearned[i], Activ * (float)1e-20 );
    }
    p->dClaInc *= 1e-20;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

