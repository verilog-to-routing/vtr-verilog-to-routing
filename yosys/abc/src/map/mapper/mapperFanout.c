/**CFile****************************************************************

  FileName    [mapperFanout.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [Procedures to manipulate fanouts of the FRAIG nodes.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - June 1, 2004.]

  Revision    [$Id: mapperFanout.c,v 1.5 2005/01/23 06:59:43 alanmi Exp $]

***********************************************************************/

#include "mapperInt.h"

ABC_NAMESPACE_IMPL_START


#ifdef MAP_ALLOCATE_FANOUT

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Add the fanout to the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_NodeAddFaninFanout( Map_Node_t * pFanin, Map_Node_t * pFanout )
{
    Map_Node_t * pPivot;

    // pFanins is a fanin of pFanout
    assert( !Map_IsComplement(pFanin) );
    assert( !Map_IsComplement(pFanout) );
    assert( Map_Regular(pFanout->p1) == pFanin || Map_Regular(pFanout->p2) == pFanin );

    pPivot = pFanin->pFanPivot;
    if ( pPivot == NULL )
    {
        pFanin->pFanPivot = pFanout;
        return;
    }

    if ( Map_Regular(pPivot->p1) == pFanin )
    {
        if ( Map_Regular(pFanout->p1) == pFanin )
        {
            pFanout->pFanFanin1 = pPivot->pFanFanin1;
            pPivot->pFanFanin1  = pFanout;
        }
        else // if ( Map_Regular(pFanout->p2) == pFanin )
        {
            pFanout->pFanFanin2 = pPivot->pFanFanin1;
            pPivot->pFanFanin1  = pFanout;
        }
    }
    else // if ( Map_Regular(pPivot->p2) == pFanin )
    {
        assert( Map_Regular(pPivot->p2) == pFanin );
        if ( Map_Regular(pFanout->p1) == pFanin )
        {
            pFanout->pFanFanin1 = pPivot->pFanFanin2;
            pPivot->pFanFanin2  = pFanout;
        }
        else // if ( Map_Regular(pFanout->p2) == pFanin )
        {
            pFanout->pFanFanin2 = pPivot->pFanFanin2;
            pPivot->pFanFanin2  = pFanout;
        }
    }
}

/**Function*************************************************************

  Synopsis    [Add the fanout to the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_NodeRemoveFaninFanout( Map_Node_t * pFanin, Map_Node_t * pFanoutToRemove )
{
    Map_Node_t * pFanout, * pFanout2, ** ppFanList;
    // start the linked list of fanouts
    ppFanList = &pFanin->pFanPivot; 
    // go through the fanouts
    Map_NodeForEachFanoutSafe( pFanin, pFanout, pFanout2 )
    {
        // skip the fanout-to-remove
        if ( pFanout == pFanoutToRemove )
            continue;
        // add useful fanouts to the list
        *ppFanList = pFanout;
        ppFanList = Map_NodeReadNextFanoutPlace( pFanin, pFanout );
    }
    *ppFanList = NULL;
}

/**Function*************************************************************

  Synopsis    [Returns the number of fanouts of a node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_NodeGetFanoutNum( Map_Node_t * pNode )
{
    Map_Node_t * pFanout;
    int Counter = 0;
    Map_NodeForEachFanout( pNode, pFanout )
        Counter++;
    return Counter;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif

ABC_NAMESPACE_IMPL_END

