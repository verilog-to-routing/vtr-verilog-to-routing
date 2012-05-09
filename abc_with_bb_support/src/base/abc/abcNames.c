/**CFile****************************************************************

  FileName    [abcNames.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Procedures working with net and node names.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcNames.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abc.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns the unique name for the object.]

  Description [If the name previously did not exist, creates a new unique
  name but does not assign this name to the object. The temporary unique
  name is stored in a static buffer inside this procedure. It is important 
  that the name is used before the function is called again!]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_ObjName( Abc_Obj_t * pObj )
{
    return Nm_ManCreateUniqueName( pObj->pNtk->pManName, pObj->Id );
}

/**Function*************************************************************

  Synopsis    [Assigns the given name to the object.]

  Description [The object should not have a name assigned. The same
  name may be used for several objects, which they share the same net
  in the original netlist. (For example, latch output and primary output 
  may have the same name.) This procedure returns the pointer to the 
  internally stored representation of the given name.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_ObjAssignName( Abc_Obj_t * pObj, char * pName, char * pSuffix )
{
    assert( pName != NULL );
    return Nm_ManStoreIdName( pObj->pNtk->pManName, pObj->Id, pObj->Type, pName, pSuffix );
}

/**Function*************************************************************

  Synopsis    [Gets the long name of the node.]

  Description [This name is the output net's name.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_ObjNameSuffix( Abc_Obj_t * pObj, char * pSuffix )
{
    static char Buffer[500];
    sprintf( Buffer, "%s%s", Abc_ObjName(pObj), pSuffix );
    return Buffer;
}

/**Function*************************************************************

  Synopsis    [Returns the dummy PI name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_ObjNameDummy( char * pPrefix, int Num, int nDigits )
{
    static char Buffer[100];
    sprintf( Buffer, "%s%0*d", pPrefix, nDigits, Num );
    return Buffer;
}

/**Function*************************************************************

  Synopsis    [Tranfers names to the old network.]

  Description [Assumes that the new nodes are attached using pObj->pCopy.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkTrasferNames( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkNew )
{
    Abc_Obj_t * pObj;
    int i;
    assert( Abc_NtkPiNum(pNtk) == Abc_NtkPiNum(pNtkNew) );
    assert( Abc_NtkPoNum(pNtk) == Abc_NtkPoNum(pNtkNew) );
    assert( Abc_NtkBoxNum(pNtk) == Abc_NtkBoxNum(pNtkNew) );
    assert( Abc_NtkAssertNum(pNtk) == Abc_NtkAssertNum(pNtkNew) );
    assert( Nm_ManNumEntries(pNtk->pManName) > 0 );
    assert( Nm_ManNumEntries(pNtkNew->pManName) == 0 );
    // copy the CI/CO/box names
    Abc_NtkForEachCi( pNtk, pObj, i )
        Abc_ObjAssignName( pObj->pCopy, Abc_ObjName(Abc_ObjFanout0Ntk(pObj)), NULL );
    Abc_NtkForEachCo( pNtk, pObj, i ) 
        Abc_ObjAssignName( pObj->pCopy, Abc_ObjName(Abc_ObjFanin0Ntk(pObj)), NULL );
    Abc_NtkForEachBox( pNtk, pObj, i ) 
        Abc_ObjAssignName( pObj->pCopy, Abc_ObjName(pObj), NULL );
}

/**Function*************************************************************

  Synopsis    [Tranfers names to the old network.]

  Description [Assumes that the new nodes are attached using pObj->pCopy.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkTrasferNamesNoLatches( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkNew )
{
    Abc_Obj_t * pObj;
    int i;
    assert( Abc_NtkPiNum(pNtk) == Abc_NtkPiNum(pNtkNew) );
    assert( Abc_NtkPoNum(pNtk) == Abc_NtkPoNum(pNtkNew) );
    assert( Abc_NtkAssertNum(pNtk) == Abc_NtkAssertNum(pNtkNew) );
    assert( Nm_ManNumEntries(pNtk->pManName) > 0 );
    assert( Nm_ManNumEntries(pNtkNew->pManName) == 0 );
    // copy the CI/CO/box name and skip latches and theirs inputs/outputs
    Abc_NtkForEachCi( pNtk, pObj, i )
        if ( Abc_ObjFaninNum(pObj) == 0 || !Abc_ObjIsLatch(Abc_ObjFanin0(pObj)) )
            Abc_ObjAssignName( pObj->pCopy, Abc_ObjName(Abc_ObjFanout0Ntk(pObj)), NULL );
    Abc_NtkForEachCo( pNtk, pObj, i ) 
        if ( Abc_ObjFanoutNum(pObj) == 0 || !Abc_ObjIsLatch(Abc_ObjFanout0(pObj)) )
            Abc_ObjAssignName( pObj->pCopy, Abc_ObjName(Abc_ObjFanin0Ntk(pObj)), NULL );
    Abc_NtkForEachBox( pNtk, pObj, i ) 
        if ( !Abc_ObjIsLatch(pObj) )
            Abc_ObjAssignName( pObj->pCopy, Abc_ObjName(pObj), NULL );
}

/**Function*************************************************************

  Synopsis    [Gets fanin node names.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NodeGetFaninNames( Abc_Obj_t * pNode )
{
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pFanin;
    int i;
    vNodes = Vec_PtrAlloc( 100 );
    Abc_ObjForEachFanin( pNode, pFanin, i )
        Vec_PtrPush( vNodes, Extra_UtilStrsav(Abc_ObjName(pFanin)) );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Gets fanin node names.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NodeGetFakeNames( int nNames )
{
    Vec_Ptr_t * vNames;
    char Buffer[5];
    int i;

    vNames = Vec_PtrAlloc( nNames );
    for ( i = 0; i < nNames; i++ )
    {
        if ( nNames < 26 )
        {
            Buffer[0] = 'a' + i;
            Buffer[1] = 0;
        }
        else
        {
            Buffer[0] = 'a' + i%26;
            Buffer[1] = '0' + i/26;
            Buffer[2] = 0;
        }
        Vec_PtrPush( vNames, Extra_UtilStrsav(Buffer) );
    }
    return vNames;
}

/**Function*************************************************************

  Synopsis    [Gets fanin node names.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeFreeNames( Vec_Ptr_t * vNames )
{
    int i;
    if ( vNames == NULL )
        return;
    for ( i = 0; i < vNames->nSize; i++ )
        free( vNames->pArray[i] );
    Vec_PtrFree( vNames );
}

/**Function*************************************************************

  Synopsis    [Collects the CI or CO names.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char ** Abc_NtkCollectCioNames( Abc_Ntk_t * pNtk, int fCollectCos )
{
    Abc_Obj_t * pObj;
    char ** ppNames;
    int i;
    if ( fCollectCos )
    {
        ppNames = ALLOC( char *, Abc_NtkCoNum(pNtk) );
        Abc_NtkForEachCo( pNtk, pObj, i )
            ppNames[i] = Abc_ObjName(pObj);
    }
    else
    {
        ppNames = ALLOC( char *, Abc_NtkCiNum(pNtk) );
        Abc_NtkForEachCi( pNtk, pObj, i )
            ppNames[i] = Abc_ObjName(pObj);
    }
    return ppNames;
}

/**Function*************************************************************

  Synopsis    [Procedure used for sorting the nodes in decreasing order of levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeCompareNames( Abc_Obj_t ** pp1, Abc_Obj_t ** pp2 )
{
    int Diff = strcmp( (char *)(*pp1)->pCopy, (char *)(*pp2)->pCopy );
    if ( Diff < 0 )
        return -1;
    if ( Diff > 0 ) 
        return 1;
    return 0; 
}

/**Function*************************************************************

  Synopsis    [Orders PIs/POs/latches alphabetically.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkOrderObjsByName( Abc_Ntk_t * pNtk, int fComb )
{
    Abc_Obj_t * pObj;
    int i;
    assert( Abc_NtkAssertNum(pNtk) == 0 );
    assert( Abc_NtkHasOnlyLatchBoxes(pNtk) );
    // temporarily store the names in the copy field
    Abc_NtkForEachPi( pNtk, pObj, i )
        pObj->pCopy = (Abc_Obj_t *)Abc_ObjName(pObj);
    Abc_NtkForEachPo( pNtk, pObj, i )
        pObj->pCopy = (Abc_Obj_t *)Abc_ObjName(pObj);
    Abc_NtkForEachBox( pNtk, pObj, i )
        pObj->pCopy = (Abc_Obj_t *)Abc_ObjName(Abc_ObjFanout0(pObj));
    // order objects alphabetically
    qsort( (void *)Vec_PtrArray(pNtk->vPis), Vec_PtrSize(pNtk->vPis), sizeof(Abc_Obj_t *), 
        (int (*)(const void *, const void *)) Abc_NodeCompareNames );
    qsort( (void *)Vec_PtrArray(pNtk->vPos), Vec_PtrSize(pNtk->vPos), sizeof(Abc_Obj_t *), 
        (int (*)(const void *, const void *)) Abc_NodeCompareNames );
    // if the comparison if combinational (latches as PIs/POs), order them too
    if ( fComb )
        qsort( (void *)Vec_PtrArray(pNtk->vBoxes), Vec_PtrSize(pNtk->vBoxes), sizeof(Abc_Obj_t *), 
            (int (*)(const void *, const void *)) Abc_NodeCompareNames );
    // order CIs/COs first PIs/POs(Asserts) then latches
    Abc_NtkOrderCisCos( pNtk );
    // clean the copy fields
    Abc_NtkForEachPi( pNtk, pObj, i )
        pObj->pCopy = NULL;
    Abc_NtkForEachPo( pNtk, pObj, i )
        pObj->pCopy = NULL;
    Abc_NtkForEachBox( pNtk, pObj, i )
        pObj->pCopy = NULL;
}

/**Function*************************************************************

  Synopsis    [Adds dummy names.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkAddDummyPiNames( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int nDigits, i;
    nDigits = Extra_Base10Log( Abc_NtkPiNum(pNtk) );
    Abc_NtkForEachPi( pNtk, pObj, i )
        Abc_ObjAssignName( pObj, Abc_ObjNameDummy("pi", i, nDigits), NULL );
}

/**Function*************************************************************

  Synopsis    [Adds dummy names.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkAddDummyPoNames( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int nDigits, i;
    nDigits = Extra_Base10Log( Abc_NtkPoNum(pNtk) );
    Abc_NtkForEachPo( pNtk, pObj, i )
        Abc_ObjAssignName( pObj, Abc_ObjNameDummy("po", i, nDigits), NULL );
}

/**Function*************************************************************

  Synopsis    [Adds dummy names.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkAddDummyAssertNames( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int nDigits, i;
    nDigits = Extra_Base10Log( Abc_NtkAssertNum(pNtk) );
    Abc_NtkForEachAssert( pNtk, pObj, i )
        Abc_ObjAssignName( pObj, Abc_ObjNameDummy("a", i, nDigits), NULL );
}

/**Function*************************************************************

  Synopsis    [Adds dummy names.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkAddDummyBoxNames( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int nDigits, i;
    assert( !Abc_NtkIsNetlist(pNtk) );
    nDigits = Extra_Base10Log( Abc_NtkLatchNum(pNtk) );
    Abc_NtkForEachLatch( pNtk, pObj, i )
    {
        Abc_ObjAssignName( pObj, Abc_ObjNameDummy("l", i, nDigits), NULL );
        Abc_ObjAssignName( Abc_ObjFanin0(pObj),  Abc_ObjNameDummy("li", i, nDigits), NULL );
        Abc_ObjAssignName( Abc_ObjFanout0(pObj), Abc_ObjNameDummy("lo", i, nDigits), NULL );
    }
/*
    nDigits = Extra_Base10Log( Abc_NtkBlackboxNum(pNtk) );
    Abc_NtkForEachBlackbox( pNtk, pObj, i )
    {
        pName = Abc_ObjAssignName( pObj, Abc_ObjNameDummy("B", i, nDigits), NULL );
        nDigitsF = Extra_Base10Log( Abc_ObjFaninNum(pObj) );
        Abc_ObjForEachFanin( pObj, pTerm, k )
            Abc_ObjAssignName( Abc_ObjFanin0(pObj), pName, Abc_ObjNameDummy("i", k, nDigitsF) );
        nDigitsF = Extra_Base10Log( Abc_ObjFanoutNum(pObj) );
        Abc_ObjForEachFanout( pObj, pTerm, k )
            Abc_ObjAssignName( Abc_ObjFanin0(pObj), pName, Abc_ObjNameDummy("o", k, nDigitsF) );
    }
*/
}

/**Function*************************************************************

  Synopsis    [Replaces names by short names.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkShortNames( Abc_Ntk_t * pNtk )
{
    Nm_ManFree( pNtk->pManName );
    pNtk->pManName = Nm_ManCreate( Abc_NtkCiNum(pNtk) + Abc_NtkCoNum(pNtk) + Abc_NtkBoxNum(pNtk) );
    Abc_NtkAddDummyPiNames( pNtk );
    Abc_NtkAddDummyPoNames( pNtk );
    Abc_NtkAddDummyAssertNames( pNtk );
    Abc_NtkAddDummyBoxNames( pNtk );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


