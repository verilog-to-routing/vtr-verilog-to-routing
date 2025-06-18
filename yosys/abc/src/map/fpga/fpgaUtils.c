/**CFile****************************************************************

  FileName    [fpgaUtils.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Technology mapping for variable-size-LUT FPGAs.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - August 18, 2004.]

  Revision    [$Id: fpgaUtils.c,v 1.3 2004/07/06 04:55:58 alanmi Exp $]

***********************************************************************/

#include "fpgaInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define FPGA_CO_LIST_SIZE  5

static void  Fpga_MappingDfs_rec( Fpga_Node_t * pNode, Fpga_NodeVec_t * vNodes, int fCollectEquiv );
static void  Fpga_MappingDfsCuts_rec( Fpga_Node_t * pNode, Fpga_NodeVec_t * vNodes );
static int   Fpga_MappingCompareOutputDelay( Fpga_Node_t ** ppNode1, Fpga_Node_t ** ppNode2 );
static void  Fpga_MappingFindLatest( Fpga_Man_t * p, int * pNodes, int nNodesMax );
static void  Fpga_DfsLim_rec( Fpga_Node_t * pNode, int Level, Fpga_NodeVec_t * vNodes );
static int   Fpga_CollectNodeTfo_rec( Fpga_Node_t * pNode, Fpga_Node_t * pPivot, Fpga_NodeVec_t * vVisited, Fpga_NodeVec_t * vTfo );
static Fpga_NodeVec_t * Fpga_MappingOrderCosByLevel( Fpga_Man_t * pMan );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Computes the DFS ordering of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_NodeVec_t * Fpga_MappingDfs( Fpga_Man_t * pMan, int fCollectEquiv )
{
    Fpga_NodeVec_t * vNodes;//, * vNodesCo;
    Fpga_Node_t * pNode;
    int i;
    // collect the CO nodes by level
//    vNodesCo = Fpga_MappingOrderCosByLevel( pMan );
    // start the array
    vNodes = Fpga_NodeVecAlloc( 100 );
    // collect the PIs
    for ( i = 0; i < pMan->nInputs; i++ )
    {
        pNode = pMan->pInputs[i];
        Fpga_NodeVecPush( vNodes, pNode );
        pNode->fMark0 = 1;
    }
    // perform the traversal
    for ( i = 0; i < pMan->nOutputs; i++ )
        Fpga_MappingDfs_rec( Fpga_Regular(pMan->pOutputs[i]), vNodes, fCollectEquiv );
//    for ( i = vNodesCo->nSize - 1; i >= 0 ; i-- )
//        for ( pNode = vNodesCo->pArray[i]; pNode; pNode = (Fpga_Node_t *)pNode->pData0 )
//            Fpga_MappingDfs_rec( pNode, vNodes, fCollectEquiv );
    // clean the node marks
    for ( i = 0; i < vNodes->nSize; i++ )
        vNodes->pArray[i]->fMark0 = 0;
//    for ( i = 0; i < pMan->nOutputs; i++ )
//        Fpga_MappingUnmark_rec( Fpga_Regular(pMan->pOutputs[i]) );
//    Fpga_NodeVecFree( vNodesCo );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Recursively computes the DFS ordering of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_MappingDfs_rec( Fpga_Node_t * pNode, Fpga_NodeVec_t * vNodes, int fCollectEquiv )
{
    assert( !Fpga_IsComplement(pNode) );
    if ( pNode->fMark0 )
        return;
    // visit the transitive fanin
    if ( Fpga_NodeIsAnd(pNode) )
    {
        Fpga_MappingDfs_rec( Fpga_Regular(pNode->p1), vNodes, fCollectEquiv );
        Fpga_MappingDfs_rec( Fpga_Regular(pNode->p2), vNodes, fCollectEquiv );
    }
    // visit the equivalent nodes
    if ( fCollectEquiv && pNode->pNextE )
        Fpga_MappingDfs_rec( pNode->pNextE, vNodes, fCollectEquiv );
    // make sure the node is not visited through the equivalent nodes
    assert( pNode->fMark0 == 0 );
    // mark the node as visited
    pNode->fMark0 = 1;
    // add the node to the list
    Fpga_NodeVecPush( vNodes, pNode );
}

/**Function*************************************************************

  Synopsis    [Computes the DFS ordering of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_NodeVec_t * Fpga_MappingDfsNodes( Fpga_Man_t * pMan, Fpga_Node_t ** ppNodes, int nNodes, int fEquiv )
{
    Fpga_NodeVec_t * vNodes;
    int i;
    // perform the traversal
    vNodes = Fpga_NodeVecAlloc( 200 );
    for ( i = 0; i < nNodes; i++ )
        Fpga_MappingDfs_rec( ppNodes[i], vNodes, fEquiv );
    for ( i = 0; i < vNodes->nSize; i++ )
        vNodes->pArray[i]->fMark0 = 0;
    return vNodes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Fpga_MappingGetAreaFlow( Fpga_Man_t * p )
{
    float aFlowFlowTotal = 0;
    int i;
    for ( i = 0; i < p->nOutputs; i++ )
    {
        if ( Fpga_NodeIsConst(p->pOutputs[i]) )
            continue;
        aFlowFlowTotal += Fpga_Regular(p->pOutputs[i])->pCutBest->aFlow;
    }
    return aFlowFlowTotal;
}

/**Function*************************************************************

  Synopsis    [Computes the area of the current mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Fpga_MappingArea( Fpga_Man_t * pMan )
{
    Fpga_Node_t * pNode;
    float aTotal;
    int i;
    // perform the traversal
    aTotal = 0;
    for ( i = 0; i < pMan->vMapping->nSize; i++ )
    {
        pNode = pMan->vMapping->pArray[i];
        aTotal += pMan->pLutLib->pLutAreas[(int)pNode->pCutBest->nLeaves];
    }
    return aTotal;
}

/**Function*************************************************************

  Synopsis    [Recursively computes the DFS ordering of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Fpga_MappingArea_rec( Fpga_Man_t * pMan, Fpga_Node_t * pNode, Fpga_NodeVec_t * vNodes )
{
    float aArea;
    int i;
    assert( !Fpga_IsComplement(pNode) );
    if ( !Fpga_NodeIsAnd(pNode) )
        return 0;
    if ( pNode->fMark0 )
        return 0;
    assert( pNode->pCutBest != NULL );
    // visit the transitive fanin of the selected cut
    aArea = 0;
    for ( i = 0; i < pNode->pCutBest->nLeaves; i++ )
        aArea += Fpga_MappingArea_rec( pMan, pNode->pCutBest->ppLeaves[i], vNodes );
    // make sure the node is not visited through the fanin nodes
    assert( pNode->fMark0 == 0 );
    // mark the node as visited
    pNode->fMark0 = 1;
    // add the node to the list
    aArea += pMan->pLutLib->pLutAreas[(int)pNode->pCutBest->nLeaves];
    // add the node to the list
    Fpga_NodeVecPush( vNodes, pNode );
    return aArea;
}

/**Function*************************************************************

  Synopsis    [Computes the area of the current mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Fpga_MappingAreaTrav( Fpga_Man_t * pMan )
{
    Fpga_NodeVec_t * vNodes;
    float aTotal;
    int i;
    // perform the traversal
    aTotal = 0;
    vNodes = Fpga_NodeVecAlloc( 100 );
    for ( i = 0; i < pMan->nOutputs; i++ )
        aTotal += Fpga_MappingArea_rec( pMan, Fpga_Regular(pMan->pOutputs[i]), vNodes );
    for ( i = 0; i < vNodes->nSize; i++ )
        vNodes->pArray[i]->fMark0 = 0;
    Fpga_NodeVecFree( vNodes );
    return aTotal;
}


/**Function*************************************************************

  Synopsis    [Recursively computes the DFS ordering of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Fpga_MappingSetRefsAndArea_rec( Fpga_Man_t * pMan, Fpga_Node_t * pNode, Fpga_Node_t ** ppStore )
{
    float aArea;
    int i;
    assert( !Fpga_IsComplement(pNode) );
    if ( pNode->nRefs++ )
        return 0;
    if ( !Fpga_NodeIsAnd(pNode) )
        return 0;
    assert( pNode->pCutBest != NULL );
    // store the node in the structure by level
    pNode->pData0 = (char *)ppStore[pNode->Level]; 
    ppStore[pNode->Level] = pNode;
    // visit the transitive fanin of the selected cut
    aArea = pMan->pLutLib->pLutAreas[(int)pNode->pCutBest->nLeaves];
    for ( i = 0; i < pNode->pCutBest->nLeaves; i++ )
        aArea += Fpga_MappingSetRefsAndArea_rec( pMan, pNode->pCutBest->ppLeaves[i], ppStore );
    return aArea;
}

/**Function*************************************************************

  Synopsis    [Sets the correct reference counts for the mapping.]

  Description [Collects the nodes in reverse topological order
  and places in them in array pMan->vMapping.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Fpga_MappingSetRefsAndArea( Fpga_Man_t * pMan )
{
    Fpga_Node_t * pNode, ** ppStore;
    float aArea;
    int i, LevelMax;

    // clean all references
    for ( i = 0; i < pMan->vNodesAll->nSize; i++ )
        pMan->vNodesAll->pArray[i]->nRefs = 0;

    // allocate place to store the nodes
    LevelMax = Fpga_MappingMaxLevel( pMan );
    ppStore = ABC_ALLOC( Fpga_Node_t *, LevelMax + 1 );
    memset( ppStore, 0, sizeof(Fpga_Node_t *) * (LevelMax + 1) );

    // collect nodes reachable from POs in the DFS order through the best cuts
    aArea = 0;
    for ( i = 0; i < pMan->nOutputs; i++ )
    {
        pNode = Fpga_Regular(pMan->pOutputs[i]);
        if ( pNode == pMan->pConst1 )
            continue;
        aArea += Fpga_MappingSetRefsAndArea_rec( pMan, pNode, ppStore );
        pNode->nRefs++;
    }

    // reconnect the nodes in reverse topological order
    pMan->vMapping->nSize = 0;
    for ( i = LevelMax; i >= 0; i-- )
        for ( pNode = ppStore[i]; pNode; pNode = (Fpga_Node_t *)pNode->pData0 )
            Fpga_NodeVecPush( pMan->vMapping, pNode );
    ABC_FREE( ppStore );
    return aArea;
}


/**Function*************************************************************

  Synopsis    [Compares the outputs by their arrival times.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_MappingCompareOutputDelay( Fpga_Node_t ** ppNode1, Fpga_Node_t ** ppNode2 )
{
    Fpga_Node_t * pNode1 = Fpga_Regular(*ppNode1);
    Fpga_Node_t * pNode2 = Fpga_Regular(*ppNode2);
    float Arrival1 = pNode1->pCutBest? pNode1->pCutBest->tArrival : 0;
    float Arrival2 = pNode2->pCutBest? pNode2->pCutBest->tArrival : 0;
    if ( Arrival1 < Arrival2 )
        return -1;
    if ( Arrival1 > Arrival2 )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Finds given number of latest arriving COs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_MappingFindLatest( Fpga_Man_t * p, int * pNodes, int nNodesMax )
{
    int nNodes, i, k, v;
    assert( p->nOutputs >= nNodesMax );
    pNodes[0] = 0;
    nNodes = 1;
    for ( i = 1; i < p->nOutputs; i++ )
    {
        for ( k = nNodes - 1; k >= 0; k-- )
            if ( Fpga_MappingCompareOutputDelay( &p->pOutputs[pNodes[k]], &p->pOutputs[i] ) >= 0 )
                break;
        if ( k == nNodesMax - 1 )
            continue;
        if ( nNodes < nNodesMax )
            nNodes++;
        for ( v = nNodes - 1; v > k+1; v-- )
            pNodes[v] = pNodes[v-1];
        pNodes[k+1] = i;
    }
}

/**Function*************************************************************

  Synopsis    [Prints a bunch of latest arriving outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_MappingPrintOutputArrivals( Fpga_Man_t * p )
{
    Fpga_Node_t * pNode;
    int pSorted[FPGA_CO_LIST_SIZE];
    int fCompl, Limit, MaxNameSize, i;

    // determine the number of nodes to print
    Limit = (p->nOutputs > FPGA_CO_LIST_SIZE)? FPGA_CO_LIST_SIZE : p->nOutputs;

    // determine the order
    Fpga_MappingFindLatest( p, pSorted, Limit );

    // determine max size of the node's name
    MaxNameSize = 0;
    for ( i = 0; i < Limit; i++ )
        if ( MaxNameSize < (int)strlen(p->ppOutputNames[pSorted[i]]) )
            MaxNameSize = strlen(p->ppOutputNames[pSorted[i]]);

    // print the latest outputs
    for ( i = 0; i < Limit; i++ )
    {
        // get the i-th latest output
        pNode  = Fpga_Regular(p->pOutputs[pSorted[i]]);
        fCompl = Fpga_IsComplement(p->pOutputs[pSorted[i]]);
        // print out the best arrival time
        printf( "Output  %-*s : ", MaxNameSize + 3, p->ppOutputNames[pSorted[i]] );
        printf( "Delay = %8.2f  ",     (double)pNode->pCutBest->tArrival );
        if ( fCompl )
            printf( "NEG" );
        else
            printf( "POS" );
        printf( "\n" );
    }
}


/**Function*************************************************************

  Synopsis    [Sets up the truth tables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_MappingSetupTruthTables( unsigned uTruths[][2] )
{
    int m, v;
    // set up the truth tables
    for ( m = 0; m < 32; m++ )
        for ( v = 0; v < 5; v++ )
            if ( m & (1 << v) )
                uTruths[v][0] |= (1 << m);
    // make adjustments for the case of 6 variables
    for ( v = 0; v < 5; v++ )
        uTruths[v][1] = uTruths[v][0];
    uTruths[5][0] = 0;
    uTruths[5][1] = FPGA_FULL;
}

/**Function*************************************************************

  Synopsis    [Sets up the mask.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_MappingSetupMask( unsigned uMask[], int nVarsMax )
{
    if ( nVarsMax == 6 )
        uMask[0] = uMask[1] = FPGA_FULL;
    else
    {
        uMask[0] = FPGA_MASK(1 << nVarsMax);
        uMask[1] = 0;
    }
}

/**Function*************************************************************

  Synopsis    [Verify one useful property.]

  Description [This procedure verifies one useful property. After 
  the FRAIG construction with choice nodes is over, each primary node 
  should have fanins that are primary nodes. The primary nodes is the 
  one that does not have pNode->pRepr set to point to another node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_ManCheckConsistency( Fpga_Man_t * p )
{
    Fpga_Node_t * pNode;
    Fpga_NodeVec_t * pVec;
    int i;
    pVec = Fpga_MappingDfs( p, 0 );
    for ( i = 0; i < pVec->nSize; i++ )
    {
        pNode = pVec->pArray[i];
        if ( Fpga_NodeIsVar(pNode) )
        {
            if ( pNode->pRepr )
                printf( "Primary input %d is a secondary node.\n", pNode->Num );
        }
        else if ( Fpga_NodeIsConst(pNode) )
        {
            if ( pNode->pRepr )
                printf( "Constant 1 %d is a secondary node.\n", pNode->Num );
        }
        else
        {
            if ( pNode->pRepr )
                printf( "Internal node %d is a secondary node.\n", pNode->Num );
            if ( Fpga_Regular(pNode->p1)->pRepr )
                printf( "Internal node %d has first fanin that is a secondary node.\n", pNode->Num );
            if ( Fpga_Regular(pNode->p2)->pRepr )
                printf( "Internal node %d has second fanin that is a secondary node.\n", pNode->Num );
        }
    }
    Fpga_NodeVecFree( pVec );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Compares the supergates by their level.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_CompareNodesByLevelDecreasing( Fpga_Node_t ** ppS1, Fpga_Node_t ** ppS2 )
{
    if ( Fpga_Regular(*ppS1)->Level > Fpga_Regular(*ppS2)->Level )
        return -1;
    if ( Fpga_Regular(*ppS1)->Level < Fpga_Regular(*ppS2)->Level )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Compares the supergates by their level.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_CompareNodesByLevelIncreasing( Fpga_Node_t ** ppS1, Fpga_Node_t ** ppS2 )
{
    if ( Fpga_Regular(*ppS1)->Level < Fpga_Regular(*ppS2)->Level )
        return -1;
    if ( Fpga_Regular(*ppS1)->Level > Fpga_Regular(*ppS2)->Level )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Orders the nodes in the decreasing order of levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_MappingSortByLevel( Fpga_Man_t * pMan, Fpga_NodeVec_t * vNodes, int fIncreasing )
{
    if ( fIncreasing )
        qsort( (void *)vNodes->pArray, (size_t)vNodes->nSize, sizeof(Fpga_Node_t *), 
                (int (*)(const void *, const void *)) Fpga_CompareNodesByLevelIncreasing );
    else
        qsort( (void *)vNodes->pArray, (size_t)vNodes->nSize, sizeof(Fpga_Node_t *), 
                (int (*)(const void *, const void *)) Fpga_CompareNodesByLevelDecreasing );
//    assert( Fpga_CompareNodesByLevel( vNodes->pArray, vNodes->pArray + vNodes->nSize - 1 ) <= 0 );
}

/**Function*************************************************************

  Synopsis    [Computes the limited DFS ordering for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_NodeVec_t * Fpga_DfsLim( Fpga_Man_t * pMan, Fpga_Node_t * pNode, int nLevels )
{
    Fpga_NodeVec_t * vNodes;
    int i;
    // perform the traversal
    vNodes = Fpga_NodeVecAlloc( 100 );
    Fpga_DfsLim_rec( pNode, nLevels, vNodes );
    for ( i = 0; i < vNodes->nSize; i++ )
        vNodes->pArray[i]->fMark0 = 0;
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Recursively computes the DFS ordering of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_DfsLim_rec( Fpga_Node_t * pNode, int Level, Fpga_NodeVec_t * vNodes )
{
    assert( !Fpga_IsComplement(pNode) );
    if ( pNode->fMark0 )
        return;
    pNode->fMark0 = 1;
    // visit the transitive fanin
    Level--;
    if ( Level > 0 && Fpga_NodeIsAnd(pNode) )
    {
        Fpga_DfsLim_rec( Fpga_Regular(pNode->p1), Level, vNodes );
        Fpga_DfsLim_rec( Fpga_Regular(pNode->p2), Level, vNodes );
    }
    // add the node to the list
    Fpga_NodeVecPush( vNodes, pNode );
}

/**Function*************************************************************

  Synopsis    [Computes the limited DFS ordering for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_ManCleanData0( Fpga_Man_t * pMan )
{
    int i;
    for ( i = 0; i < pMan->vNodesAll->nSize; i++ )
        pMan->vNodesAll->pArray[i]->pData0 = 0;
}

/**Function*************************************************************

  Synopsis    [Collects the TFO of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_NodeVec_t * Fpga_CollectNodeTfo( Fpga_Man_t * pMan, Fpga_Node_t * pNode )
{
    Fpga_NodeVec_t * vVisited, * vTfo;
    int i;
    // perform the traversal
    vVisited = Fpga_NodeVecAlloc( 100 );
    vTfo     = Fpga_NodeVecAlloc( 100 );
    for ( i = 0; i < pMan->nOutputs; i++ )
        Fpga_CollectNodeTfo_rec( Fpga_Regular(pMan->pOutputs[i]), pNode, vVisited, vTfo );
    for ( i = 0; i < vVisited->nSize; i++ )
        vVisited->pArray[i]->fMark0 = vVisited->pArray[i]->fMark1 = 0;
    Fpga_NodeVecFree( vVisited );
    return vTfo;
}

/**Function*************************************************************

  Synopsis    [Collects the TFO of the node.]

  Description [Returns 1 if the node should be collected.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_CollectNodeTfo_rec( Fpga_Node_t * pNode, Fpga_Node_t * pPivot, Fpga_NodeVec_t * vVisited, Fpga_NodeVec_t * vTfo )
{
    int Ret1, Ret2;
    assert( !Fpga_IsComplement(pNode) );
    // skip visited nodes
    if ( pNode->fMark0 )
        return pNode->fMark1;
    pNode->fMark0 = 1;
    Fpga_NodeVecPush( vVisited, pNode );

    // return the pivot node
    if ( pNode == pPivot )
    {
        pNode->fMark1 = 1;
        return 1;
    }
    if ( pNode->Level < pPivot->Level )
    {
        pNode->fMark1 = 0;
        return 0;
    }
    // visit the transitive fanin
    assert( Fpga_NodeIsAnd(pNode) );
    Ret1 = Fpga_CollectNodeTfo_rec( Fpga_Regular(pNode->p1), pPivot, vVisited, vTfo );
    Ret2 = Fpga_CollectNodeTfo_rec( Fpga_Regular(pNode->p2), pPivot, vVisited, vTfo );
    if ( Ret1 || Ret2 )
    {
        pNode->fMark1 = 1;
        Fpga_NodeVecPush( vTfo, pNode );
    }
    else
        pNode->fMark1 = 0;
    return pNode->fMark1;
}

/**Function*************************************************************

  Synopsis    [Levelizes the nodes accessible from the POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_NodeVec_t * Fpga_MappingLevelize( Fpga_Man_t * pMan, Fpga_NodeVec_t * vNodes )
{
    Fpga_NodeVec_t * vLevels;
    Fpga_Node_t ** ppNodes;
    Fpga_Node_t * pNode;
    int nNodes, nLevelsMax, i;

    // reassign the levels (this may be necessary for networks which choices)
    ppNodes = vNodes->pArray;
    nNodes  = vNodes->nSize;
    for ( i = 0; i < nNodes; i++ )
    {
        pNode = ppNodes[i];
        if ( !Fpga_NodeIsAnd(pNode) )
        {
            pNode->Level = 0;
            continue;
        }
        pNode->Level = 1 + FPGA_MAX( Fpga_Regular(pNode->p1)->Level, Fpga_Regular(pNode->p2)->Level );
    }

    // get the max levels
    nLevelsMax = 0;
    for ( i = 0; i < pMan->nOutputs; i++ )
        nLevelsMax = FPGA_MAX( nLevelsMax, (int)Fpga_Regular(pMan->pOutputs[i])->Level );
    nLevelsMax++;

    // allocate storage for levels
    vLevels = Fpga_NodeVecAlloc( nLevelsMax );
    for ( i = 0; i < nLevelsMax; i++ )
        Fpga_NodeVecPush( vLevels, NULL );

    // go through the nodes and add them to the levels
    for ( i = 0; i < nNodes; i++ )
    {
        pNode = ppNodes[i];
        pNode->pLevel = NULL;
        if ( !Fpga_NodeIsAnd(pNode) )
            continue;
        // attach the node to this level
        pNode->pLevel = Fpga_NodeVecReadEntry( vLevels, pNode->Level );
        Fpga_NodeVecWriteEntry( vLevels, pNode->Level, pNode );
    }
    return vLevels;
}

/**Function*************************************************************

  Synopsis    [Sets up the mask.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_MappingMaxLevel( Fpga_Man_t * pMan )
{
    int nLevelMax, i;
    nLevelMax = 0;
    for ( i = 0; i < pMan->nOutputs; i++ )
        nLevelMax = nLevelMax > (int)Fpga_Regular(pMan->pOutputs[i])->Level? 
                nLevelMax : (int)Fpga_Regular(pMan->pOutputs[i])->Level;
    return nLevelMax;
}


/**Function*************************************************************

  Synopsis    [Analyses choice nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_MappingUpdateLevel_rec( Fpga_Man_t * pMan, Fpga_Node_t * pNode, int fMaximum )
{
    Fpga_Node_t * pTemp;
    int Level1, Level2, LevelE;
    assert( !Fpga_IsComplement(pNode) );
    if ( !Fpga_NodeIsAnd(pNode) )
        return pNode->Level;
    // skip the visited node
    if ( pNode->TravId == pMan->nTravIds )
        return pNode->Level;
    pNode->TravId = pMan->nTravIds;
    // compute levels of the children nodes
    Level1 = Fpga_MappingUpdateLevel_rec( pMan, Fpga_Regular(pNode->p1), fMaximum );
    Level2 = Fpga_MappingUpdateLevel_rec( pMan, Fpga_Regular(pNode->p2), fMaximum );
    pNode->Level = 1 + FPGA_MAX( Level1, Level2 );
    if ( pNode->pNextE )
    {
        LevelE = Fpga_MappingUpdateLevel_rec( pMan, pNode->pNextE, fMaximum );
        if ( fMaximum )
        {
            if ( pNode->Level < (unsigned)LevelE )
                pNode->Level = LevelE;
        }
        else
        {
            if ( pNode->Level > (unsigned)LevelE )
                pNode->Level = LevelE;
        }
        // set the level of all equivalent nodes to be the same minimum
        if ( pNode->pRepr == NULL ) // the primary node
            for ( pTemp = pNode->pNextE; pTemp; pTemp = pTemp->pNextE )
                pTemp->Level = pNode->Level;
    }
    return pNode->Level;
}

/**Function*************************************************************

  Synopsis    [Resets the levels of the nodes in the choice graph.]

  Description [Makes the level of the choice nodes to be equal to the
  maximum of the level of the nodes in the equivalence class. This way
  sorting by level leads to the reverse topological order, which is
  needed for the required time computation.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_MappingSetChoiceLevels( Fpga_Man_t * pMan )
{
    int i;
    pMan->nTravIds++;
    for ( i = 0; i < pMan->nOutputs; i++ )
        Fpga_MappingUpdateLevel_rec( pMan, Fpga_Regular(pMan->pOutputs[i]), 1 );
}

/**Function*************************************************************

  Synopsis    [Reports statistics on choice nodes.]

  Description [The number of choice nodes is the number of primary nodes,
  which has pNextE set to a pointer. The number of choices is the number
  of entries in the equivalent-node lists of the primary nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_ManReportChoices( Fpga_Man_t * pMan )
{
    Fpga_Node_t * pNode, * pTemp;
    int nChoiceNodes, nChoices;
    int i, LevelMax1, LevelMax2;

    // report the number of levels
    LevelMax1 = Fpga_MappingMaxLevel( pMan );
    pMan->nTravIds++;
    for ( i = 0; i < pMan->nOutputs; i++ )
        Fpga_MappingUpdateLevel_rec( pMan, Fpga_Regular(pMan->pOutputs[i]), 0 );
    LevelMax2 = Fpga_MappingMaxLevel( pMan );

    // report statistics about choices
    nChoiceNodes = nChoices = 0;
    for ( i = 0; i < pMan->vAnds->nSize; i++ )
    {
        pNode = pMan->vAnds->pArray[i];
        if ( pNode->pRepr == NULL && pNode->pNextE != NULL )
        { // this is a choice node = the primary node that has equivalent nodes
            nChoiceNodes++;
            for ( pTemp = pNode; pTemp; pTemp = pTemp->pNextE )
                nChoices++;
        }
    }
    if ( pMan->fVerbose )
    {
    printf( "Maximum level: Original = %d. Reduced due to choices = %d.\n", LevelMax1, LevelMax2 );
    printf( "Choice stats:  Choice nodes = %d. Total choices = %d.\n", nChoiceNodes, nChoices );
    }
/*
    {
        FILE * pTable;
        pTable = fopen( "stats_choice.txt", "a+" );
        fprintf( pTable, "%s ", pMan->pFileName );
        fprintf( pTable, "%4d ", LevelMax1 );
        fprintf( pTable, "%4d ", pMan->vAnds->nSize - pMan->nInputs );
        fprintf( pTable, "%4d ", LevelMax2 );
        fprintf( pTable, "%7d ", nChoiceNodes );
        fprintf( pTable, "%7d ", nChoices + nChoiceNodes );
        fprintf( pTable, "\n" );
        fclose( pTable );
    }
*/
}

