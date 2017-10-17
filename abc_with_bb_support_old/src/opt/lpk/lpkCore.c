/**CFile****************************************************************

  FileName    [lpkCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Fast Boolean matching for LUT structures.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: lpkCore.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/
 
#include "lpkInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Prepares the mapping manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lpk_IfManStart( Lpk_Man_t * p )
{
    If_Par_t * pPars;
    assert( p->pIfMan == NULL );
    // set defaults
    pPars = ALLOC( If_Par_t, 1 );
    memset( pPars, 0, sizeof(If_Par_t) );
    // user-controlable paramters
    pPars->nLutSize    =  p->pPars->nLutSize;
    pPars->nCutsMax    = 16;
    pPars->nFlowIters  =  0; // 1
    pPars->nAreaIters  =  0; // 1 
    pPars->DelayTarget = -1;
    pPars->fPreprocess =  0;
    pPars->fArea       =  1;
    pPars->fFancy      =  0;
    pPars->fExpRed     =  0; //
    pPars->fLatchPaths =  0;
    pPars->fSeqMap     =  0;
    pPars->fVerbose    =  0;
    // internal parameters
    pPars->fTruth      =  1;
    pPars->fUsePerm    =  0; 
    pPars->nLatches    =  0;
    pPars->pLutLib     =  NULL; // Abc_FrameReadLibLut();
    pPars->pTimesArr   =  NULL; 
    pPars->pTimesArr   =  NULL;   
    pPars->fUseBdds    =  0;
    pPars->fUseSops    =  0;
    pPars->fUseCnfs    =  0;
    pPars->fUseMv      =  0;
    // start the mapping manager and set its parameters
    p->pIfMan = If_ManStart( pPars );
    If_ManSetupSetAll( p->pIfMan, 1000 );
    p->pIfMan->pPars->pTimesArr = ALLOC( float, 32 );
}

/**Function*************************************************************

  Synopsis    [Returns 1 if at least one entry has changed.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lpk_NodeHasChanged( Lpk_Man_t * p, int iNode )
{
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pTemp;
    int i;
    vNodes = Vec_VecEntry( p->vVisited, iNode );
    if ( Vec_PtrSize(vNodes) == 0 )
        return 1;
    Vec_PtrForEachEntry( vNodes, pTemp, i )
    {
        // check if the node has changed
        pTemp = Abc_NtkObj( p->pNtk, (int)pTemp );
        if ( pTemp == NULL )
            return 1;
        // check if the number of fanouts has changed
//        if ( Abc_ObjFanoutNum(pTemp) != (int)Vec_PtrEntry(vNodes, i+1) )
//            return 1;
        i++;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Prepares the mapping manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lpk_ExploreCut( Lpk_Man_t * p, Lpk_Cut_t * pCut, Kit_DsdNtk_t * pNtk )
{
    extern Abc_Obj_t * Abc_NodeFromIf_rec( Abc_Ntk_t * pNtkNew, If_Man_t * pIfMan, If_Obj_t * pIfObj, Vec_Int_t * vCover );
    Kit_DsdObj_t * pRoot;
    If_Obj_t * pDriver, * ppLeaves[16];
    Abc_Obj_t * pLeaf, * pObjNew;
    int nGain, i, clk;
//    int nOldShared;

    // check special cases
    pRoot = Kit_DsdNtkRoot( pNtk );
    if ( pRoot->Type == KIT_DSD_CONST1 )
    {
        if ( Kit_DsdLitIsCompl(pNtk->Root) )
            pObjNew = Abc_NtkCreateNodeConst0( p->pNtk );
        else
            pObjNew = Abc_NtkCreateNodeConst1( p->pNtk );
        Abc_NtkUpdate( p->pObj, pObjNew, p->vLevels );
        p->nGainTotal += pCut->nNodes - pCut->nNodesDup;
        return 1;
    }
    if ( pRoot->Type == KIT_DSD_VAR )
    {
        pObjNew = Abc_NtkObj( p->pNtk, pCut->pLeaves[ Kit_DsdLit2Var(pRoot->pFans[0]) ] );
        if ( Kit_DsdLitIsCompl(pNtk->Root) ^ Kit_DsdLitIsCompl(pRoot->pFans[0]) )
            pObjNew = Abc_NtkCreateNodeInv( p->pNtk, pObjNew );
        Abc_NtkUpdate( p->pObj, pObjNew, p->vLevels );
        p->nGainTotal += pCut->nNodes - pCut->nNodesDup;
        return 1;
    }
    assert( pRoot->Type == KIT_DSD_AND || pRoot->Type == KIT_DSD_XOR || pRoot->Type == KIT_DSD_PRIME );

    // start the mapping manager
    if ( p->pIfMan == NULL )
        Lpk_IfManStart( p );

    // prepare the mapping manager
    If_ManRestart( p->pIfMan );
    // create the PI variables
    for ( i = 0; i < p->pPars->nVarsMax; i++ )
        ppLeaves[i] = If_ManCreateCi( p->pIfMan );
    // set the arrival times
    Lpk_CutForEachLeaf( p->pNtk, pCut, pLeaf, i )
        p->pIfMan->pPars->pTimesArr[i] = (float)pLeaf->Level;
    // prepare the PI cuts
    If_ManSetupCiCutSets( p->pIfMan );
    // create the internal nodes
    p->fCalledOnce = 0;
    p->nCalledSRed = 0;
    pDriver = Lpk_MapTree_rec( p, pNtk, ppLeaves, pNtk->Root, NULL );
    if ( pDriver == NULL )
        return 0;
    // create the PO node
    If_ManCreateCo( p->pIfMan, If_Regular(pDriver) );

    // perform mapping
    p->pIfMan->pPars->fAreaOnly = 1;
clk = clock();
    If_ManPerformMappingComb( p->pIfMan );
p->timeMap += clock() - clk;

    // compute the gain in area
    nGain = pCut->nNodes - pCut->nNodesDup - (int)p->pIfMan->AreaGlo;
    if ( p->pPars->fVeryVerbose )
        printf( "       Mffc = %2d. Mapped = %2d. Gain = %3d. Depth increase = %d. SReds = %d.\n", 
            pCut->nNodes - pCut->nNodesDup, (int)p->pIfMan->AreaGlo, nGain, (int)p->pIfMan->RequiredGlo - (int)p->pObj->Level, p->nCalledSRed );

    // quit if there is no gain
    if ( !(nGain > 0 || (p->pPars->fZeroCost && nGain == 0)) )
        return 0;

    // quit if depth increases too much
    if ( (int)p->pIfMan->RequiredGlo > Abc_ObjRequiredLevel(p->pObj) )
        return 0;

    // perform replacement
    p->nGainTotal += nGain;
    p->nChanges++;
    if ( p->nCalledSRed )
        p->nBenefited++;

    // prepare the mapping manager
    If_ManCleanNodeCopy( p->pIfMan );
    If_ManCleanCutData( p->pIfMan );
    // set the PIs of the cut
    Lpk_CutForEachLeaf( p->pNtk, pCut, pLeaf, i )
        If_ObjSetCopy( If_ManCi(p->pIfMan, i), pLeaf );
    // get the area of mapping
    pObjNew = Abc_NodeFromIf_rec( p->pNtk, p->pIfMan, If_Regular(pDriver), p->vCover );
    pObjNew->pData = Hop_NotCond( pObjNew->pData, If_IsComplement(pDriver) );
    // perform replacement
    Abc_NtkUpdate( p->pObj, pObjNew, p->vLevels );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Performs resynthesis for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lpk_ResynthesizeNode( Lpk_Man_t * p )
{
    static int Count = 0;
    Kit_DsdNtk_t * pDsdNtk;
    Lpk_Cut_t * pCut;
    unsigned * pTruth;
    void * pDsd = NULL;
    int i, nSuppSize, RetValue, clk;

    // compute the cuts
clk = clock();
    if ( !Lpk_NodeCuts( p ) )
    {
p->timeCuts += clock() - clk;
        return 0;
    }
p->timeCuts += clock() - clk;

//return 0;

    if ( p->pPars->fVeryVerbose )
    printf( "Node %5d : Mffc size = %5d. Cuts = %5d.\n", p->pObj->Id, p->nMffc, p->nEvals );
    // try the good cuts
    p->nCutsTotal  += p->nCuts;
    p->nCutsUseful += p->nEvals;
    for ( i = 0; i < p->nEvals; i++ )
    {
        // get the cut
        pCut = p->pCuts + p->pEvals[i];
        if ( p->pPars->fFirst && i == 1 )
            break;

        if ( p->pObj->Id == 8835 )
        {
            int x = 0;
        }

        // compute the truth table
clk = clock();
        pTruth = Lpk_CutTruth( p, pCut );
        nSuppSize = Extra_TruthSupportSize(pTruth, pCut->nLeaves);
p->timeTruth += clock() - clk;

        pDsdNtk = Kit_DsdDecompose( pTruth, pCut->nLeaves ); 
//        Kit_DsdVerify( pDsdNtk, pTruth, pCut->nLeaves ); 
        // skip 16-input non-DSD because ISOP will not work
        if ( Kit_DsdNtkRoot(pDsdNtk)->nFans == 16 ) 
        {
            Kit_DsdNtkFree( pDsdNtk );
            continue;
        }

        // if DSD has nodes that require splitting to fit them into LUTs
        // we can skip those cuts that cannot lead to improvement
        // (a full DSD network requires  V = Nmin * (K-1) + 1 for improvement)
        if ( Kit_DsdNonDsdSizeMax(pDsdNtk) > p->pPars->nLutSize && 
             nSuppSize >= ((int)pCut->nNodes - (int)pCut->nNodesDup - 1) * (p->pPars->nLutSize - 1) + 1 )
        {
            Kit_DsdNtkFree( pDsdNtk );
            continue;
        }

        if ( p->pPars->fVeryVerbose )
        {
//            char * pFileName;
            printf( "  C%02d: L= %2d/%2d  V= %2d/%d  N= %d  W= %4.2f  ", 
                i, pCut->nLeaves, nSuppSize, pCut->nNodes, pCut->nNodesDup, pCut->nLuts, pCut->Weight );
            Kit_DsdPrint( stdout, pDsdNtk );
//            Kit_DsdPrintFromTruth( pTruth, pCut->nLeaves );
//            pFileName = Kit_TruthDumpToFile( pTruth, pCut->nLeaves, Count++ );
//            printf( "Saved truth table in file \"%s\".\n", pFileName );
        }

        // update the network
clk = clock();
        RetValue = Lpk_ExploreCut( p, pCut, pDsdNtk );
p->timeEval += clock() - clk;
        Kit_DsdNtkFree( pDsdNtk );
        if ( RetValue )
            break;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Performs resynthesis for one network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lpk_Resynthesize( Abc_Ntk_t * pNtk, Lpk_Par_t * pPars )
{
    ProgressBar * pProgress;
    Lpk_Man_t * p;
    Abc_Obj_t * pObj;
    double Delta;
    int i, Iter, nNodes, nNodesPrev, clk = clock();
    assert( Abc_NtkIsLogic(pNtk) );

    // get the number of inputs
    pPars->nLutSize = Abc_NtkGetFaninMax( pNtk );
    // adjust the number of crossbars based on LUT size
    if ( pPars->nVarsShared > pPars->nLutSize - 2 )
        pPars->nVarsShared = pPars->nLutSize - 2;
    // get the max number of LUTs tried
    pPars->nVarsMax = pPars->nLutsMax * (pPars->nLutSize - 1) + 1; // V = N * (K-1) + 1
    while ( pPars->nVarsMax > 16 )
    {
        pPars->nLutsMax--;
        pPars->nVarsMax = pPars->nLutsMax * (pPars->nLutSize - 1) + 1;

    }
    if ( pPars->fVerbose )
    {
        printf( "Resynthesis for %d %d-LUTs with %d non-MFFC LUTs, %d crossbars, and %d-input cuts.\n",
            pPars->nLutsMax, pPars->nLutSize, pPars->nLutsOver, pPars->nVarsShared, pPars->nVarsMax );
    }
 

    // convert into the AIG
    if ( !Abc_NtkToAig(pNtk) )
    {
        fprintf( stdout, "Converting to BDD has failed.\n" );
        return 0;
    }
    assert( Abc_NtkHasAig(pNtk) );

    // set the number of levels
    Abc_NtkLevel( pNtk );
    Abc_NtkStartReverseLevels( pNtk, pPars->nGrowthLevel );

    // start the manager
    p = Lpk_ManStart( pPars );
    p->pNtk = pNtk;
    p->nNodesTotal = Abc_NtkNodeNum(pNtk);
    p->vLevels = Vec_VecStart( pNtk->LevelMax ); 
    if ( p->pPars->fSatur )
        p->vVisited = Vec_VecStart( 0 );
    if ( pPars->fVerbose )
        p->nTotalNets = Abc_NtkGetTotalFanins(pNtk);

    // iterate over the network
    nNodesPrev = p->nNodesTotal;
    for ( Iter = 1; ; Iter++ )
    {
        // expand storage for changed nodes
        if ( p->pPars->fSatur )
            Vec_VecExpand( p->vVisited, Abc_NtkObjNumMax(pNtk) + 1 );

        // consider all nodes
        nNodes = Abc_NtkObjNumMax(pNtk);
        if ( !pPars->fVeryVerbose )
            pProgress = Extra_ProgressBarStart( stdout, nNodes );
        Abc_NtkForEachNode( pNtk, pObj, i )
        {
            // skip all except the final node
            if ( pPars->fFirst )
            {
                if ( !Abc_ObjIsCo(Abc_ObjFanout0(pObj)) )
                    continue;
            }

            if ( i >= nNodes )
                break;
            if ( !pPars->fVeryVerbose )
                Extra_ProgressBarUpdate( pProgress, i, NULL );
            // skip the nodes that did not change
            if ( p->pPars->fSatur && !Lpk_NodeHasChanged(p, pObj->Id) )
                continue;
            // resynthesize
            p->pObj = pObj;
            Lpk_ResynthesizeNode( p );

//            if ( p->nChanges == 3 )
//                break;
        }
        if ( !pPars->fVeryVerbose )
            Extra_ProgressBarStop( pProgress );

        // check the increase
        Delta = 100.00 * (nNodesPrev - Abc_NtkNodeNum(pNtk)) / p->nNodesTotal;
        if ( Delta < 0.05 )
            break;
        nNodesPrev = Abc_NtkNodeNum(pNtk);
        if ( !p->pPars->fSatur )
            break;

        if ( pPars->fFirst )
            break;
    }
    Abc_NtkStopReverseLevels( pNtk );

    if ( pPars->fVerbose )
    {
        p->nTotalNets2 = Abc_NtkGetTotalFanins(pNtk);
        printf( "Reduction in nodes = %5d. (%.2f %%) ", 
            p->nGainTotal, 100.0 * p->nGainTotal / p->nNodesTotal );
        printf( "Reduction in edges = %5d. (%.2f %%) ", 
            p->nTotalNets-p->nTotalNets2, 100.0*(p->nTotalNets-p->nTotalNets2)/p->nTotalNets );
        printf( "\n" );

        printf( "Nodes = %5d (%3d)  Cuts = %5d (%4d)  Changes = %5d  Iter = %2d  Benefit = %d.\n", 
            p->nNodesTotal, p->nNodesOver, p->nCutsTotal, p->nCutsUseful, p->nChanges, Iter, p->nBenefited );
        printf( "Non-DSD:" );
        for ( i = 3; i <= pPars->nVarsMax; i++ )
            if ( p->nBlocks[i] )
                printf( " %d=%d", i, p->nBlocks[i] );
        printf( "\n" );

        p->timeTotal = clock() - clk;
        p->timeEval  = p->timeEval  - p->timeMap;
        p->timeOther = p->timeTotal - p->timeCuts - p->timeTruth - p->timeEval - p->timeMap;
        PRTP( "Cuts  ", p->timeCuts,  p->timeTotal );
        PRTP( "Truth ", p->timeTruth, p->timeTotal );
        PRTP( "Eval  ", p->timeEval,  p->timeTotal );
        PRTP( "Map   ", p->timeMap,   p->timeTotal );
        PRTP( "Other ", p->timeOther, p->timeTotal );
        PRTP( "TOTAL ", p->timeTotal, p->timeTotal );
    }

    Lpk_ManStop( p );
    // check the resulting network
    if ( !Abc_NtkCheck( pNtk ) )
    {
        printf( "Lpk_Resynthesize: The network check has failed.\n" );
        return 0;
    }
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


