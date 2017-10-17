/**CFile****************************************************************

  FileName    [mapperVec.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic technology mapping engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - June 1, 2004.]

  Revision    [$Id: mapperVec.c,v 1.3 2005/01/23 06:59:45 alanmi Exp $]

***********************************************************************/

#include "mapperInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Map_NodeVecCompareLevels( Map_Node_t ** pp1, Map_Node_t ** pp2 );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Map_NodeVec_t * Map_NodeVecAlloc( int nCap )
{
    Map_NodeVec_t * p;
    p = ALLOC( Map_NodeVec_t, 1 );
    if ( nCap > 0 && nCap < 16 )
        nCap = 16;
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ALLOC( Map_Node_t *, p->nCap ) : NULL;
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_NodeVecFree( Map_NodeVec_t * p )
{
    FREE( p->pArray );
    FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Map_Node_t ** Map_NodeVecReadArray( Map_NodeVec_t * p )
{
    return p->pArray;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_NodeVecReadSize( Map_NodeVec_t * p )
{
    return p->nSize;
}

/**Function*************************************************************

  Synopsis    [Resizes the vector to the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_NodeVecGrow( Map_NodeVec_t * p, int nCapMin )
{
    if ( p->nCap >= nCapMin )
        return;
    p->pArray = REALLOC( Map_Node_t *, p->pArray, nCapMin ); 
    p->nCap   = nCapMin;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_NodeVecShrink( Map_NodeVec_t * p, int nSizeNew )
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
void Map_NodeVecClear( Map_NodeVec_t * p )
{
    p->nSize = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_NodeVecPush( Map_NodeVec_t * p, Map_Node_t * Entry )
{
    if ( p->nSize == p->nCap )
    {
        if ( p->nCap < 16 )
            Map_NodeVecGrow( p, 16 );
        else
            Map_NodeVecGrow( p, 2 * p->nCap );
    }
    p->pArray[p->nSize++] = Entry;
}

/**Function*************************************************************

  Synopsis    [Add the element while ensuring uniqueness.]

  Description [Returns 1 if the element was found, and 0 if it was new. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_NodeVecPushUnique( Map_NodeVec_t * p, Map_Node_t * Entry )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        if ( p->pArray[i] == Entry )
            return 1;
    Map_NodeVecPush( p, Entry );
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Map_Node_t * Map_NodeVecPop( Map_NodeVec_t * p )
{
    return p->pArray[--p->nSize];
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_NodeVecRemove( Map_NodeVec_t * p, Map_Node_t * Entry )
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
void Map_NodeVecWriteEntry( Map_NodeVec_t * p, int i, Map_Node_t * Entry )
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
Map_Node_t * Map_NodeVecReadEntry( Map_NodeVec_t * p, int i )
{
    assert( i >= 0 && i < p->nSize );
    return p->pArray[i];
}

/**Function*************************************************************

  Synopsis    [Sorting the entries by their integer value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_NodeVecSortByLevel( Map_NodeVec_t * p )
{
    qsort( (void *)p->pArray, p->nSize, sizeof(Map_Node_t *), 
            (int (*)(const void *, const void *)) Map_NodeVecCompareLevels );
}

/**Function*************************************************************

  Synopsis    [Comparison procedure for two clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_NodeVecCompareLevels( Map_Node_t ** pp1, Map_Node_t ** pp2 )
{
    int Level1 = Map_Regular(*pp1)->Level;
    int Level2 = Map_Regular(*pp2)->Level;
    if ( Level1 < Level2 )
        return -1;
    if ( Level1 > Level2 )
        return 1;
    if ( Map_Regular(*pp1)->Num < Map_Regular(*pp2)->Num )
        return -1;
    if ( Map_Regular(*pp1)->Num > Map_Regular(*pp2)->Num )
        return 1;
    return 0; 
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

