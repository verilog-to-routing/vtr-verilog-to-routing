/**CFile****************************************************************

  FileName    [fraigFeed.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [Procedures to support the solver feedback.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - October 1, 2004]

  Revision    [$Id: fraigFeed.c,v 1.8 2005/07/08 01:01:31 alanmi Exp $]

***********************************************************************/

#include "fraigInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int   Fraig_FeedBackPrepare( Fraig_Man_t * p, int * pModel, Msat_IntVec_t * vVars );
static int   Fraig_FeedBackInsert( Fraig_Man_t * p, int nVarsPi );
static void  Fraig_FeedBackVerify( Fraig_Man_t * p, Fraig_Node_t * pOld, Fraig_Node_t * pNew );

static void  Fraig_FeedBackCovering( Fraig_Man_t * p, Msat_IntVec_t * vPats );
static       Fraig_NodeVec_t * Fraig_FeedBackCoveringStart( Fraig_Man_t * pMan );
static int   Fraig_GetSmallestColumn( int * pHits, int nHits );
static int   Fraig_GetHittingPattern( unsigned * pSims, int nWords );
static void  Fraig_CancelCoveredColumns( Fraig_NodeVec_t * vColumns, int * pHits, int iPat );
static void  Fraig_FeedBackCheckTable( Fraig_Man_t * p );
static void  Fraig_FeedBackCheckTableF0( Fraig_Man_t * p );
static void  Fraig_ReallocateSimulationInfo( Fraig_Man_t * p );


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Initializes the feedback information.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_FeedBackInit( Fraig_Man_t * p )
{
    p->vCones    = Fraig_NodeVecAlloc( 500 );
    p->vPatsReal = Msat_IntVecAlloc( 1000 ); 
    p->pSimsReal = (unsigned *)Fraig_MemFixedEntryFetch( p->mmSims );
    memset( p->pSimsReal, 0, sizeof(unsigned) * p->nWordsDyna );
    p->pSimsTemp = (unsigned *)Fraig_MemFixedEntryFetch( p->mmSims );
    p->pSimsDiff = (unsigned *)Fraig_MemFixedEntryFetch( p->mmSims );
}

