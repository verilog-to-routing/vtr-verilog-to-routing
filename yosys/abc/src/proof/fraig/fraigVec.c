/**CFile****************************************************************

  FileName    [fraigVec.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [Vector of FRAIG nodes.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - October 1, 2004]

  Revision    [$Id: fraigVec.c,v 1.7 2005/07/08 01:01:34 alanmi Exp $]

***********************************************************************/

#include "fraigInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_NodeVec_t * Fraig_NodeVecAlloc( int nCap )
{
    Fraig_NodeVec_t * p;
    p = ABC_ALLOC( Fraig_NodeVec_t, 1 );
    if ( nCap > 0 && nCap < 8 )
        nCap = 8;
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ABC_ALLOC( Fraig_Node_t *, p->nCap ) : NULL;
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_NodeVecFree( Fraig_NodeVec_t * p )
{
    ABC_FREE( p->pArray );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Duplicates the integer array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_NodeVec_t * Fraig_NodeVecDup( Fraig_NodeVec_t * pVec )
{
    Fraig_NodeVec_t * p;
    p = ABC_ALLOC( Fraig_NodeVec_t, 1 );
    p->nSize  = pVec->nSize;
    p->nCap   = pVec->nCap;
    p->pArray = p->nCap? ABC_ALLOC( Fraig_Node_t *, p->nCap ) : NULL;
    memcpy( p->pArray, pVec->pArray, sizeof(Fraig_Node_t *) * pVec->nSize );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t ** Fraig_NodeVecReadArray( Fraig_NodeVec_t * p )
{
    return p->pArray;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_NodeVecReadSize( Fraig_NodeVec_t * p )
{
    return p->nSize;
}

/**Function*************************************************************

  Synopsis    [Resizes the vector to the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_NodeVecGrow( Fraig_NodeVec_t * p, int nCapMin )
{
    if ( p->nCap >= nCapMin )
        return;
    p->pArray = ABC_REALLOC( Fraig_Node_t *, p->pArray, nCapMin ); 
    p->nCap   = nCapMin;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_NodeVecShrink( Fraig_NodeVec_t * p, int nSizeNew )
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
void Fraig_NodeVecClear( Fraig_NodeVec_t * p )
{
    p->nSize = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_NodeVecPush( Fraig_NodeVec_t * p, Fraig_Node_t * Entry )
{
    if ( p->nSize == p->nCap )
    {
        if ( p->nCap < 16 )
            Fraig_NodeVecGrow( p, 16 );
        else
            Fraig_NodeVecGrow( p, 2 * p->nCap );
    }
    p->pArray[p->nSize++] = Entry;
}

/**Function*************************************************************

  Synopsis    [Add the element while ensuring uniqueness.]

  Description [Returns 1 if the element was found, and 0 if it was new. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_NodeVecPushUnique( Fraig_NodeVec_t * p, Fraig_Node_t * Entry )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        if ( p->pArray[i] == Entry )
            return 1;
    Fraig_NodeVecPush( p, Entry );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Inserts a new node in the order by arrival times.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_NodeVecPushOrder( Fraig_NodeVec_t * p, Fraig_Node_t * pNode )
{
    Fraig_Node_t * pNode1, * pNode2;
    int i;
    Fraig_NodeVecPush( p, pNode );
    // find the p of the node
    for ( i = p->nSize-1; i > 0; i-- )
    {
        pNode1 = p->pArray[i  ];
        pNode2 = p->pArray[i-1];
        if ( pNode1 >= pNode2 )
            break;
        p->pArray[i  ] = pNode2;
        p->pArray[i-1] = pNode1;
    }
}

/**Function*************************************************************

  Synopsis    [Add the element while ensuring uniqueness in the order.]

  Description [Returns 1 if the element was found, and 0 if it was new. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_NodeVecPushUniqueOrder( Fraig_NodeVec_t * p, Fraig_Node_t * pNode )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        if ( p->pArray[i] == pNode )
            return 1;
    Fraig_NodeVecPushOrder( p, pNode );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Inserts a new node in the order by arrival times.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_NodeVecPushOrderByLevel( Fraig_NodeVec_t * p, Fraig_Node_t * pNode )
{
    Fraig_Node_t * pNode1, * pNode2;
    int i;
    Fraig_NodeVecPush( p, pNode );
    // find the p of the node
    for ( i = p->nSize-1; i > 0; i-- )
    {
        pNode1 = p->pArray[i  ];
        pNode2 = p->pArray[i-1];
        if ( Fraig_Regular(pNode1)->Level <= Fraig_Regular(pNode2)->Level )
            break;
        p->pArray[i  ] = pNode2;
        p->pArray[i-1] = pNode1;
    }
}

/**Function*************************************************************

  Synopsis    [Add the element while ensuring uniqueness in the order.]

  Description [Returns 1 if the element was found, and 0 if it was new. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_NodeVecPushUniqueOrderByLevel( Fraig_NodeVec_t * p, Fraig_Node_t * pNode )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        if ( p->pArray[i] == pNode )
            return 1;
    Fraig_NodeVecPushOrderByLevel( p, pNode );
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t * Fraig_NodeVecPop( Fraig_NodeVec_t * p )
{
    return p->pArray[--p->nSize];
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_NodeVecRemove( Fraig_NodeVec_t * p, Fraig_Node_t * Entry )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        if ( p->pArray[i] == Entry )
            break;
    assert( i < p->nSize );
    for ( i++; i < p->nSize; i++ )
        p->pArray[i-1] = p->pArray[i];
    p->nSize--;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_NodeVecWriteEntry( Fraig_NodeVec_t * p, int i, Fraig_Node_t * Entry )
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
Fraig_Node_t * Fraig_NodeVecReadEntry( Fraig_NodeVec_t * p, int i )
{
    assert( i >= 0 && i < p->nSize );
    return p->pArray[i];
}

/**Function*************************************************************

  Synopsis    [Comparison procedure for two clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_NodeVecCompareLevelsIncreasing( Fraig_Node_t ** pp1, Fraig_Node_t ** pp2 )
{
    int Level1 = Fraig_Regular(*pp1)->Level;
    int Level2 = Fraig_Regular(*pp2)->Level;
    if ( Level1 < Level2 )
        return -1;
    if ( Level1 > Level2 )
        return 1;
    return 0; 
}

/**Function*************************************************************

  Synopsis    [Comparison procedure for two clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_NodeVecCompareLevelsDecreasing( Fraig_Node_t ** pp1, Fraig_Node_t ** pp2 )
{
    int Level1 = Fraig_Regular(*pp1)->Level;
    int Level2 = Fraig_Regular(*pp2)->Level;
    if ( Level1 > Level2 )
        return -1;
    if ( Level1 < Level2 )
        return 1;
    return 0; 
}

/**Function*************************************************************

  Synopsis    [Comparison procedure for two clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_NodeVecCompareNumbers( Fraig_Node_t ** pp1, Fraig_Node_t ** pp2 )
{
    int Num1 = Fraig_Regular(*pp1)->Num;
    int Num2 = Fraig_Regular(*pp2)->Num;
    if ( Num1 < Num2 )
        return -1;
    if ( Num1 > Num2 )
        return 1;
    return 0; 
}

/**Function*************************************************************

  Synopsis    [Comparison procedure for two clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_NodeVecCompareRefCounts( Fraig_Node_t ** pp1, Fraig_Node_t ** pp2 )
{
    int nRefs1 = Fraig_Regular(*pp1)->nRefs;
    int nRefs2 = Fraig_Regular(*pp2)->nRefs;

    if ( nRefs1 < nRefs2 )
        return -1;
    if ( nRefs1 > nRefs2 )
        return 1;

    nRefs1 = Fraig_Regular(*pp1)->Level;
    nRefs2 = Fraig_Regular(*pp2)->Level;

    if ( nRefs1 < nRefs2 )
        return -1;
    if ( nRefs1 > nRefs2 )
        return 1;
    return 0; 
}

/**Function*************************************************************

  Synopsis    [Sorting the entries by their integer value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_NodeVecSortByLevel( Fraig_NodeVec_t * p, int fIncreasing )
{
    if ( fIncreasing )
        qsort( (void *)p->pArray, (size_t)p->nSize, sizeof(Fraig_Node_t *), 
                (int (*)(const void *, const void *)) Fraig_NodeVecCompareLevelsIncreasing );
    else 
        qsort( (void *)p->pArray, (size_t)p->nSize, sizeof(Fraig_Node_t *), 
                (int (*)(const void *, const void *)) Fraig_NodeVecCompareLevelsDecreasing );
}

/**Function*************************************************************

  Synopsis    [Sorting the entries by their integer value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_NodeVecSortByNumber( Fraig_NodeVec_t * p )
{
    qsort( (void *)p->pArray, (size_t)p->nSize, sizeof(Fraig_Node_t *), 
            (int (*)(const void *, const void *)) Fraig_NodeVecCompareNumbers );
}

/**Function*************************************************************

  Synopsis    [Sorting the entries by their integer value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_NodeVecSortByRefCount( Fraig_NodeVec_t * p )
{
    qsort( (void *)p->pArray, (size_t)p->nSize, sizeof(Fraig_Node_t *), 
            (int (*)(const void *, const void *)) Fraig_NodeVecCompareRefCounts );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END

