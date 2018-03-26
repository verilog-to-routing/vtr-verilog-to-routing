/**CFile****************************************************************

  FileName    [satTruth.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT solver.]

  Synopsis    [Truth table computation package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: satTruth.c,v 1.4 2005/09/16 22:55:03 casem Exp $]

***********************************************************************/

#include "satTruth.h"
#include "misc/vec/vecSet.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

struct Tru_Man_t_
{
    int              nVars;        // the number of variables
    int              nWords;       // the number of words in the truth table
    int              nEntrySize;   // the size of one entry in 'int'
    int              nTableSize;   // hash table size
    int *            pTable;       // hash table
    Vec_Set_t *      pMem;         // memory for truth tables
    word *           pZero;        // temporary truth table 
    int              hIthVars[16]; // variable handles
    int              nTableLookups;
};

typedef struct Tru_One_t_ Tru_One_t; // 16 bytes minimum
struct Tru_One_t_
{
    int              Handle;       // support
    int              Next;         // next one in the table
    word             pTruth[0];    // truth table
};

static inline Tru_One_t * Tru_ManReadOne( Tru_Man_t * p, int h ) { return h ? (Tru_One_t *)Vec_SetEntry(p->pMem, h) : NULL; }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns the hash key.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Tru_ManHash( word * pTruth, int nWords, int nBins, int * pPrimes )
{
    int i;
    unsigned uHash = 0;
    for ( i = 0; i < nWords; i++ )
        uHash ^= pTruth[i] * pPrimes[i & 0x7];
    return uHash % nBins;
}

/**Function*************************************************************

  Synopsis    [Returns the given record.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Tru_ManLookup( Tru_Man_t * p, word * pTruth )
{
    static int s_Primes[10] = { 1291, 1699, 2357, 4177, 5147, 5647, 6343, 7103, 7873, 8147 };
    Tru_One_t * pEntry;
    int * pSpot;
    assert( (pTruth[0] & 1) == 0 );
    pSpot = p->pTable + Tru_ManHash( pTruth, p->nWords, p->nTableSize, s_Primes );
    for ( pEntry = Tru_ManReadOne(p, *pSpot); pEntry; pSpot = &pEntry->Next, pEntry = Tru_ManReadOne(p, *pSpot) )
        if ( Tru_ManEqual(pEntry->pTruth, pTruth, p->nWords) )
            return pSpot;
    return pSpot;
}

/**Function*************************************************************

  Synopsis    [Returns the given record.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Tru_ManResize( Tru_Man_t * p )
{
    Tru_One_t * pThis;
    int * pTableOld, * pSpot;
    int nTableSizeOld, iNext, Counter, i;
    assert( p->pTable != NULL );
    // replace the table
    pTableOld = p->pTable;
    nTableSizeOld = p->nTableSize;
    p->nTableSize = 2 * p->nTableSize + 1; 
    p->pTable = ABC_CALLOC( int, p->nTableSize );
    // rehash the entries from the old table
    Counter = 0;
    for ( i = 0; i < nTableSizeOld; i++ )
    for ( pThis = Tru_ManReadOne(p, pTableOld[i]),
          iNext = (pThis? pThis->Next : 0);  
          pThis;  pThis = Tru_ManReadOne(p, iNext),   
          iNext = (pThis? pThis->Next : 0)  )
    {
        assert( pThis->Handle );
        pThis->Next = 0;
        pSpot = Tru_ManLookup( p, pThis->pTruth );
        assert( *pSpot == 0 ); // should not be there
        *pSpot = pThis->Handle;
        Counter++;
    }
    assert( Counter == Vec_SetEntryNum(p->pMem) );
    ABC_FREE( pTableOld );
}

/**Function*************************************************************

  Synopsis    [Adds entry to the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Tru_ManInsert( Tru_Man_t * p, word * pTruth )
{
    int fCompl, * pSpot;
    if ( Tru_ManEqual0(pTruth, p->nWords) )
        return 0;
    if ( Tru_ManEqual1(pTruth, p->nWords) )
        return 1;
    p->nTableLookups++;
    if ( Vec_SetEntryNum(p->pMem) > 2 * p->nTableSize )
        Tru_ManResize( p );
    fCompl = pTruth[0] & 1;
    if ( fCompl )  
        Tru_ManNot( pTruth, p->nWords );
    pSpot = Tru_ManLookup( p, pTruth );
    if ( *pSpot == 0 )
    {
        Tru_One_t * pEntry;
        *pSpot = Vec_SetAppend( p->pMem, NULL, p->nEntrySize );
        assert( (*pSpot & 1) == 0 );
        pEntry = Tru_ManReadOne( p, *pSpot );
        Tru_ManCopy( pEntry->pTruth, pTruth, p->nWords );
        pEntry->Handle = *pSpot;
        pEntry->Next = 0;
    }
    if ( fCompl )  
        Tru_ManNot( pTruth, p->nWords );
    return *pSpot ^ fCompl;
}

/**Function*************************************************************

  Synopsis    [Start the truth table logging.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Tru_Man_t * Tru_ManAlloc( int nVars )
{
    word Masks[6] = 
    { 
        ABC_CONST(0xAAAAAAAAAAAAAAAA), 
        ABC_CONST(0xCCCCCCCCCCCCCCCC), 
        ABC_CONST(0xF0F0F0F0F0F0F0F0), 
        ABC_CONST(0xFF00FF00FF00FF00), 
        ABC_CONST(0xFFFF0000FFFF0000), 
        ABC_CONST(0xFFFFFFFF00000000) 
    };
    Tru_Man_t * p;
    int i, w;
    assert( nVars > 0 && nVars <= 16 );
    p = ABC_CALLOC( Tru_Man_t, 1 );
    p->nVars      = nVars;
    p->nWords     = (nVars < 6) ? 1 : (1 << (nVars-6));
    p->nEntrySize = (sizeof(Tru_One_t) + p->nWords * sizeof(word))/sizeof(int);
    p->nTableSize = 8147;
    p->pTable     = ABC_CALLOC( int, p->nTableSize );
    p->pMem       = Vec_SetAlloc( 16 );
    // initialize truth tables
    p->pZero = ABC_ALLOC( word, p->nWords );
    for ( i = 0; i < nVars; i++ )
    {
        for ( w = 0; w < p->nWords; w++ )
            if ( i < 6 )
                p->pZero[w] = Masks[i];
            else if ( w & (1 << (i-6)) )
                p->pZero[w] = ~(word)0;
            else
                p->pZero[w] = 0;
        p->hIthVars[i] = Tru_ManInsert( p, p->pZero );
        assert( !i || p->hIthVars[i] > p->hIthVars[i-1] );
    }
    Tru_ManClear( p->pZero, p->nWords );
    return p;
}

/**Function*************************************************************

  Synopsis    [Stop the truth table logging.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Tru_ManFree( Tru_Man_t * p )
{
    printf( "Lookups = %d. Entries = %d.\n", p->nTableLookups, Vec_SetEntryNum(p->pMem) );
    Vec_SetFree( p->pMem );
    ABC_FREE( p->pZero );
    ABC_FREE( p->pTable );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Returns elementary variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
word * Tru_ManVar( Tru_Man_t * p, int v )
{
    assert( v >= 0 && v < p->nVars );
    return Tru_ManReadOne( p, p->hIthVars[v] )->pTruth;
}

/**Function*************************************************************

  Synopsis    [Returns stored truth table]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
word * Tru_ManFunc( Tru_Man_t * p, int h )
{
    assert( (h & 1) == 0 );
    if ( h == 0 )
        return p->pZero;
    return Tru_ManReadOne( p, h )->pTruth;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

