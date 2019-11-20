/**CFile****************************************************************

  FileName    [mapperRefs.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic technology mapping engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - June 1, 2004.]

  Revision    [$Id: mapperRefs.h,v 1.0 2003/09/08 00:00:00 alanmi Exp $]

***********************************************************************/

#include "mapperInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int   Map_NodeIncRefPhaseAct( Map_Node_t * pNode, int fPhase );
static int   Map_NodeDecRefPhaseAct( Map_Node_t * pNode, int fPhase );
static float Map_CutRefDeref( Map_Cut_t * pCut, int fPhase, int fReference );
static void  Map_MappingSetRefs_rec( Map_Man_t * pMan, Map_Node_t * pNode, Map_Node_t ** ppStore );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reads the actual reference counter of a phase.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_NodeReadRefPhaseAct( Map_Node_t * pNode, int fPhase )
{
    assert( !Map_IsComplement(pNode) );
    if ( pNode->pCutBest[0] && pNode->pCutBest[1] ) // both assigned
        return pNode->nRefAct[fPhase];
    assert( pNode->pCutBest[0] || pNode->pCutBest[1] ); // at least one assigned
    return pNode->nRefAct[2];
}

/**Function*************************************************************

  Synopsis    [Reads the estimated reference counter of a phase.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Map_NodeReadRefPhaseEst( Map_Node_t * pNode, int fPhase )
{
    assert( !Map_IsComplement(pNode) );
    if ( pNode->pCutBest[0] && pNode->pCutBest[1] ) // both assigned
        return pNode->nRefEst[fPhase];
    assert( pNode->pCutBest[0] || pNode->pCutBest[1] ); // at least one assigned
//    return pNode->nRefEst[0] + pNode->nRefEst[1];
    return pNode->nRefEst[2];
}


/**Function*************************************************************

  Synopsis    [Increments the actual reference counter of a phase.]

  Description [Returns the old reference counter.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_NodeIncRefPhaseAct( Map_Node_t * pNode, int fPhase )
{
    assert( !Map_IsComplement(pNode) );
    if ( pNode->pCutBest[0] && pNode->pCutBest[1] ) // both assigned
        return pNode->nRefAct[fPhase]++;
    assert( pNode->pCutBest[0] || pNode->pCutBest[1] ); // at least one assigned
    return pNode->nRefAct[2]++;
}

/**Function*************************************************************

  Synopsis    [Decrements the actual reference counter of a phase.]

  Description [Returns the new reference counter.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_NodeDecRefPhaseAct( Map_Node_t * pNode, int fPhase )
{
    assert( !Map_IsComplement(pNode) );
    if ( pNode->pCutBest[0] && pNode->pCutBest[1] ) // both assigned
        return --pNode->nRefAct[fPhase];
    assert( pNode->pCutBest[0] || pNode->pCutBest[1] ); // at least one assigned
    return --pNode->nRefAct[2];
}


/**Function*************************************************************

  Synopsis    [Sets the estimated reference counter for the PIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_MappingEstimateRefsInit( Map_Man_t * p )
{
    Map_Node_t * pNode;
    int i;
    for ( i = 0; i < p->vAnds->nSize; i++ )
    {
        pNode = p->vAnds->pArray[i];
//        pNode->nRefEst[0] = pNode->nRefEst[1] = ((float)pNode->nRefs)*(float)2.0;
        pNode->nRefEst[0] = pNode->nRefEst[1] = pNode->nRefEst[2] = ((float)pNode->nRefs);
    }
}

/**Function*************************************************************

  Synopsis    [Sets the estimated reference counter.]

  Description [When this procedure is called for the first time,
  the reference counter is estimated from the AIG. Otherwise, it is
  a linear combination of reference counters in the last two iterations.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_MappingEstimateRefs( Map_Man_t * p )
{
    Map_Node_t * pNode;
    int i;
    for ( i = 0; i < p->vAnds->nSize; i++ )
    {
        pNode = p->vAnds->pArray[i];
//        pNode->nRefEst[0] = (float)((2.0 * pNode->nRefEst[0] + 1.0 * pNode->nRefAct[0]) / 3.0);
//        pNode->nRefEst[1] = (float)((2.0 * pNode->nRefEst[1] + 1.0 * pNode->nRefAct[1]) / 3.0);
//        pNode->nRefEst[2] = (float)((2.0 * pNode->nRefEst[2] + 1.0 * pNode->nRefAct[2]) / 3.0);
        pNode->nRefEst[0] = (float)((3.0 * pNode->nRefEst[0] + 1.0 * pNode->nRefAct[0]) / 4.0);
        pNode->nRefEst[1] = (float)((3.0 * pNode->nRefEst[1] + 1.0 * pNode->nRefAct[1]) / 4.0);
        pNode->nRefEst[2] = (float)((3.0 * pNode->nRefEst[2] + 1.0 * pNode->nRefAct[2]) / 4.0);
    }
}





/**function*************************************************************

  synopsis    [Computes the area flow of the cut.]

  description [Computes the area flow of the cut if it is implemented using 
  the best supergate with the best phase.]
               
  sideeffects []

  seealso     []

***********************************************************************/
float Map_CutGetAreaFlow( Map_Cut_t * pCut, int fPhase )
{
    Map_Match_t * pM = pCut->M + fPhase;
    Map_Super_t * pSuper = pM->pSuperBest;
    unsigned uPhaseTot = pM->uPhaseBest;
    Map_Cut_t * pCutFanin;
    float aFlowRes, aFlowFanin, nRefs;
    int i, fPinPhasePos;

    // start the resulting area flow
    aFlowRes = pSuper->Area;
    // iterate through the leaves
    for ( i = 0; i < pCut->nLeaves; i++ )
    {
        // get the phase of this fanin
        fPinPhasePos = ((uPhaseTot & (1 << i)) == 0);
        // get the cut implementing this phase of the fanin
        pCutFanin = pCut->ppLeaves[i]->pCutBest[fPinPhasePos];
        // if the cut is not available, we have to use the opposite phase
        if ( pCutFanin == NULL )
        {
            fPinPhasePos = !fPinPhasePos;
            pCutFanin = pCut->ppLeaves[i]->pCutBest[fPinPhasePos];
        }
        aFlowFanin = pCutFanin->M[fPinPhasePos].AreaFlow; // ignores the area of the interter
        // get the fanout count of the cut in the given phase
        nRefs = Map_NodeReadRefPhaseEst( pCut->ppLeaves[i], fPinPhasePos );
        // if the node does no fanout, assume fanout count equal to 1
        if ( nRefs == (float)0.0 )
            nRefs = (float)1.0;
        // add the area flow due to the fanin
        aFlowRes += aFlowFanin / nRefs;
    }
    pM->AreaFlow = aFlowRes;
    return aFlowRes;
}



