/**CFile****************************************************************

  FileName    [mapperCutUtils.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic technology mapping engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - June 1, 2004.]

  Revision    [$Id: mapperCutUtils.h,v 1.0 2003/09/08 00:00:00 alanmi Exp $]

***********************************************************************/

#include "mapperInt.h"

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
Map_Cut_t * Map_CutAlloc( Map_Man_t * p )
{
    Map_Cut_t * pCut;
    Map_Match_t * pMatch;
    pCut = (Map_Cut_t *)Extra_MmFixedEntryFetch( p->mmCuts );
    memset( pCut, 0, sizeof(Map_Cut_t) );

    pMatch = pCut->M;
    pMatch->AreaFlow       = MAP_FLOAT_LARGE; // unassigned
    pMatch->tArrive.Rise   = MAP_FLOAT_LARGE; // unassigned
    pMatch->tArrive.Fall   = MAP_FLOAT_LARGE; // unassigned
    pMatch->tArrive.Worst  = MAP_FLOAT_LARGE; // unassigned

    pMatch = pCut->M + 1;
    pMatch->AreaFlow       = MAP_FLOAT_LARGE; // unassigned
    pMatch->tArrive.Rise   = MAP_FLOAT_LARGE; // unassigned
    pMatch->tArrive.Fall   = MAP_FLOAT_LARGE; // unassigned
    pMatch->tArrive.Worst  = MAP_FLOAT_LARGE; // unassigned
    return pCut;
}

/**Function*************************************************************

  Synopsis    [Deallocates the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_CutFree( Map_Man_t * p, Map_Cut_t * pCut )
{
    if ( pCut )
    Extra_MmFixedEntryRecycle( p->mmCuts, (char *)pCut );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_CutPrint( Map_Man_t * p, Map_Node_t * pRoot, Map_Cut_t * pCut, int fPhase )
{
    int i;
    printf( "CUT:  Delay = (%4.2f, %4.2f). Area = %4.2f. Nodes = %d -> {", 
        pCut->M[fPhase].tArrive.Rise, pCut->M[fPhase].tArrive.Fall, pCut->M[fPhase].AreaFlow, pRoot->Num );
    for ( i = 0; i < pCut->nLeaves; i++ )
        printf( " %d", pCut->ppLeaves[i]->Num );
    printf( " } \n" );
}


/**function*************************************************************

  synopsis    [Computes the exact area associated with the cut.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
float Map_CutGetRootArea( Map_Cut_t * pCut, int fPhase )
{
    assert( pCut->M[fPhase].pSuperBest );
    return pCut->M[fPhase].pSuperBest->Area;
}

/**function*************************************************************

  synopsis    [Computes the exact area associated with the cut.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
int Map_CutGetLeafPhase( Map_Cut_t * pCut, int fPhase, int iLeaf )
{
    assert( pCut->M[fPhase].pSuperBest );
    return (( pCut->M[fPhase].uPhaseBest & (1<<iLeaf) ) == 0);
}

/**function*************************************************************

  synopsis    [Computes the exact area associated with the cut.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
int Map_NodeGetLeafPhase( Map_Node_t * pNode, int fPhase, int iLeaf )
{
    assert( pNode->pCutBest[fPhase]->M[fPhase].pSuperBest );
    return (( pNode->pCutBest[fPhase]->M[fPhase].uPhaseBest & (1<<iLeaf) ) == 0);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Map_Cut_t * Map_CutListAppend( Map_Cut_t * pSetAll, Map_Cut_t * pSets )
{
    Map_Cut_t * pPrev = NULL; // Suppress "might be used uninitialized"
    Map_Cut_t * pTemp;
    if ( pSetAll == NULL )
        return pSets;
    if ( pSets == NULL )
        return pSetAll;
    // find the last one
    for ( pTemp = pSets; pTemp; pTemp = pTemp->pNext )
        pPrev = pTemp;
    // append all the end of the current set
    assert( pPrev->pNext == NULL );
    pPrev->pNext = pSetAll;
    return pSets;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_CutListRecycle( Map_Man_t * p, Map_Cut_t * pSetList, Map_Cut_t * pSave )
{
    Map_Cut_t * pNext, * pTemp;
    for ( pTemp = pSetList, pNext = pTemp? pTemp->pNext : NULL; 
          pTemp; 
          pTemp = pNext, pNext = pNext? pNext->pNext : NULL )
        if ( pTemp != pSave )
            Extra_MmFixedEntryRecycle( p->mmCuts, (char *)pTemp );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_CutListCount( Map_Cut_t * pSets )
{
    Map_Cut_t * pTemp;
    int i;
    for ( i = 0, pTemp = pSets; pTemp; pTemp = pTemp->pNext, i++ );
    return i;
}

#if 0

/**function*************************************************************

  synopsis    [Removes the fanouts of the cut.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
void Map_CutRemoveFanouts( Map_Node_t * pNode, Map_Cut_t * pCut, int fPhase )
{
    Map_NodeVec_t * vFanouts;
    int i, k;
    for ( i = 0; i < pCut->nLeaves; i++ )
    {
        vFanouts = pCut->ppLeaves[i]->vFanouts;
        for ( k = 0; k < vFanouts->nSize; k++ )
            if ( vFanouts->pArray[k] == pNode )
                break;
        assert( k != vFanouts->nSize );
        for ( k++; k < vFanouts->nSize; k++ )
            vFanouts->pArray[k-1] = vFanouts->pArray[k];
        vFanouts->nSize--;
    }
}

/**function*************************************************************

  synopsis    [Removes the fanouts of the cut.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
void Map_CutInsertFanouts( Map_Node_t * pNode, Map_Cut_t * pCut, int fPhase )
{
    int i;
    for ( i = 0; i < pCut->nLeaves; i++ )
        Map_NodeVecPush( pCut->ppLeaves[i]->vFanouts, pNode );
}

#endif


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

