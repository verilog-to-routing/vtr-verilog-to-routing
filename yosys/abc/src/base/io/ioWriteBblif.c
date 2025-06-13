/**CFile****************************************************************

  FileName    [ioWriteBblif.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedures to write AIG in the binary format.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: ioWriteBblif.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ioAbc.h"
#include "misc/bbl/bblif.h"

ABC_NAMESPACE_IMPL_START


// For description of Binary BLIF format, refer to "abc/src/aig/bbl/bblif.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Fnction*************************************************************

  Synopsis    [Construct manager from the ABC network.]

  Description [In the ABC network each object has a unique integer ID.
  This ID is used when we construct objects of the BBLIF manager 
  corresponding to each object of the ABC network. The objects can be
  added to the manager in any order (although below they are added in the
  topological order), but by the time fanin/fanout connections are created, 
  corresponding objects are already constructed. In the end the checking
  procedure is called.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Bbl_Man_t * Bbl_ManFromAbc( Abc_Ntk_t * pNtk )
{
    Bbl_Man_t * p;
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj, * pFanin;
    int i, k;
    assert( Abc_NtkIsSopLogic(pNtk) );
    // start the data manager
    p = Bbl_ManStart( Abc_NtkName(pNtk) );
    // collect internal nodes to be added
    vNodes = Abc_NtkDfs( pNtk, 0 );
    // create combinational inputs
    Abc_NtkForEachCi( pNtk, pObj, i )
        Bbl_ManCreateObject( p, BBL_OBJ_CI, Abc_ObjId(pObj), 0, NULL );
    // create internal nodes 
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
        Bbl_ManCreateObject( p, BBL_OBJ_NODE, Abc_ObjId(pObj), Abc_ObjFaninNum(pObj), (char *)pObj->pData );
    // create combinational outputs
    Abc_NtkForEachCo( pNtk, pObj, i )
        Bbl_ManCreateObject( p, BBL_OBJ_CO, Abc_ObjId(pObj), 1, NULL );
    // create fanin/fanout connections for internal nodes
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
        Abc_ObjForEachFanin( pObj, pFanin, k )
            Bbl_ManAddFanin( p, Abc_ObjId(pObj), Abc_ObjId(pFanin) );
    // create fanin/fanout connections for combinational outputs
    Abc_NtkForEachCo( pNtk, pObj, i )
        Abc_ObjForEachFanin( pObj, pFanin, k )
            Bbl_ManAddFanin( p, Abc_ObjId(pObj), Abc_ObjId(pFanin) );
    Vec_PtrFree( vNodes );
    // sanity check
    Bbl_ManCheck( p );
    return p;
}

/**Function*************************************************************

  Synopsis    [Writes the AIG in the binary format.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteBblif( Abc_Ntk_t * pNtk, char * pFileName )
{
    Bbl_Man_t * p;
    p = Bbl_ManFromAbc( pNtk );
//Bbl_ManPrintStats( p );
//Bbl_ManDumpBlif( p, "test_bbl.blif" );
    Bbl_ManDumpBinaryBlif( p, pFileName );
    Bbl_ManStop( p );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

