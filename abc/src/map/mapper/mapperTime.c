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

#include "misc/util/utilNam.h"
#include "map/scl/sclCon.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

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
    int fPinPhase;
    float tDelay, tExtra;
    int i;

    tExtra = pNode->p->pNodeDelays ? pNode->p->pNodeDelays[pNode->Num] : 0;
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
            tDelay = ptArrIn->Rise + pSuper->tDelaysR[i].Rise + tExtra;
            if ( tDelay > tWorstLimit )
                return MAP_FLOAT_LARGE;
            if ( ptArrRes->Rise < tDelay )
                ptArrRes->Rise = tDelay;
        }

        // get the rise of the output due to fall of the inputs
        if ( pSuper->tDelaysR[i].Fall > 0 )
        {
            tDelay = ptArrIn->Fall + pSuper->tDelaysR[i].Fall + tExtra;
            if ( tDelay > tWorstLimit )
                return MAP_FLOAT_LARGE;
            if ( ptArrRes->Rise < tDelay )
                ptArrRes->Rise = tDelay;
        }

        // get the fall of the output due to rise of the inputs
        if ( pSuper->tDelaysF[i].Rise > 0 )
        {
            tDelay = ptArrIn->Rise + pSuper->tDelaysF[i].Rise + tExtra;
            if ( tDelay > tWorstLimit )
                return MAP_FLOAT_LARGE;
            if ( ptArrRes->Fall < tDelay )
                ptArrRes->Fall = tDelay;
        }

        // get the fall of the output due to fall of the inputs
        if ( pSuper->tDelaysF[i].Fall > 0 )
        {
            tDelay = ptArrIn->Fall + pSuper->tDelaysF[i].Fall + tExtra;
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

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_TimePropagateRequiredPhase( Map_Man_t * p, Map_Node_t * pNode, int fPhase )
{
    Map_Time_t * ptReqIn, * ptReqOut;
    Map_Cut_t * pCut;
    Map_Super_t * pSuper;
    float tNewReqTime, tExtra;
    unsigned uPhase;
    int fPinPhase, i;

    tExtra = pNode->p->pNodeDelays ? pNode->p->pNodeDelays[pNode->Num] : 0;
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
            tNewReqTime = ptReqOut->Rise - pSuper->tDelaysR[i].Rise - tExtra;
            ptReqIn->Rise = MAP_MIN( ptReqIn->Rise, tNewReqTime );
        }

        // get the rise of the output due to fall of the inputs
//            if ( ptArrOut->Rise < ptArrIn->Fall + pSuper->tDelaysR[i].Fall )
//                ptArrOut->Rise = ptArrIn->Fall + pSuper->tDelaysR[i].Fall;
        if ( pSuper->tDelaysR[i].Fall > 0 )
        {
            tNewReqTime = ptReqOut->Rise - pSuper->tDelaysR[i].Fall - tExtra;
            ptReqIn->Fall = MAP_MIN( ptReqIn->Fall, tNewReqTime );
        }

        // get the fall of the output due to rise of the inputs
//            if ( ptArrOut->Fall < ptArrIn->Rise + pSuper->tDelaysF[i].Rise )
//                ptArrOut->Fall = ptArrIn->Rise + pSuper->tDelaysF[i].Rise;
        if ( pSuper->tDelaysF[i].Rise > 0 )
        {
            tNewReqTime = ptReqOut->Fall - pSuper->tDelaysF[i].Rise - tExtra;
            ptReqIn->Rise = MAP_MIN( ptReqIn->Rise, tNewReqTime );
        }

        // get the fall of the output due to fall of the inputs
//            if ( ptArrOut->Fall < ptArrIn->Fall + pSuper->tDelaysF[i].Fall )
//                ptArrOut->Fall = ptArrIn->Fall + pSuper->tDelaysF[i].Fall;
        if ( pSuper->tDelaysF[i].Fall > 0 )
        {
            tNewReqTime = ptReqOut->Fall - pSuper->tDelaysF[i].Fall - tExtra;
            ptReqIn->Fall = MAP_MIN( ptReqIn->Fall, tNewReqTime );
        }
    }

    // compare the required times with the arrival times
//    assert( pNode->tArrival[fPhase].Rise < ptReqOut->Rise + p->fEpsilon );
//    assert( pNode->tArrival[fPhase].Fall < ptReqOut->Fall + p->fEpsilon );
}
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


