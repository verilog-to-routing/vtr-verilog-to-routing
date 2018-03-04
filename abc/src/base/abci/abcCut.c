/**CFile****************************************************************

  FileName    [abcCut.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Interface to cut computation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcCut.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "opt/cut/cut.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Abc_NtkPrintCuts( void * p, Abc_Ntk_t * pNtk, int fSeq );
static void Abc_NtkPrintCuts_( void * p, Abc_Ntk_t * pNtk, int fSeq );

extern int nTotal, nGood, nEqual;

static Vec_Int_t * Abc_NtkGetNodeAttributes( Abc_Ntk_t * pNtk );
static int Abc_NtkComputeArea( Abc_Ntk_t * pNtk, Cut_Man_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkCutsSubtractFanunt( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj, * pFan0, * pFan1, * pFanC;
    int i, Counter = 0;
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        if ( !Abc_NodeIsMuxType(pObj) )
            continue;
        pFanC = Abc_NodeRecognizeMux( pObj, &pFan1, &pFan0 );
        pFanC = Abc_ObjRegular(pFanC);
        pFan0 = Abc_ObjRegular(pFan0);
        assert( pFanC->vFanouts.nSize > 1 );
        pFanC->vFanouts.nSize--;
        Counter++;
        if ( Abc_NodeIsExorType(pObj) )
        {
            assert( pFan0->vFanouts.nSize > 1 );
            pFan0->vFanouts.nSize--;
            Counter++;
        }
    }
    printf("Substracted %d fanouts\n", Counter );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkCutsAddFanunt( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj, * pFan0, * pFan1, * pFanC;
    int i, Counter = 0;
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        if ( !Abc_NodeIsMuxType(pObj) )
            continue;
        pFanC = Abc_NodeRecognizeMux( pObj, &pFan1, &pFan0 );
        pFanC = Abc_ObjRegular(pFanC);
        pFan0 = Abc_ObjRegular(pFan0);
        pFanC->vFanouts.nSize++;
        Counter++;
        if ( Abc_NodeIsExorType(pObj) )
        {
            pFan0->vFanouts.nSize++;
            Counter++;
        }
    }
    printf("Added %d fanouts\n", Counter );
}

/**Function*************************************************************

  Synopsis    [Computes the cuts for the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cut_Man_t * Abc_NtkCuts( Abc_Ntk_t * pNtk, Cut_Params_t * pParams )
{
    ProgressBar * pProgress;
    Cut_Man_t * p;
    Cut_Cut_t * pList;
    Abc_Obj_t * pObj, * pNode;
    Vec_Ptr_t * vNodes;
    Vec_Int_t * vChoices;
    int i;
    abctime clk = Abc_Clock();

    extern void Abc_NtkBalanceAttach( Abc_Ntk_t * pNtk );
    extern void Abc_NtkBalanceDetach( Abc_Ntk_t * pNtk );

    if ( pParams->fAdjust )
    Abc_NtkCutsSubtractFanunt( pNtk );

    nTotal = nGood = nEqual = 0;

    assert( Abc_NtkIsStrash(pNtk) );
    // start the manager
    pParams->nIdsMax = Abc_NtkObjNumMax( pNtk );
    p = Cut_ManStart( pParams );
    // compute node attributes if local or global cuts are requested
    if ( pParams->fGlobal || pParams->fLocal )
    {
        extern Vec_Int_t * Abc_NtkGetNodeAttributes( Abc_Ntk_t * pNtk );
        Cut_ManSetNodeAttrs( p, Abc_NtkGetNodeAttributes(pNtk) );
    }
    // prepare for cut dropping
    if ( pParams->fDrop )
        Cut_ManSetFanoutCounts( p, Abc_NtkFanoutCounts(pNtk) );
    // set cuts for PIs
    Abc_NtkForEachCi( pNtk, pObj, i )
        if ( Abc_ObjFanoutNum(pObj) > 0 )
            Cut_NodeSetTriv( p, pObj->Id );
    // compute cuts for internal nodes
    vNodes = Abc_AigDfs( pNtk, 0, 1 ); // collects POs
    vChoices = Vec_IntAlloc( 100 );
    pProgress = Extra_ProgressBarStart( stdout, Vec_PtrSize(vNodes) );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
    {
        // when we reached a CO, it is time to deallocate the cuts
        if ( Abc_ObjIsCo(pObj) )
        {
            if ( pParams->fDrop )
                Cut_NodeTryDroppingCuts( p, Abc_ObjFaninId0(pObj) );
            continue;
        }
        // skip constant node, it has no cuts
//        if ( Abc_NodeIsConst(pObj) )
//            continue;
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        // compute the cuts to the internal node
        pList = (Cut_Cut_t *)Abc_NodeGetCuts( p, pObj, pParams->fDag, pParams->fTree );
        if ( pParams->fNpnSave && pList )
        {
            extern void Npn_ManSaveOne( unsigned * puTruth, int nVars );
            Cut_Cut_t * pCut;
            for ( pCut = pList; pCut; pCut = pCut->pNext )
                if ( pCut->nLeaves >= 4 )
                    Npn_ManSaveOne( Cut_CutReadTruth(pCut), pCut->nLeaves );
        }
        // consider dropping the fanins cuts
        if ( pParams->fDrop )
        {
            Cut_NodeTryDroppingCuts( p, Abc_ObjFaninId0(pObj) );
            Cut_NodeTryDroppingCuts( p, Abc_ObjFaninId1(pObj) );
        }
        // add cuts due to choices
        if ( Abc_AigNodeIsChoice(pObj) )
        {
            Vec_IntClear( vChoices );
            for ( pNode = pObj; pNode; pNode = (Abc_Obj_t *)pNode->pData )
                Vec_IntPush( vChoices, pNode->Id );
            Cut_NodeUnionCuts( p, vChoices );
        }
    }
    Extra_ProgressBarStop( pProgress );
    Vec_PtrFree( vNodes );
    Vec_IntFree( vChoices );
    Cut_ManPrintStats( p );
ABC_PRT( "TOTAL", Abc_Clock() - clk );
//    printf( "Area = %d.\n", Abc_NtkComputeArea( pNtk, p ) );
//Abc_NtkPrintCuts( p, pNtk, 0 );
//    Cut_ManPrintStatsToFile( p, pNtk->pSpec, Abc_Clock() - clk );

    // temporary printout of stats
    if ( nTotal )
    printf( "Total cuts = %d. Good cuts = %d.  Ratio = %5.2f\n", nTotal, nGood, ((double)nGood)/nTotal );
    if ( pParams->fAdjust )
    Abc_NtkCutsAddFanunt( pNtk );
    return p;
}

/**Function*************************************************************

  Synopsis    [Cut computation using the oracle.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkCutsOracle( Abc_Ntk_t * pNtk, Cut_Oracle_t * p )
{
    Abc_Obj_t * pObj;
    Vec_Ptr_t * vNodes;
    int i; //, clk = Abc_Clock();
    int fDrop = Cut_OracleReadDrop(p);

    assert( Abc_NtkIsStrash(pNtk) );

    // prepare cut droppping
    if ( fDrop )
        Cut_OracleSetFanoutCounts( p, Abc_NtkFanoutCounts(pNtk) );

    // set cuts for PIs
    Abc_NtkForEachCi( pNtk, pObj, i )
        if ( Abc_ObjFanoutNum(pObj) > 0 )
            Cut_OracleNodeSetTriv( p, pObj->Id );

    // compute cuts for internal nodes
    vNodes = Abc_AigDfs( pNtk, 0, 1 ); // collects POs
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
    {
        // when we reached a CO, it is time to deallocate the cuts
        if ( Abc_ObjIsCo(pObj) )
        {
            if ( fDrop )
                Cut_OracleTryDroppingCuts( p, Abc_ObjFaninId0(pObj) );
            continue;
        }
        // skip constant node, it has no cuts
//        if ( Abc_NodeIsConst(pObj) )
//            continue;
        // compute the cuts to the internal node
        Cut_OracleComputeCuts( p, pObj->Id, Abc_ObjFaninId0(pObj), Abc_ObjFaninId1(pObj),  
                Abc_ObjFaninC0(pObj), Abc_ObjFaninC1(pObj) );
        // consider dropping the fanins cuts
        if ( fDrop )
        {
            Cut_OracleTryDroppingCuts( p, Abc_ObjFaninId0(pObj) );
            Cut_OracleTryDroppingCuts( p, Abc_ObjFaninId1(pObj) );
        }
    }
    Vec_PtrFree( vNodes );
//ABC_PRT( "Total", Abc_Clock() - clk );
//Abc_NtkPrintCuts_( p, pNtk, 0 );
}


/**Function*************************************************************

  Synopsis    [Computes the cuts for the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cut_Man_t * Abc_NtkSeqCuts( Abc_Ntk_t * pNtk, Cut_Params_t * pParams )
{
/*
    Cut_Man_t *  p;
    Abc_Obj_t * pObj, * pNode;
    int i, nIters, fStatus;
    Vec_Int_t * vChoices;
    abctime clk = Abc_Clock();

    assert( Abc_NtkIsSeq(pNtk) );
    assert( pParams->fSeq );
//    assert( Abc_NtkIsDfsOrdered(pNtk) );

    // start the manager
    pParams->nIdsMax = Abc_NtkObjNumMax( pNtk );
    pParams->nCutSet = Abc_NtkCutSetNodeNum( pNtk );
    p = Cut_ManStart( pParams );

    // set cuts for the constant node and the PIs
    pObj = Abc_AigConst1(pNtk);
    if ( Abc_ObjFanoutNum(pObj) > 0 )
        Cut_NodeSetTriv( p, pObj->Id );
    Abc_NtkForEachPi( pNtk, pObj, i )
    {
//printf( "Setting trivial cut %d.\n", pObj->Id );
        Cut_NodeSetTriv( p, pObj->Id );
    }
    // label the cutset nodes and set their number in the array
    // assign the elementary cuts to the cutset nodes
    Abc_SeqForEachCutsetNode( pNtk, pObj, i )
    {
        assert( pObj->fMarkC == 0 );
        pObj->fMarkC = 1;
        pObj->pCopy = (Abc_Obj_t *)i;
        Cut_NodeSetTriv( p, pObj->Id );
//printf( "Setting trivial cut %d.\n", pObj->Id );
    }

    // process the nodes
    vChoices = Vec_IntAlloc( 100 );
    for ( nIters = 0; nIters < 10; nIters++ )
    {
//printf( "ITERATION %d:\n", nIters );
        // compute the cuts for the internal nodes
        Abc_AigForEachAnd( pNtk, pObj, i )
        {
            Abc_NodeGetCutsSeq( p, pObj, nIters==0 );
            // add cuts due to choices
            if ( Abc_AigNodeIsChoice(pObj) )
            {
                Vec_IntClear( vChoices );
                for ( pNode = pObj; pNode; pNode = pNode->pData )
                    Vec_IntPush( vChoices, pNode->Id );
                Cut_NodeUnionCutsSeq( p, vChoices, (pObj->fMarkC ? (int)pObj->pCopy : -1), nIters==0 );
            }
        }
        // merge the new cuts with the old cuts
        Abc_NtkForEachPi( pNtk, pObj, i )
            Cut_NodeNewMergeWithOld( p, pObj->Id );
        Abc_AigForEachAnd( pNtk, pObj, i )
            Cut_NodeNewMergeWithOld( p, pObj->Id );
        // for the cutset, transfer temp cuts to new cuts
        fStatus = 0;
        Abc_SeqForEachCutsetNode( pNtk, pObj, i )
            fStatus |= Cut_NodeTempTransferToNew( p, pObj->Id, i );
        if ( fStatus == 0 )
            break;
    }
    Vec_IntFree( vChoices );

    // if the status is not finished, transfer new to old for the cutset
    Abc_SeqForEachCutsetNode( pNtk, pObj, i )
        Cut_NodeNewMergeWithOld( p, pObj->Id );

    // transfer the old cuts to the new positions
    Abc_NtkForEachObj( pNtk, pObj, i )
        Cut_NodeOldTransferToNew( p, pObj->Id );

    // unlabel the cutset nodes
    Abc_SeqForEachCutsetNode( pNtk, pObj, i )
        pObj->fMarkC = 0;
if ( pParams->fVerbose )
{
    Cut_ManPrintStats( p );
ABC_PRT( "TOTAL ", Abc_Clock() - clk );
printf( "Converged after %d iterations.\n", nIters );
}
//Abc_NtkPrintCuts( p, pNtk, 1 );
    return p;
*/
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Computes area.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkComputeArea( Abc_Ntk_t * pNtk, Cut_Man_t * p )
{
    Abc_Obj_t * pObj;
    int Counter, i;
    Counter = 0;
    Abc_NtkForEachCo( pNtk, pObj, i )
        Counter += Cut_ManMappingArea_rec( p, Abc_ObjFaninId0(pObj) );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Computes the cuts for the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Abc_NodeGetCutsRecursive( void * p, Abc_Obj_t * pObj, int fDag, int fTree )
{
    void * pList;
    if ( (pList = Abc_NodeReadCuts( p, pObj )) )
        return pList;
    Abc_NodeGetCutsRecursive( p, Abc_ObjFanin0(pObj), fDag, fTree );
    Abc_NodeGetCutsRecursive( p, Abc_ObjFanin1(pObj), fDag, fTree );
    return Abc_NodeGetCuts( p, pObj, fDag, fTree );
}

/**Function*************************************************************

  Synopsis    [Computes the cuts for the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Abc_NodeGetCuts( void * p, Abc_Obj_t * pObj, int fDag, int fTree )
{
    Abc_Obj_t * pFanin;
    int fDagNode, fTriv, TreeCode = 0;
//    assert( Abc_NtkIsStrash(pObj->pNtk) );
    assert( Abc_ObjFaninNum(pObj) == 2 );

    // check if the node is a DAG node
    fDagNode = (Abc_ObjFanoutNum(pObj) > 1 && !Abc_NodeIsMuxControlType(pObj));
    // increment the counter of DAG nodes
    if ( fDagNode ) Cut_ManIncrementDagNodes( (Cut_Man_t *)p );
    // add the trivial cut if the node is a DAG node, or if we compute all cuts
    fTriv = fDagNode || !fDag;
    // check if fanins are DAG nodes
    if ( fTree )
    {
        pFanin = Abc_ObjFanin0(pObj);
        TreeCode |=  (Abc_ObjFanoutNum(pFanin) > 1 && !Abc_NodeIsMuxControlType(pFanin));
        pFanin = Abc_ObjFanin1(pObj);
        TreeCode |= ((Abc_ObjFanoutNum(pFanin) > 1 && !Abc_NodeIsMuxControlType(pFanin)) << 1);
    }

    // changes due to the global/local cut computation
    {
        Cut_Params_t * pParams = Cut_ManReadParams((Cut_Man_t *)p);
        if ( pParams->fLocal )
        {
            Vec_Int_t * vNodeAttrs = Cut_ManReadNodeAttrs((Cut_Man_t *)p);
            fDagNode = Vec_IntEntry( vNodeAttrs, pObj->Id );
            if ( fDagNode ) Cut_ManIncrementDagNodes( (Cut_Man_t *)p );
//            fTriv = fDagNode || !pParams->fGlobal;
            fTriv = !Vec_IntEntry( vNodeAttrs, pObj->Id );
            TreeCode = 0;
            pFanin = Abc_ObjFanin0(pObj);
            TreeCode |=  Vec_IntEntry( vNodeAttrs, pFanin->Id );
            pFanin = Abc_ObjFanin1(pObj);
            TreeCode |= (Vec_IntEntry( vNodeAttrs, pFanin->Id ) << 1);
        }
    }
    return Cut_NodeComputeCuts( (Cut_Man_t *)p, pObj->Id, Abc_ObjFaninId0(pObj), Abc_ObjFaninId1(pObj),  
        Abc_ObjFaninC0(pObj), Abc_ObjFaninC1(pObj), fTriv, TreeCode );  
}

/**Function*************************************************************

  Synopsis    [Computes the cuts for the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeGetCutsSeq( void * p, Abc_Obj_t * pObj, int fTriv )
{
/*
    int CutSetNum;
    assert( Abc_NtkIsSeq(pObj->pNtk) );
    assert( Abc_ObjFaninNum(pObj) == 2 );
    fTriv     = pObj->fMarkC ? 0 : fTriv;
    CutSetNum = pObj->fMarkC ? (int)pObj->pCopy : -1;
    Cut_NodeComputeCutsSeq( p, pObj->Id, Abc_ObjFaninId0(pObj), Abc_ObjFaninId1(pObj),  
        Abc_ObjFaninC0(pObj), Abc_ObjFaninC1(pObj), Seq_ObjFaninL0(pObj), Seq_ObjFaninL1(pObj), fTriv, CutSetNum );  
*/
}