/**function*************************************************************

  synopsis    [Computes the exact area associated with the cut.]

  description [Assumes that the cut is referenced.]
               
  sideeffects []

  seealso     []

***********************************************************************/
float Map_CutGetAreaRefed( Map_Cut_t * pCut, int fPhase )
{
    float aResult, aResult2;
    aResult2 = Map_CutRefDeref( pCut, fPhase, 0 ); // dereference
    aResult  = Map_CutRefDeref( pCut, fPhase, 1 ); // reference
//    assert( aResult == aResult2 );
    return aResult;
}

/**function*************************************************************

  synopsis    [Computes the exact area associated with the cut.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
float Map_CutGetAreaDerefed( Map_Cut_t * pCut, int fPhase )
{
    float aResult, aResult2;
    aResult2 = Map_CutRefDeref( pCut, fPhase, 1 ); // reference
    aResult  = Map_CutRefDeref( pCut, fPhase, 0 ); // dereference
//    assert( aResult == aResult2 );
    return aResult;
}

/**function*************************************************************

  synopsis    [References the cut.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
float Map_CutRef( Map_Cut_t * pCut, int fPhase )
{
    return Map_CutRefDeref( pCut, fPhase, 1 ); // reference
}

/**function*************************************************************

  synopsis    [Dereferences the cut.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
float Map_CutDeref( Map_Cut_t * pCut, int fPhase )
{
    return Map_CutRefDeref( pCut, fPhase, 0 ); // dereference
}

/**function*************************************************************

  synopsis    [References or dereferences the cut.]

  description [This reference part is similar to Cudd_NodeReclaim(). 
  The dereference part is similar to Cudd_RecursiveDeref().]
               
  sideeffects []

  seealso     []

***********************************************************************/
float Map_CutRefDeref( Map_Cut_t * pCut, int fPhase, int fReference )
{
    Map_Node_t * pNodeChild;
    Map_Cut_t * pCutChild;
    float aArea;
    int i, fPhaseChild;
//    int nRefs;

    // consider the elementary variable
    if ( pCut->nLeaves == 1 )
        return 0;
    // start the area of this cut
    aArea = Map_CutGetRootArea( pCut, fPhase );
    // go through the children
    for ( i = 0; i < pCut->nLeaves; i++ )
    {
        pNodeChild  = pCut->ppLeaves[i];
        fPhaseChild = Map_CutGetLeafPhase( pCut, fPhase, i );
        // get the reference counter of the child
/*
        // this code does not take inverters into account
        // the quality of area recovery seems to always be a little worse
        if ( fReference )
            nRefs = Map_NodeIncRefPhaseAct( pNodeChild, fPhaseChild );
        else
            nRefs = Map_NodeDecRefPhaseAct( pNodeChild, fPhaseChild );
        assert( nRefs >= 0 );
        // skip if the child was already reference before
        if ( nRefs > 0 )
            continue;
*/

        if ( fReference )
        {
            if ( pNodeChild->pCutBest[0] && pNodeChild->pCutBest[1] ) // both phases are present
            {
                // if this phase of the node is referenced, there is no recursive call
                pNodeChild->nRefAct[2]++;
                if ( pNodeChild->nRefAct[fPhaseChild]++ > 0 )
                    continue;
            }
            else // only one phase is present
            {
                // inverter should be added if the phase
                // (a) has no reference and (b) is implemented using other phase
                if ( pNodeChild->nRefAct[fPhaseChild]++ == 0 && pNodeChild->pCutBest[fPhaseChild] == NULL )
                    aArea += pNodeChild->p->pSuperLib->AreaInv;
                // if the node is referenced, there is no recursive call
                if ( pNodeChild->nRefAct[2]++ > 0 )
                    continue;
            }
        }
        else
        {
            if ( pNodeChild->pCutBest[0] && pNodeChild->pCutBest[1] ) // both phases are present
            {
                // if this phase of the node is referenced, there is no recursive call
                --pNodeChild->nRefAct[2];
                if ( --pNodeChild->nRefAct[fPhaseChild] > 0 )
                    continue;
            }
            else // only one phase is present
            {
                // inverter should be added if the phase
                // (a) has no reference and (b) is implemented using other phase
                if ( --pNodeChild->nRefAct[fPhaseChild] == 0 && pNodeChild->pCutBest[fPhaseChild] == NULL )
                    aArea += pNodeChild->p->pSuperLib->AreaInv;
                // if the node is referenced, there is no recursive call
                if ( --pNodeChild->nRefAct[2] > 0 )
                    continue;
            }
            assert( pNodeChild->nRefAct[fPhaseChild] >= 0 );
        }

        // get the child cut
        pCutChild = pNodeChild->pCutBest[fPhaseChild];
        // if the child does not have this phase mapped, take the opposite phase
        if ( pCutChild == NULL )
        {
            fPhaseChild = !fPhaseChild;
            pCutChild   = pNodeChild->pCutBest[fPhaseChild];
        }
        // reference and compute area recursively
        aArea += Map_CutRefDeref( pCutChild, fPhaseChild, fReference );
    }
    return aArea;
}




