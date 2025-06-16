/**CFile****************************************************************

  FileName    [darPrec.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware AIG rewriting.]

  Synopsis    [Truth table precomputation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: darPrec.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "darInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocated one-memory-chunk array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char ** Dar_ArrayAlloc( int nCols, int nRows, int Size )
{
    char ** pRes;
    char * pBuffer;
    int i;
    assert( nCols > 0 && nRows > 0 && Size > 0 );
    pBuffer = ABC_ALLOC( char, nCols * (sizeof(void *) + nRows * Size) );
    pRes = (char **)pBuffer;
    pRes[0] = pBuffer + nCols * sizeof(void *);
    for ( i = 1; i < nCols; i++ )
        pRes[i] = pRes[0] + i * nRows * Size;
    return pRes;
}

/**Function********************************************************************

  Synopsis    [Computes the factorial.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Dar_Factorial( int n )
{
    int i, Res = 1;
    for ( i = 1; i <= n; i++ )
        Res *= i;
    return Res;
}

/**Function********************************************************************

  Synopsis    [Fills in the array of permutations.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Dar_Permutations_rec( char ** pRes, int nFact, int n, char Array[] )
{
    char ** pNext;
    int nFactNext;
    int iTemp, iCur, iLast, k;

    if ( n == 1 )
    {
        pRes[0][0] = Array[0];
        return;
    }

    // get the next factorial
    nFactNext = nFact / n;
    // get the last entry
    iLast = n - 1;

    for ( iCur = 0; iCur < n; iCur++ )
    {
        // swap Cur and Last
        iTemp        = Array[iCur];
        Array[iCur]  = Array[iLast];
        Array[iLast] = iTemp;

        // get the pointer to the current section
        pNext = pRes + (n - 1 - iCur) * nFactNext;

        // set the last entry 
        for ( k = 0; k < nFactNext; k++ )
            pNext[k][iLast] = Array[iLast];

        // call recursively for this part
        Dar_Permutations_rec( pNext, nFactNext, n - 1, Array );

        // swap them back
        iTemp        = Array[iCur];
        Array[iCur]  = Array[iLast];
        Array[iLast] = iTemp;
    }
}

/**Function********************************************************************

  Synopsis    [Computes the set of all permutations.]

  Description [The number of permutations in the array is n!. The number of
  entries in each permutation is n. Therefore, the resulting array is a 
  two-dimentional array of the size: n! x n. To free the resulting array,
  call ABC_FREE() on the pointer returned by this procedure.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
char ** Dar_Permutations( int n )
{
    char Array[50];
    char ** pRes;
    int nFact, i;
    // allocate memory
    nFact = Dar_Factorial( n );
    pRes  = Dar_ArrayAlloc( nFact, n, sizeof(char) );
    // fill in the permutations
    for ( i = 0; i < n; i++ )
        Array[i] = i;
    Dar_Permutations_rec( pRes, nFact, n, Array );
    // print the permutations
/*
    {
    int i, k;
    for ( i = 0; i < nFact; i++ )
    {
        printf( "{" );
        for ( k = 0; k < n; k++ )
            printf( " %d", pRes[i][k] );
        printf( " }\n" );
    }
    }
*/
    return pRes;
}

