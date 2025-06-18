/**CFile****************************************************************

  FileName    [mapperTable.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic technology mapping engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - June 1, 2004.]

  Revision    [$Id: mapperTable.c,v 1.6 2005/01/23 06:59:44 alanmi Exp $]

***********************************************************************/

#include "mapperInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// the table function for the tables
#define  MAP_TABLE_HASH(u1,u2,nSize)  (((u1) + 2003 * (u2)) % nSize)

static void Map_SuperTableResize( Map_HashTable_t * pLib );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates the hash table for supergates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Map_HashTable_t * Map_SuperTableCreate( Map_SuperLib_t * pLib )
{
    Map_HashTable_t * p;
    // allocate the table
    p = ABC_ALLOC( Map_HashTable_t, 1 );
    memset( p, 0, sizeof(Map_HashTable_t) );
    p->mmMan = pLib->mmEntries;
    // allocate and clean the bins
    p->nBins = Abc_PrimeCudd(20000);
    p->pBins = ABC_ALLOC( Map_HashEntry_t *, p->nBins );
    memset( p->pBins, 0, sizeof(Map_HashEntry_t *) * p->nBins );
    return p;
}


/**Function*************************************************************

  Synopsis    [Deallocates the supergate hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_SuperTableFree( Map_HashTable_t * p )
{
    ABC_FREE( p->pBins );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Inserts a new entry into the hash table.]

  Description [This function inserts the new gate (pGate), which will be
  accessible through its canonical form (uTruthC).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_SuperTableInsertC( Map_HashTable_t * p, unsigned uTruthC[], Map_Super_t * pGate )
{
    Map_HashEntry_t * pEnt;
    unsigned Key;
    // resize the table
    if ( p->nEntries >= 2 * p->nBins )
        Map_SuperTableResize( p );
    // check if another supergate with the same canonical form exists
    Key = MAP_TABLE_HASH( uTruthC[0], uTruthC[1], p->nBins );
    for ( pEnt = p->pBins[Key]; pEnt; pEnt = pEnt->pNext )
        if ( pEnt->uTruth[0] == uTruthC[0] && pEnt->uTruth[1] == uTruthC[1] )
            break;
    // create a new entry if it does not exist
    if ( pEnt == NULL )
    {
        // add the new entry to the table
        pEnt = (Map_HashEntry_t *)Extra_MmFixedEntryFetch( p->mmMan );
        memset( pEnt, 0, sizeof(Map_HashEntry_t) );
        pEnt->uTruth[0] = uTruthC[0];
        pEnt->uTruth[1] = uTruthC[1];
        // add the hash table entry to the corresponding linked list in the table
        pEnt->pNext   = p->pBins[Key];
        p->pBins[Key] = pEnt;
        p->nEntries++;
    }
    // add the supergate to the entry
    pGate->pNext = pEnt->pGates;
    pEnt->pGates = pGate;
    return 0;
}



/**Function*************************************************************

  Synopsis    [Inserts a new entry into the library.]

  Description [This function inserts the new gate (pGate), which will be
  accessible through its unfolded function (uTruth).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_SuperTableInsert( Map_HashTable_t * p, unsigned uTruth[], Map_Super_t * pGate, unsigned uPhase )
{
    Map_HashEntry_t * pEnt;
    unsigned Key;
    // resize the table
    if ( p->nEntries >= 2 * p->nBins )
        Map_SuperTableResize( p );
    // check if this entry already exists
    Key = MAP_TABLE_HASH( uTruth[0], uTruth[1], p->nBins );
    for ( pEnt = p->pBins[Key]; pEnt; pEnt = pEnt->pNext )
        if ( pEnt->uTruth[0] == uTruth[0] && pEnt->uTruth[1] == uTruth[1] )
            return 1;
    // add the new hash table entry to the table
    pEnt = (Map_HashEntry_t *)Extra_MmFixedEntryFetch( p->mmMan );
    memset( pEnt, 0, sizeof(Map_HashEntry_t) );
    pEnt->uTruth[0] = uTruth[0];
    pEnt->uTruth[1] = uTruth[1];
    pEnt->pGates    = pGate;
    pEnt->uPhase    = uPhase;
    // add the hash table to the corresponding linked list in the table
    pEnt->pNext   = p->pBins[Key];
    p->pBins[Key] = pEnt;
    p->nEntries++;
/*
printf( "Adding gate: %10u ", Key );
Map_LibraryPrintSupergate( pGate );
Extra_PrintBinary( stdout, uTruth, 32 );
printf( "\n" );
*/
    return 0;
}

