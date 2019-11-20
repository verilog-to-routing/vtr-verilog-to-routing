/**CFile****************************************************************

  FileName    [ifMan.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [FPGA mapping based on priority cuts.]

  Synopsis    [Mapping manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 21, 2006.]

  Revision    [$Id: ifMan.c,v 1.00 2006/11/21 00:00:00 alanmi Exp $]

***********************************************************************/

#include "if.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static If_Obj_t * If_ManSetupObj( If_Man_t * p );

static void       If_ManCutSetRecycle( If_Man_t * p, If_Set_t * pSet ) { pSet->pNext = p->pFreeList; p->pFreeList = pSet;                            }
static If_Set_t * If_ManCutSetFetch( If_Man_t * p )                    { If_Set_t * pTemp = p->pFreeList; p->pFreeList = p->pFreeList->pNext; return pTemp; }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts the AIG manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_Man_t * If_ManStart( If_Par_t * pPars )
{
    If_Man_t * p;
    // start the manager
    p = ALLOC( If_Man_t, 1 );
    memset( p, 0, sizeof(If_Man_t) );
    p->pPars = pPars;
    p->fEpsilon = (float)0.001;
    // allocate arrays for nodes
    p->vCis    = Vec_PtrAlloc( 100 );
    p->vCos    = Vec_PtrAlloc( 100 );
    p->vObjs   = Vec_PtrAlloc( 100 );
    p->vMapped = Vec_PtrAlloc( 100 );
    p->vTemp   = Vec_PtrAlloc( 100 );
    // prepare the memory manager
    p->nTruthWords = p->pPars->fTruth? If_CutTruthWords( p->pPars->nLutSize ) : 0;
    p->nPermWords  = p->pPars->fUsePerm? If_CutPermWords( p->pPars->nLutSize ) : 0;
    p->nObjBytes   = sizeof(If_Obj_t) + sizeof(int) * (p->pPars->nLutSize + p->nPermWords + p->nTruthWords);
    p->nCutBytes   = sizeof(If_Cut_t) + sizeof(int) * (p->pPars->nLutSize + p->nPermWords + p->nTruthWords);
    p->nSetBytes   = sizeof(If_Set_t) + (sizeof(If_Cut_t *) + p->nCutBytes) * (p->pPars->nCutsMax + 1);
    p->pMemObj     = Mem_FixedStart( p->nObjBytes );
//    p->pMemSet     = Mem_FixedStart( p->nSetBytes );
    // report expected memory usage
    if ( p->pPars->fVerbose )
        printf( "Memory (bytes): Truth = %4d. Cut = %4d. Obj = %4d. Set = %4d.\n", 
            4 * p->nTruthWords, p->nCutBytes, p->nObjBytes, p->nSetBytes );
    // room for temporary truth tables
    p->puTemp[0] = p->pPars->fTruth? ALLOC( unsigned, 4 * p->nTruthWords ) : NULL;
    p->puTemp[1] = p->puTemp[0] + p->nTruthWords;
    p->puTemp[2] = p->puTemp[1] + p->nTruthWords;
    p->puTemp[3] = p->puTemp[2] + p->nTruthWords;
    // create the constant node
    p->pConst1 = If_ManSetupObj( p );
    p->pConst1->Type = IF_CONST1;
    p->pConst1->fPhase = 1;
    p->nObjs[IF_CONST1]++;
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManRestart( If_Man_t * p )
{
    FREE( p->pMemCi );
    Vec_PtrClear( p->vCis );
    Vec_PtrClear( p->vCos );
    Vec_PtrClear( p->vObjs );
    Vec_PtrClear( p->vMapped );
    Vec_PtrClear( p->vTemp );
    Mem_FixedRestart( p->pMemObj );
    // create the constant node
    p->pConst1 = If_ManSetupObj( p );
    p->pConst1->Type = IF_CONST1;
    p->pConst1->fPhase = 1;
    // reset the counter of other nodes
    p->nObjs[IF_CI] = p->nObjs[IF_CO] = p->nObjs[IF_AND] = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManStop( If_Man_t * p )
{
    Vec_PtrFree( p->vCis );
    Vec_PtrFree( p->vCos );
    Vec_PtrFree( p->vObjs );
    Vec_PtrFree( p->vMapped );
    Vec_PtrFree( p->vTemp );
    if ( p->vLatchOrder ) Vec_PtrFree( p->vLatchOrder );
    if ( p->vLags ) Vec_IntFree( p->vLags );
    Mem_FixedStop( p->pMemObj, 0 );
    FREE( p->pMemCi );
    FREE( p->pMemAnd );
    FREE( p->puTemp[0] );
    // free pars memory
    if ( p->pPars->pTimesArr )
        FREE( p->pPars->pTimesArr );
    if ( p->pPars->pTimesReq )
        FREE( p->pPars->pTimesReq );
    free( p );
}

/**Function*************************************************************

  Synopsis    [Creates primary input.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_Obj_t * If_ManCreateCi( If_Man_t * p )
{
    If_Obj_t * pObj;
    pObj = If_ManSetupObj( p );
    pObj->Type = IF_CI;
    Vec_PtrPush( p->vCis, pObj );
    p->nObjs[IF_CI]++;
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Creates primary output with the given driver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_Obj_t * If_ManCreateCo( If_Man_t * p, If_Obj_t * pDriver )
{
    If_Obj_t * pObj;
    pObj = If_ManSetupObj( p );
    Vec_PtrPush( p->vCos, pObj );
    pObj->Type = IF_CO;
    pObj->fCompl0 = If_IsComplement(pDriver); pDriver = If_Regular(pDriver);
    pObj->pFanin0 = pDriver; pDriver->nRefs++; 
    p->nObjs[IF_CO]++;
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Create the new node assuming it does not exist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_Obj_t * If_ManCreateAnd( If_Man_t * p, If_Obj_t * pFan0, If_Obj_t * pFan1 )
{
    If_Obj_t * pObj;
    // perform constant propagation
    if ( pFan0 == pFan1 )
        return pFan0;
    if ( pFan0 == If_Not(pFan1) )
        return If_Not(p->pConst1);
    if ( If_Regular(pFan0) == p->pConst1 )
        return pFan0 == p->pConst1 ? pFan1 : If_Not(p->pConst1);
    if ( If_Regular(pFan1) == p->pConst1 )
        return pFan1 == p->pConst1 ? pFan0 : If_Not(p->pConst1);
    // get memory for the new object
    pObj = If_ManSetupObj( p );
    pObj->Type    = IF_AND;
    pObj->fCompl0 = If_IsComplement(pFan0); pFan0 = If_Regular(pFan0);
    pObj->fCompl1 = If_IsComplement(pFan1); pFan1 = If_Regular(pFan1);
    pObj->pFanin0 = pFan0; pFan0->nRefs++; pFan0->nVisits++; pFan0->nVisitsCopy++;
    pObj->pFanin1 = pFan1; pFan1->nRefs++; pFan1->nVisits++; pFan1->nVisitsCopy++;
    pObj->fPhase  = (pObj->fCompl0 ^ pFan0->fPhase) & (pObj->fCompl1 ^ pFan1->fPhase);
    pObj->Level   = 1 + IF_MAX( pFan0->Level, pFan1->Level );
    if ( p->nLevelMax < (int)pObj->Level )
        p->nLevelMax = (int)pObj->Level;
    p->nObjs[IF_AND]++;
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Create the new node assuming it does not exist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_Obj_t * If_ManCreateXor( If_Man_t * p, If_Obj_t * pFan0, If_Obj_t * pFan1 )
{
    If_Obj_t * pRes1, * pRes2;
    pRes1 = If_ManCreateAnd( p, If_Not(pFan0), pFan1 );
    pRes2 = If_ManCreateAnd( p, pFan0, If_Not(pFan1) );
    return If_Not( If_ManCreateAnd( p, If_Not(pRes1), If_Not(pRes2) ) );
}

/**Function*************************************************************

  Synopsis    [Create the new node assuming it does not exist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_Obj_t * If_ManCreateMux( If_Man_t * p, If_Obj_t * pFan0, If_Obj_t * pFan1, If_Obj_t * pCtrl )
{
    If_Obj_t * pRes1, * pRes2;
    pRes1 = If_ManCreateAnd( p, pFan0, If_Not(pCtrl) );
    pRes2 = If_ManCreateAnd( p, pFan1, pCtrl );
    return If_Not( If_ManCreateAnd( p, If_Not(pRes1), If_Not(pRes2) ) );
}

/**Function*************************************************************

  Synopsis    [Creates the choice node.]

  Description [Should be called after the equivalence class nodes are linked.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManCreateChoice( If_Man_t * p, If_Obj_t * pObj )
{
    If_Obj_t * pTemp;
    // mark the node as a representative if its class
    assert( pObj->fRepr == 0 );
    pObj->fRepr = 1;
    // update the level of this node (needed for correct required time computation)
    for ( pTemp = pObj; pTemp; pTemp = pTemp->pEquiv )
    {
        pObj->Level = IF_MAX( pObj->Level, pTemp->Level );
        pTemp->nVisits++; pTemp->nVisitsCopy++;
    }
    // mark the largest level
    if ( p->nLevelMax < (int)pObj->Level )
        p->nLevelMax = (int)pObj->Level;
}

/**Function*************************************************************

  Synopsis    [Prepares memory for one cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManSetupCut( If_Man_t * p, If_Cut_t * pCut )
{
    memset( pCut, 0, sizeof(If_Cut_t) );
    pCut->nLimit  = p->pPars->nLutSize;
    pCut->pLeaves = (int *)(pCut + 1);
    if ( p->pPars->fUsePerm )
        pCut->pPerm  = (char *)(pCut->pLeaves + p->pPars->nLutSize);
    if ( p->pPars->fTruth )
        pCut->pTruth = pCut->pLeaves + p->pPars->nLutSize + p->nPermWords;
}

/**Function*************************************************************

  Synopsis    [Prepares memory for one cutset.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManSetupSet( If_Man_t * p, If_Set_t * pSet )
{
    char * pArray;
    int i;
    pSet->nCuts = 0;
    pSet->nCutsMax = p->pPars->nCutsMax;
    pSet->ppCuts = (If_Cut_t **)(pSet + 1);
    pArray = (char *)pSet->ppCuts + sizeof(If_Cut_t *) * (pSet->nCutsMax+1);
    for ( i = 0; i <= pSet->nCutsMax; i++ )
    {
        pSet->ppCuts[i] = (If_Cut_t *)(pArray + i * p->nCutBytes); 
        If_ManSetupCut( p, pSet->ppCuts[i] );
    }
//    pArray += (pSet->nCutsMax + 1) * p->nCutBytes;
//    assert( ((char *)pArray) - ((char *)pSet) == p->nSetBytes );
}

/**Function*************************************************************

  Synopsis    [Prepares memory for one cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManSetupCutTriv( If_Man_t * p, If_Cut_t * pCut, int ObjId )
{
    pCut->fCompl     = 0;
    pCut->nLimit     = p->pPars->nLutSize;
    pCut->nLeaves    = 1;
    pCut->pLeaves[0] = p->pPars->fLiftLeaves? (ObjId << 8) : ObjId;
    pCut->uSign      = If_ObjCutSign( pCut->pLeaves[0] );
    // set up elementary truth table of the unit cut
    if ( p->pPars->fTruth )
    {
        int i, nTruthWords;
        nTruthWords = Extra_TruthWordNum( pCut->nLimit );
        for ( i = 0; i < nTruthWords; i++ )
            If_CutTruth(pCut)[i] = 0xAAAAAAAA;
    }
}

/**Function*************************************************************

  Synopsis    [Prepares memory for the node with cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_Obj_t * If_ManSetupObj( If_Man_t * p )
{
    If_Obj_t * pObj;
    // get memory for the object
    pObj = (If_Obj_t *)Mem_FixedEntryFetch( p->pMemObj );
    memset( pObj, 0, sizeof(If_Obj_t) );
    If_ManSetupCut( p, &pObj->CutBest );
    // assign ID and save 
    pObj->Id = Vec_PtrSize(p->vObjs);
    Vec_PtrPush( p->vObjs, pObj );
    // set the required times
    pObj->Required = IF_FLOAT_LARGE;
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Prepares memory for one cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManSetupCiCutSets( If_Man_t * p )
{
    If_Obj_t * pObj;
    int i;
    assert( p->pMemCi == NULL );
    // create elementary cuts for the CIs
    If_ManForEachCi( p, pObj, i )
        If_ManSetupCutTriv( p, &pObj->CutBest, pObj->Id );
    // create elementary cutsets for the CIs
    p->pMemCi = (If_Set_t *)malloc( If_ManCiNum(p) * (sizeof(If_Set_t) + sizeof(void *)) );
    If_ManForEachCi( p, pObj, i )
    {
        pObj->pCutSet = (If_Set_t *)((char *)p->pMemCi + i * (sizeof(If_Set_t) + sizeof(void *)));
        pObj->pCutSet->nCuts = 1;
        pObj->pCutSet->nCutsMax = p->pPars->nCutsMax;
        pObj->pCutSet->ppCuts = (If_Cut_t **)(pObj->pCutSet + 1);
        pObj->pCutSet->ppCuts[0] = &pObj->CutBest;
    }
}

/**Function*************************************************************

  Synopsis    [Prepares cutset of the node.]

  Description [Elementary cutset will be added last.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_Set_t * If_ManSetupNodeCutSet( If_Man_t * p, If_Obj_t * pObj )
{
    assert( If_ObjIsAnd(pObj) );
    assert( pObj->pCutSet == NULL );
//    pObj->pCutSet = (If_Set_t *)Mem_FixedEntryFetch( p->pMemSet );
//    If_ManSetupSet( p, pObj->pCutSet );

    pObj->pCutSet = If_ManCutSetFetch( p );
    pObj->pCutSet->nCuts = 0;
    pObj->pCutSet->nCutsMax = p->pPars->nCutsMax;

    return pObj->pCutSet;
}

/**Function*************************************************************

  Synopsis    [Dereferences cutset of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManDerefNodeCutSet( If_Man_t * p, If_Obj_t * pObj )
{
    If_Obj_t * pFanin;
    assert( If_ObjIsAnd(pObj) );
    // consider the node
    assert( pObj->nVisits >= 0 );
    if ( pObj->nVisits == 0 )
    {
//        Mem_FixedEntryRecycle( p->pMemSet, (char *)pObj->pCutSet );
        If_ManCutSetRecycle( p, pObj->pCutSet );
        pObj->pCutSet = NULL;
    }
    // consider the first fanin
    pFanin = If_ObjFanin0(pObj);
    assert( pFanin->nVisits > 0 );
    if ( !If_ObjIsCi(pFanin) && --pFanin->nVisits == 0 )
    {
//        Mem_FixedEntryRecycle( p->pMemSet, (char *)pFanin->pCutSet );
        If_ManCutSetRecycle( p, pFanin->pCutSet );
        pFanin->pCutSet = NULL;
    }
    // consider the second fanin
    pFanin = If_ObjFanin1(pObj);
    assert( pFanin->nVisits > 0 );
    if ( !If_ObjIsCi(pFanin) && --pFanin->nVisits == 0 )
    {
//        Mem_FixedEntryRecycle( p->pMemSet, (char *)pFanin->pCutSet );
        If_ManCutSetRecycle( p, pFanin->pCutSet );
        pFanin->pCutSet = NULL;
    }
}

/**Function*************************************************************

  Synopsis    [Dereferences cutset of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManDerefChoiceCutSet( If_Man_t * p, If_Obj_t * pObj )
{
    If_Obj_t * pTemp;
    assert( If_ObjIsAnd(pObj) );
    assert( pObj->fRepr );
    assert( pObj->nVisits > 0 );
    // consider the nodes in the choice class
    for ( pTemp = pObj; pTemp; pTemp = pTemp->pEquiv )
    {
        assert( pTemp == pObj || pTemp->nVisits == 1 );
        if ( --pTemp->nVisits == 0 )
        {
//            Mem_FixedEntryRecycle( p->pMemSet, (char *)pTemp->pCutSet );
            If_ManCutSetRecycle( p, pTemp->pCutSet );
            pTemp->pCutSet = NULL;
        }
    }
}

/**Function*************************************************************

  Synopsis    [Dereferences cutset of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManSetupSetAll( If_Man_t * p, int nCrossCut )
{
    If_Set_t * pCutSet;
    int i, nCutSets;
    nCutSets = 128 + nCrossCut;
    p->pFreeList = p->pMemAnd = pCutSet = (If_Set_t *)malloc( nCutSets * p->nSetBytes );
    for ( i = 0; i < nCutSets; i++ )
    {
        If_ManSetupSet( p, pCutSet );
        if ( i == nCutSets - 1 )
            pCutSet->pNext = NULL;
        else
            pCutSet->pNext = (If_Set_t *)( (char *)pCutSet + p->nSetBytes );
        pCutSet = pCutSet->pNext;
    }
    assert( pCutSet == NULL );

    if ( p->pPars->fVerbose )
    {
        printf( "Node = %7d.  Ch = %5d.  Total mem = %7.2f Mb. Peak cut mem = %7.2f Mb.\n", 
            If_ManAndNum(p), p->nChoices,
            1.0 * (p->nObjBytes + 2*sizeof(void *)) * If_ManObjNum(p) / (1<<20), 
            1.0 * p->nSetBytes * nCrossCut / (1<<20) );
    }
//    printf( "Cross cut = %d.\n", nCrossCut );

}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


