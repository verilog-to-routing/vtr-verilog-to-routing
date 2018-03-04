/**CFile****************************************************************

  FileName    [aigMffc.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [Computation of MFFCs.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aigMffc.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Dereferences the node's MFFC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_NodeDeref_rec( Aig_Obj_t * pNode, unsigned LevelMin, float * pPower, float * pProbs )
{
    float Power0 = 0.0, Power1 = 0.0;
    Aig_Obj_t * pFanin;
    int Counter = 0;
    if ( pProbs )
        *pPower = 0.0;
    if ( Aig_ObjIsCi(pNode) )
        return 0;
    // consider the first fanin
    pFanin = Aig_ObjFanin0(pNode);
    assert( pFanin->nRefs > 0 );
    if ( --pFanin->nRefs == 0 && (!LevelMin || pFanin->Level > LevelMin) )
        Counter += Aig_NodeDeref_rec( pFanin, LevelMin, &Power0, pProbs );
    if ( pProbs )
        *pPower += Power0 + 2.0 * pProbs[pFanin->Id] * (1.0 - pProbs[pFanin->Id]);
    // skip the buffer
    if ( Aig_ObjIsBuf(pNode) )
        return Counter;
    assert( Aig_ObjIsNode(pNode) );
    // consider the second fanin
    pFanin = Aig_ObjFanin1(pNode);
    assert( pFanin->nRefs > 0 );
    if ( --pFanin->nRefs == 0 && (!LevelMin || pFanin->Level > LevelMin) )
        Counter += Aig_NodeDeref_rec( pFanin, LevelMin, &Power1, pProbs );
    if ( pProbs )
        *pPower += Power1 + 2.0 * pProbs[pFanin->Id] * (1.0 - pProbs[pFanin->Id]);
    return Counter + 1;
}

/**Function*************************************************************

  Synopsis    [References the node's MFFC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_NodeRef_rec( Aig_Obj_t * pNode, unsigned LevelMin )
{
    Aig_Obj_t * pFanin;
    int Counter = 0;
    if ( Aig_ObjIsCi(pNode) )
        return 0;
    // consider the first fanin
    pFanin = Aig_ObjFanin0(pNode);
    if ( pFanin->nRefs++ == 0 && (!LevelMin || pFanin->Level > LevelMin) )
        Counter += Aig_NodeRef_rec( pFanin, LevelMin );
    // skip the buffer
    if ( Aig_ObjIsBuf(pNode) )
        return Counter;
    assert( Aig_ObjIsNode(pNode) );
    // consider the second fanin
    pFanin = Aig_ObjFanin1(pNode);
    if ( pFanin->nRefs++ == 0 && (!LevelMin || pFanin->Level > LevelMin) )
        Counter += Aig_NodeRef_rec( pFanin, LevelMin );
    return Counter + 1;
}

/**Function*************************************************************

  Synopsis    [References the node's MFFC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_NodeRefLabel_rec( Aig_Man_t * p, Aig_Obj_t * pNode, unsigned LevelMin )
{
    Aig_Obj_t * pFanin;
    int Counter = 0;
    if ( Aig_ObjIsCi(pNode) )
        return 0;
    Aig_ObjSetTravIdCurrent( p, pNode );
    // consider the first fanin
    pFanin = Aig_ObjFanin0(pNode);
    if ( pFanin->nRefs++ == 0 && (!LevelMin || pFanin->Level > LevelMin) )
        Counter += Aig_NodeRefLabel_rec( p, pFanin, LevelMin );
    if ( Aig_ObjIsBuf(pNode) )
        return Counter;
    assert( Aig_ObjIsNode(pNode) );
    // consider the second fanin
    pFanin = Aig_ObjFanin1(pNode);
    if ( pFanin->nRefs++ == 0 && (!LevelMin || pFanin->Level > LevelMin) )
        Counter += Aig_NodeRefLabel_rec( p, pFanin, LevelMin );
    return Counter + 1;
}

/**Function*************************************************************

  Synopsis    [Collects the internal and boundary nodes in the derefed MFFC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_NodeMffcSupp_rec( Aig_Man_t * p, Aig_Obj_t * pNode, unsigned LevelMin, Vec_Ptr_t * vSupp, int fTopmost, Aig_Obj_t * pObjSkip )
{
    // skip visited nodes
    if ( Aig_ObjIsTravIdCurrent(p, pNode) )
        return;
    Aig_ObjSetTravIdCurrent(p, pNode);
    // add to the new support nodes
    if ( !fTopmost && pNode != pObjSkip && (Aig_ObjIsCi(pNode) || pNode->nRefs > 0 || pNode->Level <= LevelMin) )
    {
        if ( vSupp ) Vec_PtrPush( vSupp, pNode );
        return;
    }
    assert( Aig_ObjIsNode(pNode) );
    // recur on the children
    Aig_NodeMffcSupp_rec( p, Aig_ObjFanin0(pNode), LevelMin, vSupp, 0, pObjSkip );
    Aig_NodeMffcSupp_rec( p, Aig_ObjFanin1(pNode), LevelMin, vSupp, 0, pObjSkip );
}

/**Function*************************************************************

  Synopsis    [Collects the support of depth-limited MFFC.]

  Description [Returns the number of internal nodes in the MFFC.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_NodeMffcSupp( Aig_Man_t * p, Aig_Obj_t * pNode, int LevelMin, Vec_Ptr_t * vSupp )
{
    int ConeSize1, ConeSize2;
    if ( vSupp ) Vec_PtrClear( vSupp );
    if ( !Aig_ObjIsNode(pNode) )
    {
        if ( Aig_ObjIsCi(pNode) && vSupp )
            Vec_PtrPush( vSupp, pNode );
        return 0;
    }
    assert( !Aig_IsComplement(pNode) );
    assert( Aig_ObjIsNode(pNode) );
    Aig_ManIncrementTravId( p );
    ConeSize1 = Aig_NodeDeref_rec( pNode, LevelMin, NULL, NULL );
    Aig_NodeMffcSupp_rec( p, pNode, LevelMin, vSupp, 1, NULL );
    ConeSize2 = Aig_NodeRef_rec( pNode, LevelMin );
    assert( ConeSize1 == ConeSize2 );
    assert( ConeSize1 > 0 );
    return ConeSize1;
}

/**Function*************************************************************

  Synopsis    [Labels the nodes in the MFFC.]

  Description [Returns the number of internal nodes in the MFFC.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_NodeMffcLabel( Aig_Man_t * p, Aig_Obj_t * pNode, float * pPower )
{
    int ConeSize1, ConeSize2;
    assert( (pPower != NULL) == (p->vProbs != NULL) );
    assert( !Aig_IsComplement(pNode) );
    assert( Aig_ObjIsNode(pNode) );
    Aig_ManIncrementTravId( p );
    ConeSize1 = Aig_NodeDeref_rec( pNode, 0, pPower, p->vProbs? (float *)p->vProbs->pArray : NULL );
    ConeSize2 = Aig_NodeRefLabel_rec( p, pNode, 0 );
    assert( ConeSize1 == ConeSize2 );
    assert( ConeSize1 > 0 );
    return ConeSize1;
}

/**Function*************************************************************

  Synopsis    [Labels the nodes in the MFFC.]

  Description [Returns the number of internal nodes in the MFFC.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_NodeMffcLabelCut( Aig_Man_t * p, Aig_Obj_t * pNode, Vec_Ptr_t * vLeaves )
{
    Aig_Obj_t * pObj;
    int i, ConeSize1, ConeSize2;
    assert( !Aig_IsComplement(pNode) );
    assert( Aig_ObjIsNode(pNode) );
    Aig_ManIncrementTravId( p );
    Vec_PtrForEachEntry( Aig_Obj_t *, vLeaves, pObj, i )
        pObj->nRefs++;
    ConeSize1 = Aig_NodeDeref_rec( pNode, 0, NULL, NULL );
    ConeSize2 = Aig_NodeRefLabel_rec( p, pNode, 0 );
    Vec_PtrForEachEntry( Aig_Obj_t *, vLeaves, pObj, i )
        pObj->nRefs--;
    assert( ConeSize1 == ConeSize2 );
    assert( ConeSize1 > 0 );
    return ConeSize1;
}

/**Function*************************************************************

  Synopsis    [Expands the cut by adding the most closely related node.]

  Description [Returns 1 if the cut exists.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_NodeMffcExtendCut( Aig_Man_t * p, Aig_Obj_t * pNode, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vResult )
{
    Aig_Obj_t * pObj, * pLeafBest;
    int i, LevelMax, ConeSize1, ConeSize2, ConeCur1, ConeCur2, ConeBest;
    // dereference the current cut
    LevelMax = 0;
    Vec_PtrForEachEntry( Aig_Obj_t *, vLeaves, pObj, i )
        LevelMax = Abc_MaxInt( LevelMax, (int)pObj->Level );
    if ( LevelMax == 0 )
        return 0;
    // dereference the cut
    ConeSize1 = Aig_NodeDeref_rec( pNode, 0, NULL, NULL );
    // try expanding each node in the boundary
    ConeBest = ABC_INFINITY;
    pLeafBest = NULL;
    Vec_PtrForEachEntry( Aig_Obj_t *, vLeaves, pObj, i )
    {
        if ( (int)pObj->Level != LevelMax )
            continue;
        ConeCur1 = Aig_NodeDeref_rec( pObj, 0, NULL, NULL );
        if ( ConeBest > ConeCur1 )
        {
            ConeBest = ConeCur1;
            pLeafBest = pObj;
        }
        ConeCur2 = Aig_NodeRef_rec( pObj, 0 );
        assert( ConeCur1 == ConeCur2 );
    }
    assert( pLeafBest != NULL );
    assert( Aig_ObjIsNode(pLeafBest) );
    // deref the best leaf
    ConeCur1 = Aig_NodeDeref_rec( pLeafBest, 0, NULL, NULL );
    // collect the cut nodes
    Vec_PtrClear( vResult );
    Aig_ManIncrementTravId( p );
    Aig_NodeMffcSupp_rec( p, pNode, 0, vResult, 1, pLeafBest );
    // ref the nodes
    ConeCur2 = Aig_NodeRef_rec( pLeafBest, 0 );
    assert( ConeCur1 == ConeCur2 );
    // ref the original node
    ConeSize2 = Aig_NodeRef_rec( pNode, 0 );
    assert( ConeSize1 == ConeSize2 );
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

