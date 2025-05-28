/**CFile****************************************************************

  FileName    [fraigFanout.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [Procedures to manipulate fanouts of the FRAIG nodes.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - October 1, 2004]

  Revision    [$Id: fraigFanout.c,v 1.5 2005/07/08 01:01:31 alanmi Exp $]

***********************************************************************/

#include "fraigInt.h"

ABC_NAMESPACE_IMPL_START


#ifdef FRAIG_ENABLE_FANOUTS

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
void Fraig_NodeAddFaninFanout( Fraig_Node_t * pFanin, Fraig_Node_t * pFanout )
{
    Fraig_Node_t * pPivot;

    // pFanins is a fanin of pFanout
    assert( !Fraig_IsComplement(pFanin) );
    assert( !Fraig_IsComplement(pFanout) );
    assert( Fraig_Regular(pFanout->p1) == pFanin || Fraig_Regular(pFanout->p2) == pFanin );

    pPivot = pFanin->pFanPivot;
    if ( pPivot == NULL )
    {
        pFanin->pFanPivot = pFanout;
        return;
    }

    if ( Fraig_Regular(pPivot->p1) == pFanin )
    {
        if ( Fraig_Regular(pFanout->p1) == pFanin )
        {
            pFanout->pFanFanin1 = pPivot->pFanFanin1;
            pPivot->pFanFanin1  = pFanout;
        }
        else // if ( Fraig_Regular(pFanout->p2) == pFanin )
        {
            pFanout->pFanFanin2 = pPivot->pFanFanin1;
            pPivot->pFanFanin1  = pFanout;
        }
    }
    else // if ( Fraig_Regular(pPivot->p2) == pFanin )
    {
        assert( Fraig_Regular(pPivot->p2) == pFanin );
        if ( Fraig_Regular(pFanout->p1) == pFanin )
        {
            pFanout->pFanFanin1 = pPivot->pFanFanin2;
            pPivot->pFanFanin2  = pFanout;
        }
        else // if ( Fraig_Regular(pFanout->p2) == pFanin )
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
void Fraig_NodeRemoveFaninFanout( Fraig_Node_t * pFanin, Fraig_Node_t * pFanoutToRemove )
{
    Fraig_Node_t * pFanout, * pFanout2, ** ppFanList;
    // start the linked list of fanouts
    ppFanList = &pFanin->pFanPivot; 
    // go through the fanouts
    Fraig_NodeForEachFanoutSafe( pFanin, pFanout, pFanout2 )
    {
        // skip the fanout-to-remove
        if ( pFanout == pFanoutToRemove )
            continue;
        // add useful fanouts to the list
        *ppFanList = pFanout;
        ppFanList = Fraig_NodeReadNextFanoutPlace( pFanin, pFanout );
    }
    *ppFanList = NULL;
}

/**Function*************************************************************

  Synopsis    [Transfers fanout to a different node.]

  Description [Assumes that the other node currently has no fanouts.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_NodeTransferFanout( Fraig_Node_t * pNodeFrom, Fraig_Node_t * pNodeTo )
{
    Fraig_Node_t * pFanout;
    assert( pNodeTo->pFanPivot == NULL );
    assert( pNodeTo->pFanFanin1 == NULL );
    assert( pNodeTo->pFanFanin2 == NULL );
    // go through the fanouts and update their fanins
    Fraig_NodeForEachFanout( pNodeFrom, pFanout )
    {
        if ( Fraig_Regular(pFanout->p1) == pNodeFrom )
            pFanout->p1 = Fraig_NotCond( pNodeTo, Fraig_IsComplement(pFanout->p1) );
        else if ( Fraig_Regular(pFanout->p2) == pNodeFrom )
            pFanout->p2 = Fraig_NotCond( pNodeTo, Fraig_IsComplement(pFanout->p2) );            
    }
    // move the pointers
    pNodeTo->pFanPivot  = pNodeFrom->pFanPivot;  
    pNodeTo->pFanFanin1 = pNodeFrom->pFanFanin1; 
    pNodeTo->pFanFanin2 = pNodeFrom->pFanFanin2;
    pNodeFrom->pFanPivot  = NULL; 
    pNodeFrom->pFanFanin1 = NULL; 
    pNodeFrom->pFanFanin2 = NULL;  
}

/**Function*************************************************************

  Synopsis    [Returns the number of fanouts of a node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_NodeGetFanoutNum( Fraig_Node_t * pNode )
{
    Fraig_Node_t * pFanout;
    int Counter = 0;
    Fraig_NodeForEachFanout( pNode, pFanout )
        Counter++;
    return Counter;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif

ABC_NAMESPACE_IMPL_END

