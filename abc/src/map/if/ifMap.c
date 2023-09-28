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
#include "misc/extra/extra.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern char * Dau_DsdMerge( char * pDsd0i, int * pPerm0, char * pDsd1i, int * pPerm1, int fCompl0, int fCompl1, int nVars );
extern int    If_CutDelayRecCost3( If_Man_t* p, If_Cut_t* pCut, If_Obj_t * pObj );
extern int    Abc_ExactDelayCost( word * pTruth, int nVars, int * pArrTimeProfile, char * pPerm, int * Cost, int AigLevel );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Compute delay of the cut's output in terms of logic levels.]

  Description [Uses the best arrival time of the fanins of the cut
  to compute the arrival times of the output of the cut.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_ManCutAigDelay_rec( If_Man_t * p, If_Obj_t * pObj, Vec_Ptr_t * vVisited )
{
    int Delay0, Delay1;
    if ( pObj->fVisit )
        return pObj->iCopy;
    if ( If_ObjIsCi(pObj) || If_ObjIsConst1(pObj) )
        return -1;
    // store the node in the structure by level
    assert( If_ObjIsAnd(pObj) );
    pObj->fVisit = 1;
    Vec_PtrPush( vVisited, pObj );
    Delay0 = If_ManCutAigDelay_rec( p, pObj->pFanin0, vVisited );
    Delay1 = If_ManCutAigDelay_rec( p, pObj->pFanin1, vVisited );
    pObj->iCopy = (Delay0 >= 0 && Delay1 >= 0) ? 1 + Abc_MaxInt(Delay0, Delay1) : -1;
    return pObj->iCopy;
}
int If_ManCutAigDelay( If_Man_t * p, If_Obj_t * pObj, If_Cut_t * pCut )
{
    If_Obj_t * pLeaf;
    int i, Delay;
    Vec_PtrClear( p->vVisited );
    If_CutForEachLeaf( p, pCut, pLeaf, i )
    {
        assert( pLeaf->fVisit == 0 );
        pLeaf->fVisit = 1;
        Vec_PtrPush( p->vVisited, pLeaf );
        pLeaf->iCopy = If_ObjCutBest(pLeaf)->Delay;
    }
    Delay = If_ManCutAigDelay_rec( p, pObj, p->vVisited );
    Vec_PtrForEachEntry( If_Obj_t *, p->vVisited, pLeaf, i )
        pLeaf->fVisit = 0;
//    assert( Delay <= (int)pObj->Level );
    return Delay;
}

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

  Synopsis    [Counts the number of 1s in the signature.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float If_CutDelaySpecial( If_Man_t * p, If_Cut_t * pCut, int fCarry )
{
    static float Pin2Pin[2][3] = { {1.0, 1.0, 1.0}, {1.0, 1.0, 0.0} };
    If_Obj_t * pLeaf;
    float DelayCur, Delay = -IF_FLOAT_LARGE;
    int i;
    assert( pCut->nLeaves <= 3 );
    If_CutForEachLeaf( p, pCut, pLeaf, i )
    {
        DelayCur = If_ObjCutBest(pLeaf)->Delay;
        Delay = IF_MAX( Delay, Pin2Pin[fCarry][i] + DelayCur );
    }
    return Delay;
}

/**Function*************************************************************

  Synopsis    [Returns arrival time profile of the cut.]

  Description [The procedure returns static storage, which should not be
  deallocated and is only valid until before the procedure is called again.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * If_CutArrTimeProfile( If_Man_t * p, If_Cut_t * pCut )
{
    int i;
    for ( i = 0; i < If_CutLeaveNum(pCut); i++ )
        p->pArrTimeProfile[i] = (int)If_ObjCutBest(If_CutLeaf(p, pCut, i))->Delay;
    return p->pArrTimeProfile;
}

/**Function*************************************************************

  Synopsis    [Finds the best cut for the given node.]

  Description [Mapping modes: delay (0), area flow (1), area (2).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ObjPerformMappingAnd( If_Man_t * p, If_Obj_t * pObj, int Mode, int fPreprocess, int fFirst )
{
    If_Set_t * pCutSet;
    If_Cut_t * pCut0, * pCut1, * pCut;
    If_Cut_t * pCut0R, * pCut1R;
    int fFunc0R, fFunc1R;
    int i, k, v, iCutDsd, fChange;
    int fSave0 = p->pPars->fDelayOpt || p->pPars->fDelayOptLut || p->pPars->fDsdBalance || p->pPars->fUserRecLib || p->pPars->fUserSesLib || 
        p->pPars->fUseDsdTune || p->pPars->fUseCofVars || p->pPars->fUseAndVars || p->pPars->fUse34Spec || p->pPars->pLutStruct || p->pPars->pFuncCell2 || p->pPars->fUseCheck1 || p->pPars->fUseCheck2;
    int fUseAndCut = (p->pPars->nAndDelay > 0) || (p->pPars->nAndArea > 0);
    assert( !If_ObjIsAnd(pObj->pFanin0) || pObj->pFanin0->pCutSet->nCuts > 0 );
    assert( !If_ObjIsAnd(pObj->pFanin1) || pObj->pFanin1->pCutSet->nCuts > 0 );

    // prepare
    if ( Mode == 0 )
        pObj->EstRefs = (float)pObj->nRefs;
    else if ( Mode == 1 )
        pObj->EstRefs = (float)((2.0 * pObj->EstRefs + pObj->nRefs) / 3.0);
    // deref the selected cut
    if ( Mode && pObj->nRefs > 0 )
        If_CutAreaDeref( p, If_ObjCutBest(pObj) );

    // prepare the cutset
    pCutSet = If_ManSetupNodeCutSet( p, pObj );

    // get the current assigned best cut
    pCut = If_ObjCutBest(pObj);
    if ( !fFirst )
    {
        // recompute the parameters of the best cut
        if ( p->pPars->fDelayOpt )
            pCut->Delay = If_CutSopBalanceEval( p, pCut, NULL );
        else if ( p->pPars->fDsdBalance )
            pCut->Delay = If_CutDsdBalanceEval( p, pCut, NULL );
        else if ( p->pPars->fUserRecLib )
            pCut->Delay = If_CutDelayRecCost3( p, pCut, pObj ); 
        else if ( p->pPars->fUserSesLib )
        {
            int Cost = 0;
            pCut->fUser = 1;
            pCut->Delay = (float)Abc_ExactDelayCost( If_CutTruthW(p, pCut), If_CutLeaveNum(pCut), If_CutArrTimeProfile(p, pCut), If_CutPerm(pCut), &Cost, If_ManCutAigDelay(p, pObj, pCut) ); 
            if ( Cost == ABC_INFINITY )
            {
                for ( v = 0; v < If_CutLeaveNum(pCut); v++ )
                    If_CutPerm(pCut)[v] = IF_BIG_CHAR;
                pCut->Cost = IF_COST_MAX;
                pCut->fUseless = 1;
            }
        }
        else if ( p->pPars->fDelayOptLut )
            pCut->Delay = If_CutLutBalanceEval( p, pCut );
        else if( p->pPars->nGateSize > 0 )
            pCut->Delay = If_CutDelaySop( p, pCut );
        else
            pCut->Delay = If_CutDelay( p, pObj, pCut );
        assert( pCut->Delay != -1 );
//        assert( pCut->Delay <= pObj->Required + p->fEpsilon );
        if ( pCut->Delay > pObj->Required + 2*p->fEpsilon )
            Abc_Print( 1, "If_ObjPerformMappingAnd(): Warning! Node with ID %d has delay (%f) exceeding the required times (%f).\n", 
                pObj->Id, pCut->Delay, pObj->Required + p->fEpsilon );
        pCut->Area = (Mode == 2)? If_CutAreaDerefed( p, pCut ) : If_CutAreaFlow( p, pCut );
        if ( p->pPars->fEdge )
            pCut->Edge = (Mode == 2)? If_CutEdgeDerefed( p, pCut ) : If_CutEdgeFlow( p, pCut );
        if ( p->pPars->fPower )
            pCut->Power = (Mode == 2)? If_CutPowerDerefed( p, pCut, pObj ) : If_CutPowerFlow( p, pCut, pObj );
        // save the best cut from the previous iteration
        if ( !fPreprocess || pCut->nLeaves <= 1 )
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

        pCut0R = pCut0;
        pCut1R = pCut1;
        fFunc0R = pCut0->iCutFunc ^ pCut0->fCompl ^ pObj->fCompl0;
        fFunc1R = pCut1->iCutFunc ^ pCut1->fCompl ^ pObj->fCompl1;
        if ( !p->pPars->fUseTtPerm || pCut0->nLeaves > pCut1->nLeaves || (pCut0->nLeaves == pCut1->nLeaves && fFunc0R > fFunc1R) )
        {
        }
        else
        {
            ABC_SWAP( If_Cut_t *, pCut0R, pCut1R );
            ABC_SWAP( int, fFunc0R, fFunc1R );
        }        

        // merge the cuts
        if ( p->pPars->fUseTtPerm )
        {
            if ( !If_CutMerge( p, pCut0R, pCut1R, pCut ) )
                continue;
        }
        else
        {
            if ( !If_CutMergeOrdered( p, pCut0, pCut1, pCut ) )
                continue;
        }
        if ( pObj->fSpec && pCut->nLeaves == (unsigned)p->pPars->nLutSize )
            continue;
        p->nCutsMerged++;
        p->nCutsTotal++;
        // check if this cut is contained in any of the available cuts
        if ( !p->pPars->fSkipCutFilter && If_CutFilter( pCutSet, pCut, fSave0 ) )
            continue;
        // check if the cut is a special AND-gate cut
        pCut->fAndCut = fUseAndCut && pCut->nLeaves == 2 && pCut->pLeaves[0] == pObj->pFanin0->Id && pCut->pLeaves[1] == pObj->pFanin1->Id;
        //assert( pCut->nLeaves != 2 || pCut->pLeaves[0] < pCut->pLeaves[1] );
        //assert( pCut->nLeaves != 2 || pObj->pFanin0->Id < pObj->pFanin1->Id );
        // compute the truth table
        pCut->iCutFunc = -1;
        pCut->fCompl = 0;
        if ( p->pPars->fTruth )
        {
//            int nShared = pCut0->nLeaves + pCut1->nLeaves - pCut->nLeaves;
            abctime clk = 0;
            if ( p->pPars->fVerbose )
                clk = Abc_Clock();
            if ( p->pPars->fUseTtPerm )
                fChange = If_CutComputeTruthPerm( p, pCut, pCut0R, pCut1R, fFunc0R, fFunc1R );
            else
                fChange = If_CutComputeTruth( p, pCut, pCut0, pCut1, pObj->fCompl0, pObj->fCompl1 );
            if ( p->pPars->fVerbose )
                p->timeCache[4] += Abc_Clock() - clk;
            if ( !p->pPars->fSkipCutFilter && fChange && If_CutFilter( pCutSet, pCut, fSave0 ) )
                continue;
            if ( p->pPars->fLut6Filter && pCut->nLeaves == 6 && !If_CutCheckTruth6(p, pCut) )
                continue;
            if ( p->pPars->fUseDsd )
            {
                extern void If_ManCacheRecord( If_Man_t * p, int iDsd0, int iDsd1, int nShared, int iDsd );
                int truthId = Abc_Lit2Var(pCut->iCutFunc);
                if ( truthId >= Vec_IntSize(p->vTtDsds[pCut->nLeaves]) || Vec_IntEntry(p->vTtDsds[pCut->nLeaves], truthId) == -1 )
                {
                    while ( truthId >= Vec_IntSize(p->vTtDsds[pCut->nLeaves]) )
                    {
                        Vec_IntPush( p->vTtDsds[pCut->nLeaves], -1 );
                        for ( v = 0; v < Abc_MaxInt(6, pCut->nLeaves); v++ )
                            Vec_StrPush( p->vTtPerms[pCut->nLeaves], IF_BIG_CHAR );
                    }
                    iCutDsd = If_DsdManCompute( p->pIfDsdMan, If_CutTruthWR(p, pCut), pCut->nLeaves, (unsigned char *)If_CutDsdPerm(p, pCut), p->pPars->pLutStruct );
                    Vec_IntWriteEntry( p->vTtDsds[pCut->nLeaves], truthId, iCutDsd );
                }
                assert( If_DsdManSuppSize(p->pIfDsdMan, If_CutDsdLit(p, pCut)) == (int)pCut->nLeaves );
                //If_ManCacheRecord( p, If_CutDsdLit(p, pCut0), If_CutDsdLit(p, pCut1), nShared, If_CutDsdLit(p, pCut) );
            }
            // run user functions
            pCut->fUseless = 0;
            if ( p->pPars->pFuncCell || p->pPars->pFuncCell2 )
            {
                assert( p->pPars->fUseTtPerm == 0 );
                assert( pCut->nLimit >= 4 && pCut->nLimit <= 16 );
                if ( p->pPars->fUseDsd )
                    pCut->fUseless = If_DsdManCheckDec( p->pIfDsdMan, If_CutDsdLit(p, pCut) );
                else if ( p->pPars->pFuncCell2 )
                    pCut->fUseless = !p->pPars->pFuncCell2( p, (word *)If_CutTruthW(p, pCut), pCut->nLeaves, NULL, NULL );
                else
                    pCut->fUseless = !p->pPars->pFuncCell( p, If_CutTruth(p, pCut), Abc_MaxInt(6, pCut->nLeaves), pCut->nLeaves, p->pPars->pLutStruct );
                p->nCutsUselessAll += pCut->fUseless;
                p->nCutsUseless[pCut->nLeaves] += pCut->fUseless;
                p->nCutsCountAll++;
                p->nCutsCount[pCut->nLeaves]++;
                // skip 5-input cuts, which cannot be decomposed
                if ( (p->pPars->fEnableCheck75 || p->pPars->fEnableCheck75u) && pCut->nLeaves == 5 && pCut->nLimit == 5 )
                {
                    extern int If_CluCheckDecInAny( word t, int nVars );
                    extern int If_CluCheckDecOut( word t, int nVars );
                    unsigned TruthU = *If_CutTruth(p, pCut);
                    word Truth = (((word)TruthU << 32) | (word)TruthU);
                    p->nCuts5++;
                    if ( If_CluCheckDecInAny( Truth, 5 ) )
                        p->nCuts5a++;
                    else
                        continue;
                }
                else if ( p->pPars->fVerbose && pCut->nLeaves == 5 )
                {
                    extern int If_CluCheckDecInAny( word t, int nVars );
                    extern int If_CluCheckDecOut( word t, int nVars );
                    unsigned TruthU = *If_CutTruth(p, pCut);
                    word Truth = (((word)TruthU << 32) | (word)TruthU);
                    p->nCuts5++;
                    if ( If_CluCheckDecInAny( Truth, 5 ) || If_CluCheckDecOut( Truth, 5 ) )
                        p->nCuts5a++;
                }
            }
            else if ( p->pPars->fUseDsdTune )
            {
                pCut->fUseless = If_DsdManReadMark( p->pIfDsdMan, If_CutDsdLit(p, pCut) );
                p->nCutsUselessAll += pCut->fUseless;
                p->nCutsUseless[pCut->nLeaves] += pCut->fUseless;
                p->nCutsCountAll++;
                p->nCutsCount[pCut->nLeaves]++;
            }
            else if ( p->pPars->fUse34Spec )
            {
                assert( pCut->nLeaves <= 4 );
                if ( pCut->nLeaves == 4 && !Abc_Tt4Check( (int)(0xFFFF & *If_CutTruth(p, pCut)) ) )
                    pCut->fUseless = 1;
            }
            else 
            {
                if ( p->pPars->fUseAndVars )
                {
                    int iDecMask = -1, truthId = Abc_Lit2Var(pCut->iCutFunc);
                    assert( p->pPars->nLutSize <= 13 );
                    if ( truthId >= Vec_IntSize(p->vTtDecs[pCut->nLeaves]) || Vec_IntEntry(p->vTtDecs[pCut->nLeaves], truthId) == -1 )
                    {
                        while ( truthId >= Vec_IntSize(p->vTtDecs[pCut->nLeaves]) )
                            Vec_IntPush( p->vTtDecs[pCut->nLeaves], -1 );
                        if ( (int)pCut->nLeaves > p->pPars->nLutSize / 2 && (int)pCut->nLeaves <= 2 * (p->pPars->nLutSize / 2) )
                            iDecMask = Abc_TtProcessBiDec( If_CutTruthWR(p, pCut), (int)pCut->nLeaves, p->pPars->nLutSize / 2 );
                        else
                            iDecMask = 0;
                        Vec_IntWriteEntry( p->vTtDecs[pCut->nLeaves], truthId, iDecMask );
                    }
                    iDecMask = Vec_IntEntry(p->vTtDecs[pCut->nLeaves], truthId);
                    assert( iDecMask >= 0 );
                    pCut->fUseless = (int)(iDecMask == 0 && (int)pCut->nLeaves > p->pPars->nLutSize / 2);
                    p->nCutsUselessAll += pCut->fUseless;
                    p->nCutsUseless[pCut->nLeaves] += pCut->fUseless;
                    p->nCutsCountAll++;
                    p->nCutsCount[pCut->nLeaves]++;
                }
                if ( p->pPars->fUseCofVars && (!p->pPars->fUseAndVars || pCut->fUseless) )
                {
                    int iCofVar = -1, truthId = Abc_Lit2Var(pCut->iCutFunc);
                    if ( truthId >= Vec_StrSize(p->vTtVars[pCut->nLeaves]) || Vec_StrEntry(p->vTtVars[pCut->nLeaves], truthId) == (char)-1 )
                    {
                        while ( truthId >= Vec_StrSize(p->vTtVars[pCut->nLeaves]) )
                            Vec_StrPush( p->vTtVars[pCut->nLeaves], (char)-1 );
                        iCofVar = Abc_TtCheckCondDep( If_CutTruthWR(p, pCut), pCut->nLeaves, p->pPars->nLutSize / 2 );
                        Vec_StrWriteEntry( p->vTtVars[pCut->nLeaves], truthId, (char)iCofVar );
                    }
                    iCofVar = Vec_StrEntry(p->vTtVars[pCut->nLeaves], truthId);
                    assert( iCofVar >= 0 && iCofVar <= (int)pCut->nLeaves );
                    pCut->fUseless = (int)(iCofVar == (int)pCut->nLeaves && pCut->nLeaves > 0);
                    p->nCutsUselessAll += pCut->fUseless;
                    p->nCutsUseless[pCut->nLeaves] += pCut->fUseless;
                    p->nCutsCountAll++;
                    p->nCutsCount[pCut->nLeaves]++;
                }
            }
        }
        
        // compute the application-specific cost and depth
        pCut->fUser = (p->pPars->pFuncCost != NULL);
        pCut->Cost = p->pPars->pFuncCost? p->pPars->pFuncCost(p, pCut) : 0;
        if ( pCut->Cost == IF_COST_MAX )
            continue;
        // check if the cut satisfies the required times
        if ( p->pPars->fDelayOpt )
            pCut->Delay = If_CutSopBalanceEval( p, pCut, NULL );
        else if ( p->pPars->fDsdBalance )
            pCut->Delay = If_CutDsdBalanceEval( p, pCut, NULL );
        else if ( p->pPars->fUserRecLib )
            pCut->Delay = If_CutDelayRecCost3( p, pCut, pObj ); 
        else if ( p->pPars->fUserSesLib )
        {
            int Cost = 0;
            pCut->fUser = 1;
            pCut->Delay = (float)Abc_ExactDelayCost( If_CutTruthW(p, pCut), If_CutLeaveNum(pCut), If_CutArrTimeProfile(p, pCut), If_CutPerm(pCut), &Cost, If_ManCutAigDelay(p, pObj, pCut) ); 
            if ( Cost == ABC_INFINITY )
            {
                for ( v = 0; v < If_CutLeaveNum(pCut); v++ )
                    If_CutPerm(pCut)[v] = IF_BIG_CHAR;
                pCut->Cost = IF_COST_MAX;
                pCut->fUseless = 1;
            }
        }
        else if ( p->pPars->fDelayOptLut )
            pCut->Delay = If_CutLutBalanceEval( p, pCut );
        else if( p->pPars->nGateSize > 0 )
            pCut->Delay = If_CutDelaySop( p, pCut );
        else 
            pCut->Delay = If_CutDelay( p, pObj, pCut );
        if ( pCut->Delay == -1 )
            continue;
        if ( Mode && pCut->Delay > pObj->Required + p->fEpsilon )
            continue;
        // compute area of the cut (this area may depend on the application specific cost)
        pCut->Area = (Mode == 2)? If_CutAreaDerefed( p, pCut ) : If_CutAreaFlow( p, pCut );
        if ( p->pPars->fEdge )
            pCut->Edge = (Mode == 2)? If_CutEdgeDerefed( p, pCut ) : If_CutEdgeFlow( p, pCut );
        if ( p->pPars->fPower )
            pCut->Power = (Mode == 2)? If_CutPowerDerefed( p, pCut, pObj ) : If_CutPowerFlow( p, pCut, pObj );
//        pCut->AveRefs = (Mode == 0)? (float)0.0 : If_CutAverageRefs( p, pCut );
        // insert the cut into storage
        If_CutSort( p, pCutSet, pCut );
//        If_CutTraverse( p, pObj, pCut );
    } 
    assert( pCutSet->nCuts > 0 );
//    If_CutVerifyCuts( pCutSet, !p->pPars->fUseTtPerm );

    // update the best cut
    if ( !fPreprocess || pCutSet->ppCuts[0]->Delay <= pObj->Required + p->fEpsilon )
    {
        If_CutCopy( p, If_ObjCutBest(pObj), pCutSet->ppCuts[0] );
        if ( p->pPars->fUserRecLib || p->pPars->fUserSesLib )
            assert(If_ObjCutBest(pObj)->Cost < IF_COST_MAX && If_ObjCutBest(pObj)->Delay < ABC_INFINITY);
    }
    // add the trivial cut to the set
    if ( !pObj->fSkipCut && If_ObjCutBest(pObj)->nLeaves > 1 )
    {
        If_ManSetupCutTriv( p, pCutSet->ppCuts[pCutSet->nCuts++], pObj->Id );
        assert( pCutSet->nCuts <= pCutSet->nCutsMax+1 );
    }
//    if ( If_ObjCutBest(pObj)->nLeaves == 0 )
//        p->nBestCutSmall[0]++;
//    else if ( If_ObjCutBest(pObj)->nLeaves == 1 )
//        p->nBestCutSmall[1]++;

    // ref the selected cut
    if ( Mode && pObj->nRefs > 0 )
        If_CutAreaRef( p, If_ObjCutBest(pObj) );
    if ( If_ObjCutBest(pObj)->fUseless )
        Abc_Print( 1, "The best cut is useless.\n" );
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
    int i, fSave0 = p->pPars->fDelayOpt || p->pPars->fDelayOptLut || p->pPars->fDsdBalance || p->pPars->fUserRecLib || p->pPars->fUserSesLib || p->pPars->fUse34Spec;
    assert( pObj->pEquiv != NULL );

    // prepare
    if ( Mode && pObj->nRefs > 0 )
        If_CutAreaDeref( p, If_ObjCutBest(pObj) );

    // remove elementary cuts
    for ( pTemp = pObj; pTemp; pTemp = pTemp->pEquiv )
        if ( pTemp != pObj || pTemp->pCutSet->nCuts > 1 )
            pTemp->pCutSet->nCuts--;

    // update the cutset of the node
    pCutSet = pObj->pCutSet;

    // generate cuts
    for ( pTemp = pObj->pEquiv; pTemp; pTemp = pTemp->pEquiv )
    {
        if ( pTemp->pCutSet->nCuts == 0 )
            continue;
        // go through the cuts of this node
        If_ObjForEachCut( pTemp, pCutTemp, i )
        {
            if ( pCutTemp->fUseless )
                continue;
            // get the next free cut
            assert( pCutSet->nCuts <= pCutSet->nCutsMax );
            pCut = pCutSet->ppCuts[pCutSet->nCuts];
            // copy the cut into storage
            If_CutCopy( p, pCut, pCutTemp );
            // check if this cut is contained in any of the available cuts
            if ( If_CutFilter( pCutSet, pCut, fSave0 ) )
                continue;
            // check if the cut satisfies the required times
//            assert( pCut->Delay == If_CutDelay( p, pTemp, pCut ) );
            if ( Mode && pCut->Delay > pObj->Required + p->fEpsilon )
                continue;
            // set the phase attribute
            pCut->fCompl = pObj->fPhase ^ pTemp->fPhase;
            // compute area of the cut (this area may depend on the application specific cost)
            pCut->Area = (Mode == 2)? If_CutAreaDerefed( p, pCut ) : If_CutAreaFlow( p, pCut );
            if ( p->pPars->fEdge )
                pCut->Edge = (Mode == 2)? If_CutEdgeDerefed( p, pCut ) : If_CutEdgeFlow( p, pCut );
            if ( p->pPars->fPower )
                pCut->Power = (Mode == 2)? If_CutPowerDerefed( p, pCut, pObj ) : If_CutPowerFlow( p, pCut, pObj );
//            pCut->AveRefs = (Mode == 0)? (float)0.0 : If_CutAverageRefs( p, pCut );
            // insert the cut into storage
            If_CutSort( p, pCutSet, pCut );
        }
    } 
    assert( pCutSet->nCuts > 0 );

    // update the best cut
    if ( !fPreprocess || pCutSet->ppCuts[0]->Delay <= pObj->Required + p->fEpsilon )
        If_CutCopy( p, If_ObjCutBest(pObj), pCutSet->ppCuts[0] );
    // add the trivial cut to the set
    if ( !pObj->fSkipCut && If_ObjCutBest(pObj)->nLeaves > 1 )
    {
        If_ManSetupCutTriv( p, pCutSet->ppCuts[pCutSet->nCuts++], pObj->Id );
        assert( pCutSet->nCuts <= pCutSet->nCutsMax+1 );
    }

    // ref the selected cut
    if ( Mode && pObj->nRefs > 0 )
        If_CutAreaRef( p, If_ObjCutBest(pObj) );
    // free the cuts
    If_ManDerefChoiceCutSet( p, pObj );
}

/**Function*************************************************************

  Synopsis    [Performs one mapping pass over all nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_ManPerformMappingRound( If_Man_t * p, int nCutsUsed, int Mode, int fPreprocess, int fFirst, char * pLabel )
{
    ProgressBar * pProgress = NULL;
    If_Obj_t * pObj;
    int i;
    abctime clk = Abc_Clock();
    float arrTime;
    assert( Mode >= 0 && Mode <= 2 );
    p->nBestCutSmall[0] = p->nBestCutSmall[1] = 0;
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
    // make sure the visit counters are all zero
    If_ManForEachNode( p, pObj, i )
        assert( pObj->nVisits == pObj->nVisitsCopy );
    // map the internal nodes
    if ( p->pManTim != NULL )
    {
        Tim_ManIncrementTravId( p->pManTim );
        If_ManForEachObj( p, pObj, i )
        {
            if ( If_ObjIsAnd(pObj) )
            {
                If_ObjPerformMappingAnd( p, pObj, Mode, fPreprocess, fFirst );
                if ( pObj->fRepr )
                    If_ObjPerformMappingChoice( p, pObj, Mode, fPreprocess );
            }
            else if ( If_ObjIsCi(pObj) )
            {
//Abc_Print( 1, "processing CI %d\n", pObj->Id );
                arrTime = Tim_ManGetCiArrival( p->pManTim, pObj->IdPio );
                If_ObjSetArrTime( pObj, arrTime );
            }
            else if ( If_ObjIsCo(pObj) )
            {
                arrTime = If_ObjArrTime( If_ObjFanin0(pObj) );
                Tim_ManSetCoArrival( p->pManTim, pObj->IdPio, arrTime );
            }
            else if ( If_ObjIsConst1(pObj) )
            {
                arrTime = -IF_INFINITY;
                If_ObjSetArrTime( pObj, arrTime );
            }
            else
                assert( 0 );
        }
//        Tim_ManPrint( p->pManTim );
    }
    else
    {
        pProgress = Extra_ProgressBarStart( stdout, If_ManObjNum(p) );
        If_ManForEachNode( p, pObj, i )
        {
            Extra_ProgressBarUpdate( pProgress, i, pLabel );
            If_ObjPerformMappingAnd( p, pObj, Mode, fPreprocess, fFirst );
            if ( pObj->fRepr )
                If_ObjPerformMappingChoice( p, pObj, Mode, fPreprocess );
        }
    }
    Extra_ProgressBarStop( pProgress );
    // make sure the visit counters are all zero
    If_ManForEachNode( p, pObj, i )
        assert( pObj->nVisits == 0 );
    // compute required times and stats
    If_ManComputeRequired( p );
//    Tim_ManPrint( p->pManTim );
    if ( p->pPars->fVerbose )
    {
        char Symb = fPreprocess? 'P' : ((Mode == 0)? 'D' : ((Mode == 1)? 'F' : 'A'));
        Abc_Print( 1, "%c:  Del = %7.2f.  Ar = %9.1f.  Edge = %8d.  ", 
            Symb, p->RequiredGlo, p->AreaGlo, p->nNets );
        if ( p->dPower )
        Abc_Print( 1, "Switch = %7.2f.  ", p->dPower );
        Abc_Print( 1, "Cut = %8d.  ", p->nCutsMerged );
        Abc_PrintTime( 1, "T", Abc_Clock() - clk );
//    Abc_Print( 1, "Max number of cuts = %d. Average number of cuts = %5.2f.\n", 
//        p->nCutsMax, 1.0 * p->nCutsMerged / If_ManAndNum(p) );
    }
    return 1;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

