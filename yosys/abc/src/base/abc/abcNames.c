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
#include "misc/util/utilNam.h"

ABC_NAMESPACE_IMPL_START


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

  Synopsis    [Appends name to the prefix]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_ObjNamePrefix( Abc_Obj_t * pObj, char * pPrefix )
{
    static char Buffer[2000];
    sprintf( Buffer, "%s%s", pPrefix, Abc_ObjName(pObj) );
    return Buffer;
}

/**Function*************************************************************

  Synopsis    [Appends suffic to the name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_ObjNameSuffix( Abc_Obj_t * pObj, char * pSuffix )
{
    static char Buffer[2000];
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
    static char Buffer[2000];
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
        Vec_PtrPush( vNodes, Abc_UtilStrsav(Abc_ObjName(pFanin)) );
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
        ABC_FREE( vNames->pArray[i] );
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
        ppNames = ABC_ALLOC( char *, Abc_NtkCoNum(pNtk) );
        Abc_NtkForEachCo( pNtk, pObj, i )
            ppNames[i] = Abc_ObjName(pObj);
    }
    else
    {
        ppNames = ABC_ALLOC( char *, Abc_NtkCiNum(pNtk) );
        Abc_NtkForEachCi( pNtk, pObj, i )
            ppNames[i] = Abc_ObjName(pObj);
    }
    return ppNames;
}

/**Function*************************************************************

  Synopsis    [Orders PIs/POs/latches alphabetically.]

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
    Diff = (*pp1)->Id - (*pp2)->Id;
    if ( Diff < 0 )
        return -1;
    if ( Diff > 0 ) 
        return 1;
    return 0; 
}
void Abc_NtkOrderObjsByName( Abc_Ntk_t * pNtk, int fComb )
{
    Abc_Obj_t * pObj;
    int i;
    assert( Abc_NtkHasOnlyLatchBoxes(pNtk) );
    // temporarily store the names in the copy field
    Abc_NtkForEachPi( pNtk, pObj, i )
        pObj->pCopy = (Abc_Obj_t *)Abc_ObjName(pObj);
    Abc_NtkForEachPo( pNtk, pObj, i )
        pObj->pCopy = (Abc_Obj_t *)Abc_ObjName(pObj);
    Abc_NtkForEachBox( pNtk, pObj, i )
        pObj->pCopy = (Abc_Obj_t *)Abc_ObjName(Abc_ObjFanout0(pObj));
    // order objects alphabetically
    qsort( (void *)Vec_PtrArray(pNtk->vPis), (size_t)Vec_PtrSize(pNtk->vPis), sizeof(Abc_Obj_t *), 
        (int (*)(const void *, const void *)) Abc_NodeCompareNames );
    qsort( (void *)Vec_PtrArray(pNtk->vPos), (size_t)Vec_PtrSize(pNtk->vPos), sizeof(Abc_Obj_t *), 
        (int (*)(const void *, const void *)) Abc_NodeCompareNames );
    // if the comparison if combinational (latches as PIs/POs), order them too
    if ( fComb )
        qsort( (void *)Vec_PtrArray(pNtk->vBoxes), (size_t)Vec_PtrSize(pNtk->vBoxes), sizeof(Abc_Obj_t *), 
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

  Synopsis    [Creates name manager storing input/output names.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Nam_t * Abc_NtkNameMan( Abc_Ntk_t * p, int fOuts )
{
    if ( fOuts )
    {
        Abc_Obj_t * pObj;  int i;
        Abc_Nam_t * pStrsCo = Abc_NamStart( Abc_NtkCoNum(p), 24 );
        Abc_NtkForEachCo( p, pObj, i )
            Abc_NamStrFindOrAdd( pStrsCo, Abc_ObjName(pObj), NULL );
        assert( Abc_NamObjNumMax(pStrsCo) == i + 1 );
        return pStrsCo;
    }
    else
    {
        Abc_Obj_t * pObj;  int i;
        Abc_Nam_t * pStrsCi = Abc_NamStart( Abc_NtkCiNum(p), 24 );
        Abc_NtkForEachCi( p, pObj, i )
            Abc_NamStrFindOrAdd( pStrsCi, Abc_ObjName(pObj), NULL );
        assert( Abc_NamObjNumMax(pStrsCi) == i + 1 );
        return pStrsCi;
   }
}

/**Function*************************************************************

  Synopsis    [Orders PIs/POs/latches alphabetically.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeCompareIndexes( Abc_Obj_t ** pp1, Abc_Obj_t ** pp2 )
{
    int Diff = (*pp1)->iTemp - (*pp2)->iTemp;
    if ( Diff < 0 )
        return -1;
    if ( Diff > 0 ) 
        return 1;
    return 0; 
}
void Abc_NtkTransferOrder( Abc_Ntk_t * pNtkOld, Abc_Ntk_t * pNtkNew )
{
    Abc_Obj_t * pObj;  int i;
    Abc_Nam_t * pStrsCi = Abc_NtkNameMan( pNtkOld, 0 );
    Abc_Nam_t * pStrsCo = Abc_NtkNameMan( pNtkOld, 1 );
    assert( Abc_NtkPiNum(pNtkOld) == Abc_NtkPiNum(pNtkNew) );
    assert( Abc_NtkPoNum(pNtkOld) == Abc_NtkPoNum(pNtkNew) );
    assert( Abc_NtkLatchNum(pNtkOld) == Abc_NtkLatchNum(pNtkNew) );
    // transfer to the new network
    Abc_NtkForEachCi( pNtkNew, pObj, i )
    {
        pObj->iTemp = Abc_NamStrFind(pStrsCi, Abc_ObjName(pObj));
        assert( pObj->iTemp > 0 && pObj->iTemp <= Abc_NtkCiNum(pNtkNew) );
    }
    Abc_NtkForEachCo( pNtkNew, pObj, i )
    {
        pObj->iTemp = Abc_NamStrFind(pStrsCo, Abc_ObjName(pObj));
        assert( pObj->iTemp > 0 && pObj->iTemp <= Abc_NtkCoNum(pNtkNew) );
    }
    Abc_NamDeref( pStrsCi );
    Abc_NamDeref( pStrsCo );
    // order PI/PO 
    qsort( (void *)Vec_PtrArray(pNtkNew->vPis), (size_t)Vec_PtrSize(pNtkNew->vPis), sizeof(Abc_Obj_t *), 
        (int (*)(const void *, const void *)) Abc_NodeCompareIndexes );
    qsort( (void *)Vec_PtrArray(pNtkNew->vPos), (size_t)Vec_PtrSize(pNtkNew->vPos), sizeof(Abc_Obj_t *), 
        (int (*)(const void *, const void *)) Abc_NodeCompareIndexes );
    // order CI/CO 
    qsort( (void *)Vec_PtrArray(pNtkNew->vCis), (size_t)Vec_PtrSize(pNtkNew->vCis), sizeof(Abc_Obj_t *), 
        (int (*)(const void *, const void *)) Abc_NodeCompareIndexes );
    qsort( (void *)Vec_PtrArray(pNtkNew->vCos), (size_t)Vec_PtrSize(pNtkNew->vCos), sizeof(Abc_Obj_t *), 
        (int (*)(const void *, const void *)) Abc_NodeCompareIndexes );
    // order CIs/COs first PIs/POs(Asserts) then latches
    //Abc_NtkOrderCisCos( pNtk );
    // clean the copy fields
    Abc_NtkForEachCi( pNtkNew, pObj, i )
        pObj->iTemp = 0;
    Abc_NtkForEachCo( pNtkNew, pObj, i )
        pObj->iTemp = 0;
}

/**Function*************************************************************

  Synopsis    [Checks that the order and number of CI/CO is the same.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeCompareCiCo( Abc_Ntk_t * pNtkOld, Abc_Ntk_t * pNtkNew )
{
    int i;
    if ( Abc_NtkPiNum(pNtkOld) != Abc_NtkPiNum(pNtkNew) )
        return 0;
    if ( Abc_NtkPoNum(pNtkOld) != Abc_NtkPoNum(pNtkNew) )
        return 0;
    if ( Abc_NtkLatchNum(pNtkOld) != Abc_NtkLatchNum(pNtkNew) )
        return 0;
    for ( i = 0; i < Abc_NtkCiNum(pNtkOld); i++ )
        if ( strcmp(Abc_ObjName(Abc_NtkCi(pNtkOld, i)), Abc_ObjName(Abc_NtkCi(pNtkNew, i))) )
            return 0;
    for ( i = 0; i < Abc_NtkCoNum(pNtkOld); i++ )
        if ( strcmp(Abc_ObjName(Abc_NtkCo(pNtkOld, i)), Abc_ObjName(Abc_NtkCo(pNtkNew, i))) )
            return 0;
    return 1;
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
    nDigits = Abc_Base10Log( Abc_NtkPiNum(pNtk) );
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
    nDigits = Abc_Base10Log( Abc_NtkPoNum(pNtk) );
    Abc_NtkForEachPo( pNtk, pObj, i )
        Abc_ObjAssignName( pObj, Abc_ObjNameDummy("po", i, nDigits), NULL );
}

/**Function*************************************************************

  Synopsis    [Adds dummy names.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkAddDummyBoxNames( Abc_Ntk_t * pNtk )
{
    char * pName, PrefLi[100], PrefLo[100];
    Abc_Obj_t * pObj;
    int nDigits, i, k, CountCur, CountMax = 0;
    // if PIs/POs already have nodes with what looks like latch names
    // we need to add different prefix for the new latches
    Abc_NtkForEachPi( pNtk, pObj, i )
    {
        CountCur = 0;
        pName = Abc_ObjName(pObj);
        for ( k = 0; pName[k]; k++ )
            if ( pName[k] == 'l' )
                CountCur++;
            else
                break;
        CountMax = Abc_MaxInt( CountMax, CountCur );
    }
    Abc_NtkForEachPo( pNtk, pObj, i )
    {
        CountCur = 0;
        pName = Abc_ObjName(pObj);
        for ( k = 0; pName[k]; k++ )
            if ( pName[k] == 'l' )
                CountCur++;
            else
                break;
        CountMax = Abc_MaxInt( CountMax, CountCur );
    }
//printf( "CountMax = %d\n", CountMax );
    assert( CountMax < 100-2 );
    for ( i = 0; i <= CountMax; i++ )
        PrefLi[i] = PrefLo[i] = 'l';
    PrefLi[i] = 'i';
    PrefLo[i] = 'o';
    PrefLi[i+1] = 0;
    PrefLo[i+1] = 0;
    // create latch names
    assert( !Abc_NtkIsNetlist(pNtk) );
    nDigits = Abc_Base10Log( Abc_NtkLatchNum(pNtk) );
    Abc_NtkForEachLatch( pNtk, pObj, i )
    {
        Abc_ObjAssignName( pObj, Abc_ObjNameDummy("l", i, nDigits), NULL );
        Abc_ObjAssignName( Abc_ObjFanin0(pObj),  Abc_ObjNameDummy(PrefLi, i, nDigits), NULL );
        Abc_ObjAssignName( Abc_ObjFanout0(pObj), Abc_ObjNameDummy(PrefLo, i, nDigits), NULL );
    }
/*
    nDigits = Abc_Base10Log( Abc_NtkBlackboxNum(pNtk) );
    Abc_NtkForEachBlackbox( pNtk, pObj, i )
    {
        pName = Abc_ObjAssignName( pObj, Abc_ObjNameDummy("B", i, nDigits), NULL );
        nDigitsF = Abc_Base10Log( Abc_ObjFaninNum(pObj) );
        Abc_ObjForEachFanin( pObj, pTerm, k )
            Abc_ObjAssignName( Abc_ObjFanin0(pObj), pName, Abc_ObjNameDummy("i", k, nDigitsF) );
        nDigitsF = Abc_Base10Log( Abc_ObjFanoutNum(pObj) );
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
    Abc_NtkAddDummyBoxNames( pNtk );
}
void Abc_NtkCleanNames( Abc_Ntk_t * pNtk )
{  
    Abc_Obj_t * pObj; int i;
    Nm_Man_t * pManName = Nm_ManCreate( Abc_NtkCiNum(pNtk) + Abc_NtkCoNum(pNtk) + Abc_NtkBoxNum(pNtk) );
    Abc_NtkForEachCi( pNtk, pObj, i )
        Nm_ManStoreIdName( pManName, pObj->Id, pObj->Type, Abc_ObjName(pObj), NULL );
    Abc_NtkForEachCo( pNtk, pObj, i )
        Nm_ManStoreIdName( pManName, pObj->Id, pObj->Type, Abc_ObjName(pObj), NULL );
    Nm_ManFree( pNtk->pManName );
    pNtk->pManName = pManName;
}

/**Function*************************************************************

  Synopsis    [Moves names from the other network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkRedirectCiCo( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj, * pObjCi, * pFanin; 
    int i, Count = 0;
    // if CO points to CI with the same name, remove buffer between them
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        int nCiId = Nm_ManFindIdByNameTwoTypes( pNtk->pManName, Abc_ObjName(pObj), ABC_OBJ_PI, ABC_OBJ_BO );
        if ( nCiId == -1 )
            continue;
        pObjCi = Abc_NtkObj( pNtk, nCiId );
        assert( !strcmp( Abc_ObjName(pObj), Abc_ObjName(pObjCi) ) );
        pFanin = Abc_ObjFanin0(pObj);
        if ( pFanin == pObjCi )
            continue;
        assert( Abc_NodeIsBuf(pFanin) );
        Abc_ObjPatchFanin( pObj, pFanin, pObjCi );
        if ( Abc_ObjFanoutNum(pFanin) == 0 )
            Abc_NtkDeleteObj( pFanin );
        Count++;
    }
    if ( Count )
        printf( "Redirected %d POs from buffers to PIs with the same name.\n", Count );
}
void Abc_NtkMoveNames( Abc_Ntk_t * pNtk, Abc_Ntk_t * pOld )
{
    Abc_Obj_t * pObj; int i;
    Nm_ManFree( pNtk->pManName );
    pNtk->pManName = Nm_ManCreate( Abc_NtkCiNum(pNtk) + Abc_NtkCoNum(pNtk) + Abc_NtkBoxNum(pNtk) );
    Abc_NtkForEachPi( pNtk, pObj, i )
        Abc_ObjAssignName( pObj, Abc_ObjName(Abc_NtkPi(pOld, i)), NULL );
    Abc_NtkForEachPo( pNtk, pObj, i )
        Abc_ObjAssignName( pObj, Abc_ObjName(Abc_NtkPo(pOld, i)), NULL );
    Abc_NtkForEachLatch( pNtk, pObj, i )
    {
        Abc_ObjAssignName( Abc_ObjFanin0(pObj),  Abc_ObjName(Abc_ObjFanin0(Abc_NtkBox(pOld, i))),  NULL );
        Abc_ObjAssignName( Abc_ObjFanout0(pObj), Abc_ObjName(Abc_ObjFanout0(Abc_NtkBox(pOld, i))), NULL );
    }
    Abc_NtkRedirectCiCo( pNtk );
}


/**Function*************************************************************

  Synopsis    [Saves name IDs into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkStartNameIds( Abc_Ntk_t * p )
{
    char pFileName[1000];
    FILE * pFile;
    Abc_Obj_t * pObj, * pFanin;
    Vec_Ptr_t * vNodes;
    int i, Counter = 1;
    assert( Abc_NtkIsNetlist(p) );
    assert( p->vNameIds == NULL );
    assert( strlen(p->pSpec) < 1000 );
    sprintf( pFileName, "%s_%s_names.txt", Extra_FileNameGenericAppend(p->pSpec,""), Extra_FileNameExtension(p->pSpec) );
    pFile = fopen( pFileName, "wb" );
    p->vNameIds = Vec_IntStart( Abc_NtkObjNumMax(p) );
    // add inputs
    Abc_NtkForEachCi( p, pObj, i )
        fprintf( pFile, "%s            \n", Abc_ObjName(Abc_ObjFanout0(pObj)) ), Vec_IntWriteEntry(p->vNameIds, Abc_ObjId(pObj), 2*Counter++);
    // add outputs
    Abc_NtkForEachCo( p, pObj, i )
    {
        pFanin = Abc_ObjFanin0(Abc_ObjFanin0(pObj));
        if ( !Vec_IntEntry(p->vNameIds, Abc_ObjId(pFanin)) )
            fprintf( pFile, "%s            \n", Abc_ObjName(Abc_ObjFanout0(pFanin)) ), Vec_IntWriteEntry(p->vNameIds, Abc_ObjId(pFanin), 2*Counter++);
    }
    // add nodes in a topo order
    vNodes = Abc_NtkDfs( p, 1 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
        if ( !Vec_IntEntry(p->vNameIds, Abc_ObjId(pObj)) )
            fprintf( pFile, "%s            \n", Abc_ObjName(Abc_ObjFanout0(pObj)) ), Vec_IntWriteEntry(p->vNameIds, Abc_ObjId(pObj), 2*Counter++);
    Vec_PtrFree( vNodes );
    fclose( pFile );
    // transfer driver node names to COs
    Abc_NtkForEachCo( p, pObj, i )
    {
        pFanin = Abc_ObjFanin0(Abc_ObjFanin0(pObj));
        Vec_IntWriteEntry( p->vNameIds, Abc_ObjId(pObj), Vec_IntEntry(p->vNameIds, Abc_ObjId(pFanin)) );
        Vec_IntWriteEntry( p->vNameIds, Abc_ObjId(pFanin), 0 );
    }
}

/**Function*************************************************************

  Synopsis    [Remaps the AIG from the old manager into the new manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkTransferNameIds( Abc_Ntk_t * p, Abc_Ntk_t * pNew )
{
    Abc_Obj_t * pObj, * pObjNew;
    int i;
    assert( p->vNameIds != NULL );
    assert( pNew->vNameIds == NULL );
    pNew->vNameIds = Vec_IntStart( Abc_NtkObjNumMax(pNew) );
//    Abc_NtkForEachCi( p, pObj, i )
//        printf( "%d ", Vec_IntEntry(p->vNameIds, Abc_ObjId(pObj)) );
//    printf( "\n" );
    Abc_NtkForEachObj( p, pObj, i )
        if ( pObj->pCopy && i < Vec_IntSize(p->vNameIds) && Vec_IntEntry(p->vNameIds, i) )
        {
            pObjNew = Abc_ObjRegular(pObj->pCopy);
            assert( Abc_ObjNtk(pObjNew) == pNew );
            if ( Abc_ObjIsCi(pObjNew) && !Abc_ObjIsCi(pObj) ) // do not overwrite CI name by internal node name
                continue;
            Vec_IntWriteEntry( pNew->vNameIds, Abc_ObjId(pObjNew), Vec_IntEntry(p->vNameIds, i) ^ Abc_ObjIsComplement(pObj->pCopy) );
        }
}

/**Function*************************************************************

  Synopsis    [Updates file with name IDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkUpdateNameIds( Abc_Ntk_t * p )
{
    char pFileName[1000];
    Vec_Int_t * vStarts;
    Abc_Obj_t * pObj;
    FILE * pFile;
    int i, c, iVar, fCompl, fSeenSpace, Counter = 0;
    assert( !Abc_NtkIsNetlist(p) );
    assert( strlen(p->pSpec) < 1000 );
    assert( p->vNameIds != NULL );
    sprintf( pFileName, "%s_%s_names.txt", Extra_FileNameGenericAppend(p->pSpec,""), Extra_FileNameExtension(p->pSpec) );
    pFile = fopen( pFileName, "r+" );
    // collect info about lines
    fSeenSpace = 0;
    vStarts = Vec_IntAlloc( 1000 );
    Vec_IntPush( vStarts, -1 );
    while ( (c = fgetc(pFile)) != EOF && ++Counter )
        if ( c == ' ' && !fSeenSpace )
            Vec_IntPush(vStarts, Counter), fSeenSpace = 1;
        else if ( c == '\n' )
            fSeenSpace = 0;
    // add info about names
    Abc_NtkForEachObj( p, pObj, i )
    {
        if ( i == 0 || i >= Vec_IntSize(p->vNameIds) || !Vec_IntEntry(p->vNameIds, i) )
            continue;
        iVar = Abc_Lit2Var( Vec_IntEntry(p->vNameIds, i) );
        fCompl = Abc_LitIsCompl( Vec_IntEntry(p->vNameIds, i) );
        assert( iVar < Vec_IntSize(vStarts) );
        fseek( pFile, Vec_IntEntry(vStarts, iVar), SEEK_SET );
        fprintf( pFile, "%s%d", fCompl? "-":"", i );
    }
    printf( "Saved %d names into file \"%s\".\n", Vec_IntSize(vStarts)-1, pFileName );
    fclose( pFile );
    Vec_IntFree( vStarts );
    Vec_IntFreeP( &p->vNameIds );
//    Abc_NtkForEachObj( p, pObj, i )
//        Abc_ObjPrint( stdout, pObj );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

