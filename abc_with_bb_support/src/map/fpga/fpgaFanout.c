/**CFile****************************************************************

  FileName    [fpgaFanout.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [Procedures to manipulate fanouts of the FRAIG nodes.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - August 18, 2004.]

  Revision    [$Id: fpgaFanout.c,v 1.1 2005/01/23 06:59:41 alanmi Exp $]

***********************************************************************/

#include "fpgaInt.h"

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
void Fpga_NodeAddFaninFanout( Fpga_Node_t * pFanin, Fpga_Node_t * pFanout )
{
    Fpga_Node_t * pPivot;

    // pFanins is a fanin of pFanout
    assert( !Fpga_IsComplement(pFanin) );
    assert( !Fpga_IsComplement(pFanout) );
    assert( Fpga_Regular(pFanout->p1) == pFanin || Fpga_Regular(pFanout->p2) == pFanin );

    pPivot = pFanin->pFanPivot;
    if ( pPivot == NULL )
    {
        pFanin->pFanPivot = pFanout;
        return;
    }

    if ( Fpga_Regular(pPivot->p1) == pFanin )
    {
        if ( Fpga_Regular(pFanout->p1) == pFanin )
        {
            pFanout->pFanFanin1 = pPivot->pFanFanin1;
            pPivot->pFanFanin1  = pFanout;
        }
        else // if ( Fpga_Regular(pFanout->p2) == pFanin )
        {
            pFanout->pFanFanin2 = pPivot->pFanFanin1;
            pPivot->pFanFanin1  = pFanout;
        }
    }
    else // if ( Fpga_Regular(pPivot->p2) == pFanin )
    {
        assert( Fpga_Regular(pPivot->p2) == pFanin );
        if ( Fpga_Regular(pFanout->p1) == pFanin )
        {
            pFanout->pFanFanin1 = pPivot->pFanFanin2;
            pPivot->pFanFanin2  = pFanout;
        }
        else // if ( Fpga_Regular(pFanout->p2) == pFanin )
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
void Fpga_NodeRemoveFaninFanout( Fpga_Node_t * pFanin, Fpga_Node_t * pFanoutToRemove )
{
    Fpga_Node_t * pFanout, * pFanout2, ** ppFanList;
    // start the linked list of fanouts
    ppFanList = &pFanin->pFanPivot; 
    // go through the fanouts
    Fpga_NodeForEachFanoutSafe( pFanin, pFanout, pFanout2 )
    {
        // skip the fanout-to-remove
        if ( pFanout == pFanoutToRemove )
            continue;
        // add useful fanouts to the list
        *ppFanList = pFanout;
        ppFanList = Fpga_NodeReadNextFanoutPlace( pFanin, pFanout );
    }
    *ppFanList = NULL;
}

/**Function*************************************************************

  Synopsis    [Returns the number of fanouts of a node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_NodeGetFanoutNum( Fpga_Node_t * pNode )
{
    Fpga_Node_t * pFanout;
    int Counter = 0;
    Fpga_NodeForEachFanout( pNode, pFanout )
        Counter++;
    return Counter;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif

ABC_NAMESPACE_IMPL_END