/**Function*************************************************************

  Synopsis    [Computes the required times of all nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_TimePropagateRequired( Map_Man_t * p )
{
    Map_Node_t * pNode;
    //Map_Time_t tReqOutTest, * ptReqOutTest = &tReqOutTest;
    Map_Time_t * ptReqIn, * ptReqOut;
    int fPhase, k;

    // go through the nodes in the reverse topological order
    for ( k = p->vMapObjs->nSize - 1; k >= 0; k-- )
    {
        pNode = p->vMapObjs->pArray[k];
        if ( pNode->nRefAct[2] == 0 )
            continue;

        // propagate required times through the buffer
        if ( Map_NodeIsBuf(pNode) )
        {
            assert( pNode->p2 == NULL );
            Map_Regular(pNode->p1)->tRequired[ Map_IsComplement(pNode->p1)] = pNode->tRequired[0];
            Map_Regular(pNode->p1)->tRequired[!Map_IsComplement(pNode->p1)] = pNode->tRequired[1];
            continue;
        }

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
/*
    // in the end, we verify the required times
    // for this, we compute the arrival times of the outputs of each phase 
    // of the supergates using the fanins' required times as the fanins' arrival times
    // the resulting arrival time of the supergate should be less than the actual required time
    for ( k = p->vMapObjs->nSize - 1; k >= 0; k-- )
    {
        pNode = p->vMapObjs->pArray[k];
        if ( pNode->nRefAct[2] == 0 )
            continue;
        if ( !Map_NodeIsAnd(pNode) )
            continue;
        // verify that the required times are propagated correctly
//        if ( pNode->pCutBest[0] && (pNode->nRefAct[0] > 0 || pNode->pCutBest[1] == NULL) )
        if ( pNode->pCutBest[0] && pNode->tRequired[0].Worst < MAP_FLOAT_LARGE/2 )
        {
            Map_MatchComputeReqTimes( pNode->pCutBest[0], 0, ptReqOutTest );
//            assert( ptReqOutTest->Rise < pNode->tRequired[0].Rise + p->fEpsilon );
//            assert( ptReqOutTest->Fall < pNode->tRequired[0].Fall + p->fEpsilon );
        }
//        if ( pNode->pCutBest[1] && (pNode->nRefAct[1] > 0 || pNode->pCutBest[0] == NULL) )
        if ( pNode->pCutBest[1] && pNode->tRequired[1].Worst < MAP_FLOAT_LARGE/2 )
        {
            Map_MatchComputeReqTimes( pNode->pCutBest[1], 1, ptReqOutTest );
//            assert( ptReqOutTest->Rise < pNode->tRequired[1].Rise + p->fEpsilon );
//            assert( ptReqOutTest->Fall < pNode->tRequired[1].Fall + p->fEpsilon );
        }
    }
*/
}
void Map_TimeComputeRequiredGlobal( Map_Man_t * p )
{
    int fUseConMan = Scl_ConIsRunning() && Scl_ConHasOutReqs();
    Map_Time_t * ptTime, * ptTimeA;
    int fPhase, i; 
    // update the required times according to the target
    p->fRequiredGlo = Map_TimeComputeArrivalMax( p );
    if ( p->DelayTarget != -1 )
    {
        if ( p->fRequiredGlo > p->DelayTarget + p->fEpsilon )
        {
            if ( p->fMappingMode == 1 )
                printf( "Cannot meet the target required times (%4.2f). Continue anyway.\n", p->DelayTarget );
        }
        else if ( p->fRequiredGlo < p->DelayTarget - p->fEpsilon )
        {
            if ( p->fMappingMode == 1 && p->fVerbose )
                printf( "Relaxing the required times from (%4.2f) to the target (%4.2f).\n", p->fRequiredGlo, p->DelayTarget );
            p->fRequiredGlo = p->DelayTarget;
        }
    }
    // clean the required times
    for ( i = 0; i < p->vMapObjs->nSize; i++ )
    {
        p->vMapObjs->pArray[i]->tRequired[0].Rise  = MAP_FLOAT_LARGE;
        p->vMapObjs->pArray[i]->tRequired[0].Fall  = MAP_FLOAT_LARGE;
        p->vMapObjs->pArray[i]->tRequired[0].Worst = MAP_FLOAT_LARGE;
        p->vMapObjs->pArray[i]->tRequired[1].Rise  = MAP_FLOAT_LARGE;
        p->vMapObjs->pArray[i]->tRequired[1].Fall  = MAP_FLOAT_LARGE;
        p->vMapObjs->pArray[i]->tRequired[1].Worst = MAP_FLOAT_LARGE;
    }
    // set the required times for the POs
    for ( i = 0; i < p->nOutputs; i++ )
    {
        fPhase  = !Map_IsComplement(p->pOutputs[i]);
        ptTime  =  Map_Regular(p->pOutputs[i])->tRequired + fPhase;
        ptTimeA =  Map_Regular(p->pOutputs[i])->tArrival + fPhase;

        if ( fUseConMan )
        {
            float Value = Scl_ConGetOutReqFloat(i);
            // if external required time can be achieved, use it
            if ( Value > 0 && ptTimeA->Worst <= Value )//&& Value <= p->fRequiredGlo )
                ptTime->Rise = ptTime->Fall = ptTime->Worst = Value;
            // if external required cannot be achieved, set the earliest possible arrival time
            else if ( Value > 0 && ptTimeA->Worst > Value )
                ptTime->Rise = ptTime->Fall = ptTime->Worst = ptTimeA->Worst;
            // otherwise, set the global required time
            else
                ptTime->Rise = ptTime->Fall = ptTime->Worst = p->fRequiredGlo;
        }
        else
        {
            // if external required time can be achieved, use it
            if ( p->pOutputRequireds && p->pOutputRequireds[i].Worst > 0 && ptTimeA->Worst <= p->pOutputRequireds[i].Worst )//&& p->pOutputRequireds[i].Worst <= p->fRequiredGlo )
                ptTime->Rise = ptTime->Fall = ptTime->Worst = p->pOutputRequireds[i].Worst;
            // if external required cannot be achieved, set the earliest possible arrival time
            else if ( p->pOutputRequireds && p->pOutputRequireds[i].Worst > 0 && ptTimeA->Worst > p->pOutputRequireds[i].Worst )
                ptTime->Rise = ptTime->Fall = ptTime->Worst = ptTimeA->Worst;
            // otherwise, set the global required time
            else
                ptTime->Rise = ptTime->Fall = ptTime->Worst = p->fRequiredGlo;
        }
    }
    // visit nodes in the reverse topological order
    Map_TimePropagateRequired( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

