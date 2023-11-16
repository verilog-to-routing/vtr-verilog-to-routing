/**CFile****************************************************************

  FileName    [abcRestruct.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcRestruct.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "bool/dec/dec.h"
#include "opt/cut/cut.h"

#ifdef ABC_USE_CUDD
#include "bdd/extrab/extraBdd.h"
#include "bdd/dsd/dsd.h"
#endif

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
  
#ifdef ABC_USE_CUDD

#define RST_RANDOM_UNSIGNED   ((((unsigned)rand()) << 24) ^ (((unsigned)rand()) << 12) ^ ((unsigned)rand()))

typedef struct Abc_ManRst_t_   Abc_ManRst_t;
struct Abc_ManRst_t_
{
    // the network
    Abc_Ntk_t *      pNtk;              // the network for restructuring
    // user specified parameters
    int              nCutMax;           // the limit on the size of the supernode
    int              fUpdateLevel;      // turns on watching the number of levels
    int              fUseZeros;         // turns on zero-cost replacements
    int              fVerbose;          // the verbosity flag
    // internal data structures
    DdManager *      dd;                // the BDD manager
    Dsd_Manager_t *  pManDsd;           // the DSD manager
    Vec_Ptr_t *      vVisited;          // temporary
    Vec_Ptr_t *      vLeaves;           // temporary
    Vec_Ptr_t *      vDecs;             // temporary
    Vec_Ptr_t *      vTemp;             // temporary
    Vec_Int_t *      vSims;             // temporary
    Vec_Int_t *      vRands;            // temporary
    Vec_Int_t *      vOnes;             // temporary
    Vec_Int_t *      vBinate;           // temporary
    Vec_Int_t *      vTwos;             // temporary
    // node statistics
    int              nLastGain;
    int              nCutsConsidered;
    int              nCutsExplored;
    int              nNodesConsidered;
    int              nNodesRestructured;
    int              nNodesGained;
    // runtime statistics
    int              timeCut;
    int              timeBdd;
    int              timeDsd;
    int              timeEval;
    int              timeRes;
    int              timeNtk;
    int              timeTotal;
};

static Dec_Graph_t * Abc_NodeResubstitute( Abc_ManRst_t * p, Abc_Obj_t * pNode, Cut_Cut_t * pCutList );

static Dec_Graph_t * Abc_NodeRestructure( Abc_ManRst_t * p, Abc_Obj_t * pNode, Cut_Cut_t * pCutList );
static Dec_Graph_t * Abc_NodeRestructureCut( Abc_ManRst_t * p, Abc_Obj_t * pNode, Cut_Cut_t * pCut );
static Dec_Graph_t * Abc_NodeEvaluateDsd( Abc_ManRst_t * pManRst, Dsd_Node_t * pNodeDsd, Abc_Obj_t * pRoot, int Required, int nNodesSaved, int * pnNodesAdded );

static Cut_Man_t * Abc_NtkStartCutManForRestruct( Abc_Ntk_t * pNtk, int nCutMax, int fDag );
static Abc_ManRst_t * Abc_NtkManRstStart( int nCutMax, int fUpdateLevel, int fUseZeros, int fVerbose );
static void Abc_NtkManRstStop( Abc_ManRst_t * p );
static void Abc_NtkManRstPrintStats( Abc_ManRst_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Implements AIG restructuring.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRestructure( Abc_Ntk_t * pNtk, int nCutMax, int fUpdateLevel, int fUseZeros, int fVerbose )
{
    extern int           Dec_GraphUpdateNetwork( Abc_Obj_t * pRoot, Dec_Graph_t * pGraph, int fUpdateLevel, int nGain );
    ProgressBar * pProgress;
    Abc_ManRst_t * pManRst;
    Cut_Man_t * pManCut;
    Cut_Cut_t * pCutList;
    Dec_Graph_t * pGraph;
    Abc_Obj_t * pNode;
    abctime clk, clkStart = Abc_Clock();
    int fMulti = 1;
    int fResub = 0;
    int i, nNodes;

    assert( Abc_NtkIsStrash(pNtk) );
    // cleanup the AIG
    Abc_AigCleanup((Abc_Aig_t *)pNtk->pManFunc);
    Abc_NtkCleanCopy(pNtk);

    // compute the reverse levels if level update is requested
    if ( fUpdateLevel )
        Abc_NtkStartReverseLevels( pNtk, 0 );

    // start the restructuring manager
    pManRst = Abc_NtkManRstStart( nCutMax, fUpdateLevel, fUseZeros, fVerbose );
    pManRst->pNtk = pNtk;
    // start the cut manager
clk = Abc_Clock();
    pManCut = Abc_NtkStartCutManForRestruct( pNtk, nCutMax, fMulti );
pManRst->timeCut += Abc_Clock() - clk;
//    pNtk->pManCut = pManCut;

    // resynthesize each node once
    nNodes = Abc_NtkObjNumMax(pNtk);
    pProgress = Extra_ProgressBarStart( stdout, nNodes );
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        // skip the constant node
//        if ( Abc_NodeIsConst(pNode) )
//            continue;
        // skip persistant nodes
        if ( Abc_NodeIsPersistant(pNode) )
            continue;
        // skip the node if it is inside the tree
//        if ( Abc_ObjFanoutNum(pNode) < 2 )
//            continue;
        // skip the nodes with too many fanouts
        if ( Abc_ObjFanoutNum(pNode) > 1000 )
            continue;
        // stop if all nodes have been tried once
        if ( i >= nNodes )
            break;
        // get the cuts for the given node
clk = Abc_Clock();
        pCutList = (Cut_Cut_t *)Abc_NodeGetCutsRecursive( pManCut, pNode, fMulti, 0 ); 
pManRst->timeCut += Abc_Clock() - clk;

        // perform restructuring
clk = Abc_Clock();
        if ( fResub )
            pGraph = Abc_NodeResubstitute( pManRst, pNode, pCutList );
        else
            pGraph = Abc_NodeRestructure( pManRst, pNode, pCutList );
pManRst->timeRes += Abc_Clock() - clk;
        if ( pGraph == NULL )
            continue;

        // acceptable replacement found, update the graph
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pGraph, fUpdateLevel, pManRst->nLastGain );
pManRst->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pGraph );
    }
    Extra_ProgressBarStop( pProgress );
pManRst->timeTotal = Abc_Clock() - clkStart;

    // print statistics of the manager
//    if ( fVerbose )
        Abc_NtkManRstPrintStats( pManRst );
    // delete the managers
    Cut_ManStop( pManCut );
    Abc_NtkManRstStop( pManRst );
    // put the nodes into the DFS order and reassign their IDs
    Abc_NtkReassignIds( pNtk );
//    Abc_AigCheckFaninOrder( pNtk->pManFunc );
    // fix the levels
    if ( fUpdateLevel )
        Abc_NtkStopReverseLevels( pNtk );
    else
        Abc_NtkLevel( pNtk );
    // check
    if ( !Abc_NtkCheck( pNtk ) )
    {
        printf( "Abc_NtkRefactor: The network check has failed.\n" );
        return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_RestructNodeDivisors( Abc_ManRst_t * p, Abc_Obj_t * pRoot, int nNodesSaved )
{
    Abc_Obj_t * pNode, * pFanout;//, * pFanin;
    int i, k;
    // start with the leaves
    Vec_PtrClear( p->vDecs );
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vLeaves, pNode, i )
    {
        Vec_PtrPush( p->vDecs, pNode );
        assert( pNode->fMarkC == 0 );
        pNode->fMarkC = 1;
    }
    // explore the fanouts
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vDecs, pNode, i )
    {
        // if the fanout has both fanins in the set, add it
        Abc_ObjForEachFanout( pNode, pFanout, k )
        {
            if ( pFanout->fMarkC || Abc_ObjIsPo(pFanout) )
                continue;
            if ( Abc_ObjFanin0(pFanout)->fMarkC && Abc_ObjFanin1(pFanout)->fMarkC )
            {
                Vec_PtrPush( p->vDecs, pFanout );
                pFanout->fMarkC = 1;
            }
        }
    }
    // unmark the nodes
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vDecs, pNode, i )
        pNode->fMarkC = 0;
/*
    // print the nodes
    Vec_PtrForEachEntryStart( Abc_Obj_t *, p->vDecs, pNode, i, Vec_PtrSize(p->vLeaves) )
    {
        printf( "%2d %s = ", i, Abc_NodeIsTravIdCurrent(pNode)? "*" : " " );
        // find the first fanin
        Vec_PtrForEachEntry( Abc_Obj_t *, p->vDecs, pFanin, k )
            if ( Abc_ObjFanin0(pNode) == pFanin )
                break;
        if ( k < Vec_PtrSize(p->vLeaves) )
            printf( "%c", 'a' + k );
        else
            printf( "%d", k );
        printf( "%s ", Abc_ObjFaninC0(pNode)? "\'" : "" );
        // find the second fanin
        Vec_PtrForEachEntry( Abc_Obj_t *, p->vDecs, pFanin, k )
            if ( Abc_ObjFanin1(pNode) == pFanin )
                break;
        if ( k < Vec_PtrSize(p->vLeaves) )
            printf( "%c", 'a' + k );
        else
            printf( "%d", k );
        printf( "%s ", Abc_ObjFaninC1(pNode)? "\'" : "" );
        printf( "\n" );
    }
*/
    printf( "%d\n", Vec_PtrSize(p->vDecs)-nNodesSaved-Vec_PtrSize(p->vLeaves) );
}


