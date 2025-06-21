/**CFile****************************************************************

  FileName    [abcNpn.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Procedures for testing and comparing semi-canonical forms.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcNpn.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "misc/extra/extra.h"
#include "misc/vec/vec.h"

#include "bool/kit/kit.h"
#include "bool/lucky/lucky.h"
#include "opt/dau/dau.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
 
// semi-canonical form types
// 0 - none
// 1 - based on counting 1s in cofactors
// 2 - based on minimum truth table value
// 3 - exact NPN

// data-structure to store a bunch of truth tables
typedef struct Abc_TtStore_t_  Abc_TtStore_t;
struct Abc_TtStore_t_ 
{
    int                nVars;
    int                nWords;
    int                nFuncs;
    word **            pFuncs;
};

extern Abc_TtStore_t * Abc_TtStoreLoad( char * pFileName, int nVarNum );
extern void            Abc_TtStoreFree( Abc_TtStore_t * p, int nVarNum );
extern void            Abc_TtStoreWrite( char * pFileName, Abc_TtStore_t * p, int fBinary );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Counts the number of unique truth tables.]

  Description []
               
  SideEffects [] 

  SeeAlso     []

***********************************************************************/
// returns hash key of the truth table
static inline int Abc_TruthHashKey( word * pFunc, int nWords, int nTableSize )
{
    static unsigned s_BigPrimes[7] = {12582917, 25165843, 50331653, 100663319, 201326611, 402653189, 805306457};
    int w;
    word Key = 0;
    for ( w = 0; w < nWords; w++ )
        Key += pFunc[w] * s_BigPrimes[w % 7];
    return (int)(Key % nTableSize);
}
// returns 1 if the entry with this truth table exits
static inline int Abc_TruthHashLookup( word ** pFuncs, int iThis, int nWords, int * pTable, int * pNexts, int Key )
{
    int iThat;
    for ( iThat = pTable[Key]; iThat != -1; iThat = pNexts[iThat] )
        if ( !memcmp( pFuncs[iThat], pFuncs[iThis], sizeof(word) * nWords ) )
            return 1;
    return 0;
}
// hashes truth tables and collects unique ones
int Abc_TruthNpnCountUnique( Abc_TtStore_t * p )
{
    // allocate hash table
    int nTableSize = Abc_PrimeCudd(p->nFuncs);
    int * pTable = ABC_FALLOC( int, nTableSize );
    int * pNexts = ABC_FALLOC( int, nTableSize );
    // hash functions
    int i, k, Key;
    for ( i = 0; i < p->nFuncs; i++ )
    {
        Key = Abc_TruthHashKey( p->pFuncs[i], p->nWords, nTableSize );
        if ( Abc_TruthHashLookup( p->pFuncs, i, p->nWords, pTable, pNexts, Key ) ) // found equal
            p->pFuncs[i] = NULL;
        else // there is no equal (the first time this one occurs so far)
            pNexts[i] = pTable[Key], pTable[Key] = i;
    }
    ABC_FREE( pTable );
    ABC_FREE( pNexts );
    // count the number of unqiue functions
    assert( p->pFuncs[0] != NULL );
    for ( i = k = 1; i < p->nFuncs; i++ )
        if ( p->pFuncs[i] != NULL )
            p->pFuncs[k++] = p->pFuncs[i];
    return (p->nFuncs = k);
}

/**Function*************************************************************

  Synopsis    [Counts the number of unique truth tables.]

  Description []
               
  SideEffects [] 

  SeeAlso     []

***********************************************************************/
int nWords = 0; // unfortunate global variable
int Abc_TruthCompare( word ** p1, word ** p2 ) { return memcmp(*p1, *p2, sizeof(word) * nWords); }
int Abc_TruthNpnCountUniqueSort( Abc_TtStore_t * p )
{
    int i, k;
    // sort them by value
    nWords = p->nWords;
    assert( nWords > 0 );
    qsort( (void *)p->pFuncs, (size_t)p->nFuncs, sizeof(word *), (int(*)(const void *,const void *))Abc_TruthCompare );
    // count the number of unqiue functions
    for ( i = k = 1; i < p->nFuncs; i++ )
        if ( memcmp( p->pFuncs[i-1], p->pFuncs[i], sizeof(word) * nWords ) )
            p->pFuncs[k++] = p->pFuncs[i];
    return (p->nFuncs = k);
}

