/**CFile****************************************************************

  FileName    [mapperUtils.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic technology mapping engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - June 1, 2004.]

  Revision    [$Id: mapperUtils.c,v 1.8 2004/11/03 22:41:45 satrajit Exp $]

***********************************************************************/

#include "mapperInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define MAP_CO_LIST_SIZE 5

static int   Map_MappingCountLevels_rec( Map_Node_t * pNode );
static float Map_MappingSetRefsAndArea_rec( Map_Man_t * pMan, Map_Node_t * pNode );
static float Map_MappingSetRefsAndSwitch_rec( Map_Man_t * pMan, Map_Node_t * pNode );
static float Map_MappingSetRefsAndWire_rec( Map_Man_t * pMan, Map_Node_t * pNode );
static void  Map_MappingDfsCuts_rec( Map_Node_t * pNode, Map_NodeVec_t * vNodes );
static float Map_MappingArea_rec( Map_Man_t * pMan, Map_Node_t * pNode, Map_NodeVec_t * vNodes );
static int   Map_MappingCompareOutputDelay( Map_Node_t ** ppNode1, Map_Node_t ** ppNode2 );
static void  Map_MappingFindLatest( Map_Man_t * p, int * pNodes, int nNodesMax );
static unsigned Map_MappingExpandTruth_rec( unsigned uTruth, int nVars );
static int Map_MappingCountUsedNodes( Map_Man_t * pMan, int fChoices );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the DFS ordering of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_MappingDfs_rec( Map_Node_t * pNode, Map_NodeVec_t * vNodes, int fCollectEquiv )
{
    assert( !Map_IsComplement(pNode) );
    if ( pNode->fMark0 )
        return;
    // visit the transitive fanin
    if ( Map_NodeIsAnd(pNode) )
    {
        Map_MappingDfs_rec( Map_Regular(pNode->p1), vNodes, fCollectEquiv );
        Map_MappingDfs_rec( Map_Regular(pNode->p2), vNodes, fCollectEquiv );
    }
    // visit the equivalent nodes
    if ( fCollectEquiv && pNode->pNextE )
        Map_MappingDfs_rec( pNode->pNextE, vNodes, fCollectEquiv );
    // make sure the node is not visited through the equivalent nodes
    assert( pNode->fMark0 == 0 );
    // mark the node as visited
    pNode->fMark0 = 1;
    // add the node to the list
    Map_NodeVecPush( vNodes, pNode );
}
Map_NodeVec_t * Map_MappingDfs( Map_Man_t * pMan, int fCollectEquiv )
{
    Map_NodeVec_t * vNodes;
    int i;
    // perform the traversal
    vNodes = Map_NodeVecAlloc( 100 );
    for ( i = 0; i < pMan->nOutputs; i++ )
        Map_MappingDfs_rec( Map_Regular(pMan->pOutputs[i]), vNodes, fCollectEquiv );
    for ( i = 0; i < vNodes->nSize; i++ )
        vNodes->pArray[i]->fMark0 = 0;
//    for ( i = 0; i < pMan->nOutputs; i++ )
//        Map_MappingUnmark_rec( Map_Regular(pMan->pOutputs[i]) );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Computes the number of logic levels not counting PIs/POs.]

  Description []
               
  SideEffects [Note that this procedure will reassign the levels assigned
  originally by NodeCreate() because it counts the number of levels with 
  choices differently!]

  SeeAlso     []

***********************************************************************/
int Map_MappingCountLevels( Map_Man_t * pMan )
{
    int i, LevelsMax, LevelsCur;
    // perform the traversal
    LevelsMax = -1;
    for ( i = 0; i < pMan->nOutputs; i++ )
    {
        LevelsCur = Map_MappingCountLevels_rec( Map_Regular(pMan->pOutputs[i]) );
        if ( LevelsMax < LevelsCur )
            LevelsMax = LevelsCur;
    }
    for ( i = 0; i < pMan->nOutputs; i++ )
        Map_MappingUnmark_rec( Map_Regular(pMan->pOutputs[i]) );
    return LevelsMax;
}

/**Function*************************************************************

  Synopsis    [Recursively computes the number of logic levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_MappingCountLevels_rec( Map_Node_t * pNode )
{
    int Level1, Level2;
    assert( !Map_IsComplement(pNode) );
    if ( !Map_NodeIsAnd(pNode) )
    {
        pNode->Level = 0;
        return 0;
    }
    if ( pNode->fMark0 )
        return pNode->Level;
    pNode->fMark0 = 1;
    // visit the transitive fanin
    Level1 = Map_MappingCountLevels_rec( Map_Regular(pNode->p1) );
    Level2 = Map_MappingCountLevels_rec( Map_Regular(pNode->p2) );
    // set the number of levels
    pNode->Level = 1 + ((Level1>Level2)? Level1: Level2);
    return pNode->Level;
}

/**Function*************************************************************

  Synopsis    [Unmarks the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_MappingUnmark( Map_Man_t * pMan )
{
    int i;
    for ( i = 0; i < pMan->nOutputs; i++ )
        Map_MappingUnmark_rec( Map_Regular(pMan->pOutputs[i]) );
}

/**Function*************************************************************

  Synopsis    [Recursively unmarks the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_MappingUnmark_rec( Map_Node_t * pNode )
{
    assert( !Map_IsComplement(pNode) );
    if ( pNode->fMark0 == 0 )
        return;
    pNode->fMark0 = 0;
    if ( !Map_NodeIsAnd(pNode) )
        return;
    Map_MappingUnmark_rec( Map_Regular(pNode->p1) );
    Map_MappingUnmark_rec( Map_Regular(pNode->p2) );
    // visit the equivalent nodes
    if ( pNode->pNextE )
        Map_MappingUnmark_rec( pNode->pNextE );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_MappingMark_rec( Map_Node_t * pNode )
{
    assert( !Map_IsComplement(pNode) );
    if ( pNode->fMark0 == 1 )
        return;
    pNode->fMark0 = 1;
    if ( !Map_NodeIsAnd(pNode) )
        return;
    // visit the transitive fanin of the selected cut
    Map_MappingMark_rec( Map_Regular(pNode->p1) );
    Map_MappingMark_rec( Map_Regular(pNode->p2) );
}

/**Function*************************************************************

  Synopsis    [Compares the outputs by their arrival times.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_MappingCompareOutputDelay( Map_Node_t ** ppNode1, Map_Node_t ** ppNode2 )
{
    Map_Node_t * pNode1 = Map_Regular(*ppNode1);
    Map_Node_t * pNode2 = Map_Regular(*ppNode2);
    int fPhase1 = !Map_IsComplement(*ppNode1);
    int fPhase2 = !Map_IsComplement(*ppNode2);
    float Arrival1 = pNode1->tArrival[fPhase1].Worst;
    float Arrival2 = pNode2->tArrival[fPhase2].Worst;
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
void Map_MappingFindLatest( Map_Man_t * p, int * pNodes, int nNodesMax )
{
    int nNodes, i, k, v;
    assert( p->nOutputs >= nNodesMax );
    pNodes[0] = 0;
    nNodes = 1;
    for ( i = 1; i < p->nOutputs; i++ )
    {
        for ( k = nNodes - 1; k >= 0; k-- )
            if ( Map_MappingCompareOutputDelay( &p->pOutputs[pNodes[k]], &p->pOutputs[i] ) >= 0 )
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
void Map_MappingPrintOutputArrivals( Map_Man_t * p )
{
    int pSorted[MAP_CO_LIST_SIZE];
    Map_Time_t * pTimes;
    Map_Node_t * pNode;
    int fPhase, Limit, i;
    int MaxNameSize;

    // determine the number of nodes to print
    Limit = (p->nOutputs > MAP_CO_LIST_SIZE)? MAP_CO_LIST_SIZE : p->nOutputs;

    // determine the order
    Map_MappingFindLatest( p, pSorted, Limit );

    // determine max size of the node's name
    MaxNameSize = 0;
    for ( i = 0; i < Limit; i++ )
        if ( MaxNameSize < (int)strlen(p->ppOutputNames[pSorted[i]]) )
            MaxNameSize = strlen(p->ppOutputNames[pSorted[i]]);

    // print the latest outputs
    for ( i = 0; i < Limit; i++ )
    {
        // get the i-th latest output
        pNode  = Map_Regular(p->pOutputs[pSorted[i]]);
        fPhase =!Map_IsComplement(p->pOutputs[pSorted[i]]);
        pTimes = pNode->tArrival + fPhase;
        // print out the best arrival time
        printf( "Output  %-*s : ", MaxNameSize + 3, p->ppOutputNames[pSorted[i]] );
        printf( "Delay = (%5.2f, %5.2f)  ", (double)pTimes->Rise, (double)pTimes->Fall );
        printf( "%s", fPhase? "POS" : "NEG" );
        printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Sets up the truth tables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_MappingSetupTruthTables( unsigned uTruths[][2] )
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
    uTruths[5][1] = MAP_FULL;
}

/**Function*************************************************************

  Synopsis    [Sets up the truth tables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_MappingSetupTruthTablesLarge( unsigned uTruths[][32] )
{
    int m, v;
    // clean everything
    for ( m = 0; m < 32; m++ )
        for ( v = 0; v < 10; v++ )
            uTruths[v][m] = 0;
    // set up the truth tables
    for ( m = 0; m < 32; m++ )
        for ( v = 0; v < 5; v++ )
            if ( m & (1 << v) )
            {
                uTruths[v][0] |= (1 << m);
                uTruths[v+5][m] = MAP_FULL;
            }
    // extend this info for the rest of the first 5 variables
    for ( m = 0; m < 32; m++ )
        for ( v = 0; v < 5; v++ )
            uTruths[v][m] = uTruths[v][0];
/*
    // verify
    for ( m = 0; m < 1024; m++, printf("\n") )
        for ( v = 0; v < 10; v++ )
            if ( Map_InfoReadVar( uTruths[v], m ) )
                printf( "1" );
            else
                printf( "0" );
*/
}

