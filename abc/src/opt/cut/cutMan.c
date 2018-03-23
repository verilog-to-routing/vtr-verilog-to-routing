/**CFile****************************************************************

  FileName    [cutMan.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [K-feasible cut computation package.]

  Synopsis    [Cut manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cutMan.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cutInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern void Npn_StartTruth8( uint8 uTruths[][32] );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts the cut manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cut_Man_t * Cut_ManStart( Cut_Params_t * pParams )
{
    Cut_Man_t * p;
//    extern int nTruthDsd;
//    nTruthDsd = 0;
    assert( pParams->nVarsMax >= 3 && pParams->nVarsMax <= CUT_SIZE_MAX );
    p = ABC_ALLOC( Cut_Man_t, 1 );
    memset( p, 0, sizeof(Cut_Man_t) );
    // set and correct parameters
    p->pParams = pParams;
    // prepare storage for cuts
    p->vCutsNew = Vec_PtrAlloc( pParams->nIdsMax );
    Vec_PtrFill( p->vCutsNew, pParams->nIdsMax, NULL );
    // prepare storage for sequential cuts
    if ( pParams->fSeq )
    {
        p->pParams->fFilter = 1;
        p->vCutsOld = Vec_PtrAlloc( pParams->nIdsMax );
        Vec_PtrFill( p->vCutsOld, pParams->nIdsMax, NULL );
        p->vCutsTemp = Vec_PtrAlloc( pParams->nCutSet );
        Vec_PtrFill( p->vCutsTemp, pParams->nCutSet, NULL );
        if ( pParams->fTruth && pParams->nVarsMax > 5 )
        {
            pParams->fTruth = 0;
            printf( "Skipping computation of truth tables for sequential cuts with more than 5 inputs.\n" );
        }
    }
    // entry size
    p->EntrySize = sizeof(Cut_Cut_t) + pParams->nVarsMax * sizeof(int);
    if ( pParams->fTruth )
    {
        if ( pParams->nVarsMax > 14 )
        {
            pParams->fTruth = 0;
            printf( "Skipping computation of truth table for more than %d inputs.\n", 14 );
        }
        else
        {
            p->nTruthWords = Cut_TruthWords( pParams->nVarsMax );
            p->EntrySize += p->nTruthWords * sizeof(unsigned);
        }
        p->puTemp[0] = ABC_ALLOC( unsigned, 4 * p->nTruthWords );
        p->puTemp[1] = p->puTemp[0] + p->nTruthWords;
        p->puTemp[2] = p->puTemp[1] + p->nTruthWords;
        p->puTemp[3] = p->puTemp[2] + p->nTruthWords;
    }
    // enable cut computation recording
    if ( pParams->fRecord )
    {
        p->vNodeCuts   = Vec_IntStart( pParams->nIdsMax );
        p->vNodeStarts = Vec_IntStart( pParams->nIdsMax );
        p->vCutPairs   = Vec_IntAlloc( 0 );
    }
    // allocate storage for delays
    if ( pParams->fMap && !p->pParams->fSeq )
    {
        p->vDelays = Vec_IntStart( pParams->nIdsMax );
        p->vDelays2 = Vec_IntStart( pParams->nIdsMax );
        p->vCutsMax = Vec_PtrStart( pParams->nIdsMax );
    }
    // memory for cuts
    p->pMmCuts = Extra_MmFixedStart( p->EntrySize );
    p->vTemp = Vec_PtrAlloc( 100 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops the cut manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cut_ManStop( Cut_Man_t * p )
{
    if ( p->vCutsNew )    Vec_PtrFree( p->vCutsNew );
    if ( p->vCutsOld )    Vec_PtrFree( p->vCutsOld );
    if ( p->vCutsTemp )   Vec_PtrFree( p->vCutsTemp );
    if ( p->vFanCounts )  Vec_IntFree( p->vFanCounts );
    if ( p->vTemp )       Vec_PtrFree( p->vTemp );

    if ( p->vCutsMax )    Vec_PtrFree( p->vCutsMax );
    if ( p->vDelays )     Vec_IntFree( p->vDelays );
    if ( p->vDelays2 )    Vec_IntFree( p->vDelays2 );
    if ( p->vNodeCuts )   Vec_IntFree( p->vNodeCuts );
    if ( p->vNodeStarts ) Vec_IntFree( p->vNodeStarts );
    if ( p->vCutPairs )   Vec_IntFree( p->vCutPairs );
    if ( p->puTemp[0] )   ABC_FREE( p->puTemp[0] );

    Extra_MmFixedStop( p->pMmCuts );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cut_ManPrintStats( Cut_Man_t * p )
{
    if ( p->pReady )
    {
        Cut_CutRecycle( p, p->pReady );
        p->pReady = NULL;
    }
    printf( "Cut computation statistics:\n" );
    printf( "Current cuts      = %8d. (Trivial = %d.)\n", p->nCutsCur-p->nCutsTriv, p->nCutsTriv );
    printf( "Peak cuts         = %8d.\n", p->nCutsPeak );
    printf( "Total allocated   = %8d.\n", p->nCutsAlloc );
    printf( "Total deallocated = %8d.\n", p->nCutsDealloc );
    printf( "Cuts filtered     = %8d.\n", p->nCutsFilter );
    printf( "Nodes saturated   = %8d. (Max cuts = %d.)\n", p->nCutsLimit, p->pParams->nKeepMax );
    printf( "Cuts per node     = %8.1f\n", ((float)(p->nCutsCur-p->nCutsTriv))/p->nNodes );
    printf( "The cut size      = %8d bytes.\n", p->EntrySize );
    printf( "Peak memory       = %8.2f MB.\n", (float)p->nCutsPeak * p->EntrySize / (1<<20) );
    printf( "Total nodes       = %8d.\n", p->nNodes );
    if ( p->pParams->fDag || p->pParams->fTree )
    {
    printf( "DAG nodes         = %8d.\n", p->nNodesDag );
    printf( "Tree nodes        = %8d.\n", p->nNodes - p->nNodesDag );
    }
    printf( "Nodes w/o cuts    = %8d.\n", p->nNodesNoCuts );
    if ( p->pParams->fMap && !p->pParams->fSeq )
    printf( "Mapping delay     = %8d.\n", p->nDelayMin );

    ABC_PRT( "Merge ", p->timeMerge );
    ABC_PRT( "Union ", p->timeUnion );
    ABC_PRT( "Filter", p->timeFilter );
    ABC_PRT( "Truth ", p->timeTruth );
    ABC_PRT( "Map   ", p->timeMap );
//    printf( "Nodes = %d. Multi = %d.  Cuts = %d. Multi = %d.\n", 
//        p->nNodes, p->nNodesMulti, p->nCutsCur-p->nCutsTriv, p->nCutsMulti );
//    printf( "Count0 = %d. Count1 = %d. Count2 = %d.\n\n", p->Count0, p->Count1, p->Count2 );
}

    
/**Function*************************************************************

  Synopsis    [Prints some interesting stats.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cut_ManPrintStatsToFile( Cut_Man_t * p, char * pFileName, abctime TimeTotal )
{
    FILE * pTable;
    pTable = fopen( "cut_stats.txt", "a+" );
    fprintf( pTable, "%-20s ", pFileName );
    fprintf( pTable, "%8d ", p->nNodes );
    fprintf( pTable, "%6.1f ", ((float)(p->nCutsCur))/p->nNodes );
    fprintf( pTable, "%6.2f ", ((float)(100.0 * p->nCutsLimit))/p->nNodes );
    fprintf( pTable, "%6.2f ", (float)p->nCutsPeak * p->EntrySize / (1<<20) );
    fprintf( pTable, "%6.2f ", (float)(TimeTotal)/(float)(CLOCKS_PER_SEC) );
    fprintf( pTable, "\n" );
    fclose( pTable );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cut_ManSetFanoutCounts( Cut_Man_t * p, Vec_Int_t * vFanCounts )
{
    p->vFanCounts = vFanCounts;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cut_ManSetNodeAttrs( Cut_Man_t * p, Vec_Int_t * vNodeAttrs )
{
    p->vNodeAttrs = vNodeAttrs;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cut_ManReadVarsMax( Cut_Man_t * p )
{
    return p->pParams->nVarsMax;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cut_Params_t * Cut_ManReadParams( Cut_Man_t * p )
{
    return p->pParams;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Cut_ManReadNodeAttrs( Cut_Man_t * p )
{
    return p->vNodeAttrs;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cut_ManIncrementDagNodes( Cut_Man_t * p )
{
    p->nNodesDag++;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

