/**CFile****************************************************************

  FileName    [abcLib.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Functions to manipulate verilog libraries.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcLib.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abc.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
 
////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Create the library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Lib_t * Abc_LibCreate( char * pName )
{
    Abc_Lib_t * p;
    p = ALLOC( Abc_Lib_t, 1 );
    memset( p, 0, sizeof(Abc_Lib_t) );
    p->pName    = Extra_UtilStrsav( pName );
    p->tModules = st_init_table( strcmp, st_strhash );
    p->vTops    = Vec_PtrAlloc( 100 );
    p->vModules = Vec_PtrAlloc( 100 );
    p->pManFunc = Hop_ManStart();
    p->pLibrary = NULL;
    return p;
}

/**Function*************************************************************

  Synopsis    [Frees the library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_LibFree( Abc_Lib_t * pLib, Abc_Ntk_t * pNtkSave )
{
    Abc_Ntk_t * pNtk;
    int i;
    if ( pLib->pName )
        free( pLib->pName );
    if ( pLib->pManFunc )
        Hop_ManStop( pLib->pManFunc );
    if ( pLib->tModules )
        st_free_table( pLib->tModules );
    if ( pLib->vModules )
    {
        Vec_PtrForEachEntry( pLib->vModules, pNtk, i )
        {
//            pNtk->pManFunc = NULL;
            if ( pNtk == pNtkSave )
                continue;
            pNtk->pManFunc = NULL;
            pNtk->pDesign = NULL;
            Abc_NtkDelete( pNtk );
        }
        Vec_PtrFree( pLib->vModules );
    }
    if ( pLib->vTops )
        Vec_PtrFree( pLib->vTops );
    free( pLib );
}

/**Function*************************************************************

  Synopsis    [Frees the library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Lib_t * Abc_LibDupBlackboxes( Abc_Lib_t * pLib, Abc_Ntk_t * pNtkSave )
{
    Abc_Lib_t * pLibNew;
    Abc_Ntk_t * pNtkTemp;
    int i;
    assert( Vec_PtrSize(pLib->vTops) > 0 );
    assert( Vec_PtrSize(pLib->vModules) > 1 );
    pLibNew = Abc_LibCreate( pLib->pName );
//    pLibNew->pManFunc = pNtkSave->pManFunc;
    Vec_PtrPush( pLibNew->vTops, pNtkSave );
    Vec_PtrPush( pLibNew->vModules, pNtkSave );
    Vec_PtrForEachEntry( pLib->vModules, pNtkTemp, i )
        if ( Abc_NtkHasBlackbox( pNtkTemp ) )
            Vec_PtrPush( pLibNew->vModules, Abc_NtkDup(pNtkTemp) );
    return pLibNew;
}


/**Function*************************************************************

  Synopsis    [Prints the library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_LibPrint( Abc_Lib_t * pLib )
{
    Abc_Ntk_t * pNtk;
    Abc_Obj_t * pObj;
    int i, k;
    printf( "Models of design %s:\n", pLib->pName );
    Vec_PtrForEachEntry( pLib->vModules, pNtk, i )
    {
        printf( "%2d : %20s   ", i+1, pNtk->pName );
        printf( "nd = %6d   lat = %6d   whitebox = %3d   blackbox = %3d\n", 
            Abc_NtkNodeNum(pNtk), Abc_NtkLatchNum(pNtk), 
            Abc_NtkWhiteboxNum(pNtk), Abc_NtkBlackboxNum(pNtk) );
        if ( Abc_NtkBlackboxNum(pNtk) == 0 )
            continue;
        Abc_NtkForEachWhitebox( pNtk, pObj, k )
            printf( "     %20s (whitebox)\n", Abc_NtkName(pObj->pData) );
        Abc_NtkForEachBlackbox( pNtk, pObj, k )
            printf( "     %20s (blackbox)\n", Abc_NtkName(pObj->pData) );
    }
}

/**Function*************************************************************

  Synopsis    [Create the library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_LibAddModel( Abc_Lib_t * pLib, Abc_Ntk_t * pNtk )
{
    if ( st_is_member( pLib->tModules, (char *)pNtk->pName ) )
        return 0;
    st_insert( pLib->tModules, (char *)pNtk->pName, (char *)pNtk );
    Vec_PtrPush( pLib->vModules, pNtk );
    pNtk->pDesign = pLib;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Create the library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_LibFindModelByName( Abc_Lib_t * pLib, char * pName )
{
    Abc_Ntk_t * pNtk;
    if ( !st_is_member( pLib->tModules, (char *)pName ) )
        return NULL;
    st_lookup( pLib->tModules, (char *)pName, (char **)&pNtk );
    return pNtk;
}

/**Function*************************************************************

  Synopsis    [Frees the library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_LibDeriveRoot( Abc_Lib_t * pLib )
{
    Abc_Ntk_t * pNtk;
    if ( Vec_PtrSize(pLib->vModules) > 1 )
    {
        printf( "The design includes more than one module and is currently not used.\n" );
        return NULL;
    }
    pNtk = Vec_PtrEntry( pLib->vModules, 0 );  Vec_PtrClear( pLib->vModules );
    pNtk->pManFunc = pLib->pManFunc;           pLib->pManFunc = NULL;
    return pNtk;
}

/**Function*************************************************************

  Synopsis    [Detects the top-level models.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_LibFindTopLevelModels( Abc_Lib_t * pLib )
{
    Abc_Ntk_t * pNtk, * pNtkBox;
    Abc_Obj_t * pObj;
    int i, k;
    assert( Vec_PtrSize( pLib->vModules ) > 0 );
    // clear the models
    Vec_PtrForEachEntry( pLib->vModules, pNtk, i )
        pNtk->fHieVisited = 0;
    // mark all the models reachable from other models
    Vec_PtrForEachEntry( pLib->vModules, pNtk, i )
    {
        Abc_NtkForEachBox( pNtk, pObj, k )
        {
            if ( Abc_ObjIsLatch(pObj) )
                continue;
            if ( pObj->pData == NULL )
                continue;
            pNtkBox = pObj->pData;
            pNtkBox->fHieVisited = 1;
        }
    }
    // collect the models that are not marked
    Vec_PtrClear( pLib->vTops );
    Vec_PtrForEachEntry( pLib->vModules, pNtk, i )
    {
        if ( pNtk->fHieVisited == 0 )
            Vec_PtrPush( pLib->vTops, pNtk );
        else
            pNtk->fHieVisited = 0;
    }
    return Vec_PtrSize( pLib->vTops );
}


/**Function*************************************************************

  Synopsis    [Surround boxes without content (black boxes) with BIs/BOs.]

  Description [Returns the number of black boxes converted.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_LibDeriveBlackBoxes( Abc_Ntk_t * pNtk, Abc_Lib_t * pLib )
{
/*
    Abc_Obj_t * pObj, * pFanin, * pFanout;
    int i, k;
    assert( Abc_NtkIsNetlist(pNtk) );
    // collect blackbox nodes
    assert( Vec_PtrSize(pNtk->vBoxes) == 0 );
    Vec_PtrClear( pNtk->vBoxes );
    Abc_NtkForEachBox( pNtk, pObj, i )
        if ( Abc_NtkNodeNum(pObj->pData) == 0 )
            Vec_PtrPush( pNtk->vBoxes, pObj );
    // return if there is no black boxes without content
    if ( Vec_PtrSize(pNtk->vBoxes) == 0 )
        return 0;
    // print the boxes
    printf( "Black boxes are: " );
    Abc_NtkForEachBox( pNtk, pObj, i )
        printf( " %s", ((Abc_Ntk_t *)pObj->pData)->pName );
    printf( "\n" );
    // iterate through the boxes and add BIs/BOs
    Abc_NtkForEachBox( pNtk, pObj, i )
    {
        // go through the fanin nets
        Abc_ObjForEachFanin( pObj, pFanin, k )
            Abc_ObjInsertBetween( pFanin, pObj, ABC_OBJ_BI );
        // go through the fanout nets
        Abc_ObjForEachFanout( pObj, pFanout, k )
        {
            Abc_ObjInsertBetween( pObj, pFanout, ABC_OBJ_BO );
            // if the name is not given assign name
            if ( pFanout->pData == NULL )
            {
                pFanout->pData = Abc_ObjName( pFanout );
                Nm_ManStoreIdName( pNtk->pManName, pFanout->Id, pFanout->pData, NULL );
            }
        }
    }
    return Vec_PtrSize(pNtk->vBoxes);
*/
    return 0;
}

