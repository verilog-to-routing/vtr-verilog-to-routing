/**CFile****************************************************************

  FileName    [abcObj.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Object creation/duplication/deletion procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcObj.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abc.h"
#include "abcInt.h"
#include "base/main/main.h"
#include "map/mio/mio.h"

#ifdef ABC_USE_CUDD
#include "bdd/extrab/extraBdd.h"
#endif

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates a new object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_ObjAlloc( Abc_Ntk_t * pNtk, Abc_ObjType_t Type )
{
    Abc_Obj_t * pObj;
    if ( pNtk->pMmObj )
        pObj = (Abc_Obj_t *)Mem_FixedEntryFetch( pNtk->pMmObj );
    else
        pObj = (Abc_Obj_t *)ABC_ALLOC( Abc_Obj_t, 1 );
    memset( pObj, 0, sizeof(Abc_Obj_t) );
    pObj->pNtk = pNtk;
    pObj->Type = Type;
    pObj->Id   = -1;
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Recycles the object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ObjRecycle( Abc_Obj_t * pObj )
{
    Abc_Ntk_t * pNtk = pObj->pNtk;
//    int LargePiece = (4 << ABC_NUM_STEPS);
    // free large fanout arrays
//    if ( pNtk->pMmStep && pObj->vFanouts.nCap * 4 > LargePiece )
//        free( pObj->vFanouts.pArray );
    if ( pNtk->pMmStep == NULL )
    {
        ABC_FREE( pObj->vFanouts.pArray );
        ABC_FREE( pObj->vFanins.pArray );
    }
    // clean the memory to make deleted object distinct from the live one
    memset( pObj, 0, sizeof(Abc_Obj_t) );
    // recycle the object
    if ( pNtk->pMmObj )
        Mem_FixedEntryRecycle( pNtk->pMmObj, (char *)pObj );
    else
        ABC_FREE( pObj );
}

/**Function*************************************************************

  Synopsis    [Adds the node to the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkCreateObj( Abc_Ntk_t * pNtk, Abc_ObjType_t Type )
{
    Abc_Obj_t * pObj;
    // create new object, assign ID, and add to the array
    pObj = Abc_ObjAlloc( pNtk, Type );
    pObj->Id = pNtk->vObjs->nSize;
    Vec_PtrPush( pNtk->vObjs, pObj );
    pNtk->nObjCounts[Type]++;
    pNtk->nObjs++;
    // perform specialized operations depending on the object type
    switch (Type)
    {
        case ABC_OBJ_NONE:   
            assert(0); 
            break;
        case ABC_OBJ_CONST1: 
            assert(0); 
            break;
        case ABC_OBJ_PI:
//            pObj->iTemp = Vec_PtrSize(pNtk->vCis);
            Vec_PtrPush( pNtk->vPis, pObj );
            Vec_PtrPush( pNtk->vCis, pObj );
            break;
        case ABC_OBJ_PO:     
//            pObj->iTemp = Vec_PtrSize(pNtk->vCos);
            Vec_PtrPush( pNtk->vPos, pObj );
            Vec_PtrPush( pNtk->vCos, pObj );
            break;
        case ABC_OBJ_BI:     
            if ( pNtk->vCos ) Vec_PtrPush( pNtk->vCos, pObj );
            break;
        case ABC_OBJ_BO:     
            if ( pNtk->vCis ) Vec_PtrPush( pNtk->vCis, pObj );
            break;
        case ABC_OBJ_NET:  
        case ABC_OBJ_NODE: 
            break;
        case ABC_OBJ_LATCH:     
            pObj->pData = (void *)ABC_INIT_NONE;
        case ABC_OBJ_WHITEBOX:     
        case ABC_OBJ_BLACKBOX:     
            if ( pNtk->vBoxes ) Vec_PtrPush( pNtk->vBoxes, pObj );
            break;
        default:
            assert(0); 
            break;
    }
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Deletes the object from the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDeleteObj( Abc_Obj_t * pObj )
{
    Abc_Ntk_t * pNtk = pObj->pNtk;
    Vec_Ptr_t * vNodes;
    int i;
    assert( !Abc_ObjIsComplement(pObj) );
    // remove from the table of names
    if ( Nm_ManFindNameById(pObj->pNtk->pManName, pObj->Id) )
        Nm_ManDeleteIdName(pObj->pNtk->pManName, pObj->Id);
    // delete fanins and fanouts
    vNodes = Vec_PtrAlloc( 100 );
    Abc_NodeCollectFanouts( pObj, vNodes );
    for ( i = 0; i < vNodes->nSize; i++ )
        Abc_ObjDeleteFanin( (Abc_Obj_t *)vNodes->pArray[i], pObj );
    Abc_NodeCollectFanins( pObj, vNodes );
    for ( i = 0; i < vNodes->nSize; i++ )
        Abc_ObjDeleteFanin( pObj, (Abc_Obj_t *)vNodes->pArray[i] );
    Vec_PtrFree( vNodes );
    // remove from the list of objects
    Vec_PtrWriteEntry( pNtk->vObjs, pObj->Id, NULL );
    pObj->Id = (1<<26)-1;
    pNtk->nObjCounts[pObj->Type]--;
    pNtk->nObjs--;
    // perform specialized operations depending on the object type
    switch (pObj->Type)
    {
        case ABC_OBJ_NONE:   
            assert(0); 
            break;
        case ABC_OBJ_CONST1: 
            assert(0); 
            break;
        case ABC_OBJ_PI:     
            Vec_PtrRemove( pNtk->vPis, pObj );
            Vec_PtrRemove( pNtk->vCis, pObj );
            break;
        case ABC_OBJ_PO:     
            Vec_PtrRemove( pNtk->vPos, pObj );
            Vec_PtrRemove( pNtk->vCos, pObj );
            break;
        case ABC_OBJ_BI:     
            if ( pNtk->vCos ) Vec_PtrRemove( pNtk->vCos, pObj );
            break;
        case ABC_OBJ_BO:     
            if ( pNtk->vCis ) Vec_PtrRemove( pNtk->vCis, pObj );
            break;
        case ABC_OBJ_NET:  
            break;
        case ABC_OBJ_NODE: 
#ifdef ABC_USE_CUDD
            if ( Abc_NtkHasBdd(pNtk) )
                Cudd_RecursiveDeref( (DdManager *)pNtk->pManFunc, (DdNode *)pObj->pData );
#endif
            pObj->pData = NULL;
            break;
        case ABC_OBJ_LATCH:     
        case ABC_OBJ_WHITEBOX:     
        case ABC_OBJ_BLACKBOX:     
            if ( pNtk->vBoxes ) Vec_PtrRemove( pNtk->vBoxes, pObj );
            break;
        default:
            assert(0); 
            break;
    }
    // recycle the object memory
    Abc_ObjRecycle( pObj );
}

/**Function*************************************************************

  Synopsis    [Deletes the PO from the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDeleteObjPo( Abc_Obj_t * pObj )
{
    assert( Abc_ObjIsPo(pObj) );
    // remove from the table of names
    if ( Nm_ManFindNameById(pObj->pNtk->pManName, pObj->Id) )
        Nm_ManDeleteIdName(pObj->pNtk->pManName, pObj->Id);
    // delete fanins
    Abc_ObjDeleteFanin( pObj, Abc_ObjFanin0(pObj) );
    // remove from the list of objects
    Vec_PtrWriteEntry( pObj->pNtk->vObjs, pObj->Id, NULL );
    pObj->Id = (1<<26)-1;
    pObj->pNtk->nObjCounts[pObj->Type]--;
    pObj->pNtk->nObjs--;
    // recycle the object memory
    Abc_ObjRecycle( pObj );
}


/**Function*************************************************************

  Synopsis    [Deletes the node and MFFC of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDeleteObj_rec( Abc_Obj_t * pObj, int fOnlyNodes )
{
    Vec_Ptr_t * vNodes;
    int i;
    assert( !Abc_ObjIsComplement(pObj) );
    assert( !Abc_ObjIsPi(pObj) );
    assert( Abc_ObjFanoutNum(pObj) == 0 );
    // delete fanins and fanouts
    vNodes = Vec_PtrAlloc( 100 );
    Abc_NodeCollectFanins( pObj, vNodes );
    Abc_NtkDeleteObj( pObj );
    if ( fOnlyNodes )
    {
        Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
            if ( Abc_ObjIsNode(pObj) && Abc_ObjFanoutNum(pObj) == 0 )
                Abc_NtkDeleteObj_rec( pObj, fOnlyNodes );
    }
    else
    {
        Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
            if ( !Abc_ObjIsPi(pObj) && Abc_ObjFanoutNum(pObj) == 0 )
                Abc_NtkDeleteObj_rec( pObj, fOnlyNodes );
    }
    Vec_PtrFree( vNodes );
}

/**Function*************************************************************

  Synopsis    [Deletes the node and MFFC of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDeleteAll_rec( Abc_Obj_t * pObj )
{
    Vec_Ptr_t * vNodes;
    int i;
    assert( !Abc_ObjIsComplement(pObj) );
    assert( Abc_ObjFanoutNum(pObj) == 0 );
    // delete fanins and fanouts
    vNodes = Vec_PtrAlloc( 100 );
    Abc_NodeCollectFanins( pObj, vNodes );
    Abc_NtkDeleteObj( pObj );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
        if ( !Abc_ObjIsNode(pObj) && Abc_ObjFanoutNum(pObj) == 0 )
            Abc_NtkDeleteAll_rec( pObj );
    Vec_PtrFree( vNodes );
}

/**Function*************************************************************

  Synopsis    [Duplicate the Obj.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkDupObj( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pObj, int fCopyName )
{
    Abc_Obj_t * pObjNew;
    // create the new object
    pObjNew = Abc_NtkCreateObj( pNtkNew, (Abc_ObjType_t)pObj->Type );
    // transfer names of the terminal objects
    if ( fCopyName )
    {
        if ( Abc_ObjIsCi(pObj) )
        {
            if ( !Abc_NtkIsNetlist(pNtkNew) )
                Abc_ObjAssignName( pObjNew, Abc_ObjName(Abc_ObjFanout0Ntk(pObj)), NULL );
        }
        else if ( Abc_ObjIsCo(pObj) )
        {
            if ( !Abc_NtkIsNetlist(pNtkNew) )
            {
                if ( Abc_ObjIsPo(pObj) )
                    Abc_ObjAssignName( pObjNew, Abc_ObjName(Abc_ObjFanin0Ntk(pObj)), NULL );
                else
                {
                    assert( Abc_ObjIsLatch(Abc_ObjFanout0(pObj)) );
                    Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), NULL );
                }
            }
        }
        else if ( Abc_ObjIsBox(pObj) || Abc_ObjIsNet(pObj) )
            Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), NULL );
    }
    // copy functionality/names
    if ( Abc_ObjIsNode(pObj) ) // copy the function if functionality is compatible
    {
        if ( pNtkNew->ntkFunc == pObj->pNtk->ntkFunc ) 
        {
            if ( Abc_NtkIsStrash(pNtkNew) ) 
            {}
            else if ( Abc_NtkHasSop(pNtkNew) || Abc_NtkHasBlifMv(pNtkNew) )
                pObjNew->pData = Abc_SopRegister( (Mem_Flex_t *)pNtkNew->pManFunc, (char *)pObj->pData );
#ifdef ABC_USE_CUDD
            else if ( Abc_NtkHasBdd(pNtkNew) )
                pObjNew->pData = Cudd_bddTransfer((DdManager *)pObj->pNtk->pManFunc, (DdManager *)pNtkNew->pManFunc, (DdNode *)pObj->pData), Cudd_Ref((DdNode *)pObjNew->pData);
#endif
            else if ( Abc_NtkHasAig(pNtkNew) )
                pObjNew->pData = Hop_Transfer((Hop_Man_t *)pObj->pNtk->pManFunc, (Hop_Man_t *)pNtkNew->pManFunc, (Hop_Obj_t *)pObj->pData, Abc_ObjFaninNum(pObj));
            else if ( Abc_NtkHasMapping(pNtkNew) )
                pObjNew->pData = pObj->pData, pNtkNew->nBarBufs2 += !pObj->pData;
            else assert( 0 );
        }
    }
    else if ( Abc_ObjIsNet(pObj) ) // copy the name
    {
    }
    else if ( Abc_ObjIsLatch(pObj) ) // copy the reset value
        pObjNew->pData = pObj->pData;
    // transfer HAIG
//    pObjNew->pEquiv = pObj->pEquiv;
    // remember the new node in the old node
    pObj->pCopy = pObjNew;
    return pObjNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates the latch with its input/output terminals.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkDupBox( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pBox, int fCopyName )
{
    Abc_Obj_t * pTerm, * pBoxNew;
    int i;
    assert( Abc_ObjIsBox(pBox) );
    // duplicate the box 
    pBoxNew = Abc_NtkDupObj( pNtkNew, pBox, fCopyName );
    // duplicate the fanins and connect them
    Abc_ObjForEachFanin( pBox, pTerm, i )
        Abc_ObjAddFanin( pBoxNew, Abc_NtkDupObj(pNtkNew, pTerm, fCopyName) );
    // duplicate the fanouts and connect them
    Abc_ObjForEachFanout( pBox, pTerm, i )
        Abc_ObjAddFanin( Abc_NtkDupObj(pNtkNew, pTerm, fCopyName), pBoxNew );
    return pBoxNew;
}

/**Function*************************************************************

  Synopsis    [Clones the objects in the same network but does not assign its function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkCloneObj( Abc_Obj_t * pObj )
{
    Abc_Obj_t * pClone, * pFanin;
    int i;
    pClone = Abc_NtkCreateObj( pObj->pNtk, (Abc_ObjType_t)pObj->Type );   
    Abc_ObjForEachFanin( pObj, pFanin, i )
        Abc_ObjAddFanin( pClone, pFanin );
    return pClone;
}


/**Function*************************************************************

  Synopsis    [Returns the net with the given name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkFindNode( Abc_Ntk_t * pNtk, char * pName )
{
    Abc_Obj_t * pObj;
    int Num;
    // try to find the terminal
    Num = Nm_ManFindIdByName( pNtk->pManName, pName, ABC_OBJ_PO );
    if ( Num >= 0 )
        return Abc_ObjFanin0( Abc_NtkObj( pNtk, Num ) );
    Num = Nm_ManFindIdByName( pNtk->pManName, pName, ABC_OBJ_BI );
    if ( Num >= 0 )
        return Abc_ObjFanin0( Abc_NtkObj( pNtk, Num ) );
    Num = Nm_ManFindIdByName( pNtk->pManName, pName, ABC_OBJ_NODE );
    if ( Num >= 0 )
        return Abc_NtkObj( pNtk, Num );
    // find the internal node
    if ( pName[0] != 'n' )
    {
        printf( "Name \"%s\" is not found among CO or node names (internal names often look as \"n<num>\").\n", pName );
        return NULL;
    }
    Num = atoi( pName + 1 );
    if ( Num < 0 || Num >= Abc_NtkObjNumMax(pNtk) )
    {
        printf( "The node \"%s\" with ID %d is not in the current network.\n", pName, Num );
        return NULL;
    }
    pObj = Abc_NtkObj( pNtk, Num );
    if ( pObj == NULL )
    {
        printf( "The node \"%s\" with ID %d has been removed from the current network.\n", pName, Num );
        return NULL;
    }
    if ( !Abc_ObjIsNode(pObj) )
    {
        printf( "Object with ID %d is not a node.\n", Num );
        return NULL;
    }
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Returns the net with the given name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkFindNet( Abc_Ntk_t * pNtk, char * pName )
{
    Abc_Obj_t * pNet;
    int ObjId;
    assert( Abc_NtkIsNetlist(pNtk) );
    ObjId = Nm_ManFindIdByName( pNtk->pManName, pName, ABC_OBJ_NET );
    if ( ObjId == -1 )
        return NULL;
    pNet = Abc_NtkObj( pNtk, ObjId );
    return pNet;
}

/**Function*************************************************************

 Synopsis    [Returns CI with the given name.]

 Description []
              
 SideEffects []

 SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkFindCi( Abc_Ntk_t * pNtk, char * pName )
{
    int Num;
    assert( !Abc_NtkIsNetlist(pNtk) );
    Num = Nm_ManFindIdByName( pNtk->pManName, pName, ABC_OBJ_PI );
    if ( Num >= 0 )
        return Abc_NtkObj( pNtk, Num );
    Num = Nm_ManFindIdByName( pNtk->pManName, pName, ABC_OBJ_BO );
    if ( Num >= 0 )
        return Abc_NtkObj( pNtk, Num );
    return NULL;
}

/**Function*************************************************************

 Synopsis    [Returns CO with the given name.]

 Description []
              
 SideEffects []

 SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkFindCo( Abc_Ntk_t * pNtk, char * pName )
{
    int Num;
    assert( !Abc_NtkIsNetlist(pNtk) );
    Num = Nm_ManFindIdByName( pNtk->pManName, pName, ABC_OBJ_PO );
    if ( Num >= 0 )
        return Abc_NtkObj( pNtk, Num );
    Num = Nm_ManFindIdByName( pNtk->pManName, pName, ABC_OBJ_BI );
    if ( Num >= 0 )
        return Abc_NtkObj( pNtk, Num );
    return NULL;
}


/**Function*************************************************************

  Synopsis    [Finds or creates the net.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkFindOrCreateNet( Abc_Ntk_t * pNtk, char * pName )
{
    Abc_Obj_t * pNet;
    assert( Abc_NtkIsNetlist(pNtk) );
    if ( pName && (pNet = Abc_NtkFindNet( pNtk, pName )) )
        return pNet;
//printf( "Creating net %s.\n", pName );
    // create a new net
    pNet = Abc_NtkCreateNet( pNtk );
    if ( pName )
        Nm_ManStoreIdName( pNtk->pManName, pNet->Id, pNet->Type, pName, NULL );
    return pNet;
}

/**Function*************************************************************

  Synopsis    [Creates constant 0 node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkCreateNodeConst0( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsNetlist(pNtk) );
    pNode = Abc_NtkCreateNode( pNtk );   
    if ( Abc_NtkHasSop(pNtk) || Abc_NtkHasBlifMv(pNtk) )
        pNode->pData = Abc_SopRegister( (Mem_Flex_t *)pNtk->pManFunc, " 0\n" );
#ifdef ABC_USE_CUDD
    else if ( Abc_NtkHasBdd(pNtk) )
        pNode->pData = Cudd_ReadLogicZero((DdManager *)pNtk->pManFunc), Cudd_Ref( (DdNode *)pNode->pData );
#endif
    else if ( Abc_NtkHasAig(pNtk) )
        pNode->pData = Hop_ManConst0((Hop_Man_t *)pNtk->pManFunc);
    else if ( Abc_NtkHasMapping(pNtk) )
        pNode->pData = Mio_LibraryReadConst0((Mio_Library_t *)Abc_FrameReadLibGen());
    else if ( !Abc_NtkHasBlackbox(pNtk) )
        assert( 0 );
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Creates constant 1 node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkCreateNodeConst1( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsNetlist(pNtk) );
    pNode = Abc_NtkCreateNode( pNtk );   
    if ( Abc_NtkHasSop(pNtk) || Abc_NtkHasBlifMv(pNtk) )
        pNode->pData = Abc_SopRegister( (Mem_Flex_t *)pNtk->pManFunc, " 1\n" );
#ifdef ABC_USE_CUDD
    else if ( Abc_NtkHasBdd(pNtk) )
        pNode->pData = Cudd_ReadOne((DdManager *)pNtk->pManFunc), Cudd_Ref( (DdNode *)pNode->pData );
#endif
    else if ( Abc_NtkHasAig(pNtk) )
        pNode->pData = Hop_ManConst1((Hop_Man_t *)pNtk->pManFunc);
    else if ( Abc_NtkHasMapping(pNtk) )
        pNode->pData = Mio_LibraryReadConst1((Mio_Library_t *)Abc_FrameReadLibGen());
    else if ( !Abc_NtkHasBlackbox(pNtk) )
        assert( 0 );
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Creates inverter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkCreateNodeInv( Abc_Ntk_t * pNtk, Abc_Obj_t * pFanin )
{
    Abc_Obj_t * pNode;
    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsNetlist(pNtk) );
    pNode = Abc_NtkCreateNode( pNtk );   
    if ( pFanin ) Abc_ObjAddFanin( pNode, pFanin );
    if ( Abc_NtkHasSop(pNtk) )
        pNode->pData = Abc_SopRegister( (Mem_Flex_t *)pNtk->pManFunc, "0 1\n" );
#ifdef ABC_USE_CUDD
    else if ( Abc_NtkHasBdd(pNtk) )
        pNode->pData = Cudd_Not(Cudd_bddIthVar((DdManager *)pNtk->pManFunc,0)), Cudd_Ref( (DdNode *)pNode->pData );
#endif
    else if ( Abc_NtkHasAig(pNtk) )
        pNode->pData = Hop_Not(Hop_IthVar((Hop_Man_t *)pNtk->pManFunc,0));
    else if ( Abc_NtkHasMapping(pNtk) )
        pNode->pData = Mio_LibraryReadInv((Mio_Library_t *)Abc_FrameReadLibGen());
    else
        assert( 0 );
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Creates buffer.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkCreateNodeBuf( Abc_Ntk_t * pNtk, Abc_Obj_t * pFanin )
{
    Abc_Obj_t * pNode;
    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsNetlist(pNtk) );
    pNode = Abc_NtkCreateNode( pNtk ); 
    if ( pFanin ) Abc_ObjAddFanin( pNode, pFanin );
    if ( Abc_NtkHasSop(pNtk) )
        pNode->pData = Abc_SopRegister( (Mem_Flex_t *)pNtk->pManFunc, "1 1\n" );
#ifdef ABC_USE_CUDD
    else if ( Abc_NtkHasBdd(pNtk) )
        pNode->pData = Cudd_bddIthVar((DdManager *)pNtk->pManFunc,0), Cudd_Ref( (DdNode *)pNode->pData );
#endif
    else if ( Abc_NtkHasAig(pNtk) )
        pNode->pData = Hop_IthVar((Hop_Man_t *)pNtk->pManFunc,0);
    else if ( Abc_NtkHasMapping(pNtk) )
        pNode->pData = Mio_LibraryReadBuf((Mio_Library_t *)Abc_FrameReadLibGen());
    else
        assert( 0 );
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Creates AND.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkCreateNodeAnd( Abc_Ntk_t * pNtk, Vec_Ptr_t * vFanins )
{
    Abc_Obj_t * pNode;
    int i;
    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsNetlist(pNtk) );
    pNode = Abc_NtkCreateNode( pNtk );   
    for ( i = 0; i < vFanins->nSize; i++ )
        Abc_ObjAddFanin( pNode, (Abc_Obj_t *)vFanins->pArray[i] );
    if ( Abc_NtkHasSop(pNtk) )
        pNode->pData = Abc_SopCreateAnd( (Mem_Flex_t *)pNtk->pManFunc, Vec_PtrSize(vFanins), NULL );
#ifdef ABC_USE_CUDD
    else if ( Abc_NtkHasBdd(pNtk) )
        pNode->pData = Extra_bddCreateAnd( (DdManager *)pNtk->pManFunc, Vec_PtrSize(vFanins) ), Cudd_Ref((DdNode *)pNode->pData); 
#endif
    else if ( Abc_NtkHasAig(pNtk) )
        pNode->pData = Hop_CreateAnd( (Hop_Man_t *)pNtk->pManFunc, Vec_PtrSize(vFanins) ); 
    else
        assert( 0 );
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Creates OR.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkCreateNodeOr( Abc_Ntk_t * pNtk, Vec_Ptr_t * vFanins )
{
    Abc_Obj_t * pNode;
    int i;
    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsNetlist(pNtk) );
    pNode = Abc_NtkCreateNode( pNtk );   
    for ( i = 0; i < vFanins->nSize; i++ )
        Abc_ObjAddFanin( pNode, (Abc_Obj_t *)vFanins->pArray[i] );
    if ( Abc_NtkHasSop(pNtk) )
        pNode->pData = Abc_SopCreateOr( (Mem_Flex_t *)pNtk->pManFunc, Vec_PtrSize(vFanins), NULL );
#ifdef ABC_USE_CUDD
    else if ( Abc_NtkHasBdd(pNtk) )
        pNode->pData = Extra_bddCreateOr( (DdManager *)pNtk->pManFunc, Vec_PtrSize(vFanins) ), Cudd_Ref((DdNode *)pNode->pData); 
#endif
    else if ( Abc_NtkHasAig(pNtk) )
        pNode->pData = Hop_CreateOr( (Hop_Man_t *)pNtk->pManFunc, Vec_PtrSize(vFanins) ); 
    else
        assert( 0 );
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Creates EXOR.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkCreateNodeExor( Abc_Ntk_t * pNtk, Vec_Ptr_t * vFanins )
{
    Abc_Obj_t * pNode;
    int i;
    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsNetlist(pNtk) );
    pNode = Abc_NtkCreateNode( pNtk );   
    for ( i = 0; i < vFanins->nSize; i++ )
        Abc_ObjAddFanin( pNode, (Abc_Obj_t *)vFanins->pArray[i] );
    if ( Abc_NtkHasSop(pNtk) )
        pNode->pData = Abc_SopCreateXorSpecial( (Mem_Flex_t *)pNtk->pManFunc, Vec_PtrSize(vFanins) );
#ifdef ABC_USE_CUDD
    else if ( Abc_NtkHasBdd(pNtk) )
        pNode->pData = Extra_bddCreateExor( (DdManager *)pNtk->pManFunc, Vec_PtrSize(vFanins) ), Cudd_Ref((DdNode *)pNode->pData); 
#endif
    else if ( Abc_NtkHasAig(pNtk) )
        pNode->pData = Hop_CreateExor( (Hop_Man_t *)pNtk->pManFunc, Vec_PtrSize(vFanins) ); 
    else
        assert( 0 );
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Creates MUX.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkCreateNodeMux( Abc_Ntk_t * pNtk, Abc_Obj_t * pNodeC, Abc_Obj_t * pNode1, Abc_Obj_t * pNode0 )
{
    Abc_Obj_t * pNode;
    assert( Abc_NtkIsLogic(pNtk) );
    pNode = Abc_NtkCreateNode( pNtk );   
    Abc_ObjAddFanin( pNode, pNodeC );
    Abc_ObjAddFanin( pNode, pNode1 );
    Abc_ObjAddFanin( pNode, pNode0 );
    if ( Abc_NtkHasSop(pNtk) )
        pNode->pData = Abc_SopRegister( (Mem_Flex_t *)pNtk->pManFunc, "11- 1\n0-1 1\n" );
#ifdef ABC_USE_CUDD
    else if ( Abc_NtkHasBdd(pNtk) )
        pNode->pData = Cudd_bddIte((DdManager *)pNtk->pManFunc,Cudd_bddIthVar((DdManager *)pNtk->pManFunc,0),Cudd_bddIthVar((DdManager *)pNtk->pManFunc,1),Cudd_bddIthVar((DdManager *)pNtk->pManFunc,2)), Cudd_Ref( (DdNode *)pNode->pData );
#endif
    else if ( Abc_NtkHasAig(pNtk) )
        pNode->pData = Hop_Mux((Hop_Man_t *)pNtk->pManFunc,Hop_IthVar((Hop_Man_t *)pNtk->pManFunc,0),Hop_IthVar((Hop_Man_t *)pNtk->pManFunc,1),Hop_IthVar((Hop_Man_t *)pNtk->pManFunc,2));
    else
        assert( 0 );
    return pNode;
}


/**Function*************************************************************

  Synopsis    [Returns 1 if the node is a constant 0 node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeIsConst( Abc_Obj_t * pNode )    
{ 
    assert( Abc_NtkIsLogic(pNode->pNtk) || Abc_NtkIsNetlist(pNode->pNtk) );
    return Abc_ObjIsNode(pNode) && Abc_ObjFaninNum(pNode) == 0;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the node is a constant 0 node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeIsConst0( Abc_Obj_t * pNode )    
{ 
    Abc_Ntk_t * pNtk = pNode->pNtk;
    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsNetlist(pNtk) );
    assert( Abc_ObjIsNode(pNode) );      
    if ( !Abc_NodeIsConst(pNode) )
        return 0;
    if ( Abc_NtkHasSop(pNtk) )
        return Abc_SopIsConst0((char *)pNode->pData);
#ifdef ABC_USE_CUDD
    if ( Abc_NtkHasBdd(pNtk) )
        return Cudd_IsComplement(pNode->pData);
#endif
    if ( Abc_NtkHasAig(pNtk) )
        return Hop_IsComplement((Hop_Obj_t *)pNode->pData)? 1:0;
    if ( Abc_NtkHasMapping(pNtk) )
        return pNode->pData == Mio_LibraryReadConst0((Mio_Library_t *)Abc_FrameReadLibGen());
    assert( 0 );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the node is a constant 1 node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeIsConst1( Abc_Obj_t * pNode )    
{ 
    Abc_Ntk_t * pNtk = pNode->pNtk;
    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsNetlist(pNtk) );
    assert( Abc_ObjIsNode(pNode) );      
    if ( !Abc_NodeIsConst(pNode) )
        return 0;
    if ( Abc_NtkHasSop(pNtk) )
        return Abc_SopIsConst1((char *)pNode->pData);
#ifdef ABC_USE_CUDD
    if ( Abc_NtkHasBdd(pNtk) )
        return !Cudd_IsComplement(pNode->pData);
#endif
    if ( Abc_NtkHasAig(pNtk) )
        return !Hop_IsComplement((Hop_Obj_t *)pNode->pData);
    if ( Abc_NtkHasMapping(pNtk) )
        return pNode->pData == Mio_LibraryReadConst1((Mio_Library_t *)Abc_FrameReadLibGen());
    assert( 0 );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the node is a buffer.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeIsBuf( Abc_Obj_t * pNode )    
{ 
    Abc_Ntk_t * pNtk = pNode->pNtk;
    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsNetlist(pNtk) );
    assert( Abc_ObjIsNode(pNode) ); 
    if ( Abc_ObjFaninNum(pNode) != 1 )
        return 0;
    if ( Abc_NtkHasSop(pNtk) )
        return Abc_SopIsBuf((char *)pNode->pData);
#ifdef ABC_USE_CUDD
    if ( Abc_NtkHasBdd(pNtk) )
        return !Cudd_IsComplement(pNode->pData);
#endif
    if ( Abc_NtkHasAig(pNtk) )
        return !Hop_IsComplement((Hop_Obj_t *)pNode->pData);
    if ( Abc_NtkHasMapping(pNtk) )
        return pNode->pData == Mio_LibraryReadBuf((Mio_Library_t *)Abc_FrameReadLibGen());
    assert( 0 );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the node is an inverter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeIsInv( Abc_Obj_t * pNode )    
{ 
    Abc_Ntk_t * pNtk = pNode->pNtk;
    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsNetlist(pNtk) );
    assert( Abc_ObjIsNode(pNode) ); 
    if ( Abc_ObjFaninNum(pNode) != 1 )
        return 0;
    if ( Abc_NtkHasSop(pNtk) )
        return Abc_SopIsInv((char *)pNode->pData);
#ifdef ABC_USE_CUDD
    if ( Abc_NtkHasBdd(pNtk) )
        return Cudd_IsComplement(pNode->pData);
#endif
    if ( Abc_NtkHasAig(pNtk) )
        return Hop_IsComplement((Hop_Obj_t *)pNode->pData)? 1:0;
    if ( Abc_NtkHasMapping(pNtk) )
        return pNode->pData == Mio_LibraryReadInv((Mio_Library_t *)Abc_FrameReadLibGen());
    assert( 0 );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Complements the local functions of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeComplement( Abc_Obj_t * pNode )
{
    assert( Abc_NtkIsLogic(pNode->pNtk) || Abc_NtkIsNetlist(pNode->pNtk) );
    assert( Abc_ObjIsNode(pNode) ); 
    if ( Abc_NtkHasSop(pNode->pNtk) )
        Abc_SopComplement( (char *)pNode->pData );
    else if ( Abc_NtkHasAig(pNode->pNtk) )
        pNode->pData = Hop_Not( (Hop_Obj_t *)pNode->pData );
#ifdef ABC_USE_CUDD
    else if ( Abc_NtkHasBdd(pNode->pNtk) )
        pNode->pData = Cudd_Not( pNode->pData );
#endif
    else
        assert( 0 );
}

/**Function*************************************************************

  Synopsis    [Changes the polarity of one fanin.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeComplementInput( Abc_Obj_t * pNode, Abc_Obj_t * pFanin )
{
    int iFanin;
    if ( (iFanin = Vec_IntFind( &pNode->vFanins, pFanin->Id )) == -1 )
    {
        printf( "Node %s should be among", Abc_ObjName(pFanin) );
        printf( " the fanins of node %s...\n", Abc_ObjName(pNode) );
        return;
    }
    if ( Abc_NtkHasSop(pNode->pNtk) )
        Abc_SopComplementVar( (char *)pNode->pData, iFanin );
    else if ( Abc_NtkHasAig(pNode->pNtk) )
        pNode->pData = Hop_Complement( (Hop_Man_t *)pNode->pNtk->pManFunc, (Hop_Obj_t *)pNode->pData, iFanin );
#ifdef ABC_USE_CUDD
    else if ( Abc_NtkHasBdd(pNode->pNtk) )
    {
        DdManager * dd = (DdManager *)pNode->pNtk->pManFunc;
        DdNode * bVar, * bCof0, * bCof1;
        bVar = Cudd_bddIthVar( dd, iFanin );
        bCof0 = Cudd_Cofactor( dd, (DdNode *)pNode->pData, Cudd_Not(bVar) );   Cudd_Ref( bCof0 );
        bCof1 = Cudd_Cofactor( dd, (DdNode *)pNode->pData, bVar );             Cudd_Ref( bCof1 );
        Cudd_RecursiveDeref( dd, (DdNode *)pNode->pData );
        pNode->pData = Cudd_bddIte( dd, bVar, bCof0, bCof1 );        Cudd_Ref( (DdNode *)pNode->pData );
        Cudd_RecursiveDeref( dd, bCof0 );
        Cudd_RecursiveDeref( dd, bCof1 );
    }
#endif
    else
        assert( 0 );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END

