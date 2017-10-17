/**CFile****************************************************************

  FileName    [seqRetCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Construction and manipulation of sequential AIGs.]

  Synopsis    [The core of retiming procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: seqRetCore.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "seqInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

/* 
    Retiming can be represented in three equivalent forms:
    - as a set of integer lags for each node (array of chars by node ID)
    - as a set of node numbers with lag for each, fwd and bwd (two arrays of Seq_RetStep_t_)
    - as a set of latch moves over the nodes, fwd and bwd (two arrays of node pointers Abc_Obj_t *)
*/

static void        Abc_ObjRetimeForward( Abc_Obj_t * pObj );
static int         Abc_ObjRetimeBackward( Abc_Obj_t * pObj, Abc_Ntk_t * pNtk, stmm_table * tTable, Vec_Int_t * vValues );
static void        Abc_ObjRetimeBackwardUpdateEdge( Abc_Obj_t * pObj, int Edge, stmm_table * tTable );
static void        Abc_NtkRetimeSetInitialValues( Abc_Ntk_t * pNtk, stmm_table * tTable, int * pModel );

static void        Seq_NtkImplementRetimingForward( Abc_Ntk_t * pNtk, Vec_Ptr_t * vMoves );
static int         Seq_NtkImplementRetimingBackward( Abc_Ntk_t * pNtk, Vec_Ptr_t * vMoves, int fVerbose );
static void        Abc_ObjRetimeForward( Abc_Obj_t * pObj );
static int         Abc_ObjRetimeBackward( Abc_Obj_t * pObj, Abc_Ntk_t * pNtk, stmm_table * tTable, Vec_Int_t * vValues );
static void        Abc_ObjRetimeBackwardUpdateEdge( Abc_Obj_t * pObj, int Edge, stmm_table * tTable );
static void        Abc_NtkRetimeSetInitialValues( Abc_Ntk_t * pNtk, stmm_table * tTable, int * pModel );

static Vec_Ptr_t * Abc_NtkUtilRetimingTry( Abc_Ntk_t * pNtk, bool fForward );
static Vec_Ptr_t * Abc_NtkUtilRetimingGetMoves( Abc_Ntk_t * pNtk, Vec_Int_t * vSteps, bool fForward );
static Vec_Int_t * Abc_NtkUtilRetimingSplit( Vec_Str_t * vLags, int fForward );
static void        Abc_ObjRetimeForwardTry( Abc_Obj_t * pObj, int nLatches );  
static void        Abc_ObjRetimeBackwardTry( Abc_Obj_t * pObj, int nLatches );
  

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs performs optimal delay retiming.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seq_NtkSeqRetimeDelay( Abc_Ntk_t * pNtk, int nMaxIters, int fInitial, int fVerbose )
{
    Abc_Seq_t * p = pNtk->pManFunc;
    int RetValue;
    if ( !fInitial )
        Seq_NtkLatchSetValues( pNtk, ABC_INIT_DC );
    // get the retiming lags
    p->nMaxIters = nMaxIters;
    if ( !Seq_AigRetimeDelayLags( pNtk, fVerbose ) )
        return;
    // implement this retiming
    RetValue = Seq_NtkImplementRetiming( pNtk, p->vLags, fVerbose );
    if ( RetValue == 0 )
        printf( "Retiming completed but initial state computation has failed.\n" );
}

