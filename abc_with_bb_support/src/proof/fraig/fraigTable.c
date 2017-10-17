/**CFile****************************************************************

  FileName    [fraigTable.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [Structural and functional hash tables.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - October 1, 2004]

  Revision    [$Id: fraigTable.c,v 1.7 2005/07/08 01:01:34 alanmi Exp $]

***********************************************************************/

#include "fraigInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Fraig_TableResizeS( Fraig_HashTable_t * p );
static void Fraig_TableResizeF( Fraig_HashTable_t * p, int fUseSimR );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_HashTable_t * Fraig_HashTableCreate( int nSize )
{
    Fraig_HashTable_t * p;
    // allocate the table
    p = ABC_ALLOC( Fraig_HashTable_t, 1 );
    memset( p, 0, sizeof(Fraig_HashTable_t) );
    // allocate and clean the bins
    p->nBins = Abc_PrimeCudd(nSize);
    p->pBins = ABC_ALLOC( Fraig_Node_t *, p->nBins );
    memset( p->pBins, 0, sizeof(Fraig_Node_t *) * p->nBins );
    return p;
}

/**Function*************************************************************

  Synopsis    [Deallocates the supergate hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_HashTableFree( Fraig_HashTable_t * p )
{
    ABC_FREE( p->pBins );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Looks up an entry in the structural hash table.]

  Description [If the entry with the same children does not exists, 
  creates it, inserts it into the table, and returns 0. If the entry 
  with the same children exists, finds it, and return 1. In both cases, 
  the new/old entry is returned in ppNodeRes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_HashTableLookupS( Fraig_Man_t * pMan, Fraig_Node_t * p1, Fraig_Node_t * p2, Fraig_Node_t ** ppNodeRes )
{
    Fraig_HashTable_t * p = pMan->pTableS;
    Fraig_Node_t * pEnt;
    unsigned Key;

    // order the arguments
    if ( Fraig_Regular(p1)->Num > Fraig_Regular(p2)->Num )
        pEnt = p1, p1 = p2, p2 = pEnt;

    Key = Fraig_HashKey2( p1, p2, p->nBins );
    Fraig_TableBinForEachEntryS( p->pBins[Key], pEnt )
        if ( pEnt->p1 == p1 && pEnt->p2 == p2 )
        {
            *ppNodeRes = pEnt;
            return 1;
        }
    // check if it is a good time for table resizing
    if ( p->nEntries >= 2 * p->nBins )
    {
        Fraig_TableResizeS( p );
        Key = Fraig_HashKey2( p1, p2, p->nBins );
    }
    // create the new node
    pEnt = Fraig_NodeCreate( pMan, p1, p2 );
    // add the node to the corresponding linked list in the table
    pEnt->pNextS = p->pBins[Key];
    p->pBins[Key] = pEnt;
    *ppNodeRes = pEnt;
    p->nEntries++;
    return 0;
}


/**Function*************************************************************

  Synopsis    [Insert the entry in the functional hash table.]

  Description [If the entry with the same key exists, return it right away.  
  If the entry with the same key does not exists, inserts it and returns NULL. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t * Fraig_HashTableLookupF( Fraig_Man_t * pMan, Fraig_Node_t * pNode )
{
    Fraig_HashTable_t * p = pMan->pTableF;
    Fraig_Node_t * pEnt, * pEntD;
    unsigned Key;

    // go through the hash table entries
    Key = pNode->uHashR % p->nBins;
    Fraig_TableBinForEachEntryF( p->pBins[Key], pEnt )
    {
        // if their simulation info differs, skip
        if ( !Fraig_CompareSimInfo( pNode, pEnt, pMan->nWordsRand, 1 ) )
            continue;
        // equivalent up to the complement
        Fraig_TableBinForEachEntryD( pEnt, pEntD )
        {
            // if their simulation info differs, skip
            if ( !Fraig_CompareSimInfo( pNode, pEntD, pMan->iWordStart, 0 ) )
                continue;
            // found a simulation-equivalent node
            return pEntD; 
        }
        // did not find a simulation equivalent node
        // add the node to the corresponding linked list
        pNode->pNextD = pEnt->pNextD;
        pEnt->pNextD  = pNode;
        // return NULL, because there is no functional equivalence in this case
        return NULL;
    }

    // check if it is a good time for table resizing
    if ( p->nEntries >= 2 * p->nBins )
    {
        Fraig_TableResizeF( p, 1 );
        Key = pNode->uHashR % p->nBins;
    }

    // add the node to the corresponding linked list in the table
    pNode->pNextF = p->pBins[Key];
    p->pBins[Key] = pNode;
    p->nEntries++;
    // return NULL, because there is no functional equivalence in this case
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Insert the entry in the functional hash table.]

  Description [If the entry with the same key exists, return it right away.  
  If the entry with the same key does not exists, inserts it and returns NULL. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t * Fraig_HashTableLookupF0( Fraig_Man_t * pMan, Fraig_Node_t * pNode )
{
    Fraig_HashTable_t * p = pMan->pTableF0;
    Fraig_Node_t * pEnt;
    unsigned Key;

    // go through the hash table entries
    Key = pNode->uHashD % p->nBins;
    Fraig_TableBinForEachEntryF( p->pBins[Key], pEnt )
    {
        // if their simulation info differs, skip
        if ( !Fraig_CompareSimInfo( pNode, pEnt, pMan->iWordStart, 0 ) )
            continue;
        // found a simulation-equivalent node
        return pEnt; 
    }

    // check if it is a good time for table resizing
    if ( p->nEntries >= 2 * p->nBins )
    {
        Fraig_TableResizeF( p, 0 );
        Key = pNode->uHashD % p->nBins;
    }

    // add the node to the corresponding linked list in the table
    pNode->pNextF = p->pBins[Key];
    p->pBins[Key] = pNode;
    p->nEntries++;
    // return NULL, because there is no functional equivalence in this case
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Insert the entry in the functional hash table.]

  Description [Unconditionally add the node to the corresponding 
  linked list in the table.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_HashTableInsertF0( Fraig_Man_t * pMan, Fraig_Node_t * pNode )
{
    Fraig_HashTable_t * p = pMan->pTableF0;
    unsigned Key = pNode->uHashD % p->nBins;

    pNode->pNextF = p->pBins[Key];
    p->pBins[Key] = pNode;
    p->nEntries++;
}


/**Function*************************************************************

  Synopsis    [Resizes the table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_TableResizeS( Fraig_HashTable_t * p )
{
    Fraig_Node_t ** pBinsNew;
    Fraig_Node_t * pEnt, * pEnt2;
    int nBinsNew, Counter, i;
    abctime clk;
    unsigned Key;

clk = Abc_Clock();
    // get the new table size
    nBinsNew = Abc_PrimeCudd(2 * p->nBins); 
    // allocate a new array
    pBinsNew = ABC_ALLOC( Fraig_Node_t *, nBinsNew );
    memset( pBinsNew, 0, sizeof(Fraig_Node_t *) * nBinsNew );
    // rehash the entries from the old table
    Counter = 0;
    for ( i = 0; i < p->nBins; i++ )
        Fraig_TableBinForEachEntrySafeS( p->pBins[i], pEnt, pEnt2 )
        {
            Key = Fraig_HashKey2( pEnt->p1, pEnt->p2, nBinsNew );
            pEnt->pNextS = pBinsNew[Key];
            pBinsNew[Key] = pEnt;
            Counter++;
        }
    assert( Counter == p->nEntries );
//    printf( "Increasing the structural table size from %6d to %6d. ", p->nBins, nBinsNew );
//    ABC_PRT( "Time", Abc_Clock() - clk );
    // replace the table and the parameters
    ABC_FREE( p->pBins );
    p->pBins = pBinsNew;
    p->nBins = nBinsNew;
}

/**Function*************************************************************

  Synopsis    [Resizes the table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_TableResizeF( Fraig_HashTable_t * p, int fUseSimR )
{
    Fraig_Node_t ** pBinsNew;
    Fraig_Node_t * pEnt, * pEnt2;
    int nBinsNew, Counter, i;
    abctime clk;
    unsigned Key;

clk = Abc_Clock();
    // get the new table size
    nBinsNew = Abc_PrimeCudd(2 * p->nBins); 
    // allocate a new array
    pBinsNew = ABC_ALLOC( Fraig_Node_t *, nBinsNew );
    memset( pBinsNew, 0, sizeof(Fraig_Node_t *) * nBinsNew );
    // rehash the entries from the old table
    Counter = 0;
    for ( i = 0; i < p->nBins; i++ )
        Fraig_TableBinForEachEntrySafeF( p->pBins[i], pEnt, pEnt2 )
        {
            if ( fUseSimR )
                Key = pEnt->uHashR % nBinsNew;
            else
                Key = pEnt->uHashD % nBinsNew;
            pEnt->pNextF = pBinsNew[Key];
            pBinsNew[Key] = pEnt;
            Counter++;
        }
    assert( Counter == p->nEntries );
//    printf( "Increasing the functional table size from %6d to %6d. ", p->nBins, nBinsNew );
//    ABC_PRT( "Time", Abc_Clock() - clk );
    // replace the table and the parameters
    ABC_FREE( p->pBins );
    p->pBins = pBinsNew;
    p->nBins = nBinsNew;
}


/**Function*************************************************************

  Synopsis    [Compares two pieces of simulation info.]

  Description [Returns 1 if they are equal.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_CompareSimInfo( Fraig_Node_t * pNode1, Fraig_Node_t * pNode2, int iWordLast, int fUseRand )
{
    int i;
    assert( !Fraig_IsComplement(pNode1) );
    assert( !Fraig_IsComplement(pNode2) );
    if ( fUseRand )
    {
        // if their signatures differ, skip
        if ( pNode1->uHashR != pNode2->uHashR )
            return 0;
        // check the simulation info
        for ( i = 0; i < iWordLast; i++ )
            if ( pNode1->puSimR[i] != pNode2->puSimR[i] )
                return 0;
    }
    else
    {
        // if their signatures differ, skip
        if ( pNode1->uHashD != pNode2->uHashD )
            return 0;
        // check the simulation info
        for ( i = 0; i < iWordLast; i++ )
            if ( pNode1->puSimD[i] != pNode2->puSimD[i] )
                return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Find the number of the different pattern.]

  Description [Returns -1 if there is no such pattern]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_FindFirstDiff( Fraig_Node_t * pNode1, Fraig_Node_t * pNode2, int fCompl, int iWordLast, int fUseRand )
{
    int i, v;
    assert( !Fraig_IsComplement(pNode1) );
    assert( !Fraig_IsComplement(pNode2) );
    // take into account possible internal complementation
    fCompl ^= pNode1->fInv;
    fCompl ^= pNode2->fInv;
    // find the pattern
    if ( fCompl )
    {
        if ( fUseRand )
        {
            for ( i = 0; i < iWordLast; i++ )
                if ( pNode1->puSimR[i] != ~pNode2->puSimR[i] )
                    for ( v = 0; v < 32; v++ )
                        if ( (pNode1->puSimR[i] ^ ~pNode2->puSimR[i]) & (1 << v) )
                            return i * 32 + v;
        }
        else
        {
            for ( i = 0; i < iWordLast; i++ )
                if ( pNode1->puSimD[i] !=  ~pNode2->puSimD[i] )
                    for ( v = 0; v < 32; v++ )
                        if ( (pNode1->puSimD[i] ^ ~pNode2->puSimD[i]) & (1 << v) )
                            return i * 32 + v;
        }
    }
    else
    {
        if ( fUseRand )
        {
            for ( i = 0; i < iWordLast; i++ )
                if ( pNode1->puSimR[i] != pNode2->puSimR[i] )
                    for ( v = 0; v < 32; v++ )
                        if ( (pNode1->puSimR[i] ^ pNode2->puSimR[i]) & (1 << v) )
                            return i * 32 + v;
        }
        else
        {
            for ( i = 0; i < iWordLast; i++ )
                if ( pNode1->puSimD[i] !=  pNode2->puSimD[i] )
                    for ( v = 0; v < 32; v++ )
                        if ( (pNode1->puSimD[i] ^ pNode2->puSimD[i]) & (1 << v) )
                            return i * 32 + v;
        }
    }
    return -1;
}

/**Function*************************************************************

  Synopsis    [Compares two pieces of simulation info.]

  Description [Returns 1 if they are equal.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_CompareSimInfoUnderMask( Fraig_Node_t * pNode1, Fraig_Node_t * pNode2, int iWordLast, int fUseRand, unsigned * puMask )
{
    unsigned * pSims1, * pSims2;
    int i;
    assert( !Fraig_IsComplement(pNode1) );
    assert( !Fraig_IsComplement(pNode2) );
    // get hold of simulation info
    pSims1 = fUseRand? pNode1->puSimR : pNode1->puSimD;
    pSims2 = fUseRand? pNode2->puSimR : pNode2->puSimD;    
    // check the simulation info
    for ( i = 0; i < iWordLast; i++ )
        if ( (pSims1[i] & puMask[i]) != (pSims2[i] & puMask[i]) )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Compares two pieces of simulation info.]

  Description [Returns 1 if they are equal.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_CollectXors( Fraig_Node_t * pNode1, Fraig_Node_t * pNode2, int iWordLast, int fUseRand, unsigned * puMask )
{
    unsigned * pSims1, * pSims2;
    int i;
    assert( !Fraig_IsComplement(pNode1) );
    assert( !Fraig_IsComplement(pNode2) );
    // get hold of simulation info
    pSims1 = fUseRand? pNode1->puSimR : pNode1->puSimD;
    pSims2 = fUseRand? pNode2->puSimR : pNode2->puSimD;    
    // check the simulation info
    for ( i = 0; i < iWordLast; i++ )
        puMask[i] = ( pSims1[i] ^ pSims2[i] );
}


/**Function*************************************************************

  Synopsis    [Prints stats of the structural table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_TablePrintStatsS( Fraig_Man_t * pMan )
{
    Fraig_HashTable_t * pT = pMan->pTableS;
    Fraig_Node_t * pNode;
    int i, Counter;

    printf( "Structural table. Table size = %d. Number of entries = %d.\n", pT->nBins, pT->nEntries );
    for ( i = 0; i < pT->nBins; i++ )
    {
        Counter = 0;
        Fraig_TableBinForEachEntryS( pT->pBins[i], pNode )
            Counter++;
        if ( Counter > 1 )
        {
            printf( "%d ", Counter );
            if ( Counter > 50 )
                printf( "{%d} ", i );
        }
    }
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Prints stats of the structural table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_TablePrintStatsF( Fraig_Man_t * pMan )
{
    Fraig_HashTable_t * pT = pMan->pTableF;
    Fraig_Node_t * pNode;
    int i, Counter;

    printf( "Functional table. Table size = %d. Number of entries = %d.\n", pT->nBins, pT->nEntries );
    for ( i = 0; i < pT->nBins; i++ )
    {
        Counter = 0;
        Fraig_TableBinForEachEntryF( pT->pBins[i], pNode )
            Counter++;
        if ( Counter > 1 )
            printf( "{%d} ", Counter );
    }
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Prints stats of the structural table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_TablePrintStatsF0( Fraig_Man_t * pMan )
{
    Fraig_HashTable_t * pT = pMan->pTableF0;
    Fraig_Node_t * pNode;
    int i, Counter;

    printf( "Zero-node table. Table size = %d. Number of entries = %d.\n", pT->nBins, pT->nEntries );
    for ( i = 0; i < pT->nBins; i++ )
    {
        Counter = 0;
        Fraig_TableBinForEachEntryF( pT->pBins[i], pNode )
            Counter++;
        if ( Counter == 0 )
            continue;
/*
        printf( "\nBin = %4d :  Number of entries = %4d\n", i, Counter );
        Fraig_TableBinForEachEntryF( pT->pBins[i], pNode )
            printf( "Node %5d. Hash = %10d.\n", pNode->Num, pNode->uHashD );
*/
    }
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Rehashes the table after the simulation info has changed.]

  Description [Assumes that the hash values have been updated after performing
  additional simulation. Rehashes the table using the new hash values. 
  Uses pNextF to link the entries in the bins. Uses pNextD to link the entries 
  with identical hash values. Returns 1 if the identical entries have been found.
  Note that identical hash values may mean that the simulation data is different.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_TableRehashF0( Fraig_Man_t * pMan, int fLinkEquiv )
{
    Fraig_HashTable_t * pT = pMan->pTableF0;
    Fraig_Node_t ** pBinsNew;
    Fraig_Node_t * pEntF, * pEntF2, * pEnt, * pEntD2, * pEntN;
    int ReturnValue, Counter, i;
    unsigned Key;

    // allocate a new array of bins
    pBinsNew = ABC_ALLOC( Fraig_Node_t *, pT->nBins );
    memset( pBinsNew, 0, sizeof(Fraig_Node_t *) * pT->nBins );

    // rehash the entries in the table
    // go through all the nodes in the F-lists (and possible in D-lists, if used)
    Counter = 0;
    ReturnValue = 0;
    for ( i = 0; i < pT->nBins; i++ )
        Fraig_TableBinForEachEntrySafeF( pT->pBins[i], pEntF, pEntF2 )
        Fraig_TableBinForEachEntrySafeD( pEntF, pEnt, pEntD2 )
        {
            // decide where to put entry pEnt
            Key = pEnt->uHashD % pT->nBins;
            if ( fLinkEquiv )
            {
                // go through the entries in the new bin
                Fraig_TableBinForEachEntryF( pBinsNew[Key], pEntN )
                {
                    // if they have different values skip
                    if ( pEnt->uHashD != pEntN->uHashD )
                        continue;
                    // they have the same hash value, add pEnt to the D-list pEnt3
                    pEnt->pNextD  = pEntN->pNextD;
                    pEntN->pNextD = pEnt;
                    ReturnValue = 1;
                    Counter++;
                    break;
                }
                if ( pEntN != NULL ) // already linked
                    continue;
                // we did not find equal entry
            }
            // link the new entry
            pEnt->pNextF  = pBinsNew[Key];
            pBinsNew[Key] = pEnt;
            pEnt->pNextD  = NULL;
            Counter++;
        }
    assert( Counter == pT->nEntries );
    // replace the table and the parameters
    ABC_FREE( pT->pBins );
    pT->pBins = pBinsNew;
    return ReturnValue;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

