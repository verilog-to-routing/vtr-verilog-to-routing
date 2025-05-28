/**CFile****************************************************************

  FileName    [ifDec66.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [FPGA mapping based on priority cuts.]

  Synopsis    [Fast checking procedures.]

  Author      [Alessandro Tempia Calvino]
  
  Affiliation [EPFL]

  Date        [Ver. 1.0. Started - Feb 8, 2024.]

  Revision    [$Id: ifDec66.c,v 1.00 2008/02/08 00:00:00 tempia Exp $]

***********************************************************************/

#include "if.h"
#include "bool/kit/kit.h"
#include "misc/vec/vecMem.h"

ABC_NAMESPACE_IMPL_START

#define CLU_VAR_MAX  11
#define CLU_MEM_MAX  1000  // 1 GB
#define CLU_UNUSED   0xff

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// decomposition
typedef struct If_Grp_t_ If_Grp_t;
struct If_Grp_t_
{
    char       nVars;
    char       nMyu;
    char       pVars[CLU_VAR_MAX];
};

// hash table entry
typedef struct If_Hte_t_ If_Hte_t;
struct If_Hte_t_
{
    If_Hte_t * pNext;
    unsigned   Group;
    unsigned   Counter;
    word       pTruth[1];
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

static inline unsigned If_CluGrp2Uns2( If_Grp_t * pG )
{
    char * pChar = (char *)pG;
    unsigned Res = 0;
    int i;
    for ( i = 0; i < 8; i++ )
        Res |= ((pChar[i] & 15) << (i << 2));
    return Res;
}

static inline void If_CluUns2Grp2( unsigned Group, If_Grp_t * pG )
{
    char * pChar = (char *)pG;
    int i;
    for ( i = 0; i < 8; i++ )
        pChar[i] = ((Group >> (i << 2)) & 15);
}

unsigned int If_CluPrimeCudd2( unsigned int p )
{
    int i,pn;

    p--;
    do {
        p++;
        if (p&1) {
        pn = 1;
        i = 3;
        while ((unsigned) (i * i) <= p) {
        if (p % i == 0) {
            pn = 0;
            break;
        }
        i += 2;
        }
    } else {
        pn = 0;
    }
    } while (!pn);
    return(p);

} /* end of Cudd_Prime */

// hash table
static inline int If_CluWordNum2( int nVars )
{
    return nVars <= 6 ? 1 : 1 << (nVars-6);
}

static inline word If_CluAdjust2( word t, int nVars )
{
    assert( nVars >= 0 && nVars <= 6 );
    if ( nVars == 6 )
        return t;
    t &= (((word)1) << (1 << nVars)) - 1;
    if ( nVars == 0 )
        t |= t << (1<<nVars++);
    if ( nVars == 1 )
        t |= t << (1<<nVars++);
    if ( nVars == 2 )
        t |= t << (1<<nVars++);
    if ( nVars == 3 )
        t |= t << (1<<nVars++);
    if ( nVars == 4 )
        t |= t << (1<<nVars++);
    if ( nVars == 5 )
        t |= t << (1<<nVars++);
    return t;
}

int If_CluHashFindMedian2( If_Man_t * p, int t )
{
    If_Hte_t * pEntry;
    Vec_Int_t * vCounters;
    int i, Max = 0, Total = 0, Half = 0;
    vCounters = Vec_IntStart( 1000 );
    for ( i = 0; i < p->nTableSize[t]; i++ )
    {
        for ( pEntry = ((If_Hte_t **)p->pHashTable[t])[i]; pEntry; pEntry = pEntry->pNext )
        {
            if ( Max < (int)pEntry->Counter )
            {
                Max = pEntry->Counter;
                Vec_IntSetEntry( vCounters, pEntry->Counter, 0 );
            }
            Vec_IntAddToEntry( vCounters, pEntry->Counter, 1 );
            Total++;
        }
    }
    for ( i = Max; i > 0; i-- )
    {
        Half += Vec_IntEntry( vCounters, i );
        if ( Half > Total/2 )
            break;
    }
/*
    printf( "total = %d  ", Total );
    printf( "half = %d  ", Half );
    printf( "i = %d  ", i );
    printf( "Max = %d.\n", Max );
*/
    Vec_IntFree( vCounters );
    return Abc_MaxInt( i, 1 );
}

int If_CluHashKey2( word * pTruth, int nWords, int Size )
{
    static unsigned BigPrimes[8] = {12582917, 25165843, 50331653, 100663319, 201326611, 402653189, 805306457, 1610612741};
    unsigned Value = 0;
    int i;
    if ( nWords < 4 )
    {
        unsigned char * s = (unsigned char *)pTruth;
        for ( i = 0; i < 8 * nWords; i++ )
            Value ^= BigPrimes[i % 7] * s[i];
    }
    else
    {
        unsigned * s = (unsigned *)pTruth;
        for ( i = 0; i < 2 * nWords; i++ )
            Value ^= BigPrimes[i % 7] * s[i];
    }
    return Value % Size;
}

unsigned * If_CluHashLookup2( If_Man_t * p, word * pTruth, int t )
{
    If_Hte_t * pEntry, * pPrev;
    int nWords, HashKey;
    if ( p == NULL )
        return NULL;
    nWords = If_CluWordNum2(p->pPars->nLutSize);
    if ( p->pMemEntries == NULL )
        p->pMemEntries = Mem_FixedStart( sizeof(If_Hte_t) + sizeof(word) * (If_CluWordNum2(p->pPars->nLutSize) - 1) );
    if ( p->pHashTable[t] == NULL )
    {
        // decide how large should be the table
        int nEntriesMax1 = 4 * If_CluPrimeCudd2( Vec_PtrSize(p->vObjs) * p->pPars->nCutsMax );
        int nEntriesMax2 = (int)(((double)CLU_MEM_MAX * (1 << 20)) / If_CluWordNum2(p->pPars->nLutSize) / 8);
//        int nEntriesMax2 = 10000;
        // create table
        p->nTableSize[t] = If_CluPrimeCudd2( Abc_MinInt(nEntriesMax1, nEntriesMax2)/2 );
        p->pHashTable[t] = ABC_CALLOC( void *, p->nTableSize[t] );
    }
    // check if this entry exists
    HashKey = If_CluHashKey2( pTruth, nWords, p->nTableSize[t] );
    for ( pEntry = ((If_Hte_t **)p->pHashTable[t])[HashKey]; pEntry; pEntry = pEntry->pNext )
        if ( memcmp(pEntry->pTruth, pTruth, sizeof(word) * nWords) == 0 )
        {
            pEntry->Counter++;
            return &pEntry->Group;
        }
    // resize the hash table
    if ( p->nTableEntries[t] >= 2 * p->nTableSize[t] )
    {
        // collect useful entries
        If_Hte_t * pPrev;
        Vec_Ptr_t * vUseful = Vec_PtrAlloc( p->nTableEntries[t] );
        int i, Median = If_CluHashFindMedian2( p, t );
        for ( i = 0; i < p->nTableSize[t]; i++ )
        {
            for ( pEntry = ((If_Hte_t **)p->pHashTable[t])[i]; pEntry; )
            {
                if ( (int)pEntry->Counter > Median )
                {
                    Vec_PtrPush( vUseful, pEntry );
                    pEntry = pEntry->pNext;
                }
                else
                {
                    pPrev = pEntry->pNext;
                    Mem_FixedEntryRecycle( p->pMemEntries, (char *)pEntry );
                    pEntry = pPrev;
                }
            }
        }
        // add useful entries
        memset( p->pHashTable[t], 0, sizeof(void *) * p->nTableSize[t] );
        Vec_PtrForEachEntry( If_Hte_t *, vUseful, pEntry, i )
        {
            HashKey = If_CluHashKey2( pEntry->pTruth, nWords, p->nTableSize[t] );
            pPrev = ((If_Hte_t **)p->pHashTable[t])[HashKey];
            if ( pPrev == NULL || pEntry->Counter >= pPrev->Counter )
            {
                pEntry->pNext = pPrev;
                ((If_Hte_t **)p->pHashTable[t])[HashKey] = pEntry;
            }
            else
            {
                while ( pPrev->pNext && pEntry->Counter < pPrev->pNext->Counter )
                    pPrev = pPrev->pNext;
                pEntry->pNext = pPrev->pNext;
                pPrev->pNext = pEntry;
            }
        }
        p->nTableEntries[t] = Vec_PtrSize( vUseful );
        Vec_PtrFree( vUseful );
    }
    // create entry
    p->nTableEntries[t]++;
    pEntry = (If_Hte_t *)Mem_FixedEntryFetch( p->pMemEntries );
    memcpy( pEntry->pTruth, pTruth, sizeof(word) * nWords );
    pEntry->Group = CLU_UNUSED;
    pEntry->Counter = 1;
    // insert at the beginning
//    pEntry->pNext = ((If_Hte_t **)p->pHashTable[t])[HashKey];
//    ((If_Hte_t **)p->pHashTable[t])[HashKey] = pEntry;
    // insert at the end
    pEntry->pNext = NULL;
    for ( pPrev = ((If_Hte_t **)p->pHashTable[t])[HashKey]; pPrev && pPrev->pNext; pPrev = pPrev->pNext );
    if ( pPrev == NULL )
        ((If_Hte_t **)p->pHashTable[t])[HashKey] = pEntry;
    else
        pPrev->pNext = pEntry;
    return &pEntry->Group;
}

// returns if successful
int If_CluCheckXX( If_Man_t * p, word * pTruth0, int lutSize, int nVars, int fHashing )
{
    If_Grp_t G1 = {0};
    unsigned * pHashed = NULL;

    if ( p && fHashing )
    {
        pHashed = If_CluHashLookup2( p, pTruth0, 0 );
        if ( pHashed && *pHashed != CLU_UNUSED )
            If_CluUns2Grp2( *pHashed, &G1 );
    }

    /* new entry */
    if ( G1.nVars == 0 )
    {
        G1.nVars = acdXX_evaluate( pTruth0, lutSize, nVars );
    }

    if ( pHashed )
        *pHashed = If_CluGrp2Uns2( &G1 );

    return G1.nVars;
}

// returns the best group found
int If_CluCheckXXExt( void * pMan, word * pTruth, int nVars, int nLutLeaf, int nLutRoot, 
                   char * pLut0, char * pLut1, word * pFunc0, word * pFunc1 )
{
    (void)pMan;
    assert( nLutLeaf == nLutRoot );
    unsigned char result[32];
    int i;

    if ( acdXX_decompose( pTruth, nLutRoot, nVars, result ) )
    {
        /* decomposition failed */
        return 0;
    }

    /* copy LUT bound set */
    unsigned char * pResult = result + 2;
    int Lut1Size = (int) (*pResult++);
    pLut1[0] = Lut1Size;
    pLut1[1] = 0; /* not used */
    for (i = 0; i < Lut1Size; ++i)
    {
        pLut1[2+i] = *pResult++;
    }
    int func_num_bytes = ( Lut1Size <= 3 ) ? 1 : ( 1 << ( Lut1Size - 3 ) );
    *pFunc1 = 0;
    for (i = 0; i < func_num_bytes; ++i)
    {
        *pFunc1 |= ( ( (word) *pResult++ ) & 0xFF ) << 8*i;
    }

    /* copy LUT composition */
    int Lut0Size = (int) (*pResult++);
    pLut0[0] = Lut0Size;
    pLut0[1] = 0; /* not used */
    for (i = 0; i < Lut0Size; ++i)
    {
        pLut0[2+i] = *pResult++;
    }
    func_num_bytes = ( Lut0Size <= 3 ) ? 1 : ( 1 << ( Lut0Size - 3 ) );
    *pFunc0 = 0;
    for (i = 0; i < func_num_bytes; ++i)
    {
        *pFunc0 |= ( ( (word) *pResult++ ) & 0xFF ) << 8*i;
    }

    *pFunc1 = If_CluAdjust2( *pFunc1, Lut1Size );
    *pFunc0 = If_CluAdjust2( *pFunc0, Lut0Size );

    return 1;
}

/**Function*************************************************************

  Synopsis    [Performs ACD into 66 cascade.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_CutPerformCheckXX( If_Man_t * p, unsigned * pTruth0, int nVars, int nLeaves, char * pStr )
{
    unsigned pTruth[IF_MAX_FUNC_LUTSIZE > 5 ? 1 << (IF_MAX_FUNC_LUTSIZE - 5) : 1];
    int Length;
    // stretch the truth table
    memcpy( pTruth, pTruth0, sizeof(word) * Abc_TtWordNum(nVars) );
    Abc_TtStretch6( (word *)pTruth, nLeaves, p->pPars->nLutSize );

    // if cutmin is disabled, minimize the function
    if ( !p->pPars->fCutMin )
        nLeaves = Abc_TtMinBase( (word *)pTruth, NULL, nLeaves, nVars );

    // quit if parameters are wrong
    Length = strlen(pStr);
    if ( Length != 2 )
    {
        printf( "Wrong LUT struct (%s)\n", pStr );
        return 0;
    }

    int lutSize = pStr[0] - '0';
    if ( lutSize < 3 || lutSize > 6 )
    {
        printf( "The LUT size (%d) should belong to {3,4,5,6}.\n", lutSize );
        return 0;
    }

    if ( nLeaves >= 2 * lutSize  )
    {
        printf( "The cut size (%d) is too large for the LUT structure %s.\n", nLeaves, pStr );
        return 0;
    }

    // consider trivial case
    if ( nLeaves <= lutSize )
        return 1;

    return If_CluCheckXX(p, (word*)pTruth, lutSize, nVars, 1);
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END