/**Function*************************************************************

  Synopsis    [Computes actual reference counters.]

  Description [Collects the nodes used in the mapping in array pMan->vMapping.
  Nodes are collected in reverse topological order to facilitate the 
  computation of required times.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_MappingSetRefs( Map_Man_t * pMan )
{
    Map_Node_t * pNode, ** ppStore;
    int i, fPhase, LevelMax;

    // clean all references
    for ( i = 0; i < pMan->vNodesAll->nSize; i++ )
    {
        pNode = pMan->vNodesAll->pArray[i];
        pNode->nRefAct[0] = 0;
        pNode->nRefAct[1] = 0;
        pNode->nRefAct[2] = 0;
    }

    // find the largest level of a node
    LevelMax = 0;
    for ( i = 0; i < pMan->nOutputs; i++ )
        if ( LevelMax < (int)Map_Regular(pMan->pOutputs[i])->Level )
            LevelMax = Map_Regular(pMan->pOutputs[i])->Level;

    // allocate place to store the nodes
    ppStore = ALLOC( Map_Node_t *, LevelMax + 1 );
    memset( ppStore, 0, sizeof(Map_Node_t *) * (LevelMax + 1) );

    // visit nodes reachable from POs in the DFS order through the best cuts
    for ( i = 0; i < pMan->nOutputs; i++ )
    {
        pNode  = pMan->pOutputs[i];
        fPhase = !Map_IsComplement(pNode);
        if ( !Map_NodeIsConst(pNode) )
            Map_MappingSetRefs_rec( pMan, pNode, ppStore );
    }

    // reconnect the nodes in reverse topological order
    pMan->vMapping->nSize = 0;
    for ( i = LevelMax; i >= 0; i-- )
        for ( pNode = ppStore[i]; pNode; pNode = (Map_Node_t *)pNode->pData0 )
            Map_NodeVecPush( pMan->vMapping, pNode );
    free( ppStore );
}

/**Function*************************************************************

  Synopsis    [Recursively computes the DFS ordering of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_MappingSetRefs_rec( Map_Man_t * pMan, Map_Node_t * pNode, Map_Node_t ** ppStore )
{
    Map_Cut_t * pCut;
    Map_Node_t * pNodeR;
    unsigned uPhase;
    int i, fPhase, fInvPin;

    // get the regular node and its phase
    pNodeR = Map_Regular(pNode);
    fPhase = !Map_IsComplement(pNode);

    // add the node to the list of all visited nodes
    if ( pNodeR->nRefAct[2]++ == 0 )
//        Map_NodeVecPush( pMan->vMapping, pNodeR );
        pNodeR->pData0 = (char *)ppStore[pNodeR->Level], ppStore[pNodeR->Level] = pNodeR;

    // quit if the node was already visited in this phase
    if ( pNodeR->nRefAct[fPhase]++ )
        return;

    // quit if this is a PI node
    if ( Map_NodeIsVar(pNodeR) )
        return;

    // get the cut implementing this or opposite polarity
    pCut = pNodeR->pCutBest[fPhase];
    if ( pCut == NULL )
    {
        fPhase = !fPhase;
        pCut   = pNodeR->pCutBest[fPhase];
    }

    // visit the transitive fanin
    uPhase = pCut->M[fPhase].uPhaseBest;
    for ( i = 0; i < pCut->nLeaves; i++ )
    {
        fInvPin = ((uPhase & (1 << i)) > 0);
        Map_MappingSetRefs_rec( pMan, Map_NotCond(pCut->ppLeaves[i], fInvPin), ppStore );
    }
}


/**Function*************************************************************

  Synopsis    [Computes the array of mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Map_MappingGetArea( Map_Man_t * pMan, Map_NodeVec_t * vMapping )
{
    Map_Node_t * pNode;
    float Area;
    int i;
    Area = 0.0;
    for ( i = 0; i < vMapping->nSize; i++ )
    {
        pNode = vMapping->pArray[i];
        // at least one phase has the best cut assigned
        assert( pNode->pCutBest[0] != NULL || pNode->pCutBest[1] != NULL );
        // at least one phase is used in the mapping
        assert( pNode->nRefAct[0] > 0 || pNode->nRefAct[1] > 0 );
        // compute the array due to the supergate
        if ( Map_NodeIsAnd(pNode) )
        {
            // count area of the negative phase
            if ( pNode->pCutBest[0] && (pNode->nRefAct[0] > 0 || pNode->pCutBest[1] == NULL) )
                Area += pNode->pCutBest[0]->M[0].pSuperBest->Area;
            // count area of the positive phase
            if ( pNode->pCutBest[1] && (pNode->nRefAct[1] > 0 || pNode->pCutBest[0] == NULL) )
                Area += pNode->pCutBest[1]->M[1].pSuperBest->Area;
        }
        // count area of the interver if we need to implement one phase with another phase
        if ( (pNode->pCutBest[0] == NULL && pNode->nRefAct[0] > 0) || 
             (pNode->pCutBest[1] == NULL && pNode->nRefAct[1] > 0) )
            Area += pMan->pSuperLib->AreaInv;
    }
    // add buffers for each CO driven by a CI
    for ( i = 0; i < pMan->nOutputs; i++ )
        if ( Map_NodeIsVar(pMan->pOutputs[i]) && !Map_IsComplement(pMan->pOutputs[i]) )
            Area += pMan->pSuperLib->AreaBuf;
    return Area;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


