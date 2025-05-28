/**CFile****************************************************************

  FileName    [extraUtilMaj.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [extra]

  Synopsis    [Path enumeration.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: extraUtilMaj.c,v 1.0 2003/02/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "misc/vec/vec.h"
#include "misc/vec/vecMem.h"
#include "misc/extra/extra.h"
#include "misc/util/utilTruth.h"
#include "opt/dau/dau.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Gem_Man_t_  Gem_Man_t;
typedef struct Gem_Obj_t_  Gem_Obj_t;

struct Gem_Man_t_
{
    int          nVars;         // max variable count
    int          nWords;        // truth tabel word count
    int          nObjsAlloc;    // allocated objects
    int          nObjs;         // used objects
    Gem_Obj_t *  pObjs;         // function objects
    Vec_Mem_t *  vTtMem;        // truth table memory and hash table
    word **      pTtElems;      // elementary truth tables
    int          fVerbose;
};

struct Gem_Obj_t_ // 8 bytes
{
    unsigned     nVars   :  4;  // variable count
    unsigned     nNodes  :  4;  // node count
    unsigned     History :  8;  // (i < j) ? {vi, vj} : {vi, 0}
    unsigned     Groups  : 16;  // mask with last vars in each symmetric group
    int          Predec;        // predecessor 
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gem_PrintNode( Gem_Man_t * p, int f, char * pLabel, int fUpdate )
{
    Gem_Obj_t * pObj = p->pObjs + f; 
    int GroupsMod = pObj->Groups;
    if ( !p->fVerbose )
        return;
    printf( "Node %6d : %s  Pred = %6d  Vars = %d  Nodes = %d  History = %d%d  Profile: ", 
        f, pLabel, pObj->Predec, pObj->nVars, pObj->nNodes, pObj->History & 0xF, pObj->History >> 4 );
    Extra_PrintBinary2( stdout, (unsigned*)&GroupsMod, p->nVars ); printf("%s\n", fUpdate?" *":"");
}
Gem_Man_t * Gem_ManAlloc( int nVars, int fVerbose )
{
    Gem_Man_t * p;
    assert( nVars <= 16 );
    p = ABC_CALLOC( Gem_Man_t, 1 );
    p->nVars      = nVars;
    p->nWords     = Abc_TtWordNum( nVars );
    p->nObjsAlloc = 10000000;
    p->nObjs      = 2;
    p->pObjs      = ABC_CALLOC( Gem_Obj_t, p->nObjsAlloc );
    p->pObjs[1].nVars = p->pObjs[1].Groups = 1; // buffer
    p->vTtMem     = Vec_MemAllocForTT( nVars, 0 );
    p->pTtElems   = (word **)Extra_ArrayAlloc( nVars + 4, p->nWords, sizeof(word) );
    p->fVerbose   = fVerbose;
    Abc_TtElemInit( p->pTtElems, nVars );
    Gem_PrintNode( p, 1, "Original", 0 );
    return p;
}
int Gem_ManFree( Gem_Man_t * p )
{
    Vec_MemHashFree( p->vTtMem );
    Vec_MemFree( p->vTtMem );
    ABC_FREE( p->pTtElems );
    ABC_FREE( p->pObjs );
    ABC_FREE( p );
    return 1;
}
void Gem_ManRealloc( Gem_Man_t * p )
{
    int nObjNew = Abc_MinInt( 2 * p->nObjsAlloc, 0x7FFFFFFF );
    assert( p->nObjs == p->nObjsAlloc );
    if ( p->nObjs == 0x7FFFFFFF )
        printf( "Hard limit on the number of nodes (0x7FFFFFFF) is reached. Quitting...\n" ), exit(1);
    assert( p->nObjs < nObjNew );
    printf("Extending object storage: %d -> %d.\n", p->nObjsAlloc, nObjNew );
    p->pObjs = ABC_REALLOC( Gem_Obj_t, p->pObjs, nObjNew );
    memset( p->pObjs + p->nObjsAlloc, 0, sizeof(Gem_Obj_t) * (nObjNew - p->nObjsAlloc) );
    p->nObjsAlloc = nObjNew;
}

/**Function*************************************************************

  Synopsis    [Derive groups using symmetry info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gem_GroupsDerive( word * pTruth, int nVars, word * pCof0, word * pCof1 )
{
    int v, Res = 1 << (nVars-1);
    for ( v = 0; v < nVars-1; v++ )
        if ( !Abc_TtVarsAreSymmetric(pTruth, nVars, v, v+1, pCof0, pCof1) )
            Res |= (1 << v);
    return Res;
}

/**Function*************************************************************

  Synopsis    [Extends function f by replacing var i with a new gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gem_GroupVarRemove( int Groups, int i ) // remove i-th var
{
    int Mask = i ? Abc_InfoMask( i ) : 0;
    assert( i >= 0 );
    assert( (Groups >> i) & 1 );
    return (Groups & Mask) | ((Groups & ~Mask) >> 1);
}
int Gem_GroupVarsInsert1( int Groups, int i, int fGroup ) // insert one bit after i
{
    int Mask = i+1 ? Abc_InfoMask( i+1 ) : 0;
    assert( i+1 >= 0 );
    assert( !fGroup || i == -1 || ((Groups >> i) & 1) );
    assert( fGroup == 0 || fGroup == 1 );
    return (Groups & Mask) | ((Groups & ~Mask) << 1) | (fGroup << (i+1));
}
int Gem_GroupVarsInsert3( int Groups, int i ) // insert group of 3 bits after i
{
    int Mask = i+1 ? Abc_InfoMask( i+1 ) : 0;
    assert( i+1 >= 0 );
    assert( i == -1 || (Groups >> i) & 1 );
    return (Groups & Mask) | ((Groups & ~Mask) << 3) | (0x4 << (i+1));
}
int Gem_GroupUnpack( int Groups, int * pVars )
{
    int v, nGroups = 0;
    for ( v = 0; Groups; v++, Groups >>= 1 )
        if ( Groups & 1 )
            pVars[nGroups++] = v;
    return nGroups;
}
int Gem_FuncFindPlace( word * pTruth, int nWords, int Groups, word * pBest, int fOneVar )
{
    int pLast[16], nGroups = Gem_GroupUnpack( Groups, pLast );
    int g, v, Value, BestPlace = nGroups ? pLast[nGroups - 1] : -1;
    assert( nGroups >= 0 );
    Abc_TtCopy( pBest, pTruth, nWords, 0 );
    for ( g = nGroups - 1; g >= 0; g-- )
    {
        int Limit = g ? pLast[g-1] : -1;
        for ( v = pLast[g]; v > Limit; v-- )
        {
            Abc_TtSwapAdjacent( pTruth, nWords, v+0 );
            if ( fOneVar ) 
                continue;
            Abc_TtSwapAdjacent( pTruth, nWords, v+1 );
            Abc_TtSwapAdjacent( pTruth, nWords, v+2 );
        }
        Value = memcmp(pBest, pTruth, sizeof(word)*nWords);
        if ( Value < 0 )
        {
            Abc_TtCopy( pBest, pTruth, nWords, 0 );
            BestPlace = Limit;
        }
    }
    return BestPlace;
}
void Gem_FuncExpand( Gem_Man_t * p, int f, int i )
{
    Gem_Obj_t * pNew = p->pObjs + p->nObjs, * pObj = p->pObjs + f; 
    word * pTruth   = Vec_MemReadEntry( p->vTtMem, f );
    word * pResult  = p->pTtElems[p->nVars];
    word * pCofs[2] = { p->pTtElems[p->nVars+2], p->pTtElems[p->nVars+3] };
    int v, iFunc;
    char pCanonPermC[16];
    assert( i < (int)pObj->nVars );
    assert( (int)pObj->nVars + 2 <= p->nVars );
    Abc_TtCopy( pResult, pTruth, p->nWords, 0 );
    // move i variable to the end
    for ( v = i; v < (int)pObj->nVars-1; v++ )
        Abc_TtSwapAdjacent( pResult, p->nWords, v );
    // create new symmetric group
    assert( v == (int)pObj->nVars-1 );
    Abc_TtCofactor0p( pCofs[0], pResult, p->nWords, v );
    Abc_TtCofactor1p( pCofs[1], pResult, p->nWords, v );
    Abc_TtMaj( pResult, p->pTtElems[v], p->pTtElems[v+1], p->pTtElems[v+2], p->nWords );
    Abc_TtMux( pResult, pResult, pCofs[1], pCofs[0], p->nWords );
    // canonicize
    //Extra_PrintHex( stdout, (unsigned*)pResult, pObj->nVars + 2 ); printf("\n");
    Abc_TtCanonicizePerm( pResult, pObj->nVars + 2, pCanonPermC );
    Abc_TtStretch6( pResult, Abc_MaxInt(6, pObj->nVars+2), p->nVars );
    //Extra_PrintHex( stdout, (unsigned*)pResult, pObj->nVars + 2 ); printf("\n\n");
    iFunc = Vec_MemHashInsert( p->vTtMem, pResult );
    if ( iFunc < p->nObjs )
        return;
    assert( iFunc == p->nObjs );
    pNew->nVars   = pObj->nVars + 2;
    pNew->nNodes  = pObj->nNodes + 1;
    pNew->Groups  = Gem_GroupsDerive( pResult, pNew->nVars, pCofs[0], pCofs[1] );
    pNew->Predec  = f;
    pNew->History = i; 
    Gem_PrintNode( p, iFunc, "Expand  ", 0 );

    assert( p->nObjs < p->nObjsAlloc );
    if ( ++p->nObjs == p->nObjsAlloc )
        Gem_ManRealloc( p );
}

/**Function*************************************************************

  Synopsis    [Reduces function f by crossbaring variables i and j.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gem_FuncCheckMajority( Gem_Man_t * p, int f )
{
    Gem_Obj_t * pObj = p->pObjs + f;
    word * pTruth = Vec_MemReadEntry( p->vTtMem, f );
    int Polar = Abc_TtIsFullySymmetric( pTruth, pObj->nVars );
    if ( Polar != -1 )
    {
        int nHalfVars = (pObj->nVars+1) >> 1;
        int Mask = Abc_Tt6Mask( nHalfVars );
        printf( "Found symmetric %d-variable function: ", pObj->nVars );
        Extra_PrintBinary2( stdout, (unsigned *)&Polar, pObj->nVars + 1 );
        printf( "   " );
        if ( (pObj->nVars & 1) && Polar == (Mask << nHalfVars) )
        {
            printf( "This is majority-%d.\n", pObj->nVars );
            return 0;
        }
        printf( "\n" );
    }
    return 0;
}
int Gem_FuncReduce( Gem_Man_t * p, int f, int i, int j )
{
    Gem_Obj_t * pNew = p->pObjs + p->nObjs, * pObj = p->pObjs + f; 
    word * pTruth   = Vec_MemReadEntry( p->vTtMem, f );
    word * pResult  = p->pTtElems[p->nVars];
    word * pCofs[2] = { p->pTtElems[p->nVars+2], p->pTtElems[p->nVars+3] };
    int v, iFunc;
    char pCanonPermC[16];
    assert( i < j && j < 16 );
    Abc_TtCopy( pResult, pTruth, p->nWords, 0 );
    // move j variable to the end
    for ( v = j; v < (int)pObj->nVars-1; v++ )
        Abc_TtSwapAdjacent( pResult, p->nWords, v );
    assert( v == (int)pObj->nVars-1 );
    // move i variable to the end
    for ( v = i; v < (int)pObj->nVars-2; v++ )
        Abc_TtSwapAdjacent( pResult, p->nWords, v );
    assert( v == (int)pObj->nVars-2 );
    // create new variable
    Abc_TtCofactor0p( pCofs[0], pResult, p->nWords, v+1 );
    Abc_TtCofactor1p( pCofs[1], pResult, p->nWords, v+1 );
    Abc_TtCofactor0( pCofs[0], p->nWords, v );
    Abc_TtCofactor1( pCofs[1], p->nWords, v );
    Abc_TtMux( pResult, p->pTtElems[v], pCofs[1], pCofs[0], p->nWords );
    // canonicize
    //Extra_PrintHex( stdout, (unsigned*)pResult, pObj->nVars - 1 ); printf("\n");
    Abc_TtCanonicizePerm( pResult, pObj->nVars - 1, pCanonPermC );
    Abc_TtStretch6( pResult, Abc_MaxInt(6, pObj->nVars-1), p->nVars );
    //Extra_PrintHex( stdout, (unsigned*)pResult, pObj->nVars - 1 ); printf("\n\n");
    iFunc = Vec_MemHashInsert( p->vTtMem, pResult );
    if ( iFunc < p->nObjs )
        return 0;
    assert( iFunc == p->nObjs );
    pNew->nVars   = pObj->nVars - 1;
    pNew->nNodes  = pObj->nNodes;
    pNew->Groups  = Gem_GroupsDerive( pResult, pNew->nVars, pCofs[0], pCofs[1] );
    pNew->Predec  = f;
    pNew->History = (j << 4) | i;
    Gem_PrintNode( p, iFunc, "Crossbar", 0 );
    Gem_FuncCheckMajority( p, iFunc );

    assert( p->nObjs < p->nObjsAlloc );
    if ( ++p->nObjs == p->nObjsAlloc )
        Gem_ManRealloc( p );
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gem_Enumerate( int nVars, int fDump, int fVerbose )
{
    abctime clk = Abc_Clock();
    Gem_Man_t * p = Gem_ManAlloc( nVars, fVerbose );
    int v, f, i, j, nObjsStop = 1;
    for ( v = 1; v <= nVars-2; v++ )
    {
        // expand functions by adding a gate
        int nObjsStopPrev = nObjsStop;
        nObjsStop = p->nObjs;
        printf( "Expanding  var %2d (functions = %10d)  ", v, p->nObjs );
        Abc_PrintTime( 0, "Time", Abc_Clock() - clk );
        for ( f = 0; f < nObjsStop; f++ )
            if ( v == (int)p->pObjs[f].nVars || (v > (int)p->pObjs[f].nVars && f >= nObjsStopPrev) )
                for ( i = 0; i < v; i++ )
                    if ( (int)p->pObjs[f].Groups & (1 << i) )
                        Gem_FuncExpand( p, f, i );
        // reduce functions by adding a crossbar
        printf( "Connecting var %2d (functions = %10d)  ", v, p->nObjs );
        Abc_PrintTime( 0, "Time", Abc_Clock() - clk );
        for ( f = nObjsStop; f < p->nObjs; f++ )
            for ( i = 0; i < (int)p->pObjs[f].nVars; i++ )
                if ( (int)p->pObjs[f].Groups & (1 << i) )
                    for ( j = i+1; j < (int)p->pObjs[f].nVars; j++ )
                        if ( (int)p->pObjs[f].Groups & (1 << j) )
                            if ( Gem_FuncReduce( p, f, i, j ) )
                                return Gem_ManFree( p );
    }
    printf( "Finished          (functions = %10d)  ", p->nObjs );
    Abc_PrintTime( 0, "Time", Abc_Clock() - clk );
    if ( fDump ) Vec_MemDumpTruthTables( p->vTtMem, "enum", nVars );
    Gem_ManFree( p );
    return 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

