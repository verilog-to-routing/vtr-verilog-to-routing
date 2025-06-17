/**CFile****************************************************************

  FileName    [reoUnits.c]

  PackageName [REO: A specialized DD reordering engine.]

  Synopsis    [Procedures which support internal data structures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - October 15, 2002.]

  Revision    [$Id: reoUnits.c,v 1.0 2002/15/10 03:00:00 alanmi Exp $]

***********************************************************************/

#include "reo.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void reoUnitsAddToFreeUnitList( reo_man * p );

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Extract the next unit from the free unit list.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
reo_unit * reoUnitsGetNextUnit(reo_man * p )
{
    reo_unit * pUnit;
    // check there are stil units to extract
    if ( p->pUnitFreeList == NULL )
        reoUnitsAddToFreeUnitList( p );
    // extract the next unit from the linked list
    pUnit            = p->pUnitFreeList;
    p->pUnitFreeList = pUnit->Next;
    p->nUnitsUsed++;
    return pUnit;
}

/**Function*************************************************************

  Synopsis    [Returns the unit to the free unit list.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void reoUnitsRecycleUnit( reo_man * p, reo_unit * pUnit )
{
    pUnit->Next      = p->pUnitFreeList;
    p->pUnitFreeList = pUnit;
    p->nUnitsUsed--;
}

/**Function*************************************************************

  Synopsis    [Returns the list of units to the free unit list.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void reoUnitsRecycleUnitList( reo_man * p, reo_plane * pPlane )
{
    reo_unit * pUnit;
    reo_unit * pTail = NULL; // Suppress "might be used uninitialized"

    if ( pPlane->pHead == NULL )
        return;

    // find the tail
    for ( pUnit = pPlane->pHead; pUnit; pUnit = pUnit->Next )
        pTail = pUnit;
    pTail->Next = p->pUnitFreeList;
    p->pUnitFreeList    = pPlane->pHead;
    memset( pPlane, 0, sizeof(reo_plane) );
//    pPlane->pHead = NULL;
}

/**Function*************************************************************

  Synopsis    [Stops the unit dispenser.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void reoUnitsStopDispenser( reo_man * p )
{
    int i;
    for ( i = 0; i < p->nMemChunks; i++ )
        ABC_FREE( p->pMemChunks[i] );
//    printf("\nThe number of chunks used is %d, each of them %d units\n", p->nMemChunks, REO_CHUNK_SIZE );
    p->nMemChunks = 0;
}

/**Function*************************************************************

  Synopsis    [Adds one unit to the list of units which constitutes the plane.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void reoUnitsAddUnitToPlane( reo_plane * pPlane, reo_unit * pUnit )
{
    if ( pPlane->pHead == NULL )
    {
        pPlane->pHead = pUnit;
        pUnit->Next   = NULL;
    }
    else
    {
        pUnit->Next   = pPlane->pHead;
        pPlane->pHead = pUnit;
    }
    pPlane->statsNodes++;
}


/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void reoUnitsAddToFreeUnitList( reo_man * p )
{
    int c;
    // check that we still have chunks left
    if ( p->nMemChunks == p->nMemChunksAlloc )
    {
        printf( "reoUnitsAddToFreeUnitList(): Memory manager ran out of memory!\n" );
        fflush( stdout );
        return;
    }
    // allocate the next chunk
    assert( p->pUnitFreeList == NULL );
    p->pUnitFreeList = ABC_ALLOC( reo_unit, REO_CHUNK_SIZE );
    // split chunks into list-connected units
    for ( c = 0; c < REO_CHUNK_SIZE-1; c++ )
        (p->pUnitFreeList + c)->Next = p->pUnitFreeList + c + 1;
    // set the last pointer to NULL
    (p->pUnitFreeList + REO_CHUNK_SIZE-1)->Next = NULL;
    // add the chunk to the array of chunks
    p->pMemChunks[p->nMemChunks++] = p->pUnitFreeList;
}


////////////////////////////////////////////////////////////////////////
///                         END OF FILE                              ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END