/**Function*************************************************************

  Synopsis    [Computes the cuts for the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Abc_NodeReadCuts( void * p, Abc_Obj_t * pObj )
{
    return Cut_NodeReadCutsNew( (Cut_Man_t *)p, pObj->Id );  
}

/**Function*************************************************************

  Synopsis    [Computes the cuts for the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeFreeCuts( void * p, Abc_Obj_t * pObj )
{
    Cut_NodeFreeCuts( (Cut_Man_t *)p, pObj->Id );
}

/**Function*************************************************************

  Synopsis    [Computes the cuts for the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPrintCuts( void * p, Abc_Ntk_t * pNtk, int fSeq )
{
    Cut_Cut_t * pList;
    Abc_Obj_t * pObj;
    int i;
    printf( "Cuts of the network:\n" );
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        pList = (Cut_Cut_t *)Abc_NodeReadCuts( (Cut_Man_t *)p, pObj );
        printf( "Node %s:\n", Abc_ObjName(pObj) );
        Cut_CutPrintList( pList, fSeq );
    }
}

/**Function*************************************************************

  Synopsis    [Computes the cuts for the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPrintCuts_( void * p, Abc_Ntk_t * pNtk, int fSeq )
{
    Cut_Cut_t * pList;
    Abc_Obj_t * pObj;
    pObj = Abc_NtkObj( pNtk, 2 * Abc_NtkObjNum(pNtk) / 3 );
    pList = (Cut_Cut_t *)Abc_NodeReadCuts( (Cut_Man_t *)p, pObj );
    printf( "Node %s:\n", Abc_ObjName(pObj) );
    Cut_CutPrintList( pList, fSeq );
}

/**Function*************************************************************

  Synopsis    [Assigns global attributes randomly.]

  Description [Old code.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Abc_NtkGetNodeAttributes( Abc_Ntk_t * pNtk ) 
{
    Vec_Int_t * vAttrs;
//    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj;//, * pTemp;
    int i;//, k;
    int nNodesTotal = 0, nMffcsTotal = 0;
    extern Vec_Ptr_t * Abc_NodeMffcInsideCollect( Abc_Obj_t * pNode );

    vAttrs = Vec_IntStart( Abc_NtkObjNumMax(pNtk) + 1 );
//    Abc_NtkForEachCi( pNtk, pObj, i )
//        Vec_IntWriteEntry( vAttrs, pObj->Id, 1 );

    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        if ( Abc_ObjIsNode(pObj) )
            nNodesTotal++;
        if ( Abc_ObjIsCo(pObj) && Abc_ObjIsNode(Abc_ObjFanin0(pObj)) )
            nMffcsTotal += Abc_NodeMffcSize( Abc_ObjFanin0(pObj) );
//        if ( Abc_ObjIsNode(pObj) && (rand() % 4 == 0) )
//        if ( Abc_ObjIsNode(pObj) && Abc_ObjFanoutNum(pObj) > 1 && !Abc_NodeIsMuxControlType(pObj) && (rand() % 3 == 0) )
        if ( Abc_ObjIsNode(pObj) && Abc_ObjFanoutNum(pObj) > 1 && !Abc_NodeIsMuxControlType(pObj) )
        {
            int nMffc = Abc_NodeMffcSize(pObj);
            nMffcsTotal += Abc_NodeMffcSize(pObj);
//            printf( "%d ", nMffc );

            if ( nMffc > 2 || Abc_ObjFanoutNum(pObj) > 8 )
                Vec_IntWriteEntry( vAttrs, pObj->Id, 1 );
        }
    }
/*
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        if ( Vec_IntEntry( vAttrs, pObj->Id ) )
        {
            vNodes = Abc_NodeMffcInsideCollect( pObj );
            Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pTemp, k )
                if ( pTemp != pObj )
                    Vec_IntWriteEntry( vAttrs, pTemp->Id, 0 );
            Vec_PtrFree( vNodes );
        }
    }
*/
    printf( "Total nodes = %d. Total MFFC nodes = %d.\n", nNodesTotal, nMffcsTotal );
    return vAttrs; 
}