/**Function*************************************************************

  Synopsis    [Looks up an entry in the library.]

  Description [This function looks up the function, given by its truth table,
  and return two things: (1) the linked list of supergates, which can implement
  the functions of this N-class; (2) the phase, which should be applied to the
  given function, in order to derive the canonical form of this N-class.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Map_Super_t * Map_SuperTableLookupC( Map_SuperLib_t * p, unsigned uTruth[] )
{
    Map_HashEntry_t * pEnt;
    unsigned Key;
    Key = MAP_TABLE_HASH( uTruth[0], uTruth[1], p->tTableC->nBins );
    for ( pEnt = p->tTableC->pBins[Key]; pEnt; pEnt = pEnt->pNext )
        if ( pEnt->uTruth[0] == uTruth[0] && pEnt->uTruth[1] == uTruth[1] )
            return pEnt->pGates;
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Looks up an entry in the library.]

  Description [This function looks up the function, given by its truth table,
  and return two things: (1) the linked list of supergates, which can implement
  the functions of this N-class; (2) the phase, which should be applied to the
  given function, in order to derive the canonical form of this N-class.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Map_Super_t * Map_SuperTableLookup( Map_HashTable_t * p, unsigned uTruth[], unsigned * puPhase )
{
    Map_HashEntry_t * pEnt;
    unsigned Key;
    Key = MAP_TABLE_HASH( uTruth[0], uTruth[1], p->nBins );
    for ( pEnt = p->pBins[Key]; pEnt; pEnt = pEnt->pNext )
        if ( pEnt->uTruth[0] == uTruth[0] && pEnt->uTruth[1] == uTruth[1] )
        {
            *puPhase = pEnt->uPhase;
            return pEnt->pGates;
        }
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Resizes the table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_SuperTableResize( Map_HashTable_t * p )
{
    Map_HashEntry_t ** pBinsNew;
    Map_HashEntry_t * pEnt, * pEnt2;
    int nBinsNew, Counter, i;
    unsigned Key;
    // get the new table size
    nBinsNew = Abc_PrimeCudd(2 * p->nBins); 
    // allocate a new array
    pBinsNew = ABC_ALLOC( Map_HashEntry_t *, nBinsNew );
    memset( pBinsNew, 0, sizeof(Map_HashEntry_t *) * nBinsNew );
    // rehash the entries from the old table
    Counter = 0;
    for ( i = 0; i < p->nBins; i++ )
        for ( pEnt = p->pBins[i], pEnt2 = pEnt? pEnt->pNext: NULL; pEnt; 
              pEnt = pEnt2, pEnt2 = pEnt? pEnt->pNext: NULL )
        {
            Key = MAP_TABLE_HASH( pEnt->uTruth[0], pEnt->uTruth[1], nBinsNew );
            pEnt->pNext   = pBinsNew[Key];
            pBinsNew[Key] = pEnt;
            Counter++;
        }
    assert( Counter == p->nEntries );
    // replace the table and the parameters
    ABC_FREE( p->pBins );
    p->pBins = pBinsNew;
    p->nBins = nBinsNew;
}

/**Function*************************************************************

  Synopsis    [Compares the supergates by the number of times they are used.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_SuperTableCompareSupergates( Map_Super_t ** ppS1, Map_Super_t ** ppS2 )
{
    if ( (*ppS1)->nUsed > (*ppS2)->nUsed )
        return -1;
    if ( (*ppS1)->nUsed < (*ppS2)->nUsed )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Compares the supergates by the number of times they are used.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_SuperTableCompareGatesInList( Map_Super_t ** ppS1, Map_Super_t ** ppS2 )
{
//   if ( (*ppS1)->tDelayMax.Rise > (*ppS2)->tDelayMax.Rise )
    if ( (*ppS1)->Area > (*ppS2)->Area )
        return -1;
//   if ( (*ppS1)->tDelayMax.Rise < (*ppS2)->tDelayMax.Rise )
    if ( (*ppS1)->Area < (*ppS2)->Area )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Sorts supergates by usefulness and prints out most useful.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_SuperTableSortSupergates( Map_HashTable_t * p, int nSupersMax )
{
    Map_HashEntry_t * pEnt;
    Map_Super_t ** ppSupers;
    Map_Super_t * pSuper;
    int nSupers, i;

    // copy all the supergates into one array
    ppSupers = ABC_ALLOC( Map_Super_t *, nSupersMax );
    nSupers = 0;
    for ( i = 0; i < p->nBins; i++ )
        for ( pEnt = p->pBins[i]; pEnt; pEnt = pEnt->pNext )
            for ( pSuper = pEnt->pGates; pSuper; pSuper = pSuper->pNext )
                ppSupers[nSupers++] = pSuper;

    // sort by usage
    qsort( (void *)ppSupers, (size_t)nSupers, sizeof(Map_Super_t *), 
            (int (*)(const void *, const void *)) Map_SuperTableCompareSupergates );
    assert( Map_SuperTableCompareSupergates( ppSupers, ppSupers + nSupers - 1 ) <= 0 );

    // print out the "top ten"
//    for ( i = 0; i < nSupers; i++ )
    for ( i = 0; i < 10; i++ )
    {
        if ( ppSupers[i]->nUsed == 0 )
            break;
        printf( "%5d : ",        ppSupers[i]->nUsed );
        printf( "%5d   ",        ppSupers[i]->Num );
        printf( "A = %5.2f   ",  ppSupers[i]->Area );
        printf( "D = %5.2f   ",  ppSupers[i]->tDelayMax.Rise );
        printf( "%s",            ppSupers[i]->pFormula );
        printf( "\n" );
    }
    ABC_FREE( ppSupers );
}

/**Function*************************************************************

  Synopsis    [Sorts supergates by max delay for each truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_SuperTableSortSupergatesByDelay( Map_HashTable_t * p, int nSupersMax )
{
    Map_HashEntry_t * pEnt;
    Map_Super_t ** ppSupers;
    Map_Super_t * pSuper;
    int nSupers, i, k;

    ppSupers = ABC_ALLOC( Map_Super_t *, nSupersMax );
    for ( i = 0; i < p->nBins; i++ )
        for ( pEnt = p->pBins[i]; pEnt; pEnt = pEnt->pNext )
        {
            // collect the gates in this entry
            nSupers = 0;
            for ( pSuper = pEnt->pGates; pSuper; pSuper = pSuper->pNext )
            {
                // skip supergates, whose root is the AND gate
//                if ( strcmp( Mio_GateReadName(pSuper->pRoot), "and" ) == 0 )
//                    continue;
                ppSupers[nSupers++] = pSuper;
            }
            pEnt->pGates = NULL;
            if ( nSupers == 0 )
                continue;
            // sort the gates by delay
            qsort( (void *)ppSupers, (size_t)nSupers, sizeof(Map_Super_t *), 
                    (int (*)(const void *, const void *)) Map_SuperTableCompareGatesInList );
            assert( Map_SuperTableCompareGatesInList( ppSupers, ppSupers + nSupers - 1 ) <= 0 );
            // link them in the reverse order
            for ( k = 0; k < nSupers; k++ )
            {
                ppSupers[k]->pNext = pEnt->pGates;
                pEnt->pGates = ppSupers[k];
            }
            // save the number of supergates in the list
            pEnt->pGates->nSupers = nSupers;
        }
    ABC_FREE( ppSupers );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

