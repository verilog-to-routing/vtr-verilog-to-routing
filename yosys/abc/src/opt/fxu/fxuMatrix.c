/**CFile****************************************************************

  FileName    [fxuMatrix.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Procedures to manipulate the sparse matrix.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fxuMatrix.c,v 1.0 2003/02/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fxuInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fxu_Matrix * Fxu_MatrixAllocate()
{
    Fxu_Matrix * p;
    p = ABC_ALLOC( Fxu_Matrix, 1 );
    memset( p, 0, sizeof(Fxu_Matrix) );
    p->nTableSize = Abc_PrimeCudd(10000);
    p->pTable = ABC_ALLOC( Fxu_ListDouble, p->nTableSize );
    memset( p->pTable, 0, sizeof(Fxu_ListDouble) * p->nTableSize );
#ifndef USE_SYSTEM_MEMORY_MANAGEMENT
    {
        // get the largest size in bytes for the following structures:
        // Fxu_Cube, Fxu_Var, Fxu_Lit, Fxu_Pair, Fxu_Double, Fxu_Single
        // (currently, Fxu_Var, Fxu_Pair, Fxu_Double take 10 machine words)
        int nSizeMax, nSizeCur;
        nSizeMax = -1;
        nSizeCur = sizeof(Fxu_Cube);
        if ( nSizeMax < nSizeCur )
             nSizeMax = nSizeCur;
        nSizeCur = sizeof(Fxu_Var);
        if ( nSizeMax < nSizeCur )
             nSizeMax = nSizeCur;
        nSizeCur = sizeof(Fxu_Lit);
        if ( nSizeMax < nSizeCur )
             nSizeMax = nSizeCur;
        nSizeCur = sizeof(Fxu_Pair);
        if ( nSizeMax < nSizeCur )
             nSizeMax = nSizeCur;
        nSizeCur = sizeof(Fxu_Double);
        if ( nSizeMax < nSizeCur )
             nSizeMax = nSizeCur;
        nSizeCur = sizeof(Fxu_Single);
        if ( nSizeMax < nSizeCur )
             nSizeMax = nSizeCur;
        p->pMemMan  = Extra_MmFixedStart( nSizeMax ); 
    }
#endif
    p->pHeapDouble = Fxu_HeapDoubleStart();
    p->pHeapSingle = Fxu_HeapSingleStart();
    p->vPairs = Vec_PtrAlloc( 100 );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_MatrixDelete( Fxu_Matrix * p )
{
    Fxu_HeapDoubleCheck( p->pHeapDouble );
    Fxu_HeapDoubleStop( p->pHeapDouble );
    Fxu_HeapSingleStop( p->pHeapSingle );

    // delete other things
#ifdef USE_SYSTEM_MEMORY_MANAGEMENT
    // this code is not needed when the custom memory manager is used
    {
        Fxu_Cube * pCube, * pCube2;
        Fxu_Var * pVar, * pVar2;
        Fxu_Lit * pLit, * pLit2;
        Fxu_Double * pDiv, * pDiv2;
        Fxu_Single * pSingle, * pSingle2;
        Fxu_Pair * pPair, * pPair2;
        int i;
        // delete the divisors
        Fxu_MatrixForEachDoubleSafe( p, pDiv, pDiv2, i )
        {
            Fxu_DoubleForEachPairSafe( pDiv, pPair, pPair2 )
                MEM_FREE_FXU( p, Fxu_Pair, 1, pPair );
               MEM_FREE_FXU( p, Fxu_Double, 1, pDiv );
        }
        Fxu_MatrixForEachSingleSafe( p, pSingle, pSingle2 )
               MEM_FREE_FXU( p, Fxu_Single, 1, pSingle );
        // delete the entries
        Fxu_MatrixForEachCube( p, pCube )
            Fxu_CubeForEachLiteralSafe( pCube, pLit, pLit2 )
                MEM_FREE_FXU( p, Fxu_Lit, 1, pLit );
        // delete the cubes
        Fxu_MatrixForEachCubeSafe( p, pCube, pCube2 )
            MEM_FREE_FXU( p, Fxu_Cube, 1, pCube );
        // delete the vars
        Fxu_MatrixForEachVariableSafe( p, pVar, pVar2 )
            MEM_FREE_FXU( p, Fxu_Var, 1, pVar );
    }
#else
    Extra_MmFixedStop( p->pMemMan );
#endif

    Vec_PtrFree( p->vPairs );
    ABC_FREE( p->pppPairs );
    ABC_FREE( p->ppPairs );
//    ABC_FREE( p->pPairsTemp );
    ABC_FREE( p->pTable );
    ABC_FREE( p->ppVars );
    ABC_FREE( p );
}



/**Function*************************************************************

  Synopsis    [Adds a variable to the matrix.]

  Description [This procedure always adds variables at the end of the matrix.
  It assigns the var's node and number. It adds the var to the linked list of
  all variables and to the table of all nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fxu_Var * Fxu_MatrixAddVar( Fxu_Matrix * p )
{
    Fxu_Var * pVar;
    pVar = MEM_ALLOC_FXU( p, Fxu_Var, 1 );
    memset( pVar, 0, sizeof(Fxu_Var) );
    pVar->iVar = p->lVars.nItems;
    p->ppVars[pVar->iVar] = pVar;
    Fxu_ListMatrixAddVariable( p, pVar );
    return pVar;
}

/**Function*************************************************************

  Synopsis    [Adds a literal to the matrix.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fxu_Cube * Fxu_MatrixAddCube( Fxu_Matrix * p, Fxu_Var * pVar, int iCube )
{
    Fxu_Cube * pCube;
    pCube = MEM_ALLOC_FXU( p, Fxu_Cube, 1 );
    memset( pCube, 0, sizeof(Fxu_Cube) );
    pCube->pVar  = pVar;
    pCube->iCube = iCube;
    Fxu_ListMatrixAddCube( p, pCube );
    return pCube;
}

/**Function*************************************************************

  Synopsis    [Adds a literal to the matrix.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_MatrixAddLiteral( Fxu_Matrix * p, Fxu_Cube * pCube, Fxu_Var * pVar )
{
    Fxu_Lit * pLit;
    pLit = MEM_ALLOC_FXU( p, Fxu_Lit, 1 );
    memset( pLit, 0, sizeof(Fxu_Lit) );
    // insert the literal into two linked lists
    Fxu_ListCubeAddLiteral( pCube, pLit );
    Fxu_ListVarAddLiteral( pVar, pLit );
    // set the back pointers
    pLit->pCube = pCube;
    pLit->pVar  = pVar;
    pLit->iCube = pCube->iCube;
    pLit->iVar  = pVar->iVar;
    // increment the literal counter
    p->nEntries++;
}

/**Function*************************************************************

  Synopsis    [Deletes the divisor from the matrix.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_MatrixDelDivisor( Fxu_Matrix * p, Fxu_Double * pDiv )
{
    // delete divisor from the table
    Fxu_ListTableDelDivisor( p, pDiv );
    // recycle the divisor
    MEM_FREE_FXU( p, Fxu_Double, 1, pDiv );
}

/**Function*************************************************************

  Synopsis    [Deletes the literal fromthe matrix.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_MatrixDelLiteral( Fxu_Matrix * p, Fxu_Lit * pLit )
{
    // delete the literal
    Fxu_ListCubeDelLiteral( pLit->pCube, pLit );
    Fxu_ListVarDelLiteral( pLit->pVar, pLit );
    MEM_FREE_FXU( p, Fxu_Lit, 1, pLit );
    // increment the literal counter
    p->nEntries--;
}


/**Function*************************************************************

  Synopsis    [Creates and adds a single cube divisor.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_MatrixAddSingle( Fxu_Matrix * p, Fxu_Var * pVar1, Fxu_Var * pVar2, int Weight )
{
    Fxu_Single * pSingle;
    assert( pVar1->iVar < pVar2->iVar );
    pSingle = MEM_ALLOC_FXU( p, Fxu_Single, 1 );
    memset( pSingle, 0, sizeof(Fxu_Single) );
    pSingle->Num = p->lSingles.nItems;
    pSingle->Weight = Weight;
    pSingle->HNum = 0;
    pSingle->pVar1 = pVar1;
    pSingle->pVar2 = pVar2;
    Fxu_ListMatrixAddSingle( p, pSingle );
    // add to the heap
    Fxu_HeapSingleInsert( p->pHeapSingle, pSingle );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_MatrixAddDivisor( Fxu_Matrix * p, Fxu_Cube * pCube1, Fxu_Cube * pCube2 )
{
    Fxu_Pair * pPair;
    Fxu_Double * pDiv;
    int nBase, nLits1, nLits2;
    int fFound;
    unsigned Key;

    // canonicize the pair
    Fxu_PairCanonicize( &pCube1, &pCube2 );
    // compute the hash key
    Key = Fxu_PairHashKey( p, pCube1, pCube2, &nBase, &nLits1, &nLits2 );

    // create the cube pair
    pPair = Fxu_PairAlloc( p, pCube1, pCube2 );
    pPair->nBase  = nBase;
    pPair->nLits1 = nLits1;
    pPair->nLits2 = nLits2;

    // check if the divisor for this pair already exists
    fFound = 0;
    Key %= p->nTableSize;
    Fxu_TableForEachDouble( p, Key, pDiv )
    {
        if ( Fxu_PairCompare( pPair, pDiv->lPairs.pTail ) ) // they are equal
        {
            fFound = 1;
            break;
        }
    }

    if ( !fFound )
    {   // create the new divisor
        pDiv = MEM_ALLOC_FXU( p, Fxu_Double, 1 );
        memset( pDiv, 0, sizeof(Fxu_Double) );
        pDiv->Key = Key;
        // set the number of this divisor
        pDiv->Num = p->nDivsTotal++; // p->nDivs;
        // insert the divisor in the table
        Fxu_ListTableAddDivisor( p, pDiv );
        // set the initial cost of the divisor
        pDiv->Weight -= pPair->nLits1 + pPair->nLits2;
    }

    // link the pair to the cubes
    Fxu_PairAdd( pPair );
    // connect the pair and the divisor
    pPair->pDiv = pDiv;
    Fxu_ListDoubleAddPairLast( pDiv, pPair );    
    // update the max number of pairs in a divisor
//    if ( p->nPairsMax < pDiv->lPairs.nItems )
//        p->nPairsMax = pDiv->lPairs.nItems;
    // update the divisor's weight
    pDiv->Weight += pPair->nLits1 + pPair->nLits2 - 1 + pPair->nBase;
    if ( fFound ) // update the divisor in the heap
        Fxu_HeapDoubleUpdate( p->pHeapDouble, pDiv );
    else  // add the new divisor to the heap
        Fxu_HeapDoubleInsert( p->pHeapDouble, pDiv );
}

/*
    {
        int piVarsC1[100], piVarsC2[100], nVarsC1, nVarsC2;
        Fxu_Double * pDivCur;
        Fxu_MatrixGetDoubleVars( p, pDiv, piVarsC1, piVarsC2, &nVarsC1, &nVarsC2 );
        pDivCur = Fxu_MatrixFindDouble( p, piVarsC1, piVarsC2, nVarsC1, nVarsC2 );
        assert( pDivCur == pDiv );
    }
*/

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