/**Function*************************************************************

  Synopsis    [Assigns global attributes randomly.]

  Description [Old code.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkSubDagSize_rec( Abc_Obj_t * pObj, Vec_Int_t * vAttrs ) 
{
    if ( Abc_NodeIsTravIdCurrent(pObj) )
        return 0;
    Abc_NodeSetTravIdCurrent(pObj);
    if ( Vec_IntEntry( vAttrs, pObj->Id ) )
        return 0;
    if ( Abc_ObjIsCi(pObj) )
        return 1;
    assert( Abc_ObjFaninNum(pObj) == 2 );
    return 1 + Abc_NtkSubDagSize_rec(Abc_ObjFanin0(pObj), vAttrs) +
        Abc_NtkSubDagSize_rec(Abc_ObjFanin1(pObj), vAttrs);
}

/**Function*************************************************************

  Synopsis    [Assigns global attributes randomly.]

  Description [Old code.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Abc_NtkGetNodeAttributes2( Abc_Ntk_t * pNtk ) 
{
    Vec_Int_t * vAttrs;
    Abc_Obj_t * pObj;
    int i, nSize;
    assert( Abc_NtkIsDfsOrdered(pNtk) );
    vAttrs = Vec_IntStart( Abc_NtkObjNumMax(pNtk) + 1 );
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        // skip no-nodes and nodes without fanouts
        if ( pObj->Id == 0 || !(Abc_ObjIsNode(pObj) && Abc_ObjFanoutNum(pObj) > 1 && !Abc_NodeIsMuxControlType(pObj)) )
            continue;
        // the node has more than one fanout - count its sub-DAG size
        Abc_NtkIncrementTravId( pNtk );
        nSize = Abc_NtkSubDagSize_rec( pObj, vAttrs );
        if ( nSize > 15 )
            Vec_IntWriteEntry( vAttrs, pObj->Id, 1 );
    }
    return vAttrs; 
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

