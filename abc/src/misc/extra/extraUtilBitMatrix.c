/**CFile****************************************************************

  FileName    [extraUtilBitMatrix.c]

  PackageName [extra]

  Synopsis    [Various reusable software utilities.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 1, 2003.]

  Revision    [$Id: extraUtilBitMatrix.c,v 1.0 2003/09/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "extra.h"

ABC_NAMESPACE_IMPL_START


/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

struct Extra_BitMat_t_
{
    unsigned ** ppData;      // bit data
    int         nSize;       // the number of bits in one dimension
    int         nWords;      // the number of words in one dimension
    int         nBitShift;   // the number of bits to shift to get words
    unsigned    uMask;       // the mask to get the number of bits in the word
    int         nLookups;    // the number of lookups  
    int         nInserts;    // the number of inserts
    int         nDeletes;    // the number of deletions
};

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

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function*************************************************************

  Synopsis    [Starts the bit matrix.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Extra_BitMat_t * Extra_BitMatrixStart( int nSize )
{
    Extra_BitMat_t * p;
    int i;
    p = ABC_ALLOC( Extra_BitMat_t, 1 );
    memset( p, 0, sizeof(Extra_BitMat_t) );
    p->nSize     = nSize;
    p->nBitShift = (sizeof(unsigned) == 4) ?  5:  6;
    p->uMask     = (sizeof(unsigned) == 4) ? 31: 63;
    p->nWords    = nSize / (8 * sizeof(unsigned)) + ((nSize % (8 * sizeof(unsigned))) > 0);
    p->ppData    = ABC_ALLOC( unsigned *, nSize );
    p->ppData[0] = ABC_ALLOC( unsigned, nSize * p->nWords );
    memset( p->ppData[0], 0, sizeof(unsigned) * nSize * p->nWords );
    for ( i = 1; i < nSize; i++ )
        p->ppData[i] = p->ppData[i-1] + p->nWords;
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops the bit matrix.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_BitMatrixClean( Extra_BitMat_t * p )
{
    memset( p->ppData[0], 0, sizeof(unsigned) * p->nSize * p->nWords );
}

/**Function*************************************************************

  Synopsis    [Stops the bit matrix.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_BitMatrixStop( Extra_BitMat_t * p )
{
    ABC_FREE( p->ppData[0] );
    ABC_FREE( p->ppData );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Prints the bit-matrix.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_BitMatrixPrint( Extra_BitMat_t * pMat )
{
    int i, k, nVars;
    printf( "\n" );
    nVars = Extra_BitMatrixReadSize( pMat );
    for ( i = 0; i < nVars; i++ )
    {
        for ( k = 0; k <= i; k++ )
            printf( " " );
        for ( k = i+1; k < nVars; k++ )
            if ( Extra_BitMatrixLookup1( pMat, i, k ) )
                printf( "1" );
            else
                printf( "." );
        printf( "\n" );
    }
}


/**Function*************************************************************

  Synopsis    [Reads the matrix size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_BitMatrixReadSize( Extra_BitMat_t * p )
{
    return p->nSize;
}

/**Function*************************************************************

  Synopsis    [Inserts the element into the upper part.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_BitMatrixInsert1( Extra_BitMat_t * p, int i, int k )
{
    p->nInserts++;
    if ( i < k )
        p->ppData[i][k>>p->nBitShift] |= (1<<(k & p->uMask));
    else
        p->ppData[k][i>>p->nBitShift] |= (1<<(i & p->uMask));
}

/**Function*************************************************************

  Synopsis    [Inserts the element into the upper part.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_BitMatrixLookup1( Extra_BitMat_t * p, int i, int k )
{
    p->nLookups++;
    if ( i < k )
        return ((p->ppData[i][k>>p->nBitShift] & (1<<(k & p->uMask))) > 0);
    else
        return ((p->ppData[k][i>>p->nBitShift] & (1<<(i & p->uMask))) > 0);
}

/**Function*************************************************************

  Synopsis    [Inserts the element into the upper part.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_BitMatrixDelete1( Extra_BitMat_t * p, int i, int k )
{
    p->nDeletes++;
    if ( i < k )
        p->ppData[i][k>>p->nBitShift] &= ~(1<<(k & p->uMask));
    else
        p->ppData[k][i>>p->nBitShift] &= ~(1<<(i & p->uMask));
}



/**Function*************************************************************

  Synopsis    [Inserts the element into the upper part.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_BitMatrixInsert2( Extra_BitMat_t * p, int i, int k )
{
    p->nInserts++;
    if ( i > k )
        p->ppData[i][k>>p->nBitShift] |= (1<<(k & p->uMask));
    else
        p->ppData[k][i>>p->nBitShift] |= (1<<(i & p->uMask));
}

/**Function*************************************************************

  Synopsis    [Inserts the element into the upper part.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_BitMatrixLookup2( Extra_BitMat_t * p, int i, int k )
{
    p->nLookups++;
    if ( i > k )
        return ((p->ppData[i][k>>p->nBitShift] & (1<<(k & p->uMask))) > 0);
    else
        return ((p->ppData[k][i>>p->nBitShift] & (1<<(i & p->uMask))) > 0);
}

/**Function*************************************************************

  Synopsis    [Inserts the element into the upper part.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_BitMatrixDelete2( Extra_BitMat_t * p, int i, int k )
{
    p->nDeletes++;
    if ( i > k )
        p->ppData[i][k>>p->nBitShift] &= ~(1<<(k & p->uMask));
    else
        p->ppData[k][i>>p->nBitShift] &= ~(1<<(i & p->uMask));
}


/**Function*************************************************************

  Synopsis    [Inserts the element into the upper part.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_BitMatrixOr( Extra_BitMat_t * p, int i, unsigned * pInfo )
{
    int w;
    for ( w = 0; w < p->nWords; w++ )
        p->ppData[i][w] |= pInfo[w];
}

/**Function*************************************************************

  Synopsis    [Inserts the element into the upper part.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_BitMatrixOrTwo( Extra_BitMat_t * p, int i, int j )
{
    int w;
    for ( w = 0; w < p->nWords; w++ )
        p->ppData[i][w] = p->ppData[j][w] = (p->ppData[i][w] | p->ppData[j][w]);
}

/**Function*************************************************************

  Synopsis    [Counts the number of 1's in the upper rectangle.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_BitMatrixCountOnesUpper( Extra_BitMat_t * p )
{
    int i, k, nTotal = 0;
    for ( i = 0; i < p->nSize; i++ )
        for ( k = i + 1; k < p->nSize; k++ )
            nTotal += ( (p->ppData[i][k>>5] & (1 << (k&31))) > 0 );
    return nTotal;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the matrices have no entries in common.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_BitMatrixIsDisjoint( Extra_BitMat_t * p1, Extra_BitMat_t * p2 )
{
    int i, w;
    assert( p1->nSize == p2->nSize );
    for ( i = 0; i < p1->nSize; i++ )
        for ( w = 0; w < p1->nWords; w++ )
            if ( p1->ppData[i][w] & p2->ppData[i][w] )
                return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the matrix is a set of cliques.]

  Description [For example pairwise symmetry info should satisfy this property.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_BitMatrixIsClique( Extra_BitMat_t * pMat )
{
    int v, u, i;
    for ( v = 0; v < pMat->nSize; v++ )
    for ( u = v+1; u < pMat->nSize; u++ )
    {
        if ( !Extra_BitMatrixLookup1( pMat, v, u ) )
            continue;
        // v and u are symmetric
        for ( i = 0; i < pMat->nSize; i++ )
        {
            if ( i == v || i == u )
                continue;
            // i is neither v nor u
            // the symmetry status of i is the same w.r.t. to v and u
            if ( Extra_BitMatrixLookup1( pMat, i, v ) != Extra_BitMatrixLookup1( pMat, i, u ) )
                return 0;
        }
    }
    return 1;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

