/**CFile****************************************************************

  FileName    [seqRetCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Construction and manipulation of sequential AIGs.]

  Synopsis    [The core of FPGA mapping/retiming package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: seqRetCore.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "seqInt.h"
#include "dec.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Abc_Ntk_t * Seq_NtkRetimeDerive( Abc_Ntk_t * pNtk, int fVerbose );
static Abc_Obj_t * Seq_NodeRetimeDerive( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pNode, char * pSop, Vec_Ptr_t * vFanins );
static Abc_Ntk_t * Seq_NtkRetimeReconstruct( Abc_Ntk_t * pNtkOld, Abc_Ntk_t * pNtkSeq );
static Abc_Obj_t * Seq_EdgeReconstruct_rec( Abc_Obj_t * pGoal, Abc_Obj_t * pNode );
static Abc_Obj_t * Seq_EdgeReconstructPO( Abc_Obj_t * pNode );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs FPGA mapping and retiming.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Seq_NtkRetime( Abc_Ntk_t * pNtk, int nMaxIters, int fInitial, int fVerbose )
{
    Abc_Seq_t * p;
    Abc_Ntk_t * pNtkSeq, * pNtkNew;
    int RetValue;
    assert( !Abc_NtkHasAig(pNtk) );
    // derive the isomorphic seq AIG
    pNtkSeq = Seq_NtkRetimeDerive( pNtk, fVerbose );
    p = pNtkSeq->pManFunc;
    p->nMaxIters = nMaxIters;

    if ( !fInitial )
        Seq_NtkLatchSetValues( pNtkSeq, ABC_INIT_DC );
    // find the best mapping and retiming 
    if ( !Seq_NtkRetimeDelayLags( pNtk, pNtkSeq, fVerbose ) )
        return NULL;

    // implement the retiming
    RetValue = Seq_NtkImplementRetiming( pNtkSeq, p->vLags, fVerbose );
    if ( RetValue == 0 )
        printf( "Retiming completed but initial state computation has failed.\n" );
//return pNtkSeq;

    // create the final mapped network
    pNtkNew = Seq_NtkRetimeReconstruct( pNtk, pNtkSeq );
    Abc_NtkDelete( pNtkSeq );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Derives the isomorphic seq AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Seq_NtkRetimeDerive( Abc_Ntk_t * pNtk, int fVerbose )
{
    Abc_Seq_t * p;
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObj, * pFanin, * pMirror;
    Vec_Ptr_t * vMapAnds, * vMirrors;
    Vec_Vec_t * vMapFanins;
    int i, k, RetValue, fHasBdds;
    char * pSop;

    // make sure it is an AIG without self-feeding latches
    assert( !Abc_NtkHasAig(pNtk) );
    if ( RetValue = Abc_NtkRemoveSelfFeedLatches(pNtk) )
        printf( "Modified %d self-feeding latches. The result will not verify.\n", RetValue );
    assert( Abc_NtkCountSelfFeedLatches(pNtk) == 0 );

    // remove the dangling nodes
    Abc_NtkCleanup( pNtk, fVerbose );

    // transform logic functions from BDD to SOP
    if ( fHasBdds = Abc_NtkIsBddLogic(pNtk) )
    {
        if ( !Abc_NtkBddToSop(pNtk, 0) )
        {
            printf( "Seq_NtkRetimeDerive(): Converting to SOPs has failed.\n" );
            return NULL;
        }
    }

    // start the network
    pNtkNew = Abc_NtkAlloc( ABC_NTK_SEQ, ABC_FUNC_AIG, 1 );
    // duplicate the name and the spec
    pNtkNew->pName = Extra_UtilStrsav(pNtk->pName);
    pNtkNew->pSpec = Extra_UtilStrsav(pNtk->pSpec);

    // map the constant nodes
    Abc_NtkCleanCopy( pNtk );
    // clone the PIs/POs/latches
    Abc_NtkForEachPi( pNtk, pObj, i )
        Abc_NtkDupObj( pNtkNew, pObj, 0 );
    Abc_NtkForEachPo( pNtk, pObj, i )
        Abc_NtkDupObj( pNtkNew, pObj, 0 );
    // copy the names
    Abc_NtkForEachPi( pNtk, pObj, i )
        Abc_ObjAssignName( pObj->pCopy, Abc_ObjName(pObj), NULL );
    Abc_NtkForEachPo( pNtk, pObj, i )
        Abc_ObjAssignName( pObj->pCopy, Abc_ObjName(pObj), NULL );

    // create one AND for each logic node in the topological order
    vMapAnds = Abc_NtkDfs( pNtk, 0 );
    Vec_PtrForEachEntry( vMapAnds, pObj, i )
    {
        if ( pObj->Id == 0 )
        {
            pObj->pCopy = Abc_AigConst1(pNtkNew);
            continue;
        }
        pObj->pCopy = Abc_NtkCreateNode( pNtkNew );
    }

    // make the new seq AIG point to the old network through pNext
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( pObj->pCopy ) pObj->pCopy->pNext = pObj;

    // make latches point to the latch fanins
    Abc_NtkForEachLatch( pNtk, pObj, i )
    {
        assert( !Abc_ObjIsLatch(Abc_ObjFanin0(pObj)) );
        pObj->pCopy = Abc_ObjFanin0(pObj)->pCopy;
    }

    // create internal AND nodes w/o strashing for each logic node (including constants)
    vMapFanins = Vec_VecStart( Vec_PtrSize(vMapAnds) );
    Vec_PtrForEachEntry( vMapAnds, pObj, i )
    {
        // get the SOP of the node
        if ( Abc_NtkHasMapping(pNtk) )
            pSop = Mio_GateReadSop(pObj->pData);
        else
            pSop = pObj->pData;
        pFanin = Seq_NodeRetimeDerive( pNtkNew, pObj, pSop, Vec_VecEntry(vMapFanins, i) );
        Abc_ObjAddFanin( pObj->pCopy, pFanin );
        Abc_ObjAddFanin( pObj->pCopy, pFanin );
    }
    // connect the POs
    Abc_NtkForEachPo( pNtk, pObj, i )
        Abc_ObjAddFanin( pObj->pCopy, Abc_ObjFanin0(pObj)->pCopy );
    
    // start the storage for initial states
    p = pNtkNew->pManFunc;
    Seq_Resize( p, Abc_NtkObjNumMax(pNtkNew) );

    // add the sequential edges
    Vec_PtrForEachEntry( vMapAnds, pObj, i )
    {
        vMirrors = Vec_VecEntry( vMapFanins, i );
        Abc_ObjForEachFanin( pObj, pFanin, k )
        {
            pMirror = Vec_PtrEntry( vMirrors, k );
            if ( Abc_ObjIsLatch(pFanin) )
            {
                Seq_NodeInsertFirst( pMirror, 0, Abc_LatchInit(pFanin) );
                Seq_NodeInsertFirst( pMirror, 1, Abc_LatchInit(pFanin) );
            }
        }
    }
    // add the sequential edges to the POs
    Abc_NtkForEachPo( pNtk, pObj, i )
    {
        pFanin = Abc_ObjFanin0(pObj);
        if ( Abc_ObjIsLatch(pFanin) )
            Seq_NodeInsertFirst( pObj->pCopy, 0, Abc_LatchInit(pFanin) );
    }


    // save the fanin/delay info
    p->vMapAnds   = vMapAnds;
    p->vMapFanins = vMapFanins;
    p->vMapCuts   = Vec_VecStart( Vec_PtrSize(p->vMapAnds) );
    p->vMapDelays = Vec_VecStart( Vec_PtrSize(p->vMapAnds) );
    Vec_PtrForEachEntry( p->vMapAnds, pObj, i )
    {
        // change the node to be the new one
        Vec_PtrWriteEntry( p->vMapAnds, i, pObj->pCopy );
        // collect the new fanins of this node
        Abc_ObjForEachFanin( pObj, pFanin, k )
            Vec_VecPush( p->vMapCuts, i, (void *)( (pFanin->pCopy->Id << 8) | Abc_ObjIsLatch(pFanin) ) );
        // collect the delay info
        if ( !Abc_NtkHasMapping(pNtk) )
        {
            Abc_ObjForEachFanin( pObj, pFanin, k )
                Vec_VecPush( p->vMapDelays, i, (void *)Abc_Float2Int(1.0) );
        }
        else
        {
            Mio_Pin_t * pPin = Mio_GateReadPins(pObj->pData);
            float Max, tDelayBlockRise, tDelayBlockFall;
            Abc_ObjForEachFanin( pObj, pFanin, k )
            {
                tDelayBlockRise = (float)Mio_PinReadDelayBlockRise( pPin );  
                tDelayBlockFall = (float)Mio_PinReadDelayBlockFall( pPin ); 
                Max = ABC_MAX( tDelayBlockRise, tDelayBlockFall );
                Vec_VecPush( p->vMapDelays, i, (void *)Abc_Float2Int(Max) );
                pPin = Mio_PinReadNext(pPin);
            }
        }
    }

    // set the cutset composed of latch drivers
//    Abc_NtkAigCutsetCopy( pNtk );
//    Seq_NtkLatchGetEqualFaninNum( pNtkNew );

    // convert the network back into BDDs if this is how it was
    if ( fHasBdds )
        Abc_NtkSopToBdd(pNtk);

    // copy EXDC and check correctness
    if ( pNtk->pExdc )
        fprintf( stdout, "Warning: EXDC is not copied when converting to sequential AIG.\n" );
    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Seq_NtkRetimeDerive(): Network check has failed.\n" );
    return pNtkNew;
}


/**Function*************************************************************

  Synopsis    [Strashes one logic node using its SOP.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Seq_NodeRetimeDerive( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pRoot, char * pSop, Vec_Ptr_t * vFanins )
{
    Dec_Graph_t * pFForm;
    Dec_Node_t * pNode;
    Abc_Obj_t * pResult, * pFanin, * pMirror;
    int i, nFanins;

    // get the number of node's fanins
    nFanins = Abc_ObjFaninNum( pRoot );
    assert( nFanins == Abc_SopGetVarNum(pSop) );
    if ( nFanins < 2 )
    {
        if ( Abc_SopIsConst1(pSop) )
            pFanin = Abc_AigConst1(pNtkNew);
        else if ( Abc_SopIsConst0(pSop) )
            pFanin = Abc_ObjNot( Abc_AigConst1(pNtkNew) );
        else if ( Abc_SopIsBuf(pSop) )
            pFanin = Abc_ObjFanin0(pRoot)->pCopy;
        else if ( Abc_SopIsInv(pSop) )
            pFanin = Abc_ObjNot( Abc_ObjFanin0(pRoot)->pCopy );
        else
            assert( 0 );
        // create the node with these fanins
        pMirror = Abc_NtkCreateNode( pNtkNew );
        Abc_ObjAddFanin( pMirror, pFanin );
        Abc_ObjAddFanin( pMirror, pFanin );
        Vec_PtrPush( vFanins, pMirror );
        return pMirror;
    }

    // perform factoring
    pFForm = Dec_Factor( pSop );
    // collect the fanins
    Dec_GraphForEachLeaf( pFForm, pNode, i )
    {
        pFanin = Abc_ObjFanin(pRoot,i)->pCopy;
        pMirror = Abc_NtkCreateNode( pNtkNew );
        Abc_ObjAddFanin( pMirror, pFanin );
        Abc_ObjAddFanin( pMirror, pFanin );
        Vec_PtrPush( vFanins, pMirror );
        pNode->pFunc = pMirror;
    }
    // perform strashing
    pResult = Dec_GraphToNetworkNoStrash( pNtkNew, pFForm );
    Dec_GraphFree( pFForm );
    return pResult;
}


/**Function*************************************************************

  Synopsis    [Reconstructs the network after retiming.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Seq_NtkRetimeReconstruct( Abc_Ntk_t * pNtkOld, Abc_Ntk_t * pNtkSeq )
{
    Abc_Seq_t * p = pNtkSeq->pManFunc;
    Seq_Lat_t * pRing0, * pRing1;
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObj, * pFanin, * pFaninNew, * pMirror;
    Vec_Ptr_t * vMirrors;
    int i, k;

    assert( !Abc_NtkIsSeq(pNtkOld) );
    assert( Abc_NtkIsSeq(pNtkSeq) );

    // transfer the pointers pNtkOld->pNtkSeq from pCopy to pNext
    Abc_NtkForEachObj( pNtkOld, pObj, i )
        pObj->pNext = pObj->pCopy;

    // start the final network
    pNtkNew = Abc_NtkStartFrom( pNtkSeq, pNtkOld->ntkType, pNtkOld->ntkFunc );

    // transfer the pointers to the old network
    if ( Abc_AigConst1(pNtkOld) ) 
        Abc_AigConst1(pNtkOld)->pCopy = Abc_AigConst1(pNtkNew);
    Abc_NtkForEachPi( pNtkOld, pObj, i )
        pObj->pCopy = pObj->pNext->pCopy;
    Abc_NtkForEachPo( pNtkOld, pObj, i )
        pObj->pCopy = pObj->pNext->pCopy;

    // copy the internal nodes of the old network into the new network
    // transfer the pointers pNktOld->pNtkNew to pNtkSeq->pNtkNew
    Abc_NtkForEachNode( pNtkOld, pObj, i )
    {
        if ( i == 0 ) continue;
        Abc_NtkDupObj( pNtkNew, pObj, 0 );
        pObj->pNext->pCopy = pObj->pCopy;
    }
    Abc_NtkForEachLatch( pNtkOld, pObj, i )
        pObj->pCopy = Abc_ObjFanin0(pObj)->pCopy;

    // share the latches
    Seq_NtkShareLatches( pNtkNew, pNtkSeq );

    // connect the objects
//    Abc_NtkForEachNode( pNtkOld, pObj, i )
    Vec_PtrForEachEntry( p->vMapAnds, pObj, i )
    {
        // pObj is from pNtkSeq - transform to pNtkOld
        pObj = pObj->pNext;
        // iterate through the fanins of this node in the old network
        vMirrors = Vec_VecEntry( p->vMapFanins, i );
        Abc_ObjForEachFanin( pObj, pFanin, k )
        {
            pMirror = Vec_PtrEntry( vMirrors, k );
            assert( Seq_ObjFaninL0(pMirror) == Seq_ObjFaninL1(pMirror) );
            pRing0 = Seq_NodeGetRing( pMirror, 0 );
            pRing1 = Seq_NodeGetRing( pMirror, 1 );
            if ( pRing0 == NULL )
            {
                Abc_ObjAddFanin( pObj->pCopy, pFanin->pCopy );
                continue;
            }
//            assert( pRing0->pLatch == pRing1->pLatch );
            if ( pRing0->pLatch->pData > pRing1->pLatch->pData )
                Abc_ObjAddFanin( pObj->pCopy, pRing0->pLatch );
            else
                Abc_ObjAddFanin( pObj->pCopy, pRing1->pLatch );
        }
    }

    // connect the POs
    Abc_NtkForEachPo( pNtkOld, pObj, i )
    {
        pFanin = Abc_ObjFanin0(pObj);
        pRing0 = Seq_NodeGetRing( Abc_NtkPo(pNtkSeq, i), 0 );
        if ( pRing0 )
            pFaninNew = pRing0->pLatch;
        else 
            pFaninNew = pFanin->pCopy;
        assert( pFaninNew != NULL );
        Abc_ObjAddFanin( pObj->pCopy, pFaninNew );
    }

    // clean the result of latch sharing
    Seq_NtkShareLatchesClean( pNtkSeq );

    // add the latches and their names
    Abc_NtkAddDummyBoxNames( pNtkNew );
    Abc_NtkOrderCisCos( pNtkNew );
    // fix the problem with complemented and duplicated CO edges
    Abc_NtkLogicMakeSimpleCos( pNtkNew, 1 );
    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Seq_NtkRetimeReconstruct(): Network check has failed.\n" );
    return pNtkNew;

}

/**Function*************************************************************

  Synopsis    [Reconstructs the network after retiming.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Seq_EdgeReconstruct_rec( Abc_Obj_t * pGoal, Abc_Obj_t * pNode )
{
    Seq_Lat_t * pRing;
    Abc_Obj_t * pFanin, * pRes = NULL;

    if ( !Abc_AigNodeIsAnd(pNode) )
        return NULL;

    // consider the first fanin
    pFanin = Abc_ObjFanin0(pNode);
    if ( pFanin->pCopy == NULL ) // internal node
        pRes = Seq_EdgeReconstruct_rec( pGoal, pFanin );
    else if ( pFanin == pGoal )
    {
        if ( pRing = Seq_NodeGetRing( pNode, 0 ) )
            pRes = pRing->pLatch;
        else
            pRes = pFanin->pCopy;
    }
    if ( pRes != NULL )
        return pRes;

    // consider the second fanin
    pFanin = Abc_ObjFanin1(pNode);
    if ( pFanin->pCopy == NULL ) // internal node
        pRes = Seq_EdgeReconstruct_rec( pGoal, pFanin );
    else if ( pFanin == pGoal )
    {
        if ( pRing = Seq_NodeGetRing( pNode, 1 ) )
            pRes = pRing->pLatch;
        else
            pRes = pFanin->pCopy;
    }
    return pRes;
}

/**Function*************************************************************

  Synopsis    [Reconstructs the network after retiming.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Seq_EdgeReconstructPO( Abc_Obj_t * pNode )
{
    Seq_Lat_t * pRing;
    assert( Abc_ObjIsPo(pNode) );
    if ( pRing = Seq_NodeGetRing( pNode, 0 ) )
        return pRing->pLatch;
    else
        return Abc_ObjFanin0(pNode)->pCopy;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


