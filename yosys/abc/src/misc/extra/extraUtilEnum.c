/**CFile****************************************************************

  FileName    [extraUtilEnum.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [extra]

  Synopsis    [Function enumeration.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: extraUtilEnum.c,v 1.0 2003/02/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "misc/vec/vec.h"
#include "misc/vec/vecHsh.h"
#include "bool/kit/kit.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_GetFirst( int * pnVars, int * pnMints, int * pnFuncs, unsigned * pVars, unsigned * pMints, unsigned * pFuncs )
{
    int nVars  = 8;
    int nMints = 16;
    int nFuncs = 8;
    char * pMintStrs[16] = { 
        "1-1-1-1-",
        "1-1--11-",
        "1-1-1--1",
        "1-1--1-1",

        "-11-1-1-",
        "-11--11-",
        "-11-1--1",
        "-11--1-1",

        "1--11-1-",
        "1--1-11-",
        "1--11--1",
        "1--1-1-1",

        "-1-11-1-",
        "-1-1-11-",
        "-1-11--1",
        "-1-1-1-1"
    };
    char * pFuncStrs[8] = { 
        "1111101011111010",
        "0000010100000101",
        "1111110010101001",
        "0000001101010110",
        "1111111111001101",
        "0000000000110010",
        "1111111111111110",
        "0000000000000001",
    };
    int i, k;
    *pnVars  = nVars;
    *pnMints = nMints;
    *pnFuncs = nFuncs;
    // extract mints
    for ( i = 0; i < nMints; i++ )
        for ( k = 0; k < nVars; k++ )
            if ( pMintStrs[i][k] == '1' )
                pMints[i] |= (1 << k), pVars[k] |= (1 << i);
    // extract funcs
    for ( i = 0; i < nFuncs; i++ )
        for ( k = 0; k < nMints; k++ )
            if ( pFuncStrs[i][k] == '1' )
                pFuncs[i] |= (1 << k);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_GetSecond( int * pnVars, int * pnMints, int * pnFuncs, unsigned * pVars, unsigned * pMints, unsigned * pFuncs )
{
    int nVars  = 10;
    int nMints = 32;
    int nFuncs = 7;
    char * pMintStrs[32] = { 
        "1-1---1---",
        "1-1----1--",
        "1-1-----1-",
        "1-1------1",

        "1--1--1---",
        "1--1---1--",
        "1--1----1-",
        "1--1-----1",

        "1---1-1---",
        "1---1--1--",
        "1---1---1-",
        "1---1----1",

        "1----11---",
        "1----1-1--",
        "1----1--1-",
        "1----1---1",


        "-11---1---",
        "-11----1--",
        "-11-----1-",
        "-11------1",

        "-1-1--1---",
        "-1-1---1--",
        "-1-1----1-",
        "-1-1-----1",

        "-1--1-1---",
        "-1--1--1--",
        "-1--1---1-",
        "-1--1----1",

        "-1---11---",
        "-1---1-1--",
        "-1---1--1-",
        "-1---1---1"
    };
    char * pFuncStrs[7] = { 
        "11111110110010001110110010000000",
        "00000001001101110001001101111111",
        "10000001001001000001001001001000",
        "01001000000100101000000100100100",
        "00100100100000010100100000010010",
        "00010010010010000010010010000001",
        "11111111111111111111000000000000"
    };
    int i, k;
    *pnVars  = nVars;
    *pnMints = nMints;
    *pnFuncs = nFuncs;
    // extract mints
    for ( i = 0; i < nMints; i++ )
        for ( k = 0; k < nVars; k++ )
            if ( pMintStrs[i][k] == '1' )
                pMints[i] |= (1 << k), pVars[k] |= (1 << i);
    // extract funcs
    for ( i = 0; i < nFuncs; i++ )
        for ( k = 0; k < nMints; k++ )
            if ( pFuncStrs[i][k] == '1' )
                pFuncs[i] |= (1 << k);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_GetThird( int * pnVars, int * pnMints, int * pnFuncs, unsigned * pVars, unsigned * pMints, unsigned * pFuncs )
{
    int nVars  = 8;
    int nMints = 16;
    int nFuncs = 7;
    char * pMintStrs[16] = { 
        "1---1---",
        "1----1--",
        "1-----1-",
        "1------1",

        "-1--1---",
        "-1---1--",
        "-1----1-",
        "-1-----1",

        "--1-1---",
        "--1--1--",
        "--1---1-",
        "--1----1",

        "---11---",
        "---1-1--",
        "---1--1-",
        "---1---1"
    };
    char * pFuncStrs[7] = { 
        "1111111011001000",
        "0000000100110111",
        "1000000100100100",
        "0100100000010010",
        "0010010010000001",
        "0001001001001000",
        "1111111111111111"
    };
    int i, k;
    *pnVars  = nVars;
    *pnMints = nMints;
    *pnFuncs = nFuncs;
    // extract mints
    for ( i = 0; i < nMints; i++ )
        for ( k = 0; k < nVars; k++ )
            if ( pMintStrs[i][k] == '1' )
                pMints[i] |= (1 << k), pVars[k] |= (1 << i);
    // extract funcs
    for ( i = 0; i < nFuncs; i++ )
        for ( k = 0; k < nMints; k++ )
            if ( pFuncStrs[i][k] == '1' )
                pFuncs[i] |= (1 << k);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_EnumPrint_rec( Vec_Int_t * vGates, int i, int nVars )
{
    int Fan0 = Vec_IntEntry(vGates, 2*i);
    int Fan1 = Vec_IntEntry(vGates, 2*i+1);
    char * pOper = (char*)(Fan0 < Fan1 ? "" : "+");
    if ( Fan0 > Fan1 )
        ABC_SWAP( int, Fan0, Fan1 );
    if ( Fan0 < nVars )
        printf( "%c", 'a'+Fan0 );
    else
    {
        printf( "(" );
        Abc_EnumPrint_rec( vGates, Fan0, nVars );
        printf( ")" );
    }
    printf( "%s", pOper );
    if ( Fan1 < nVars )
        printf( "%c", 'a'+Fan1 );
    else
    {
        printf( "(" );
        Abc_EnumPrint_rec( vGates, Fan1, nVars );
        printf( ")" );
    }
}
void Abc_EnumPrint( Vec_Int_t * vGates, int i, int nVars )
{
    assert( 2*i < Vec_IntSize(vGates) );
    Abc_EnumPrint_rec( vGates, i, nVars );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int  Abc_DataHasBit( word * p, word i )  { return (p[(i)>>6] & (((word)1)<<((i) & 63))) > 0; }
static inline void Abc_DataXorBit( word * p, word i )  { p[(i)>>6] ^= (((word)1)<<((i) & 63));             }

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_EnumerateFunctions( int nDecMax )
{
    int nVars;
    int nMints;
    int nFuncs;
    unsigned pVars[100] = {0};
    unsigned pMints[100] = {0};
    unsigned pFuncs[100] = {0};
    unsigned Truth;
    int FuncDone[100] = {0}, nFuncDone = 0;
    int GateCount[100] = {0};
    int i, k, n, a, b, v;
    abctime clk = Abc_Clock();
    Vec_Int_t * vGates = Vec_IntAlloc( 100000 );
    Vec_Int_t * vTruths = Vec_IntAlloc( 100000 );
//    Vec_Int_t * vHash = Vec_IntStartFull( 1 << 16 );
    word * pHash;

    // extract data
//    Abc_GetFirst( &nVars, &nMints, &nFuncs, pVars, pMints, pFuncs );
    Abc_GetSecond( &nVars, &nMints, &nFuncs, pVars, pMints, pFuncs );
//    Abc_GetThird( &nVars, &nMints, &nFuncs, pVars, pMints, pFuncs );

    // create hash table
    assert( nMints == 16 || nMints == 32 );
    pHash = (word *)ABC_CALLOC( char, 1 << (nMints-3) );

    // create elementary gates
    for ( k = 0; k < nVars; k++ )
    {
//        Vec_IntWriteEntry( vHash, pVars[k], k );
        Abc_DataXorBit( pHash, pVars[k] );
        Vec_IntPush( vTruths, pVars[k] );
        Vec_IntPush( vGates, -1 );
        Vec_IntPush( vGates, -1 );
    }

    // go through different number of variables
    GateCount[0] = 0;
    GateCount[1] = nVars;
    assert( Vec_IntSize(vTruths) == nVars );
    for ( n = 0; n < nDecMax && nFuncDone < nFuncs; n++ )
    {
        for ( a = 0; a <= n; a++ )
        for ( b = a; b <= n; b++ )
        if ( a + b == n )
        {
            printf( "Trying %d + %d + 1 = %d\n", a, b, n+1 );
            for ( i = GateCount[a]; i < GateCount[a+1]; i++ )
            for ( k = GateCount[b]; k < GateCount[b+1]; k++ )
            if ( i < k )
            {
                Truth = Vec_IntEntry(vTruths, i) & Vec_IntEntry(vTruths, k);
//                if ( Vec_IntEntry(vHash, Truth) == -1 )
                if ( !Abc_DataHasBit(pHash, Truth) )
                {
//                    Vec_IntWriteEntry( vHash, Truth, Vec_IntSize(vTruths) );
                    Abc_DataXorBit( pHash, Truth );
                    Vec_IntPush( vTruths, Truth );
                    Vec_IntPush( vGates, i );
                    Vec_IntPush( vGates, k );

                    for ( v = 0; v < nFuncs; v++ )
                    if ( !FuncDone[v] && Truth == pFuncs[v] )
                    {
                        printf( "Found function %d with %d gates: ", v, n+1 );
                        Abc_EnumPrint( vGates, Vec_IntSize(vTruths)-1, nVars );
                        FuncDone[v] = 1;
                        nFuncDone++;
                    }
                }
                Truth = Vec_IntEntry(vTruths, i) | Vec_IntEntry(vTruths, k);
//                if ( Vec_IntEntry(vHash, Truth) == -1 )
                if ( !Abc_DataHasBit(pHash, Truth) )
                {
//                    Vec_IntWriteEntry( vHash, Truth, Vec_IntSize(vTruths) );
                    Abc_DataXorBit( pHash, Truth );
                    Vec_IntPush( vTruths, Truth );
                    Vec_IntPush( vGates, k );
                    Vec_IntPush( vGates, i );

                    for ( v = 0; v < nFuncs; v++ )
                    if ( !FuncDone[v] && Truth == pFuncs[v] )
                    {
                        printf( "Found function %d with %d gates: ", v, n+1 );
                        Abc_EnumPrint( vGates, Vec_IntSize(vTruths)-1, nVars );
                        FuncDone[v] = 1;
                        nFuncDone++;
                    }
                }
            }
        }
        GateCount[n+2] = Vec_IntSize(vTruths);
        printf( "Finished %d gates.  Truths = %10d.  ", n+1, Vec_IntSize(vTruths) );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    }
    ABC_FREE( pHash );
//    Vec_IntFree( vHash );
    Vec_IntFree( vGates );
    Vec_IntFree( vTruths );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
#define ABC_ENUM_MAX 16
static word s_Truths6[6] = {
    ABC_CONST(0xAAAAAAAAAAAAAAAA),
    ABC_CONST(0xCCCCCCCCCCCCCCCC),
    ABC_CONST(0xF0F0F0F0F0F0F0F0),
    ABC_CONST(0xFF00FF00FF00FF00),
    ABC_CONST(0xFFFF0000FFFF0000),
    ABC_CONST(0xFFFFFFFF00000000)
};
typedef struct Abc_EnuMan_t_ Abc_EnuMan_t;
struct Abc_EnuMan_t_
{
    int              nVars;        // support size
    int              nVarsFree;    // number of PIs used
    int              fVerbose;     // verbose flag
    int              fUseXor;      // using XOR gate
    int              nNodeMax;     // the max number of nodes
    int              nNodes;       // current number of gates
    int              nTops;        // the number of fanoutless gates
    int              pFans0[ABC_ENUM_MAX];   // fanins
    int              pFans1[ABC_ENUM_MAX];   // fanins 
    int              fCompl0[ABC_ENUM_MAX];  // complements
    int              fCompl1[ABC_ENUM_MAX];  // complements
    int              Polar[ABC_ENUM_MAX];    // polarity
    int              pRefs[ABC_ENUM_MAX];    // references
    int              pLevel[ABC_ENUM_MAX];   // level 
    word             pTruths[ABC_ENUM_MAX];  // truth tables
    word             nTries;       // attempts to build a gate
    word             nBuilds;      // actually built gates
    word             nFinished;    // finished structures
};
static inline void Abc_EnumRef( Abc_EnuMan_t * p, int i )
{
    assert( p->pRefs[i] >= 0 );
    if ( p->pRefs[i]++ == 0 )
        p->nTops--;
}
static inline void Abc_EnumDeref( Abc_EnuMan_t * p, int i )
{
    if ( --p->pRefs[i] == 0 )
        p->nTops++;
    assert( p->pRefs[i] >= 0 );
}
static inline void Abc_EnumRefNode( Abc_EnuMan_t * p, int i )
{
    Abc_EnumRef( p, p->pFans0[i] );
    Abc_EnumRef( p, p->pFans1[i] );
    p->nTops++;
    p->nNodes++; 
    assert( i < p->nNodes );
}
static inline void Abc_EnumDerefNode( Abc_EnuMan_t * p, int i )
{
    assert( i < p->nNodes );
    Abc_EnumDeref( p, p->pFans0[i] );
    Abc_EnumDeref( p, p->pFans1[i] );
    p->nTops--;
    p->nNodes--; 
}
static inline void Abc_EnumPrintOne( Abc_EnuMan_t * p )
{
    int i;
    Kit_DsdPrintFromTruth( (unsigned *)(p->pTruths + p->nNodes - 1), p->nVars ); 
    for ( i = p->nVars; i < p->nNodes; i++ )
        if ( p->Polar[i] == 4 )
            printf( "  %c=%c+%c", 'a'+i, 'a'+p->pFans0[i], 'a'+p->pFans1[i] );
        else
            printf( "  %c=%s%c%s%c", 'a'+i, p->fCompl0[i]?"!":"", 'a'+p->pFans0[i], p->fCompl1[i]?"!":"", 'a'+p->pFans1[i] );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_EnumEquiv( word a, word b )
{
    return a == b || a == ~b;
}
static inline int Abc_EnumerateFilter( Abc_EnuMan_t * p )
{
    int fUseFull = 1;
    int n = p->nNodes;
    int i = p->pFans0[n];
    int k = p->pFans1[n], t;
    word * pTruths = p->pTruths;
    word uTruth = pTruths[n];
    assert( i < k );
    // skip constants
    if ( Abc_EnumEquiv(uTruth, 0) )
        return 1;
    // skip equal ones
    for ( t = 0; t < n; t++ )
        if ( Abc_EnumEquiv(uTruth, pTruths[t]) )
            return 1;
    if ( fUseFull )
    {
        // skip those that can be derived by any pair
        int a, b; 
        for ( a = 0; a <= i; a++ )
        for ( b = a + 1; b <= k; b++ )
        {
            if ( a == i && b == k )
                continue;
            if ( Abc_EnumEquiv(uTruth,  pTruths[a] &  pTruths[b]) )
                return 1;
            if ( Abc_EnumEquiv(uTruth,  pTruths[a] & ~pTruths[b]) )
                return 1;
            if ( Abc_EnumEquiv(uTruth, ~pTruths[a] &  pTruths[b]) )
                return 1;
            if ( Abc_EnumEquiv(uTruth, ~pTruths[a] & ~pTruths[b]) )
                return 1;
            if ( p->fUseXor && Abc_EnumEquiv(uTruth,  pTruths[a] ^ pTruths[b]) )
                return 1;
        }
    }
    else
    {
        // skip those that can be derived by fanin and any other one in the cone
        int uTruthI = p->fCompl0[n] ? ~pTruths[i] : pTruths[i];
        int uTruthK = p->fCompl1[n] ? ~pTruths[k] : pTruths[k];
        assert( p->fUseXor == 0 );
        for ( t = 0; t < k; t++ )
            if ( Abc_EnumEquiv(uTruth, pTruths[t] & uTruthI) || Abc_EnumEquiv(uTruth, ~pTruths[t] & uTruthI) )
                return 1;
        for ( t = 0; t < i; t++ )
            if ( Abc_EnumEquiv(uTruth, pTruths[t] & uTruthK) || Abc_EnumEquiv(uTruth, ~pTruths[t] & uTruthK) )
                return 1;
    }
    return 0;
}
void Abc_EnumerateFuncs_rec( Abc_EnuMan_t * p, int fNew, int iNode1st ) // the first node on the last level
{
    if ( p->nNodes == p->nNodeMax )
    {
        assert( p->nTops == 1 );
        if ( p->fVerbose )
            Abc_EnumPrintOne( p );
        p->nFinished++;
        return;
    }
    {
    int i, k, c, cLim = 4 + p->fUseXor, n = p->nNodes;
    int nRefedFans = p->nNodeMax - n + 1 - p->nTops;
    int high0 = fNew ? iNode1st : p->pFans1[n-1];
    int high1 = fNew ? n        : iNode1st;
    int low0  = fNew ? 0        : p->pFans0[n-1];
    int c0    = fNew ? 0        : p->Polar[n-1];
    int Level = p->pLevel[high0];
    assert( p->nTops > 0 && p->nTops <= p->nNodeMax - n + 1 );
    // go through nodes 
    for ( k = high0; k < high1; k++ )
    {
        if ( nRefedFans == 0 && p->pRefs[k] > 0 )
            continue;
        if ( p->pRefs[k] > 0 )
            nRefedFans--;
        assert( nRefedFans >= 0 );
        // try second fanin
        for ( i = (k == high0) ? low0 : 0; i < k; i++ )
        {
            if ( nRefedFans == 0 && p->pRefs[i] > 0 )
                continue;
            if ( Level == 0 && p->pRefs[i] == 0 && p->pRefs[k] == 0 && (i+1 != k || (i > 0 && p->pRefs[i-1] == 0)) ) // NPN
                continue;
            if ( p->pLevel[k] == 0 && p->pRefs[k] == 0 && p->pRefs[i] != 0 && k > 0 && p->pRefs[k-1] == 0 ) // NPN
                continue;
//            if ( p->pLevel[i] == 0 && p->pRefs[i] == 0 && p->pRefs[k] != 0 && i > 0 && p->pRefs[i-1] == 0 ) // NPN
//                continue;
            // try four polarities
            for ( c = (k == high0 && i == low0 && !fNew) ? c0 + 1 : 0; c < cLim; c++ )
            {
                if ( p->pLevel[i] == 0 && p->pRefs[i] == 0 && (c & 1) == 1 ) // NPN
                    continue;
                if ( p->pLevel[k] == 0 && p->pRefs[k] == 0 && (c & 2) == 2 ) // NPN
                    continue;
                p->nTries++;
                // create node
                assert( i < k );
                p->pFans0[n]  = i;
                p->pFans1[n]  = k;
                p->fCompl0[n] = c & 1;
                p->fCompl1[n] = (c >> 1) & 1;
                p->Polar[n]   = c;
                if ( c == 4 )
                    p->pTruths[n] = p->pTruths[i] ^ p->pTruths[k];
                else
                    p->pTruths[n] = ((c & 1) ? ~p->pTruths[i] : p->pTruths[i]) & ((c & 2) ? ~p->pTruths[k] : p->pTruths[k]);
                if ( Abc_EnumerateFilter(p) )
                    continue;
                p->nBuilds++;
                assert( Level == Abc_MaxInt(p->pLevel[i], p->pLevel[k]) );
                p->pLevel[n]  = Level + 1;
                Abc_EnumRefNode( p, n );
                Abc_EnumerateFuncs_rec( p, 0, fNew ? n : iNode1st );
                Abc_EnumDerefNode( p, n );
                assert( n == p->nNodes );
            }
        }
        if ( p->pRefs[k] > 0 )
            nRefedFans++;
    }
    if ( fNew )
        return;
    // start a new level
    Abc_EnumerateFuncs_rec( p, 1, iNode1st );
    }
}
void Abc_EnumerateFuncs( int nVars, int nGates, int fVerbose )
{
    abctime clk = Abc_Clock();
    Abc_EnuMan_t P, * p = &P; 
    int i;
    if ( nVars > nGates + 1 )
    {
        printf( "The gate count %d is not enough to have functions with %d inputs.\n", nGates, nVars );
        return;
    }
    assert( nVars >= 2 && nVars <= 6 );
    assert( nGates > 0 && nVars + nGates < ABC_ENUM_MAX );
    memset( p, 0, sizeof(Abc_EnuMan_t) );
    p->fVerbose  = fVerbose;
    p->fUseXor   = 0;
    p->nVars     = nVars;
    p->nNodeMax  = nVars + nGates;
    p->nNodes    = nVars;
    p->nTops     = nVars;
    for ( i = 0; i < nVars; i++ )
        p->pTruths[i] = s_Truths6[i];
    Abc_EnumerateFuncs_rec( p, 1, 0 );
    assert( p->nNodes == nVars );
    assert( p->nTops == nVars );
    // report statistics
    printf( "Vars = %d.  Gates = %d.  Tries = %u. Builds = %u.  Finished = %d. ", 
        nVars, nGates, (unsigned)p->nTries, (unsigned)p->nBuilds, (unsigned)p->nFinished );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

