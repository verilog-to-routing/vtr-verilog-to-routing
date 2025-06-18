/**CFile****************************************************************

  FileName    [ifDec16.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [FPGA mapping based on priority cuts.]

  Synopsis    [Fast checking procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 21, 2006.]

  Revision    [$Id: ifDec16.c,v 1.00 2006/11/21 00:00:00 alanmi Exp $]

***********************************************************************/

#include "if.h"
#include "bool/kit/kit.h"
#include "misc/vec/vecMem.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define CLU_VAR_MAX  16
#define CLU_WRD_MAX  (1 << ((CLU_VAR_MAX)-6))
#define CLU_MEM_MAX  1000  // 1 GB
#define CLU_UNUSED   0xff

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

// variable swapping code
static word PMasks[5][3] = {
    { ABC_CONST(0x9999999999999999), ABC_CONST(0x2222222222222222), ABC_CONST(0x4444444444444444) },
    { ABC_CONST(0xC3C3C3C3C3C3C3C3), ABC_CONST(0x0C0C0C0C0C0C0C0C), ABC_CONST(0x3030303030303030) },
    { ABC_CONST(0xF00FF00FF00FF00F), ABC_CONST(0x00F000F000F000F0), ABC_CONST(0x0F000F000F000F00) },
    { ABC_CONST(0xFF0000FFFF0000FF), ABC_CONST(0x0000FF000000FF00), ABC_CONST(0x00FF000000FF0000) },
    { ABC_CONST(0xFFFF00000000FFFF), ABC_CONST(0x00000000FFFF0000), ABC_CONST(0x0000FFFF00000000) }
};
// elementary truth tables
static word Truth6[6] = {
    ABC_CONST(0xAAAAAAAAAAAAAAAA),
    ABC_CONST(0xCCCCCCCCCCCCCCCC),
    ABC_CONST(0xF0F0F0F0F0F0F0F0),
    ABC_CONST(0xFF00FF00FF00FF00),
    ABC_CONST(0xFFFF0000FFFF0000),
    ABC_CONST(0xFFFFFFFF00000000)
};
static word TruthAll[CLU_VAR_MAX][CLU_WRD_MAX] = {{0}};

extern void Kit_DsdPrintFromTruth( unsigned * pTruth, int nVars );
extern void Extra_PrintBinary( FILE * pFile, unsigned Sign[], int nBits );

extern int If_CluSupportSize( word * t, int nVars );

int s_Count2 = 0;
int s_Count3 = 0;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

static inline unsigned If_CluGrp2Uns( If_Grp_t * pG )
{
    char * pChar = (char *)pG;
    unsigned Res = 0;
    int i;
    for ( i = 0; i < 8; i++ )
        Res |= ((pChar[i] & 15) << (i << 2));
    return Res;
}

static inline void If_CluUns2Grp( unsigned Group, If_Grp_t * pG )
{
    char * pChar = (char *)pG;
    int i;
    for ( i = 0; i < 8; i++ )
        pChar[i] = ((Group >> (i << 2)) & 15);
}

unsigned int If_CluPrimeCudd( unsigned int p )
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
static inline int If_CluWordNum( int nVars )
{
    return nVars <= 6 ? 1 : 1 << (nVars-6);
}
static inline int If_CluCountOnes( word t )
{
    t =    (t & ABC_CONST(0x5555555555555555)) + ((t>> 1) & ABC_CONST(0x5555555555555555));
    t =    (t & ABC_CONST(0x3333333333333333)) + ((t>> 2) & ABC_CONST(0x3333333333333333));
    t =    (t & ABC_CONST(0x0F0F0F0F0F0F0F0F)) + ((t>> 4) & ABC_CONST(0x0F0F0F0F0F0F0F0F));
    t =    (t & ABC_CONST(0x00FF00FF00FF00FF)) + ((t>> 8) & ABC_CONST(0x00FF00FF00FF00FF));
    t =    (t & ABC_CONST(0x0000FFFF0000FFFF)) + ((t>>16) & ABC_CONST(0x0000FFFF0000FFFF));
    return (t & ABC_CONST(0x00000000FFFFFFFF)) +  (t>>32);
}

