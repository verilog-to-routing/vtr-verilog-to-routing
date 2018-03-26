/**CFile****************************************************************

  FileName    [mapperMatch.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic technology mapping engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - June 1, 2004.]

  Revision    [$Id: mapperMatch.c,v 1.7 2004/09/30 21:18:10 satrajit Exp $]

***********************************************************************/

#include "mapperInt.h"

#include "misc/util/utilNam.h"
#include "map/scl/sclCon.h"

ABC_NAMESPACE_IMPL_START


/*
    A potential improvement:
    When an internal node is not used in the mapping, its required times 
    are set to be +infinity. So when we recover area, we try to find the 
    best match for area and completely disregard the delay for the nodes
    that are not currently used in the mapping because any match whose 
    arrival times are less than the required times (+infinity) can be used.
    It may be possible to develop a better approach to recover area for
    the nodes that are not currently used in the mapping...
*/

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Cleans the match.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_MatchClean( Map_Match_t * pMatch )
{
    memset( pMatch, 0, sizeof(Map_Match_t) );
    pMatch->AreaFlow          = MAP_FLOAT_LARGE; // unassigned
    pMatch->tArrive.Rise   = MAP_FLOAT_LARGE; // unassigned
    pMatch->tArrive.Fall   = MAP_FLOAT_LARGE; // unassigned
    pMatch->tArrive.Worst  = MAP_FLOAT_LARGE; // unassigned
}