/**Function*************************************************************

  Synopsis    [Starts the cut manager for rewriting.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dec_Graph_t * Abc_NodeRestructure( Abc_ManRst_t * p, Abc_Obj_t * pNode, Cut_Cut_t * pCutList )
{
    Dec_Graph_t * pGraph;
    Cut_Cut_t * pCut;
//    int nCuts;
    p->nNodesConsidered++;
/*
    // count the number of cuts with four inputs or more
    nCuts = 0;
    for ( pCut = pCutList; pCut; pCut = pCut->pNext )
        nCuts += (int)(pCut->nLeaves > 3);
    printf( "-----------------------------------\n" );
    printf( "Node %6d : Factor-cuts = %5d.\n", pNode->Id, nCuts );
*/
    // go through the interesting cuts
    for ( pCut = pCutList; pCut; pCut = pCut->pNext )
    {
        if ( pCut->nLeaves < 4 )
            continue;
        if ( (pGraph = Abc_NodeRestructureCut( p, pNode, pCut )) )
            return pGraph;
    }
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Starts the cut manager for rewriting.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dec_Graph_t * Abc_NodeRestructureCut( Abc_ManRst_t * p, Abc_Obj_t * pRoot, Cut_Cut_t * pCut )
{
    extern DdNode * Abc_NodeConeBdd( DdManager * dd, DdNode ** pbVars, Abc_Obj_t * pNode, Vec_Ptr_t * vFanins, Vec_Ptr_t * vVisited );
    Dec_Graph_t * pGraph;
    Dsd_Node_t * pNodeDsd;
    Abc_Obj_t * pLeaf;
    DdNode * bFunc;
    int nNodesSaved, nNodesAdded;
    int Required, nMaxSize, clk, i;
    int fVeryVerbose = 0;

    p->nCutsConsidered++;

    // get the required time for the node
    Required = p->fUpdateLevel? Abc_ObjRequiredLevel(pRoot) : ABC_INFINITY;

    // collect the leaves of the cut
    Vec_PtrClear( p->vLeaves );
    for ( i = 0; i < (int)pCut->nLeaves; i++ )
    {
        pLeaf = Abc_NtkObj(pRoot->pNtk, pCut->pLeaves[i]);
        if ( pLeaf == NULL )  // the so-called "bad cut phenomenon" is due to removed nodes
            return NULL;
        Vec_PtrPush( p->vLeaves, pLeaf );
    }

clk = Abc_Clock();
    // collect the internal nodes of the cut
//    Abc_NodeConeCollect( &pRoot, 1, p->vLeaves, p->vVisited, 0 );
    // derive the BDD of the cut
    bFunc = Abc_NodeConeBdd( p->dd, p->dd->vars, pRoot, p->vLeaves, p->vVisited );  Cudd_Ref( bFunc );
p->timeBdd += Abc_Clock() - clk;

    // consider the special case, when the function is a constant
    if ( Cudd_IsConstant(bFunc) )
    {
        p->nLastGain = Abc_NodeMffcSize( pRoot );
        p->nNodesGained += p->nLastGain;
        p->nNodesRestructured++;
        Cudd_RecursiveDeref( p->dd, bFunc );
        if ( Cudd_IsComplement(bFunc) )
            return Dec_GraphCreateConst0();
        return Dec_GraphCreateConst1();
    }

clk = Abc_Clock();
    // try disjoint support decomposition
    pNodeDsd = Dsd_DecomposeOne( p->pManDsd, bFunc );
p->timeDsd += Abc_Clock() - clk;

    // skip nodes with non-decomposable blocks
    Dsd_TreeNodeGetInfoOne( pNodeDsd, NULL, &nMaxSize );
    if ( nMaxSize > 3 )
    {
        Cudd_RecursiveDeref( p->dd, bFunc );
        return NULL;
    }


/*
    // skip nodes that cannot be improved
    if ( Vec_PtrSize(p->vVisited) <= Dsd_TreeGetAigCost(pNodeDsd) )
    {
        Cudd_RecursiveDeref( p->dd, bFunc );
        return NULL;
    }
*/

    p->nCutsExplored++;

    // mark the fanin boundary 
    // (can mark only essential fanins, belonging to bNodeFunc!)
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vLeaves, pLeaf, i )
        pLeaf->vFanouts.nSize++;
    // label MFFC with current traversal ID
    Abc_NtkIncrementTravId( pRoot->pNtk );
    nNodesSaved = Abc_NodeMffcLabelAig( pRoot );
    // unmark the fanin boundary and set the fanins as leaves in the form
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vLeaves, pLeaf, i )
        pLeaf->vFanouts.nSize--;
/*
    if ( nNodesSaved < 3 )
    {
        Cudd_RecursiveDeref( p->dd, bFunc );
        return NULL;
    }
*/

