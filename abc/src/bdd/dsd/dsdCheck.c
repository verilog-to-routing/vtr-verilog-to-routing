/**CFile****************************************************************

  FileName    [dsdCheck.c]

  PackageName [DSD: Disjoint-support decomposition package.]

  Synopsis    [Procedures to check the identity of root functions.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 8.0. Started - September 22, 2003.]

  Revision    [$Id: dsdCheck.c,v 1.0 2002/22/09 00:00:00 alanmi Exp $]

***********************************************************************/

#include "dsdInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Dsd_Cache_t_  Dds_Cache_t;
typedef struct Dsd_Entry_t_  Dsd_Entry_t;

struct Dsd_Cache_t_
{
    Dsd_Entry_t *     pTable;
    int               nTableSize;
    int               nSuccess;
    int               nFailure;
};

struct Dsd_Entry_t_
{
    DdNode * bX[5];
};

static Dds_Cache_t * pCache;

static int Dsd_CheckRootFunctionIdentity_rec( DdManager * dd, DdNode * bF1, DdNode * bF2, DdNode * bC1, DdNode * bC2 );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function********************************************************************

  Synopsis    [(Re)allocates the local cache.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Dsd_CheckCacheAllocate( int nEntries )
{
    int nRequested;

    pCache = ABC_ALLOC( Dds_Cache_t, 1 );
    memset( pCache, 0, sizeof(Dds_Cache_t) );

    // check what is the size of the current cache
    nRequested = Abc_PrimeCudd( nEntries );
    if ( pCache->nTableSize != nRequested )
    { // the current size is different
        // deallocate the old, allocate the new
        if ( pCache->nTableSize )
            Dsd_CheckCacheDeallocate();
        // allocate memory for the hash table
        pCache->nTableSize = nRequested;
        pCache->pTable = ABC_ALLOC( Dsd_Entry_t, nRequested );
    }
    // otherwise, there is no need to allocate, just clean
    Dsd_CheckCacheClear();
//  printf( "\nThe number of allocated cache entries = %d.\n\n", pCache->nTableSize );
}

/**Function********************************************************************

  Synopsis    [Deallocates the local cache.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Dsd_CheckCacheDeallocate()
{
    ABC_FREE( pCache->pTable );
    ABC_FREE( pCache );
}

/**Function********************************************************************

  Synopsis    [Clears the local cache.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Dsd_CheckCacheClear()
{
    int i;
    for ( i = 0; i < pCache->nTableSize; i++ )
        pCache->pTable[0].bX[0] = NULL;
}


/**Function********************************************************************

  Synopsis    [Checks whether it is true that bF1(bC1=0) == bF2(bC2=0).]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Dsd_CheckRootFunctionIdentity( DdManager * dd, DdNode * bF1, DdNode * bF2, DdNode * bC1, DdNode * bC2 )
{
    int RetValue;
//  pCache->nSuccess = 0;
//  pCache->nFailure = 0;
    RetValue = Dsd_CheckRootFunctionIdentity_rec(dd, bF1, bF2, bC1, bC2);
//  printf( "Cache success = %d. Cache failure = %d.\n", pCache->nSuccess, pCache->nFailure );
    return RetValue;
}

/**Function********************************************************************

  Synopsis    [Performs the recursive step of Dsd_CheckRootFunctionIdentity().]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Dsd_CheckRootFunctionIdentity_rec( DdManager * dd, DdNode * bF1, DdNode * bF2, DdNode * bC1, DdNode * bC2 )
{
    unsigned HKey;

    // if either bC1 or bC2 is zero, the test is true
//  if ( bC1 == b0 || bC2 == b0 )  return 1;
    assert( bC1 != b0 );
    assert( bC2 != b0 );

    // if both bC1 and bC2 are one - perform comparison
    if ( bC1 == b1 && bC2 == b1 )  return (int)( bF1 == bF2 );

    if ( bF1 == b0 )
        return Cudd_bddLeq( dd, bC2, Cudd_Not(bF2) );

    if ( bF1 == b1 )
        return Cudd_bddLeq( dd, bC2, bF2 );

    if ( bF2 == b0 )
        return Cudd_bddLeq( dd, bC1, Cudd_Not(bF1) );

    if ( bF2 == b1 )
        return Cudd_bddLeq( dd, bC1, bF1 );

    // otherwise, keep expanding

    // check cache
//  HKey = _Hash( ((unsigned)bF1), ((unsigned)bF2), ((unsigned)bC1), ((unsigned)bC2) );
    HKey = hashKey4( bF1, bF2, bC1, bC2, pCache->nTableSize );
    if ( pCache->pTable[HKey].bX[0] == bF1 && 
         pCache->pTable[HKey].bX[1] == bF2 && 
         pCache->pTable[HKey].bX[2] == bC1 && 
         pCache->pTable[HKey].bX[3] == bC2 )
    {
        pCache->nSuccess++;
        return (int)(ABC_PTRUINT_T)pCache->pTable[HKey].bX[4]; // the last bit records the result (yes/no)
    }
    else
    {

        // determine the top variables
        int RetValue;
        DdNode * bA[4]  = { bF1, bF2, bC1, bC2 }; // arguments
        DdNode * bAR[4] = { Cudd_Regular(bF1), Cudd_Regular(bF2), Cudd_Regular(bC1), Cudd_Regular(bC2) }; // regular arguments
        int CurLevel[4] = { cuddI(dd,bAR[0]->index), cuddI(dd,bAR[1]->index), cuddI(dd,bAR[2]->index), cuddI(dd,bAR[3]->index) };
        int TopLevel = CUDD_CONST_INDEX;
        int i;
        DdNode * bE[4], * bT[4];
        DdNode * bF1next, * bF2next, * bC1next, * bC2next;

        pCache->nFailure++;

        // determine the top level
        for ( i = 0; i < 4; i++ )
            if ( TopLevel > CurLevel[i] )
                 TopLevel = CurLevel[i];

        // compute the cofactors
        for ( i = 0; i < 4; i++ )
        if ( TopLevel == CurLevel[i] )
        {
            if ( bA[i] != bAR[i] ) // complemented
            {
                bE[i] = Cudd_Not(cuddE(bAR[i]));
                bT[i] = Cudd_Not(cuddT(bAR[i]));
            }
            else
            {
                bE[i] = cuddE(bAR[i]);
                bT[i] = cuddT(bAR[i]);
            }
        }
        else
            bE[i] = bT[i] = bA[i];

        // solve subproblems
        // three cases are possible

        // (1) the top var belongs to both C1 and C2
        //     in this case, any cofactor of F1 and F2 will do, 
        //     as long as the corresponding cofactor of C1 and C2 is not equal to 0
        if ( TopLevel == CurLevel[2] && TopLevel == CurLevel[3] ) 
        {
            if ( bE[2] != b0 ) // C1
            {
                bF1next = bE[0];
                bC1next = bE[2];
            }
            else
            {
                bF1next = bT[0];
                bC1next = bT[2];
            }
            if ( bE[3] != b0 ) // C2
            {
                bF2next = bE[1];
                bC2next = bE[3];
            }
            else
            {
                bF2next = bT[1];
                bC2next = bT[3];
            }
            RetValue = Dsd_CheckRootFunctionIdentity_rec( dd, bF1next, bF2next, bC1next, bC2next );
        }
        // (2) the top var belongs to either C1 or C2
        //     in this case normal splitting of cofactors
        else if ( TopLevel == CurLevel[2] && TopLevel != CurLevel[3] ) 
        {
            if ( bE[2] != b0 ) // C1
            {
                bF1next = bE[0];
                bC1next = bE[2];
            }
            else
            {
                bF1next = bT[0];
                bC1next = bT[2];
            }
            // split around this variable
            RetValue = Dsd_CheckRootFunctionIdentity_rec( dd, bF1next, bE[1], bC1next, bE[3] );
            if ( RetValue == 1 ) // test another branch; otherwise, there is no need to test
                RetValue = Dsd_CheckRootFunctionIdentity_rec( dd, bF1next, bT[1], bC1next, bT[3] );
        }
        else if ( TopLevel != CurLevel[2] && TopLevel == CurLevel[3] ) 
        {
            if ( bE[3] != b0 ) // C2
            {
                bF2next = bE[1];
                bC2next = bE[3];
            }
            else
            {
                bF2next = bT[1];
                bC2next = bT[3];
            }
            // split around this variable
            RetValue = Dsd_CheckRootFunctionIdentity_rec( dd, bE[0], bF2next, bE[2], bC2next );
            if ( RetValue == 1 ) // test another branch; otherwise, there is no need to test
                RetValue = Dsd_CheckRootFunctionIdentity_rec( dd, bT[0], bF2next, bT[2], bC2next );
        }
        // (3) the top var does not belong to C1 and C2
        //     in this case normal splitting of cofactors
        else // if ( TopLevel != CurLevel[2] && TopLevel != CurLevel[3] )
        {
            // split around this variable
            RetValue = Dsd_CheckRootFunctionIdentity_rec( dd, bE[0], bE[1], bE[2], bE[3] );
            if ( RetValue == 1 ) // test another branch; otherwise, there is no need to test
                RetValue = Dsd_CheckRootFunctionIdentity_rec( dd, bT[0], bT[1], bT[2], bT[3] );
        }

        // set cache
        for ( i = 0; i < 4; i++ )
            pCache->pTable[HKey].bX[i] = bA[i];
        pCache->pTable[HKey].bX[4] = (DdNode*)(ABC_PTRUINT_T)RetValue;

        return RetValue;
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END

