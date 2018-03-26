/**CFile****************************************************************

  FileName    [ivySeq.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [And-Inverter Graph package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 11, 2006.]
 
  Revision    [$Id: ivySeq.c,v 1.00 2006/05/11 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ivy.h"
#include "bool/deco/deco.h"
#include "opt/rwt/rwt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Ivy_NodeRewriteSeq( Ivy_Man_t * pMan, Rwt_Man_t * p, Ivy_Obj_t * pNode, int fUseZeroCost );
static void Ivy_GraphPrepare( Dec_Graph_t * pGraph, Ivy_Cut_t * pCut, Vec_Ptr_t * vFanins, char * pPerm );
static unsigned Ivy_CutGetTruth( Ivy_Man_t * p, Ivy_Obj_t * pObj, int * pNums, int nNums );
static Dec_Graph_t * Rwt_CutEvaluateSeq( Ivy_Man_t * pMan, Rwt_Man_t * p, Ivy_Obj_t * pRoot, Ivy_Cut_t * pCut, char * pPerm, Vec_Ptr_t * vFaninsCur, int nNodesSaved, int * pGainBest, unsigned uTruth );
static int Ivy_GraphToNetworkSeqCountSeq( Ivy_Man_t * p, Ivy_Obj_t * pRoot, Dec_Graph_t * pGraph, int NodeMax );
static Ivy_Obj_t * Ivy_GraphToNetworkSeq( Ivy_Man_t * p, Dec_Graph_t * pGraph );
static void Ivy_GraphUpdateNetworkSeq( Ivy_Man_t * p, Ivy_Obj_t * pRoot, Dec_Graph_t * pGraph, int nGain );
static Ivy_Store_t * Ivy_CutComputeForNode( Ivy_Man_t * p, Ivy_Obj_t * pObj, int nLeaves );

static inline int Ivy_CutHashValue( int NodeId )  { return 1 << (NodeId % 31); }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

//int nMoves;
//int nMovesS;
//int nClauses;
//int timeInv;

/**Function*************************************************************

  Synopsis    [Performs incremental rewriting of the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_ManRewriteSeq( Ivy_Man_t * p, int fUseZeroCost, int fVerbose )
{ 
    Rwt_Man_t * pManRwt;
    Ivy_Obj_t * pNode;
    int i, nNodes, nGain;
    abctime clk, clkStart = Abc_Clock();

    // set the DC latch values
    Ivy_ManForEachLatch( p, pNode, i )
        pNode->Init = IVY_INIT_DC;
    // start the rewriting manager
    pManRwt = Rwt_ManStart( 0 );
    p->pData = pManRwt;
    if ( pManRwt == NULL )
        return 0; 
    // create fanouts
    if ( p->fFanout == 0 )
        Ivy_ManStartFanout( p );
    // resynthesize each node once
    nNodes = Ivy_ManObjIdMax(p);
    Ivy_ManForEachNode( p, pNode, i )
    {
        assert( !Ivy_ObjIsBuf(pNode) );
        assert( !Ivy_ObjIsBuf(Ivy_ObjFanin0(pNode)) );
        assert( !Ivy_ObjIsBuf(Ivy_ObjFanin1(pNode)) );
        // fix the fanin buffer problem
//        Ivy_NodeFixBufferFanins( p, pNode );
//        if ( Ivy_ObjIsBuf(pNode) )
//            continue;
        // stop if all nodes have been tried once
        if ( i > nNodes ) 
            break;
        // for each cut, try to resynthesize it
        nGain = Ivy_NodeRewriteSeq( p, pManRwt, pNode, fUseZeroCost );
        if ( nGain > 0 || (nGain == 0 && fUseZeroCost) )
        {
            Dec_Graph_t * pGraph = (Dec_Graph_t *)Rwt_ManReadDecs(pManRwt);
            int fCompl           = Rwt_ManReadCompl(pManRwt);
            // complement the FF if needed
clk = Abc_Clock();
            if ( fCompl ) Dec_GraphComplement( pGraph );
            Ivy_GraphUpdateNetworkSeq( p, pNode, pGraph, nGain );
            if ( fCompl ) Dec_GraphComplement( pGraph );
Rwt_ManAddTimeUpdate( pManRwt, Abc_Clock() - clk );
        }
    }
Rwt_ManAddTimeTotal( pManRwt, Abc_Clock() - clkStart );
    // print stats
    if ( fVerbose )
        Rwt_ManPrintStats( pManRwt );
    // delete the managers
    Rwt_ManStop( pManRwt );
    p->pData = NULL;
    // fix the levels
    Ivy_ManResetLevels( p );
//    if ( Ivy_ManCheckFanoutNums(p) )
//        printf( "Ivy_ManRewritePre(): The check has failed.\n" );
    // check
    if ( !Ivy_ManCheck(p) )
        printf( "Ivy_ManRewritePre(): The check has failed.\n" );
    return 1;
}


/**Function*************************************************************

  Synopsis    [Performs rewriting for one node.]

  Description [This procedure considers all the cuts computed for the node
  and tries to rewrite each of them using the "forest" of different AIG
  structures precomputed and stored in the RWR manager. 
  Determines the best rewriting and computes the gain in the number of AIG
  nodes in the final network. In the end, p->vFanins contains information 
  about the best cut that can be used for rewriting, while p->pGraph gives 
  the decomposition dag (represented using decomposition graph data structure).
  Returns gain in the number of nodes or -1 if node cannot be rewritten.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_NodeRewriteSeq( Ivy_Man_t * pMan, Rwt_Man_t * p, Ivy_Obj_t * pNode, int fUseZeroCost )
{
    int fVeryVerbose = 0;
    Dec_Graph_t * pGraph;
    Ivy_Store_t * pStore;
    Ivy_Cut_t * pCut;
    Ivy_Obj_t * pFanin;//, * pFanout;
    Vec_Ptr_t * vFanout;
    unsigned uPhase;
    unsigned uTruthBest = 0; // Suppress "might be used uninitialized"
    unsigned uTruth;//, nNewClauses;
    char * pPerm;
    int nNodesSaved;
    int nNodesSaveCur = -1; // Suppress "might be used uninitialized"
    int i, c, GainCur = -1, GainBest = -1;
    abctime clk, clk2;//, clk3;

    p->nNodesConsidered++;
    // get the node's cuts
clk = Abc_Clock();
    pStore = Ivy_CutComputeForNode( pMan, pNode, 5 );
p->timeCut += Abc_Clock() - clk;

    // go through the cuts
clk = Abc_Clock();
    vFanout = Vec_PtrAlloc( 100 );
    for ( c = 1; c < pStore->nCuts; c++ )
    {
        pCut = pStore->pCuts + c;
        // consider only 4-input cuts
        if ( pCut->nSize != 4 )
            continue;
        // skip the cuts with buffers
        for ( i = 0; i < (int)pCut->nSize; i++ )
            if ( Ivy_ObjIsBuf( Ivy_ManObj(pMan, Ivy_LeafId(pCut->pArray[i])) ) )
                break;
        if ( i != pCut->nSize )
        {
            p->nCutsBad++;
            continue;
        }
        p->nCutsGood++;
        // get the fanin permutation
clk2 = Abc_Clock();
        uTruth = 0xFFFF & Ivy_CutGetTruth( pMan, pNode, pCut->pArray, pCut->nSize );  // truth table
p->timeTruth += Abc_Clock() - clk2;
        pPerm = p->pPerms4[ (int)p->pPerms[uTruth] ];
        uPhase = p->pPhases[uTruth];
        // collect fanins with the corresponding permutation/phase
        Vec_PtrClear( p->vFaninsCur );
        Vec_PtrFill( p->vFaninsCur, (int)pCut->nSize, 0 );
        for ( i = 0; i < (int)pCut->nSize; i++ )
        {
            pFanin = Ivy_ManObj( pMan, Ivy_LeafId( pCut->pArray[(int)pPerm[i]] ) );
            assert( Ivy_ObjIsNode(pFanin) || Ivy_ObjIsCi(pFanin) || Ivy_ObjIsConst1(pFanin) );
            pFanin = Ivy_NotCond(pFanin, ((uPhase & (1<<i)) > 0) );
            Vec_PtrWriteEntry( p->vFaninsCur, i, pFanin );
        }
clk2 = Abc_Clock();
        // mark the fanin boundary 
        Vec_PtrForEachEntry( Ivy_Obj_t *, p->vFaninsCur, pFanin, i )
            Ivy_ObjRefsInc( Ivy_Regular(pFanin) );
        // label MFFC with current ID
        Ivy_ManIncrementTravId( pMan );
        nNodesSaved = Ivy_ObjMffcLabel( pMan, pNode );
        // label fanouts with the current ID
//        Ivy_ObjForEachFanout( pMan, pNode, vFanout, pFanout, i )
//            Ivy_ObjSetTravIdCurrent( pMan, pFanout );
        // unmark the fanin boundary
        Vec_PtrForEachEntry( Ivy_Obj_t *, p->vFaninsCur, pFanin, i )
            Ivy_ObjRefsDec( Ivy_Regular(pFanin) );
p->timeMffc += Abc_Clock() - clk2;

        // evaluate the cut
clk2 = Abc_Clock();
        pGraph = Rwt_CutEvaluateSeq( pMan, p, pNode, pCut, pPerm, p->vFaninsCur, nNodesSaved, &GainCur, uTruth );
p->timeEval += Abc_Clock() - clk2;


        // check if the cut is better than the current best one
        if ( pGraph != NULL && GainBest < GainCur )
        {
            // save this form
            nNodesSaveCur = nNodesSaved;
            GainBest   = GainCur;
            p->pGraph  = pGraph;
            p->pCut    = pCut;
            p->pPerm   = pPerm;
            p->fCompl  = ((uPhase & (1<<4)) > 0);
            uTruthBest = uTruth;
            // collect fanins in the
            Vec_PtrClear( p->vFanins );
            Vec_PtrForEachEntry( Ivy_Obj_t *, p->vFaninsCur, pFanin, i )
                Vec_PtrPush( p->vFanins, pFanin );
        }
    }
    Vec_PtrFree( vFanout );
p->timeRes += Abc_Clock() - clk;

    if ( GainBest == -1 )
        return -1;
/*
    {
    Ivy_Cut_t * pCut = p->pCut;
    printf( "Node %5d. Using cut : {", Ivy_ObjId(pNode) );
    for ( i = 0; i < pCut->nSize; i++ )
        printf( " %d(%d)", Ivy_LeafId(pCut->pArray[i]), Ivy_LeafLat(pCut->pArray[i]) );
    printf( " }\n" );
    }
*/