void If_CluHashTableCheck( If_Man_t * p )
{
    int t = 1;
    If_Hte_t * pEntry;
    int i, RetValue, Status;
    for ( i = 0; i < p->nTableSize[t]; i++ )
    {
        for ( pEntry = ((If_Hte_t **)p->pHashTable[t])[i]; pEntry; pEntry = pEntry->pNext )
        {        
            Status = ((pEntry->Group & 15) > 0);
            RetValue = If_CutPerformCheck16( NULL, (unsigned *)pEntry->pTruth, 13, If_CluSupportSize(pEntry->pTruth, 13), "555" );
            if ( RetValue != Status ) 
            {
                Kit_DsdPrintFromTruth( (unsigned*)pEntry->pTruth, 13 ); printf( "\n" );
                RetValue = If_CutPerformCheck16( NULL, (unsigned *)pEntry->pTruth, 13, If_CluSupportSize(pEntry->pTruth, 13), "555" );
                printf( "Hash table problem!!!\n" );
            }
        }
    }
}
void If_CluHashPrintStats( If_Man_t * p, int t )
{
    If_Hte_t * pEntry;
    int i, Counter;
    for ( i = 0; i < p->nTableSize[t]; i++ )
    {
        Counter = 0;
        for ( pEntry = ((If_Hte_t **)p->pHashTable[t])[i]; pEntry; pEntry = pEntry->pNext )
            Counter++;
        if ( Counter == 0 )
            continue;
        if ( Counter < 10 )
            continue;
        printf( "%d=%d ", i, Counter );
    }
}
int If_CluHashFindMedian( If_Man_t * p, int t )
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
int If_CluHashKey( word * pTruth, int nWords, int Size )
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
unsigned * If_CluHashLookup( If_Man_t * p, word * pTruth, int t )
{
    If_Hte_t * pEntry, * pPrev;
    int nWords, HashKey;
    if ( p == NULL )
        return NULL;
    nWords = If_CluWordNum(p->pPars->nLutSize);
    if ( p->pMemEntries == NULL )
        p->pMemEntries = Mem_FixedStart( sizeof(If_Hte_t) + sizeof(word) * (If_CluWordNum(p->pPars->nLutSize) - 1) );
    if ( p->pHashTable[t] == NULL )
    {
        // decide how large should be the table
        int nEntriesMax1 = 4 * If_CluPrimeCudd( Vec_PtrSize(p->vObjs) * p->pPars->nCutsMax );
        int nEntriesMax2 = (int)(((double)CLU_MEM_MAX * (1 << 20)) / If_CluWordNum(p->pPars->nLutSize) / 8);
//        int nEntriesMax2 = 10000;
        // create table
        p->nTableSize[t] = If_CluPrimeCudd( Abc_MinInt(nEntriesMax1, nEntriesMax2)/2 );
        p->pHashTable[t] = ABC_CALLOC( void *, p->nTableSize[t] );
    }
    // check if this entry exists
    HashKey = If_CluHashKey( pTruth, nWords, p->nTableSize[t] );
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
        int i, Median = If_CluHashFindMedian( p, t );
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
            HashKey = If_CluHashKey( pEntry->pTruth, nWords, p->nTableSize[t] );
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

// variable permutation for large functions
static inline void If_CluClear( word * pIn, int nVars )
{
    int w, nWords = If_CluWordNum( nVars );
    for ( w = 0; w < nWords; w++ )
        pIn[w] = 0;
}
static inline void If_CluFill( word * pIn, int nVars )
{
    int w, nWords = If_CluWordNum( nVars );
    for ( w = 0; w < nWords; w++ )
        pIn[w] = ~(word)0;
}
static inline void If_CluCopy( word * pOut, word * pIn, int nVars )
{
    int w, nWords = If_CluWordNum( nVars );
    for ( w = 0; w < nWords; w++ )
        pOut[w] = pIn[w];
}
static inline int If_CluEqual( word * pOut, word * pIn, int nVars )
{
    int w, nWords = If_CluWordNum( nVars );
    for ( w = 0; w < nWords; w++ )
        if ( pOut[w] != pIn[w] )
            return 0;
    return 1;
}
static inline void If_CluAnd( word * pRes, word * pIn1, word * pIn2, int nVars )
{
    int w, nWords = If_CluWordNum( nVars );
    for ( w = 0; w < nWords; w++ )
        pRes[w] = pIn1[w] & pIn2[w];
}
static inline void If_CluSharp( word * pRes, word * pIn1, word * pIn2, int nVars )
{
    int w, nWords = If_CluWordNum( nVars );
    for ( w = 0; w < nWords; w++ )
        pRes[w] = pIn1[w] & ~pIn2[w];
} 
static inline void If_CluOr( word * pRes, word * pIn1, word * pIn2, int nVars )
{
    int w, nWords = If_CluWordNum( nVars );
    for ( w = 0; w < nWords; w++ )
        pRes[w] = pIn1[w] | pIn2[w];
}
static inline word If_CluAdjust( word t, int nVars )
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
static inline void If_CluAdjustBig( word * pF, int nVarsCur, int nVarsMax )
{
    int v, nWords;
    if ( nVarsCur == nVarsMax )
        return;
    assert( nVarsCur < nVarsMax );
    for ( v = Abc_MaxInt( nVarsCur, 6 ); v < nVarsMax; v++ )
    {
        nWords = If_CluWordNum( v );
        If_CluCopy( pF + nWords, pF, v );
    }
}
static inline void If_CluSwapAdjacent( word * pOut, word * pIn, int iVar, int nVars )
{
    int i, k, nWords = If_CluWordNum( nVars );
    assert( iVar < nVars - 1 );
    if ( iVar < 5 )
    {
        int Shift = (1 << iVar);
        for ( i = 0; i < nWords; i++ )
            pOut[i] = (pIn[i] & PMasks[iVar][0]) | ((pIn[i] & PMasks[iVar][1]) << Shift) | ((pIn[i] & PMasks[iVar][2]) >> Shift);
    }
    else if ( iVar > 5 )
    {
        int Step = (1 << (iVar - 6));
        for ( k = 0; k < nWords; k += 4*Step )
        {
            for ( i = 0; i < Step; i++ )
                pOut[i] = pIn[i];
            for ( i = 0; i < Step; i++ )
                pOut[Step+i] = pIn[2*Step+i];
            for ( i = 0; i < Step; i++ )
                pOut[2*Step+i] = pIn[Step+i];
            for ( i = 0; i < Step; i++ )
                pOut[3*Step+i] = pIn[3*Step+i];
            pIn  += 4*Step;
            pOut += 4*Step;
        }
    }
    else // if ( iVar == 5 )
    {
        for ( i = 0; i < nWords; i += 2 )
        {
            pOut[i]   = (pIn[i]   & ABC_CONST(0x00000000FFFFFFFF)) | ((pIn[i+1] & ABC_CONST(0x00000000FFFFFFFF)) << 32);
            pOut[i+1] = (pIn[i+1] & ABC_CONST(0xFFFFFFFF00000000)) | ((pIn[i]   & ABC_CONST(0xFFFFFFFF00000000)) >> 32);
        }
    }
}

void If_CluChangePhase( word * pF, int nVars, int iVar )
{
    int nWords = If_CluWordNum( nVars );
    assert( iVar < nVars );
    if ( iVar < 6 )
    {
        int i, Shift = (1 << iVar);
        for ( i = 0; i < nWords; i++ )
            pF[i] = ((pF[i] & ~Truth6[iVar]) << Shift) | ((pF[i] & Truth6[iVar]) >> Shift);
    }
    else
    {
        word Temp;
        int i, k, Step = (1 << (iVar - 6));
        for ( k = 0; k < nWords; k += 2*Step )
        {
            for ( i = 0; i < Step; i++ )
            {
                Temp = pF[i];
                pF[i] = pF[Step+i];
                pF[Step+i] = Temp;
            }
            pF += 2*Step;
        }
    }
}
void If_CluCountOnesInCofs( word * pTruth, int nVars, int * pStore )
{
    int nWords = If_CluWordNum( nVars );
    int i, k, nOnes = 0, Limit = Abc_MinInt( nVars, 6 );
    memset( pStore, 0, sizeof(int) * 2 * nVars );
    // compute positive cofactors
    for ( k = 0; k < nWords; k++ )
        for ( i = 0; i < Limit; i++ )
            pStore[2*i+1] += If_CluCountOnes( pTruth[k] & Truth6[i] );
    if ( nVars > 6 )
    for ( k = 0; k < nWords; k++ )
        for ( i = 6; i < nVars; i++ )
            if ( k & (1 << (i-6)) )
                pStore[2*i+1] += If_CluCountOnes( pTruth[k] );
    // compute negative cofactors
    for ( k = 0; k < nWords; k++ )
        nOnes += If_CluCountOnes( pTruth[k] );
    for ( i = 0; i < nVars; i++ )
        pStore[2*i] = nOnes - pStore[2*i+1];
}
unsigned If_CluSemiCanonicize( word * pTruth, int nVars, int * pCanonPerm )
{
    word pFunc[CLU_WRD_MAX], * pIn = pTruth, * pOut = pFunc, * pTemp;
    int pStore[CLU_VAR_MAX*2];
    unsigned uCanonPhase = 0;
    int i, Temp, fChange, Counter = 0;
//Kit_DsdPrintFromTruth( (unsigned*)pTruth, nVars ); printf( "\n" );

    // collect signatures 
    If_CluCountOnesInCofs( pTruth, nVars, pStore );
    // canonicize phase
    for ( i = 0; i < nVars; i++ )
    {
        if ( pStore[2*i+0] <= pStore[2*i+1] )
            continue;
        uCanonPhase |= (1 << i);
        Temp = pStore[2*i+0];
        pStore[2*i+0] = pStore[2*i+1];
        pStore[2*i+1] = Temp;
        If_CluChangePhase( pIn, nVars, i );
    }
    // compute permutation
    for ( i = 0; i < nVars; i++ )
        pCanonPerm[i] = i;
    do {
        fChange = 0;
        for ( i = 0; i < nVars-1; i++ )
        {
            if ( pStore[2*i] <= pStore[2*(i+1)] )
                continue;
            Counter++;
            fChange = 1;

            Temp = pCanonPerm[i];
            pCanonPerm[i] = pCanonPerm[i+1];
            pCanonPerm[i+1] = Temp;

            Temp = pStore[2*i];
            pStore[2*i] = pStore[2*(i+1)];
            pStore[2*(i+1)] = Temp;

            Temp = pStore[2*i+1];
            pStore[2*i+1] = pStore[2*(i+1)+1];
            pStore[2*(i+1)+1] = Temp;

            If_CluSwapAdjacent( pOut, pIn, i, nVars );
            pTemp = pIn; pIn = pOut; pOut = pTemp;
        }
    } while ( fChange );
    // swap if it was moved an odd number of times
    if ( Counter & 1 )
        If_CluCopy( pOut, pIn, nVars );
    return uCanonPhase;
}
void If_CluSemiCanonicizeVerify( word * pTruth, word * pTruth0, int nVars, int * pCanonPerm, unsigned uCanonPhase )
{
    word pFunc[CLU_WRD_MAX], pGunc[CLU_WRD_MAX], * pIn = pTruth, * pOut = pFunc, * pTemp;
    int i, Temp, fChange, Counter = 0;
    If_CluCopy( pGunc, pTruth, nVars );
    // undo permutation
    do {
        fChange = 0;
        for ( i = 0; i < nVars-1; i++ )
        {
            if ( pCanonPerm[i] < pCanonPerm[i+1] )
                continue;

            Counter++;
            fChange = 1;

            Temp = pCanonPerm[i];
            pCanonPerm[i] = pCanonPerm[i+1];
            pCanonPerm[i+1] = Temp;

            If_CluSwapAdjacent( pOut, pIn, i, nVars );
            pTemp = pIn; pIn = pOut; pOut = pTemp;
        }
    } while ( fChange );
    if ( Counter & 1 )
        If_CluCopy( pOut, pIn, nVars );
    // undo phase
    for ( i = 0; i < nVars; i++ )
        if ( (uCanonPhase >> i) & 1 )
            If_CluChangePhase( pTruth, nVars, i );
    // compare
    if ( !If_CluEqual(pTruth0, pTruth, nVars) )
    {
        Kit_DsdPrintFromTruth( (unsigned*)pTruth0, nVars ); printf( "\n" );
        Kit_DsdPrintFromTruth( (unsigned*)pGunc, nVars ); printf( "\n" );
        Kit_DsdPrintFromTruth( (unsigned*)pTruth, nVars ); printf( "\n" );
        printf( "SemiCanonical verification FAILED!\n" );
    }
}


void If_CluPrintGroup( If_Grp_t * g )
{
    int i;
    printf( "Vars = %d   ", g->nVars );
    printf( "Myu = %d   {", g->nMyu );
    for ( i = 0; i < g->nVars; i++ )
        printf( " %c", 'a' + g->pVars[i] );
    printf( " }\n" );
}

void If_CluPrintConfig( int nVars, If_Grp_t * g, If_Grp_t * r, word BStruth, word * pFStruth )
{
    assert( r->nVars == nVars - g->nVars + 1 + (g->nMyu > 2) );
    If_CluPrintGroup( g );
    if ( g->nVars < 6 )
        BStruth = If_CluAdjust( BStruth, g->nVars );
    Kit_DsdPrintFromTruth( (unsigned *)&BStruth, g->nVars );
    printf( "\n" );
    If_CluPrintGroup( r );
    if ( r->nVars < 6 )
        pFStruth[0] = If_CluAdjust( pFStruth[0], r->nVars );
    Kit_DsdPrintFromTruth( (unsigned *)pFStruth, r->nVars );
    printf( "\n" );
}


void If_CluInitTruthTables()
{
    int i, k;
    assert( CLU_VAR_MAX <= 16 );
    for ( i = 0; i < 6; i++ )
        for ( k = 0; k < CLU_WRD_MAX; k++ )
            TruthAll[i][k] = Truth6[i];
    for ( i = 6; i < CLU_VAR_MAX; i++ )
        for ( k = 0; k < CLU_WRD_MAX; k++ )
            TruthAll[i][k] = ((k >> (i-6)) & 1) ? ~(word)0 : 0;

//    Extra_PrintHex( stdout, TruthAll[6], 8 ); printf( "\n" );
//    Extra_PrintHex( stdout, TruthAll[7], 8 ); printf( "\n" );
}


// verification
static void If_CluComposeLut( int nVars, If_Grp_t * g, word * t, word f[6][CLU_WRD_MAX], word * r )
{
    word c[CLU_WRD_MAX];
    int m, v;
    If_CluClear( r, nVars ); 
    for ( m = 0; m < (1<<g->nVars); m++ )
    {
        if ( !((t[m >> 6] >> (m & 63)) & 1) )
            continue;
        If_CluFill( c, nVars );
        for ( v = 0; v < g->nVars; v++ )
            if ( (m >> v) & 1 )
                If_CluAnd( c, c, f[v], nVars );
            else
                If_CluSharp( c, c, f[v], nVars );
        If_CluOr( r, r, c, nVars );
    }
}
void If_CluVerify( word * pF, int nVars, If_Grp_t * g, If_Grp_t * r, word BStruth, word * pFStruth )
{
    word pTTFans[6][CLU_WRD_MAX], pTTWire[CLU_WRD_MAX], pTTRes[CLU_WRD_MAX];
    int i;
    assert( g->nVars <= 6 && r->nVars <= 6 );

    if ( TruthAll[0][0] == 0 )
        If_CluInitTruthTables();

    for ( i = 0; i < g->nVars; i++ )
        If_CluCopy( pTTFans[i], TruthAll[(int)g->pVars[i]], nVars );
    If_CluComposeLut( nVars, g, &BStruth, pTTFans, pTTWire );

    for ( i = 0; i < r->nVars; i++ )
        if ( r->pVars[i] == nVars )
            If_CluCopy( pTTFans[i], pTTWire, nVars );
        else
            If_CluCopy( pTTFans[i], TruthAll[(int)r->pVars[i]], nVars );
    If_CluComposeLut( nVars, r, pFStruth, pTTFans, pTTRes );

    if ( !If_CluEqual(pTTRes, pF, nVars) )
    {
        printf( "\n" );
        If_CluPrintConfig( nVars, g, r, BStruth, pFStruth );
        Kit_DsdPrintFromTruth( (unsigned*)pTTRes, nVars ); printf( "\n" );
        Kit_DsdPrintFromTruth( (unsigned*)pF, nVars ); printf( "\n" );
//        Extra_PrintHex( stdout, (unsigned *)pF, nVars ); printf( "\n" );
        printf( "Verification FAILED!\n" );
    }
//    else
//        printf( "Verification succeed!\n" );
}
void If_CluVerify3( word * pF, int nVars, If_Grp_t * g, If_Grp_t * g2, If_Grp_t * r, word BStruth, word BStruth2, word FStruth )
{
    word pTTFans[6][CLU_WRD_MAX], pTTWire[CLU_WRD_MAX], pTTWire2[CLU_WRD_MAX], pTTRes[CLU_WRD_MAX];
    int i;
    assert( g->nVars >= 2 && g2->nVars >= 2 && r->nVars >= 2 );
    assert( g->nVars <= 6 && g2->nVars <= 6 && r->nVars <= 6 );

    if ( TruthAll[0][0] == 0 )
        If_CluInitTruthTables();

    for ( i = 0; i < g->nVars; i++ )
        If_CluCopy( pTTFans[i], TruthAll[(int)g->pVars[i]], nVars );
    If_CluComposeLut( nVars, g, &BStruth, pTTFans, pTTWire );

    for ( i = 0; i < g2->nVars; i++ )
        If_CluCopy( pTTFans[i], TruthAll[(int)g2->pVars[i]], nVars );
    If_CluComposeLut( nVars, g2, &BStruth2, pTTFans, pTTWire2 );

    for ( i = 0; i < r->nVars; i++ )
        if ( r->pVars[i] == nVars )
            If_CluCopy( pTTFans[i], pTTWire, nVars );
        else if ( r->pVars[i] == nVars + 1 )
            If_CluCopy( pTTFans[i], pTTWire2, nVars );
        else
            If_CluCopy( pTTFans[i], TruthAll[(int)r->pVars[i]], nVars );
    If_CluComposeLut( nVars, r, &FStruth, pTTFans, pTTRes );

    if ( !If_CluEqual(pTTRes, pF, nVars) )
    {
        printf( "%d\n", nVars );
//        If_CluPrintConfig( nVars, g, r, BStruth, pFStruth );
//        Extra_PrintHex( stdout, (unsigned *)pF, nVars ); printf( "\n" );

        Kit_DsdPrintFromTruth( (unsigned*)&BStruth, g->nVars );   printf( "    " ); If_CluPrintGroup(g);  // printf( "\n" );
        Kit_DsdPrintFromTruth( (unsigned*)&BStruth2, g2->nVars ); printf( "    " ); If_CluPrintGroup(g2); // printf( "\n" );
        Kit_DsdPrintFromTruth( (unsigned*)&FStruth, r->nVars );   printf( "    " ); If_CluPrintGroup(r);  // printf( "\n" );

        Kit_DsdPrintFromTruth( (unsigned*)pTTWire, nVars ); printf( "\n" );
        Kit_DsdPrintFromTruth( (unsigned*)pTTWire2, nVars ); printf( "\n" );
        Kit_DsdPrintFromTruth( (unsigned*)pTTRes, nVars ); printf( "\n" );
        Kit_DsdPrintFromTruth( (unsigned*)pF, nVars ); printf( "\n" );
//        Extra_PrintHex( stdout, (unsigned *)pF, nVars ); printf( "\n" );
        printf( "Verification FAILED!\n" );
    }
//    else
//        printf( "Verification succeed!\n" );
}



void If_CluSwapVars( word * pTruth, int nVars, int * V2P, int * P2V, int iVar, int jVar )
{
    word low2High, high2Low, temp;
    int nWords = If_CluWordNum(nVars);
    int shift, step, iStep, jStep;
    int w = 0, i = 0, j = 0;
    static word PPMasks[6][6] = {
        { ABC_CONST(0x2222222222222222), ABC_CONST(0x0A0A0A0A0A0A0A0A), ABC_CONST(0x00AA00AA00AA00AA), ABC_CONST(0x0000AAAA0000AAAA), ABC_CONST(0x00000000AAAAAAAA), ABC_CONST(0xAAAAAAAAAAAAAAAA) },
        { ABC_CONST(0x0000000000000000), ABC_CONST(0x0C0C0C0C0C0C0C0C), ABC_CONST(0x00CC00CC00CC00CC), ABC_CONST(0x0000CCCC0000CCCC), ABC_CONST(0x00000000CCCCCCCC), ABC_CONST(0xCCCCCCCCCCCCCCCC) },
        { ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000), ABC_CONST(0x00F000F000F000F0), ABC_CONST(0x0000F0F00000F0F0), ABC_CONST(0x00000000F0F0F0F0), ABC_CONST(0xF0F0F0F0F0F0F0F0) },
        { ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000), ABC_CONST(0x0000FF000000FF00), ABC_CONST(0x00000000FF00FF00), ABC_CONST(0xFF00FF00FF00FF00) },
        { ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000), ABC_CONST(0x00000000FFFF0000), ABC_CONST(0xFFFF0000FFFF0000) },
        { ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000), ABC_CONST(0x0000000000000000), ABC_CONST(0xFFFFFFFF00000000) }
    };
    if( iVar == jVar )
        return;
    if( jVar < iVar )
    {
        int varTemp = jVar;
        jVar = iVar;
        iVar = varTemp;
    }
    if ( iVar <= 5 && jVar <= 5 )
    {
        shift = (1 <<  jVar) - (1 << iVar);
        for ( w = 0; w < nWords; w++ )
        {
            low2High = (pTruth[w] & PPMasks[iVar][jVar - 1] ) << shift;
            pTruth[w] &= ~PPMasks[iVar][jVar - 1];
            high2Low = (pTruth[w] & (PPMasks[iVar][jVar - 1] << shift )) >> shift;
            pTruth[w] &= ~ (PPMasks[iVar][jVar - 1] << shift);
            pTruth[w] = pTruth[w] | low2High | high2Low;
        }
    }
    else if( iVar <= 5 && jVar > 5 )
    {
        step = If_CluWordNum(jVar + 1)/2;
        shift = 1 << iVar;
        for ( w = 0; w < nWords; w += 2*step )
        {
            for (j = 0; j < step; j++)
            {
                low2High = (pTruth[w + j] & PPMasks[iVar][5]) >> shift;
                pTruth[w + j] &= ~PPMasks[iVar][5];
                high2Low = (pTruth[w + step + j] & (PPMasks[iVar][5] >> shift)) << shift;
                pTruth[w + step + j] &= ~(PPMasks[iVar][5] >> shift);
                pTruth[w + j] |= high2Low;
                pTruth[w + step + j] |= low2High;            
            }
        }
    }
    else
    {
        iStep = If_CluWordNum(iVar + 1)/2;
        jStep = If_CluWordNum(jVar + 1)/2;
        for (w = 0; w < nWords; w += 2*jStep)
        {
            for (i = 0; i < jStep; i += 2*iStep)
            {
                for (j = 0; j < iStep; j++)
                {
                    temp = pTruth[w + iStep + i + j];
                    pTruth[w + iStep + i + j] = pTruth[w + jStep + i + j];
                    pTruth[w + jStep + i + j] = temp;
                }
            }
        }
    }    
    if ( V2P && P2V )
    {
        V2P[P2V[iVar]] = jVar;
        V2P[P2V[jVar]] = iVar;
        P2V[iVar] ^= P2V[jVar];
        P2V[jVar] ^= P2V[iVar];
        P2V[iVar] ^= P2V[jVar];
    }
}
void If_CluReverseOrder( word * pTruth, int nVars, int * V2P, int * P2V, int iVarStart )
{
    int i, j, k;
    for ( k = 0; k < (nVars-iVarStart)/2 ; k++ )
    {
        i = iVarStart + k;
        j = nVars - 1 - k;
        If_CluSwapVars( pTruth, nVars, V2P, P2V, i, j );
    }
}

