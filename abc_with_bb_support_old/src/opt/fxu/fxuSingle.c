/**CFile****************************************************************

  FileName    [fxuSingle.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Procedures to compute the set of single-cube divisors.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fxuSingle.c,v 1.0 2003/02/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fxuInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes and adds all single-cube divisors to storage.]

  Description [This procedure should be called once when the matrix is
  already contructed before the process of logic extraction begins..]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_MatrixComputeSingles( Fxu_Matrix * p )
{
    Fxu_Var * pVar;
    // iterate through the columns in the matrix
    Fxu_MatrixForEachVariable( p, pVar )
        Fxu_MatrixComputeSinglesOne( p, pVar );
}

/**Function*************************************************************

  Synopsis    [Adds the single-cube divisors associated with a new column.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_MatrixComputeSinglesOne( Fxu_Matrix * p, Fxu_Var * pVar )
{
//    int * pValue2Node = p->pValue2Node;
    Fxu_Lit * pLitV, * pLitH;
    Fxu_Var * pVar2;
    int Coin;
//    int CounterAll;
//    int CounterTest;
    int WeightCur;

    // start collecting the affected vars
    Fxu_MatrixRingVarsStart( p );
    // go through all the literals of this variable
    for ( pLitV = pVar->lLits.pHead; pLitV; pLitV = pLitV->pVNext )
        // for this literal, go through all the horizontal literals
        for ( pLitH = pLitV->pHPrev; pLitH; pLitH = pLitH->pHPrev )
        {
            // get another variable
            pVar2 = pLitH->pVar;
//            CounterAll++;
            // skip the var if it is already used
            if ( pVar2->pOrder )
                continue;
            // skip the var if it belongs to the same node
//            if ( pValue2Node[pVar->iVar] == pValue2Node[pVar2->iVar] )
//                continue;
            // collect the var
            Fxu_MatrixRingVarsAdd( p, pVar2 );
        }
    // stop collecting the selected vars
    Fxu_MatrixRingVarsStop( p );

    // iterate through the selected vars
    Fxu_MatrixForEachVarInRing( p, pVar2 )
    {
//        CounterTest++;
        // count the coincidence
        Coin = Fxu_SingleCountCoincidence( p, pVar2, pVar );
        assert( Coin > 0 );
        // get the new weight
        WeightCur = Coin - 2;
        if ( WeightCur >= 0 )
            Fxu_MatrixAddSingle( p, pVar2, pVar, WeightCur );
    }

    // unmark the vars
    Fxu_MatrixRingVarsUnmark( p );
}

/**Function*************************************************************

  Synopsis    [Computes the coincidence count of two columns.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fxu_SingleCountCoincidence( Fxu_Matrix * p, Fxu_Var * pVar1, Fxu_Var * pVar2 )
{
	Fxu_Lit * pLit1, * pLit2;
	int Result;

	// compute the coincidence count
	Result = 0;
	pLit1  = pVar1->lLits.pHead;
	pLit2  = pVar2->lLits.pHead;
	while ( 1 )
	{
		if ( pLit1 && pLit2 )
		{
			if ( pLit1->pCube->pVar->iVar == pLit2->pCube->pVar->iVar )
			{ // the variables are the same
			    if ( pLit1->iCube == pLit2->iCube )
			    { // the literals are the same
				    pLit1 = pLit1->pVNext;
				    pLit2 = pLit2->pVNext;
				    // add this literal to the coincidence
				    Result++;
			    }
			    else if ( pLit1->iCube < pLit2->iCube )
				    pLit1 = pLit1->pVNext;
			    else
				    pLit2 = pLit2->pVNext;
			}
			else if ( pLit1->pCube->pVar->iVar < pLit2->pCube->pVar->iVar )
				pLit1 = pLit1->pVNext;
			else
				pLit2 = pLit2->pVNext;
		}
		else if ( pLit1 && !pLit2 )
			pLit1 = pLit1->pVNext;
		else if ( !pLit1 && pLit2 )
			pLit2 = pLit2->pVNext;
		else
			break;
	}
	return Result;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