/**Function*************************************************************

  Synopsis    [Prints out one NPN transform.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_TruthNpnPrint( char * pCanonPermInit, unsigned uCanonPhase, int nVars )
{
    char pCanonPerm[16]; int i;
    assert( nVars <= 16 );
    for ( i = 0; i < nVars; i++ )
        pCanonPerm[i] = pCanonPermInit ? pCanonPermInit[i] : 'a' + i;
    printf( "   %c = ( ", Abc_InfoHasBit(&uCanonPhase, nVars) ? 'Z':'z' );
    for ( i = 0; i < nVars; i++ )
        printf( "%c%s", pCanonPerm[i] + ('A'-'a') * Abc_InfoHasBit(&uCanonPhase, pCanonPerm[i]-'a'), i == nVars-1 ? "":"," );
    printf( " )  " );
}

/**Function*************************************************************

  Synopsis    [Apply decomposition to the truth table.]

  Description [Returns the number of AIG nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_TruthNpnPerform( Abc_TtStore_t * p, int NpnType, int fVerbose )
{
    unsigned pAux[2048];
    word pAuxWord[1024], pAuxWord1[1024];
    char pCanonPerm[16];
    unsigned uCanonPhase=0;
    abctime clk = Abc_Clock();
    int i;

    char * pAlgoName = NULL;
    if ( NpnType == 0 )
        pAlgoName = "uniqifying          ";
    else if ( NpnType == 1 )
        pAlgoName = "exact NPN           ";
    else if ( NpnType == 2 )
        pAlgoName = "counting 1s         ";
    else if ( NpnType == 3 )
        pAlgoName = "Jake's hybrid fast  ";
    else if ( NpnType == 4 )
        pAlgoName = "Jake's hybrid good  ";
    else if ( NpnType == 5 )
        pAlgoName = "new hybrid fast     ";
    else if ( NpnType == 6 )
        pAlgoName = "new phase flipping  ";
    else if ( NpnType == 7 )
        pAlgoName = "new hier. matching  ";
    else if ( NpnType == 8 )
        pAlgoName = "new adap. matching  ";
    else if ( NpnType == 9 )
        pAlgoName = "adjustable algorithm (heuristic) ";
    else if ( NpnType == 10 )
        pAlgoName = "adjustable algorithm (exact)     ";
    else if ( NpnType == 11 )
        pAlgoName = "new cost-aware exact algorithm   ";
    else if ( NpnType == 12 )
        pAlgoName = "new hybrid fast (P) ";

    assert( p->nVars <= 16 );
    if ( pAlgoName )
        printf( "Applying %-20s to %8d func%s of %2d vars...  ",  
            pAlgoName, p->nFuncs, (p->nFuncs == 1 ? "":"s"), p->nVars );
    if ( fVerbose )
        printf( "\n" );

    if ( NpnType == 0 )
    {
        for ( i = 0; i < p->nFuncs; i++ )
        {
            if ( fVerbose )
                printf( "%7d : ", i );
            if ( fVerbose )
                Extra_PrintHex( stdout, (unsigned *)p->pFuncs[i], p->nVars ), printf( "\n" );
        }
    }
    else if ( NpnType == 1 )
    {
        permInfo* pi; 
        Abc_TruthNpnCountUnique(p);
        pi = setPermInfoPtr(p->nVars);
        for ( i = 0; i < p->nFuncs; i++ )
        {
            if ( fVerbose )
                printf( "%7d : ", i );
            simpleMinimal(p->pFuncs[i], pAuxWord, pAuxWord1, pi, p->nVars);
            if ( fVerbose )
                Extra_PrintHex( stdout, (unsigned *)p->pFuncs[i], p->nVars ), Abc_TruthNpnPrint(pCanonPerm, uCanonPhase, p->nVars), printf( "\n" );
        }
        freePermInfoPtr(pi);
    }
    else if ( NpnType == 2 )
    {
        for ( i = 0; i < p->nFuncs; i++ )
        {
            if ( fVerbose )
                printf( "%7d : ", i );
            resetPCanonPermArray(pCanonPerm, p->nVars);
            uCanonPhase = Kit_TruthSemiCanonicize( (unsigned *)p->pFuncs[i], pAux, p->nVars, pCanonPerm );
            if ( fVerbose )
                Extra_PrintHex( stdout, (unsigned *)p->pFuncs[i], p->nVars ), Abc_TruthNpnPrint(pCanonPerm, uCanonPhase, p->nVars), printf( "\n" );
        }
    }
    else if ( NpnType == 3 )
    {
        for ( i = 0; i < p->nFuncs; i++ )
        {
            if ( fVerbose )
                printf( "%7d : ", i );
            resetPCanonPermArray(pCanonPerm, p->nVars);
            uCanonPhase = luckyCanonicizer_final_fast( p->pFuncs[i], p->nVars, pCanonPerm );
            if ( fVerbose )
                Extra_PrintHex( stdout, (unsigned *)p->pFuncs[i], p->nVars ), Abc_TruthNpnPrint(pCanonPerm, uCanonPhase, p->nVars), printf( "\n" );
        }
    }
    else if ( NpnType == 4 )
    {
        for ( i = 0; i < p->nFuncs; i++ )
        {
            if ( fVerbose )
                printf( "%7d : ", i );
            resetPCanonPermArray(pCanonPerm, p->nVars);
            uCanonPhase = luckyCanonicizer_final_fast1( p->pFuncs[i], p->nVars, pCanonPerm );
            if ( fVerbose )
                Extra_PrintHex( stdout, (unsigned *)p->pFuncs[i], p->nVars ), Abc_TruthNpnPrint(pCanonPerm, uCanonPhase, p->nVars), printf( "\n" );
        }
    }
    else if ( NpnType == 5 )
    {
        for ( i = 0; i < p->nFuncs; i++ )
        {
            if ( fVerbose )
                printf( "%7d : ", i );
            uCanonPhase = Abc_TtCanonicize( p->pFuncs[i], p->nVars, pCanonPerm );
            if ( fVerbose )
                Extra_PrintHex( stdout, (unsigned *)p->pFuncs[i], p->nVars ), Abc_TruthNpnPrint(pCanonPerm, uCanonPhase, p->nVars), printf( "\n" );
        }
    }
    else if ( NpnType == 6 )
    {
        for ( i = 0; i < p->nFuncs; i++ )
        {
            if ( fVerbose )
                printf( "%7d : ", i );
            uCanonPhase = Abc_TtCanonicizePhase( p->pFuncs[i], p->nVars );
            if ( fVerbose )
                Extra_PrintHex( stdout, (unsigned *)p->pFuncs[i], p->nVars ), Abc_TruthNpnPrint(NULL, uCanonPhase, p->nVars), printf( "\n" );
        }
    }
    else if ( NpnType == 7 )
    {
        int fExact = 0;
		Abc_TtHieMan_t * pMan = Abc_TtHieManStart( p->nVars, 5 );
        for ( i = 0; i < p->nFuncs; i++ )
        {
            if ( fVerbose )
                printf( "%7d : ", i );
            uCanonPhase = Abc_TtCanonicizeHie( pMan, p->pFuncs[i], p->nVars, pCanonPerm, fExact );
            if ( fVerbose )
//                Extra_PrintHex( stdout, (unsigned *)p->pFuncs[i], p->nVars ), Abc_TruthNpnPrint(NULL, uCanonPhase, p->nVars), printf( "\n" );
                printf( "\n" );
        }
        // nClasses = Abc_TtManNumClasses( pMan );
		Abc_TtHieManStop( pMan );
    }
    else if ( NpnType == 8 )
    {
//        typedef unsigned(*TtCanonicizeFunc)(Abc_TtHieMan_t * p, word * pTruth, int nVars, char * pCanonPerm, int flag);
        unsigned Abc_TtCanonicizeWrap(TtCanonicizeFunc func, Abc_TtHieMan_t * p, word * pTruth, int nVars, char * pCanonPerm, int flag);
        unsigned Abc_TtCanonicizeAda(Abc_TtHieMan_t * p, word * pTruth, int nVars, char * pCanonPerm, int iThres);
		
		int fHigh = 1, iEnumThres = 25;
		Abc_TtHieMan_t * pMan = Abc_TtHieManStart(p->nVars, 5);
		for ( i = 0; i < p->nFuncs; i++ )
        {
            if ( fVerbose )
                printf( "%7d : ", i );
            uCanonPhase = Abc_TtCanonicizeWrap(Abc_TtCanonicizeAda, pMan, p->pFuncs[i], p->nVars, pCanonPerm, fHigh*100 + iEnumThres);
            if ( fVerbose )
                Extra_PrintHex( stdout, (unsigned *)p->pFuncs[i], p->nVars ), Abc_TruthNpnPrint(pCanonPerm, uCanonPhase, p->nVars), printf( "\n" );
        }
		Abc_TtHieManStop(pMan);
    }
    else if ( NpnType == 9 || NpnType == 10 || NpnType == 11 )
    {
//        typedef unsigned(*TtCanonicizeFunc)(Abc_TtHieMan_t * p, word * pTruth, int nVars, char * pCanonPerm, int flag);
        unsigned Abc_TtCanonicizeWrap(TtCanonicizeFunc func, Abc_TtHieMan_t * p, word * pTruth, int nVars, char * pCanonPerm, int flag);
        unsigned Abc_TtCanonicizeAda(Abc_TtHieMan_t * p, word * pTruth, int nVars, char * pCanonPerm, int iThres);
        unsigned Abc_TtCanonicizeCA(Abc_TtHieMan_t * p, word * pTruth, int nVars, char * pCanonPerm, int iThres);
		
		Abc_TtHieMan_t * pMan = Abc_TtHieManStart(p->nVars, 5);
		for ( i = 0; i < p->nFuncs; i++ )
        {
            if ( fVerbose )
                printf( "%7d : ", i );
            if ( NpnType == 9 )
                uCanonPhase = Abc_TtCanonicizeWrap(Abc_TtCanonicizeAda, pMan, p->pFuncs[i], p->nVars, pCanonPerm,  125); // -A 8, adjustable algorithm (heuristic)
            else if ( NpnType == 10 )
                uCanonPhase = Abc_TtCanonicizeWrap(Abc_TtCanonicizeAda, pMan, p->pFuncs[i], p->nVars, pCanonPerm, 1199); // -A 9, adjustable algorithm (exact)
            else if ( NpnType == 11 )
                uCanonPhase = Abc_TtCanonicizeWrap(Abc_TtCanonicizeCA,  pMan, p->pFuncs[i], p->nVars, pCanonPerm,    1); // -A 10, new cost-aware exact algorithm
            if ( fVerbose )
                Extra_PrintHex( stdout, (unsigned *)p->pFuncs[i], p->nVars ), Abc_TruthNpnPrint(pCanonPerm, uCanonPhase, p->nVars), printf( "\n" );
        }
		Abc_TtHieManStop(pMan);
    }
    else if ( NpnType == 12 )
    {
        for ( i = 0; i < p->nFuncs; i++ )
        {
            if ( fVerbose )
                printf( "%7d : ", i );
            uCanonPhase = Abc_TtCanonicizePerm( p->pFuncs[i], p->nVars, pCanonPerm );
            if ( fVerbose )
                Extra_PrintHex( stdout, (unsigned *)p->pFuncs[i], p->nVars ), Abc_TruthNpnPrint(pCanonPerm, uCanonPhase, p->nVars), printf( "\n" );
        }
    }
    else assert( 0 );
    clk = Abc_Clock() - clk;
    printf( "Classes =%9d  ", Abc_TruthNpnCountUnique(p) );
    Abc_PrintTime( 1, "Time", clk );
}

/**Function*************************************************************

  Synopsis    [Apply decomposition to truth tables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_TruthNpnTest( char * pFileName, int NpnType, int nVarNum, int fDumpRes, int fBinary, int fVerbose )
{
    Abc_TtStore_t * p;
    char * pFileNameOut;

    // read info from file
    p = Abc_TtStoreLoad( pFileName, nVarNum );
    if ( p == NULL )
        return;

    // consider functions from the file
    Abc_TruthNpnPerform( p, NpnType, fVerbose );

    // write the result
    if ( fDumpRes )
    {
        if ( fBinary )
            pFileNameOut = Extra_FileNameGenericAppend( pFileName, "_out.tt" );
        else
            pFileNameOut = Extra_FileNameGenericAppend( pFileName, "_out.txt" );
        Abc_TtStoreWrite( pFileNameOut, p, fBinary );
        if ( fVerbose )
            printf( "The resulting functions are written into file \"%s\".\n", pFileNameOut );
    }

    // delete data-structure
    Abc_TtStoreFree( p, nVarNum );
//    printf( "Finished computing canonical forms for functions from file \"%s\".\n", pFileName );
}


/**Function*************************************************************

  Synopsis    [Testbench for decomposition algorithms.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NpnTest( char * pFileName, int NpnType, int nVarNum, int fDumpRes, int fBinary, int fVerbose )
{
    if ( fVerbose )
        printf( "Using truth tables from file \"%s\"...\n", pFileName );
    if ( NpnType >= 0 && NpnType <= 12 )
        Abc_TruthNpnTest( pFileName, NpnType, nVarNum, fDumpRes, fBinary, fVerbose );
    else
        printf( "Unknown canonical form value (%d).\n", NpnType );
    fflush( stdout );
    return 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