// moves one var (v) to the given position (p)
void If_CluMoveVar2( word * pF, int nVars, int * Var2Pla, int * Pla2Var, int v, int p )
{
    If_CluSwapVars( pF, nVars, Var2Pla, Pla2Var, Var2Pla[v], p );
}

// moves one var (v) to the given position (p)
void If_CluMoveVar( word * pF, int nVars, int * Var2Pla, int * Pla2Var, int v, int p )
{
    word pG[CLU_WRD_MAX], * pIn = pF, * pOut = pG, * pTemp;
    int iPlace0, iPlace1, Count = 0;
    assert( v >= 0 && v < nVars );
    while ( Var2Pla[v] < p )
    {
        iPlace0 = Var2Pla[v];
        iPlace1 = Var2Pla[v]+1;
        If_CluSwapAdjacent( pOut, pIn, iPlace0, nVars );
        pTemp = pIn; pIn = pOut, pOut = pTemp;
        Var2Pla[Pla2Var[iPlace0]]++;
        Var2Pla[Pla2Var[iPlace1]]--;
        Pla2Var[iPlace0] ^= Pla2Var[iPlace1];
        Pla2Var[iPlace1] ^= Pla2Var[iPlace0];
        Pla2Var[iPlace0] ^= Pla2Var[iPlace1];
        Count++;
    }
    while ( Var2Pla[v] > p )
    {
        iPlace0 = Var2Pla[v]-1;
        iPlace1 = Var2Pla[v];
        If_CluSwapAdjacent( pOut, pIn, iPlace0, nVars );
        pTemp = pIn; pIn = pOut, pOut = pTemp;
        Var2Pla[Pla2Var[iPlace0]]++;
        Var2Pla[Pla2Var[iPlace1]]--;
        Pla2Var[iPlace0] ^= Pla2Var[iPlace1];
        Pla2Var[iPlace1] ^= Pla2Var[iPlace0];
        Pla2Var[iPlace0] ^= Pla2Var[iPlace1];
        Count++;
    }
    if ( Count & 1 )
        If_CluCopy( pF, pIn, nVars );
    assert( Pla2Var[p] == v );
}

// moves vars to be the most signiticant ones (Group[0] is MSB)
void If_CluMoveGroupToMsb( word * pF, int nVars, int * V2P, int * P2V, If_Grp_t * g )
{
    int v;
    for ( v = 0; v < g->nVars; v++ )
        If_CluMoveVar( pF, nVars, V2P, P2V, g->pVars[g->nVars - 1 - v], nVars - 1 - v );
}


// reverses the variable order
void If_CluReverseOrder_old( word * pF, int nVars, int * V2P, int * P2V, int iVarStart )
{
    word pG[CLU_WRD_MAX];
    int v;

    If_CluCopy( pG, pF, nVars );

//    for ( v = 0; v < nVars; v++ )
//        printf( "%c ", 'a' + P2V[v] );
//    printf( "  ---  " );

    for ( v = iVarStart; v < nVars; v++ )
        If_CluMoveVar( pF, nVars, V2P, P2V, P2V[iVarStart], nVars - 1 - (v - iVarStart) );

//    for ( v = 0; v < nVars; v++ )
//        printf( "%c ", 'a' + P2V[v] );
//    printf( "\n" );

//    if ( iVarStart > 0 )
//        return;

    If_CluReverseOrder( pG, nVars, NULL, NULL, iVarStart );
    if ( If_CluEqual( pG, pF, nVars ) )
    {
//        printf( "+" );
    }
    else
    {
/*
        printf( "\n" );
        Kit_DsdPrintFromTruth( (unsigned*)pF, nVars ); printf( "\n" );
        Kit_DsdPrintFromTruth( (unsigned*)pG, nVars ); 
        printf( "\n" );
*/
        printf( "%d ", nVars );
    }
}

// return the number of cofactors w.r.t. the topmost vars (nBSsize)
int If_CluCountCofs( word * pF, int nVars, int nBSsize, int iShift, word pCofs[3][CLU_WRD_MAX/4] )
{
    word iCofs[128] = {0}, iCof, Result = 0;
    word * pCofA, * pCofB;
    int nMints = (1 << nBSsize);
    int i, c, w, nCofs;
    assert( nBSsize >= 2 && nBSsize <= 7 && nBSsize < nVars );
    if ( nVars - nBSsize < 6 )
    {
        int nShift = (1 << (nVars - nBSsize));
        word Mask  = ((((word)1) << nShift) - 1);
        for ( nCofs = i = 0; i < nMints; i++ )
        {
            iCof = (pF[(iShift + i * nShift) / 64] >> ((iShift + i * nShift) & 63)) & Mask;
            for ( c = 0; c < nCofs; c++ )
                if ( iCof == iCofs[c] )
                    break;
            if ( c == nCofs )
                iCofs[nCofs++] = iCof;
            if ( pCofs && iCof != iCofs[0] )
                Result |= (((word)1) << i);
            if ( nCofs == 5 )
                break;
        }
        if ( nCofs <= 2 && pCofs )
        {
            assert( nBSsize <= 6 );
            pCofs[0][0] = iCofs[0];
            pCofs[1][0] = (nCofs == 2) ? iCofs[1] : iCofs[0];
            pCofs[2][0] = Result;
        }
    }
    else
    {
        int nWords = If_CluWordNum( nVars - nBSsize );
        assert( nWords * nMints == If_CluWordNum(nVars) );
        for ( nCofs = i = 0; i < nMints; i++ )
        {
            pCofA = pF + i * nWords;
            for ( c = 0; c < nCofs; c++ )
            {
                pCofB = pF + iCofs[c] * nWords;
                for ( w = 0; w < nWords; w++ )
                    if ( pCofA[w] != pCofB[w] )
                        break;
                if ( w == nWords )
                    break;
            }
            if ( c == nCofs )
                iCofs[nCofs++] = i;
            if ( pCofs )
            {
                assert( nBSsize <= 6 );
                pCofB = pF + iCofs[0] * nWords;
                for ( w = 0; w < nWords; w++ )
                    if ( pCofA[w] != pCofB[w] )
                        break;
                if ( w != nWords )
                    Result |= (((word)1) << i);
            }
            if ( nCofs == 5 )
                break;
        }
        if ( nCofs <= 2 && pCofs )
        {
            If_CluCopy( pCofs[0], pF + iCofs[0] * nWords, nVars - nBSsize );
            If_CluCopy( pCofs[1], pF + ((nCofs == 2) ? iCofs[1] : iCofs[0]) * nWords, nVars - nBSsize );
            pCofs[2][0] = Result;
        }
    }
    assert( nCofs >= 1 && nCofs <= 5 );
    return nCofs;
}

