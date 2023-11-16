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

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Recursively flattens logic hierarchy of the netlist.]

  Description [When this procedure is called, the PI/PO nets of the old 
  netlist point to the corresponding nets of the flattened netlist.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkFlattenLogicHierarchy2_rec( Abc_Ntk_t * pNtkNew, Abc_Ntk_t * pNtk, int * pCounter )
{
    char Suffix[2000] = {0};
    Abc_Ntk_t * pNtkModel;
    Abc_Obj_t * pObj, * pTerm, * pNet, * pFanin;
    int i, k;

    // process the blackbox
    if ( Abc_NtkHasBlackbox(pNtk) )
    {
        // duplicate the blackbox
        assert( Abc_NtkBoxNum(pNtk) == 1 );
        pObj = Abc_NtkBox( pNtk, 0 );
        Abc_NtkDupBox( pNtkNew, pObj, 1 );
        pObj->pCopy->pData = pNtk;

        // connect blackbox fanins to the PI nets
        assert( Abc_ObjFaninNum(pObj->pCopy) == Abc_NtkPiNum(pNtk) );
        Abc_NtkForEachPi( pNtk, pTerm, i )
            Abc_ObjAddFanin( Abc_ObjFanin(pObj->pCopy,i), Abc_ObjFanout0(pTerm)->pCopy );

        // connect blackbox fanouts to the PO nets
        assert( Abc_ObjFanoutNum(pObj->pCopy) == Abc_NtkPoNum(pNtk) );
        Abc_NtkForEachPo( pNtk, pTerm, i )
            Abc_ObjAddFanin( Abc_ObjFanin0(pTerm)->pCopy, Abc_ObjFanout(pObj->pCopy,i) );
        return;
    }

    (*pCounter)++;

    // create the suffix, which will be appended to the internal names
    if ( *pCounter )
        sprintf( Suffix, "_%s_%d", Abc_NtkName(pNtk), *pCounter );

    // duplicate nets of all boxes, including latches
    Abc_NtkForEachBox( pNtk, pObj, i )
    {
        Abc_ObjForEachFanin( pObj, pTerm, k )
        {
            pNet = Abc_ObjFanin0(pTerm);
            if ( pNet->pCopy )
                continue;
            pNet->pCopy = Abc_NtkFindOrCreateNet( pNtkNew, Abc_ObjNameSuffix(pNet, Suffix) );
        }
        Abc_ObjForEachFanout( pObj, pTerm, k )
        {
            pNet = Abc_ObjFanout0(pTerm);
            if ( pNet->pCopy )
                continue;
            pNet->pCopy = Abc_NtkFindOrCreateNet( pNtkNew, Abc_ObjNameSuffix(pNet, Suffix) );
        }
    }

    // mark objects that will not be used
    Abc_NtkIncrementTravId( pNtk );
    Abc_NtkForEachPi( pNtk, pTerm, i )
        Abc_NodeSetTravIdCurrent( pTerm );
    Abc_NtkForEachPo( pNtk, pTerm, i )
    {
        Abc_NodeSetTravIdCurrent( pTerm );
        // if the netlist has net names beginning with "abc_property_"
        // these names will be addes as primary outputs of the network
        pNet = Abc_ObjFanin0(pTerm);
        if ( strncmp( Abc_ObjName(pNet), "abc_property", 12 ) )
            continue;
        Abc_ObjAddFanin( Abc_NtkCreatePo(pNet->pCopy->pNtk), pNet->pCopy );
        if ( Nm_ManFindNameById(pNet->pCopy->pNtk->pManName, pNet->pCopy->Id) )
            Nm_ManDeleteIdName(pNet->pCopy->pNtk->pManName, pNet->pCopy->Id);
        Abc_ObjAssignName( pNet->pCopy, Abc_ObjName(pNet), Suffix );
    }
    Abc_NtkForEachBox( pNtk, pObj, i )
    {
        if ( Abc_ObjIsLatch(pObj) )
            continue;
        Abc_NodeSetTravIdCurrent( pObj );
        Abc_ObjForEachFanin( pObj, pTerm, k )
            Abc_NodeSetTravIdCurrent( pTerm );
        Abc_ObjForEachFanout( pObj, pTerm, k )
            Abc_NodeSetTravIdCurrent( pTerm );
    }

    // duplicate objects that do not have prototypes yet
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        if ( Abc_NodeIsTravIdCurrent(pObj) )
            continue;
        if ( pObj->pCopy )
            continue;
        Abc_NtkDupObj( pNtkNew, pObj, 0 );
    }

    // connect objects
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( !Abc_NodeIsTravIdCurrent(pObj) )
            Abc_ObjForEachFanin( pObj, pFanin, k )
                if ( !Abc_NodeIsTravIdCurrent(pFanin) )
                    Abc_ObjAddFanin( pObj->pCopy, pFanin->pCopy );

    // call recursively
    Abc_NtkForEachBox( pNtk, pObj, i )
    {
        if ( Abc_ObjIsLatch(pObj) )
            continue;
        pNtkModel = (Abc_Ntk_t *)pObj->pData;
        // check the match between the number of actual and formal parameters
        assert( Abc_ObjFaninNum(pObj) == Abc_NtkPiNum(pNtkModel) );
        assert( Abc_ObjFanoutNum(pObj) == Abc_NtkPoNum(pNtkModel) );
        // clean the node copy fields
        Abc_NtkCleanCopy( pNtkModel );
        // map PIs/POs
        Abc_ObjForEachFanin( pObj, pTerm, k )
            Abc_ObjFanout0( Abc_NtkPi(pNtkModel, k) )->pCopy = Abc_ObjFanin0(pTerm)->pCopy;
        Abc_ObjForEachFanout( pObj, pTerm, k )
            Abc_ObjFanin0( Abc_NtkPo(pNtkModel, k) )->pCopy = Abc_ObjFanout0(pTerm)->pCopy;
        // call recursively
        Abc_NtkFlattenLogicHierarchy2_rec( pNtkNew, pNtkModel, pCounter );
    }

    // if it is a BLIF-MV netlist transfer the values of all nets
    if ( Abc_NtkHasBlifMv(pNtk) && Abc_NtkMvVar(pNtk) )
    {
        if ( Abc_NtkMvVar( pNtkNew ) == NULL )
            Abc_NtkStartMvVars( pNtkNew );
        Abc_NtkForEachNet( pNtk, pObj, i )
            Abc_NtkSetMvVarValues( pObj->pCopy, Abc_ObjMvVarNum(pObj) );
    }
}