/**Function*************************************************************

  Synopsis    [Performs most forward retiming.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seq_NtkSeqRetimeForward( Abc_Ntk_t * pNtk, int fInitial, int fVerbose )
{
    Vec_Ptr_t * vMoves;
    Abc_Obj_t * pNode;
    int i;
    if ( !fInitial )
        Seq_NtkLatchSetValues( pNtk, ABC_INIT_DC );
    // get the forward moves
    vMoves = Abc_NtkUtilRetimingTry( pNtk, 1 );
    // undo the forward moves
    Vec_PtrForEachEntryReverse( vMoves, pNode, i )
        Abc_ObjRetimeBackwardTry( pNode, 1 );
    // implement this forward retiming
    Seq_NtkImplementRetimingForward( pNtk, vMoves );
    Vec_PtrFree( vMoves ); 
}

/**Function*************************************************************

  Synopsis    [Performs most backward retiming.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seq_NtkSeqRetimeBackward( Abc_Ntk_t * pNtk, int fInitial, int fVerbose )
{
    Vec_Ptr_t * vMoves;
    Abc_Obj_t * pNode;
    int i, RetValue;
    if ( !fInitial )
        Seq_NtkLatchSetValues( pNtk, ABC_INIT_DC );
    // get the backward moves
    vMoves = Abc_NtkUtilRetimingTry( pNtk, 0 );
    // undo the backward moves
    Vec_PtrForEachEntryReverse( vMoves, pNode, i )
        Abc_ObjRetimeForwardTry( pNode, 1 );
    // implement this backward retiming
    RetValue = Seq_NtkImplementRetimingBackward( pNtk, vMoves, fVerbose );
    Vec_PtrFree( vMoves ); 
    if ( RetValue == 0 )
        printf( "Retiming completed but initial state computation has failed.\n" );
}




/**Function*************************************************************

  Synopsis    [Implements the retiming on the sequential AIG.]

  Description [Split the retiming into forward and backward.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Seq_NtkImplementRetiming( Abc_Ntk_t * pNtk, Vec_Str_t * vLags, int fVerbose )
{
    Vec_Int_t * vSteps;
    Vec_Ptr_t * vMoves;
    int RetValue;

    // forward retiming
    vSteps = Abc_NtkUtilRetimingSplit( vLags, 1 );
    // translate each set of steps into moves
    if ( fVerbose )
    printf( "The number of forward steps  = %6d.\n", Vec_IntSize(vSteps) );
    vMoves = Abc_NtkUtilRetimingGetMoves( pNtk, vSteps, 1 );
    if ( fVerbose )
    printf( "The number of forward moves  = %6d.\n", Vec_PtrSize(vMoves) );
    // implement this retiming
    Seq_NtkImplementRetimingForward( pNtk, vMoves );
    Vec_IntFree( vSteps );
    Vec_PtrFree( vMoves );

    // backward retiming
    vSteps = Abc_NtkUtilRetimingSplit( vLags, 0 );
    // translate each set of steps into moves
    if ( fVerbose )
    printf( "The number of backward steps = %6d.\n", Vec_IntSize(vSteps) );
    vMoves = Abc_NtkUtilRetimingGetMoves( pNtk, vSteps, 0 );
    if ( fVerbose )
    printf( "The number of backward moves = %6d.\n", Vec_PtrSize(vMoves) );
    // implement this retiming
    RetValue = Seq_NtkImplementRetimingBackward( pNtk, vMoves, fVerbose );
    Vec_IntFree( vSteps );
    Vec_PtrFree( vMoves );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Implements the given retiming on the sequential AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seq_NtkImplementRetimingForward( Abc_Ntk_t * pNtk, Vec_Ptr_t * vMoves )
{
    Abc_Obj_t * pNode;
    int i;
    Vec_PtrForEachEntry( vMoves, pNode, i )
        Abc_ObjRetimeForward( pNode );
}

/**Function*************************************************************

  Synopsis    [Retimes node forward by one latch.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ObjRetimeForward( Abc_Obj_t * pObj )  
{
    Abc_Obj_t * pFanout;
    int Init0, Init1, Init, i;
    assert( Abc_ObjFaninNum(pObj) == 2 );
    assert( Seq_ObjFaninL0(pObj) >= 1 );
    assert( Seq_ObjFaninL1(pObj) >= 1 );
    // remove the init values from the fanins
    Init0 = Seq_NodeDeleteFirst( pObj, 0 ); 
    Init1 = Seq_NodeDeleteFirst( pObj, 1 );
    assert( Init0 != ABC_INIT_NONE );
    assert( Init1 != ABC_INIT_NONE );
    // take into account the complements in the node
    if ( Abc_ObjFaninC0(pObj) )
    {
        if ( Init0 == ABC_INIT_ZERO )
            Init0 = ABC_INIT_ONE;
        else if ( Init0 == ABC_INIT_ONE )
            Init0 = ABC_INIT_ZERO;
    }
    if ( Abc_ObjFaninC1(pObj) )
    {
        if ( Init1 == ABC_INIT_ZERO )
            Init1 = ABC_INIT_ONE;
        else if ( Init1 == ABC_INIT_ONE )
            Init1 = ABC_INIT_ZERO;
    }
    // compute the value at the output of the node
    if ( Init0 == ABC_INIT_ZERO || Init1 == ABC_INIT_ZERO )
        Init = ABC_INIT_ZERO;
    else if ( Init0 == ABC_INIT_ONE && Init1 == ABC_INIT_ONE )
        Init = ABC_INIT_ONE;
    else
        Init = ABC_INIT_DC;

    // make sure the label is clean
    Abc_ObjForEachFanout( pObj, pFanout, i )
        assert( pFanout->fMarkC == 0 );
    // add the init values to the fanouts
    Abc_ObjForEachFanout( pObj, pFanout, i )
    {
        if ( pFanout->fMarkC )
            continue;
        pFanout->fMarkC = 1;
        if ( Abc_ObjFaninId0(pFanout) != Abc_ObjFaninId1(pFanout) )
            Seq_NodeInsertLast( pFanout, Abc_ObjFanoutEdgeNum(pObj, pFanout), Init );
        else
        {
            assert( Abc_ObjFanin0(pFanout) == pObj );
            Seq_NodeInsertLast( pFanout, 0, Init );
            Seq_NodeInsertLast( pFanout, 1, Init );
        }
    }
    // clean the label
    Abc_ObjForEachFanout( pObj, pFanout, i )
        pFanout->fMarkC = 0;
}


/**Function*************************************************************

  Synopsis    [Implements the given retiming on the sequential AIG.]

  Description [Returns 0 of initial state computation fails.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Seq_NtkImplementRetimingBackward( Abc_Ntk_t * pNtk, Vec_Ptr_t * vMoves, int fVerbose )
{
    Seq_RetEdge_t RetEdge;
    stmm_table * tTable;
    stmm_generator * gen;
    Vec_Int_t * vValues;
    Abc_Ntk_t * pNtkProb, * pNtkMiter, * pNtkCnf;
    Abc_Obj_t * pNode, * pNodeNew;
    int * pModel, RetValue, i, clk;

    // return if the retiming is trivial
    if ( Vec_PtrSize(vMoves) == 0 )
        return 1;

    // create the network for the initial state computation
    // start the table and the array of PO values
    pNtkProb = Abc_NtkAlloc( ABC_NTK_LOGIC, ABC_FUNC_SOP, 1 );
    tTable   = stmm_init_table( stmm_numcmp, stmm_numhash );
    vValues  = Vec_IntAlloc( 100 );

    // perform the backward moves and build the network for initial state computation
    RetValue = 0;
    Vec_PtrForEachEntry( vMoves, pNode, i )
        RetValue |= Abc_ObjRetimeBackward( pNode, pNtkProb, tTable, vValues );

    // add the PIs corresponding to the white spots
    stmm_foreach_item( tTable, gen, (char **)&RetEdge, (char **)&pNodeNew )
        Abc_ObjAddFanin( pNodeNew, Abc_NtkCreatePi(pNtkProb) );

    // add the PI/PO names
    Abc_NtkAddDummyPiNames( pNtkProb );
    Abc_NtkAddDummyPoNames( pNtkProb );
    Abc_NtkAddDummyAssertNames( pNtkProb );

    // make sure everything is okay with the network structure
    if ( !Abc_NtkDoCheck( pNtkProb ) )
    {
        printf( "Seq_NtkImplementRetimingBackward: The internal network check has failed.\n" );
        Abc_NtkRetimeSetInitialValues( pNtk, tTable, NULL );
        Abc_NtkDelete( pNtkProb );
        stmm_free_table( tTable );
        Vec_IntFree( vValues );
        return 0;
    }

    // check if conflict is found
    if ( RetValue )
    {
        printf( "Seq_NtkImplementRetimingBackward: A top level conflict is detected. DC latch values are used.\n" );
        Abc_NtkRetimeSetInitialValues( pNtk, tTable, NULL );
        Abc_NtkDelete( pNtkProb );
        stmm_free_table( tTable );
        Vec_IntFree( vValues );
        return 0;
    }

    // get the miter cone
    pNtkMiter = Abc_NtkCreateTarget( pNtkProb, pNtkProb->vCos, vValues );
    Abc_NtkDelete( pNtkProb );
    Vec_IntFree( vValues );

    if ( fVerbose )
    printf( "The number of ANDs in the AIG = %5d.\n", Abc_NtkNodeNum(pNtkMiter) );

    // transform the miter into a logic network for efficient CNF construction
//    pNtkCnf = Abc_Ntk_Renode( pNtkMiter, 0, 100, 1, 0, 0 );
//    Abc_NtkDelete( pNtkMiter );
    pNtkCnf = pNtkMiter;

    // solve the miter
clk = clock();
//    RetValue = Abc_NtkMiterSat_OldAndRusty( pNtkCnf, 30, 0 );
    RetValue = Abc_NtkMiterSat( pNtkCnf, (sint64)500000, (sint64)50000000, 0, 0, NULL, NULL );
if ( fVerbose )
if ( clock() - clk > 100 )
{
PRT( "SAT solving time", clock() - clk );
}
    pModel = pNtkCnf->pModel;  pNtkCnf->pModel = NULL;
    Abc_NtkDelete( pNtkCnf );

    // analyze the result
    if ( RetValue == -1 || RetValue == 1 )
    {
        Abc_NtkRetimeSetInitialValues( pNtk, tTable, NULL );
        if ( RetValue == 1 )
            printf( "Seq_NtkImplementRetimingBackward: The problem is unsatisfiable. DC latch values are used.\n" );
        else
            printf( "Seq_NtkImplementRetimingBackward: The SAT problem timed out. DC latch values are used.\n" );
        stmm_free_table( tTable );
        return 0;
    }

    // set the values of the latches
    Abc_NtkRetimeSetInitialValues( pNtk, tTable, pModel );
    stmm_free_table( tTable );
    free( pModel );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Retimes node backward by one latch.]

  Description [Constructs the problem for initial state computation.
  Returns 1 if the conflict is found.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_ObjRetimeBackward( Abc_Obj_t * pObj, Abc_Ntk_t * pNtkNew, stmm_table * tTable, Vec_Int_t * vValues )  
{
    Abc_Obj_t * pFanout;
    Abc_InitType_t Init, Value;
    Seq_RetEdge_t RetEdge;
    Abc_Obj_t * pNodeNew, * pFanoutNew, * pBuffer;
    int i, Edge, fMet0, fMet1, fMetN;

    // make sure the node can be retimed
    assert( Seq_ObjFanoutLMin(pObj) > 0 );
    // get the fanout values
    fMet0 = fMet1 = fMetN = 0;
    Abc_ObjForEachFanout( pObj, pFanout, i )
    {
        if ( Abc_ObjFaninId0(pFanout) == pObj->Id )
        {
            Init = Seq_NodeGetInitLast( pFanout, 0 );
            if ( Init == ABC_INIT_ZERO )
                fMet0 = 1;
            else if ( Init == ABC_INIT_ONE )
                fMet1 = 1;
            else if ( Init == ABC_INIT_NONE )
                fMetN = 1;
        }
        if ( Abc_ObjFaninId1(pFanout) == pObj->Id )
        {
            Init = Seq_NodeGetInitLast( pFanout, 1 );
            if ( Init == ABC_INIT_ZERO )
                fMet0 = 1;
            else if ( Init == ABC_INIT_ONE )
                fMet1 = 1;
            else if ( Init == ABC_INIT_NONE )
                fMetN = 1;
        }
    }

    // consider the case when all fanout latches have don't-care values
    // the new values on the fanin edges will be don't-cares
    if ( !fMet0 && !fMet1 && !fMetN )
    {
        // make sure the label is clean
        Abc_ObjForEachFanout( pObj, pFanout, i )
            assert( pFanout->fMarkC == 0 );
        // update the fanout edges
        Abc_ObjForEachFanout( pObj, pFanout, i )
        {
            if ( pFanout->fMarkC )
                continue;
            pFanout->fMarkC = 1;
            if ( Abc_ObjFaninId0(pFanout) == pObj->Id )
                Seq_NodeDeleteLast( pFanout, 0 );
            if ( Abc_ObjFaninId1(pFanout) == pObj->Id )
                Seq_NodeDeleteLast( pFanout, 1 );
        }
        // clean the label
        Abc_ObjForEachFanout( pObj, pFanout, i )
            pFanout->fMarkC = 0;
        // update the fanin edges
        Abc_ObjRetimeBackwardUpdateEdge( pObj, 0, tTable );
        Abc_ObjRetimeBackwardUpdateEdge( pObj, 1, tTable );
        Seq_NodeInsertFirst( pObj, 0, ABC_INIT_DC );
        Seq_NodeInsertFirst( pObj, 1, ABC_INIT_DC );
        return 0;
    }
    // the initial values on the fanout edges contain 0, 1, or unknown
    // the new values on the fanin edges will be unknown

    // add new AND-gate to the network
    pNodeNew = Abc_NtkCreateNode( pNtkNew );
    pNodeNew->pData = Abc_SopCreateAnd2( pNtkNew->pManFunc, Abc_ObjFaninC0(pObj), Abc_ObjFaninC1(pObj) );

    // add PO fanouts if any
    if ( fMet0 )
    {
        Abc_ObjAddFanin( Abc_NtkCreatePo(pNtkNew), pNodeNew );
        Vec_IntPush( vValues, 0 );
    }
    if ( fMet1 )
    {
        Abc_ObjAddFanin( Abc_NtkCreatePo(pNtkNew), pNodeNew );
        Vec_IntPush( vValues, 1 );
    }

    // make sure the label is clean
    Abc_ObjForEachFanout( pObj, pFanout, i )
        assert( pFanout->fMarkC == 0 );
    // perform the changes
    Abc_ObjForEachFanout( pObj, pFanout, i )
    {
        if ( pFanout->fMarkC )
            continue;
        pFanout->fMarkC = 1;
        if ( Abc_ObjFaninId0(pFanout) == pObj->Id )
        {
            Edge = 0;
            Value = Seq_NodeDeleteLast( pFanout, Edge );
            if ( Value == ABC_INIT_NONE )
            {
                // value is unknown, remove it from the table
                RetEdge.iNode  = pFanout->Id;
                RetEdge.iEdge  = Edge;
                RetEdge.iLatch = Seq_ObjFaninL( pFanout, Edge ); // after edge is removed
                if ( !stmm_delete( tTable, (char **)&RetEdge, (char **)&pFanoutNew ) )
                    assert( 0 );
                // create the fanout of the AND gate
                Abc_ObjAddFanin( pFanoutNew, pNodeNew );
            }
        }
        if ( Abc_ObjFaninId1(pFanout) == pObj->Id )
        {
            Edge = 1;
            Value = Seq_NodeDeleteLast( pFanout, Edge );
            if ( Value == ABC_INIT_NONE )
            {
                // value is unknown, remove it from the table
                RetEdge.iNode  = pFanout->Id;
                RetEdge.iEdge  = Edge;
                RetEdge.iLatch = Seq_ObjFaninL( pFanout, Edge ); // after edge is removed
                if ( !stmm_delete( tTable, (char **)&RetEdge, (char **)&pFanoutNew ) )
                    assert( 0 );
                // create the fanout of the AND gate
                Abc_ObjAddFanin( pFanoutNew, pNodeNew );
            }
        }
    }
    // clean the label
    Abc_ObjForEachFanout( pObj, pFanout, i )
        pFanout->fMarkC = 0;

    // update the fanin edges
    Abc_ObjRetimeBackwardUpdateEdge( pObj, 0, tTable );
    Abc_ObjRetimeBackwardUpdateEdge( pObj, 1, tTable );
    Seq_NodeInsertFirst( pObj, 0, ABC_INIT_NONE );
    Seq_NodeInsertFirst( pObj, 1, ABC_INIT_NONE );

    // add the buffer
    pBuffer = Abc_NtkCreateNode( pNtkNew );
    pBuffer->pData = Abc_SopCreateBuf( pNtkNew->pManFunc );
    Abc_ObjAddFanin( pNodeNew, pBuffer );
    // point to it from the table
    RetEdge.iNode  = pObj->Id;
    RetEdge.iEdge  = 0;
    RetEdge.iLatch = 0;
    if ( stmm_insert( tTable, (char *)Seq_RetEdge2Int(RetEdge), (char *)pBuffer ) )
        assert( 0 );

    // add the buffer
    pBuffer = Abc_NtkCreateNode( pNtkNew );
    pBuffer->pData = Abc_SopCreateBuf( pNtkNew->pManFunc );
    Abc_ObjAddFanin( pNodeNew, pBuffer );
    // point to it from the table
    RetEdge.iNode  = pObj->Id;
    RetEdge.iEdge  = 1;
    RetEdge.iLatch = 0;
    if ( stmm_insert( tTable, (char *)Seq_RetEdge2Int(RetEdge), (char *)pBuffer ) )
        assert( 0 );

    // report conflict is found
    return fMet0 && fMet1;
}

/**Function*************************************************************

  Synopsis    [Generates the printable edge label with the initial state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ObjRetimeBackwardUpdateEdge( Abc_Obj_t * pObj, int Edge, stmm_table * tTable )
{
    Abc_Obj_t * pFanoutNew;
    Seq_RetEdge_t RetEdge;
    Abc_InitType_t Init;
    int nLatches, i;

    // get the number of latches on the edge
    nLatches = Seq_ObjFaninL( pObj, Edge );
    for ( i = nLatches - 1; i >= 0; i-- )
    {
        // get the value of this latch
        Init = Seq_NodeGetInitOne( pObj, Edge, i );
        if ( Init != ABC_INIT_NONE )
            continue;
        // get the retiming edge
        RetEdge.iNode  = pObj->Id;
        RetEdge.iEdge  = Edge;
        RetEdge.iLatch = i;
        // remove entry from table and add it with a different key
        if ( !stmm_delete( tTable, (char **)&RetEdge, (char **)&pFanoutNew ) )
            assert( 0 );
        RetEdge.iLatch++;
        if ( stmm_insert( tTable, (char *)Seq_RetEdge2Int(RetEdge), (char *)pFanoutNew ) )
            assert( 0 );
    }
}

/**Function*************************************************************

  Synopsis    [Sets the initial values.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkRetimeSetInitialValues( Abc_Ntk_t * pNtk, stmm_table * tTable, int * pModel )
{
    Abc_Obj_t * pNode;
    stmm_generator * gen;
    Seq_RetEdge_t RetEdge;
    Abc_InitType_t Init;
    int i;

    i = 0;
    stmm_foreach_item( tTable, gen, (char **)&RetEdge, NULL )
    {
        pNode = Abc_NtkObj( pNtk, RetEdge.iNode );
        Init = pModel? (pModel[i]? ABC_INIT_ONE : ABC_INIT_ZERO) : ABC_INIT_DC;
        Seq_NodeSetInitOne( pNode, RetEdge.iEdge, RetEdge.iLatch, Init );
        i++;
    }
}



/**Function*************************************************************

  Synopsis    [Performs forward retiming of the sequential AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NtkUtilRetimingTry( Abc_Ntk_t * pNtk, bool fForward )
{
    Vec_Ptr_t * vNodes, * vMoves;
    Abc_Obj_t * pNode, * pFanout, * pFanin;
    int i, k, nLatches;
    assert( Abc_NtkIsSeq( pNtk ) );
    // assume that all nodes can be retimed
    vNodes = Vec_PtrAlloc( 100 );
    Abc_AigForEachAnd( pNtk, pNode, i )
    {
        Vec_PtrPush( vNodes, pNode );
        pNode->fMarkA = 1;
    }
    // process the nodes
    vMoves = Vec_PtrAlloc( 100 );
    Vec_PtrForEachEntry( vNodes, pNode, i )
    {
//        printf( "(%d,%d) ", Seq_ObjFaninL0(pNode), Seq_ObjFaninL0(pNode) );
        // unmark the node as processed
        pNode->fMarkA = 0;
        // get the number of latches to retime
        if ( fForward )
            nLatches = Seq_ObjFaninLMin(pNode);
        else
            nLatches = Seq_ObjFanoutLMin(pNode);
        if ( nLatches == 0 )
            continue;
        assert( nLatches > 0 );
        // retime the latches forward
        if ( fForward )
            Abc_ObjRetimeForwardTry( pNode, nLatches );
        else
            Abc_ObjRetimeBackwardTry( pNode, nLatches );
        // write the moves
        for ( k = 0; k < nLatches; k++ )
            Vec_PtrPush( vMoves, pNode );
        // schedule fanouts for updating
        if ( fForward )
        {
            Abc_ObjForEachFanout( pNode, pFanout, k )
            {
                if ( Abc_ObjFaninNum(pFanout) != 2 || pFanout->fMarkA )
                    continue;
                pFanout->fMarkA = 1;
                Vec_PtrPush( vNodes, pFanout );
            }
        }
        else
        {
            Abc_ObjForEachFanin( pNode, pFanin, k )
            {
                if ( Abc_ObjFaninNum(pFanin) != 2 || pFanin->fMarkA )
                    continue;
                pFanin->fMarkA = 1;
                Vec_PtrPush( vNodes, pFanin );
            }
        }
    }
    Vec_PtrFree( vNodes );
    // make sure the marks are clean the the retiming is final
    Abc_AigForEachAnd( pNtk, pNode, i )
    {
        assert( pNode->fMarkA == 0 );
        if ( fForward ) 
            assert( Seq_ObjFaninLMin(pNode) == 0 );
        else
            assert( Seq_ObjFanoutLMin(pNode) == 0 );
    }
    return vMoves;
}

/**Function*************************************************************

  Synopsis    [Translates retiming steps into retiming moves.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NtkUtilRetimingGetMoves( Abc_Ntk_t * pNtk, Vec_Int_t * vSteps, bool fForward )
{
    Seq_RetStep_t RetStep;
    Vec_Ptr_t * vMoves;
    Abc_Obj_t * pNode;
    int i, k, iNode, nLatches, Number;
    int fChange;
    assert( Abc_NtkIsSeq( pNtk ) );

/*
    // try implementing all the moves at once
    Vec_IntForEachEntry( vSteps, Number, i )
    {
        // get the retiming step
        RetStep = Seq_Int2RetStep( Number );
        // get the node to be retimed
        pNode = Abc_NtkObj( pNtk, RetStep.iNode );
        assert( RetStep.nLatches > 0 );
        nLatches = RetStep.nLatches;

        if ( fForward )
            Abc_ObjRetimeForwardTry( pNode, nLatches );
        else
            Abc_ObjRetimeBackwardTry( pNode, nLatches );
    }
    // now look if any node has wrong number of latches
    Abc_AigForEachAnd( pNtk, pNode, i )
    {
        if ( Seq_ObjFaninL0(pNode) < 0 )
            printf( "Wrong 0node %d.\n", pNode->Id );
        if ( Seq_ObjFaninL1(pNode) < 0 )
            printf( "Wrong 1node %d.\n", pNode->Id );
    }
    // try implementing all the moves at once
    Vec_IntForEachEntry( vSteps, Number, i )
    {
        // get the retiming step
        RetStep = Seq_Int2RetStep( Number );
        // get the node to be retimed
        pNode = Abc_NtkObj( pNtk, RetStep.iNode );
        assert( RetStep.nLatches > 0 );
        nLatches = RetStep.nLatches;

        if ( !fForward )
            Abc_ObjRetimeForwardTry( pNode, nLatches );
        else
            Abc_ObjRetimeBackwardTry( pNode, nLatches );
    }
*/

    // process the nodes
    vMoves = Vec_PtrAlloc( 100 );
    while ( Vec_IntSize(vSteps) > 0 )
    {
        iNode = 0;
        fChange = 0;
        Vec_IntForEachEntry( vSteps, Number, i )
        {
            // get the retiming step
            RetStep = Seq_Int2RetStep( Number );
            // get the node to be retimed
            pNode = Abc_NtkObj( pNtk, RetStep.iNode );
            assert( RetStep.nLatches > 0 );
            // get the number of latches that can be retimed
            if ( fForward )
                nLatches = Seq_ObjFaninLMin(pNode);
            else
                nLatches = Seq_ObjFanoutLMin(pNode);
            if ( nLatches == 0 )
            {
                Vec_IntWriteEntry( vSteps, iNode++, Seq_RetStep2Int(RetStep) );
                continue;
            }
            assert( nLatches > 0 );
            fChange = 1;
            // get the number of latches to be retimed over this node
            nLatches = ABC_MIN( nLatches, (int)RetStep.nLatches );
            // retime the latches forward
            if ( fForward )
                Abc_ObjRetimeForwardTry( pNode, nLatches );
            else
                Abc_ObjRetimeBackwardTry( pNode, nLatches );
            // write the moves
            for ( k = 0; k < nLatches; k++ )
                Vec_PtrPush( vMoves, pNode );
            // subtract the retiming performed
            RetStep.nLatches -= nLatches;
            // store the node if it is not retimed completely
            if ( RetStep.nLatches > 0 )
                Vec_IntWriteEntry( vSteps, iNode++, Seq_RetStep2Int(RetStep) );
        }
        // reduce the array
        Vec_IntShrink( vSteps, iNode );
        if ( !fChange )
        {
            printf( "Warning: %d strange steps (a minor bug to be fixed later).\n", Vec_IntSize(vSteps) );
/*
            Vec_IntForEachEntry( vSteps, Number, i )
            {
                RetStep = Seq_Int2RetStep( Number );
                printf( "%d(%d) ", RetStep.iNode, RetStep.nLatches );
            }
            printf( "\n" );
*/
            break;
        }
    }
    // undo the tentative retiming
    if ( fForward )
    {
        Vec_PtrForEachEntryReverse( vMoves, pNode, i )
            Abc_ObjRetimeBackwardTry( pNode, 1 );
    }
    else
    {
        Vec_PtrForEachEntryReverse( vMoves, pNode, i )
            Abc_ObjRetimeForwardTry( pNode, 1 );
    }
    return vMoves;
}