// return the number of cofactors w.r.t. the topmost vars (nBSsize)
int If_CluCountCofs4( word * pF, int nVars, int nBSsize, word pCofs[6][CLU_WRD_MAX/4] )
{
    word iCofs[128] = {0}, iCof, Result0 = 0, Result1 = 0;
    int nMints = (1 << nBSsize);
    int i, c, nCofs = 0;
    assert( pCofs );
    assert( nBSsize >= 2 && nBSsize <= 6 && nBSsize < nVars );
    if ( nVars - nBSsize < 6 )
    {
        int nShift = (1 << (nVars - nBSsize));
        word Mask  = ((((word)1) << nShift) - 1);
        for ( nCofs = i = 0; i < nMints; i++ )
        {
            iCof = (pF[(i * nShift) / 64] >> ((i * nShift) & 63)) & Mask;
            for ( c = 0; c < nCofs; c++ )
                if ( iCof == iCofs[c] )
                    break;
            if ( c == nCofs )
                iCofs[nCofs++] = iCof;
            if ( c == 1 || c == 3 )
                Result0 |= (((word)1) << i);
            if ( c == 2 || c == 3 )
                Result1 |= (((word)1) << i);
        }
        assert( nCofs >= 3 && nCofs <= 4 );
        pCofs[0][0] = iCofs[0];
        pCofs[1][0] = iCofs[1];
        pCofs[2][0] = iCofs[2];
        pCofs[3][0] = (nCofs == 4) ? iCofs[3] : iCofs[2];
        pCofs[4][0] = Result0;
        pCofs[5][0] = Result1;
    }
    else
        assert( 0 );
    return nCofs;
}

void If_CluCofactors( word * pF, int nVars, int iVar, word * pCof0, word * pCof1 )
{
    int nWords = If_CluWordNum( nVars );
    assert( iVar < nVars );
    if ( iVar < 6 )
    {
        int i, Shift = (1 << iVar);
        for ( i = 0; i < nWords; i++ )
        {
            pCof0[i] = (pF[i] & ~Truth6[iVar]) | ((pF[i] & ~Truth6[iVar]) << Shift);
            pCof1[i] = (pF[i] &  Truth6[iVar]) | ((pF[i] &  Truth6[iVar]) >> Shift);
        }
    }
    else
    {
        int i, k, Step = (1 << (iVar - 6));
        for ( k = 0; k < nWords; k += 2*Step )
        {
            for ( i = 0; i < Step; i++ )
            {
                pCof0[i] = pCof0[Step+i] = pF[i];
                pCof1[i] = pCof1[Step+i] = pF[Step+i];
            }
            pF    += 2*Step;
            pCof0 += 2*Step;
            pCof1 += 2*Step;
        }
    }
}

// returns 1 if we have special case of cofactors; otherwise, returns 0
int If_CluDetectSpecialCaseCofs( word * pF, int nVars, int iVar )
{
    word Cof0, Cof1;
    int State[6] = {0};
    int i, nWords = If_CluWordNum( nVars );
    assert( iVar < nVars );
    if ( iVar < 6 )
    {
        int Shift = (1 << iVar);
        for ( i = 0; i < nWords; i++ )
        {
            Cof0 =  (pF[i] & ~Truth6[iVar]);
            Cof1 = ((pF[i] &  Truth6[iVar]) >> Shift);

            if ( Cof0 == 0 )
                State[0]++;
            else if ( Cof0 == ~Truth6[iVar] )
                State[1]++;
            else if ( Cof1 == 0 )
                State[2]++;
            else if ( Cof1 == ~Truth6[iVar] )
                State[3]++;
            else if ( Cof0 == ~Cof1 )
                State[4]++;
            else if ( Cof0 == Cof1 )
                State[5]++;
        }
    }
    else
    {
        int k, Step = (1 << (iVar - 6));
        for ( k = 0; k < nWords; k += 2*Step )
        {
            for ( i = 0; i < Step; i++ )
            {
                Cof0 = pF[i];
                Cof1 = pF[Step+i];

                if ( Cof0 == 0 )
                    State[0]++;
                else if ( Cof0 == ~(word)0 )
                    State[1]++;
                else if ( Cof1 == 0 )
                    State[2]++;
                else if ( Cof1 == ~(word)0 )
                    State[3]++;
                else if ( Cof0 == ~Cof1 )
                    State[4]++;
                else if ( Cof0 == Cof1 )
                    State[5]++;
            }
            pF    += 2*Step;
        }
        nWords /= 2;
    }
    assert( State[5] != nWords );
    for ( i = 0; i < 5; i++ )
    {
        assert( State[i] <= nWords );
        if ( State[i] == nWords )
            return i;
    }
    return -1;
}

// returns 1 if we have special case of cofactors; otherwise, returns 0
If_Grp_t If_CluDecUsingCofs( word * pTruth, int nVars, int nLutLeaf )
{
    If_Grp_t G = {0};
    word pF2[CLU_WRD_MAX], * pF = pF2;
    int Var2Pla[CLU_VAR_MAX+2], Pla2Var[CLU_VAR_MAX+2];
    int V2P[CLU_VAR_MAX+2], P2V[CLU_VAR_MAX+2];
    int nVarsNeeded = nVars - nLutLeaf;
    int v, i, k, iVar, State;
//Kit_DsdPrintFromTruth( (unsigned*)pTruth, nVars ); printf( "\n" );
    // create local copy
    If_CluCopy( pF, pTruth, nVars );
    for ( k = 0; k < nVars; k++ )
        Var2Pla[k] = Pla2Var[k] = k;
    // find decomposable vars 
    for ( i = 0; i < nVarsNeeded; i++ )
    {
        for ( v = nVars - 1; v >= 0; v-- )
        {
            State = If_CluDetectSpecialCaseCofs( pF, nVars, v );
            if ( State == -1 )
                continue;
            // update the variable place
            iVar = Pla2Var[v];
            while ( Var2Pla[iVar] < nVars - 1 )
            {
                int iPlace0 = Var2Pla[iVar];
                int iPlace1 = Var2Pla[iVar]+1;
                Var2Pla[Pla2Var[iPlace0]]++;
                Var2Pla[Pla2Var[iPlace1]]--;
                Pla2Var[iPlace0] ^= Pla2Var[iPlace1];
                Pla2Var[iPlace1] ^= Pla2Var[iPlace0];
                Pla2Var[iPlace0] ^= Pla2Var[iPlace1];
            }
            // move this variable to the top
            for ( k = 0; k < nVars; k++ )
                V2P[k] = P2V[k] = k;
//Kit_DsdPrintFromTruth( (unsigned*)pF, nVars ); printf( "\n" );
            If_CluMoveVar( pF, nVars, V2P, P2V, v, nVars - 1 );
//Kit_DsdPrintFromTruth( (unsigned*)pF, nVars ); printf( "\n" );
            // choose cofactor to follow
            iVar = nVars - 1;
            if ( State == 0 || State == 1 ) // need cof1
            {
                if ( iVar < 6 )
                    pF[0] = (pF[0] &  Truth6[iVar]) | ((pF[0] &  Truth6[iVar]) >> (1 << iVar));
                else
                    pF += If_CluWordNum( nVars ) / 2;
            }
            else // need cof0
            {
                if ( iVar < 6 )
                    pF[0] = (pF[0] & ~Truth6[iVar]) | ((pF[0] & ~Truth6[iVar]) << (1 << iVar));
            }
            // update the variable count
            nVars--;
            break;
        }
        if ( v == -1 )
            return G;
    }
    // create the resulting group
    G.nVars = nLutLeaf;
    G.nMyu = 2;
    for ( v = 0; v < G.nVars; v++ )
        G.pVars[v] = Pla2Var[v];
    return G;
}



// deriving decomposition
word If_CluDeriveDisjoint( word * pF, int nVars, int * V2P, int * P2V, If_Grp_t * g, If_Grp_t * r )
{
    word pCofs[3][CLU_WRD_MAX/4];
    int i, RetValue, nFSset = nVars - g->nVars;
    RetValue = If_CluCountCofs( pF, nVars, g->nVars, 0, pCofs );
//    assert( RetValue == 2 );

    if ( nFSset < 6 )
        pF[0] = (pCofs[1][0] << (1 << nFSset)) | pCofs[0][0];
    else
    {
        If_CluCopy( pF, pCofs[0], nFSset );
        If_CluCopy( pF + If_CluWordNum(nFSset), pCofs[1], nFSset );
    }
    // create the resulting group
    if ( r )
    {
        r->nVars = nFSset + 1;
        r->nMyu = 0;
        for ( i = 0; i < nFSset; i++ )
            r->pVars[i] = P2V[i];
        r->pVars[nFSset] = nVars;
    }
    return pCofs[2][0];
}
void If_CluDeriveDisjoint4( word * pF, int nVars, int * V2P, int * P2V, If_Grp_t * g, If_Grp_t * r, word * pTruth0, word * pTruth1 )
{
    word pCofs[6][CLU_WRD_MAX/4];
    word Cof0, Cof1;
    int i, RetValue, nFSset = nVars - g->nVars;

    assert( g->nVars <= 6 && nFSset <= 4 );

    RetValue = If_CluCountCofs4( pF, nVars, g->nVars, pCofs );
    if ( RetValue != 3 && RetValue != 4 )
        printf( "If_CluDeriveDisjoint4(): Error!!!\n" );

    Cof0  = (pCofs[1][0] << (1 << nFSset)) | pCofs[0][0];
    Cof1  = (pCofs[3][0] << (1 << nFSset)) | pCofs[2][0];
    pF[0] = (Cof1 << (1 << (nFSset+1))) | Cof0;
    pF[0] = If_CluAdjust( pF[0], nFSset + 2 );

    // create the resulting group
    r->nVars = nFSset + 2;
    r->nMyu = 0;
    for ( i = 0; i < nFSset; i++ )
        r->pVars[i] = P2V[i];
    r->pVars[nFSset] = nVars;
    r->pVars[nFSset+1] = nVars+1;

    *pTruth0 = If_CluAdjust( pCofs[4][0], g->nVars );
    *pTruth1 = If_CluAdjust( pCofs[5][0], g->nVars );
}

word If_CluDeriveNonDisjoint( word * pF, int nVars, int * V2P, int * P2V, If_Grp_t * g, If_Grp_t * r )
{
    word pCofs[2][CLU_WRD_MAX];
    word Truth0, Truth1, Truth;
    int i, nFSset = nVars - g->nVars, nFSset1 = nFSset + 1;
    If_CluCofactors( pF, nVars, nVars - 1, pCofs[0], pCofs[1] );

//    Extra_PrintHex( stdout, (unsigned *)pCofs[0], nVars ); printf( "\n" );
//    Extra_PrintHex( stdout, (unsigned *)pCofs[1], nVars ); printf( "\n" );

    g->nVars--;
    Truth0 = If_CluDeriveDisjoint( pCofs[0], nVars - 1, V2P, P2V, g, NULL );
    Truth1 = If_CluDeriveDisjoint( pCofs[1], nVars - 1, V2P, P2V, g, NULL );
    Truth  = (Truth1 << (1 << g->nVars)) | Truth0;
    g->nVars++;
    if ( nFSset1 < 6 )
        pF[0] = (pCofs[1][0] << (1 << nFSset1)) | pCofs[0][0];
    else
    {
        If_CluCopy( pF, pCofs[0], nFSset1 );
        If_CluCopy( pF + If_CluWordNum(nFSset1), pCofs[1], nFSset1 );
    }

//    Extra_PrintHex( stdout, (unsigned *)&Truth0, 6 ); printf( "\n" );
//    Extra_PrintHex( stdout, (unsigned *)&Truth1, 6 ); printf( "\n" );
//    Extra_PrintHex( stdout, (unsigned *)&pCofs[0][0], 6 ); printf( "\n" );
//    Extra_PrintHex( stdout, (unsigned *)&pCofs[1][0], 6 ); printf( "\n" );
//    Extra_PrintHex( stdout, (unsigned *)&Truth, 6 ); printf( "\n" );
//    Extra_PrintHex( stdout, (unsigned *)&pF[0], 6 ); printf( "\n" );

    // create the resulting group
    r->nVars = nFSset + 2;
    r->nMyu = 0;
    for ( i = 0; i < nFSset; i++ )
        r->pVars[i] = P2V[i];
    r->pVars[nFSset] = nVars;
    r->pVars[nFSset+1] = g->pVars[g->nVars - 1];
    return Truth;
}

