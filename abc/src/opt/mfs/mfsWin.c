/**CFile****************************************************************

  FileName    [mfsWin.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [The good old minimization with complete don't-cares.]

  Synopsis    [Procedures to compute windows stretching to the PIs.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: mfsWin.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "mfsInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns 1 if the node should be a root.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_MfsComputeRootsCheck( Abc_Obj_t * pNode, int nLevelMax, int nFanoutLimit )
{
    Abc_Obj_t * pFanout;
    int i;
    // the node is the root if one of the following is true:
    // (1) the node has more than fanouts than the limit
    if ( Abc_ObjFanoutNum(pNode) > nFanoutLimit )
        return 1;
    // (2) the node has CO fanouts
    // (3) the node has fanouts above the cutoff level
    Abc_ObjForEachFanout( pNode, pFanout, i )
        if ( Abc_ObjIsCo(pFanout) || (int)pFanout->Level > nLevelMax )
            return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Recursively collects the root candidates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_MfsComputeRoots_rec( Abc_Obj_t * pNode, int nLevelMax, int nFanoutLimit, Vec_Ptr_t * vRoots )
{
    Abc_Obj_t * pFanout;
    int i;
    assert( Abc_ObjIsNode(pNode) );
    if ( Abc_NodeIsTravIdCurrent(pNode) )
        return;
    Abc_NodeSetTravIdCurrent( pNode );
    // check if the node should be the root
    if ( Abc_MfsComputeRootsCheck( pNode, nLevelMax, nFanoutLimit ) )
        Vec_PtrPush( vRoots, pNode );
    else // if not, explore its fanouts
        Abc_ObjForEachFanout( pNode, pFanout, i )
            Abc_MfsComputeRoots_rec( pFanout, nLevelMax, nFanoutLimit, vRoots );
}

/**Function*************************************************************

  Synopsis    [Recursively collects the root candidates.]

  Description [Returns 1 if the only root is this node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_MfsComputeRoots( Abc_Obj_t * pNode, int nWinTfoMax, int nFanoutLimit )
{
    Vec_Ptr_t * vRoots;
    vRoots = Vec_PtrAlloc( 10 );
    Abc_NtkIncrementTravId( pNode->pNtk );
    Abc_MfsComputeRoots_rec( pNode, pNode->Level + nWinTfoMax, nFanoutLimit, vRoots );
    assert( Vec_PtrSize(vRoots) > 0 );
//    if ( Vec_PtrSize(vRoots) == 1 && Vec_PtrEntry(vRoots, 0) == pNode )
//        return 0;
    return vRoots;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

