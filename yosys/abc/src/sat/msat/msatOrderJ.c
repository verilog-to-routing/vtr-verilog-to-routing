/**CFile****************************************************************

  FileName    [msatOrder.c]

  PackageName [A C version of SAT solver MINISAT, originally developed 
  in C++ by Niklas Een and Niklas Sorensson, Chalmers University of 
  Technology, Sweden: http://www.cs.chalmers.se/~een/Satzoo.]

  Synopsis    [The manager of variable assignment.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: msatOrder.c,v 1.0 2005/05/30 1:00:00 alanmi Exp $]

***********************************************************************/

#include "msatInt.h"

ABC_NAMESPACE_IMPL_START


/* 
The J-boundary (justification boundary) is defined as a set of unassigned 
variables belonging to the cone of interest, such that for each of them,
there exist an adjacent assigned variable in the cone of interest.
*/

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Msat_OrderVar_t_  Msat_OrderVar_t;
typedef struct Msat_OrderRing_t_ Msat_OrderRing_t;

// the variable data structure
struct Msat_OrderVar_t_
{
    Msat_OrderVar_t    * pNext;
    Msat_OrderVar_t    * pPrev;
    int                  Num;
};

// the ring of variables data structure (J-boundary)
struct Msat_OrderRing_t_
{
    Msat_OrderVar_t    * pRoot; 
    int                 nItems;
};

// the variable package data structure
struct Msat_Order_t_
{
    Msat_Solver_t *      pSat;         // the SAT solver 
    Msat_OrderVar_t *    pVars;        // the storage for variables
    int                 nVarsAlloc;   // the number of variables allocated
    Msat_OrderRing_t     rVars;        // the J-boundary as a ring of variables
};

//The solver can communicate to the variable order the following parts:
//- the array of current assignments (pSat->pAssigns)
//- the array of variable activities (pSat->pdActivity)
//- the array of variables currently in the cone (pSat->vConeVars)
//- the array of arrays of variables adjucent to each(pSat->vAdjacents)

#define Msat_OrderVarIsInBoundary( p, i )   ((p)->pVars[i].pNext)
#define Msat_OrderVarIsAssigned( p, i )     ((p)->pSat->pAssigns[i] != MSAT_VAR_UNASSIGNED)
#define Msat_OrderVarIsUsedInCone( p, i )   ((p)->pSat->vVarsUsed->pArray[i])

// iterator through the entries in J-boundary
#define Msat_OrderRingForEachEntry( pRing, pVar, pNext )         \
    for ( pVar = pRing,                                         \
          pNext = pVar? pVar->pNext : NULL;                     \
          pVar;                                                 \
          pVar = (pNext != pRing)? pNext : NULL,                \
          pNext = pVar? pVar->pNext : NULL )

static void Msat_OrderRingAddLast( Msat_OrderRing_t * pRing, Msat_OrderVar_t * pVar );
static void Msat_OrderRingRemove( Msat_OrderRing_t * pRing, Msat_OrderVar_t * pVar );

extern clock_t timeSelect;
extern clock_t timeAssign;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates the ordering structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Msat_Order_t * Msat_OrderAlloc( Msat_Solver_t * pSat )
{
    Msat_Order_t * p;
    p = ALLOC( Msat_Order_t, 1 );
    memset( p, 0, sizeof(Msat_Order_t) );
    p->pSat = pSat;
    Msat_OrderSetBounds( p, pSat->nVarsAlloc );
    return p;
}

