/**CFile****************************************************************

  FileName    [abcNtk.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Network creation/duplication/deletion procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcNtk.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

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

  Synopsis    [Creates a new Ntk.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkAlloc( Abc_NtkType_t Type, Abc_NtkFunc_t Func, int fUseMemMan )
{
    Abc_Ntk_t * pNtk;
    pNtk = ALLOC( Abc_Ntk_t, 1 );
    memset( pNtk, 0, sizeof(Abc_Ntk_t) );
    pNtk->ntkType     = Type;
    pNtk->ntkFunc     = Func;
    // start the object storage
    pNtk->vObjs       = Vec_PtrAlloc( 100 );
    pNtk->vAsserts    = Vec_PtrAlloc( 100 );
    pNtk->vPios       = Vec_PtrAlloc( 100 );
    pNtk->vPis        = Vec_PtrAlloc( 100 );
    pNtk->vPos        = Vec_PtrAlloc( 100 );
    pNtk->vCis        = Vec_PtrAlloc( 100 );
    pNtk->vCos        = Vec_PtrAlloc( 100 );
    pNtk->vBoxes      = Vec_PtrAlloc( 100 );
    // start the memory managers
    pNtk->pMmObj      = fUseMemMan? Extra_MmFixedStart( sizeof(Abc_Obj_t) ) : NULL;
    pNtk->pMmStep     = fUseMemMan? Extra_MmStepStart( ABC_NUM_STEPS ) : NULL;
    // get ready to assign the first Obj ID
    pNtk->nTravIds    = 1;
    // start the functionality manager
    if ( Abc_NtkIsStrash(pNtk) )
        pNtk->pManFunc = Abc_AigAlloc( pNtk );
    else if ( Abc_NtkHasSop(pNtk) || Abc_NtkHasBlifMv(pNtk) )
        pNtk->pManFunc = Extra_MmFlexStart();
    else if ( Abc_NtkHasBdd(pNtk) )
        pNtk->pManFunc = Cudd_Init( 20, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    else if ( Abc_NtkHasAig(pNtk) )
        pNtk->pManFunc = Hop_ManStart();
    else if ( Abc_NtkHasMapping(pNtk) )
        pNtk->pManFunc = Abc_FrameReadLibGen();
    else if ( !Abc_NtkHasBlackbox(pNtk) )
        assert( 0 );
    // name manager
    pNtk->pManName = Nm_ManCreate( 200 );
    // attribute manager
    pNtk->vAttrs = Vec_PtrStart( VEC_ATTR_TOTAL_NUM );
    return pNtk;
}

/**Function*************************************************************

  Synopsis    [Starts a new network using existing network as a model.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkStartFrom( Abc_Ntk_t * pNtk, Abc_NtkType_t Type, Abc_NtkFunc_t Func )
{
    Abc_Ntk_t * pNtkNew; 
    Abc_Obj_t * pObj;
    int fCopyNames, i;
    if ( pNtk == NULL )
        return NULL;
    // decide whether to copy the names
    fCopyNames = ( Type != ABC_NTK_NETLIST );
    // start the network
    pNtkNew = Abc_NtkAlloc( Type, Func, 1 );
    // duplicate the name and the spec
    pNtkNew->pName = Extra_UtilStrsav(pNtk->pName);
    pNtkNew->pSpec = Extra_UtilStrsav(pNtk->pSpec);
    // clean the node copy fields
    Abc_NtkCleanCopy( pNtk );
    // map the constant nodes
    if ( Abc_NtkIsStrash(pNtk) && Abc_NtkIsStrash(pNtkNew) )
        Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);
    // clone CIs/CIs/boxes
    Abc_NtkForEachPi( pNtk, pObj, i )
        Abc_NtkDupObj( pNtkNew, pObj, fCopyNames );
    Abc_NtkForEachPo( pNtk, pObj, i )
        Abc_NtkDupObj( pNtkNew, pObj, fCopyNames );
    Abc_NtkForEachAssert( pNtk, pObj, i )
        Abc_NtkDupObj( pNtkNew, pObj, fCopyNames );
    Abc_NtkForEachBox( pNtk, pObj, i )
        Abc_NtkDupBox( pNtkNew, pObj, fCopyNames );
    // transfer the names
//    Abc_NtkTrasferNames( pNtk, pNtkNew );
    Abc_ManTimeDup( pNtk, pNtkNew );
    // check that the CI/CO/latches are copied correctly
    assert( Abc_NtkCiNum(pNtk)    == Abc_NtkCiNum(pNtkNew) );
    assert( Abc_NtkCoNum(pNtk)    == Abc_NtkCoNum(pNtkNew) );
    assert( Abc_NtkLatchNum(pNtk) == Abc_NtkLatchNum(pNtkNew) );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Starts a new network using existing network as a model.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkStartFromNoLatches( Abc_Ntk_t * pNtk, Abc_NtkType_t Type, Abc_NtkFunc_t Func )
{
    Abc_Ntk_t * pNtkNew; 
    Abc_Obj_t * pObj;
    int i;
    if ( pNtk == NULL )
        return NULL;
    assert( Type != ABC_NTK_NETLIST );
    // start the network
    pNtkNew = Abc_NtkAlloc( Type, Func, 1 );
    // duplicate the name and the spec
    pNtkNew->pName = Extra_UtilStrsav(pNtk->pName);
    pNtkNew->pSpec = Extra_UtilStrsav(pNtk->pSpec);
    // clean the node copy fields
    Abc_NtkCleanCopy( pNtk );
    // map the constant nodes
    if ( Abc_NtkIsStrash(pNtk) && Abc_NtkIsStrash(pNtkNew) )
        Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);
    // clone CIs/CIs/boxes
    Abc_NtkForEachPi( pNtk, pObj, i )
        Abc_NtkDupObj( pNtkNew, pObj, 1 );
    Abc_NtkForEachPo( pNtk, pObj, i )
        Abc_NtkDupObj( pNtkNew, pObj, 1 );
    Abc_NtkForEachAssert( pNtk, pObj, i )
        Abc_NtkDupObj( pNtkNew, pObj, 1 );
    Abc_NtkForEachBox( pNtk, pObj, i )
    {
        if ( Abc_ObjIsLatch(pObj) )
            continue;
        Abc_NtkDupBox(pNtkNew, pObj, 1);
    }
    // transfer the names
//    Abc_NtkTrasferNamesNoLatches( pNtk, pNtkNew );
    Abc_ManTimeDup( pNtk, pNtkNew );
    // check that the CI/CO/latches are copied correctly
    assert( Abc_NtkPiNum(pNtk) == Abc_NtkPiNum(pNtkNew) );
    assert( Abc_NtkPoNum(pNtk) == Abc_NtkPoNum(pNtkNew) );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Finalizes the network using the existing network as a model.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkFinalize( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkNew )
{
    Abc_Obj_t * pObj, * pDriver, * pDriverNew;
    int i;
    // set the COs of the strashed network
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        pDriver    = Abc_ObjFanin0Ntk( Abc_ObjFanin0(pObj) );
        pDriverNew = Abc_ObjNotCond(pDriver->pCopy, Abc_ObjFaninC0(pObj));
        Abc_ObjAddFanin( pObj->pCopy, pDriverNew );
    }
}

/**Function*************************************************************

  Synopsis    [Starts a new network using existing network as a model.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkStartRead( char * pName )
{
    Abc_Ntk_t * pNtkNew; 
    // allocate the empty network
    pNtkNew = Abc_NtkAlloc( ABC_NTK_NETLIST, ABC_FUNC_SOP, 1 );
    // set the specs
    pNtkNew->pName = Extra_FileNameGeneric(pName);
    pNtkNew->pSpec = Extra_UtilStrsav(pName);
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Finalizes the network using the existing network as a model.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkFinalizeRead( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pBox, * pObj, * pTerm, * pNet;
    int i;
    if ( Abc_NtkHasBlackbox(pNtk) && Abc_NtkBoxNum(pNtk) == 0 )
    {
        pBox = Abc_NtkCreateBlackbox(pNtk);
        Abc_NtkForEachPi( pNtk, pObj, i )
        {
            pTerm = Abc_NtkCreateBi(pNtk);
            Abc_ObjAddFanin( pTerm, Abc_ObjFanout0(pObj) );
            Abc_ObjAddFanin( pBox, pTerm );
        }
        Abc_NtkForEachPo( pNtk, pObj, i )
        {
            pTerm = Abc_NtkCreateBo(pNtk);
            Abc_ObjAddFanin( pTerm, pBox );
            Abc_ObjAddFanin( Abc_ObjFanin0(pObj), pTerm );
        }
        return;
    }
    assert( Abc_NtkIsNetlist(pNtk) );

    // check if constant 0 net is used
    pNet = Abc_NtkFindNet( pNtk, "1\'b0" );
    if ( pNet )
    {
        if ( Abc_ObjFanoutNum(pNet) == 0 )
            Abc_NtkDeleteObj(pNet);
        else if ( Abc_ObjFaninNum(pNet) == 0 )
            Abc_ObjAddFanin( pNet, Abc_NtkCreateNodeConst0(pNtk) );
    }
    // check if constant 1 net is used
    pNet = Abc_NtkFindNet( pNtk, "1\'b1" );
    if ( pNet )
    {
        if ( Abc_ObjFanoutNum(pNet) == 0 )
            Abc_NtkDeleteObj(pNet);
        else if ( Abc_ObjFaninNum(pNet) == 0 )
            Abc_ObjAddFanin( pNet, Abc_NtkCreateNodeConst1(pNtk) );
    }
    // fix the net drivers
    Abc_NtkFixNonDrivenNets( pNtk );

    // reorder the CI/COs to PI/POs first
    Abc_NtkOrderCisCos( pNtk );
}

/**Function*************************************************************

  Synopsis    [Duplicate the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDup( Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pNtkNew; 
    Abc_Obj_t * pObj, * pFanin;
    int i, k;
    if ( pNtk == NULL )
        return NULL;
    // start the network
    pNtkNew = Abc_NtkStartFrom( pNtk, pNtk->ntkType, pNtk->ntkFunc );
    // copy the internal nodes
    if ( Abc_NtkIsStrash(pNtk) )
    {
        // copy the AND gates
        Abc_AigForEachAnd( pNtk, pObj, i )
            pObj->pCopy = Abc_AigAnd( pNtkNew->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj) );
        // relink the choice nodes
        Abc_AigForEachAnd( pNtk, pObj, i )
            if ( pObj->pData )
                pObj->pCopy->pData = ((Abc_Obj_t *)pObj->pData)->pCopy;
        // relink the CO nodes
        Abc_NtkForEachCo( pNtk, pObj, i )
            Abc_ObjAddFanin( pObj->pCopy, Abc_ObjChild0Copy(pObj) );
        // get the number of nodes before and after
        if ( Abc_NtkNodeNum(pNtk) != Abc_NtkNodeNum(pNtkNew) )
            printf( "Warning: Structural hashing during duplication reduced %d nodes (this is a minor bug).\n",
                Abc_NtkNodeNum(pNtk) - Abc_NtkNodeNum(pNtkNew) );
    }
    else
    {
        // duplicate the nets and nodes (CIs/COs/latches already dupped)
        Abc_NtkForEachObj( pNtk, pObj, i )
            if ( pObj->pCopy == NULL )
                Abc_NtkDupObj(pNtkNew, pObj, 0);
        // reconnect all objects (no need to transfer attributes on edges)
        Abc_NtkForEachObj( pNtk, pObj, i )
            if ( !Abc_ObjIsBox(pObj) && !Abc_ObjIsBo(pObj) )
                Abc_ObjForEachFanin( pObj, pFanin, k )
                    Abc_ObjAddFanin( pObj->pCopy, pFanin->pCopy );
    }
    // duplicate the EXDC Ntk
    if ( pNtk->pExdc )
        pNtkNew->pExdc = Abc_NtkDup( pNtk->pExdc );
    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkDup(): Network check has failed.\n" );
    pNtk->pCopy = pNtkNew;
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Duplicate the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDouble( Abc_Ntk_t * pNtk )
{
    char Buffer[500];
    Abc_Ntk_t * pNtkNew; 
    Abc_Obj_t * pObj, * pFanin;
    int i, k;
    assert( Abc_NtkIsLogic(pNtk) );

    // start the network
    pNtkNew = Abc_NtkAlloc( pNtk->ntkType, pNtk->ntkFunc, 1 );
    sprintf( Buffer, "%s%s", pNtk->pName, "_2x" );
    pNtkNew->pName = Extra_UtilStrsav(Buffer);

    // clean the node copy fields
    Abc_NtkCleanCopy( pNtk );
    // clone CIs/CIs/boxes
    Abc_NtkForEachPi( pNtk, pObj, i )
        Abc_NtkDupObj( pNtkNew, pObj, 0 );
    Abc_NtkForEachPo( pNtk, pObj, i )
        Abc_NtkDupObj( pNtkNew, pObj, 0 );
    Abc_NtkForEachAssert( pNtk, pObj, i )
        Abc_NtkDupObj( pNtkNew, pObj, 0 );
    Abc_NtkForEachBox( pNtk, pObj, i )
        Abc_NtkDupBox( pNtkNew, pObj, 0 );
    // copy the internal nodes
    // duplicate the nets and nodes (CIs/COs/latches already dupped)
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( pObj->pCopy == NULL )
            Abc_NtkDupObj(pNtkNew, pObj, 0);
    // reconnect all objects (no need to transfer attributes on edges)
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( !Abc_ObjIsBox(pObj) && !Abc_ObjIsBo(pObj) )
            Abc_ObjForEachFanin( pObj, pFanin, k )
                Abc_ObjAddFanin( pObj->pCopy, pFanin->pCopy );

    // clean the node copy fields
    Abc_NtkCleanCopy( pNtk );
    // clone CIs/CIs/boxes
    Abc_NtkForEachPi( pNtk, pObj, i )
        Abc_NtkDupObj( pNtkNew, pObj, 0 );
    Abc_NtkForEachPo( pNtk, pObj, i )
        Abc_NtkDupObj( pNtkNew, pObj, 0 );
    Abc_NtkForEachAssert( pNtk, pObj, i )
        Abc_NtkDupObj( pNtkNew, pObj, 0 );
    Abc_NtkForEachBox( pNtk, pObj, i )
        Abc_NtkDupBox( pNtkNew, pObj, 0 );
    // copy the internal nodes
    // duplicate the nets and nodes (CIs/COs/latches already dupped)
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( pObj->pCopy == NULL )
            Abc_NtkDupObj(pNtkNew, pObj, 0);
    // reconnect all objects (no need to transfer attributes on edges)
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( !Abc_ObjIsBox(pObj) && !Abc_ObjIsBo(pObj) )
            Abc_ObjForEachFanin( pObj, pFanin, k )
                Abc_ObjAddFanin( pObj->pCopy, pFanin->pCopy );

    // assign names
    Abc_NtkForEachCi( pNtk, pObj, i )
    {
        Abc_ObjAssignName( Abc_NtkCi(pNtkNew,                      i), "1_", Abc_ObjName(pObj) );
        Abc_ObjAssignName( Abc_NtkCi(pNtkNew, Abc_NtkCiNum(pNtk) + i), "2_", Abc_ObjName(pObj) );
    }
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        Abc_ObjAssignName( Abc_NtkCo(pNtkNew,                      i), "1_", Abc_ObjName(pObj) );
        Abc_ObjAssignName( Abc_NtkCo(pNtkNew, Abc_NtkCoNum(pNtk) + i), "2_", Abc_ObjName(pObj) );
    }

    // perform the final check
    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkDup(): Network check has failed.\n" );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Attaches the second network at the bottom of the first.]

  Description [Returns the first network. Deletes the second network.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkAttachBottom( Abc_Ntk_t * pNtkTop, Abc_Ntk_t * pNtkBottom )
{
    Abc_Obj_t * pObj, * pFanin, * pBuffer;
    Vec_Ptr_t * vNodes;
    int i, k;
    assert( pNtkBottom != NULL );
    if ( pNtkTop == NULL )
        return pNtkBottom;
    // make sure the networks are combinational
    assert( Abc_NtkPiNum(pNtkTop) == Abc_NtkCiNum(pNtkTop) );
    assert( Abc_NtkPiNum(pNtkBottom) == Abc_NtkCiNum(pNtkBottom) );
    // make sure the POs of the bottom correspond to the PIs of the top
    assert( Abc_NtkPoNum(pNtkBottom) == Abc_NtkPiNum(pNtkTop) );
    assert( Abc_NtkPiNum(pNtkBottom) <  Abc_NtkPiNum(pNtkTop) );
    // add buffers for the PIs of the top - save results in the POs of the bottom
    Abc_NtkForEachPi( pNtkTop, pObj, i )
    {
        pBuffer = Abc_NtkCreateNodeBuf( pNtkTop, NULL );
        Abc_ObjTransferFanout( pObj, pBuffer );
        Abc_NtkPo(pNtkBottom, i)->pCopy = pBuffer;
    }
    // remove useless PIs of the top
    for ( i = Abc_NtkPiNum(pNtkTop) - 1; i >= Abc_NtkPiNum(pNtkBottom); i-- )
        Abc_NtkDeleteObj( Abc_NtkPi(pNtkTop, i) );
    assert( Abc_NtkPiNum(pNtkBottom) == Abc_NtkPiNum(pNtkTop) );
    // copy the bottom network
    Abc_NtkForEachPi( pNtkBottom, pObj, i )
        Abc_NtkPi(pNtkBottom, i)->pCopy = Abc_NtkPi(pNtkTop, i);
    // construct all nodes
    vNodes = Abc_NtkDfs( pNtkBottom, 0 );
    Vec_PtrForEachEntry( vNodes, pObj, i )
    {
        Abc_NtkDupObj(pNtkTop, pObj, 0);
        Abc_ObjForEachFanin( pObj, pFanin, k )
            Abc_ObjAddFanin( pObj->pCopy, pFanin->pCopy );
    }
    Vec_PtrFree( vNodes );
    // connect the POs
    Abc_NtkForEachPo( pNtkBottom, pObj, i )
        Abc_ObjAddFanin( pObj->pCopy, Abc_ObjFanin0(pObj)->pCopy );
    // delete old network
    Abc_NtkDelete( pNtkBottom );
    // return the network
    if ( !Abc_NtkCheck( pNtkTop ) )
        fprintf( stdout, "Abc_NtkAttachBottom(): Network check has failed.\n" );
    return pNtkTop;
}

/**Function*************************************************************

  Synopsis    [Creates the network composed of one logic cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkCreateCone( Abc_Ntk_t * pNtk, Abc_Obj_t * pNode, char * pNodeName, int fUseAllCis )
{
    Abc_Ntk_t * pNtkNew; 
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj, * pFanin, * pNodeCoNew;
    char Buffer[1000];
    int i, k;

    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsStrash(pNtk) );
    assert( Abc_ObjIsNode(pNode) ); 
    
    // start the network
    pNtkNew = Abc_NtkAlloc( pNtk->ntkType, pNtk->ntkFunc, 1 );
    // set the name
    sprintf( Buffer, "%s_%s", pNtk->pName, pNodeName );
    pNtkNew->pName = Extra_UtilStrsav(Buffer);

    // establish connection between the constant nodes
    if ( Abc_NtkIsStrash(pNtk) )
        Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);

    // collect the nodes in the TFI of the output (mark the TFI)
    vNodes = Abc_NtkDfsNodes( pNtk, &pNode, 1 );
    // create the PIs
    Abc_NtkForEachCi( pNtk, pObj, i )
    {
        if ( fUseAllCis || Abc_NodeIsTravIdCurrent(pObj) ) // TravId is set by DFS
        {
            pObj->pCopy = Abc_NtkCreatePi(pNtkNew);
            Abc_ObjAssignName( pObj->pCopy, Abc_ObjName(pObj), NULL );
        }
    }
    // add the PO corresponding to this output
    pNodeCoNew = Abc_NtkCreatePo( pNtkNew );
    Abc_ObjAssignName( pNodeCoNew, pNodeName, NULL );
    // copy the nodes
    Vec_PtrForEachEntry( vNodes, pObj, i )
    {
        // if it is an AIG, add to the hash table
        if ( Abc_NtkIsStrash(pNtk) )
        {
            pObj->pCopy = Abc_AigAnd( pNtkNew->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj) );
        }
        else
        {
            Abc_NtkDupObj( pNtkNew, pObj, 0 );
            Abc_ObjForEachFanin( pObj, pFanin, k )
                Abc_ObjAddFanin( pObj->pCopy, pFanin->pCopy );
        }
    }
    // connect the internal nodes to the new CO
    Abc_ObjAddFanin( pNodeCoNew, pNode->pCopy );
    Vec_PtrFree( vNodes );

    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkCreateCone(): Network check has failed.\n" );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Creates the network composed of several logic cones.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkCreateConeArray( Abc_Ntk_t * pNtk, Vec_Ptr_t * vRoots, int fUseAllCis )
{
    Abc_Ntk_t * pNtkNew; 
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj, * pFanin, * pNodeCoNew;
    char Buffer[1000];
    int i, k;

    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsStrash(pNtk) );
    
    // start the network
    pNtkNew = Abc_NtkAlloc( pNtk->ntkType, pNtk->ntkFunc, 1 );
    // set the name
    sprintf( Buffer, "%s_part", pNtk->pName );
    pNtkNew->pName = Extra_UtilStrsav(Buffer);

    // establish connection between the constant nodes
    if ( Abc_NtkIsStrash(pNtk) )
        Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);

    // collect the nodes in the TFI of the output (mark the TFI)
    vNodes = Abc_NtkDfsNodes( pNtk, (Abc_Obj_t **)Vec_PtrArray(vRoots), Vec_PtrSize(vRoots) );

    // create the PIs
    Abc_NtkForEachCi( pNtk, pObj, i )
    {
        if ( fUseAllCis || Abc_NodeIsTravIdCurrent(pObj) ) // TravId is set by DFS
        {
            pObj->pCopy = Abc_NtkCreatePi(pNtkNew);
            Abc_ObjAssignName( pObj->pCopy, Abc_ObjName(pObj), NULL );
        }
    }

    // copy the nodes
    Vec_PtrForEachEntry( vNodes, pObj, i )
    {
        // if it is an AIG, add to the hash table
        if ( Abc_NtkIsStrash(pNtk) )
        {
            pObj->pCopy = Abc_AigAnd( pNtkNew->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj) );
        }
        else
        {
            Abc_NtkDupObj( pNtkNew, pObj, 0 );
            Abc_ObjForEachFanin( pObj, pFanin, k )
                Abc_ObjAddFanin( pObj->pCopy, pFanin->pCopy );
        }
    }
    Vec_PtrFree( vNodes );

    // add the PO corresponding to the nodes
    Vec_PtrForEachEntry( vRoots, pObj, i )
    {
        // create the PO node
        pNodeCoNew = Abc_NtkCreatePo( pNtkNew );
        // connect the internal nodes to the new CO
        if ( Abc_ObjIsCo(pObj) )
            Abc_ObjAddFanin( pNodeCoNew, Abc_ObjChild0Copy(pObj) );
        else
            Abc_ObjAddFanin( pNodeCoNew, pObj->pCopy );
        // assign the name
        Abc_ObjAssignName( pNodeCoNew, Abc_ObjName(pObj), NULL );
    }

    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkCreateConeArray(): Network check has failed.\n" );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Creates the network composed of MFFC of one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkCreateMffc( Abc_Ntk_t * pNtk, Abc_Obj_t * pNode, char * pNodeName )
{
    Abc_Ntk_t * pNtkNew; 
    Abc_Obj_t * pObj, * pFanin, * pNodeCoNew;
    Vec_Ptr_t * vCone, * vSupp;
    char Buffer[1000];
    int i, k;

    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsStrash(pNtk) );
    assert( Abc_ObjIsNode(pNode) ); 
    
    // start the network
    pNtkNew = Abc_NtkAlloc( pNtk->ntkType, pNtk->ntkFunc, 1 );
    // set the name
    sprintf( Buffer, "%s_%s", pNtk->pName, pNodeName );
    pNtkNew->pName = Extra_UtilStrsav(Buffer);

    // establish connection between the constant nodes
    if ( Abc_NtkIsStrash(pNtk) )
        Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);

    // collect the nodes in MFFC
    vCone = Vec_PtrAlloc( 100 );
    vSupp = Vec_PtrAlloc( 100 );
    Abc_NodeDeref_rec( pNode );
    Abc_NodeMffsConeSupp( pNode, vCone, vSupp );
    Abc_NodeRef_rec( pNode );
    // create the PIs
    Vec_PtrForEachEntry( vSupp, pObj, i )
    {
        pObj->pCopy = Abc_NtkCreatePi(pNtkNew);
        Abc_ObjAssignName( pObj->pCopy, Abc_ObjName(pObj), NULL );
    }
    // create the PO
    pNodeCoNew = Abc_NtkCreatePo( pNtkNew );
    Abc_ObjAssignName( pNodeCoNew, pNodeName, NULL );
    // copy the nodes
    Vec_PtrForEachEntry( vCone, pObj, i )
    {
        // if it is an AIG, add to the hash table
        if ( Abc_NtkIsStrash(pNtk) )
        {
            pObj->pCopy = Abc_AigAnd( pNtkNew->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj) );
        }
        else
        {
            Abc_NtkDupObj( pNtkNew, pObj, 0 );
            Abc_ObjForEachFanin( pObj, pFanin, k )
                Abc_ObjAddFanin( pObj->pCopy, pFanin->pCopy );
        }
    }
    // connect the topmost node
    Abc_ObjAddFanin( pNodeCoNew, pNode->pCopy );
    Vec_PtrFree( vCone );
    Vec_PtrFree( vSupp );

    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkCreateMffc(): Network check has failed.\n" );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Creates the miter composed of one multi-output cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkCreateTarget( Abc_Ntk_t * pNtk, Vec_Ptr_t * vRoots, Vec_Int_t * vValues )
{
    Vec_Ptr_t * vNodes;
    Abc_Ntk_t * pNtkNew; 
    Abc_Obj_t * pObj, * pFinal, * pOther, * pNodePo;
    int i;

    assert( Abc_NtkIsLogic(pNtk) );
    
    // start the network
    Abc_NtkCleanCopy( pNtk );
    pNtkNew = Abc_NtkAlloc( ABC_NTK_STRASH, ABC_FUNC_AIG, 1 );
    pNtkNew->pName = Extra_UtilStrsav(pNtk->pName);

    // collect the nodes in the TFI of the output
    vNodes = Abc_NtkDfsNodes( pNtk, (Abc_Obj_t **)vRoots->pArray, vRoots->nSize );
    // create the PIs
    Abc_NtkForEachCi( pNtk, pObj, i )
    {
        pObj->pCopy = Abc_NtkCreatePi(pNtkNew);
        Abc_ObjAssignName( pObj->pCopy, Abc_ObjName(pObj), NULL );
    }
    // copy the nodes
    Vec_PtrForEachEntry( vNodes, pObj, i )
        pObj->pCopy = Abc_NodeStrash( pNtkNew, pObj, 0 );
    Vec_PtrFree( vNodes );

    // add the PO
    pFinal = Abc_AigConst1( pNtkNew );
    Vec_PtrForEachEntry( vRoots, pObj, i )
    {
        if ( Abc_ObjIsCo(pObj) )
            pOther = Abc_ObjFanin0(pObj)->pCopy;
        else
            pOther = pObj->pCopy;
        if ( Vec_IntEntry(vValues, i) == 0 )
            pOther = Abc_ObjNot(pOther);
        pFinal = Abc_AigAnd( pNtkNew->pManFunc, pFinal, pOther );
    }

    // add the PO corresponding to this output
    pNodePo = Abc_NtkCreatePo( pNtkNew );
    Abc_ObjAddFanin( pNodePo, pFinal );
    Abc_ObjAssignName( pNodePo, "miter", NULL );
    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkCreateTarget(): Network check has failed.\n" );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Creates the network composed of one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkCreateFromNode( Abc_Ntk_t * pNtk, Abc_Obj_t * pNode )
{    
    Abc_Ntk_t * pNtkNew; 
    Abc_Obj_t * pFanin, * pNodePo;
    int i;
    // start the network
    pNtkNew = Abc_NtkAlloc( pNtk->ntkType, pNtk->ntkFunc, 1 );
    pNtkNew->pName = Extra_UtilStrsav(Abc_ObjName(pNode));
    // add the PIs corresponding to the fanins of the node
    Abc_ObjForEachFanin( pNode, pFanin, i )
    {
        pFanin->pCopy = Abc_NtkCreatePi( pNtkNew );
        Abc_ObjAssignName( pFanin->pCopy, Abc_ObjName(pFanin), NULL );
    }
    // duplicate and connect the node
    pNode->pCopy = Abc_NtkDupObj( pNtkNew, pNode, 0 );
    Abc_ObjForEachFanin( pNode, pFanin, i )
        Abc_ObjAddFanin( pNode->pCopy, pFanin->pCopy );
    // create the only PO
    pNodePo = Abc_NtkCreatePo( pNtkNew );
    Abc_ObjAddFanin( pNodePo, pNode->pCopy );
    Abc_ObjAssignName( pNodePo, Abc_ObjName(pNode), NULL );
    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkCreateFromNode(): Network check has failed.\n" );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Creates the network composed of one node with the given SOP.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkCreateWithNode( char * pSop )
{    
    Abc_Ntk_t * pNtkNew; 
    Abc_Obj_t * pFanin, * pNode, * pNodePo;
    Vec_Ptr_t * vNames;
    int i, nVars;
    // start the network
    pNtkNew = Abc_NtkAlloc( ABC_NTK_LOGIC, ABC_FUNC_SOP, 1 );
    pNtkNew->pName = Extra_UtilStrsav("ex");
    // create PIs
    Vec_PtrPush( pNtkNew->vObjs, NULL );
    nVars = Abc_SopGetVarNum( pSop );
    vNames = Abc_NodeGetFakeNames( nVars );
    for ( i = 0; i < nVars; i++ )
        Abc_ObjAssignName( Abc_NtkCreatePi(pNtkNew), Vec_PtrEntry(vNames, i), NULL );
    Abc_NodeFreeNames( vNames );
    // create the node, add PIs as fanins, set the function
    pNode = Abc_NtkCreateNode( pNtkNew );
    Abc_NtkForEachPi( pNtkNew, pFanin, i )
        Abc_ObjAddFanin( pNode, pFanin );
    pNode->pData = Abc_SopRegister( pNtkNew->pManFunc, pSop );
    // create the only PO
    pNodePo = Abc_NtkCreatePo(pNtkNew);
    Abc_ObjAddFanin( pNodePo, pNode );
    Abc_ObjAssignName( pNodePo, "F", NULL );
    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkCreateWithNode(): Network check has failed.\n" );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Deletes the Ntk.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDelete( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    void * pAttrMan;
    int TotalMemory, i;
    int LargePiece = (4 << ABC_NUM_STEPS);
    if ( pNtk == NULL )
        return;
    // free the HAIG
    if ( pNtk->pHaig )
        Abc_NtkHaigStop( pNtk );
    // free EXDC Ntk
    if ( pNtk->pExdc )
        Abc_NtkDelete( pNtk->pExdc );
    // dereference the BDDs
    if ( Abc_NtkHasBdd(pNtk) )
    {
        Abc_NtkForEachNode( pNtk, pObj, i )
            Cudd_RecursiveDeref( pNtk->pManFunc, pObj->pData );
    }
    // make sure all the marks are clean
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        // free large fanout arrays
        if ( pNtk->pMmObj && pObj->vFanouts.nCap * 4 > LargePiece )
            FREE( pObj->vFanouts.pArray );
        // these flags should be always zero
        // if this is not true, something is wrong somewhere
        assert( pObj->fMarkA == 0 );
        assert( pObj->fMarkB == 0 );
        assert( pObj->fMarkC == 0 );
    }
    // free the nodes
    if ( pNtk->pMmStep == NULL )
    {
        Abc_NtkForEachObj( pNtk, pObj, i )
        {
            FREE( pObj->vFanouts.pArray );
            FREE( pObj->vFanins.pArray );
        }
    }
    if ( pNtk->pMmObj == NULL )
    {
        Abc_NtkForEachObj( pNtk, pObj, i )
            free( pObj );
    }
        
    // free the arrays
    Vec_PtrFree( pNtk->vPios );
    Vec_PtrFree( pNtk->vPis );
    Vec_PtrFree( pNtk->vPos );
    Vec_PtrFree( pNtk->vCis );
    Vec_PtrFree( pNtk->vCos );
    Vec_PtrFree( pNtk->vAsserts );
    Vec_PtrFree( pNtk->vObjs );
    Vec_PtrFree( pNtk->vBoxes );
    if ( pNtk->vLevelsR ) Vec_IntFree( pNtk->vLevelsR );
    if ( pNtk->pModel ) free( pNtk->pModel );
    TotalMemory  = 0;
    TotalMemory += pNtk->pMmObj? Extra_MmFixedReadMemUsage(pNtk->pMmObj)  : 0;
    TotalMemory += pNtk->pMmStep? Extra_MmStepReadMemUsage(pNtk->pMmStep) : 0;
//    fprintf( stdout, "The total memory allocated internally by the network = %0.2f Mb.\n", ((double)TotalMemory)/(1<<20) );
    // free the storage 
    if ( pNtk->pMmObj )
        Extra_MmFixedStop( pNtk->pMmObj );
    if ( pNtk->pMmStep )
        Extra_MmStepStop ( pNtk->pMmStep );
    // name manager
    Nm_ManFree( pNtk->pManName );
    // free the timing manager
    if ( pNtk->pManTime )
        Abc_ManTimeStop( pNtk->pManTime );
    // start the functionality manager
    if ( Abc_NtkIsStrash(pNtk) )
        Abc_AigFree( pNtk->pManFunc );
    else if ( Abc_NtkHasSop(pNtk) || Abc_NtkHasBlifMv(pNtk) )
        Extra_MmFlexStop( pNtk->pManFunc );
    else if ( Abc_NtkHasBdd(pNtk) )
        Extra_StopManager( pNtk->pManFunc );
    else if ( Abc_NtkHasAig(pNtk) )
        { if ( pNtk->pManFunc ) Hop_ManStop( pNtk->pManFunc ); }
    else if ( Abc_NtkHasMapping(pNtk) )
        pNtk->pManFunc = NULL;
    else if ( !Abc_NtkHasBlackbox(pNtk) )
        assert( 0 );
    // free the hierarchy
    if ( pNtk->pDesign )
    {
        Abc_LibFree( pNtk->pDesign, pNtk );
        pNtk->pDesign = NULL;
    }
//    if ( pNtk->pBlackBoxes ) 
//        Vec_IntFree( pNtk->pBlackBoxes );
    // free node attributes
    Vec_PtrForEachEntry( pNtk->vAttrs, pAttrMan, i )
        if ( pAttrMan )
        {
//printf( "deleting attr\n" );
            Vec_AttFree( pAttrMan, 1 );
        }
    Vec_PtrFree( pNtk->vAttrs );
    FREE( pNtk->pName );
    FREE( pNtk->pSpec );
    free( pNtk );
}

/**Function*************************************************************

  Synopsis    [Reads the verilog file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkFixNonDrivenNets( Abc_Ntk_t * pNtk )
{ 
    Vec_Ptr_t * vNets;
    Abc_Obj_t * pNet, * pNode;
    int i;

    if ( Abc_NtkNodeNum(pNtk) == 0 && Abc_NtkBoxNum(pNtk) == 0 )
        return;

    // check for non-driven nets
    vNets = Vec_PtrAlloc( 100 );
    Abc_NtkForEachNet( pNtk, pNet, i )
    {
        if ( Abc_ObjFaninNum(pNet) > 0 )
            continue;
        // add the constant 0 driver
        pNode = Abc_NtkCreateNodeConst0( pNtk );
        // add the fanout net
        Abc_ObjAddFanin( pNet, pNode );
        // add the net to those for which the warning will be printed
        Vec_PtrPush( vNets, pNet );
    }

    // print the warning
    if ( vNets->nSize > 0 )
    {
        printf( "Warning: Constant-0 drivers added to %d non-driven nets in network \"%s\":\n", Vec_PtrSize(vNets), pNtk->pName );
        Vec_PtrForEachEntry( vNets, pNet, i )
        {
            printf( "%s%s", (i? ", ": ""), Abc_ObjName(pNet) );
            if ( i == 3 )
            {
                if ( Vec_PtrSize(vNets) > 3 )
                    printf( " ..." );
                break;
            }
        }
        printf( "\n" );
    }
    Vec_PtrFree( vNets );
}


/**Function*************************************************************

  Synopsis    [Converts the network to combinational.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkMakeComb( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;

    if ( Abc_NtkIsComb(pNtk) )
        return;

    assert( !Abc_NtkIsNetlist(pNtk) );
    assert( Abc_NtkHasOnlyLatchBoxes(pNtk) );

    // detach the latches
//    Abc_NtkForEachLatch( pNtk, pObj, i )
    Vec_PtrForEachEntryReverse( pNtk->vBoxes, pObj, i )
        Abc_NtkDeleteObj( pObj );
    assert( Abc_NtkLatchNum(pNtk) == 0 );
    assert( Abc_NtkBoxNum(pNtk) == 0 );

    // move CIs to become PIs
    Vec_PtrClear( pNtk->vPis );
    Abc_NtkForEachCi( pNtk, pObj, i )
    {
        if ( Abc_ObjIsBo(pObj) )
        {
            pObj->Type = ABC_OBJ_PI;
            pNtk->nObjCounts[ABC_OBJ_PI]++;
            pNtk->nObjCounts[ABC_OBJ_BO]--;
        }
        Vec_PtrPush( pNtk->vPis, pObj );
    }
    assert( Abc_NtkBoNum(pNtk) == 0 );

    // move COs to become POs
    Vec_PtrClear( pNtk->vPos );
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        if ( Abc_ObjIsBi(pObj) )
        {
            pObj->Type = ABC_OBJ_PO;
            pNtk->nObjCounts[ABC_OBJ_PO]++;
            pNtk->nObjCounts[ABC_OBJ_BI]--;
        }
        Vec_PtrPush( pNtk->vPos, pObj );
    }
    assert( Abc_NtkBiNum(pNtk) == 0 );

    if ( !Abc_NtkCheck( pNtk ) )
        fprintf( stdout, "Abc_NtkMakeComb(): Network check has failed.\n" );
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


