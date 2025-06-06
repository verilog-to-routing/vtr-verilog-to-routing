/**CFile****************************************************************

  FileName    [giaSatoko.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Interface to Satoko solver.]

  Author      [Alan Mishchenko, Bruno Schmitt]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaSatoko.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satSolver3.h"

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
sat_solver3 * Gia_ManSat3Init( Cnf_Dat_t * pCnf )
{
    sat_solver3 * pSat = sat_solver3_new();
    int i;
    //sat_solver_setnvars( pSat, p->nVars );
    for ( i = 0; i < pCnf->nClauses; i++ )
    {
        if ( !sat_solver3_addclause( pSat, pCnf->pClauses[i], pCnf->pClauses[i+1] ) )
        {
            sat_solver3_delete( pSat );
            return NULL;
        }
    }
    return pSat;
}
void Gia_ManSat3Report( int iOutput, int status, abctime clk )
{
    if ( iOutput >= 0 )
        Abc_Print( 1, "Output %6d : ", iOutput );
    else
        Abc_Print( 1, "Total: " );

    if ( status == l_Undef )
        Abc_Print( 1, "UNDECIDED      " );
    else if ( status == l_True )
        Abc_Print( 1, "SATISFIABLE    " );
    else
        Abc_Print( 1, "UNSATISFIABLE  " );

    Abc_PrintTime( 1, "Time", clk );
}
sat_solver3 * Gia_ManSat3Create( Gia_Man_t * p )
{
    Cnf_Dat_t * pCnf = (Cnf_Dat_t *)Mf_ManGenerateCnf( p, 8, 0, 1, 0, 0 );
    sat_solver3 * pSat = Gia_ManSat3Init( pCnf );
    int status = pSat ? sat_solver3_simplify(pSat) : 0;
    Cnf_DataFree( pCnf );
    if ( status )
        return pSat;
    if ( pSat )
        sat_solver3_delete( pSat );
    return NULL;
}
int Gia_ManSat3CallOne( Gia_Man_t * p, int iOutput )
{
    abctime clk = Abc_Clock();
    sat_solver3 * pSat;
    int status, Cost = 0;

    pSat = Gia_ManSat3Create( p );
    if ( pSat )
    {
        status = sat_solver3_solve( pSat, NULL, NULL, 0, 0, 0, 0 );
        Cost = (unsigned)pSat->stats.conflicts;
        sat_solver3_delete( pSat );
    }
    else 
        status = l_False;

    Gia_ManSat3Report( iOutput, status, Abc_Clock() - clk );
    return Cost;
}
void Gia_ManSat3Call( Gia_Man_t * p, int fSplit )
{
    Gia_Man_t * pOne;
    Gia_Obj_t * pRoot;
    int i;
    if ( fSplit )
    {
        abctime clk = Abc_Clock();
        Gia_ManForEachCo( p, pRoot, i )
        {
            pOne = Gia_ManDupDfsCone( p, pRoot );
            Gia_ManSat3CallOne( pOne, i );
            Gia_ManStop( pOne );
        }
        Abc_PrintTime( 1, "Total time", Abc_Clock() - clk );
        return;
    }
    Gia_ManSat3CallOne( p, -1 );
}    


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

