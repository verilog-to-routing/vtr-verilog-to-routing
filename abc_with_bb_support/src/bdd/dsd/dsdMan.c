/**CFile****************************************************************

  FileName    [dsdMan.c]

  PackageName [DSD: Disjoint-support decomposition package.]

  Synopsis    [APIs of the DSD manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 8.0. Started - September 22, 2003.]

  Revision    [$Id: dsdMan.c,v 1.0 2002/22/09 00:00:00 alanmi Exp $]

***********************************************************************/

#include "dsdInt.h"

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       API OF DSD MANAGER                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts the DSD manager.]

  Description [Takes the started BDD manager and the maximum support size
  of the function to be DSD-decomposed. The manager should have at least as
  many variables as there are variables in the support. The functions should 
  be expressed using the first nSuppSizeMax variables in the manager (these
  may be ordered not necessarily on top of the manager).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dsd_Manager_t * Dsd_ManagerStart( DdManager * dd, int nSuppMax, int fVerbose )
{
	Dsd_Manager_t * dMan;
	Dsd_Node_t * pNode;
	int i;

	assert( nSuppMax <= dd->size );

	dMan = ALLOC( Dsd_Manager_t, 1 );
	memset( dMan, 0, sizeof(Dsd_Manager_t) );
	dMan->dd          = dd;
	dMan->nInputs     = nSuppMax;
	dMan->fVerbose    = fVerbose;
	dMan->nRoots      = 0;
	dMan->nRootsAlloc = 50;
	dMan->pRoots      = (Dsd_Node_t **) malloc( dMan->nRootsAlloc * sizeof(Dsd_Node_t *) );
	dMan->pInputs     = (Dsd_Node_t **) malloc( dMan->nInputs     * sizeof(Dsd_Node_t *) );

	// create the primary inputs and insert them into the table
	dMan->Table       = st_init_table(st_ptrcmp, st_ptrhash);
	for ( i = 0; i < dMan->nInputs; i++ )
	{
		pNode = Dsd_TreeNodeCreate( DSD_NODE_BUF, 1, 0 );
		pNode->G = dd->vars[i];  Cudd_Ref( pNode->G );
		pNode->S = dd->vars[i];  Cudd_Ref( pNode->S );
		st_insert( dMan->Table, (char*)dd->vars[i], (char*)pNode );
		dMan->pInputs[i] = pNode;
	}
	pNode = Dsd_TreeNodeCreate( DSD_NODE_CONST1, 0, 0 );
	pNode->G = b1;  Cudd_Ref( pNode->G );
	pNode->S = b1;  Cudd_Ref( pNode->S );
	st_insert( dMan->Table, (char*)b1, (char*)pNode );
    dMan->pConst1 = pNode;

	Dsd_CheckCacheAllocate( 5000 );
	return dMan;
}

/**Function*************************************************************

  Synopsis    [Stops the DSD manager.]

  Description [Stopping the DSD manager automatically derefereces and
  deallocates all the DSD nodes that were created during the life time
  of the DSD manager. As a result, the user does not need to deref or
  deallocate any DSD nodes or trees that are derived and placed in
  the manager while it exists.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dsd_ManagerStop( Dsd_Manager_t * dMan )
{
	st_generator * gen;
	Dsd_Node_t * pNode;
	DdNode * bFunc;
	// delete the nodes
	st_foreach_item( dMan->Table, gen, (char**)&bFunc, (char**)&pNode )
		Dsd_TreeNodeDelete( dMan->dd, Dsd_Regular(pNode) );
	st_free_table(dMan->Table);
	free( dMan->pInputs );
	free( dMan->pRoots );
	free( dMan );
	Dsd_CheckCacheDeallocate();
}

////////////////////////////////////////////////////////////////////////
///                           END OF FILE                            ///
////////////////////////////////////////////////////////////////////////
