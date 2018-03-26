/**CFile****************************************************************

  FileName    [msatSolverCore.c]

  PackageName [A C version of SAT solver MINISAT, originally developed 
  in C++ by Niklas Een and Niklas Sorensson, Chalmers University of 
  Technology, Sweden: http://www.cs.chalmers.se/~een/Satzoo.]

  Synopsis    [The SAT solver core procedures.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: msatSolverCore.c,v 1.2 2004/05/12 03:37:40 satrajit Exp $]

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

  Synopsis    [Adds one variable to the solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int  Msat_SolverAddVar( Msat_Solver_t * p, int Level )
{
    if ( p->nVars == p->nVarsAlloc )
        Msat_SolverResize( p, 2 * p->nVarsAlloc );
    p->pLevel[p->nVars] = Level;
    p->nVars++;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Adds one clause to the solver's clause database.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int  Msat_SolverAddClause( Msat_Solver_t * p, Msat_IntVec_t * vLits )
{
    Msat_Clause_t * pC; 
    int  Value;
    Value = Msat_ClauseCreate( p, vLits, 0, &pC );
    if ( pC != NULL )
        Msat_ClauseVecPush( p->vClauses, pC );
//    else if ( p->fProof )
//        Msat_ClauseCreateFake( p, vLits );
    return Value;
}

/**Function*************************************************************

  Synopsis    [Returns search-space coverage. Not extremely reliable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
double Msat_SolverProgressEstimate( Msat_Solver_t * p )
{
    double dProgress = 0.0;
    double dF = 1.0 / p->nVars;
    int i;
    for ( i = 0; i < p->nVars; i++ )
        if ( p->pAssigns[i] != MSAT_VAR_UNASSIGNED )
            dProgress += pow( dF, p->pLevel[i] );
    return dProgress / p->nVars;
}

/**Function*************************************************************

  Synopsis    [Prints statistics about the solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_SolverPrintStats( Msat_Solver_t * p )
{
    printf("C solver (%d vars; %d clauses; %d learned):\n", 
        p->nVars, Msat_ClauseVecReadSize(p->vClauses), Msat_ClauseVecReadSize(p->vLearned) );
    printf("starts        : %d\n", (int)p->Stats.nStarts);
    printf("conflicts     : %d\n", (int)p->Stats.nConflicts);
    printf("decisions     : %d\n", (int)p->Stats.nDecisions);
    printf("propagations  : %d\n", (int)p->Stats.nPropagations);
    printf("inspects      : %d\n", (int)p->Stats.nInspects);
}

/**Function*************************************************************

  Synopsis    [Top-level solve.]

  Description [If using assumptions (non-empty 'assumps' vector), you must 
  call 'simplifyDB()' first to see that no top-level conflict is present 
  (which would put the solver in an undefined state. If the last argument
  is given (vProj), the solver enumerates through the satisfying solutions,
  which are projected on the variables listed in this array. Note that the
  variables in the array may be complemented, in which case the derived
  assignment for the variable is complemented.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int  Msat_SolverSolve( Msat_Solver_t * p, Msat_IntVec_t * vAssumps, int nBackTrackLimit, int nTimeLimit )
{
    Msat_SearchParams_t Params = { 0.95, 0.999 };
    double nConflictsLimit, nLearnedLimit;
    Msat_Type_t Status;
    abctime timeStart = Abc_Clock();

//    p->pFreq = ABC_ALLOC( int, p->nVarsAlloc );
//    memset( p->pFreq, 0, sizeof(int) * p->nVarsAlloc );
 
    if ( vAssumps )
    {
        int * pAssumps, nAssumps, i;

        assert( Msat_IntVecReadSize(p->vTrailLim) == 0 );

        nAssumps = Msat_IntVecReadSize( vAssumps );
        pAssumps = Msat_IntVecReadArray( vAssumps );
        for ( i = 0; i < nAssumps; i++ )
        {
            if ( !Msat_SolverAssume(p, pAssumps[i]) || Msat_SolverPropagate(p) )
            {
                Msat_QueueClear( p->pQueue );
                Msat_SolverCancelUntil( p, 0 );
                return MSAT_FALSE;
            }
        }
    }
    p->nLevelRoot   = Msat_SolverReadDecisionLevel(p);
    p->nClausesInit = Msat_ClauseVecReadSize( p->vClauses );    
    nConflictsLimit = 100;
    nLearnedLimit   = Msat_ClauseVecReadSize(p->vClauses) / 3;
    Status = MSAT_UNKNOWN;
    p->nBackTracks = (int)p->Stats.nConflicts;
    while ( Status == MSAT_UNKNOWN )
    {
        if ( p->fVerbose )
            printf("Solving -- conflicts=%d   learnts=%d   progress=%.4f %%\n", 
                (int)nConflictsLimit, (int)nLearnedLimit, p->dProgress*100);
        Status = Msat_SolverSearch( p, (int)nConflictsLimit, (int)nLearnedLimit, nBackTrackLimit, &Params );
        nConflictsLimit *= 1.5;
        nLearnedLimit   *= 1.1;
        // if the limit on the number of backtracks is given, quit the restart loop
        if ( nBackTrackLimit > 0 && (int)p->Stats.nConflicts - p->nBackTracks > nBackTrackLimit )
            break;
        // if the runtime limit is exceeded, quit the restart loop
        if ( nTimeLimit > 0 && Abc_Clock() - timeStart >= nTimeLimit * CLOCKS_PER_SEC )
            break;
    }
    Msat_SolverCancelUntil( p, 0 );
    p->nBackTracks = (int)p->Stats.nConflicts - p->nBackTracks;
/*
    ABC_PRT( "True solver runtime", Abc_Clock() - timeStart );
    // print the statistics
    {
        int i, Counter = 0;
        for ( i = 0; i < p->nVars; i++ )
            if ( p->pFreq[i] > 0 )
            {
                printf( "%d ", p->pFreq[i] );
                Counter++;
            }
        if ( Counter )
            printf( "\n" );
        printf( "Total = %d. Used = %d.  Decisions = %d. Imps = %d. Conflicts = %d. ", p->nVars, Counter, (int)p->Stats.nDecisions, (int)p->Stats.nPropagations, (int)p->Stats.nConflicts );
        ABC_PRT( "Time", Abc_Clock() - timeStart );
    }
*/
    return Status;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