// check non-disjoint decomposition
int If_CluCheckNonDisjointGroup( word * pF, int nVars, int * V2P, int * P2V, If_Grp_t * g )
{
    int v, i, nCofsBest2;
    if ( (g->nMyu == 3 || g->nMyu == 4) )
    {
        word pCofs[2][CLU_WRD_MAX];
        // try cofactoring w.r.t. each variable
        for ( v = 0; v < g->nVars; v++ )
        {
            If_CluCofactors( pF, nVars, V2P[(int)g->pVars[v]], pCofs[0], pCofs[1] );
            nCofsBest2 = If_CluCountCofs( pCofs[0], nVars, g->nVars, 0, NULL );
            if ( nCofsBest2 > 2 )
                continue;
            nCofsBest2 = If_CluCountCofs( pCofs[1], nVars, g->nVars, 0, NULL );
            if ( nCofsBest2 > 2 )
                continue;
            // found good shared variable - move to the end
            If_CluMoveVar( pF, nVars, V2P, P2V, g->pVars[v], nVars-1 );
            for ( i = 0; i < g->nVars; i++ )
                g->pVars[i] = P2V[nVars-g->nVars+i];
            return 1;
        }
    }
    return 0;
}


// finds a good var group (cof count < 6; vars are MSBs)
If_Grp_t If_CluFindGroup( word * pF, int nVars, int iVarStart, int iVarStop, int * V2P, int * P2V, int nBSsize, int fDisjoint )
{
    int fVerbose = 0;
    int nRounds = 2;//nBSsize;
    If_Grp_t G = {0}, * g = &G;//, BestG = {0};
    int i, r, v, nCofs, VarBest, nCofsBest2;
    assert( nVars > nBSsize && nVars >= nBSsize + iVarStart && nVars <= CLU_VAR_MAX );
    assert( nBSsize >= 2 && nBSsize <= 6 );
    assert( !iVarStart || !iVarStop );
    // start with the default group
    g->nVars = nBSsize;
    g->nMyu = If_CluCountCofs( pF, nVars, nBSsize, 0, NULL );
    for ( i = 0; i < nBSsize; i++ )
        g->pVars[i] = P2V[nVars-nBSsize+i];
    // check if good enough
    if ( g->nMyu == 2 )
        return G;
    if ( !fDisjoint && If_CluCheckNonDisjointGroup( pF, nVars, V2P, P2V, g ) )
    {
//        BestG = G;
        return G;
    }
    if ( nVars == nBSsize + iVarStart )
    {
        g->nVars = 0;
        return G;
    }

    if ( fVerbose )
    {
        printf( "Iter %2d  ", -1 );
        If_CluPrintGroup( g );
    }

    // try to find better group
    for ( r = 0; r < nRounds; r++ )
    {
        if ( nBSsize < nVars-1 )
        {
            // find the best var to add
            VarBest = P2V[nVars-1-nBSsize];
            nCofsBest2 = If_CluCountCofs( pF, nVars, nBSsize+1, 0, NULL );
            for ( v = nVars-2-nBSsize; v >= iVarStart; v-- )
            {
//                If_CluMoveVar( pF, nVars, V2P, P2V, P2V[v], nVars-1-nBSsize );
                If_CluMoveVar2( pF, nVars, V2P, P2V, P2V[v], nVars-1-nBSsize );
                nCofs = If_CluCountCofs( pF, nVars, nBSsize+1, 0, NULL );
                if ( nCofsBest2 >= nCofs )
                {
                    nCofsBest2 = nCofs;
                    VarBest = P2V[nVars-1-nBSsize];
                }
            }
            // go back
//            If_CluMoveVar( pF, nVars, V2P, P2V, VarBest, nVars-1-nBSsize );
            If_CluMoveVar2( pF, nVars, V2P, P2V, VarBest, nVars-1-nBSsize );
            // update best bound set
            nCofs = If_CluCountCofs( pF, nVars, nBSsize+1, 0, NULL );
            assert( nCofs == nCofsBest2 );
        }

        // find the best var to remove
        VarBest = P2V[nVars-1-nBSsize];
        nCofsBest2 = If_CluCountCofs( pF, nVars, nBSsize, 0, NULL );
        for ( v = nVars-nBSsize; v < nVars-iVarStop; v++ )
        {
//            If_CluMoveVar( pF, nVars, V2P, P2V, P2V[v], nVars-1-nBSsize );
            If_CluMoveVar2( pF, nVars, V2P, P2V, P2V[v], nVars-1-nBSsize );
            nCofs = If_CluCountCofs( pF, nVars, nBSsize, 0, NULL );
            if ( nCofsBest2 >= nCofs )
            {
                nCofsBest2 = nCofs;
                VarBest = P2V[nVars-1-nBSsize];
            }
        }

        // go back
//        If_CluMoveVar( pF, nVars, V2P, P2V, VarBest, nVars-1-nBSsize );
        If_CluMoveVar2( pF, nVars, V2P, P2V, VarBest, nVars-1-nBSsize );
        // update best bound set
        nCofs = If_CluCountCofs( pF, nVars, nBSsize, 0, NULL );
        assert( nCofs == nCofsBest2 );
        if ( g->nMyu >= nCofs )
        {
            g->nVars = nBSsize;
            g->nMyu = nCofs;
            for ( i = 0; i < nBSsize; i++ )
                g->pVars[i] = P2V[nVars-nBSsize+i];
        }

        if ( fVerbose )
        {
            printf( "Iter %2d  ", r );
            If_CluPrintGroup( g );
        }

        // check if good enough
        if ( g->nMyu == 2 )
            return G;
        if ( !fDisjoint && If_CluCheckNonDisjointGroup( pF, nVars, V2P, P2V, g ) )
        {
//            BestG = G;
            return G;
        }
    }

    assert( r == nRounds );
    g->nVars = 0;
    return G;
//    return BestG;
}


// double check that the given group has a decomposition
void If_CluCheckGroup( word * pTruth, int nVars, If_Grp_t * g )
{
    word pF[CLU_WRD_MAX];
    int v, nCofs, V2P[CLU_VAR_MAX], P2V[CLU_VAR_MAX];
    assert( g->nVars >= 2 && g->nVars <= 6 ); // vars
    assert( g->nMyu >= 2 && g->nMyu <= 4 ); // cofs
    // create permutation
    for ( v = 0; v < nVars; v++ )
        V2P[v] = P2V[v] = v;
    // create truth table
    If_CluCopy( pF, pTruth, nVars );
    // move group up
    If_CluMoveGroupToMsb( pF, nVars, V2P, P2V, g );
    // check the number of cofactors
    nCofs = If_CluCountCofs( pF, nVars, g->nVars, 0, NULL );
    if ( nCofs != g->nMyu )
        printf( "Group check 0 has failed.\n" );
    // check compatible
    if ( nCofs > 2 )
    {
        nCofs = If_CluCountCofs( pF, nVars-1, g->nVars-1, 0, NULL );
        if ( nCofs > 2 )
            printf( "Group check 1 has failed.\n" );
        nCofs = If_CluCountCofs( pF, nVars-1, g->nVars-1, (1 << (nVars-1)), NULL );
        if ( nCofs > 2 )
            printf( "Group check 2 has failed.\n" );
    }
}


// double check that the permutation derived is correct
void If_CluCheckPerm( word * pTruth, word * pF, int nVars, int * V2P, int * P2V )
{
    int i;
    for ( i = 0; i < nVars; i++ )
        If_CluMoveVar( pF, nVars, V2P, P2V, i, i );

    if ( !If_CluEqual( pTruth, pF, nVars ) )
        printf( "Permutation FAILED.\n" );
//    else
//        printf( "Permutation successful\n" );
}




static inline int If_CluSuppIsMinBase( int Supp )
{
    return (Supp & (Supp+1)) == 0;
}
static inline int If_CluHasVar( word * t, int nVars, int iVar )
{
    int nWords = If_CluWordNum( nVars );
    assert( iVar < nVars );
    if ( iVar < 6 )
    {
        int i, Shift = (1 << iVar);
        for ( i = 0; i < nWords; i++ )
            if ( (t[i] & ~Truth6[iVar]) != ((t[i] & Truth6[iVar]) >> Shift) )
                return 1;
        return 0;
    }
    else
    {
        int i, k, Step = (1 << (iVar - 6));
        for ( k = 0; k < nWords; k += 2*Step )
        {
            for ( i = 0; i < Step; i++ )
                if ( t[i] != t[Step+i] )
                    return 1;
            t += 2*Step;
        }
        return 0;
    }
}
static inline int If_CluSupport( word * t, int nVars )
{
    int v, Supp = 0;
    for ( v = 0; v < nVars; v++ )
        if ( If_CluHasVar( t, nVars, v ) )
            Supp |= (1 << v);
    return Supp;
}
int If_CluSupportSize( word * t, int nVars )
{
    int v, SuppSize = 0;
    for ( v = 0; v < nVars; v++ )
        if ( If_CluHasVar( t, nVars, v ) )
            SuppSize++;
    return SuppSize;
}
static inline void If_CluTruthShrink( word * pF, int nVars, int nVarsAll, unsigned Phase )
{
    word pG[CLU_WRD_MAX], * pIn = pF, * pOut = pG, * pTemp;
    int i, k, Var = 0, Counter = 0;
    assert( nVarsAll <= 16 );
    for ( i = 0; i < nVarsAll; i++ )
        if ( Phase & (1 << i) )
        {
            for ( k = i-1; k >= Var; k-- )
            {
                If_CluSwapAdjacent( pOut, pIn, k, nVarsAll );
                pTemp = pIn; pIn = pOut, pOut = pTemp;
                Counter++;
            }
            Var++;
        }
    assert( Var == nVars );
    // swap if it was moved an odd number of times
    if ( Counter & 1 )
        If_CluCopy( pOut, pIn, nVarsAll );
}
int If_CluMinimumBase( word * t, int * pSupp, int nVarsAll, int * pnVars )
{
    int v, iVar = 0, uSupp = 0;
    assert( nVarsAll <= 16 );
    for ( v = 0; v < nVarsAll; v++ )
        if ( If_CluHasVar( t, nVarsAll, v ) )
        {
            uSupp |= (1 << v);
            if ( pSupp )
                pSupp[iVar] = pSupp[v];
            iVar++;
        }
    if ( pnVars )
        *pnVars = iVar;
    if ( If_CluSuppIsMinBase( uSupp ) )
        return 0;
    If_CluTruthShrink( t, iVar, nVarsAll, uSupp );
    return 1;
}

