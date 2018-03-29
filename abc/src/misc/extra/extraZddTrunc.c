/**CFile****************************************************************

  FileName    [extraZddTrunc.c]

  PackageName [extra]

  Synopsis    [Procedure to truncate a ZDD using variable probabilities.]

  Author      [Alan Mishchenko]

  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - September 1, 2003.]

  Revision    [$Id: extraZddTrunc.c,v 1.0 2003/05/21 18:03:50 alanmi Exp $]

***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "misc/st/st.h"
#include "bdd/cudd/cuddInt.h"

#ifdef _WIN32
#define inline __inline // compatible with MS VS 6.0
#endif

ABC_NAMESPACE_IMPL_START


#define TEST_VAR_MAX 10
#define TEST_SET_MAX 10

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/


// dynamic vector of intergers
typedef struct Vec_Int_t_       Vec_Int_t;
struct Vec_Int_t_ 
{
    int              nCap;
    int              nSize;
    int *            pArray;
};
static inline Vec_Int_t * Vec_IntAlloc( int nCap )
{
    Vec_Int_t * p;
    p = ABC_ALLOC( Vec_Int_t, 1 );
    if ( nCap > 0 && nCap < 16 )
        nCap = 16;
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ABC_ALLOC( int, p->nCap ) : NULL;
    return p;
}
static inline void Vec_IntFree( Vec_Int_t * p )
{
    ABC_FREE( p->pArray );
    ABC_FREE( p );
}
static inline int * Vec_IntReleaseArray( Vec_Int_t * p )
{
    int * pArray = p->pArray;
    p->nCap = 0;
    p->nSize = 0;
    p->pArray = NULL;
    return pArray;
}
static inline int Vec_IntAddToEntry( Vec_Int_t * p, int i, int Addition )
{
    assert( i >= 0 && i < p->nSize );
    return p->pArray[i] += Addition;
}
static inline void Vec_IntGrow( Vec_Int_t * p, int nCapMin )
{
    if ( p->nCap >= nCapMin )
        return;
    p->pArray = ABC_REALLOC( int, p->pArray, nCapMin ); 
    assert( p->pArray );
    p->nCap   = nCapMin;
}
static inline int Vec_IntPop( Vec_Int_t * p )
{
    assert( p->nSize > 0 );
    return p->pArray[--p->nSize];
}
static inline void Vec_IntPush( Vec_Int_t * p, int Entry )
{
    if ( p->nSize == p->nCap )
    {
        if ( p->nCap < 16 )
            Vec_IntGrow( p, 16 );
        else
            Vec_IntGrow( p, 2 * p->nCap );
    }
    p->pArray[p->nSize++] = Entry;
}
static inline void Vec_IntAppend( Vec_Int_t * vVec1, Vec_Int_t * vVec2 )
{
    int i;
    for ( i = 0; i < vVec2->nSize; i++ )
        Vec_IntPush( vVec1, vVec2->pArray[i] );
}


/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Compute the set of subsets whose probability is more than ProbLimit.]

  Description [The resulting array has the following form:  The first integer entry 
  is the number of resulting subsets.  The following integer entries in the array
  contain as many subsets. Each subset is an array of integers followed by -1. 
  See how subsets are printed in the included test procedure below.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Extra_zddTruncate_rec(
  DdManager * dd, 
  DdNode * zFunc,       // zFunc is the ZDD to be truncated
  double * pVarProbs,    // pVarProbs is probabilities of each variable (should have at least dd->sizeZ entries)
  double ProbLimit,      // ProbLimit is the limit on the probabilities (only those more than this will be collected)
  double ProbThis,       // current path probability
  Vec_Int_t * vSubset,  // current subset under construction
  Vec_Int_t * vResult ) // resulting subsets to be returned to the user
{
    // quit if probability of the path is less then the limit
    if ( ProbThis < ProbLimit )
        return;
    // quit if there is no subsets
    if ( zFunc == Cudd_ReadZero(dd) ) 
        return;
    // quit and save a new subset if there is one
    if ( zFunc == Cudd_ReadOne(dd) ) 
    {
        Vec_IntAddToEntry( vResult, 0, 1 );
        Vec_IntAppend( vResult, vSubset );
        Vec_IntPush( vResult, -1 );
        return;
    }
    // call recursively for the set without the given variable
    Extra_zddTruncate_rec( dd, cuddE(zFunc), pVarProbs, ProbLimit, ProbThis, vSubset, vResult );
    // call recursively for the set with the given variable
    Vec_IntPush( vSubset, Cudd_NodeReadIndex(zFunc) );
    Extra_zddTruncate_rec( dd, cuddT(zFunc), pVarProbs, ProbLimit, ProbThis * pVarProbs[Cudd_NodeReadIndex(zFunc)], vSubset, vResult );
    Vec_IntPop( vSubset );
}
int * Extra_zddTruncate( 
  DdManager * dd, 
  DdNode * zFunc,       // zFunc is the ZDD to be truncated
  double * pVarProbs,    // pVarProbs is probabilities of each variable (should have at least dd->sizeZ entries)
  double ProbLimit )     // ProbLimit is the limit on the probabilities (only those more than this will be collected)
{
    Vec_Int_t * vSubset, * vResult;
    int i, sizeZ = Cudd_ReadZddSize(dd);
    int * pResult;
    // check that probabilities are reasonable
    assert( ProbLimit > 0 && ProbLimit <= 1 );
    for ( i = 0; i < sizeZ; i++ )
        assert( pVarProbs[i] > 0 && pVarProbs[i] <= 1 );
    // enumerate assignments satisfying the probability limit
    vSubset = Vec_IntAlloc( sizeZ );
    vResult = Vec_IntAlloc( 10 * sizeZ );
    Vec_IntPush( vResult, 0 );
    Extra_zddTruncate_rec( dd, zFunc, pVarProbs, ProbLimit, 1, vSubset, vResult );
    Vec_IntFree( vSubset );
    pResult = Vec_IntReleaseArray( vResult );
    Vec_IntFree( vResult );
    return pResult;
} // end of Extra_zddTruncate 

/**Function*************************************************************

  Synopsis    [Creates the combination composed of a single ZDD variable.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Extra_zddVariable( DdManager * dd, int iVar )
{
    DdNode * zRes;
    do {
        dd->reordered = 0;
        zRes = cuddZddGetNode( dd, iVar, Cudd_ReadOne(dd), Cudd_ReadZero(dd) ); 
    } while (dd->reordered == 1);
    return zRes;
}

/**Function********************************************************************

  Synopsis    [Creates ZDD representing a given set of subsets.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * Extra_zddCreateSubsets(
  DdManager * dd,
  int pSubsets[][TEST_VAR_MAX+1],
  int nSubsets )
{
    int i, k;
    DdNode * zOne, * zVar, * zRes, * zTemp;
    zRes = Cudd_ReadZero(dd); Cudd_Ref( zRes );
    for ( i = 0; i < nSubsets; i++ )
    {
        zOne = Cudd_ReadOne(dd); Cudd_Ref( zOne );
        for ( k = 0; pSubsets[i][k] != -1; k++ )
        {
            assert( pSubsets[i][k] < TEST_VAR_MAX );
            zVar = Extra_zddVariable( dd, pSubsets[i][k] );
            zOne = Cudd_zddUnateProduct( dd, zTemp = zOne, zVar ); Cudd_Ref( zOne );
            Cudd_RecursiveDerefZdd( dd, zTemp );
        }
        zRes = Cudd_zddUnion( dd, zRes, zOne ); Cudd_Ref( zRes );
        Cudd_RecursiveDerefZdd( dd, zOne );
    }
    Cudd_Deref( zRes );
    return zRes;
}

/**Function********************************************************************

  Synopsis    [Prints a set of subsets represented using as an array.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Extra_zddPrintSubsets( int * pSubsets )
{
    int i, k, Counter = 0;
    printf( "The set contains %d subsets:\n", pSubsets[0] );
    for ( i = k = 0; i < pSubsets[0]; i++ )
    {
        printf( "Subset %3d : {", Counter );
        for ( k++; pSubsets[k] != -1; k++ )
            printf( " %d", pSubsets[k] );
        printf( " }\n" );
        Counter++;
    }
}

/**Function********************************************************************

  Synopsis    [Testbench for the above truncation procedure.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Extra_zddTruncateTest()
{
    // input data
    int nSubsets = 5;
    int pSubsets[TEST_SET_MAX][TEST_VAR_MAX+1] = { {0, 3, 5, -1}, {1, 2, 3, 6, 9, -1}, {1, 5, 7, 8, -1}, {2, 4, -1}, {0, 5, 6, 9, -1} };
    // varible probabilities
    double pVarProbs[TEST_VAR_MAX] = { 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1 };
    double ProbLimit = 0.001;
    // output data
    int * pOutput;
    // start the manager and create ZDD representing the input subsets
    DdManager * dd = Cudd_Init( 0, TEST_VAR_MAX, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS , 0 );
    DdNode * zFunc = Extra_zddCreateSubsets( dd, pSubsets, nSubsets );  Cudd_Ref( zFunc );
    assert( nSubsets <= TEST_SET_MAX );
    // print the input ZDD
    printf( "The initial ZDD representing %d subsets:\n", nSubsets );
    Cudd_zddPrintMinterm( dd, zFunc );
    // compute the result of truncation
    pOutput = Extra_zddTruncate( dd, zFunc, pVarProbs, ProbLimit );
    // print the resulting ZDD
    printf( "The resulting ZDD representing %d subsets:\n", pOutput[0] );
    // print the resulting subsets
    Extra_zddPrintSubsets( pOutput );
    // cleanup
    ABC_FREE( pOutput );
    Cudd_RecursiveDerefZdd( dd, zFunc );
    Cudd_Quit( dd );
} 




////////////////////////////////////////////////////////////////////////
///                           END OF FILE                            ///
////////////////////////////////////////////////////////////////////////
ABC_NAMESPACE_IMPL_END

