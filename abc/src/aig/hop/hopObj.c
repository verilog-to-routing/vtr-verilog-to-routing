/**CFile****************************************************************

  FileName    [hopObj.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Minimalistic And-Inverter Graph package.]

  Synopsis    [Adding/removing objects.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 11, 2006.]

  Revision    [$Id: hopObj.c,v 1.00 2006/05/11 00:00:00 alanmi Exp $]

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

  Synopsis    [Creates primary input.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Hop_ObjCreatePi( Hop_Man_t * p )
{
    Hop_Obj_t * pObj;
    pObj = Hop_ManFetchMemory( p );
    pObj->Type = AIG_PI;
    pObj->PioNum = Vec_PtrSize( p->vPis );
    Vec_PtrPush( p->vPis, pObj );
    p->nObjs[AIG_PI]++;
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Creates primary output with the given driver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Hop_ObjCreatePo( Hop_Man_t * p, Hop_Obj_t * pDriver )
{
    Hop_Obj_t * pObj;
    pObj = Hop_ManFetchMemory( p );
    pObj->Type = AIG_PO;
    Vec_PtrPush( p->vPos, pObj );
    // add connections
    pObj->pFanin0 = pDriver;
    if ( p->fRefCount )
        Hop_ObjRef( Hop_Regular(pDriver) );
    else
        pObj->nRefs = Hop_ObjLevel( Hop_Regular(pDriver) );
    // set the phase
    pObj->fPhase = Hop_ObjPhaseCompl(pDriver);
    // update node counters of the manager
    p->nObjs[AIG_PO]++;
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Create the new node assuming it does not exist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Hop_ObjCreate( Hop_Man_t * p, Hop_Obj_t * pGhost )
{
    Hop_Obj_t * pObj;
    assert( !Hop_IsComplement(pGhost) );
    assert( Hop_ObjIsNode(pGhost) );
    assert( pGhost == &p->Ghost );
    // get memory for the new object
    pObj = Hop_ManFetchMemory( p );
    pObj->Type = pGhost->Type;
    // add connections
    Hop_ObjConnect( p, pObj, pGhost->pFanin0, pGhost->pFanin1 );
    // update node counters of the manager
    p->nObjs[Hop_ObjType(pObj)]++;
    assert( pObj->pData == NULL );
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Connect the object to the fanin.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hop_ObjConnect( Hop_Man_t * p, Hop_Obj_t * pObj, Hop_Obj_t * pFan0, Hop_Obj_t * pFan1 )
{
    assert( !Hop_IsComplement(pObj) );
    assert( Hop_ObjIsNode(pObj) );
    // add the first fanin
    pObj->pFanin0 = pFan0;
    pObj->pFanin1 = pFan1;
    // increment references of the fanins and add their fanouts
    if ( p->fRefCount )
    {
        if ( pFan0 != NULL )
            Hop_ObjRef( Hop_ObjFanin0(pObj) );
        if ( pFan1 != NULL )
            Hop_ObjRef( Hop_ObjFanin1(pObj) );
    }
    else
        pObj->nRefs = Hop_ObjLevelNew( pObj );
    // set the phase
    pObj->fPhase = Hop_ObjPhaseCompl(pFan0) & Hop_ObjPhaseCompl(pFan1);
    // add the node to the structural hash table
    Hop_TableInsert( p, pObj );
}

/**Function*************************************************************

  Synopsis    [Connect the object to the fanin.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hop_ObjDisconnect( Hop_Man_t * p, Hop_Obj_t * pObj )
{
    assert( !Hop_IsComplement(pObj) );
    assert( Hop_ObjIsNode(pObj) );
    // remove connections
    if ( pObj->pFanin0 != NULL )
        Hop_ObjDeref(Hop_ObjFanin0(pObj));
    if ( pObj->pFanin1 != NULL )
        Hop_ObjDeref(Hop_ObjFanin1(pObj));
    // remove the node from the structural hash table
    Hop_TableDelete( p, pObj );
    // add the first fanin
    pObj->pFanin0 = NULL;
    pObj->pFanin1 = NULL;
}

/**Function*************************************************************

  Synopsis    [Deletes the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hop_ObjDelete( Hop_Man_t * p, Hop_Obj_t * pObj )
{
    assert( !Hop_IsComplement(pObj) );
    assert( !Hop_ObjIsTerm(pObj) );
    assert( Hop_ObjRefs(pObj) == 0 );
    // update node counters of the manager
    p->nObjs[pObj->Type]--;
    p->nDeleted++;
    // remove connections
    Hop_ObjDisconnect( p, pObj );
    // remove PIs/POs from the arrays
    if ( Hop_ObjIsPi(pObj) )
        Vec_PtrRemove( p->vPis, pObj );
    // free the node
    Hop_ManRecycleMemory( p, pObj );
}

/**Function*************************************************************

  Synopsis    [Deletes the MFFC of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hop_ObjDelete_rec( Hop_Man_t * p, Hop_Obj_t * pObj )
{
    Hop_Obj_t * pFanin0, * pFanin1;
    assert( !Hop_IsComplement(pObj) );
    if ( Hop_ObjIsConst1(pObj) || Hop_ObjIsPi(pObj) )
        return;
    assert( Hop_ObjIsNode(pObj) );
    pFanin0 = Hop_ObjFanin0(pObj);
    pFanin1 = Hop_ObjFanin1(pObj);
    Hop_ObjDelete( p, pObj );
    if ( pFanin0 && !Hop_ObjIsNone(pFanin0) && Hop_ObjRefs(pFanin0) == 0 )
        Hop_ObjDelete_rec( p, pFanin0 );
    if ( pFanin1 && !Hop_ObjIsNone(pFanin1) && Hop_ObjRefs(pFanin1) == 0 )
        Hop_ObjDelete_rec( p, pFanin1 );
}

/**Function*************************************************************

  Synopsis    [Returns the representative of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Hop_ObjRepr( Hop_Obj_t * pObj )
{
    assert( !Hop_IsComplement(pObj) );
    if ( pObj->pData == NULL || pObj->pData == pObj )
        return pObj;
    return Hop_ObjRepr( (Hop_Obj_t *)pObj->pData );
}

/**Function*************************************************************

  Synopsis    [Sets an equivalence relation between the nodes.]

  Description [Makes the representative of pNew point to the representaive of pOld.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hop_ObjCreateChoice( Hop_Obj_t * pOld, Hop_Obj_t * pNew )
{
    Hop_Obj_t * pOldRepr;
    Hop_Obj_t * pNewRepr;
    assert( pOld != NULL && pNew != NULL );
    pOldRepr = Hop_ObjRepr(pOld);
    pNewRepr = Hop_ObjRepr(pNew);
    if ( pNewRepr != pOldRepr )
        pNewRepr->pData = pOldRepr;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

