/**CFile****************************************************************

  FileName    [nwkFanio.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Logic network representation.]

  Synopsis    [Manipulation of fanins/fanouts.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: nwkFanio.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

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

  Synopsis    [Collects fanins of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ObjCollectFanins( Nwk_Obj_t * pNode, Vec_Ptr_t * vNodes )
{
    Nwk_Obj_t * pFanin;
    int i;
    Vec_PtrClear(vNodes);
    Nwk_ObjForEachFanin( pNode, pFanin, i )
        Vec_PtrPush( vNodes, pFanin );
}

/**Function*************************************************************

  Synopsis    [Collects fanouts of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ObjCollectFanouts( Nwk_Obj_t * pNode, Vec_Ptr_t * vNodes )
{
    Nwk_Obj_t * pFanout;
    int i;
    Vec_PtrClear(vNodes);
    Nwk_ObjForEachFanout( pNode, pFanout, i )
        Vec_PtrPush( vNodes, pFanout );
}

/**Function*************************************************************

  Synopsis    [Returns the number of the fanin of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nwk_ObjFindFanin( Nwk_Obj_t * pObj, Nwk_Obj_t * pFanin )
{  
    Nwk_Obj_t * pTemp;
    int i;
    Nwk_ObjForEachFanin( pObj, pTemp, i )
        if ( pTemp == pFanin )
            return i;
    return -1;
}

/**Function*************************************************************

  Synopsis    [Returns the number of the fanout of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nwk_ObjFindFanout( Nwk_Obj_t * pObj, Nwk_Obj_t * pFanout )
{  
    Nwk_Obj_t * pTemp;
    int i;
    Nwk_ObjForEachFanout( pObj, pTemp, i )
        if ( pTemp == pFanout )
            return i;
    return -1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the node has to be reallocated.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Nwk_ObjReallocIsNeeded( Nwk_Obj_t * pObj )
{  
    return pObj->nFanins + pObj->nFanouts == pObj->nFanioAlloc;
}
 
/**Function*************************************************************

  Synopsis    [Reallocates the object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static Nwk_Obj_t * Nwk_ManReallocNode( Nwk_Obj_t * pObj )
{  
    Nwk_Obj_t ** pFanioOld = pObj->pFanio;
    assert( Nwk_ObjReallocIsNeeded(pObj) );
    pObj->pFanio = (Nwk_Obj_t **)Aig_MmFlexEntryFetch( pObj->pMan->pMemObjs, 2 * pObj->nFanioAlloc * sizeof(Nwk_Obj_t *) );
    memmove( pObj->pFanio, pFanioOld, pObj->nFanioAlloc * sizeof(Nwk_Obj_t *) );
    pObj->nFanioAlloc *= 2;
    pObj->pMan->nRealloced++;
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Creates fanout/fanin relationship between the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ObjAddFanin( Nwk_Obj_t * pObj, Nwk_Obj_t * pFanin )
{
    int i;
    assert( pObj->pMan == pFanin->pMan );
    assert( pObj->Id >= 0 && pFanin->Id >= 0 );
    if ( Nwk_ObjReallocIsNeeded(pObj) )
        Nwk_ManReallocNode( pObj );
    if ( Nwk_ObjReallocIsNeeded(pFanin) )
        Nwk_ManReallocNode( pFanin );
    for ( i = pObj->nFanins + pObj->nFanouts; i > pObj->nFanins; i-- )
        pObj->pFanio[i] = pObj->pFanio[i-1];
    pObj->pFanio[pObj->nFanins++] = pFanin;
    pFanin->pFanio[pFanin->nFanins + pFanin->nFanouts++] = pObj;
    pObj->Level = Abc_MaxInt( pObj->Level, pFanin->Level + Nwk_ObjIsNode(pObj) );
}

/**Function*************************************************************

  Synopsis    [Removes fanout/fanin relationship between the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ObjDeleteFanin( Nwk_Obj_t * pObj, Nwk_Obj_t * pFanin )
{
    int i, k, Limit, fFound;
    // remove pFanin from the fanin list of pObj
    Limit = pObj->nFanins + pObj->nFanouts;
    fFound = 0;
    for ( k = i = 0; i < Limit; i++ )
        if ( fFound || pObj->pFanio[i] != pFanin )
            pObj->pFanio[k++] = pObj->pFanio[i];
        else
            fFound = 1;
    assert( i == k + 1 ); // if it fails, likely because of duplicated fanin
    pObj->nFanins--;
    // remove pObj from the fanout list of pFanin
    Limit = pFanin->nFanins + pFanin->nFanouts;
    fFound = 0;
    for ( k = i = pFanin->nFanins; i < Limit; i++ )
        if ( fFound || pFanin->pFanio[i] != pObj )
            pFanin->pFanio[k++] = pFanin->pFanio[i];
        else
            fFound = 1;
    assert( i == k + 1 ); // if it fails, likely because of duplicated fanout
    pFanin->nFanouts--; 
}

/**Function*************************************************************

  Synopsis    [Replaces a fanin of the node.]

  Description [The node is pObj. An old fanin of this node (pFaninOld) has to be
  replaced by a new fanin (pFaninNew). Assumes that the node and the old fanin 
  are not complemented. The new fanin can be complemented. In this case, the
  polarity of the new fanin will change, compared to the polarity of the old fanin.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ObjPatchFanin( Nwk_Obj_t * pObj, Nwk_Obj_t * pFaninOld, Nwk_Obj_t * pFaninNew )
{
    int i, k, iFanin, Limit;
    assert( pFaninOld != pFaninNew );
    assert( pObj != pFaninOld );
    assert( pObj != pFaninNew );
    assert( pObj->pMan == pFaninOld->pMan );
    assert( pObj->pMan == pFaninNew->pMan );
    // update the fanin
    iFanin = Nwk_ObjFindFanin( pObj, pFaninOld );
    if ( iFanin == -1 )
    {
        printf( "Nwk_ObjPatchFanin(); Error! Node %d is not among", pFaninOld->Id );
        printf( " the fanins of node %d...\n", pObj->Id );
        return;
    }
    pObj->pFanio[iFanin] = pFaninNew;
    // remove pObj from the fanout list of pFaninOld
    Limit = pFaninOld->nFanins + pFaninOld->nFanouts;
    for ( k = i = pFaninOld->nFanins; i < Limit; i++ )
        if ( pFaninOld->pFanio[i] != pObj )
            pFaninOld->pFanio[k++] = pFaninOld->pFanio[i];
    pFaninOld->nFanouts--;
    // add pObj to the fanout list of pFaninNew
    if ( Nwk_ObjReallocIsNeeded(pFaninNew) )
        Nwk_ManReallocNode( pFaninNew );
    pFaninNew->pFanio[pFaninNew->nFanins + pFaninNew->nFanouts++] = pObj;
}


/**Function*************************************************************

  Synopsis    [Transfers fanout from the old node to the new node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ObjTransferFanout( Nwk_Obj_t * pNodeFrom, Nwk_Obj_t * pNodeTo )
{
    Vec_Ptr_t * vFanouts = pNodeFrom->pMan->vTemp;
    Nwk_Obj_t * pTemp;
    int nFanoutsOld, i;
    assert( !Nwk_ObjIsCo(pNodeFrom) && !Nwk_ObjIsCo(pNodeTo) );
    assert( pNodeFrom->pMan == pNodeTo->pMan );
    assert( pNodeFrom != pNodeTo );
    assert( Nwk_ObjFanoutNum(pNodeFrom) > 0 );
    // get the fanouts of the old node
    nFanoutsOld = Nwk_ObjFanoutNum(pNodeTo);
    Nwk_ObjCollectFanouts( pNodeFrom, vFanouts );
    // patch the fanin of each of them
    Vec_PtrForEachEntry( Nwk_Obj_t *, vFanouts, pTemp, i )
        Nwk_ObjPatchFanin( pTemp, pNodeFrom, pNodeTo );
    assert( Nwk_ObjFanoutNum(pNodeFrom) == 0 );
    assert( Nwk_ObjFanoutNum(pNodeTo) == nFanoutsOld + Vec_PtrSize(vFanouts) );
}

/**Function*************************************************************

  Synopsis    [Replaces the node by a new node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ObjReplace( Nwk_Obj_t * pNodeOld, Nwk_Obj_t * pNodeNew )
{
    assert( pNodeOld->pMan == pNodeNew->pMan );
    assert( pNodeOld != pNodeNew );
    assert( Nwk_ObjFanoutNum(pNodeOld) > 0 );
    // transfer the fanouts to the old node
    Nwk_ObjTransferFanout( pNodeOld, pNodeNew );
    // remove the old node
    Nwk_ManDeleteNode_rec( pNodeOld );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

