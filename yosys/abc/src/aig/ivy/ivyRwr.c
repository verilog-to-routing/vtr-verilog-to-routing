/**CFile****************************************************************

  FileName    [ivyRwt.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [And-Inverter Graph package.]

  Synopsis    [Rewriting based on precomputation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 11, 2006.]

  Revision    [$Id: ivyRwt.c,v 1.00 2006/05/11 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ivy.h"
#include "bool/deco/deco.h"
#include "opt/rwt/rwt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static unsigned Ivy_NodeGetTruth( Ivy_Obj_t * pObj, int * pNums, int nNums );
static int Ivy_NodeRewrite( Ivy_Man_t * pMan, Rwt_Man_t * p, Ivy_Obj_t * pNode, int fUpdateLevel, int fUseZeroCost );
static Dec_Graph_t * Rwt_CutEvaluate( Ivy_Man_t * pMan, Rwt_Man_t * p, Ivy_Obj_t * pRoot, 
    Vec_Ptr_t * vFaninsCur, int nNodesSaved, int LevelMax, int * pGainBest, unsigned uTruth );

static int Ivy_GraphToNetworkCount( Ivy_Man_t * p, Ivy_Obj_t * pRoot, Dec_Graph_t * pGraph, int NodeMax, int LevelMax );
static void Ivy_GraphUpdateNetwork( Ivy_Man_t * p, Ivy_Obj_t * pRoot, Dec_Graph_t * pGraph, int fUpdateLevel, int nGain );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs incremental rewriting of the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_ManRewritePre( Ivy_Man_t * p, int fUpdateLevel, int fUseZeroCost, int fVerbose )
{
    Rwt_Man_t * pManRwt;
    Ivy_Obj_t * pNode;
    int i, nNodes, nGain;
    abctime clk, clkStart = Abc_Clock();
    // start the rewriting manager
    pManRwt = Rwt_ManStart( 0 );
    p->pData = pManRwt;
    if ( pManRwt == NULL )
        return 0; 
    // create fanouts
    if ( fUpdateLevel && p->fFanout == 0 )
        Ivy_ManStartFanout( p );
    // compute the reverse levels if level update is requested
    if ( fUpdateLevel )
        Ivy_ManRequiredLevels( p );
    // set the number of levels
//    p->nLevelMax = Ivy_ManLevels( p );
    // resynthesize each node once
    nNodes = Ivy_ManObjIdMax(p);
    Ivy_ManForEachNode( p, pNode, i )
    {
        // fix the fanin buffer problem
        Ivy_NodeFixBufferFanins( p, pNode, 1 );
        if ( Ivy_ObjIsBuf(pNode) )
            continue;
        // stop if all nodes have been tried once
        if ( i > nNodes )
            break;
        // for each cut, try to resynthesize it
        nGain = Ivy_NodeRewrite( p, pManRwt, pNode, fUpdateLevel, fUseZeroCost );
        if ( nGain > 0 || (nGain == 0 && fUseZeroCost) )
        {
            Dec_Graph_t * pGraph = (Dec_Graph_t *)Rwt_ManReadDecs(pManRwt);
            int fCompl           = Rwt_ManReadCompl(pManRwt);
/*
            {
                Ivy_Obj_t * pObj;
                int i;
                printf( "USING: (" );
                Vec_PtrForEachEntry( Ivy_Obj_t *, Rwt_ManReadLeaves(pManRwt), pObj, i )
                    printf( "%d ", Ivy_ObjFanoutNum(Ivy_Regular(pObj)) );
                printf( ")   Gain = %d.\n", nGain );
            }
            if ( nGain > 0 )
            { // print stats on the MFFC
                extern void Ivy_NodeMffcConeSuppPrint( Ivy_Obj_t * pNode );
                printf( "Node %6d : Gain = %4d  ", pNode->Id, nGain );
                Ivy_NodeMffcConeSuppPrint( pNode );
            }
*/
            // complement the FF if needed
