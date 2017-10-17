/**CFile****************************************************************

  FileName    [fxuList.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Operations on lists.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fxuList.c,v 1.0 2003/02/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fxuInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

// matrix -> var

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_ListMatrixAddVariable( Fxu_Matrix * p, Fxu_Var * pLink ) 
{
	Fxu_ListVar * pList = &p->lVars;
	if ( pList->pHead == NULL )
	{
		pList->pHead = pLink;
		pList->pTail = pLink;
		pLink->pPrev = NULL;
		pLink->pNext = NULL;
	}
	else
	{
		pLink->pNext = NULL;
		pList->pTail->pNext = pLink;
		pLink->pPrev = pList->pTail;
		pList->pTail = pLink;
	}
	pList->nItems++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_ListMatrixDelVariable( Fxu_Matrix * p, Fxu_Var * pLink )
{
	Fxu_ListVar * pList = &p->lVars;
	if ( pList->pHead == pLink )
		 pList->pHead = pLink->pNext;
	if ( pList->pTail == pLink )
		 pList->pTail = pLink->pPrev;
	if ( pLink->pPrev )
		 pLink->pPrev->pNext = pLink->pNext;
	if ( pLink->pNext )
		 pLink->pNext->pPrev = pLink->pPrev;
	pList->nItems--;
}


// matrix -> cube

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_ListMatrixAddCube( Fxu_Matrix * p, Fxu_Cube * pLink )
{
	Fxu_ListCube * pList = &p->lCubes;
	if ( pList->pHead == NULL )
	{
		pList->pHead = pLink;
		pList->pTail = pLink;
		pLink->pPrev = NULL;
		pLink->pNext = NULL;
	}
	else
	{
		pLink->pNext = NULL;
		pList->pTail->pNext = pLink;
		pLink->pPrev = pList->pTail;
		pList->pTail = pLink;
	}
	pList->nItems++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_ListMatrixDelCube( Fxu_Matrix * p, Fxu_Cube * pLink )
{
	Fxu_ListCube * pList = &p->lCubes;
	if ( pList->pHead == pLink )
		 pList->pHead = pLink->pNext;
	if ( pList->pTail == pLink )
		 pList->pTail = pLink->pPrev;
	if ( pLink->pPrev )
		 pLink->pPrev->pNext = pLink->pNext;
	if ( pLink->pNext )
		 pLink->pNext->pPrev = pLink->pPrev;
	pList->nItems--;
}


// matrix -> single

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_ListMatrixAddSingle( Fxu_Matrix * p, Fxu_Single * pLink )
{
	Fxu_ListSingle * pList = &p->lSingles;
	if ( pList->pHead == NULL )
	{
		pList->pHead = pLink;
		pList->pTail = pLink;
		pLink->pPrev = NULL;
		pLink->pNext = NULL;
	}
	else
	{
		pLink->pNext = NULL;
		pList->pTail->pNext = pLink;
		pLink->pPrev = pList->pTail;
		pList->pTail = pLink;
	}
	pList->nItems++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_ListMatrixDelSingle( Fxu_Matrix * p, Fxu_Single * pLink )
{
	Fxu_ListSingle * pList = &p->lSingles;
	if ( pList->pHead == pLink )
		 pList->pHead = pLink->pNext;
	if ( pList->pTail == pLink )
		 pList->pTail = pLink->pPrev;
	if ( pLink->pPrev )
		 pLink->pPrev->pNext = pLink->pNext;
	if ( pLink->pNext )
		 pLink->pNext->pPrev = pLink->pPrev;
	pList->nItems--;
}


// table -> divisor

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_ListTableAddDivisor( Fxu_Matrix * p, Fxu_Double * pLink ) 
{
	Fxu_ListDouble * pList = &(p->pTable[pLink->Key]);
	if ( pList->pHead == NULL )
	{
		pList->pHead = pLink;
		pList->pTail = pLink;
		pLink->pPrev = NULL;
		pLink->pNext = NULL;
	}
	else
	{
		pLink->pNext = NULL;
		pList->pTail->pNext = pLink;
		pLink->pPrev = pList->pTail;
		pList->pTail = pLink;
	}
	pList->nItems++;
    p->nDivs++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_ListTableDelDivisor( Fxu_Matrix * p, Fxu_Double * pLink ) 
{
	Fxu_ListDouble * pList = &(p->pTable[pLink->Key]);
	if ( pList->pHead == pLink )
		 pList->pHead = pLink->pNext;
	if ( pList->pTail == pLink )
		 pList->pTail = pLink->pPrev;
	if ( pLink->pPrev )
		 pLink->pPrev->pNext = pLink->pNext;
	if ( pLink->pNext )
		 pLink->pNext->pPrev = pLink->pPrev;
	pList->nItems--;
    p->nDivs--;
}


// cube -> literal 

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_ListCubeAddLiteral( Fxu_Cube * pCube, Fxu_Lit * pLink )
{
	Fxu_ListLit * pList = &(pCube->lLits);
	if ( pList->pHead == NULL )
	{
		pList->pHead = pLink;
		pList->pTail = pLink;
		pLink->pHPrev = NULL;
		pLink->pHNext = NULL;
	}
	else
	{
		pLink->pHNext = NULL;
		pList->pTail->pHNext = pLink;
		pLink->pHPrev = pList->pTail;
		pList->pTail = pLink;
	}
	pList->nItems++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_ListCubeDelLiteral( Fxu_Cube * pCube, Fxu_Lit * pLink )
{
	Fxu_ListLit * pList = &(pCube->lLits);
	if ( pList->pHead == pLink )
		 pList->pHead = pLink->pHNext;
	if ( pList->pTail == pLink )
		 pList->pTail = pLink->pHPrev;
	if ( pLink->pHPrev )
		 pLink->pHPrev->pHNext = pLink->pHNext;
	if ( pLink->pHNext )
		 pLink->pHNext->pHPrev = pLink->pHPrev;
	pList->nItems--;
}


// var -> literal

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_ListVarAddLiteral( Fxu_Var * pVar, Fxu_Lit * pLink )
{
	Fxu_ListLit * pList = &(pVar->lLits);
	if ( pList->pHead == NULL )
	{
		pList->pHead = pLink;
		pList->pTail = pLink;
		pLink->pVPrev = NULL;
		pLink->pVNext = NULL;
	}
	else
	{
		pLink->pVNext = NULL;
		pList->pTail->pVNext = pLink;
		pLink->pVPrev = pList->pTail;
		pList->pTail = pLink;
	}
	pList->nItems++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_ListVarDelLiteral( Fxu_Var * pVar, Fxu_Lit * pLink )
{
	Fxu_ListLit * pList = &(pVar->lLits);
	if ( pList->pHead == pLink )
		 pList->pHead = pLink->pVNext;
	if ( pList->pTail == pLink )
		 pList->pTail = pLink->pVPrev;
	if ( pLink->pVPrev )
		 pLink->pVPrev->pVNext = pLink->pVNext;
	if ( pLink->pVNext )
		 pLink->pVNext->pVPrev = pLink->pVPrev;
	pList->nItems--;
}



// divisor -> pair

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_ListDoubleAddPairLast( Fxu_Double * pDiv, Fxu_Pair * pLink )
{
	Fxu_ListPair * pList = &pDiv->lPairs;
	if ( pList->pHead == NULL )
	{
		pList->pHead = pLink;
		pList->pTail = pLink;
		pLink->pDPrev = NULL;
		pLink->pDNext = NULL;
	}
	else
	{
		pLink->pDNext = NULL;
		pList->pTail->pDNext = pLink;
		pLink->pDPrev = pList->pTail;
		pList->pTail = pLink;
	}
	pList->nItems++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_ListDoubleAddPairFirst( Fxu_Double * pDiv, Fxu_Pair * pLink )
{
	Fxu_ListPair * pList = &pDiv->lPairs;
	if ( pList->pHead == NULL )
	{
		pList->pHead = pLink;
		pList->pTail = pLink;
		pLink->pDPrev = NULL;
		pLink->pDNext = NULL;
	}
	else
	{
		pLink->pDPrev = NULL;
		pList->pHead->pDPrev = pLink;
		pLink->pDNext = pList->pHead;
		pList->pHead = pLink;
	}
	pList->nItems++;
}

/**Function*************************************************************

  Synopsis    [Adds the entry in the middle of the list after the spot.]

  Description [Assumes that spot points to the link, after which the given
  link should be added. Spot cannot be NULL or the tail of the list.
  Therefore, the head and the tail of the list are not changed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_ListDoubleAddPairMiddle( Fxu_Double * pDiv, Fxu_Pair * pSpot, Fxu_Pair * pLink )
{
	Fxu_ListPair * pList = &pDiv->lPairs;
	assert( pSpot );
	assert( pSpot != pList->pTail );
	pLink->pDPrev = pSpot;
	pLink->pDNext = pSpot->pDNext;
	pLink->pDPrev->pDNext = pLink;
	pLink->pDNext->pDPrev = pLink;
	pList->nItems++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_ListDoubleDelPair( Fxu_Double * pDiv, Fxu_Pair * pLink )
{
	Fxu_ListPair * pList = &pDiv->lPairs;
	if ( pList->pHead == pLink )
		 pList->pHead = pLink->pDNext;
	if ( pList->pTail == pLink )
		 pList->pTail = pLink->pDPrev;
	if ( pLink->pDPrev )
		 pLink->pDPrev->pDNext = pLink->pDNext;
	if ( pLink->pDNext )
		 pLink->pDNext->pDPrev = pLink->pDPrev;
	pList->nItems--;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_ListDoubleAddPairPlace( Fxu_Double * pDiv, Fxu_Pair * pPair, Fxu_Pair * pPairSpot )
{
	printf( "Fxu_ListDoubleAddPairPlace() is called!\n" );
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

