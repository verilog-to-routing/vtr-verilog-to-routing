/**CFile****************************************************************

  FileName    [fpgaVec.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Technology mapping for variable-size-LUT FPGAs.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - August 18, 2004.]

  Revision    [$Id: fpgaVec.c,v 1.3 2005/01/23 06:59:42 alanmi Exp $]

***********************************************************************/

#include "fpgaInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Fpga_NodeVecCompareLevels( Fpga_Node_t ** pp1, Fpga_Node_t ** pp2 );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_NodeVec_t * Fpga_NodeVecAlloc( int nCap )
{
    Fpga_NodeVec_t * p;
    p = ABC_ALLOC( Fpga_NodeVec_t, 1 );
    if ( nCap > 0 && nCap < 16 )
        nCap = 16;
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ABC_ALLOC( Fpga_Node_t *, p->nCap ) : NULL;
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_NodeVecFree( Fpga_NodeVec_t * p )
{
    ABC_FREE( p->pArray );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Node_t ** Fpga_NodeVecReadArray( Fpga_NodeVec_t * p )
{
    return p->pArray;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_NodeVecReadSize( Fpga_NodeVec_t * p )
{
    return p->nSize;
}

/**Function*************************************************************

  Synopsis    [Resizes the vector to the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_NodeVecGrow( Fpga_NodeVec_t * p, int nCapMin )
{
    if ( p->nCap >= nCapMin )
        return;
    p->pArray = ABC_REALLOC( Fpga_Node_t *, p->pArray, nCapMin ); 
    p->nCap   = nCapMin;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_NodeVecShrink( Fpga_NodeVec_t * p, int nSizeNew )
{
    assert( p->nSize >= nSizeNew );
    p->nSize = nSizeNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_NodeVecClear( Fpga_NodeVec_t * p )
{
    p->nSize = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_NodeVecPush( Fpga_NodeVec_t * p, Fpga_Node_t * Entry )
{
    if ( p->nSize == p->nCap )
    {
        if ( p->nCap < 16 )
            Fpga_NodeVecGrow( p, 16 );
        else
            Fpga_NodeVecGrow( p, 2 * p->nCap );
    }
    p->pArray[p->nSize++] = Entry;
}

/**Function*************************************************************

  Synopsis    [Add the element while ensuring uniqueness.]

  Description [Returns 1 if the element was found, and 0 if it was new. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_NodeVecPushUnique( Fpga_NodeVec_t * p, Fpga_Node_t * Entry )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        if ( p->pArray[i] == Entry )
            return 1;
    Fpga_NodeVecPush( p, Entry );
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Node_t * Fpga_NodeVecPop( Fpga_NodeVec_t * p )
{
    return p->pArray[--p->nSize];
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_NodeVecWriteEntry( Fpga_NodeVec_t * p, int i, Fpga_Node_t * Entry )
{
    assert( i >= 0 && i < p->nSize );
    p->pArray[i] = Entry;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Node_t * Fpga_NodeVecReadEntry( Fpga_NodeVec_t * p, int i )
{
    assert( i >= 0 && i < p->nSize );
    return p->pArray[i];
}

/**Function*************************************************************

  Synopsis    [Comparison procedure for two nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_NodeVecCompareLevels( Fpga_Node_t ** pp1, Fpga_Node_t ** pp2 )
{
    int Level1 = Fpga_Regular(*pp1)->Level;
    int Level2 = Fpga_Regular(*pp2)->Level;
    if ( Level1 < Level2 )
        return -1;
    if ( Level1 > Level2 )
        return 1;
    if ( Fpga_Regular(*pp1)->Num < Fpga_Regular(*pp2)->Num )
        return -1;
    if ( Fpga_Regular(*pp1)->Num > Fpga_Regular(*pp2)->Num )
        return 1;
    return 0; 
}

/**Function*************************************************************

  Synopsis    [Sorting the entries by their integer value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_NodeVecSortByLevel( Fpga_NodeVec_t * p )
{
    qsort( (void *)p->pArray, p->nSize, sizeof(Fpga_Node_t *), 
            (int (*)(const void *, const void *)) Fpga_NodeVecCompareLevels );
}

/**Function*************************************************************

  Synopsis    [Compares the arrival times.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_NodeVecCompareArrivals( Fpga_Node_t ** ppS1, Fpga_Node_t ** ppS2 )
{
    if ( (*ppS1)->pCutBest->tArrival < (*ppS2)->pCutBest->tArrival )
        return -1;
    if ( (*ppS1)->pCutBest->tArrival > (*ppS2)->pCutBest->tArrival )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Orders the nodes in the increasing order of the arrival times.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_SortNodesByArrivalTimes( Fpga_NodeVec_t * p )
{
    qsort( (void *)p->pArray, p->nSize, sizeof(Fpga_Node_t *), 
            (int (*)(const void *, const void *)) Fpga_NodeVecCompareArrivals );
//    assert( Fpga_CompareNodesByLevel( p->pArray, p->pArray + p->nSize - 1 ) <= 0 );
}


/**Function*************************************************************

  Synopsis    [Computes the union of nodes in two arrays.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_NodeVecUnion( Fpga_NodeVec_t * p, Fpga_NodeVec_t * p1, Fpga_NodeVec_t * p2 )
{
    int i;
    Fpga_NodeVecClear( p );
    for ( i = 0; i < p1->nSize; i++ )
        Fpga_NodeVecPush( p, p1->pArray[i] );
    for ( i = 0; i < p2->nSize; i++ )
        Fpga_NodeVecPush( p, p2->pArray[i] );
}

/**Function*************************************************************

  Synopsis    [Inserts a new node in the order by arrival times.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_NodeVecPushOrder( Fpga_NodeVec_t * vNodes, Fpga_Node_t * pNode, int fIncreasing )
{
    Fpga_Node_t * pNode1, * pNode2;
    int i;
    Fpga_NodeVecPush( vNodes, pNode );
    // find the place of the node
    for ( i = vNodes->nSize-1; i > 0; i-- )
    {
        pNode1 = vNodes->pArray[i  ];
        pNode2 = vNodes->pArray[i-1];
        if (( fIncreasing && pNode1->pCutBest->tArrival >= pNode2->pCutBest->tArrival) ||
            (!fIncreasing && pNode1->pCutBest->tArrival <= pNode2->pCutBest->tArrival) )
            break;
        vNodes->pArray[i  ] = pNode2;
        vNodes->pArray[i-1] = pNode1;
    }
}

/**Function*************************************************************

  Synopsis    [Inserts a new node in the order by arrival times.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_NodeVecReverse( Fpga_NodeVec_t * vNodes )
{
    Fpga_Node_t * pNode1, * pNode2;
    int i;
    for ( i = 0; i < vNodes->nSize/2; i++ )
    {
        pNode1 = vNodes->pArray[i];
        pNode2 = vNodes->pArray[vNodes->nSize-1-i];
        vNodes->pArray[i]                 = pNode2;
        vNodes->pArray[vNodes->nSize-1-i] = pNode1;
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END

