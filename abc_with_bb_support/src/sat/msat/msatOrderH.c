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


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// the variable package data structure
struct Msat_Order_t_
{
    Msat_Solver_t *      pSat;         // the SAT solver 
    Msat_IntVec_t *      vIndex;       // the heap
    Msat_IntVec_t *      vHeap;        // the mapping of var num into its heap num
};

//The solver can communicate to the variable order the following parts:
//- the array of current assignments (pSat->pAssigns)
//- the array of variable activities (pSat->pdActivity)
//- the array of variables currently in the cone (pSat->vConeVars)
//- the array of arrays of variables adjucent to each(pSat->vAdjacents)

#define HLEFT(i)               ((i)<<1)
#define HRIGHT(i)             (((i)<<1)+1)
#define HPARENT(i)             ((i)>>1)
#define HCOMPARE(p, i, j)      ((p)->pSat->pdActivity[i] > (p)->pSat->pdActivity[j])
#define HHEAP(p, i)            ((p)->vHeap->pArray[i])
#define HSIZE(p)               ((p)->vHeap->nSize)
#define HOKAY(p, i)            ((i) >= 0 && (i) < (p)->vIndex->nSize)
#define HINHEAP(p, i)          (HOKAY(p, i) && (p)->vIndex->pArray[i] != 0)
#define HEMPTY(p)              (HSIZE(p) == 1)

static int Msat_HeapCheck_rec( Msat_Order_t * p, int i );
static int Msat_HeapGetTop( Msat_Order_t * p );
static void Msat_HeapInsert( Msat_Order_t * p, int n );
static void Msat_HeapIncrease( Msat_Order_t * p, int n );
static void Msat_HeapPercolateUp( Msat_Order_t * p, int i );
static void Msat_HeapPercolateDown( Msat_Order_t * p, int i );

extern abctime timeSelect;

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
    p = ABC_ALLOC( Msat_Order_t, 1 );
    memset( p, 0, sizeof(Msat_Order_t) );
    p->pSat   = pSat;
    p->vIndex = Msat_IntVecAlloc( 0 );
    p->vHeap  = Msat_IntVecAlloc( 0 );
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
    Msat_IntVecGrow( p->vIndex, nVarsMax );
    Msat_IntVecGrow( p->vHeap, nVarsMax + 1 );
    p->vIndex->nSize = nVarsMax;
    p->vHeap->nSize = 0;
}

