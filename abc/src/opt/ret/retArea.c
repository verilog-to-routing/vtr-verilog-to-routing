/**CFile****************************************************************

  FileName    [retArea.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Retiming package.]

  Synopsis    [Min-area retiming.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - Oct 31, 2006.]

  Revision    [$Id: retArea.c,v 1.00 2006/10/31 00:00:00 alanmi Exp $]

***********************************************************************/

#include "retInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Abc_Ntk_t * Abc_NtkRetimeMinAreaOne( Abc_Ntk_t * pNtk, int fForward, int fUseOldNames, int fVerbose );
static void        Abc_NtkRetimeMinAreaPrepare( Abc_Ntk_t * pNtk, int fForward );
static void        Abc_NtkRetimeMinAreaInitValues( Abc_Ntk_t * pNtk, Vec_Ptr_t * vMinCut );
static Abc_Ntk_t * Abc_NtkRetimeMinAreaConstructNtk( Abc_Ntk_t * pNtk, Vec_Ptr_t * vMinCut );
static void        Abc_NtkRetimeMinAreaUpdateLatches( Abc_Ntk_t * pNtk, Vec_Ptr_t * vMinCut, int fForward, int fUseOldNames );

extern Abc_Ntk_t * Abc_NtkAttachBottom( Abc_Ntk_t * pNtkTop, Abc_Ntk_t * pNtkBottom );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs min-area retiming.]

  Description [Returns the number of latches reduced.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRetimeMinArea( Abc_Ntk_t * pNtk, int fForwardOnly, int fBackwardOnly, int fUseOldNames, int fVerbose )
{
    Abc_Ntk_t * pNtkTotal = NULL, * pNtkBottom;
    Vec_Int_t * vValuesNew = NULL, * vValues;
    int nLatches = Abc_NtkLatchNum(pNtk);
    int fOneFrame = 0;
    assert( !fForwardOnly || !fBackwardOnly );
    // there should not be black boxes
    assert( Abc_NtkIsSopLogic(pNtk) );
    assert( Abc_NtkLatchNum(pNtk) == Vec_PtrSize(pNtk->vBoxes) );
    // reorder CI/CO/latch inputs
    Abc_NtkOrderCisCos( pNtk );
    // perform forward retiming
    if ( !fBackwardOnly )
    {
        if ( fOneFrame )
            Abc_NtkRetimeMinAreaOne( pNtk, 1, fUseOldNames, fVerbose );
        else
            while ( Abc_NtkRetimeMinAreaOne( pNtk, 1, fUseOldNames, fVerbose ) );
    }
    // remember initial values
    vValues = Abc_NtkCollectLatchValues( pNtk );
    // perform backward retiming
    if ( !fForwardOnly )
    {
        if ( fOneFrame )
            pNtkTotal = Abc_NtkRetimeMinAreaOne( pNtk, 0, fUseOldNames, fVerbose );
        else
            while ( (pNtkBottom = Abc_NtkRetimeMinAreaOne( pNtk, 0, fUseOldNames, fVerbose )) )
                pNtkTotal = Abc_NtkAttachBottom( pNtkTotal, pNtkBottom );  
    }
    // compute initial values
    vValuesNew = Abc_NtkRetimeInitialValues( pNtkTotal, vValues, fVerbose );
    if ( pNtkTotal ) Abc_NtkDelete( pNtkTotal );
    // insert new initial values
    Abc_NtkInsertLatchValues( pNtk, vValuesNew );
    if ( vValuesNew ) Vec_IntFree( vValuesNew );
    if ( vValues )    Vec_IntFree( vValues );
    // fix the COs (this changes the circuit structure)
//    Abc_NtkLogicMakeSimpleCos( pNtk, 0 );
    // check for correctness
    if ( !Abc_NtkCheck( pNtk ) )
        fprintf( stdout, "Abc_NtkRetimeMinArea(): Network check has failed.\n" );
    // return the number of latches saved
    return nLatches - Abc_NtkLatchNum(pNtk);
}

/**Function*************************************************************

  Synopsis    [Performs min-area retiming backward.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkRetimeMinAreaOne( Abc_Ntk_t * pNtk, int fForward, int fUseOldNames, int fVerbose )
{ 
    Abc_Ntk_t * pNtkNew = NULL;
    Vec_Ptr_t * vMinCut;
    // mark current latches and TFI(POs)
    Abc_NtkRetimeMinAreaPrepare( pNtk, fForward );
    // run the maximum forward flow
    vMinCut = Abc_NtkMaxFlow( pNtk, fForward, fVerbose );
//    assert( Vec_PtrSize(vMinCut) <= Abc_NtkLatchNum(pNtk) );
    // create new latch boundary if there is improvement
    if ( Vec_PtrSize(vMinCut) < Abc_NtkLatchNum(pNtk) )
    {
        pNtkNew = (Abc_Ntk_t *)1;
        if ( fForward )
            Abc_NtkRetimeMinAreaInitValues( pNtk, vMinCut );
        else
            pNtkNew = Abc_NtkRetimeMinAreaConstructNtk( pNtk, vMinCut );
        Abc_NtkRetimeMinAreaUpdateLatches( pNtk, vMinCut, fForward, fUseOldNames );
    }
    // clean up
    Vec_PtrFree( vMinCut );
    Abc_NtkCleanMarkA( pNtk );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Marks the cone with MarkA.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkMarkCone_rec( Abc_Obj_t * pObj, int fForward )
{
    Abc_Obj_t * pNext;
    int i;
    if ( pObj->fMarkA )
        return;
    pObj->fMarkA = 1;
    if ( fForward )
    {
        Abc_ObjForEachFanout( pObj, pNext, i )
            Abc_NtkMarkCone_rec( pNext, fForward );
    }
    else
    {
        Abc_ObjForEachFanin( pObj, pNext, i )
            Abc_NtkMarkCone_rec( pNext, fForward );
    }
}

/**Function*************************************************************

  Synopsis    [Marks the cone with MarkA.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkUnmarkCone_rec( Abc_Obj_t * pObj, int fForward )
{
    Abc_Obj_t * pNext;
    int i;
    if ( !pObj->fMarkA || Abc_ObjIsLatch(pObj) )
        return;
    pObj->fMarkA = 0;
    if ( fForward )
    {
        Abc_ObjForEachFanout( pObj, pNext, i )
            Abc_NtkUnmarkCone_rec( pNext, fForward );
    }
    else
    {
        Abc_ObjForEachFanin( pObj, pNext, i )
            Abc_NtkUnmarkCone_rec( pNext, fForward );
    }
}

/**Function*************************************************************

  Synopsis    [Prepares the network for running MaxFlow.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkRetimeMinAreaPrepare( Abc_Ntk_t * pNtk, int fForward )
{
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj, * pFanin;
    int i, k;
    if ( fForward )
    {
        // mark the frontier
        Abc_NtkForEachPo( pNtk, pObj, i )
            pObj->fMarkA = 1;
        Abc_NtkForEachLatch( pNtk, pObj, i )
        {
            pObj->fMarkA = 1;
            Abc_ObjFanin0(pObj)->fMarkA = 1;
        }
        // mark the nodes reachable from the PIs
        Abc_NtkForEachPi( pNtk, pObj, i )
            Abc_NtkMarkCone_rec( pObj, fForward );
        // collect the unmarked fanins of the marked nodes
        vNodes = Vec_PtrAlloc( 100 );
        Abc_NtkForEachObj( pNtk, pObj, i )
            if ( pObj->fMarkA )
                Abc_ObjForEachFanin( pObj, pFanin, k )
                    if ( !pFanin->fMarkA )
                        Vec_PtrPush( vNodes, pFanin );
        // mark these nodes
        Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
            pObj->fMarkA = 1;
        Vec_PtrFree( vNodes );
    }
    else
    {
        // mark the frontier
        Abc_NtkForEachPi( pNtk, pObj, i )
            pObj->fMarkA = 1;
        Abc_NtkForEachLatch( pNtk, pObj, i )
        {
            pObj->fMarkA = 1;
            Abc_ObjFanout0(pObj)->fMarkA = 1;
        }
        // mark the nodes reachable from the POs
        Abc_NtkForEachPo( pNtk, pObj, i )
            Abc_NtkMarkCone_rec( pObj, fForward );
    }
}

/**Function*************************************************************

  Synopsis    [Compute initial values.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRetimeMinAreaInitValues_rec( Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFanin;
    int i;
    // skip visited nodes
    if ( Abc_NodeIsTravIdCurrent(pObj) )
        return (int)(ABC_PTRUINT_T)pObj->pCopy;
    Abc_NodeSetTravIdCurrent(pObj);
    // consider the case of a latch output
    if ( Abc_ObjIsBo(pObj) )
    {
        assert( Abc_ObjIsLatch(Abc_ObjFanin0(pObj)) );
        pObj->pCopy = (Abc_Obj_t *)(ABC_PTRUINT_T)Abc_NtkRetimeMinAreaInitValues_rec( Abc_ObjFanin0(pObj) );
        return (int)(ABC_PTRUINT_T)pObj->pCopy;
    }
    assert( Abc_ObjIsNode(pObj) );
    // visit the fanins
    Abc_ObjForEachFanin( pObj, pFanin, i )
        Abc_NtkRetimeMinAreaInitValues_rec( pFanin );
    // compute the value of the node
    pObj->pCopy = (Abc_Obj_t *)(ABC_PTRUINT_T)Abc_ObjSopSimulate(pObj);
    return (int)(ABC_PTRUINT_T)pObj->pCopy;
}

/**Function*************************************************************

  Synopsis    [Compute initial values.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkRetimeMinAreaInitValues( Abc_Ntk_t * pNtk, Vec_Ptr_t * vMinCut )
{
    Abc_Obj_t * pObj;
    int i;
    // transfer initial values to pCopy and mark the latches
    Abc_NtkIncrementTravId(pNtk);
    Abc_NtkForEachLatch( pNtk, pObj, i )
    {
        pObj->pCopy = (Abc_Obj_t *)(ABC_PTRUINT_T)Abc_LatchIsInit1(pObj);
        Abc_NodeSetTravIdCurrent( pObj );
    }
    // propagate initial values
    Vec_PtrForEachEntry( Abc_Obj_t *, vMinCut, pObj, i )
        Abc_NtkRetimeMinAreaInitValues_rec( pObj );
    // unmark the latches
    Abc_NtkForEachLatch( pNtk, pObj, i )
        Abc_NodeSetTravIdPrevious( pObj );
}

/**Function*************************************************************

  Synopsis    [Performs min-area retiming backward.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkRetimeMinAreaConstructNtk_rec( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFanin;
    int i;
    // skip visited nodes
    if ( Abc_NodeIsTravIdCurrent(pObj) )
        return pObj->pCopy;
    Abc_NodeSetTravIdCurrent(pObj);
    // consider the case of a latch output
    if ( Abc_ObjIsBi(pObj) )
    {
        pObj->pCopy = Abc_NtkRetimeMinAreaConstructNtk_rec( pNtkNew, Abc_ObjFanin0(pObj) );
        return pObj->pCopy;
    }
    assert( Abc_ObjIsNode(pObj) );
    // visit the fanins
    Abc_ObjForEachFanin( pObj, pFanin, i )
        Abc_NtkRetimeMinAreaConstructNtk_rec( pNtkNew, pFanin );
    // compute the value of the node
    Abc_NtkDupObj( pNtkNew, pObj, 0 );
    Abc_ObjForEachFanin( pObj, pFanin, i )
        Abc_ObjAddFanin( pObj->pCopy, pFanin->pCopy );
    return pObj->pCopy;
}

/**Function*************************************************************

  Synopsis    [Creates the network from computing initial state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkRetimeMinAreaConstructNtk( Abc_Ntk_t * pNtk, Vec_Ptr_t * vMinCut )
{
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObj, * pObjNew;
    int i;
    // create new network
    pNtkNew = Abc_NtkAlloc( ABC_NTK_LOGIC, ABC_FUNC_SOP, 1 );
    // map new latches into PIs of the new network
    Abc_NtkIncrementTravId(pNtk);
    Vec_PtrForEachEntry( Abc_Obj_t *, vMinCut, pObj, i )
    {
        pObj->pCopy = Abc_NtkCreatePi(pNtkNew);
        Abc_NodeSetTravIdCurrent( pObj );
    }
    // construct the network recursively
    Abc_NtkForEachLatch( pNtk, pObj, i )
    {
        pObjNew = Abc_NtkRetimeMinAreaConstructNtk_rec( pNtkNew, Abc_ObjFanin0(pObj) );
        Abc_ObjAddFanin( Abc_NtkCreatePo(pNtkNew), pObjNew );
    }
    // unmark the nodes in the cut
    Vec_PtrForEachEntry( Abc_Obj_t *, vMinCut, pObj, i )
        Abc_NodeSetTravIdPrevious( pObj );
    // unmark the latches
    Abc_NtkForEachLatch( pNtk, pObj, i )
        Abc_NodeSetTravIdPrevious( pObj );
    // assign dummy node names
    Abc_NtkAddDummyPiNames( pNtkNew );
    Abc_NtkAddDummyPoNames( pNtkNew );
    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkRetimeMinAreaConstructNtk(): Network check has failed.\n" );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Updates the network after backward retiming.]

  Description [Assumes that fMarkA denotes all nodes reachabe from
  the latches toward the cut.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkRetimeMinAreaUpdateLatches( Abc_Ntk_t * pNtk, Vec_Ptr_t * vMinCut, int fForward, int fUseOldNames )
{
    Vec_Ptr_t * vCis, * vCos, * vBoxes, * vBoxesNew, * vNodes, * vBuffers;
    Abc_Obj_t * pObj, * pLatch, * pLatchIn, * pLatchOut, * pNext, * pBuffer;
    int i, k;
    // create new latches
    Vec_PtrShrink( pNtk->vCis, Abc_NtkCiNum(pNtk) - Abc_NtkLatchNum(pNtk) );
    Vec_PtrShrink( pNtk->vCos, Abc_NtkCoNum(pNtk) - Abc_NtkLatchNum(pNtk) );
    vCis   = pNtk->vCis;   pNtk->vCis   = NULL;  
    vCos   = pNtk->vCos;   pNtk->vCos   = NULL;  
    vBoxes = pNtk->vBoxes; pNtk->vBoxes = NULL; 
    // transfer boxes
    vBoxesNew = Vec_PtrAlloc(100);
    Vec_PtrForEachEntry( Abc_Obj_t *, vBoxes, pObj, i )
        if ( !Abc_ObjIsLatch(pObj) )
            Vec_PtrPush( vBoxesNew, pObj );
    // create or reuse latches
    vNodes = Vec_PtrAlloc( 100 );
    vBuffers = Vec_PtrAlloc( 100 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vMinCut, pObj, i )
    {
        if ( Abc_ObjIsCi(pObj) && fForward )
        {
            pLatchOut = pObj;
            pLatch    = Abc_ObjFanin0(pLatchOut);
            pLatchIn  = Abc_ObjFanin0(pLatch);
            assert( Abc_ObjIsBo(pLatchOut) && Abc_ObjIsLatch(pLatch) && Abc_ObjIsBi(pLatchIn) );
            // mark the latch as reused
            Abc_NodeSetTravIdCurrent( pLatch );

            // check if there are marked fanouts
            // (these are fanouts to be connected to the latch input)
            Abc_ObjForEachFanout( pObj, pNext, k )
                if ( pNext->fMarkA )
                    break;
            if ( k < Abc_ObjFanoutNum(pObj) )
            {
                // add the buffer
                pBuffer = Abc_NtkCreateNodeBuf( pNtk, Abc_ObjFanin0(pLatchIn) );
                Abc_ObjAssignName( pBuffer, Abc_ObjName(pObj), "_buf" );
                Abc_ObjPatchFanin( pLatchIn, Abc_ObjFanin0(pLatchIn), pBuffer );
                Vec_PtrPush( vBuffers, pBuffer );
                // redirect edges to the unvisited fanouts of the node
                Abc_NodeCollectFanouts( pObj, vNodes );
                Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNext, k )
                    if ( pNext->fMarkA )
                        Abc_ObjPatchFanin( pNext, pObj, pBuffer );
            }
            assert( Abc_ObjFanoutNum(pObj) > 0 );
//            if ( Abc_ObjFanoutNum(pObj) == 0 )
//                Abc_NtkDeleteObj_rec( pObj, 0 );
        }
        else if ( Abc_ObjIsCo(pObj) && !fForward )
        {
            pLatchIn  = pObj;
            pLatch    = Abc_ObjFanout0(pLatchIn);
            pLatchOut = Abc_ObjFanout0(pLatch);
            assert( Abc_ObjIsBo(pLatchOut) && Abc_ObjIsLatch(pLatch) && Abc_ObjIsBi(pLatchIn) );
            // mark the latch as reused
            Abc_NodeSetTravIdCurrent( pLatch );
            assert( !Abc_ObjFanin0(pLatchIn)->fMarkA );
        }
        else
        {
            pLatchOut = Abc_NtkCreateBo(pNtk);
            pLatch    = Abc_NtkCreateLatch(pNtk);
            pLatchIn  = Abc_NtkCreateBi(pNtk);

            if ( fUseOldNames )
            {
                Abc_ObjAssignName( pLatchOut, Abc_ObjName(pLatch), "_out" );
                Abc_ObjAssignName( pLatchIn,  Abc_ObjName(pLatch), "_in" );
            }
            else
            {
                Abc_ObjAssignName( pLatchOut, Abc_ObjName(pObj), "_o1" );
                Abc_ObjAssignName( pLatchIn,  Abc_ObjName(pObj), "_i1" );
            }
            // connect
            Abc_ObjAddFanin( pLatchOut, pLatch );
            Abc_ObjAddFanin( pLatch, pLatchIn );
            if ( fForward )
            {
                pLatch->pData = (void *)(ABC_PTRUINT_T)(pObj->pCopy? ABC_INIT_ONE : ABC_INIT_ZERO);
                // redirect edges to the unvisited fanouts of the node
                Abc_NodeCollectFanouts( pObj, vNodes );
                Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNext, k )
                    if ( !pNext->fMarkA )
                        Abc_ObjPatchFanin( pNext, pObj, pLatchOut );
            }
            else
            {
                // redirect edges to the visited fanouts of the node
                Abc_NodeCollectFanouts( pObj, vNodes );
                Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNext, k )
                    if ( pNext->fMarkA )
                        Abc_ObjPatchFanin( pNext, pObj, pLatchOut );
            }
            // connect latch to the node
            Abc_ObjAddFanin( pLatchIn, pObj );
        }
        Vec_PtrPush( vCis, pLatchOut );
        Vec_PtrPush( vBoxesNew, pLatch );
        Vec_PtrPush( vCos, pLatchIn );
    }
    Vec_PtrFree( vNodes );
    // remove buffers
    Vec_PtrForEachEntry( Abc_Obj_t *, vBuffers, pObj, i )
    {
        Abc_ObjTransferFanout( pObj, Abc_ObjFanin0(pObj) );
        Abc_NtkDeleteObj( pObj );
    }
    Vec_PtrFree( vBuffers );
    // remove useless latches
    Vec_PtrForEachEntry( Abc_Obj_t *, vBoxes, pObj, i )
    {
        if ( !Abc_ObjIsLatch(pObj) )
            continue;
        if ( Abc_NodeIsTravIdCurrent(pObj) )
            continue;
        pLatchOut = Abc_ObjFanout0(pObj);
        pLatch    = pObj;
        pLatchIn  = Abc_ObjFanin0(pObj);
        if ( Abc_ObjFanoutNum(pLatchOut) > 0 )
            Abc_ObjTransferFanout( pLatchOut, Abc_ObjFanin0(pLatchIn) );
        Abc_NtkDeleteObj( pLatchOut );
        Abc_NtkDeleteObj( pObj );
        Abc_NtkDeleteObj( pLatchIn );
    }
    // set the arrays
    pNtk->vCis = vCis;
    pNtk->vCos = vCos;
    pNtk->vBoxes = vBoxesNew;
    Vec_PtrFree( vBoxes );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

