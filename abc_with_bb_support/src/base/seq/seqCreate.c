/**CFile****************************************************************

  FileName    [seqCreate.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Construction and manipulation of sequential AIGs.]

  Synopsis    [Transformations to and from the sequential AIG.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: seqCreate.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "seqInt.h"

/*
    A sequential network is similar to AIG in that it contains only
    AND gates. However, the AND-gates are currently not hashed. 

    When converting AIG into sequential AIG:
    - Const1/PIs/POs remain the same as in the original AIG.
    - Instead of the latches, a new cutset is added, which is currently
      defined as a set of AND gates that have a latch among their fanouts.
    - The edges of a sequential AIG are labeled with latch attributes
      in addition to the complementation attibutes. 
    - The attributes contain information about the number of latches 
      and their initial states. 
    - The number of latches is stored directly on the edges. The initial 
      states are stored in the sequential AIG manager.

    In the current version of the code, the sequential AIG is static 
    in the sense that the new AIG nodes are never created.
    The retiming (or retiming/mapping) is performed by moving the
    latches over the static nodes of the AIG.
    The new initial state after backward retiming is computed 
    by setting up and solving a SAT problem.
*/

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Abc_Obj_t * Abc_NodeAigToSeq( Abc_Obj_t * pObjNew, Abc_Obj_t * pObj, int Edge, Vec_Int_t * vInitValues );
static void        Abc_NtkAigCutsetCopy( Abc_Ntk_t * pNtk );
static Abc_Obj_t * Abc_NodeSeqToLogic( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pFanin, Seq_Lat_t * pRing, int nLatches );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Converts combinational AIG with latches into sequential AIG.]

  Description [The const/PI/PO nodes are duplicated. The internal
  nodes are duplicated in the topological order. The dangling nodes
  are not duplicated. The choice nodes are duplicated.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkAigToSeq( Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObj, * pFaninNew;
    Vec_Int_t * vInitValues;
    Abc_InitType_t Init;
    int i, k, RetValue;

    // make sure it is an AIG without self-feeding latches
    assert( Abc_NtkIsStrash(pNtk) );
    assert( Abc_NtkIsDfsOrdered(pNtk) );

    if ( RetValue = Abc_NtkRemoveSelfFeedLatches(pNtk) )
        printf( "Modified %d self-feeding latches. The result will not verify.\n", RetValue );
    assert( Abc_NtkCountSelfFeedLatches(pNtk) == 0 );

    // start the network
    pNtkNew = Abc_NtkAlloc( ABC_NTK_SEQ, ABC_FUNC_AIG, 1 );
    // duplicate the name and the spec
    pNtkNew->pName = Extra_UtilStrsav(pNtk->pName);
    pNtkNew->pSpec = Extra_UtilStrsav(pNtk->pSpec);

    // map the constant nodes
    Abc_NtkCleanCopy( pNtk );
    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);

    // copy all objects, except the latches and constant
    Vec_PtrFill( pNtkNew->vObjs, Abc_NtkObjNumMax(pNtk), NULL );
    Vec_PtrWriteEntry( pNtkNew->vObjs, 0, Abc_AigConst1(pNtk)->pCopy );
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        if ( i == 0 || Abc_ObjIsLatch(pObj) )
            continue;
        pObj->pCopy = Abc_ObjAlloc( pNtkNew, pObj->Type );
        pObj->pCopy->Id     = pObj->Id;      // the ID is the same for both
        pObj->pCopy->fPhase = pObj->fPhase;  // used to work with choices
        pObj->pCopy->Level  = pObj->Level;   // used for upper bound on clock cycle
        Vec_PtrWriteEntry( pNtkNew->vObjs, pObj->pCopy->Id, pObj->pCopy );
        pNtkNew->nObjs++;
    }
    pNtkNew->nObjCounts[ABC_OBJ_NODE] = pNtk->nObjCounts[ABC_OBJ_NODE];

    // create PI/PO and their names
    Abc_NtkForEachPi( pNtk, pObj, i )
    {
        Vec_PtrPush( pNtkNew->vPis, pObj->pCopy );
        Vec_PtrPush( pNtkNew->vCis, pObj->pCopy );
        Abc_ObjAssignName( pObj->pCopy, Abc_ObjName(pObj), NULL );
    }
    Abc_NtkForEachPo( pNtk, pObj, i ) 
    {
        Vec_PtrPush( pNtkNew->vPos, pObj->pCopy );
        Vec_PtrPush( pNtkNew->vCos, pObj->pCopy );
        Abc_ObjAssignName( pObj->pCopy, Abc_ObjName(pObj), NULL );
    }
    Abc_NtkForEachAssert( pNtk, pObj, i ) 
    {
        Vec_PtrPush( pNtkNew->vAsserts, pObj->pCopy );
        Vec_PtrPush( pNtkNew->vCos, pObj->pCopy );
        Abc_ObjAssignName( pObj->pCopy, Abc_ObjName(pObj), NULL );
    }

    // relink the choice nodes
    Abc_AigForEachAnd( pNtk, pObj, i )
        if ( pObj->pData )
            pObj->pCopy->pData = ((Abc_Obj_t *)pObj->pData)->pCopy;

    // start the storage for initial states
    Seq_Resize( pNtkNew->pManFunc, Abc_NtkObjNumMax(pNtkNew) );
    // reconnect the internal nodes
    vInitValues = Vec_IntAlloc( 100 );
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        // skip constants, PIs, and latches
        if ( Abc_ObjFaninNum(pObj) == 0 || Abc_ObjIsLatch(pObj) )
            continue;
        // process the first fanin
        Vec_IntClear( vInitValues );
        pFaninNew = Abc_NodeAigToSeq( pObj->pCopy, pObj, 0, vInitValues );
        Abc_ObjAddFanin( pObj->pCopy, pFaninNew );
        // store the initial values
        Vec_IntForEachEntry( vInitValues, Init, k )
            Seq_NodeInsertFirst( pObj->pCopy, 0, Init );
        // skip single-input nodes
        if ( Abc_ObjFaninNum(pObj) == 1 )
            continue;
        // process the second fanin
        Vec_IntClear( vInitValues );
        pFaninNew = Abc_NodeAigToSeq( pObj->pCopy, pObj, 1, vInitValues );
        Abc_ObjAddFanin( pObj->pCopy, pFaninNew );
        // store the initial values
        Vec_IntForEachEntry( vInitValues, Init, k )
            Seq_NodeInsertFirst( pObj->pCopy, 1, Init );
    }
    Vec_IntFree( vInitValues );

    // set the cutset composed of latch drivers
    Abc_NtkAigCutsetCopy( pNtk );
    Seq_NtkLatchGetEqualFaninNum( pNtkNew );

    // copy EXDC and check correctness
    if ( pNtk->pExdc )
        fprintf( stdout, "Warning: EXDC is not copied when converting to sequential AIG.\n" );
    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkAigToSeq(): Network check has failed.\n" );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Determines the fanin that is transparent for latches.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NodeAigToSeq( Abc_Obj_t * pObjNew, Abc_Obj_t * pObj, int Edge, Vec_Int_t * vInitValues )
{
    Abc_Obj_t * pFanin, * pFaninNew;
    Abc_InitType_t Init;
    // get the given fanin of the node
    pFanin = Abc_ObjFanin( pObj, Edge );
    // if fanin is the internal node, return its copy in the corresponding polarity
    if ( !Abc_ObjIsLatch(pFanin) )
        return Abc_ObjNotCond( pFanin->pCopy, Abc_ObjFaninC(pObj, Edge) );
    // fanin is a latch
    // get the new fanins
    pFaninNew = Abc_NodeAigToSeq( pObjNew, pFanin, 0, vInitValues );
    // get the initial state
    Init = Abc_LatchInit(pFanin);
    // complement the initial state if the inv is retimed over the latch
    if ( Abc_ObjIsComplement(pFaninNew) ) 
    {
        if ( Init == ABC_INIT_ZERO )
            Init = ABC_INIT_ONE;
        else if ( Init == ABC_INIT_ONE )
            Init = ABC_INIT_ZERO;
        else if ( Init != ABC_INIT_DC )
            assert( 0 );
    }
    // record the initial state
    Vec_IntPush( vInitValues, Init );
    return Abc_ObjNotCond( pFaninNew, Abc_ObjFaninC(pObj, Edge) );
}

