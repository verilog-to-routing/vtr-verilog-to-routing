/**CFile****************************************************************

  FileName    [fxuPrint.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Various printing procedures.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fxuPrint.c,v 1.0 2003/02/01 00:00:00 alanmi Exp $]

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
void Fxu_MatrixPrint( FILE * pFile, Fxu_Matrix * p )
{
    Fxu_Var * pVar;
    Fxu_Cube * pCube;
    Fxu_Double * pDiv;
    Fxu_Single * pSingle;
    Fxu_Lit * pLit;
    Fxu_Pair * pPair;
    int i, LastNum;
    int fStdout;

    fStdout = 1;
    if ( pFile == NULL )
    {
        pFile = fopen( "matrix.txt", "w" );
        fStdout = 0;
    }

    fprintf( pFile, "Matrix has %d vars, %d cubes, %d literals, %d divisors.\n", 
        p->lVars.nItems, p->lCubes.nItems, p->nEntries, p->nDivs );
    fprintf( pFile, "Divisors selected so far: single = %d, double = %d.\n", 
        p->nDivs1, p->nDivs2 );
    fprintf( pFile, "\n" );

    // print the numbers on top of the matrix
    for ( i = 0; i < 12; i++ )
        fprintf( pFile, " " );
    Fxu_MatrixForEachVariable( p, pVar )
        fprintf( pFile, "%d", pVar->iVar % 10 );
    fprintf( pFile, "\n" );

    // print the rows
    Fxu_MatrixForEachCube( p, pCube )
    {
        fprintf( pFile, "%4d", pCube->iCube );
        fprintf( pFile, "  " );
        fprintf( pFile, "%4d", pCube->pVar->iVar );
        fprintf( pFile, "  " );

        // print the literals
        LastNum = -1;
        Fxu_CubeForEachLiteral( pCube, pLit )
        {
            for ( i = LastNum + 1; i < pLit->pVar->iVar; i++ )
                fprintf( pFile, "." );
            fprintf( pFile, "1" );
            LastNum = i;
        }
        for ( i = LastNum + 1; i < p->lVars.nItems; i++ )
            fprintf( pFile, "." );
        fprintf( pFile, "\n" );
    }
    fprintf( pFile, "\n" );

    // print the double-cube divisors
    fprintf( pFile, "The double divisors are:\n" );
    Fxu_MatrixForEachDouble( p, pDiv, i )
    {
        fprintf( pFile, "Divisor #%3d (lit=%d,%d) (w=%2d):  ", 
            pDiv->Num, pDiv->lPairs.pHead->nLits1, 
            pDiv->lPairs.pHead->nLits2, pDiv->Weight );
        Fxu_DoubleForEachPair( pDiv, pPair )
            fprintf( pFile, " <%d, %d> (b=%d)", 
                pPair->pCube1->iCube, pPair->pCube2->iCube, pPair->nBase );
        fprintf( pFile, "\n" );
    }
    fprintf( pFile, "\n" );

    // print the divisors associated with each cube
    fprintf( pFile, "The cubes are:\n" );
    Fxu_MatrixForEachCube( p, pCube )
    {
        fprintf( pFile, "Cube #%3d: ", pCube->iCube );
        if ( pCube->pVar->ppPairs )
        {
            Fxu_CubeForEachPair( pCube, pPair, i )
                fprintf( pFile, " <%d %d> (d=%d) (b=%d)", 
                    pPair->iCube1, pPair->iCube2, pPair->pDiv->Num, pPair->nBase );
        }
        fprintf( pFile, "\n" );
    }
    fprintf( pFile, "\n" );

    // print the single-cube divisors
    fprintf( pFile, "The single divisors are:\n" );
    Fxu_MatrixForEachSingle( p, pSingle )
    {
        fprintf( pFile, "Single-cube divisor #%5d: Var1 = %4d. Var2 = %4d. Weight = %2d\n", 
            pSingle->Num, pSingle->pVar1->iVar, pSingle->pVar2->iVar, pSingle->Weight );
    }
    fprintf( pFile, "\n" );

/*
    {
        int Index;
        fprintf( pFile, "Distribution of divisors in the hash table:\n" );
        for ( Index = 0; Index < p->nTableSize; Index++ )
            fprintf( pFile, " %d", p->pTable[Index].nItems );
        fprintf( pFile, "\n" );
    }
*/
    if ( !fStdout )
        fclose( pFile );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_MatrixPrintDivisorProfile( FILE * pFile, Fxu_Matrix * p )
{
    Fxu_Double * pDiv;
    int WeightMax;
    int * pProfile;
    int Counter1; // the number of -1 weight
    int CounterL; // the number of less than -1 weight
    int i;

    WeightMax = Fxu_HeapDoubleReadMaxWeight( p->pHeapDouble );
    pProfile = ABC_ALLOC( int, (WeightMax + 1) );
    memset( pProfile, 0, sizeof(int) * (WeightMax + 1) );

    Counter1 = 0;
    CounterL = 0;
    Fxu_MatrixForEachDouble( p, pDiv, i )
    {
        assert( pDiv->Weight <= WeightMax );
        if ( pDiv->Weight == -1 )
            Counter1++;
        else if ( pDiv->Weight < 0 )
            CounterL++;
        else
            pProfile[ pDiv->Weight ]++;
    }

    fprintf( pFile, "The double divisors profile:\n" );
    fprintf( pFile, "Weight  < -1 divisors = %6d\n", CounterL );
    fprintf( pFile, "Weight    -1 divisors = %6d\n", Counter1 );
    for ( i = 0; i <= WeightMax; i++ )
        if ( pProfile[i] )
            fprintf( pFile, "Weight   %3d divisors = %6d\n", i, pProfile[i] );
    fprintf( pFile, "End of divisor profile printout\n" );
    ABC_FREE( pProfile );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