/* 
    printf( "%5d : Cut-size = %d.  Old AIG = %2d.  New AIG = %2d.  Old MFFC = %2d.\n",
        pRoot->Id, pCut->nLeaves, Vec_PtrSize(p->vVisited), Dsd_TreeGetAigCost(pNodeDsd), 
        nNodesSaved );
    Dsd_NodePrint( stdout, pNodeDsd );

    Abc_RestructNodeDivisors( p, pRoot );

    if ( pRoot->Id == 433 )
    {
        int x = 0;
    }
*/
//    Abc_RestructNodeDivisors( p, pRoot, nNodesSaved );


    // detect how many new nodes will be added (while taking into account reused nodes)
clk = Abc_Clock();
    if ( nMaxSize > 3 )
        pGraph = NULL;
    else
        pGraph = Abc_NodeEvaluateDsd( p, pNodeDsd, pRoot, Required, nNodesSaved, &nNodesAdded );
//    pGraph = NULL;
p->timeEval += Abc_Clock() - clk;

    // quit if there is no improvement
    if ( pGraph == NULL || nNodesAdded == -1 || (nNodesAdded == nNodesSaved && !p->fUseZeros) )
    {
        Cudd_RecursiveDeref( p->dd, bFunc );
        if ( pGraph ) Dec_GraphFree( pGraph );
        return NULL;
    }

/*
    // print stats
    printf( "%5d : Cut-size = %d.  Old AIG = %2d.  New AIG = %2d.  Old MFFC = %2d.  New MFFC = %2d. Gain = %d.\n",
        pRoot->Id, pCut->nLeaves, Vec_PtrSize(p->vVisited), Dsd_TreeGetAigCost(pNodeDsd), 
        nNodesSaved, nNodesAdded, (nNodesAdded == -1)? 0 : nNodesSaved-nNodesAdded );
//    Dsd_NodePrint( stdout, pNodeDsd );
//    Dec_GraphPrint( stdout, pGraph, NULL, NULL );
*/

    // compute the total gain in the number of nodes
    p->nLastGain = nNodesSaved - nNodesAdded;
    p->nNodesGained += p->nLastGain;
    p->nNodesRestructured++;

    // report the progress
    if ( fVeryVerbose )
    {
        printf( "Node %6s : ",  Abc_ObjName(pRoot) );
        printf( "Cone = %2d. ", p->vLeaves->nSize );
        printf( "BDD = %2d. ",  Cudd_DagSize(bFunc) );
        printf( "FF = %2d. ",   1 + Dec_GraphNodeNum(pGraph) );
        printf( "MFFC = %2d. ", nNodesSaved );
        printf( "Add = %2d. ",  nNodesAdded );
        printf( "GAIN = %2d. ", p->nLastGain );
        printf( "\n" );
    }
    Cudd_RecursiveDeref( p->dd, bFunc );
    return pGraph;
}