/**Function*************************************************************

  Synopsis    [Sets up the mask.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_MappingSetupMask( unsigned uMask[], int nVarsMax )
{
    if ( nVarsMax == 6 )
        uMask[0] = uMask[1] = MAP_FULL;
    else
    {
        uMask[0] = MAP_MASK(1 << nVarsMax);
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
int Map_ManCheckConsistency( Map_Man_t * p )
{
    Map_Node_t * pNode;
    Map_NodeVec_t * pVec;
    int i;
    pVec = Map_MappingDfs( p, 0 );
    for ( i = 0; i < pVec->nSize; i++ )
    {
        pNode = pVec->pArray[i];
        if ( Map_NodeIsVar(pNode) )
        {
            if ( pNode->pRepr )
                printf( "Primary input %d is a secondary node.\n", pNode->Num );
        }
        else if ( Map_NodeIsConst(pNode) )
        {
            if ( pNode->pRepr )
                printf( "Constant 1 %d is a secondary node.\n", pNode->Num );
        }
        else
        {
            if ( pNode->pRepr )
                printf( "Internal node %d is a secondary node.\n", pNode->Num );
            if ( Map_Regular(pNode->p1)->pRepr )
                printf( "Internal node %d has first fanin that is a secondary node.\n", pNode->Num );
            if ( Map_Regular(pNode->p2)->pRepr )
                printf( "Internal node %d has second fanin that is a secondary node.\n", pNode->Num );
        }
    }
    Map_NodeVecFree( pVec );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if current mapping of the node violates fanout limits.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_MappingNodeIsViolator( Map_Node_t * pNode, Map_Cut_t * pCut, int fPosPol )
{
    return pNode->nRefAct[fPosPol] > (int)pCut->M[fPosPol].pSuperBest->nFanLimit;
}

/**Function*************************************************************

  Synopsis    [Computes the total are flow of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Map_MappingGetAreaFlow( Map_Man_t * p )
{
    Map_Node_t * pNode;
    Map_Cut_t * pCut;
    float aFlowFlowTotal = 0;
    int fPosPol, i;
    for ( i = 0; i < p->nOutputs; i++ )
    {
        pNode = Map_Regular(p->pOutputs[i]);
        if ( !Map_NodeIsAnd(pNode) )
            continue;
        fPosPol = !Map_IsComplement(p->pOutputs[i]);
        pCut = pNode->pCutBest[fPosPol];
        if ( pCut == NULL )
        {
            fPosPol = !fPosPol;
            pCut = pNode->pCutBest[fPosPol];
        }
        aFlowFlowTotal += pNode->pCutBest[fPosPol]->M[fPosPol].AreaFlow;
    }
    return aFlowFlowTotal;
}


/**Function*************************************************************

  Synopsis    [Compares the supergates by their level.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_CompareNodesByLevel( Map_Node_t ** ppS1, Map_Node_t ** ppS2 )
{
    Map_Node_t * pN1 = Map_Regular(*ppS1);
    Map_Node_t * pN2 = Map_Regular(*ppS2);
    if ( pN1->Level > pN2->Level )
        return -1;
    if ( pN1->Level < pN2->Level )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Orders the nodes in the decreasing order of levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_MappingSortByLevel( Map_Man_t * pMan, Map_NodeVec_t * vNodes )
{
    qsort( (void *)vNodes->pArray, (size_t)vNodes->nSize, sizeof(Map_Node_t *), 
            (int (*)(const void *, const void *)) Map_CompareNodesByLevel );
//    assert( Map_CompareNodesByLevel( vNodes->pArray, vNodes->pArray + vNodes->nSize - 1 ) <= 0 );
}


/**Function*************************************************************

  Synopsis    [Compares the supergates by their pointer.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_CompareNodesByPointer( Map_Node_t ** ppS1, Map_Node_t ** ppS2 )
{
    if ( *ppS1 < *ppS2 )
        return -1;
    if ( *ppS1 > *ppS2 )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Counts how many AIG nodes are mapped in both polarities.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_MappingCountDoubles( Map_Man_t * pMan, Map_NodeVec_t * vNodes )
{
    Map_Node_t * pNode;
    int Counter, i;
    // count the number of equal adjacent nodes
    Counter = 0;
    for ( i = 0; i < vNodes->nSize; i++ )
    {
        pNode = vNodes->pArray[i];
        if ( !Map_NodeIsAnd(pNode) )
            continue;
        if ( (pNode->nRefAct[0] && pNode->pCutBest[0]) && 
             (pNode->nRefAct[1] && pNode->pCutBest[1]) )
            Counter++;
    }
    return Counter;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
 st__table * Map_CreateTableGate2Super( Map_Man_t * pMan )
{
    Map_Super_t * pSuper;
    st__table * tTable;
    int i, nInputs, v;
    tTable = st__init_table(strcmp, st__strhash);
    for ( i = 0; i < pMan->pSuperLib->nSupersAll; i++ )
    {
        pSuper = pMan->pSuperLib->ppSupers[i];
        if ( pSuper->nGates == 1 )
        {
            // skip different versions of the same root gate
            nInputs = Mio_GateReadPinNum(pSuper->pRoot);
            for ( v = 0; v < nInputs; v++ )
                if ( pSuper->pFanins[v]->Num != nInputs - 1 - v )
                    break;
            if ( v != nInputs )
                continue;
//            printf( "%s\n", Mio_GateReadName(pSuper->pRoot) );
            if ( st__insert( tTable, (char *)pSuper->pRoot, (char *)pSuper ) )
            {
                assert( 0 );
            }
        }
    }
    return tTable;
}

/**Function*************************************************************

  Synopsis    [Get the FRAIG node with phase.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_ManCleanData( Map_Man_t * p )
{
    int i;
    for ( i = 0; i < p->vMapObjs->nSize; i++ )
        p->vMapObjs->pArray[i]->pData0 = p->vMapObjs->pArray[i]->pData1 = 0;
}

/**Function*************************************************************

  Synopsis    [Expand the truth table]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_MappingExpandTruth( unsigned uTruth[2], int nVars )
{
    assert( nVars < 7 );
    if ( nVars == 6 )
        return;
    if ( nVars < 5 )
    {
        uTruth[0] &= MAP_MASK( (1<<nVars) );
        uTruth[0]  = Map_MappingExpandTruth_rec( uTruth[0], nVars );
    }
    uTruth[1] = uTruth[0];
}

/**Function*************************************************************

  Synopsis    [Expand the truth table]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Map_MappingExpandTruth_rec( unsigned uTruth, int nVars )
{
    assert( nVars < 6 );
    if ( nVars == 5 )
        return uTruth;
    return Map_MappingExpandTruth_rec( uTruth | (uTruth << (1 << nVars)), nVars + 1 );    
}

/**Function*************************************************************

  Synopsis    [Compute the arrival times.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Map_MappingComputeDelayWithFanouts( Map_Man_t * p )
{
    Map_Node_t * pNode;
    float Result;
    int i;
    for ( i = 0; i < p->vMapObjs->nSize; i++ )
    {
        // skip primary inputs
        pNode = p->vMapObjs->pArray[i];
        if ( !Map_NodeIsAnd( pNode ) )
            continue;
        // skip a secondary node
        if ( pNode->pRepr )
            continue;
        // count the switching nodes
        if ( pNode->nRefAct[0] > 0 )
            Map_TimeCutComputeArrival( pNode, pNode->pCutBest[0], 0, MAP_FLOAT_LARGE );
        if ( pNode->nRefAct[1] > 0 )
            Map_TimeCutComputeArrival( pNode, pNode->pCutBest[1], 1, MAP_FLOAT_LARGE );
    }
    Result = Map_TimeComputeArrivalMax(p);
    printf( "Max arrival times with fanouts = %10.2f.\n", Result );
    return Result;
}


/**Function*************************************************************

  Synopsis    [Sets up the mask.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_MappingGetMaxLevel( Map_Man_t * pMan )
{
    int nLevelMax, i;
    nLevelMax = 0;
    for ( i = 0; i < pMan->nOutputs; i++ )
        nLevelMax = ((unsigned)nLevelMax) > Map_Regular(pMan->pOutputs[i])->Level? 
                nLevelMax : Map_Regular(pMan->pOutputs[i])->Level;
    return nLevelMax;
}

/**Function*************************************************************

  Synopsis    [Analyses choice nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_MappingUpdateLevel_rec( Map_Man_t * pMan, Map_Node_t * pNode, int fMaximum )
{
    Map_Node_t * pTemp;
    int Level1, Level2, LevelE;
    assert( !Map_IsComplement(pNode) );
    if ( !Map_NodeIsAnd(pNode) )
        return pNode->Level;
    // skip the visited node
    if ( pNode->TravId == pMan->nTravIds )
        return pNode->Level;
    pNode->TravId = pMan->nTravIds;
    // compute levels of the children nodes
    Level1 = Map_MappingUpdateLevel_rec( pMan, Map_Regular(pNode->p1), fMaximum );
    Level2 = Map_MappingUpdateLevel_rec( pMan, Map_Regular(pNode->p2), fMaximum );
    pNode->Level = 1 + MAP_MAX( Level1, Level2 );
    if ( pNode->pNextE )
    {
        LevelE = Map_MappingUpdateLevel_rec( pMan, pNode->pNextE, fMaximum );
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
void Map_MappingSetChoiceLevels( Map_Man_t * pMan )
{
    int i;
    pMan->nTravIds++;
    for ( i = 0; i < pMan->nOutputs; i++ )
        Map_MappingUpdateLevel_rec( pMan, Map_Regular(pMan->pOutputs[i]), 1 );
}

/**Function*************************************************************

  Synopsis    [Reports statistics on choice nodes.]

  Description [The number of choice nodes is the number of primary nodes,
  which has pNextE set to a pointer. The number of choices is the number
  of entries in the equivalent-node lists of the primary nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_MappingReportChoices( Map_Man_t * pMan )
{
    Map_Node_t * pNode, * pTemp;
    int nChoiceNodes, nChoices;
    int i, LevelMax1, LevelMax2;

    // report the number of levels
    LevelMax1 = Map_MappingGetMaxLevel( pMan );
    pMan->nTravIds++;
    for ( i = 0; i < pMan->nOutputs; i++ )
        Map_MappingUpdateLevel_rec( pMan, Map_Regular(pMan->pOutputs[i]), 0 );
    LevelMax2 = Map_MappingGetMaxLevel( pMan );

    // report statistics about choices
    nChoiceNodes = nChoices = 0;
    for ( i = 0; i < pMan->vMapObjs->nSize; i++ )
    {
        pNode = pMan->vMapObjs->pArray[i];
        if ( pNode->pRepr == NULL && pNode->pNextE != NULL )
        { // this is a choice node = the primary node that has equivalent nodes
            nChoiceNodes++;
            for ( pTemp = pNode; pTemp; pTemp = pTemp->pNextE )
                nChoices++;
        }
    }
    printf( "Maximum level: Original = %d. Reduced due to choices = %d.\n", LevelMax1, LevelMax2 );
    printf( "Choice stats:  Choice nodes = %d. Total choices = %d.\n", nChoiceNodes, nChoices );
}

/**Function*************************************************************

  Synopsis    [Computes the maximum and minimum levels of the choice nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_MappingCountUsedNodes( Map_Man_t * pMan, int fChoices )
{
    Map_NodeVec_t * vNodes;
    int Result;
    vNodes = Map_MappingDfs( pMan, fChoices );
    Result = vNodes->nSize;
    Map_NodeVecFree( vNodes );
    return Result;    
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

