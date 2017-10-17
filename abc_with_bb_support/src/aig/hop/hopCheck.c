/**CFile****************************************************************

  FileName    [hopCheck.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Minimalistic And-Inverter Graph package.]

  Synopsis    [AIG checking procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 11, 2006.]

  Revision    [$Id: hopCheck.c,v 1.00 2006/05/11 00:00:00 alanmi Exp $]

***********************************************************************/

#include "hop.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Checks the consistency of the AIG manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Hop_ManCheck( Hop_Man_t * p )
{
    Hop_Obj_t * pObj, * pObj2;
    int i;
    // check primary inputs
    Hop_ManForEachPi( p, pObj, i )
    {
        if ( Hop_ObjFanin0(pObj) || Hop_ObjFanin1(pObj) )
        {
            printf( "Hop_ManCheck: The PI node \"%p\" has fanins.\n", pObj );
            return 0;
        }
    }
    // check primary outputs
    Hop_ManForEachPo( p, pObj, i )
    {
        if ( !Hop_ObjFanin0(pObj) )
        {
            printf( "Hop_ManCheck: The PO node \"%p\" has NULL fanin.\n", pObj );
            return 0;
        }
        if ( Hop_ObjFanin1(pObj) )
        {
            printf( "Hop_ManCheck: The PO node \"%p\" has second fanin.\n", pObj );
            return 0;
        }
    }
    // check internal nodes
    Hop_ManForEachNode( p, pObj, i )
    {
        if ( !Hop_ObjFanin0(pObj) || !Hop_ObjFanin1(pObj) )
        {
            printf( "Hop_ManCheck: The AIG has internal node \"%p\" with a NULL fanin.\n", pObj );
            return 0;
        }
        if ( Hop_ObjFanin0(pObj)->Id >= Hop_ObjFanin1(pObj)->Id )
        {
            printf( "Hop_ManCheck: The AIG has node \"%p\" with a wrong ordering of fanins.\n", pObj );
            return 0;
        }
        pObj2 = Hop_TableLookup( p, pObj );
        if ( pObj2 != pObj )
        {
            printf( "Hop_ManCheck: Node \"%p\" is not in the structural hashing table.\n", pObj );
            return 0;
        }
    }
    // count the total number of nodes
    if ( Hop_ManObjNum(p) != 1 + Hop_ManPiNum(p) + Hop_ManPoNum(p) + Hop_ManAndNum(p) + Hop_ManExorNum(p) )
    {
        printf( "Hop_ManCheck: The number of created nodes is wrong.\n" );
        return 0;
    }
    // count the number of nodes in the table
    if ( Hop_TableCountEntries(p) != Hop_ManAndNum(p) + Hop_ManExorNum(p) )
    {
        printf( "Hop_ManCheck: The number of nodes in the structural hashing table is wrong.\n" );
        return 0;
    }
//    if ( !Hop_ManIsAcyclic(p) )
//        return 0;
    return 1; 
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