/**Function*************************************************************

  Synopsis    [Moves closer to the end the node that is best for sharing.]

  Description [If the flag is set, tries to find an EXOR, otherwise, tries
  to find an OR.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeEdgeDsdPermute( Dec_Graph_t * pGraph, Abc_ManRst_t * pManRst, Vec_Int_t * vEdges, int fExor )
{
    Dec_Edge_t eNode1, eNode2, eNode3;
    Abc_Obj_t * pNode1, * pNode2, * pNode3, * pTemp;
    int LeftBound = 0, RightBound, i;
    // get the right bound
    RightBound = Vec_IntSize(vEdges) - 2;
    assert( LeftBound <= RightBound );
    if ( LeftBound == RightBound )
        return;
    // get the two last nodes
    eNode1 = Dec_IntToEdge( Vec_IntEntry(vEdges, RightBound + 1) );
    eNode2 = Dec_IntToEdge( Vec_IntEntry(vEdges, RightBound    ) );
    pNode1 = (Abc_Obj_t *)Dec_GraphNode( pGraph, eNode1.Node )->pFunc;
    pNode2 = (Abc_Obj_t *)Dec_GraphNode( pGraph, eNode2.Node )->pFunc;
    pNode1 = !pNode1? NULL : Abc_ObjNotCond( pNode1, eNode1.fCompl );
    pNode2 = !pNode2? NULL : Abc_ObjNotCond( pNode2, eNode2.fCompl );
    // quit if the last node does not exist
    if ( pNode1 == NULL )
        return;
    // find the first node that can be shared
    for ( i = RightBound; i >= LeftBound; i-- )
    {
        // get the third node
        eNode3 = Dec_IntToEdge( Vec_IntEntry(vEdges, i) );
        pNode3 = (Abc_Obj_t *)Dec_GraphNode( pGraph, eNode3.Node )->pFunc;
        pNode3 = !pNode3? NULL : Abc_ObjNotCond( pNode3, eNode3.fCompl );
        if ( pNode3 == NULL )
            continue;
        // check if the node exists
        if ( fExor )
        {
            if ( pNode1 && pNode3 )
            {
                pTemp = Abc_AigXorLookup( (Abc_Aig_t *)pManRst->pNtk->pManFunc, pNode1, pNode3, NULL );
                if ( !pTemp || Abc_NodeIsTravIdCurrent(Abc_ObjRegular(pTemp)) )
                    continue;

                if ( pNode3 == pNode2 )
                    return;
                Vec_IntWriteEntry( vEdges, i,          Dec_EdgeToInt(eNode2) );
                Vec_IntWriteEntry( vEdges, RightBound, Dec_EdgeToInt(eNode3) );
                return;
            }
        }
        else
        {
            if ( pNode1 && pNode3 )
            {
                pTemp = Abc_AigAndLookup( (Abc_Aig_t *)pManRst->pNtk->pManFunc, Abc_ObjNot(pNode1), Abc_ObjNot(pNode3) );
                if ( !pTemp || Abc_NodeIsTravIdCurrent(Abc_ObjRegular(pTemp)) )
                    continue;

                if ( eNode3.Node == eNode2.Node )
                    return;
                Vec_IntWriteEntry( vEdges, i,          Dec_EdgeToInt(eNode2) );
                Vec_IntWriteEntry( vEdges, RightBound, Dec_EdgeToInt(eNode3) );
                return;
            }
        }
    }
}

/**Function*************************************************************

  Synopsis    [Adds the new edge in the given order.]

  Description [Similar to Vec_IntPushOrder, except in decreasing order.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeEdgeDsdPushOrdered( Dec_Graph_t * pGraph, Vec_Int_t * vEdges, int Edge )
{
    int i, NodeOld, NodeNew;
    vEdges->nSize++;
    for ( i = vEdges->nSize-2; i >= 0; i-- )
    {
        NodeOld = Dec_IntToEdge(vEdges->pArray[i]).Node;
        NodeNew = Dec_IntToEdge(Edge).Node;
        // use <= because we are trying to push the new (non-existent) nodes as far as possible
        if ( Dec_GraphNode(pGraph, NodeOld)->Level <= Dec_GraphNode(pGraph, NodeNew)->Level )
            vEdges->pArray[i+1] = vEdges->pArray[i];
        else
            break;
    }
    vEdges->pArray[i+1] = Edge;
}

/**Function*************************************************************

  Synopsis    [Evaluation one DSD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dec_Edge_t Abc_NodeEvaluateDsd_rec( Dec_Graph_t * pGraph, Abc_ManRst_t * pManRst, Dsd_Node_t * pNodeDsd, int Required, int nNodesSaved, int * pnNodesAdded )
{
    Dec_Edge_t eNode1, eNode2, eNode3, eResult, eQuit = { 0, 2006 };
    Abc_Obj_t * pNode1, * pNode2, * pNode3, * pNode4, * pTemp;
    Dsd_Node_t * pChildDsd;
    Dsd_Type_t DecType;
    Vec_Int_t * vEdges;
    int Level1, Level2, Level3, Level4;
    int i, Index, fCompl, Type;

    // remove the complemented attribute
    fCompl   = Dsd_IsComplement( pNodeDsd );
    pNodeDsd = Dsd_Regular( pNodeDsd );

    // consider the trivial case
    DecType = Dsd_NodeReadType( pNodeDsd );
    if ( DecType == DSD_NODE_BUF )
    {
        Index = Dsd_NodeReadFunc(pNodeDsd)->index;
        assert( Index < Dec_GraphLeaveNum(pGraph) );
        eResult = Dec_EdgeCreate( Index, fCompl );
        return eResult;
    }
    assert( DecType == DSD_NODE_OR || DecType == DSD_NODE_EXOR || DecType == DSD_NODE_PRIME );

    // solve the problem for the children
    vEdges = Vec_IntAlloc( Dsd_NodeReadDecsNum(pNodeDsd) );
    Dsd_NodeForEachChild( pNodeDsd, i, pChildDsd )
    {
        eResult = Abc_NodeEvaluateDsd_rec( pGraph, pManRst, pChildDsd, Required, nNodesSaved, pnNodesAdded );
        if ( eResult.Node == eQuit.Node ) // infeasible
        {
            Vec_IntFree( vEdges );
            return eQuit;
        }
        // order the inputs only if this is OR or EXOR
        if ( DecType == DSD_NODE_PRIME )
            Vec_IntPush( vEdges, Dec_EdgeToInt(eResult) );
        else
            Abc_NodeEdgeDsdPushOrdered( pGraph, vEdges, Dec_EdgeToInt(eResult) );
    }
    // the edges are sorted by the level of their nodes in decreasing order


    // consider special cases
    if ( DecType == DSD_NODE_OR )
    {
        // try to balance the nodes by delay
        assert( Vec_IntSize(vEdges) > 1 );
        while ( Vec_IntSize(vEdges) > 1 )
        {
            // permute the last two entries
            if ( Vec_IntSize(vEdges) > 2 )
                Abc_NodeEdgeDsdPermute( pGraph, pManRst, vEdges, 0 );
            // get the two last nodes
            eNode1 = Dec_IntToEdge( Vec_IntPop(vEdges) );
            eNode2 = Dec_IntToEdge( Vec_IntPop(vEdges) );
            pNode1 = (Abc_Obj_t *)Dec_GraphNode( pGraph, eNode1.Node )->pFunc;
            pNode2 = (Abc_Obj_t *)Dec_GraphNode( pGraph, eNode2.Node )->pFunc;
            pNode1 = !pNode1? NULL : Abc_ObjNotCond( pNode1, eNode1.fCompl );
            pNode2 = !pNode2? NULL : Abc_ObjNotCond( pNode2, eNode2.fCompl );
            // check if the new node exists
            pNode3 = NULL;
            if ( pNode1 && pNode2 )
            {
                pNode3 = Abc_AigAndLookup( (Abc_Aig_t *)pManRst->pNtk->pManFunc, Abc_ObjNot(pNode1), Abc_ObjNot(pNode2) ); 
                pNode3 = !pNode3? NULL : Abc_ObjNot(pNode3);
            }
            // create the new node
            eNode3 = Dec_GraphAddNodeOr( pGraph, eNode1, eNode2 );
            // set level
            Level1 = Dec_GraphNode( pGraph, eNode1.Node )->Level;
            Level2 = Dec_GraphNode( pGraph, eNode2.Node )->Level;
            Dec_GraphNode( pGraph, eNode3.Node )->Level = 1 + Abc_MaxInt(Level1, Level2);
            // get the new node if possible
            if ( pNode3 )
            {
                Dec_GraphNode( pGraph, eNode3.Node )->pFunc = Abc_ObjNotCond(pNode3, eNode3.fCompl);
                Level3 = Dec_GraphNode( pGraph, eNode3.Node )->Level;
                assert( Required == ABC_INFINITY || Level3 == (int)Abc_ObjRegular(pNode3)->Level );
            }
            if ( !pNode3 || Abc_NodeIsTravIdCurrent(Abc_ObjRegular(pNode3)) )
            {
                (*pnNodesAdded)++;
                if ( *pnNodesAdded > nNodesSaved )
                {
                    Vec_IntFree( vEdges );
                    return eQuit;
                }
            }
            // add the resulting node to the form
            Abc_NodeEdgeDsdPushOrdered( pGraph, vEdges, Dec_EdgeToInt(eNode3) );
        }
        // get the last node
        eResult = Dec_IntToEdge( Vec_IntPop(vEdges) );
        Vec_IntFree( vEdges );
        // complement the graph if the node was complemented
        eResult.fCompl ^= fCompl;
        return eResult;
    }
    if ( DecType == DSD_NODE_EXOR )
    {
        // try to balance the nodes by delay
        assert( Vec_IntSize(vEdges) > 1 );
        while ( Vec_IntSize(vEdges) > 1 )
        {
            // permute the last two entries
            if ( Vec_IntSize(vEdges) > 2 )
                Abc_NodeEdgeDsdPermute( pGraph, pManRst, vEdges, 1 );
            // get the two last nodes
            eNode1 = Dec_IntToEdge( Vec_IntPop(vEdges) );
            eNode2 = Dec_IntToEdge( Vec_IntPop(vEdges) );
            pNode1 = (Abc_Obj_t *)Dec_GraphNode( pGraph, eNode1.Node )->pFunc;
            pNode2 = (Abc_Obj_t *)Dec_GraphNode( pGraph, eNode2.Node )->pFunc;
            pNode1 = !pNode1? NULL : Abc_ObjNotCond( pNode1, eNode1.fCompl );
            pNode2 = !pNode2? NULL : Abc_ObjNotCond( pNode2, eNode2.fCompl );
            // check if the new node exists
            Type = 0;
            pNode3 = NULL;
            if ( pNode1 && pNode2 )
                pNode3 = Abc_AigXorLookup( (Abc_Aig_t *)pManRst->pNtk->pManFunc, pNode1, pNode2, &Type ); 
            // create the new node
            eNode3 = Dec_GraphAddNodeXor( pGraph, eNode1, eNode2, Type ); // should have the same structure as in AIG
            // set level
            Level1 = Dec_GraphNode( pGraph, eNode1.Node )->Level;
            Level2 = Dec_GraphNode( pGraph, eNode2.Node )->Level;
            Dec_GraphNode( pGraph, eNode3.Node )->Level = 2 + Abc_MaxInt(Level1, Level2);
            // get the new node if possible
            if ( pNode3 )
            {
                Dec_GraphNode( pGraph, eNode3.Node )->pFunc = Abc_ObjNotCond(pNode3, eNode3.fCompl);
                Level3 = Dec_GraphNode( pGraph, eNode3.Node )->Level;
                assert( Required == ABC_INFINITY || Level3 == (int)Abc_ObjRegular(pNode3)->Level );
            }
            if ( !pNode3 || Abc_NodeIsTravIdCurrent(Abc_ObjRegular(pNode3)) )
            {
                (*pnNodesAdded)++;
                if ( !pNode1 || !pNode2 )
                    (*pnNodesAdded) += 2;
                else if ( Type == 0 )
                {
                    pTemp = Abc_AigAndLookup( (Abc_Aig_t *)pManRst->pNtk->pManFunc, pNode1, Abc_ObjNot(pNode2) );
                    if ( !pTemp || Abc_NodeIsTravIdCurrent(Abc_ObjRegular(pTemp)) )
                        (*pnNodesAdded)++;
                    pTemp = Abc_AigAndLookup( (Abc_Aig_t *)pManRst->pNtk->pManFunc, Abc_ObjNot(pNode1), pNode2 );
                    if ( !pTemp || Abc_NodeIsTravIdCurrent(Abc_ObjRegular(pTemp)) )
                        (*pnNodesAdded)++;
                }
                else
                {
                    pTemp = Abc_AigAndLookup( (Abc_Aig_t *)pManRst->pNtk->pManFunc, Abc_ObjNot(pNode1), Abc_ObjNot(pNode2) );
                    if ( !pTemp || Abc_NodeIsTravIdCurrent(Abc_ObjRegular(pTemp)) )
                        (*pnNodesAdded)++;
                    pTemp = Abc_AigAndLookup( (Abc_Aig_t *)pManRst->pNtk->pManFunc, pNode1, pNode2 );
                    if ( !pTemp || Abc_NodeIsTravIdCurrent(Abc_ObjRegular(pTemp)) )
                        (*pnNodesAdded)++;
                }
                if ( *pnNodesAdded > nNodesSaved )
                {
                    Vec_IntFree( vEdges );
                    return eQuit;
                }
            }
            // add the resulting node to the form
            Abc_NodeEdgeDsdPushOrdered( pGraph, vEdges, Dec_EdgeToInt(eNode3) );
        }
        // get the last node
        eResult = Dec_IntToEdge( Vec_IntPop(vEdges) );
        Vec_IntFree( vEdges );
        // complement the graph if the node is complemented
        eResult.fCompl ^= fCompl;
        return eResult;
    }
    if ( DecType == DSD_NODE_PRIME )
    {
        DdNode * bLocal, * bVar, * bCofT, * bCofE;
        bLocal = Dsd_TreeGetPrimeFunction( pManRst->dd, pNodeDsd );  Cudd_Ref( bLocal );
//Extra_bddPrint( pManRst->dd, bLocal );

        bVar  = pManRst->dd->vars[0];
        bCofE = Cudd_Cofactor( pManRst->dd, bLocal, Cudd_Not(bVar) );  Cudd_Ref( bCofE );
        bCofT = Cudd_Cofactor( pManRst->dd, bLocal, bVar );            Cudd_Ref( bCofT );
        if ( !Extra_bddIsVar(bCofE) || !Extra_bddIsVar(bCofT) )
        {
            Cudd_RecursiveDeref( pManRst->dd, bCofE );
            Cudd_RecursiveDeref( pManRst->dd, bCofT );
            bVar  = pManRst->dd->vars[1];
            bCofE = Cudd_Cofactor( pManRst->dd, bLocal, Cudd_Not(bVar) );  Cudd_Ref( bCofE );
            bCofT = Cudd_Cofactor( pManRst->dd, bLocal, bVar );            Cudd_Ref( bCofT );
            if ( !Extra_bddIsVar(bCofE) || !Extra_bddIsVar(bCofT) )
            {
                Cudd_RecursiveDeref( pManRst->dd, bCofE );
                Cudd_RecursiveDeref( pManRst->dd, bCofT );
                bVar  = pManRst->dd->vars[2];
                bCofE = Cudd_Cofactor( pManRst->dd, bLocal, Cudd_Not(bVar) );  Cudd_Ref( bCofE );
                bCofT = Cudd_Cofactor( pManRst->dd, bLocal, bVar );            Cudd_Ref( bCofT );
                if ( !Extra_bddIsVar(bCofE) || !Extra_bddIsVar(bCofT) )
                {
                    Cudd_RecursiveDeref( pManRst->dd, bCofE );
                    Cudd_RecursiveDeref( pManRst->dd, bCofT );
                    Cudd_RecursiveDeref( pManRst->dd, bLocal );
                    Vec_IntFree( vEdges );
                    return eQuit;
                }
            }
        }
        Cudd_RecursiveDeref( pManRst->dd, bLocal );
        // we found the control variable (bVar) and the var-cofactors (bCofT, bCofE)

        // find the graph nodes
        eNode1 = Dec_IntToEdge( Vec_IntEntry(vEdges, bVar->index) );
        eNode2 = Dec_IntToEdge( Vec_IntEntry(vEdges, Cudd_Regular(bCofT)->index) );
        eNode3 = Dec_IntToEdge( Vec_IntEntry(vEdges, Cudd_Regular(bCofE)->index) );
        // add the complements to the graph nodes
        eNode2.fCompl ^= Cudd_IsComplement(bCofT);
        eNode3.fCompl ^= Cudd_IsComplement(bCofE);

        // because the cofactors are vars, we can just as well deref them here
        Cudd_RecursiveDeref( pManRst->dd, bCofE );
        Cudd_RecursiveDeref( pManRst->dd, bCofT );

        // find the ABC nodes
        pNode1 = (Abc_Obj_t *)Dec_GraphNode( pGraph, eNode1.Node )->pFunc;
        pNode2 = (Abc_Obj_t *)Dec_GraphNode( pGraph, eNode2.Node )->pFunc;
        pNode3 = (Abc_Obj_t *)Dec_GraphNode( pGraph, eNode3.Node )->pFunc;
        pNode1 = !pNode1? NULL : Abc_ObjNotCond( pNode1, eNode1.fCompl );
        pNode2 = !pNode2? NULL : Abc_ObjNotCond( pNode2, eNode2.fCompl );
        pNode3 = !pNode3? NULL : Abc_ObjNotCond( pNode3, eNode3.fCompl );

        // check if the new node exists
        Type = 0;
        pNode4 = NULL;
        if ( pNode1 && pNode2 && pNode3 )
            pNode4 = Abc_AigMuxLookup( (Abc_Aig_t *)pManRst->pNtk->pManFunc, pNode1, pNode2, pNode3, &Type ); 

        // create the new node
        eResult = Dec_GraphAddNodeMux( pGraph, eNode1, eNode2, eNode3, Type ); // should have the same structure as AIG

        // set level
        Level1 = Dec_GraphNode( pGraph, eNode1.Node )->Level;
        Level2 = Dec_GraphNode( pGraph, eNode2.Node )->Level;
        Level3 = Dec_GraphNode( pGraph, eNode3.Node )->Level;
        Dec_GraphNode( pGraph, eResult.Node )->Level = 2 + Abc_MaxInt( Abc_MaxInt(Level1, Level2), Level3 );
        // get the new node if possible
        if ( pNode4 )
        {
            Dec_GraphNode( pGraph, eResult.Node )->pFunc = Abc_ObjNotCond(pNode4, eResult.fCompl);
            Level4 = Dec_GraphNode( pGraph, eResult.Node )->Level;
            assert( Required == ABC_INFINITY || Level4 == (int)Abc_ObjRegular(pNode4)->Level );
        }
        if ( !pNode4 || Abc_NodeIsTravIdCurrent(Abc_ObjRegular(pNode4)) )
        {
            (*pnNodesAdded)++;
            if ( Type == 0 ) 
            {
                if ( !pNode1 || !pNode2 )
                    (*pnNodesAdded)++;
                else
                {
                    pTemp = Abc_AigAndLookup( (Abc_Aig_t *)pManRst->pNtk->pManFunc, pNode1, pNode2 );
                    if ( !pTemp || Abc_NodeIsTravIdCurrent(Abc_ObjRegular(pTemp)) )
                        (*pnNodesAdded)++;
                }
                if ( !pNode1 || !pNode3 )
                    (*pnNodesAdded)++;
                else
                {
                    pTemp = Abc_AigAndLookup( (Abc_Aig_t *)pManRst->pNtk->pManFunc, Abc_ObjNot(pNode1), pNode3 );
                    if ( !pTemp || Abc_NodeIsTravIdCurrent(Abc_ObjRegular(pTemp)) )
                        (*pnNodesAdded)++;
                }
            }
            else
            {
                if ( !pNode1 || !pNode2 )
                    (*pnNodesAdded)++;
                else
                {
                    pTemp = Abc_AigAndLookup( (Abc_Aig_t *)pManRst->pNtk->pManFunc, pNode1, Abc_ObjNot(pNode2) );
                    if ( !pTemp || Abc_NodeIsTravIdCurrent(Abc_ObjRegular(pTemp)) )
                        (*pnNodesAdded)++;
                }
                if ( !pNode1 || !pNode3 )
                    (*pnNodesAdded)++;
                else
                {
                    pTemp = Abc_AigAndLookup( (Abc_Aig_t *)pManRst->pNtk->pManFunc, Abc_ObjNot(pNode1), Abc_ObjNot(pNode3) );
                    if ( !pTemp || Abc_NodeIsTravIdCurrent(Abc_ObjRegular(pTemp)) )
                        (*pnNodesAdded)++;
                }
            }
            if ( *pnNodesAdded > nNodesSaved )
            {
                Vec_IntFree( vEdges );
                return eQuit;
            }
        }

        Vec_IntFree( vEdges );
        // complement the graph if the node was complemented
        eResult.fCompl ^= fCompl;
        return eResult;
    }
    Vec_IntFree( vEdges );
    return eQuit;
}

/**Function*************************************************************

  Synopsis    [Evaluation one DSD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dec_Graph_t * Abc_NodeEvaluateDsd( Abc_ManRst_t * pManRst, Dsd_Node_t * pNodeDsd, Abc_Obj_t * pRoot, int Required, int nNodesSaved, int * pnNodesAdded )
{
    Dec_Graph_t * pGraph;
    Dec_Edge_t gEdge;
    Abc_Obj_t * pLeaf;
    Dec_Node_t * pNode;
    int i;

    // create the graph and set the leaves
    pGraph = Dec_GraphCreate( Vec_PtrSize(pManRst->vLeaves) );
    Dec_GraphForEachLeaf( pGraph, pNode, i )
    {
        pLeaf = (Abc_Obj_t *)Vec_PtrEntry( pManRst->vLeaves, i );
        pNode->pFunc = pLeaf;
        pNode->Level = pLeaf->Level;
    }

    // create the decomposition structure from the DSD
    *pnNodesAdded = 0;
    gEdge = Abc_NodeEvaluateDsd_rec( pGraph, pManRst, pNodeDsd, Required, nNodesSaved, pnNodesAdded );
    if ( gEdge.Node > 1000 ) // infeasible
    {
        *pnNodesAdded = -1;
        Dec_GraphFree( pGraph );
        return NULL;
    }

    // quit if the root node is the same
    pLeaf = (Abc_Obj_t *)Dec_GraphNode( pGraph, gEdge.Node )->pFunc;
    if ( Abc_ObjRegular(pLeaf) == pRoot )
    {
        *pnNodesAdded = -1;
        Dec_GraphFree( pGraph );
        return NULL;
    }

    Dec_GraphSetRoot( pGraph, gEdge );
    return pGraph;
}



/**Function*************************************************************

  Synopsis    [Starts the cut manager for rewriting.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cut_Man_t * Abc_NtkStartCutManForRestruct( Abc_Ntk_t * pNtk, int nCutMax, int fDag )
{
    static Cut_Params_t Params, * pParams = &Params;
    Cut_Man_t * pManCut;
    Abc_Obj_t * pObj;
    int i;
    // start the cut manager
    memset( pParams, 0, sizeof(Cut_Params_t) );
    pParams->nVarsMax  = nCutMax; // the max cut size ("k" of the k-feasible cuts)
    pParams->nKeepMax  = 250;     // the max number of cuts kept at a node
    pParams->fTruth    = 0;       // compute truth tables
    pParams->fFilter   = 1;       // filter dominated cuts
    pParams->fSeq      = 0;       // compute sequential cuts
    pParams->fDrop     = 0;       // drop cuts on the fly
    pParams->fDag      = fDag;    // compute DAG cuts
    pParams->fTree     = 0;       // compute tree cuts
    pParams->fVerbose  = 0;       // the verbosiness flag
    pParams->nIdsMax   = Abc_NtkObjNumMax( pNtk );
    pManCut = Cut_ManStart( pParams );
    if ( pParams->fDrop )
        Cut_ManSetFanoutCounts( pManCut, Abc_NtkFanoutCounts(pNtk) );
    // set cuts for PIs
    Abc_NtkForEachCi( pNtk, pObj, i )
        if ( Abc_ObjFanoutNum(pObj) > 0 )
            Cut_NodeSetTriv( pManCut, pObj->Id );
    return pManCut;
}

/**Function*************************************************************

  Synopsis    [Starts the resynthesis manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_ManRst_t * Abc_NtkManRstStart( int nCutMax, int fUpdateLevel, int fUseZeros, int fVerbose )
{
    Abc_ManRst_t * p;
    p = ABC_ALLOC( Abc_ManRst_t, 1 );
    memset( p, 0, sizeof(Abc_ManRst_t) );
    // set the parameters
    p->nCutMax      = nCutMax;
    p->fUpdateLevel = fUpdateLevel;
    p->fUseZeros    = fUseZeros;
    p->fVerbose     = fVerbose;
    // start the BDD manager
    p->dd = Cudd_Init( p->nCutMax, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    Cudd_zddVarsFromBddVars( p->dd, 2 );
    // start the DSD manager
    p->pManDsd = Dsd_ManagerStart( p->dd, p->dd->size, 0 );
    // other temp datastructures
    p->vVisited     = Vec_PtrAlloc( 100 );
    p->vLeaves      = Vec_PtrAlloc( 100 );
    p->vDecs        = Vec_PtrAlloc( 100 );
    p->vTemp        = Vec_PtrAlloc( 100 );
    p->vSims        = Vec_IntAlloc( 100 );
    p->vOnes        = Vec_IntAlloc( 100 );
    p->vBinate      = Vec_IntAlloc( 100 );
    p->vTwos        = Vec_IntAlloc( 100 );
    p->vRands       = Vec_IntAlloc( 20 );
    
    {
        int i;
        for ( i = 0; i < 20; i++ )
            Vec_IntPush( p->vRands, (int)RST_RANDOM_UNSIGNED );
    }
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops the resynthesis manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkManRstStop( Abc_ManRst_t * p )
{
    Dsd_ManagerStop( p->pManDsd );
    Extra_StopManager( p->dd );
    Vec_PtrFree( p->vDecs );
    Vec_PtrFree( p->vLeaves );
    Vec_PtrFree( p->vVisited );
    Vec_PtrFree( p->vTemp );
    Vec_IntFree( p->vSims );
    Vec_IntFree( p->vOnes );
    Vec_IntFree( p->vBinate );
    Vec_IntFree( p->vTwos );
    Vec_IntFree( p->vRands );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Stops the resynthesis manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkManRstPrintStats( Abc_ManRst_t * p )
{
    printf( "Refactoring statistics:\n" );
    printf( "Nodes considered   = %8d.\n", p->nNodesConsidered   );
    printf( "Cuts considered    = %8d.\n", p->nCutsConsidered    );
    printf( "Cuts explored      = %8d.\n", p->nCutsExplored      );
    printf( "Nodes restructured = %8d.\n", p->nNodesRestructured );
    printf( "Calculated gain    = %8d.\n", p->nNodesGained       );
    ABC_PRT( "Cuts       ", p->timeCut );
    ABC_PRT( "Resynthesis", p->timeRes );
    ABC_PRT( "    BDD    ", p->timeBdd );
    ABC_PRT( "    DSD    ", p->timeDsd );
    ABC_PRT( "    Eval   ", p->timeEval );
    ABC_PRT( "AIG update ", p->timeNtk );
    ABC_PRT( "TOTAL      ", p->timeTotal );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_Abc_NodeResubCollectDivs( Abc_ManRst_t * p, Abc_Obj_t * pRoot, Cut_Cut_t * pCut )
{
    Abc_Obj_t * pNode, * pFanout;
    int i, k;
    // collect the leaves of the cut
    Vec_PtrClear( p->vDecs );
    Abc_NtkIncrementTravId( pRoot->pNtk );
    for ( i = 0; i < (int)pCut->nLeaves; i++ )
    {
        pNode = Abc_NtkObj(pRoot->pNtk, pCut->pLeaves[i]);
        if ( pNode == NULL )  // the so-called "bad cut phenomenon" is due to removed nodes
            return 0;
        Vec_PtrPush( p->vDecs, pNode );
        Abc_NodeSetTravIdCurrent( pNode );        
    }
    // explore the fanouts
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vDecs, pNode, i )
    {
        // if the fanout has both fanins in the set, add it
        Abc_ObjForEachFanout( pNode, pFanout, k )
        {
            if ( Abc_NodeIsTravIdCurrent(pFanout) || Abc_ObjIsPo(pFanout) )
                continue;
            if ( Abc_NodeIsTravIdCurrent(Abc_ObjFanin0(pFanout)) && Abc_NodeIsTravIdCurrent(Abc_ObjFanin1(pFanout)) )
            {
                Vec_PtrPush( p->vDecs, pFanout );
                Abc_NodeSetTravIdCurrent( pFanout );     
            }
        }
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeResubMffc_rec( Abc_Obj_t * pNode )
{
    if ( Abc_NodeIsTravIdCurrent(pNode) )
        return 0;
    Abc_NodeSetTravIdCurrent( pNode ); 
    return 1 + Abc_NodeResubMffc_rec( Abc_ObjFanin0(pNode) ) +
        Abc_NodeResubMffc_rec( Abc_ObjFanin1(pNode) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeResubMffc( Abc_ManRst_t * p, Vec_Ptr_t * vDecs, int nLeaves, Abc_Obj_t * pRoot )
{
    Abc_Obj_t * pObj;
    int Counter, i, k;
    // increment the traversal ID for the leaves
    Abc_NtkIncrementTravId( pRoot->pNtk );
    // label the leaves
    Vec_PtrForEachEntryStop( Abc_Obj_t *, vDecs, pObj, i, nLeaves )
        Abc_NodeSetTravIdCurrent( pObj ); 
    // make sure the node is in the cone and is no one of the leaves
    assert( Abc_NodeIsTravIdPrevious(pRoot) );
    Counter = Abc_NodeResubMffc_rec( pRoot );
    // move the labeled nodes to the end 
    Vec_PtrClear( p->vTemp );
    k = 0;
    Vec_PtrForEachEntryStart( Abc_Obj_t *, vDecs, pObj, i, nLeaves )
        if ( Abc_NodeIsTravIdCurrent(pObj) )
            Vec_PtrPush( p->vTemp, pObj );
        else
            Vec_PtrWriteEntry( vDecs, k++, pObj );
    // add the labeled nodes
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vTemp, pObj, i )
        Vec_PtrWriteEntry( vDecs, k++, pObj );
    assert( k == Vec_PtrSize(p->vDecs) );
    assert( pRoot == Vec_PtrEntryLast(p->vDecs) );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Performs simulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeMffcSimulate( Vec_Ptr_t * vDecs, int nLeaves, Vec_Int_t * vRands, Vec_Int_t * vSims )
{
    Abc_Obj_t * pObj;
    unsigned uData0, uData1, uData;
    int i;
    // initialize random simulation data
    Vec_IntClear( vSims );
    Vec_PtrForEachEntryStop( Abc_Obj_t *, vDecs, pObj, i, nLeaves )
    {
        uData = (unsigned)Vec_IntEntry( vRands, i );
        pObj->pData = (void *)(ABC_PTRUINT_T)uData;
        Vec_IntPush( vSims, uData );
    }
    // simulate
    Vec_PtrForEachEntryStart( Abc_Obj_t *, vDecs, pObj, i, nLeaves )
    {
        uData0 = (unsigned)(ABC_PTRUINT_T)Abc_ObjFanin0(pObj)->pData;
        uData1 = (unsigned)(ABC_PTRUINT_T)Abc_ObjFanin1(pObj)->pData;
        uData = (Abc_ObjFaninC0(pObj)? ~uData0 : uData0) & (Abc_ObjFaninC1(pObj)? ~uData1 : uData1);
        pObj->pData = (void *)(ABC_PTRUINT_T)uData;
        Vec_IntPush( vSims, uData );
    }
}

/**Function*************************************************************

  Synopsis    [Full equality check.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeCheckFull( Abc_ManRst_t * p, Dec_Graph_t * pGraph )
{
    return 1;
}
/**Function*************************************************************

  Synopsis    [Detect contants.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dec_Graph_t * Abc_NodeMffcConstants( Abc_ManRst_t * p, Vec_Int_t * vSims )
{
    Dec_Graph_t * pGraph = NULL;
    unsigned uRoot;
    // get the root node
    uRoot = (unsigned)Vec_IntEntryLast( vSims );
    // get the graph if the node looks constant
    if ( uRoot == 0 )
        pGraph = Dec_GraphCreateConst0();
    else if ( uRoot == ~(unsigned)0 )
        pGraph = Dec_GraphCreateConst1();
    // check the graph
    assert(pGraph);
    if ( Abc_NodeCheckFull( p, pGraph ) )
        return pGraph;
    Dec_GraphFree( pGraph );
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Detect single non-overlaps.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dec_Graph_t * Abc_NodeMffcSingleVar( Abc_ManRst_t * p, Vec_Int_t * vSims, int nNodes, Vec_Int_t * vOnes )
{
    Dec_Graph_t * pGraph;
    unsigned uRoot, uNode;
    int i;

    Vec_IntClear( vOnes );
    Vec_IntClear( p->vBinate );
    uRoot = (unsigned)Vec_IntEntryLast( vSims );
    for ( i = 0; i < nNodes; i++ )
    {
        uNode = (unsigned)Vec_IntEntry( vSims, i );
        if ( uRoot == uNode || uRoot == ~uNode )
        {
            pGraph = Dec_GraphCreate( 1 );
            Dec_GraphNode( pGraph, 0 )->pFunc = Vec_PtrEntry( p->vDecs, i );
            Dec_GraphSetRoot( pGraph, Dec_IntToEdge( (int)(uRoot == ~uNode) ) );
            // check the graph
            if ( Abc_NodeCheckFull( p, pGraph ) )
                return pGraph;
            Dec_GraphFree( pGraph );
        }
        if ( (uRoot & uNode) == 0 )
            Vec_IntPush( vOnes, i << 1 );
        else if ( (uRoot & ~uNode) == 0 )
            Vec_IntPush( vOnes, (i << 1) + 1 );
        else
            Vec_IntPush( p->vBinate, i );
    }    
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Detect single non-overlaps.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dec_Graph_t * Abc_NodeMffcSingleNode( Abc_ManRst_t * p, Vec_Int_t * vSims, int nNodes, Vec_Int_t * vOnes )
{
    Dec_Graph_t * pGraph;
    Dec_Edge_t eNode0, eNode1, eRoot;
    unsigned uRoot;
    int i, k;
    uRoot = (unsigned)Vec_IntEntryLast( vSims );
    for ( i = 0; i < vOnes->nSize; i++ )
        for ( k = i+1; k < vOnes->nSize; k++ )
            if ( ~uRoot == ((unsigned)vOnes->pArray[i] | (unsigned)vOnes->pArray[k]) )
            {
                eNode0 = Dec_IntToEdge( vOnes->pArray[i] ^ 1 );
                eNode1 = Dec_IntToEdge( vOnes->pArray[k] ^ 1 );
                pGraph = Dec_GraphCreate( 2 );
                Dec_GraphNode( pGraph, 0 )->pFunc = Vec_PtrEntry( p->vDecs, eNode0.Node );
                Dec_GraphNode( pGraph, 1 )->pFunc = Vec_PtrEntry( p->vDecs, eNode1.Node );
                eRoot = Dec_GraphAddNodeAnd( pGraph, eNode0, eNode1 );
                Dec_GraphSetRoot( pGraph, eRoot );
                if ( Abc_NodeCheckFull( p, pGraph ) )
                    return pGraph;
                Dec_GraphFree( pGraph );
            }
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Detect single non-overlaps.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dec_Graph_t * Abc_NodeMffcDoubleNode( Abc_ManRst_t * p, Vec_Int_t * vSims, int nNodes, Vec_Int_t * vOnes )
{
//    Dec_Graph_t * pGraph;
//    unsigned uRoot, uNode;
//    int i;


    return NULL;
}

/**Function*************************************************************

  Synopsis    [Evaluates resubstution of one cut.]

  Description [Returns the graph to add if any.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dec_Graph_t * Abc_NodeResubEval( Abc_ManRst_t * p, Abc_Obj_t * pRoot, Cut_Cut_t * pCut )
{
    Dec_Graph_t * pGraph;
    int nNodesSaved;

    // collect the nodes in the cut
    if ( !Abc_Abc_NodeResubCollectDivs( p, pRoot, pCut ) )
        return NULL;

    // label MFFC and count its size
    nNodesSaved = Abc_NodeResubMffc( p, p->vDecs, pCut->nLeaves, pRoot );
    assert( nNodesSaved > 0 );

    // simulate MFFC
    Abc_NodeMffcSimulate( p->vDecs, pCut->nLeaves, p->vRands, p->vSims );

    // check for constant output
    pGraph = Abc_NodeMffcConstants( p, p->vSims );
    if ( pGraph )
    {
        p->nNodesGained += nNodesSaved;
        p->nNodesRestructured++;
        return pGraph;
    }

    // check for one literal (fill up the ones array)
    pGraph = Abc_NodeMffcSingleVar( p, p->vSims, Vec_IntSize(p->vSims) - nNodesSaved, p->vOnes );
    if ( pGraph )
    {
        p->nNodesGained += nNodesSaved;
        p->nNodesRestructured++;
        return pGraph;
    }
    if ( nNodesSaved == 1 )
        return NULL;

    // look for one node
    pGraph = Abc_NodeMffcSingleNode( p, p->vSims, Vec_IntSize(p->vSims) - nNodesSaved, p->vOnes );
    if ( pGraph )
    {
        p->nNodesGained += nNodesSaved - 1;
        p->nNodesRestructured++;
        return pGraph;
    }
    if ( nNodesSaved == 2 )
        return NULL;

    // look for two nodes
    pGraph = Abc_NodeMffcDoubleNode( p, p->vSims, Vec_IntSize(p->vSims) - nNodesSaved, p->vOnes );
    if ( pGraph )
    {
        p->nNodesGained += nNodesSaved - 2;
        p->nNodesRestructured++;
        return pGraph;
    }
    if ( nNodesSaved == 3 )
        return NULL;
/*
    // look for MUX/EXOR
    pGraph = Abc_NodeMffcMuxNode( p, p->vSims, Vec_IntSize(p->vSims) - nNodesSaved );
    if ( pGraph )
    {
        p->nNodesGained += nNodesSaved - 1;
        p->nNodesRestructured++;
        return pGraph;
    }
*/
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Performs resubstution.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dec_Graph_t * Abc_NodeResubstitute( Abc_ManRst_t * p, Abc_Obj_t * pNode, Cut_Cut_t * pCutList )
{
    Dec_Graph_t * pGraph, * pGraphBest = NULL;
    Cut_Cut_t * pCut;
    int nCuts;
    p->nNodesConsidered++;

    // count the number of cuts with four inputs or more
    nCuts = 0;
    for ( pCut = pCutList; pCut; pCut = pCut->pNext )
        nCuts += (int)(pCut->nLeaves > 3);
    printf( "-----------------------------------\n" );
    printf( "Node %6d : Factor-cuts = %5d.\n", pNode->Id, nCuts );

    // go through the interesting cuts
    for ( pCut = pCutList; pCut; pCut = pCut->pNext )
    {
        if ( pCut->nLeaves < 4 )
            continue;
        pGraph = Abc_NodeResubEval( p, pNode, pCut );
        if ( pGraph == NULL )
            continue;
        if ( !pGraphBest || Dec_GraphNodeNum(pGraph) < Dec_GraphNodeNum(pGraphBest) )
        {
            if ( pGraphBest ) 
                Dec_GraphFree(pGraphBest);
            pGraphBest = pGraph;
        }
        else
            Dec_GraphFree(pGraph);
    }
    return pGraphBest;
}

#else

int Abc_NtkRestructure( Abc_Ntk_t * pNtk, int nCutMax, int fUpdateLevel, int fUseZeros, int fVerbose ) { return 1; }

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