/**Function*************************************************************

  Synopsis    [Permutes the given vector of minterms.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dar_TruthPermute_int( int * pMints, int nMints, char * pPerm, int nVars, int * pMintsP )
{
    int m, v;
    // clean the storage for minterms
    memset( pMintsP, 0, sizeof(int) * nMints );
    // go through minterms and add the variables
    for ( m = 0; m < nMints; m++ )
        for ( v = 0; v < nVars; v++ )
            if ( pMints[m] & (1 << v) )
                pMintsP[m] |= (1 << pPerm[v]);
}

/**Function*************************************************************

  Synopsis    [Permutes the function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Dar_TruthPermute( unsigned Truth, char * pPerms, int nVars, int fReverse )
{
    unsigned Result;
    int * pMints;
    int * pMintsP;
    int nMints;
    int i, m;

    assert( nVars < 6 );
    nMints  = (1 << nVars);
    pMints  = ABC_ALLOC( int, nMints );
    pMintsP = ABC_ALLOC( int, nMints );
    for ( i = 0; i < nMints; i++ )
        pMints[i] = i;

    Dar_TruthPermute_int( pMints, nMints, pPerms, nVars, pMintsP );

    Result = 0;
    if ( fReverse )
    {
        for ( m = 0; m < nMints; m++ )
            if ( Truth & (1 << pMintsP[m]) )
                Result |= (1 << m);
    }
    else
    {
        for ( m = 0; m < nMints; m++ )
            if ( Truth & (1 << m) )
                Result |= (1 << pMintsP[m]);
    }

    ABC_FREE( pMints );
    ABC_FREE( pMintsP );

    return Result;
}

/**Function*************************************************************

  Synopsis    [Changes the phase of the function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Dar_TruthPolarize( unsigned uTruth, int Polarity, int nVars )
{
    // elementary truth tables
    static unsigned Signs[5] = {
        0xAAAAAAAA,    // 1010 1010 1010 1010 1010 1010 1010 1010
        0xCCCCCCCC,    // 1010 1010 1010 1010 1010 1010 1010 1010
        0xF0F0F0F0,    // 1111 0000 1111 0000 1111 0000 1111 0000
        0xFF00FF00,    // 1111 1111 0000 0000 1111 1111 0000 0000
        0xFFFF0000     // 1111 1111 1111 1111 0000 0000 0000 0000
    };
    unsigned uTruthRes, uCof0, uCof1;
    int nMints, Shift, v;
    assert( nVars < 6 );
    nMints = (1 << nVars);
    uTruthRes = uTruth;
    for ( v = 0; v < nVars; v++ )
        if ( Polarity & (1 << v) )
        {
            uCof0  = uTruth & ~Signs[v];
            uCof1  = uTruth &  Signs[v];
            Shift  = (1 << v);
            uCof0 <<= Shift;
            uCof1 >>= Shift;
            uTruth = uCof0 | uCof1;
        }
    return uTruth;
}

/**Function*************************************************************

  Synopsis    [Computes NPN canonical forms for 4-variable functions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dar_Truth4VarNPN( unsigned short ** puCanons, char ** puPhases, char ** puPerms, unsigned char ** puMap )
{
    unsigned short * uCanons;
    unsigned char * uMap;
    unsigned uTruth, uPhase, uPerm;
    char ** pPerms4, * uPhases, * uPerms;
    int nFuncs, nClasses;
    int i, k;

    nFuncs  = (1 << 16);
    uCanons = ABC_CALLOC( unsigned short, nFuncs );
    uPhases = ABC_CALLOC( char, nFuncs );
    uPerms  = ABC_CALLOC( char, nFuncs );
    uMap    = ABC_CALLOC( unsigned char, nFuncs );
    pPerms4 = Dar_Permutations( 4 );

    nClasses = 1;
    nFuncs = (1 << 15);
    for ( uTruth = 1; uTruth < (unsigned)nFuncs; uTruth++ )
    {
        // skip already assigned
        if ( uCanons[uTruth] )
        {
            assert( uTruth > uCanons[uTruth] );
            uMap[~uTruth & 0xFFFF] = uMap[uTruth] = uMap[uCanons[uTruth]];
            continue;
        }
        uMap[uTruth] = nClasses++;
        for ( i = 0; i < 16; i++ )
        {
            uPhase = Dar_TruthPolarize( uTruth, i, 4 );
            for ( k = 0; k < 24; k++ )
            {
                uPerm = Dar_TruthPermute( uPhase, pPerms4[k], 4, 0 );
                if ( uCanons[uPerm] == 0 )
                {
                    uCanons[uPerm] = uTruth;
                    uPhases[uPerm] = i;
                    uPerms[uPerm]  = k;
                    uMap[uPerm]    = uMap[uTruth];

                    uPerm = ~uPerm & 0xFFFF;
                    uCanons[uPerm] = uTruth;
                    uPhases[uPerm] = i | 16;
                    uPerms[uPerm]  = k;
                    uMap[uPerm]    = uMap[uTruth];
                }
                else
                    assert( uCanons[uPerm] == uTruth );
            }
            uPhase = Dar_TruthPolarize( ~uTruth & 0xFFFF, i, 4 ); 
            for ( k = 0; k < 24; k++ )
            {
                uPerm = Dar_TruthPermute( uPhase, pPerms4[k], 4, 0 );
                if ( uCanons[uPerm] == 0 )
                {
                    uCanons[uPerm] = uTruth;
                    uPhases[uPerm] = i;
                    uPerms[uPerm]  = k;
                    uMap[uPerm]    = uMap[uTruth];

                    uPerm = ~uPerm & 0xFFFF;
                    uCanons[uPerm] = uTruth;
                    uPhases[uPerm] = i | 16;
                    uPerms[uPerm]  = k;
                    uMap[uPerm]    = uMap[uTruth];
                }
                else
                    assert( uCanons[uPerm] == uTruth );
            }
        }
    }
    for ( uTruth = 1; uTruth < 0xffff; uTruth++ )
        assert( uMap[uTruth] != 0 );
    uPhases[(1<<16)-1] = 16;
    assert( nClasses == 222 );
    ABC_FREE( pPerms4 );
    if ( puCanons ) 
        *puCanons = uCanons;
    else
        ABC_FREE( uCanons );
    if ( puPhases ) 
        *puPhases = uPhases;
    else
        ABC_FREE( uPhases );
    if ( puPerms ) 
        *puPerms = uPerms;
    else
        ABC_FREE( uPerms );
    if ( puMap ) 
        *puMap = uMap;
    else
        ABC_FREE( uMap );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