//clk3 = Abc_Clock();
//nNewClauses = Ivy_CutTruthPrint( pMan, p->pCut, uTruth );
//timeInv += Abc_Clock() - clk;

//    nClauses += nNewClauses;
//    nMoves++;
//    if ( nNewClauses > 0 )
//        nMovesS++;

    // copy the leaves
    Ivy_GraphPrepare( (Dec_Graph_t *)p->pGraph, (Ivy_Cut_t *)p->pCut, p->vFanins, p->pPerm );

    p->nScores[p->pMap[uTruthBest]]++;
    p->nNodesGained += GainBest;
    if ( fUseZeroCost || GainBest > 0 )
        p->nNodesRewritten++;

/*
    if ( GainBest > 0 )
    {
        Ivy_Cut_t * pCut = p->pCut;
        printf( "Node %5d. Using cut : {", Ivy_ObjId(pNode) );
        for ( i = 0; i < pCut->nSize; i++ )
            printf( " %5d(%2d)", Ivy_LeafId(pCut->pArray[i]), Ivy_LeafLat(pCut->pArray[i]) );
        printf( " }\n" );
    }
*/

    // report the progress
    if ( fVeryVerbose && GainBest > 0 )
    {
        printf( "Node %6d :   ", Ivy_ObjId(pNode) );
        printf( "Fanins = %d. ", p->vFanins->nSize );
        printf( "Save = %d.  ", nNodesSaveCur );
        printf( "Add = %d.  ",  nNodesSaveCur-GainBest );
        printf( "GAIN = %d.  ", GainBest );
        printf( "Cone = %d.  ", p->pGraph? Dec_GraphNodeNum((Dec_Graph_t *)p->pGraph) : 0 );
        printf( "Class = %d.  ", p->pMap[uTruthBest] );
        printf( "\n" );
    }
    return GainBest;
}


