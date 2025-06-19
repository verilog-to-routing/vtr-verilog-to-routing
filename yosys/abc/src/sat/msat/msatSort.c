/**CFile****************************************************************

  FileName    [msatSort.c]

  PackageName [A C version of SAT solver MINISAT, originally developed 
  in C++ by Niklas Een and Niklas Sorensson, Chalmers University of 
  Technology, Sweden: http://www.cs.chalmers.se/~een/Satzoo.]

  Synopsis    [Sorting clauses.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: msatSort.c,v 1.0 2004/01/01 1:00:00 alanmi Exp $]

***********************************************************************/

#include "msatInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Msat_SolverSortCompare( Msat_Clause_t ** ppC1, Msat_Clause_t ** ppC2 );

// Returns a random float 0 <= x < 1. Seed must never be 0.
static double drand(double seed) {
    int q;
    seed *= 1389796;
    q = (int)(seed / 2147483647);
    seed -= (double)q * 2147483647;
    return seed / 2147483647; }

// Returns a random integer 0 <= x < size. Seed must never be 0.
static int irand(double seed, int size) {
    return (int)(drand(seed) * size); }

static void Msat_SolverSort( Msat_Clause_t ** array, int size, double seed );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Msat_SolverSort the learned clauses in the increasing order of activity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_SolverSortDB( Msat_Solver_t * p )
{
    Msat_ClauseVec_t * pVecClauses;
    Msat_Clause_t ** pLearned;
    int nLearned;
    // read the parameters
    pVecClauses = Msat_SolverReadLearned( p );
    nLearned    = Msat_ClauseVecReadSize( pVecClauses );
    pLearned    = Msat_ClauseVecReadArray( pVecClauses );
    // Msat_SolverSort the array
//    qMsat_SolverSort( (void *)pLearned, nLearned, sizeof(Msat_Clause_t *), 
//            (int (*)(const void *, const void *)) Msat_SolverSortCompare );
//    printf( "Msat_SolverSorting.\n" );
    Msat_SolverSort( pLearned, nLearned, 91648253 );
/*
    if ( nLearned > 2 )
    {
    printf( "Clause 1: %0.20f\n", Msat_ClauseReadActivity(pLearned[0]) );
    printf( "Clause 2: %0.20f\n", Msat_ClauseReadActivity(pLearned[1]) );
    printf( "Clause 3: %0.20f\n", Msat_ClauseReadActivity(pLearned[2]) );
    }
*/
}

/**Function*************************************************************

  Synopsis    [Comparison procedure for two clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Msat_SolverSortCompare( Msat_Clause_t ** ppC1, Msat_Clause_t ** ppC2 )
{
    float Value1 = Msat_ClauseReadActivity( *ppC1 );
    float Value2 = Msat_ClauseReadActivity( *ppC2 );
    if ( Value1 < Value2 )
        return -1;
    if ( Value1 > Value2 )
        return 1;
    return 0;
}


/**Function*************************************************************

  Synopsis    [Selection sort for small array size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_SolverSortSelection( Msat_Clause_t ** array, int size )
{
    Msat_Clause_t * tmp;
    int i, j, best_i;
    for ( i = 0; i < size-1; i++ )
    {
        best_i = i;
        for (j = i+1; j < size; j++)
        {
            if ( Msat_ClauseReadActivity(array[j]) < Msat_ClauseReadActivity(array[best_i]) )
                best_i = j;
        }
        tmp = array[i]; array[i] = array[best_i]; array[best_i] = tmp;
    }
}

/**Function*************************************************************

  Synopsis    [The original MiniSat sorting procedure.]

  Description [This procedure is used to preserve trace-equivalence
  with the orignal C++ implemenation of the solver.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_SolverSort( Msat_Clause_t ** array, int size, double seed )
{
    if (size <= 15)
        Msat_SolverSortSelection( array, size );
    else
    {
        Msat_Clause_t *   pivot = array[irand(seed, size)];
        Msat_Clause_t *   tmp;
        int              i = -1;
        int              j = size;

        for(;;)
        {
            do i++; while( Msat_ClauseReadActivity(array[i]) < Msat_ClauseReadActivity(pivot) );
            do j--; while( Msat_ClauseReadActivity(pivot) < Msat_ClauseReadActivity(array[j]) );

            if ( i >= j ) break;

            tmp = array[i]; array[i] = array[j]; array[j] = tmp;
        }
        Msat_SolverSort(array    , i     , seed);
        Msat_SolverSort(&array[i], size-i, seed);
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