/**Function*************************************************************

  Synopsis    [Processes the feedback from teh solver.]

  Description [Array pModel gives the value of each variable in the SAT 
  solver. Array vVars is the array of integer numbers of variables
  involves in this conflict.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_FeedBack( Fraig_Man_t * p, int * pModel, Msat_IntVec_t * vVars, Fraig_Node_t * pOld, Fraig_Node_t * pNew )
{
    int nVarsPi, nWords;
    int i;
    abctime clk = Abc_Clock();

    // get the number of PI vars in the feedback (also sets the PI values)
    nVarsPi = Fraig_FeedBackPrepare( p, pModel, vVars );

    // set the PI values
    nWords = Fraig_FeedBackInsert( p, nVarsPi );
    assert( p->iWordStart + nWords <= p->nWordsDyna );

    // resimulates the words from p->iWordStart to iWordStop
    for ( i = 1; i < p->vNodes->nSize; i++ )
        if ( Fraig_NodeIsAnd(p->vNodes->pArray[i]) )
            Fraig_NodeSimulate( p->vNodes->pArray[i], p->iWordStart, p->iWordStart + nWords, 0 );

    if ( p->fDoSparse )
        Fraig_TableRehashF0( p, 0 );

    if ( !p->fChoicing )
    Fraig_FeedBackVerify( p, pOld, pNew );

    // if there is no room left, compress the patterns
    if ( p->iWordStart + nWords == p->nWordsDyna )
        p->iWordStart = Fraig_FeedBackCompress( p );
    else  // otherwise, update the starting word
        p->iWordStart += nWords;

p->timeFeed += Abc_Clock() - clk;
}

/**Function*************************************************************

  Synopsis    [Get the number and values of the PI variables.]

  Description [Returns the number of PI variables involved in this feedback.
  Fills in the internal presence and value data for the primary inputs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_FeedBackPrepare( Fraig_Man_t * p, int * pModel, Msat_IntVec_t * vVars )
{
    Fraig_Node_t * pNode;
    int i, nVars, nVarsPis, * pVars;

    // clean the presence flag for all PIs
    for ( i = 0; i < p->vInputs->nSize; i++ )
    {
        pNode = p->vInputs->pArray[i];
        pNode->fFeedUse = 0;
    }

    // get the variables involved in the feedback
    nVars = Msat_IntVecReadSize(vVars);
    pVars = Msat_IntVecReadArray(vVars);

    // set the values for the present variables
    nVarsPis = 0;
    for ( i = 0; i < nVars; i++ )
    {
        pNode = p->vNodes->pArray[ pVars[i] ];
        if ( !Fraig_NodeIsVar(pNode) )
            continue;
        // set its value
        pNode->fFeedUse = 1;
        pNode->fFeedVal = !MSAT_LITSIGN(pModel[pVars[i]]);
        nVarsPis++;
    }
    return nVarsPis;
}

/**Function*************************************************************

  Synopsis    [Inserts the new simulation patterns.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_FeedBackInsert( Fraig_Man_t * p, int nVarsPi )
{
    Fraig_Node_t * pNode;
    int nWords, iPatFlip, nPatFlipLimit, i, w;
    int fUseNoPats = 0;
    int fUse2Pats = 0;

    // get the number of words 
    if ( fUse2Pats )
        nWords = FRAIG_NUM_WORDS( 2 * nVarsPi + 1 );
    else if ( fUseNoPats )
        nWords = 1;
    else
        nWords = FRAIG_NUM_WORDS( nVarsPi + 1 );
    // update the number of words if they do not fit into the simulation info
    if ( nWords > p->nWordsDyna - p->iWordStart )
        nWords = p->nWordsDyna - p->iWordStart; 
    // determine the bound on the flipping bit
    nPatFlipLimit = nWords * 32 - 2;

    // mark the real pattern
    Msat_IntVecPush( p->vPatsReal, p->iWordStart * 32 ); 
    // record the real pattern
    Fraig_BitStringSetBit( p->pSimsReal, p->iWordStart * 32 );

    // set the values at the PIs
    iPatFlip = 1;
    for ( i = 0; i < p->vInputs->nSize; i++ )
    {
        pNode = p->vInputs->pArray[i];
        for ( w = p->iWordStart; w < p->iWordStart + nWords; w++ )
            if ( !pNode->fFeedUse )
                pNode->puSimD[w] = FRAIG_RANDOM_UNSIGNED;
            else if ( pNode->fFeedVal )
                pNode->puSimD[w] = FRAIG_FULL;
            else // if ( !pNode->fFeedVal )
                pNode->puSimD[w] = 0;

        if ( fUse2Pats )
        {
            // flip two patterns
            if ( pNode->fFeedUse && 2 * iPatFlip < nPatFlipLimit )
            {
                Fraig_BitStringXorBit( pNode->puSimD + p->iWordStart, 2 * iPatFlip - 1 );
                Fraig_BitStringXorBit( pNode->puSimD + p->iWordStart, 2 * iPatFlip     );
                Fraig_BitStringXorBit( pNode->puSimD + p->iWordStart, 2 * iPatFlip + 1 );
                iPatFlip++;
            }
        }
        else if ( fUseNoPats )
        {
        }
        else
        {
            // flip the diagonal
            if ( pNode->fFeedUse && iPatFlip < nPatFlipLimit )
            {
                Fraig_BitStringXorBit( pNode->puSimD + p->iWordStart, iPatFlip );
                iPatFlip++;
    //            Extra_PrintBinary( stdout, &pNode->puSimD, 45 ); printf( "\n" );
            }
        }
        // clean the use mask
        pNode->fFeedUse = 0;

        // add the info to the D hash value of the PIs
        for ( w = p->iWordStart; w < p->iWordStart + nWords; w++ )
            pNode->uHashD ^= pNode->puSimD[w] * s_FraigPrimes[w];

    }
    return nWords;
}


/**Function*************************************************************

  Synopsis    [Checks that the SAT solver pattern indeed distinquishes the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_FeedBackVerify( Fraig_Man_t * p, Fraig_Node_t * pOld, Fraig_Node_t * pNew )
{
    int fValue1, fValue2, iPat;
    iPat   = Msat_IntVecReadEntry( p->vPatsReal, Msat_IntVecReadSize(p->vPatsReal)-1 );
    fValue1 = (Fraig_BitStringHasBit( pOld->puSimD, iPat ));
    fValue2 = (Fraig_BitStringHasBit( pNew->puSimD, iPat ));
/*
Fraig_PrintNode( p, pOld );
printf( "\n" );
Fraig_PrintNode( p, pNew );
printf( "\n" );
*/
//    assert( fValue1 != fValue2 );
}