/**Function*************************************************************

  Synopsis    [Cleans the ordering structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_OrderClean( Msat_Order_t * p, Msat_IntVec_t * vCone )
{
    int i;
    for ( i = 0; i < p->vIndex->nSize; i++ )
        p->vIndex->pArray[i] = 0;
    for ( i = 0; i < vCone->nSize; i++ )
    {
        assert( i+1 < p->vHeap->nCap );
        p->vHeap->pArray[i+1] = vCone->pArray[i];

        assert( vCone->pArray[i] < p->vIndex->nSize );
        p->vIndex->pArray[vCone->pArray[i]] = i+1;
    }
    p->vHeap->nSize = vCone->nSize + 1;
}

/**Function*************************************************************

  Synopsis    [Checks that the J-boundary is okay.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Msat_OrderCheck( Msat_Order_t * p )
{
    return Msat_HeapCheck_rec( p, 1 );
}

/**Function*************************************************************

  Synopsis    [Frees the ordering structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_OrderFree( Msat_Order_t * p )
{
    Msat_IntVecFree( p->vHeap );
    Msat_IntVecFree( p->vIndex );
    ABC_FREE( p );
}



/**Function*************************************************************

  Synopsis    [Selects the next variable to assign.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Msat_OrderVarSelect( Msat_Order_t * p )
{
    // Activity based decision:
//    while (!heap.empty()){
//        Var next = heap.getmin();
//        if (toLbool(assigns[next]) == l_Undef)
//            return next;
//    }
//    return var_Undef;

    int Var;
    abctime clk = Abc_Clock();

    while ( !HEMPTY(p) )
    {
        Var = Msat_HeapGetTop(p);
        if ( (p)->pSat->pAssigns[Var] == MSAT_VAR_UNASSIGNED )
        {
//assert( Msat_OrderCheck(p) );
timeSelect += Abc_Clock() - clk;
            return Var;
        }
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
}

/**Function*************************************************************

  Synopsis    [Updates the order after a variable is unassigned.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_OrderVarUnassigned( Msat_Order_t * p, int Var )
{
//    if (!heap.inHeap(x))
//        heap.insert(x);

    abctime clk = Abc_Clock();
    if ( !HINHEAP(p,Var) )
        Msat_HeapInsert( p, Var );
timeSelect += Abc_Clock() - clk;
}

/**Function*************************************************************

  Synopsis    [Updates the order after a variable changed weight.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_OrderUpdate( Msat_Order_t * p, int Var )
{
//    if (heap.inHeap(x))
//        heap.increase(x);

    abctime clk = Abc_Clock();
    if ( HINHEAP(p,Var) )
        Msat_HeapIncrease( p, Var );
timeSelect += Abc_Clock() - clk;
}




/**Function*************************************************************

  Synopsis    [Checks the heap property recursively.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Msat_HeapCheck_rec( Msat_Order_t * p, int i )
{
    return i >= HSIZE(p) ||
        (
            ( HPARENT(i) == 0 || !HCOMPARE(p, HHEAP(p, i), HHEAP(p, HPARENT(i))) ) &&

            Msat_HeapCheck_rec( p, HLEFT(i) ) && 
            
            Msat_HeapCheck_rec( p, HRIGHT(i) )
        );
}

/**Function*************************************************************

  Synopsis    [Retrieves the minimum element.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Msat_HeapGetTop( Msat_Order_t * p )
{
    int Result, NewTop;
    Result                    = HHEAP(p, 1);
    NewTop                    = Msat_IntVecPop( p->vHeap );
    p->vHeap->pArray[1]       = NewTop;
    p->vIndex->pArray[NewTop] = 1;
    p->vIndex->pArray[Result] = 0;
    if ( p->vHeap->nSize > 1 )
        Msat_HeapPercolateDown( p, 1 );
    return Result;
}

/**Function*************************************************************

  Synopsis    [Inserts the new element.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_HeapInsert( Msat_Order_t * p, int n )
{
    assert( HOKAY(p, n) );
    p->vIndex->pArray[n] = HSIZE(p);
    Msat_IntVecPush( p->vHeap, n );
    Msat_HeapPercolateUp( p, p->vIndex->pArray[n] );
}

/**Function*************************************************************

  Synopsis    [Inserts the new element.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_HeapIncrease( Msat_Order_t * p, int n )
{
    Msat_HeapPercolateUp( p, p->vIndex->pArray[n] );
}

/**Function*************************************************************

  Synopsis    [Moves the entry up.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_HeapPercolateUp( Msat_Order_t * p, int i )
{
    int x = HHEAP(p, i);
    while ( HPARENT(i) != 0 && HCOMPARE(p, x, HHEAP(p, HPARENT(i))) )
    {
        p->vHeap->pArray[i]            = HHEAP(p, HPARENT(i));
        p->vIndex->pArray[HHEAP(p, i)] = i;
        i                              = HPARENT(i);
    }
    p->vHeap->pArray[i]  = x;
    p->vIndex->pArray[x] = i;
}

/**Function*************************************************************

  Synopsis    [Moves the entry down.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_HeapPercolateDown( Msat_Order_t * p, int i )
{
    int x = HHEAP(p, i);
    int Child;
    while ( HLEFT(i) < HSIZE(p) )
    {
        if ( HRIGHT(i) < HSIZE(p) && HCOMPARE(p, HHEAP(p, HRIGHT(i)), HHEAP(p, HLEFT(i))) )
            Child = HRIGHT(i);
        else
            Child = HLEFT(i);
        if ( !HCOMPARE(p, HHEAP(p, Child), x) )
            break;
        p->vHeap->pArray[i]            = HHEAP(p, Child);
        p->vIndex->pArray[HHEAP(p, i)] = i;
        i                              = Child;
    }
    p->vHeap->pArray[i]  = x;
    p->vIndex->pArray[x] = i;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

