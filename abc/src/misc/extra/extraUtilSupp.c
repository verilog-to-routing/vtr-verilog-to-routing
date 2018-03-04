/**CFile****************************************************************

  FileName    [extraUtilSupp.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [extra]

  Synopsis    [Support minimization.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: extraUtilSupp.c,v 1.0 2003/02/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "misc/vec/vec.h"
#include "misc/vec/vecWec.h"
#include "extra.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern void         Extra_PrintBinary( FILE * pFile, unsigned Sign[], int nBits );


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Generate m-out-of-n vectors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_SuppCountOnes( unsigned i )
{
    i = i - ((i >> 1) & 0x55555555);
    i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
    i = ((i + (i >> 4)) & 0x0F0F0F0F);
    return (i*(0x01010101))>>24;
}
Vec_Wrd_t * Abc_SuppGen( int m, int n )
{
    Vec_Wrd_t * vRes = Vec_WrdAlloc( 1000 );
    int i, Size = (1 << n);
    for ( i = 0; i < Size; i++ )
        if ( Abc_SuppCountOnes(i) == m )
            Vec_WrdPush( vRes, i );
    return vRes;
}

/**Function*************************************************************

  Synopsis    [Perform verification.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_SuppVerify( Vec_Wrd_t * p, word * pMatrix, int nVars, int nVarsMin )
{
    Vec_Wrd_t * pNew;
    word * pLimit, * pEntry1, * pEntry2;
    word Entry, EntryNew;
    int i, k, v, Value, Counter = 0;
    pNew = Vec_WrdAlloc( Vec_WrdSize(p) );
    Vec_WrdForEachEntry( p, Entry, i )
    {
        EntryNew = 0;
        for ( v = 0; v < nVarsMin; v++ )
        {
            Value = 0;
            for ( k = 0; k < nVars; k++ )
                if ( ((pMatrix[v] >> k) & 1) && ((Entry >> k) & 1) )
                    Value ^= 1;
            if ( Value )
                EntryNew |= (((word)1) << v);            
        }
        Vec_WrdPush( pNew, EntryNew );
    }
    // check that they are disjoint
    pLimit  = Vec_WrdLimit(pNew);
    pEntry1 = Vec_WrdArray(pNew);
    for ( ; pEntry1 < pLimit; pEntry1++ )
    for ( pEntry2 = pEntry1 + 1; pEntry2 < pLimit; pEntry2++ )
        if ( *pEntry1 == *pEntry2 )
            Counter++;
    if ( Counter )
        printf( "The total of %d pairs fail verification.\n", Counter );
    else
        printf( "Verification successful.\n" );
    Vec_WrdFree( pNew );
}

/**Function*************************************************************

  Synopsis    [Generate pairs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Wrd_t * Abc_SuppGenPairs( Vec_Wrd_t * p, int nBits )
{
    Vec_Wrd_t * vRes = Vec_WrdAlloc( 1000 );
    unsigned * pMap = ABC_CALLOC( unsigned, 1 << Abc_MaxInt(0,nBits-5) ); 
    word * pLimit = Vec_WrdLimit(p);
    word * pEntry1 = Vec_WrdArray(p);
    word * pEntry2, Value;
    for ( ; pEntry1 < pLimit; pEntry1++ )
    for ( pEntry2 = pEntry1 + 1; pEntry2 < pLimit; pEntry2++ )
    {
        Value = *pEntry1 ^ *pEntry2;
        if ( Abc_InfoHasBit(pMap, (int)Value) )
            continue;
        Abc_InfoXorBit( pMap, (int)Value );
        Vec_WrdPush( vRes, Value );
    }
    ABC_FREE( pMap );
    return vRes;
}
Vec_Wrd_t * Abc_SuppGenPairs2( int nOnes, int nBits )
{
    Vec_Wrd_t * vRes = Vec_WrdAlloc( 1000 );
    int i, k, Size = (1 << nBits), Value;
    for ( i = 0; i < Size; i++ )
    {
        Value = Abc_SuppCountOnes(i);
        for ( k = 1; k <= nOnes; k++ )
            if ( Value == 2*k )
                break;
        if ( k <= nOnes )
            Vec_WrdPush( vRes, i );
    }
    return vRes;
}

/**Function*************************************************************

  Synopsis    [Select variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_SuppPrintMask( word uMask, int nBits )
{
    int i;
    for ( i = 0; i < nBits; i++ )
        printf( "%d", (int)((uMask >> i) & 1) );
    printf( "\n" );
}
void Abc_SuppGenProfile( Vec_Wrd_t * p, int nBits, int * pCounts )
{
    word Ent;
    int i, k, b;
    Vec_WrdForEachEntry( p, Ent, i )
        for ( b = ((Ent >> nBits) & 1), k = 0; k < nBits; k++ )
            pCounts[k] += ((Ent >> k) & 1) ^ b;
}
void Abc_SuppPrintProfile( Vec_Wrd_t * p, int nBits )
{
    int k, Counts[64] = {0};
    Abc_SuppGenProfile( p, nBits, Counts );
    for ( k = 0; k < nBits; k++ )
        printf( "%2d : %6d  %6.2f %%\n", k, Counts[k], 100.0 * Counts[k] / Vec_WrdSize(p) );
}
int Abc_SuppGenFindBest( Vec_Wrd_t * p, int nBits, int * pMerit )
{
    int k, kBest = 0, Counts[64] = {0};
    Abc_SuppGenProfile( p, nBits, Counts );
    for ( k = 1; k < nBits; k++ )
        if ( Counts[kBest] < Counts[k] )
            kBest = k;
    *pMerit = Counts[kBest];
    return kBest;
}
void Abc_SuppGenSelectVar( Vec_Wrd_t * p, int nBits, int iVar )
{
    word * pEntry = Vec_WrdArray(p);
    word * pLimit = Vec_WrdLimit(p);
    for ( ; pEntry < pLimit; pEntry++ )
        if ( (*pEntry >> iVar) & 1 )
            *pEntry ^= (((word)1) << nBits);
}
void Abc_SuppGenFilter( Vec_Wrd_t * p, int nBits )
{
    word Ent;
    int i, k = 0;
    Vec_WrdForEachEntry( p, Ent, i )
        if ( ((Ent >> nBits) & 1) == 0 )
            Vec_WrdWriteEntry( p, k++, Ent );
    Vec_WrdShrink( p, k );
}
word Abc_SuppFindOne( Vec_Wrd_t * p, int nBits )
{
    word uMask = 0;
    int Prev = -1, This, Var;
    while ( 1 )
    {
        Var = Abc_SuppGenFindBest( p, nBits, &This );
        if ( Prev >= This )
            break;
        Prev = This;
        Abc_SuppGenSelectVar( p, nBits, Var );
        uMask |= (((word)1) << Var);
    }
    return uMask;
}
int Abc_SuppMinimize( word * pMatrix, Vec_Wrd_t * p, int nBits, int fVerbose )
{
    int i;
    for ( i = 0; Vec_WrdSize(p) > 0; i++ )
    {
//        Abc_SuppPrintProfile( p, nBits );
        pMatrix[i] = Abc_SuppFindOne( p, nBits );
        Abc_SuppGenFilter( p, nBits );   
        if ( !fVerbose )
            continue;
        // print stats
        printf( "%2d : ", i );
        printf( "%6d  ", Vec_WrdSize(p) );
        Abc_SuppPrintMask( pMatrix[i], nBits );
//        printf( "\n" );
    }
    return i;
}

/**Function*************************************************************

  Synopsis    [Create representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_SuppTest( int nOnes, int nVars, int fUseSimple, int fCheck, int fVerbose )
{
    int nVarsMin;
    word Matrix[64];
    abctime clk = Abc_Clock();
    // create the problem
    Vec_Wrd_t * vRes = Abc_SuppGen( nOnes, nVars );
    Vec_Wrd_t * vPairs = fUseSimple ? Abc_SuppGenPairs2( nOnes, nVars ) : Abc_SuppGenPairs( vRes, nVars );
    assert( nVars < 100 );
    printf( "M = %2d  N = %2d : ", nOnes, nVars );
    printf( "K = %6d   ",  Vec_WrdSize(vRes) );
    printf( "Total = %12.0f  ", 0.5 * Vec_WrdSize(vRes) * (Vec_WrdSize(vRes) - 1) );
    printf( "Distinct = %8d  ",  Vec_WrdSize(vPairs) );
    Abc_PrintTime( 1, "Reduction time", Abc_Clock() - clk );
    // solve the problem
    clk = Abc_Clock();
    nVarsMin = Abc_SuppMinimize( Matrix, vPairs, nVars, fVerbose );
    printf( "Solution with %d variables found.  ", nVarsMin );
    Abc_PrintTime( 1, "Covering time", Abc_Clock() - clk );
    if ( fCheck )
        Abc_SuppVerify( vRes, Matrix, nVars, nVarsMin );
    Vec_WrdFree( vPairs );
    Vec_WrdFree( vRes );
}


/**Function*************************************************************

  Synopsis    [Reads the input part of the cubes specified in MIN format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Wrd_t * Abc_SuppReadMin( char * pFileName, int * pnVars )
{
    Vec_Wrd_t * vRes; word uCube;
    int nCubes = 0, nVars = -1, iVar;
    char * pCur, * pToken, * pStart = "INPUT F-COVER";
    char * pStr = Extra_FileReadContents( pFileName );
    if ( pStr == NULL )
        { printf( "Cannot open input file (%s).\n", pFileName ); return NULL; }
    pCur = strstr( pStr, pStart );
    if ( pCur == NULL )
        { printf( "Cannot find beginning of cube cover (%s).\n", pStart ); return NULL; }
    pToken = strtok( pCur + strlen(pStart), " \t\r\n," );
    nCubes = atoi( pToken );
    if ( nCubes < 1 || nCubes > 1000000 )
        { printf( "The number of cubes in not in the range [1; 1000000].\n" ); return NULL; }
    vRes = Vec_WrdAlloc( 1000 );
    uCube = 0; iVar = 0;
    while ( (pToken = strtok(NULL, " \t\r\n,")) != NULL )
    {
        if ( strlen(pToken) > 2 )
        {
            if ( !strncmp(pToken, "INPUT", strlen("INPUT")) )
                break;
            if ( iVar > 64 )
                { printf( "The number of inputs (%d) is too high.\n", iVar ); Vec_WrdFree(vRes); return NULL; }
            if ( nVars == -1 )
                nVars = iVar;
            else if ( nVars != iVar )
                { printf( "The number of inputs (%d) does not match declaration (%d).\n", nVars, iVar ); Vec_WrdFree(vRes); return NULL; }
            Vec_WrdPush( vRes, uCube );
            uCube = 0; iVar = 0;
            continue;
        }
        if ( pToken[1] == '0' && pToken[0] == '1' ) // value 1
            uCube |= (((word)1) << iVar);
        else if ( pToken[1] != '1' || pToken[0] != '0' ) // value 0
            { printf( "Strange literal representation (%s) of cube %d.\n", pToken, nCubes ); Vec_WrdFree(vRes); return NULL; }
        iVar++;
    }
    ABC_FREE( pStr );
    if ( Vec_WrdSize(vRes) != nCubes )
        { printf( "The number of cubes (%d) does not match declaration (%d).\n", Vec_WrdSize(vRes), nCubes ); Vec_WrdFree(vRes); return NULL; }
    else
        printf( "Successfully parsed function with %d inputs and %d cubes.\n", nVars, nCubes );
    *pnVars = nVars;
    return vRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Wrd_t * Abc_SuppDiffMatrix( Vec_Wrd_t * vCubes )
{
    abctime clk = Abc_Clock();
    word * pEnt2, * pEnt = Vec_WrdArray( vCubes );
    word * pLimit = Vec_WrdLimit( vCubes );
    Vec_Wrd_t * vRes, * vPairs = Vec_WrdAlloc( Vec_WrdSize(vCubes) * (Vec_WrdSize(vCubes) - 1) / 2 );
    word * pStore = Vec_WrdArray( vPairs );
    for ( ; pEnt < pLimit; pEnt++ )
    for ( pEnt2 = pEnt+1; pEnt2 < pLimit; pEnt2++ )
        *pStore++ = *pEnt ^ *pEnt2;
    vPairs->nSize = Vec_WrdCap(vPairs);
    assert( pStore == Vec_WrdLimit(vPairs) );
//    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
//    vRes = Vec_WrdUniqifyHash( vPairs, 1 );
    vRes = Vec_WrdDup( vPairs );
    printf( "Successfully generated diff matrix with %10d rows (%6.2f %%).  ", 
        Vec_WrdSize(vRes), 100.0 * Vec_WrdSize(vRes) / Vec_WrdSize(vPairs) );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    Vec_WrdFree( vPairs );
    return vRes;    
}

/**Function*************************************************************

  Synopsis    [Solve difference matrix.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_SuppCountOnes64( word i )
{
    i = i - ((i >> 1) & 0x5555555555555555);
    i = (i & 0x3333333333333333) + ((i >> 2) & 0x3333333333333333);
    i = ((i + (i >> 4)) & 0x0F0F0F0F0F0F0F0F);
    return (i*(0x0101010101010101))>>56;
}
int Abc_SuppFindVar( Vec_Wec_t * pS, Vec_Wec_t * pD, int nVars )
{
    int v, vBest = -1, dBest = -1;
    for ( v = 0; v < nVars; v++ )
    {
        if ( Vec_WecLevelSize(pS, v) )
            continue;
        if ( vBest == -1 || dBest > Vec_WecLevelSize(pD, v) )
            vBest = v, dBest = Vec_WecLevelSize(pD, v);
    }
    return vBest;
}
void Abc_SuppRemove( Vec_Wrd_t * p, int * pCounts, Vec_Wec_t * pS, Vec_Wec_t * pD, int iVar, int nVars )
{
    word Entry; int i, v;
    assert( Vec_WecLevelSize(pS, iVar) == 0 );
    Vec_IntClear( Vec_WecEntry(pD, iVar) );
    Vec_WrdForEachEntry( p, Entry, i )
    {
        if ( ((Entry >> iVar) & 1) == 0 )
            continue;
        pCounts[i]--;
        if ( pCounts[i] == 1 )
        {
            for ( v = 0; v < nVars; v++ )
                if ( (Entry >> v) & 1 )
                {
                    Vec_IntRemove( Vec_WecEntry(pD, v), i );
                    Vec_WecPush( pS, v, i );
                }
        }
        else if ( pCounts[i] == 2 )
        {
            for ( v = 0; v < nVars; v++ )
                if ( (Entry >> v) & 1 )
                    Vec_WecPush( pD, v, i );
        }        
    }
}
void Abc_SuppProfile( Vec_Wec_t * pS, Vec_Wec_t * pD, int nVars )
{
    int v;
    for ( v = 0; v < nVars; v++ )
        printf( "%2d : S = %3d  D = %3d\n", v, Vec_WecLevelSize(pS, v), Vec_WecLevelSize(pD, v) );
}
int Abc_SuppSolve( Vec_Wrd_t * p, int nVars )
{
    abctime clk = Abc_Clock();
    Vec_Wrd_t * pNew = Vec_WrdDup( p );
    Vec_Wec_t * vSingles = Vec_WecStart( 64 );
    Vec_Wec_t * vDoubles = Vec_WecStart( 64 );
    word Entry; int i, v, iVar, nVarsNew = nVars;
    int * pCounts = ABC_ALLOC( int, Vec_WrdSize(p) );
    Vec_WrdForEachEntry( p, Entry, i )
    {
        pCounts[i] = Abc_SuppCountOnes64( Entry );
        if ( pCounts[i] == 1 )
        {
            for ( v = 0; v < nVars; v++ )
                if ( (Entry >> v) & 1 )
                    Vec_WecPush( vSingles, v, i );
        }
        else if ( pCounts[i] == 2 )
        {
            for ( v = 0; v < nVars; v++ )
                if ( (Entry >> v) & 1 )
                    Vec_WecPush( vDoubles, v, i );
        }
    }
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
//    Abc_SuppProfile( vSingles, vDoubles, nVars );
    // find variable in 0 singles and the smallest doubles
    while ( 1 )
    {
        iVar = Abc_SuppFindVar( vSingles, vDoubles, nVars );
        if ( iVar == -1 )
            break;
//        printf( "Selected variable %d.\n", iVar );
        Abc_SuppRemove( pNew, pCounts, vSingles, vDoubles, iVar, nVars );
//        Abc_SuppProfile( vSingles, vDoubles, nVars );
        nVarsNew--;
    }
//    printf( "Result = %d (out of %d)\n", nVarsNew, nVars );
    Vec_WecFree( vSingles );
    Vec_WecFree( vDoubles );
    Vec_WrdFree( pNew );
    ABC_FREE( pCounts );
    return nVarsNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_SuppReadMinTest( char * pFileName )
{
//    int fVerbose = 0;
    abctime clk = Abc_Clock();
//    word Matrix[64];
    int nVars, nVarsMin;
    Vec_Wrd_t * vPairs, * vCubes;
    vCubes = Abc_SuppReadMin( pFileName, &nVars );
    if ( vCubes == NULL )
        return;
    vPairs = Abc_SuppDiffMatrix( vCubes );
    Vec_WrdFreeP( &vCubes );
    // solve the problem
    clk = Abc_Clock();
//    nVarsMin = Abc_SuppMinimize( Matrix, vPairs, nVars, fVerbose );
    nVarsMin = Abc_SuppSolve( vPairs, nVars );
    printf( "Solution with %d variables found.  ", nVarsMin );
    Abc_PrintTime( 1, "Covering time", Abc_Clock() - clk );

    Vec_WrdFreeP( &vPairs );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

