/**CFile****************************************************************

  FileName    [nwkObj.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Logic network representation.]

  Synopsis    [Manipulation of objects.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: nwkObj.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

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

  Synopsis    [Creates an object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Nwk_Obj_t * Nwk_ManCreateObj( Nwk_Man_t * p, int nFanins, int nFanouts )
{
    Nwk_Obj_t * pObj;
    pObj = (Nwk_Obj_t *)Aig_MmFlexEntryFetch( p->pMemObjs, sizeof(Nwk_Obj_t) + (nFanins + nFanouts + p->nFanioPlus) * sizeof(Nwk_Obj_t *) );
    memset( pObj, 0, sizeof(Nwk_Obj_t) );
    pObj->pFanio = (Nwk_Obj_t **)((char *)pObj + sizeof(Nwk_Obj_t));
    pObj->Id = Vec_PtrSize( p->vObjs );
    Vec_PtrPush( p->vObjs, pObj );
    pObj->pMan        = p;
    pObj->nFanioAlloc = nFanins + nFanouts + p->nFanioPlus;
    return pObj;
}


/**Function*************************************************************

  Synopsis    [Creates a primary input.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Nwk_Obj_t * Nwk_ManCreateCi( Nwk_Man_t * p, int nFanouts )
{
    Nwk_Obj_t * pObj;
    pObj = Nwk_ManCreateObj( p, 1, nFanouts );
    pObj->PioId = Vec_PtrSize( p->vCis );
    Vec_PtrPush( p->vCis, pObj );
    pObj->Type = NWK_OBJ_CI;
    p->nObjs[NWK_OBJ_CI]++;
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Creates a primary output.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Nwk_Obj_t * Nwk_ManCreateCo( Nwk_Man_t * p )
{
    Nwk_Obj_t * pObj;
    pObj = Nwk_ManCreateObj( p, 1, 1 );
    pObj->PioId = Vec_PtrSize( p->vCos );
    Vec_PtrPush( p->vCos, pObj );
    pObj->Type = NWK_OBJ_CO;
    p->nObjs[NWK_OBJ_CO]++;
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Creates a latch.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Nwk_Obj_t * Nwk_ManCreateLatch( Nwk_Man_t * p )
{
    Nwk_Obj_t * pObj;
    pObj = Nwk_ManCreateObj( p, 1, 1 );
    pObj->Type = NWK_OBJ_LATCH;
    p->nObjs[NWK_OBJ_LATCH]++;
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Creates a node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Nwk_Obj_t * Nwk_ManCreateNode( Nwk_Man_t * p, int nFanins, int nFanouts )
{
    Nwk_Obj_t * pObj;
    pObj = Nwk_ManCreateObj( p, nFanins, nFanouts );
    pObj->Type = NWK_OBJ_NODE;
    p->nObjs[NWK_OBJ_NODE]++;
    return pObj;
}


/**Function*************************************************************

  Synopsis    [Deletes the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManDeleteNode( Nwk_Obj_t * pObj )
{
    Vec_Ptr_t * vNodes = pObj->pMan->vTemp;
    Nwk_Obj_t * pTemp;
    int i;
    assert( Nwk_ObjFanoutNum(pObj) == 0 );
    // delete fanins
    Nwk_ObjCollectFanins( pObj, vNodes );
    Vec_PtrForEachEntry( Nwk_Obj_t *, vNodes, pTemp, i )
        Nwk_ObjDeleteFanin( pObj, pTemp );
    // remove from the list of objects
    Vec_PtrWriteEntry( pObj->pMan->vObjs, pObj->Id, NULL );
    pObj->pMan->nObjs[pObj->Type]--;
    memset( pObj, 0, sizeof(Nwk_Obj_t) );
    pObj->Id = -1;
}

/**Function*************************************************************

  Synopsis    [Deletes the node and MFFC of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManDeleteNode_rec( Nwk_Obj_t * pObj )
{
    Vec_Ptr_t * vNodes;
    int i;
    assert( !Nwk_ObjIsCi(pObj) );
    assert( Nwk_ObjFanoutNum(pObj) == 0 );
    vNodes = Vec_PtrAlloc( 100 );
    Nwk_ObjCollectFanins( pObj, vNodes );
    Nwk_ManDeleteNode( pObj );
    Vec_PtrForEachEntry( Nwk_Obj_t *, vNodes, pObj, i )
        if ( Nwk_ObjIsNode(pObj) && Nwk_ObjFanoutNum(pObj) == 0 )
            Nwk_ManDeleteNode_rec( pObj );
    Vec_PtrFree( vNodes );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

