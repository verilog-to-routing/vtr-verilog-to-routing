/**CFile****************************************************************

  FileName    [giaMem.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [Memory managers.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: giaMem.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

struct Gia_MmFixed_t_
{
    // information about individual entries
    int           nEntrySize;    // the size of one entry
    int           nEntriesAlloc; // the total number of entries allocated
    int           nEntriesUsed;  // the number of entries in use
    int           nEntriesMax;   // the max number of entries in use
    char *        pEntriesFree;  // the linked list of free entries

    // this is where the memory is stored
    int           nChunkSize;    // the size of one chunk
    int           nChunksAlloc;  // the maximum number of memory chunks 
    int           nChunks;       // the current number of memory chunks 
    char **       pChunks;       // the allocated memory

    // statistics
    int           nMemoryUsed;   // memory used in the allocated entries
    int           nMemoryAlloc;  // memory allocated
};

struct Gia_MmFlex_t_
{
    // information about individual entries
    int           nEntriesUsed;  // the number of entries allocated
    char *        pCurrent;      // the current pointer to free memory
    char *        pEnd;          // the first entry outside the free memory

    // this is where the memory is stored
    int           nChunkSize;    // the size of one chunk
    int           nChunksAlloc;  // the maximum number of memory chunks 
    int           nChunks;       // the current number of memory chunks 
    char **       pChunks;       // the allocated memory

    // statistics
    int           nMemoryUsed;   // memory used in the allocated entries
    int           nMemoryAlloc;  // memory allocated
};

struct Gia_MmStep_t_
{
    int               nMems;    // the number of fixed memory managers employed
    Gia_MmFixed_t **  pMems;    // memory managers: 2^1 words, 2^2 words, etc
    int               nMapSize; // the size of the memory array
    Gia_MmFixed_t **  pMap;     // maps the number of bytes into its memory manager
    // additional memory chunks
    int           nChunksAlloc;  // the maximum number of memory chunks 
    int           nChunks;       // the current number of memory chunks 
    char **       pChunks;       // the allocated memory
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates memory pieces of fixed size.]

  Description [The size of the chunk is computed as the minimum of
  1024 entries and 64K. Can only work with entry size at least 4 byte long.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_MmFixed_t * Gia_MmFixedStart( int nEntrySize, int nEntriesMax )
{
    Gia_MmFixed_t * p;

    p = ABC_ALLOC( Gia_MmFixed_t, 1 );
    memset( p, 0, sizeof(Gia_MmFixed_t) );

    p->nEntrySize    = nEntrySize;
    p->nEntriesAlloc = 0;
    p->nEntriesUsed  = 0;
    p->pEntriesFree  = NULL;

    p->nChunkSize = nEntriesMax / 8;
    if ( p->nChunkSize < 8 )
        p->nChunkSize = 8;

    p->nChunksAlloc  = 64;
    p->nChunks       = 0;
    p->pChunks       = ABC_ALLOC( char *, p->nChunksAlloc );

    p->nMemoryUsed   = 0;
    p->nMemoryAlloc  = 0;
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_MmFixedStop( Gia_MmFixed_t * p, int fVerbose )
{
    int i;
    if ( p == NULL )
        return;
    if ( fVerbose )
    {
        printf( "Fixed memory manager: Entry = %5d. Chunk = %5d. Chunks used = %5d.\n",
            p->nEntrySize, p->nChunkSize, p->nChunks );
        printf( "   Entries used = %8d. Entries peak = %8d. Memory used = %8d. Memory alloc = %8d.\n",
            p->nEntriesUsed, p->nEntriesMax, p->nEntrySize * p->nEntriesUsed, p->nMemoryAlloc );
    }
    for ( i = 0; i < p->nChunks; i++ )
        ABC_FREE( p->pChunks[i] );
    ABC_FREE( p->pChunks );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Gia_MmFixedEntryFetch( Gia_MmFixed_t * p )
{
    char * pTemp;
    int i;

    // check if there are still free entries
    if ( p->nEntriesUsed == p->nEntriesAlloc )
    { // need to allocate more entries
        assert( p->pEntriesFree == NULL );
        if ( p->nChunks == p->nChunksAlloc )
        {
            p->nChunksAlloc *= 2;
            p->pChunks = ABC_REALLOC( char *, p->pChunks, p->nChunksAlloc ); 
        }
        p->pEntriesFree = ABC_ALLOC( char, p->nEntrySize * p->nChunkSize );
        p->nMemoryAlloc += p->nEntrySize * p->nChunkSize;
        // transform these entries into a linked list
        pTemp = p->pEntriesFree;
        for ( i = 1; i < p->nChunkSize; i++ )
        {
            *((char **)pTemp) = pTemp + p->nEntrySize;
            pTemp += p->nEntrySize;
        }
        // set the last link
        *((char **)pTemp) = NULL;
        // add the chunk to the chunk storage
        p->pChunks[ p->nChunks++ ] = p->pEntriesFree;
        // add to the number of entries allocated
        p->nEntriesAlloc += p->nChunkSize;
    }
    // incrememt the counter of used entries
    p->nEntriesUsed++;
    if ( p->nEntriesMax < p->nEntriesUsed )
        p->nEntriesMax = p->nEntriesUsed;
    // return the first entry in the free entry list
    pTemp = p->pEntriesFree;
    p->pEntriesFree = *((char **)pTemp);
    return pTemp;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_MmFixedEntryRecycle( Gia_MmFixed_t * p, char * pEntry )
{
    // decrement the counter of used entries
    p->nEntriesUsed--;
    // add the entry to the linked list of free entries
    *((char **)pEntry) = p->pEntriesFree;
    p->pEntriesFree = pEntry;
}

/**Function*************************************************************

  Synopsis    []

  Description [Relocates all the memory except the first chunk.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_MmFixedRestart( Gia_MmFixed_t * p )
{
    int i;
    char * pTemp;
    if ( p->nChunks == 0 )
        return;
    // deallocate all chunks except the first one
    for ( i = 1; i < p->nChunks; i++ )
        ABC_FREE( p->pChunks[i] );
    p->nChunks = 1;
    // transform these entries into a linked list
    pTemp = p->pChunks[0];
    for ( i = 1; i < p->nChunkSize; i++ )
    {
        *((char **)pTemp) = pTemp + p->nEntrySize;
        pTemp += p->nEntrySize;
    }
    // set the last link
    *((char **)pTemp) = NULL;
    // set the free entry list
    p->pEntriesFree  = p->pChunks[0];
    // set the correct statistics
    p->nMemoryAlloc  = p->nEntrySize * p->nChunkSize;
    p->nMemoryUsed   = 0;
    p->nEntriesAlloc = p->nChunkSize;
    p->nEntriesUsed  = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_MmFixedReadMemUsage( Gia_MmFixed_t * p )
{
    return p->nMemoryAlloc;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_MmFixedReadMaxEntriesUsed( Gia_MmFixed_t * p )
{
    return p->nEntriesMax;
}



/**Function*************************************************************

  Synopsis    [Allocates entries of flexible size.]

  Description [Can only work with entry size at least 4 byte long.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_MmFlex_t * Gia_MmFlexStart()
{
    Gia_MmFlex_t * p;

    p = ABC_ALLOC( Gia_MmFlex_t, 1 );
    memset( p, 0, sizeof(Gia_MmFlex_t) );

    p->nEntriesUsed  = 0;
    p->pCurrent      = NULL;
    p->pEnd          = NULL;

    p->nChunkSize    = (1 << 18);
    p->nChunksAlloc  = 64;
    p->nChunks       = 0;
    p->pChunks       = ABC_ALLOC( char *, p->nChunksAlloc );

    p->nMemoryUsed   = 0;
    p->nMemoryAlloc  = 0;
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_MmFlexStop( Gia_MmFlex_t * p, int fVerbose )
{
    int i;
    if ( p == NULL )
        return;
    if ( fVerbose )
    {
        printf( "Flexible memory manager: Chunk size = %d. Chunks used = %d.\n",
            p->nChunkSize, p->nChunks );
        printf( "   Entries used = %d. Memory used = %d. Memory alloc = %d.\n",
            p->nEntriesUsed, p->nMemoryUsed, p->nMemoryAlloc );
    }
    for ( i = 0; i < p->nChunks; i++ )
        ABC_FREE( p->pChunks[i] );
    ABC_FREE( p->pChunks );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Gia_MmFlexEntryFetch( Gia_MmFlex_t * p, int nBytes )
{
    char * pTemp;
    // check if there are still free entries
    if ( p->pCurrent == NULL || p->pCurrent + nBytes > p->pEnd )
    { // need to allocate more entries
        if ( p->nChunks == p->nChunksAlloc )
        {
            p->nChunksAlloc *= 2;
            p->pChunks = ABC_REALLOC( char *, p->pChunks, p->nChunksAlloc ); 
        }
        if ( nBytes > p->nChunkSize )
        {
            // resize the chunk size if more memory is requested than it can give
            // (ideally, this should never happen)
            p->nChunkSize = 2 * nBytes;
        }
        p->pCurrent = ABC_ALLOC( char, p->nChunkSize );
        p->pEnd     = p->pCurrent + p->nChunkSize;
        p->nMemoryAlloc += p->nChunkSize;
        // add the chunk to the chunk storage
        p->pChunks[ p->nChunks++ ] = p->pCurrent;
    }
    assert( p->pCurrent + nBytes <= p->pEnd );
    // increment the counter of used entries
    p->nEntriesUsed++;
    // keep track of the memory used
    p->nMemoryUsed += nBytes;
    // return the next entry
    pTemp = p->pCurrent;
    p->pCurrent += nBytes;
    return pTemp;
}

/**Function*************************************************************

  Synopsis    []

  Description [Relocates all the memory except the first chunk.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_MmFlexRestart( Gia_MmFlex_t * p )
{
    int i;
    if ( p->nChunks == 0 )
        return;
    // deallocate all chunks except the first one
    for ( i = 1; i < p->nChunks; i++ )
        ABC_FREE( p->pChunks[i] );
    p->nChunks  = 1;
    p->nMemoryAlloc = p->nChunkSize;
    // transform these entries into a linked list
    p->pCurrent = p->pChunks[0];
    p->pEnd     = p->pCurrent + p->nChunkSize;
    p->nEntriesUsed = 0;
    p->nMemoryUsed = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_MmFlexReadMemUsage( Gia_MmFlex_t * p )
{
    return p->nMemoryUsed;
}





/**Function*************************************************************

  Synopsis    [Starts the hierarchical memory manager.]

  Description [This manager can allocate entries of any size.
  Iternally they are mapped into the entries with the number of bytes
  equal to the power of 2. The smallest entry size is 8 bytes. The
  next one is 16 bytes etc. So, if the user requests 6 bytes, he gets 
  8 byte entry. If we asks for 25 bytes, he gets 32 byte entry etc.
  The input parameters "nSteps" says how many fixed memory managers
  are employed internally. Calling this procedure with nSteps equal
  to 10 results in 10 hierarchically arranged internal memory managers, 
  which can allocate up to 4096 (1Kb) entries. Requests for larger 
  entries are handed over to malloc() and then ABC_FREE()ed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_MmStep_t * Gia_MmStepStart( int nSteps )
{
    Gia_MmStep_t * p;
    int i, k;
    p = ABC_ALLOC( Gia_MmStep_t, 1 );
    memset( p, 0, sizeof(Gia_MmStep_t) );
    p->nMems = nSteps;
    // start the fixed memory managers
    p->pMems = ABC_ALLOC( Gia_MmFixed_t *, p->nMems );
    for ( i = 0; i < p->nMems; i++ )
        p->pMems[i] = Gia_MmFixedStart( (8<<i), (1<<13) );
    // set up the mapping of the required memory size into the corresponding manager
    p->nMapSize = (4<<p->nMems);
    p->pMap = ABC_ALLOC( Gia_MmFixed_t *, p->nMapSize+1 );
    p->pMap[0] = NULL;
    for ( k = 1; k <= 4; k++ )
        p->pMap[k] = p->pMems[0];
    for ( i = 0; i < p->nMems; i++ )
        for ( k = (4<<i)+1; k <= (8<<i); k++ )
            p->pMap[k] = p->pMems[i];
//for ( i = 1; i < 100; i ++ )
//printf( "%10d: size = %10d\n", i, p->pMap[i]->nEntrySize );
    p->nChunksAlloc  = 64;
    p->nChunks       = 0;
    p->pChunks       = ABC_ALLOC( char *, p->nChunksAlloc );
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops the memory manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_MmStepStop( Gia_MmStep_t * p, int fVerbose )
{
    int i;
    for ( i = 0; i < p->nMems; i++ )
        Gia_MmFixedStop( p->pMems[i], fVerbose );
    if ( p->nChunksAlloc )
    {
        for ( i = 0; i < p->nChunks; i++ )
            ABC_FREE( p->pChunks[i] );
        ABC_FREE( p->pChunks );
    }
    ABC_FREE( p->pMems );
    ABC_FREE( p->pMap );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Creates the entry.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Gia_MmStepEntryFetch( Gia_MmStep_t * p, int nBytes )
{
    if ( nBytes == 0 )
        return NULL;
    if ( nBytes > p->nMapSize )
    {
        if ( p->nChunks == p->nChunksAlloc )
        {
            p->nChunksAlloc *= 2;
            p->pChunks = ABC_REALLOC( char *, p->pChunks, p->nChunksAlloc ); 
        }
        p->pChunks[ p->nChunks++ ] = ABC_ALLOC( char, nBytes );
        return p->pChunks[p->nChunks-1];
    }
    return Gia_MmFixedEntryFetch( p->pMap[nBytes] );
}


/**Function*************************************************************

  Synopsis    [Recycles the entry.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_MmStepEntryRecycle( Gia_MmStep_t * p, char * pEntry, int nBytes )
{
    if ( nBytes == 0 )
        return;
    if ( nBytes > p->nMapSize )
    {
//        ABC_FREE( pEntry );
        return;
    }
    Gia_MmFixedEntryRecycle( p->pMap[nBytes], pEntry );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_MmStepReadMemUsage( Gia_MmStep_t * p )
{
    int i, nMemTotal = 0;
    for ( i = 0; i < p->nMems; i++ )
        nMemTotal += p->pMems[i]->nMemoryAlloc;
    return nMemTotal;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
ABC_NAMESPACE_IMPL_END

