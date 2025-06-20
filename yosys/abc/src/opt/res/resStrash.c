/**CFile****************************************************************

  FileName    [resStrash.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Resynthesis package.]

  Synopsis    [Structural hashing of the nodes in the window.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 15, 2007.]

  Revision    [$Id: resStrash.c,v 1.00 2007/01/15 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "resInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern Abc_Obj_t * Abc_ConvertAigToAig( Abc_Ntk_t * pAig, Abc_Obj_t * pObjOld );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Structurally hashes the given window.]

  Description [The first PO is the observability condition. The second 
  is the node's function. The remaining POs are the candidate divisors.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Res_WndStrash( Res_Win_t * p )
{
    Vec_Ptr_t * vPairs;
    Abc_Ntk_t * pAig;
    Abc_Obj_t * pObj, * pMiter;
    int i;
    assert( Abc_NtkHasAig(p->pNode->pNtk) );
//    Abc_NtkCleanCopy( p->pNode->pNtk );
    // create the network
    pAig = Abc_NtkAlloc( ABC_NTK_STRASH, ABC_FUNC_AIG, 1 );
    pAig->pName = Extra_UtilStrsav( "window" );
    // create the inputs
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vLeaves, pObj, i )
        pObj->pCopy = Abc_NtkCreatePi( pAig );
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vBranches, pObj, i )
        pObj->pCopy = Abc_NtkCreatePi( pAig );
    // go through the nodes in the topological order
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vNodes, pObj, i )
    {
        pObj->pCopy = Abc_ConvertAigToAig( pAig, pObj );
        if ( pObj == p->pNode )
            pObj->pCopy = Abc_ObjNot( pObj->pCopy );
    }
    // collect the POs
    vPairs = Vec_PtrAlloc( 2 * Vec_PtrSize(p->vRoots) );
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vRoots, pObj, i )
    {
        Vec_PtrPush( vPairs, pObj->pCopy );
        Vec_PtrPush( vPairs, NULL );
    }
    // mark the TFO of the node
    Abc_NtkIncrementTravId( p->pNode->pNtk );
    Res_WinSweepLeafTfo_rec( p->pNode, (int)p->pNode->Level + p->nWinTfoMax );
    // update strashing of the node
    p->pNode->pCopy = Abc_ObjNot( p->pNode->pCopy );
    Abc_NodeSetTravIdPrevious( p->pNode );
    // redo strashing in the TFO
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vNodes, pObj, i )
    {
        if ( Abc_NodeIsTravIdCurrent(pObj) )
            pObj->pCopy = Abc_ConvertAigToAig( pAig, pObj );
    }
    // collect the POs
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vRoots, pObj, i )
        Vec_PtrWriteEntry( vPairs, 2 * i + 1, pObj->pCopy );
    // add the miter
    pMiter = Abc_AigMiter( (Abc_Aig_t *)pAig->pManFunc, vPairs, 0 );
    Abc_ObjAddFanin( Abc_NtkCreatePo(pAig), pMiter );
    Vec_PtrFree( vPairs );
    // add the node
    Abc_ObjAddFanin( Abc_NtkCreatePo(pAig), p->pNode->pCopy );
    // add the fanins
    Abc_ObjForEachFanin( p->pNode, pObj, i )
        Abc_ObjAddFanin( Abc_NtkCreatePo(pAig), pObj->pCopy );
    // add the divisors
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vDivs, pObj, i )
        Abc_ObjAddFanin( Abc_NtkCreatePo(pAig), pObj->pCopy );
    // add the names
    Abc_NtkAddDummyPiNames( pAig );
    Abc_NtkAddDummyPoNames( pAig );
    // check the resulting network
    if ( !Abc_NtkCheck( pAig ) )
        fprintf( stdout, "Res_WndStrash(): Network check has failed.\n" );
    return pAig;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