// returns the best group found
If_Grp_t If_CluCheck( If_Man_t * p, word * pTruth0, int nVars, int iVarStart, int iVarStop, int nLutLeaf, int nLutRoot, 
                     If_Grp_t * pR, word * pFunc0, word * pFunc1, word * pLeftOver, int fHashing )
{
//    int fEnableHashing = 0;
    If_Grp_t G1 = {0}, R = {0};
    unsigned * pHashed = NULL;
    word Truth, pTruth[CLU_WRD_MAX], pF[CLU_WRD_MAX];//, pG[CLU_WRD_MAX];
    int V2P[CLU_VAR_MAX+2], P2V[CLU_VAR_MAX+2], pCanonPerm[CLU_VAR_MAX];
    int i, nSupp, uCanonPhase;
    int nLutSize = p ? p->pPars->nLutSize : nVars;
    assert( nVars <= CLU_VAR_MAX );
    assert( nVars <= nLutLeaf + nLutRoot - 1 );

    if ( pR )
    {
        pR->nVars = 0;
        *pFunc0 = 0;
        *pFunc1 = 0;
    }

    // canonicize truth table
    If_CluCopy( pTruth, pTruth0, nLutSize );

    if ( 0 )
    {
        uCanonPhase = If_CluSemiCanonicize( pTruth, nVars, pCanonPerm );
        If_CluAdjustBig( pTruth, nVars, nLutSize );
    }

//    If_CluSemiCanonicizeVerify( pTruth, pTruth0, nVars, pCanonPerm, uCanonPhase );
//    If_CluCopy( pTruth, pTruth0, nLutSize );

 /*
    {
        int pCanonPerm[32];
        short pStore[32];
        unsigned uCanonPhase;
        If_CluCopy( pF, pTruth, nVars );
        uCanonPhase = Kit_TruthSemiCanonicize( pF, pG, nVars, pCanonPerm );
        G1.nVars = 1;
        return G1;
    }
*/
    // check minnimum base
    If_CluCopy( pF, pTruth, nVars );
    for ( i = 0; i < nVars; i++ )
        V2P[i] = P2V[i] = i;
    // check support
    nSupp = If_CluSupport( pF, nVars );
//Extra_PrintBinary( stdout, &nSupp, 16 );  printf( "\n" );
    if ( !nSupp || !If_CluSuppIsMinBase(nSupp) )
    {
//        assert( 0 );     
        return G1;
    }

    // check hash table
    if ( p && fHashing )
    {
        pHashed = If_CluHashLookup( p, pTruth, 0 );
        if ( pHashed && *pHashed != CLU_UNUSED )
            If_CluUns2Grp( *pHashed, &G1 );
    }

    // update the variable order so that the first var was the last one
    if ( iVarStop )
        If_CluMoveVar( pF, nVars, V2P, P2V, 0, nVars-1 );

    if ( G1.nVars == 0 ) 
    {
        s_Count2++;

        // detect easy cofs
        if ( iVarStart == 0 )
            G1 = If_CluDecUsingCofs( pTruth, nVars, nLutLeaf );
        if ( G1.nVars == 0 )
        {
            // perform testing
            G1 = If_CluFindGroup( pF, nVars, iVarStart, iVarStop, V2P, P2V, nLutLeaf, nLutLeaf + nLutRoot == nVars + 1 );
    //        If_CluCheckPerm( pTruth, pF, nVars, V2P, P2V );
            if ( G1.nVars == 0 )
            {
                // perform testing with a smaller set
                if ( nVars < nLutLeaf + nLutRoot - 2 )
                {
                    nLutLeaf--;
                    G1 = If_CluFindGroup( pF, nVars, iVarStart, iVarStop, V2P, P2V, nLutLeaf, nLutLeaf + nLutRoot == nVars + 1 );
                    nLutLeaf++;
                }
                // perform testing with a smaller set
                if ( nLutLeaf > 4 && nVars < nLutLeaf + nLutRoot - 3 )
                {
                    nLutLeaf--;
                    nLutLeaf--;
                    G1 = If_CluFindGroup( pF, nVars, iVarStart, iVarStop, V2P, P2V, nLutLeaf, nLutLeaf + nLutRoot == nVars + 1 );
                    nLutLeaf++;
                    nLutLeaf++;
                }
                if ( G1.nVars == 0 )
                {
                    // perform testing with a different order
                    If_CluReverseOrder( pF, nVars, V2P, P2V, iVarStart );
                    G1 = If_CluFindGroup( pF, nVars, iVarStart, iVarStop, V2P, P2V, nLutLeaf, nLutLeaf + nLutRoot == nVars + 1 );

                    // check permutation
    //                If_CluCheckPerm( pTruth, pF, nVars, V2P, P2V );
                    if ( G1.nVars == 0 )
                    {
                        // remember free set, just in case
//                        for ( i = 0; i < nVars - nLutLeaf; i++ )
///                           G1.pVars[nLutLeaf+i] = P2V[i];
                        // if <XY>, this will not be used
                        // if <XYZ>, this will not be hashed

    /*
                        if ( nVars == 6 )
                        {
                            Extra_PrintHex( stdout, (unsigned *)pF, nVars );  printf( "    " );
                            Kit_DsdPrintFromTruth( (unsigned*)pF, nVars );  printf( "\n" );
                            if ( !If_CutPerformCheck07( (unsigned *)pF, 6, 6, NULL ) )
                                printf( "no\n" );
                        } 
    */
                        if ( pHashed )
                            *pHashed = If_CluGrp2Uns( &G1 );
                        return G1;
                    }
                }
            }
        }
    }

    // derive
    if ( pR )
    {
        int iNewPos;

        If_CluMoveGroupToMsb( pF, nVars, V2P, P2V, &G1 );
        if ( G1.nMyu == 2 )
        {
            Truth = If_CluDeriveDisjoint( pF, nVars, V2P, P2V, &G1, &R );
            iNewPos = R.nVars - 1;
        }
        else
        {
            Truth = If_CluDeriveNonDisjoint( pF, nVars, V2P, P2V, &G1, &R );
            iNewPos = R.nVars - 2;
        }
        assert( R.pVars[iNewPos] == nVars );

        // adjust the functions
        Truth = If_CluAdjust( Truth, G1.nVars );
        if ( R.nVars < 6 )
            pF[0] = If_CluAdjust( pF[0], R.nVars );

//        Kit_DsdPrintFromTruth( (unsigned*)&Truth, G1.nVars ); printf( "  ...1\n" );
//        Kit_DsdPrintFromTruth( (unsigned*)pF, R.nVars );      printf( "  ...1\n" );

        // update the variable order of R so that the new var was the first one
//        if ( iVarStart == 0 )
        {
            int k, V2P2[CLU_VAR_MAX+2], P2V2[CLU_VAR_MAX+2];
            assert( iNewPos >= iVarStart );
            for ( k = 0; k < R.nVars; k++ )
                V2P2[k] = P2V2[k] = k;
            If_CluMoveVar( pF, R.nVars, V2P2, P2V2, iNewPos, iVarStart );
            for ( k = iNewPos; k > iVarStart; k-- )
                R.pVars[k] = R.pVars[k-1];
            R.pVars[iVarStart] = nVars;
        }

//        Kit_DsdPrintFromTruth( (unsigned*)pF, R.nVars ); printf( "  ...2\n" );

        if ( pLeftOver )
        {
            if ( R.nVars < 6 )
                *pLeftOver = If_CluAdjust( pF[0], R.nVars );
            else
                If_CluCopy( pLeftOver, pF, R.nVars );
            If_CluAdjustBig( pLeftOver, R.nVars, nLutSize );
        }

        // perform checking
        if ( 0 )
        {
            If_CluCheckGroup( pTruth, nVars, &G1 );
            If_CluVerify( pTruth, nVars, &G1, &R, Truth, pF );
        } 

        // save functions
        *pR = R;
        if ( pFunc0 )
            *pFunc0 = pF[0];
        if ( pFunc1 )
            *pFunc1 = Truth;
    }

    if ( pHashed )
        *pHashed = If_CluGrp2Uns( &G1 );
    return G1;
}

/*
static inline word Abc_Tt6Cofactor0( word t, int iVar )
{
    assert( iVar >= 0 && iVar < 6 );
    return (t &~Truth6[iVar]) | ((t &~Truth6[iVar]) << (1<<iVar));
}
static inline word Abc_Tt6Cofactor1( word t, int iVar )
{
    assert( iVar >= 0 && iVar < 6 );
    return (t & Truth6[iVar]) | ((t & Truth6[iVar]) >> (1<<iVar));
}
*/
int If_CluCheckDecInAny( word t, int nVars )
{
    int v, u, Cof2[2], Cof4[4];
    for ( v = 0; v < nVars; v++ )
    {
        Cof2[0] = Abc_Tt6Cofactor0( t, v );
        Cof2[1] = Abc_Tt6Cofactor1( t, v );
        for ( u = v+1; u < nVars; u++ )
        {
            Cof4[0] = Abc_Tt6Cofactor0( Cof2[0], u );
            Cof4[1] = Abc_Tt6Cofactor1( Cof2[0], u );
            Cof4[2] = Abc_Tt6Cofactor0( Cof2[1], u );
            Cof4[3] = Abc_Tt6Cofactor1( Cof2[1], u );
            if ( Cof4[0] == Cof4[1] && Cof4[0] == Cof4[2] )
                return 1;
            if ( Cof4[0] == Cof4[2] && Cof4[0] == Cof4[3] )
                return 1;
            if ( Cof4[0] == Cof4[1] && Cof4[0] == Cof4[3] )
                return 1;
            if ( Cof4[1] == Cof4[2] && Cof4[1] == Cof4[3] )
                return 1;
        }
    }
    return 0;
}
int If_CluCheckDecIn( word t, int nVars )
{
    int v, u, Cof2[2], Cof4[4];
//    for ( v = 0; v < nVars; v++ )
    for ( v = 0; v < 1; v++ ) // restrict to the first (decomposed) input
    {
        Cof2[0] = Abc_Tt6Cofactor0( t, v );
        Cof2[1] = Abc_Tt6Cofactor1( t, v );
        for ( u = v+1; u < nVars; u++ )
        {
            Cof4[0] = Abc_Tt6Cofactor0( Cof2[0], u );
            Cof4[1] = Abc_Tt6Cofactor1( Cof2[0], u );
            Cof4[2] = Abc_Tt6Cofactor0( Cof2[1], u );
            Cof4[3] = Abc_Tt6Cofactor1( Cof2[1], u );
            if ( Cof4[0] == Cof4[1] && Cof4[0] == Cof4[2] )
                return 1;
            if ( Cof4[0] == Cof4[2] && Cof4[0] == Cof4[3] )
                return 1;
            if ( Cof4[0] == Cof4[1] && Cof4[0] == Cof4[3] )
                return 1;
            if ( Cof4[1] == Cof4[2] && Cof4[1] == Cof4[3] )
                return 1;
        }
    }
    return 0;
}
int If_CluCheckDecInU( word t, int nVars )
{
    int v, u, Cof2[2], Cof4[4];
//    for ( v = 0; v < nVars; v++ )
    for ( v = 0; v < 1; v++ ) // restrict to the first (decomposed) input
    {
        Cof2[0] = Abc_Tt6Cofactor0( t, v );
        Cof2[1] = Abc_Tt6Cofactor1( t, v );
        for ( u = v+1; u < nVars; u++ )
        {
            Cof4[0] = Abc_Tt6Cofactor0( Cof2[0], u ); // 00
            Cof4[1] = Abc_Tt6Cofactor1( Cof2[0], u ); // 01
            Cof4[2] = Abc_Tt6Cofactor0( Cof2[1], u ); // 10
            Cof4[3] = Abc_Tt6Cofactor1( Cof2[1], u ); // 11 
            if ( Cof4[0] == Cof4[1] && Cof4[0] == Cof4[2] ) //  F * a
                return 1;
            if ( Cof4[0] == Cof4[2] && Cof4[0] == Cof4[3] ) // !F * a
                return 1;
        }
    }
    return 0;
}
int If_CluCheckDecOut( word t, int nVars )
{
    int v;
    for ( v = 0; v < nVars; v++ )
        if ( 
             (t & Truth6[v]) == 0   ||  //  F * !a
             (~t & Truth6[v]) == 0  ||  // !F * !a
             (t & ~Truth6[v]) == 0  ||  //  F *  a
             (~t & ~Truth6[v]) == 0     // !F *  a   
           )
            return 1;
    return 0;
}
int If_CluCheckDecOutU( word t, int nVars )
{
    int v;
    for ( v = 0; v < nVars; v++ )
        if ( 
             (t & ~Truth6[v]) == 0  ||  //  F *  a
             (~t & ~Truth6[v]) == 0     // !F *  a   
           )
            return 1;
    return 0;
}

