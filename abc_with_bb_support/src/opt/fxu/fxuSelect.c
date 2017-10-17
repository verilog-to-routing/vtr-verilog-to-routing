/**CFile****************************************************************

  FileName    [fxuSelect.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Procedures to select the best divisor/complement pair.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fxuSelect.c,v 1.0 2003/02/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fxuInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define MAX_SIZE_LOOKAHEAD      20

static int Fxu_MatrixFindComplement( Fxu_Matrix * p, int iVar );

static Fxu_Double * Fxu_MatrixFindComplementSingle( Fxu_Matrix * p, Fxu_Single * pSingle );
static Fxu_Single * Fxu_MatrixFindComplementDouble2( Fxu_Matrix * p, Fxu_Double * pDouble );
static Fxu_Double * Fxu_MatrixFindComplementDouble4( Fxu_Matrix * p, Fxu_Double * pDouble );

Fxu_Double * Fxu_MatrixFindDouble( Fxu_Matrix * p, 
     int piVarsC1[], int piVarsC2[], int nVarsC1, int nVarsC2 );
void Fxu_MatrixGetDoubleVars( Fxu_Matrix * p, Fxu_Double * pDouble, 
    int piVarsC1[], int piVarsC2[], int * pnVarsC1, int * pnVarsC2 );


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Selects the best pair (Single,Double) and returns their weight.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fxu_Select( Fxu_Matrix * p, Fxu_Single ** ppSingle, Fxu_Double ** ppDouble )
{
    // the top entries
    Fxu_Single * pSingles[MAX_SIZE_LOOKAHEAD] = {0};
    Fxu_Double * pDoubles[MAX_SIZE_LOOKAHEAD] = {0};
    // the complements
    Fxu_Double * pSCompl[MAX_SIZE_LOOKAHEAD] = {0};
    Fxu_Single * pDComplS[MAX_SIZE_LOOKAHEAD] = {0};
    Fxu_Double * pDComplD[MAX_SIZE_LOOKAHEAD] = {0};
    Fxu_Pair * pPair;
    int nSingles;
    int nDoubles;
    int i;
    int WeightBest;
    int WeightCur;
    int iNum, fBestS;

    // collect the top entries from the queues
    for ( nSingles = 0; nSingles < MAX_SIZE_LOOKAHEAD; nSingles++ )
    {
        pSingles[nSingles] = Fxu_HeapSingleGetMax( p->pHeapSingle );
        if ( pSingles[nSingles] == NULL )
            break;
    }
    // put them back into the queue
    for ( i = 0; i < nSingles; i++ )
        if ( pSingles[i] )
            Fxu_HeapSingleInsert( p->pHeapSingle, pSingles[i] );
        
    // the same for doubles
    // collect the top entries from the queues
    for ( nDoubles = 0; nDoubles < MAX_SIZE_LOOKAHEAD; nDoubles++ )
    {
        pDoubles[nDoubles] = Fxu_HeapDoubleGetMax( p->pHeapDouble );
        if ( pDoubles[nDoubles] == NULL )
            break;
    }
    // put them back into the queue
    for ( i = 0; i < nDoubles; i++ )
        if ( pDoubles[i] )
            Fxu_HeapDoubleInsert( p->pHeapDouble, pDoubles[i] );

    // for each single, find the complement double (if any)
    for ( i = 0; i < nSingles; i++ )
        if ( pSingles[i] )
            pSCompl[i] = Fxu_MatrixFindComplementSingle( p, pSingles[i] );

    // for each double, find the complement single or double (if any)
    for ( i = 0; i < nDoubles; i++ )
        if ( pDoubles[i] )
        {
            pPair = pDoubles[i]->lPairs.pHead;
            if ( pPair->nLits1 == 1 && pPair->nLits2 == 1 )
            {
                pDComplS[i] = Fxu_MatrixFindComplementDouble2( p, pDoubles[i] );
                pDComplD[i] = NULL;
            }
//            else if ( pPair->nLits1 == 2 && pPair->nLits2 == 2 )
//            {
//                pDComplS[i] = NULL;
//                pDComplD[i] = Fxu_MatrixFindComplementDouble4( p, pDoubles[i] );
//            }
            else
            {
                pDComplS[i] = NULL;
                pDComplD[i] = NULL;
            }
        }

    // select the best pair
    WeightBest = -1;
    for ( i = 0; i < nSingles; i++ )
    {
        WeightCur = pSingles[i]->Weight;
        if ( pSCompl[i] )
        {
            // add the weight of the double
            WeightCur += pSCompl[i]->Weight;
            // there is no need to implement this double, so...
            pPair      = pSCompl[i]->lPairs.pHead;
            WeightCur += pPair->nLits1 + pPair->nLits2;
        }
        if ( WeightBest < WeightCur )
        {
            WeightBest = WeightCur;
            *ppSingle = pSingles[i];
            *ppDouble = pSCompl[i];
            fBestS = 1;
            iNum = i;
        }
    }
    for ( i = 0; i < nDoubles; i++ )
    {
        WeightCur = pDoubles[i]->Weight;
        if ( pDComplS[i] )
        {
            // add the weight of the single
            WeightCur += pDComplS[i]->Weight;
            // there is no need to implement this double, so...
            pPair      = pDoubles[i]->lPairs.pHead;
            WeightCur += pPair->nLits1 + pPair->nLits2;
        }
        if ( WeightBest < WeightCur )
        {
            WeightBest = WeightCur;
            *ppSingle = pDComplS[i];
            *ppDouble = pDoubles[i];
            fBestS = 0;
            iNum = i;
        }
    }
/*
    // print the statistics
    printf( "\n" );
    for ( i = 0; i < nSingles; i++ )
    {
        printf( "Single #%d: Weight = %3d. ", i, pSingles[i]->Weight );
        printf( "Compl: " );
        if ( pSCompl[i] == NULL )
            printf( "None." );
        else
            printf( "D  Weight = %3d  Sum = %3d", 
                pSCompl[i]->Weight, pSCompl[i]->Weight + pSingles[i]->Weight );
        printf( "\n" );
    }
    printf( "\n" );
    for ( i = 0; i < nDoubles; i++ )
    {
        printf( "Double #%d: Weight = %3d. ", i, pDoubles[i]->Weight );
        printf( "Compl: " );
        if ( pDComplS[i] == NULL && pDComplD[i] == NULL )
            printf( "None." );
        else if ( pDComplS[i] )
            printf( "S  Weight = %3d  Sum = %3d", 
                pDComplS[i]->Weight, pDComplS[i]->Weight + pDoubles[i]->Weight );
        else if ( pDComplD[i] )
            printf( "D  Weight = %3d  Sum = %3d", 
                pDComplD[i]->Weight, pDComplD[i]->Weight + pDoubles[i]->Weight );
        printf( "\n" );
    }
    if ( WeightBest == -1 )
        printf( "Selected NONE\n" );
    else
    {
        printf( "Selected = %s.  ", fBestS? "S": "D" );
        printf( "Number = %d.  ", iNum );
        printf( "Weight = %d.\n", WeightBest );
    }
    printf( "\n" );
*/
    return WeightBest;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fxu_Double * Fxu_MatrixFindComplementSingle( Fxu_Matrix * p, Fxu_Single * pSingle )
{
//    int * pValue2Node = p->pValue2Node;
    int iVar1,  iVar2;
    int iVar1C, iVar2C;
    // get the variables of this single div
    iVar1  = pSingle->pVar1->iVar;
    iVar2  = pSingle->pVar2->iVar;
    iVar1C = Fxu_MatrixFindComplement( p, iVar1 );
    iVar2C = Fxu_MatrixFindComplement( p, iVar2 );
    if ( iVar1C == -1 || iVar2C == -1 )
        return NULL;
    assert( iVar1C < iVar2C );
    return Fxu_MatrixFindDouble( p, &iVar1C, &iVar2C, 1, 1 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fxu_Single * Fxu_MatrixFindComplementDouble2( Fxu_Matrix * p, Fxu_Double * pDouble )
{
//    int * pValue2Node = p->pValue2Node;
    int piVarsC1[10], piVarsC2[10];
    int nVarsC1, nVarsC2;
    int iVar1,  iVar2, iVarTemp;
    int iVar1C, iVar2C;
    Fxu_Single * pSingle;

    // get the variables of this double div
    Fxu_MatrixGetDoubleVars( p, pDouble, piVarsC1, piVarsC2, &nVarsC1, &nVarsC2 );
    assert( nVarsC1 == 1 );
    assert( nVarsC2 == 1 );
    iVar1 = piVarsC1[0];
    iVar2 = piVarsC2[0];
    assert( iVar1 < iVar2 );

    iVar1C = Fxu_MatrixFindComplement( p, iVar1 );
    iVar2C = Fxu_MatrixFindComplement( p, iVar2 );
    if ( iVar1C == -1 || iVar2C == -1 )
        return NULL;
 
    // go through the queque and find this one
//    assert( iVar1C < iVar2C );
    if ( iVar1C > iVar2C )
    {
        iVarTemp = iVar1C;
        iVar1C = iVar2C;
        iVar2C = iVarTemp;
    }

    Fxu_MatrixForEachSingle( p, pSingle )
        if ( pSingle->pVar1->iVar == iVar1C && pSingle->pVar2->iVar == iVar2C )
            return pSingle;
    return NULL;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fxu_Double * Fxu_MatrixFindComplementDouble4( Fxu_Matrix * p, Fxu_Double * pDouble )
{
//    int * pValue2Node = p->pValue2Node;
    int piVarsC1[10], piVarsC2[10];
    int nVarsC1, nVarsC2;
    int iVar11,  iVar12,  iVar21,  iVar22;
    int iVar11C, iVar12C, iVar21C, iVar22C;
    int RetValue;

    // get the variables of this double div
    Fxu_MatrixGetDoubleVars( p, pDouble, piVarsC1, piVarsC2, &nVarsC1, &nVarsC2 );
    assert( nVarsC1 == 2 && nVarsC2 == 2 );

    iVar11 = piVarsC1[0];
    iVar12 = piVarsC1[1];
    iVar21 = piVarsC2[0];
    iVar22 = piVarsC2[1];
    assert( iVar11 < iVar21 );

    iVar11C = Fxu_MatrixFindComplement( p, iVar11 );
    iVar12C = Fxu_MatrixFindComplement( p, iVar12 );
    iVar21C = Fxu_MatrixFindComplement( p, iVar21 );
    iVar22C = Fxu_MatrixFindComplement( p, iVar22 );
    if ( iVar11C == -1 || iVar12C == -1 || iVar21C == -1 || iVar22C == -1 )
        return NULL;
    if ( iVar11C != iVar21 || iVar12C != iVar22 || 
         iVar21C != iVar11 || iVar22C != iVar12 )
         return NULL;

    // a'b' + ab   =>  a'b  + ab'
    // a'b  + ab'  =>  a'b' + ab
    // swap the second pair in each cube
    RetValue    = piVarsC1[1];
    piVarsC1[1] = piVarsC2[1];
    piVarsC2[1] = RetValue;

    return Fxu_MatrixFindDouble( p, piVarsC1, piVarsC2, 2, 2 );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fxu_MatrixFindComplement( Fxu_Matrix * p, int iVar )
{
    return iVar ^ 1;
/*
//    int * pValue2Node = p->pValue2Node;
    int iVarC;
    int iNode;
    int Beg, End;

    // get the nodes
    iNode = pValue2Node[iVar];
    // get the first node with the same var
    for ( Beg = iVar; Beg >= 0; Beg-- )
        if ( pValue2Node[Beg] != iNode )
        {
            Beg++;
            break;
        }
    // get the last node with the same var
    for ( End = iVar;          ; End++ )
        if ( pValue2Node[End] != iNode )
        {
            End--;
            break;
        }

    // if one of the vars is not binary, quit
    if ( End - Beg > 1 )
        return -1;

    // get the complements
    if ( iVar == Beg )
        iVarC = End;
    else if ( iVar == End ) 
        iVarC = Beg;
    else
    {
        assert( 0 );
    }
    return iVarC;
*/
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_MatrixGetDoubleVars( Fxu_Matrix * p, Fxu_Double * pDouble, 
    int piVarsC1[], int piVarsC2[], int * pnVarsC1, int * pnVarsC2 )
{
    Fxu_Pair * pPair;
	Fxu_Lit * pLit1, * pLit2;
    int nLits1, nLits2;

    // get the first pair
    pPair = pDouble->lPairs.pHead;
    // init the parameters
	nLits1 = 0;
	nLits2 = 0;
	pLit1 = pPair->pCube1->lLits.pHead;
	pLit2 = pPair->pCube2->lLits.pHead;
	while ( 1 )
	{
		if ( pLit1 && pLit2 )
		{
			if ( pLit1->iVar == pLit2->iVar )
			{ // ensure cube-free
				pLit1 = pLit1->pHNext;
				pLit2 = pLit2->pHNext;
			}
			else if ( pLit1->iVar < pLit2->iVar )
            {
                piVarsC1[nLits1++] = pLit1->iVar;
 				pLit1 = pLit1->pHNext;
           }
			else
            {
                piVarsC2[nLits2++] = pLit2->iVar;
				pLit2 = pLit2->pHNext;
            }
		}
		else if ( pLit1 && !pLit2 )
        {
            piVarsC1[nLits1++] = pLit1->iVar;
    		pLit1 = pLit1->pHNext;
        }
		else if ( !pLit1 && pLit2 )
        {
            piVarsC2[nLits2++] = pLit2->iVar;
			pLit2 = pLit2->pHNext;
        }
		else
			break;
	}
    *pnVarsC1 = nLits1;
    *pnVarsC2 = nLits2;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fxu_Double * Fxu_MatrixFindDouble( Fxu_Matrix * p, 
     int piVarsC1[], int piVarsC2[], int nVarsC1, int nVarsC2 )
{
    int piVarsC1_[100], piVarsC2_[100];
    int nVarsC1_, nVarsC2_, i;
    Fxu_Double * pDouble;
    Fxu_Pair * pPair;
    unsigned Key;

    assert( nVarsC1 > 0 );
    assert( nVarsC2 > 0 );
    assert( piVarsC1[0] < piVarsC2[0] );

    // get the hash key
    Key = Fxu_PairHashKeyArray( p, piVarsC1, piVarsC2, nVarsC1, nVarsC2 );
    
    // check if the divisor for this pair already exists
    Key %= p->nTableSize;
	Fxu_TableForEachDouble( p, Key, pDouble )
    {
        pPair = pDouble->lPairs.pHead;
        if ( pPair->nLits1 != nVarsC1 )
            continue;
        if ( pPair->nLits2 != nVarsC2 )
            continue;
        // get the cubes of this divisor
        Fxu_MatrixGetDoubleVars( p, pDouble, piVarsC1_, piVarsC2_, &nVarsC1_, &nVarsC2_ );
        // compare lits of the first cube
        for ( i = 0; i < nVarsC1; i++ )
            if ( piVarsC1[i] != piVarsC1_[i] )
                break;
        if ( i != nVarsC1 )
            continue;
        // compare lits of the second cube
        for ( i = 0; i < nVarsC2; i++ )
            if ( piVarsC2[i] != piVarsC2_[i] )
                break;
        if ( i != nVarsC2 )
            continue;
        return pDouble;
    }
    return NULL;
}





/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fxu_SelectSCD( Fxu_Matrix * p, int WeightLimit, Fxu_Var ** ppVar1, Fxu_Var ** ppVar2 )
{
//    int * pValue2Node = p->pValue2Node;
    Fxu_Var * pVar1;
    Fxu_Var * pVar2, * pVarTemp;
    Fxu_Lit * pLitV, * pLitH;
    int Coin;
    int CounterAll;
    int CounterTest;
    int WeightCur;
    int WeightBest;

    CounterAll = 0;
    CounterTest = 0;

    WeightBest = -10;
    
    // iterate through the columns in the matrix
    Fxu_MatrixForEachVariable( p, pVar1 )
    {
        // start collecting the affected vars
        Fxu_MatrixRingVarsStart( p );

        // go through all the literals of this variable
        for ( pLitV = pVar1->lLits.pHead; pLitV; pLitV = pLitV->pVNext )
        {
            // for this literal, go through all the horizontal literals
            for ( pLitH = pLitV->pHNext; pLitH; pLitH = pLitH->pHNext )
            {
                // get another variable
                pVar2 = pLitH->pVar;
                CounterAll++;
                // skip the var if it is already used
                if ( pVar2->pOrder )
                    continue;
                // skip the var if it belongs to the same node
//                if ( pValue2Node[pVar1->iVar] == pValue2Node[pVar2->iVar] )
//                    continue;
                // collect the var
                Fxu_MatrixRingVarsAdd( p, pVar2 );
            }
        }
        // stop collecting the selected vars
        Fxu_MatrixRingVarsStop( p );

        // iterate through the selected vars
        Fxu_MatrixForEachVarInRing( p, pVar2 )
        {
            CounterTest++;

            // count the coincidence
            Coin = Fxu_SingleCountCoincidence( p, pVar1, pVar2 );
            assert( Coin > 0 );

            // get the new weight
            WeightCur = Coin - 2;

            // compare the weights
            if ( WeightBest < WeightCur )
            {
                WeightBest = WeightCur;
                *ppVar1 = pVar1;
                *ppVar2 = pVar2;
            }
        }
        // unmark the vars
        Fxu_MatrixForEachVarInRingSafe( p, pVar2, pVarTemp )
            pVar2->pOrder = NULL;
        Fxu_MatrixRingVarsReset( p );
    }

//    if ( WeightBest == WeightLimit )
//        return -1;
    return WeightBest;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

