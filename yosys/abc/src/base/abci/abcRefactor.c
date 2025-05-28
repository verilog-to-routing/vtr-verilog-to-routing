/**CFile****************************************************************

  FileName    [abcRefactor.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Resynthesis based on collapsing and refactoring.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcRefactor.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "bool/dec/dec.h"
#include "bool/kit/kit.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Abc_ManRef_t_   Abc_ManRef_t;
struct Abc_ManRef_t_
{
    // user specified parameters
    int              nNodeSizeMax;      // the limit on the size of the supernode
    int              nConeSizeMax;      // the limit on the size of the containing cone
    int              fVerbose;          // the verbosity flag
    // internal data structures
    Vec_Ptr_t *      vVars;             // truth tables
    Vec_Ptr_t *      vFuncs;            // functions
    Vec_Int_t *      vMemory;           // memory
    Vec_Str_t *      vCube;             // temporary
    Vec_Int_t *      vForm;             // temporary
    Vec_Ptr_t *      vVisited;          // temporary
    Vec_Ptr_t *      vLeaves;           // temporary
    // node statistics
    int              nLastGain;
    int              nNodesConsidered;
    int              nNodesRefactored;
    int              nNodesGained;
    int              nNodesBeg;
    int              nNodesEnd;
    // runtime statistics
    abctime          timeCut;
    abctime          timeTru;
    abctime          timeDcs;
    abctime          timeSop;
    abctime          timeFact;
    abctime          timeEval;
    abctime          timeRes;
    abctime          timeNtk;
    abctime          timeTotal;
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns function of the cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
word * Abc_NodeConeTruth( Vec_Ptr_t * vVars, Vec_Ptr_t * vFuncs, int nWordsMax, Abc_Obj_t * pRoot, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vVisited )
{
    Abc_Obj_t * pNode;
    word * pTruth0, * pTruth1, * pTruth = NULL;
    int i, k, nWords = Abc_Truth6WordNum( Vec_PtrSize(vLeaves) );
    // get nodes in the cut without fanins in the DFS order
    Abc_NodeConeCollect( &pRoot, 1, vLeaves, vVisited, 0 );
    // set elementary functions
    Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pNode, i )
        pNode->pCopy = (Abc_Obj_t *)Vec_PtrEntry( vVars, i );
    // prepare functions
    for ( i = Vec_PtrSize(vFuncs); i < Vec_PtrSize(vVisited); i++ )
        Vec_PtrPush( vFuncs, ABC_ALLOC(word, nWordsMax) );
    // compute functions for the collected nodes
    Vec_PtrForEachEntry( Abc_Obj_t *, vVisited, pNode, i )
    {
        assert( !Abc_ObjIsPi(pNode) );
        pTruth0 = (word *)Abc_ObjFanin0(pNode)->pCopy;
        pTruth1 = (word *)Abc_ObjFanin1(pNode)->pCopy;
        pTruth  = (word *)Vec_PtrEntry( vFuncs, i );
        if ( Abc_ObjFaninC0(pNode) )
        {
            if ( Abc_ObjFaninC1(pNode) )
                for ( k = 0; k < nWords; k++ )
                    pTruth[k] = ~pTruth0[k] & ~pTruth1[k];
            else
                for ( k = 0; k < nWords; k++ )
                    pTruth[k] = ~pTruth0[k] &  pTruth1[k];
        }
        else
        {
            if ( Abc_ObjFaninC1(pNode) )
                for ( k = 0; k < nWords; k++ )
                    pTruth[k] =  pTruth0[k] & ~pTruth1[k];
            else
                for ( k = 0; k < nWords; k++ )
                    pTruth[k] =  pTruth0[k] &  pTruth1[k];
        }
        pNode->pCopy = (Abc_Obj_t *)pTruth;
    }
    return pTruth;
}
int Abc_NodeConeIsConst0( word * pTruth, int nVars )
{
    int k, nWords = Abc_Truth6WordNum( nVars );
    for ( k = 0; k < nWords; k++ )
        if ( pTruth[k] )
            return 0;
    return 1;
}
int Abc_NodeConeIsConst1( word * pTruth, int nVars )
{
    int k, nWords = Abc_Truth6WordNum( nVars );
    for ( k = 0; k < nWords; k++ )
        if ( ~pTruth[k] )
            return 0;
    return 1;
}


/**Function*************************************************************

  Synopsis    [Resynthesizes the node using refactoring.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dec_Graph_t * Abc_NodeRefactor( Abc_ManRef_t * p, Abc_Obj_t * pNode, Vec_Ptr_t * vFanins, int nMinSaved, int fUpdateLevel, int fUseZeros, int fUseDcs, int fVerbose )
{
    extern int    Dec_GraphToNetworkCount( Abc_Obj_t * pRoot, Dec_Graph_t * pGraph, int NodeMax, int LevelMax );
    int fVeryVerbose = 0;
    int nVars = Vec_PtrSize(vFanins);
    int nWordsMax = Abc_Truth6WordNum(p->nNodeSizeMax);
    Dec_Graph_t * pFForm;
    Abc_Obj_t * pFanin;
    word * pTruth;
    abctime clk;
    int i, nNodesSaved, nNodesAdded, Required;
    if ( fUseZeros )
        nMinSaved = 0;

    p->nNodesConsidered++;

    Required = fUpdateLevel? Abc_ObjRequiredLevel(pNode) : ABC_INFINITY;

    // get the function of the cut
clk = Abc_Clock();
    pTruth = Abc_NodeConeTruth( p->vVars, p->vFuncs, nWordsMax, pNode, vFanins, p->vVisited );
p->timeTru += Abc_Clock() - clk;
    if ( pTruth == NULL )
        return NULL;

    // always accept the case of constant node
    if ( Abc_NodeConeIsConst0(pTruth, nVars) || Abc_NodeConeIsConst1(pTruth, nVars) )
    {
        p->nLastGain = Abc_NodeMffcSize( pNode );
        p->nNodesGained += p->nLastGain;
        p->nNodesRefactored++;
        return Abc_NodeConeIsConst0(pTruth, nVars) ? Dec_GraphCreateConst0() : Dec_GraphCreateConst1();
    }

    // get the factored form
clk = Abc_Clock();
    pFForm = (Dec_Graph_t *)Kit_TruthToGraph( (unsigned *)pTruth, nVars, p->vMemory );
p->timeFact += Abc_Clock() - clk;

    // mark the fanin boundary 
    // (can mark only essential fanins, belonging to bNodeFunc!)
    Vec_PtrForEachEntry( Abc_Obj_t *, vFanins, pFanin, i )
        pFanin->vFanouts.nSize++;
    // label MFFC with current traversal ID
    Abc_NtkIncrementTravId( pNode->pNtk );
    nNodesSaved = Abc_NodeMffcLabelAig( pNode );
    // unmark the fanin boundary and set the fanins as leaves in the form
    Vec_PtrForEachEntry( Abc_Obj_t *, vFanins, pFanin, i )
    {
        pFanin->vFanouts.nSize--;
        Dec_GraphNode(pFForm, i)->pFunc = pFanin;
    }

    // detect how many new nodes will be added (while taking into account reused nodes)
clk = Abc_Clock();
    nNodesAdded = Dec_GraphToNetworkCount( pNode, pFForm, nNodesSaved, Required );
p->timeEval += Abc_Clock() - clk;
    // quit if there is no improvement
    //if ( nNodesAdded == -1 || (nNodesAdded == nNodesSaved && !fUseZeros) )
    if ( nNodesAdded == -1 || nNodesSaved - nNodesAdded < nMinSaved )
    {
        Dec_GraphFree( pFForm );
        return NULL;
    }

    // compute the total gain in the number of nodes
    p->nLastGain = nNodesSaved - nNodesAdded;
    p->nNodesGained += p->nLastGain;
    p->nNodesRefactored++;

    // report the progress
    if ( fVeryVerbose )
    {
        printf( "Node %6s : ",  Abc_ObjName(pNode) );
        printf( "Cone = %2d. ", vFanins->nSize );
        printf( "FF = %2d. ",   1 + Dec_GraphNodeNum(pFForm) );
        printf( "MFFC = %2d. ", nNodesSaved );
        printf( "Add = %2d. ",  nNodesAdded );
        printf( "GAIN = %2d. ", p->nLastGain );
        printf( "\n" );
    }
    return pFForm;
}


/**Function*************************************************************

  Synopsis    [Starts the resynthesis manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_ManRef_t * Abc_NtkManRefStart( int nNodeSizeMax, int nConeSizeMax, int fUseDcs, int fVerbose )
{
    Abc_ManRef_t * p;
    p = ABC_ALLOC( Abc_ManRef_t, 1 );
    memset( p, 0, sizeof(Abc_ManRef_t) );
    p->vCube        = Vec_StrAlloc( 100 );
    p->vVisited     = Vec_PtrAlloc( 100 );
    p->nNodeSizeMax = nNodeSizeMax;
    p->nConeSizeMax = nConeSizeMax;
    p->fVerbose     = fVerbose;
    p->vVars        = Vec_PtrAllocTruthTables( Abc_MaxInt(nNodeSizeMax, 6) );
    p->vFuncs       = Vec_PtrAlloc( 100 );
    p->vMemory      = Vec_IntAlloc( 1 << 16 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops the resynthesis manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkManRefStop( Abc_ManRef_t * p )
{
    Vec_PtrFreeFree( p->vFuncs );
    Vec_PtrFree( p->vVars );
    Vec_IntFree( p->vMemory );
    Vec_PtrFree( p->vVisited );
    Vec_StrFree( p->vCube );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Stops the resynthesis manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkManRefPrintStats( Abc_ManRef_t * p )
{
    printf( "Refactoring statistics:\n" );
    printf( "Nodes considered  = %8d.\n", p->nNodesConsidered );
    printf( "Nodes refactored  = %8d.\n", p->nNodesRefactored );
    printf( "Gain              = %8d. (%6.2f %%).\n", p->nNodesBeg-p->nNodesEnd, 100.0*(p->nNodesBeg-p->nNodesEnd)/p->nNodesBeg );
    ABC_PRT( "Cuts       ", p->timeCut );
    ABC_PRT( "Resynthesis", p->timeRes );
    ABC_PRT( "    BDD    ", p->timeTru );
    ABC_PRT( "    DCs    ", p->timeDcs );
    ABC_PRT( "    SOP    ", p->timeSop );
    ABC_PRT( "    FF     ", p->timeFact );
    ABC_PRT( "    Eval   ", p->timeEval );
    ABC_PRT( "AIG update ", p->timeNtk );
    ABC_PRT( "TOTAL      ", p->timeTotal );
}

/**Function*************************************************************

  Synopsis    [Performs incremental resynthesis of the AIG.]

  Description [Starting from each node, computes a reconvergence-driven cut, 
  derives BDD of the cut function, constructs ISOP, factors the ISOP, 
  and replaces the current implementation of the MFFC of the node by the 
  new factored form, if the number of AIG nodes is reduced and the total
  number of levels of the AIG network is not increated. Returns the
  number of AIG nodes saved.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRefactor( Abc_Ntk_t * pNtk, int nNodeSizeMax, int nMinSaved, int nConeSizeMax, int fUpdateLevel, int fUseZeros, int fUseDcs, int fVerbose )
{
    extern int           Dec_GraphUpdateNetwork( Abc_Obj_t * pRoot, Dec_Graph_t * pGraph, int fUpdateLevel, int nGain );
    ProgressBar * pProgress;
    Abc_ManRef_t * pManRef;
    Abc_ManCut_t * pManCut;
    Dec_Graph_t * pFForm;
    Vec_Ptr_t * vFanins;
    Abc_Obj_t * pNode;
    abctime clk, clkStart = Abc_Clock();
    int i, nNodes, RetValue = 1;

    assert( Abc_NtkIsStrash(pNtk) );
    // cleanup the AIG
    Abc_AigCleanup((Abc_Aig_t *)pNtk->pManFunc);
    // start the managers
    pManCut = Abc_NtkManCutStart( nNodeSizeMax, nConeSizeMax, 2, 1000 );
    pManRef = Abc_NtkManRefStart( nNodeSizeMax, nConeSizeMax, fUseDcs, fVerbose );
    pManRef->vLeaves   = Abc_NtkManCutReadCutLarge( pManCut );
    // compute the reverse levels if level update is requested
    if ( fUpdateLevel )
        Abc_NtkStartReverseLevels( pNtk, 0 );

    // resynthesize each node once
    pManRef->nNodesBeg = Abc_NtkNodeNum(pNtk);
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
        // skip the nodes with many fanouts
        if ( Abc_ObjFanoutNum(pNode) > 1000 )
            continue;
        // stop if all nodes have been tried once
        if ( i >= nNodes )
            break;
        // compute a reconvergence-driven cut
clk = Abc_Clock();
        vFanins = Abc_NodeFindCut( pManCut, pNode, fUseDcs );
pManRef->timeCut += Abc_Clock() - clk;
        // evaluate this cut
clk = Abc_Clock();
        pFForm = Abc_NodeRefactor( pManRef, pNode, vFanins, nMinSaved, fUpdateLevel, fUseZeros, fUseDcs, fVerbose );
pManRef->timeRes += Abc_Clock() - clk;
        if ( pFForm == NULL )
            continue;
        // acceptable replacement found, update the graph
clk = Abc_Clock();
        if ( !Dec_GraphUpdateNetwork( pNode, pFForm, fUpdateLevel, pManRef->nLastGain ) )
        {
            Dec_GraphFree( pFForm );
            RetValue = -1;
            break;
        }
pManRef->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFForm );
    }
    Extra_ProgressBarStop( pProgress );
pManRef->timeTotal = Abc_Clock() - clkStart;
    pManRef->nNodesEnd = Abc_NtkNodeNum(pNtk);

    // print statistics of the manager
    if ( fVerbose )
        Abc_NtkManRefPrintStats( pManRef );
    // delete the managers
    Abc_NtkManCutStop( pManCut );
    Abc_NtkManRefStop( pManRef );
    // put the nodes into the DFS order and reassign their IDs
    Abc_NtkReassignIds( pNtk );
//    Abc_AigCheckFaninOrder( pNtk->pManFunc );
    if ( RetValue != -1 )
    {
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
    }
    return RetValue;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

