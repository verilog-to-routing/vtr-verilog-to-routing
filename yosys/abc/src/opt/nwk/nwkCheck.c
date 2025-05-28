/**CFile****************************************************************

  FileName    [nwkCheck.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Logic network representation.]

  Synopsis    [Consistency checking procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: nwkCheck.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "nwk.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Checking the logic network for consistency.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nwk_ManCheck( Nwk_Man_t * p )
{
    Nwk_Obj_t * pObj, * pNext;
    int i, k, m;
    // check if the nodes have duplicated fanins
    Nwk_ManForEachNode( p, pObj, i )
    {
        for ( k = 0; k < pObj->nFanins; k++ )
            for ( m = k + 1; m < pObj->nFanins; m++ )
                if ( pObj->pFanio[k] == pObj->pFanio[m] )
                    printf( "Node %d has duplicated fanin %d.\n", pObj->Id, pObj->pFanio[k]->Id );
    }
    // check if all nodes are in the correct fanin/fanout relationship
    Nwk_ManForEachObj( p, pObj, i )
    {
        Nwk_ObjForEachFanin( pObj, pNext, k )
            if ( Nwk_ObjFanoutNum(pNext) < 100 && Nwk_ObjFindFanout( pNext, pObj ) == -1 )
                printf( "Nwk_ManCheck(): Object %d has fanin %d which does not have a corresponding fanout.\n", pObj->Id, pNext->Id );
        Nwk_ObjForEachFanout( pObj, pNext, k )
            if ( Nwk_ObjFindFanin( pNext, pObj ) == -1 )
                printf( "Nwk_ManCheck(): Object %d has fanout %d which does not have a corresponding fanin.\n", pObj->Id, pNext->Id );
    }
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

