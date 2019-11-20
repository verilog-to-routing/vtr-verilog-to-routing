/**CFile****************************************************************

  FileName    [seqFpgaIter.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Construction and manipulation of sequential AIGs.]

  Synopsis    [Iterative delay computation in FPGA mapping/retiming package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: seqFpgaIter.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "seqInt.h"
#include "main.h"
#include "fpga.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void        Seq_FpgaMappingCollectNode_rec( Abc_Obj_t * pAnd, Vec_Ptr_t * vMapping, Vec_Vec_t * vMapCuts );
static Cut_Cut_t * Seq_FpgaMappingSelectCut( Abc_Obj_t * pAnd );

extern Cut_Man_t * Abc_NtkSeqCuts( Abc_Ntk_t * pNtk, Cut_Params_t * pParams );
extern Cut_Man_t * Abc_NtkCuts( Abc_Ntk_t * pNtk, Cut_Params_t * pParams );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the retiming lags for FPGA mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Seq_FpgaMappingDelays( Abc_Ntk_t * pNtk, int fVerbose )
{
    Abc_Seq_t * p = pNtk->pManFunc;
    Cut_Params_t Params, * pParams = &Params;
    Abc_Obj_t * pObj;
    int i, clk;

    // set defaults for cut computation
    memset( pParams, 0, sizeof(Cut_Params_t) );
    pParams->nVarsMax  = p->nVarsMax;  // the max cut size ("k" of the k-feasible cuts)
    pParams->nKeepMax  = 1000;  // the max number of cuts kept at a node
    pParams->fTruth    = 0;     // compute truth tables
    pParams->fFilter   = 1;     // filter dominated cuts
    pParams->fSeq      = 1;     // compute sequential cuts
    pParams->fVerbose  = fVerbose;     // the verbosiness flag

    // compute the cuts
clk = clock();
    p->pCutMan = Abc_NtkSeqCuts( pNtk, pParams );
//    pParams->fSeq = 0;
//    p->pCutMan = Abc_NtkCuts( pNtk, pParams );
p->timeCuts = clock() - clk;

    if ( fVerbose )
    Cut_ManPrintStats( p->pCutMan );

    // compute area flows
//    Seq_MapComputeAreaFlows( pNtk, fVerbose );

    // compute the delays
clk = clock();
    if ( !Seq_AigRetimeDelayLags( pNtk, fVerbose ) )
        return 0;
    p->timeDelay = clock() - clk;

    // collect the nodes and cuts used in the mapping
    p->vMapAnds = Vec_PtrAlloc( 1000 );
    p->vMapCuts = Vec_VecAlloc( 1000 );
    Abc_NtkIncrementTravId( pNtk );
    Abc_NtkForEachPo( pNtk, pObj, i )
        Seq_FpgaMappingCollectNode_rec( Abc_ObjFanin0(pObj), p->vMapAnds, p->vMapCuts );

    if ( fVerbose )
    printf( "The number of LUTs = %d.\n", Vec_PtrSize(p->vMapAnds) );

    // remove the cuts
    Cut_ManStop( p->pCutMan );
    p->pCutMan = NULL;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Derives the parameters of the best mapping/retiming for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seq_FpgaMappingCollectNode_rec( Abc_Obj_t * pAnd, Vec_Ptr_t * vMapping, Vec_Vec_t * vMapCuts )
{
    Abc_Obj_t * pFanin;
    Cut_Cut_t * pCutBest;
    int k;

    // skip if this is a non-PI node
    if ( !Abc_AigNodeIsAnd(pAnd) )
        return;
    // skip a visited node
    if ( Abc_NodeIsTravIdCurrent(pAnd) )
        return;
    Abc_NodeSetTravIdCurrent(pAnd);

    // visit the fanins of the node
    pCutBest = Seq_FpgaMappingSelectCut( pAnd );
    for ( k = 0; k < (int)pCutBest->nLeaves; k++ )
    {
        pFanin = Abc_NtkObj( pAnd->pNtk, pCutBest->pLeaves[k] >> 8 );
        Seq_FpgaMappingCollectNode_rec( pFanin, vMapping, vMapCuts );
    }

    // add this node
    Vec_PtrPush( vMapping, pAnd );
    for ( k = 0; k < (int)pCutBest->nLeaves; k++ )
        Vec_VecPush( vMapCuts, Vec_PtrSize(vMapping)-1, (void *)pCutBest->pLeaves[k] );
}

/**Function*************************************************************

  Synopsis    [Selects the best cut to represent the node in the mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cut_Cut_t * Seq_FpgaMappingSelectCut( Abc_Obj_t * pAnd )
{
    Abc_Obj_t * pFanin;
    Cut_Cut_t * pCut, * pCutBest, * pList;
    float CostCur, CostMin = ABC_INFINITY;
    int ArrivalCut, ArrivalMin, i;
    // get the arrival time of the best non-trivial cut
    ArrivalMin = Seq_NodeGetLValue( pAnd );
    // iterate through the cuts and select the one with the minimum cost
    pList = Abc_NodeReadCuts( Seq_NodeCutMan(pAnd), pAnd );
    CostMin = ABC_INFINITY;
    pCutBest = NULL;
    for ( pCut = pList->pNext; pCut; pCut = pCut->pNext )
    {
        ArrivalCut = *((int *)&pCut->uSign);
//        assert( ArrivalCut >= ArrivalMin );
        if ( ArrivalCut > ArrivalMin )
            continue;
        CostCur = 0.0;
        for ( i = 0; i < (int)pCut->nLeaves; i++ )
        {
            pFanin = Abc_NtkObj( pAnd->pNtk, pCut->pLeaves[i] >> 8 );
            if ( Abc_ObjIsPi(pFanin) )
                continue;
            if ( Abc_NodeIsTravIdCurrent(pFanin) )
                continue;
            CostCur += (float)(1.0 / Abc_ObjFanoutNum(pFanin));
//            CostCur += Seq_NodeGetFlow( pFanin );
        }
        if ( CostMin > CostCur )
        {
            CostMin = CostCur;
            pCutBest = pCut;
        }
    }
    assert( pCutBest != NULL );
    return pCutBest;
}


/**Function*************************************************************

  Synopsis    [Computes the l-value of the cut.]

  Description [The node should be internal.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Seq_FpgaCutUpdateLValue( Cut_Cut_t * pCut, Abc_Obj_t * pObj, int Fi )
{
    Abc_Obj_t * pFanin;
    int i, lValueMax, lValueCur;
    assert( Abc_AigNodeIsAnd(pObj) );
    lValueMax = -ABC_INFINITY;
    for ( i = 0; i < (int)pCut->nLeaves; i++ )
    {
//        lValue0 = Seq_NodeGetLValue(Abc_ObjFanin0(pObj)) - Fi * Abc_ObjFaninL0(pObj);
        pFanin    = Abc_NtkObj(pObj->pNtk, pCut->pLeaves[i] >> 8);
        lValueCur = Seq_NodeGetLValue(pFanin) - Fi * (pCut->pLeaves[i] & 255);
        if ( lValueMax < lValueCur )
            lValueMax = lValueCur;
    }
    lValueMax += 1;
    *((int *)&pCut->uSign) = lValueMax;
    return lValueMax;
}

/**Function*************************************************************

  Synopsis    [Computes the l-value of the node.]

  Description [The node can be internal or a PO.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Seq_FpgaNodeUpdateLValue( Abc_Obj_t * pObj, int Fi )
{
    Cut_Cut_t * pCut, * pList;
    int lValueNew, lValueOld, lValueCut;
    assert( !Abc_ObjIsPi(pObj) );
    assert( Abc_ObjFaninNum(pObj) > 0 );
    if ( Abc_ObjIsPo(pObj) )
    {
        lValueNew = Seq_NodeGetLValue(Abc_ObjFanin0(pObj)) - Fi * Seq_ObjFaninL0(pObj);
        return (lValueNew > Fi)? SEQ_UPDATE_FAIL : SEQ_UPDATE_NO;
    }
    // get the arrival time of the best non-trivial cut
    pList = Abc_NodeReadCuts( Seq_NodeCutMan(pObj), pObj );
    // skip the choice nodes
    if ( pList == NULL )
        return SEQ_UPDATE_NO;
    lValueNew = ABC_INFINITY;
    for ( pCut = pList->pNext; pCut; pCut = pCut->pNext )
    {
        lValueCut = Seq_FpgaCutUpdateLValue( pCut, pObj, Fi );
        if ( lValueNew > lValueCut )
            lValueNew = lValueCut;
    }
    // compare the arrival time with the previous arrival time
    lValueOld = Seq_NodeGetLValue(pObj);
//    if ( lValueNew == lValueOld )
    if ( lValueNew <= lValueOld )
        return SEQ_UPDATE_NO;
    Seq_NodeSetLValue( pObj, lValueNew );
//printf( "%d -> %d   ", lValueOld, lValueNew );
    return SEQ_UPDATE_YES;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