clk = Abc_Clock();
            if ( fCompl ) Dec_GraphComplement( pGraph );
            Ivy_GraphUpdateNetwork( p, pNode, pGraph, fUpdateLevel, nGain );
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
    if ( fUpdateLevel )
        Vec_IntFree( p->vRequired ), p->vRequired = NULL;
    else
        Ivy_ManResetLevels( p );
    // check
    if ( (i = Ivy_ManCleanup(p)) )
        printf( "Cleanup after rewriting removed %d dangling nodes.\n", i );
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
int Ivy_NodeRewrite( Ivy_Man_t * pMan, Rwt_Man_t * p, Ivy_Obj_t * pNode, int fUpdateLevel, int fUseZeroCost )
{
    int fVeryVerbose = 0;
    Dec_Graph_t * pGraph;
    Ivy_Store_t * pStore;
    Ivy_Cut_t * pCut;
    Ivy_Obj_t * pFanin;
    unsigned uPhase;
    unsigned uTruthBest = 0; // Suppress "might be used uninitialized"
    unsigned uTruth;
    char * pPerm;
    int Required, nNodesSaved;
    int nNodesSaveCur = -1; // Suppress "might be used uninitialized"
    int i, c, GainCur = -1, GainBest = -1;
    abctime clk, clk2;

    p->nNodesConsidered++;
    // get the required times
    Required = fUpdateLevel? Vec_IntEntry( pMan->vRequired, pNode->Id ) : 1000000;
    // get the node's cuts
clk = Abc_Clock();
    pStore = Ivy_NodeFindCutsAll( pMan, pNode, 5 );
p->timeCut += Abc_Clock() - clk;

    // go through the cuts
clk = Abc_Clock();
    for ( c = 1; c < pStore->nCuts; c++ )
    {
        pCut = pStore->pCuts + c;
        // consider only 4-input cuts
        if ( pCut->nSize != 4 )
            continue;
        // skip the cuts with buffers
        for ( i = 0; i < (int)pCut->nSize; i++ )
            if ( Ivy_ObjIsBuf( Ivy_ManObj(pMan, pCut->pArray[i]) ) )
                break;
        if ( i != pCut->nSize )
        {
            p->nCutsBad++;
            continue;
        }
        p->nCutsGood++;
        // get the fanin permutation
clk2 = Abc_Clock();
        uTruth = 0xFFFF & Ivy_NodeGetTruth( pNode, pCut->pArray, pCut->nSize );  // truth table
p->timeTruth += Abc_Clock() - clk2;
        pPerm = p->pPerms4[ (int) p->pPerms[uTruth] ];
        uPhase = p->pPhases[uTruth];
        // collect fanins with the corresponding permutation/phase
        Vec_PtrClear( p->vFaninsCur );
        Vec_PtrFill( p->vFaninsCur, (int)pCut->nSize, 0 );
        for ( i = 0; i < (int)pCut->nSize; i++ )
        {
            pFanin = Ivy_ManObj( pMan, pCut->pArray[(int)pPerm[i]] );
            assert( Ivy_ObjIsNode(pFanin) || Ivy_ObjIsCi(pFanin) );
            pFanin = Ivy_NotCond(pFanin, ((uPhase & (1<<i)) > 0) );
            Vec_PtrWriteEntry( p->vFaninsCur, i, pFanin );
        }
clk2 = Abc_Clock();
/*
        printf( "Considering: (" );
        Vec_PtrForEachEntry( Ivy_Obj_t *, p->vFaninsCur, pFanin, i )
            printf( "%d ", Ivy_ObjFanoutNum(Ivy_Regular(pFanin)) );
        printf( ")\n" );
*/
        // mark the fanin boundary 
        Vec_PtrForEachEntry( Ivy_Obj_t *, p->vFaninsCur, pFanin, i )
            Ivy_ObjRefsInc( Ivy_Regular(pFanin) );
        // label MFFC with current ID
        Ivy_ManIncrementTravId( pMan );
        nNodesSaved = Ivy_ObjMffcLabel( pMan, pNode );
        // unmark the fanin boundary
        Vec_PtrForEachEntry( Ivy_Obj_t *, p->vFaninsCur, pFanin, i )
            Ivy_ObjRefsDec( Ivy_Regular(pFanin) );
p->timeMffc += Abc_Clock() - clk2;

        // evaluate the cut
clk2 = Abc_Clock();
        pGraph = Rwt_CutEvaluate( pMan, p, pNode, p->vFaninsCur, nNodesSaved, Required, &GainCur, uTruth );
p->timeEval += Abc_Clock() - clk2;

        // check if the cut is better than the current best one
        if ( pGraph != NULL && GainBest < GainCur )
        {
            // save this form
            nNodesSaveCur = nNodesSaved;
            GainBest  = GainCur;
            p->pGraph  = pGraph;
            p->fCompl = ((uPhase & (1<<4)) > 0);
            uTruthBest = uTruth;
            // collect fanins in the
            Vec_PtrClear( p->vFanins );
            Vec_PtrForEachEntry( Ivy_Obj_t *, p->vFaninsCur, pFanin, i )
                Vec_PtrPush( p->vFanins, pFanin );
        }
    }
p->timeRes += Abc_Clock() - clk;

    if ( GainBest == -1 )
        return -1;

//    printf( "%d", nNodesSaveCur - GainBest );
/*
    if ( GainBest > 0 )
    {
        if ( Rwt_CutIsintean( pNode, p->vFanins ) )
            printf( "b" );
        else
        {
            printf( "Node %d : ", pNode->Id );
            Vec_PtrForEachEntry( Ivy_Obj_t *, p->vFanins, pFanin, i )
                printf( "%d ", Ivy_Regular(pFanin)->Id );
            printf( "a" );
        }
    }
*/
/*
    if ( GainBest > 0 )
        if ( p->fCompl )
            printf( "c" );
        else
            printf( "." );
*/

    // copy the leaves
    Vec_PtrForEachEntry( Ivy_Obj_t *, p->vFanins, pFanin, i )
        Dec_GraphNode((Dec_Graph_t *)p->pGraph, i)->pFunc = pFanin;

    p->nScores[p->pMap[uTruthBest]]++;
    p->nNodesGained += GainBest;
    if ( fUseZeroCost || GainBest > 0 )
        p->nNodesRewritten++;

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

  Synopsis    [Computes the truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Ivy_NodeGetTruth_rec( Ivy_Obj_t * pObj, int * pNums, int nNums )
{
    static unsigned uMasks[5] = { 0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000 };
    unsigned uTruth0, uTruth1;
    int i;
    for ( i = 0; i < nNums; i++ )
        if ( pObj->Id == pNums[i] )
            return uMasks[i];
    assert( Ivy_ObjIsNode(pObj) || Ivy_ObjIsBuf(pObj) );
    uTruth0 = Ivy_NodeGetTruth_rec( Ivy_ObjFanin0(pObj), pNums, nNums );
    if ( Ivy_ObjFaninC0(pObj) )
        uTruth0 = ~uTruth0;
    if ( Ivy_ObjIsBuf(pObj) )
        return uTruth0;
    uTruth1 = Ivy_NodeGetTruth_rec( Ivy_ObjFanin1(pObj), pNums, nNums );
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
unsigned Ivy_NodeGetTruth( Ivy_Obj_t * pObj, int * pNums, int nNums )
{
    assert( nNums < 6 );
    return Ivy_NodeGetTruth_rec( pObj, pNums, nNums );
}

/**Function*************************************************************

  Synopsis    [Evaluates the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dec_Graph_t * Rwt_CutEvaluate( Ivy_Man_t * pMan, Rwt_Man_t * p, Ivy_Obj_t * pRoot, Vec_Ptr_t * vFaninsCur, int nNodesSaved, int LevelMax, int * pGainBest, unsigned uTruth )
{
    Vec_Ptr_t * vSubgraphs;
    Dec_Graph_t * pGraphBest = NULL; // Suppress "might be used uninitialized"
    Dec_Graph_t * pGraphCur;
    Rwt_Node_t * pNode, * pFanin;
    int nNodesAdded, GainBest, i, k;
    // find the matching class of subgraphs
    vSubgraphs = Vec_VecEntry( p->vClasses, p->pMap[uTruth] );
    p->nSubgraphs += vSubgraphs->nSize;
    // determine the best subgraph
    GainBest = -1;
    Vec_PtrForEachEntry( Rwt_Node_t *, vSubgraphs, pNode, i )
    {
        // get the current graph
        pGraphCur = (Dec_Graph_t *)pNode->pNext;
        // copy the leaves
        Vec_PtrForEachEntry( Rwt_Node_t *, vFaninsCur, pFanin, k )
            Dec_GraphNode(pGraphCur, k)->pFunc = pFanin;
        // detect how many unlabeled nodes will be reused
        nNodesAdded = Ivy_GraphToNetworkCount( pMan, pRoot, pGraphCur, nNodesSaved, LevelMax );
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

  Synopsis    [Counts the number of new nodes added when using this graph.]

  Description [AIG nodes for the fanins should be assigned to pNode->pFunc 
  of the leaves of the graph before calling this procedure. 
  Returns -1 if the number of nodes and levels exceeded the given limit or 
  the number of levels exceeded the maximum allowed level.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_GraphToNetworkCount( Ivy_Man_t * p, Ivy_Obj_t * pRoot, Dec_Graph_t * pGraph, int NodeMax, int LevelMax )
{
    Dec_Node_t * pNode, * pNode0, * pNode1;
    Ivy_Obj_t * pAnd, * pAnd0, * pAnd1;
    int i, Counter, LevelNew, LevelOld;
    // check for constant function or a literal
    if ( Dec_GraphIsConst(pGraph) || Dec_GraphIsVar(pGraph) )
        return 0;
    // set the levels of the leaves
    Dec_GraphForEachLeaf( pGraph, pNode, i )
        pNode->Level = Ivy_Regular((Ivy_Obj_t *)pNode->pFunc)->Level;
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
        if ( pAnd0 && pAnd1 )
        {
            // if they are both present, find the resulting node
            pAnd0 = Ivy_NotCond( pAnd0, pNode->eEdge0.fCompl );
            pAnd1 = Ivy_NotCond( pAnd1, pNode->eEdge1.fCompl );
            pAnd  = Ivy_TableLookup( p, Ivy_ObjCreateGhost(p, pAnd0, pAnd1, IVY_AND, IVY_INIT_NONE) );
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
        // count the number of new levels
        LevelNew = 1 + RWT_MAX( pNode0->Level, pNode1->Level );
        if ( pAnd )
        {
            if ( Ivy_Regular(pAnd) == p->pConst1 )
                LevelNew = 0;
            else if ( Ivy_Regular(pAnd) == Ivy_Regular(pAnd0) )
                LevelNew = (int)Ivy_Regular(pAnd0)->Level;
            else if ( Ivy_Regular(pAnd) == Ivy_Regular(pAnd1) )
                LevelNew = (int)Ivy_Regular(pAnd1)->Level;
            LevelOld = (int)Ivy_Regular(pAnd)->Level;
//            assert( LevelNew == LevelOld );
        }
        if ( LevelNew > LevelMax )
            return -1;
        pNode->pFunc = pAnd;
        pNode->Level = LevelNew;
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
Ivy_Obj_t * Ivy_GraphToNetwork( Ivy_Man_t * p, Dec_Graph_t * pGraph )
{
    Ivy_Obj_t * pAnd0, * pAnd1;
    Dec_Node_t * pNode = NULL; // Suppress "might be used uninitialized"
    int i;
    // check for constant function
    if ( Dec_GraphIsConst(pGraph) )
        return Ivy_NotCond( Ivy_ManConst1(p), Dec_GraphIsComplement(pGraph) );
    // check for a literal
    if ( Dec_GraphIsVar(pGraph) )
        return Ivy_NotCond( (Ivy_Obj_t *)Dec_GraphVar(pGraph)->pFunc, Dec_GraphIsComplement(pGraph) );
    // build the AIG nodes corresponding to the AND gates of the graph
    Dec_GraphForEachNode( pGraph, pNode, i )
    {
        pAnd0 = Ivy_NotCond( (Ivy_Obj_t *)Dec_GraphNode(pGraph, pNode->eEdge0.Node)->pFunc, pNode->eEdge0.fCompl ); 
        pAnd1 = Ivy_NotCond( (Ivy_Obj_t *)Dec_GraphNode(pGraph, pNode->eEdge1.Node)->pFunc, pNode->eEdge1.fCompl ); 
        pNode->pFunc = Ivy_And( p, pAnd0, pAnd1 );
    }
    // complement the result if necessary
    return Ivy_NotCond( (Ivy_Obj_t *)pNode->pFunc, Dec_GraphIsComplement(pGraph) );
}

/**Function*************************************************************

  Synopsis    [Replaces MFFC of the node by the new factored form.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_GraphUpdateNetwork( Ivy_Man_t * p, Ivy_Obj_t * pRoot, Dec_Graph_t * pGraph, int fUpdateLevel, int nGain )
{
    Ivy_Obj_t * pRootNew;
    int nNodesNew, nNodesOld, Required;
    Required = fUpdateLevel? Vec_IntEntry( p->vRequired, pRoot->Id ) : 1000000;
    nNodesOld = Ivy_ManNodeNum(p);
    // create the new structure of nodes
    pRootNew = Ivy_GraphToNetwork( p, pGraph );
    assert( (int)Ivy_Regular(pRootNew)->Level <= Required );
//    if ( Ivy_Regular(pRootNew)->Level == Required )
//        printf( "Difference %d.\n", Ivy_Regular(pRootNew)->Level - Required );
    // remove the old nodes
//    Ivy_AigReplace( pMan->pManFunc, pRoot, pRootNew, fUpdateLevel );
/*
    if ( Ivy_IsComplement(pRootNew) )
        printf( "c" );
    else
        printf( "d" );
    if ( Ivy_ObjRefs(Ivy_Regular(pRootNew)) > 0 )
        printf( "%d", Ivy_ObjRefs(Ivy_Regular(pRootNew)) );
    printf( " " );
*/
    Ivy_ObjReplace( p, pRoot, pRootNew, 1, 0, 1 );
    // compare the gains
    nNodesNew = Ivy_ManNodeNum(p);
    assert( nGain <= nNodesOld - nNodesNew );
    // propagate the buffer
    Ivy_ManPropagateBuffers( p, 1 );
}

/**Function*************************************************************

  Synopsis    [Replaces MFFC of the node by the new factored form.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_GraphUpdateNetwork3( Ivy_Man_t * p, Ivy_Obj_t * pRoot, Dec_Graph_t * pGraph, int fUpdateLevel, int nGain )
{
    Ivy_Obj_t * pRootNew, * pFanin;
    int nNodesNew, nNodesOld, i, nRefsOld;
    nNodesOld = Ivy_ManNodeNum(p);

//printf( "Before = %d. ", Ivy_ManNodeNum(p) );
    // mark the cut
    Vec_PtrForEachEntry( Ivy_Obj_t *, ((Rwt_Man_t *)p->pData)->vFanins, pFanin, i )
        Ivy_ObjRefsInc( Ivy_Regular(pFanin) );
    // deref the old cone
    nRefsOld = pRoot->nRefs;  
    pRoot->nRefs = 0;
    Ivy_ObjDelete_rec( p, pRoot, 0 );
    pRoot->nRefs = nRefsOld;
    // unmark the cut
    Vec_PtrForEachEntry( Ivy_Obj_t *, ((Rwt_Man_t *)p->pData)->vFanins, pFanin, i )
        Ivy_ObjRefsDec( Ivy_Regular(pFanin) );
//printf( "Deref = %d. ", Ivy_ManNodeNum(p) );
 
    // create the new structure of nodes
    pRootNew = Ivy_GraphToNetwork( p, pGraph );
//printf( "Create = %d. ", Ivy_ManNodeNum(p) );
    // remove the old nodes
//    Ivy_AigReplace( pMan->pManFunc, pRoot, pRootNew, fUpdateLevel );
/*
    if ( Ivy_IsComplement(pRootNew) )
        printf( "c" );
    else
        printf( "d" );
    if ( Ivy_ObjRefs(Ivy_Regular(pRootNew)) > 0 )
        printf( "%d", Ivy_ObjRefs(Ivy_Regular(pRootNew)) );
    printf( " " );
*/
    Ivy_ObjReplace( p, pRoot, pRootNew, 0, 0, 1 );
//printf( "Replace = %d. ", Ivy_ManNodeNum(p) );

    // delete remaining dangling nodes
    Vec_PtrForEachEntry( Ivy_Obj_t *, ((Rwt_Man_t *)p->pData)->vFanins, pFanin, i )
    {
        pFanin = Ivy_Regular(pFanin);
        if ( !Ivy_ObjIsNone(pFanin) && Ivy_ObjRefs(pFanin) == 0 )
            Ivy_ObjDelete_rec( p, pFanin, 1 );
    }
//printf( "Deref = %d. ", Ivy_ManNodeNum(p) );
//printf( "\n" );

    // compare the gains
    nNodesNew = Ivy_ManNodeNum(p);
    assert( nGain <= nNodesOld - nNodesNew );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