/**Function*************************************************************

  Synopsis    [Flattens the logic hierarchy of the netlist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkFlattenLogicHierarchy2( Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pNtkNew; 
    Abc_Obj_t * pTerm, * pNet;
    int i, Counter;
    extern Abc_Des_t * Abc_DesDupBlackboxes( Abc_Des_t * p, Abc_Ntk_t * pNtkSave );

    assert( Abc_NtkIsNetlist(pNtk) );
    // start the network
    pNtkNew = Abc_NtkAlloc( pNtk->ntkType, pNtk->ntkFunc, 1 );
    // duplicate the name and the spec
    pNtkNew->pName = Abc_UtilStrsav(pNtk->pName);
    pNtkNew->pSpec = Abc_UtilStrsav(pNtk->pSpec);

    // clean the node copy fields
    Abc_NtkCleanCopy( pNtk );

    // duplicate PIs/POs and their nets
    Abc_NtkForEachPi( pNtk, pTerm, i )
    {
        Abc_NtkDupObj( pNtkNew, pTerm, 0 );
        pNet = Abc_ObjFanout0( pTerm );
        pNet->pCopy = Abc_NtkFindOrCreateNet( pNtkNew, Abc_ObjName(pNet) );
        Abc_ObjAddFanin( pNet->pCopy, pTerm->pCopy );
    }
    Abc_NtkForEachPo( pNtk, pTerm, i )
    {
        Abc_NtkDupObj( pNtkNew, pTerm, 0 );
        pNet = Abc_ObjFanin0( pTerm );
        pNet->pCopy = Abc_NtkFindOrCreateNet( pNtkNew, Abc_ObjName(pNet) );
        Abc_ObjAddFanin( pTerm->pCopy, pNet->pCopy );
    }

    // recursively flatten hierarchy, create internal logic, add new PI/PO names if there are black boxes
    Counter = -1;
    Abc_NtkFlattenLogicHierarchy2_rec( pNtkNew, pNtk, &Counter );
    printf( "Hierarchy reader flattened %d instances of logic boxes and left %d black boxes.\n", 
        Counter, Abc_NtkBlackboxNum(pNtkNew) );

    if ( pNtk->pDesign )
    {
        // pass on the design
        assert( Vec_PtrEntry(pNtk->pDesign->vTops, 0) == pNtk );
        pNtkNew->pDesign = Abc_DesDupBlackboxes( pNtk->pDesign, pNtkNew );
        // update the pointers
        Abc_NtkForEachBlackbox( pNtkNew, pTerm, i )
            pTerm->pData = ((Abc_Ntk_t *)pTerm->pData)->pCopy;
    }

    // we may have added property outputs
    Abc_NtkOrderCisCos( pNtkNew );

    // copy the timing information
//    Abc_ManTimeDup( pNtk, pNtkNew );
    // duplicate EXDC 
    if ( pNtk->pExdc )
        printf( "EXDC is not transformed.\n" );
    if ( !Abc_NtkCheck( pNtkNew ) )
    {
        fprintf( stdout, "Abc_NtkFlattenLogicHierarchy2(): Network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;
}



/**Function*************************************************************

  Synopsis    [Recursively flattens logic hierarchy of the netlist.]

  Description [When this procedure is called, the PI/PO nets of the old 
  netlist point to the corresponding nets of the flattened netlist.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkFlattenLogicHierarchy_rec( Abc_Ntk_t * pNtkNew, Abc_Ntk_t * pNtk, int * pCounter, Vec_Str_t * vPref )
{
    Abc_Ntk_t * pNtkModel;
    Abc_Obj_t * pObj, * pTerm, * pNet, * pFanin;
    int i, k, Length;

    // process the blackbox
    if ( Abc_NtkHasBlackbox(pNtk) )
    {
        //printf( "Flatting black box \"%s\".\n", pNtk->pName );
        // duplicate the blackbox
        assert( Abc_NtkBoxNum(pNtk) == 1 );
        pObj = Abc_NtkBox( pNtk, 0 );
        Abc_NtkDupBox( pNtkNew, pObj, 1 );
        pObj->pCopy->pData = pNtk;

        // connect blackbox fanins to the PI nets
        assert( Abc_ObjFaninNum(pObj->pCopy) == Abc_NtkPiNum(pNtk) );
        Abc_NtkForEachPi( pNtk, pTerm, i )
            Abc_ObjAddFanin( Abc_ObjFanin(pObj->pCopy,i), Abc_ObjFanout0(pTerm)->pCopy );

        // connect blackbox fanouts to the PO nets
        assert( Abc_ObjFanoutNum(pObj->pCopy) == Abc_NtkPoNum(pNtk) );
        Abc_NtkForEachPo( pNtk, pTerm, i )
            Abc_ObjAddFanin( Abc_ObjFanin0(pTerm)->pCopy, Abc_ObjFanout(pObj->pCopy,i) );
        return;
    }

    (*pCounter)++;

    // create the suffix, which will be appended to the internal names
    if ( *pCounter )
    {
        char Buffer[20];
        sprintf( Buffer, "(%d)", *pCounter );
        Vec_StrPrintStr( vPref, Buffer );
    }
    Vec_StrPush( vPref, '|' );
    Vec_StrPush( vPref, 0 );

    // duplicate nets of all boxes, including latches
    Abc_NtkForEachBox( pNtk, pObj, i )
    {
        Abc_ObjForEachFanin( pObj, pTerm, k )
        {
            pNet = Abc_ObjFanin0(pTerm);
            if ( pNet->pCopy )
                continue;
            pNet->pCopy = Abc_NtkFindOrCreateNet( pNtkNew, Abc_ObjNamePrefix(pNet, Vec_StrArray(vPref)) );
        }
        Abc_ObjForEachFanout( pObj, pTerm, k )
        {
            pNet = Abc_ObjFanout0(pTerm);
            if ( pNet->pCopy )
                continue;
            pNet->pCopy = Abc_NtkFindOrCreateNet( pNtkNew, Abc_ObjNamePrefix(pNet, Vec_StrArray(vPref)) );
        }
    }

    // mark objects that will not be used
    Abc_NtkIncrementTravId( pNtk );
    Abc_NtkForEachPi( pNtk, pTerm, i )
        Abc_NodeSetTravIdCurrent( pTerm );
    Abc_NtkForEachPo( pNtk, pTerm, i )
    {
        Abc_NodeSetTravIdCurrent( pTerm );
        // if the netlist has net names beginning with "abc_property_"
        // these names will be addes as primary outputs of the network
        pNet = Abc_ObjFanin0(pTerm);
        if ( strncmp( Abc_ObjName(pNet), "abc_property", 12 ) )
            continue;
        Abc_ObjAddFanin( Abc_NtkCreatePo(pNet->pCopy->pNtk), pNet->pCopy );
        if ( Nm_ManFindNameById(pNet->pCopy->pNtk->pManName, pNet->pCopy->Id) )
            Nm_ManDeleteIdName(pNet->pCopy->pNtk->pManName, pNet->pCopy->Id);
        Abc_ObjAssignName( pNet->pCopy, Vec_StrArray(vPref), Abc_ObjName(pNet) );
    }
    Abc_NtkForEachBox( pNtk, pObj, i )
    {
        if ( Abc_ObjIsLatch(pObj) )
            continue;
        Abc_NodeSetTravIdCurrent( pObj );
        Abc_ObjForEachFanin( pObj, pTerm, k )
            Abc_NodeSetTravIdCurrent( pTerm );
        Abc_ObjForEachFanout( pObj, pTerm, k )
            Abc_NodeSetTravIdCurrent( pTerm );
    }

    // duplicate objects that do not have prototypes yet
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        if ( Abc_NodeIsTravIdCurrent(pObj) )
            continue;
        if ( pObj->pCopy )
            continue;
        Abc_NtkDupObj( pNtkNew, pObj, 0 );
    }

    // connect objects
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( !Abc_NodeIsTravIdCurrent(pObj) )
            Abc_ObjForEachFanin( pObj, pFanin, k )
                if ( !Abc_NodeIsTravIdCurrent(pFanin) )
                    Abc_ObjAddFanin( pObj->pCopy, pFanin->pCopy );

    // call recursively
    Vec_StrPop( vPref );
    Length = Vec_StrSize( vPref );
    Abc_NtkForEachBox( pNtk, pObj, i )
    {
        if ( Abc_ObjIsLatch(pObj) )
            continue;
        pNtkModel = (Abc_Ntk_t *)pObj->pData;
        // check the match between the number of actual and formal parameters
        assert( Abc_ObjFaninNum(pObj) == Abc_NtkPiNum(pNtkModel) );
        assert( Abc_ObjFanoutNum(pObj) == Abc_NtkPoNum(pNtkModel) );
        // clean the node copy fields
        Abc_NtkCleanCopy( pNtkModel );
        // map PIs/POs
        Abc_ObjForEachFanin( pObj, pTerm, k )
            Abc_ObjFanout0( Abc_NtkPi(pNtkModel, k) )->pCopy = Abc_ObjFanin0(pTerm)->pCopy;
        Abc_ObjForEachFanout( pObj, pTerm, k )
            Abc_ObjFanin0( Abc_NtkPo(pNtkModel, k) )->pCopy = Abc_ObjFanout0(pTerm)->pCopy;
        // create name
        Vec_StrShrink( vPref, Length );
        Vec_StrPrintStr( vPref, Abc_NtkName(pNtkModel) );
        // call recursively
        Abc_NtkFlattenLogicHierarchy_rec( pNtkNew, pNtkModel, pCounter, vPref );
    }

    // if it is a BLIF-MV netlist transfer the values of all nets
    if ( Abc_NtkHasBlifMv(pNtk) && Abc_NtkMvVar(pNtk) )
    {
        if ( Abc_NtkMvVar( pNtkNew ) == NULL )
            Abc_NtkStartMvVars( pNtkNew );
        Abc_NtkForEachNet( pNtk, pObj, i )
            Abc_NtkSetMvVarValues( pObj->pCopy, Abc_ObjMvVarNum(pObj) );
    }
}

/**Function*************************************************************

  Synopsis    [Returns 0 if CI names are repeated.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCompareNames( Abc_Ntk_t ** p1, Abc_Ntk_t ** p2 )
{
    return strcmp( Abc_NtkName(*p1), Abc_NtkName(*p2) );
}

/**Function*************************************************************

  Synopsis    [Prints information about boxes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPrintBoxInfo( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vMods;
    Abc_Ntk_t * pModel, * pBoxModel;
    Abc_Obj_t * pObj;
    Vec_Int_t * vCounts;
    int i, k, Num;
    if ( pNtk->pDesign == NULL || pNtk->pDesign->vModules == NULL )
    {
//        printf( "There is no hierarchy information.\n" );
        return;
    }
    // sort models by name
    vMods = pNtk->pDesign->vModules;
    Vec_PtrSort( vMods, (int(*)(const void *, const void *))Abc_NtkCompareNames );
//    Vec_PtrForEachEntry( Abc_Ntk_t *, vMods, pModel, i )
//        printf( "%s\n", Abc_NtkName(pModel) );

    // swap the first model
    Num = Vec_PtrFind( vMods, pNtk );
    assert( Num >= 0 && Num < Vec_PtrSize(vMods) );
    pBoxModel = (Abc_Ntk_t *)Vec_PtrEntry(vMods, 0);
    Vec_PtrWriteEntry(vMods, 0, (Abc_Ntk_t *)Vec_PtrEntry(vMods, Num) );
    Vec_PtrWriteEntry(vMods, Num, pBoxModel );

    // print models
    vCounts = Vec_IntStart( Vec_PtrSize(vMods) );
    Vec_PtrForEachEntry( Abc_Ntk_t *, vMods, pModel, i )
    {
        if ( Abc_NtkBoxNum(pModel) == 0 )
            continue;
        Vec_IntFill( vCounts, Vec_IntSize(vCounts), 0 );
        Abc_NtkForEachBox( pModel, pObj, k )
        {
            pBoxModel = (Abc_Ntk_t *)pObj->pData;
            if ( pBoxModel == NULL )
                continue;
            Num = Vec_PtrFind( vMods, pBoxModel );
            assert( Num >= 0 && Num < Vec_PtrSize(vMods) );
            Vec_IntAddToEntry( vCounts, Num, 1 );
        }

//        Abc_NtkPrintStats( pModel, 0, 0, 0, 0, 0, 0, 0, 0, 0 );
        printf( "MODULE  " );
        printf( "%-30s : ", Abc_NtkName(pModel) );
        printf( "PI=%6d ", Abc_NtkPiNum(pModel) );
        printf( "PO=%6d ", Abc_NtkPoNum(pModel) );
        printf( "BB=%6d ", Abc_NtkBoxNum(pModel) );
        printf( "ND=%6d ", Abc_NtkNodeNum(pModel) ); // sans constants
        printf( "Lev=%5d ", Abc_NtkLevel(pModel) );
        printf( "\n" );

        Vec_IntForEachEntry( vCounts, Num, k )
            if ( Num )
                printf( "%15d : %s\n", Num, Abc_NtkName((Abc_Ntk_t *)Vec_PtrEntry(vMods, k)) );
    }
    Vec_IntFree( vCounts );
    Vec_PtrForEachEntry( Abc_Ntk_t *, vMods, pModel, i )
    {
        if ( Abc_NtkBoxNum(pModel) != 0 )
            continue;
        printf( "MODULE   " );
        printf( "%-30s : ", Abc_NtkName(pModel) );
        printf( "PI=%6d ", Abc_NtkPiNum(pModel) );
        printf( "PO=%6d ", Abc_NtkPoNum(pModel) );
        printf( "BB=%6d ", Abc_NtkBoxNum(pModel) );
        printf( "ND=%6d ", Abc_NtkNodeNum(pModel) );
        printf( "Lev=%5d ", Abc_NtkLevel(pModel) );
        printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Flattens the logic hierarchy of the netlist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkFlattenLogicHierarchy( Abc_Ntk_t * pNtk )
{
    extern Abc_Des_t * Abc_DesDupBlackboxes( Abc_Des_t * p, Abc_Ntk_t * pNtkSave );
    Vec_Str_t * vPref;
    Abc_Ntk_t * pNtkNew; 
    Abc_Obj_t * pTerm, * pNet;
    int i, Counter = -1;

    assert( Abc_NtkIsNetlist(pNtk) );
//    Abc_NtkPrintBoxInfo( pNtk );

    // start the network
    pNtkNew = Abc_NtkAlloc( pNtk->ntkType, pNtk->ntkFunc, 1 );
    // duplicate the name and the spec
    pNtkNew->pName = Extra_UtilStrsav(pNtk->pName);
    pNtkNew->pSpec = Extra_UtilStrsav(pNtk->pSpec);

    // clean the node copy fields
    Abc_NtkCleanCopy( pNtk );

    // duplicate PIs/POs and their nets
    Abc_NtkForEachPi( pNtk, pTerm, i )
    {
        Abc_NtkDupObj( pNtkNew, pTerm, 0 );
        pNet = Abc_ObjFanout0( pTerm );
        pNet->pCopy = Abc_NtkFindOrCreateNet( pNtkNew, Abc_ObjName(pNet) );
        Abc_ObjAddFanin( pNet->pCopy, pTerm->pCopy );
    }
    Abc_NtkForEachPo( pNtk, pTerm, i )
    {
        Abc_NtkDupObj( pNtkNew, pTerm, 0 );
        pNet = Abc_ObjFanin0( pTerm );
        pNet->pCopy = Abc_NtkFindOrCreateNet( pNtkNew, Abc_ObjName(pNet) );
        Abc_ObjAddFanin( pTerm->pCopy, pNet->pCopy );
    }

    // recursively flatten hierarchy, create internal logic, add new PI/PO names if there are black boxes
    vPref = Vec_StrAlloc( 1000 );
    Vec_StrPrintStr( vPref, Abc_NtkName(pNtk) );
    Abc_NtkFlattenLogicHierarchy_rec( pNtkNew, pNtk, &Counter, vPref );
    printf( "Hierarchy reader flattened %d instances of logic boxes and left %d black boxes.\n", 
        Counter, Abc_NtkBlackboxNum(pNtkNew) );
    Vec_StrFree( vPref );

    if ( pNtk->pDesign )
    {
        // pass on the design
        assert( Vec_PtrEntry(pNtk->pDesign->vTops, 0) == pNtk );
        pNtkNew->pDesign = Abc_DesDupBlackboxes( pNtk->pDesign, pNtkNew );
        // update the pointers
        Abc_NtkForEachBlackbox( pNtkNew, pTerm, i )
            pTerm->pData = ((Abc_Ntk_t *)pTerm->pData)->pCopy;
    }

    // we may have added property outputs
    Abc_NtkOrderCisCos( pNtkNew );

    // copy the timing information
//    Abc_ManTimeDup( pNtk, pNtkNew );
    // duplicate EXDC 
    if ( pNtk->pExdc )
        printf( "EXDC is not transformed.\n" );
    if ( !Abc_NtkCheck( pNtkNew ) )
    {
        fprintf( stdout, "Abc_NtkFlattenLogicHierarchy(): Network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;
}


/**Function*************************************************************

  Synopsis    [Extracts blackboxes by making them into additional PIs/POs.]

  Description [The input netlist has not logic hierarchy. The resulting
  netlist has additional PIs/POs for each blackbox input/output.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkConvertBlackboxes( Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObj, * pNet, * pFanin, * pTerm;
    int i, k;

    assert( Abc_NtkIsNetlist(pNtk) );
    assert( Abc_NtkWhiteboxNum(pNtk) == 0 );

    // start the network
    pNtkNew = Abc_NtkAlloc( pNtk->ntkType, pNtk->ntkFunc, 1 );
    // duplicate the name and the spec
    pNtkNew->pName = Extra_UtilStrsav( pNtk->pName );
    pNtkNew->pSpec = Extra_UtilStrsav( pNtk->pSpec );

    // clean the node copy fields
    Abc_NtkCleanCopy( pNtk );

    // mark the nodes that should not be connected
    Abc_NtkIncrementTravId( pNtk );
    Abc_NtkForEachBlackbox( pNtk, pObj, i )
        Abc_NodeSetTravIdCurrent( pObj );
    Abc_NtkForEachCi( pNtk, pTerm, i )
        Abc_NodeSetTravIdCurrent( pTerm );
    Abc_NtkForEachCo( pNtk, pTerm, i )
        Abc_NodeSetTravIdCurrent( pTerm );
    // unmark PIs and LIs/LOs
    Abc_NtkForEachPi( pNtk, pTerm, i )
        Abc_NodeSetTravIdPrevious( pTerm );
    Abc_NtkForEachLatchInput( pNtk, pTerm, i )
        Abc_NodeSetTravIdPrevious( pTerm );
    Abc_NtkForEachLatchOutput( pNtk, pTerm, i )
        Abc_NodeSetTravIdPrevious( pTerm );
    // copy the box outputs
    Abc_NtkForEachBlackbox( pNtk, pObj, i )
        Abc_ObjForEachFanout( pObj, pTerm, k )
            pTerm->pCopy = Abc_NtkCreatePi( pNtkNew );

    // duplicate other objects
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( !Abc_NodeIsTravIdCurrent(pObj) )
            Abc_NtkDupObj( pNtkNew, pObj, Abc_ObjIsNet(pObj) );

    // connect all objects
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( !Abc_NodeIsTravIdCurrent(pObj) )
            Abc_ObjForEachFanin( pObj, pFanin, k )
                Abc_ObjAddFanin( pObj->pCopy, pFanin->pCopy );

    // create unique PO for each net feeding into blackboxes or POs
    Abc_NtkIncrementTravId( pNtk );
    Abc_NtkForEachCo( pNtk, pTerm, i )
    {
        // skip latch inputs
        assert( Abc_ObjFanoutNum(pTerm) <= 1 );
        if ( Abc_ObjFanoutNum(pTerm) > 0 && Abc_ObjIsLatch(Abc_ObjFanout0(pTerm)) )
            continue;
        // check if the net is visited
        pNet = Abc_ObjFanin0(pTerm);
        if ( Abc_NodeIsTravIdCurrent(pNet) )
            continue;
        // create PO
        Abc_NodeSetTravIdCurrent( pNet );
        Abc_ObjAddFanin( Abc_NtkCreatePo(pNtkNew), pNet->pCopy );
    }

    // check integrity
    if ( !Abc_NtkCheck( pNtkNew ) )
    {
        fprintf( stdout, "Abc_NtkConvertBlackboxes(): Network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Inserts blackboxes into the netlist.]

  Description [The first arg is the netlist with blackboxes without logic hierarchy.
  The second arg is a non-hierarchical netlist derived from the above netlist after processing.
  This procedure create a new netlist, which is comparable to the original netlist with
  blackboxes, except that it contains logic nodes from the netlist after processing.]
               
  SideEffects [This procedure silently assumes that blackboxes appear
  only in the top-level model. If they appear in other models as well, 
  the name of the model and its number were appended to the names of 
  blackbox inputs/outputs.]

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkInsertNewLogic( Abc_Ntk_t * pNtkH, Abc_Ntk_t * pNtkL )
{
    Abc_Des_t * pDesign;
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObjH, * pObjL, * pNetH, * pNetL, * pTermH;
    int i, k;

    assert( Abc_NtkIsNetlist(pNtkH) );
    assert( Abc_NtkWhiteboxNum(pNtkH) == 0 );
    assert( Abc_NtkBlackboxNum(pNtkH) > 0 );

    assert( Abc_NtkIsNetlist(pNtkL) );
    assert( Abc_NtkWhiteboxNum(pNtkL) == 0 );
    assert( Abc_NtkBlackboxNum(pNtkL) == 0 );

    // prepare the logic network for copying
    Abc_NtkCleanCopy( pNtkL );

    // start the network
    pNtkNew = Abc_NtkAlloc( pNtkL->ntkType, pNtkL->ntkFunc, 1 );
    // duplicate the name and the spec
    pNtkNew->pName = Extra_UtilStrsav( pNtkH->pName );
    pNtkNew->pSpec = Extra_UtilStrsav( pNtkH->pSpec );

    // make sure every PI/PO has a PI/PO in the processed network
    Abc_NtkForEachPi( pNtkH, pObjH, i )
    {
        pNetH = Abc_ObjFanout0(pObjH);
        pNetL = Abc_NtkFindNet( pNtkL, Abc_ObjName(pNetH) );
        if ( pNetL == NULL || !Abc_ObjIsPi( Abc_ObjFanin0(pNetL) ) )
        {
            printf( "Error in Abc_NtkInsertNewLogic(): There is no PI corresponding to the PI %s.\n", Abc_ObjName(pNetH) );
            Abc_NtkDelete( pNtkNew );
            return NULL;
        }
        if ( pNetL->pCopy )
        {
            printf( "Error in Abc_NtkInsertNewLogic(): Primary input %s is repeated twice.\n", Abc_ObjName(pNetH) );
            Abc_NtkDelete( pNtkNew );
            return NULL;
        }
        // create the new net
        pNetL->pCopy = Abc_NtkFindOrCreateNet( pNtkNew, Abc_ObjName(pNetH) );
        Abc_NtkDupObj( pNtkNew, Abc_ObjFanin0(pNetL), 0 );
    }

    // make sure every BB has a PI/PO in the processed network
    Abc_NtkForEachBlackbox( pNtkH, pObjH, i )
    {
        // duplicate the box 
        Abc_NtkDupBox( pNtkNew, pObjH, 0 );
        pObjH->pCopy->pData = pObjH->pData;
        // create PIs
        Abc_ObjForEachFanout( pObjH, pTermH, k )
        {
            pNetH = Abc_ObjFanout0( pTermH );
            pNetL = Abc_NtkFindNet( pNtkL, Abc_ObjName(pNetH) );
            if ( pNetL == NULL || !Abc_ObjIsPi( Abc_ObjFanin0(pNetL) ) )
            {
                printf( "Error in Abc_NtkInsertNewLogic(): There is no PI corresponding to the inpout %s of blackbox %s.\n", Abc_ObjName(pNetH), Abc_ObjName(pObjH) );
                Abc_NtkDelete( pNtkNew );
                return NULL;
            }
            if ( pNetL->pCopy )
            {
                printf( "Error in Abc_NtkInsertNewLogic(): Box output %s is repeated twice.\n", Abc_ObjName(pNetH) );
                Abc_NtkDelete( pNtkNew );
                return NULL;
            }
            // create net and map the PI
            pNetL->pCopy = Abc_NtkFindOrCreateNet( pNtkNew, Abc_ObjName(pNetH) );
            Abc_ObjFanin0(pNetL)->pCopy = pTermH->pCopy;
        }
    }

    Abc_NtkForEachPo( pNtkH, pObjH, i )
    {
        pNetH = Abc_ObjFanin0(pObjH);
        pNetL = Abc_NtkFindNet( pNtkL, Abc_ObjName(pNetH) );
        if ( pNetL == NULL || !Abc_ObjIsPo( Abc_ObjFanout0(pNetL) ) )
        {
            printf( "Error in Abc_NtkInsertNewLogic(): There is no PO corresponding to the PO %s.\n", Abc_ObjName(pNetH) );
            Abc_NtkDelete( pNtkNew );
            return NULL;
        }
        if ( pNetL->pCopy )
            continue;
        // create the new net
        pNetL->pCopy = Abc_NtkFindOrCreateNet( pNtkNew, Abc_ObjName(pNetH) );
        Abc_NtkDupObj( pNtkNew, Abc_ObjFanout0(pNetL), 0 );
    }
    Abc_NtkForEachBlackbox( pNtkH, pObjH, i )
    {
        Abc_ObjForEachFanin( pObjH, pTermH, k )
        {
            char * pName;
            pNetH = Abc_ObjFanin0( pTermH );
            pName = Abc_ObjName(pNetH);
            pNetL = Abc_NtkFindNet( pNtkL, Abc_ObjName(pNetH) );
            if ( pNetL == NULL || !Abc_ObjIsPo( Abc_ObjFanout0(pNetL) ) )
            {
                printf( "There is no PO corresponding to the input %s of blackbox %s.\n", Abc_ObjName(pNetH), Abc_ObjName(pObjH) );
                Abc_NtkDelete( pNtkNew );
                return NULL;
            }
            // create net and map the PO
            if ( pNetL->pCopy )
            {
                if ( Abc_ObjFanout0(pNetL)->pCopy == NULL )
                    Abc_ObjFanout0(pNetL)->pCopy = pTermH->pCopy;
                else
                    Abc_ObjAddFanin( pTermH->pCopy, pNetL->pCopy );
                continue;
            }
            pNetL->pCopy = Abc_NtkFindOrCreateNet( pNtkNew, Abc_ObjName(pNetH) );
            Abc_ObjFanout0(pNetL)->pCopy = pTermH->pCopy;
        }
    }

    // duplicate other objects of the logic network
    Abc_NtkForEachObj( pNtkL, pObjL, i )
        if ( pObjL->pCopy == NULL && !Abc_ObjIsPo(pObjL) ) // skip POs feeding into PIs
            Abc_NtkDupObj( pNtkNew, pObjL, Abc_ObjIsNet(pObjL) );

    // connect objects
    Abc_NtkForEachObj( pNtkL, pObjL, i )
        Abc_ObjForEachFanin( pObjL, pNetL, k )
            if ( pObjL->pCopy )
                Abc_ObjAddFanin( pObjL->pCopy, pNetL->pCopy );

    // transfer the design
    pDesign = pNtkH->pDesign;  pNtkH->pDesign = NULL;
    assert( Vec_PtrEntry( pDesign->vModules, 0 ) == pNtkH );
    Vec_PtrWriteEntry( pDesign->vModules, 0, pNtkNew );
    pNtkNew->pDesign = pDesign;

    // check integrity
    if ( !Abc_NtkCheck( pNtkNew ) )
    {
        fprintf( stdout, "Abc_NtkInsertNewLogic(): Network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

