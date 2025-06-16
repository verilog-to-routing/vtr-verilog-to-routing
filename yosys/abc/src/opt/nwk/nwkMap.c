/**CFile****************************************************************

  FileName    [nwkMap.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Logic network representation.]

  Synopsis    [Interface to technology mapping.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: nwkMap.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "nwk.h"
#include "map/if/if.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Load the network into FPGA manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManSetIfParsDefault( If_Par_t * pPars )
{
//    extern void * Abc_FrameReadLibLut();
    // set defaults
    memset( pPars, 0, sizeof(If_Par_t) );
    // user-controlable paramters
//    pPars->nLutSize    = -1;
    pPars->nLutSize    =  6;
    pPars->nCutsMax    =  8;
    pPars->nFlowIters  =  1;
    pPars->nAreaIters  =  2;
    pPars->DelayTarget = -1;
    pPars->Epsilon     =  (float)0.005;
    pPars->fPreprocess =  1;
    pPars->fArea       =  0;
    pPars->fFancy      =  0;
    pPars->fExpRed     =  1; ////
    pPars->fLatchPaths =  0;
    pPars->fEdge       =  1;
    pPars->fPower      =  0;
    pPars->fCutMin     =  0;
    pPars->fVerbose    =  0;
    // internal parameters
    pPars->fTruth      =  0;
    pPars->nLatchesCi  =  0;
    pPars->nLatchesCo  =  0;
    pPars->fLiftLeaves =  0;
//    pPars->pLutLib     =  Abc_FrameReadLibLut();
    pPars->pLutLib     =  NULL;
    pPars->pTimesArr   =  NULL; 
    pPars->pTimesArr   =  NULL;   
    pPars->pFuncCost   =  NULL;   
/*
    if ( pPars->nLutSize == -1 )
    {
        if ( pPars->pLutLib == NULL )
        {
            printf( "The LUT library is not given.\n" );
            return;
        }
        // get LUT size from the library
        pPars->nLutSize = pPars->pLutLib->LutMax;
    }
*/
}