/**Function*************************************************************

  Synopsis    [Compress the simulation patterns.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_FeedBackCompress( Fraig_Man_t * p )
{
    unsigned * pSims;
    unsigned uHash;
    int i, w, t, nPats, * pPats;
    int fPerformChecks = (p->nBTLimit == -1);

    // solve the covering problem
    if ( fPerformChecks )
    {
        Fraig_FeedBackCheckTable( p );
        if ( p->fDoSparse ) 
            Fraig_FeedBackCheckTableF0( p );
    }

    // solve the covering problem
    Fraig_FeedBackCovering( p, p->vPatsReal );


    // get the number of additional patterns
    nPats = Msat_IntVecReadSize( p->vPatsReal );
    pPats = Msat_IntVecReadArray( p->vPatsReal );
    // get the new starting word
    p->iWordStart = FRAIG_NUM_WORDS( p->iPatsPerm + nPats );

    // set the simulation info for the PIs
    for ( i = 0; i < p->vInputs->nSize; i++ )
    {
        // get hold of the simulation info for this PI
        pSims = p->vInputs->pArray[i]->puSimD;
        // clean the storage for the new patterns
        for ( w = p->iWordPerm; w < p->iWordStart; w++ )
            p->pSimsTemp[w] = 0;
        // set the patterns
        for ( t = 0; t < nPats; t++ )
            if ( Fraig_BitStringHasBit( pSims, pPats[t] ) )
            {
                // check if this pattern falls into temporary storage
                if ( p->iPatsPerm + t < p->iWordPerm * 32 )
                    Fraig_BitStringSetBit( pSims, p->iPatsPerm + t );
                else
                    Fraig_BitStringSetBit( p->pSimsTemp, p->iPatsPerm + t );
            }
        // copy the pattern 
        for ( w = p->iWordPerm; w < p->iWordStart; w++ )
            pSims[w] = p->pSimsTemp[w];
        // recompute the hashing info
        uHash = 0;
        for ( w = 0; w < p->iWordStart; w++ )
            uHash ^= pSims[w] * s_FraigPrimes[w];
        p->vInputs->pArray[i]->uHashD = uHash;
    }

    // update info about the permanently stored patterns
    p->iWordPerm = p->iWordStart;
    p->iPatsPerm += nPats;
    assert( p->iWordPerm == FRAIG_NUM_WORDS( p->iPatsPerm ) );

    // resimulate and recompute the hash values
    for ( i = 1; i < p->vNodes->nSize; i++ )
        if ( Fraig_NodeIsAnd(p->vNodes->pArray[i]) )
        {
            p->vNodes->pArray[i]->uHashD = 0;
            Fraig_NodeSimulate( p->vNodes->pArray[i], 0, p->iWordPerm, 0 );
        }

    // double-check that the nodes are still distinguished
    if ( fPerformChecks )
        Fraig_FeedBackCheckTable( p );

    // rehash the values in the F0 table
    if ( p->fDoSparse ) 
    {
        Fraig_TableRehashF0( p, 0 );
        if ( fPerformChecks )
            Fraig_FeedBackCheckTableF0( p );
    }

    // check if we need to resize the simulation info
    // if less than FRAIG_WORDS_STORE words are left, reallocate simulation info
    if ( p->iWordPerm + FRAIG_WORDS_STORE > p->nWordsDyna )
        Fraig_ReallocateSimulationInfo( p );

    // set the real patterns
    Msat_IntVecClear( p->vPatsReal );
    memset( p->pSimsReal, 0, sizeof(unsigned)*p->nWordsDyna );
    for ( w = 0; w < p->iWordPerm; w++ )
        p->pSimsReal[w] = FRAIG_FULL;
    if ( p->iPatsPerm % 32 > 0 )
        p->pSimsReal[p->iWordPerm-1] = FRAIG_MASK( p->iPatsPerm % 32 );
//    printf( "The number of permanent words = %d.\n", p->iWordPerm );
    return p->iWordStart;
}




/**Function*************************************************************

  Synopsis    [Checks the correctness of the functional simulation table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_FeedBackCovering( Fraig_Man_t * p, Msat_IntVec_t * vPats )
{
    Fraig_NodeVec_t * vColumns;
    unsigned * pSims;
    int * pHits, iPat, iCol, i;
    int nOnesTotal, nSolStarting;
    int fVeryVerbose = 0;

    // collect the pairs to be distinguished
    vColumns = Fraig_FeedBackCoveringStart( p );
    // collect the number of 1s in each simulation vector
    nOnesTotal = 0;
    pHits = ABC_ALLOC( int, vColumns->nSize );
    for ( i = 0; i < vColumns->nSize; i++ )
    {
        pSims = (unsigned *)vColumns->pArray[i];
        pHits[i] = Fraig_BitStringCountOnes( pSims, p->iWordStart );
        nOnesTotal += pHits[i];
//        assert( pHits[i] > 0 );
    }
 
    // go through the patterns
    nSolStarting = Msat_IntVecReadSize(vPats);
    while ( (iCol = Fraig_GetSmallestColumn( pHits, vColumns->nSize )) != -1 )
    {
        // find the pattern, which hits this column
        iPat = Fraig_GetHittingPattern( (unsigned *)vColumns->pArray[iCol], p->iWordStart );
        // cancel the columns covered by this pattern
        Fraig_CancelCoveredColumns( vColumns, pHits, iPat );
        // save the pattern
        Msat_IntVecPush( vPats, iPat );
    }

    // free the set of columns
    for ( i = 0; i < vColumns->nSize; i++ )
        Fraig_MemFixedEntryRecycle( p->mmSims, (char *)vColumns->pArray[i] );

    // print stats related to the covering problem
    if ( p->fVerbose && fVeryVerbose )
    {
        printf( "%3d\\%3d\\%3d   ",     p->nWordsRand, p->nWordsDyna, p->iWordPerm );
        printf( "Col (pairs) = %5d.  ", vColumns->nSize );
        printf( "Row (pats) = %5d.  ",  p->iWordStart * 32 );
        printf( "Dns = %6.2f %%.  ",    vColumns->nSize==0? 0.0 : 100.0 * nOnesTotal / vColumns->nSize / p->iWordStart / 32 );
        printf( "Sol = %3d (%3d).  ",   Msat_IntVecReadSize(vPats), nSolStarting );
        printf( "\n" );
    }
    Fraig_NodeVecFree( vColumns );
    ABC_FREE( pHits );
}


/**Function*************************************************************

  Synopsis    [Checks the correctness of the functional simulation table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_NodeVec_t * Fraig_FeedBackCoveringStart( Fraig_Man_t * p )
{
    Fraig_NodeVec_t * vColumns;
    Fraig_HashTable_t * pT = p->pTableF;
    Fraig_Node_t * pEntF, * pEntD;
    unsigned * pSims;
    unsigned * pUnsigned1, * pUnsigned2;
    int i, k, m, w;//, nOnes;

    // start the set of columns
    vColumns = Fraig_NodeVecAlloc( 100 );

    // go through the pairs of nodes to be distinguished
    for ( i = 0; i < pT->nBins; i++ )
    Fraig_TableBinForEachEntryF( pT->pBins[i], pEntF )
    {
        p->vCones->nSize = 0;
        Fraig_TableBinForEachEntryD( pEntF, pEntD )
            Fraig_NodeVecPush( p->vCones, pEntD );
        if ( p->vCones->nSize == 1 )
            continue;
        //////////////////////////////// bug fix by alanmi, September 14, 2006
        if ( p->vCones->nSize > 20 ) 
            continue;
        ////////////////////////////////

        for ( k = 0; k < p->vCones->nSize; k++ )
            for ( m = k+1; m < p->vCones->nSize; m++ )
            {
                if ( !Fraig_CompareSimInfoUnderMask( p->vCones->pArray[k], p->vCones->pArray[m], p->iWordStart, 0, p->pSimsReal ) )
                    continue;

                // primary simulation patterns (counter-examples) cannot distinguish this pair
                // get memory to store the feasible simulation patterns
                pSims = (unsigned *)Fraig_MemFixedEntryFetch( p->mmSims );
                // find the pattern that distinguish this column, exept the primary ones
                pUnsigned1 = p->vCones->pArray[k]->puSimD;
                pUnsigned2 = p->vCones->pArray[m]->puSimD;
                for ( w = 0; w < p->iWordStart; w++ )
                    pSims[w] = (pUnsigned1[w] ^ pUnsigned2[w]) & ~p->pSimsReal[w];
                // store the pattern
                Fraig_NodeVecPush( vColumns, (Fraig_Node_t *)pSims );
//                nOnes = Fraig_BitStringCountOnes(pSims, p->iWordStart);
//                assert( nOnes > 0 );
            }      
    }

    // if the flag is not set, do not consider sparse nodes in p->pTableF0
    if ( !p->fDoSparse )
        return vColumns;

    // recalculate their hash values based on p->pSimsReal
    pT = p->pTableF0;
    for ( i = 0; i < pT->nBins; i++ )
    Fraig_TableBinForEachEntryF( pT->pBins[i], pEntF )
    {
        pSims = pEntF->puSimD;
        pEntF->uHashD = 0;
        for ( w = 0; w < p->iWordStart; w++ )
            pEntF->uHashD ^= (pSims[w] & p->pSimsReal[w]) * s_FraigPrimes[w];
    }

    // rehash the table using these values
    Fraig_TableRehashF0( p, 1 );

    // collect the classes of equivalent node pairs
    for ( i = 0; i < pT->nBins; i++ )
    Fraig_TableBinForEachEntryF( pT->pBins[i], pEntF )
    {
        p->vCones->nSize = 0;
        Fraig_TableBinForEachEntryD( pEntF, pEntD )
            Fraig_NodeVecPush( p->vCones, pEntD );
        if ( p->vCones->nSize == 1 )
            continue;

        // primary simulation patterns (counter-examples) cannot distinguish all these pairs
        for ( k = 0; k < p->vCones->nSize; k++ )
            for ( m = k+1; m < p->vCones->nSize; m++ )
            {
                // get memory to store the feasible simulation patterns
                pSims = (unsigned *)Fraig_MemFixedEntryFetch( p->mmSims );
                // find the patterns that are not distinquished
                pUnsigned1 = p->vCones->pArray[k]->puSimD;
                pUnsigned2 = p->vCones->pArray[m]->puSimD;
                for ( w = 0; w < p->iWordStart; w++ )
                    pSims[w] = (pUnsigned1[w] ^ pUnsigned2[w]) & ~p->pSimsReal[w];
                // store the pattern
                Fraig_NodeVecPush( vColumns, (Fraig_Node_t *)pSims );
//                nOnes = Fraig_BitStringCountOnes(pSims, p->iWordStart);
//                assert( nOnes > 0 );
            }      
    }
    return vColumns;
}

/**Function*************************************************************

  Synopsis    [Selects the column, which has the smallest number of hits.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_GetSmallestColumn( int * pHits, int nHits )
{
    int i, iColMin = -1, nHitsMin = 1000000;
    for ( i = 0; i < nHits; i++ )
    {
        // skip covered columns
        if ( pHits[i] == 0 )
            continue;
        // take the column if it can only be covered by one pattern
        if ( pHits[i] == 1 )
            return i;
        // find the column, which requires the smallest number of patterns
        if ( nHitsMin > pHits[i] )
        {
            nHitsMin = pHits[i];
            iColMin = i;
        }
    }
    return iColMin;
}

/**Function*************************************************************

  Synopsis    [Select the pattern, which hits this column.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_GetHittingPattern( unsigned * pSims, int nWords )
{
    int i, b;
    for ( i = 0; i < nWords; i++ )
    {
        if ( pSims[i] == 0 )
            continue;
        for ( b = 0; b < 32; b++ )
            if ( pSims[i] & (1 << b) )
                return i * 32 + b;
    }
    return -1;
}

/**Function*************************************************************

  Synopsis    [Cancel covered patterns.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_CancelCoveredColumns( Fraig_NodeVec_t * vColumns, int * pHits, int iPat )
{
    unsigned * pSims;
    int i;
    for ( i = 0; i < vColumns->nSize; i++ )
    {
        pSims = (unsigned *)vColumns->pArray[i];
        if ( Fraig_BitStringHasBit( pSims, iPat ) )
            pHits[i] = 0;
    }
}


/**Function*************************************************************

  Synopsis    [Checks the correctness of the functional simulation table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_FeedBackCheckTable( Fraig_Man_t * p )
{
    Fraig_HashTable_t * pT = p->pTableF;
    Fraig_Node_t * pEntF, * pEntD;
    int i, k, m, nPairs;

    nPairs = 0;
    for ( i = 0; i < pT->nBins; i++ )
    Fraig_TableBinForEachEntryF( pT->pBins[i], pEntF )
    {
        p->vCones->nSize = 0;
        Fraig_TableBinForEachEntryD( pEntF, pEntD )
            Fraig_NodeVecPush( p->vCones, pEntD );
        if ( p->vCones->nSize == 1 )
            continue;
        for ( k = 0; k < p->vCones->nSize; k++ )
            for ( m = k+1; m < p->vCones->nSize; m++ )
            {
                if ( Fraig_CompareSimInfo( p->vCones->pArray[k], p->vCones->pArray[m], p->iWordStart, 0 ) )
                    printf( "Nodes %d and %d have the same D simulation info.\n", 
                        p->vCones->pArray[k]->Num, p->vCones->pArray[m]->Num );
                nPairs++;
            }   
    }
//    printf( "\nThe total of %d node pairs have been verified.\n", nPairs );
}

/**Function*************************************************************

  Synopsis    [Checks the correctness of the functional simulation table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_FeedBackCheckTableF0( Fraig_Man_t * p )
{
    Fraig_HashTable_t * pT = p->pTableF0;
    Fraig_Node_t * pEntF;
    int i, k, m, nPairs;

    nPairs = 0;
    for ( i = 0; i < pT->nBins; i++ )
    {
        p->vCones->nSize = 0;
        Fraig_TableBinForEachEntryF( pT->pBins[i], pEntF )
            Fraig_NodeVecPush( p->vCones, pEntF );
        if ( p->vCones->nSize == 1 )
            continue;
        for ( k = 0; k < p->vCones->nSize; k++ )
            for ( m = k+1; m < p->vCones->nSize; m++ )
            {
                if ( Fraig_CompareSimInfo( p->vCones->pArray[k], p->vCones->pArray[m], p->iWordStart, 0 ) )
                    printf( "Nodes %d and %d have the same D simulation info.\n", 
                        p->vCones->pArray[k]->Num, p->vCones->pArray[m]->Num );
                nPairs++;
            }   
    }
//    printf( "\nThe total of %d node pairs have been verified.\n", nPairs );
}

/**Function*************************************************************

  Synopsis    [Doubles the size of simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_ReallocateSimulationInfo( Fraig_Man_t * p )
{
    Fraig_MemFixed_t * mmSimsNew;        // new memory manager for simulation info
    Fraig_Node_t * pNode;
    unsigned * pSimsNew;
    unsigned uSignOld;
    int i;

    // allocate a new memory manager
    p->nWordsDyna *= 2;
    mmSimsNew = Fraig_MemFixedStart( sizeof(unsigned) * (p->nWordsRand + p->nWordsDyna) );

    // set the new data for the constant node
    pNode = p->pConst1;
    pNode->puSimR = (unsigned *)Fraig_MemFixedEntryFetch( mmSimsNew );
    pNode->puSimD = pNode->puSimR + p->nWordsRand;
    memset( pNode->puSimR, 0, sizeof(unsigned) * p->nWordsRand );
    memset( pNode->puSimD, 0, sizeof(unsigned) * p->nWordsDyna );

    // copy the simulation info of the PIs
    for ( i = 0; i < p->vInputs->nSize; i++ )
    {
        pNode = p->vInputs->pArray[i];
        // copy the simulation info
        pSimsNew = (unsigned *)Fraig_MemFixedEntryFetch( mmSimsNew );
        memmove( pSimsNew, pNode->puSimR, sizeof(unsigned) * (p->nWordsRand + p->iWordStart) );
        // attach the new info
        pNode->puSimR = pSimsNew;
        pNode->puSimD = pNode->puSimR + p->nWordsRand;
        // signatures remain without changes
    }

    // replace the manager to free up some memory
    Fraig_MemFixedStop( p->mmSims, 0 );
    p->mmSims = mmSimsNew;

    // resimulate the internal nodes (this should lead to the same signatures)
    for ( i = 1; i < p->vNodes->nSize; i++ )
    {
        pNode = p->vNodes->pArray[i];
        if ( !Fraig_NodeIsAnd(pNode) )
            continue;
        // allocate memory for the simulation info
        pNode->puSimR = (unsigned *)Fraig_MemFixedEntryFetch( mmSimsNew );
        pNode->puSimD = pNode->puSimR + p->nWordsRand;
        // derive random simulation info
        uSignOld = pNode->uHashR;
        pNode->uHashR = 0;
        Fraig_NodeSimulate( pNode, 0, p->nWordsRand, 1 );
        assert( uSignOld == pNode->uHashR );
        // derive dynamic simulation info
        uSignOld = pNode->uHashD;
        pNode->uHashD = 0;
        Fraig_NodeSimulate( pNode, 0, p->iWordStart, 0 );
        assert( uSignOld == pNode->uHashD );
    }

    // realloc temporary storage
    p->pSimsReal = (unsigned *)Fraig_MemFixedEntryFetch( mmSimsNew );
    memset( p->pSimsReal, 0, sizeof(unsigned) * p->nWordsDyna );
    p->pSimsTemp = (unsigned *)Fraig_MemFixedEntryFetch( mmSimsNew );
    p->pSimsDiff = (unsigned *)Fraig_MemFixedEntryFetch( mmSimsNew );
}


/**Function*************************************************************

  Synopsis    [Generated trivial counter example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Fraig_ManAllocCounterExample( Fraig_Man_t * p )
{
    int * pModel;
    pModel = ABC_ALLOC( int, p->vInputs->nSize );
    memset( pModel, 0, sizeof(int) * p->vInputs->nSize );
    return pModel;
}


/**Function*************************************************************

  Synopsis    [Saves the counter example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_ManSimulateBitNode_rec( Fraig_Man_t * p, Fraig_Node_t * pNode )
{
    int Value0, Value1;
    if ( Fraig_NodeIsTravIdCurrent( p, pNode ) )
        return pNode->fMark3;
    Fraig_NodeSetTravIdCurrent( p, pNode );
    Value0 = Fraig_ManSimulateBitNode_rec( p, Fraig_Regular(pNode->p1) );
    Value1 = Fraig_ManSimulateBitNode_rec( p, Fraig_Regular(pNode->p2) );
    Value0 ^= Fraig_IsComplement(pNode->p1);
    Value1 ^= Fraig_IsComplement(pNode->p2);
    pNode->fMark3 = Value0 & Value1;
    return pNode->fMark3;
}

/**Function*************************************************************

  Synopsis    [Simulates one bit.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_ManSimulateBitNode( Fraig_Man_t * p, Fraig_Node_t * pNode, int * pModel )
{
    int fCompl, RetValue, i;
    // set the PI values
    Fraig_ManIncrementTravId( p );
    for ( i = 0; i < p->vInputs->nSize; i++ )
    {
        Fraig_NodeSetTravIdCurrent( p, p->vInputs->pArray[i] );
        p->vInputs->pArray[i]->fMark3 = pModel[i];
    }
    // perform the traversal
    fCompl = Fraig_IsComplement(pNode);
    RetValue = Fraig_ManSimulateBitNode_rec( p, Fraig_Regular(pNode) );
    return fCompl ^ RetValue;
}


/**Function*************************************************************

  Synopsis    [Saves the counter example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Fraig_ManSaveCounterExample( Fraig_Man_t * p, Fraig_Node_t * pNode )
{
    int * pModel;
    int iPattern;
    int i, fCompl;

    // the node can be complemented
    fCompl = Fraig_IsComplement(pNode);
    // because we compare with constant 0, p->pConst1 should also be complemented
    fCompl = !fCompl;

    // derive the model
    pModel = Fraig_ManAllocCounterExample( p );
    iPattern = Fraig_FindFirstDiff( p->pConst1, Fraig_Regular(pNode), fCompl, p->nWordsRand, 1 );
    if ( iPattern >= 0 )
    {
        for ( i = 0; i < p->vInputs->nSize; i++ )
            if ( Fraig_BitStringHasBit( p->vInputs->pArray[i]->puSimR, iPattern ) )
                pModel[i] = 1;
/*
printf( "SAT solver's pattern:\n" );
for ( i = 0; i < p->vInputs->nSize; i++ )
    printf( "%d", pModel[i] );
printf( "\n" );
*/
        assert( Fraig_ManSimulateBitNode( p, pNode, pModel ) );
        return pModel;
    }
    iPattern = Fraig_FindFirstDiff( p->pConst1, Fraig_Regular(pNode), fCompl, p->iWordStart, 0 );
    if ( iPattern >= 0 )
    {
        for ( i = 0; i < p->vInputs->nSize; i++ )
            if ( Fraig_BitStringHasBit( p->vInputs->pArray[i]->puSimD, iPattern ) )
                pModel[i] = 1;
/*
printf( "SAT solver's pattern:\n" );
for ( i = 0; i < p->vInputs->nSize; i++ )
    printf( "%d", pModel[i] );
printf( "\n" );
*/
        assert( Fraig_ManSimulateBitNode( p, pNode, pModel ) );
        return pModel;
    }
    ABC_FREE( pModel );
    return NULL;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

