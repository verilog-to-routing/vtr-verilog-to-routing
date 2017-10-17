/**CFile****************************************************************

  FileName    [mapperTime.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic technology mapping engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - June 1, 2004.]

  Revision    [$Id: mapperTime.c,v 1.3 2005/03/02 02:35:54 alanmi Exp $]

***********************************************************************/

#include "mapperInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void  Map_TimePropagateRequired( Map_Man_t * p, Map_NodeVec_t * vNodes );
static void  Map_TimePropagateRequiredPhase( Map_Man_t * p, Map_Node_t * pNode, int fPhase );
static float Map_MatchComputeReqTimes( Map_Cut_t * pCut, int fPhase, Map_Time_t * ptArrRes );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**function*************************************************************

  synopsis    [Computes the exact area associated with the cut.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
float Map_TimeMatchWithInverter( Map_Man_t * p, Map_Match_t * pMatch )
{
    Map_Time_t tArrInv;
    tArrInv.Fall  = pMatch->tArrive.Rise + p->pSuperLib->tDelayInv.Fall;
    tArrInv.Rise  = pMatch->tArrive.Fall + p->pSuperLib->tDelayInv.Rise;
    tArrInv.Worst = MAP_MAX( tArrInv.Rise, tArrInv.Fall ); 
    return tArrInv.Worst;
}

/**Function*************************************************************

  Synopsis    [Computes the arrival times of the cut recursively.]

  Description [When computing the arrival time for the previously unused 
  cuts, their arrival time may be incorrect because their fanins have 
  incorrect arrival time. This procedure is called to fix this problem.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_TimeCutComputeArrival_rec( Map_Cut_t * pCut, int fPhase )
{
    int i, fPhaseLeaf;
    for ( i = 0; i < pCut->nLeaves; i++ )
    {
        fPhaseLeaf = Map_CutGetLeafPhase( pCut, fPhase, i );
        if ( pCut->ppLeaves[i]->nRefAct[fPhaseLeaf] > 0 )
            continue;
        Map_TimeCutComputeArrival_rec( pCut->ppLeaves[i]->pCutBest[fPhaseLeaf], fPhaseLeaf );
    }
    Map_TimeCutComputeArrival( NULL, pCut, fPhase, MAP_FLOAT_LARGE );
}

/**Function*************************************************************

  Synopsis    [Computes the arrival times of the cut.]

  Description [Computes the arrival times of the cut if it is implemented using 
  the given supergate with the given phase. Uses the constraint-type specification
  of rise/fall arrival times.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Map_TimeCutComputeArrival( Map_Node_t * pNode, Map_Cut_t * pCut, int fPhase, float tWorstLimit )
{
    Map_Match_t * pM = pCut->M + fPhase;
    Map_Super_t * pSuper = pM->pSuperBest;
    unsigned uPhaseTot = pM->uPhaseBest;
    Map_Time_t * ptArrRes = &pM->tArrive;
    Map_Time_t * ptArrIn;
    bool fPinPhase;
    float tDelay;
    int i;

    ptArrRes->Rise  = ptArrRes->Fall = 0.0;
    ptArrRes->Worst = MAP_FLOAT_LARGE;
    for ( i = pCut->nLeaves - 1; i >= 0; i-- )
    {
        // get the phase of the given pin
        fPinPhase = ((uPhaseTot & (1 << i)) == 0);
        ptArrIn = pCut->ppLeaves[i]->tArrival + fPinPhase;

        // get the rise of the output due to rise of the inputs
        if ( pSuper->tDelaysR[i].Rise > 0 )
        {
            tDelay = ptArrIn->Rise + pSuper->tDelaysR[i].Rise;
            if ( tDelay > tWorstLimit )
                return MAP_FLOAT_LARGE;
            if ( ptArrRes->Rise < tDelay )
                ptArrRes->Rise = tDelay;
        }

        // get the rise of the output due to fall of the inputs
        if ( pSuper->tDelaysR[i].Fall > 0 )
        {
            tDelay = ptArrIn->Fall + pSuper->tDelaysR[i].Fall;
            if ( tDelay > tWorstLimit )
                return MAP_FLOAT_LARGE;
            if ( ptArrRes->Rise < tDelay )
                ptArrRes->Rise = tDelay;
        }

        // get the fall of the output due to rise of the inputs
        if ( pSuper->tDelaysF[i].Rise > 0 )
        {
            tDelay = ptArrIn->Rise + pSuper->tDelaysF[i].Rise;
            if ( tDelay > tWorstLimit )
                return MAP_FLOAT_LARGE;
            if ( ptArrRes->Fall < tDelay )
                ptArrRes->Fall = tDelay;
        }

        // get the fall of the output due to fall of the inputs
        if ( pSuper->tDelaysF[i].Fall > 0 )
        {
            tDelay = ptArrIn->Fall + pSuper->tDelaysF[i].Fall;
            if ( tDelay > tWorstLimit )
                return MAP_FLOAT_LARGE;
            if ( ptArrRes->Fall < tDelay )
                ptArrRes->Fall = tDelay;
        }
    }
    // return the worst-case of rise/fall arrival times
    ptArrRes->Worst = MAP_MAX(ptArrRes->Rise, ptArrRes->Fall);
    return ptArrRes->Worst;
}


/**Function*************************************************************

  Synopsis    [Computes the maximum arrival times.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Map_TimeComputeArrivalMax( Map_Man_t * p )
{
    float tReqMax, tReq;
    int i, fPhase;
    // get the critical PO arrival time
    tReqMax = -MAP_FLOAT_LARGE;
    for ( i = 0; i < p->nOutputs; i++ )
    {
        if ( Map_NodeIsConst(p->pOutputs[i]) )
            continue;
        fPhase  = !Map_IsComplement(p->pOutputs[i]);
        tReq    = Map_Regular(p->pOutputs[i])->tArrival[fPhase].Worst;
        tReqMax = MAP_MAX( tReqMax, tReq );
    }
    return tReqMax;
}

/**Function*************************************************************

  Synopsis    [Computes the required times of all nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_TimeComputeRequiredGlobal( Map_Man_t * p )
{
    p->fRequiredGlo = Map_TimeComputeArrivalMax( p );
    // update the required times according to the target
    if ( p->DelayTarget != -1 )
    {
        if ( p->fRequiredGlo > p->DelayTarget + p->fEpsilon )
        {
            if ( p->fMappingMode == 1 )
                printf( "Cannot meet the target required times (%4.2f). Continue anyway.\n", p->DelayTarget );
        }
        else if ( p->fRequiredGlo < p->DelayTarget - p->fEpsilon )
        {
            if ( p->fMappingMode == 1 )
                printf( "Relaxing the required times from (%4.2f) to the target (%4.2f).\n", p->fRequiredGlo, p->DelayTarget );
            p->fRequiredGlo = p->DelayTarget;
        }
    }
    Map_TimeComputeRequired( p, p->fRequiredGlo );
}

/**Function*************************************************************

  Synopsis    [Computes the required times of all nodes.]

  Description [This procedure assumes that the nodes used in the mapping
  are collected in p->vMapping.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_TimeComputeRequired( Map_Man_t * p, float fRequired )
{
    Map_Time_t * ptTime;
    int fPhase, i;

    // clean the required times
    for ( i = 0; i < p->vAnds->nSize; i++ )
    {
        p->vAnds->pArray[i]->tRequired[0].Rise = MAP_FLOAT_LARGE;
        p->vAnds->pArray[i]->tRequired[0].Fall = MAP_FLOAT_LARGE;
        p->vAnds->pArray[i]->tRequired[0].Worst = MAP_FLOAT_LARGE;
        p->vAnds->pArray[i]->tRequired[1].Rise = MAP_FLOAT_LARGE;
        p->vAnds->pArray[i]->tRequired[1].Fall = MAP_FLOAT_LARGE;
        p->vAnds->pArray[i]->tRequired[1].Worst = MAP_FLOAT_LARGE;
    }

    // set the required times for the POs
    for ( i = 0; i < p->nOutputs; i++ )
    {
        fPhase  = !Map_IsComplement(p->pOutputs[i]);
        ptTime  =  Map_Regular(p->pOutputs[i])->tRequired + fPhase;
        ptTime->Rise = ptTime->Fall = ptTime->Worst = fRequired;
    }

    // sorts the nodes in the decreasing order of levels
    // this puts the nodes in reverse topological order
//    Map_MappingSortByLevel( p, p->vMapping );
    // the array is already sorted by construction in Map_MappingSetRefs()

    Map_TimePropagateRequired( p, p->vMapping );
}

/**Function*************************************************************

  Synopsis    [Computes the required times of the given nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_TimePropagateRequired( Map_Man_t * p, Map_NodeVec_t * vNodes )
{
    Map_Node_t * pNode;
    Map_Time_t tReqOutTest, * ptReqOutTest = &tReqOutTest;
    Map_Time_t * ptReqIn, * ptReqOut;
    int fPhase, k;

    // go through the nodes in the reverse topological order
    for ( k = 0; k < vNodes->nSize; k++ )
    {
        pNode = vNodes->pArray[k];

        // this computation works for regular nodes only
        assert( !Map_IsComplement(pNode) );
        // at least one phase should be mapped
        assert( pNode->pCutBest[0] != NULL || pNode->pCutBest[1] != NULL );
        // the node should be used in the currently assigned mapping
        assert( pNode->nRefAct[0] > 0 || pNode->nRefAct[1] > 0 );

        // if one of the cuts is not given, project the required times from the other cut
        if ( pNode->pCutBest[0] == NULL || pNode->pCutBest[1] == NULL )
        {
//            assert( 0 );
            // get the missing phase 
            fPhase = (pNode->pCutBest[1] == NULL); 
            // check if the missing phase is needed in the mapping
            if ( pNode->nRefAct[fPhase] > 0 )
            {
                // get the pointers to the required times of the missing phase
                ptReqOut = pNode->tRequired +  fPhase;
//                assert( ptReqOut->Fall < MAP_FLOAT_LARGE );
                // get the pointers to the required times of the present phase
                ptReqIn  = pNode->tRequired + !fPhase;
                // propagate the required times from the missing phase to the present phase
    //            tArrInv.Fall  = pMatch->tArrive.Rise + p->pSuperLib->tDelayInv.Fall;
    //            tArrInv.Rise  = pMatch->tArrive.Fall + p->pSuperLib->tDelayInv.Rise;
                ptReqIn->Fall = MAP_MIN( ptReqIn->Fall, ptReqOut->Rise - p->pSuperLib->tDelayInv.Rise );
                ptReqIn->Rise = MAP_MIN( ptReqIn->Rise, ptReqOut->Fall - p->pSuperLib->tDelayInv.Fall );
            }
        }

        // finalize the worst case computation
        pNode->tRequired[0].Worst = MAP_MIN( pNode->tRequired[0].Fall, pNode->tRequired[0].Rise );
        pNode->tRequired[1].Worst = MAP_MIN( pNode->tRequired[1].Fall, pNode->tRequired[1].Rise );

        // skip the PIs
        if ( !Map_NodeIsAnd(pNode) )
            continue;

        // propagate required times of different phases of the node
        // the ordering of phases does not matter since they are mapped independently
        if ( pNode->pCutBest[0] && pNode->tRequired[0].Worst < MAP_FLOAT_LARGE )
            Map_TimePropagateRequiredPhase( p, pNode, 0 );
        if ( pNode->pCutBest[1] && pNode->tRequired[1].Worst < MAP_FLOAT_LARGE )
            Map_TimePropagateRequiredPhase( p, pNode, 1 );
    }

    // in the end, we verify the required times
    // for this, we compute the arrival times of the outputs of each phase 
    // of the supergates using the fanins' required times as the fanins' arrival times
    // the resulting arrival time of the supergate should be less than the actual required time
    for ( k = 0; k < vNodes->nSize; k++ )
    {
        pNode = vNodes->pArray[k];
        if ( !Map_NodeIsAnd(pNode) )
            continue;
        // verify that the required times are propagated correctly
//        if ( pNode->pCutBest[0] && (pNode->nRefAct[0] > 0 || pNode->pCutBest[1] == NULL) )
        if ( pNode->pCutBest[0] && pNode->tRequired[0].Worst < MAP_FLOAT_LARGE )
        {
            Map_MatchComputeReqTimes( pNode->pCutBest[0], 0, ptReqOutTest );
            assert( ptReqOutTest->Rise < pNode->tRequired[0].Rise + p->fEpsilon );
            assert( ptReqOutTest->Fall < pNode->tRequired[0].Fall + p->fEpsilon );
        }
//        if ( pNode->pCutBest[1] && (pNode->nRefAct[1] > 0 || pNode->pCutBest[0] == NULL) )
        if ( pNode->pCutBest[1] && pNode->tRequired[1].Worst < MAP_FLOAT_LARGE )
        {
            Map_MatchComputeReqTimes( pNode->pCutBest[1], 1, ptReqOutTest );
            assert( ptReqOutTest->Rise < pNode->tRequired[1].Rise + p->fEpsilon );
            assert( ptReqOutTest->Fall < pNode->tRequired[1].Fall + p->fEpsilon );
        }
    }

}

/**Function*************************************************************

  Synopsis    [Computes the required times of the given nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_TimePropagateRequiredPhase( Map_Man_t * p, Map_Node_t * pNode, int fPhase )
{
    Map_Time_t * ptReqIn, * ptReqOut;
    Map_Cut_t * pCut;
    Map_Super_t * pSuper;
    float tNewReqTime;
    unsigned uPhase;
    int fPinPhase, i;

    // get the cut to be propagated
    pCut = pNode->pCutBest[fPhase];
    assert( pCut != NULL );
    // get the supergate and its polarity
    pSuper  = pCut->M[fPhase].pSuperBest;
    uPhase  = pCut->M[fPhase].uPhaseBest;
    // get the required time of the output of the supergate
    ptReqOut = pNode->tRequired + fPhase;
    // set the required time of the children
    for ( i = 0; i < pCut->nLeaves; i++ )
    {
        // get the phase of the given pin of the supergate
        fPinPhase = ((uPhase & (1 << i)) == 0);
        ptReqIn = pCut->ppLeaves[i]->tRequired + fPinPhase;
        assert( pCut->ppLeaves[i]->nRefAct[2] > 0 );

        // get the rise of the output due to rise of the inputs
//            if ( ptArrOut->Rise < ptArrIn->Rise + pSuper->tDelaysR[i].Rise )
//                ptArrOut->Rise = ptArrIn->Rise + pSuper->tDelaysR[i].Rise;
        if ( pSuper->tDelaysR[i].Rise > 0 )
        {
            tNewReqTime = ptReqOut->Rise - pSuper->tDelaysR[i].Rise;
            ptReqIn->Rise = MAP_MIN( ptReqIn->Rise, tNewReqTime );
        }

        // get the rise of the output due to fall of the inputs
//            if ( ptArrOut->Rise < ptArrIn->Fall + pSuper->tDelaysR[i].Fall )
//                ptArrOut->Rise = ptArrIn->Fall + pSuper->tDelaysR[i].Fall;
        if ( pSuper->tDelaysR[i].Fall > 0 )
        {
            tNewReqTime = ptReqOut->Rise - pSuper->tDelaysR[i].Fall;
            ptReqIn->Fall = MAP_MIN( ptReqIn->Fall, tNewReqTime );
        }

        // get the fall of the output due to rise of the inputs
//            if ( ptArrOut->Fall < ptArrIn->Rise + pSuper->tDelaysF[i].Rise )
//                ptArrOut->Fall = ptArrIn->Rise + pSuper->tDelaysF[i].Rise;
        if ( pSuper->tDelaysF[i].Rise > 0 )
        {
            tNewReqTime = ptReqOut->Fall - pSuper->tDelaysF[i].Rise;
            ptReqIn->Rise = MAP_MIN( ptReqIn->Rise, tNewReqTime );
        }

        // get the fall of the output due to fall of the inputs
//            if ( ptArrOut->Fall < ptArrIn->Fall + pSuper->tDelaysF[i].Fall )
//                ptArrOut->Fall = ptArrIn->Fall + pSuper->tDelaysF[i].Fall;
        if ( pSuper->tDelaysF[i].Fall > 0 )
        {
            tNewReqTime = ptReqOut->Fall - pSuper->tDelaysF[i].Fall;
            ptReqIn->Fall = MAP_MIN( ptReqIn->Fall, tNewReqTime );
        }
    }

    // compare the required times with the arrival times
    assert( pNode->tArrival[fPhase].Rise < ptReqOut->Rise + p->fEpsilon );
    assert( pNode->tArrival[fPhase].Fall < ptReqOut->Fall + p->fEpsilon );
}

/**Function*************************************************************

  Synopsis    [Computes the arrival times of the cut.]

  Description [Computes the arrival times of the cut if it is implemented using 
  the given supergate with the given phase. Uses the constraint-type specification
  of rise/fall arrival times.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Map_MatchComputeReqTimes( Map_Cut_t * pCut, int fPhase, Map_Time_t * ptArrRes )
{
    Map_Time_t * ptArrIn;
    Map_Super_t * pSuper;
    unsigned uPhaseTot;
    int fPinPhase, i;
    float tDelay;

    // get the supergate and the phase
    pSuper = pCut->M[fPhase].pSuperBest;
    uPhaseTot = pCut->M[fPhase].uPhaseBest;

    // propagate the arrival times 
    ptArrRes->Rise = ptArrRes->Fall = -MAP_FLOAT_LARGE;
    for ( i = 0; i < pCut->nLeaves; i++ )
    {
        // get the phase of the given pin
        fPinPhase = ((uPhaseTot & (1 << i)) == 0);
        ptArrIn = pCut->ppLeaves[i]->tRequired + fPinPhase;
//        assert( ptArrIn->Worst < MAP_FLOAT_LARGE );

        // get the rise of the output due to rise of the inputs
        if ( pSuper->tDelaysR[i].Rise > 0 )
        {
            tDelay = ptArrIn->Rise + pSuper->tDelaysR[i].Rise;
            if ( ptArrRes->Rise < tDelay )
                ptArrRes->Rise = tDelay;
        }

        // get the rise of the output due to fall of the inputs
        if ( pSuper->tDelaysR[i].Fall > 0 )
        {
            tDelay = ptArrIn->Fall + pSuper->tDelaysR[i].Fall;
            if ( ptArrRes->Rise < tDelay )
                ptArrRes->Rise = tDelay;
        }

        // get the fall of the output due to rise of the inputs
        if ( pSuper->tDelaysF[i].Rise > 0 )
        {
            tDelay = ptArrIn->Rise + pSuper->tDelaysF[i].Rise;
            if ( ptArrRes->Fall < tDelay )
                ptArrRes->Fall = tDelay;
        }

        // get the fall of the output due to fall of the inputs
        if ( pSuper->tDelaysF[i].Fall > 0 )
        {
            tDelay = ptArrIn->Fall + pSuper->tDelaysF[i].Fall;
            if ( ptArrRes->Fall < tDelay )
                ptArrRes->Fall = tDelay;
        }
    }
    // return the worst-case of rise/fall arrival times
    return MAP_MAX(ptArrRes->Rise, ptArrRes->Fall);
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


