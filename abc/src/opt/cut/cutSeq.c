/**CFile****************************************************************

  FileName    [cutSeq.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [K-feasible cut computation package.]

  Synopsis    [Sequential cut computation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cutSeq.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cutInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Shifts all cut leaves of the node by the given number of latches.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Cut_NodeShiftCutLeaves( Cut_Cut_t * pList, int nLat )
{
    Cut_Cut_t * pTemp;
    int i;
    // shift the cuts by as many latches
    Cut_ListForEachCut( pList, pTemp )
    {
        pTemp->uSign = 0;
        for ( i = 0; i < (int)pTemp->nLeaves; i++ )
        {
            pTemp->pLeaves[i] += nLat;
            pTemp->uSign      |= Cut_NodeSign( pTemp->pLeaves[i] );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Computes sequential cuts for the node from its fanins.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cut_NodeComputeCutsSeq( Cut_Man_t * p, int Node, int Node0, int Node1, int fCompl0, int fCompl1, int nLat0, int nLat1, int fTriv, int CutSetNum )
{
    Cut_List_t Super, * pSuper = &Super;
    Cut_Cut_t * pListNew;
    abctime clk;
    
    // get the number of cuts at the node
    p->nNodeCuts = Cut_CutCountList( Cut_NodeReadCutsOld(p, Node) );
    if ( p->nNodeCuts >= p->pParams->nKeepMax )
        return;

    // count only the first visit
    if ( p->nNodeCuts == 0 )
        p->nNodes++;

    // store the fanin lists
    p->pStore0[0] = Cut_NodeReadCutsOld( p, Node0 );
    p->pStore0[1] = Cut_NodeReadCutsNew( p, Node0 );
    p->pStore1[0] = Cut_NodeReadCutsOld( p, Node1 );
    p->pStore1[1] = Cut_NodeReadCutsNew( p, Node1 );

    // duplicate the cut lists if fanin nodes are non-standard
    if ( Node == Node0 || Node == Node1 || Node0 == Node1 )
    {
        p->pStore0[0] = Cut_CutDupList( p, p->pStore0[0] );
        p->pStore0[1] = Cut_CutDupList( p, p->pStore0[1] );
        p->pStore1[0] = Cut_CutDupList( p, p->pStore1[0] );
        p->pStore1[1] = Cut_CutDupList( p, p->pStore1[1] );
    }

    // shift the cuts by as many latches and recompute signatures
    if ( nLat0 ) Cut_NodeShiftCutLeaves( p->pStore0[0], nLat0 );
    if ( nLat0 ) Cut_NodeShiftCutLeaves( p->pStore0[1], nLat0 );
    if ( nLat1 ) Cut_NodeShiftCutLeaves( p->pStore1[0], nLat1 );
    if ( nLat1 ) Cut_NodeShiftCutLeaves( p->pStore1[1], nLat1 );

    // store the original lists for comparison
    p->pCompareOld = Cut_NodeReadCutsOld( p, Node );
    p->pCompareNew = Cut_NodeReadCutsNew( p, Node );

    // merge the old and the new
clk = Abc_Clock();
    Cut_ListStart( pSuper );
    Cut_NodeDoComputeCuts( p, pSuper, Node, fCompl0, fCompl1, p->pStore0[0], p->pStore1[1], 0, 0 );
    Cut_NodeDoComputeCuts( p, pSuper, Node, fCompl0, fCompl1, p->pStore0[1], p->pStore1[0], 0, 0 );
    Cut_NodeDoComputeCuts( p, pSuper, Node, fCompl0, fCompl1, p->pStore0[1], p->pStore1[1], fTriv, 0 );
    pListNew = Cut_ListFinish( pSuper );
p->timeMerge += Abc_Clock() - clk;

    // shift the cuts by as many latches and recompute signatures
    if ( Node == Node0 || Node == Node1 || Node0 == Node1 )
    {
        Cut_CutRecycleList( p, p->pStore0[0] );
        Cut_CutRecycleList( p, p->pStore0[1] );
        Cut_CutRecycleList( p, p->pStore1[0] );
        Cut_CutRecycleList( p, p->pStore1[1] );
    }
    else
    {
        if ( nLat0 ) Cut_NodeShiftCutLeaves( p->pStore0[0], -nLat0 );
        if ( nLat0 ) Cut_NodeShiftCutLeaves( p->pStore0[1], -nLat0 );
        if ( nLat1 ) Cut_NodeShiftCutLeaves( p->pStore1[0], -nLat1 );
        if ( nLat1 ) Cut_NodeShiftCutLeaves( p->pStore1[1], -nLat1 );
    }

    // set the lists at the node
    if ( CutSetNum >= 0 )
    {
        assert( Cut_NodeReadCutsTemp(p, CutSetNum) == NULL );
        Cut_NodeWriteCutsTemp( p, CutSetNum, pListNew );
    }
    else
    {
        assert( Cut_NodeReadCutsNew(p, Node) == NULL );
        Cut_NodeWriteCutsNew( p, Node, pListNew );
    }

    // mark the node if we exceeded the number of cuts
    if ( p->nNodeCuts >= p->pParams->nKeepMax )
        p->nCutsLimit++;
}

/**Function*************************************************************

  Synopsis    [Merges the new cuts with the old cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cut_NodeNewMergeWithOld( Cut_Man_t * p, int Node )
{
    Cut_Cut_t * pListOld, * pListNew, * pList;
    // get the new cuts
    pListNew = Cut_NodeReadCutsNew( p, Node );
    if ( pListNew == NULL )
        return;
    Cut_NodeWriteCutsNew( p, Node, NULL );
    // get the old cuts
    pListOld = Cut_NodeReadCutsOld( p, Node );
    if ( pListOld == NULL )
    {
        Cut_NodeWriteCutsOld( p, Node, pListNew );
        return;
    }
    // merge the lists
    pList = Cut_CutMergeLists( pListOld, pListNew );
    Cut_NodeWriteCutsOld( p, Node, pList );
}


/**Function*************************************************************

  Synopsis    [Transfers the temporary cuts to be the new cuts.]

  Description [Returns 1 if something was transferred.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cut_NodeTempTransferToNew( Cut_Man_t * p, int Node, int CutSetNum )
{
    Cut_Cut_t * pList;
    pList = Cut_NodeReadCutsTemp( p, CutSetNum );
    Cut_NodeWriteCutsTemp( p, CutSetNum, NULL );
    Cut_NodeWriteCutsNew( p, Node, pList );
    return pList != NULL;
}

/**Function*************************************************************

  Synopsis    [Transfers the old cuts to be the new cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cut_NodeOldTransferToNew( Cut_Man_t * p, int Node )
{
    Cut_Cut_t * pList;
    pList = Cut_NodeReadCutsOld( p, Node );
    Cut_NodeWriteCutsOld( p, Node, NULL );
    Cut_NodeWriteCutsNew( p, Node, pList );
//    Cut_CutListVerify( pList );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