int If_CutPerformCheck45( If_Man_t * p, unsigned * pTruth, int nVars, int nLeaves, char * pStr )
{    
    // 5LUT -> 4LUT
    If_Grp_t G, R;
    word Func0, Func1;
    G = If_CluCheck( p, (word *)pTruth, nLeaves, 0, 0, 5, 4, &R, &Func0, &Func1, NULL, 0 );
    if ( G.nVars == 0 )
        return 0;
    Func0 = If_CluAdjust( Func0, R.nVars );
    Func1 = If_CluAdjust( Func1, G.nVars );
#if 0
    Kit_DsdPrintFromTruth( pTruth, nVars ); printf( "\n" );
    Kit_DsdPrintFromTruth( (unsigned*)&Func0, R.nVars ); printf( "\n" );
    Kit_DsdPrintFromTruth( (unsigned*)&Func1, G.nVars ); printf( "\n" );
    If_CluPrintGroup( &R );
    If_CluPrintGroup( &G );
#endif
    if ( G.nVars < 5 || (p->pPars->fEnableCheck75 && If_CluCheckDecOut(Func1, 5)) || (p->pPars->fEnableCheck75u && If_CluCheckDecOutU(Func1, 5)) )
        return 1;
    return 0;
}
int If_CutPerformCheck54( If_Man_t * p, unsigned * pTruth, int nVars, int nLeaves, char * pStr )
{    
    // 4LUT -> 5LUT
    If_Grp_t G, R;
    word Func0, Func1;
    G = If_CluCheck( p, (word *)pTruth, nLeaves, 0, 0, 4, 5, &R, &Func0, &Func1, NULL, 0 );
    if ( G.nVars == 0 )
        return 0;
    Func0 = If_CluAdjust( Func0, R.nVars );
    Func1 = If_CluAdjust( Func1, G.nVars );
#if 0
    Kit_DsdPrintFromTruth( pTruth, nVars ); printf( "\n" );
    Kit_DsdPrintFromTruth( (unsigned*)&Func0, R.nVars ); printf( "\n" );
    Kit_DsdPrintFromTruth( (unsigned*)&Func1, G.nVars ); printf( "\n" );
    If_CluPrintGroup( &R );
    If_CluPrintGroup( &G );
#endif
    if ( R.nVars < 5 || (p->pPars->fEnableCheck75 && If_CluCheckDecIn(Func0, 5)) || (p->pPars->fEnableCheck75u && If_CluCheckDecInU(Func0, 5)) )
        return 1;
    return 0;
}

// returns the best group found
If_Grp_t If_CluCheck3( If_Man_t * p, word * pTruth0, int nVars, int nLutLeaf, int nLutLeaf2, int nLutRoot, 
                      If_Grp_t * pR, If_Grp_t * pG2, word * pFunc0, word * pFunc1, word * pFunc2 )
{
    int fEnableHashing = 0;
    static int Counter = 0;
    unsigned * pHashed = NULL;
    word pLeftOver[CLU_WRD_MAX], Func0, Func1, Func2;
    If_Grp_t G1 = {0}, G2 = {0}, R = {0}, R2 = {0};
    int i;
    Counter++;
//    if ( Counter == 37590 )
//    {
//        int ns = 0;
//    }

    // check hash table
    if ( p && fEnableHashing )
    {
        pHashed = If_CluHashLookup( p, pTruth0, 1 );
        if ( pHashed && *pHashed != CLU_UNUSED )
        {
            If_CluUns2Grp( *pHashed, &G1 );
            return G1;
        }
    }
    s_Count3++;

    // check two-node decomposition
    G1 = If_CluCheck( p, pTruth0, nVars, 0, 0, nLutLeaf, nLutRoot + nLutLeaf2 - 1, &R2, &Func0, &Func1, pLeftOver, 0 );
    // decomposition does not exist
    if ( G1.nVars == 0 )
    {
        // check for decomposition with two outputs
        if ( (G1.nMyu == 3 || G1.nMyu == 4) && nLutLeaf == nLutLeaf2 && nVars - nLutLeaf + 2 <= nLutRoot )
        {
            int V2P[CLU_VAR_MAX+2], P2V[CLU_VAR_MAX+2];
            word Func0, Func1, Func2;
            int iVar0, iVar1;

            G1.nVars = nLutLeaf;
            If_CluCopy( pLeftOver, pTruth0, nVars );
            for ( i = 0; i < nVars; i++ )
                V2P[i] = P2V[i] = i;

            If_CluMoveGroupToMsb( pLeftOver, nVars, V2P, P2V, &G1 );
            If_CluDeriveDisjoint4( pLeftOver, nVars, V2P, P2V, &G1, &R, &Func1, &Func2 );

            // move the two vars to the front
            for ( i = 0; i < R.nVars; i++ )
                V2P[i] = P2V[i] = i;
            If_CluMoveVar( pLeftOver, R.nVars, V2P, P2V, R.nVars-2, 0 );
            If_CluMoveVar( pLeftOver, R.nVars, V2P, P2V, R.nVars-1, 1 );
            iVar0 = R.pVars[R.nVars-2];
            iVar1 = R.pVars[R.nVars-1];
            for ( i = R.nVars-1; i > 1; i-- )
                R.pVars[i] = R.pVars[i-2];
            R.pVars[0] = iVar0;
            R.pVars[1] = iVar1;

            Func0 = pLeftOver[0];
            If_CluVerify3( pTruth0, nVars, &G1, &G1, &R, Func1, Func2, Func0 );
            if ( pFunc1 && pFunc2 )
            {
                *pFunc0 = Func0;
                *pFunc1 = Func1;
                *pFunc2 = Func2;
                *pG2 = G1;
                *pR = R;
            }

            if ( pHashed )
                *pHashed = If_CluGrp2Uns( &G1 );
//                Kit_DsdPrintFromTruth( (unsigned*)pTruth0, nVars );  printf( "\n" );
//                If_CluPrintGroup( &G1 );
            return G1;
        }

/*
//        if ( nVars == 6 )
        {
//            Extra_PrintHex( stdout, (unsigned *)pTruth0, nVars );  printf( "    " );
            Kit_DsdPrintFromTruth( (unsigned*)pTruth0, nVars );  printf( "\n" );
            if ( p != NULL )
            If_CluCheck3( NULL, pTruth0, nVars, nLutLeaf, nLutLeaf2, nLutRoot, pR, pG2, pFunc0, pFunc1, pFunc2 );
        } 
*/
        if ( pHashed )
            *pHashed = If_CluGrp2Uns( &G1 );
        return G1;
    }
    // decomposition exists and sufficient
    if ( R2.nVars <= nLutRoot )
    {
        if ( pG2 )     *pG2 = G2;
        if ( pR )      *pR  = R2;
        if ( pFunc0 )  *pFunc0 = Func0;
        if ( pFunc1 )  *pFunc1 = Func1;
        if ( pFunc2 )  *pFunc2 = 0;
        if ( pHashed )
            *pHashed = If_CluGrp2Uns( &G1 );
        return G1;
    }

    // try second decomposition
    {
        int Test = 0;
        if ( Test )
        {
            Kit_DsdPrintFromTruth( (unsigned*)&pLeftOver, R2.nVars ); printf( "\n" );
        }
    }

    // the new variable is at the bottom - skip it (iVarStart = 1)
    if ( p->pPars->nStructType == 0 ) // allowed
        G2 = If_CluCheck( p, pLeftOver, R2.nVars, 0, 0, nLutLeaf2, nLutRoot, &R, &Func0, &Func2, NULL, 0 );
    else if ( p->pPars->nStructType == 1 ) // not allowed
        G2 = If_CluCheck( p, pLeftOver, R2.nVars, 1, 0, nLutLeaf2, nLutRoot, &R, &Func0, &Func2, NULL, 0 );
    else if ( p->pPars->nStructType == 2 ) // required
        G2 = If_CluCheck( p, pLeftOver, R2.nVars, 0, 1, nLutLeaf2, nLutRoot, &R, &Func0, &Func2, NULL, 0 );
    else assert( 0 );

    if ( G2.nVars == 0 )
    {
        if ( pHashed )
            *pHashed = If_CluGrp2Uns( &G2 );
        return G2;
    }
    // remap variables
    for ( i = 0; i < G2.nVars; i++ )
    {
        assert( G2.pVars[i] < R2.nVars );
        G2.pVars[i] = R2.pVars[ (int)G2.pVars[i] ];
    }
    // remap variables
    for ( i = 0; i < R.nVars; i++ )
    {
        if ( R.pVars[i] == R2.nVars )
            R.pVars[i] = nVars + 1;
        else
            R.pVars[i] = R2.pVars[ (int)R.pVars[i] ];
    }

    // decomposition exist
    if ( pG2 )     *pG2 = G2;
    if ( pR )      *pR  = R;
    if ( pFunc0 )  *pFunc0 = Func0;
    if ( pFunc1 )  *pFunc1 = Func1;
    if ( pFunc2 )  *pFunc2 = Func2;
    if ( pHashed )
        *pHashed = If_CluGrp2Uns( &G1 );

    // verify
//    If_CluVerify3( pTruth0, nVars, &G1, &G2, &R, Func1, Func2, Func0 );
    return G1;
}

// returns the best group found
int If_CluCheckExt( void * pMan, word * pTruth, int nVars, int nLutLeaf, int nLutRoot, 
                   char * pLut0, char * pLut1, word * pFunc0, word * pFunc1 )
{
    If_Man_t * p = (If_Man_t *)pMan;
    If_Grp_t G, R;
    G = If_CluCheck( p, pTruth, nVars, 0, 0, nLutLeaf, nLutRoot, &R, pFunc0, pFunc1, NULL, 0 );
    memcpy( pLut0, &R, sizeof(If_Grp_t) );
    memcpy( pLut1, &G, sizeof(If_Grp_t) );
//    memcpy( pLut2, &G2, sizeof(If_Grp_t) );
    return (G.nVars > 0);
}

// returns the best group found
int If_CluCheckExt3( void * pMan, word * pTruth, int nVars, int nLutLeaf, int nLutLeaf2, int nLutRoot, 
                    char * pLut0, char * pLut1, char * pLut2, word * pFunc0, word * pFunc1, word * pFunc2 )
{
    If_Man_t * p = (If_Man_t *)pMan;
    If_Grp_t G, G2, R;
    G = If_CluCheck3( p, pTruth, nVars, nLutLeaf, nLutLeaf2, nLutRoot, &R, &G2, pFunc0, pFunc1, pFunc2 );
    memcpy( pLut0, &R, sizeof(If_Grp_t) );
    memcpy( pLut1, &G, sizeof(If_Grp_t) );
    memcpy( pLut2, &G2, sizeof(If_Grp_t) );
    return (G.nVars > 0);
}


// computes delay of the decomposition
float If_CluDelayMax( If_Grp_t * g, float * pDelays )
{
    float Delay = 0.0;
    int i;
    for ( i = 0; i < g->nVars; i++ )
        Delay = Abc_MaxFloat( Delay, pDelays[(int)g->pVars[i]] );
    return Delay;
}

