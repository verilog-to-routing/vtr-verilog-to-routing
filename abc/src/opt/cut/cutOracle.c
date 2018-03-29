/**CFile****************************************************************

  FileName    [cutOracle.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [K-feasible cut computation package.]

  Synopsis    [Procedures to compute cuts for a node using the oracle.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cutOracle.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cutInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

struct Cut_OracleStruct_t_
{
    // cut comptupatation parameters
    Cut_Params_t *     pParams;
    Vec_Int_t *        vFanCounts;
    int                fSimul;
    // storage for cuts
    Vec_Ptr_t *        vCutsNew;
    Vec_Ptr_t *        vCuts0;
    Vec_Ptr_t *        vCuts1;
    // oracle info 
    Vec_Int_t *        vNodeCuts;
    Vec_Int_t *        vNodeStarts;
    Vec_Int_t *        vCutPairs;
    // memory management
    Extra_MmFixed_t *  pMmCuts;
    int                EntrySize;
    int                nTruthWords;
    // stats
    abctime            timeTotal;
    int                nCuts;
    int                nCutsTriv;
};

static Cut_Cut_t * Cut_CutStart( Cut_Oracle_t * p );
static Cut_Cut_t * Cut_CutTriv( Cut_Oracle_t * p, int Node );
static Cut_Cut_t * Cut_CutMerge( Cut_Oracle_t * p, Cut_Cut_t * pCut0, Cut_Cut_t * pCut1 );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts the cut oracle.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cut_Oracle_t * Cut_OracleStart( Cut_Man_t * pMan )
{
    Cut_Oracle_t * p;

    assert( pMan->pParams->nVarsMax >= 3 && pMan->pParams->nVarsMax <= CUT_SIZE_MAX );
    assert( pMan->pParams->fRecord );

    p = ABC_ALLOC( Cut_Oracle_t, 1 );
    memset( p, 0, sizeof(Cut_Oracle_t) );

    // set and correct parameters
    p->pParams     = pMan->pParams;

    // transfer the recording info
    p->vNodeCuts   = pMan->vNodeCuts;    pMan->vNodeCuts   = NULL;
    p->vNodeStarts = pMan->vNodeStarts;  pMan->vNodeStarts = NULL;
    p->vCutPairs   = pMan->vCutPairs;    pMan->vCutPairs   = NULL;

    // prepare storage for cuts
    p->vCutsNew = Vec_PtrAlloc( p->pParams->nIdsMax );
    Vec_PtrFill( p->vCutsNew, p->pParams->nIdsMax, NULL );
    p->vCuts0 = Vec_PtrAlloc( 100 );
    p->vCuts1 = Vec_PtrAlloc( 100 );

    // entry size
    p->EntrySize = sizeof(Cut_Cut_t) + p->pParams->nVarsMax * sizeof(int);
    if ( p->pParams->fTruth )
    {
        if ( p->pParams->nVarsMax > 8 )
        {
            p->pParams->fTruth = 0;
            printf( "Skipping computation of truth table for more than 8 inputs.\n" );
        }
        else
        {
            p->nTruthWords = Cut_TruthWords( p->pParams->nVarsMax );
            p->EntrySize += p->nTruthWords * sizeof(unsigned);
        }
    }
    // memory for cuts
    p->pMmCuts = Extra_MmFixedStart( p->EntrySize );
    return p;
}
/**Function*************************************************************

  Synopsis    [Stop the cut oracle.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cut_OracleStop( Cut_Oracle_t * p )
{
//    if ( p->pParams->fVerbose )
    {
        printf( "Cut computation statistics with oracle:\n" );
        printf( "Current cuts      = %8d. (Trivial = %d.)\n", p->nCuts-p->nCutsTriv, p->nCutsTriv );
        ABC_PRT( "Total time ", p->timeTotal );
    }

    if ( p->vCuts0 )      Vec_PtrFree( p->vCuts0 );
    if ( p->vCuts1 )      Vec_PtrFree( p->vCuts1 );
    if ( p->vCutsNew )    Vec_PtrFree( p->vCutsNew );
    if ( p->vFanCounts )  Vec_IntFree( p->vFanCounts );

    if ( p->vNodeCuts )   Vec_IntFree( p->vNodeCuts );
    if ( p->vNodeStarts ) Vec_IntFree( p->vNodeStarts );
    if ( p->vCutPairs )   Vec_IntFree( p->vCutPairs );

    Extra_MmFixedStop( p->pMmCuts );
    ABC_FREE( p ); 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cut_OracleSetFanoutCounts( Cut_Oracle_t * p, Vec_Int_t * vFanCounts )
{
    p->vFanCounts = vFanCounts;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cut_OracleReadDrop( Cut_Oracle_t * p )
{
    return p->pParams->fDrop;
}

/**Function*************************************************************

  Synopsis    [Sets the trivial cut for the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cut_OracleNodeSetTriv( Cut_Oracle_t * p, int Node )
{
    assert( Vec_PtrEntry( p->vCutsNew, Node ) == NULL );
    Vec_PtrWriteEntry( p->vCutsNew, Node, Cut_CutTriv(p, Node) );
}



/**Function*************************************************************

  Synopsis    [Allocates the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cut_Cut_t * Cut_CutStart( Cut_Oracle_t * p )
{
    Cut_Cut_t * pCut;
    // cut allocation
    pCut = (Cut_Cut_t *)Extra_MmFixedEntryFetch( p->pMmCuts );
    memset( pCut, 0, sizeof(Cut_Cut_t) );
    pCut->nVarsMax   = p->pParams->nVarsMax;
    pCut->fSimul     = p->fSimul;
    p->nCuts++;
    return pCut;
}

/**Function*************************************************************

  Synopsis    [Creates the trivial cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cut_Cut_t * Cut_CutTriv( Cut_Oracle_t * p, int Node )
{
    Cut_Cut_t * pCut;
    pCut = Cut_CutStart( p );
    pCut->nLeaves    = 1;
    pCut->pLeaves[0] = Node;
    if ( p->pParams->fTruth )
    {
        unsigned * pTruth = Cut_CutReadTruth(pCut);
        int i;
        for ( i = 0; i < p->nTruthWords; i++ )
            pTruth[i] = 0xAAAAAAAA;
    }
    p->nCutsTriv++;
    return pCut;
} 

/**Function*************************************************************

  Synopsis    [Merges two cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cut_Cut_t * Cut_CutMerge( Cut_Oracle_t * p, Cut_Cut_t * pCut0, Cut_Cut_t * pCut1 )
{
    Cut_Cut_t * pCut;
    int Limit, i, k, c;
    // create the leaves of the new cut
    pCut = Cut_CutStart( p );
    Limit = p->pParams->nVarsMax;
    for ( i = k = c = 0; c < Limit; c++ )
    {
        if ( k == (int)pCut1->nLeaves )
        {
            if ( i == (int)pCut0->nLeaves )
            {
                pCut->nLeaves = c;
                return pCut;
            }
            pCut->pLeaves[c] = pCut0->pLeaves[i++];
            continue;
        }
        if ( i == (int)pCut0->nLeaves )
        {
            if ( k == (int)pCut1->nLeaves )
            {
                pCut->nLeaves = c;
                return pCut;
            }
            pCut->pLeaves[c] = pCut1->pLeaves[k++];
            continue;
        }
        if ( pCut0->pLeaves[i] < pCut1->pLeaves[k] )
        {
            pCut->pLeaves[c] = pCut0->pLeaves[i++];
            continue;
        }
        if ( pCut0->pLeaves[i] > pCut1->pLeaves[k] )
        {
            pCut->pLeaves[c] = pCut1->pLeaves[k++];
            continue;
        }
        pCut->pLeaves[c] = pCut0->pLeaves[i++]; 
        k++;
    }
    assert( i == (int)pCut0->nLeaves && k == (int)pCut1->nLeaves );
    pCut->nLeaves = c;
    return pCut;
} 

/**Function*************************************************************

  Synopsis    [Reconstruct the cuts of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cut_Cut_t * Cut_OracleComputeCuts( Cut_Oracle_t * p, int Node, int Node0, int Node1, int fCompl0, int fCompl1 )
{
    Cut_Cut_t * pList = NULL, ** ppTail = &pList;
    Cut_Cut_t * pCut, * pCut0, * pCut1, * pList0, * pList1;
    int iCutStart, nCuts, i, Entry;
    abctime clk = Abc_Clock();

    // get the cuts of the children
    pList0 = (Cut_Cut_t *)Vec_PtrEntry( p->vCutsNew, Node0 );
    pList1 = (Cut_Cut_t *)Vec_PtrEntry( p->vCutsNew, Node1 );
    assert( pList0 && pList1 );

    // get the complemented attribute of the cut
    p->fSimul = (fCompl0 ^ pList0->fSimul) & (fCompl1 ^ pList1->fSimul);

    // collect the cuts
    Vec_PtrClear( p->vCuts0 );
    Cut_ListForEachCut( pList0, pCut )
        Vec_PtrPush( p->vCuts0, pCut );
    Vec_PtrClear( p->vCuts1 );
    Cut_ListForEachCut( pList1, pCut )
        Vec_PtrPush( p->vCuts1, pCut );

    // get the first and last cuts of this node
    nCuts = Vec_IntEntry(p->vNodeCuts, Node);
    iCutStart = Vec_IntEntry(p->vNodeStarts, Node);

    // create trivial cut
    assert( Vec_IntEntry(p->vCutPairs, iCutStart) == 0 );
    pCut = Cut_CutTriv( p, Node );
    *ppTail = pCut;
    ppTail = &pCut->pNext;
    // create other cuts
    for ( i = 1; i < nCuts; i++ )
    {
        Entry = Vec_IntEntry( p->vCutPairs, iCutStart + i );
        pCut0 = (Cut_Cut_t *)Vec_PtrEntry( p->vCuts0, Entry & 0xFFFF );
        pCut1 = (Cut_Cut_t *)Vec_PtrEntry( p->vCuts1, Entry >> 16 );
        pCut  = Cut_CutMerge( p, pCut0, pCut1 );
        *ppTail = pCut;
        ppTail = &pCut->pNext;
        // compute the truth table
        if ( p->pParams->fTruth )
            Cut_TruthComputeOld( pCut, pCut0, pCut1, fCompl0, fCompl1 );
    }
    *ppTail = NULL;

    // write the new cut
    assert( Vec_PtrEntry( p->vCutsNew, Node ) == NULL );
    Vec_PtrWriteEntry( p->vCutsNew, Node, pList );
p->timeTotal += Abc_Clock() - clk;
    return pList;
}

/**Function*************************************************************

  Synopsis    [Deallocates the cuts at the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cut_OracleFreeCuts( Cut_Oracle_t * p, int Node )
{
    Cut_Cut_t * pList, * pCut, * pCut2;
    pList = (Cut_Cut_t *)Vec_PtrEntry( p->vCutsNew, Node );
    if ( pList == NULL )
        return;
    Cut_ListForEachCutSafe( pList, pCut, pCut2 )
        Extra_MmFixedEntryRecycle( p->pMmCuts, (char *)pCut );
    Vec_PtrWriteEntry( p->vCutsNew, Node, pList );
}

/**Function*************************************************************

  Synopsis    [Consider dropping cuts if they are useless by now.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cut_OracleTryDroppingCuts( Cut_Oracle_t * p, int Node )
{
    int nFanouts;
    assert( p->vFanCounts );
    nFanouts = Vec_IntEntry( p->vFanCounts, Node );
    assert( nFanouts > 0 );
    if ( --nFanouts == 0 )
        Cut_OracleFreeCuts( p, Node );
    Vec_IntWriteEntry( p->vFanCounts, Node, nFanouts );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

