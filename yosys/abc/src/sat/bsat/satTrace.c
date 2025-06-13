/**CFile****************************************************************

  FileName    [satTrace.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT sat_solver.]

  Synopsis    [Records the trace of SAT solving in the CNF form.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: satTrace.c,v 1.4 2005/09/16 22:55:03 casem Exp $]

***********************************************************************/

#include "satSolver.h"

ABC_NAMESPACE_IMPL_START


/*
    The trace of SAT solving contains the original clause of the problem
    along with the learned clauses derived during SAT solving.
    The first line of the resulting file contains 3 numbers instead of 2:
    c <num_vars> <num_all_clauses> <num_root_clauses>
*/

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Start the trace recording.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverTraceStart( sat_solver * pSat, char * pName )
{
    assert( pSat->pFile == NULL );
    pSat->pFile = fopen( pName, "w" );
    fprintf( pSat->pFile, "                                        \n" );
    pSat->nClauses = 0;
    pSat->nRoots = 0;
}   

/**Function*************************************************************

  Synopsis    [Stops the trace recording.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverTraceStop( sat_solver * pSat )
{
    if ( pSat->pFile == NULL )
        return;
    rewind( pSat->pFile );
    fprintf( pSat->pFile, "p %d %d %d", sat_solver_nvars(pSat), pSat->nClauses, pSat->nRoots );
    fclose( pSat->pFile );
    pSat->pFile = NULL;
}


/**Function*************************************************************

  Synopsis    [Writes one clause into the trace file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverTraceWrite( sat_solver * pSat, int * pBeg, int * pEnd, int fRoot )
{
    if ( pSat->pFile == NULL )
        return;
    pSat->nClauses++;
    pSat->nRoots += fRoot;
    for ( ; pBeg < pEnd ; pBeg++ )
        fprintf( pSat->pFile, " %d", lit_print(*pBeg) );
    fprintf( pSat->pFile, " 0\n" );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