/**Function*************************************************************

  Synopsis    [Sets the bound of the ordering structure.]

  Description [Should be called whenever the SAT solver is resized.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_OrderSetBounds( Msat_Order_t * p, int nVarsMax )
{
    int i;
    // add variables if they are missing
    if ( p->nVarsAlloc < nVarsMax )
    {
        p->pVars = REALLOC( Msat_OrderVar_t, p->pVars, nVarsMax );
        for ( i = p->nVarsAlloc; i < nVarsMax; i++ )
        {
            p->pVars[i].pNext = p->pVars[i].pPrev = NULL;
            p->pVars[i].Num = i;
        }
        p->nVarsAlloc = nVarsMax;
    }
}

/**Function*************************************************************

  Synopsis    [Cleans the ordering structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_OrderClean( Msat_Order_t * p, Msat_IntVec_t * vCone )
{
    Msat_OrderVar_t * pVar, * pNext;
    // quickly undo the ring
    Msat_OrderRingForEachEntry( p->rVars.pRoot, pVar, pNext )
        pVar->pNext = pVar->pPrev = NULL;
    p->rVars.pRoot  = NULL;
    p->rVars.nItems = 0;
}

/**Function*************************************************************

  Synopsis    [Checks that the J-boundary is okay.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Msat_OrderCheck( Msat_Order_t * p )
{
    Msat_OrderVar_t * pVar, * pNext;
    Msat_IntVec_t * vRound;
    int * pRound, nRound;
    int * pVars, nVars, i, k;
    int Counter = 0;

    // go through all the variables in the boundary
    Msat_OrderRingForEachEntry( p->rVars.pRoot, pVar, pNext )
    {
        assert( !Msat_OrderVarIsAssigned(p, pVar->Num) );
        // go though all the variables in the neighborhood
        // and check that it is true that there is least one assigned
        vRound = (Msat_IntVec_t *)Msat_ClauseVecReadEntry( p->pSat->vAdjacents, pVar->Num );
        nRound = Msat_IntVecReadSize( vRound );
        pRound = Msat_IntVecReadArray( vRound );
        for ( i = 0; i < nRound; i++ )
        {
            if ( !Msat_OrderVarIsUsedInCone(p, pRound[i]) )
                continue;
            if ( Msat_OrderVarIsAssigned(p, pRound[i]) )
                break;
        }
//        assert( i != nRound );
//        if ( i == nRound )
//            return 0;
        if ( i == nRound )
            Counter++;
    }
    if ( Counter > 0 )
        printf( "%d(%d) ", Counter, p->rVars.nItems );

    // we may also check other unassigned variables in the cone
    // to make sure that if they are not in J-boundary, 
    // then they do not have an assigned neighbor
    nVars = Msat_IntVecReadSize( p->pSat->vConeVars );
    pVars = Msat_IntVecReadArray( p->pSat->vConeVars );
    for ( i = 0; i < nVars; i++ )
    {
        assert( Msat_OrderVarIsUsedInCone(p, pVars[i]) );
        // skip assigned vars, vars in the boundary, and vars not used in the cone
        if ( Msat_OrderVarIsAssigned(p, pVars[i]) || 
             Msat_OrderVarIsInBoundary(p, pVars[i]) )
            continue;
        // make sure, it does not have assigned neighbors
        vRound = (Msat_IntVec_t *)Msat_ClauseVecReadEntry( p->pSat->vAdjacents, pVars[i] );
        nRound = Msat_IntVecReadSize( vRound );
        pRound = Msat_IntVecReadArray( vRound );
        for ( k = 0; k < nRound; k++ )
        {
            if ( !Msat_OrderVarIsUsedInCone(p, pRound[k]) )
                continue;
            if ( Msat_OrderVarIsAssigned(p, pRound[k]) )
                break;
        }
//        assert( k == nRound );
//        if ( k != nRound )
//            return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Frees the ordering structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_OrderFree( Msat_Order_t * p )
{
    free( p->pVars );
    free( p );
}

/**Function*************************************************************

  Synopsis    [Selects the next variable to assign.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Msat_OrderVarSelect( Msat_Order_t * p )
{
    Msat_OrderVar_t * pVar, * pNext, * pVarBest;
    double * pdActs = p->pSat->pdActivity;
    double dfActBest;
//    clock_t clk = clock();

    pVarBest = NULL;
    dfActBest = -1.0;
    Msat_OrderRingForEachEntry( p->rVars.pRoot, pVar, pNext )
    {
        if ( dfActBest < pdActs[pVar->Num] )
        {
            dfActBest = pdActs[pVar->Num];
            pVarBest  = pVar;
        }
    }
//timeSelect += clock() - clk;
//timeAssign += clock() - clk;

//if ( pVarBest && pVarBest->Num % 1000 == 0 )
//printf( "%d ", p->rVars.nItems );

//    Msat_OrderCheck( p );
    if ( pVarBest )
    {
        assert( Msat_OrderVarIsUsedInCone(p, pVarBest->Num) );
        return pVarBest->Num;
    }
    return MSAT_ORDER_UNKNOWN;
}

/**Function*************************************************************

  Synopsis    [Updates J-boundary when the variable is assigned.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_OrderVarAssigned( Msat_Order_t * p, int Var )
{
    Msat_IntVec_t * vRound;
    int i;//, clk = clock();

    // make sure the variable is in the boundary
    assert( Var < p->nVarsAlloc );
    // if it is not in the boundary (initial decision, random decision), do not remove
    if ( Msat_OrderVarIsInBoundary( p, Var ) )
        Msat_OrderRingRemove( &p->rVars, &p->pVars[Var] );
    // add to the boundary those neighbors that are (1) unassigned, (2) not in boundary
    // because for them we know that there is a variable (Var) which is assigned
    vRound = (Msat_IntVec_t *)p->pSat->vAdjacents->pArray[Var];
    for ( i = 0; i < vRound->nSize; i++ )
    {
        if ( !Msat_OrderVarIsUsedInCone(p, vRound->pArray[i]) )
            continue;
        if ( Msat_OrderVarIsAssigned(p, vRound->pArray[i]) )
            continue;
        if ( Msat_OrderVarIsInBoundary(p, vRound->pArray[i]) )
            continue;
        Msat_OrderRingAddLast( &p->rVars, &p->pVars[vRound->pArray[i]] );
    }
//timeSelect += clock() - clk;
//    Msat_OrderCheck( p );
}

/**Function*************************************************************

  Synopsis    [Updates the order after a variable is unassigned.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_OrderVarUnassigned( Msat_Order_t * p, int Var )
{
    Msat_IntVec_t * vRound, * vRound2;
    int i, k;//, clk = clock();

    // make sure the variable is not in the boundary
    assert( Var < p->nVarsAlloc );
    assert( !Msat_OrderVarIsInBoundary( p, Var ) );
    // go through its neigbors - if one of them is assigned add this var
    // add to the boundary those neighbors that are not there already
    // this will also get rid of variable outside of the current cone
    // because they are unassigned in Msat_SolverPrepare()
    vRound = (Msat_IntVec_t *)p->pSat->vAdjacents->pArray[Var];
    for ( i = 0; i < vRound->nSize; i++ )
        if ( Msat_OrderVarIsAssigned(p, vRound->pArray[i]) ) 
            break;
    if ( i != vRound->nSize )
        Msat_OrderRingAddLast( &p->rVars, &p->pVars[Var] );

    // unassigning a variable may lead to its adjacents dropping from the boundary
    for ( i = 0; i < vRound->nSize; i++ )
        if ( Msat_OrderVarIsInBoundary(p, vRound->pArray[i]) )
        { // the neighbor is in the J-boundary (and unassigned)
            assert( !Msat_OrderVarIsAssigned(p, vRound->pArray[i]) );
            vRound2 = (Msat_IntVec_t *)p->pSat->vAdjacents->pArray[vRound->pArray[i]];
            // go through its neighbors and determine if there is at least one assigned
            for ( k = 0; k < vRound2->nSize; k++ )
                if ( Msat_OrderVarIsAssigned(p, vRound2->pArray[k]) ) 
                    break;
            if ( k == vRound2->nSize ) // there is no assigned vars, delete this one
                Msat_OrderRingRemove( &p->rVars, &p->pVars[vRound->pArray[i]] );
        }
//timeSelect += clock() - clk;
//    Msat_OrderCheck( p );
}

/**Function*************************************************************

  Synopsis    [Updates the order after a variable changed weight.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_OrderUpdate( Msat_Order_t * p, int Var )
{
}


/**Function*************************************************************

  Synopsis    [Adds node to the end of the ring.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_OrderRingAddLast( Msat_OrderRing_t * pRing, Msat_OrderVar_t * pVar )
{
//printf( "adding %d\n", pVar->Num );
    // check that the node is not in a ring
    assert( pVar->pPrev == NULL );
    assert( pVar->pNext == NULL );
    // if the ring is empty, make the node point to itself
    pRing->nItems++;
    if ( pRing->pRoot == NULL )
    {
        pRing->pRoot  = pVar;
        pVar->pPrev  = pVar;
        pVar->pNext  = pVar;
        return;
    }
    // if the ring is not empty, add it as the last entry
    pVar->pPrev = pRing->pRoot->pPrev;
    pVar->pNext = pRing->pRoot;
    pVar->pPrev->pNext = pVar;
    pVar->pNext->pPrev = pVar;

    // move the root so that it points to the new entry
//    pRing->pRoot = pRing->pRoot->pPrev;
}

/**Function*************************************************************

  Synopsis    [Removes the node from the ring.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_OrderRingRemove( Msat_OrderRing_t * pRing, Msat_OrderVar_t * pVar )
{
//printf( "removing %d\n", pVar->Num );
    // check that the var is in a ring
    assert( pVar->pPrev );
    assert( pVar->pNext );
    pRing->nItems--;
    if ( pRing->nItems == 0 )
    {
        assert( pRing->pRoot == pVar );
        pVar->pPrev = NULL;
        pVar->pNext = NULL;
        pRing->pRoot = NULL;
        return;
    }
    // move the root if needed
    if ( pRing->pRoot == pVar )
        pRing->pRoot = pVar->pNext;
    // move the root to the next entry after pVar
    // this way all the additions to the list will be traversed first
//    pRing->pRoot = pVar->pPrev;
    // delete the node
    pVar->pPrev->pNext = pVar->pNext;
    pVar->pNext->pPrev = pVar->pPrev;
    pVar->pPrev = NULL;
    pVar->pNext = NULL;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

