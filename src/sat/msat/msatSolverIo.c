/**CFile****************************************************************

  FileName    [msatSolverIo.c]

  PackageName [A C version of SAT solver MINISAT, originally developed 
  in C++ by Niklas Een and Niklas Sorensson, Chalmers University of 
  Technology, Sweden: http://www.cs.chalmers.se/~een/Satzoo.]

  Synopsis    [Input/output of CNFs.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: msatSolverIo.c,v 1.0 2004/01/01 1:00:00 alanmi Exp $]

***********************************************************************/

#include "msatInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static char * Msat_TimeStamp();

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_SolverPrintAssignment( Msat_Solver_t * p )
{
    int i;
    printf( "Current assignments are: \n" );
    for ( i = 0; i < p->nVars; i++ )
        printf( "%d", i % 10 );
    printf( "\n" );
    for ( i = 0; i < p->nVars; i++ )
        if ( p->pAssigns[i] == MSAT_VAR_UNASSIGNED )
            printf( "." );
        else
        {
            assert( i == MSAT_LIT2VAR(p->pAssigns[i]) );
            if ( MSAT_LITSIGN(p->pAssigns[i]) )
                printf( "0" );
            else 
                printf( "1" );
        }
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_SolverPrintClauses( Msat_Solver_t * p )
{
    Msat_Clause_t ** pClauses;
    int nClauses, i;

    printf( "Original clauses: \n" );
    nClauses = Msat_ClauseVecReadSize( p->vClauses );
    pClauses = Msat_ClauseVecReadArray( p->vClauses );
    for ( i = 0; i < nClauses; i++ )
    {
        printf( "%3d: ", i );
        Msat_ClausePrint( pClauses[i] );
    }

    printf( "Learned clauses: \n" );
    nClauses = Msat_ClauseVecReadSize( p->vLearned );
    pClauses = Msat_ClauseVecReadArray( p->vLearned );
    for ( i = 0; i < nClauses; i++ )
    {
        printf( "%3d: ", i );
        Msat_ClausePrint( pClauses[i] );
    }

    printf( "Variable activity: \n" );
    for ( i = 0; i < p->nVars; i++ )
        printf( "%3d : %.4f\n", i, p->pdActivity[i] );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_SolverWriteDimacs( Msat_Solver_t * p, char * pFileName )
{
    FILE * pFile;
    Msat_Clause_t ** pClauses;
    int nClauses, i;

    nClauses = Msat_ClauseVecReadSize(p->vClauses) + Msat_ClauseVecReadSize(p->vLearned);
    for ( i = 0; i < p->nVars; i++ )
        nClauses += ( p->pLevel[i] == 0 );

    pFile = fopen( pFileName, "wb" );
    fprintf( pFile, "c Produced by Msat_SolverWriteDimacs() on %s\n", Msat_TimeStamp() );
    fprintf( pFile, "p cnf %d %d\n", p->nVars, nClauses );

    nClauses = Msat_ClauseVecReadSize( p->vClauses );
    pClauses = Msat_ClauseVecReadArray( p->vClauses );
    for ( i = 0; i < nClauses; i++ )
        Msat_ClauseWriteDimacs( pFile, pClauses[i], 1 );

    nClauses = Msat_ClauseVecReadSize( p->vLearned );
    pClauses = Msat_ClauseVecReadArray( p->vLearned );
    for ( i = 0; i < nClauses; i++ )
        Msat_ClauseWriteDimacs( pFile, pClauses[i], 1 );

    // write zero-level assertions
    for ( i = 0; i < p->nVars; i++ )
        if ( p->pLevel[i] == 0 )
            fprintf( pFile, "%s%d 0\n", ((p->pAssigns[i]&1)? "-": ""), i + 1 );

    fprintf( pFile, "\n" );
    fclose( pFile );
}


/**Function*************************************************************

  Synopsis    [Returns the time stamp.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Msat_TimeStamp()
{
    static char Buffer[100];
    time_t ltime;
    char * TimeStamp;
    // get the current time
    time( &ltime );
    TimeStamp = asctime( localtime( &ltime ) );
    TimeStamp[ strlen(TimeStamp) - 1 ] = 0;
    strcpy( Buffer, TimeStamp );
    return Buffer;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

