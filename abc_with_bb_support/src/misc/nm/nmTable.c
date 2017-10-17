/**CFile****************************************************************

  FileName    [nmTable.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Name manager.]

  Synopsis    [Hash table for the name manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: nmTable.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "nmInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// hashing for integers
static unsigned Nm_HashNumber( int Num, int TableSize ) 
{
    unsigned Key = 0;
    Key ^= ( Num        & 0xFF) * 7937;
    Key ^= ((Num >>  8) & 0xFF) * 2971;
    Key ^= ((Num >> 16) & 0xFF) * 911;
    Key ^= ((Num >> 24) & 0xFF) * 353;
    return Key % TableSize;
}

// hashing for strings
static unsigned Nm_HashString( char * pName, int TableSize ) 
{
    static int s_Primes[10] = { 
        1291, 1699, 2357, 4177, 5147, 
        5647, 6343, 7103, 7873, 8147
    };
    unsigned i, Key = 0;
    for ( i = 0; pName[i] != '\0'; i++ )
        Key ^= s_Primes[i%10]*pName[i]*pName[i];
    return Key % TableSize;
}

static void Nm_ManResize( Nm_Man_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Adds an entry to two hash tables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nm_ManTableAdd( Nm_Man_t * p, Nm_Entry_t * pEntry )
{
    Nm_Entry_t ** ppSpot, * pOther;
    // resize the tables if needed
    if ( p->nEntries > p->nBins * p->nSizeFactor )
        Nm_ManResize( p );
    // add the entry to the table Id->Name
    assert( Nm_ManTableLookupId(p, pEntry->ObjId) == NULL );
    ppSpot = p->pBinsI2N + Nm_HashNumber(pEntry->ObjId, p->nBins);
    pEntry->pNextI2N = *ppSpot;
    *ppSpot = pEntry;
    // check if an entry with the same name already exists
    if ( (pOther = Nm_ManTableLookupName(p, pEntry->Name, -1)) )
    {
        // entry with the same name already exists - add it to the ring
        pEntry->pNameSake = pOther->pNameSake? pOther->pNameSake : pOther;
        pOther->pNameSake = pEntry;
    }
    else
    {
        // entry with the same name does not exist - add it to the table
        ppSpot = p->pBinsN2I + Nm_HashString(pEntry->Name, p->nBins);
        pEntry->pNextN2I = *ppSpot;
        *ppSpot = pEntry;
    }
    // report successfully added entry
    p->nEntries++;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Deletes the entry from two hash tables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nm_ManTableDelete( Nm_Man_t * p, int ObjId )
{
    Nm_Entry_t ** ppSpot, * pEntry, * pPrev;
    int fRemoved;
    p->nEntries--;
    // remove the entry from the table Id->Name
    assert( Nm_ManTableLookupId(p, ObjId) != NULL );
    ppSpot = p->pBinsI2N + Nm_HashNumber(ObjId, p->nBins);
    while ( (*ppSpot)->ObjId != (unsigned)ObjId )
        ppSpot = &(*ppSpot)->pNextI2N;
    pEntry = *ppSpot; 
    *ppSpot = (*ppSpot)->pNextI2N;
    // remove the entry from the table Name->Id
    ppSpot = p->pBinsN2I + Nm_HashString(pEntry->Name, p->nBins);
    while ( *ppSpot && *ppSpot != pEntry )
        ppSpot = &(*ppSpot)->pNextN2I;
    // remember if we found this one in the list
    fRemoved = (*ppSpot != NULL);
    if ( *ppSpot )
    {
        assert( *ppSpot == pEntry );
        *ppSpot = (*ppSpot)->pNextN2I;
    }
    // quit if this entry has no namesakes
    if ( pEntry->pNameSake == NULL )
    {
        assert( fRemoved );
        return 1;
    }
    // remove entry from the ring of namesakes
    assert( pEntry->pNameSake != pEntry );
    for ( pPrev = pEntry; pPrev->pNameSake != pEntry; pPrev = pPrev->pNameSake );
    assert( !strcmp(pPrev->Name, pEntry->Name) );
    assert( pPrev->pNameSake == pEntry );
    if ( pEntry->pNameSake == pPrev ) // two entries in the ring
        pPrev->pNameSake = NULL;
    else
        pPrev->pNameSake = pEntry->pNameSake;
    // reinsert the ring back if we removed its connection with the list in the table
    if ( fRemoved )
    {
        assert( pPrev->pNextN2I == NULL );
        pPrev->pNextN2I = *ppSpot;
        *ppSpot = pPrev;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Looks up the entry by ID.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Nm_Entry_t * Nm_ManTableLookupId( Nm_Man_t * p, int ObjId )
{
    Nm_Entry_t * pEntry;
    for ( pEntry = p->pBinsI2N[ Nm_HashNumber(ObjId, p->nBins) ]; pEntry; pEntry = pEntry->pNextI2N )
        if ( pEntry->ObjId == (unsigned)ObjId )
            return pEntry;
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Looks up the entry by name and type.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Nm_Entry_t * Nm_ManTableLookupName( Nm_Man_t * p, char * pName, int Type )
{
    Nm_Entry_t * pEntry, * pTemp;
    for ( pEntry = p->pBinsN2I[ Nm_HashString(pName, p->nBins) ]; pEntry; pEntry = pEntry->pNextN2I )
    {
        // check the entry itself
        if ( !strcmp(pEntry->Name, pName) && (Type == -1 || pEntry->Type == (unsigned)Type) )
            return pEntry;
        // if there is no namesakes, continue
        if ( pEntry->pNameSake == NULL )
            continue;
        // check the list of namesakes
        for ( pTemp = pEntry->pNameSake; pTemp != pEntry; pTemp = pTemp->pNameSake )
            if ( !strcmp(pTemp->Name, pName) && (Type == -1 || pTemp->Type == (unsigned)Type) )
                return pTemp;
    }
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Profiles hash tables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nm_ManProfile( Nm_Man_t * p )
{
    Nm_Entry_t * pEntry;
    int Counter, e;
    printf( "I2N table: " );
    for ( e = 0; e < p->nBins; e++ )
    {
        Counter = 0;
        for ( pEntry = p->pBinsI2N[e]; pEntry; pEntry = pEntry->pNextI2N )
            Counter++;
        printf( "%d ", Counter );
    }
    printf( "\n" );
    printf( "N2I table: " );
    for ( e = 0; e < p->nBins; e++ )
    {
        Counter = 0;
        for ( pEntry = p->pBinsN2I[e]; pEntry; pEntry = pEntry->pNextN2I )
            Counter++;
        printf( "%d ", Counter );
    }
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Resizes the table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nm_ManResize( Nm_Man_t * p )
{
    Nm_Entry_t ** pBinsNewI2N, ** pBinsNewN2I, * pEntry, * pEntry2, ** ppSpot;
    int nBinsNew, Counter, e;
    abctime clk;

clk = Abc_Clock();
    // get the new table size
    nBinsNew = Abc_PrimeCudd( p->nGrowthFactor * p->nBins ); 
    // allocate a new array
    pBinsNewI2N = ABC_ALLOC( Nm_Entry_t *, nBinsNew );
    pBinsNewN2I = ABC_ALLOC( Nm_Entry_t *, nBinsNew );
    memset( pBinsNewI2N, 0, sizeof(Nm_Entry_t *) * nBinsNew );
    memset( pBinsNewN2I, 0, sizeof(Nm_Entry_t *) * nBinsNew );
    // rehash entries in Id->Name table
    Counter = 0;
    for ( e = 0; e < p->nBins; e++ )
        for ( pEntry = p->pBinsI2N[e], pEntry2 = pEntry? pEntry->pNextI2N : NULL; 
              pEntry; pEntry = pEntry2, pEntry2 = pEntry? pEntry->pNextI2N : NULL )
            {
                ppSpot = pBinsNewI2N + Nm_HashNumber(pEntry->ObjId, nBinsNew);
                pEntry->pNextI2N = *ppSpot;
                *ppSpot = pEntry;
                Counter++;
            }
    // rehash entries in Name->Id table
    for ( e = 0; e < p->nBins; e++ )
        for ( pEntry = p->pBinsN2I[e], pEntry2 = pEntry? pEntry->pNextN2I : NULL; 
              pEntry; pEntry = pEntry2, pEntry2 = pEntry? pEntry->pNextN2I : NULL )
            {
                ppSpot = pBinsNewN2I + Nm_HashString(pEntry->Name, nBinsNew);
                pEntry->pNextN2I = *ppSpot;
                *ppSpot = pEntry;
            }
    assert( Counter == p->nEntries );
//    printf( "Increasing the structural table size from %6d to %6d. ", p->nBins, nBinsNew );
//    ABC_PRT( "Time", Abc_Clock() - clk );
    // replace the table and the parameters
    ABC_FREE( p->pBinsI2N );
    ABC_FREE( p->pBinsN2I );
    p->pBinsI2N = pBinsNewI2N;
    p->pBinsN2I = pBinsNewN2I;
    p->nBins = nBinsNew;
//    Nm_ManProfile( p );
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

