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
#include "base/main/main.h"
#include "map/mio/mio.h"
#include "aig/gia/gia.h"

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

  Synopsis    [Creates a new Ntk.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkAlloc( Abc_NtkType_t Type, Abc_NtkFunc_t Func, int fUseMemMan )
{
    Abc_Ntk_t * pNtk;
    pNtk = ABC_ALLOC( Abc_Ntk_t, 1 );
    memset( pNtk, 0, sizeof(Abc_Ntk_t) );
    pNtk->ntkType     = Type;
    pNtk->ntkFunc     = Func;
    // start the object storage
    pNtk->vObjs       = Vec_PtrAlloc( 100 );
    pNtk->vPios       = Vec_PtrAlloc( 100 );
    pNtk->vPis        = Vec_PtrAlloc( 100 );
    pNtk->vPos        = Vec_PtrAlloc( 100 );
    pNtk->vCis        = Vec_PtrAlloc( 100 );
    pNtk->vCos        = Vec_PtrAlloc( 100 );
    pNtk->vBoxes      = Vec_PtrAlloc( 100 );
    pNtk->vLtlProperties = Vec_PtrAlloc( 100 );
    // start the memory managers
    pNtk->pMmObj      = fUseMemMan? Mem_FixedStart( sizeof(Abc_Obj_t) ) : NULL;
    pNtk->pMmStep     = fUseMemMan? Mem_StepStart( ABC_NUM_STEPS ) : NULL;
    // get ready to assign the first Obj ID
    pNtk->nTravIds    = 1;
    // start the functionality manager
    if ( !Abc_NtkIsStrash(pNtk) )
        Vec_PtrPush( pNtk->vObjs, NULL );
    if ( Abc_NtkIsStrash(pNtk) )
        pNtk->pManFunc = Abc_AigAlloc( pNtk );
    else if ( Abc_NtkHasSop(pNtk) || Abc_NtkHasBlifMv(pNtk) )
        pNtk->pManFunc = Mem_FlexStart();
#ifdef ABC_USE_CUDD
    else if ( Abc_NtkHasBdd(pNtk) )
        pNtk->pManFunc = Cudd_Init( 20, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
#endif
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
    // estimated AndGateDelay
    pNtk->AndGateDelay = 0.0;
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
    pNtkNew->nConstrs   = pNtk->nConstrs;
    pNtkNew->nBarBufs   = pNtk->nBarBufs;
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
    Abc_NtkForEachBox( pNtk, pObj, i )
        Abc_NtkDupBox( pNtkNew, pObj, fCopyNames );
    // transfer logic level
    Abc_NtkForEachCi( pNtk, pObj, i )
        pObj->pCopy->Level = pObj->Level;
    // transfer the names
//    Abc_NtkTrasferNames( pNtk, pNtkNew );
    Abc_ManTimeDup( pNtk, pNtkNew );
    if ( pNtk->vOnehots )
        pNtkNew->vOnehots = (Vec_Ptr_t *)Vec_VecDupInt( (Vec_Vec_t *)pNtk->vOnehots );
    if ( pNtk->pSeqModel )
        pNtkNew->pSeqModel = Abc_CexDup( pNtk->pSeqModel, Abc_NtkLatchNum(pNtk) );
    if ( pNtk->vObjPerm )
        pNtkNew->vObjPerm = Vec_IntDup( pNtk->vObjPerm );
    pNtkNew->AndGateDelay = pNtk->AndGateDelay;
    if ( pNtkNew->pManTime && Abc_FrameReadLibGen() && pNtkNew->AndGateDelay == 0.0 )
        pNtkNew->AndGateDelay = Mio_LibraryReadDelayAigNode((Mio_Library_t *)Abc_FrameReadLibGen());
    // initialize logic level of the CIs
    if ( pNtk->AndGateDelay != 0.0 && pNtk->pManTime != NULL && pNtk->ntkType != ABC_NTK_STRASH && Type == ABC_NTK_STRASH )
    {
        Abc_NtkForEachCi( pNtk, pObj, i )
            pObj->pCopy->Level = (int)(Abc_MaxFloat(0, Abc_NodeReadArrivalWorst(pObj)) / pNtk->AndGateDelay);
    }
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
Abc_Ntk_t * Abc_NtkStartFromWithLatches( Abc_Ntk_t * pNtk, Abc_NtkType_t Type, Abc_NtkFunc_t Func, int nLatches )
{
    Abc_Ntk_t * pNtkNew; 
    Abc_Obj_t * pObj, * pNode0, * pNode1;
    int fCopyNames, i;
    if ( pNtk == NULL )
        return NULL;
    assert( Abc_NtkLatchNum(pNtk) == 0 );
    // decide whether to copy the names
    fCopyNames = ( Type != ABC_NTK_NETLIST );
    // start the network
    pNtkNew = Abc_NtkAlloc( Type, Func, 1 );
    pNtkNew->nConstrs   = pNtk->nConstrs;
    pNtkNew->nBarBufs   = pNtk->nBarBufs;
    // duplicate the name and the spec
    pNtkNew->pName = Extra_UtilStrsav(pNtk->pName);
    pNtkNew->pSpec = Extra_UtilStrsav(pNtk->pSpec);
    // clean the node copy fields
    Abc_NtkCleanCopy( pNtk );
    // map the constant nodes
    if ( Abc_NtkIsStrash(pNtk) && Abc_NtkIsStrash(pNtkNew) )
        Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);
    // clone CIs/CIs/boxes
    for ( i = 0; i < Abc_NtkPiNum(pNtk)-nLatches; i++ )
        Abc_NtkDupObj( pNtkNew, Abc_NtkPi(pNtk, i), fCopyNames );
    for ( i = 0; i < Abc_NtkPoNum(pNtk)-nLatches; i++ )
        Abc_NtkDupObj( pNtkNew, Abc_NtkPo(pNtk, i), fCopyNames );
    for ( i = 0; i < nLatches; i++ )
    {
        pObj = Abc_NtkCreateLatch(pNtkNew);
        Abc_LatchSetInit0( pObj );
        pNode0 = Abc_NtkCreateBi(pNtkNew);
        Abc_NtkPo(pNtk, Abc_NtkPoNum(pNtk)-nLatches+i)->pCopy = pNode0;
        pNode1 = Abc_NtkCreateBo(pNtkNew);
        Abc_NtkPi(pNtk, Abc_NtkPiNum(pNtk)-nLatches+i)->pCopy = pNode1;
        Abc_ObjAddFanin( pObj, pNode0 );
        Abc_ObjAddFanin( pNode1, pObj );
        Abc_ObjAssignName( pNode0, Abc_ObjName(pNode0), NULL );
        Abc_ObjAssignName( pNode1, Abc_ObjName(pNode1), NULL );
    }
    // transfer logic level
//    Abc_NtkForEachCi( pNtk, pObj, i )
//        pObj->pCopy->Level = pObj->Level;
    // transfer the names
//    Abc_NtkTrasferNames( pNtk, pNtkNew );
    Abc_ManTimeDup( pNtk, pNtkNew );
    if ( pNtk->vOnehots )
        pNtkNew->vOnehots = (Vec_Ptr_t *)Vec_VecDupInt( (Vec_Vec_t *)pNtk->vOnehots );
    if ( pNtk->pSeqModel )
        pNtkNew->pSeqModel = Abc_CexDup( pNtk->pSeqModel, Abc_NtkLatchNum(pNtk) );
    if ( pNtk->vObjPerm )
        pNtkNew->vObjPerm = Vec_IntDup( pNtk->vObjPerm );
    pNtkNew->AndGateDelay = pNtk->AndGateDelay;
    // initialize logic level of the CIs
    if ( pNtk->AndGateDelay != 0.0 && pNtk->pManTime != NULL && pNtk->ntkType != ABC_NTK_STRASH && Type == ABC_NTK_STRASH )
    {
        Abc_NtkForEachCi( pNtk, pObj, i )
            pObj->pCopy->Level = (int)(Abc_MaxFloat(0, Abc_NodeReadArrivalWorst(pObj)) / pNtk->AndGateDelay);
    }
    // check that the CI/CO/latches are copied correctly
    assert( Abc_NtkCiNum(pNtk)    == Abc_NtkCiNum(pNtkNew) );
    assert( Abc_NtkCoNum(pNtk)    == Abc_NtkCoNum(pNtkNew) );
    assert( nLatches              == Abc_NtkLatchNum(pNtkNew) );
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
    pNtkNew->nConstrs   = pNtk->nConstrs;
    pNtkNew->nBarBufs   = pNtk->nBarBufs;
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
    Abc_NtkForEachBox( pNtk, pObj, i )
    {
        if ( Abc_ObjIsLatch(pObj) )
            continue;
        Abc_NtkDupBox(pNtkNew, pObj, 1);
    }
    if ( pNtk->vObjPerm )
        pNtkNew->vObjPerm = Vec_IntDup( pNtk->vObjPerm );
    pNtkNew->AndGateDelay = pNtk->AndGateDelay;
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
    // duplicate timing manager
    if ( pNtk->pManTime )
        Abc_NtkTimeInitialize( pNtkNew, pNtk );
    if ( pNtk->vPhases )
        Abc_NtkTransferPhases( pNtkNew, pNtk );
    if ( pNtk->pWLoadUsed )
        pNtkNew->pWLoadUsed = Abc_UtilStrsav( pNtk->pWLoadUsed );
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
    if ( pNtkNew->pName == NULL || strlen(pNtkNew->pName) == 0 )
    {
        ABC_FREE( pNtkNew->pName );
        pNtkNew->pName = Extra_UtilStrsav("unknown");
    }
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
            pObj->pCopy = Abc_AigAnd( (Abc_Aig_t *)pNtkNew->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj) );
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
                Abc_NtkDupObj(pNtkNew, pObj, Abc_NtkHasBlackbox(pNtk) && Abc_ObjIsNet(pObj));
        // reconnect all objects (no need to transfer attributes on edges)
        Abc_NtkForEachObj( pNtk, pObj, i )
            if ( !Abc_ObjIsBox(pObj) && !Abc_ObjIsBo(pObj) )
                Abc_ObjForEachFanin( pObj, pFanin, k )
                    Abc_ObjAddFanin( pObj->pCopy, pFanin->pCopy );
    }
    // duplicate the EXDC Ntk
    if ( pNtk->pExdc )
        pNtkNew->pExdc = Abc_NtkDup( pNtk->pExdc );
    if ( pNtk->pExcare )
        pNtkNew->pExcare = Abc_NtkDup( (Abc_Ntk_t *)pNtk->pExcare );
    // duplicate timing manager
    if ( pNtk->pManTime )
        Abc_NtkTimeInitialize( pNtkNew, pNtk );
    if ( pNtk->vPhases )
        Abc_NtkTransferPhases( pNtkNew, pNtk );
    if ( pNtk->pWLoadUsed )
        pNtkNew->pWLoadUsed = Abc_UtilStrsav( pNtk->pWLoadUsed );
    // check correctness
    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkDup(): Network check has failed.\n" );
    pNtk->pCopy = pNtkNew;
    return pNtkNew;
}
Abc_Ntk_t * Abc_NtkDupDfs( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vNodes;
    Abc_Ntk_t * pNtkNew; 
    Abc_Obj_t * pObj, * pFanin;
    int i, k;
    if ( pNtk == NULL )
        return NULL;
    assert( !Abc_NtkIsStrash(pNtk) && !Abc_NtkIsNetlist(pNtk) );
    // start the network
    pNtkNew = Abc_NtkStartFrom( pNtk, pNtk->ntkType, pNtk->ntkFunc );
    // copy the internal nodes
    vNodes = Abc_NtkDfs( pNtk, 0 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
        Abc_NtkDupObj( pNtkNew, pObj, 0 );
    Vec_PtrFree( vNodes );
    // reconnect all objects (no need to transfer attributes on edges)
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( !Abc_ObjIsBox(pObj) && !Abc_ObjIsBo(pObj) )
            Abc_ObjForEachFanin( pObj, pFanin, k )
                if ( pObj->pCopy && pFanin->pCopy )
                    Abc_ObjAddFanin( pObj->pCopy, pFanin->pCopy );
    // duplicate the EXDC Ntk
    if ( pNtk->pExdc )
        pNtkNew->pExdc = Abc_NtkDup( pNtk->pExdc );
    if ( pNtk->pExcare )
        pNtkNew->pExcare = Abc_NtkDup( (Abc_Ntk_t *)pNtk->pExcare );
    // duplicate timing manager
    if ( pNtk->pManTime )
        Abc_NtkTimeInitialize( pNtkNew, pNtk );
    if ( pNtk->vPhases )
        Abc_NtkTransferPhases( pNtkNew, pNtk );
    if ( pNtk->pWLoadUsed )
        pNtkNew->pWLoadUsed = Abc_UtilStrsav( pNtk->pWLoadUsed );
    // check correctness
    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkDup(): Network check has failed.\n" );
    pNtk->pCopy = pNtkNew;
    return pNtkNew;
}
Abc_Ntk_t * Abc_NtkDupDfsNoBarBufs( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vNodes;
    Abc_Ntk_t * pNtkNew; 
    Abc_Obj_t * pObj, * pFanin;
    int i, k;
    if ( pNtk == NULL )
        return NULL;
    assert( Abc_NtkIsLogic(pNtk) );
    assert( pNtk->nBarBufs2 > 0 );
    // start the network
    pNtkNew = Abc_NtkStartFrom( pNtk, pNtk->ntkType, pNtk->ntkFunc );
    // copy the internal nodes
    vNodes = Abc_NtkDfs2( pNtk );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
        if ( Abc_ObjIsBarBuf(pObj) )
            pObj->pCopy = Abc_ObjFanin0(pObj)->pCopy;
        else
            Abc_NtkDupObj( pNtkNew, pObj, 0 );
    Vec_PtrFree( vNodes );
    // reconnect all objects (no need to transfer attributes on edges)
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( !Abc_ObjIsBox(pObj) && !Abc_ObjIsBo(pObj) && !Abc_ObjIsBarBuf(pObj) )
            Abc_ObjForEachFanin( pObj, pFanin, k )
                if ( pObj->pCopy && pFanin->pCopy )
                    Abc_ObjAddFanin( pObj->pCopy, pFanin->pCopy );
    // duplicate the EXDC Ntk
    if ( pNtk->pExdc )
        pNtkNew->pExdc = Abc_NtkDup( pNtk->pExdc );
    if ( pNtk->pExcare )
        pNtkNew->pExcare = Abc_NtkDup( (Abc_Ntk_t *)pNtk->pExcare );
    // duplicate timing manager
    if ( pNtk->pManTime )
        Abc_NtkTimeInitialize( pNtkNew, pNtk );
    if ( pNtk->vPhases )
        Abc_NtkTransferPhases( pNtkNew, pNtk );
    if ( pNtk->pWLoadUsed )
        pNtkNew->pWLoadUsed = Abc_UtilStrsav( pNtk->pWLoadUsed );
    // check correctness
    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkDup(): Network check has failed.\n" );
    pNtk->pCopy = pNtkNew;
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Duplicate the AIG while adding latches.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkRestrashWithLatches( Abc_Ntk_t * pNtk, int nLatches )
{
    Abc_Ntk_t * pNtkAig;
    Abc_Obj_t * pObj;
    int i;
    assert( Abc_NtkIsStrash(pNtk) );
    // start the new network (constants and CIs of the old network will point to the their counterparts in the new network)
    pNtkAig = Abc_NtkStartFromWithLatches( pNtk, ABC_NTK_STRASH, ABC_FUNC_AIG, nLatches );
    // restrash the nodes (assuming a topological order of the old network)
    Abc_NtkForEachNode( pNtk, pObj, i )
        pObj->pCopy = Abc_AigAnd( (Abc_Aig_t *)pNtkAig->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj) );
    // finalize the network
    Abc_NtkFinalize( pNtk, pNtkAig );
    // make sure everything is okay
    if ( !Abc_NtkCheck( pNtkAig ) )
    {
        printf( "Abc_NtkStrash: The network check has failed.\n" );
        Abc_NtkDelete( pNtkAig );
        return NULL;
    }
    return pNtkAig;

}

/**Function*************************************************************

  Synopsis    [Duplicate the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDupTransformMiter( Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObj, * pObj2, * pMiter;
    int i;
    assert( Abc_NtkIsStrash(pNtk) );
    // start the network
    pNtkNew = Abc_NtkAlloc( pNtk->ntkType, pNtk->ntkFunc, 1 );
    pNtkNew->nConstrs   = pNtk->nConstrs;
    pNtkNew->nBarBufs   = pNtk->nBarBufs;
    // duplicate the name and the spec
    pNtkNew->pName = Extra_UtilStrsav(pNtk->pName);
    pNtkNew->pSpec = Extra_UtilStrsav(pNtk->pSpec);
    // clean the node copy fields
    Abc_NtkCleanCopy( pNtk );
    // map the constant nodes
     Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);
    // clone CIs/CIs/boxes
    Abc_NtkForEachPi( pNtk, pObj, i )
        Abc_NtkDupObj( pNtkNew, pObj, 1 );
    Abc_NtkForEachPo( pNtk, pObj, i )
        Abc_NtkDupObj( pNtkNew, pObj, 1 ), i++;
    Abc_NtkForEachBox( pNtk, pObj, i )
        Abc_NtkDupBox( pNtkNew, pObj, 1 );
    // copy the AND gates
    Abc_AigForEachAnd( pNtk, pObj, i )
        pObj->pCopy = Abc_AigAnd( (Abc_Aig_t *)pNtkNew->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj) );
    // create new miters
    Abc_NtkForEachPo( pNtk, pObj, i )
    {
        pObj2 = Abc_NtkPo( pNtk, ++i );
        pMiter = Abc_AigXor( (Abc_Aig_t *)pNtkNew->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild0Copy(pObj2) );
        Abc_ObjAddFanin( pObj->pCopy, pMiter );
    }
    Abc_NtkForEachLatchInput( pNtk, pObj, i )
        Abc_ObjAddFanin( pObj->pCopy, Abc_ObjChild0Copy(pObj) );
    // cleanup
    Abc_AigCleanup( (Abc_Aig_t *)pNtkNew->pManFunc );
    // check that the CI/CO/latches are copied correctly
    assert( Abc_NtkPiNum(pNtk) == Abc_NtkPiNum(pNtkNew) );
    assert( Abc_NtkPoNum(pNtk) == 2*Abc_NtkPoNum(pNtkNew) );
    assert( Abc_NtkLatchNum(pNtk) == Abc_NtkLatchNum(pNtkNew) );
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
    Abc_NtkOrderCisCos( pNtkNew );

    // perform the final check
    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkDup(): Network check has failed.\n" );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Duplicate the bottom levels of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkBottom( Abc_Ntk_t * pNtk, int Level )
{
    char Buffer[500];
    Abc_Ntk_t * pNtkNew; 
    Abc_Obj_t * pObj, * pFanin;
    int i, k;
    assert( Abc_NtkIsLogic(pNtk) );
    assert( Abc_NtkLatchNum(pNtk) == 0 );

    // start the network
    pNtkNew = Abc_NtkAlloc( pNtk->ntkType, pNtk->ntkFunc, 1 );
    sprintf( Buffer, "%s%s", pNtk->pName, "_bot" );
    pNtkNew->pName = Extra_UtilStrsav(Buffer);

    // clean the node copy fields
    Abc_NtkCleanCopy( pNtk );
    // clone CIs/CIs/boxes
    Abc_NtkForEachPi( pNtk, pObj, i )
        Abc_NtkDupObj( pNtkNew, pObj, 1 );

    // copy the internal nodes
    // duplicate the nets and nodes (CIs/COs/latches already dupped)
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( pObj->pCopy == NULL && Abc_ObjIsNode(pObj) && Abc_ObjLevel(pObj) <= Level )
            Abc_NtkDupObj(pNtkNew, pObj, 0);
    // reconnect all objects (no need to transfer attributes on edges)
    Abc_NtkForEachObj( pNtk, pObj, i )
        Abc_ObjForEachFanin( pObj, pFanin, k )
            if ( pObj->pCopy && pFanin->pCopy )
                Abc_ObjAddFanin( pObj->pCopy, pFanin->pCopy );

    // create new primary outputs
    Abc_NtkForEachObj( pNtk, pObj, i )
        Abc_ObjForEachFanin( pObj, pFanin, k )
            if ( !pObj->pCopy && pFanin->pCopy && Abc_ObjIsNode(pFanin) )
            {
                Abc_Obj_t * pNodeNew = Abc_NtkCreatePo(pNtkNew);
                Abc_ObjAddFanin( pNodeNew, pFanin->pCopy );
                Abc_ObjAssignName( pNodeNew, Abc_ObjName(pNodeNew), NULL );
            }

    // perform the final check
    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkBottom(): Network check has failed.\n" );
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
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
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
    assert( Abc_ObjIsNode(pNode) || (Abc_NtkIsStrash(pNtk) && (Abc_AigNodeIsConst(pNode) || Abc_ObjIsCi(pNode)))  ); 
    
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
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
    {
        // if it is an AIG, add to the hash table
        if ( Abc_NtkIsStrash(pNtk) )
        {
            pObj->pCopy = Abc_AigAnd( (Abc_Aig_t *)pNtkNew->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj) );
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
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
    {
        // if it is an AIG, add to the hash table
        if ( Abc_NtkIsStrash(pNtk) )
        {
            pObj->pCopy = Abc_AigAnd( (Abc_Aig_t *)pNtkNew->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj) );
        }
        else
        {
            Abc_NtkDupObj( pNtkNew, pObj, 0 );
            Abc_ObjForEachFanin( pObj, pFanin, k )
                Abc_ObjAddFanin( pObj->pCopy, pFanin->pCopy );
        }
    }
    Vec_PtrFree( vNodes );

    // add the POs corresponding to the root nodes
    Vec_PtrForEachEntry( Abc_Obj_t *, vRoots, pObj, i )
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

  Synopsis    [Adds new nodes to the cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkAppendToCone( Abc_Ntk_t * pNtkNew, Abc_Ntk_t * pNtk, Vec_Ptr_t * vRoots )
{
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj;
    int i, iNodeId;

    assert( Abc_NtkIsStrash(pNtkNew) );
    assert( Abc_NtkIsStrash(pNtk) );

    // collect the nodes in the TFI of the output (mark the TFI)
    vNodes = Abc_NtkDfsNodes( pNtk, (Abc_Obj_t **)Vec_PtrArray(vRoots), Vec_PtrSize(vRoots) );

    // establish connection between the constant nodes
    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);

    // create the PIs
    Abc_NtkForEachCi( pNtk, pObj, i )
    {
        // skip CIs that are not used
        if ( !Abc_NodeIsTravIdCurrent(pObj) )
            continue;
        // find the corresponding CI in the new network
        iNodeId = Nm_ManFindIdByNameTwoTypes( pNtkNew->pManName, Abc_ObjName(pObj), ABC_OBJ_PI, ABC_OBJ_BO );
        if ( iNodeId == -1 )
        {
            pObj->pCopy = Abc_NtkCreatePi(pNtkNew);
            Abc_ObjAssignName( pObj->pCopy, Abc_ObjName(pObj), NULL );
        }
        else
            pObj->pCopy = Abc_NtkObj( pNtkNew, iNodeId );
    }

    // copy the nodes
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
        pObj->pCopy = Abc_AigAnd( (Abc_Aig_t *)pNtkNew->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj) );
    Vec_PtrFree( vNodes );

    // do not add the COs
    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkAppendToCone(): Network check has failed.\n" );
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
    Abc_NodeMffcConeSupp( pNode, vCone, vSupp );
    Abc_NodeRef_rec( pNode );
    // create the PIs
    Vec_PtrForEachEntry( Abc_Obj_t *, vSupp, pObj, i )
    {
        pObj->pCopy = Abc_NtkCreatePi(pNtkNew);
        Abc_ObjAssignName( pObj->pCopy, Abc_ObjName(pObj), NULL );
    }
    // create the PO
    pNodeCoNew = Abc_NtkCreatePo( pNtkNew );
    Abc_ObjAssignName( pNodeCoNew, pNodeName, NULL );
    // copy the nodes
    Vec_PtrForEachEntry( Abc_Obj_t *, vCone, pObj, i )
    {
        // if it is an AIG, add to the hash table
        if ( Abc_NtkIsStrash(pNtk) )
        {
            pObj->pCopy = Abc_AigAnd( (Abc_Aig_t *)pNtkNew->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj) );
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
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
        pObj->pCopy = Abc_NodeStrash( pNtkNew, pObj, 0 );
    Vec_PtrFree( vNodes );

    // add the PO
    pFinal = Abc_AigConst1( pNtkNew );
    Vec_PtrForEachEntry( Abc_Obj_t *, vRoots, pObj, i )
    {
        if ( Abc_ObjIsCo(pObj) )
            pOther = Abc_ObjFanin0(pObj)->pCopy;
        else
            pOther = pObj->pCopy;
        if ( Vec_IntEntry(vValues, i) == 0 )
            pOther = Abc_ObjNot(pOther);
        pFinal = Abc_AigAnd( (Abc_Aig_t *)pNtkNew->pManFunc, pFinal, pOther );
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
        Abc_ObjAssignName( Abc_NtkCreatePi(pNtkNew), (char *)Vec_PtrEntry(vNames, i), NULL );
    Abc_NodeFreeNames( vNames );
    // create the node, add PIs as fanins, set the function
    pNode = Abc_NtkCreateNode( pNtkNew );
    Abc_NtkForEachPi( pNtkNew, pFanin, i )
        Abc_ObjAddFanin( pNode, pFanin );
    pNode->pData = Abc_SopRegister( (Mem_Flex_t *)pNtkNew->pManFunc, pSop );
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
//    int LargePiece = (4 << ABC_NUM_STEPS);
    if ( pNtk == NULL )
        return;
    // free EXDC Ntk
    if ( pNtk->pExdc )
        Abc_NtkDelete( pNtk->pExdc );
    if ( pNtk->pExcare )
        Abc_NtkDelete( (Abc_Ntk_t *)pNtk->pExcare );
    // dereference the BDDs
    if ( Abc_NtkHasBdd(pNtk) )
    {
#ifdef ABC_USE_CUDD
        Abc_NtkForEachNode( pNtk, pObj, i )
            Cudd_RecursiveDeref( (DdManager *)pNtk->pManFunc, (DdNode *)pObj->pData );
#endif
    }
    // make sure all the marks are clean
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        // free large fanout arrays
//        if ( pNtk->pMmObj && pObj->vFanouts.nCap * 4 > LargePiece )
//            ABC_FREE( pObj->vFanouts.pArray );
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
            ABC_FREE( pObj->vFanouts.pArray );
            ABC_FREE( pObj->vFanins.pArray );
        }
    }
    if ( pNtk->pMmObj == NULL )
    {
        Abc_NtkForEachObj( pNtk, pObj, i )
            ABC_FREE( pObj );
    }
        
    // free the arrays
    Vec_PtrFree( pNtk->vPios );
    Vec_PtrFree( pNtk->vPis );
    Vec_PtrFree( pNtk->vPos );
    Vec_PtrFree( pNtk->vCis );
    Vec_PtrFree( pNtk->vCos );
    Vec_PtrFree( pNtk->vObjs );
    Vec_PtrFree( pNtk->vBoxes );
    ABC_FREE( pNtk->vTravIds.pArray );
    if ( pNtk->vLevelsR ) Vec_IntFree( pNtk->vLevelsR );
    ABC_FREE( pNtk->pModel );
    ABC_FREE( pNtk->pSeqModel );
    if ( pNtk->vSeqModelVec )
        Vec_PtrFreeFree( pNtk->vSeqModelVec );
    TotalMemory  = 0;
    TotalMemory += pNtk->pMmObj? Mem_FixedReadMemUsage(pNtk->pMmObj)  : 0;
    TotalMemory += pNtk->pMmStep? Mem_StepReadMemUsage(pNtk->pMmStep) : 0;
//    fprintf( stdout, "The total memory allocated internally by the network = %0.2f MB.\n", ((double)TotalMemory)/(1<<20) );
    // free the storage 
    if ( pNtk->pMmObj )
        Mem_FixedStop( pNtk->pMmObj, 0 );
    if ( pNtk->pMmStep )
        Mem_StepStop ( pNtk->pMmStep, 0 );
    // name manager
    Nm_ManFree( pNtk->pManName );
    // free the timing manager
    if ( pNtk->pManTime )
        Abc_ManTimeStop( pNtk->pManTime );
    Vec_IntFreeP( &pNtk->vPhases );
    // start the functionality manager
    if ( Abc_NtkIsStrash(pNtk) )
        Abc_AigFree( (Abc_Aig_t *)pNtk->pManFunc );
    else if ( Abc_NtkHasSop(pNtk) || Abc_NtkHasBlifMv(pNtk) )
        Mem_FlexStop( (Mem_Flex_t *)pNtk->pManFunc, 0 );
#ifdef ABC_USE_CUDD
    else if ( Abc_NtkHasBdd(pNtk) )
        Extra_StopManager( (DdManager *)pNtk->pManFunc );
#endif
    else if ( Abc_NtkHasAig(pNtk) )
        { if ( pNtk->pManFunc ) Hop_ManStop( (Hop_Man_t *)pNtk->pManFunc ); }
    else if ( Abc_NtkHasMapping(pNtk) )
        pNtk->pManFunc = NULL;
    else if ( !Abc_NtkHasBlackbox(pNtk) )
        assert( 0 );
    // free the hierarchy
    if ( pNtk->pDesign )
    {
        Abc_DesFree( pNtk->pDesign, pNtk );
        pNtk->pDesign = NULL;
    }
//    if ( pNtk->pBlackBoxes ) 
//        Vec_IntFree( pNtk->pBlackBoxes );
    // free node attributes
    Vec_PtrForEachEntry( Abc_Obj_t *, pNtk->vAttrs, pAttrMan, i )
        if ( pAttrMan )
            Vec_AttFree( (Vec_Att_t *)pAttrMan, 1 );
    assert( pNtk->pSCLib == NULL );
    Vec_IntFreeP( &pNtk->vGates );
    Vec_PtrFree( pNtk->vAttrs );
    Vec_IntFreeP( &pNtk->vNameIds );
    ABC_FREE( pNtk->pWLoadUsed );
    ABC_FREE( pNtk->pName );
    ABC_FREE( pNtk->pSpec );
    ABC_FREE( pNtk->pLutTimes );
    if ( pNtk->vOnehots )
        Vec_VecFree( (Vec_Vec_t *)pNtk->vOnehots );
    Vec_PtrFreeP( &pNtk->vLtlProperties );
    Vec_IntFreeP( &pNtk->vObjPerm );
    Vec_IntFreeP( &pNtk->vTopo );
    Vec_IntFreeP( &pNtk->vFins );
    ABC_FREE( pNtk );
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

    // special case
    pNet = Abc_NtkFindNet( pNtk, "[_c1_]" );
    if ( pNet != NULL )
    {
        pNode = Abc_NtkCreateNodeConst1( pNtk );
        Abc_ObjAddFanin( pNet, pNode );
    }

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
        Vec_PtrForEachEntry( Abc_Obj_t *, vNets, pNet, i )
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
void Abc_NtkMakeComb( Abc_Ntk_t * pNtk, int fRemoveLatches )
{
    Abc_Obj_t * pObj;
    int i;

    if ( Abc_NtkIsComb(pNtk) )
        return;

    assert( !Abc_NtkIsNetlist(pNtk) );
    assert( Abc_NtkHasOnlyLatchBoxes(pNtk) );

    // detach the latches
//    Abc_NtkForEachLatch( pNtk, pObj, i )
    Vec_PtrForEachEntryReverse( Abc_Obj_t *, pNtk->vBoxes, pObj, i )
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

    if ( fRemoveLatches )
    {
        // remove registers
        Vec_Ptr_t * vBos;
        vBos = Vec_PtrAlloc( 100 );
        Vec_PtrClear( pNtk->vPos );
        Abc_NtkForEachCo( pNtk, pObj, i )
            if ( Abc_ObjIsBi(pObj) )
                Vec_PtrPush( vBos, pObj );
            else
                Vec_PtrPush( pNtk->vPos, pObj );
        // remove COs
        Vec_PtrFree( pNtk->vCos );
        pNtk->vCos = NULL;
        // remove the BOs
        Vec_PtrForEachEntry( Abc_Obj_t *, vBos, pObj, i )
            Abc_NtkDeleteObj( pObj );
        Vec_PtrFree( vBos );
        // create COs
        pNtk->vCos = Vec_PtrDup( pNtk->vPos );
        // cleanup
        if ( Abc_NtkIsLogic(pNtk) )
            Abc_NtkCleanup( pNtk, 0 );
        else if ( Abc_NtkIsStrash(pNtk) )
            Abc_AigCleanup( (Abc_Aig_t *)pNtk->pManFunc );
        else
            assert( 0 );
    }
    else
    {
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
    }
    assert( Abc_NtkBiNum(pNtk) == 0 );

    if ( !Abc_NtkCheck( pNtk ) )
        fprintf( stdout, "Abc_NtkMakeComb(): Network check has failed.\n" );
}

/**Function*************************************************************

  Synopsis    [Converts the network to sequential.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkMakeSeq( Abc_Ntk_t * pNtk, int nLatchesToAdd )
{
    Abc_Obj_t * pObjLi, * pObjLo, * pObj;
    int i;
    assert( Abc_NtkBoxNum(pNtk) == 0 );
    if ( !Abc_NtkIsComb(pNtk) )
    {
        printf( "The network is a not a combinational one.\n" );
        return;
    }
    if ( nLatchesToAdd >= Abc_NtkPiNum(pNtk) )
    {
        printf( "The number of latches is more or equal than the number of PIs.\n" );
        return;
    }
    if ( nLatchesToAdd >= Abc_NtkPoNum(pNtk) )
    {
        printf( "The number of latches is more or equal than the number of POs.\n" );
        return;
    }

    // move the last PIs to become CIs
    Vec_PtrClear( pNtk->vPis );
    Abc_NtkForEachCi( pNtk, pObj, i )
    {
        if ( i < Abc_NtkCiNum(pNtk) - nLatchesToAdd )
        {
            Vec_PtrPush( pNtk->vPis, pObj );
            continue;
        }
        pObj->Type = ABC_OBJ_BO;
        pNtk->nObjCounts[ABC_OBJ_PI]--;
        pNtk->nObjCounts[ABC_OBJ_BO]++;
    }

    // move the last POs to become COs
    Vec_PtrClear( pNtk->vPos );
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        if ( i < Abc_NtkCoNum(pNtk) - nLatchesToAdd )
        {
            Vec_PtrPush( pNtk->vPos, pObj );
            continue;
        }
        pObj->Type = ABC_OBJ_BI;
        pNtk->nObjCounts[ABC_OBJ_PO]--;
        pNtk->nObjCounts[ABC_OBJ_BI]++;
    }

    // create latches
    for ( i = 0; i < nLatchesToAdd; i++ )
    {
        pObjLo = Abc_NtkCi( pNtk, Abc_NtkCiNum(pNtk) - nLatchesToAdd + i );
        pObjLi = Abc_NtkCo( pNtk, Abc_NtkCoNum(pNtk) - nLatchesToAdd + i );
        pObj = Abc_NtkCreateLatch( pNtk );
        Abc_ObjAddFanin( pObj, pObjLi );
        Abc_ObjAddFanin( pObjLo, pObj );
        Abc_LatchSetInit0( pObj );
    }

    if ( !Abc_NtkCheck( pNtk ) )
        fprintf( stdout, "Abc_NtkMakeSeq(): Network check has failed.\n" );
}


/**Function*************************************************************

  Synopsis    [Removes all POs, except one.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkMakeOnePo( Abc_Ntk_t * pNtkInit, int Output, int nRange )
{
    Abc_Ntk_t * pNtk;
    Vec_Ptr_t * vPosLeft;
    Vec_Ptr_t * vCosLeft;
    Abc_Obj_t * pNodePo;
    int i;
    assert( !Abc_NtkIsNetlist(pNtkInit) );
    assert( Abc_NtkHasOnlyLatchBoxes(pNtkInit) );
    if ( Output < 0 || Output >= Abc_NtkPoNum(pNtkInit) )
    {
        printf( "PO index is incorrect.\n" );
        return NULL;
    }

    pNtk = Abc_NtkDup( pNtkInit );
    if ( Abc_NtkPoNum(pNtk) == 1 )
        return pNtk;

    if ( nRange < 1 )
        nRange = 1;

    // filter POs
    vPosLeft = Vec_PtrAlloc( nRange );
    Abc_NtkForEachPo( pNtk, pNodePo, i )
        if ( i < Output || i >= Output + nRange )
            Abc_NtkDeleteObjPo( pNodePo );
        else
            Vec_PtrPush( vPosLeft, pNodePo );
    // filter COs
    vCosLeft = Vec_PtrDup( vPosLeft );
    for ( i = Abc_NtkPoNum(pNtk); i < Abc_NtkCoNum(pNtk); i++ )
        Vec_PtrPush( vCosLeft, Abc_NtkCo(pNtk, i) );
    // update arrays
    Vec_PtrFree( pNtk->vPos );  pNtk->vPos = vPosLeft;
    Vec_PtrFree( pNtk->vCos );  pNtk->vCos = vCosLeft;

    // clean the network
    if ( Abc_NtkIsStrash(pNtk) )
    {
        Abc_AigCleanup( (Abc_Aig_t *)pNtk->pManFunc );
        printf( "Run sequential cleanup (\"scl\") to get rid of dangling logic.\n" );
    }
    else
    {
        printf( "Run sequential cleanup (\"st; scl\") to get rid of dangling logic.\n" );
    }

    if ( !Abc_NtkCheck( pNtk ) )
        fprintf( stdout, "Abc_NtkMakeComb(): Network check has failed.\n" );
    return pNtk;
}

/**Function*************************************************************

  Synopsis    [Removes POs with suppsize less than 2 and PIs without fanout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkTrim( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i, k, m;

    // filter POs
    k = m = 0;
    Abc_NtkForEachCo( pNtk, pObj, i )
    {  
        if ( Abc_ObjIsPo(pObj) )
        {
            // remove constant nodes and PI pointers
            if ( Abc_ObjFaninNum(Abc_ObjFanin0(pObj)) == 0 )
            {
                Abc_ObjDeleteFanin( pObj, Abc_ObjFanin0(pObj) );
                if ( Abc_ObjFanoutNum(Abc_ObjFanin0(pObj)) == 0 && !Abc_ObjIsPi(Abc_ObjFanin0(pObj)) )
                    Abc_NtkDeleteObj_rec( Abc_ObjFanin0(pObj), 1 );
                pNtk->vObjs->pArray[pObj->Id] = NULL;
                pObj->Id = (1<<26)-1;
                pNtk->nObjCounts[pObj->Type]--;
                pNtk->nObjs--;
                Abc_ObjRecycle( pObj );
                continue;
            }
            // remove buffers/inverters of PIs
            if ( Abc_ObjFaninNum(Abc_ObjFanin0(pObj)) == 1 )
            {
                if ( Abc_ObjIsPi(Abc_ObjFanin0(Abc_ObjFanin0(pObj))) )
                {
                    Abc_ObjDeleteFanin( pObj, Abc_ObjFanin0(pObj) );
                    if ( Abc_ObjFanoutNum(Abc_ObjFanin0(pObj)) == 0 )
                        Abc_NtkDeleteObj_rec( Abc_ObjFanin0(pObj), 1 );
                    pNtk->vObjs->pArray[pObj->Id] = NULL;
                    pObj->Id = (1<<26)-1;
                    pNtk->nObjCounts[pObj->Type]--;
                    pNtk->nObjs--;
                    Abc_ObjRecycle( pObj );
                    continue;
                }
            }
            Vec_PtrWriteEntry( pNtk->vPos, m++, pObj );
        }
        Vec_PtrWriteEntry( pNtk->vCos, k++, pObj );
    }
    Vec_PtrShrink( pNtk->vPos, m );
    Vec_PtrShrink( pNtk->vCos, k );

    // filter PIs
    k = m = 0;
    Abc_NtkForEachCi( pNtk, pObj, i )
    {
        if ( Abc_ObjIsPi(pObj) )
        {
            if ( Abc_ObjFanoutNum(pObj) == 0 )
            {
                pNtk->vObjs->pArray[pObj->Id] = NULL;
                pObj->Id = (1<<26)-1;
                pNtk->nObjCounts[pObj->Type]--;
                pNtk->nObjs--;
                Abc_ObjRecycle( pObj );
                continue;
            }
            Vec_PtrWriteEntry( pNtk->vPis, m++, pObj );
        }
        Vec_PtrWriteEntry( pNtk->vCis, k++, pObj );
    }
    Vec_PtrShrink( pNtk->vPis, m );
    Vec_PtrShrink( pNtk->vCis, k );

    return Abc_NtkDup( pNtk );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDropSatOutputs( Abc_Ntk_t * pNtk, Vec_Ptr_t * vCexes, int fVerbose )
{
    Abc_Obj_t * pObj, * pConst0, * pFaninNew;
    int i, Counter = 0;
    assert( Vec_PtrSize(vCexes) == Abc_NtkPoNum(pNtk) );
    pConst0 = Abc_ObjNot( Abc_AigConst1(pNtk) );
    Abc_NtkForEachPo( pNtk, pObj, i )
    {
        if ( Vec_PtrEntry( vCexes, i ) == NULL )
            continue;
        Counter++;
        pFaninNew = Abc_ObjNotCond( pConst0, Abc_ObjFaninC0(pObj) );
        Abc_ObjPatchFanin( pObj, Abc_ObjFanin0(pObj), pFaninNew );
        assert( Abc_ObjChild0(pObj) == pConst0 );
        // if a PO is driven by a latch, they have the same name...
//        if ( Abc_ObjIsBo(pObj) )
//            Nm_ManDeleteIdName( pNtk->pManName, Abc_ObjId(pObj) );
    }
    if ( fVerbose )
        printf( "Logic cones of %d POs have been replaced by constant 0.\n", Counter );
    Counter = Abc_AigCleanup( (Abc_Aig_t *)pNtk->pManFunc );
//    printf( "Cleanup removed %d nodes.\n", Counter );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDropOneOutput( Abc_Ntk_t * pNtk, int iOutput, int fSkipSweep, int fUseConst1 )
{
    Abc_Obj_t * pObj, * pConst0, * pFaninNew;
    pObj = Abc_NtkPo( pNtk, iOutput );
    if ( Abc_ObjFanin0(pObj) == Abc_AigConst1(pNtk) )
    {
        if ( !Abc_ObjFaninC0(pObj) ^ fUseConst1 )
            Abc_ObjXorFaninC( pObj, 0 );
        return;
    }
    pConst0 = Abc_ObjNotCond( Abc_AigConst1(pNtk), !fUseConst1 );
    pFaninNew = Abc_ObjNotCond( pConst0, Abc_ObjFaninC0(pObj) );
    Abc_ObjPatchFanin( pObj, Abc_ObjFanin0(pObj), pFaninNew );
    assert( Abc_ObjChild0(pObj) == pConst0 );
    if ( fSkipSweep )
        return;
    Abc_AigCleanup( (Abc_Aig_t *)pNtk->pManFunc );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkSwapOneOutput( Abc_Ntk_t * pNtk, int iOutput )
{
    Abc_Obj_t * pObj1, * pObj2;
    Abc_Obj_t * pChild1Old, * pChild2Old;
    Abc_Obj_t * pChild1, * pChild2;
    if ( iOutput == 0 )
        return;
    pObj1      = Abc_NtkPo( pNtk, 0 );
    pObj2      = Abc_NtkPo( pNtk, iOutput );
    if ( Abc_ObjFanin0(pObj1) == Abc_ObjFanin0(pObj2) )
    {
        if ( Abc_ObjFaninC0(pObj1) ^ Abc_ObjFaninC0(pObj2) )
        {
            Abc_ObjXorFaninC( pObj1, 0 );
            Abc_ObjXorFaninC( pObj2, 0 );
        }
        return;
    }
    pChild1Old = Abc_ObjChild0( pObj1 );
    pChild2Old = Abc_ObjChild0( pObj2 );
    pChild1    = Abc_ObjNotCond( pChild1Old, Abc_ObjFaninC0(pObj2) );
    pChild2    = Abc_ObjNotCond( pChild2Old, Abc_ObjFaninC0(pObj1) );
    Abc_ObjPatchFanin( pObj1, Abc_ObjFanin0(pObj1), pChild2 );
    Abc_ObjPatchFanin( pObj2, Abc_ObjFanin0(pObj2), pChild1 );
    assert( Abc_ObjChild0(pObj1) == pChild2Old );
    assert( Abc_ObjChild0(pObj2) == pChild1Old );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkRemovePo( Abc_Ntk_t * pNtk, int iOutput, int fRemoveConst0 )
{
    Abc_Obj_t * pObj = Abc_NtkPo(pNtk, iOutput);
    if ( Abc_ObjFanin0(pObj) == Abc_AigConst1(pNtk) && Abc_ObjFaninC0(pObj) == fRemoveConst0 )
        Abc_NtkDeleteObj( pObj );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Abc_NtkReadFlopPerm( char * pFileName, int nFlops )
{
    char Buffer[1000];
    FILE * pFile;
    Vec_Int_t * vFlops;
    int iFlop = -1;
    pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open input file \"%s\".\n", pFileName );
        return NULL;
    }
    vFlops = Vec_IntAlloc( nFlops );
    while ( fgets( Buffer, 1000, pFile ) != NULL )
    {
        if ( Buffer[0] == ' ' || Buffer[0] == '\r' || Buffer[0] == '\n' )
            continue;
        iFlop = atoi( Buffer );
        if ( iFlop < 0 || iFlop >= nFlops )
        {
            printf( "Flop ID (%d) is out of range.\n", iFlop );
            fclose( pFile );
            Vec_IntFree( vFlops );
            return NULL;
        }
        Vec_IntPush( vFlops, iFlop );
    }
    fclose( pFile );
    if ( Vec_IntSize(vFlops) != nFlops )
    {
        printf( "The number of flops read in from file (%d) is different from the number of flops in the circuit (%d).\n", iFlop, nFlops );
        Vec_IntFree( vFlops );
        return NULL;
    }
    return vFlops;    
}
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPermute( Abc_Ntk_t * pNtk, int fInputs, int fOutputs, int fFlops, char * pFlopPermFile )
{
    Abc_Obj_t * pTemp;
    Vec_Int_t * vInputs, * vOutputs, * vFlops, * vTemp;
    int i, k, Entry;
    // start permutation arrays
    if ( pFlopPermFile )
    {
        vFlops = Abc_NtkReadFlopPerm( pFlopPermFile, Abc_NtkLatchNum(pNtk) );
        if ( vFlops == NULL )
            return;
        fInputs  = 0;
        fOutputs = 0;
        fFlops   = 0;
    }
    else
        vFlops   = Vec_IntStartNatural( Abc_NtkLatchNum(pNtk) );
    vInputs  = Vec_IntStartNatural( Abc_NtkPiNum(pNtk) );
    vOutputs = Vec_IntStartNatural( Abc_NtkPoNum(pNtk) );
    // permute inputs
    if ( fInputs )
    for ( i = 0; i < Abc_NtkPiNum(pNtk); i++ )
    {
        k = rand() % Abc_NtkPiNum(pNtk);
        // swap indexes
        Entry = Vec_IntEntry( vInputs, i );
        Vec_IntWriteEntry( vInputs, i, Vec_IntEntry(vInputs, k) );
        Vec_IntWriteEntry( vInputs, k, Entry );
        // swap PIs
        pTemp = (Abc_Obj_t *)Vec_PtrEntry( pNtk->vPis, i );
        Vec_PtrWriteEntry( pNtk->vPis, i, Vec_PtrEntry(pNtk->vPis, k) );
        Vec_PtrWriteEntry( pNtk->vPis, k, pTemp );
        // swap CIs
        pTemp = (Abc_Obj_t *)Vec_PtrEntry( pNtk->vCis, i );
        Vec_PtrWriteEntry( pNtk->vCis, i, Vec_PtrEntry(pNtk->vCis, k) );
        Vec_PtrWriteEntry( pNtk->vCis, k, pTemp );
//printf( "Swapping PIs %d and %d.\n", i, k );
    }
    // permute outputs
    if ( fOutputs )
    for ( i = 0; i < Abc_NtkPoNum(pNtk); i++ )
    {
        k = rand() % Abc_NtkPoNum(pNtk);
        // swap indexes
        Entry = Vec_IntEntry( vOutputs, i );
        Vec_IntWriteEntry( vOutputs, i, Vec_IntEntry(vOutputs, k) );
        Vec_IntWriteEntry( vOutputs, k, Entry );
        // swap POs
        pTemp = (Abc_Obj_t *)Vec_PtrEntry( pNtk->vPos, i );
        Vec_PtrWriteEntry( pNtk->vPos, i, Vec_PtrEntry(pNtk->vPos, k) );
        Vec_PtrWriteEntry( pNtk->vPos, k, pTemp );
        // swap COs
        pTemp = (Abc_Obj_t *)Vec_PtrEntry( pNtk->vCos, i );
        Vec_PtrWriteEntry( pNtk->vCos, i, Vec_PtrEntry(pNtk->vCos, k) );
        Vec_PtrWriteEntry( pNtk->vCos, k, pTemp );
//printf( "Swapping POs %d and %d.\n", i, k );
    }
    // permute flops
    assert( Abc_NtkBoxNum(pNtk) == Abc_NtkLatchNum(pNtk) );
    if ( fFlops )
    for ( i = 0; i < Abc_NtkLatchNum(pNtk); i++ )
    {
        k = rand() % Abc_NtkLatchNum(pNtk);
        // swap indexes
        Entry = Vec_IntEntry( vFlops, i );
        Vec_IntWriteEntry( vFlops, i, Vec_IntEntry(vFlops, k) );
        Vec_IntWriteEntry( vFlops, k, Entry );
        // swap flops
        pTemp = (Abc_Obj_t *)Vec_PtrEntry( pNtk->vBoxes, i );
        Vec_PtrWriteEntry( pNtk->vBoxes, i, Vec_PtrEntry(pNtk->vBoxes, k) );
        Vec_PtrWriteEntry( pNtk->vBoxes, k, pTemp );
        // swap CIs
        pTemp = (Abc_Obj_t *)Vec_PtrEntry( pNtk->vCis, Abc_NtkPiNum(pNtk)+i );
        Vec_PtrWriteEntry( pNtk->vCis, Abc_NtkPiNum(pNtk)+i, Vec_PtrEntry(pNtk->vCis, Abc_NtkPiNum(pNtk)+k) );
        Vec_PtrWriteEntry( pNtk->vCis, Abc_NtkPiNum(pNtk)+k, pTemp );
        // swap COs
        pTemp = (Abc_Obj_t *)Vec_PtrEntry( pNtk->vCos, Abc_NtkPoNum(pNtk)+i );
        Vec_PtrWriteEntry( pNtk->vCos, Abc_NtkPoNum(pNtk)+i, Vec_PtrEntry(pNtk->vCos, Abc_NtkPoNum(pNtk)+k) );
        Vec_PtrWriteEntry( pNtk->vCos, Abc_NtkPoNum(pNtk)+k, pTemp );

//printf( "Swapping flops %d and %d.\n", i, k );
    }
    // invert arrays
    vInputs = Vec_IntInvert( vTemp = vInputs, -1 );
    Vec_IntFree( vTemp );
    vOutputs = Vec_IntInvert( vTemp = vOutputs, -1 );
    Vec_IntFree( vTemp );
    vFlops = Vec_IntInvert( vTemp = vFlops, -1 );
    Vec_IntFree( vTemp );
    // pack the results into the output array
    Vec_IntFreeP( &pNtk->vObjPerm );
    pNtk->vObjPerm = Vec_IntAlloc( Abc_NtkPiNum(pNtk) + Abc_NtkPoNum(pNtk) + Abc_NtkLatchNum(pNtk) );
    Vec_IntForEachEntry( vInputs, Entry, i )
        Vec_IntPush( pNtk->vObjPerm, Entry );
    Vec_IntForEachEntry( vOutputs, Entry, i )
        Vec_IntPush( pNtk->vObjPerm, Entry );
    Vec_IntForEachEntry( vFlops, Entry, i )
        Vec_IntPush( pNtk->vObjPerm, Entry );
    // cleanup
    Vec_IntFree( vInputs );
    Vec_IntFree( vOutputs );
    Vec_IntFree( vFlops );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeCompareByFanoutCount( Abc_Obj_t ** pp1, Abc_Obj_t ** pp2 )
{
    int Diff = Abc_ObjFanoutNum(*pp2) - Abc_ObjFanoutNum(*pp1);
    if ( Diff < 0 )
        return -1;
    if ( Diff > 0 ) 
        return 1;
    Diff = strcmp( Abc_ObjName(*pp1), Abc_ObjName(*pp2) );
    if ( Diff < 0 )
        return -1;
    if ( Diff > 0 ) 
        return 1;
    return 0; 
}
void Abc_NtkPermutePiUsingFanout( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode; int i;
    qsort( (void *)Vec_PtrArray(pNtk->vPis), Vec_PtrSize(pNtk->vPis), sizeof(Abc_Obj_t *), 
        (int (*)(const void *, const void *)) Abc_NodeCompareByFanoutCount );
    Vec_PtrClear( pNtk->vCis );
    Vec_PtrForEachEntry( Abc_Obj_t *, pNtk->vPis, pNode, i )
        Vec_PtrPush( pNtk->vCis, pNode );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkUnpermute( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vTemp, * vTemp2, * vLatch;
    int i, * pInputs, * pOutputs, * pFlops;
    if ( pNtk->vObjPerm == NULL )
    {
        printf( "Abc_NtkUnpermute(): Initial permutation is not available.\n" );
        return;
    }
    assert( Abc_NtkBoxNum(pNtk) == Abc_NtkLatchNum(pNtk) );
    // get reverve permutation
    pInputs  = Vec_IntArray( pNtk->vObjPerm );
    pOutputs = pInputs  + Abc_NtkPiNum(pNtk);
    pFlops   = pOutputs + Abc_NtkPoNum(pNtk);
    // create new PI array
    vTemp = Vec_PtrAlloc( Abc_NtkPiNum(pNtk) );
    for ( i = 0; i < Abc_NtkPiNum(pNtk); i++ )
        Vec_PtrPush( vTemp, Abc_NtkPi(pNtk, pInputs[i]) );
    Vec_PtrFreeP( &pNtk->vPis );
    pNtk->vPis = vTemp;
    // create new PO array
    vTemp = Vec_PtrAlloc( Abc_NtkPoNum(pNtk) );
    for ( i = 0; i < Abc_NtkPoNum(pNtk); i++ )
        Vec_PtrPush( vTemp, Abc_NtkPo(pNtk, pOutputs[i]) );
    Vec_PtrFreeP( &pNtk->vPos );
    pNtk->vPos = vTemp;
    // create new CI/CO arrays
    vTemp  = Vec_PtrDup( pNtk->vPis );
    vTemp2 = Vec_PtrDup( pNtk->vPos );
    vLatch = Vec_PtrAlloc( Abc_NtkLatchNum(pNtk) );
    for ( i = 0; i < Abc_NtkLatchNum(pNtk); i++ )
    {
//printf( "Setting flop %d to be %d.\n", i, pFlops[i] );
        Vec_PtrPush( vTemp,  Abc_NtkCi(pNtk, Abc_NtkPiNum(pNtk) + pFlops[i]) );
        Vec_PtrPush( vTemp2, Abc_NtkCo(pNtk, Abc_NtkPoNum(pNtk) + pFlops[i]) );
        Vec_PtrPush( vLatch, Abc_NtkBox(pNtk, pFlops[i]) );
    }
    Vec_PtrFreeP( &pNtk->vCis );
    Vec_PtrFreeP( &pNtk->vCos );
    Vec_PtrFreeP( &pNtk->vBoxes );
    pNtk->vCis   = vTemp;
    pNtk->vCos   = vTemp2;
    pNtk->vBoxes = vLatch;
    // cleanup
    Vec_IntFreeP( &pNtk->vObjPerm );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkNodeDup( Abc_Ntk_t * pNtkInit, int nLimit, int fVerbose )
{
    Vec_Ptr_t * vNodes, * vFanouts;
    Abc_Ntk_t * pNtk;
    Abc_Obj_t * pObj, * pObjNew, * pFanin, * pFanout;
    int i, k;
    pNtk = Abc_NtkDup( pNtkInit );
    vNodes = Vec_PtrAlloc( 100 );
    vFanouts = Vec_PtrAlloc( 100 );
    do
    {
        Vec_PtrClear( vNodes );
        Abc_NtkForEachNode( pNtk, pObj, i )
            if ( Abc_ObjFanoutNum(pObj) >= nLimit )
                Vec_PtrPush( vNodes, pObj );
        Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
        {
            pObjNew = Abc_NtkDupObj( pNtk, pObj, 0 );
            Abc_ObjForEachFanin( pObj, pFanin, k )
                Abc_ObjAddFanin( pObjNew, pFanin );
            Abc_NodeCollectFanouts( pObj, vFanouts );
            Vec_PtrShrink( vFanouts, nLimit / 2 );
            Vec_PtrForEachEntry( Abc_Obj_t *, vFanouts, pFanout, k )
                Abc_ObjPatchFanin( pFanout, pObj, pObjNew );
        }
        if ( fVerbose )
            printf( "Duplicated %d nodes.\n", Vec_PtrSize(vNodes) );
    }
    while ( Vec_PtrSize(vNodes) > 0 );
    Vec_PtrFree( vFanouts );
    Vec_PtrFree( vNodes );
    return pNtk;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkCreateFromSops( char * pName, Vec_Ptr_t * vSops )
{
    int i, k, nObjBeg;
    char * pSop = (char *)Vec_PtrEntry(vSops, 0);
    Abc_Ntk_t * pNtk = Abc_NtkAlloc( ABC_NTK_LOGIC, ABC_FUNC_SOP, 1 );
    pNtk->pName = Extra_UtilStrsav( pName );
    for ( k = 0; pSop[k] != ' '; k++ )
        Abc_NtkCreatePi( pNtk );
    nObjBeg = Abc_NtkObjNumMax(pNtk);
    Vec_PtrForEachEntry( char *, vSops, pSop, i )
    {
        Abc_Obj_t * pObj = Abc_NtkCreateNode( pNtk );
        pObj->pData = Abc_SopRegister( (Mem_Flex_t*)pNtk->pManFunc, pSop );
        for ( k = 0; pSop[k] != ' '; k++ )
            Abc_ObjAddFanin( pObj, Abc_NtkCi(pNtk, k) );
    }
    for ( i = 0; i < Vec_PtrSize(vSops); i++ )
    {
        Abc_Obj_t * pObj = Abc_NtkObj( pNtk, nObjBeg + i );
        Abc_Obj_t * pObjPo = Abc_NtkCreatePo( pNtk );
        Abc_ObjAddFanin( pObjPo, pObj );
    }
    Abc_NtkAddDummyPiNames( pNtk );
    Abc_NtkAddDummyPoNames( pNtk );
    return pNtk;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkCreateFromGias( char * pName, Vec_Ptr_t * vGias )
{
    Gia_Man_t * pGia = (Gia_Man_t *)Vec_PtrEntry(vGias, 0);
    Abc_Ntk_t * pNtk = Abc_NtkAlloc( ABC_NTK_STRASH, ABC_FUNC_AIG, 1 );
    Abc_Obj_t * pAbcObj, * pAbcObjPo;
    Gia_Obj_t * pObj; int i, k;
    pNtk->pName = Extra_UtilStrsav( pName );
    for ( k = 0; k < Gia_ManCiNum(pGia); k++ )
        Abc_NtkCreatePi( pNtk );
    Vec_PtrForEachEntry( Gia_Man_t *, vGias, pGia, i )
    {
        assert( Gia_ManCoNum(pGia) == 1 );
        Gia_ManCleanValue(pGia);
        Gia_ManForEachCi( pGia, pObj, k )
            pObj->Value = Abc_ObjId( Abc_NtkCi(pNtk, k) );
        Gia_ManForEachAnd( pGia, pObj, k )
        {
            Abc_Obj_t * pAbcObj0 = Abc_NtkObj( pNtk, Gia_ObjFanin0(pObj)->Value );
            Abc_Obj_t * pAbcObj1 = Abc_NtkObj( pNtk, Gia_ObjFanin1(pObj)->Value );
            pAbcObj0 = Abc_ObjNotCond( pAbcObj0, Gia_ObjFaninC0(pObj) );
            pAbcObj1 = Abc_ObjNotCond( pAbcObj1, Gia_ObjFaninC1(pObj) );
            pAbcObj  = Abc_AigAnd( (Abc_Aig_t *)pNtk->pManFunc, pAbcObj0, pAbcObj1 );
            pObj->Value = Abc_ObjId( pAbcObj ); 
        }
        pObj = Gia_ManCo(pGia, 0);
        if ( Gia_ObjFaninId0p(pGia, pObj) == 0 )
            pAbcObj = Abc_ObjNot( Abc_AigConst1(pNtk) );
        else
            pAbcObj = Abc_NtkObj( pNtk, Gia_ObjFanin0(pObj)->Value );
        pAbcObj = Abc_ObjNotCond( pAbcObj, Gia_ObjFaninC0(pObj) );
        pAbcObjPo = Abc_NtkCreatePo( pNtk );
        Abc_ObjAddFanin( pAbcObjPo, pAbcObj );
    }
    Abc_NtkAddDummyPiNames( pNtk );
    Abc_NtkAddDummyPoNames( pNtk );
    return pNtk;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