/**Function*************************************************************

  Synopsis    [Load the network into FPGA manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_Man_t * Nwk_ManToIf( Aig_Man_t * p, If_Par_t * pPars, Vec_Ptr_t * vAigToIf )
{
    extern Vec_Int_t * Saig_ManComputeSwitchProbs( Aig_Man_t * p, int nFrames, int nPref, int fProbOne );
    Vec_Int_t * vSwitching = NULL, * vSwitching2 = NULL;
    float * pSwitching = NULL, * pSwitching2 = NULL;
    If_Man_t * pIfMan;
    If_Obj_t * pIfObj = NULL;
    Aig_Obj_t * pNode, * pFanin, * pPrev;
    int i;
    abctime clk = Abc_Clock();
    // set the number of registers (switch activity will be combinational)
    Aig_ManSetRegNum( p, 0 );
    if ( pPars->fPower )
    {
        vSwitching  = Saig_ManComputeSwitchProbs( p, 48, 16, 0 );
        if ( pPars->fVerbose )
        {
            ABC_PRT( "Computing switching activity", Abc_Clock() - clk );
        }
        pSwitching  = (float *)vSwitching->pArray;
        vSwitching2 = Vec_IntStart( Aig_ManObjNumMax(p) );
        pSwitching2 = (float *)vSwitching2->pArray;
    }
    // start the mapping manager and set its parameters
    pIfMan = If_ManStart( pPars );
    pIfMan->vSwitching = vSwitching2;
    // load the AIG into the mapper
    Aig_ManForEachObj( p, pNode, i )
    {
        if ( Aig_ObjIsAnd(pNode) )
        {
            pIfObj = If_ManCreateAnd( pIfMan, 
                If_NotCond( (If_Obj_t *)Aig_ObjFanin0(pNode)->pData, Aig_ObjFaninC0(pNode) ), 
                If_NotCond( (If_Obj_t *)Aig_ObjFanin1(pNode)->pData, Aig_ObjFaninC1(pNode) ) );
//            printf( "no%d=%d\n ", If_ObjId(pIfObj), If_ObjLevel(pIfObj) );
        }
        else if ( Aig_ObjIsCi(pNode) )
        {
            pIfObj = If_ManCreateCi( pIfMan );
            If_ObjSetLevel( pIfObj, Aig_ObjLevel(pNode) );
//            printf( "pi%d=%d\n ", If_ObjId(pIfObj), If_ObjLevel(pIfObj) );
            if ( pIfMan->nLevelMax < (int)pIfObj->Level )
                pIfMan->nLevelMax = (int)pIfObj->Level;
        }
        else if ( Aig_ObjIsCo(pNode) )
        {
            pIfObj = If_ManCreateCo( pIfMan, If_NotCond( (If_Obj_t *)Aig_ObjFanin0(pNode)->pData, Aig_ObjFaninC0(pNode) ) );
//            printf( "po%d=%d\n ", If_ObjId(pIfObj), If_ObjLevel(pIfObj) );
        }
        else if ( Aig_ObjIsConst1(pNode) )
            pIfObj = If_ManConst1( pIfMan );
        else // add the node to the mapper
            assert( 0 );
        // save the result
        assert( Vec_PtrEntry(vAigToIf, i) == NULL );
        Vec_PtrWriteEntry( vAigToIf, i, pIfObj );
        pNode->pData = pIfObj;
        if ( vSwitching2 )
            pSwitching2[pIfObj->Id] = pSwitching[pNode->Id];            
        // set up the choice node
        if ( Aig_ObjIsChoice( p, pNode ) )
        {
            for ( pPrev = pNode, pFanin = Aig_ObjEquiv(p, pNode); pFanin; pPrev = pFanin, pFanin = Aig_ObjEquiv(p, pFanin) )
                If_ObjSetChoice( (If_Obj_t *)pPrev->pData, (If_Obj_t *)pFanin->pData );
            If_ManCreateChoice( pIfMan, (If_Obj_t *)pNode->pData );
        }
//        assert( If_ObjLevel(pIfObj) == Aig_ObjLevel(pNode) );
    }
    if ( vSwitching )
        Vec_IntFree( vSwitching );
    return pIfMan;
}


/**Function*************************************************************

  Synopsis    [Recursively derives the local AIG for the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Nwk_NodeIfToHop2_rec( Hop_Man_t * pHopMan, If_Man_t * pIfMan, If_Obj_t * pIfObj, Vec_Ptr_t * vVisited )
{
    If_Cut_t * pCut;
    If_Obj_t * pTemp;
    Hop_Obj_t * gFunc, * gFunc0, * gFunc1;
    // get the best cut
    pCut = If_ObjCutBest(pIfObj);
    // if the cut is visited, return the result
    if ( If_CutData(pCut) )
        return (Hop_Obj_t *)If_CutData(pCut);
    // mark the node as visited
    Vec_PtrPush( vVisited, pCut );
    // insert the worst case
    If_CutSetData( pCut, (void *)1 );
    // skip in case of primary input
    if ( If_ObjIsCi(pIfObj) )
        return (Hop_Obj_t *)If_CutData(pCut);
    // compute the functions of the children
    for ( pTemp = pIfObj; pTemp; pTemp = pTemp->pEquiv )
    {
        gFunc0 = Nwk_NodeIfToHop2_rec( pHopMan, pIfMan, pTemp->pFanin0, vVisited );
        if ( gFunc0 == (void *)1 )
            continue;
        gFunc1 = Nwk_NodeIfToHop2_rec( pHopMan, pIfMan, pTemp->pFanin1, vVisited );
        if ( gFunc1 == (void *)1 )
            continue;
        // both branches are solved
        gFunc = Hop_And( pHopMan, Hop_NotCond(gFunc0, pTemp->fCompl0), Hop_NotCond(gFunc1, pTemp->fCompl1) ); 
        if ( pTemp->fPhase != pIfObj->fPhase )
            gFunc = Hop_Not(gFunc);
        If_CutSetData( pCut, gFunc );
        break;
    }
    return (Hop_Obj_t *)If_CutData(pCut);
}

/**Function*************************************************************

  Synopsis    [Derives the local AIG for the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Nwk_NodeIfToHop( Hop_Man_t * pHopMan, If_Man_t * pIfMan, If_Obj_t * pIfObj )
{
    If_Cut_t * pCut;
    Hop_Obj_t * gFunc;
    If_Obj_t * pLeaf;
    int i;
    // get the best cut
    pCut = If_ObjCutBest(pIfObj);
    assert( pCut->nLeaves > 1 );
    // set the leaf variables
    If_CutForEachLeaf( pIfMan, pCut, pLeaf, i )
        If_CutSetData( If_ObjCutBest(pLeaf), Hop_IthVar(pHopMan, i) );
    // recursively compute the function while collecting visited cuts
    Vec_PtrClear( pIfMan->vTemp );
    gFunc = Nwk_NodeIfToHop2_rec( pHopMan, pIfMan, pIfObj, pIfMan->vTemp ); 
    if ( gFunc == (void *)1 )
    {
        printf( "Nwk_NodeIfToHop(): Computing local AIG has failed.\n" );
        return NULL;
    }
//    printf( "%d ", Vec_PtrSize(p->vTemp) );
    // clean the cuts
    If_CutForEachLeaf( pIfMan, pCut, pLeaf, i )
        If_CutSetData( If_ObjCutBest(pLeaf), NULL );
    Vec_PtrForEachEntry( If_Cut_t *, pIfMan->vTemp, pCut, i )
        If_CutSetData( pCut, NULL );
    return gFunc;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Nwk_Man_t * Nwk_ManFromIf( If_Man_t * pIfMan, Aig_Man_t * p, Vec_Ptr_t * vAigToIf )
{
    Vec_Ptr_t * vIfToAig;
    Nwk_Man_t * pNtk;
    Nwk_Obj_t * pObjNew = NULL;
    Aig_Obj_t * pObj, * pObjRepr;
    If_Obj_t * pIfObj = NULL;
    If_Cut_t * pCutBest;
    int i, k, nLeaves, * ppLeaves;
    assert( Aig_ManCiNum(p) == If_ManCiNum(pIfMan) );
    assert( Aig_ManCoNum(p) == If_ManCoNum(pIfMan) );
    assert( Aig_ManNodeNum(p) == If_ManAndNum(pIfMan) );
    Aig_ManCleanData( p );
    If_ManCleanCutData( pIfMan );
    // create mapping of IF to AIG
    vIfToAig = Vec_PtrStart( If_ManObjNum(pIfMan) );
    Aig_ManForEachObj( p, pObj, i )
    {
        pIfObj = (If_Obj_t *)Vec_PtrEntry( vAigToIf, i );
        Vec_PtrWriteEntry( vIfToAig, pIfObj->Id, pObj );
    }
    // construct the network
    pNtk = Nwk_ManAlloc();
    pNtk->pName = Abc_UtilStrsav( p->pName );
    pNtk->pSpec = Abc_UtilStrsav( p->pSpec );
//    pNtk->nLatches = Aig_ManRegNum(p);
//    pNtk->nTruePis = Nwk_ManCiNum(pNtk) - pNtk->nLatches;
//    pNtk->nTruePos = Nwk_ManCoNum(pNtk) - pNtk->nLatches;
    Aig_ManForEachObj( p, pObj, i )
    {
        pIfObj = (If_Obj_t *)Vec_PtrEntry( vAigToIf, i );
        if ( pIfObj->nRefs == 0 && !If_ObjIsTerm(pIfObj) )
            continue;
        if ( Aig_ObjIsNode(pObj) )
        {
            pCutBest = If_ObjCutBest( pIfObj );
            nLeaves  = If_CutLeaveNum( pCutBest ); 
            ppLeaves = If_CutLeaves( pCutBest );
            // create node
            pObjNew = Nwk_ManCreateNode( pNtk, nLeaves, pIfObj->nRefs );
            for ( k = 0; k < nLeaves; k++ )
            {
                pObjRepr = (Aig_Obj_t *)Vec_PtrEntry( vIfToAig, ppLeaves[k] );
                Nwk_ObjAddFanin( pObjNew, (Nwk_Obj_t *)pObjRepr->pData );
            }
            // get the functionality
            pObjNew->pFunc = Nwk_NodeIfToHop( pNtk->pManHop, pIfMan, pIfObj );
        }
        else if ( Aig_ObjIsCi(pObj) )
            pObjNew = Nwk_ManCreateCi( pNtk, pIfObj->nRefs );
        else if ( Aig_ObjIsCo(pObj) )
        {
            pObjNew = Nwk_ManCreateCo( pNtk );
            pObjNew->fInvert = Aig_ObjFaninC0(pObj);
            Nwk_ObjAddFanin( pObjNew, (Nwk_Obj_t *)Aig_ObjFanin0(pObj)->pData );
//printf( "%d ", pObjNew->Id );
        }
        else if ( Aig_ObjIsConst1(pObj) )
        {
            pObjNew = Nwk_ManCreateNode( pNtk, 0, pIfObj->nRefs );
            pObjNew->pFunc = Hop_ManConst1( pNtk->pManHop );
        }
        else
            assert( 0 );
        pObj->pData = pObjNew;
    }
//printf( "\n" );
    Vec_PtrFree( vIfToAig );
    pNtk->pManTime = Tim_ManDup( pIfMan->pManTim, 0 );
    Nwk_ManMinimumBase( pNtk, 0 );
    assert( Nwk_ManCheck( pNtk ) );
    return pNtk;
}

/**Function*************************************************************

  Synopsis    [Interface with the FPGA mapping package.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Nwk_Man_t * Nwk_MappingIf( Aig_Man_t * p, Tim_Man_t * pManTime, If_Par_t * pPars )
{
    Nwk_Man_t * pNtk;
    If_Man_t * pIfMan;
    Vec_Ptr_t * vAigToIf;
    // set the arrival times
    pPars->pTimesArr = ABC_ALLOC( float, Aig_ManCiNum(p) );
    memset( pPars->pTimesArr, 0, sizeof(float) * Aig_ManCiNum(p) );
    // translate into the mapper
    vAigToIf = Vec_PtrStart( Aig_ManObjNumMax(p) );
    pIfMan = Nwk_ManToIf( p, pPars, vAigToIf );    
    if ( pIfMan == NULL )
        return NULL;
    pIfMan->pManTim = Tim_ManDup( pManTime, 0 );
    pIfMan->pPars->fCutMin = 0; // is not compatible with deriving result
    if ( !If_ManPerformMapping( pIfMan ) )
    {
        If_ManStop( pIfMan );
        return NULL;
    }
    // transform the result of mapping into the new network
    pNtk = Nwk_ManFromIf( pIfMan, p, vAigToIf );
    if ( pPars->fBidec && pPars->nLutSize <= 8 )
        Nwk_ManBidecResyn( pNtk, 0 );
    If_ManStop( pIfMan );
    Vec_PtrFree( vAigToIf );
    return pNtk;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

