/**CFile****************************************************************

  FileName    [abcReorder.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Reordering local BDDs of the nodes.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcReorder.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"

#ifdef ABC_USE_CUDD
#include "bdd/reo/reo.h"
#endif

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

#ifdef ABC_USE_CUDD

/**Function*************************************************************

  Synopsis    [Reorders BDD of the local function of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeBddReorder( reo_man * p, Abc_Obj_t * pNode )
{
    Abc_Obj_t * pFanin;
    DdNode * bFunc;
    int * pOrder, i;
    // create the temporary array for the variable order
    pOrder = ABC_ALLOC( int, Abc_ObjFaninNum(pNode) );
    for ( i = 0; i < Abc_ObjFaninNum(pNode); i++ )
        pOrder[i] = -1;
    // reorder the BDD
    bFunc = Extra_Reorder( p, (DdManager *)pNode->pNtk->pManFunc, (DdNode *)pNode->pData, pOrder ); Cudd_Ref( bFunc );
    Cudd_RecursiveDeref( (DdManager *)pNode->pNtk->pManFunc, (DdNode *)pNode->pData );
    pNode->pData = bFunc;
    // update the fanin order
    Abc_ObjForEachFanin( pNode, pFanin, i )
        pOrder[i] = pNode->vFanins.pArray[ pOrder[i] ];
    Abc_ObjForEachFanin( pNode, pFanin, i )
        pNode->vFanins.pArray[i] = pOrder[i];
    ABC_FREE( pOrder );
}

/**Function*************************************************************

  Synopsis    [Reorders BDDs of the local functions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkBddReorder( Abc_Ntk_t * pNtk, int fVerbose )
{
	reo_man * p;
    Abc_Obj_t * pNode;
    int i;
    Abc_NtkRemoveDupFanins( pNtk );
    Abc_NtkMinimumBase( pNtk );
    p = Extra_ReorderInit( Abc_NtkGetFaninMax(pNtk), 100 );
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        if ( Abc_ObjFaninNum(pNode) < 3 )
            continue;
        if ( fVerbose )
            fprintf( stdout, "%10s: ", Abc_ObjName(pNode) );
        if ( fVerbose )
            fprintf( stdout, "Before = %5d  BDD nodes.  ", Cudd_DagSize((DdNode *)pNode->pData) );
        Abc_NodeBddReorder( p, pNode );
        if ( fVerbose )
            fprintf( stdout, "After = %5d  BDD nodes.\n", Cudd_DagSize((DdNode *)pNode->pData) );
    }
    Extra_ReorderQuit( p );
}

#else

void Abc_NtkBddReorder( Abc_Ntk_t * pNtk, int fVerbose ) {}

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

