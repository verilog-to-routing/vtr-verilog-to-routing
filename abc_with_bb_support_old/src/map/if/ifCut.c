/**CFile****************************************************************

  FileName    [ifCut.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [FPGA mapping based on priority cuts.]

  Synopsis    [Cut computation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 21, 2006.]

  Revision    [$Id: ifCut.c,v 1.00 2006/11/21 00:00:00 alanmi Exp $]

***********************************************************************/

#include "if.h"

static int wiremap = 1; // set to 1 for WIREMAP janders

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Returns 1 if pDom is contained in pCut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int If_CutCheckDominance( If_Cut_t * pDom, If_Cut_t * pCut )
{
    int i, k;
    for ( i = 0; i < (int)pDom->nLeaves; i++ )
    {
        for ( k = 0; k < (int)pCut->nLeaves; k++ )
            if ( pDom->pLeaves[i] == pCut->pLeaves[k] )
                break;
        if ( k == (int)pCut->nLeaves ) // node i in pDom is not contained in pCut
            return 0;
    }
    // every node in pDom is contained in pCut
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if pDom is equal to pCut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int If_CutCheckEquality( If_Cut_t * pDom, If_Cut_t * pCut )
{
    int i;
    if ( (int)pDom->nLeaves != (int)pCut->nLeaves )
        return 0;
    for ( i = 0; i < (int)pDom->nLeaves; i++ )
        if ( pDom->pLeaves[i] != pCut->pLeaves[i] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the cut is contained.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_CutFilter( If_Set_t * pCutSet, If_Cut_t * pCut )
{ 
    If_Cut_t * pTemp;
    int i, k;
    assert( pCutSet->ppCuts[pCutSet->nCuts] == pCut );
    for ( i = 0; i < pCutSet->nCuts; i++ )
    {
        pTemp = pCutSet->ppCuts[i];
        if ( pTemp->nLeaves > pCut->nLeaves )
        {
            // do not fiter the first cut
            if ( i == 0 )
                continue;
            // skip the non-contained cuts
            if ( (pTemp->uSign & pCut->uSign) != pCut->uSign )
                continue;
            // check containment seriously
            if ( If_CutCheckDominance( pCut, pTemp ) )
            {
//                p->ppCuts[i] = p->ppCuts[p->nCuts-1];
//                p->ppCuts[p->nCuts-1] = pTemp;
//                p->nCuts--;
//                i--;
                // remove contained cut
                for ( k = i; k < pCutSet->nCuts; k++ )
                    pCutSet->ppCuts[k] = pCutSet->ppCuts[k+1];
                pCutSet->ppCuts[pCutSet->nCuts] = pTemp;
                pCutSet->nCuts--;
                i--;
            }
         }
        else
        {
            // skip the non-contained cuts
            if ( (pTemp->uSign & pCut->uSign) != pTemp->uSign )
                continue;
            // check containment seriously
            if ( If_CutCheckDominance( pTemp, pCut ) )
                return 1;
        }
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Merges two cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int If_CutMergeOrdered( If_Cut_t * pC0, If_Cut_t * pC1, If_Cut_t * pC )
{ 
    int i, k, c;
    assert( pC0->nLeaves >= pC1->nLeaves );
    // the case of the largest cut sizes
    if ( pC0->nLeaves == pC->nLimit && pC1->nLeaves == pC->nLimit )
    {
        for ( i = 0; i < (int)pC0->nLeaves; i++ )
            if ( pC0->pLeaves[i] != pC1->pLeaves[i] )
                return 0;
        for ( i = 0; i < (int)pC0->nLeaves; i++ )
            pC->pLeaves[i] = pC0->pLeaves[i];
        pC->nLeaves = pC0->nLeaves;
        return 1;
    }
    // the case when one of the cuts is the largest
    if ( pC0->nLeaves == pC->nLimit )
    {
        for ( i = 0; i < (int)pC1->nLeaves; i++ )
        {
            for ( k = (int)pC0->nLeaves - 1; k >= 0; k-- )
                if ( pC0->pLeaves[k] == pC1->pLeaves[i] )
                    break;
            if ( k == -1 ) // did not find
                return 0;
        }
        for ( i = 0; i < (int)pC0->nLeaves; i++ )
            pC->pLeaves[i] = pC0->pLeaves[i];
        pC->nLeaves = pC0->nLeaves;
        return 1;
    }

    // compare two cuts with different numbers
    i = k = 0;
    for ( c = 0; c < (int)pC->nLimit; c++ )
    {
        if ( k == (int)pC1->nLeaves )
        {
            if ( i == (int)pC0->nLeaves )
            {
                pC->nLeaves = c;
                return 1;
            }
            pC->pLeaves[c] = pC0->pLeaves[i++];
            continue;
        }
        if ( i == (int)pC0->nLeaves )
        {
            if ( k == (int)pC1->nLeaves )
            {
                pC->nLeaves = c;
                return 1;
            }
            pC->pLeaves[c] = pC1->pLeaves[k++];
            continue;
        }
        if ( pC0->pLeaves[i] < pC1->pLeaves[k] )
        {
            pC->pLeaves[c] = pC0->pLeaves[i++];
            continue;
        }
        if ( pC0->pLeaves[i] > pC1->pLeaves[k] )
        {
            pC->pLeaves[c] = pC1->pLeaves[k++];
            continue;
        }
        pC->pLeaves[c] = pC0->pLeaves[i++]; 
        k++;
    }
    if ( i < (int)pC0->nLeaves || k < (int)pC1->nLeaves )
        return 0;
    pC->nLeaves = c;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Merges two cuts.]

  Description [Special case when the cut is known to exist.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int If_CutMergeOrdered2( If_Cut_t * pC0, If_Cut_t * pC1, If_Cut_t * pC )
{ 
    int i, k, c;
    assert( pC0->nLeaves >= pC1->nLeaves );
    // copy the first cut
    for ( i = 0; i < (int)pC0->nLeaves; i++ )
        pC->pLeaves[i] = pC0->pLeaves[i];
    pC->nLeaves = pC0->nLeaves;
    // the case when one of the cuts is the largest
    if ( pC0->nLeaves == pC->nLimit )
        return 1;
    // add nodes of the second cut
    k = 0;
    for ( i = 0; i < (int)pC1->nLeaves; i++ )
    {
        // find k-th node before which i-th node should be added
        for ( ; k < (int)pC->nLeaves; k++ )
            if ( pC->pLeaves[k] >= pC1->pLeaves[i] )
                break;
        // check the case when this should be the last node
        if ( k == (int)pC->nLeaves )
        {
            pC->pLeaves[k++] = pC1->pLeaves[i];
            pC->nLeaves++;
            continue;
        }
        // check the case when equal node is found
        if ( pC1->pLeaves[i] == pC->pLeaves[k] )
            continue;
        // add the node
        for ( c = (int)pC->nLeaves; c > k; c-- )
            pC->pLeaves[c] = pC->pLeaves[c-1];
        pC->pLeaves[k++] = pC1->pLeaves[i];
        pC->nLeaves++;
    }
/*
    assert( pC->nLeaves <= pC->nLimit );
    for ( i = 1; i < (int)pC->nLeaves; i++ )
        assert( pC->pLeaves[i-1] < pC->pLeaves[i] );
*/
    return 1;
}

/**Function*************************************************************

  Synopsis    [Prepares the object for FPGA mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_CutMerge( If_Cut_t * pCut0, If_Cut_t * pCut1, If_Cut_t * pCut )
{ 
    assert( pCut->nLimit > 0 );
    // merge the nodes
    if ( pCut0->nLeaves < pCut1->nLeaves )
    {
        if ( !If_CutMergeOrdered( pCut1, pCut0, pCut ) )
            return 0;
    }
    else
    {
        if ( !If_CutMergeOrdered( pCut0, pCut1, pCut ) )
            return 0;
    }
    pCut->uSign = pCut0->uSign | pCut1->uSign;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Prepares the object for FPGA mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_CutCompareDelay( If_Cut_t ** ppC0, If_Cut_t ** ppC1 )
{
    If_Cut_t * pC0 = *ppC0;
    If_Cut_t * pC1 = *ppC1;
    if ( pC0->Delay < pC1->Delay - 0.0001 )
        return -1;
    if ( pC0->Delay > pC1->Delay + 0.0001 )
        return 1;
    if ( pC0->nLeaves < pC1->nLeaves )
        return -1;
    if ( pC0->nLeaves > pC1->nLeaves )
        return 1;
    if ( pC0->Area < pC1->Area - 0.0001 )
        return -1;
    if ( pC0->Area > pC1->Area + 0.0001 )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Prepares the object for FPGA mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_CutCompareDelayOld( If_Cut_t ** ppC0, If_Cut_t ** ppC1 )
{
    If_Cut_t * pC0 = *ppC0;
    If_Cut_t * pC1 = *ppC1;
    if ( pC0->Delay < pC1->Delay - 0.0001 )
        return -1;
    if ( pC0->Delay > pC1->Delay + 0.0001 )
        return 1;
    if ( pC0->Area < pC1->Area - 0.0001 )
        return -1;
    if ( pC0->Area > pC1->Area + 0.0001 )
        return 1;
    if ( pC0->nLeaves < pC1->nLeaves )
        return -1;
    if ( pC0->nLeaves > pC1->nLeaves )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Prepares the object for FPGA mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_CutCompareArea( If_Cut_t ** ppC0, If_Cut_t ** ppC1 )
{
    If_Cut_t * pC0 = *ppC0;
    If_Cut_t * pC1 = *ppC1;
    if ( pC0->Area < pC1->Area - 0.0001 )
        return -1;
    if ( pC0->Area > pC1->Area + 0.0001 )
        return 1;
    if ( pC0->AveRefs > pC1->AveRefs )
        return -1;
    if ( pC0->AveRefs < pC1->AveRefs )
        return 1;
    if ( pC0->nLeaves < pC1->nLeaves )
        return -1;
    if ( pC0->nLeaves > pC1->nLeaves )
        return 1;
    if ( pC0->Delay < pC1->Delay - 0.0001 )
        return -1;
    if ( pC0->Delay > pC1->Delay + 0.0001 )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Sorts the cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManSortCuts( If_Man_t * p, int Mode )
{
/*
    // sort the cuts
    if ( Mode || p->pPars->fArea ) // area
        qsort( p->ppCuts, p->nCuts, sizeof(If_Cut_t *), (int (*)(const void *, const void *))If_CutCompareArea );
    else if ( p->pPars->fFancy )
        qsort( p->ppCuts, p->nCuts, sizeof(If_Cut_t *), (int (*)(const void *, const void *))If_CutCompareDelayOld );
    else
        qsort( p->ppCuts, p->nCuts, sizeof(If_Cut_t *), (int (*)(const void *, const void *))If_CutCompareDelay );
*/
}

/**Function*************************************************************

  Synopsis    [Comparison function for two cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int If_ManSortCompare( If_Man_t * p, If_Cut_t * pC0, If_Cut_t * pC1 )
{
    if ( p->SortMode == 1 ) // area
    {
        if ( pC0->Area < pC1->Area - 0.0001 )
            return -1;
        if ( pC0->Area > pC1->Area + 0.0001 )
            return 1;
        if ( pC0->AveRefs > pC1->AveRefs )
            return -1;
        if ( pC0->AveRefs < pC1->AveRefs )
            return 1;
        if ( pC0->nLeaves < pC1->nLeaves )
            return -1;
        if ( pC0->nLeaves > pC1->nLeaves )
            return 1;
        if ( pC0->Delay < pC1->Delay - 0.0001 )
            return -1;
        if ( pC0->Delay > pC1->Delay + 0.0001 )
            return 1;
        return 0;
    }
    if ( p->SortMode == 0 ) // delay
    {
        if ( pC0->Delay < pC1->Delay - 0.0001 )
            return -1;
        if ( pC0->Delay > pC1->Delay + 0.0001 )
            return 1;
        if ( pC0->nLeaves < pC1->nLeaves )
            return -1;
        if ( pC0->nLeaves > pC1->nLeaves )
            return 1;
        if ( pC0->Area < pC1->Area - 0.0001 )
            return -1;
        if ( pC0->Area > pC1->Area + 0.0001 )
            return 1;
        return 0;
    }
    assert( p->SortMode == 2 ); // delay old
    if ( pC0->Delay < pC1->Delay - 0.0001 )
        return -1;
    if ( pC0->Delay > pC1->Delay + 0.0001 )
        return 1;
    if ( pC0->Area < pC1->Area - 0.0001 )
        return -1;
    if ( pC0->Area > pC1->Area + 0.0001 )
        return 1;
    if ( pC0->nLeaves < pC1->nLeaves )
        return -1;
    if ( pC0->nLeaves > pC1->nLeaves )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Performs incremental sorting of cuts.]

  Description [Currently only the trivial sorting is implemented.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_CutSort( If_Man_t * p, If_Set_t * pCutSet, If_Cut_t * pCut )
{
//    int Counter = 0;
    int i;

    // the new cut is the last one
    assert( pCutSet->ppCuts[pCutSet->nCuts] == pCut );
    assert( pCutSet->nCuts <= pCutSet->nCutsMax );

    // cut structure is empty
    if ( pCutSet->nCuts == 0 )
    {
        pCutSet->nCuts++;
        return;
    }

    // the cut will be added - find its place
    for ( i = pCutSet->nCuts-1; i >= 0; i-- )
    {
//        Counter++;
        if ( If_ManSortCompare( p, pCutSet->ppCuts[i], pCut ) <= 0 )
            break;
        pCutSet->ppCuts[i+1] = pCutSet->ppCuts[i];
        pCutSet->ppCuts[i] = pCut;
    }
//    printf( "%d ", Counter );

    // update the number of cuts
    if ( pCutSet->nCuts < pCutSet->nCutsMax )
        pCutSet->nCuts++;
}

/**Function*************************************************************

  Synopsis    [Computes area flow.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float If_CutFlow( If_Man_t * p, If_Cut_t * pCut )
{
    If_Obj_t * pLeaf;
    float Flow;
    int i;
    assert( p->pPars->fSeqMap || pCut->nLeaves > 1 );
    Flow = If_CutLutArea(p, pCut);
    if (wiremap) Flow += pCut->nLeaves*0.01; // janders
    If_CutForEachLeaf( p, pCut, pLeaf, i )
    {
      if ( pLeaf->nRefs == 0 ) {
            Flow += If_ObjCutBest(pLeaf)->Area;
	    if (wiremap) Flow += 0.01*If_ObjCutBest(pLeaf)->nLeaves; // janders
      }
        else if ( p->pPars->fSeqMap ) // seq
            Flow += If_ObjCutBest(pLeaf)->Area / pLeaf->nRefs;
        else 
        {
            assert( pLeaf->EstRefs > p->fEpsilon );
            Flow += If_ObjCutBest(pLeaf)->Area / pLeaf->EstRefs;
	    if (wiremap) Flow += (0.01*If_ObjCutBest(pLeaf)->nLeaves / pLeaf->EstRefs); // janders
        }
    }
    return Flow;
}

/**Function*************************************************************

  Synopsis    [Average number of references of the leaves.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float If_CutAverageRefs( If_Man_t * p, If_Cut_t * pCut )
{
    If_Obj_t * pLeaf;
    int nRefsTotal, i;
    assert( p->pPars->fSeqMap || pCut->nLeaves > 1 );
    nRefsTotal = 0;
    If_CutForEachLeaf( p, pCut, pLeaf, i )
        nRefsTotal += pLeaf->nRefs;
    return ((float)nRefsTotal)/pCut->nLeaves;
}

/**Function*************************************************************

  Synopsis    [Computes area of the first level.]

  Description [The cut need to be derefed.]
               
  SideEffects []

  SeeAlso     []
 
***********************************************************************/
float If_CutDeref( If_Man_t * p, If_Cut_t * pCut, int nLevels )
{
    If_Obj_t * pLeaf;
    float Area;
    int i;
    Area = If_CutLutArea(p, pCut);
    If_CutForEachLeaf( p, pCut, pLeaf, i )
    {
        assert( pLeaf->nRefs > 0 );
        if ( --pLeaf->nRefs > 0 || !If_ObjIsAnd(pLeaf) || nLevels == 1 )
            continue;
        Area += If_CutDeref( p, If_ObjCutBest(pLeaf), nLevels - 1 );
    }
    return Area;
}

/**Function*************************************************************

  Synopsis    [Computes area of the first level.]

  Description [The cut need to be derefed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float If_CutRef( If_Man_t * p, If_Cut_t * pCut, int nLevels )
{
    If_Obj_t * pLeaf;
    float Area;
    int i;
    Area = If_CutLutArea(p, pCut);
    If_CutForEachLeaf( p, pCut, pLeaf, i )
    {
        assert( pLeaf->nRefs >= 0 );
        if ( pLeaf->nRefs++ > 0 || !If_ObjIsAnd(pLeaf) || nLevels == 1 )
            continue;
        Area += If_CutRef( p, If_ObjCutBest(pLeaf), nLevels - 1 );
    }
    return Area;
}

float If_CutDerefEdge( If_Man_t * p, If_Cut_t * pCut, int nLevels )
{
    If_Obj_t * pLeaf;
    float Area;
    int i;
    Area = pCut->nLeaves;
    If_CutForEachLeaf( p, pCut, pLeaf, i )
    {
        assert( pLeaf->nRefs > 0 );
        if ( --pLeaf->nRefs > 0 || !If_ObjIsAnd(pLeaf) || nLevels == 1 )
            continue;
        Area += If_CutDeref( p, If_ObjCutBest(pLeaf), nLevels - 1 );
    }
    return Area;
}


float If_CutRefEdge( If_Man_t * p, If_Cut_t * pCut, int nLevels )
{
    If_Obj_t * pLeaf;
    float Area;
    int i;
    Area = pCut->nLeaves;
    If_CutForEachLeaf( p, pCut, pLeaf, i )
    {
        assert( pLeaf->nRefs >= 0 );
        if ( pLeaf->nRefs++ > 0 || !If_ObjIsAnd(pLeaf) || nLevels == 1 )
            continue;
        Area += If_CutRef( p, If_ObjCutBest(pLeaf), nLevels - 1 );
    }
    return Area;
}

/**Function*************************************************************

  Synopsis    [Prints one cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_CutPrint( If_Man_t * p, If_Cut_t * pCut )
{
    unsigned i;
    printf( "{" );
    for ( i = 0; i < pCut->nLeaves; i++ )
        printf( " %d", pCut->pLeaves[i] );
    printf( " }\n" );
}

/**Function*************************************************************

  Synopsis    [Prints one cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_CutPrintTiming( If_Man_t * p, If_Cut_t * pCut )
{
    If_Obj_t * pLeaf;
    unsigned i;
    printf( "{" );
    If_CutForEachLeaf( p, pCut, pLeaf, i )
        printf( " %d(%.2f/%.2f)", pLeaf->Id, If_ObjCutBest(pLeaf)->Delay, pLeaf->Required );
    printf( " }\n" );
}

/**Function*************************************************************

  Synopsis    [Computes area of the first level.]

  Description [The cut need to be derefed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float If_CutAreaDerefed( If_Man_t * p, If_Cut_t * pCut, int nLevels )
{
    float aResult, aResult2;
    assert( p->pPars->fSeqMap || pCut->nLeaves > 1 );
    aResult2 = If_CutRef( p, pCut, nLevels );
    aResult  = If_CutDeref( p, pCut, nLevels );
    assert( aResult > aResult2 - p->fEpsilon );
    assert( aResult < aResult2 + p->fEpsilon );
    if (wiremap) { // janders
      aResult += 0.01*If_CutRefEdge(p, pCut, nLevels); 
      If_CutDerefEdge(p, pCut, nLevels); 
    }
    return aResult;
}

/**Function*************************************************************

  Synopsis    [Computes area of the first level.]

  Description [The cut need to be derefed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float If_CutAreaRefed( If_Man_t * p, If_Cut_t * pCut, int nLevels )
{
    float aResult, aResult2;
    assert( p->pPars->fSeqMap || pCut->nLeaves > 1 );
    aResult2 = If_CutDeref( p, pCut, nLevels );
    aResult  = If_CutRef( p, pCut, nLevels );
    assert( aResult > aResult2 - p->fEpsilon );
    assert( aResult < aResult2 + p->fEpsilon );
    return aResult;
}

/**Function*************************************************************

  Synopsis    [Moves the cut over the latch.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_CutLift( If_Cut_t * pCut )
{
    unsigned i;
    for ( i = 0; i < pCut->nLeaves; i++ )
    {
        assert( (pCut->pLeaves[i] & 255) < 255 );
        pCut->pLeaves[i]++;
    }
}

/**Function*************************************************************

  Synopsis    [Computes area of the first level.]

  Description [The cut need to be derefed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_CutCopy( If_Man_t * p, If_Cut_t * pCutDest, If_Cut_t * pCutSrc )
{
    int * pLeaves;
    char * pPerm;
    unsigned * pTruth;
    // save old arrays
    pLeaves = pCutDest->pLeaves;
    pPerm   = pCutDest->pPerm;
    pTruth  = pCutDest->pTruth;
    // copy the cut info
    memcpy( pCutDest, pCutSrc, p->nCutBytes );
    // restore the arrays
    pCutDest->pLeaves = pLeaves;
    pCutDest->pPerm   = pPerm;
    pCutDest->pTruth  = pTruth;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