/**Function*************************************************************

  Synopsis    [Collects the cut set nodes.]

  Description [These are internal AND gates that have latch fanouts.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkAigCutsetCopy( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pLatch, * pDriver, * pDriverNew;
    int i;
    Abc_NtkIncrementTravId(pNtk);
    Abc_NtkForEachLatch( pNtk, pLatch, i )
    {
        pDriver = Abc_ObjFanin0(pLatch);
        if ( Abc_NodeIsTravIdCurrent(pDriver) || !Abc_AigNodeIsAnd(pDriver) )
            continue;
        Abc_NodeSetTravIdCurrent(pDriver);
        pDriverNew = pDriver->pCopy;
        Vec_PtrPush( pDriverNew->pNtk->vCutSet, pDriverNew );
    }
}

/**Function*************************************************************

  Synopsis    [Converts a sequential AIG into a logic SOP network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkSeqToLogicSop( Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pNtkNew; 
    Abc_Obj_t * pObj, * pFaninNew;
    Seq_Lat_t * pRing;
    int i;

    assert( Abc_NtkIsSeq(pNtk) );
    // start the network without latches
    pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_LOGIC, ABC_FUNC_SOP );
    // duplicate the nodes
    Abc_AigForEachAnd( pNtk, pObj, i )
    {
        Abc_NtkDupObj(pNtkNew, pObj, 0);
        pObj->pCopy->pData = Abc_SopCreateAnd2( pNtkNew->pManFunc, Abc_ObjFaninC0(pObj), Abc_ObjFaninC1(pObj) );
    }
    // share and create the latches
    Seq_NtkShareLatches( pNtkNew, pNtk );
    // connect the objects
    Abc_AigForEachAnd( pNtk, pObj, i )
    {
        if ( pRing = Seq_NodeGetRing(pObj,0) )
            pFaninNew = pRing->pLatch;
        else
            pFaninNew = Abc_ObjFanin0(pObj)->pCopy;
        Abc_ObjAddFanin( pObj->pCopy, pFaninNew );

        if ( pRing = Seq_NodeGetRing(pObj,1) )
            pFaninNew = pRing->pLatch;
        else
            pFaninNew = Abc_ObjFanin1(pObj)->pCopy;
        Abc_ObjAddFanin( pObj->pCopy, pFaninNew );
    }
    // connect the POs
    Abc_NtkForEachPo( pNtk, pObj, i )
    {
        if ( pRing = Seq_NodeGetRing(pObj,0) )
            pFaninNew = pRing->pLatch;
        else
            pFaninNew = Abc_ObjFanin0(pObj)->pCopy;
        pFaninNew = Abc_ObjNotCond( pFaninNew, Abc_ObjFaninC0(pObj) );
        Abc_ObjAddFanin( pObj->pCopy, pFaninNew );
    }
    // clean the latch pointers
    Seq_NtkShareLatchesClean( pNtk );

    // add the latches and their names
    Abc_NtkAddDummyBoxNames( pNtkNew );
    Abc_NtkOrderCisCos( pNtkNew );
    // fix the problem with complemented and duplicated CO edges
    Abc_NtkLogicMakeSimpleCos( pNtkNew, 0 );
    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkSeqToLogicSop(): Network check has failed.\n" );
    return pNtkNew;
}


/**Function*************************************************************

  Synopsis    [Converts a sequential AIG into a logic SOP network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkSeqToLogicSop_old( Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pNtkNew; 
    Abc_Obj_t * pObj, * pFaninNew;
    int i;

    assert( Abc_NtkIsSeq(pNtk) );
    // start the network without latches
    pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_LOGIC, ABC_FUNC_SOP );

    // duplicate the nodes, create node functions
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        // skip the constant
        if ( Abc_ObjFaninNum(pObj) == 0 )
            continue;
        // duplicate the node
        Abc_NtkDupObj(pNtkNew, pObj, 0);
        if ( Abc_ObjFaninNum(pObj) == 1 )
        {
            assert( !Abc_ObjFaninC0(pObj) );
            pObj->pCopy->pData = Abc_SopCreateBuf( pNtkNew->pManFunc );
            continue;
        }
        pObj->pCopy->pData = Abc_SopCreateAnd2( pNtkNew->pManFunc, Abc_ObjFaninC0(pObj), Abc_ObjFaninC1(pObj) );
    }
    // connect the objects
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        assert( (int)pObj->Id == i );
        // skip PIs and the constant
        if ( Abc_ObjFaninNum(pObj) == 0 )
            continue;
        // create the edge
        pFaninNew = Abc_NodeSeqToLogic( pNtkNew, Abc_ObjFanin0(pObj), Seq_NodeGetRing(pObj,0), Seq_ObjFaninL0(pObj) );
        Abc_ObjAddFanin( pObj->pCopy, pFaninNew );
        if ( Abc_ObjFaninNum(pObj) == 1 )
        {
            // create the complemented edge
            if ( Abc_ObjFaninC0(pObj) )
                Abc_ObjSetFaninC( pObj->pCopy, 0 );
            continue;
        }
        // create the edge
        pFaninNew = Abc_NodeSeqToLogic( pNtkNew, Abc_ObjFanin1(pObj), Seq_NodeGetRing(pObj,1), Seq_ObjFaninL1(pObj) );
        Abc_ObjAddFanin( pObj->pCopy, pFaninNew );
        // the complemented edges are subsumed by the node function
    }
    // add the latches and their names
    Abc_NtkAddDummyBoxNames( pNtkNew );
    Abc_NtkOrderCisCos( pNtkNew );
    // fix the problem with complemented and duplicated CO edges
    Abc_NtkLogicMakeSimpleCos( pNtkNew, 0 );
    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkSeqToLogicSop(): Network check has failed.\n" );
    return pNtkNew;
}
 

/**Function*************************************************************

  Synopsis    [Creates latches on one edge.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NodeSeqToLogic( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pFanin, Seq_Lat_t * pRing, int nLatches )
{
    Abc_Obj_t * pLatch;
    if ( nLatches == 0 )
    {
        assert( pFanin->pCopy );
        return pFanin->pCopy;
    }
    pFanin = Abc_NodeSeqToLogic( pNtkNew, pFanin, Seq_LatNext(pRing), nLatches - 1 );
    pLatch = Abc_NtkCreateLatch( pNtkNew );
    pLatch->pData = (void *)Seq_LatInit( pRing );
    Abc_ObjAddFanin( pLatch, pFanin );
    return pLatch;    
}

/**Function*************************************************************

  Synopsis    [Makes sure that every node in the table is in the network and vice versa.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Abc_NtkSeqCheck( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i, nFanins;
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        nFanins = Abc_ObjFaninNum(pObj);
        if ( nFanins == 0 )
        {
            if ( pObj != Abc_AigConst1(pNtk) )
            {
                printf( "Abc_SeqCheck: The AIG has non-standard constant nodes.\n" );
                return 0;
            }
            continue;
        }
        if ( nFanins == 1 )
        {
            printf( "Abc_SeqCheck: The AIG has single input nodes.\n" );
            return 0;
        }
        if ( nFanins > 2 )
        {
            printf( "Abc_SeqCheck: The AIG has non-standard nodes.\n" );
            return 0;
        }
    }
    // check the correctness of the internal representation of the initial states
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        nFanins = Abc_ObjFaninNum(pObj);
        if ( nFanins == 0 )
            continue;
        if ( nFanins == 1 )
        {
            if ( Seq_NodeCountLats(pObj, 0) != Seq_ObjFaninL0(pObj) )
            {
                printf( "Abc_SeqCheck: Node %d has mismatch in the number of latches.\n", Abc_ObjName(pObj) );
                return 0;
            }
        }
        // look at both inputs
        if ( Seq_NodeCountLats(pObj, 0) != Seq_ObjFaninL0(pObj) )
        {
            printf( "Abc_SeqCheck: The first fanin of node %d has mismatch in the number of latches.\n", Abc_ObjName(pObj) );
            return 0;
        }
        if ( Seq_NodeCountLats(pObj, 1) != Seq_ObjFaninL1(pObj) )
        {
            printf( "Abc_SeqCheck: The second fanin of node %d has mismatch in the number of latches.\n", Abc_ObjName(pObj) );
            return 0;
        }
    }
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


