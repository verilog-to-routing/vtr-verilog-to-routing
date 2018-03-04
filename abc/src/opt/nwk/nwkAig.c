/**CFile****************************************************************

  FileName    [nwkAig.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Netlist representation.]

  Synopsis    [Translating of AIG into the network.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: nwkAig.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

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

  Synopsis    [Converts AIG into the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Nwk_Man_t * Nwk_ManDeriveFromAig( Aig_Man_t * p )
{
    Nwk_Man_t * pNtk;
    Aig_Obj_t * pObj;
    int i;
    pNtk = Nwk_ManAlloc();
    pNtk->nFanioPlus = 0;
    Hop_ManStop( pNtk->pManHop );
    pNtk->pManHop = NULL;
    pNtk->pName = Abc_UtilStrsav( p->pName );
    pNtk->pSpec = Abc_UtilStrsav( p->pSpec );
    pObj = Aig_ManConst1(p);
    pObj->pData = Nwk_ManCreateNode( pNtk, 0, pObj->nRefs );
    Aig_ManForEachCi( p, pObj, i )
        pObj->pData = Nwk_ManCreateCi( pNtk, pObj->nRefs );
    Aig_ManForEachNode( p, pObj, i )
    {
        pObj->pData = Nwk_ManCreateNode( pNtk, 2, pObj->nRefs );
        Nwk_ObjAddFanin( (Nwk_Obj_t *)pObj->pData, (Nwk_Obj_t *)Aig_ObjFanin0(pObj)->pData );
        Nwk_ObjAddFanin( (Nwk_Obj_t *)pObj->pData, (Nwk_Obj_t *)Aig_ObjFanin1(pObj)->pData );
    }
    Aig_ManForEachCo( p, pObj, i )
    {
        pObj->pData = Nwk_ManCreateCo( pNtk );
        Nwk_ObjAddFanin( (Nwk_Obj_t *)pObj->pData, (Nwk_Obj_t *)Aig_ObjFanin0(pObj)->pData );
    }
    return pNtk;
}

/**Function*************************************************************

  Synopsis    [Converts AIG into the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Nwk_ManDeriveRetimingCut( Aig_Man_t * p, int fForward, int fVerbose )
{ 
    Vec_Ptr_t * vNodes;
    Nwk_Man_t * pNtk;
    Nwk_Obj_t * pNode;
    Aig_Obj_t * pObj;
    int i;
    pNtk = Nwk_ManDeriveFromAig( p );
    if ( fForward )
        vNodes = Nwk_ManRetimeCutForward( pNtk, Aig_ManRegNum(p), fVerbose );
    else
        vNodes = Nwk_ManRetimeCutBackward( pNtk, Aig_ManRegNum(p), fVerbose );
    Aig_ManForEachObj( p, pObj, i )
        ((Nwk_Obj_t *)pObj->pData)->pCopy = pObj;
    Vec_PtrForEachEntry( Nwk_Obj_t *, vNodes, pNode, i )
        Vec_PtrWriteEntry( vNodes, i, pNode->pCopy );
    Nwk_ManFree( pNtk );
//    assert( Vec_PtrSize(vNodes) <= Aig_ManRegNum(p) );
    return vNodes;
}


ABC_NAMESPACE_IMPL_END

#include "proof/abs/abs.h"

ABC_NAMESPACE_IMPL_START

/**Function*************************************************************

  Synopsis    [Collects reachable nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManColleacReached_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vNodes, Vec_Int_t * vLeaves )
{
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return;
    Gia_ObjSetTravIdCurrent(p, pObj);
    if ( Gia_ObjIsCi(pObj) )
    {
        Vec_IntPush( vLeaves, Gia_ObjId(p, pObj) );
        return;
    }
    assert( Gia_ObjIsAnd(pObj) );
    Nwk_ManColleacReached_rec( p, Gia_ObjFanin0(pObj), vNodes, vLeaves );
    Nwk_ManColleacReached_rec( p, Gia_ObjFanin1(pObj), vNodes, vLeaves );
    Vec_IntPush( vNodes, Gia_ObjId(p, pObj) );
}


/**Function*************************************************************

  Synopsis    [Converts AIG into the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Nwk_Man_t * Nwk_ManCreateFromGia( Gia_Man_t * p, Vec_Int_t * vPPis, Vec_Int_t * vNodes, Vec_Int_t * vLeaves, Vec_Int_t ** pvMapInv )
{
    Nwk_Man_t * pNtk;
    Nwk_Obj_t ** ppCopies;
    Gia_Obj_t * pObj;
    Vec_Int_t * vMaps;
    int i;
//    assert( Vec_IntSize(vLeaves) >= Vec_IntSize(vPPis) );
    Gia_ManCreateRefs( p );
    pNtk = Nwk_ManAlloc();
    pNtk->pName = Abc_UtilStrsav( p->pName );
    pNtk->nFanioPlus = 0;
    Hop_ManStop( pNtk->pManHop );
    pNtk->pManHop = NULL;
    // allocate
    vMaps = Vec_IntAlloc( Vec_IntSize(vNodes) + Abc_MaxInt(Vec_IntSize(vPPis), Vec_IntSize(vLeaves)) + 1 );
    ppCopies = ABC_ALLOC( Nwk_Obj_t *, Gia_ManObjNum(p) );
    // copy objects
    pObj = Gia_ManConst0(p);
//    ppCopies[Gia_ObjId(p,pObj)] = Nwk_ManCreateNode( pNtk, 0, Gia_ObjRefNum(p,pObj) );
    ppCopies[Gia_ObjId(p,pObj)] = Nwk_ManCreateNode( pNtk, 0, Gia_ObjRefNum(p,pObj) + (Vec_IntSize(vLeaves) > Vec_IntSize(vPPis) ? Vec_IntSize(vLeaves) - Vec_IntSize(vPPis) : 0) );
    Vec_IntPush( vMaps, Gia_ObjId(p,pObj) );
    Gia_ManForEachObjVec( vLeaves, p, pObj, i )
    {
        ppCopies[Gia_ObjId(p,pObj)] = Nwk_ManCreateCi( pNtk, Gia_ObjRefNum(p,pObj) );
        assert( Vec_IntSize(vMaps) == Nwk_ObjId(ppCopies[Gia_ObjId(p,pObj)]) );
        Vec_IntPush( vMaps, Gia_ObjId(p,pObj) );
    }
    for ( i = Vec_IntSize(vLeaves); i < Vec_IntSize(vPPis); i++ )
    {
        Nwk_ManCreateCi( pNtk, 0 );
        Vec_IntPush( vMaps, -1 );
    }
    Gia_ManForEachObjVec( vNodes, p, pObj, i )
    {
        ppCopies[Gia_ObjId(p,pObj)] = Nwk_ManCreateNode( pNtk, 2, Gia_ObjRefNum(p,pObj) );
        Nwk_ObjAddFanin( ppCopies[Gia_ObjId(p,pObj)], ppCopies[Gia_ObjFaninId0p(p,pObj)] );
        Nwk_ObjAddFanin( ppCopies[Gia_ObjId(p,pObj)], ppCopies[Gia_ObjFaninId1p(p,pObj)] );
        assert( Vec_IntSize(vMaps) == Nwk_ObjId(ppCopies[Gia_ObjId(p,pObj)]) );
        Vec_IntPush( vMaps, Gia_ObjId(p,pObj) );
    }
    Gia_ManForEachObjVec( vPPis, p, pObj, i )
    {
        assert( ppCopies[Gia_ObjId(p,pObj)] != NULL );
        Nwk_ObjAddFanin( Nwk_ManCreateCo(pNtk), ppCopies[Gia_ObjId(p,pObj)] );
    }
    for ( i = Vec_IntSize(vPPis); i < Vec_IntSize(vLeaves); i++ )
        Nwk_ObjAddFanin( Nwk_ManCreateCo(pNtk), ppCopies[0] );
    assert( Vec_IntSize(vMaps) == Vec_IntSize(vNodes) + Abc_MaxInt(Vec_IntSize(vPPis), Vec_IntSize(vLeaves)) + 1 );
    ABC_FREE( ppCopies );
    *pvMapInv = vMaps;
    return pNtk;
}


/**Function*************************************************************

  Synopsis    [Returns min-cut in the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManDeriveMinCut( Gia_Man_t * p, int fVerbose )
{
    Nwk_Man_t * pNtk;
    Nwk_Obj_t * pNode;
    Vec_Ptr_t * vMinCut;
    Vec_Int_t * vPPis, * vNodes, * vLeaves, * vNodes2, * vLeaves2, * vMapInv;
    Vec_Int_t * vCommon, * vDiff0, * vDiff1;
    Gia_Obj_t * pObj;
    int i, iObjId;
    // get inputs
    Gia_ManGlaCollect( p, p->vGateClasses, NULL, &vPPis, NULL, NULL );
    // collect nodes rechable from PPIs
    vNodes = Vec_IntAlloc( 100 );
    vLeaves = Vec_IntAlloc( 100 );
    Gia_ManIncrementTravId( p );
    Gia_ManForEachObjVec( vPPis, p, pObj, i )
        Nwk_ManColleacReached_rec( p, pObj, vNodes, vLeaves );
    // derive the new network
    pNtk = Nwk_ManCreateFromGia( p, vPPis, vNodes, vLeaves, &vMapInv );
    assert( Nwk_ManPiNum(pNtk) == Nwk_ManPoNum(pNtk) );
    // create min-cut
    vMinCut = Nwk_ManRetimeCutBackward( pNtk, Nwk_ManPiNum(pNtk), fVerbose );
    // copy from the GIA back
//    Aig_ManForEachObj( p, pObj, i )
//        ((Nwk_Obj_t *)pObj->pData)->pCopy = pObj;
    // mark min-cut nodes
    vNodes2 = Vec_IntAlloc( 100 );
    vLeaves2 = Vec_IntAlloc( 100 );
    Gia_ManIncrementTravId( p );
    Vec_PtrForEachEntry( Nwk_Obj_t *, vMinCut, pNode, i )
    {
        pObj = Gia_ManObj( p, Vec_IntEntry(vMapInv, Nwk_ObjId(pNode)) );
        if ( Gia_ObjIsConst0(pObj) )
            continue;
        Nwk_ManColleacReached_rec( p, pObj, vNodes2, vLeaves2 );
    }
    if ( fVerbose )
        printf( "Min-cut: %d -> %d.  Nodes %d -> %d.  ", Vec_IntSize(vPPis)+1, Vec_PtrSize(vMinCut), Vec_IntSize(vNodes), Vec_IntSize(vNodes2) );
    Vec_IntFree( vPPis );
    Vec_PtrFree( vMinCut );
    Vec_IntFree( vMapInv );
    Nwk_ManFree( pNtk );

    // sort the results
    Vec_IntSort( vNodes, 0 );
    Vec_IntSort( vNodes2, 0 );
    vCommon = Vec_IntAlloc( Vec_IntSize(vNodes) );
    vDiff0 = Vec_IntAlloc( 100 );
    vDiff1 = Vec_IntAlloc( 100 );
    Vec_IntTwoSplit( vNodes, vNodes2, vCommon, vDiff0, vDiff1 );
    if ( fVerbose )
        printf( "Common = %d.  Diff0 = %d. Diff1 = %d.\n", Vec_IntSize(vCommon), Vec_IntSize(vDiff0), Vec_IntSize(vDiff1) );

    // fill in
    Vec_IntForEachEntry( vDiff0, iObjId, i )
        Vec_IntWriteEntry( p->vGateClasses, iObjId, 1 );

    Vec_IntFree( vLeaves );
    Vec_IntFree( vNodes );
    Vec_IntFree( vLeaves2 );
    Vec_IntFree( vNodes2 );

    Vec_IntFree( vCommon );
    Vec_IntFree( vDiff0 );
    Vec_IntFree( vDiff1 );

    // check abstraction
    Gia_ManForEachObj( p, pObj, i )
    {
        if ( Vec_IntEntry( p->vGateClasses, i ) == 0 )
            continue;
        assert( Gia_ObjIsConst0(pObj) || Gia_ObjIsRo(p, pObj) || Gia_ObjIsAnd(pObj) );
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