// returns delay of the decomposition;  sets area of the cut as its cost
float If_CutDelayLutStruct( If_Man_t * p, If_Cut_t * pCut, char * pStr, float WireDelay )
{
    float Delays[CLU_VAR_MAX+2];
    int fUsed[CLU_VAR_MAX+2] = {0};
    If_Obj_t * pLeaf;
    If_Grp_t G1 = {0}, G2 = {0}, G3 = {0};
    int nLeaves = If_CutLeaveNum(pCut);
    int i, nLutLeaf, nLutRoot;
    // mark the cut as user cut
//    pCut->fUser = 1;
    // quit if parameters are wrong
    if ( strlen(pStr) != 2 )
    {
        printf( "Wrong LUT struct (%s)\n", pStr );
        return ABC_INFINITY;
    }
    nLutLeaf = pStr[0] - '0';
    if ( nLutLeaf < 3 || nLutLeaf > 6 )
    {
        printf( "Leaf size (%d) should belong to {3,4,5,6}.\n", nLutLeaf );
        return ABC_INFINITY;
    }
    nLutRoot = pStr[1] - '0';
    if ( nLutRoot < 3 || nLutRoot > 6 )
    {
        printf( "Root size (%d) should belong to {3,4,5,6}.\n", nLutRoot );
        return ABC_INFINITY;
    }
    if ( nLeaves > nLutLeaf + nLutRoot - 1 )
    {
        printf( "The cut size (%d) is too large for the LUT structure %d%d.\n", If_CutLeaveNum(pCut), nLutLeaf, nLutRoot );
        return ABC_INFINITY;
    }

    // remember the delays
    If_CutForEachLeaf( p, pCut, pLeaf, i )
        Delays[i] = If_ObjCutBest(pLeaf)->Delay;

    // consider easy case
    if ( nLeaves <= Abc_MaxInt( nLutLeaf, nLutRoot ) )
    {
        char * pPerm = If_CutPerm( pCut );
        assert( nLeaves <= 6 );
        for ( i = 0; i < nLeaves; i++ )
        {
            pPerm[i] = 1;
            G1.pVars[i] = i;
        }
        G1.nVars = nLeaves;
        return 1.0 + If_CluDelayMax( &G1, Delays );
    }

    // derive the first group
    G1 = If_CluCheck( p, If_CutTruthW(p, pCut), nLeaves, 0, 0, nLutLeaf, nLutRoot, NULL, NULL, NULL, NULL, 1 );
    if ( G1.nVars == 0 )
        return ABC_INFINITY;

    // compute the delay
    Delays[nLeaves] = If_CluDelayMax( &G1, Delays ) + ((WireDelay == 0.0)?1.0:WireDelay);
    if ( G2.nVars )
        Delays[nLeaves+1] = If_CluDelayMax( &G2, Delays ) + ((WireDelay == 0.0)?1.0:WireDelay);

    // mark used groups
    for ( i = 0; i < G1.nVars; i++ )
        fUsed[(int)G1.pVars[i]] = 1;
    for ( i = 0; i < G2.nVars; i++ )
        fUsed[(int)G2.pVars[i]] = 1;
    // mark unused groups
    assert( G1.nMyu >= 2 && G1.nMyu <= 4 );
    if ( G1.nMyu > 2 )
        fUsed[(int)G1.pVars[G1.nVars-1]] = 0;
    assert( !G2.nVars || (G2.nMyu >= 2 && G2.nMyu <= 4) );
    if ( G2.nMyu > 2 )
        fUsed[(int)G2.pVars[G2.nVars-1]] = 0;

    // create remaining group
    assert( G3.nVars == 0 );
    for ( i = 0; i < nLeaves; i++ )
        if ( !fUsed[i] )
            G3.pVars[(int)G3.nVars++] = i;
    G3.pVars[(int)G3.nVars++] = nLeaves;
    if ( G2.nVars )
        G3.pVars[(int)G3.nVars++] = nLeaves+1;
    assert( G1.nVars + G2.nVars + G3.nVars == nLeaves + 
        (G1.nVars > 0) + (G2.nVars > 0) + (G1.nMyu > 2) + (G2.nMyu > 2) );
    // what if both non-disjoint vars are the same???

    pCut->Cost = 2 + (G2.nVars > 0);
    return 1.0 + If_CluDelayMax( &G3, Delays );
}

//#define IF_TRY_NEW

#ifdef IF_TRY_NEW
static Vec_Mem_t * s_vTtMem = NULL;
static Vec_Mem_t * s_vTtMem2 = NULL;
int If_TtMemCutNum()  { return Vec_MemEntryNum(s_vTtMem); }
int If_TtMemCutNum2() { return Vec_MemEntryNum(s_vTtMem2); }
//        printf( "Unique TTs = %d.  Unique classes = %d.    ", If_TtMemCutNum(), If_TtMemCutNum2() );
//        printf( "Check2 = %d.  Check3 = %d.\n", s_Count2, s_Count3 );
#endif

/**Function*************************************************************

  Synopsis    [Performs additional check.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_CutPerformCheck16( If_Man_t * p, unsigned * pTruth0, int nVars, int nLeaves, char * pStr )
{
    unsigned pTruth[IF_MAX_FUNC_LUTSIZE > 5 ? 1 << (IF_MAX_FUNC_LUTSIZE - 5) : 1];
    If_Grp_t G1 = {0};//, G3 = {0};
    int i, nLutLeaf, nLutLeaf2, nLutRoot, Length;
    // stretch the truth table
    assert( nVars >= 6 );
    memcpy( pTruth, pTruth0, sizeof(word) * Abc_TtWordNum(nVars) );
    Abc_TtStretch6( (word *)pTruth, nLeaves, p->pPars->nLutSize );

#ifdef IF_TRY_NEW
    {
        word pCopy[1024];
        int nWords = Abc_TruthWordNum(nVars);
        int iNum;
        if ( s_vTtMem == NULL )
        {
            s_vTtMem = Vec_MemAlloc( Abc_Truth6WordNum(nVars), 12 ); // 32 KB/page for 6-var functions
            Vec_MemHashAlloc( s_vTtMem, 10000 );
        }
        if ( s_vTtMem2 == NULL )
        {
            s_vTtMem2 = Vec_MemAlloc( Abc_Truth6WordNum(nVars), 12 ); // 32 KB/page for 6-var functions
            Vec_MemHashAlloc( s_vTtMem2, 10000 );
        }
        memcpy( pCopy, pTruth, sizeof(word) * Abc_Truth6WordNum(nVars) );
        if ( pCopy[0] & 1 )
            for ( i = 0; i < nWords; i++ )
                pCopy[i] = ~pCopy[i];
        iNum = Vec_MemHashInsert( s_vTtMem, pCopy );
        if ( iNum == Vec_MemEntryNum(s_vTtMem) - 1 )
        {
            int pCanonPerm[16];
            char pCanonPermC[16];
            Abc_TtCanonicize( pCopy, nVars, pCanonPermC );
//            If_CluSemiCanonicize( pCopy, nVars, pCanonPerm );
            Vec_MemHashInsert( s_vTtMem2, pCopy );
        }
    }
#endif

    // if cutmin is disabled, minimize the function
    if ( !p->pPars->fCutMin )
        nLeaves = Abc_TtMinBase( (word *)pTruth, NULL, nLeaves, nVars );

    // quit if parameters are wrong
    Length = strlen(pStr);
    if ( Length != 2 && Length != 3 )
    {
        printf( "Wrong LUT struct (%s)\n", pStr );
        return 0;
    }
    for ( i = 0; i < Length; i++ )
        if ( pStr[i] - '0' < 3 || pStr[i] - '0' > 6 )
        {
            printf( "The LUT size (%d) should belong to {3,4,5,6}.\n", pStr[i] - '0' );
            return 0;
        }

    nLutLeaf  =                   pStr[0] - '0';
    nLutLeaf2 = ( Length == 3 ) ? pStr[1] - '0' : 0;
    nLutRoot  =                   pStr[Length-1] - '0';
    if ( nLeaves > nLutLeaf - 1 + (nLutLeaf2 ? nLutLeaf2 - 1 : 0) + nLutRoot )
    {
        printf( "The cut size (%d) is too large for the LUT structure %s.\n", nLeaves, pStr );
        return 0;
    }
    // consider easy case
    if ( nLeaves <= Abc_MaxInt( nLutLeaf2, Abc_MaxInt(nLutLeaf, nLutRoot) ) )
        return 1;

    // derive the first group
    if ( Length == 2 )
        G1 = If_CluCheck( p, (word *)pTruth, nLeaves, 0, 0, nLutLeaf, nLutRoot, NULL, NULL, NULL, NULL, 1 );
    else
        G1 = If_CluCheck3( p, (word *)pTruth, nLeaves, nLutLeaf, nLutLeaf2, nLutRoot, NULL, NULL, NULL, NULL, NULL );

//    if ( G1.nVars > 0 )
//        If_CluPrintGroup( &G1 );

    return (int)(G1.nVars > 0);
}


// testing procedure
void If_CluTest()
{
//    word t = 0xff00f0f0ccccaaaa;
//    word t = 0xfedcba9876543210;
//    word t = 0xec64000000000000;
//    word t = 0x0100200000000001;
//    word t2[4] = { 0x0000800080008000, 0x8000000000008000, 0x8000000080008000, 0x0000000080008000 };
//    word t = 0x07770FFF07770FFF;
//    word t = 0x002000D000D00020;
//    word t = 0x000F000E000F000F;
//    word t = 0xF7FFF7F7F7F7F7F7;
//    word t = 0x0234AFDE23400BCE;
//    word t = 0x0080008880A088FF;
//    word s = t;
//    word t = 0xFFBBBBFFF0B0B0F0;
    word t = ABC_CONST(0x6DD9926D962D6996);
    int nVars     = 6;
    int nLutLeaf  = 4;
    int nLutLeaf2 = 4;
    int nLutRoot  = 4;
/*
    word t2[2] = { 0x7f807f807f80807f, 0x7f807f807f807f80 };
    int nVars     = 7;
    int nLutLeaf  = 3;
    int nLutLeaf2 = 3;
    int nLutRoot  = 3;
*/

    If_Grp_t G;
    If_Grp_t G2, R;
    word Func0, Func1, Func2;


    return;

/*
    int pCanonPerm[CLU_VAR_MAX];
    int uCanonPhase;

    Kit_DsdPrintFromTruth( (unsigned*)&s, nVars ); printf( "\n" );
    uCanonPhase = If_CluSemiCanonicize( &s, nVars, pCanonPerm );
    Kit_DsdPrintFromTruth( (unsigned*)&s, nVars ); printf( "\n" );

    If_CluSemiCanonicizeVerify( &s, &t, nVars, pCanonPerm, uCanonPhase );
*/

    Kit_DsdPrintFromTruth( (unsigned*)&t, nVars ); printf( "\n" );
    G = If_CluCheck3( NULL, &t, nVars, nLutLeaf, nLutLeaf2, nLutRoot, &R, &G2, &Func0, &Func1, &Func2 );
    If_CluPrintGroup( &G );
    If_CluPrintGroup( &G2 );
    If_CluPrintGroup( &R );

//    If_CluVerify3( &t, nVars, &G, &G2, &R, Func1, Func2, Func0 );

    return;

//    If_CutPerformCheck07( NULL, (unsigned *)&t, 6, 6, NULL );
//    return;

//    Kit_DsdPrintFromTruth( (unsigned*)&t, nVars ); printf( "\n" );
//    G = If_CluCheck( NULL, &t, nVars, 0, nLutLeaf, nLutRoot, NULL, NULL, NULL, NULL, 0 );
//    If_CluPrintGroup( &G );
}




////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