/**Function*************************************************************

  Synopsis    [Compares two matches.]

  Description [Returns 1 if the second match is better. Otherwise returns 0.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_MatchCompare( Map_Man_t * pMan, Map_Match_t * pM1, Map_Match_t * pM2, int fDoingArea )
{
//    if ( pM1->pSuperBest == pM2->pSuperBest )
//        return 0;
    if ( !fDoingArea )
    {
        // compare the arrival times
        if ( pM1->tArrive.Worst < pM2->tArrive.Worst - pMan->fEpsilon )
            return 0;
        if ( pM1->tArrive.Worst > pM2->tArrive.Worst + pMan->fEpsilon )
            return 1;
        // compare the areas or area flows
        if ( pM1->AreaFlow < pM2->AreaFlow - pMan->fEpsilon )
            return 0;
        if ( pM1->AreaFlow > pM2->AreaFlow + pMan->fEpsilon )
            return 1;
        // compare the fanout limits
        if ( pM1->pSuperBest->nFanLimit > pM2->pSuperBest->nFanLimit )
            return 0;
        if ( pM1->pSuperBest->nFanLimit < pM2->pSuperBest->nFanLimit )
            return 1;
        // compare the number of leaves
        if ( pM1->pSuperBest->nFanins < pM2->pSuperBest->nFanins )
            return 0;
        if ( pM1->pSuperBest->nFanins > pM2->pSuperBest->nFanins )
            return 1;
        // otherwise prefer the old cut
        return 0;
    }
    else
    {
        // compare the areas or area flows
        if ( pM1->AreaFlow < pM2->AreaFlow - pMan->fEpsilon )
            return 0;
        if ( pM1->AreaFlow > pM2->AreaFlow + pMan->fEpsilon )
            return 1;

        // make decision based on cell profile
        if ( pMan->fUseProfile && pM1->pSuperBest && pM1->pSuperBest )
        {
            int M1req = Mio_GateReadProfile(pM1->pSuperBest->pRoot);
            int M2req = Mio_GateReadProfile(pM2->pSuperBest->pRoot);
            int M1act = Mio_GateReadProfile2(pM1->pSuperBest->pRoot);
            int M2act = Mio_GateReadProfile2(pM2->pSuperBest->pRoot);
            //printf( "%d %d  ", M1req, M2req );
            if ( M1act < M1req && M2act > M2req )
                return 0;
            if ( M2act < M2req && M1act > M1req )
                return 1;
        }

        // compare the arrival times
        if ( pM1->tArrive.Worst < pM2->tArrive.Worst - pMan->fEpsilon )
            return 0;
        if ( pM1->tArrive.Worst > pM2->tArrive.Worst + pMan->fEpsilon )
            return 1;
        // compare the fanout limits
        if ( pM1->pSuperBest->nFanLimit > pM2->pSuperBest->nFanLimit )
            return 0;
        if ( pM1->pSuperBest->nFanLimit < pM2->pSuperBest->nFanLimit )
            return 1;
        // compare the number of leaves
        if ( pM1->pSuperBest->nFanins < pM2->pSuperBest->nFanins )
            return 0;
        if ( pM1->pSuperBest->nFanins > pM2->pSuperBest->nFanins )
            return 1;
        // otherwise prefer the old cut
        return 0;
    }
}

/**Function*************************************************************

  Synopsis    [Find the best matching of the cut.]

  Description [The parameters: the node (pNode), the cut (pCut), the phase to be matched 
  (fPhase), and the upper bound on the arrival times of the cut (fWorstLimit). This 
  procedure goes through the matching supergates up to the phase assignment, and selects the
  best supergate, which will be used to map the cut. As a result of calling this procedure
  the matching information is written into pMatch.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_MatchNodeCut( Map_Man_t * p, Map_Node_t * pNode, Map_Cut_t * pCut, int fPhase, float fWorstLimit )
{
    Map_Match_t MatchBest, * pMatch = pCut->M + fPhase;
    Map_Super_t * pSuper;
    int i, Counter;

    // save the current match of the cut
    MatchBest = *pMatch;
    // go through the supergates
    for ( pSuper = pMatch->pSupers, Counter = 0; pSuper; pSuper = pSuper->pNext, Counter++ )
    {
        p->nMatches++;
        // this is an attempt to reduce the runtime of matching and area 
        // at the cost of rare and very minor increase in delay
        // (the supergates are sorted by increasing area)
        if ( Counter == 30 )
           break;

        // go through different phases of the given match and supergate
        pMatch->pSuperBest = pSuper;
        for ( i = 0; i < (int)pSuper->nPhases; i++ )
        {
            p->nPhases++;
            // find the overall phase of this match
            pMatch->uPhaseBest = pMatch->uPhase ^ pSuper->uPhases[i];
            if ( p->fMappingMode == 0 )
            {
                // get the arrival time
                Map_TimeCutComputeArrival( pNode, pCut, fPhase, fWorstLimit );
                // skip the cut if the arrival times exceed the required times
                if ( pMatch->tArrive.Worst > fWorstLimit + p->fEpsilon )
                    continue;
                // get the area (area flow)
                pMatch->AreaFlow = Map_CutGetAreaFlow( pCut, fPhase );
            }
            else
            {
                // get the area (area flow)
                if ( p->fMappingMode == 2 || p->fMappingMode == 3 )
                    pMatch->AreaFlow = Map_CutGetAreaDerefed( pCut, fPhase );
                else if ( p->fMappingMode == 4 )
                    pMatch->AreaFlow = Map_SwitchCutGetDerefed( pNode, pCut, fPhase );
                else 
                    pMatch->AreaFlow = Map_CutGetAreaFlow( pCut, fPhase );
                // skip if the cut is too large
                if ( pMatch->AreaFlow > MatchBest.AreaFlow + p->fEpsilon )
                    continue;
                // get the arrival time
                Map_TimeCutComputeArrival( pNode, pCut, fPhase, fWorstLimit );
                // skip the cut if the arrival times exceed the required times
                if ( pMatch->tArrive.Worst > fWorstLimit + p->fEpsilon )
                    continue;
            }

            // if the cut is non-trivial, compare it
            if ( Map_MatchCompare( p, &MatchBest, pMatch, p->fMappingMode ) )
            {
                MatchBest = *pMatch;
                // if we are mapping for delay, the worst-case limit should be reduced
                if ( p->fMappingMode == 0 )
                    fWorstLimit = MatchBest.tArrive.Worst;
            }
        }
    }
    // set the best match
    *pMatch = MatchBest;

    // recompute the arrival time and area (area flow) of this cut
    if ( pMatch->pSuperBest )
    {
        Map_TimeCutComputeArrival( pNode, pCut, fPhase, MAP_FLOAT_LARGE );
        if ( p->fMappingMode == 2 || p->fMappingMode == 3 )
            pMatch->AreaFlow = Map_CutGetAreaDerefed( pCut, fPhase );
        else if ( p->fMappingMode == 4 )
            pMatch->AreaFlow = Map_SwitchCutGetDerefed( pNode, pCut, fPhase );
        else 
            pMatch->AreaFlow = Map_CutGetAreaFlow( pCut, fPhase );
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Find the matching of one polarity of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_MatchNodePhase( Map_Man_t * p, Map_Node_t * pNode, int fPhase )
{
    Map_Match_t MatchBest, * pMatch;
    Map_Cut_t * pCut, * pCutBest;
    float Area1 = 0.0; // Suppress "might be used uninitialized
    float Area2, fWorstLimit;

    // skip the cuts that have been unassigned during area recovery
    pCutBest = pNode->pCutBest[fPhase];
    if ( p->fMappingMode != 0 && pCutBest == NULL )
        return 1;

    // recompute the arrival times of the current best match 
    // because the arrival times of the fanins may have changed 
    // as a result of remapping fanins in the topological order
    if ( p->fMappingMode != 0 )
    {
        Map_TimeCutComputeArrival( pNode, pCutBest, fPhase, MAP_FLOAT_LARGE );
        // make sure that the required times are met
//        assert( pCutBest->M[fPhase].tArrive.Rise < pNode->tRequired[fPhase].Rise + p->fEpsilon );
//        assert( pCutBest->M[fPhase].tArrive.Fall < pNode->tRequired[fPhase].Fall + p->fEpsilon );
    }

    // recompute the exact area of the current best match
    // because the exact area of the fanins may have changed
    // as a result of remapping fanins in the topological order
    if ( p->fMappingMode == 2 || p->fMappingMode == 3 )
    {
        pMatch = pCutBest->M + fPhase;
        if ( pNode->nRefAct[fPhase] > 0 || 
            (pNode->pCutBest[!fPhase] == NULL && pNode->nRefAct[!fPhase] > 0) )
            pMatch->AreaFlow = Area1 = Map_CutDeref( pCutBest, fPhase, p->fUseProfile );
        else
            pMatch->AreaFlow = Area1 = Map_CutGetAreaDerefed( pCutBest, fPhase );
    }
    else if ( p->fMappingMode == 4 )
    {
        pMatch = pCutBest->M + fPhase;
        if ( pNode->nRefAct[fPhase] > 0 || 
            (pNode->pCutBest[!fPhase] == NULL && pNode->nRefAct[!fPhase] > 0) )
            pMatch->AreaFlow = Area1 = Map_SwitchCutDeref( pNode, pCutBest, fPhase );
        else
            pMatch->AreaFlow = Area1 = Map_SwitchCutGetDerefed( pNode, pCutBest, fPhase );
    }

    // save the old mapping
    if ( pCutBest )
        MatchBest = pCutBest->M[fPhase];
    else
        Map_MatchClean( &MatchBest );
 
    // select the new best cut
    fWorstLimit = pNode->tRequired[fPhase].Worst;
    for ( pCut = pNode->pCuts->pNext; pCut; pCut = pCut->pNext )
    {
        // limit gate sizes based on fanout count
        if ( p->fSkipFanout && ((pNode->nRefs > 3 && pCut->nLeaves > 2) || (pNode->nRefs > 1 && pCut->nLeaves > 3)) )
            continue;
        pMatch = pCut->M + fPhase;
        if ( pMatch->pSupers == NULL )
            continue;

        // find the matches for the cut
        Map_MatchNodeCut( p, pNode, pCut, fPhase, fWorstLimit );
        if ( pMatch->pSuperBest == NULL || pMatch->tArrive.Worst > fWorstLimit + p->fEpsilon )
            continue;

        // if the cut can be matched compare the matchings
        if ( Map_MatchCompare( p, &MatchBest, pMatch, p->fMappingMode ) )
        {
            pCutBest  =  pCut;
            MatchBest = *pMatch;
            // if we are mapping for delay, the worst-case limit should be tightened
            if ( p->fMappingMode == 0 )
                fWorstLimit = MatchBest.tArrive.Worst;
        }
    }

    if ( pCutBest == NULL )
        return 1;

    // set the new mapping
    pNode->pCutBest[fPhase] = pCutBest;
    pCutBest->M[fPhase]     = MatchBest;

    // reference the new cut if it used
    if ( p->fMappingMode >= 2 && 
         (pNode->nRefAct[fPhase] > 0 || 
         (pNode->pCutBest[!fPhase] == NULL && pNode->nRefAct[!fPhase] > 0)) )
    {
        if ( p->fMappingMode == 2 || p->fMappingMode == 3 )
            Area2 = Map_CutRef( pNode->pCutBest[fPhase], fPhase, p->fUseProfile );
        else if ( p->fMappingMode == 4 )
            Area2 = Map_SwitchCutRef( pNode, pNode->pCutBest[fPhase], fPhase );
        else 
            assert( 0 );
//        assert( Area2 < Area1 + p->fEpsilon );
    }

    // make sure that the requited times are met
//    assert( MatchBest.tArrive.Rise < pNode->tRequired[fPhase].Rise + p->fEpsilon );
//    assert( MatchBest.tArrive.Fall < pNode->tRequired[fPhase].Fall + p->fEpsilon );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Sets the PI arrival times.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_MappingSetPiArrivalTimes( Map_Man_t * p )
{
    Map_Node_t * pNode;
    int i;
    for ( i = 0; i < p->nInputs; i++ )
    {
        pNode = p->pInputs[i];
        // set the arrival time of the positive phase
        if ( Scl_ConIsRunning() )
        {
            float Time = Scl_ConGetInArrFloat( i );
            pNode->tArrival[1].Fall  = Time;
            pNode->tArrival[1].Rise  = Time;
            pNode->tArrival[1].Worst = Time;
        }
        else
            pNode->tArrival[1] = p->pInputArrivals[i];
        pNode->tArrival[1].Rise  += p->pNodeDelays ? p->pNodeDelays[pNode->Num] : 0;
        pNode->tArrival[1].Fall  += p->pNodeDelays ? p->pNodeDelays[pNode->Num] : 0;
        pNode->tArrival[1].Worst += p->pNodeDelays ? p->pNodeDelays[pNode->Num] : 0;
        // set the arrival time of the negative phase
        pNode->tArrival[0].Rise  = pNode->tArrival[1].Fall + p->pSuperLib->tDelayInv.Rise;
        pNode->tArrival[0].Fall  = pNode->tArrival[1].Rise + p->pSuperLib->tDelayInv.Fall;
        pNode->tArrival[0].Worst = MAP_MAX(pNode->tArrival[0].Rise, pNode->tArrival[0].Fall);
    }
}

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

  Synopsis    [Attempts dropping one phase of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_NodeTryDroppingOnePhase( Map_Man_t * p, Map_Node_t * pNode )
{
    Map_Match_t * pMatchBest0, * pMatchBest1;
    float tWorst0Using1, tWorst1Using0; 
    int fUsePhase1, fUsePhase0;

    // nothing to do if one of the phases is already dropped
    if ( pNode->pCutBest[0] == NULL || pNode->pCutBest[1] == NULL )
        return;

    // do not drop while recovering area flow
    if ( p->fMappingMode == 1 )//|| p->fMappingMode == 2 )
        return;

    // get the pointers to the matches of the best cuts
    pMatchBest0 = pNode->pCutBest[0]->M + 0;
    pMatchBest1 = pNode->pCutBest[1]->M + 1;

    // get the worst arrival times of each phase
    // implemented using the other phase with inverter added
    tWorst0Using1 = Map_TimeMatchWithInverter( p, pMatchBest1 );
    tWorst1Using0 = Map_TimeMatchWithInverter( p, pMatchBest0 );

    // consider the case of mapping for delay
    if ( p->fMappingMode == 0 && p->DelayTarget < ABC_INFINITY )
    { 
        // if the arrival time of a phase is larger than the arrival time 
        // of the opposite phase plus the inverter, drop this phase
        if ( pMatchBest0->tArrive.Worst > tWorst0Using1 + p->fEpsilon ) 
            pNode->pCutBest[0] = NULL;
        else if ( pMatchBest1->tArrive.Worst > tWorst1Using0 + p->fEpsilon ) 
            pNode->pCutBest[1] = NULL;
        return;
    }

    // do not perform replacement if one of the phases is unused
    if ( pNode->nRefAct[0] == 0 || pNode->nRefAct[1] == 0 )
        return;
 
    // check if replacement of each phase is possible using required times
    fUsePhase0 = fUsePhase1 = 0;
    if ( p->fMappingMode == 2 )
    {
        fUsePhase0 = (pNode->tRequired[1].Worst > tWorst1Using0 + 3*p->pSuperLib->tDelayInv.Worst + p->fEpsilon);
        fUsePhase1 = (pNode->tRequired[0].Worst > tWorst0Using1 + 3*p->pSuperLib->tDelayInv.Worst + p->fEpsilon);
    }
    else if ( p->fMappingMode == 3 || p->fMappingMode == 4 )
    {
        fUsePhase0 = (pNode->tRequired[1].Worst > tWorst1Using0 + p->fEpsilon);
        fUsePhase1 = (pNode->tRequired[0].Worst > tWorst0Using1 + p->fEpsilon);
    }
    if ( !fUsePhase0 && !fUsePhase1 )
        return;

    // if replacement is possible both ways, use the one that works better
    if ( fUsePhase0 && fUsePhase1 )
    {
        if ( pMatchBest0->AreaFlow < pMatchBest1->AreaFlow )
            fUsePhase1 = 0;
        else
            fUsePhase0 = 0;
    }
    // only one phase should be used
    assert( fUsePhase0 ^ fUsePhase1 );

    // set the corresponding cut to NULL
    if ( fUsePhase0 )
    {
        // deref phase 1 cut if necessary
        if ( p->fMappingMode >= 2 && pNode->nRefAct[1] > 0 )
            Map_CutDeref( pNode->pCutBest[1], 1, p->fUseProfile );
        // get rid of the cut
        pNode->pCutBest[1] = NULL;
        // ref phase 0 cut if necessary
        if ( p->fMappingMode >= 2 && pNode->nRefAct[0] == 0 )
            Map_CutRef( pNode->pCutBest[0], 0, p->fUseProfile );
    }
    else
    {
        // deref phase 0 cut if necessary
        if ( p->fMappingMode >= 2 && pNode->nRefAct[0] > 0 )
            Map_CutDeref( pNode->pCutBest[0], 0, p->fUseProfile );
        // get rid of the cut
        pNode->pCutBest[0] = NULL;
        // ref phase 1 cut if necessary
        if ( p->fMappingMode >= 2 && pNode->nRefAct[1] == 0 )
            Map_CutRef( pNode->pCutBest[1], 1, p->fUseProfile );
    }
}


/**Function*************************************************************

  Synopsis    [Transfers the arrival times from the best cuts to the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_NodeTransferArrivalTimes( Map_Man_t * p, Map_Node_t * pNode )
{
    // if both phases are available, set their arrival times
    if ( pNode->pCutBest[0] && pNode->pCutBest[1] )
    {
        pNode->tArrival[0] = pNode->pCutBest[0]->M[0].tArrive;
        pNode->tArrival[1] = pNode->pCutBest[1]->M[1].tArrive;
    }
    // if only one phase is available, compute the arrival time of other phase
    else if ( pNode->pCutBest[0] )
    {
        pNode->tArrival[0] = pNode->pCutBest[0]->M[0].tArrive;
        pNode->tArrival[1].Rise  = pNode->tArrival[0].Fall + p->pSuperLib->tDelayInv.Rise;
        pNode->tArrival[1].Fall  = pNode->tArrival[0].Rise + p->pSuperLib->tDelayInv.Fall;
        pNode->tArrival[1].Worst = MAP_MAX(pNode->tArrival[1].Rise, pNode->tArrival[1].Fall);
    }
    else if ( pNode->pCutBest[1] )
    {
        pNode->tArrival[1] = pNode->pCutBest[1]->M[1].tArrive;
        pNode->tArrival[0].Rise  = pNode->tArrival[1].Fall + p->pSuperLib->tDelayInv.Rise;
        pNode->tArrival[0].Fall  = pNode->tArrival[1].Rise + p->pSuperLib->tDelayInv.Fall;
        pNode->tArrival[0].Worst = MAP_MAX(pNode->tArrival[0].Rise, pNode->tArrival[0].Fall);
    }
    else 
    {
        assert( 0 );
    }

//    assert( pNode->tArrival[0].Rise < pNode->tRequired[0].Rise + p->fEpsilon );
//    assert( pNode->tArrival[0].Fall < pNode->tRequired[0].Fall + p->fEpsilon );

//    assert( pNode->tArrival[1].Rise < pNode->tRequired[1].Rise + p->fEpsilon );
//    assert( pNode->tArrival[1].Fall < pNode->tRequired[1].Fall + p->fEpsilon );
}

/**Function*************************************************************

  Synopsis    [Computes the best matches of the nodes.]

  Description [Uses parameter p->fMappingMode to decide how to assign
  the matches for both polarities of the node. While the matches are 
  being assigned, one of them may turn out to be better than the other 
  (in terms of delay, for example). In this case, the worse match can 
  be permanently dropped, and the corresponding pointer set to NULL.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_MappingMatches( Map_Man_t * p )
{
    ProgressBar * pProgress;
    Map_Node_t * pNode;
    int i;

    assert( p->fMappingMode >= 0 && p->fMappingMode <= 4 );

    // use the externally given PI arrival times
    if ( p->fMappingMode == 0 )
        Map_MappingSetPiArrivalTimes( p );

    // estimate the fanouts
    if ( p->fMappingMode == 0 )
        Map_MappingEstimateRefsInit( p );
    else if ( p->fMappingMode == 1 )
        Map_MappingEstimateRefs( p );

    // the PI cuts are matched in the cut computation package
    // in the loop below we match the internal nodes
    pProgress = Extra_ProgressBarStart( stdout, p->vMapObjs->nSize );
    for ( i = 0; i < p->vMapObjs->nSize; i++ )
    {
        pNode = p->vMapObjs->pArray[i];
        if ( Map_NodeIsBuf(pNode) )
        {
            assert( pNode->p2 == NULL );
            pNode->tArrival[0] = Map_Regular(pNode->p1)->tArrival[ Map_IsComplement(pNode->p1)];
            pNode->tArrival[1] = Map_Regular(pNode->p1)->tArrival[!Map_IsComplement(pNode->p1)];
            continue;
        }

        // skip primary inputs and secondary nodes if mapping with choices
        if ( !Map_NodeIsAnd( pNode ) || pNode->pRepr )
            continue;

        // make sure that at least one non-trival cut is present
        if ( pNode->pCuts->pNext == NULL )
        {
            Extra_ProgressBarStop( pProgress );
            printf( "\nError: A node in the mapping graph does not have feasible cuts.\n" );
            return 0;
        }

        // match negative phase
        if ( !Map_MatchNodePhase( p, pNode, 0 ) )
        {
            Extra_ProgressBarStop( pProgress );
            return 0;
        }
        // match positive phase
        if ( !Map_MatchNodePhase( p, pNode, 1 ) )
        {
            Extra_ProgressBarStop( pProgress );
            return 0;
        }

        // make sure that at least one phase is mapped
        if ( pNode->pCutBest[0] == NULL && pNode->pCutBest[1] == NULL )
        {
            printf( "\nError: Could not match both phases of AIG node %d.\n", pNode->Num );
            printf( "Please make sure that the supergate library has equivalents of AND2 or NAND2.\n" );
            printf( "If such supergates exist in the library, report a bug.\n" );
            Extra_ProgressBarStop( pProgress );
            return 0;
        }

        // if both phases are assigned, check if one of them can be dropped
        Map_NodeTryDroppingOnePhase( p, pNode );
        // set the arrival times of the node using the best cuts
        Map_NodeTransferArrivalTimes( p, pNode );

        // update the progress bar
        Extra_ProgressBarUpdate( pProgress, i, "Matches ..." );
    }
    Extra_ProgressBarStop( pProgress );
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
ABC_NAMESPACE_IMPL_END

