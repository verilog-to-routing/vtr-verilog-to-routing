/**CFile****************************************************************

  FileName    [cutNode.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [K-feasible cut computation package.]

  Synopsis    [Procedures to compute cuts for a node.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cutNode.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

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

  Synopsis    [Allocates the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cut_Cut_t * Cut_CutAlloc( Cut_Man_t * p )
{
    Cut_Cut_t * pCut;
    // cut allocation
    pCut = (Cut_Cut_t *)Extra_MmFixedEntryFetch( p->pMmCuts );
    memset( pCut, 0, sizeof(Cut_Cut_t) );
    pCut->nVarsMax   = p->pParams->nVarsMax;
    pCut->fSimul     = p->fSimul;
    // statistics
    p->nCutsAlloc++;
    p->nCutsCur++;
    if ( p->nCutsPeak < p->nCutsAlloc - p->nCutsDealloc )
        p->nCutsPeak = p->nCutsAlloc - p->nCutsDealloc;
    return pCut;
}

/**Function*************************************************************

  Synopsis    [Recybles the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cut_CutRecycle( Cut_Man_t * p, Cut_Cut_t * pCut )
{
    p->nCutsDealloc++;
    p->nCutsCur--;
    if ( pCut->nLeaves == 1 )
        p->nCutsTriv--;
    Extra_MmFixedEntryRecycle( p->pMmCuts, (char *)pCut );
}

/**Function*************************************************************

  Synopsis    [Compares two cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cut_CutCompare( Cut_Cut_t * pCut1, Cut_Cut_t * pCut2 )
{
    int i;
    if ( pCut1->nLeaves < pCut2->nLeaves )
        return -1;
    if ( pCut1->nLeaves > pCut2->nLeaves )
        return 1;
    for ( i = 0; i < (int)pCut1->nLeaves; i++ )
    {
        if ( pCut1->pLeaves[i] < pCut2->pLeaves[i] )
            return -1;
        if ( pCut1->pLeaves[i] > pCut2->pLeaves[i] )
            return 1;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Duplicates the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cut_Cut_t * Cut_CutDupList( Cut_Man_t * p, Cut_Cut_t * pList )
{
    Cut_Cut_t * pHead = NULL, ** ppTail = &pHead;
    Cut_Cut_t * pTemp, * pCopy;
    if ( pList == NULL )
        return NULL;
    Cut_ListForEachCut( pList, pTemp )
    {
        pCopy = (Cut_Cut_t *)Extra_MmFixedEntryFetch( p->pMmCuts );
        memcpy( pCopy, pTemp, p->EntrySize );
        *ppTail = pCopy;
        ppTail = &pCopy->pNext;
    }
    *ppTail = NULL;
    return pHead;
}

/**Function*************************************************************

  Synopsis    [Recycles the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cut_CutRecycleList( Cut_Man_t * p, Cut_Cut_t * pList )
{
    Cut_Cut_t * pCut, * pCut2;
    Cut_ListForEachCutSafe( pList, pCut, pCut2 )
        Extra_MmFixedEntryRecycle( p->pMmCuts, (char *)pCut );
}

/**Function*************************************************************

  Synopsis    [Counts the number of cuts in the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cut_CutCountList( Cut_Cut_t * pList )
{
    int Counter = 0;
    Cut_ListForEachCut( pList, pList )
        Counter++;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Merges two NULL-terminated linked lists.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cut_Cut_t * Cut_CutMergeLists( Cut_Cut_t * pList1, Cut_Cut_t * pList2 )
{
    Cut_Cut_t * pList = NULL, ** ppTail = &pList;
    Cut_Cut_t * pCut;
    while ( pList1 && pList2 )
    {
        if ( Cut_CutCompare(pList1, pList2) < 0 )
        {
            pCut = pList1;
            pList1 = pList1->pNext;
        }
        else
        {
            pCut = pList2;
            pList2 = pList2->pNext;
        }
        *ppTail = pCut;
        ppTail = &pCut->pNext;
    }
    *ppTail = pList1? pList1: pList2;
    return pList;
}

/**Function*************************************************************

  Synopsis    [Sets the number of the cuts in the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cut_CutNumberList( Cut_Cut_t * pList )
{
    Cut_Cut_t * pCut;
    int i = 0;
    Cut_ListForEachCut( pList, pCut )
        pCut->Num0 = i++;
}

/**Function*************************************************************

  Synopsis    [Creates the trivial cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cut_Cut_t * Cut_CutCreateTriv( Cut_Man_t * p, int Node )
{
    Cut_Cut_t * pCut;
    if ( p->pParams->fSeq )
        Node <<= CUT_SHIFT;
    pCut = Cut_CutAlloc( p );
    pCut->nLeaves    = 1;
    pCut->pLeaves[0] = Node;
    pCut->uSign      = Cut_NodeSign( Node );
    if ( p->pParams->fTruth )
    {
/*
        if ( pCut->nVarsMax == 4 )
            Cut_CutWriteTruth( pCut, p->uTruthVars[0] );
        else
            Extra_BitCopy( pCut->nLeaves, p->uTruths[0], (uint8*)Cut_CutReadTruth(pCut) );
*/
        unsigned * pTruth = Cut_CutReadTruth(pCut);
        int i;
        for ( i = 0; i < p->nTruthWords; i++ )
            pTruth[i] = 0xAAAAAAAA;
    }
    p->nCutsTriv++;
    return pCut;
} 


