/**CFile****************************************************************

  FileName    [aigFactor.c]

  SystemName  []

  PackageName []

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation []

  Date        [Ver. 1.0. Started - April 17, 2009.]

  Revision    [$Id: aigFactor.c,v 1.00 2009/04/17 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig.h"
#include "bool/kit/kit.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Detects multi-input AND gate rooted at this node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManFindImplications_rec( Aig_Obj_t * pObj, Vec_Ptr_t * vImplics )
{
    if ( Aig_IsComplement(pObj) || Aig_ObjIsCi(pObj) )
    {
        Vec_PtrPushUnique( vImplics, pObj );
        return;
    }
    Aig_ManFindImplications_rec( Aig_ObjChild0(pObj), vImplics );
    Aig_ManFindImplications_rec( Aig_ObjChild1(pObj), vImplics );
}

/**Function*************************************************************

  Synopsis    [Returns the nodes whose values are implied by pNode.]

  Description [Attention!  Both pNode and results can be complemented!
  Also important: Currently, this procedure only does backward propagation. 
  In general, it may find more implications if forward propagation is enabled.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManFindImplications( Aig_Man_t * p, Aig_Obj_t * pNode )
{
    Vec_Ptr_t * vImplics;
    vImplics = Vec_PtrAlloc( 100 );
    Aig_ManFindImplications_rec( pNode, vImplics );
    return vImplics;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the cone of the node overlaps with the vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManFindConeOverlap_rec( Aig_Man_t * p, Aig_Obj_t * pNode )
{
    if ( Aig_ObjIsTravIdPrevious( p, pNode ) )
        return 1;
    if ( Aig_ObjIsTravIdCurrent( p, pNode ) )
        return 0;
    Aig_ObjSetTravIdCurrent( p, pNode );
    if ( Aig_ObjIsCi(pNode) )
        return 0;
    if ( Aig_ManFindConeOverlap_rec( p, Aig_ObjFanin0(pNode) ) )
        return 1;
    if ( Aig_ManFindConeOverlap_rec( p, Aig_ObjFanin1(pNode) ) )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the cone of the node overlaps with the vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManFindConeOverlap( Aig_Man_t * p, Vec_Ptr_t * vImplics, Aig_Obj_t * pNode )
{
    Aig_Obj_t * pTemp;
    int i;
    assert( !Aig_IsComplement(pNode) );
    assert( !Aig_ObjIsConst1(pNode) );
    Aig_ManIncrementTravId( p );
    Vec_PtrForEachEntry( Aig_Obj_t *, vImplics, pTemp, i )
        Aig_ObjSetTravIdCurrent( p, Aig_Regular(pTemp) );
    Aig_ManIncrementTravId( p );
    return Aig_ManFindConeOverlap_rec( p, pNode );
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the cone of the node overlaps with the vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Aig_ManDeriveNewCone_rec( Aig_Man_t * p, Aig_Obj_t * pNode )
{
    if ( Aig_ObjIsTravIdCurrent( p, pNode ) )
        return (Aig_Obj_t *)pNode->pData;
    Aig_ObjSetTravIdCurrent( p, pNode );
    if ( Aig_ObjIsCi(pNode) )
        return (Aig_Obj_t *)(pNode->pData = pNode);
    Aig_ManDeriveNewCone_rec( p, Aig_ObjFanin0(pNode) );
    Aig_ManDeriveNewCone_rec( p, Aig_ObjFanin1(pNode) );
    return (Aig_Obj_t *)(pNode->pData = Aig_And( p, Aig_ObjChild0Copy(pNode), Aig_ObjChild1Copy(pNode) ));
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the cone of the node overlaps with the vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Aig_ManDeriveNewCone( Aig_Man_t * p, Vec_Ptr_t * vImplics, Aig_Obj_t * pNode )
{
    Aig_Obj_t * pTemp;
    int i;
    assert( !Aig_IsComplement(pNode) );
    assert( !Aig_ObjIsConst1(pNode) );
    Aig_ManIncrementTravId( p );
    Vec_PtrForEachEntry( Aig_Obj_t *, vImplics, pTemp, i )
    {
        Aig_ObjSetTravIdCurrent( p, Aig_Regular(pTemp) );
        Aig_Regular(pTemp)->pData = Aig_NotCond( Aig_ManConst1(p), Aig_IsComplement(pTemp) );
    }
    return Aig_ManDeriveNewCone_rec( p, pNode );
}

/**Function*************************************************************

  Synopsis    [Returns algebraic factoring of B in terms of A.]

  Description [Returns internal node C (an AND gate) that is equal to B 
  under assignment A = 'Value', or NULL if there is no such node C. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Aig_ManFactorAlgebraic_int( Aig_Man_t * p, Aig_Obj_t * pPoA, Aig_Obj_t * pPoB, int Value )
{
    Aig_Obj_t * pNodeA, * pNodeC;
    Vec_Ptr_t * vImplics;
    int RetValue;
    if ( Aig_ObjIsConst1(Aig_ObjFanin0(pPoA)) || Aig_ObjIsConst1(Aig_ObjFanin0(pPoB)) )
        return NULL;
    if ( Aig_ObjIsCi(Aig_ObjFanin0(pPoB)) )
        return NULL;
    // get the internal node representing function of A under assignment A = 'Value'
    pNodeA = Aig_ObjChild0( pPoA );
    pNodeA = Aig_NotCond( pNodeA, Value==0 );
    // find implications of this signal (nodes whose value is fixed under assignment A = 'Value')
    vImplics = Aig_ManFindImplications( p, pNodeA );
    // check if the TFI cone of B overlaps with the implied nodes
    RetValue = Aig_ManFindConeOverlap( p, vImplics, Aig_ObjFanin0(pPoB) );
    if ( RetValue == 0 ) // no overlap
    {
        Vec_PtrFree( vImplics );
        return NULL;
    }
    // there is overlap - derive node representing value of B under assignment A = 'Value'
    pNodeC = Aig_ManDeriveNewCone( p, vImplics, Aig_ObjFanin0(pPoB) );
    pNodeC = Aig_NotCond( pNodeC, Aig_ObjFaninC0(pPoB) );
    Vec_PtrFree( vImplics );
    return pNodeC;
}

/**Function*************************************************************

  Synopsis    [Returns algebraic factoring of B in terms of A.]

  Description [Returns internal node C (an AND gate) that is equal to B 
  under assignment A = 'Value', or NULL if there is no such node C. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Aig_ManFactorAlgebraic( Aig_Man_t * p, int iPoA, int iPoB, int Value )
{
    assert( iPoA >= 0 && iPoA < Aig_ManCoNum(p) );
    assert( iPoB >= 0 && iPoB < Aig_ManCoNum(p) );
    assert( Value == 0 || Value == 1 );
    return Aig_ManFactorAlgebraic_int( p, Aig_ManCo(p, iPoA), Aig_ManCo(p, iPoB), Value ); 
}

/**Function*************************************************************

  Synopsis    [Testing procedure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManFactorAlgebraicTest( Aig_Man_t * p )
{
    int iPoA  = 0;
    int iPoB  = 1;
    int Value = 0;
    Aig_Obj_t * pRes;
//    Aig_Obj_t * pObj;
//    int i;
    pRes = Aig_ManFactorAlgebraic( p, iPoA, iPoB, Value );
    Aig_ManShow( p, 0, NULL );
    Aig_ObjPrint( p, pRes );
    printf( "\n" );
/*
    printf( "Results:\n" );
    Aig_ManForEachObj( p, pObj, i )
    {
        printf( "Object = %d.\n", i );
        Aig_ObjPrint( p, pObj );
        printf( "\n" );
        Aig_ObjPrint( p, pObj->pData );
        printf( "\n" );
    }
*/
}



