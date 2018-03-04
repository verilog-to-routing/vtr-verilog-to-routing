/**CFile****************************************************************

  FileName    [fxuHeapD.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [The priority queue for double cube divisors.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fxuHeapD.c,v 1.0 2003/02/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fxuInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define FXU_HEAP_DOUBLE_WEIGHT(pDiv)           ((pDiv)->Weight)
#define FXU_HEAP_DOUBLE_CURRENT(p, pDiv)       ((p)->pTree[(pDiv)->HNum])
#define FXU_HEAP_DOUBLE_PARENT_EXISTS(p, pDiv) ((pDiv)->HNum > 1)
#define FXU_HEAP_DOUBLE_CHILD1_EXISTS(p, pDiv) (((pDiv)->HNum << 1) <= p->nItems)
#define FXU_HEAP_DOUBLE_CHILD2_EXISTS(p, pDiv) ((((pDiv)->HNum << 1)+1) <= p->nItems)
#define FXU_HEAP_DOUBLE_PARENT(p, pDiv)        ((p)->pTree[(pDiv)->HNum >> 1])
#define FXU_HEAP_DOUBLE_CHILD1(p, pDiv)        ((p)->pTree[(pDiv)->HNum << 1])
#define FXU_HEAP_DOUBLE_CHILD2(p, pDiv)        ((p)->pTree[((pDiv)->HNum << 1)+1])
#define FXU_HEAP_DOUBLE_ASSERT(p, pDiv)        assert( (pDiv)->HNum >= 1 && (pDiv)->HNum <= p->nItemsAlloc )

static void Fxu_HeapDoubleResize( Fxu_HeapDouble * p );                  
static void Fxu_HeapDoubleSwap( Fxu_Double ** pDiv1, Fxu_Double ** pDiv2 );  
static void Fxu_HeapDoubleMoveUp( Fxu_HeapDouble * p, Fxu_Double * pDiv );  
static void Fxu_HeapDoubleMoveDn( Fxu_HeapDouble * p, Fxu_Double * pDiv );  

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fxu_HeapDouble * Fxu_HeapDoubleStart()
{
	Fxu_HeapDouble * p;
	p = ABC_ALLOC( Fxu_HeapDouble, 1 );
	memset( p, 0, sizeof(Fxu_HeapDouble) );
	p->nItems      = 0;
	p->nItemsAlloc = 10000;
	p->pTree       = ABC_ALLOC( Fxu_Double *, p->nItemsAlloc + 1 );
	p->pTree[0]    = NULL;
	return p;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_HeapDoubleResize( Fxu_HeapDouble * p )
{
	p->nItemsAlloc *= 2;
	p->pTree = ABC_REALLOC( Fxu_Double *, p->pTree, p->nItemsAlloc + 1 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_HeapDoubleStop( Fxu_HeapDouble * p )
{
	ABC_FREE( p->pTree );
	ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_HeapDoublePrint( FILE * pFile, Fxu_HeapDouble * p )
{
	Fxu_Double * pDiv;
	int Counter = 1;
	int Degree  = 1;

	Fxu_HeapDoubleCheck( p );
	fprintf( pFile, "The contents of the heap:\n" );
	fprintf( pFile, "Level %d:  ", Degree );
	Fxu_HeapDoubleForEachItem( p, pDiv )
	{
		assert( Counter == p->pTree[Counter]->HNum );
		fprintf( pFile, "%2d=%3d  ", Counter, FXU_HEAP_DOUBLE_WEIGHT(p->pTree[Counter]) );
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
void Fxu_HeapDoubleCheck( Fxu_HeapDouble * p )
{
	Fxu_Double * pDiv;
	Fxu_HeapDoubleForEachItem( p, pDiv )
	{
		assert( pDiv->HNum == p->i );
        Fxu_HeapDoubleCheckOne( p, pDiv );
	}
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_HeapDoubleCheckOne( Fxu_HeapDouble * p, Fxu_Double * pDiv )
{
    int Weight1, Weight2;
	if ( FXU_HEAP_DOUBLE_CHILD1_EXISTS(p,pDiv) )
	{
        Weight1 = FXU_HEAP_DOUBLE_WEIGHT(pDiv);
        Weight2 = FXU_HEAP_DOUBLE_WEIGHT( FXU_HEAP_DOUBLE_CHILD1(p,pDiv) );
        assert( Weight1 >= Weight2 );
	}
	if ( FXU_HEAP_DOUBLE_CHILD2_EXISTS(p,pDiv) )
	{
        Weight1 = FXU_HEAP_DOUBLE_WEIGHT(pDiv);
        Weight2 = FXU_HEAP_DOUBLE_WEIGHT( FXU_HEAP_DOUBLE_CHILD2(p,pDiv) );
        assert( Weight1 >= Weight2 );
	}
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_HeapDoubleInsert( Fxu_HeapDouble * p, Fxu_Double * pDiv )
{
	if ( p->nItems == p->nItemsAlloc )
		Fxu_HeapDoubleResize( p );
	// put the last entry to the last place and move up
	p->pTree[++p->nItems] = pDiv;
	pDiv->HNum = p->nItems;
	// move the last entry up if necessary
	Fxu_HeapDoubleMoveUp( p, pDiv );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_HeapDoubleUpdate( Fxu_HeapDouble * p, Fxu_Double * pDiv )
{
//printf( "Updating divisor %d.\n", pDiv->Num );

	FXU_HEAP_DOUBLE_ASSERT(p,pDiv);
	if ( FXU_HEAP_DOUBLE_PARENT_EXISTS(p,pDiv) && 
         FXU_HEAP_DOUBLE_WEIGHT(pDiv) > FXU_HEAP_DOUBLE_WEIGHT( FXU_HEAP_DOUBLE_PARENT(p,pDiv) ) )
		Fxu_HeapDoubleMoveUp( p, pDiv );
	else if ( FXU_HEAP_DOUBLE_CHILD1_EXISTS(p,pDiv) && 
        FXU_HEAP_DOUBLE_WEIGHT(pDiv) < FXU_HEAP_DOUBLE_WEIGHT( FXU_HEAP_DOUBLE_CHILD1(p,pDiv) ) )
		Fxu_HeapDoubleMoveDn( p, pDiv );
	else if ( FXU_HEAP_DOUBLE_CHILD2_EXISTS(p,pDiv) && 
        FXU_HEAP_DOUBLE_WEIGHT(pDiv) < FXU_HEAP_DOUBLE_WEIGHT( FXU_HEAP_DOUBLE_CHILD2(p,pDiv) ) )
		Fxu_HeapDoubleMoveDn( p, pDiv );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_HeapDoubleDelete( Fxu_HeapDouble * p, Fxu_Double * pDiv )
{
	FXU_HEAP_DOUBLE_ASSERT(p,pDiv);
	// put the last entry to the deleted place
	// decrement the number of entries
	p->pTree[pDiv->HNum] = p->pTree[p->nItems--];
	p->pTree[pDiv->HNum]->HNum = pDiv->HNum;
	// move the top entry down if necessary
	Fxu_HeapDoubleUpdate( p, p->pTree[pDiv->HNum] );
    pDiv->HNum = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fxu_Double * Fxu_HeapDoubleReadMax( Fxu_HeapDouble * p )
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
Fxu_Double * Fxu_HeapDoubleGetMax( Fxu_HeapDouble * p )
{
	Fxu_Double * pDiv;
	if ( p->nItems == 0 )
		return NULL;
	// prepare the return value
	pDiv = p->pTree[1];
	pDiv->HNum = 0;
	// put the last entry on top
	// decrement the number of entries
	p->pTree[1] = p->pTree[p->nItems--];
	p->pTree[1]->HNum = 1;
	// move the top entry down if necessary
	Fxu_HeapDoubleMoveDn( p, p->pTree[1] );
	return pDiv;	 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int  Fxu_HeapDoubleReadMaxWeight( Fxu_HeapDouble * p )
{
	if ( p->nItems == 0 )
		return -1;
	else
		return FXU_HEAP_DOUBLE_WEIGHT(p->pTree[1]);
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_HeapDoubleSwap( Fxu_Double ** pDiv1, Fxu_Double ** pDiv2 )
{
	Fxu_Double * pDiv;
	int Temp;
	pDiv   = *pDiv1;
	*pDiv1 = *pDiv2;
	*pDiv2 = pDiv;
	Temp          = (*pDiv1)->HNum;
	(*pDiv1)->HNum = (*pDiv2)->HNum;
	(*pDiv2)->HNum = Temp;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_HeapDoubleMoveUp( Fxu_HeapDouble * p, Fxu_Double * pDiv )
{
	Fxu_Double ** ppDiv, ** ppPar;
	ppDiv = &FXU_HEAP_DOUBLE_CURRENT(p, pDiv);
	while ( FXU_HEAP_DOUBLE_PARENT_EXISTS(p,*ppDiv) )
	{
		ppPar = &FXU_HEAP_DOUBLE_PARENT(p,*ppDiv);
		if ( FXU_HEAP_DOUBLE_WEIGHT(*ppDiv) > FXU_HEAP_DOUBLE_WEIGHT(*ppPar) )
		{
			Fxu_HeapDoubleSwap( ppDiv, ppPar );
			ppDiv = ppPar;
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
void Fxu_HeapDoubleMoveDn( Fxu_HeapDouble * p, Fxu_Double * pDiv )
{
	Fxu_Double ** ppChild1, ** ppChild2, ** ppDiv;
	ppDiv = &FXU_HEAP_DOUBLE_CURRENT(p, pDiv);
	while ( FXU_HEAP_DOUBLE_CHILD1_EXISTS(p,*ppDiv) )
	{ // if Child1 does not exist, Child2 also does not exists

		// get the children
		ppChild1 = &FXU_HEAP_DOUBLE_CHILD1(p,*ppDiv);
        if ( FXU_HEAP_DOUBLE_CHILD2_EXISTS(p,*ppDiv) )
        {
            ppChild2 = &FXU_HEAP_DOUBLE_CHILD2(p,*ppDiv);

            // consider two cases
            if ( FXU_HEAP_DOUBLE_WEIGHT(*ppDiv) >= FXU_HEAP_DOUBLE_WEIGHT(*ppChild1) &&
                 FXU_HEAP_DOUBLE_WEIGHT(*ppDiv) >= FXU_HEAP_DOUBLE_WEIGHT(*ppChild2) )
            { // Div is larger than both, skip
                break;
            }
            else
            { // Div is smaller than one of them, then swap it with larger 
                if ( FXU_HEAP_DOUBLE_WEIGHT(*ppChild1) >= FXU_HEAP_DOUBLE_WEIGHT(*ppChild2) )
                {
			        Fxu_HeapDoubleSwap( ppDiv, ppChild1 );
		            // update the pointer
		            ppDiv = ppChild1;
                }
                else
                {
			        Fxu_HeapDoubleSwap( ppDiv, ppChild2 );
		            // update the pointer
		            ppDiv = ppChild2;
                }
            }
        }
        else // Child2 does not exist
        {
            // consider two cases
            if ( FXU_HEAP_DOUBLE_WEIGHT(*ppDiv) >= FXU_HEAP_DOUBLE_WEIGHT(*ppChild1) )
            { // Div is larger than Child1, skip
                break;
            }
            else
            { // Div is smaller than Child1, then swap them
			    Fxu_HeapDoubleSwap( ppDiv, ppChild1 );
		        // update the pointer
		        ppDiv = ppChild1;
            }
        }
	}
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

