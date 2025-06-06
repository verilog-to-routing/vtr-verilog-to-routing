/**CFile****************************************************************

  FileName    [abcRefs.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Procedures using reference counting of the AIG nodes.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcRefs.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Abc_NodeRefDeref( Abc_Obj_t * pNode, int fReference, int fLabel );
static int Abc_NodeRefDerefStop( Abc_Obj_t * pNode, int fReference );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns the MFFC size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeMffcSize( Abc_Obj_t * pNode )
{
    int nConeSize1, nConeSize2;
//    assert( Abc_NtkIsStrash(pNode->pNtk) );
//    assert( !Abc_ObjIsComplement( pNode ) );
    assert( Abc_ObjIsNode( pNode ) );
    if ( Abc_ObjFaninNum(pNode) == 0 )
        return 0;
    nConeSize1 = Abc_NodeRefDeref( pNode, 0, 0 ); // dereference
    nConeSize2 = Abc_NodeRefDeref( pNode, 1, 0 ); // reference
    assert( nConeSize1 == nConeSize2 );
    assert( nConeSize1 > 0 );
    return nConeSize1;
}

/**Function*************************************************************

  Synopsis    [Returns the MFFC size while stopping at the complemented edges.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeMffcSizeStop( Abc_Obj_t * pNode )
{
    int nConeSize1, nConeSize2;
    assert( Abc_NtkIsStrash(pNode->pNtk) );
    assert( !Abc_ObjIsComplement( pNode ) );
    assert( Abc_ObjIsNode( pNode ) );
    if ( Abc_ObjFaninNum(pNode) == 0 )
        return 0;
    nConeSize1 = Abc_NodeRefDerefStop( pNode, 0 ); // dereference
    nConeSize2 = Abc_NodeRefDerefStop( pNode, 1 ); // reference
    assert( nConeSize1 == nConeSize2 );
    assert( nConeSize1 > 0 );
    return nConeSize1;
}

/**Function*************************************************************

  Synopsis    [Labels MFFC with the current traversal ID.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeMffcLabelAig( Abc_Obj_t * pNode )
{
    int nConeSize1, nConeSize2;
    assert( Abc_NtkIsStrash(pNode->pNtk) );
    assert( !Abc_ObjIsComplement( pNode ) );
    assert( Abc_ObjIsNode( pNode ) );
    if ( Abc_ObjFaninNum(pNode) == 0 )
        return 0;
    nConeSize1 = Abc_NodeRefDeref( pNode, 0, 1 ); // dereference
    nConeSize2 = Abc_NodeRefDeref( pNode, 1, 0 ); // reference
    assert( nConeSize1 == nConeSize2 );
    assert( nConeSize1 > 0 );
    return nConeSize1;
}

/**Function*************************************************************

  Synopsis    [References/references the node and returns MFFC size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeRefDeref( Abc_Obj_t * pNode, int fReference, int fLabel )
{
    Abc_Obj_t * pNode0, * pNode1;
    int Counter;
    // label visited nodes
    if ( fLabel )
        Abc_NodeSetTravIdCurrent( pNode );
    // skip the CI
    if ( Abc_ObjIsCi(pNode) )
        return 0;
    // process the internal node
    pNode0 = Abc_ObjFanin0(pNode);
    pNode1 = Abc_ObjFanin1(pNode);
    Counter = 1;
    if ( fReference )
    {
        if ( pNode0->vFanouts.nSize++ == 0 )
            Counter += Abc_NodeRefDeref( pNode0, fReference, fLabel );
        if ( pNode1->vFanouts.nSize++ == 0 )
            Counter += Abc_NodeRefDeref( pNode1, fReference, fLabel );
    }
    else
    {
        assert( pNode0->vFanouts.nSize > 0 );
        assert( pNode1->vFanouts.nSize > 0 );
        if ( --pNode0->vFanouts.nSize == 0 )
            Counter += Abc_NodeRefDeref( pNode0, fReference, fLabel );
        if ( --pNode1->vFanouts.nSize == 0 )
            Counter += Abc_NodeRefDeref( pNode1, fReference, fLabel );
    }
    return Counter;
}


/**Function*************************************************************

  Synopsis    [References/references the node and returns MFFC size.]

  Description [Stops at the complemented edges.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeRefDerefStop( Abc_Obj_t * pNode, int fReference )
{
    Abc_Obj_t * pNode0, * pNode1;
    int Counter;
    // skip the CI
    if ( Abc_ObjIsCi(pNode) )
        return 0;
    // process the internal node
    pNode0 = Abc_ObjFanin0(pNode);
    pNode1 = Abc_ObjFanin1(pNode);
    Counter = 1;
    if ( fReference )
    {
        if ( !Abc_ObjFaninC0(pNode) && pNode0->vFanouts.nSize++ == 0 )
            Counter += Abc_NodeRefDerefStop( pNode0, fReference );
        if ( !Abc_ObjFaninC1(pNode) && pNode1->vFanouts.nSize++ == 0 )
            Counter += Abc_NodeRefDerefStop( pNode1, fReference );
    }
    else
    {
        assert( pNode0->vFanouts.nSize > 0 );
        assert( pNode1->vFanouts.nSize > 0 );
        if ( !Abc_ObjFaninC0(pNode) && --pNode0->vFanouts.nSize == 0 )
            Counter += Abc_NodeRefDerefStop( pNode0, fReference );
        if ( !Abc_ObjFaninC1(pNode) && --pNode1->vFanouts.nSize == 0 )
            Counter += Abc_NodeRefDerefStop( pNode1, fReference );
    }
    return Counter;
}




/**Function*************************************************************

  Synopsis    [Dereferences the node's MFFC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeDeref_rec( Abc_Obj_t * pNode )
{
    Abc_Obj_t * pFanin;
    int i, Counter = 1;
    if ( Abc_ObjIsCi(pNode) )
        return 0;
    Abc_ObjForEachFanin( pNode, pFanin, i )
    {
        assert( pFanin->vFanouts.nSize > 0 );
        if ( --pFanin->vFanouts.nSize == 0 )
            Counter += Abc_NodeDeref_rec( pFanin );
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [References the node's MFFC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeRef_rec( Abc_Obj_t * pNode )
{
    Abc_Obj_t * pFanin;
    int i, Counter = 1;
    if ( Abc_ObjIsCi(pNode) )
        return 0;
    Abc_ObjForEachFanin( pNode, pFanin, i )
    {
        if ( pFanin->vFanouts.nSize++ == 0 )
            Counter += Abc_NodeRef_rec( pFanin );
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Collects the internal and boundary nodes in the derefed MFFC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeMffcConeSupp_rec( Abc_Obj_t * pNode, Vec_Ptr_t * vCone, Vec_Ptr_t * vSupp, int fTopmost )
{
    Abc_Obj_t * pFanin;
    int i;
    // skip visited nodes
    if ( Abc_NodeIsTravIdCurrent(pNode) )
        return;
    Abc_NodeSetTravIdCurrent(pNode);
    // add to the new support nodes
    if ( !fTopmost && (Abc_ObjIsCi(pNode) || pNode->vFanouts.nSize > 0) )
    {
        if ( vSupp ) Vec_PtrPush( vSupp, pNode );
        return;
    }
    // recur on the children
    Abc_ObjForEachFanin( pNode, pFanin, i )
        Abc_NodeMffcConeSupp_rec( pFanin, vCone, vSupp, 0 );
    // collect the internal node
    if ( vCone ) Vec_PtrPush( vCone, pNode );
//    printf( "%d ", pNode->Id );
}

/**Function*************************************************************

  Synopsis    [Collects the support of the derefed MFFC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeMffcConeSupp( Abc_Obj_t * pNode, Vec_Ptr_t * vCone, Vec_Ptr_t * vSupp )
{
    assert( Abc_ObjIsNode(pNode) );
    assert( !Abc_ObjIsComplement(pNode) );
    if ( vCone ) Vec_PtrClear( vCone );
    if ( vSupp ) Vec_PtrClear( vSupp );
    Abc_NtkIncrementTravId( pNode->pNtk );
    Abc_NodeMffcConeSupp_rec( pNode, vCone, vSupp, 1 );
//    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Collects the support of the derefed MFFC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeMffcConeSuppPrint( Abc_Obj_t * pNode )
{
    Vec_Ptr_t * vCone, * vSupp;
    Abc_Obj_t * pObj;
    int i;
    vCone = Vec_PtrAlloc( 100 );
    vSupp = Vec_PtrAlloc( 100 );
    Abc_NodeDeref_rec( pNode );
    Abc_NodeMffcConeSupp( pNode, vCone, vSupp );
    Abc_NodeRef_rec( pNode );
    printf( "Node = %6s : Supp = %3d  Cone = %3d  (", 
        Abc_ObjName(pNode), Vec_PtrSize(vSupp), Vec_PtrSize(vCone) );
    Vec_PtrForEachEntry( Abc_Obj_t *, vCone, pObj, i )
        printf( " %s", Abc_ObjName(pObj) );
    printf( " )\n" );
    Vec_PtrFree( vCone );
    Vec_PtrFree( vSupp );
}

/**Function*************************************************************

  Synopsis    [Collects the internal nodes of the MFFC limited by cut.]

  Description []
               
  SideEffects [Increments the trav ID and marks visited nodes.]

  SeeAlso     []

***********************************************************************/
int Abc_NodeMffcInside( Abc_Obj_t * pNode, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vInside )
{
    Abc_Obj_t * pObj;
    int i, Count1, Count2;
    // increment the fanout counters for the leaves
    Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pObj, i )
        pObj->vFanouts.nSize++;
    // dereference the node
    Count1 = Abc_NodeDeref_rec( pNode );
    // collect the nodes inside the MFFC
    Abc_NodeMffcConeSupp( pNode, vInside, NULL );
    // reference it back
    Count2 = Abc_NodeRef_rec( pNode );
    assert( Count1 == Count2 );
    // remove the extra counters
    Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pObj, i )
        pObj->vFanouts.nSize--;
    return Count1;
}

