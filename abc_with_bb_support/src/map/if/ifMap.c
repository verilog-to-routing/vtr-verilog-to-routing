/**CFile****************************************************************

  FileName    [ifMap.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [FPGA mapping based on priority cuts.]

  Synopsis    [Mapping procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 21, 2006.]

  Revision    [$Id: ifMap.c,v 1.00 2006/11/21 00:00:00 alanmi Exp $]

***********************************************************************/

#include "if.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Counts the number of 1s in the signature.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int If_WordCountOnes( unsigned uWord )
{
    uWord = (uWord & 0x55555555) + ((uWord>>1) & 0x55555555);
    uWord = (uWord & 0x33333333) + ((uWord>>2) & 0x33333333);
    uWord = (uWord & 0x0F0F0F0F) + ((uWord>>4) & 0x0F0F0F0F);
    uWord = (uWord & 0x00FF00FF) + ((uWord>>8) & 0x00FF00FF);
    return  (uWord & 0x0000FFFF) + (uWord>>16);
}

/**Function*************************************************************

  Synopsis    [Finds the best cut for the given node.]

  Description [Mapping modes: delay (0), area flow (1), area (2).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ObjPerformMappingAnd( If_Man_t * p, If_Obj_t * pObj, int Mode, int fPreprocess )
{
    If_Set_t * pCutSet;
    If_Cut_t * pCut0, * pCut1, * pCut;
    int i, k;

    assert( p->pPars->fSeqMap || !If_ObjIsAnd(pObj->pFanin0) || pObj->pFanin0->pCutSet->nCuts > 1 );
    assert( p->pPars->fSeqMap || !If_ObjIsAnd(pObj->pFanin1) || pObj->pFanin1->pCutSet->nCuts > 1 );

    // prepare
    if ( !p->pPars->fSeqMap )
    {
        if ( Mode == 0 )
            pObj->EstRefs = (float)pObj->nRefs;
        else if ( Mode == 1 )
            pObj->EstRefs = (float)((2.0 * pObj->EstRefs + pObj->nRefs) / 3.0);
    }
    if ( Mode && pObj->nRefs > 0 )
        If_CutDeref( p, If_ObjCutBest(pObj), IF_INFINITY );

    // prepare the cutset
    pCutSet = If_ManSetupNodeCutSet( p, pObj );

    // get the current assigned best cut
    pCut = If_ObjCutBest(pObj);
    if ( pCut->nLeaves > 0 )
    {
        // recompute the parameters of the best cut
        pCut->Delay = If_CutDelay( p, pCut );
        assert( pCut->Delay <= pObj->Required + p->fEpsilon );
        pCut->Area = (Mode == 2)? If_CutAreaDerefed( p, pCut, IF_INFINITY ) : If_CutFlow( p, pCut );
        // save the best cut from the previous iteration
        if ( !fPreprocess )
            If_CutCopy( p, pCutSet->ppCuts[pCutSet->nCuts++], pCut );
    }

    // generate cuts
    If_ObjForEachCut( pObj->pFanin0, pCut0, i )
    If_ObjForEachCut( pObj->pFanin1, pCut1, k )
    {
        // get the next free cut
        assert( pCutSet->nCuts <= pCutSet->nCutsMax );
        pCut = pCutSet->ppCuts[pCutSet->nCuts];
        // make sure K-feasible cut exists
        if ( If_WordCountOnes(pCut0->uSign | pCut1->uSign) > p->pPars->nLutSize )
            continue;
        // merge the nodes
        if ( !If_CutMerge( pCut0, pCut1, pCut ) )
            continue;
        assert( p->pPars->fSeqMap || pCut->nLeaves > 1 );
        p->nCutsMerged++;
        // check if this cut is contained in any of the available cuts
//        if ( p->pPars->pFuncCost == NULL && If_CutFilter( p, pCut ) ) // do not filter functionality cuts
        if ( If_CutFilter( pCutSet, pCut ) )
            continue;
        // compute the truth table
        pCut->fCompl = 0;
        if ( p->pPars->fTruth )
            If_CutComputeTruth( p, pCut, pCut0, pCut1, pObj->fCompl0, pObj->fCompl1 );
        // compute the application-specific cost and depth
        pCut->fUser = (p->pPars->pFuncCost != NULL);
        pCut->Cost = p->pPars->pFuncCost? p->pPars->pFuncCost(pCut) : 0;
        if ( pCut->Cost == IF_COST_MAX )
            continue;
        // check if the cut satisfies the required times
        pCut->Delay = If_CutDelay( p, pCut );
//        printf( "%.2f ", pCut->Delay );
        if ( Mode && pCut->Delay > pObj->Required + p->fEpsilon )
            continue;
        // compute area of the cut (this area may depend on the application specific cost)
        pCut->Area = (Mode == 2)? If_CutAreaDerefed( p, pCut, IF_INFINITY ) : If_CutFlow( p, pCut );
        pCut->AveRefs = (Mode == 0)? (float)0.0 : If_CutAverageRefs( p, pCut );
        // insert the cut into storage
        If_CutSort( p, pCutSet, pCut );
    } 
    assert( pCutSet->nCuts > 0 );

    // add the trivial cut to the set
    If_ManSetupCutTriv( p, pCutSet->ppCuts[pCutSet->nCuts++], pObj->Id );
    assert( pCutSet->nCuts <= pCutSet->nCutsMax+1 );

    // update the best cut
    if ( !fPreprocess || pCutSet->ppCuts[0]->Delay <= pObj->Required + p->fEpsilon )
        If_CutCopy( p, If_ObjCutBest(pObj), pCutSet->ppCuts[0] );
    assert( p->pPars->fSeqMap || If_ObjCutBest(pObj)->nLeaves > 1 );

    // ref the selected cut
    if ( Mode && pObj->nRefs > 0 )
        If_CutRef( p, If_ObjCutBest(pObj), IF_INFINITY );

    // call the user specified function for each cut
    if ( p->pPars->pFuncUser )
        If_ObjForEachCut( pObj, pCut, i )
            p->pPars->pFuncUser( p, pObj, pCut );

    // free the cuts
    If_ManDerefNodeCutSet( p, pObj );
}

/**Function*************************************************************

  Synopsis    [Finds the best cut for the choice node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ObjPerformMappingChoice( If_Man_t * p, If_Obj_t * pObj, int Mode, int fPreprocess )
{
    If_Set_t * pCutSet;
    If_Obj_t * pTemp;
    If_Cut_t * pCutTemp, * pCut;
    int i;
    assert( pObj->pEquiv != NULL );

    // prepare
    if ( Mode && pObj->nRefs > 0 )
        If_CutDeref( p, If_ObjCutBest(pObj), IF_INFINITY );

    // remove elementary cuts
    for ( pTemp = pObj; pTemp; pTemp = pTemp->pEquiv )
        pTemp->pCutSet->nCuts--;

    // update the cutset of the node
    pCutSet = pObj->pCutSet;

    // generate cuts
    for ( pTemp = pObj->pEquiv; pTemp; pTemp = pTemp->pEquiv )
    {
        assert( pTemp->nRefs == 0 );
        assert( p->pPars->fSeqMap || pTemp->pCutSet->nCuts > 0 );
        // go through the cuts of this node
        If_ObjForEachCut( pTemp, pCutTemp, i )
        {
            assert( p->pPars->fSeqMap || pCutTemp->nLeaves > 1 );
            // get the next free cut
            assert( pCutSet->nCuts <= pCutSet->nCutsMax );
            pCut = pCutSet->ppCuts[pCutSet->nCuts];
            // copy the cut into storage
            If_CutCopy( p, pCut, pCutTemp );
            // check if this cut is contained in any of the available cuts
            if ( If_CutFilter( pCutSet, pCut ) )
                continue;
            // check if the cut satisfies the required times
            assert( pCut->Delay == If_CutDelay( p, pCut ) );
            if ( Mode && pCut->Delay > pObj->Required + p->fEpsilon )
                continue;
            // set the phase attribute
            assert( pCut->fCompl == 0 );
            pCut->fCompl ^= (pObj->fPhase ^ pTemp->fPhase); // why ^= ?
            // compute area of the cut (this area may depend on the application specific cost)
            pCut->Area = (Mode == 2)? If_CutAreaDerefed( p, pCut, IF_INFINITY ) : If_CutFlow( p, pCut );
            pCut->AveRefs = (Mode == 0)? (float)0.0 : If_CutAverageRefs( p, pCut );
            // insert the cut into storage
            If_CutSort( p, pCutSet, pCut );
        }
    }
    assert( pCutSet->nCuts > 0 );

    // add the trivial cut to the set
    If_ManSetupCutTriv( p, pCutSet->ppCuts[pCutSet->nCuts++], pObj->Id );
    assert( pCutSet->nCuts <= pCutSet->nCutsMax+1 );

    // update the best cut
    if ( !fPreprocess || pCutSet->ppCuts[0]->Delay <= pObj->Required + p->fEpsilon )
        If_CutCopy( p, If_ObjCutBest(pObj), pCutSet->ppCuts[0] );
    assert( p->pPars->fSeqMap || If_ObjCutBest(pObj)->nLeaves > 1 );

    // ref the selected cut
    if ( Mode && pObj->nRefs > 0 )
        If_CutRef( p, If_ObjCutBest(pObj), IF_INFINITY );

    // free the cuts
    If_ManDerefChoiceCutSet( p, pObj );
}

/**Function*************************************************************

  Synopsis    [Performs one mapping pass over all nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_ManPerformMappingRound( If_Man_t * p, int nCutsUsed, int Mode, int fPreprocess, char * pLabel )
{
//    ProgressBar * pProgress;
    If_Obj_t * pObj;
    int i, clk = clock();
    assert( Mode >= 0 && Mode <= 2 );
    // set the sorting function
    if ( Mode || p->pPars->fArea ) // area
        p->SortMode = 1;
    else if ( p->pPars->fFancy )
        p->SortMode = 2;
    else
        p->SortMode = 0;
    // set the cut number
    p->nCutsUsed   = nCutsUsed;
    p->nCutsMerged = 0;
    // map the internal nodes
//    pProgress = Extra_ProgressBarStart( stdout, If_ManObjNum(p) );
    If_ManForEachNode( p, pObj, i )
    {
//        Extra_ProgressBarUpdate( pProgress, i, pLabel );
        If_ObjPerformMappingAnd( p, pObj, Mode, fPreprocess );
        if ( pObj->fRepr )
            If_ObjPerformMappingChoice( p, pObj, Mode, fPreprocess );
    }
//    Extra_ProgressBarStop( pProgress );
    // make sure the visit counters are all zero
    If_ManForEachNode( p, pObj, i )
        assert( pObj->nVisits == 0 );
    // compute required times and stats
    If_ManComputeRequired( p );
    if ( p->pPars->fVerbose )
    {
        char Symb = fPreprocess? 'P' : ((Mode == 0)? 'D' : ((Mode == 1)? 'F' : 'A'));
        printf( "%c: Del = %7.2f. Ar = %9.1f. Net = %8d. Cut = %8d. ", 
            Symb, p->RequiredGlo, p->AreaGlo, p->nNets, p->nCutsMerged );
        PRT( "T", clock() - clk );
//    printf( "Max number of cuts = %d. Average number of cuts = %5.2f.\n", 
//        p->nCutsMax, 1.0 * p->nCutsMerged / If_ManAndNum(p) );
    }
    return 1;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