/**Function*************************************************************

  Synopsis    [Returns the array of CO nodes sorted by level.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_NodeVec_t * Fpga_MappingOrderCosByLevel( Fpga_Man_t * pMan )
{
    Fpga_Node_t * pNode;
    Fpga_NodeVec_t * vNodes;
    int i, nLevels;
    // get the largest level of a CO
    nLevels = Fpga_MappingMaxLevel( pMan );
    // allocate the array of nodes
    vNodes = Fpga_NodeVecAlloc( nLevels + 1 );
    for ( i = 0; i <= nLevels; i++ )
        Fpga_NodeVecPush( vNodes, NULL );
    // clean the marks
    for ( i = 0; i < pMan->nOutputs; i++ )
        Fpga_Regular(pMan->pOutputs[i])->fMark0 = 0;
    // put the nodes into the structure
    for ( i = 0; i < pMan->nOutputs; i++ )
    {
        pNode = Fpga_Regular(pMan->pOutputs[i]);
        if ( pNode->fMark0 )
            continue;
        pNode->fMark0 = 1;
        pNode->pData0 = (char *)Fpga_NodeVecReadEntry( vNodes, pNode->Level );
        Fpga_NodeVecWriteEntry( vNodes, pNode->Level, pNode );
    }
    for ( i = 0; i < pMan->nOutputs; i++ )
        Fpga_Regular(pMan->pOutputs[i])->fMark0 = 0;
    return vNodes;

}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

