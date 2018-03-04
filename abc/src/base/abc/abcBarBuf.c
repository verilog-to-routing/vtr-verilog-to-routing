/**CFile****************************************************************

  FileName    [abcHie.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Procedures to handle hierarchy.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcHie.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define ABC_OBJ_VOID ((Abc_Obj_t *)(ABC_PTRINT_T)1)

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Checks the the hie design has no duplicated networks.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCheckSingleInstance( Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pTemp, * pModel;
    Abc_Obj_t * pBox;
    int i, k, RetValue = 1;
    if ( pNtk->pDesign == NULL )
        return 1;
    Vec_PtrForEachEntry( Abc_Ntk_t *, pNtk->pDesign->vModules, pTemp, i )
        pTemp->fHieVisited = 0;
    Vec_PtrForEachEntry( Abc_Ntk_t *, pNtk->pDesign->vModules, pTemp, i )
        Abc_NtkForEachBox( pTemp, pBox, k )
        {
            pModel = (Abc_Ntk_t *)pBox->pData;
            if ( pModel == NULL )
                continue;
            if ( Abc_NtkLatchNum(pModel) > 0 )
            {
                printf( "Network \"%s\" contains %d flops.\n",                     
                    Abc_NtkName(pNtk), Abc_NtkLatchNum(pModel) );
                RetValue = 0;
            }
            if ( pModel->fHieVisited )
            {
                printf( "Network \"%s\" contains box \"%s\" whose model \"%s\" is instantiated more than once.\n", 
                    Abc_NtkName(pNtk), Abc_ObjName(pBox), Abc_NtkName(pModel) );
                RetValue = 0;
            }
            pModel->fHieVisited = 1;
        }
    Vec_PtrForEachEntry( Abc_Ntk_t *, pNtk->pDesign->vModules, pTemp, i )
        pTemp->fHieVisited = 0;
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Collect PIs and POs of internal networks in the topo order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCollectPiPos_rec( Abc_Obj_t * pNet, Vec_Ptr_t * vLiMaps, Vec_Ptr_t * vLoMaps )
{
    extern int Abc_NtkCollectPiPos_int( Abc_Obj_t * pBox, Abc_Ntk_t * pNtk, Vec_Ptr_t * vLiMaps, Vec_Ptr_t * vLoMaps );
    Abc_Obj_t * pObj, * pFanin; 
    int i, Counter = 0;
    assert( Abc_ObjIsNet(pNet) );
    if ( Abc_NodeIsTravIdCurrent( pNet ) )
        return 0;
    Abc_NodeSetTravIdCurrent( pNet );
    pObj = Abc_ObjFanin0(pNet);
    if ( Abc_ObjIsNode(pObj) )
        Abc_ObjForEachFanin( pObj, pFanin, i )
            Counter += Abc_NtkCollectPiPos_rec( pFanin, vLiMaps, vLoMaps );
    if ( Abc_ObjIsNode(pObj) )
        return Counter;
    if ( Abc_ObjIsBo(pObj) )
        pObj = Abc_ObjFanin0(pObj);
    assert( Abc_ObjIsBox(pObj) );
    Abc_ObjForEachFanout( pObj, pFanin, i )
        Abc_NodeSetTravIdCurrent( Abc_ObjFanout0(pFanin) );
    Abc_ObjForEachFanin( pObj, pFanin, i )
        Counter += Abc_NtkCollectPiPos_rec( Abc_ObjFanin0(pFanin), vLiMaps, vLoMaps );
    Counter += Abc_NtkCollectPiPos_int( pObj, Abc_ObjModel(pObj), vLiMaps, vLoMaps );
    return Counter;
}
int Abc_NtkCollectPiPos_int( Abc_Obj_t * pBox, Abc_Ntk_t * pNtk, Vec_Ptr_t * vLiMaps, Vec_Ptr_t * vLoMaps )
{
    Abc_Obj_t * pObj; 
    int i, Counter = 0;
    // mark primary inputs
    Abc_NtkIncrementTravId( pNtk );
    Abc_NtkForEachPi( pNtk, pObj, i )
        Abc_NodeSetTravIdCurrent( Abc_ObjFanout0(pObj) );
    // add primary inputs
    if ( pBox )
    {
        Abc_ObjForEachFanin( pBox, pObj, i )
            Vec_PtrPush( vLiMaps, pObj );
        Abc_NtkForEachPi( pNtk, pObj, i )
            Vec_PtrPush( vLoMaps, pObj );
    }
    // visit primary outputs
    Abc_NtkForEachPo( pNtk, pObj, i )
        Counter += Abc_NtkCollectPiPos_rec( Abc_ObjFanin0(pObj), vLiMaps, vLoMaps );
    // add primary outputs
    if ( pBox )
    {
        Abc_NtkForEachPo( pNtk, pObj, i )
            Vec_PtrPush( vLiMaps, pObj );
        Abc_ObjForEachFanout( pBox, pObj, i )
            Vec_PtrPush( vLoMaps, pObj );
        Counter++;
    }
    return Counter;
}
int Abc_NtkCollectPiPos( Abc_Ntk_t * pNtk, Vec_Ptr_t ** pvLiMaps, Vec_Ptr_t ** pvLoMaps )
{
    assert( Abc_NtkIsNetlist(pNtk) );
    *pvLiMaps = Vec_PtrAlloc( 1000 );
    *pvLoMaps = Vec_PtrAlloc( 1000 );
    return Abc_NtkCollectPiPos_int( NULL, pNtk, *pvLiMaps, *pvLoMaps );
}

/**Function*************************************************************

  Synopsis    [Derives logic network with barbufs from the netlist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkToBarBufs_rec( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pNet )
{
    Abc_Obj_t * pObj, * pFanin; 
    int i;
    assert( Abc_ObjIsNet(pNet) );
    if ( pNet->pCopy )
        return pNet->pCopy;
    pObj = Abc_ObjFanin0(pNet);
    assert( Abc_ObjIsNode(pObj) );
    pNet->pCopy = Abc_NtkDupObj( pNtkNew, pObj, 0 );
    Abc_ObjForEachFanin( pObj, pFanin, i )
        Abc_ObjAddFanin( pObj->pCopy, Abc_NtkToBarBufs_rec(pNtkNew, pFanin) );
    return pNet->pCopy;
}
Abc_Ntk_t * Abc_NtkToBarBufs( Abc_Ntk_t * pNtk )
{
    char Buffer[1000];
    Vec_Ptr_t * vLiMaps, * vLoMaps;
    Abc_Ntk_t * pNtkNew, * pTemp;
    Abc_Obj_t * pLatch, * pObjLi, * pObjLo;
    Abc_Obj_t * pObj, * pLiMap, * pLoMap;
    int i, k, nBoxes;
    assert( Abc_NtkIsNetlist(pNtk) );
    if ( !Abc_NtkCheckSingleInstance(pNtk) )
        return NULL;
    assert( pNtk->pDesign != NULL );
    // start the network
    pNtkNew = Abc_NtkAlloc( ABC_NTK_LOGIC, pNtk->ntkFunc, 1 );
    pNtkNew->pName = Extra_UtilStrsav(pNtk->pName);
    pNtkNew->pSpec = Extra_UtilStrsav(pNtk->pSpec);
    // clone CIs/CIs/boxes
    Abc_NtkCleanCopy_rec( pNtk );
    Abc_NtkForEachPi( pNtk, pObj, i )
        Abc_ObjFanout0(pObj)->pCopy = Abc_NtkDupObj( pNtkNew, pObj, 1 );
    Abc_NtkForEachPo( pNtk, pObj, i )
        Abc_NtkDupObj( pNtkNew, pObj, 1 );
    // create latches and transfer copy labels
    nBoxes = Abc_NtkCollectPiPos( pNtk, &vLiMaps, &vLoMaps );
    Vec_PtrForEachEntryTwo( Abc_Obj_t *, vLiMaps, Abc_Obj_t *, vLoMaps, pLiMap, pLoMap, i )
    {
        pObjLi = Abc_NtkCreateBi(pNtkNew);
        pLatch = Abc_NtkCreateLatch(pNtkNew);
        pObjLo = Abc_NtkCreateBo(pNtkNew);
        Abc_ObjAddFanin( pLatch, pObjLi );
        Abc_ObjAddFanin( pObjLo, pLatch );
        pLatch->pData = (void *)ABC_INIT_ZERO;
        pTemp = NULL;
        if ( Abc_ObjFanin0(pLiMap)->pNtk != pNtk )
            pTemp = Abc_ObjFanin0(pLiMap)->pNtk;
        else if ( Abc_ObjFanout0(pLoMap)->pNtk != pNtk )
            pTemp = Abc_ObjFanout0(pLoMap)->pNtk;
        else assert( 0 );
        sprintf( Buffer, "_%s_in", Abc_NtkName(pTemp) );
        Abc_ObjAssignName( pObjLi, Abc_ObjName(Abc_ObjFanin0(pLiMap)), Buffer );
        sprintf( Buffer, "_%s_out", Abc_NtkName(pTemp) );
        Abc_ObjAssignName( pObjLo, Abc_ObjName(Abc_ObjFanout0(pLoMap)), Buffer );
        pLiMap->pCopy = pObjLi;
        Abc_ObjFanout0(pLoMap)->pCopy = pObjLo;
        assert( Abc_ObjIsNet(Abc_ObjFanout0(pLoMap)) );
    }
    Vec_PtrFree( vLiMaps );
    Vec_PtrFree( vLoMaps );
    // rebuild networks
    Vec_PtrForEachEntry( Abc_Ntk_t *, pNtk->pDesign->vModules, pTemp, i )
        Abc_NtkForEachCo( pTemp, pObj, k )
            Abc_ObjAddFanin( pObj->pCopy, Abc_NtkToBarBufs_rec(pNtkNew, Abc_ObjFanin0(pObj)) );
    pNtkNew->nBarBufs = Abc_NtkLatchNum(pNtkNew);
    printf( "Hierarchy reader flattened %d instances of logic boxes and introduced %d barbufs.\n", nBoxes, pNtkNew->nBarBufs );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Converts the logic with barbufs into a hierarchical network.]

  Description [The base network is the original hierarchical network. The
  second argument is the optimized network with barbufs.  This procedure
  reconstructs the original hierarcical network which adding logic from
  the optimized network.  It is assumed that the PIs/POs of the original
  network one-to-one mapping with the flops of the optimized network.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkFromBarBufs_rec( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFanin; 
    int i;
    if ( pObj->pCopy )
        return pObj->pCopy;
    Abc_NtkDupObj( pNtkNew, pObj, 0 );
    Abc_ObjForEachFanin( pObj, pFanin, i )
        Abc_ObjAddFanin( pObj->pCopy, Abc_NtkFromBarBufs_rec(pNtkNew, pFanin) );
    return pObj->pCopy;
}
Abc_Ntk_t * Abc_NtkFromBarBufs( Abc_Ntk_t * pNtkBase, Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pNtkNew, * pTemp;
    Vec_Ptr_t * vLiMaps, * vLoMaps;
    Abc_Obj_t * pObj, * pLiMap, * pLoMap;
    int i, k;
    assert( pNtkBase->pDesign != NULL );
    assert( Abc_NtkIsNetlist(pNtk) );
    assert( Abc_NtkIsNetlist(pNtkBase) );
    assert( Abc_NtkLatchNum(pNtkBase) == 0 );
    assert( Abc_NtkLatchNum(pNtk) == pNtk->nBarBufs );
    assert( Abc_NtkWhiteboxNum(pNtk) == 0 );
    assert( Abc_NtkBlackboxNum(pNtk) == 0 );
    assert( Abc_NtkPiNum(pNtk) == Abc_NtkPiNum(pNtkBase) );
    assert( Abc_NtkPoNum(pNtk) == Abc_NtkPoNum(pNtkBase) );
    // start networks
    Abc_NtkCleanCopy_rec( pNtkBase );
    Vec_PtrForEachEntry( Abc_Ntk_t *, pNtkBase->pDesign->vModules, pTemp, i )
        pTemp->pCopy = Abc_NtkStartFrom( pTemp, pNtk->ntkType, pNtk->ntkFunc );
    Vec_PtrForEachEntry( Abc_Ntk_t *, pNtkBase->pDesign->vModules, pTemp, i )
        pTemp->pCopy->pAltView = pTemp->pAltView ? pTemp->pAltView->pCopy : NULL;
    // update box models
    Vec_PtrForEachEntry( Abc_Ntk_t *, pNtkBase->pDesign->vModules, pTemp, i )
        Abc_NtkForEachBox( pTemp, pObj, k )
            if ( Abc_ObjIsWhitebox(pObj) || Abc_ObjIsBlackbox(pObj) )
                pObj->pCopy->pData = Abc_ObjModel(pObj)->pCopy;
    // create the design
    pNtkNew = pNtkBase->pCopy;
    pNtkNew->pDesign = Abc_DesCreate( pNtkBase->pDesign->pName );
    Vec_PtrForEachEntry( Abc_Ntk_t *, pNtkBase->pDesign->vModules, pTemp, i )
        Abc_DesAddModel( pNtkNew->pDesign, pTemp->pCopy );
    Vec_PtrForEachEntry( Abc_Ntk_t *, pNtkBase->pDesign->vTops, pTemp, i )
        Vec_PtrPush( pNtkNew->pDesign->vTops, pTemp->pCopy );
    assert( Vec_PtrEntry(pNtkNew->pDesign->vTops, 0) == pNtkNew );
    // transfer copy attributes to pNtk
    Abc_NtkCleanCopy( pNtk );
    Abc_NtkForEachPi( pNtk, pObj, i )
        pObj->pCopy = Abc_NtkPi(pNtkNew, i);
    Abc_NtkForEachPo( pNtk, pObj, i )
        pObj->pCopy = Abc_NtkPo(pNtkNew, i);
    Abc_NtkCollectPiPos( pNtkBase, &vLiMaps, &vLoMaps );
    assert( Vec_PtrSize(vLiMaps) == Abc_NtkLatchNum(pNtk) );
    assert( Vec_PtrSize(vLoMaps) == Abc_NtkLatchNum(pNtk) );
    Vec_PtrForEachEntryTwo( Abc_Obj_t *, vLiMaps, Abc_Obj_t *, vLoMaps, pLiMap, pLoMap, i )
    {
        pObj = Abc_NtkBox( pNtk, i );
        Abc_ObjFanin0(pObj)->pCopy = pLiMap->pCopy; 
        Abc_ObjFanout0(pObj)->pCopy = pLoMap->pCopy; 
    }
    Vec_PtrFree( vLiMaps );
    Vec_PtrFree( vLoMaps );
    // create internal nodes
    Abc_NtkForEachCo( pNtk, pObj, i )
        Abc_ObjAddFanin( pObj->pCopy, Abc_NtkFromBarBufs_rec(pObj->pCopy->pNtk, Abc_ObjFanin0(pObj)) );
    // transfer net names
    Abc_NtkForEachCi( pNtk, pObj, i )
    {
        if ( Abc_ObjFanoutNum(pObj->pCopy) == 0 ) // handle PI without fanout
            Abc_ObjAddFanin( Abc_NtkCreateNet(pObj->pCopy->pNtk), pObj->pCopy );
        Nm_ManStoreIdName( pObj->pCopy->pNtk->pManName, Abc_ObjFanout0(pObj->pCopy)->Id, Abc_ObjFanout0(pObj->pCopy)->Type, Abc_ObjName(Abc_ObjFanout0(pObj)), NULL );
    }
    Abc_NtkForEachCo( pNtk, pObj, i )
        Nm_ManStoreIdName( pObj->pCopy->pNtk->pManName, Abc_ObjFanin0(pObj->pCopy)->Id, Abc_ObjFanin0(pObj->pCopy)->Type, Abc_ObjName(Abc_ObjFanin0(pObj)), NULL );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Collect nodes in the barbuf-friendly order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkToBarBufsCollect_rec( Abc_Obj_t * pObj, Vec_Ptr_t * vNodes )
{
    Abc_Obj_t * pFanin; 
    int i;
    if ( Abc_NodeIsTravIdCurrent( pObj ) )
        return;
    Abc_NodeSetTravIdCurrent( pObj );
    assert( Abc_ObjIsNode(pObj) );
    Abc_ObjForEachFanin( pObj, pFanin, i )
        Abc_NtkToBarBufsCollect_rec( pFanin, vNodes );
    Vec_PtrPush( vNodes, pObj );
}
Vec_Ptr_t * Abc_NtkToBarBufsCollect( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj;
    int i;
    assert( Abc_NtkIsLogic(pNtk) );
    assert( pNtk->nBarBufs > 0 );
    assert( pNtk->nBarBufs == Abc_NtkLatchNum(pNtk) );
    vNodes = Vec_PtrAlloc( Abc_NtkObjNum(pNtk) );
    Abc_NtkIncrementTravId( pNtk );
    Abc_NtkForEachCi( pNtk, pObj, i )
    {
        if ( i >= Abc_NtkCiNum(pNtk) - pNtk->nBarBufs )
            break;
        Vec_PtrPush( vNodes, pObj );
        Abc_NodeSetTravIdCurrent( pObj );
    }
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        if ( i < Abc_NtkCoNum(pNtk) - pNtk->nBarBufs )
            continue;
        Abc_NtkToBarBufsCollect_rec( Abc_ObjFanin0(pObj), vNodes );
        Vec_PtrPush( vNodes, pObj );
        Vec_PtrPush( vNodes, Abc_ObjFanout0(pObj) );
        Vec_PtrPush( vNodes, Abc_ObjFanout0(Abc_ObjFanout0(pObj)) );
        Abc_NodeSetTravIdCurrent( pObj );
        Abc_NodeSetTravIdCurrent( Abc_ObjFanout0(pObj) );
        Abc_NodeSetTravIdCurrent( Abc_ObjFanout0(Abc_ObjFanout0(pObj)) );
    }
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        if ( i >= Abc_NtkCoNum(pNtk) - pNtk->nBarBufs )
            break;
        Abc_NtkToBarBufsCollect_rec( Abc_ObjFanin0(pObj), vNodes );
        Vec_PtrPush( vNodes, pObj );
        Abc_NodeSetTravIdCurrent( pObj );
    }
    assert( Vec_PtrSize(vNodes) == Abc_NtkObjNum(pNtk) );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Count barrier buffers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCountBarBufs( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i, Counter = 0;
    Abc_NtkForEachNode( pNtk, pObj, i )
        Counter += Abc_ObjIsBarBuf( pObj );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Converts the network to dedicated barbufs and back.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkBarBufsToBuffers( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vNodes;
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObj, * pFanin;
    int i, k;
    assert( Abc_NtkIsLogic(pNtk) );
    assert( pNtk->pDesign == NULL );
    assert( pNtk->nBarBufs > 0 );
    assert( pNtk->nBarBufs == Abc_NtkLatchNum(pNtk) );
    vNodes = Abc_NtkToBarBufsCollect( pNtk );
    // start the network
    pNtkNew = Abc_NtkAlloc( ABC_NTK_LOGIC, pNtk->ntkFunc, 1 );
    pNtkNew->pName = Extra_UtilStrsav(pNtk->pName);
    pNtkNew->pSpec = Extra_UtilStrsav(pNtk->pSpec);
    // create objects
    Abc_NtkCleanCopy( pNtk );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
    {
        if ( Abc_ObjIsPi(pObj) )
            Abc_NtkDupObj( pNtkNew, pObj, 1 );
        else if ( Abc_ObjIsPo( pObj) )
            Abc_ObjAddFanin( Abc_NtkDupObj(pNtkNew, pObj, 1), Abc_ObjFanin0(pObj)->pCopy );
        else if ( Abc_ObjIsBi(pObj) || Abc_ObjIsBo(pObj) )
            pObj->pCopy = Abc_ObjFanin0(pObj)->pCopy;
        else if ( Abc_ObjIsLatch(pObj) )
            Abc_ObjAddFanin( (pObj->pCopy = Abc_NtkCreateNode(pNtkNew)), Abc_ObjFanin0(pObj)->pCopy );
        else if ( Abc_ObjIsNode(pObj) )
        {
            Abc_NtkDupObj( pNtkNew, pObj, 1 );
            Abc_ObjForEachFanin( pObj, pFanin, k )
                Abc_ObjAddFanin( pObj->pCopy, pFanin->pCopy );
        }
        else assert( 0 );
    }
    Vec_PtrFree( vNodes );
    return pNtkNew;
}
Abc_Ntk_t * Abc_NtkBarBufsFromBuffers( Abc_Ntk_t * pNtkBase, Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObj, * pFanin, * pLatch;
    int i, k, nBarBufs;
    assert( Abc_NtkIsLogic(pNtkBase) );
    assert( Abc_NtkIsLogic(pNtk) );
    assert( pNtkBase->nBarBufs == Abc_NtkLatchNum(pNtkBase) );
    // start the network
    pNtkNew = Abc_NtkStartFrom( pNtkBase, pNtk->ntkType, pNtk->ntkFunc );
    // transfer PI pointers
    Abc_NtkForEachPi( pNtk, pObj, i )
        pObj->pCopy = Abc_NtkPi(pNtkNew, i);
    // assuming that the order/number of barbufs remains the same
    nBarBufs = 0;
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        if ( Abc_ObjIsBarBuf(pObj) )
        {
            pLatch = Abc_NtkBox(pNtkNew, nBarBufs++);
            Abc_ObjAddFanin( Abc_ObjFanin0(pLatch), Abc_ObjFanin0(pObj)->pCopy );
            pObj->pCopy = Abc_ObjFanout0(pLatch);
        }
        else
        {
            Abc_NtkDupObj( pNtkNew, pObj, 1 );
            Abc_ObjForEachFanin( pObj, pFanin, k )
                Abc_ObjAddFanin( pObj->pCopy, pFanin->pCopy );
        }
    }
    assert( nBarBufs == pNtkBase->nBarBufs );
    // connect POs
    Abc_NtkForEachPo( pNtk, pObj, i )
        Abc_ObjAddFanin( Abc_NtkPo(pNtkNew, i), Abc_ObjFanin0(pObj)->pCopy );
    return pNtkNew;
}
Abc_Ntk_t * Abc_NtkBarBufsOnOffTest( Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pNtkNew, * pNtkNew2;
    pNtkNew  = Abc_NtkBarBufsToBuffers( pNtk );
    pNtkNew2 = Abc_NtkBarBufsFromBuffers( pNtk, pNtkNew );
    Abc_NtkDelete( pNtkNew );
    return pNtkNew2;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

