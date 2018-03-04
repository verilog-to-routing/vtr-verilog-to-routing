/**CFile****************************************************************

  FileName    [satStore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT solver.]

  Synopsis    [Records the trace of SAT solving in the CNF form.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: satStore.c,v 1.4 2005/09/16 22:55:03 casem Exp $]

***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "satStore.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Fetches memory.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Sto_ManMemoryFetch( Sto_Man_t * p, int nBytes )
{
    char * pMem;
    if ( p->pChunkLast == NULL || nBytes > p->nChunkSize - p->nChunkUsed )
    {
        pMem = (char *)ABC_ALLOC( char, p->nChunkSize );
        *(char **)pMem = p->pChunkLast;
        p->pChunkLast = pMem;
        p->nChunkUsed = sizeof(char *);
    }
    pMem = p->pChunkLast + p->nChunkUsed;
    p->nChunkUsed += nBytes;
    return pMem;
}

/**Function*************************************************************

  Synopsis    [Frees memory manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sto_ManMemoryStop( Sto_Man_t * p )
{
    char * pMem, * pNext;
    if ( p->pChunkLast == NULL )
        return;
    for ( pMem = p->pChunkLast; (pNext = *(char **)pMem); pMem = pNext )
        ABC_FREE( pMem );
    ABC_FREE( pMem );
}

/**Function*************************************************************

  Synopsis    [Reports memory usage in bytes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sto_ManMemoryReport( Sto_Man_t * p )
{
    int Total;
    char * pMem, * pNext;
    if ( p->pChunkLast == NULL )
        return 0;
    Total = p->nChunkUsed; 
    for ( pMem = p->pChunkLast; (pNext = *(char **)pMem); pMem = pNext )
        Total += p->nChunkSize;
    return Total;
}


/**Function*************************************************************

  Synopsis    [Allocate proof manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sto_Man_t * Sto_ManAlloc()
{
    Sto_Man_t * p;
    // allocate the manager
    p = (Sto_Man_t *)ABC_ALLOC( char, sizeof(Sto_Man_t) );
    memset( p, 0, sizeof(Sto_Man_t) );
    // memory management
    p->nChunkSize = (1<<16); // use 64K chunks
    return p;    
}

/**Function*************************************************************

  Synopsis    [Deallocate proof manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sto_ManFree( Sto_Man_t * p )
{
    Sto_ManMemoryStop( p );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Adds one clause to the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sto_ManAddClause( Sto_Man_t * p, lit * pBeg, lit * pEnd )
{
    Sto_Cls_t * pClause;
    lit Lit, * i, * j;
    int nSize;

    // process the literals
    if ( pBeg < pEnd )
    {
        // insertion sort
        for ( i = pBeg + 1; i < pEnd; i++ )
        {
            Lit = *i;
            for ( j = i; j > pBeg && *(j-1) > Lit; j-- )
                *j = *(j-1);
            *j = Lit;
        }
        // make sure there is no duplicated variables
        for ( i = pBeg + 1; i < pEnd; i++ )
            if ( lit_var(*(i-1)) == lit_var(*i) )
            {
                printf( "The clause contains two literals of the same variable: %d and %d.\n", *(i-1), *i );
                return 0;
            }
        // check the largest var size
        p->nVars = STO_MAX( p->nVars, lit_var(*(pEnd-1)) + 1 );
    }

    // get memory for the clause
    nSize = sizeof(Sto_Cls_t) + sizeof(lit) * (pEnd - pBeg);
    nSize = (nSize / sizeof(char*) + ((nSize % sizeof(char*)) > 0)) * sizeof(char*); // added by Saurabh on Sep 3, 2009
    pClause = (Sto_Cls_t *)Sto_ManMemoryFetch( p, nSize );
    memset( pClause, 0, sizeof(Sto_Cls_t) );

    // assign the clause
    pClause->Id = p->nClauses++;
    pClause->nLits = pEnd - pBeg;
    memcpy( pClause->pLits, pBeg, sizeof(lit) * (pEnd - pBeg) );
//    assert( pClause->pLits[0] >= 0 );

    // add the clause to the list
    if ( p->pHead == NULL )
        p->pHead = pClause;
    if ( p->pTail == NULL )
        p->pTail = pClause;
    else
    {
        p->pTail->pNext = pClause;
        p->pTail = pClause;
    }

    // add the empty clause
    if ( pClause->nLits == 0 )
    {
        if ( p->pEmpty )
        {
            printf( "More than one empty clause!\n" );
            return 0;
        }
        p->pEmpty = pClause;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Mark all clauses added so far as root clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sto_ManMarkRoots( Sto_Man_t * p )
{
    Sto_Cls_t * pClause;
    p->nRoots = 0;
    Sto_ManForEachClause( p, pClause )
    {
        pClause->fRoot = 1;
        p->nRoots++;
    }
}

/**Function*************************************************************

  Synopsis    [Mark all clauses added so far as clause of A.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sto_ManMarkClausesA( Sto_Man_t * p )
{
    Sto_Cls_t * pClause;
    p->nClausesA = 0;
    Sto_ManForEachClause( p, pClause )
    {
        pClause->fA = 1;
        p->nClausesA++;
    }
}

/**Function*************************************************************

  Synopsis    [Returns the literal of the last clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sto_ManChangeLastClause( Sto_Man_t * p )
{
    Sto_Cls_t * pClause, * pPrev;
    pPrev = NULL;
    Sto_ManForEachClause( p, pClause )
        pPrev = pClause;
    assert( pPrev != NULL );
    assert( pPrev->fA == 1 );
    assert( pPrev->nLits == 1 );
    p->nClausesA--;
    pPrev->fA = 0;
    return pPrev->pLits[0] >> 1;
}


/**Function*************************************************************

  Synopsis    [Writes the stored clauses into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sto_ManDumpClauses( Sto_Man_t * p, char * pFileName )
{
    FILE * pFile;
    Sto_Cls_t * pClause;
    int i;
    // start the file
    pFile = fopen( pFileName, "w" );
    if ( pFile == NULL )
    {
        printf( "Error: Cannot open output file (%s).\n", pFileName );
        return;
    }
    // write the data
    fprintf( pFile, "p %d %d %d %d\n", p->nVars, p->nClauses, p->nRoots, p->nClausesA );
    Sto_ManForEachClause( p, pClause )
    {
        for ( i = 0; i < (int)pClause->nLits; i++ )
            fprintf( pFile, " %d", lit_print(pClause->pLits[i]) );
        fprintf( pFile, " 0\n" );
    }
//    fprintf( pFile, " 0\n" );
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    [Reads one literal from file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sto_ManLoadNumber( FILE * pFile, int * pNumber )
{
    int Char, Number = 0, Sign = 0;
    // skip space-like chars
    do {
        Char = fgetc( pFile );
        if ( Char == EOF )
            return 0;
    } while ( Char == ' ' || Char == '\t' || Char == '\r' || Char == '\n' );
    // read the literal
    while ( 1 )
    {
        // get the next character
        Char = fgetc( pFile );
        if ( Char == ' ' || Char == '\t' || Char == '\r' || Char == '\n' )
            break;
        // check that the char is a digit
        if ( (Char < '0' || Char > '9') && Char != '-' )
        {
            printf( "Error: Wrong char (%c) in the input file.\n", Char );
            return 0;
        }
        // check if this is a minus
        if ( Char == '-' )
            Sign = 1;
        else
            Number = 10 * Number + Char;
    }
    // return the number
    *pNumber = Sign? -Number : Number;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Reads CNF from file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sto_Man_t * Sto_ManLoadClauses( char * pFileName )
{
    FILE * pFile;
    Sto_Man_t * p;
    Sto_Cls_t * pClause;
    char pBuffer[1024];
    int nLits, nLitsAlloc, Counter, Number;
    lit * pLits;

    // start the file
    pFile = fopen( pFileName, "r" );
    if ( pFile == NULL )
    {
        printf( "Error: Cannot open input file (%s).\n", pFileName );
        return NULL;
    }

    // create the manager
    p = Sto_ManAlloc();

    // alloc the array of literals
    nLitsAlloc = 1024;
    pLits = (lit *)ABC_ALLOC( char, sizeof(lit) * nLitsAlloc );

    // read file header
    p->nVars = p->nClauses = p->nRoots = p->nClausesA = 0;
    while ( fgets( pBuffer, 1024, pFile ) )
    {
        if ( pBuffer[0] == 'c' )
            continue;
        if ( pBuffer[0] == 'p' )
        {
            sscanf( pBuffer + 1, "%d %d %d %d", &p->nVars, &p->nClauses, &p->nRoots, &p->nClausesA );
            break;
        }
        printf( "Warning: Skipping line: \"%s\"\n", pBuffer );
    }

    // read the clauses
    nLits = 0;
    while ( Sto_ManLoadNumber(pFile, &Number) )
    {
        if ( Number == 0 )
        {
            int RetValue;
            RetValue = Sto_ManAddClause( p, pLits, pLits + nLits );
            assert( RetValue );
            nLits = 0;
            continue;
        }
        if ( nLits == nLitsAlloc )
        {
            nLitsAlloc *= 2;
            pLits = ABC_REALLOC( lit, pLits, nLitsAlloc );
        }
        pLits[ nLits++ ] = lit_read(Number);
    }
    if ( nLits > 0 )
        printf( "Error: The last clause was not saved.\n" );

    // count clauses
    Counter = 0;
    Sto_ManForEachClause( p, pClause )
        Counter++;

    // check the number of clauses
    if ( p->nClauses != Counter )
    {
        printf( "Error: The actual number of clauses (%d) is different than declared (%d).\n", Counter, p->nClauses );
        Sto_ManFree( p );
        return NULL;
    }

    ABC_FREE( pLits );
    fclose( pFile );
    return p;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