/**Function*************************************************************

  Synopsis    [Determines what support variables can be cofactored.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_SuppMinPerform( Aig_Man_t * p, Vec_Ptr_t * vOrGate, Vec_Ptr_t * vNodes, Vec_Ptr_t * vSupp )
{
    Aig_Obj_t * pObj;
    Vec_Ptr_t * vTrSupp, * vTrNode, * vCofs;
    unsigned * uFunc = NULL, * uCare, * uFunc0, * uFunc1;
    unsigned * uCof0, * uCof1, * uCare0, * uCare1;
    int i, nWords = Abc_TruthWordNum( Vec_PtrSize(vSupp) );
    // assign support nodes
    vTrSupp = Vec_PtrAllocTruthTables( Vec_PtrSize(vSupp) );
    Vec_PtrForEachEntry( Aig_Obj_t *, vSupp, pObj, i )
        pObj->pData = Vec_PtrEntry( vTrSupp, i );
    // compute internal nodes
    vTrNode = Vec_PtrAllocSimInfo( Vec_PtrSize(vNodes) + 5, nWords );
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
    {
        pObj->pData = uFunc = (unsigned *)Vec_PtrEntry( vTrNode, i );
        uFunc0 = (unsigned *)Aig_ObjFanin0(pObj)->pData;
        uFunc1 = (unsigned *)Aig_ObjFanin1(pObj)->pData;
        Kit_TruthAndPhase( uFunc, uFunc0, uFunc1, Vec_PtrSize(vSupp), Aig_ObjFaninC0(pObj), Aig_ObjFaninC1(pObj) );
    }
    // uFunc contains the result of computation
    // compute care set
    uCare = (unsigned *)Vec_PtrEntry( vTrNode, Vec_PtrSize(vNodes) );
    Kit_TruthClear( uCare, Vec_PtrSize(vSupp) );
    Vec_PtrForEachEntry( Aig_Obj_t *, vOrGate, pObj, i )
        Kit_TruthOrPhase( uCare, uCare, (unsigned *)Aig_Regular(pObj)->pData, Vec_PtrSize(vSupp), 0, Aig_IsComplement(pObj) );
    // try cofactoring each variable in both polarities
    vCofs = Vec_PtrAlloc( 10 );
    uCof0  = (unsigned *)Vec_PtrEntry( vTrNode, Vec_PtrSize(vNodes)+1 );
    uCof1  = (unsigned *)Vec_PtrEntry( vTrNode, Vec_PtrSize(vNodes)+2 );
    uCare0 = (unsigned *)Vec_PtrEntry( vTrNode, Vec_PtrSize(vNodes)+3 );
    uCare1 = (unsigned *)Vec_PtrEntry( vTrNode, Vec_PtrSize(vNodes)+4 );
    Vec_PtrForEachEntry( Aig_Obj_t *, vSupp, pObj, i )
    {
        Kit_TruthCofactor0New( uCof0, uFunc, Vec_PtrSize(vSupp), i );
        Kit_TruthCofactor1New( uCof1, uFunc, Vec_PtrSize(vSupp), i );
        Kit_TruthCofactor0New( uCare0, uCare, Vec_PtrSize(vSupp), i );
        Kit_TruthCofactor1New( uCare1, uCare, Vec_PtrSize(vSupp), i );
        if ( Kit_TruthIsEqualWithCare( uCof0, uCof1, uCare1, Vec_PtrSize(vSupp) ) )
            Vec_PtrPush( vCofs, Aig_Not(pObj) );
        else if ( Kit_TruthIsEqualWithCare( uCof0, uCof1, uCare0, Vec_PtrSize(vSupp) ) )
            Vec_PtrPush( vCofs, pObj );
    }
    Vec_PtrFree( vTrNode );
    Vec_PtrFree( vTrSupp );
    return vCofs;
}



/**Function*************************************************************

  Synopsis    [Returns the new node after cofactoring.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Aig_SuppMinReconstruct( Aig_Man_t * p, Vec_Ptr_t * vCofs, Vec_Ptr_t * vNodes, Vec_Ptr_t * vSupp )
{
    Aig_Obj_t * pObj = NULL;
    int i;
    // set the value of the support variables
    Vec_PtrForEachEntry( Aig_Obj_t *, vSupp, pObj, i )
        assert( !Aig_IsComplement(pObj) );
    Vec_PtrForEachEntry( Aig_Obj_t *, vSupp, pObj, i )
        pObj->pData = pObj;
    // set the value of the cofactoring variables
    Vec_PtrForEachEntry( Aig_Obj_t *, vCofs, pObj, i )
        Aig_Regular(pObj)->pData = Aig_NotCond( Aig_ManConst1(p), Aig_IsComplement(pObj) );
    // reconstruct the node
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
        pObj->pData = Aig_And( p, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    return (Aig_Obj_t *)pObj->pData;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if all nodes of vOrGate are in vSupp.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_SuppMinGateIsInSupport( Aig_Man_t * p, Vec_Ptr_t * vOrGate, Vec_Ptr_t * vSupp )
{
    Aig_Obj_t * pObj;
    int i;
    Aig_ManIncrementTravId( p );
    Vec_PtrForEachEntry( Aig_Obj_t *, vSupp, pObj, i )
        Aig_ObjSetTravIdCurrent( p, pObj );
    Vec_PtrForEachEntry( Aig_Obj_t *, vOrGate, pObj, i )
        if ( !Aig_ObjIsTravIdCurrent( p, Aig_Regular(pObj) ) )
            return 0;
    return 1;
}


/**Function*************************************************************

  Synopsis    [Collects fanins of the marked nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_SuppMinCollectSupport( Aig_Man_t * p, Vec_Ptr_t * vNodes )
{
    Vec_Ptr_t * vSupp;
    Aig_Obj_t * pObj, * pFanin;
    int i;
    vSupp = Vec_PtrAlloc( 4 );
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
    {
        assert( Aig_ObjIsTravIdCurrent(p, pObj) );
        assert( Aig_ObjIsNode(pObj) );
        pFanin = Aig_ObjFanin0( pObj );
        if ( !Aig_ObjIsTravIdCurrent(p, pFanin) )
        {
            Aig_ObjSetTravIdCurrent( p, pFanin );
            Vec_PtrPush( vSupp, pFanin );
        }
        pFanin = Aig_ObjFanin1( pObj );
        if ( !Aig_ObjIsTravIdCurrent(p, pFanin) )
        {
            Aig_ObjSetTravIdCurrent( p, pFanin );
            Vec_PtrPush( vSupp, pFanin );
        }
    }
    return vSupp;
}

/**Function*************************************************************

  Synopsis    [Marks the nodes in the cone with current trav ID.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_SuppMinCollectCone_rec( Aig_Man_t * p, Aig_Obj_t * pObj, Vec_Ptr_t * vNodes )
{
    if ( Aig_ObjIsTravIdCurrent( p, pObj ) ) // visited
        return;
    if ( !Aig_ObjIsTravIdPrevious( p, pObj ) ) // not visited, but outside
        return;
    assert( Aig_ObjIsTravIdPrevious(p, pObj) ); // not visited, inside
    assert( Aig_ObjIsNode(pObj) );
    Aig_ObjSetTravIdCurrent( p, pObj );
    Aig_SuppMinCollectCone_rec( p, Aig_ObjFanin0(pObj), vNodes );
    Aig_SuppMinCollectCone_rec( p, Aig_ObjFanin1(pObj), vNodes );
    Vec_PtrPush( vNodes, pObj );
}

/**Function*************************************************************

  Synopsis    [Collects nodes with the current trav ID rooted in the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_SuppMinCollectCone( Aig_Man_t * p, Aig_Obj_t * pRoot )
{
    Vec_Ptr_t * vNodes;
    assert( !Aig_IsComplement(pRoot) ); 
//    assert( Aig_ObjIsTravIdCurrent( p, pRoot ) );
    vNodes = Vec_PtrAlloc( 4 );
    Aig_ManIncrementTravId( p );
    Aig_SuppMinCollectCone_rec( p, Aig_Regular(pRoot), vNodes );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Marks the nodes in the cone with current trav ID.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_SuppMinHighlightCone_rec( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    int RetValue;
    if ( Aig_ObjIsTravIdCurrent( p, pObj ) ) // visited, marks there
        return 1;
    if ( Aig_ObjIsTravIdPrevious( p, pObj ) ) // visited, no marks there
        return 0;
    Aig_ObjSetTravIdPrevious( p, pObj );
    if ( Aig_ObjIsCi(pObj) )
        return 0;
    RetValue = Aig_SuppMinHighlightCone_rec( p, Aig_ObjFanin0(pObj) ) |
               Aig_SuppMinHighlightCone_rec( p, Aig_ObjFanin1(pObj) );
//    printf( "%d %d\n", Aig_ObjId(pObj), RetValue );
    if ( RetValue )
        Aig_ObjSetTravIdCurrent( p, pObj );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Marks the nodes in the cone with current trav ID.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_SuppMinHighlightCone( Aig_Man_t * p, Aig_Obj_t * pRoot, Vec_Ptr_t * vOrGate )
{
    Aig_Obj_t * pLeaf;
    int i, RetValue;
    assert( !Aig_IsComplement(pRoot) );
    Aig_ManIncrementTravId( p );
    Aig_ManIncrementTravId( p );
    Vec_PtrForEachEntry( Aig_Obj_t *, vOrGate, pLeaf, i )
        Aig_ObjSetTravIdCurrent( p, Aig_Regular(pLeaf) );
    RetValue = Aig_SuppMinHighlightCone_rec( p, pRoot );
    Vec_PtrForEachEntry( Aig_Obj_t *, vOrGate, pLeaf, i )
        Aig_ObjSetTravIdPrevious( p, Aig_Regular(pLeaf) );
    return RetValue;
}


/**Function*************************************************************

  Synopsis    [Collects the supergate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_SuppMinCollectSuper_rec( Aig_Obj_t * pObj, Vec_Ptr_t * vSuper )
{
    // if the new node is complemented or a PI, another gate begins
    if ( Aig_IsComplement(pObj) || Aig_ObjIsCi(pObj) ) // || (Aig_ObjRefs(pObj) > 1) )
    {
        Vec_PtrPushUnique( vSuper, Aig_Not(pObj) );
        return;
    }
    // go through the branches
    Aig_SuppMinCollectSuper_rec( Aig_ObjChild0(pObj), vSuper );
    Aig_SuppMinCollectSuper_rec( Aig_ObjChild1(pObj), vSuper );
}

/**Function*************************************************************

  Synopsis    [Collects the supergate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_SuppMinCollectSuper( Aig_Obj_t * pObj )
{
    Vec_Ptr_t * vSuper;
    assert( !Aig_IsComplement(pObj) );
    assert( !Aig_ObjIsCi(pObj) );
    vSuper = Vec_PtrAlloc( 4 );
    Aig_SuppMinCollectSuper_rec( Aig_ObjChild0(pObj), vSuper );
    Aig_SuppMinCollectSuper_rec( Aig_ObjChild1(pObj), vSuper );
    return vSuper;
}

/**Function*************************************************************

  Synopsis    [Returns the result of support minimization.]

  Description [Returns internal AIG node that is equal to pFunc under 
  assignment pCond == 1, or NULL if there is no such node. status is 
  -1 if condition is not OR; 
  -2 if cone is too large or no cone; 
  -3 if no support reduction is possible.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Aig_ManSupportMinimization( Aig_Man_t * p, Aig_Obj_t * pCond, Aig_Obj_t * pFunc, int * pStatus )
{
    int nSuppMax = 16;
    Vec_Ptr_t * vOrGate, * vNodes, * vSupp, * vCofs;
    Aig_Obj_t * pResult;
    int RetValue;
    *pStatus = 0;
    // if pCond is not OR
    if ( !Aig_IsComplement(pCond) || Aig_ObjIsCi(Aig_Regular(pCond)) || Aig_ObjIsConst1(Aig_Regular(pCond)) )
    {
        *pStatus = -1;
        return NULL;
    }
    // if pFunc is not a node
    if ( !Aig_ObjIsNode(Aig_Regular(pFunc)) )
    {
        *pStatus = -2;
        return NULL;
    }
    // collect the multi-input OR gate rooted in the condition
    vOrGate = Aig_SuppMinCollectSuper( Aig_Regular(pCond) );
    if ( Vec_PtrSize(vOrGate) > nSuppMax )
    {
        Vec_PtrFree( vOrGate );
        *pStatus = -2;
        return NULL;
    }
    // highlight the cone limited by these gates
    RetValue = Aig_SuppMinHighlightCone( p, Aig_Regular(pFunc), vOrGate );
    if ( RetValue == 0 ) // no overlap
    {
        Vec_PtrFree( vOrGate );
        *pStatus = -2;
        return NULL;
    }
    // collect the cone rooted in pFunc limited by vOrGate
    vNodes = Aig_SuppMinCollectCone( p, Aig_Regular(pFunc) );
    // collect the support nodes reachable from the cone
    vSupp = Aig_SuppMinCollectSupport( p, vNodes );
    if ( Vec_PtrSize(vSupp) > nSuppMax )
    {
        Vec_PtrFree( vOrGate );
        Vec_PtrFree( vNodes );
        Vec_PtrFree( vSupp );
        *pStatus = -2;
        return NULL;
    }
    // check if all nodes belonging to OR gate are included in the support
    // (if this is not the case, don't-care minimization is not possible)
    if ( !Aig_SuppMinGateIsInSupport( p, vOrGate, vSupp ) )
    {
        Vec_PtrFree( vOrGate );
        Vec_PtrFree( vNodes );
        Vec_PtrFree( vSupp );
        *pStatus = -3;
        return NULL;
    }
    // create truth tables of all nodes and find the maximal number
    // of support varialbles that can be replaced by constants
    vCofs = Aig_SuppMinPerform( p, vOrGate, vNodes, vSupp );
    if ( Vec_PtrSize(vCofs) == 0 )
    {
        Vec_PtrFree( vCofs );
        Vec_PtrFree( vOrGate );
        Vec_PtrFree( vNodes );
        Vec_PtrFree( vSupp );
        *pStatus = -3;
        return NULL;
    }
    // reconstruct the cone
    pResult = Aig_SuppMinReconstruct( p, vCofs, vNodes, vSupp );
    pResult = Aig_NotCond( pResult, Aig_IsComplement(pFunc) );
    Vec_PtrFree( vCofs );
    Vec_PtrFree( vOrGate );
    Vec_PtrFree( vNodes );
    Vec_PtrFree( vSupp );
    return pResult;
}
/**Function*************************************************************

  Synopsis    [Testing procedure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManSupportMinimizationTest()
{
    Aig_Man_t * p;
    Aig_Obj_t * pFunc, * pCond, * pRes;
    int i, Status;
    p = Aig_ManStart( 100 );
    for ( i = 0; i < 5; i++ )
        Aig_IthVar(p,i);
    pFunc = Aig_Mux( p, Aig_IthVar(p,3), Aig_IthVar(p,1), Aig_IthVar(p,0) );
    pFunc = Aig_Mux( p, Aig_IthVar(p,4), Aig_IthVar(p,2), pFunc );
    pCond = Aig_Or( p, Aig_IthVar(p,3), Aig_IthVar(p,4) );
    pRes  = Aig_ManSupportMinimization( p, pCond, pFunc, &Status );
    assert( Status == 0 );

    Aig_ObjPrint( p, Aig_Regular(pRes) );                printf( "\n" );
    Aig_ObjPrint( p, Aig_ObjFanin0(Aig_Regular(pRes)) ); printf( "\n" );
    Aig_ObjPrint( p, Aig_ObjFanin1(Aig_Regular(pRes)) ); printf( "\n" );

    Aig_ManStop( p );
}
void Aig_ManSupportMinimizationTest2()
{
    Aig_Man_t * p;
    Aig_Obj_t * node09, * node10, * node11, * node12, * node13, * pRes;
    int i, Status;
    p = Aig_ManStart( 100 );
    for ( i = 0; i < 3; i++ )
        Aig_IthVar(p,i);

    node09 = Aig_And( p, Aig_IthVar(p,0), Aig_Not(Aig_IthVar(p,1)) );
    node10 = Aig_And( p, Aig_Not(node09), Aig_Not(Aig_IthVar(p,2)) );
    node11 = Aig_And( p, node10, Aig_IthVar(p,1) );

    node12 = Aig_Or( p, Aig_IthVar(p,1), Aig_IthVar(p,2) );
    node13 = Aig_Or( p, node12, Aig_IthVar(p,0) );

    pRes  = Aig_ManSupportMinimization( p, node13, node11, &Status );
    assert( Status == 0 );

    printf( "Compl = %d  ", Aig_IsComplement(pRes) );
    Aig_ObjPrint( p, Aig_Regular(pRes) );                printf( "\n" );
    if ( Aig_ObjIsNode(Aig_Regular(pRes)) )
    {
        Aig_ObjPrint( p, Aig_ObjFanin0(Aig_Regular(pRes)) ); printf( "\n" );
        Aig_ObjPrint( p, Aig_ObjFanin1(Aig_Regular(pRes)) ); printf( "\n" );
    }

    Aig_ManStop( p );
}
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
ABC_NAMESPACE_IMPL_END