/**Function*************************************************************

  Synopsis    [Collects the internal nodes of the MFFC limited by cut.]

  Description []
               
  SideEffects [Increments the trav ID and marks visited nodes.]

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NodeMffcInsideCollect( Abc_Obj_t * pNode )
{
    Vec_Ptr_t * vInside;
    int Count1, Count2;
    // dereference the node
    Count1 = Abc_NodeDeref_rec( pNode );
    // collect the nodes inside the MFFC
    vInside = Vec_PtrAlloc( 10 );
    Abc_NodeMffcConeSupp( pNode, vInside, NULL );
    // reference it back
    Count2 = Abc_NodeRef_rec( pNode );
    assert( Count1 == Count2 );
    return vInside;
}

/**Function*************************************************************

  Synopsis    [Collects the internal and boundary nodes in the derefed MFFC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeMffcLabel_rec( Abc_Obj_t * pNode, int fTopmost, Vec_Ptr_t * vNodes )
{
    Abc_Obj_t * pFanin;
    int i;
    // add to the new support nodes
    if ( !fTopmost && (Abc_ObjIsCi(pNode) || pNode->vFanouts.nSize > 0) )
        return;
    // skip visited nodes
    if ( Abc_NodeIsTravIdCurrent(pNode) )
        return;
    Abc_NodeSetTravIdCurrent(pNode);
    // recur on the children
    Abc_ObjForEachFanin( pNode, pFanin, i )
        Abc_NodeMffcLabel_rec( pFanin, 0, vNodes );
    // collect the internal node
//    printf( "%d ", pNode->Id );
    if ( vNodes )
        Vec_PtrPush( vNodes, pNode );
}

/**Function*************************************************************

  Synopsis    [Collects the internal nodes of the MFFC limited by cut.]

  Description []
               
  SideEffects [Increments the trav ID and marks visited nodes.]

  SeeAlso     []

***********************************************************************/
int Abc_NodeMffcLabel( Abc_Obj_t * pNode, Vec_Ptr_t * vNodes )
{
    int Count1, Count2;
    // dereference the node
    Count1 = Abc_NodeDeref_rec( pNode );
    // collect the nodes inside the MFFC
    Abc_NtkIncrementTravId( pNode->pNtk );
    Abc_NodeMffcLabel_rec( pNode, 1, vNodes );
    // reference it back
    Count2 = Abc_NodeRef_rec( pNode );
    assert( Count1 == Count2 );
    return Count1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

