/**CFile****************************************************************

  FileName    [msatSolverApi.c]

  PackageName [A C version of SAT solver MINISAT, originally developed 
  in C++ by Niklas Een and Niklas Sorensson, Chalmers University of 
  Technology, Sweden: http://www.cs.chalmers.se/~een/Satzoo.]

  Synopsis    [APIs of the SAT solver.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: msatSolverApi.c,v 1.0 2004/01/01 1:00:00 alanmi Exp $]

***********************************************************************/

#include "msatInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Msat_SolverSetupTruthTables( unsigned uTruths[][2] );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Simple SAT solver APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int                Msat_SolverReadVarNum( Msat_Solver_t * p )                  { return p->nVars;      }
int                Msat_SolverReadClauseNum( Msat_Solver_t * p )               { return p->nClauses;   }
int                Msat_SolverReadVarAllocNum( Msat_Solver_t * p )             { return p->nVarsAlloc; }
int                Msat_SolverReadDecisionLevel( Msat_Solver_t * p )           { return Msat_IntVecReadSize(p->vTrailLim); }
int *              Msat_SolverReadDecisionLevelArray( Msat_Solver_t * p )      { return p->pLevel;     }
Msat_Clause_t **    Msat_SolverReadReasonArray( Msat_Solver_t * p )            { return p->pReasons;   }
Msat_Type_t         Msat_SolverReadVarValue( Msat_Solver_t * p, Msat_Var_t Var ) { return (Msat_Type_t)p->pAssigns[Var]; }
Msat_ClauseVec_t *  Msat_SolverReadLearned( Msat_Solver_t * p )                { return p->vLearned;   }
Msat_ClauseVec_t ** Msat_SolverReadWatchedArray( Msat_Solver_t * p )           { return p->pvWatched;  }
int *              Msat_SolverReadAssignsArray( Msat_Solver_t * p )            { return p->pAssigns;   }
int *              Msat_SolverReadModelArray( Msat_Solver_t * p )              { return p->pModel;     }
int                Msat_SolverReadBackTracks( Msat_Solver_t * p )              { return (int)p->Stats.nConflicts; }
int                Msat_SolverReadInspects( Msat_Solver_t * p )                { return (int)p->Stats.nInspects;  }
Msat_MmStep_t *     Msat_SolverReadMem( Msat_Solver_t * p )                    { return p->pMem;       }
int *              Msat_SolverReadSeenArray( Msat_Solver_t * p )               { return p->pSeen;      }
int                Msat_SolverIncrementSeenId( Msat_Solver_t * p )             { return ++p->nSeenId;  }
void               Msat_SolverSetVerbosity( Msat_Solver_t * p, int fVerbose )  { p->fVerbose = fVerbose; }
void               Msat_SolverClausesIncrement( Msat_Solver_t * p )            { p->nClausesAlloc++;   }
void               Msat_SolverClausesDecrement( Msat_Solver_t * p )            { p->nClausesAlloc--;   }
void               Msat_SolverClausesIncrementL( Msat_Solver_t * p )           { p->nClausesAllocL++;  }
void               Msat_SolverClausesDecrementL( Msat_Solver_t * p )           { p->nClausesAllocL--;  }
void               Msat_SolverMarkLastClauseTypeA( Msat_Solver_t * p )         { Msat_ClauseSetTypeA( Msat_ClauseVecReadEntry( p->vClauses, Msat_ClauseVecReadSize(p->vClauses)-1 ), 1 ); }
void               Msat_SolverMarkClausesStart( Msat_Solver_t * p )            { p->nClausesStart = Msat_ClauseVecReadSize(p->vClauses); }
float *            Msat_SolverReadFactors( Msat_Solver_t * p )                 { return p->pFactors;   }

