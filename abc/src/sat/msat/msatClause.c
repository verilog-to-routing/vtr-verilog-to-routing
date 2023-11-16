/**CFile****************************************************************

  FileName    [msatClause.c]

  PackageName [A C version of SAT solver MINISAT, originally developed 
  in C++ by Niklas Een and Niklas Sorensson, Chalmers University of 
  Technology, Sweden: http://www.cs.chalmers.se/~een/Satzoo.]

  Synopsis    [Procedures working with SAT clauses.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: msatClause.c,v 1.0 2004/01/01 1:00:00 alanmi Exp $]

***********************************************************************/

#include "msatInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

struct Msat_Clause_t_
{
    int           Num;              // unique number of the clause
    unsigned      fLearned   :  1;  // 1 if the clause is learned
    unsigned      fMark      :  1;  // used to mark visited clauses during proof recording
    unsigned      fTypeA     :  1;  // used to mark clauses belonging to A for interpolant computation
    unsigned      nSize      : 14;  // the number of literals in the clause
    unsigned      nSizeAlloc : 15;  // the number of bytes allocated for the clause
    Msat_Lit_t     pData[0];
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates a new clause.]

  Description [Returns FALSE if top-level conflict detected (must be handled); 
  TRUE otherwise. 'pClause_out' may be set to NULL if clause is already 
  satisfied by the top-level assignment.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int  Msat_ClauseCreate( Msat_Solver_t * p, Msat_IntVec_t * vLits, int  fLearned, Msat_Clause_t ** pClause_out )
{
    int * pAssigns = Msat_SolverReadAssignsArray(p);
    Msat_ClauseVec_t ** pvWatched;
    Msat_Clause_t * pC;
    int * pLits;
    int nLits, i, j;
    int nBytes;
    Msat_Var_t Var;
    int  Sign;

    *pClause_out = NULL;

    nLits = Msat_IntVecReadSize(vLits);
    pLits = Msat_IntVecReadArray(vLits);

    if ( !fLearned ) 
    {
        int * pSeen = Msat_SolverReadSeenArray( p );
        int nSeenId;
        assert( Msat_SolverReadDecisionLevel(p) == 0 );
        // sorting literals makes the code trace-equivalent 
        // with to the original C++ solver
        Msat_IntVecSort( vLits, 0 );
        // increment the counter of seen twice
        nSeenId = Msat_SolverIncrementSeenId( p );
        nSeenId = Msat_SolverIncrementSeenId( p );
        // nSeenId - 1 stands for negative
        // nSeenId     stands for positive
        // Remove false literals

        // there is a bug here!!!!
        // when the same var in opposite polarities is given, it drops one polarity!!!

        for ( i = j = 0; i < nLits; i++ ) {
            // get the corresponding variable
            Var  = MSAT_LIT2VAR(pLits[i]);
            Sign = MSAT_LITSIGN(pLits[i]); // Sign=0 for positive
            // check if we already saw this variable in the this clause
            if ( pSeen[Var] >= nSeenId - 1 )
            {
                if ( (pSeen[Var] != nSeenId) == Sign ) // the same lit
                    continue;
                return 1; // two opposite polarity lits -- don't add the clause
            }
            // mark the variable as seen
            pSeen[Var] = nSeenId - !Sign;

            // analize the value of this literal
            if ( pAssigns[Var] != MSAT_VAR_UNASSIGNED )
            {
                if ( pAssigns[Var] == pLits[i] )
                    return 1;  // the clause is always true -- don't add anything
                // the literal has no impact - skip it
                continue;
            }
            // otherwise, add this literal to the clause
            pLits[j++] = pLits[i];
        }
        Msat_IntVecShrink( vLits, j );
        nLits = j;
/*
        // the problem with this code is that performance is very
        // sensitive to the ordering of adjacency lits
        // the best ordering requires fanins first, next fanouts
        // this ordering is more convenient to make from FRAIG

        // create the adjacency information
        if ( nLits > 2 )
        {
            Msat_Var_t VarI, VarJ;
            Msat_IntVec_t * pAdjI, * pAdjJ;

            for ( i = 0; i < nLits; i++ )
            {
                VarI = MSAT_LIT2VAR(pLits[i]);
                pAdjI = (Msat_IntVec_t *)p->vAdjacents->pArray[VarI];

                for ( j = i+1; j < nLits; j++ )
                {
                    VarJ = MSAT_LIT2VAR(pLits[j]);
                    pAdjJ = (Msat_IntVec_t *)p->vAdjacents->pArray[VarJ];

                    Msat_IntVecPushUniqueOrder( pAdjI, VarJ, 1 );
                    Msat_IntVecPushUniqueOrder( pAdjJ, VarI, 1 );
                }
            }
        }
*/
    }
    // 'vLits' is now the (possibly) reduced vector of literals.
    if ( nLits == 0 )
        return 0;
    if ( nLits == 1 )
        return Msat_SolverEnqueue( p, pLits[0], NULL );

    // Allocate clause:
//    nBytes = sizeof(unsigned)*(nLits + 1 + (int)fLearned);
    nBytes = sizeof(unsigned)*(nLits + 2 + (int)fLearned);
#ifdef USE_SYSTEM_MEMORY_MANAGEMENT
    pC = (Msat_Clause_t *)ABC_ALLOC( char, nBytes );
#else
    pC = (Msat_Clause_t *)Msat_MmStepEntryFetch( Msat_SolverReadMem(p), nBytes );
#endif
    pC->Num        = p->nClauses++;
    pC->fTypeA     = 0;
    pC->fMark      = 0;
    pC->fLearned   = fLearned;
    pC->nSize      = nLits;
    pC->nSizeAlloc = nBytes;
    memcpy( pC->pData, pLits, sizeof(int)*nLits );

    // For learnt clauses only:
    if ( fLearned )
    {
        int * pLevel = Msat_SolverReadDecisionLevelArray( p );
        int iLevelMax, iLevelCur, iLitMax;

        // Put the second watch on the literal with highest decision level:
        iLitMax = 1;
        iLevelMax = pLevel[ MSAT_LIT2VAR(pLits[1]) ];
        for ( i = 2; i < nLits; i++ )
        {
            iLevelCur = pLevel[ MSAT_LIT2VAR(pLits[i]) ];
            assert( iLevelCur != -1 );
            if ( iLevelMax < iLevelCur )
            // this is very strange - shouldn't it be???
            // if ( iLevelMax > iLevelCur )
                iLevelMax = iLevelCur, iLitMax = i;
        }
        pC->pData[1]       = pLits[iLitMax];
        pC->pData[iLitMax] = pLits[1];

        // Bumping:
        // (newly learnt clauses should be considered active)
        Msat_ClauseWriteActivity( pC, 0.0 );
        Msat_SolverClaBumpActivity( p, pC ); 
//        if ( nLits < 20 )
        for ( i = 0; i < nLits; i++ )
        {
            Msat_SolverVarBumpActivity( p, pLits[i] );
//            Msat_SolverVarBumpActivity( p, pLits[i] );
//            p->pFreq[ MSAT_LIT2VAR(pLits[i]) ]++;
        }
    }

    // Store clause:
    pvWatched = Msat_SolverReadWatchedArray( p );
    Msat_ClauseVecPush( pvWatched[ MSAT_LITNOT(pC->pData[0]) ], pC );
    Msat_ClauseVecPush( pvWatched[ MSAT_LITNOT(pC->pData[1]) ], pC );
    *pClause_out = pC;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Deallocates the clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_ClauseFree( Msat_Solver_t * p, Msat_Clause_t * pC, int  fRemoveWatched )
{
    if ( fRemoveWatched )
    {
        Msat_Lit_t Lit;
        Msat_ClauseVec_t ** pvWatched;
        pvWatched = Msat_SolverReadWatchedArray( p );
        Lit = MSAT_LITNOT( pC->pData[0] );
        Msat_ClauseRemoveWatch( pvWatched[Lit], pC );
        Lit = MSAT_LITNOT( pC->pData[1] );
        Msat_ClauseRemoveWatch( pvWatched[Lit], pC ); 
    }

#ifdef USE_SYSTEM_MEMORY_MANAGEMENT
    ABC_FREE( pC );
#else
    Msat_MmStepEntryRecycle( Msat_SolverReadMem(p), (char *)pC, pC->nSizeAlloc );
#endif

}

/**Function*************************************************************

  Synopsis    [Access the data field of the clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int   Msat_ClauseReadLearned( Msat_Clause_t * pC )  {  return pC->fLearned; }
int   Msat_ClauseReadSize( Msat_Clause_t * pC )     {  return pC->nSize;    }
int * Msat_ClauseReadLits( Msat_Clause_t * pC )     {  return pC->pData;    }
int   Msat_ClauseReadMark( Msat_Clause_t * pC )     {  return pC->fMark;    }
int   Msat_ClauseReadNum( Msat_Clause_t * pC )      {  return pC->Num;      }
int   Msat_ClauseReadTypeA( Msat_Clause_t * pC )    {  return pC->fTypeA;    }

void  Msat_ClauseSetMark( Msat_Clause_t * pC, int  fMark )   {  pC->fMark = fMark;   }
void  Msat_ClauseSetNum( Msat_Clause_t * pC, int Num )       {  pC->Num = Num;       }
void  Msat_ClauseSetTypeA( Msat_Clause_t * pC, int  fTypeA ) {  pC->fTypeA = fTypeA; }

/**Function*************************************************************

  Synopsis    [Checks whether the learned clause is locked.]

  Description [The clause may be locked if it is the reason of a
  recent conflict. Such clause cannot be removed from the database.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int  Msat_ClauseIsLocked( Msat_Solver_t * p, Msat_Clause_t * pC )
{
    Msat_Clause_t ** pReasons = Msat_SolverReadReasonArray( p );
    return (int )(pReasons[MSAT_LIT2VAR(pC->pData[0])] == pC);
}

/**Function*************************************************************

  Synopsis    [Reads the activity of the given clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Msat_ClauseReadActivity( Msat_Clause_t * pC )
{
    float f;

    memcpy( &f, pC->pData + pC->nSize, sizeof (f));
    return f;
}

/**Function*************************************************************

  Synopsis    [Sets the activity of the clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_ClauseWriteActivity( Msat_Clause_t * pC, float Num )
{
    memcpy( pC->pData + pC->nSize, &Num, sizeof (Num) );
}

/**Function*************************************************************

  Synopsis    [Propages the assignment.]

  Description [The literal that has become true (Lit) is given to this 
  procedure. The array of current variable assignments is given for
  efficiency. The output literal (pLit_out) can be the second watched
  literal (if TRUE is returned) or the conflict literal (if FALSE is 
  returned). This messy interface is used to improve performance.
  This procedure accounts for ~50% of the runtime of the solver.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int  Msat_ClausePropagate( Msat_Clause_t * pC, Msat_Lit_t Lit, int * pAssigns, Msat_Lit_t * pLit_out )
{
    // make sure the false literal is pC->pData[1]
    Msat_Lit_t LitF = MSAT_LITNOT(Lit);
    if ( pC->pData[0] == LitF )
         pC->pData[0] = pC->pData[1], pC->pData[1] = LitF;
    assert( pC->pData[1] == LitF );
    // if the 0-th watch is true, clause is already satisfied
    if ( pAssigns[MSAT_LIT2VAR(pC->pData[0])] == pC->pData[0] )
        return 1; 
    // look for a new watch
    if ( pC->nSize > 2 )
    {
        int i;
        for ( i = 2; i < (int)pC->nSize; i++ )
            if ( pAssigns[MSAT_LIT2VAR(pC->pData[i])] != MSAT_LITNOT(pC->pData[i]) )
            {
                pC->pData[1] = pC->pData[i], pC->pData[i] = LitF;
                *pLit_out = MSAT_LITNOT(pC->pData[1]);
                return 1; 
            }
    }
    // clause is unit under assignment
    *pLit_out = pC->pData[0];
    return 0;
}

/**Function*************************************************************

  Synopsis    [Simplifies the clause.]

  Description [Assumes everything has been propagated! (esp. watches 
  in clauses are NOT unsatisfied)]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int  Msat_ClauseSimplify( Msat_Clause_t * pC, int * pAssigns )
{
    Msat_Var_t Var;
    int i, j;
    for ( i = j = 0; i < (int)pC->nSize; i++ )
    {
        Var = MSAT_LIT2VAR(pC->pData[i]);
        if ( pAssigns[Var] == MSAT_VAR_UNASSIGNED )
        {
            pC->pData[j++] = pC->pData[i];
            continue;
        }
        if ( pAssigns[Var] == pC->pData[i] )
            return 1;
        // otherwise, the value of the literal is false
        // make sure, this literal is not watched
        assert( i >= 2 );
    }
    // if the size has changed, update it and move activity
    if ( j < (int)pC->nSize )
    {
        float Activ = Msat_ClauseReadActivity(pC);
        pC->nSize = j;
        Msat_ClauseWriteActivity(pC, Activ);
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Computes reason of conflict in the given clause.]

  Description [If the literal is unassigned, finds the reason by 
  complementing literals in the given cluase (pC). If the literal is
  assigned, makes sure that this literal is the first one in the clause
  and computes the complement of all other literals in the clause.
  Returns the reason in the given array (vLits_out). If the clause is
  learned, bumps its activity.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_ClauseCalcReason( Msat_Solver_t * p, Msat_Clause_t * pC, Msat_Lit_t Lit, Msat_IntVec_t * vLits_out )
{
    int i;
    // clear the reason
    Msat_IntVecClear( vLits_out );
    assert( Lit == MSAT_LIT_UNASSIGNED || Lit == pC->pData[0] );
    for ( i = (Lit != MSAT_LIT_UNASSIGNED); i < (int)pC->nSize; i++ )
    {
        assert( Msat_SolverReadAssignsArray(p)[MSAT_LIT2VAR(pC->pData[i])] == MSAT_LITNOT(pC->pData[i]) );
        Msat_IntVecPush( vLits_out, MSAT_LITNOT(pC->pData[i]) );
    }
    if ( pC->fLearned ) 
        Msat_SolverClaBumpActivity( p, pC );
}

/**Function*************************************************************

  Synopsis    [Removes the given clause from the watched list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_ClauseRemoveWatch( Msat_ClauseVec_t * vClauses, Msat_Clause_t * pC )
{
    Msat_Clause_t ** pClauses;
    int nClauses, i;
    nClauses = Msat_ClauseVecReadSize( vClauses );
    pClauses = Msat_ClauseVecReadArray( vClauses );
    for ( i = 0; pClauses[i] != pC; i++ )
        assert( i < nClauses );
    for (      ; i < nClauses - 1; i++ )
        pClauses[i] = pClauses[i+1];
    Msat_ClauseVecPop( vClauses );
}

/**Function*************************************************************

  Synopsis    [Prints the given clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_ClausePrint( Msat_Clause_t * pC )
{
    int i;
    if ( pC == NULL )
        printf( "NULL pointer" );
    else 
    {
        if ( pC->fLearned )
            printf( "Act = %.4f  ", Msat_ClauseReadActivity(pC) );
        for ( i = 0; i < (int)pC->nSize; i++ )
            printf( " %s%d", ((pC->pData[i]&1)? "-": ""),  pC->pData[i]/2 + 1 );
    }
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Writes the given clause in a file in DIMACS format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_ClauseWriteDimacs( FILE * pFile, Msat_Clause_t * pC, int  fIncrement )
{
    int i;
    for ( i = 0; i < (int)pC->nSize; i++ )
        fprintf( pFile, "%s%d ", ((pC->pData[i]&1)? "-": ""),  pC->pData[i]/2 + (int)(fIncrement>0) );
    if ( fIncrement )
        fprintf( pFile, "0" );
    fprintf( pFile, "\n" );
}

/**Function*************************************************************

  Synopsis    [Prints the given clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_ClausePrintSymbols( Msat_Clause_t * pC )
{
    int i;
    if ( pC == NULL )
        printf( "NULL pointer" );
    else 
    {
//        if ( pC->fLearned )
//            printf( "Act = %.4f  ", Msat_ClauseReadActivity(pC) );
        for ( i = 0; i < (int)pC->nSize; i++ )
            printf(" " L_LIT, L_lit(pC->pData[i]));
    }
    printf( "\n" );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