/**Function*************************************************************

  Synopsis    [Print the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cut_CutPrint( Cut_Cut_t * pCut, int fSeq )
{
    int i;
    assert( pCut->nLeaves > 0 );
    printf( "%d : {", pCut->nLeaves );
    for ( i = 0; i < (int)pCut->nLeaves; i++ )
    {
        if ( fSeq )
        {
            printf( " %d", pCut->pLeaves[i] >> CUT_SHIFT );
            if ( pCut->pLeaves[i] & CUT_MASK )
                printf( "(%d)", pCut->pLeaves[i] & CUT_MASK );
        }
        else
            printf( " %d", pCut->pLeaves[i] );
    }
    printf( " }" );
//    printf( "\nSign = " );
//    Extra_PrintBinary( stdout, &pCut->uSign, 32 );
}

/**Function*************************************************************

  Synopsis    [Print the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cut_CutPrintList( Cut_Cut_t * pList, int fSeq )
{
    Cut_Cut_t * pCut;
    for ( pCut = pList; pCut; pCut = pCut->pNext )
        Cut_CutPrint( pCut, fSeq ), printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Consider dropping cuts if they are useless by now.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cut_CutPrintMerge( Cut_Cut_t * pCut, Cut_Cut_t * pCut0, Cut_Cut_t * pCut1 )
{
    printf( "\n" );
    printf( "%d : %5d %5d %5d %5d %5d\n", 
        pCut0->nLeaves, 
        pCut0->nLeaves > 0 ? pCut0->pLeaves[0] : -1, 
        pCut0->nLeaves > 1 ? pCut0->pLeaves[1] : -1, 
        pCut0->nLeaves > 2 ? pCut0->pLeaves[2] : -1, 
        pCut0->nLeaves > 3 ? pCut0->pLeaves[3] : -1, 
        pCut0->nLeaves > 4 ? pCut0->pLeaves[4] : -1
        );
    printf( "%d : %5d %5d %5d %5d %5d\n", 
        pCut1->nLeaves, 
        pCut1->nLeaves > 0 ? pCut1->pLeaves[0] : -1, 
        pCut1->nLeaves > 1 ? pCut1->pLeaves[1] : -1, 
        pCut1->nLeaves > 2 ? pCut1->pLeaves[2] : -1, 
        pCut1->nLeaves > 3 ? pCut1->pLeaves[3] : -1, 
        pCut1->nLeaves > 4 ? pCut1->pLeaves[4] : -1
        );
    if ( pCut == NULL )
        printf( "Cannot merge\n" );
    else
        printf( "%d : %5d %5d %5d %5d %5d\n", 
            pCut->nLeaves, 
            pCut->nLeaves > 0 ? pCut->pLeaves[0] : -1, 
            pCut->nLeaves > 1 ? pCut->pLeaves[1] : -1, 
            pCut->nLeaves > 2 ? pCut->pLeaves[2] : -1, 
            pCut->nLeaves > 3 ? pCut->pLeaves[3] : -1, 
            pCut->nLeaves > 4 ? pCut->pLeaves[4] : -1
            );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