/**Function*************************************************************

  Synopsis    [Reads the clause with the given number.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Msat_Clause_t * Msat_SolverReadClause( Msat_Solver_t * p, int Num )
{
    int nClausesP;
    assert( Num < p->nClauses );
    nClausesP = Msat_ClauseVecReadSize( p->vClauses );
    if ( Num < nClausesP )
        return Msat_ClauseVecReadEntry( p->vClauses, Num );
    return Msat_ClauseVecReadEntry( p->vLearned, Num - nClausesP );
}

/**Function*************************************************************

  Synopsis    [Reads the clause with the given number.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Msat_ClauseVec_t *  Msat_SolverReadAdjacents( Msat_Solver_t * p )
{
    return p->vAdjacents;
}

/**Function*************************************************************

  Synopsis    [Reads the clause with the given number.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Msat_IntVec_t *  Msat_SolverReadConeVars( Msat_Solver_t * p )
{
    return p->vConeVars;
}

/**Function*************************************************************

  Synopsis    [Reads the clause with the given number.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Msat_IntVec_t *  Msat_SolverReadVarsUsed( Msat_Solver_t * p )
{
    return p->vVarsUsed;
}


/**Function*************************************************************

  Synopsis    [Allocates the solver.]

  Description [After the solver is allocated, the procedure
  Msat_SolverClean() should be called to set the number of variables.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Msat_Solver_t * Msat_SolverAlloc( int nVarsAlloc,
    double dClaInc, double dClaDecay, 
    double dVarInc, double dVarDecay, 
    int  fVerbose )
{
    Msat_Solver_t * p;
    int i;

    assert(sizeof(Msat_Lit_t) == sizeof(unsigned));
    assert(sizeof(float)     == sizeof(unsigned));

    p = ABC_ALLOC( Msat_Solver_t, 1 );
    memset( p, 0, sizeof(Msat_Solver_t) );

    p->nVarsAlloc = nVarsAlloc;
    p->nVars     = 0;

    p->nClauses  = 0;
    p->vClauses  = Msat_ClauseVecAlloc( 512 );
    p->vLearned  = Msat_ClauseVecAlloc( 512 );

    p->dClaInc   = dClaInc;
    p->dClaDecay = dClaDecay;
    p->dVarInc   = dVarInc;
    p->dVarDecay = dVarDecay;

    p->pdActivity = ABC_ALLOC( double, p->nVarsAlloc );
    p->pFactors   = ABC_ALLOC( float, p->nVarsAlloc );
    for ( i = 0; i < p->nVarsAlloc; i++ )
    {
        p->pdActivity[i] = 0.0;
        p->pFactors[i]   = 1.0;
    }

    p->pAssigns  = ABC_ALLOC( int, p->nVarsAlloc ); 
    p->pModel    = ABC_ALLOC( int, p->nVarsAlloc ); 
    for ( i = 0; i < p->nVarsAlloc; i++ )
        p->pAssigns[i] = MSAT_VAR_UNASSIGNED;
//    p->pOrder    = Msat_OrderAlloc( p->pAssigns, p->pdActivity, p->nVarsAlloc );
    p->pOrder    = Msat_OrderAlloc( p );

    p->pvWatched = ABC_ALLOC( Msat_ClauseVec_t *, 2 * p->nVarsAlloc );
    for ( i = 0; i < 2 * p->nVarsAlloc; i++ )
        p->pvWatched[i] = Msat_ClauseVecAlloc( 16 );
    p->pQueue    = Msat_QueueAlloc( p->nVarsAlloc );

    p->vTrail    = Msat_IntVecAlloc( p->nVarsAlloc );
    p->vTrailLim = Msat_IntVecAlloc( p->nVarsAlloc );
    p->pReasons  = ABC_ALLOC( Msat_Clause_t *, p->nVarsAlloc );
    memset( p->pReasons, 0, sizeof(Msat_Clause_t *) * p->nVarsAlloc );
    p->pLevel = ABC_ALLOC( int, p->nVarsAlloc );
    for ( i = 0; i < p->nVarsAlloc; i++ )
        p->pLevel[i] = -1;
    p->dRandSeed = 91648253;
    p->fVerbose  = fVerbose;
    p->dProgress = 0.0;
//    p->pModel = Msat_IntVecAlloc( p->nVarsAlloc );
    p->pMem = Msat_MmStepStart( 10 );

    p->vConeVars   = Msat_IntVecAlloc( p->nVarsAlloc ); 
    p->vAdjacents  = Msat_ClauseVecAlloc( p->nVarsAlloc );
    for ( i = 0; i < p->nVarsAlloc; i++ )
        Msat_ClauseVecPush( p->vAdjacents, (Msat_Clause_t *)Msat_IntVecAlloc(5) );
    p->vVarsUsed   = Msat_IntVecAlloc( p->nVarsAlloc ); 
    Msat_IntVecFill( p->vVarsUsed, p->nVarsAlloc, 1 );


    p->pSeen     = ABC_ALLOC( int, p->nVarsAlloc );
    memset( p->pSeen, 0, sizeof(int) * p->nVarsAlloc );
    p->nSeenId   = 1;
    p->vReason   = Msat_IntVecAlloc( p->nVarsAlloc );
    p->vTemp     = Msat_IntVecAlloc( p->nVarsAlloc );
    return p;
}

/**Function*************************************************************

  Synopsis    [Resizes the solver.]

  Description [Assumes that the solver contains some clauses, and that 
  it is currently between the calls. Resizes the solver to accomodate 
  more variables.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_SolverResize( Msat_Solver_t * p, int nVarsAlloc )
{
    int nVarsAllocOld, i;

    nVarsAllocOld = p->nVarsAlloc;
    p->nVarsAlloc = nVarsAlloc;

    p->pdActivity = ABC_REALLOC( double, p->pdActivity, p->nVarsAlloc );
    p->pFactors   = ABC_REALLOC( float, p->pFactors, p->nVarsAlloc );
    for ( i = nVarsAllocOld; i < p->nVarsAlloc; i++ )
    {
        p->pdActivity[i] = 0.0;
        p->pFactors[i]   = 1.0;
    }

    p->pAssigns  = ABC_REALLOC( int, p->pAssigns, p->nVarsAlloc );
    p->pModel    = ABC_REALLOC( int, p->pModel, p->nVarsAlloc );
    for ( i = nVarsAllocOld; i < p->nVarsAlloc; i++ )
        p->pAssigns[i] = MSAT_VAR_UNASSIGNED;

//    Msat_OrderRealloc( p->pOrder, p->pAssigns, p->pdActivity, p->nVarsAlloc );
    Msat_OrderSetBounds( p->pOrder, p->nVarsAlloc );

    p->pvWatched = ABC_REALLOC( Msat_ClauseVec_t *, p->pvWatched, 2 * p->nVarsAlloc );
    for ( i = 2 * nVarsAllocOld; i < 2 * p->nVarsAlloc; i++ )
        p->pvWatched[i] = Msat_ClauseVecAlloc( 16 );

    Msat_QueueFree( p->pQueue );
    p->pQueue    = Msat_QueueAlloc( p->nVarsAlloc );

    p->pReasons  = ABC_REALLOC( Msat_Clause_t *, p->pReasons, p->nVarsAlloc );
    p->pLevel    = ABC_REALLOC( int, p->pLevel, p->nVarsAlloc );
    for ( i = nVarsAllocOld; i < p->nVarsAlloc; i++ )
    {
        p->pReasons[i] = NULL;
        p->pLevel[i] = -1;
    }

    p->pSeen     = ABC_REALLOC( int, p->pSeen, p->nVarsAlloc );
    for ( i = nVarsAllocOld; i < p->nVarsAlloc; i++ )
        p->pSeen[i] = 0;

    Msat_IntVecGrow( p->vTrail, p->nVarsAlloc );
    Msat_IntVecGrow( p->vTrailLim, p->nVarsAlloc );

    // make sure the array of adjucents has room to store the variable numbers
    for ( i = Msat_ClauseVecReadSize(p->vAdjacents); i < p->nVarsAlloc; i++ )
        Msat_ClauseVecPush( p->vAdjacents, (Msat_Clause_t *)Msat_IntVecAlloc(5) );
    Msat_IntVecFill( p->vVarsUsed, p->nVarsAlloc, 1 );
}

/**Function*************************************************************

  Synopsis    [Prepares the solver.]

  Description [Cleans the solver assuming that the problem will involve 
  the given number of variables (nVars). This procedure is useful 
  for many small (incremental) SAT problems, to prevent the solver
  from being reallocated each time.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_SolverClean( Msat_Solver_t * p, int nVars )
{
    int i;
    // free the clauses
    int nClauses;
    Msat_Clause_t ** pClauses;

    assert( p->nVarsAlloc >= nVars );
    p->nVars    = nVars;
    p->nClauses = 0;

    nClauses = Msat_ClauseVecReadSize( p->vClauses );
    pClauses = Msat_ClauseVecReadArray( p->vClauses );
    for ( i = 0; i < nClauses; i++ )
        Msat_ClauseFree( p, pClauses[i], 0 );
//    Msat_ClauseVecFree( p->vClauses );
    Msat_ClauseVecClear( p->vClauses );

    nClauses = Msat_ClauseVecReadSize( p->vLearned );
    pClauses = Msat_ClauseVecReadArray( p->vLearned );
    for ( i = 0; i < nClauses; i++ )
        Msat_ClauseFree( p, pClauses[i], 0 );
//    Msat_ClauseVecFree( p->vLearned );
    Msat_ClauseVecClear( p->vLearned );

//    ABC_FREE( p->pdActivity );
    for ( i = 0; i < p->nVars; i++ )
        p->pdActivity[i] = 0;

//    Msat_OrderFree( p->pOrder );
//    Msat_OrderClean( p->pOrder, p->nVars, NULL );
    Msat_OrderSetBounds( p->pOrder, p->nVars );

    for ( i = 0; i < 2 * p->nVars; i++ )
//        Msat_ClauseVecFree( p->pvWatched[i] );
        Msat_ClauseVecClear( p->pvWatched[i] );
//    ABC_FREE( p->pvWatched );
//    Msat_QueueFree( p->pQueue );
    Msat_QueueClear( p->pQueue );

//    ABC_FREE( p->pAssigns );
    for ( i = 0; i < p->nVars; i++ )
        p->pAssigns[i] = MSAT_VAR_UNASSIGNED;
//    Msat_IntVecFree( p->vTrail );
    Msat_IntVecClear( p->vTrail );
//    Msat_IntVecFree( p->vTrailLim );
    Msat_IntVecClear( p->vTrailLim );
//    ABC_FREE( p->pReasons );
    memset( p->pReasons, 0, sizeof(Msat_Clause_t *) * p->nVars );
//    ABC_FREE( p->pLevel );
    for ( i = 0; i < p->nVars; i++ )
        p->pLevel[i] = -1;
//    Msat_IntVecFree( p->pModel );
//    Msat_MmStepStop( p->pMem, 0 );
    p->dRandSeed = 91648253;
    p->dProgress = 0.0;

//    ABC_FREE( p->pSeen );
    memset( p->pSeen, 0, sizeof(int) * p->nVars );
    p->nSeenId = 1;
//    Msat_IntVecFree( p->vReason );
    Msat_IntVecClear( p->vReason );
//    Msat_IntVecFree( p->vTemp );
    Msat_IntVecClear( p->vTemp );
//    printf(" The number of clauses remaining = %d (%d).\n", p->nClausesAlloc, p->nClausesAllocL );
//    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Frees the solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_SolverFree( Msat_Solver_t * p )
{
    int i;

    // free the clauses
    int nClauses;
    Msat_Clause_t ** pClauses;
//printf( "clauses = %d. learned = %d.\n", Msat_ClauseVecReadSize( p->vClauses ), 
//                                         Msat_ClauseVecReadSize( p->vLearned ) );

    nClauses = Msat_ClauseVecReadSize( p->vClauses );
    pClauses = Msat_ClauseVecReadArray( p->vClauses );
    for ( i = 0; i < nClauses; i++ )
        Msat_ClauseFree( p, pClauses[i], 0 );
    Msat_ClauseVecFree( p->vClauses );

    nClauses = Msat_ClauseVecReadSize( p->vLearned );
    pClauses = Msat_ClauseVecReadArray( p->vLearned );
    for ( i = 0; i < nClauses; i++ )
        Msat_ClauseFree( p, pClauses[i], 0 );
    Msat_ClauseVecFree( p->vLearned );

    ABC_FREE( p->pdActivity );
    ABC_FREE( p->pFactors );
    Msat_OrderFree( p->pOrder );

    for ( i = 0; i < 2 * p->nVarsAlloc; i++ )
        Msat_ClauseVecFree( p->pvWatched[i] );
    ABC_FREE( p->pvWatched );
    Msat_QueueFree( p->pQueue );

    ABC_FREE( p->pAssigns );
    ABC_FREE( p->pModel );
    Msat_IntVecFree( p->vTrail );
    Msat_IntVecFree( p->vTrailLim );
    ABC_FREE( p->pReasons );
    ABC_FREE( p->pLevel );

    Msat_MmStepStop( p->pMem, 0 );

    nClauses = Msat_ClauseVecReadSize( p->vAdjacents );
    pClauses = Msat_ClauseVecReadArray( p->vAdjacents );
    for ( i = 0; i < nClauses; i++ )
        Msat_IntVecFree( (Msat_IntVec_t *)pClauses[i] );
    Msat_ClauseVecFree( p->vAdjacents );
    Msat_IntVecFree( p->vConeVars );
    Msat_IntVecFree( p->vVarsUsed );

    ABC_FREE( p->pSeen );
    Msat_IntVecFree( p->vReason );
    Msat_IntVecFree( p->vTemp );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Prepares the solver to run on a subset of variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_SolverPrepare( Msat_Solver_t * p, Msat_IntVec_t * vVars )
{

    int i;
    // undo the previous data
    for ( i = 0; i < p->nVarsAlloc; i++ )
    {
        p->pAssigns[i]   = MSAT_VAR_UNASSIGNED;
        p->pReasons[i]   = NULL;
        p->pLevel[i]     = -1;
        p->pdActivity[i] = 0.0;
    }

    // set the new variable order
    Msat_OrderClean( p->pOrder, vVars );

    Msat_QueueClear( p->pQueue );
    Msat_IntVecClear( p->vTrail );
    Msat_IntVecClear( p->vTrailLim );
    p->dProgress = 0.0;
}

/**Function*************************************************************

  Synopsis    [Sets up the truth tables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_SolverSetupTruthTables( unsigned uTruths[][2] )
{
    int m, v;
    // set up the truth tables
    for ( m = 0; m < 32; m++ )
        for ( v = 0; v < 5; v++ )
            if ( m & (1 << v) )
                uTruths[v][0] |= (1 << m);
    // make adjustments for the case of 6 variables
    for ( v = 0; v < 5; v++ )
        uTruths[v][1] = uTruths[v][0];
    uTruths[5][0] = 0;
    uTruths[5][1] = ~((unsigned)0);
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

