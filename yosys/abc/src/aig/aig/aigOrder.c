/**CFile****************************************************************

  FileName    [aigOrder.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [Dynamically updated topological order.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aigOrder.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Initializes the order datastructure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManOrderStart( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i;
    assert( Aig_ManBufNum(p) == 0 );
    // allocate order datastructure
    assert( p->pOrderData == NULL );
    p->nOrderAlloc = 2 * Aig_ManObjNumMax(p);
    if ( p->nOrderAlloc < (1<<12) )
        p->nOrderAlloc = (1<<12);
    p->pOrderData = ABC_ALLOC( unsigned, 2 * p->nOrderAlloc );
    memset( p->pOrderData, 0xFF, sizeof(unsigned) * 2 * p->nOrderAlloc );
    // add the constant node
    p->pOrderData[0] = p->pOrderData[1] = 0;
    p->iPrev = p->iNext = 0;
    // add the internal nodes
    Aig_ManForEachNode( p, pObj, i )
        Aig_ObjOrderInsert( p, pObj->Id );
}

/**Function*************************************************************

  Synopsis    [Deletes the order datastructure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManOrderStop( Aig_Man_t * p )
{
    assert( p->pOrderData );
    ABC_FREE( p->pOrderData );
    p->nOrderAlloc = 0;
    p->iPrev = p->iNext = 0;
}

/**Function*************************************************************

  Synopsis    [Inserts an entry before iNext.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjOrderInsert( Aig_Man_t * p, int ObjId )
{
    int iPrev;
    assert( ObjId != 0 );
    assert( Aig_ObjIsNode( Aig_ManObj(p, ObjId) ) );
    if ( ObjId >= p->nOrderAlloc )
    {
        int nOrderAlloc = 2 * ObjId; 
        p->pOrderData = ABC_REALLOC( unsigned, p->pOrderData, 2 * nOrderAlloc );
        memset( p->pOrderData + 2 * p->nOrderAlloc, 0xFF, sizeof(unsigned) * 2 * (nOrderAlloc - p->nOrderAlloc) );
        p->nOrderAlloc = nOrderAlloc;
    }
    assert( p->pOrderData[2*ObjId] == 0xFFFFFFFF );   // prev
    assert( p->pOrderData[2*ObjId+1] == 0xFFFFFFFF ); // next
    iPrev = p->pOrderData[2*p->iNext];
    assert( p->pOrderData[2*iPrev+1] == (unsigned)p->iNext );
    p->pOrderData[2*ObjId] = iPrev;
    p->pOrderData[2*iPrev+1] = ObjId;
    p->pOrderData[2*p->iNext] = ObjId;
    p->pOrderData[2*ObjId+1] = p->iNext;
    p->nAndTotal++;
}

/**Function*************************************************************

  Synopsis    [Removes the entry.]

  Description [If iPrev is removed, it slides backward. 
  If iNext is removed, it slides forward.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjOrderRemove( Aig_Man_t * p, int ObjId )
{
    int iPrev, iNext;
    assert( ObjId != 0 );
    assert( Aig_ObjIsNode( Aig_ManObj(p, ObjId) ) );
    iPrev = p->pOrderData[2*ObjId];
    iNext = p->pOrderData[2*ObjId+1];
    p->pOrderData[2*ObjId] = 0xFFFFFFFF;
    p->pOrderData[2*ObjId+1] = 0xFFFFFFFF;
    p->pOrderData[2*iNext] = iPrev;
    p->pOrderData[2*iPrev+1] = iNext;
    if ( p->iPrev == ObjId )
    {
        p->nAndPrev--;
        p->iPrev = iPrev;
    }
    if ( p->iNext == ObjId )
        p->iNext = iNext;
    p->nAndTotal--;
}

/**Function*************************************************************

  Synopsis    [Advances the order forward.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjOrderAdvance( Aig_Man_t * p )
{
    assert( p->pOrderData );
    assert( p->pOrderData[2*p->iPrev+1] == (unsigned)p->iNext );
    p->iPrev = p->iNext;
    p->nAndPrev++;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

