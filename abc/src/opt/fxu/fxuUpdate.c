/**CFile****************************************************************

  FileName    [fxuUpdate.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Updating the sparse matrix when divisors are accepted.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fxuUpdate.c,v 1.0 2003/02/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fxuInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Fxu_UpdateDoublePairs( Fxu_Matrix * p, Fxu_Double * pDouble, Fxu_Var * pVar );
static void Fxu_UpdateMatrixDoubleCreateCubes( Fxu_Matrix * p, Fxu_Cube * pCube1, Fxu_Cube * pCube2, Fxu_Double * pDiv );
static void Fxu_UpdateMatrixDoubleClean( Fxu_Matrix * p, Fxu_Cube * pCubeUse, Fxu_Cube * pCubeRem );
static void Fxu_UpdateMatrixSingleClean( Fxu_Matrix * p, Fxu_Var * pVar1, Fxu_Var * pVar2, Fxu_Var * pVarNew );

static void Fxu_UpdateCreateNewVars( Fxu_Matrix * p, Fxu_Var ** ppVarC, Fxu_Var ** ppVarD, int nCubes );
static int  Fxu_UpdatePairCompare( Fxu_Pair ** ppP1, Fxu_Pair ** ppP2 );
static void Fxu_UpdatePairsSort( Fxu_Matrix * p, Fxu_Double * pDouble );

static void Fxu_UpdateCleanOldDoubles( Fxu_Matrix * p, Fxu_Double * pDiv, Fxu_Cube * pCube );
static void Fxu_UpdateAddNewDoubles( Fxu_Matrix * p, Fxu_Cube * pCube );
static void Fxu_UpdateCleanOldSingles( Fxu_Matrix * p );
static void Fxu_UpdateAddNewSingles( Fxu_Matrix * p, Fxu_Var * pVar );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Updates the matrix after selecting two divisors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_Update( Fxu_Matrix * p, Fxu_Single * pSingle, Fxu_Double * pDouble )
{
	Fxu_Cube * pCube, * pCubeNew;
	Fxu_Var * pVarC, * pVarD;
	Fxu_Var * pVar1, * pVar2;

    // consider trivial cases
    if ( pSingle == NULL )
    {
        assert( pDouble->Weight == Fxu_HeapDoubleReadMaxWeight( p->pHeapDouble ) );
        Fxu_UpdateDouble( p );
        return;
    }
    if ( pDouble == NULL )
    {
        assert( pSingle->Weight == Fxu_HeapSingleReadMaxWeight( p->pHeapSingle ) );
        Fxu_UpdateSingle( p );
        return;
    }

    // get the variables of the single
    pVar1 = pSingle->pVar1;
    pVar2 = pSingle->pVar2;

    // remove the best double from the heap
    Fxu_HeapDoubleDelete( p->pHeapDouble, pDouble );
	// remove the best divisor from the table
	Fxu_ListTableDelDivisor( p, pDouble );

    // create two new columns (vars)
    Fxu_UpdateCreateNewVars( p, &pVarC, &pVarD, 1 );
    // create one new row (cube)
    pCubeNew = Fxu_MatrixAddCube( p, pVarD, 0 );
    pCubeNew->pFirst = pCubeNew;
    // set the first cube of the positive var
    pVarD->pFirst = pCubeNew;

    // start collecting the affected vars and cubes
    Fxu_MatrixRingCubesStart( p );
    Fxu_MatrixRingVarsStart( p );
    // add the vars
    Fxu_MatrixRingVarsAdd( p, pVar1 );
    Fxu_MatrixRingVarsAdd( p, pVar2 );
    // remove the literals and collect the affected cubes
    // remove the divisors associated with this cube
   	// add to the affected cube the literal corresponding to the new column
	Fxu_UpdateMatrixSingleClean( p, pVar1, pVar2, pVarD );
	// replace each two cubes of the pair by one new cube
	// the new cube contains the base and the new literal
    Fxu_UpdateDoublePairs( p, pDouble, pVarC );
    // stop collecting the affected vars and cubes
    Fxu_MatrixRingCubesStop( p );
    Fxu_MatrixRingVarsStop( p );

    // add the literals to the new cube
    assert( pVar1->iVar < pVar2->iVar );
    assert( Fxu_SingleCountCoincidence( p, pVar1, pVar2 ) == 0 );
    Fxu_MatrixAddLiteral( p, pCubeNew, pVar1 );
    Fxu_MatrixAddLiteral( p, pCubeNew, pVar2 );

    // create new doubles; we cannot add them in the same loop
	// because we first have to create *all* new cubes for each node
    Fxu_MatrixForEachCubeInRing( p, pCube )
   		Fxu_UpdateAddNewDoubles( p, pCube );
    // update the singles after removing some literals
    Fxu_UpdateCleanOldSingles( p );

    // undo the temporary rings with cubes and vars
    Fxu_MatrixRingCubesUnmark( p );
    Fxu_MatrixRingVarsUnmark( p );
    // we should undo the rings before creating new singles

    // create new singles
    Fxu_UpdateAddNewSingles( p, pVarC );
    Fxu_UpdateAddNewSingles( p, pVarD );

	// recycle the divisor
	MEM_FREE_FXU( p, Fxu_Double, 1, pDouble );
	p->nDivs3++;
}

/**Function*************************************************************

  Synopsis    [Updates after accepting single cube divisor.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_UpdateSingle( Fxu_Matrix * p )
{
    Fxu_Single * pSingle;
	Fxu_Cube * pCube, * pCubeNew;
	Fxu_Var * pVarC, * pVarD;
	Fxu_Var * pVar1, * pVar2;

    // read the best divisor from the heap
    pSingle = Fxu_HeapSingleReadMax( p->pHeapSingle );
    // get the variables of this single-cube divisor
    pVar1 = pSingle->pVar1;
    pVar2 = pSingle->pVar2;

    // create two new columns (vars)
    Fxu_UpdateCreateNewVars( p, &pVarC, &pVarD, 1 );
    // create one new row (cube)
    pCubeNew = Fxu_MatrixAddCube( p, pVarD, 0 );
    pCubeNew->pFirst = pCubeNew;
    // set the first cube
    pVarD->pFirst = pCubeNew;

    // start collecting the affected vars and cubes
    Fxu_MatrixRingCubesStart( p );
    Fxu_MatrixRingVarsStart( p );
    // add the vars
    Fxu_MatrixRingVarsAdd( p, pVar1 );
    Fxu_MatrixRingVarsAdd( p, pVar2 );
    // remove the literals and collect the affected cubes
    // remove the divisors associated with this cube
   	// add to the affected cube the literal corresponding to the new column
	Fxu_UpdateMatrixSingleClean( p, pVar1, pVar2, pVarD );
    // stop collecting the affected vars and cubes
    Fxu_MatrixRingCubesStop( p );
    Fxu_MatrixRingVarsStop( p );

    // add the literals to the new cube
    assert( pVar1->iVar < pVar2->iVar );
    assert( Fxu_SingleCountCoincidence( p, pVar1, pVar2 ) == 0 );
    Fxu_MatrixAddLiteral( p, pCubeNew, pVar1 );
    Fxu_MatrixAddLiteral( p, pCubeNew, pVar2 );

    // create new doubles; we cannot add them in the same loop
	// because we first have to create *all* new cubes for each node
    Fxu_MatrixForEachCubeInRing( p, pCube )
   		Fxu_UpdateAddNewDoubles( p, pCube );
    // update the singles after removing some literals
    Fxu_UpdateCleanOldSingles( p );
    // we should undo the rings before creating new singles

    // unmark the cubes
    Fxu_MatrixRingCubesUnmark( p );
    Fxu_MatrixRingVarsUnmark( p );

    // create new singles
    Fxu_UpdateAddNewSingles( p, pVarC );
    Fxu_UpdateAddNewSingles( p, pVarD );
	p->nDivs1++;
}

/**Function*************************************************************

  Synopsis    [Updates the matrix after accepting a double cube divisor.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_UpdateDouble( Fxu_Matrix * p )
{
    Fxu_Double * pDiv;
	Fxu_Cube * pCube, * pCubeNew1, * pCubeNew2;
	Fxu_Var * pVarC, * pVarD;

    // remove the best divisor from the heap
    pDiv = Fxu_HeapDoubleGetMax( p->pHeapDouble );
	// remove the best divisor from the table
	Fxu_ListTableDelDivisor( p, pDiv );

    // create two new columns (vars)
    Fxu_UpdateCreateNewVars( p, &pVarC, &pVarD, 2 );
    // create two new rows (cubes)
    pCubeNew1 = Fxu_MatrixAddCube( p, pVarD, 0 );
    pCubeNew1->pFirst = pCubeNew1;
    pCubeNew2 = Fxu_MatrixAddCube( p, pVarD, 1 );
    pCubeNew2->pFirst = pCubeNew1;
    // set the first cube
    pVarD->pFirst = pCubeNew1;

    // add the literals to the new cubes
    Fxu_UpdateMatrixDoubleCreateCubes( p, pCubeNew1, pCubeNew2, pDiv );

    // start collecting the affected cubes and vars
    Fxu_MatrixRingCubesStart( p );
    Fxu_MatrixRingVarsStart( p );
	// replace each two cubes of the pair by one new cube
	// the new cube contains the base and the new literal
    Fxu_UpdateDoublePairs( p, pDiv, pVarD );
    // stop collecting the affected cubes and vars
    Fxu_MatrixRingCubesStop( p );
    Fxu_MatrixRingVarsStop( p );
    
    // create new doubles; we cannot add them in the same loop
	// because we first have to create *all* new cubes for each node
    Fxu_MatrixForEachCubeInRing( p, pCube )
   		Fxu_UpdateAddNewDoubles( p, pCube );
    // update the singles after removing some literals
    Fxu_UpdateCleanOldSingles( p );

    // undo the temporary rings with cubes and vars
    Fxu_MatrixRingCubesUnmark( p );
    Fxu_MatrixRingVarsUnmark( p );
    // we should undo the rings before creating new singles

    // create new singles
    Fxu_UpdateAddNewSingles( p, pVarC );
    Fxu_UpdateAddNewSingles( p, pVarD );

    // recycle the divisor
	MEM_FREE_FXU( p, Fxu_Double, 1, pDiv );
	p->nDivs2++;
}

/**Function*************************************************************

  Synopsis    [Update the pairs.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_UpdateDoublePairs( Fxu_Matrix * p, Fxu_Double * pDouble, Fxu_Var * pVar )
{
    Fxu_Pair * pPair;
    Fxu_Cube * pCubeUse, * pCubeRem;
    int i;

    // collect and sort the pairs
    Fxu_UpdatePairsSort( p, pDouble );
//    for ( i = 0; i < p->nPairsTemp; i++ )
    for ( i = 0; i < p->vPairs->nSize; i++ )
    {
        // get the pair
//        pPair = p->pPairsTemp[i];
        pPair = (Fxu_Pair *)p->vPairs->pArray[i];
	    // out of the two cubes, select the one which comes earlier
	    pCubeUse = Fxu_PairMinCube( pPair );
	    pCubeRem = Fxu_PairMaxCube( pPair );
        // collect the affected cube
        assert( pCubeUse->pOrder == NULL );
        Fxu_MatrixRingCubesAdd( p, pCubeUse );

	    // remove some literals from pCubeUse and all literals from pCubeRem
	    Fxu_UpdateMatrixDoubleClean( p, pCubeUse, pCubeRem );
	    // add a literal that depends on the new variable
	    Fxu_MatrixAddLiteral( p, pCubeUse, pVar );	
        // check the literal count
        assert( pCubeUse->lLits.nItems == pPair->nBase + 1 );
        assert( pCubeRem->lLits.nItems == 0 );

	    // update the divisors by removing useless pairs
	    Fxu_UpdateCleanOldDoubles( p, pDouble, pCubeUse );
	    Fxu_UpdateCleanOldDoubles( p, pDouble, pCubeRem );
	    // remove the pair
	    MEM_FREE_FXU( p, Fxu_Pair, 1, pPair );
    }
    p->vPairs->nSize = 0;
}

/**Function*************************************************************

  Synopsis    [Add two cubes corresponding to the given double-cube divisor.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_UpdateMatrixDoubleCreateCubes( Fxu_Matrix * p, Fxu_Cube * pCube1, Fxu_Cube * pCube2, Fxu_Double * pDiv )
{
	Fxu_Lit * pLit1, * pLit2;
    Fxu_Pair * pPair;
	int nBase, nLits1, nLits2;

	// fill in the SOP and copy the fanins
	nBase = nLits1 = nLits2 = 0;
	pPair = pDiv->lPairs.pHead;
	pLit1 = pPair->pCube1->lLits.pHead;
	pLit2 = pPair->pCube2->lLits.pHead;
	while ( 1 )
	{
		if ( pLit1 && pLit2 )
		{
			if ( pLit1->iVar == pLit2->iVar )
			{ // skip the cube free part
				pLit1 = pLit1->pHNext;
				pLit2 = pLit2->pHNext;
                nBase++;
			}
			else if ( pLit1->iVar < pLit2->iVar )
			{	// add literal to the first cube
                Fxu_MatrixAddLiteral( p, pCube1, pLit1->pVar );
				// move to the next literal in this cube
				pLit1 = pLit1->pHNext;
                nLits1++;
			}
			else
			{	// add literal to the second cube
                Fxu_MatrixAddLiteral( p, pCube2, pLit2->pVar );
				// move to the next literal in this cube
				pLit2 = pLit2->pHNext;
                nLits2++;
			}
		}
		else if ( pLit1 && !pLit2 )
		{	// add literal to the first cube
            Fxu_MatrixAddLiteral( p, pCube1, pLit1->pVar );
			// move to the next literal in this cube
			pLit1 = pLit1->pHNext;
            nLits1++;
		}
		else if ( !pLit1 && pLit2 )
		{	// add literal to the second cube
            Fxu_MatrixAddLiteral( p, pCube2, pLit2->pVar );
			// move to the next literal in this cube
			pLit2 = pLit2->pHNext;
            nLits2++;
		}
		else
			break;
	}
	assert( pPair->nLits1 == nLits1 );
	assert( pPair->nLits2 == nLits2 );
	assert( pPair->nBase == nBase );
}


/**Function*************************************************************

  Synopsis    [Create the node equal to double-cube divisor.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_UpdateMatrixDoubleClean( Fxu_Matrix * p, Fxu_Cube * pCubeUse, Fxu_Cube * pCubeRem )
{
	Fxu_Lit * pLit1, * bLit1Next;
	Fxu_Lit * pLit2, * bLit2Next;

	// initialize the starting literals
	pLit1     = pCubeUse->lLits.pHead;
	pLit2     = pCubeRem->lLits.pHead;
	bLit1Next = pLit1? pLit1->pHNext: NULL;
	bLit2Next = pLit2? pLit2->pHNext: NULL;
	// go through the pair and remove the literals in the base
	// from the first cube and all literals from the second cube
	while ( 1 )
	{
		if ( pLit1 && pLit2 )
		{
			if ( pLit1->iVar == pLit2->iVar )
            {  // this literal is present in both cubes - it belongs to the base
                // mark the affected var
                if ( pLit1->pVar->pOrder == NULL )
                    Fxu_MatrixRingVarsAdd( p, pLit1->pVar );
                // leave the base in pCubeUse; delete it from pCubeRem
				Fxu_MatrixDelLiteral( p, pLit2 );
				// step to the next literals
				pLit1     = bLit1Next;
				pLit2     = bLit2Next;
				bLit1Next = pLit1? pLit1->pHNext: NULL;
				bLit2Next = pLit2? pLit2->pHNext: NULL;
			}
			else if ( pLit1->iVar < pLit2->iVar )
			{ // this literal is present in pCubeUse - remove it
                // mark the affected var
                if ( pLit1->pVar->pOrder == NULL )
                    Fxu_MatrixRingVarsAdd( p, pLit1->pVar );
                // delete this literal
                Fxu_MatrixDelLiteral( p, pLit1 );
				// step to the next literals
				pLit1     = bLit1Next;
				bLit1Next = pLit1? pLit1->pHNext: NULL;
			}
			else
			{ // this literal is present in pCubeRem - remove it
                // mark the affected var
                if ( pLit2->pVar->pOrder == NULL )
                    Fxu_MatrixRingVarsAdd( p, pLit2->pVar );
                // delete this literal
				Fxu_MatrixDelLiteral( p, pLit2 );
				// step to the next literals
				pLit2     = bLit2Next;
				bLit2Next = pLit2? pLit2->pHNext: NULL;
			}
		}
		else if ( pLit1 && !pLit2 )
		{ // this literal is present in pCubeUse - leave it
            // mark the affected var
            if ( pLit1->pVar->pOrder == NULL )
                Fxu_MatrixRingVarsAdd( p, pLit1->pVar );
            // delete this literal
			Fxu_MatrixDelLiteral( p, pLit1 );
			// step to the next literals
			pLit1     = bLit1Next;
			bLit1Next = pLit1? pLit1->pHNext: NULL;
		}
		else if ( !pLit1 && pLit2 )
		{ // this literal is present in pCubeRem - remove it
            // mark the affected var
            if ( pLit2->pVar->pOrder == NULL )
                Fxu_MatrixRingVarsAdd( p, pLit2->pVar );
            // delete this literal
			Fxu_MatrixDelLiteral( p, pLit2 );
			// step to the next literals
			pLit2     = bLit2Next;
			bLit2Next = pLit2? pLit2->pHNext: NULL;
		}
		else
			break;
	}
}

/**Function*************************************************************

  Synopsis    [Updates the matrix after selecting a single cube divisor.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_UpdateMatrixSingleClean( Fxu_Matrix * p, Fxu_Var * pVar1, Fxu_Var * pVar2, Fxu_Var * pVarNew )
{
	Fxu_Lit * pLit1, * bLit1Next;
	Fxu_Lit * pLit2, * bLit2Next;

    // initialize the starting literals
	pLit1     = pVar1->lLits.pHead;
	pLit2     = pVar2->lLits.pHead;
	bLit1Next = pLit1? pLit1->pVNext: NULL;
	bLit2Next = pLit2? pLit2->pVNext: NULL;
	while ( 1 )
	{
		if ( pLit1 && pLit2 )
		{
    		if ( pLit1->pCube->pVar->iVar == pLit2->pCube->pVar->iVar )
			{ // these literals coincide 
			    if ( pLit1->iCube == pLit2->iCube )
			    { // these literals coincide 
                
                    // collect the affected cube
                    assert( pLit1->pCube->pOrder == NULL );
                    Fxu_MatrixRingCubesAdd( p, pLit1->pCube );

                    // add the literal to this cube corresponding to the new column
		            Fxu_MatrixAddLiteral( p, pLit1->pCube, pVarNew );
                    // clean the old cubes
		            Fxu_UpdateCleanOldDoubles( p, NULL, pLit1->pCube );

				    // remove the literals 
				    Fxu_MatrixDelLiteral( p, pLit1 );
				    Fxu_MatrixDelLiteral( p, pLit2 );

				    // go to the next literals
				    pLit1     = bLit1Next;
				    pLit2     = bLit2Next;
				    bLit1Next = pLit1? pLit1->pVNext: NULL;
				    bLit2Next = pLit2? pLit2->pVNext: NULL;
			    }
			    else if ( pLit1->iCube < pLit2->iCube )
			    {
				    pLit1     = bLit1Next;
				    bLit1Next = pLit1? pLit1->pVNext: NULL;
			    }
			    else
			    {
				    pLit2     = bLit2Next;
				    bLit2Next = pLit2? pLit2->pVNext: NULL;
			    }
            }
			else if ( pLit1->pCube->pVar->iVar < pLit2->pCube->pVar->iVar )
			{
				pLit1     = bLit1Next;
				bLit1Next = pLit1? pLit1->pVNext: NULL;
			}
			else
			{
				pLit2     = bLit2Next;
				bLit2Next = pLit2? pLit2->pVNext: NULL;
			}
		}
		else if ( pLit1 && !pLit2 )
		{
			pLit1     = bLit1Next;
			bLit1Next = pLit1? pLit1->pVNext: NULL;
		}
		else if ( !pLit1 && pLit2 )
		{
			pLit2     = bLit2Next;
			bLit2Next = pLit2? pLit2->pVNext: NULL;
		}
		else
			break;
	}
}

/**Function*************************************************************

  Synopsis    [Sort the pairs.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_UpdatePairsSort( Fxu_Matrix * p, Fxu_Double * pDouble )
{
    Fxu_Pair * pPair;
    // order the pairs by the first cube to ensure that new literals are added 
    // to the matrix from top to bottom - collect pairs into the array
    p->vPairs->nSize = 0;
	Fxu_DoubleForEachPair( pDouble, pPair )
        Vec_PtrPush( p->vPairs, pPair );
    if ( p->vPairs->nSize < 2 )
        return;
    // sort
    qsort( (void *)p->vPairs->pArray, p->vPairs->nSize, sizeof(Fxu_Pair *), 
        (int (*)(const void *, const void *)) Fxu_UpdatePairCompare );
    assert( Fxu_UpdatePairCompare( (Fxu_Pair**)p->vPairs->pArray, (Fxu_Pair**)p->vPairs->pArray + p->vPairs->nSize - 1 ) < 0 );
}

/**Function*************************************************************

  Synopsis    [Compares the vars by their number.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fxu_UpdatePairCompare( Fxu_Pair ** ppP1, Fxu_Pair ** ppP2 )
{
    Fxu_Cube * pC1 = (*ppP1)->pCube1;
    Fxu_Cube * pC2 = (*ppP2)->pCube1;
    int iP1CubeMin, iP2CubeMin;
    if ( pC1->pVar->iVar < pC2->pVar->iVar )
        return -1;
    if ( pC1->pVar->iVar > pC2->pVar->iVar )
        return 1;
    iP1CubeMin = Fxu_PairMinCubeInt( *ppP1 );
    iP2CubeMin = Fxu_PairMinCubeInt( *ppP2 );
    if ( iP1CubeMin < iP2CubeMin )
        return -1;
    if ( iP1CubeMin > iP2CubeMin )
        return 1;
    assert( 0 );
    return 0;
}


/**Function*************************************************************

  Synopsis    [Create new variables.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_UpdateCreateNewVars( Fxu_Matrix * p, Fxu_Var ** ppVarC, Fxu_Var ** ppVarD, int nCubes )
{
    Fxu_Var * pVarC, * pVarD;

	// add a new column for the complement
	pVarC = Fxu_MatrixAddVar( p );
    pVarC->nCubes = 0;
	// add a new column for the divisor
	pVarD = Fxu_MatrixAddVar( p );
    pVarD->nCubes = nCubes;

    // mark this entry in the Value2Node array
//    assert( p->pValue2Node[pVarC->iVar] > 0 );
//    p->pValue2Node[pVarD->iVar  ] = p->pValue2Node[pVarC->iVar];
//    p->pValue2Node[pVarD->iVar+1] = p->pValue2Node[pVarC->iVar]+1;

    *ppVarC = pVarC;
    *ppVarD = pVarD;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_UpdateCleanOldDoubles( Fxu_Matrix * p, Fxu_Double * pDiv, Fxu_Cube * pCube )
{
    Fxu_Double * pDivCur;
	Fxu_Pair * pPair;
    int i;

    // if the cube is a recently introduced one
    // it does not have pairs allocated
    // in this case, there is nothing to update
    if ( pCube->pVar->ppPairs == NULL )
        return;

	// go through all the pairs of this cube
	Fxu_CubeForEachPair( pCube, pPair, i )
	{
         // get the divisor of this pair
        pDivCur = pPair->pDiv;
		// skip the current divisor
		if ( pDivCur == pDiv )
			continue;
		// remove this pair
	    Fxu_ListDoubleDelPair( pDivCur, pPair );	
		// the divisor may have become useless by now
		if ( pDivCur->lPairs.nItems == 0 )
        {
            assert( pDivCur->Weight == pPair->nBase - 1 );
      		Fxu_HeapDoubleDelete( p->pHeapDouble, pDivCur );
			Fxu_MatrixDelDivisor( p, pDivCur );
        }
        else
        {
	        // update the divisor's weight
	        pDivCur->Weight -= pPair->nLits1 + pPair->nLits2 - 1 + pPair->nBase;
      	    Fxu_HeapDoubleUpdate( p->pHeapDouble, pDivCur );
        }
		MEM_FREE_FXU( p, Fxu_Pair, 1, pPair );
	}
	// finally erase all the pair info associated with this cube
	Fxu_PairClearStorage( pCube );
}

/**Function*************************************************************

  Synopsis    [Adds the new divisors that depend on the cube.]

  Description [Go through all the non-empty cubes of this cover 
  (except the given cube) and, for each of them, add the new divisor 
  with the given cube.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_UpdateAddNewDoubles( Fxu_Matrix * p, Fxu_Cube * pCube )
{
	Fxu_Cube * pTemp;
    assert( pCube->pOrder );

    // if the cube is a recently introduced one
    // it does not have pairs allocated
    // in this case, there is nothing to update
    if ( pCube->pVar->ppPairs == NULL )
        return;

	for ( pTemp = pCube->pFirst; pTemp->pVar == pCube->pVar; pTemp = pTemp->pNext )
    {
        // do not add pairs with the empty cubes
		if ( pTemp->lLits.nItems == 0 )
            continue;
        // to prevent adding duplicated pairs of the new cubes
        // do not add the pair, if the current cube is marked 
        if ( pTemp->pOrder && pTemp->iCube >= pCube->iCube )
            continue;
		Fxu_MatrixAddDivisor( p, pTemp, pCube );
    }
}

/**Function*************************************************************

  Synopsis    [Removes old single cube divisors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_UpdateCleanOldSingles( Fxu_Matrix * p )
{ 
    Fxu_Single * pSingle, * pSingle2;
    int WeightNew;
    int Counter = 0;

    Fxu_MatrixForEachSingleSafe( p, pSingle, pSingle2 )
    {
        // if at least one of the variables is marked, recalculate
        if ( pSingle->pVar1->pOrder || pSingle->pVar2->pOrder )
        {
            Counter++;
            // get the new weight
            WeightNew = -2 + Fxu_SingleCountCoincidence( p, pSingle->pVar1, pSingle->pVar2 );
            if ( WeightNew >= 0 )
            {
                pSingle->Weight = WeightNew;
                Fxu_HeapSingleUpdate( p->pHeapSingle, pSingle );
            }
            else
            {
                Fxu_HeapSingleDelete( p->pHeapSingle, pSingle );
                Fxu_ListMatrixDelSingle( p, pSingle );
		        MEM_FREE_FXU( p, Fxu_Single, 1, pSingle );
            }
        }
    }
//    printf( "Called procedure %d times.\n", Counter );
}

/**Function*************************************************************

  Synopsis    [Updates the single cube divisors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_UpdateAddNewSingles( Fxu_Matrix * p, Fxu_Var * pVar )
{
    Fxu_MatrixComputeSinglesOne( p, pVar );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

