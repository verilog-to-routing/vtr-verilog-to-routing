/**CFile****************************************************************

  FileName    [fxuHeapS.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [The priority queue for variables.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fxuHeapS.c,v 1.0 2003/02/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fxuInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define FXU_HEAP_SINGLE_WEIGHT(pSingle)           ((pSingle)->Weight)
#define FXU_HEAP_SINGLE_CURRENT(p, pSingle)       ((p)->pTree[(pSingle)->HNum])
#define FXU_HEAP_SINGLE_PARENT_EXISTS(p, pSingle) ((pSingle)->HNum > 1)
#define FXU_HEAP_SINGLE_CHILD1_EXISTS(p, pSingle) (((pSingle)->HNum << 1) <= p->nItems)
#define FXU_HEAP_SINGLE_CHILD2_EXISTS(p, pSingle) ((((pSingle)->HNum << 1)+1) <= p->nItems)
#define FXU_HEAP_SINGLE_PARENT(p, pSingle)        ((p)->pTree[(pSingle)->HNum >> 1])
#define FXU_HEAP_SINGLE_CHILD1(p, pSingle)        ((p)->pTree[(pSingle)->HNum << 1])
#define FXU_HEAP_SINGLE_CHILD2(p, pSingle)        ((p)->pTree[((pSingle)->HNum << 1)+1])
#define FXU_HEAP_SINGLE_ASSERT(p, pSingle)        assert( (pSingle)->HNum >= 1 && (pSingle)->HNum <= p->nItemsAlloc )

static void Fxu_HeapSingleResize( Fxu_HeapSingle * p );                  
static void Fxu_HeapSingleSwap( Fxu_Single ** pSingle1, Fxu_Single ** pSingle2 );  
static void Fxu_HeapSingleMoveUp( Fxu_HeapSingle * p, Fxu_Single * pSingle );  
static void Fxu_HeapSingleMoveDn( Fxu_HeapSingle * p, Fxu_Single * pSingle );  

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fxu_HeapSingle * Fxu_HeapSingleStart()
{
    Fxu_HeapSingle * p;
    p = ABC_ALLOC( Fxu_HeapSingle, 1 );
    memset( p, 0, sizeof(Fxu_HeapSingle) );
    p->nItems      = 0;
    p->nItemsAlloc = 2000;
    p->pTree       = ABC_ALLOC( Fxu_Single *, p->nItemsAlloc + 10 );
    p->pTree[0]    = NULL;
    return p;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_HeapSingleResize( Fxu_HeapSingle * p )
{
    p->nItemsAlloc *= 2;
    p->pTree = ABC_REALLOC( Fxu_Single *, p->pTree, p->nItemsAlloc + 10 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_HeapSingleStop( Fxu_HeapSingle * p )
{
    int i;
    i = 0;
    ABC_FREE( p->pTree );
    i = 1;
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_HeapSinglePrint( FILE * pFile, Fxu_HeapSingle * p )
{
    Fxu_Single * pSingle;
    int Counter = 1;
    int Degree  = 1;

    Fxu_HeapSingleCheck( p );
    fprintf( pFile, "The contents of the heap:\n" );
    fprintf( pFile, "Level %d:  ", Degree );
    Fxu_HeapSingleForEachItem( p, pSingle )
    {
        assert( Counter == p->pTree[Counter]->HNum );
        fprintf( pFile, "%2d=%3d  ", Counter, FXU_HEAP_SINGLE_WEIGHT(p->pTree[Counter]) );
        if ( ++Counter == (1 << Degree) )
        {
            fprintf( pFile, "\n" );
            Degree++;
            fprintf( pFile, "Level %d:  ", Degree );
        }
    }
    fprintf( pFile, "\n" );
    fprintf( pFile, "End of the heap printout.\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_HeapSingleCheck( Fxu_HeapSingle * p )
{
    Fxu_Single * pSingle;
    Fxu_HeapSingleForEachItem( p, pSingle )
    {
        assert( pSingle->HNum == p->i );
        Fxu_HeapSingleCheckOne( p, pSingle );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_HeapSingleCheckOne( Fxu_HeapSingle * p, Fxu_Single * pSingle )
{
    int Weight1, Weight2;
    if ( FXU_HEAP_SINGLE_CHILD1_EXISTS(p,pSingle) )
    {
        Weight1 = FXU_HEAP_SINGLE_WEIGHT(pSingle);
        Weight2 = FXU_HEAP_SINGLE_WEIGHT( FXU_HEAP_SINGLE_CHILD1(p,pSingle) );
        assert( Weight1 >= Weight2 );
    }
    if ( FXU_HEAP_SINGLE_CHILD2_EXISTS(p,pSingle) )
    {
        Weight1 = FXU_HEAP_SINGLE_WEIGHT(pSingle);
        Weight2 = FXU_HEAP_SINGLE_WEIGHT( FXU_HEAP_SINGLE_CHILD2(p,pSingle) );
        assert( Weight1 >= Weight2 );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_HeapSingleInsert( Fxu_HeapSingle * p, Fxu_Single * pSingle )
{
    if ( p->nItems == p->nItemsAlloc )
        Fxu_HeapSingleResize( p );
    // put the last entry to the last place and move up
    p->pTree[++p->nItems] = pSingle;
    pSingle->HNum = p->nItems;
    // move the last entry up if necessary
    Fxu_HeapSingleMoveUp( p, pSingle );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_HeapSingleUpdate( Fxu_HeapSingle * p, Fxu_Single * pSingle )
{
    FXU_HEAP_SINGLE_ASSERT(p,pSingle);
    if ( FXU_HEAP_SINGLE_PARENT_EXISTS(p,pSingle) && 
         FXU_HEAP_SINGLE_WEIGHT(pSingle) > FXU_HEAP_SINGLE_WEIGHT( FXU_HEAP_SINGLE_PARENT(p,pSingle) ) )
        Fxu_HeapSingleMoveUp( p, pSingle );
    else if ( FXU_HEAP_SINGLE_CHILD1_EXISTS(p,pSingle) && 
        FXU_HEAP_SINGLE_WEIGHT(pSingle) < FXU_HEAP_SINGLE_WEIGHT( FXU_HEAP_SINGLE_CHILD1(p,pSingle) ) )
        Fxu_HeapSingleMoveDn( p, pSingle );
    else if ( FXU_HEAP_SINGLE_CHILD2_EXISTS(p,pSingle) && 
        FXU_HEAP_SINGLE_WEIGHT(pSingle) < FXU_HEAP_SINGLE_WEIGHT( FXU_HEAP_SINGLE_CHILD2(p,pSingle) ) )
        Fxu_HeapSingleMoveDn( p, pSingle );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_HeapSingleDelete( Fxu_HeapSingle * p, Fxu_Single * pSingle )
{
    int Place = pSingle->HNum;
    FXU_HEAP_SINGLE_ASSERT(p,pSingle);
    // put the last entry to the deleted place
    // decrement the number of entries
    p->pTree[Place] = p->pTree[p->nItems--];
    p->pTree[Place]->HNum = Place;
    // move the top entry down if necessary
    Fxu_HeapSingleUpdate( p, p->pTree[Place] );
    pSingle->HNum = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fxu_Single * Fxu_HeapSingleReadMax( Fxu_HeapSingle * p )
{
    if ( p->nItems == 0 )
        return NULL;
    return p->pTree[1];
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fxu_Single * Fxu_HeapSingleGetMax( Fxu_HeapSingle * p )
{
    Fxu_Single * pSingle;
    if ( p->nItems == 0 )
        return NULL;
    // prepare the return value
    pSingle = p->pTree[1];
    pSingle->HNum = 0;
    // put the last entry on top
    // decrement the number of entries
    p->pTree[1] = p->pTree[p->nItems--];
    p->pTree[1]->HNum = 1;
    // move the top entry down if necessary
    Fxu_HeapSingleMoveDn( p, p->pTree[1] );
    return pSingle;     
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int  Fxu_HeapSingleReadMaxWeight( Fxu_HeapSingle * p )
{
    if ( p->nItems == 0 )
        return -1;
    return FXU_HEAP_SINGLE_WEIGHT(p->pTree[1]);
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_HeapSingleSwap( Fxu_Single ** pSingle1, Fxu_Single ** pSingle2 )
{
    Fxu_Single * pSingle;
    int Temp;
    pSingle   = *pSingle1;
    *pSingle1 = *pSingle2;
    *pSingle2 = pSingle;
    Temp          = (*pSingle1)->HNum;
    (*pSingle1)->HNum = (*pSingle2)->HNum;
    (*pSingle2)->HNum = Temp;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_HeapSingleMoveUp( Fxu_HeapSingle * p, Fxu_Single * pSingle )
{
    Fxu_Single ** ppSingle, ** ppPar;
    ppSingle = &FXU_HEAP_SINGLE_CURRENT(p, pSingle);
    while ( FXU_HEAP_SINGLE_PARENT_EXISTS(p,*ppSingle) )
    {
        ppPar = &FXU_HEAP_SINGLE_PARENT(p,*ppSingle);
        if ( FXU_HEAP_SINGLE_WEIGHT(*ppSingle) > FXU_HEAP_SINGLE_WEIGHT(*ppPar) )
        {
            Fxu_HeapSingleSwap( ppSingle, ppPar );
            ppSingle = ppPar;
        }
        else
            break;
    }
}
 
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_HeapSingleMoveDn( Fxu_HeapSingle * p, Fxu_Single * pSingle )
{
    Fxu_Single ** ppChild1, ** ppChild2, ** ppSingle;
    ppSingle = &FXU_HEAP_SINGLE_CURRENT(p, pSingle);
    while ( FXU_HEAP_SINGLE_CHILD1_EXISTS(p,*ppSingle) )
    { // if Child1 does not exist, Child2 also does not exists

        // get the children
        ppChild1 = &FXU_HEAP_SINGLE_CHILD1(p,*ppSingle);
        if ( FXU_HEAP_SINGLE_CHILD2_EXISTS(p,*ppSingle) )
        {
            ppChild2 = &FXU_HEAP_SINGLE_CHILD2(p,*ppSingle);

            // consider two cases
            if ( FXU_HEAP_SINGLE_WEIGHT(*ppSingle) >= FXU_HEAP_SINGLE_WEIGHT(*ppChild1) &&
                 FXU_HEAP_SINGLE_WEIGHT(*ppSingle) >= FXU_HEAP_SINGLE_WEIGHT(*ppChild2) )
            { // Var is larger than both, skip
                break;
            }
            else
            { // Var is smaller than one of them, then swap it with larger 
                if ( FXU_HEAP_SINGLE_WEIGHT(*ppChild1) >= FXU_HEAP_SINGLE_WEIGHT(*ppChild2) )
                {
                    Fxu_HeapSingleSwap( ppSingle, ppChild1 );
                    // update the pointer
                    ppSingle = ppChild1;
                }
                else
                {
                    Fxu_HeapSingleSwap( ppSingle, ppChild2 );
                    // update the pointer
                    ppSingle = ppChild2;
                }
            }
        }
        else // Child2 does not exist
        {
            // consider two cases
            if ( FXU_HEAP_SINGLE_WEIGHT(*ppSingle) >= FXU_HEAP_SINGLE_WEIGHT(*ppChild1) )
            { // Var is larger than Child1, skip
                break;
            }
            else
            { // Var is smaller than Child1, then swap them
                Fxu_HeapSingleSwap( ppSingle, ppChild1 );
                // update the pointer
                ppSingle = ppChild1;
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
ABC_NAMESPACE_IMPL_END

