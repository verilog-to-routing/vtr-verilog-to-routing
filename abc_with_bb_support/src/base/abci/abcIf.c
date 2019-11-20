/**CFile****************************************************************

  FileName    [abcIf.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Interface with the FPGA mapping package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 21, 2006.]

  Revision    [$Id: abcIf.c,v 1.00 2006/11/21 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abc.h"
#include "if.h"
#include "kit.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static If_Man_t *  Abc_NtkToIf( Abc_Ntk_t * pNtk, If_Par_t * pPars );
static Abc_Ntk_t * Abc_NtkFromIf( If_Man_t * pIfMan, Abc_Ntk_t * pNtk );
extern Abc_Obj_t * Abc_NodeFromIf_rec( Abc_Ntk_t * pNtkNew, If_Man_t * pIfMan, If_Obj_t * pIfObj, Vec_Int_t * vCover );
static Hop_Obj_t * Abc_NodeIfToHop( Hop_Man_t * pHopMan, If_Man_t * pIfMan, If_Obj_t * pIfObj );
static Vec_Ptr_t * Abc_NtkFindGoodOrder( Abc_Ntk_t * pNtk );

extern void Abc_NtkBddReorder( Abc_Ntk_t * pNtk, int fVerbose );
 
////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Interface with the FPGA mapping package.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkIf( Abc_Ntk_t * pNtk, If_Par_t * pPars )
{
    Abc_Ntk_t * pNtkNew;
    If_Man_t * pIfMan;

    assert( Abc_NtkIsStrash(pNtk) );

    // get timing information
    pPars->pTimesArr = Abc_NtkGetCiArrivalFloats(pNtk);
    pPars->pTimesReq = NULL;

    // set the latch paths
    if ( pPars->fLatchPaths && pPars->pTimesArr )
    {
        int c;
        for ( c = 0; c < Abc_NtkPiNum(pNtk); c++ )
            pPars->pTimesArr[c] = -ABC_INFINITY;
    }

    // perform FPGA mapping
    pIfMan = Abc_NtkToIf( pNtk, pPars );    
    if ( pIfMan == NULL )
        return NULL;
    if ( !If_ManPerformMapping( pIfMan ) )
    {
        If_ManStop( pIfMan );
        return NULL;
    }

    // transform the result of mapping into the new network
    pNtkNew = Abc_NtkFromIf( pIfMan, pNtk );
    if ( pNtkNew == NULL )
        return NULL;
    If_ManStop( pIfMan );

    // duplicate EXDC
    if ( pNtk->pExdc )
        pNtkNew->pExdc = Abc_NtkDup( pNtk->pExdc );

    // make sure that everything is okay
    if ( !Abc_NtkCheck( pNtkNew ) )
    {
        printf( "Abc_NtkIf: The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Load the network into FPGA manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_Man_t * Abc_NtkToIf( Abc_Ntk_t * pNtk, If_Par_t * pPars )
{
    ProgressBar * pProgress;
    If_Man_t * pIfMan;
    Abc_Obj_t * pNode, * pFanin, * pPrev;
    Vec_Ptr_t * vNodes;
    int i;

    assert( Abc_NtkIsStrash(pNtk) );
//    vNodes = Abc_NtkFindGoodOrder( pNtk );
    vNodes = Abc_AigDfs( pNtk, 0, 0 );

    // start the mapping manager and set its parameters
    pIfMan = If_ManStart( pPars );

    // print warning about excessive memory usage
    if ( 1.0 * Abc_NtkObjNum(pNtk) * pIfMan->nObjBytes / (1<<30) > 1.0 )
        printf( "Warning: The mapper will allocate %.1f Gb for to represent the subject graph with %d AIG nodes.\n", 
            1.0 * Abc_NtkObjNum(pNtk) * pIfMan->nObjBytes / (1<<30), Abc_NtkObjNum(pNtk) );

    // create PIs and remember them in the old nodes
    Abc_AigConst1(pNtk)->pCopy = (Abc_Obj_t *)If_ManConst1( pIfMan );
    Abc_NtkForEachCi( pNtk, pNode, i )
    {
        pNode->pCopy = (Abc_Obj_t *)If_ManCreateCi( pIfMan );
//printf( "AIG CI %2d -> IF CI %2d\n", pNode->Id, ((If_Obj_t *)pNode->pCopy)->Id );
    }

    // load the AIG into the mapper
    pProgress = Extra_ProgressBarStart( stdout, Abc_NtkObjNumMax(pNtk) );
//    Abc_AigForEachAnd( pNtk, pNode, i )
    Vec_PtrForEachEntry( vNodes, pNode, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, "Initial" );
        // add the node to the mapper
        pNode->pCopy = (Abc_Obj_t *)If_ManCreateAnd( pIfMan, 
            If_NotCond( (If_Obj_t *)Abc_ObjFanin0(pNode)->pCopy, Abc_ObjFaninC0(pNode) ), 
            If_NotCond( (If_Obj_t *)Abc_ObjFanin1(pNode)->pCopy, Abc_ObjFaninC1(pNode) ) );
        // set up the choice node
        if ( Abc_AigNodeIsChoice( pNode ) )
        {
            pIfMan->nChoices++;
            for ( pPrev = pNode, pFanin = pNode->pData; pFanin; pPrev = pFanin, pFanin = pFanin->pData )
                If_ObjSetChoice( (If_Obj_t *)pPrev->pCopy, (If_Obj_t *)pFanin->pCopy );
            If_ManCreateChoice( pIfMan, (If_Obj_t *)pNode->pCopy );
        }
//printf( "AIG node %2d -> IF node %2d\n", pNode->Id, ((If_Obj_t *)pNode->pCopy)->Id );
    }
    Extra_ProgressBarStop( pProgress );
    Vec_PtrFree( vNodes );

    // set the primary outputs without copying the phase
    Abc_NtkForEachCo( pNtk, pNode, i )
        If_ManCreateCo( pIfMan, If_NotCond( (If_Obj_t *)Abc_ObjFanin0(pNode)->pCopy, Abc_ObjFaninC0(pNode) ) );
    return pIfMan;
}

/**Function*************************************************************

  Synopsis    [Creates the mapped network.]

  Description [Assuming the copy field of the mapped nodes are NULL.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkFromIf( If_Man_t * pIfMan, Abc_Ntk_t * pNtk )
{
    ProgressBar * pProgress;
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pNode, * pNodeNew;
    Vec_Int_t * vCover;
    int i, nDupGates;
    // create the new network
    if ( pIfMan->pPars->fUseBdds || pIfMan->pPars->fUseCnfs || pIfMan->pPars->fUseMv )
        pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_LOGIC, ABC_FUNC_BDD );
    else if ( pIfMan->pPars->fUseSops )
        pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_LOGIC, ABC_FUNC_SOP );
    else
        pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_LOGIC, ABC_FUNC_AIG );
    // prepare the mapping manager
    If_ManCleanNodeCopy( pIfMan );
    If_ManCleanCutData( pIfMan );
    // make the mapper point to the new network
    If_ObjSetCopy( If_ManConst1(pIfMan), Abc_NtkCreateNodeConst1(pNtkNew) );
    Abc_NtkForEachCi( pNtk, pNode, i )
        If_ObjSetCopy( If_ManCi(pIfMan, i), pNode->pCopy );
    // process the nodes in topological order
    vCover = Vec_IntAlloc( 1 << 16 );
    pProgress = Extra_ProgressBarStart( stdout, Abc_NtkCoNum(pNtk) );
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, "Final" );
        pNodeNew = Abc_NodeFromIf_rec( pNtkNew, pIfMan, If_ObjFanin0(If_ManCo(pIfMan, i)), vCover );
        pNodeNew = Abc_ObjNotCond( pNodeNew, If_ObjFaninC0(If_ManCo(pIfMan, i)) );
        Abc_ObjAddFanin( pNode->pCopy, pNodeNew );
    }
    Extra_ProgressBarStop( pProgress );
    Vec_IntFree( vCover );
    // remove the constant node if not used
    pNodeNew = (Abc_Obj_t *)If_ObjCopy( If_ManConst1(pIfMan) );
    if ( Abc_ObjFanoutNum(pNodeNew) == 0 )
        Abc_NtkDeleteObj( pNodeNew );
    // minimize the node
    if ( pIfMan->pPars->fUseBdds || pIfMan->pPars->fUseCnfs || pIfMan->pPars->fUseMv )
        Abc_NtkSweep( pNtkNew, 0 );
    if ( pIfMan->pPars->fUseBdds )
        Abc_NtkBddReorder( pNtkNew, 0 );
    // decouple the PO driver nodes to reduce the number of levels
    nDupGates = Abc_NtkLogicMakeSimpleCos( pNtkNew, 1 );
//    if ( nDupGates && If_ManReadVerbose(pIfMan) )
//        printf( "Duplicated %d gates to decouple the CO drivers.\n", nDupGates );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Derive one node after FPGA mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NodeFromIf_rec( Abc_Ntk_t * pNtkNew, If_Man_t * pIfMan, If_Obj_t * pIfObj, Vec_Int_t * vCover )
{
    Abc_Obj_t * pNodeNew;
    If_Cut_t * pCutBest;
    If_Obj_t * pIfLeaf;
    int i;
    // return if the result if known
    pNodeNew = (Abc_Obj_t *)If_ObjCopy( pIfObj );
    if ( pNodeNew )
        return pNodeNew;
    assert( pIfObj->Type == IF_AND );
    // get the parameters of the best cut
    // create a new node 
    pNodeNew = Abc_NtkCreateNode( pNtkNew );
    pCutBest = If_ObjCutBest( pIfObj );
    if ( pIfMan->pPars->fUseCnfs || pIfMan->pPars->fUseMv )
    {
        If_CutForEachLeafReverse( pIfMan, pCutBest, pIfLeaf, i )
            Abc_ObjAddFanin( pNodeNew, Abc_NodeFromIf_rec(pNtkNew, pIfMan, pIfLeaf, vCover) );
    }
    else
    {
        If_CutForEachLeaf( pIfMan, pCutBest, pIfLeaf, i )
            Abc_ObjAddFanin( pNodeNew, Abc_NodeFromIf_rec(pNtkNew, pIfMan, pIfLeaf, vCover) );
    }
    // set the level of the new node
    pNodeNew->Level = Abc_ObjLevelNew( pNodeNew );
    // derive the function of this node
    if ( pIfMan->pPars->fTruth )
    {
        if ( pIfMan->pPars->fUseBdds )
        { 
            // transform truth table into the BDD 
            pNodeNew->pData = Kit_TruthToBdd( pNtkNew->pManFunc, If_CutTruth(pCutBest), If_CutLeaveNum(pCutBest), 0 );  Cudd_Ref(pNodeNew->pData); 
        }
        else if ( pIfMan->pPars->fUseCnfs || pIfMan->pPars->fUseMv )
        { 
            // transform truth table into the BDD 
            pNodeNew->pData = Kit_TruthToBdd( pNtkNew->pManFunc, If_CutTruth(pCutBest), If_CutLeaveNum(pCutBest), 1 );  Cudd_Ref(pNodeNew->pData); 
        }
        else if ( pIfMan->pPars->fUseSops ) 
        {
            // transform truth table into the SOP
            int RetValue = Kit_TruthIsop( If_CutTruth(pCutBest), If_CutLeaveNum(pCutBest), vCover, 1 );
            assert( RetValue == 0 || RetValue == 1 );
            // check the case of constant cover
            if ( Vec_IntSize(vCover) == 0 || (Vec_IntSize(vCover) == 1 && Vec_IntEntry(vCover,0) == 0) )
            {
                assert( RetValue == 0 );
                pNodeNew->pData = Abc_SopCreateAnd( pNtkNew->pManFunc, If_CutLeaveNum(pCutBest), NULL );
                pNodeNew = (Vec_IntSize(vCover) == 0) ? Abc_NtkCreateNodeConst0(pNtkNew) : Abc_NtkCreateNodeConst1(pNtkNew);
            }
            else
            {
                // derive the AIG for that tree
                pNodeNew->pData = Abc_SopCreateFromIsop( pNtkNew->pManFunc, If_CutLeaveNum(pCutBest), vCover );
                if ( RetValue )
                    Abc_SopComplement( pNodeNew->pData );
            }
        }
        else 
        {
            extern Hop_Obj_t * Kit_GraphToHop( Hop_Man_t * pMan, Kit_Graph_t * pGraph );
            // transform truth table into the decomposition tree
            Kit_Graph_t * pGraph = Kit_TruthToGraph( If_CutTruth(pCutBest), If_CutLeaveNum(pCutBest), vCover );
            // derive the AIG for the decomposition tree
            pNodeNew->pData = Kit_GraphToHop( pNtkNew->pManFunc, pGraph );
            Kit_GraphFree( pGraph );
        }
        // complement the node if the cut was complemented
        if ( pCutBest->fCompl )
            Abc_NodeComplement( pNodeNew );
    }
    else
        pNodeNew->pData = Abc_NodeIfToHop( pNtkNew->pManFunc, pIfMan, pIfObj );
    If_ObjSetCopy( pIfObj, pNodeNew );
    return pNodeNew;
}

/**Function*************************************************************

  Synopsis    [Recursively derives the truth table for the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Abc_NodeIfToHop_rec( Hop_Man_t * pHopMan, If_Man_t * pIfMan, If_Obj_t * pIfObj, Vec_Ptr_t * vVisited )
{
    If_Cut_t * pCut;
    Hop_Obj_t * gFunc, * gFunc0, * gFunc1;
    // get the best cut
    pCut = If_ObjCutBest(pIfObj);
    // if the cut is visited, return the result
    if ( If_CutData(pCut) )
        return If_CutData(pCut);
    // compute the functions of the children
    gFunc0 = Abc_NodeIfToHop_rec( pHopMan, pIfMan, pIfObj->pFanin0, vVisited );
    gFunc1 = Abc_NodeIfToHop_rec( pHopMan, pIfMan, pIfObj->pFanin1, vVisited );
    // get the function of the cut
    gFunc  = Hop_And( pHopMan, Hop_NotCond(gFunc0, pIfObj->fCompl0), Hop_NotCond(gFunc1, pIfObj->fCompl1) );  
    assert( If_CutData(pCut) == NULL );
    If_CutSetData( pCut, gFunc );
    // add this cut to the visited list
    Vec_PtrPush( vVisited, pCut );
    return gFunc;
}

/**Function*************************************************************

  Synopsis    [Derives the truth table for one cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Abc_NodeIfToHop( Hop_Man_t * pHopMan, If_Man_t * pIfMan, If_Obj_t * pIfObj )
{
    If_Cut_t * pCut;
    Hop_Obj_t * gFunc;
    If_Obj_t * pLeaf;
    int i;
    // get the best cut
    pCut = If_ObjCutBest(pIfObj);
    assert( pCut->nLeaves > 1 );
    // set the leaf variables
    If_CutForEachLeaf( pIfMan, pCut, pLeaf, i )
        If_CutSetData( If_ObjCutBest(pLeaf), Hop_IthVar(pHopMan, i) );
    // recursively compute the function while collecting visited cuts
    Vec_PtrClear( pIfMan->vTemp );
    gFunc = Abc_NodeIfToHop_rec( pHopMan, pIfMan, pIfObj, pIfMan->vTemp ); 
//    printf( "%d ", Vec_PtrSize(p->vTemp) );
    // clean the cuts
    If_CutForEachLeaf( pIfMan, pCut, pLeaf, i )
        If_CutSetData( If_ObjCutBest(pLeaf), NULL );
    Vec_PtrForEachEntry( pIfMan->vTemp, pCut, i )
        If_CutSetData( pCut, NULL );
    return gFunc;
}


/**Function*************************************************************

  Synopsis    [Comparison for two nodes with the flow.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_ObjCompareFlow( Abc_Obj_t ** ppNode0, Abc_Obj_t ** ppNode1 )
{
    float Flow0 = Abc_Int2Float((int)(*ppNode0)->pCopy);
    float Flow1 = Abc_Int2Float((int)(*ppNode1)->pCopy);
    if ( Flow0 > Flow1 )
        return -1;
    if ( Flow0 < Flow1 )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Orders AIG nodes so that nodes from larger cones go first.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkFindGoodOrder_rec( Abc_Obj_t * pNode, Vec_Ptr_t * vNodes )
{
    if ( !Abc_ObjIsNode(pNode) )
        return;
    assert( Abc_ObjIsNode( pNode ) );
    // if this node is already visited, skip
    if ( Abc_NodeIsTravIdCurrent( pNode ) )
        return;
    // mark the node as visited
    Abc_NodeSetTravIdCurrent( pNode );
    // visit the transitive fanin of the node
    Abc_NtkFindGoodOrder_rec( Abc_ObjFanin0(pNode), vNodes );
    Abc_NtkFindGoodOrder_rec( Abc_ObjFanin1(pNode), vNodes );
    // add the node after the fanins have been added
    Vec_PtrPush( vNodes, pNode );
}

/**Function*************************************************************

  Synopsis    [Orders AIG nodes so that nodes from larger cones go first.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NtkFindGoodOrder( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vNodes, * vCos;
    Abc_Obj_t * pNode, * pFanin0, * pFanin1;
    float Flow0, Flow1;
    int i;

    // initialize the flow
    Abc_AigConst1(pNtk)->pCopy = NULL;
    Abc_NtkForEachCi( pNtk, pNode, i )
        pNode->pCopy = NULL;
    // compute the flow
    Abc_AigForEachAnd( pNtk, pNode, i )
    {
        pFanin0 = Abc_ObjFanin0(pNode);
        pFanin1 = Abc_ObjFanin1(pNode);
        Flow0 = Abc_Int2Float((int)pFanin0->pCopy)/Abc_ObjFanoutNum(pFanin0);
        Flow1 = Abc_Int2Float((int)pFanin1->pCopy)/Abc_ObjFanoutNum(pFanin1);
        pNode->pCopy = (Abc_Obj_t *)Abc_Float2Int(Flow0 + Flow1+(float)1.0);
    }
    // find the flow of the COs
    vCos = Vec_PtrAlloc( Abc_NtkCoNum(pNtk) );
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        pNode->pCopy = Abc_ObjFanin0(pNode)->pCopy;
//        pNode->pCopy = (Abc_Obj_t *)Abc_Float2Int((float)Abc_ObjFanin0(pNode)->Level);
        Vec_PtrPush( vCos, pNode );
    }

    // sort nodes in the increasing order of the flow
    qsort( (Abc_Obj_t **)Vec_PtrArray(vCos), Abc_NtkCoNum(pNtk), 
        sizeof(Abc_Obj_t *), (int (*)(const void *, const void *))Abc_ObjCompareFlow );
    // verify sorting
    pFanin0 = Vec_PtrEntry(vCos, 0);
    pFanin1 = Vec_PtrEntryLast(vCos);
    assert( Abc_Int2Float((int)pFanin0->pCopy) >= Abc_Int2Float((int)pFanin1->pCopy) );

    // collect the nodes in the topological order from the new array
    Abc_NtkIncrementTravId( pNtk );
    vNodes = Vec_PtrAlloc( 100 );
    Vec_PtrForEachEntry( vCos, pNode, i )
    {
        Abc_NtkFindGoodOrder_rec( Abc_ObjFanin0(pNode), vNodes );
//        printf( "%.2f ", Abc_Int2Float((int)pNode->pCopy) );
    }
    Vec_PtrFree( vCos );
    return vNodes;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


