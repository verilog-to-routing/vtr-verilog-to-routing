/**CFile****************************************************************

  FileName    [msatSolverSearch.c]

  PackageName [A C version of SAT solver MINISAT, originally developed 
  in C++ by Niklas Een and Niklas Sorensson, Chalmers University of 
  Technology, Sweden: http://www.cs.chalmers.se/~een/Satzoo.]

  Synopsis    [The search part of the solver.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: msatSolverSearch.c,v 1.0 2004/01/01 1:00:00 alanmi Exp $]

***********************************************************************/

#include "msatInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void            Msat_SolverUndoOne( Msat_Solver_t * p );
static void            Msat_SolverCancel( Msat_Solver_t * p );
static Msat_Clause_t * Msat_SolverRecord( Msat_Solver_t * p, Msat_IntVec_t * vLits );
static void            Msat_SolverAnalyze( Msat_Solver_t * p, Msat_Clause_t * pC, Msat_IntVec_t * vLits_out, int * pLevel_out );
static void            Msat_SolverReduceDB( Msat_Solver_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Makes the next assumption (Lit).]

  Description [Returns FALSE if immediate conflict.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int  Msat_SolverAssume( Msat_Solver_t * p, Msat_Lit_t Lit )
{
    assert( Msat_QueueReadSize(p->pQueue) == 0 );
    if ( p->fVerbose )
        printf(L_IND"assume("L_LIT")\n", L_ind, L_lit(Lit));
    Msat_IntVecPush( p->vTrailLim, Msat_IntVecReadSize(p->vTrail) );
//    assert( Msat_IntVecReadSize(p->vTrailLim) <= Msat_IntVecReadSize(p->vTrail) + 1 );
//    assert( Msat_IntVecReadSize( p->vTrailLim ) < p->nVars );
    return Msat_SolverEnqueue( p, Lit, NULL );
}

/**Function*************************************************************

  Synopsis    [Reverts one variable binding on the trail.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_SolverUndoOne( Msat_Solver_t * p )
{
    Msat_Lit_t Lit;
    Msat_Var_t Var;
    Lit = Msat_IntVecPop( p->vTrail ); 
    Var = MSAT_LIT2VAR(Lit);
    p->pAssigns[Var] = MSAT_VAR_UNASSIGNED;
    p->pReasons[Var] = NULL;
    p->pLevel[Var]   = -1;
//    Msat_OrderUndo( p->pOrder, Var );
    Msat_OrderVarUnassigned( p->pOrder, Var );

    if ( p->fVerbose )
        printf(L_IND"unbind("L_LIT")\n", L_ind, L_lit(Lit)); 
}

/**Function*************************************************************

  Synopsis    [Reverts to the state before last Msat_SolverAssume().]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_SolverCancel( Msat_Solver_t * p )
{
    int c;
    assert( Msat_QueueReadSize(p->pQueue) == 0 );
    if ( p->fVerbose )
    {
        if ( Msat_IntVecReadSize(p->vTrail) != Msat_IntVecReadEntryLast(p->vTrailLim) )
        {
            Msat_Lit_t Lit;
            Lit = Msat_IntVecReadEntry( p->vTrail, Msat_IntVecReadEntryLast(p->vTrailLim) ); 
            printf(L_IND"cancel("L_LIT")\n", L_ind, L_lit(Lit));
        }
    }
    for ( c = Msat_IntVecReadSize(p->vTrail) - Msat_IntVecPop( p->vTrailLim ); c != 0; c-- )
        Msat_SolverUndoOne( p );
}

/**Function*************************************************************

  Synopsis    [Reverts to the state at given level.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_SolverCancelUntil( Msat_Solver_t * p, int Level )
{
    while ( Msat_IntVecReadSize(p->vTrailLim) > Level )
        Msat_SolverCancel(p);
}


/**Function*************************************************************

  Synopsis    [Record a clause and drive backtracking.]

  Description [vLits[0] must contain the asserting literal.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Msat_Clause_t * Msat_SolverRecord( Msat_Solver_t * p, Msat_IntVec_t * vLits )
{
    Msat_Clause_t * pC;
    int Value;
    assert( Msat_IntVecReadSize(vLits) != 0 );
    Value = Msat_ClauseCreate( p, vLits, 1, &pC );
    assert( Value );
    Value = Msat_SolverEnqueue( p, Msat_IntVecReadEntry(vLits,0), pC );
    assert( Value );
    if ( pC )
        Msat_ClauseVecPush( p->vLearned, pC );
    return pC;
}

/**Function*************************************************************

  Synopsis    [Enqueues one variable assignment.]

  Description [Puts a new fact on the propagation queue and immediately 
  updates the variable value. Should a conflict arise, FALSE is returned.
  Otherwise returns TRUE.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int  Msat_SolverEnqueue( Msat_Solver_t * p, Msat_Lit_t Lit, Msat_Clause_t * pC )
{
    Msat_Var_t Var = MSAT_LIT2VAR(Lit);

    // skip literals that are not in the current cone
    if ( !Msat_IntVecReadEntry( p->vVarsUsed, Var ) )
        return 1;

//    assert( Msat_QueueReadSize(p->pQueue) == Msat_IntVecReadSize(p->vTrail) );
    // if the literal is assigned
    // return 1 if the assignment is consistent
    // return 0 if the assignment is inconsistent (conflict)
    if ( p->pAssigns[Var] != MSAT_VAR_UNASSIGNED )
        return p->pAssigns[Var] == Lit;
    // new fact - store it
    if ( p->fVerbose )
    {
//        printf(L_IND"bind("L_LIT")\n", L_ind, L_lit(Lit));
        printf(L_IND"bind("L_LIT")  ", L_ind, L_lit(Lit));
        Msat_ClausePrintSymbols( pC );
    }
    p->pAssigns[Var] = Lit;
    p->pLevel[Var]   = Msat_IntVecReadSize(p->vTrailLim);
//    p->pReasons[Var] = p->pLevel[Var]? pC: NULL;
    p->pReasons[Var] = pC;
    Msat_IntVecPush( p->vTrail, Lit );
    Msat_QueueInsert( p->pQueue, Lit );

    Msat_OrderVarAssigned( p->pOrder, Var );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Propagates the assignments in the queue.]

  Description [Propagates all enqueued facts. If a conflict arises, 
  the conflicting clause is returned, otherwise NULL.]
               
  SideEffects [The propagation queue is empty, even if there was a conflict.]

  SeeAlso     []

***********************************************************************/
Msat_Clause_t * Msat_SolverPropagate( Msat_Solver_t * p )
{
    Msat_ClauseVec_t ** pvWatched = p->pvWatched;
    Msat_Clause_t ** pClauses;
    Msat_Clause_t * pConflict;
    Msat_Lit_t Lit, Lit_out;
    int i, j, nClauses;

    // propagate all the literals in the queue
    while ( (Lit = Msat_QueueExtract( p->pQueue )) >= 0 )
    {
        p->Stats.nPropagations++;
        // get the clauses watched by this literal
        nClauses = Msat_ClauseVecReadSize( pvWatched[Lit] );
        pClauses = Msat_ClauseVecReadArray( pvWatched[Lit] );
        // go through the watched clauses and decide what to do with them
        for ( i = j = 0; i < nClauses; i++ )
        {
            p->Stats.nInspects++;
            // clear the returned literal
            Lit_out = -1;
            // propagate the clause
            if ( !Msat_ClausePropagate( pClauses[i], Lit, p->pAssigns, &Lit_out ) )
            {   // the clause is unit
                // "Lit_out" contains the new assignment to be enqueued
                if ( Msat_SolverEnqueue( p, Lit_out, pClauses[i] ) ) 
                { // consistent assignment 
                    // no changes to the implication queue; the watch is the same too
                    pClauses[j++] = pClauses[i];
                    continue;
                }
                // remember the reason of conflict (will be returned)
                pConflict = pClauses[i];
                // leave the remaning clauses in the same watched list
                for ( ; i < nClauses; i++ )
                    pClauses[j++] = pClauses[i];
                Msat_ClauseVecShrink( pvWatched[Lit], j );
                // clear the propagation queue
                Msat_QueueClear( p->pQueue );
                return pConflict;
            }
            // the clause is not unit
            // in this case "Lit_out" contains the new watch if it has changed
            if ( Lit_out >= 0 )
                Msat_ClauseVecPush( pvWatched[Lit_out], pClauses[i] );
            else // the watch did not change
                pClauses[j++] = pClauses[i];
        }
        Msat_ClauseVecShrink( pvWatched[Lit], j );
    }
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Simplifies the data base.]

  Description [Simplify all constraints according to the current top-level 
  assigment (redundant constraints may be removed altogether).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int  Msat_SolverSimplifyDB( Msat_Solver_t * p )
{
    Msat_ClauseVec_t * vClauses;
    Msat_Clause_t ** pClauses;
    int nClauses, Type, i, j;
    int * pAssigns;
    int Counter;

    assert( Msat_SolverReadDecisionLevel(p) == 0 );
    if ( Msat_SolverPropagate(p) != NULL )
        return 0;
//Msat_SolverPrintClauses( p );
//Msat_SolverPrintAssignment( p );
//printf( "Simplification\n" );

    // simplify and reassign clause numbers
    Counter = 0;
    pAssigns = Msat_SolverReadAssignsArray( p );
    for ( Type = 0; Type < 2; Type++ )
    {
        vClauses = Type? p->vLearned : p->vClauses;
        nClauses = Msat_ClauseVecReadSize( vClauses );
        pClauses = Msat_ClauseVecReadArray( vClauses );
        for ( i = j = 0; i < nClauses; i++ )
            if ( Msat_ClauseSimplify( pClauses[i], pAssigns ) )
                Msat_ClauseFree( p, pClauses[i], 1 );
            else
            {
                pClauses[j++] = pClauses[i];
                Msat_ClauseSetNum( pClauses[i], Counter++ );
            }
        Msat_ClauseVecShrink( vClauses, j );
    }
    p->nClauses = Counter;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Cleans the clause databased from the useless learnt clauses.]

  Description [Removes half of the learnt clauses, minus the clauses locked 
  by the current assignment. Locked clauses are clauses that are reason 
  to a some assignment.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_SolverReduceDB( Msat_Solver_t * p )
{
    Msat_Clause_t ** pLearned;
    int nLearned, i, j;
    double dExtraLim = p->dClaInc / Msat_ClauseVecReadSize(p->vLearned); 
    // Remove any clause below this activity

    // sort the learned clauses in the increasing order of activity
    Msat_SolverSortDB( p );

    // discard the first half the clauses (the less active ones)
    nLearned = Msat_ClauseVecReadSize( p->vLearned );
    pLearned = Msat_ClauseVecReadArray( p->vLearned );
    for ( i = j = 0; i < nLearned / 2; i++ )
        if ( !Msat_ClauseIsLocked( p, pLearned[i]) )
            Msat_ClauseFree( p, pLearned[i], 1 );
        else
            pLearned[j++] = pLearned[i];
    // filter the more active clauses and leave those above the limit
    for (  ; i < nLearned; i++ )
        if ( !Msat_ClauseIsLocked( p, pLearned[i] ) && 
            Msat_ClauseReadActivity(pLearned[i]) < dExtraLim )
            Msat_ClauseFree( p, pLearned[i], 1 );
        else
            pLearned[j++] = pLearned[i];
    Msat_ClauseVecShrink( p->vLearned, j );
}

/**Function*************************************************************

  Synopsis    [Removes the learned clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_SolverRemoveLearned( Msat_Solver_t * p )
{
    Msat_Clause_t ** pLearned;
    int nLearned, i;

    // discard the learned clauses
    nLearned = Msat_ClauseVecReadSize( p->vLearned );
    pLearned = Msat_ClauseVecReadArray( p->vLearned );
    for ( i = 0; i < nLearned; i++ )
    {
        assert( !Msat_ClauseIsLocked( p, pLearned[i]) );
        
        Msat_ClauseFree( p, pLearned[i], 1 );
    }
    Msat_ClauseVecShrink( p->vLearned, 0 );
    p->nClauses = Msat_ClauseVecReadSize(p->vClauses);

    for ( i = 0; i < p->nVarsAlloc; i++ )
        p->pReasons[i] = NULL;
}

/**Function*************************************************************

  Synopsis    [Removes the recently added clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_SolverRemoveMarked( Msat_Solver_t * p )
{
    Msat_Clause_t ** pLearned, ** pClauses;
    int nLearned, nClauses, i;

    // discard the learned clauses
    nClauses = Msat_ClauseVecReadSize( p->vClauses );
    pClauses = Msat_ClauseVecReadArray( p->vClauses );
    for ( i = p->nClausesStart; i < nClauses; i++ )
    {
//        assert( !Msat_ClauseIsLocked( p, pClauses[i]) );
        Msat_ClauseFree( p, pClauses[i], 1 );
    }
    Msat_ClauseVecShrink( p->vClauses, p->nClausesStart );

    // discard the learned clauses
    nLearned = Msat_ClauseVecReadSize( p->vLearned );
    pLearned = Msat_ClauseVecReadArray( p->vLearned );
    for ( i = 0; i < nLearned; i++ )
    {
//        assert( !Msat_ClauseIsLocked( p, pLearned[i]) );
        Msat_ClauseFree( p, pLearned[i], 1 );
    }
    Msat_ClauseVecShrink( p->vLearned, 0 );
    p->nClauses = Msat_ClauseVecReadSize(p->vClauses);
/*
    // undo the previous data
    for ( i = 0; i < p->nVarsAlloc; i++ )
    {
        p->pAssigns[i]   = MSAT_VAR_UNASSIGNED;
        p->pReasons[i]   = NULL;
        p->pLevel[i]     = -1;
        p->pdActivity[i] = 0.0;
    }
    Msat_OrderClean( p->pOrder, p->nVars, NULL );
    Msat_QueueClear( p->pQueue );
*/
}



/**Function*************************************************************

  Synopsis    [Analyze conflict and produce a reason clause.]

  Description [Current decision level must be greater than root level.]
               
  SideEffects [vLits_out[0] is the asserting literal at level pLevel_out.]

  SeeAlso     []

***********************************************************************/
void Msat_SolverAnalyze( Msat_Solver_t * p, Msat_Clause_t * pC, Msat_IntVec_t * vLits_out, int * pLevel_out )
{
    Msat_Lit_t LitQ, Lit = MSAT_LIT_UNASSIGNED;
    Msat_Var_t VarQ, Var;
    int * pReasonArray, nReasonSize;
    int j, pathC = 0, nLevelCur = Msat_IntVecReadSize(p->vTrailLim);
    int iStep = Msat_IntVecReadSize(p->vTrail) - 1;

    // increment the seen counter
    p->nSeenId++;
    // empty the vector array
    Msat_IntVecClear( vLits_out );
    Msat_IntVecPush( vLits_out, -1 ); // (leave room for the asserting literal)
    *pLevel_out = 0;
    do {
        assert( pC != NULL );  // (otherwise should be UIP)
        // get the reason of conflict
        Msat_ClauseCalcReason( p, pC, Lit, p->vReason );
        nReasonSize  = Msat_IntVecReadSize( p->vReason );
        pReasonArray = Msat_IntVecReadArray( p->vReason );
        for ( j = 0; j < nReasonSize; j++ ) {
            LitQ = pReasonArray[j];
            VarQ = MSAT_LIT2VAR(LitQ);
            if ( p->pSeen[VarQ] != p->nSeenId ) {
                p->pSeen[VarQ] = p->nSeenId;

                // added to better fine-tune the search
                Msat_SolverVarBumpActivity( p, LitQ );

                // skip all the literals on this decision level
                if ( p->pLevel[VarQ] == nLevelCur )
                    pathC++;
                else if ( p->pLevel[VarQ] > 0 ) { 
                    // add the literals on other decision levels but
                    // exclude variables from decision level 0
                    Msat_IntVecPush( vLits_out, MSAT_LITNOT(LitQ) );
                    if ( *pLevel_out < p->pLevel[VarQ] )
                        *pLevel_out = p->pLevel[VarQ];
                }
            }
        }
        // Select next clause to look at:
        do {
//            Lit = Msat_IntVecReadEntryLast(p->vTrail);
            Lit = Msat_IntVecReadEntry( p->vTrail, iStep-- );
            Var = MSAT_LIT2VAR(Lit);
            pC = p->pReasons[Var];
//            Msat_SolverUndoOne( p );
        } while ( p->pSeen[Var] != p->nSeenId );
        pathC--;
    } while ( pathC > 0 );
    // we do not unbind the variables above
    // this will be done after conflict analysis

    Msat_IntVecWriteEntry( vLits_out, 0, MSAT_LITNOT(Lit) );
    if ( p->fVerbose )
    {
        printf( L_IND"Learnt {", L_ind );
        nReasonSize  = Msat_IntVecReadSize( vLits_out );
        pReasonArray = Msat_IntVecReadArray( vLits_out );
        for ( j = 0; j < nReasonSize; j++ ) 
            printf(" "L_LIT, L_lit(pReasonArray[j]));
        printf(" } at level %d\n", *pLevel_out); 
    }
}

/**Function*************************************************************

  Synopsis    [The search procedure called between the restarts.]

  Description [Search for a satisfying solution as long as the number of 
  conflicts does not exceed the limit (nConfLimit) while keeping the number 
  of learnt clauses below the provided limit (nLearnedLimit). NOTE! Use 
  negative value for nConfLimit or nLearnedLimit to indicate infinity.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Msat_Type_t Msat_SolverSearch( Msat_Solver_t * p, int nConfLimit, int nLearnedLimit, int nBackTrackLimit, Msat_SearchParams_t * pPars )
{
    Msat_Clause_t * pConf;
    Msat_Var_t Var;
    int nLevelBack, nConfs, nAssigns, Value;
    int i;

    assert( Msat_SolverReadDecisionLevel(p) == p->nLevelRoot );
    p->Stats.nStarts++;
    p->dVarDecay = 1 / pPars->dVarDecay;
    p->dClaDecay = 1 / pPars->dClaDecay;

    // reset the activities
    for ( i = 0; i < p->nVars; i++ )
       p->pdActivity[i] = (double)p->pFactors[i];
//       p->pdActivity[i] = 0.0;

    nConfs = 0;
    while ( 1 )
    {
        pConf = Msat_SolverPropagate( p );
        if ( pConf != NULL ){
            // CONFLICT
            if ( p->fVerbose )
            {
//                printf(L_IND"**CONFLICT**\n", L_ind);
                printf(L_IND"**CONFLICT**  ", L_ind);
                Msat_ClausePrintSymbols( pConf );
            }
            // count conflicts
            p->Stats.nConflicts++;
            nConfs++;

            // if top level, return UNSAT
            if ( Msat_SolverReadDecisionLevel(p) == p->nLevelRoot )
                return MSAT_FALSE;

            // perform conflict analysis
            Msat_SolverAnalyze( p, pConf, p->vTemp, &nLevelBack );
            Msat_SolverCancelUntil( p, (p->nLevelRoot > nLevelBack)? p->nLevelRoot : nLevelBack );
            Msat_SolverRecord( p, p->vTemp );

            // it is important that recording is done after cancelling
            // because canceling cleans the queue while recording adds to it
            Msat_SolverVarDecayActivity( p );
            Msat_SolverClaDecayActivity( p );

        }
        else{
            // NO CONFLICT
            if ( Msat_IntVecReadSize(p->vTrailLim) == 0 ) {
                // Simplify the set of problem clauses:
//                Value = Msat_SolverSimplifyDB(p);
//                assert( Value );
            }
            nAssigns = Msat_IntVecReadSize( p->vTrail );
            if ( nLearnedLimit >= 0 && Msat_ClauseVecReadSize(p->vLearned) >= nLearnedLimit + nAssigns ) {
                // Reduce the set of learnt clauses:
                Msat_SolverReduceDB(p);
            }

            Var = Msat_OrderVarSelect( p->pOrder );
            if ( Var == MSAT_ORDER_UNKNOWN ) {
                // Model found and stored in p->pAssigns
                memcpy( p->pModel, p->pAssigns, sizeof(int) * p->nVars );
                Msat_QueueClear( p->pQueue );
                Msat_SolverCancelUntil( p, p->nLevelRoot );
                return MSAT_TRUE;
            }
            if ( nConfLimit > 0 && nConfs > nConfLimit ) {
                // Reached bound on number of conflicts:
                p->dProgress = Msat_SolverProgressEstimate( p );
                Msat_QueueClear( p->pQueue );
                Msat_SolverCancelUntil( p, p->nLevelRoot );
                return MSAT_UNKNOWN; 
            }
            else if ( nBackTrackLimit > 0 && (int)p->Stats.nConflicts - p->nBackTracks > nBackTrackLimit ) {
                // Reached bound on number of conflicts:
                Msat_QueueClear( p->pQueue );
                Msat_SolverCancelUntil( p, p->nLevelRoot );
                return MSAT_UNKNOWN; 
            }
            else{
                // New variable decision:
                p->Stats.nDecisions++;
                assert( Var != MSAT_ORDER_UNKNOWN && Var >= 0 && Var < p->nVars );
                Value = Msat_SolverAssume(p, MSAT_VAR2LIT(Var,0) );
                assert( Value );
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

