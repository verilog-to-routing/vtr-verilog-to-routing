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

ABC_NAMESPACE_IMPL_START


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
Abc_Des_t * Abc_DesCreate( char * pName )
{
    Abc_Des_t * p;
    p = ABC_ALLOC( Abc_Des_t, 1 );
    memset( p, 0, sizeof(Abc_Des_t) );
    p->pName    = Abc_UtilStrsav( pName );
    p->tModules = st__init_table( strcmp, st__strhash );
    p->vTops    = Vec_PtrAlloc( 100 );
    p->vModules = Vec_PtrAlloc( 100 );
    p->pManFunc = Hop_ManStart();
    p->pLibrary = NULL;
    return p;
}

/**Function*************************************************************

  Synopsis    [Removes all pointers to the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_DesCleanManPointer( Abc_Des_t * p, void * pMan )
{
    Abc_Ntk_t * pTemp;
    int i;
    if ( p == NULL )
        return;
    if ( p->pManFunc == pMan )
        p->pManFunc = NULL;
    Vec_PtrForEachEntry( Abc_Ntk_t *, p->vModules, pTemp, i )
        if ( pTemp->pManFunc == pMan )
            pTemp->pManFunc = NULL;
}

/**Function*************************************************************

  Synopsis    [Frees the library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_DesFree( Abc_Des_t * p, Abc_Ntk_t * pNtkSave )
{
    Abc_Ntk_t * pNtk;
    int i;
    if ( p->pName )
        ABC_FREE( p->pName );
    if ( p->pManFunc )
        Hop_ManStop( (Hop_Man_t *)p->pManFunc );
    if ( p->tModules )
        st__free_table( p->tModules );
    if ( p->vModules )
    {
        Vec_PtrForEachEntry( Abc_Ntk_t *, p->vModules, pNtk, i )
        {
            if ( pNtk == pNtkSave )
                continue;
            pNtk->pDesign = NULL;
            if ( (pNtkSave && pNtk->pManFunc == pNtkSave->pManFunc) || (pNtk->pManFunc == p->pManFunc) )
                pNtk->pManFunc = NULL;
            Abc_NtkDelete( pNtk );
        }
        Vec_PtrFree( p->vModules );
    }
    if ( p->vTops )
        Vec_PtrFree( p->vTops );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Duplicated the library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Des_t * Abc_DesDup( Abc_Des_t * p )
{
    Abc_Des_t * pNew;
    Abc_Ntk_t * pTemp;
    Abc_Obj_t * pObj;
    int i, k;
    pNew = Abc_DesCreate( p->pName );
    Vec_PtrForEachEntry( Abc_Ntk_t *, p->vModules, pTemp, i )
        Abc_DesAddModel( pNew, Abc_NtkDup(pTemp) );
    Vec_PtrForEachEntry( Abc_Ntk_t *, p->vTops, pTemp, i )
        Vec_PtrPush( pNew->vTops, pTemp->pCopy );
    Vec_PtrForEachEntry( Abc_Ntk_t *, p->vModules, pTemp, i )
        pTemp->pCopy->pAltView = pTemp->pAltView ? pTemp->pAltView->pCopy : NULL;
    // update box models
    Vec_PtrForEachEntry( Abc_Ntk_t *, p->vModules, pTemp, i )
        Abc_NtkForEachBox( pTemp, pObj, k )
            if ( Abc_ObjIsWhitebox(pObj) || Abc_ObjIsBlackbox(pObj) )
                pObj->pCopy->pData = Abc_ObjModel(pObj)->pCopy;
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Frees the library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Des_t * Abc_DesDupBlackboxes( Abc_Des_t * p, Abc_Ntk_t * pNtkSave )
{
    Abc_Des_t * pNew;
    Abc_Ntk_t * pNtkTemp;
    int i;
    assert( Vec_PtrSize(p->vTops) > 0 );
    assert( Vec_PtrSize(p->vModules) > 1 );
    pNew = Abc_DesCreate( p->pName );
//    pNew->pManFunc = pNtkSave->pManFunc;
    Vec_PtrPush( pNew->vTops, pNtkSave );
    Vec_PtrPush( pNew->vModules, pNtkSave );
    Vec_PtrForEachEntry( Abc_Ntk_t *, p->vModules, pNtkTemp, i )
        if ( Abc_NtkHasBlackbox( pNtkTemp ) )
            Vec_PtrPush( pNew->vModules, Abc_NtkDup(pNtkTemp) );
    return pNew;
}


/**Function*************************************************************

  Synopsis    [Prints the library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_DesPrint( Abc_Des_t * p )
{
    Abc_Ntk_t * pNtk;
    Abc_Obj_t * pObj;
    int i, k;
    printf( "Models of design %s:\n", p->pName );
    Vec_PtrForEachEntry( Abc_Ntk_t *, p->vModules, pNtk, i )
    {
        printf( "%2d : %20s   ", i+1, pNtk->pName );
        printf( "nd = %6d   lat = %6d   whitebox = %3d   blackbox = %3d\n", 
            Abc_NtkNodeNum(pNtk), Abc_NtkLatchNum(pNtk), 
            Abc_NtkWhiteboxNum(pNtk), Abc_NtkBlackboxNum(pNtk) );
        if ( Abc_NtkBlackboxNum(pNtk) == 0 )
            continue;
        Abc_NtkForEachWhitebox( pNtk, pObj, k )
            printf( "     %20s (whitebox)\n", Abc_NtkName((Abc_Ntk_t *)pObj->pData) );
        Abc_NtkForEachBlackbox( pNtk, pObj, k )
            printf( "     %20s (blackbox)\n", Abc_NtkName((Abc_Ntk_t *)pObj->pData) );
    }
}

/**Function*************************************************************

  Synopsis    [Create the library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_DesAddModel( Abc_Des_t * p, Abc_Ntk_t * pNtk )
{
    if ( st__is_member( p->tModules, (char *)pNtk->pName ) )
        return 0;
    st__insert( p->tModules, (char *)pNtk->pName, (char *)pNtk );
    assert( pNtk->Id == 0 );
    pNtk->Id = Vec_PtrSize(p->vModules);
    Vec_PtrPush( p->vModules, pNtk );
    pNtk->pDesign = p;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Create the library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_DesFindModelByName( Abc_Des_t * p, char * pName )
{
    Abc_Ntk_t * pNtk;
    if ( ! st__is_member( p->tModules, (char *)pName ) )
        return NULL;
    st__lookup( p->tModules, (char *)pName, (char **)&pNtk );
    return pNtk;
}

/**Function*************************************************************

  Synopsis    [Frees the library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_DesDeriveRoot( Abc_Des_t * p )
{
    Abc_Ntk_t * pNtk;
    if ( Vec_PtrSize(p->vModules) > 1 )
    {
        printf( "The design includes more than one module and is currently not used.\n" );
        return NULL;
    }
    pNtk = (Abc_Ntk_t *)Vec_PtrEntry( p->vModules, 0 );  Vec_PtrClear( p->vModules );
    pNtk->pManFunc = p->pManFunc;           p->pManFunc = NULL;
    return pNtk;
}

/**Function*************************************************************

  Synopsis    [Detects the top-level models.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_DesFindTopLevelModels( Abc_Des_t * p )
{
    Abc_Ntk_t * pNtk, * pNtkBox;
    Abc_Obj_t * pObj;
    int i, k;
    assert( Vec_PtrSize( p->vModules ) > 0 );
    // clear the models
    Vec_PtrForEachEntry( Abc_Ntk_t *, p->vModules, pNtk, i )
        pNtk->fHieVisited = 0;
    // mark all the models reachable from other models
    Vec_PtrForEachEntry( Abc_Ntk_t *, p->vModules, pNtk, i )
    {
        Abc_NtkForEachBox( pNtk, pObj, k )
        {
            if ( Abc_ObjIsLatch(pObj) )
                continue;
            if ( pObj->pData == NULL )
                continue;
            pNtkBox = (Abc_Ntk_t *)pObj->pData;
            pNtkBox->fHieVisited = 1;
        }
    }
    // collect the models that are not marked
    Vec_PtrClear( p->vTops );
    Vec_PtrForEachEntry( Abc_Ntk_t *, p->vModules, pNtk, i )
    {
        if ( pNtk->fHieVisited == 0 )
            Vec_PtrPush( p->vTops, pNtk );
        else
            pNtk->fHieVisited = 0;
    }
    return Vec_PtrSize( p->vTops );
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
    pNtkGate = (Abc_Ntk_t *)pBox->pData;
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

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

