/**CFile****************************************************************

  FileName    [darCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware AIG rewriting.]

  Synopsis    [Core of the rewriting package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: darCore.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "darInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns the structure with default assignment of parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dar_ManDefaultRwrParams( Dar_RwrPar_t * pPars )
{
    memset( pPars, 0, sizeof(Dar_RwrPar_t) );
    pPars->nCutsMax     =  8; // 8
    pPars->nSubgMax     =  5; // 5 is a "magic number"
    pPars->fFanout      =  1;
    pPars->fUpdateLevel =  0;
    pPars->fUseZeros    =  0;
    pPars->fVerbose     =  0;
    pPars->fVeryVerbose =  0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dar_ManRewrite( Aig_Man_t * pAig, Dar_RwrPar_t * pPars )
{
    Dar_Man_t * p;
    ProgressBar * pProgress;
    Dar_Cut_t * pCut;
    Aig_Obj_t * pObj, * pObjNew;
    int i, k, nNodesOld, nNodeBefore, nNodeAfter, Required;
    int clk = 0, clkStart;
    // prepare the library
    Dar_LibPrepare( pPars->nSubgMax ); 
    // create rewriting manager
    p = Dar_ManStart( pAig, pPars );
    // remove dangling nodes
    Aig_ManCleanup( pAig );
    // if updating levels is requested, start fanout and timing
    if ( p->pPars->fFanout )
        Aig_ManFanoutStart( pAig );
    if ( p->pPars->fUpdateLevel )
        Aig_ManStartReverseLevels( pAig, 0 );
    // set elementary cuts for the PIs
    Dar_ManCutsStart( p );
    // resynthesize each node once
    clkStart = clock();
    p->nNodesInit = Aig_ManNodeNum(pAig);
    nNodesOld = Vec_PtrSize( pAig->vObjs );

    pProgress = Extra_ProgressBarStart( stdout, nNodesOld );
    Aig_ManForEachObj( pAig, pObj, i )
//    pProgress = Extra_ProgressBarStart( stdout, 100 );
//    Aig_ManOrderStart( pAig );
//    Aig_ManForEachNodeInOrder( pAig, pObj )
    {
//        Extra_ProgressBarUpdate( pProgress, 100*pAig->nAndPrev/pAig->nAndTotal, NULL );

        Extra_ProgressBarUpdate( pProgress, i, NULL );
        if ( !Aig_ObjIsNode(pObj) )
            continue;
        if ( i > nNodesOld )
            break;

        // consider freeing the cuts
//        if ( (i & 0xFFF) == 0 && Aig_MmFixedReadMemUsage(p->pMemCuts)/(1<<20) > 100 )
//            Dar_ManCutsStart( p );

        // compute cuts for the node
        p->nNodesTried++;
clk = clock();
        Dar_ObjComputeCuts_rec( p, pObj );
p->timeCuts += clock() - clk;

        // check if there is a trivial cut
        Dar_ObjForEachCut( pObj, pCut, k )
            if ( pCut->nLeaves == 0 || (pCut->nLeaves == 1 && pCut->pLeaves[0] != pObj->Id && Aig_ManObj(p->pAig, pCut->pLeaves[0])) )
                break;
        if ( k < (int)pObj->nCuts )
        {
            assert( pCut->nLeaves < 2 );
            if ( pCut->nLeaves == 0 ) // replace by constant
            {
                assert( pCut->uTruth == 0 || pCut->uTruth == 0xFFFF );
                pObjNew = Aig_NotCond( Aig_ManConst1(p->pAig), pCut->uTruth==0 );
            }
            else
            {
                assert( pCut->uTruth == 0xAAAA || pCut->uTruth == 0x5555 );
                pObjNew = Aig_NotCond( Aig_ManObj(p->pAig, pCut->pLeaves[0]), pCut->uTruth==0x5555 );
            }
            // remove the old cuts
            Dar_ObjSetCuts( pObj, NULL );
            // replace the node
            Aig_ObjReplace( pAig, pObj, pObjNew, 1, p->pPars->fUpdateLevel );
            continue;
        }

        // evaluate the cuts
        p->GainBest = -1;
        Required = pAig->vLevelR? Aig_ObjRequiredLevel(pAig, pObj) : AIG_INFINITY;
        Dar_ObjForEachCut( pObj, pCut, k )
            Dar_LibEval( p, pObj, pCut, Required );
        // check the best gain
        if ( !(p->GainBest > 0 || (p->GainBest == 0 && p->pPars->fUseZeros)) )
        {
//            Aig_ObjOrderAdvance( pAig );
            continue;
        }
        // remove the old cuts
        Dar_ObjSetCuts( pObj, NULL );
        // if we end up here, a rewriting step is accepted
        nNodeBefore = Aig_ManNodeNum( pAig );
        pObjNew = Dar_LibBuildBest( p ); // pObjNew can be complemented!
        pObjNew = Aig_NotCond( pObjNew, Aig_ObjPhaseReal(pObjNew) ^ pObj->fPhase );
        assert( (int)Aig_Regular(pObjNew)->Level <= Required );
        // replace the node
        Aig_ObjReplace( pAig, pObj, pObjNew, 1, p->pPars->fUpdateLevel );
        // compare the gains
        nNodeAfter = Aig_ManNodeNum( pAig );
        assert( p->GainBest <= nNodeBefore - nNodeAfter );
        // count gains of this class
        p->ClassGains[p->ClassBest] += nNodeBefore - nNodeAfter;
    }
//    Aig_ManOrderStop( pAig );

p->timeTotal = clock() - clkStart;
p->timeOther = p->timeTotal - p->timeCuts - p->timeEval;

    Extra_ProgressBarStop( pProgress );
    p->nCutMemUsed = Aig_MmFixedReadMemUsage(p->pMemCuts)/(1<<20);
    Dar_ManCutsFree( p );
    // put the nodes into the DFS order and reassign their IDs
//    Aig_NtkReassignIds( p );
    // fix the levels
//    Aig_ManVerifyLevel( pAig );
    if ( p->pPars->fFanout )
        Aig_ManFanoutStop( pAig );
    if ( p->pPars->fUpdateLevel )
    {
//        Aig_ManVerifyReverseLevel( pAig );
        Aig_ManStopReverseLevels( pAig );
    }
    // stop the rewriting manager
    Dar_ManStop( p );
    Aig_ManCheckPhase( pAig );
    // check
    if ( !Aig_ManCheck( pAig ) )
    {
        printf( "Aig_ManRewrite: The network check has failed.\n" );
        return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_MmFixed_t * Dar_ManComputeCuts( Aig_Man_t * pAig, int nCutsMax )
{
    Dar_Man_t * p;
    Dar_RwrPar_t Pars, * pPars = &Pars; 
    Aig_Obj_t * pObj;
    Aig_MmFixed_t * pMemCuts;
    int i, nNodes, clk = 0, clkStart = clock();
    // remove dangling nodes
    if ( nNodes = Aig_ManCleanup( pAig ) )
    {
//        printf( "Removing %d nodes.\n", nNodes );
    }
    // create default parameters
    Dar_ManDefaultRwrParams( pPars );
    pPars->nCutsMax = nCutsMax;
    // create rewriting manager
    p = Dar_ManStart( pAig, pPars );
    // set elementary cuts for the PIs
    Dar_ManCutsStart( p );
    // compute cuts for each nodes in the topological order
    Aig_ManForEachNode( pAig, pObj, i )
        Dar_ObjComputeCuts( p, pObj );
    // free the cuts
    pMemCuts = p->pMemCuts;
    p->pMemCuts = NULL;
//    Dar_ManCutsFree( p );
    // stop the rewriting manager
    Dar_ManStop( p );
    return pMemCuts;
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