/**Function*************************************************************

  Synopsis    [Splits retiming into forward and backward.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Abc_NtkUtilRetimingSplit( Vec_Str_t * vLags, int fForward )
{
    Vec_Int_t * vNodes;
    Seq_RetStep_t RetStep;
    int Value, i;
    vNodes = Vec_IntAlloc( 100 );
    Vec_StrForEachEntry( vLags, Value, i )
    {
        if ( Value < 0 && fForward )
        {
            RetStep.iNode = i;
            RetStep.nLatches = -Value;
            Vec_IntPush( vNodes, Seq_RetStep2Int(RetStep) );
        }
        else if ( Value > 0 && !fForward )
        {
            RetStep.iNode = i;
            RetStep.nLatches = Value;
            Vec_IntPush( vNodes, Seq_RetStep2Int(RetStep) );
        }
    }
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Retime node forward without initial states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ObjRetimeForwardTry( Abc_Obj_t * pObj, int nLatches )  
{
    Abc_Obj_t * pFanout;
    int i;
    // make sure it is an AND gate
    assert( Abc_ObjFaninNum(pObj) == 2 );
    // make sure it has enough latches
//    assert( Seq_ObjFaninL0(pObj) >= nLatches );
//    assert( Seq_ObjFaninL1(pObj) >= nLatches );
    // subtract these latches on the fanin side
    Seq_ObjAddFaninL0( pObj, -nLatches );
    Seq_ObjAddFaninL1( pObj, -nLatches );
    // make sure the label is clean
    Abc_ObjForEachFanout( pObj, pFanout, i )
        assert( pFanout->fMarkC == 0 );
    // add these latches on the fanout side
    Abc_ObjForEachFanout( pObj, pFanout, i )
    {
        if ( pFanout->fMarkC )
            continue;
        pFanout->fMarkC = 1;
        if ( Abc_ObjFaninId0(pFanout) != Abc_ObjFaninId1(pFanout) )
            Seq_ObjAddFanoutL( pObj, pFanout, nLatches );
        else
        {
            assert( Abc_ObjFanin0(pFanout) == pObj );
            Seq_ObjAddFaninL0( pFanout, nLatches );
            Seq_ObjAddFaninL1( pFanout, nLatches );
        }
    }
    // clean the label
    Abc_ObjForEachFanout( pObj, pFanout, i )
        pFanout->fMarkC = 0;
}

/**Function*************************************************************

  Synopsis    [Retime node backward without initial states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ObjRetimeBackwardTry( Abc_Obj_t * pObj, int nLatches )  
{
    Abc_Obj_t * pFanout;
    int i;
    // make sure it is an AND gate
    assert( Abc_ObjFaninNum(pObj) == 2 );
    // make sure the label is clean
    Abc_ObjForEachFanout( pObj, pFanout, i )
        assert( pFanout->fMarkC == 0 );
    // subtract these latches on the fanout side
    Abc_ObjForEachFanout( pObj, pFanout, i )
    {
        if ( pFanout->fMarkC )
            continue;
        pFanout->fMarkC = 1;
//        assert( Abc_ObjFanoutL(pObj, pFanout) >= nLatches );
        if ( Abc_ObjFaninId0(pFanout) != Abc_ObjFaninId1(pFanout) )
            Seq_ObjAddFanoutL( pObj, pFanout, -nLatches );
        else
        {
            assert( Abc_ObjFanin0(pFanout) == pObj );
            Seq_ObjAddFaninL0( pFanout, -nLatches );
            Seq_ObjAddFaninL1( pFanout, -nLatches );
        }
    }
    // clean the label
    Abc_ObjForEachFanout( pObj, pFanout, i )
        pFanout->fMarkC = 0;
    // add these latches on the fanin side
    Seq_ObjAddFaninL0( pObj, nLatches );
    Seq_ObjAddFaninL1( pObj, nLatches );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


