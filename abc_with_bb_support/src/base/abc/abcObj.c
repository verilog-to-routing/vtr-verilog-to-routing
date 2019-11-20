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
#include "main.h"
#include "mio.h"

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
        pObj = (Abc_Obj_t *)Extra_MmFixedEntryFetch( pNtk->pMmObj );
    else
        pObj = (Abc_Obj_t *)ALLOC( Abc_Obj_t, 1 );
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
    int LargePiece = (4 << ABC_NUM_STEPS);
    // free large fanout arrays
    if ( pNtk->pMmStep && pObj->vFanouts.nCap * 4 > LargePiece )
        FREE( pObj->vFanouts.pArray );
    if ( pNtk->pMmStep == NULL )
    {
        FREE( pObj->vFanouts.pArray );
        FREE( pObj->vFanins.pArray );
    }
    // clean the memory to make deleted object distinct from the live one
    memset( pObj, 0, sizeof(Abc_Obj_t) );
    // recycle the object
    if ( pNtk->pMmObj )
        Extra_MmFixedEntryRecycle( pNtk->pMmObj, (char *)pObj );
    else
        free( pObj );
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
	Abc_LatchInfo_t * pLatchInfo;

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
        case ABC_OBJ_PIO:    
            assert(0); 
            break;
        case ABC_OBJ_PI:     
            Vec_PtrPush( pNtk->vPis, pObj );
            Vec_PtrPush( pNtk->vCis, pObj );
            break;
        case ABC_OBJ_PO:     
            Vec_PtrPush( pNtk->vPos, pObj );
            Vec_PtrPush( pNtk->vCos, pObj );
            break;
        case ABC_OBJ_BI:     
            if ( pNtk->vCos ) Vec_PtrPush( pNtk->vCos, pObj );
            break;
        case ABC_OBJ_BO:     
            if ( pNtk->vCis ) Vec_PtrPush( pNtk->vCis, pObj );
            break;
        case ABC_OBJ_ASSERT:     
            Vec_PtrPush( pNtk->vAsserts, pObj );
            Vec_PtrPush( pNtk->vCos, pObj );
            break;
        case ABC_OBJ_NET:  
        case ABC_OBJ_NODE: 
            break;
        case ABC_OBJ_LATCH:     
            pLatchInfo = (Abc_LatchInfo_t *)ALLOC( Abc_LatchInfo_t, 1);
			pLatchInfo->InitValue = ABC_INIT_NONE;
			pObj->pData = (void *)pLatchInfo;
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
	Abc_LatchInfo_t * pLatchInfo;
    int i;
    assert( !Abc_ObjIsComplement(pObj) );
    // remove from the table of names
    if ( Nm_ManFindNameById(pObj->pNtk->pManName, pObj->Id) )
        Nm_ManDeleteIdName(pObj->pNtk->pManName, pObj->Id);
    // delete fanins and fanouts
    vNodes = Vec_PtrAlloc( 100 );
    Abc_NodeCollectFanouts( pObj, vNodes );
    for ( i = 0; i < vNodes->nSize; i++ )
        Abc_ObjDeleteFanin( vNodes->pArray[i], pObj );
    Abc_NodeCollectFanins( pObj, vNodes );
    for ( i = 0; i < vNodes->nSize; i++ )
        Abc_ObjDeleteFanin( pObj, vNodes->pArray[i] );
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
        case ABC_OBJ_PIO:    
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
        case ABC_OBJ_ASSERT:     
            Vec_PtrRemove( pNtk->vAsserts, pObj );
            Vec_PtrRemove( pNtk->vCos, pObj );
            break;
        case ABC_OBJ_NET:  
            break;
        case ABC_OBJ_NODE: 
            if ( Abc_NtkHasBdd(pNtk) )
                Cudd_RecursiveDeref( pNtk->pManFunc, pObj->pData );
            pObj->pData = NULL;
            break;
        case ABC_OBJ_LATCH: 
			pLatchInfo = ((Abc_LatchInfo_t *)pObj->pData);
			if (pLatchInfo->pClkName)
				FREE(pLatchInfo->pClkName);
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
        Vec_PtrForEachEntry( vNodes, pObj, i )
            if ( Abc_ObjIsNode(pObj) && Abc_ObjFanoutNum(pObj) == 0 )
                Abc_NtkDeleteObj_rec( pObj, fOnlyNodes );
    }
    else
    {
        Vec_PtrForEachEntry( vNodes, pObj, i )
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
    Vec_PtrForEachEntry( vNodes, pObj, i )
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
	Abc_LatchInfo_t * pLatchInfo;

    // create the new object
    pObjNew = Abc_NtkCreateObj( pNtkNew, pObj->Type );
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
                pObjNew->pData = Abc_SopRegister( pNtkNew->pManFunc, pObj->pData );
            else if ( Abc_NtkHasBdd(pNtkNew) )
                pObjNew->pData = Cudd_bddTransfer(pObj->pNtk->pManFunc, pNtkNew->pManFunc, pObj->pData), Cudd_Ref(pObjNew->pData);
            else if ( Abc_NtkHasAig(pNtkNew) )
                pObjNew->pData = Hop_Transfer(pObj->pNtk->pManFunc, pNtkNew->pManFunc, pObj->pData, Abc_ObjFaninNum(pObj));
            else if ( Abc_NtkHasMapping(pNtkNew) )
                pObjNew->pData = pObj->pData;
            else assert( 0 );
        }
    }
    else if ( Abc_ObjIsNet(pObj) ) // copy the name
    {
    }
	else if ( Abc_ObjIsLatch(pObj) ) { // copy the reset value
		pLatchInfo = (Abc_LatchInfo_t *)ALLOC( Abc_LatchInfo_t, 1);
		pLatchInfo->InitValue = ((Abc_LatchInfo_t *)pObj->pData)->InitValue;
		pLatchInfo->pClkName = strdup(((Abc_LatchInfo_t *)pObj->pData)->pClkName);
		pLatchInfo->LatchType = ((Abc_LatchInfo_t *)pObj->pData)->LatchType;
		pObjNew->pData = (void *)pLatchInfo;
	}
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
    pClone = Abc_NtkCreateObj( pObj->pNtk, pObj->Type );   
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
        pNode->pData = Abc_SopRegister( pNtk->pManFunc, " 0\n" );
    else if ( Abc_NtkHasBdd(pNtk) )
        pNode->pData = Cudd_ReadLogicZero(pNtk->pManFunc), Cudd_Ref( pNode->pData );
    else if ( Abc_NtkHasAig(pNtk) )
        pNode->pData = Hop_ManConst0(pNtk->pManFunc);
    else if ( Abc_NtkHasMapping(pNtk) )
        pNode->pData = Mio_LibraryReadConst0(Abc_FrameReadLibGen());
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
        pNode->pData = Abc_SopRegister( pNtk->pManFunc, " 1\n" );
    else if ( Abc_NtkHasBdd(pNtk) )
        pNode->pData = Cudd_ReadOne(pNtk->pManFunc), Cudd_Ref( pNode->pData );
    else if ( Abc_NtkHasAig(pNtk) )
        pNode->pData = Hop_ManConst1(pNtk->pManFunc);
    else if ( Abc_NtkHasMapping(pNtk) )
        pNode->pData = Mio_LibraryReadConst1(Abc_FrameReadLibGen());
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
        pNode->pData = Abc_SopRegister( pNtk->pManFunc, "0 1\n" );
    else if ( Abc_NtkHasBdd(pNtk) )
        pNode->pData = Cudd_Not(Cudd_bddIthVar(pNtk->pManFunc,0)), Cudd_Ref( pNode->pData );
    else if ( Abc_NtkHasAig(pNtk) )
        pNode->pData = Hop_Not(Hop_IthVar(pNtk->pManFunc,0));
    else if ( Abc_NtkHasMapping(pNtk) )
        pNode->pData = Mio_LibraryReadInv(Abc_FrameReadLibGen());
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
        pNode->pData = Abc_SopRegister( pNtk->pManFunc, "1 1\n" );
    else if ( Abc_NtkHasBdd(pNtk) )
        pNode->pData = Cudd_bddIthVar(pNtk->pManFunc,0), Cudd_Ref( pNode->pData );
    else if ( Abc_NtkHasAig(pNtk) )
        pNode->pData = Hop_IthVar(pNtk->pManFunc,0);
    else if ( Abc_NtkHasMapping(pNtk) )
        pNode->pData = Mio_LibraryReadBuf(Abc_FrameReadLibGen());
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
        Abc_ObjAddFanin( pNode, vFanins->pArray[i] );
    if ( Abc_NtkHasSop(pNtk) )
        pNode->pData = Abc_SopCreateAnd( pNtk->pManFunc, Vec_PtrSize(vFanins), NULL );
    else if ( Abc_NtkHasBdd(pNtk) )
        pNode->pData = Extra_bddCreateAnd( pNtk->pManFunc, Vec_PtrSize(vFanins) ), Cudd_Ref(pNode->pData); 
    else if ( Abc_NtkHasAig(pNtk) )
        pNode->pData = Hop_CreateAnd( pNtk->pManFunc, Vec_PtrSize(vFanins) ); 
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
        Abc_ObjAddFanin( pNode, vFanins->pArray[i] );
    if ( Abc_NtkHasSop(pNtk) )
        pNode->pData = Abc_SopCreateOr( pNtk->pManFunc, Vec_PtrSize(vFanins), NULL );
    else if ( Abc_NtkHasBdd(pNtk) )
        pNode->pData = Extra_bddCreateOr( pNtk->pManFunc, Vec_PtrSize(vFanins) ), Cudd_Ref(pNode->pData); 
    else if ( Abc_NtkHasAig(pNtk) )
        pNode->pData = Hop_CreateOr( pNtk->pManFunc, Vec_PtrSize(vFanins) ); 
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
        Abc_ObjAddFanin( pNode, vFanins->pArray[i] );
    if ( Abc_NtkHasSop(pNtk) )
        pNode->pData = Abc_SopCreateXorSpecial( pNtk->pManFunc, Vec_PtrSize(vFanins) );
    else if ( Abc_NtkHasBdd(pNtk) )
        pNode->pData = Extra_bddCreateExor( pNtk->pManFunc, Vec_PtrSize(vFanins) ), Cudd_Ref(pNode->pData); 
    else if ( Abc_NtkHasAig(pNtk) )
        pNode->pData = Hop_CreateExor( pNtk->pManFunc, Vec_PtrSize(vFanins) ); 
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
        pNode->pData = Abc_SopRegister( pNtk->pManFunc, "11- 1\n0-1 1\n" );
    else if ( Abc_NtkHasBdd(pNtk) )
        pNode->pData = Cudd_bddIte(pNtk->pManFunc,Cudd_bddIthVar(pNtk->pManFunc,0),Cudd_bddIthVar(pNtk->pManFunc,1),Cudd_bddIthVar(pNtk->pManFunc,2)), Cudd_Ref( pNode->pData );
    else if ( Abc_NtkHasAig(pNtk) )
        pNode->pData = Hop_Mux(pNtk->pManFunc,Hop_IthVar(pNtk->pManFunc,0),Hop_IthVar(pNtk->pManFunc,1),Hop_IthVar(pNtk->pManFunc,2));
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
bool Abc_NodeIsConst( Abc_Obj_t * pNode )    
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
bool Abc_NodeIsConst0( Abc_Obj_t * pNode )    
{ 
    Abc_Ntk_t * pNtk = pNode->pNtk;
    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsNetlist(pNtk) );
    assert( Abc_ObjIsNode(pNode) );      
    if ( !Abc_NodeIsConst(pNode) )
        return 0;
    if ( Abc_NtkHasSop(pNtk) )
        return Abc_SopIsConst0(pNode->pData);
    if ( Abc_NtkHasBdd(pNtk) )
        return Cudd_IsComplement(pNode->pData);
    if ( Abc_NtkHasAig(pNtk) )
        return Hop_IsComplement(pNode->pData);
    if ( Abc_NtkHasMapping(pNtk) )
        return pNode->pData == Mio_LibraryReadConst0(Abc_FrameReadLibGen());
    assert( 0 );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the node is a constant 1 node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Abc_NodeIsConst1( Abc_Obj_t * pNode )    
{ 
    Abc_Ntk_t * pNtk = pNode->pNtk;
    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsNetlist(pNtk) );
    assert( Abc_ObjIsNode(pNode) );      
    if ( !Abc_NodeIsConst(pNode) )
        return 0;
    if ( Abc_NtkHasSop(pNtk) )
        return Abc_SopIsConst1(pNode->pData);
    if ( Abc_NtkHasBdd(pNtk) )
        return !Cudd_IsComplement(pNode->pData);
    if ( Abc_NtkHasAig(pNtk) )
        return !Hop_IsComplement(pNode->pData);
    if ( Abc_NtkHasMapping(pNtk) )
        return pNode->pData == Mio_LibraryReadConst1(Abc_FrameReadLibGen());
    assert( 0 );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the node is a buffer.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Abc_NodeIsBuf( Abc_Obj_t * pNode )    
{ 
    Abc_Ntk_t * pNtk = pNode->pNtk;
    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsNetlist(pNtk) );
    assert( Abc_ObjIsNode(pNode) ); 
    if ( Abc_ObjFaninNum(pNode) != 1 )
        return 0;
    if ( Abc_NtkHasSop(pNtk) )
        return Abc_SopIsBuf(pNode->pData);
    if ( Abc_NtkHasBdd(pNtk) )
        return !Cudd_IsComplement(pNode->pData);
    if ( Abc_NtkHasAig(pNtk) )
        return !Hop_IsComplement(pNode->pData);
    if ( Abc_NtkHasMapping(pNtk) )
        return pNode->pData == Mio_LibraryReadBuf(Abc_FrameReadLibGen());
    assert( 0 );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the node is an inverter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Abc_NodeIsInv( Abc_Obj_t * pNode )    
{ 
    Abc_Ntk_t * pNtk = pNode->pNtk;
    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsNetlist(pNtk) );
    assert( Abc_ObjIsNode(pNode) ); 
    if ( Abc_ObjFaninNum(pNode) != 1 )
        return 0;
    if ( Abc_NtkHasSop(pNtk) )
        return Abc_SopIsInv(pNode->pData);
    if ( Abc_NtkHasBdd(pNtk) )
        return Cudd_IsComplement(pNode->pData);
    if ( Abc_NtkHasAig(pNtk) )
        return Hop_IsComplement(pNode->pData);
    if ( Abc_NtkHasMapping(pNtk) )
        return pNode->pData == Mio_LibraryReadInv(Abc_FrameReadLibGen());
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
        Abc_SopComplement( pNode->pData );
    else if ( Abc_NtkHasBdd(pNode->pNtk) )
        pNode->pData = Cudd_Not( pNode->pData );
    else if ( Abc_NtkHasAig(pNode->pNtk) )
        pNode->pData = Hop_Not( pNode->pData );
    else
        assert( 0 );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