/**Function*************************************************************

  Synopsis    [Derive the AIG of the logic in the netlist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeStrashUsingNetwork_rec( Abc_Ntk_t * pNtkAig, Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFanin;
    int i;
    assert( !Abc_ObjIsNet(pObj) );
    if ( pObj->pCopy )
        return;
    // call for the fanins
    Abc_ObjForEachFanin( pObj, pFanin, i )
        Abc_NodeStrashUsingNetwork_rec( pNtkAig, Abc_ObjFanin0Ntk(Abc_ObjFanin0(pObj)) );
    // compute for the node
    pObj->pCopy = Abc_NodeStrash( pNtkAig, pObj, 0 );
    // set for the fanout net
    Abc_ObjFanout0(pObj)->pCopy = pObj->pCopy;
}

/**Function*************************************************************

  Synopsis    [Derive the AIG of the logic in the netlist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeStrashUsingNetwork( Abc_Ntk_t * pNtkAig, Abc_Obj_t * pBox )
{ 
    Abc_Ntk_t * pNtkGate;
    Abc_Obj_t * pObj;
    unsigned * pPolarity;
    int i, fCompl;
    assert( Abc_ObjIsBox(pBox) );
    pNtkGate = pBox->pData;
    pPolarity = (unsigned *)pBox->pNext;
    assert( Abc_NtkIsNetlist(pNtkGate) );
    assert( Abc_NtkLatchNum(pNtkGate) == 0 );
    Abc_NtkCleanCopy( pNtkGate );
    // set the PI values
    Abc_NtkForEachPi( pNtkGate, pObj, i )
    {
        fCompl = (pPolarity && Abc_InfoHasBit(pPolarity, i));
        pObj->pCopy = Abc_ObjNotCond( Abc_ObjFanin(pBox,i)->pCopy, fCompl );
        Abc_ObjFanout0(pObj)->pCopy = pObj->pCopy;
    }
    // build recursively and set the PO values
    Abc_NtkForEachPo( pNtkGate, pObj, i )
    {
        Abc_NodeStrashUsingNetwork_rec( pNtkAig, Abc_ObjFanin0Ntk(Abc_ObjFanin0(pObj)) );
        Abc_ObjFanout(pBox,i)->pCopy = Abc_ObjFanin0(pObj)->pCopy;
    }
//printf( "processing %d\n", pBox->Id );
}

/**Function*************************************************************

  Synopsis    [Derive the AIG of the logic in the netlist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_LibDeriveAig( Abc_Ntk_t * pNtk, Abc_Lib_t * pLib )
{
    ProgressBar * pProgress;
    Vec_Ptr_t * vNodes;
    Abc_Ntk_t * pNtkAig;
    Abc_Obj_t * pObj;
    int i, nBoxes;
    // explicitly derive black boxes
    assert( Abc_NtkIsNetlist(pNtk) );
    nBoxes = Abc_LibDeriveBlackBoxes( pNtk, pLib );
    if ( nBoxes )
        printf( "Detected and transformed %d black boxes.\n", nBoxes );
    // create the new network with black boxes in place
    pNtkAig = Abc_NtkStartFrom( pNtk, ABC_NTK_STRASH, ABC_FUNC_AIG );
    // transer to the nets
    Abc_NtkForEachCi( pNtk, pObj, i )
        Abc_ObjFanout0(pObj)->pCopy = pObj->pCopy;
    // build the AIG for the remaining logic in the netlist
    vNodes = Abc_NtkDfs( pNtk, 0 );
    pProgress = Extra_ProgressBarStart( stdout, Vec_PtrSize(vNodes) );
    Vec_PtrForEachEntry( vNodes, pObj, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        if ( Abc_ObjIsNode(pObj) )
        {
            pObj->pCopy = Abc_NodeStrash( pNtkAig, pObj, 0 );
            Abc_ObjFanout0(pObj)->pCopy = pObj->pCopy;
            continue;
        }
        Abc_NodeStrashUsingNetwork( pNtkAig, pObj );
    }
    Extra_ProgressBarStop( pProgress );
    Vec_PtrFree( vNodes );
    // deallocate memory manager, which remembers the phase
    if ( pNtk->pData )
    {
        Extra_MmFlexStop( pNtk->pData );
        pNtk->pData = NULL;
    }
    // set the COs
//    Abc_NtkFinalize( pNtk, pNtkAig );
    Abc_NtkForEachCo( pNtk, pObj, i )
        Abc_ObjAddFanin( pObj->pCopy, Abc_ObjFanin0(pObj)->pCopy );
    Abc_AigCleanup( pNtkAig->pManFunc );
    // make sure that everything is okay
    if ( !Abc_NtkCheck( pNtkAig ) )
    {
        printf( "Abc_LibDeriveAig: The network check has failed.\n" );
        return 0;
    }
    return pNtkAig;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


