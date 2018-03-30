/**CFile****************************************************************

  FileName    [utilSort.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Merge sort with user-specified cost.]

  Synopsis    [Merge sort with user-specified cost.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - Feburary 13, 2011.]

  Revision    [$Id: utilSort.c,v 1.00 2011/02/11 00:00:00 alanmi Exp $]

***********************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "abc_global.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Merging two lists of entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_SortMerge( int * p1Beg, int * p1End, int * p2Beg, int * p2End, int * pOut )
{
    int nEntries = (p1End - p1Beg) + (p2End - p2Beg);
    int * pOutBeg = pOut;
    while ( p1Beg < p1End && p2Beg < p2End )
    {
        if ( *p1Beg == *p2Beg )
            *pOut++ = *p1Beg++, *pOut++ = *p2Beg++; 
        else if ( *p1Beg < *p2Beg )
            *pOut++ = *p1Beg++; 
        else // if ( *p1Beg > *p2Beg )
            *pOut++ = *p2Beg++; 
    }
    while ( p1Beg < p1End )
        *pOut++ = *p1Beg++; 
    while ( p2Beg < p2End )
        *pOut++ = *p2Beg++;
    assert( pOut - pOutBeg == nEntries );
}

/**Function*************************************************************

  Synopsis    [Recursive sorting.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_Sort_rec( int * pInBeg, int * pInEnd, int * pOutBeg )
{
    int nSize = pInEnd - pInBeg;
    assert( nSize > 0 );
    if ( nSize == 1 )
        return;
    if ( nSize == 2 )
    {
         if ( pInBeg[0] > pInBeg[1] )
         {
             pInBeg[0] ^= pInBeg[1];
             pInBeg[1] ^= pInBeg[0];
             pInBeg[0] ^= pInBeg[1];
         }
    }
    else if ( nSize < 8 )
    {
        int temp, i, j, best_i;
        for ( i = 0; i < nSize-1; i++ )
        {
            best_i = i;
            for ( j = i+1; j < nSize; j++ )
                if ( pInBeg[j] < pInBeg[best_i] )
                    best_i = j;
            temp = pInBeg[i]; 
            pInBeg[i] = pInBeg[best_i]; 
            pInBeg[best_i] = temp;
        }
    }
    else
    {
        Abc_Sort_rec( pInBeg, pInBeg + nSize/2, pOutBeg );
        Abc_Sort_rec( pInBeg + nSize/2, pInEnd, pOutBeg + nSize/2 );
        Abc_SortMerge( pInBeg, pInBeg + nSize/2, pInBeg + nSize/2, pInEnd, pOutBeg );
        memcpy( pInBeg, pOutBeg, sizeof(int) * nSize );
    }
}

/**Function*************************************************************

  Synopsis    [Returns the sorted array of integers.]

  Description [This procedure is about 10% faster than qsort().]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_MergeSort( int * pInput, int nSize )
{
    int * pOutput;
    if ( nSize < 2 )
        return;
    pOutput = (int *) malloc( sizeof(int) * nSize );
    Abc_Sort_rec( pInput, pInput + nSize, pOutput );
    free( pOutput );
}



/**Function*************************************************************

  Synopsis    [Merging two lists of entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_MergeSortCostMerge( int * p1Beg, int * p1End, int * p2Beg, int * p2End, int * pOut )
{
    int nEntries = (p1End - p1Beg) + (p2End - p2Beg);
    int * pOutBeg = pOut;
    while ( p1Beg < p1End && p2Beg < p2End )
    {
        if ( p1Beg[1] == p2Beg[1] )
            *pOut++ = *p1Beg++, *pOut++ = *p1Beg++, *pOut++ = *p2Beg++, *pOut++ = *p2Beg++; 
        else if ( p1Beg[1] < p2Beg[1] )
            *pOut++ = *p1Beg++, *pOut++ = *p1Beg++; 
        else // if ( p1Beg[1] > p2Beg[1] )
            *pOut++ = *p2Beg++, *pOut++ = *p2Beg++; 
    }
    while ( p1Beg < p1End )
        *pOut++ = *p1Beg++, *pOut++ = *p1Beg++; 
    while ( p2Beg < p2End )
        *pOut++ = *p2Beg++, *pOut++ = *p2Beg++;
    assert( pOut - pOutBeg == nEntries );
}

/**Function*************************************************************

  Synopsis    [Recursive sorting.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_MergeSortCost_rec( int * pInBeg, int * pInEnd, int * pOutBeg )
{
    int nSize = (pInEnd - pInBeg)/2;
    assert( nSize > 0 );
    if ( nSize == 1 )
        return;
    if ( nSize == 2 )
    {
         if ( pInBeg[1] > pInBeg[3] )
         {
             pInBeg[1] ^= pInBeg[3];
             pInBeg[3] ^= pInBeg[1];
             pInBeg[1] ^= pInBeg[3];
             pInBeg[0] ^= pInBeg[2];
             pInBeg[2] ^= pInBeg[0];
             pInBeg[0] ^= pInBeg[2];
         }
    }
    else if ( nSize < 8 )
    {
        int temp, i, j, best_i;
        for ( i = 0; i < nSize-1; i++ )
        {
            best_i = i;
            for ( j = i+1; j < nSize; j++ )
                if ( pInBeg[2*j+1] < pInBeg[2*best_i+1] )
                    best_i = j;
            temp = pInBeg[2*i]; 
            pInBeg[2*i] = pInBeg[2*best_i]; 
            pInBeg[2*best_i] = temp;
            temp = pInBeg[2*i+1]; 
            pInBeg[2*i+1] = pInBeg[2*best_i+1]; 
            pInBeg[2*best_i+1] = temp;
        }
    }
    else
    {
        Abc_MergeSortCost_rec( pInBeg, pInBeg + 2*(nSize/2), pOutBeg );
        Abc_MergeSortCost_rec( pInBeg + 2*(nSize/2), pInEnd, pOutBeg + 2*(nSize/2) );
        Abc_MergeSortCostMerge( pInBeg, pInBeg + 2*(nSize/2), pInBeg + 2*(nSize/2), pInEnd, pOutBeg );
        memcpy( pInBeg, pOutBeg, sizeof(int) * 2 * nSize );
    }
}

/**Function*************************************************************

  Synopsis    [Sorting procedure.]

  Description [Returns permutation for the non-decreasing order of costs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Abc_MergeSortCost( int * pCosts, int nSize )
{
    int i, * pResult, * pInput, * pOutput;
    pResult = (int *) calloc( sizeof(int), nSize );
    if ( nSize < 2 )
        return pResult;
    pInput  = (int *) malloc( sizeof(int) * 2 * nSize );
    pOutput = (int *) malloc( sizeof(int) * 2 * nSize );
    for ( i = 0; i < nSize; i++ )
        pInput[2*i] = i, pInput[2*i+1] = pCosts[i];
    Abc_MergeSortCost_rec( pInput, pInput + 2*nSize, pOutput );
    for ( i = 0; i < nSize; i++ )
        pResult[i] = pInput[2*i];
    free( pOutput );
    free( pInput );
    return pResult;
}


// this implementation uses 3x less memory but is 30% slower due to cache misses

#if 0  

/**Function*************************************************************

  Synopsis    [Merging two lists of entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_MergeSortCostMerge( int * p1Beg, int * p1End, int * p2Beg, int * p2End, int * pOut, int * pCost )
{
    int nEntries = (p1End - p1Beg) + (p2End - p2Beg);
    int * pOutBeg = pOut;
    while ( p1Beg < p1End && p2Beg < p2End )
    {
        if ( pCost[*p1Beg] == pCost[*p2Beg] )
            *pOut++ = *p1Beg++, *pOut++ = *p2Beg++; 
        else if ( pCost[*p1Beg] < pCost[*p2Beg] )
            *pOut++ = *p1Beg++; 
        else // if ( pCost[*p1Beg] > pCost[*p2Beg] )
            *pOut++ = *p2Beg++; 
    }
    while ( p1Beg < p1End )
        *pOut++ = *p1Beg++; 
    while ( p2Beg < p2End )
        *pOut++ = *p2Beg++;
    assert( pOut - pOutBeg == nEntries );
}

/**Function*************************************************************

  Synopsis    [Recursive sorting.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_MergeSortCost_rec( int * pInBeg, int * pInEnd, int * pOutBeg, int * pCost )
{
    int nSize = pInEnd - pInBeg;
    assert( nSize > 0 );
    if ( nSize == 1 )
        return;
    if ( nSize == 2 )
    {
         if ( pCost[pInBeg[0]] > pCost[pInBeg[1]] )
         {
             pInBeg[0] ^= pInBeg[1];
             pInBeg[1] ^= pInBeg[0];
             pInBeg[0] ^= pInBeg[1];
         }
    }
    else if ( nSize < 8 )
    {
        int temp, i, j, best_i;
        for ( i = 0; i < nSize-1; i++ )
        {
            best_i = i;
            for ( j = i+1; j < nSize; j++ )
                if ( pCost[pInBeg[j]] < pCost[pInBeg[best_i]] )
                    best_i = j;
            temp = pInBeg[i]; 
            pInBeg[i] = pInBeg[best_i]; 
            pInBeg[best_i] = temp;
        }
    }
    else
    {
        Abc_MergeSortCost_rec( pInBeg, pInBeg + nSize/2, pOutBeg, pCost );
        Abc_MergeSortCost_rec( pInBeg + nSize/2, pInEnd, pOutBeg + nSize/2, pCost );
        Abc_MergeSortCostMerge( pInBeg, pInBeg + nSize/2, pInBeg + nSize/2, pInEnd, pOutBeg, pCost );
        memcpy( pInBeg, pOutBeg, sizeof(int) * nSize );
    }
}

/**Function*************************************************************

  Synopsis    [Sorting procedure.]

  Description [Returns permutation for the non-decreasing order of costs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Abc_MergeSortCost( int * pCosts, int nSize )
{
    int i, * pInput, * pOutput;
    pInput = (int *) malloc( sizeof(int) * nSize );
    for ( i = 0; i < nSize; i++ )
        pInput[i] = i;
    if ( nSize < 2 )
        return pInput;
    pOutput = (int *) malloc( sizeof(int) * nSize );
    Abc_MergeSortCost_rec( pInput, pInput + nSize, pOutput, pCosts );
    free( pOutput );
    return pInput;
}

#endif




/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_SortNumCompare( int * pNum1, int * pNum2 )
{
    return *pNum1 - *pNum2;
}

/**Function*************************************************************

  Synopsis    [Testing the sorting procedure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_SortTest()
{
    int fUseNew = 0;
    int i, nSize = 50000000;
    int * pArray = (int *)malloc( sizeof(int) * nSize );
    int * pPerm;
    abctime clk;
    // generate numbers
    srand( 1000 );
    for ( i = 0; i < nSize; i++ )
        pArray[i] = rand();

    // try sorting
    if ( fUseNew )
    {
        int fUseCost = 1;
        if ( fUseCost )
        {
            clk = Abc_Clock();
            pPerm = Abc_MergeSortCost( pArray, nSize );
            Abc_PrintTime( 1, "New sort", Abc_Clock() - clk );
            // check
            for ( i = 1; i < nSize; i++ )
                assert( pArray[pPerm[i-1]] <= pArray[pPerm[i]] );
            free( pPerm );
        }
        else
        {
            clk = Abc_Clock();
            Abc_MergeSort( pArray, nSize );
            Abc_PrintTime( 1, "New sort", Abc_Clock() - clk );
            // check
            for ( i = 1; i < nSize; i++ )
                assert( pArray[i-1] <= pArray[i] );
        }
    }
    else
    {
        clk = Abc_Clock();
        qsort( (void *)pArray, nSize, sizeof(int), (int (*)(const void *, const void *)) Abc_SortNumCompare );
        Abc_PrintTime( 1, "Old sort", Abc_Clock() - clk );
        // check
        for ( i = 1; i < nSize; i++ )
            assert( pArray[i-1] <= pArray[i] );
    }

    free( pArray );
}




/**Function*************************************************************

  Synopsis    [QuickSort algorithm as implemented by qsort().]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_QuickSort1CompareInc( word * p1, word * p2 )
{
    if ( (unsigned)(*p1) < (unsigned)(*p2) )
        return -1;
    if ( (unsigned)(*p1) > (unsigned)(*p2) )
        return 1;
    return 0;
}
int Abc_QuickSort1CompareDec( word * p1, word * p2 )
{
    if ( (unsigned)(*p1) > (unsigned)(*p2) )
        return -1;
    if ( (unsigned)(*p1) < (unsigned)(*p2) )
        return 1;
    return 0;
}
void Abc_QuickSort1( word * pData, int nSize, int fDecrease )
{
    int i, fVerify = 0;
    if ( fDecrease )
    {
        qsort( (void *)pData, nSize, sizeof(word), (int (*)(const void *, const void *))Abc_QuickSort1CompareDec );
        if ( fVerify )
            for ( i = 1; i < nSize; i++ )
                assert( (unsigned)pData[i-1] >= (unsigned)pData[i] );
    }
    else
    {
        qsort( (void *)pData, nSize, sizeof(word), (int (*)(const void *, const void *))Abc_QuickSort1CompareInc );
        if ( fVerify )
            for ( i = 1; i < nSize; i++ )
                assert( (unsigned)pData[i-1] <= (unsigned)pData[i] );
    }
}

/**Function*************************************************************

  Synopsis    [QuickSort algorithm based on 2/3-way partitioning.]

  Description [This code is based on the online presentation 
  "QuickSort is Optimal" by Robert Sedgewick and Jon Bentley.
  http://www.sorting-algorithms.com/static/QuicksortIsOptimal.pdf 

  The first 32-bits of the input data contain values to be compared. 
  The last 32-bits contain the user's data. When sorting is finished, 
  the 64-bit words are ordered in the increasing order of their value ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Abc_SelectSortInc( word * pData, int nSize )
{
    int i, j, best_i;
    for ( i = 0; i < nSize-1; i++ )
    {
        best_i = i;
        for ( j = i+1; j < nSize; j++ )
            if ( (unsigned)pData[j] < (unsigned)pData[best_i] )
                best_i = j;
        ABC_SWAP( word, pData[i], pData[best_i] );
    }
}
static inline void Abc_SelectSortDec( word * pData, int nSize )
{
    int i, j, best_i;
    for ( i = 0; i < nSize-1; i++ )
    {
        best_i = i;
        for ( j = i+1; j < nSize; j++ )
            if ( (unsigned)pData[j] > (unsigned)pData[best_i] )
                best_i = j;
        ABC_SWAP( word, pData[i], pData[best_i] );
    }
}

void Abc_QuickSort2Inc_rec( word * pData, int l, int r )
{
    word v = pData[r];
    int i = l-1, j = r;
    if ( l >= r )
        return;
    assert( l < r );
    if ( r - l < 10 )
    {
        Abc_SelectSortInc( pData + l, r - l + 1 );
        return;
    }
    while ( 1 )
    {
        while ( (unsigned)pData[++i] < (unsigned)v );
        while ( (unsigned)v < (unsigned)pData[--j] )
            if ( j == l )
                break;
        if ( i >= j )
            break;
        ABC_SWAP( word, pData[i], pData[j] );
    }
    ABC_SWAP( word, pData[i], pData[r] ); 
    Abc_QuickSort2Inc_rec( pData, l, i-1 );
    Abc_QuickSort2Inc_rec( pData, i+1, r );
}
void Abc_QuickSort2Dec_rec( word * pData, int l, int r )
{
    word v = pData[r];
    int i = l-1, j = r;
    if ( l >= r )
        return;
    assert( l < r );
    if ( r - l < 10 )
    {
        Abc_SelectSortDec( pData + l, r - l + 1 );
        return;
    }
    while ( 1 )
    {
        while ( (unsigned)pData[++i] > (unsigned)v );
        while ( (unsigned)v > (unsigned)pData[--j] )
            if ( j == l )
                break;
        if ( i >= j )
            break;
        ABC_SWAP( word, pData[i], pData[j] );
    }
    ABC_SWAP( word, pData[i], pData[r] ); 
    Abc_QuickSort2Dec_rec( pData, l, i-1 );
    Abc_QuickSort2Dec_rec( pData, i+1, r );
}

void Abc_QuickSort3Inc_rec( word * pData, int l, int r )
{
    word v = pData[r];
    int k, i = l-1, j = r, p = l-1, q = r;
    if ( l >= r )
        return;
    assert( l < r );
    if ( r - l < 10 )
    {
        Abc_SelectSortInc( pData + l, r - l + 1 );
        return;
    }
    while ( 1 )
    {
        while ( (unsigned)pData[++i] < (unsigned)v );
        while ( (unsigned)v < (unsigned)pData[--j] )
            if ( j == l )
                break;
        if ( i >= j )
            break;
        ABC_SWAP( word, pData[i], pData[j] );
        if ( (unsigned)pData[i] == (unsigned)v ) 
            { p++; ABC_SWAP( word, pData[p], pData[i] ); }
        if ( (unsigned)v == (unsigned)pData[j] ) 
            { q--; ABC_SWAP( word, pData[j], pData[q] ); }
    }
    ABC_SWAP( word, pData[i], pData[r] ); 
    j = i-1; i = i+1;
    for ( k = l; k < p; k++, j-- ) 
        ABC_SWAP( word, pData[k], pData[j] );
    for ( k = r-1; k > q; k--, i++ ) 
        ABC_SWAP( word, pData[i], pData[k] );
    Abc_QuickSort3Inc_rec( pData, l, j );
    Abc_QuickSort3Inc_rec( pData, i, r );
}
void Abc_QuickSort3Dec_rec( word * pData, int l, int r )
{
    word v = pData[r];
    int k, i = l-1, j = r, p = l-1, q = r;
    if ( l >= r )
        return;
    assert( l < r );
    if ( r - l < 10 )
    {
        Abc_SelectSortDec( pData + l, r - l + 1 );
        return;
    }
    while ( 1 )
    {
        while ( (unsigned)pData[++i] > (unsigned)v );
        while ( (unsigned)v > (unsigned)pData[--j] )
            if ( j == l )
                break;
        if ( i >= j )
            break;
        ABC_SWAP( word, pData[i], pData[j] );
        if ( (unsigned)pData[i] == (unsigned)v ) 
            { p++; ABC_SWAP( word, pData[p], pData[i] ); }
        if ( (unsigned)v == (unsigned)pData[j] ) 
            { q--; ABC_SWAP( word, pData[j], pData[q] ); }
    }
    ABC_SWAP( word, pData[i], pData[r] ); 
    j = i-1; i = i+1;
    for ( k = l; k < p; k++, j-- ) 
        ABC_SWAP( word, pData[k], pData[j] );
    for ( k = r-1; k > q; k--, i++ ) 
        ABC_SWAP( word, pData[i], pData[k] );
    Abc_QuickSort3Dec_rec( pData, l, j );
    Abc_QuickSort3Dec_rec( pData, i, r );
}

void Abc_QuickSort2( word * pData, int nSize, int fDecrease )
{
    int i, fVerify = 0;
    if ( fDecrease )
    {
        Abc_QuickSort2Dec_rec( pData, 0, nSize - 1 );
        if ( fVerify )
            for ( i = 1; i < nSize; i++ )
                assert( (unsigned)pData[i-1] >= (unsigned)pData[i] );
    }
    else
    {
        Abc_QuickSort2Inc_rec( pData, 0, nSize - 1 );
        if ( fVerify )
            for ( i = 1; i < nSize; i++ )
                assert( (unsigned)pData[i-1] <= (unsigned)pData[i] );
    }
}
void Abc_QuickSort3( word * pData, int nSize, int fDecrease )
{
    int i, fVerify = 1;
    if ( fDecrease )
    {
        Abc_QuickSort2Dec_rec( pData, 0, nSize - 1 );
        if ( fVerify )
            for ( i = 1; i < nSize; i++ )
                assert( (unsigned)pData[i-1] >= (unsigned)pData[i] );
    }
    else
    {
        Abc_QuickSort2Inc_rec( pData, 0, nSize - 1 );
        if ( fVerify )
            for ( i = 1; i < nSize; i++ )
                assert( (unsigned)pData[i-1] <= (unsigned)pData[i] );
    }
}

/**Function*************************************************************

  Synopsis    [Wrapper around QuickSort to sort entries based on cost.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_QuickSortCostData( int * pCosts, int nSize, int fDecrease, word * pData, int * pResult )
{
    int i;
    for ( i = 0; i < nSize; i++ )
        pData[i] = ((word)i << 32) | pCosts[i];
    Abc_QuickSort3( pData, nSize, fDecrease );
    for ( i = 0; i < nSize; i++ )
        pResult[i] = (int)(pData[i] >> 32);
}
int * Abc_QuickSortCost( int * pCosts, int nSize, int fDecrease )
{
    word * pData = ABC_ALLOC( word, nSize );
    int * pResult = ABC_ALLOC( int, nSize );
    Abc_QuickSortCostData( pCosts, nSize, fDecrease, pData, pResult );
    ABC_FREE( pData );
    return pResult;
}

//        extern void Abc_QuickSortTest();
//        Abc_QuickSortTest();

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_QuickSortTest()
{
    int nSize = 1000000;
    int fVerbose = 0;
    word * pData1, * pData2;
    int i;
    abctime clk = Abc_Clock();
    // generate numbers
    pData1 = ABC_ALLOC( word, nSize );
    pData2 = ABC_ALLOC( word, nSize );
    srand( 1111 );
    for ( i = 0; i < nSize; i++ )
        pData2[i] = pData1[i] = ((word)i << 32) | rand();
    Abc_PrintTime( 1, "Prepare ", Abc_Clock() - clk );
    // perform sorting
    clk = Abc_Clock();
    Abc_QuickSort3( pData1, nSize, 1 );
    Abc_PrintTime( 1, "Sort new", Abc_Clock() - clk );
    // print the result
    if ( fVerbose )
    {
        for ( i = 0; i < nSize; i++ )
            printf( "(%d,%d) ", (int)(pData1[i] >> 32), (int)pData1[i] );
        printf( "\n" );
    }
    // create new numbers
    clk = Abc_Clock();
    Abc_QuickSort1( pData2, nSize, 1 );
    Abc_PrintTime( 1, "Sort old", Abc_Clock() - clk );
    // print the result
    if ( fVerbose )
    {
        for ( i = 0; i < nSize; i++ )
            printf( "(%d,%d) ", (int)(pData2[i] >> 32), (int)pData2[i] );
        printf( "\n" );
    }
    ABC_FREE( pData1 );
    ABC_FREE( pData2 );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