/**Function*************************************************************

  Synopsis    [Evaluates the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dec_Graph_t * Rwt_CutEvaluateSeq( Ivy_Man_t * pMan, Rwt_Man_t * p, Ivy_Obj_t * pRoot, Ivy_Cut_t * pCut, char * pPerm, Vec_Ptr_t * vFaninsCur, int nNodesSaved, int * pGainBest, unsigned uTruth )
{
    Vec_Ptr_t * vSubgraphs;
    Dec_Graph_t * pGraphBest = NULL; // Suppress "might be used uninitialized"
    Dec_Graph_t * pGraphCur;
    Rwt_Node_t * pNode;
    int nNodesAdded, GainBest, i;
    // find the matching class of subgraphs
    vSubgraphs = Vec_VecEntry( p->vClasses, p->pMap[uTruth] );
    p->nSubgraphs += vSubgraphs->nSize;
    // determine the best subgraph
    GainBest = -1;
    Vec_PtrForEachEntry( Rwt_Node_t *, vSubgraphs, pNode, i )
    {
        // get the current graph
        pGraphCur = (Dec_Graph_t *)pNode->pNext;

//        if ( pRoot->Id == 8648 )
//        Dec_GraphPrint( stdout, pGraphCur, NULL, NULL );
        // copy the leaves
//        Vec_PtrForEachEntry( Ivy_Obj_t *, vFaninsCur, pFanin, k )
//            Dec_GraphNode(pGraphCur, k)->pFunc = pFanin;
        Ivy_GraphPrepare( pGraphCur, pCut, vFaninsCur, pPerm );

        // detect how many unlabeled nodes will be reused
        nNodesAdded = Ivy_GraphToNetworkSeqCountSeq( pMan, pRoot, pGraphCur, nNodesSaved );
        if ( nNodesAdded == -1 )
            continue;
        assert( nNodesSaved >= nNodesAdded );
        // count the gain at this node
        if ( GainBest < nNodesSaved - nNodesAdded )
        {
            GainBest   = nNodesSaved - nNodesAdded;
            pGraphBest = pGraphCur;
        }
    }
    if ( GainBest == -1 )
        return NULL;
    *pGainBest = GainBest;
    return pGraphBest;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_GraphPrepare( Dec_Graph_t * pGraph, Ivy_Cut_t * pCut, Vec_Ptr_t * vFanins, char * pPerm )
{
    Dec_Node_t * pNode, * pNode0, * pNode1;
    int i;
    assert( Dec_GraphLeaveNum(pGraph) == pCut->nSize );
    assert( Vec_PtrSize(vFanins) == pCut->nSize );
    // label the leaves with latch numbers
    Dec_GraphForEachLeaf( pGraph, pNode, i )
    {
        pNode->pFunc = Vec_PtrEntry( vFanins, i );
        pNode->nLat2 = Ivy_LeafLat( pCut->pArray[(int)pPerm[i]] );
    }
    // propagate latches through the nodes
    Dec_GraphForEachNode( pGraph, pNode, i )
    {
        // get the children of this node
        pNode0 = Dec_GraphNode( pGraph, pNode->eEdge0.Node );
        pNode1 = Dec_GraphNode( pGraph, pNode->eEdge1.Node );
        // distribute the latches
        pNode->nLat2 = IVY_MIN( pNode0->nLat2, pNode1->nLat2 );
        pNode->nLat0 = pNode0->nLat2 - pNode->nLat2;
        pNode->nLat1 = pNode1->nLat2 - pNode->nLat2;
    }
}

/**Function*************************************************************

  Synopsis    [Counts the number of new nodes added when using this graph.]

  Description [AIG nodes for the fanins should be assigned to pNode->pFunc 
  of the leaves of the graph before calling this procedure. 
  Returns -1 if the number of nodes and levels exceeded the given limit or 
  the number of levels exceeded the maximum allowed level.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_GraphToNetworkSeqCountSeq( Ivy_Man_t * p, Ivy_Obj_t * pRoot, Dec_Graph_t * pGraph, int NodeMax )
{
    Dec_Node_t * pNode, * pNode0, * pNode1;
    Ivy_Obj_t * pAnd, * pAnd0, * pAnd1;
    int i, k, Counter, fCompl;
    // check for constant function or a literal
    if ( Dec_GraphIsConst(pGraph) || Dec_GraphIsVar(pGraph) )
        return 0;
    // compute the AIG size after adding the internal nodes
    Counter = 0;
    Dec_GraphForEachNode( pGraph, pNode, i )
    {
        // get the children of this node
        pNode0 = Dec_GraphNode( pGraph, pNode->eEdge0.Node );
        pNode1 = Dec_GraphNode( pGraph, pNode->eEdge1.Node );
        // get the AIG nodes corresponding to the children 
        pAnd0 = (Ivy_Obj_t *)pNode0->pFunc; 
        pAnd1 = (Ivy_Obj_t *)pNode1->pFunc; 
        // skip the latches
        for ( k = 0; pAnd0 && k < (int)pNode->nLat0; k++ )
        {
            fCompl = Ivy_IsComplement(pAnd0);
            pAnd0 = Ivy_TableLookup( p, Ivy_ObjCreateGhost(p, Ivy_Regular(pAnd0), NULL, IVY_LATCH, IVY_INIT_DC) );
            if ( pAnd0 )
                pAnd0 = Ivy_NotCond( pAnd0, fCompl );
        }
        for ( k = 0; pAnd1 && k < (int)pNode->nLat1; k++ )
        {
            fCompl = Ivy_IsComplement(pAnd1);
            pAnd1 = Ivy_TableLookup( p, Ivy_ObjCreateGhost(p, Ivy_Regular(pAnd1), NULL, IVY_LATCH, IVY_INIT_DC) );
            if ( pAnd1 )
                pAnd1 = Ivy_NotCond( pAnd1, fCompl );
        }
        // get the new node
        if ( pAnd0 && pAnd1 )
        {
            // if they are both present, find the resulting node
            pAnd0 = Ivy_NotCond( pAnd0, pNode->eEdge0.fCompl );
            pAnd1 = Ivy_NotCond( pAnd1, pNode->eEdge1.fCompl );
            assert( !Ivy_ObjIsLatch(Ivy_Regular(pAnd0)) || !Ivy_ObjIsLatch(Ivy_Regular(pAnd1)) );
            if ( Ivy_Regular(pAnd0) == Ivy_Regular(pAnd1) || Ivy_ObjIsConst1(Ivy_Regular(pAnd0)) || Ivy_ObjIsConst1(Ivy_Regular(pAnd1)) )
                pAnd = Ivy_And( p, pAnd0, pAnd1 );
            else
                pAnd = Ivy_TableLookup( p, Ivy_ObjCreateGhost(p, pAnd0, pAnd1, IVY_AND, IVY_INIT_NONE) );
            // return -1 if the node is the same as the original root
            if ( Ivy_Regular(pAnd) == pRoot )
                return -1;
        }
        else
            pAnd = NULL;
        // count the number of added nodes
        if ( pAnd == NULL || Ivy_ObjIsTravIdCurrent(p, Ivy_Regular(pAnd)) )
        {
            if ( ++Counter > NodeMax )
                return -1;
        }
        pNode->pFunc = pAnd;
    }
    return Counter;
}


/**Function*************************************************************

  Synopsis    [Transforms the decomposition graph into the AIG.]

  Description [AIG nodes for the fanins should be assigned to pNode->pFunc
  of the leaves of the graph before calling this procedure.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ivy_Obj_t * Ivy_GraphToNetworkSeq( Ivy_Man_t * p, Dec_Graph_t * pGraph )
{
    Ivy_Obj_t * pAnd0, * pAnd1;
    Dec_Node_t * pNode = NULL; // Suppress "might be used uninitialized"
    int i, k;
    // check for constant function
    if ( Dec_GraphIsConst(pGraph) )
        return Ivy_NotCond( Ivy_ManConst1(p), Dec_GraphIsComplement(pGraph) );
    // check for a literal
    if ( Dec_GraphIsVar(pGraph) )
    {
        // get the variable node
        pNode = Dec_GraphVar(pGraph);
        // add the remaining latches
        for ( k = 0; k < (int)pNode->nLat2; k++ )
            pNode->pFunc = Ivy_Latch( p, (Ivy_Obj_t *)pNode->pFunc, IVY_INIT_DC );
        return Ivy_NotCond( (Ivy_Obj_t *)pNode->pFunc, Dec_GraphIsComplement(pGraph) );
    }
    // build the AIG nodes corresponding to the AND gates of the graph
    Dec_GraphForEachNode( pGraph, pNode, i )
    {
        pAnd0 = Ivy_NotCond( (Ivy_Obj_t *)Dec_GraphNode(pGraph, pNode->eEdge0.Node)->pFunc, pNode->eEdge0.fCompl ); 
        pAnd1 = Ivy_NotCond( (Ivy_Obj_t *)Dec_GraphNode(pGraph, pNode->eEdge1.Node)->pFunc, pNode->eEdge1.fCompl ); 
        // add the latches
        for ( k = 0; k < (int)pNode->nLat0; k++ )
            pAnd0 = Ivy_Latch( p, pAnd0, IVY_INIT_DC );
        for ( k = 0; k < (int)pNode->nLat1; k++ )
            pAnd1 = Ivy_Latch( p, pAnd1, IVY_INIT_DC );
        // create the node
        pNode->pFunc = Ivy_And( p, pAnd0, pAnd1 );
    }
    // add the remaining latches
    for ( k = 0; k < (int)pNode->nLat2; k++ )
        pNode->pFunc = Ivy_Latch( p, (Ivy_Obj_t *)pNode->pFunc, IVY_INIT_DC );
    // complement the result if necessary
    return Ivy_NotCond( (Ivy_Obj_t *)pNode->pFunc, Dec_GraphIsComplement(pGraph) );
}

/**Function*************************************************************

  Synopsis    [Replaces MFFC of the node by the new factored form.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_GraphUpdateNetworkSeq( Ivy_Man_t * p, Ivy_Obj_t * pRoot, Dec_Graph_t * pGraph, int nGain )
{
    Ivy_Obj_t * pRootNew;
    int nNodesNew, nNodesOld;
    nNodesOld = Ivy_ManNodeNum(p);
    // create the new structure of nodes
    pRootNew = Ivy_GraphToNetworkSeq( p, pGraph );
    Ivy_ObjReplace( p, pRoot, pRootNew, 1, 0, 0 );
    // compare the gains
    nNodesNew = Ivy_ManNodeNum(p);
    assert( nGain <= nNodesOld - nNodesNew );
    // propagate the buffer
    Ivy_ManPropagateBuffers( p, 0 );
}









/**Function*************************************************************

  Synopsis    [Computes the truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Ivy_CutGetTruth_rec( Ivy_Man_t * p, int Leaf, int * pNums, int nNums )
{
    static unsigned uMasks[5] = { 0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000 };
    unsigned uTruth0, uTruth1;
    Ivy_Obj_t * pObj;
    int i;
    for ( i = 0; i < nNums; i++ )
        if ( Leaf == pNums[i] )
            return uMasks[i];
    pObj = Ivy_ManObj( p, Ivy_LeafId(Leaf) );
    if ( Ivy_ObjIsLatch(pObj) )
    {
        assert( !Ivy_ObjFaninC0(pObj) );
        Leaf = Ivy_LeafCreate( Ivy_ObjFaninId0(pObj), Ivy_LeafLat(Leaf) + 1 );
        return Ivy_CutGetTruth_rec( p, Leaf, pNums, nNums );
    }
    assert( Ivy_ObjIsNode(pObj) || Ivy_ObjIsBuf(pObj) );
    Leaf = Ivy_LeafCreate( Ivy_ObjFaninId0(pObj), Ivy_LeafLat(Leaf) );
    uTruth0 = Ivy_CutGetTruth_rec( p, Leaf, pNums, nNums );
    if ( Ivy_ObjFaninC0(pObj) )
        uTruth0 = ~uTruth0;
    if ( Ivy_ObjIsBuf(pObj) )
        return uTruth0;
    Leaf = Ivy_LeafCreate( Ivy_ObjFaninId1(pObj), Ivy_LeafLat(Leaf) );
    uTruth1 = Ivy_CutGetTruth_rec( p, Leaf, pNums, nNums );
    if ( Ivy_ObjFaninC1(pObj) )
        uTruth1 = ~uTruth1;
    return uTruth0 & uTruth1;
}


/**Function*************************************************************

  Synopsis    [Computes the truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Ivy_CutGetTruth( Ivy_Man_t * p, Ivy_Obj_t * pObj, int * pNums, int nNums )
{
    assert( Ivy_ObjIsNode(pObj) );
    assert( nNums < 6 );
    return Ivy_CutGetTruth_rec( p, Ivy_LeafCreate(pObj->Id, 0), pNums, nNums );
}





/**Function*************************************************************

  Synopsis    [Returns 1 if the cut can be constructed; 0 otherwise.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Ivy_CutPrescreen( Ivy_Cut_t * pCut, int Id0, int Id1 )
{
    int i;
    if ( pCut->nSize < pCut->nSizeMax )
        return 1;
    for ( i = 0; i < pCut->nSize; i++ )
        if ( pCut->pArray[i] == Id0 || pCut->pArray[i] == Id1 )
            return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Derives new cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Ivy_CutDeriveNew2( Ivy_Cut_t * pCut, Ivy_Cut_t * pCutNew, int IdOld, int IdNew0, int IdNew1 )
{
    unsigned uHash = 0;
    int i, k;
    assert( pCut->nSize > 0 );
    assert( IdNew0 < IdNew1 );
    for ( i = k = 0; i < pCut->nSize; i++ )
    {
        if ( pCut->pArray[i] == IdOld )
            continue;
        if ( IdNew0 >= 0 )
        {
            if ( IdNew0 <= pCut->pArray[i] )
            {
                if ( IdNew0 < pCut->pArray[i] )
                {
                    if ( k == pCut->nSizeMax )
                        return 0;
                    pCutNew->pArray[ k++ ] = IdNew0;
                    uHash |= Ivy_CutHashValue( IdNew0 );
                }
                IdNew0 = -1;
            }
        }
        if ( IdNew1 >= 0 )
        {
            if ( IdNew1 <= pCut->pArray[i] )
            {
                if ( IdNew1 < pCut->pArray[i] )
                {
                    if ( k == pCut->nSizeMax )
                        return 0;
                    pCutNew->pArray[ k++ ] = IdNew1;
                    uHash |= Ivy_CutHashValue( IdNew1 );
                }
                IdNew1 = -1;
            }
        }
        if ( k == pCut->nSizeMax )
            return 0;
        pCutNew->pArray[ k++ ] = pCut->pArray[i];
        uHash |= Ivy_CutHashValue( pCut->pArray[i] );
    }
    if ( IdNew0 >= 0 )
    {
        if ( k == pCut->nSizeMax )
            return 0;
        pCutNew->pArray[ k++ ] = IdNew0;
        uHash |= Ivy_CutHashValue( IdNew0 );
    }
    if ( IdNew1 >= 0 )
    {
        if ( k == pCut->nSizeMax )
            return 0;
        pCutNew->pArray[ k++ ] = IdNew1;
        uHash |= Ivy_CutHashValue( IdNew1 );
    }
    pCutNew->nSize = k;
    pCutNew->uHash = uHash;
    assert( pCutNew->nSize <= pCut->nSizeMax );
    for ( i = 1; i < pCutNew->nSize; i++ )
        assert( pCutNew->pArray[i-1] < pCutNew->pArray[i] );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Derives new cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Ivy_CutDeriveNew( Ivy_Cut_t * pCut, Ivy_Cut_t * pCutNew, int IdOld, int IdNew0, int IdNew1 )
{
    unsigned uHash = 0;
    int i, k; 
    assert( pCut->nSize > 0 );
    assert( IdNew0 < IdNew1 );
    for ( i = k = 0; i < pCut->nSize; i++ )
    {
        if ( pCut->pArray[i] == IdOld )
            continue;
        if ( IdNew0 <= pCut->pArray[i] )
        {
            if ( IdNew0 < pCut->pArray[i] )
            {
                pCutNew->pArray[ k++ ] = IdNew0;
                uHash |= Ivy_CutHashValue( IdNew0 );
            }
            IdNew0 = 0x7FFFFFFF;
        }
        if ( IdNew1 <= pCut->pArray[i] )
        {
            if ( IdNew1 < pCut->pArray[i] )
            {
                pCutNew->pArray[ k++ ] = IdNew1;
                uHash |= Ivy_CutHashValue( IdNew1 );
            }
            IdNew1 = 0x7FFFFFFF;
        }
        pCutNew->pArray[ k++ ] = pCut->pArray[i];
        uHash |= Ivy_CutHashValue( pCut->pArray[i] );
    }
    if ( IdNew0 < 0x7FFFFFFF )
    {
        pCutNew->pArray[ k++ ] = IdNew0;
        uHash |= Ivy_CutHashValue( IdNew0 );
    }
    if ( IdNew1 < 0x7FFFFFFF )
    {
        pCutNew->pArray[ k++ ] = IdNew1;
        uHash |= Ivy_CutHashValue( IdNew1 );
    }
    pCutNew->nSize = k;
    pCutNew->uHash = uHash;
    assert( pCutNew->nSize <= pCut->nSizeMax );
//    for ( i = 1; i < pCutNew->nSize; i++ )
//        assert( pCutNew->pArray[i-1] < pCutNew->pArray[i] );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Find the hash value of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Ivy_NodeCutHash( Ivy_Cut_t * pCut )
{
    int i;
    pCut->uHash = 0;
    for ( i = 0; i < pCut->nSize; i++ )
        pCut->uHash |= (1 << (pCut->pArray[i] % 31));
    return pCut->uHash;
}

/**Function*************************************************************

  Synopsis    [Derives new cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Ivy_CutDeriveNew3( Ivy_Cut_t * pCut, Ivy_Cut_t * pCutNew, int IdOld, int IdNew0, int IdNew1 )
{
    int i, k; 
    assert( pCut->nSize > 0 );
    assert( IdNew0 < IdNew1 );
    for ( i = k = 0; i < pCut->nSize; i++ )
    {
        if ( pCut->pArray[i] == IdOld )
            continue;
        if ( IdNew0 <= pCut->pArray[i] )
        {
            if ( IdNew0 < pCut->pArray[i] )
                pCutNew->pArray[ k++ ] = IdNew0;
            IdNew0 = 0x7FFFFFFF;
        }
        if ( IdNew1 <= pCut->pArray[i] )
        {
            if ( IdNew1 < pCut->pArray[i] )
                pCutNew->pArray[ k++ ] = IdNew1;
            IdNew1 = 0x7FFFFFFF;
        }
        pCutNew->pArray[ k++ ] = pCut->pArray[i];
    }
    if ( IdNew0 < 0x7FFFFFFF )
        pCutNew->pArray[ k++ ] = IdNew0;
    if ( IdNew1 < 0x7FFFFFFF )
        pCutNew->pArray[ k++ ] = IdNew1;
    pCutNew->nSize = k;
    assert( pCutNew->nSize <= pCut->nSizeMax );
    Ivy_NodeCutHash( pCutNew );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if pDom is contained in pCut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Ivy_CutCheckDominance( Ivy_Cut_t * pDom, Ivy_Cut_t * pCut )
{
    int i, k;
    for ( i = 0; i < pDom->nSize; i++ )
    {
        assert( i==0 || pDom->pArray[i-1] < pDom->pArray[i] );
        for ( k = 0; k < pCut->nSize; k++ )
            if ( pDom->pArray[i] == pCut->pArray[k] )
                break;
        if ( k == pCut->nSize ) // node i in pDom is not contained in pCut
            return 0;
    }
    // every node in pDom is contained in pCut
    return 1;
}

/**Function*************************************************************

  Synopsis    [Check if the cut exists.]

  Description [Returns 1 if the cut exists.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_CutFindOrAddFilter( Ivy_Store_t * pCutStore, Ivy_Cut_t * pCutNew )
{
    Ivy_Cut_t * pCut;
    int i, k;
    assert( pCutNew->uHash );
    // try to find the cut
    for ( i = 0; i < pCutStore->nCuts; i++ )
    {
        pCut = pCutStore->pCuts + i;
        if ( pCut->nSize == 0 )
            continue;
        if ( pCut->nSize == pCutNew->nSize )
        {
            if ( pCut->uHash == pCutNew->uHash )
            {
                for ( k = 0; k < pCutNew->nSize; k++ )
                    if ( pCut->pArray[k] != pCutNew->pArray[k] )
                        break;
                if ( k == pCutNew->nSize )
                    return 1;
            }
            continue;
        }
        if ( pCut->nSize < pCutNew->nSize )
        {
            // skip the non-contained cuts
            if ( (pCut->uHash & pCutNew->uHash) != pCut->uHash )
                continue;
            // check containment seriously
            if ( Ivy_CutCheckDominance( pCut, pCutNew ) )
                return 1;
            continue;
        }
        // check potential containment of other cut

        // skip the non-contained cuts
        if ( (pCut->uHash & pCutNew->uHash) != pCutNew->uHash )
            continue;
        // check containment seriously
        if ( Ivy_CutCheckDominance( pCutNew, pCut ) )
        {
            // remove the current cut
            pCut->nSize = 0;
        }
    }
    assert( pCutStore->nCuts < pCutStore->nCutsMax );
    // add the cut
    pCut = pCutStore->pCuts + pCutStore->nCuts++;
    *pCut = *pCutNew;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Compresses the cut representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_CutCompactAll( Ivy_Store_t * pCutStore )
{
    Ivy_Cut_t * pCut;
    int i, k;
    pCutStore->nCutsM = 0;
    for ( i = k = 0; i < pCutStore->nCuts; i++ )
    {
        pCut = pCutStore->pCuts + i;
        if ( pCut->nSize == 0 )
            continue;
        if ( pCut->nSize < pCut->nSizeMax )
            pCutStore->nCutsM++;
        pCutStore->pCuts[k++] = *pCut;
    }
    pCutStore->nCuts = k;
}

/**Function*************************************************************

  Synopsis    [Print the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_CutPrintForNode( Ivy_Cut_t * pCut )
{
    int i;
    assert( pCut->nSize > 0 );
    printf( "%d : {", pCut->nSize );
    for ( i = 0; i < pCut->nSize; i++ )
        printf( " %d", pCut->pArray[i] );
    printf( " }\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_CutPrintForNodes( Ivy_Store_t * pCutStore )
{
    int i;
    printf( "Node %d\n", pCutStore->pCuts[0].pArray[0] );
    for ( i = 0; i < pCutStore->nCuts; i++ )
        Ivy_CutPrintForNode( pCutStore->pCuts + i );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     [] 

***********************************************************************/
static inline int Ivy_CutReadLeaf( Ivy_Obj_t * pFanin )
{
    int nLats, iLeaf;
    assert( !Ivy_IsComplement(pFanin) );
    if ( !Ivy_ObjIsLatch(pFanin) )
        return Ivy_LeafCreate( pFanin->Id, 0 );
    iLeaf = Ivy_CutReadLeaf(Ivy_ObjFanin0(pFanin));
    nLats = Ivy_LeafLat(iLeaf);
    assert( nLats < IVY_LEAF_MASK );
    return 1 + iLeaf;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ivy_Store_t * Ivy_CutComputeForNode( Ivy_Man_t * p, Ivy_Obj_t * pObj, int nLeaves )
{
    static Ivy_Store_t CutStore, * pCutStore = &CutStore;
    Ivy_Cut_t CutNew, * pCutNew = &CutNew, * pCut;
    Ivy_Obj_t * pLeaf;
    int i, k, Temp, nLats, iLeaf0, iLeaf1;

    assert( nLeaves <= IVY_CUT_INPUT );

    // start the structure
    pCutStore->nCuts = 0;
    pCutStore->nCutsMax = IVY_CUT_LIMIT;
    // start the trivial cut
    pCutNew->uHash = 0;
    pCutNew->nSize = 1;
    pCutNew->nSizeMax = nLeaves;
    pCutNew->pArray[0] = Ivy_LeafCreate( pObj->Id, 0 );
    pCutNew->uHash = Ivy_CutHashValue( pCutNew->pArray[0] );
    // add the trivial cut
    pCutStore->pCuts[pCutStore->nCuts++] = *pCutNew;
    assert( pCutStore->nCuts == 1 );

    // explore the cuts
    for ( i = 0; i < pCutStore->nCuts; i++ )
    {
        // expand this cut
        pCut = pCutStore->pCuts + i;
        if ( pCut->nSize == 0 )
            continue;
        for ( k = 0; k < pCut->nSize; k++ )
        {
            pLeaf = Ivy_ManObj( p, Ivy_LeafId(pCut->pArray[k]) );
            if ( Ivy_ObjIsCi(pLeaf) || Ivy_ObjIsConst1(pLeaf) )
                continue;
            assert( Ivy_ObjIsNode(pLeaf) );
            nLats = Ivy_LeafLat(pCut->pArray[k]);

            // get the fanins fanins
            iLeaf0 = Ivy_CutReadLeaf( Ivy_ObjFanin0(pLeaf) );
            iLeaf1 = Ivy_CutReadLeaf( Ivy_ObjFanin1(pLeaf) );
            assert( nLats + Ivy_LeafLat(iLeaf0) < IVY_LEAF_MASK && nLats + Ivy_LeafLat(iLeaf1) < IVY_LEAF_MASK );
            iLeaf0 = nLats + iLeaf0;
            iLeaf1 = nLats + iLeaf1;
            if ( !Ivy_CutPrescreen( pCut, iLeaf0, iLeaf1 ) )
                continue;
            // the given cut exist
            if ( iLeaf0 > iLeaf1 )
                Temp = iLeaf0, iLeaf0 = iLeaf1, iLeaf1 = Temp;
            // create the new cut
            if ( !Ivy_CutDeriveNew( pCut, pCutNew, pCut->pArray[k], iLeaf0, iLeaf1 ) )
                continue;
            // add the cut
            Ivy_CutFindOrAddFilter( pCutStore, pCutNew );
            if ( pCutStore->nCuts == IVY_CUT_LIMIT )
                break;
        }
        if ( pCutStore->nCuts == IVY_CUT_LIMIT )
            break;
    }
    if ( pCutStore->nCuts == IVY_CUT_LIMIT )
        pCutStore->fSatur = 1;
    else
        pCutStore->fSatur = 0;
//    printf( "%d ", pCutStore->nCuts );
    Ivy_CutCompactAll( pCutStore );
//    printf( "%d \n", pCutStore->nCuts );
//    Ivy_CutPrintForNodes( pCutStore );
    return pCutStore;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_CutComputeAll( Ivy_Man_t * p, int nInputs )
{
    Ivy_Store_t * pStore;
    Ivy_Obj_t * pObj;
    int i, nCutsTotal, nCutsTotalM, nNodeTotal, nNodeOver;
    abctime clk = Abc_Clock();
    if ( nInputs > IVY_CUT_INPUT )
    {
        printf( "Cannot compute cuts for more than %d inputs.\n", IVY_CUT_INPUT );
        return;
    }
    nNodeTotal = nNodeOver = 0;
    nCutsTotal = nCutsTotalM = -Ivy_ManNodeNum(p);
    Ivy_ManForEachObj( p, pObj, i )
    {
        if ( !Ivy_ObjIsNode(pObj) )
            continue;
        pStore = Ivy_CutComputeForNode( p, pObj, nInputs );
        nCutsTotal  += pStore->nCuts;
        nCutsTotalM += pStore->nCutsM;
        nNodeOver   += pStore->fSatur;
        nNodeTotal++;
    }
    printf( "All = %6d. Minus = %6d. Triv = %6d.   Node = %6d. Satur = %6d.  ", 
        nCutsTotal, nCutsTotalM, Ivy_ManPiNum(p) + Ivy_ManNodeNum(p), nNodeTotal, nNodeOver );
    ABC_PRT( "Time", Abc_Clock() - clk );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

