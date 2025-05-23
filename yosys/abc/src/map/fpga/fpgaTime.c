/**CFile****************************************************************

  FileName    [fpgaTime.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Technology mapping for variable-size-LUT FPGAs.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - August 18, 2004.]

  Revision    [$Id: fpgaTime.c,v 1.1 2005/01/23 06:59:42 alanmi Exp $]

***********************************************************************/

#include "fpgaInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the arrival times of the cut.]

  Description [Computes the maximum arrival time of the cut leaves and
  adds the delay of the LUT.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Fpga_TimeCutComputeArrival( Fpga_Man_t * pMan, Fpga_Cut_t * pCut )
{
    int i;
    float tArrival;
    tArrival = -FPGA_FLOAT_LARGE;
    for ( i = 0; i < pCut->nLeaves; i++ )
        if ( tArrival < pCut->ppLeaves[i]->pCutBest->tArrival )
            tArrival = pCut->ppLeaves[i]->pCutBest->tArrival;
    tArrival += pMan->pLutLib->pLutDelays[(int)pCut->nLeaves][0];
    return tArrival;
}

/**Function*************************************************************

  Synopsis    [Computes the arrival times of the cut recursively.]

  Description [When computing the arrival time for the previously unused 
  cuts, their arrival time may be incorrect because their fanins have 
  incorrect arrival time. This procedure is called to fix this problem.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Fpga_TimeCutComputeArrival_rec( Fpga_Man_t * pMan, Fpga_Cut_t * pCut )
{
    int i;
    for ( i = 0; i < pCut->nLeaves; i++ )
        if ( pCut->ppLeaves[i]->nRefs == 0 )
            Fpga_TimeCutComputeArrival_rec( pMan, pCut->ppLeaves[i]->pCutBest );
    return Fpga_TimeCutComputeArrival( pMan, pCut );
}

/**Function*************************************************************

  Synopsis    [Computes the maximum arrival times.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Fpga_TimeComputeArrivalMax( Fpga_Man_t * p )
{
    float fRequired;
    int i;
    if ( p->fLatchPaths && p->nLatches == 0 )
    {
        printf( "Delay optimization of latch path is not performed because there is no latches.\n" );
        p->fLatchPaths = 0;
    }
    // get the critical PO arrival time
    fRequired = -FPGA_FLOAT_LARGE;
    if ( p->fLatchPaths )
    {
        for ( i = p->nOutputs - p->nLatches; i < p->nOutputs; i++ )
        {
            if ( Fpga_NodeIsConst(p->pOutputs[i]) )
                continue;
            fRequired = FPGA_MAX( fRequired, Fpga_Regular(p->pOutputs[i])->pCutBest->tArrival );
//            printf( " %5.1f", Fpga_Regular(p->pOutputs[i])->pCutBest->tArrival );
        }
//        printf( "Required latches = %5.1f\n", fRequired );
    }
    else
    {
        for ( i = 0; i < p->nOutputs; i++ )
        {
            if ( Fpga_NodeIsConst(p->pOutputs[i]) )
                continue;
            fRequired = FPGA_MAX( fRequired, Fpga_Regular(p->pOutputs[i])->pCutBest->tArrival );
//            printf( " %5.1f", Fpga_Regular(p->pOutputs[i])->pCutBest->tArrival );
        }
//        printf( "Required outputs = %5.1f\n", fRequired );
    }
    return fRequired;
}

/**Function*************************************************************

  Synopsis    [Computes the required times of all nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_TimeComputeRequiredGlobal( Fpga_Man_t * p, int fFirstTime )
{
    p->fRequiredGlo = Fpga_TimeComputeArrivalMax( p );
    // update the required times according to the target
    if ( p->DelayTarget != -1 )
    {
        if ( p->fRequiredGlo > p->DelayTarget + p->fEpsilon )
        {
            if ( fFirstTime )
                printf( "Cannot meet the target required times (%4.2f). Mapping continues anyway.\n", p->DelayTarget );
        }
        else if ( p->fRequiredGlo < p->DelayTarget - p->fEpsilon )
        {
            if ( fFirstTime )
                printf( "Relaxing the required times from (%4.2f) to the target (%4.2f).\n", p->fRequiredGlo, p->DelayTarget );
            p->fRequiredGlo = p->DelayTarget;
        }
    }
    Fpga_TimeComputeRequired( p, p->fRequiredGlo );
}

/**Function*************************************************************

  Synopsis    [Computes the required times of all nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_TimeComputeRequired( Fpga_Man_t * p, float fRequired )
{
    int i;
    // clean the required times and the fanout counts for all nodes
    for ( i = 0; i < p->vAnds->nSize; i++ )
        p->vAnds->pArray[i]->tRequired = FPGA_FLOAT_LARGE;
    // set the required times for the POs
    if ( p->fLatchPaths )
        for ( i = p->nOutputs - p->nLatches; i < p->nOutputs; i++ )
            Fpga_Regular(p->pOutputs[i])->tRequired = fRequired;
    else
        for ( i = 0; i < p->nOutputs; i++ )
            Fpga_Regular(p->pOutputs[i])->tRequired = fRequired;
    // collect nodes reachable from POs in the DFS order through the best cuts
    Fpga_TimePropagateRequired( p, p->vMapping );
/*
    {
        int Counter = 0;
        for ( i = 0; i < p->vAnds->nSize; i++ )
            if ( p->vAnds->pArray[i]->tRequired > FPGA_FLOAT_LARGE - 100 )
                Counter++;
        printf( "The number of nodes with large required times = %d.\n", Counter );
    }
*/
}

/**Function*************************************************************

  Synopsis    [Computes the required times of the given nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_TimePropagateRequired( Fpga_Man_t * p, Fpga_NodeVec_t * vNodes )
{
    Fpga_Node_t * pNode, * pChild;
    float fRequired;
    int i, k;

    // sorts the nodes in the decreasing order of levels
//    Fpga_MappingSortByLevel( p, vNodes, 0 );
    // the nodes area already sorted in Fpga_MappingSetRefsAndArea()

    // go through the nodes in the reverse topological order
    for ( k = 0; k < vNodes->nSize; k++ )
    {
        pNode = vNodes->pArray[k];
        if ( !Fpga_NodeIsAnd(pNode) )
            continue;
        // get the required time for children
        fRequired = pNode->tRequired - p->pLutLib->pLutDelays[(int)pNode->pCutBest->nLeaves][0];
        // update the required time of the children
        for ( i = 0; i < pNode->pCutBest->nLeaves; i++ )
        {
            pChild = pNode->pCutBest->ppLeaves[i];
            pChild->tRequired = FPGA_MIN( pChild->tRequired, fRequired );
        }
    }
}



/**Function*************************************************************

  Synopsis    [Computes the required times of all nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_TimePropagateArrival( Fpga_Man_t * p )
{
    Fpga_Node_t * pNode;
    Fpga_Cut_t * pCut;
    int i;

    // clean the required times and the fanout counts for all nodes
    for ( i = 0; i < p->vAnds->nSize; i++ )
    {
        pNode = p->vAnds->pArray[i];
        for ( pCut = pNode->pCuts->pNext; pCut; pCut = pCut->pNext )
            pCut->tArrival = Fpga_TimeCutComputeArrival( p, pCut );
    }
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

