/**CFile****************************************************************

  FileName    [giaMinLut.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Collapsing AIG.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaMinLut.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "giaAig.h"
#include "base/main/mainInt.h"
#include "opt/sfm/sfm.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define TREE_MAX_VARS 16

typedef struct Tree_Sto_t_ Tree_Sto_t; 
struct Tree_Sto_t_
{
    int             nIns;
    int             nOuts;
    int             pTried[TREE_MAX_VARS];
    int             pPerm[TREE_MAX_VARS];
    int             pIPerm[TREE_MAX_VARS];
    int             nNodes[TREE_MAX_VARS];
    Vec_Int_t       vCofs[TREE_MAX_VARS];
    word *          pMem;
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
Tree_Sto_t * Gia_ManTreeDup( Tree_Sto_t * p )
{
    Tree_Sto_t * pSto = ABC_CALLOC( Tree_Sto_t, 1 );
    int i, k, Obj;
    *pSto = *p;
    pSto->pMem = Abc_TtDup( pSto->pMem, p->nOuts*Abc_TtWordNum(p->nIns), 0 );
    memset( pSto->vCofs, 0, sizeof(Vec_Int_t)*TREE_MAX_VARS );
    for ( i = 0; i < TREE_MAX_VARS; i++ )
        Vec_IntForEachEntry( p->vCofs+i, Obj, k )
            Vec_IntPush( pSto->vCofs+i, Obj );
    return pSto;
}
void Gia_ManTreeFree( Tree_Sto_t * p )
{
    int i;
    for ( i = 0; i < TREE_MAX_VARS; i++ )
        ABC_FREE( p->vCofs[i].pArray );
    ABC_FREE( p->pMem );
    ABC_FREE( p );
}
int Gia_ManTreeCountNodes( Tree_Sto_t * p )
{
    int i, nNodes = 0;
    for ( i = 0; i < TREE_MAX_VARS; i++ )
        nNodes += p->nNodes[i];
    return nNodes;
}
void Gia_ManTreePrint( Tree_Sto_t * p )
{
    int i;
    printf( "Tree with %d nodes:\n", Gia_ManTreeCountNodes(p) );
    for ( i = p->nIns-1; i >= 0; i-- )
        printf( "Level %2d  Var %2d : %s  Nodes = %3d  Cofs = %3d\n", 
            i, p->pIPerm[i], p->pTried[i]?"*":" ", p->nNodes[i], Vec_IntSize(p->vCofs+i) );
//    for ( i = p->nIns-1; i >= 0; i-- )
//        printf( "Var %2d    Level %2d\n", i, p->pPerm[i] );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManFindOrAddNode( Tree_Sto_t * pSto, int iVar, int Truth, word * pCof )
{
    int k, Obj;
    if ( iVar > 5 )
    {
        int nWords = Abc_TtWordNum(iVar);
        Vec_IntForEachEntry( pSto->vCofs+iVar, Obj, k )
            if ( Abc_TtEqual( pSto->pMem + Obj, pCof, nWords ) )
                return 1;
        Vec_IntPush( pSto->vCofs+iVar, pCof - pSto->pMem );
    }
    else
    {
        Vec_IntForEachEntry( pSto->vCofs+iVar, Obj, k )
            if ( Obj == Truth )
                return 1;
        Vec_IntPush( pSto->vCofs+iVar, Truth );
    }
    return 0;
}
int Gia_ManProcessLevel( Tree_Sto_t * pSto, int iVar )
{
    int k, Obj, nNodes = 0;
    //Vec_IntPrint( pSto->vCofs+iVar );
    Vec_IntClear( pSto->vCofs+iVar );
    if ( iVar > 5 )
    {
        int nWords = Abc_TtWordNum(iVar);
        Vec_IntForEachEntry( pSto->vCofs+iVar+1, Obj, k )
        {
            word * pCof0 = pSto->pMem + Obj;
            word * pCof1 = pCof0 + nWords;
            Gia_ManFindOrAddNode( pSto, iVar, -1, pCof0 );
            if ( Abc_TtEqual( pCof0, pCof1, nWords ) )
                continue;
            Gia_ManFindOrAddNode( pSto, iVar, -1, pCof1 );
            nNodes++;
        }
    }
    else
    {
        Vec_IntForEachEntry( pSto->vCofs+iVar+1, Obj, k )
        {
            unsigned Cof0 = iVar < 5 ? Abc_Tt5Cofactor0( Obj, iVar ) : (unsigned) pSto->pMem[Obj]; 
            unsigned Cof1 = iVar < 5 ? Abc_Tt5Cofactor1( Obj, iVar ) : (unsigned)(pSto->pMem[Obj] >> 32); 
            Gia_ManFindOrAddNode( pSto, iVar, Cof0, NULL );
            if ( Cof0 == Cof1 )
                continue;
            Gia_ManFindOrAddNode( pSto, iVar, Cof1, NULL );
            nNodes++;
        }
    }
    //printf( "Level %2d :  Nodes = %3d  Cofs = %3d\n", iVar, nNodes, Vec_IntSize(pSto->vCofs+iVar) );
    //Vec_IntPrint( pSto->vCofs+iVar );
    //printf( "\n" );
    return nNodes;
}
Tree_Sto_t * Gia_ManContructTree( word * pTruths, int nIns, int nOuts, int nWords )
{
    Tree_Sto_t * pSto = ABC_CALLOC( Tree_Sto_t, 1 ); int i;
    assert( Abc_TtWordNum(nIns) == nWords );
    assert( nIns+1 <= TREE_MAX_VARS );
    pSto->pMem  = Abc_TtDup(pTruths, nOuts*nWords, 0);
    pSto->nIns  = nIns;
    pSto->nOuts = nOuts;
    for ( i = 0; i < nIns; i++ )
        pSto->pPerm[i] = pSto->pIPerm[i] = i;
    for ( i = 0; i < nOuts; i++ )
        Gia_ManFindOrAddNode( pSto, nIns, (unsigned)pSto->pMem[i], pSto->pMem + i*nWords );
    for ( i = nIns-1; i >= 0; i-- )
        pSto->nNodes[i] = Gia_ManProcessLevel( pSto, i );
    return pSto;
}
void Gia_ManContructTreeTest( word * pTruths, int nIns, int nOuts, int nWords )
{
    Tree_Sto_t * pSto = Gia_ManContructTree( pTruths, nIns, nOuts, nWords );
    printf( "Total nodes = %d.\n", Gia_ManTreeCountNodes(pSto) );
    Gia_ManTreeFree( pSto );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManSwapTree( Tree_Sto_t * pSto, int i )
{
    int nNodes = pSto->nNodes[i+1] + pSto->nNodes[i];
    int v, o, nWords = Abc_TtWordNum(pSto->nIns);
    //printf( "Swapping %2d and %2d  ", i, i+1 );
    assert( i >= 0 && i < pSto->nIns-1 );
    for ( o = 0; o < pSto->nOuts; o++ )
        Abc_TtSwapAdjacent( pSto->pMem + o*nWords, nWords, i );
    for ( v = 5; v > i+1; v-- )
    pSto->nNodes[v] = Gia_ManProcessLevel( pSto, v );
    pSto->nNodes[i+1] = Gia_ManProcessLevel( pSto, i+1 );
    pSto->nNodes[i]   = Gia_ManProcessLevel( pSto, i );
    ABC_SWAP( int, pSto->pTried[i], pSto->pTried[i+1] );
    ABC_SWAP( int, pSto->pIPerm[i], pSto->pIPerm[i+1] );
    pSto->pPerm[pSto->pIPerm[i+1]] = i+1;
    pSto->pPerm[pSto->pIPerm[i]]   = i;
    return pSto->nNodes[i+1] + pSto->nNodes[i] - nNodes;
}
int Gia_ManFindBestPosition( word * pTruths, int nIns, int nOuts, int nWords, word * pStore, int fMoveMore, int * pnNodesMin, int fVerbose )
{
    Tree_Sto_t * pSto = Gia_ManContructTree( pTruths, nIns, nOuts, nWords );
    //int v, vBest = nIns-1, nNodesCur = Gia_ManTreeCountNodes(pSto), nNodesMin = nNodesCur;
    int v, vBest = -1, nNodesCur = Gia_ManTreeCountNodes(pSto), nNodesMin = ABC_INFINITY;
    if ( fVerbose )
        Gia_ManTreePrint( pSto );
    Abc_TtCopy( pStore+(nIns-1)*nOuts*nWords, pSto->pMem, nOuts*nWords, 0 );
    for ( v = nIns-2; v >= 0; v-- )
    {
        nNodesCur += Gia_ManSwapTree( pSto, v );
        if ( fMoveMore ? nNodesMin >= nNodesCur : nNodesMin > nNodesCur )
        {
            nNodesMin = nNodesCur;
            vBest = v;
        }
        if ( fVerbose )
            printf( "Level %2d -> %2d :  Nodes = %4d.    ", v+1, v, nNodesCur );
        Abc_TtCopy( pStore+v*nOuts*nWords, pSto->pMem, nOuts*nWords, 0 );
        if ( fVerbose )
            Gia_ManContructTreeTest( pSto->pMem, nIns, nOuts, nWords );
    }
    assert( vBest != nIns-1 );
    Gia_ManTreeFree( pSto );
    if ( fVerbose )
        printf( "Best level = %d. Best nodes = %d.\n", vBest, nNodesMin );
    if ( pnNodesMin )
        *pnNodesMin = nNodesMin;
    return vBest;
}
void Gia_ManPermStats( int nIns, int * pIPerm, int * pTried )
{
    int v;
    for ( v = nIns-1; v >= 0; v-- )
        printf( "Level = %2d : Var = %2d  Tried = %2d\n", v, pIPerm[v], pTried[v] );
    printf( "\n" );
}
int Gia_ManPermuteTreeOne( word * pTruths, int nIns, int nOuts, int nWords, int fRandom, int * pIPermOut, int fVeryVerbose, int fVerbose )
{
    extern void Gia_ManDumpMuxes( Tree_Sto_t * p, char * pFileName, int * pIPerm );
    word * pStore = ABC_ALLOC( word, nIns*nOuts*nWords );
    int pTried[TREE_MAX_VARS] = {0};
    int pIPerm[TREE_MAX_VARS] = {0};
    int v, r, Pos, nNodesPrev = -1, nNodesMin = 0, nNoChange = 0;
    int nNodesBeg, nNodesEnd;
    Tree_Sto_t * pSto;
    for ( v = 0; v < nIns; v++ )
        pIPerm[v] = v;
    pSto = Gia_ManContructTree( pTruths, nIns, nOuts, nWords );
    nNodesBeg = Gia_ManTreeCountNodes(pSto);
    //Gia_ManDumpMuxes( pSto, "from_tt1.aig", pIPerm );
    Gia_ManTreeFree( pSto );
    if ( fRandom )
    for ( v = 0; v < nIns; v++ )
    {
        //int o, vRand = rand() % nIns;
        int o, vRand = Gia_ManRandom(0) % nIns;
        for ( o = 0; o < nOuts; o++ )
            Abc_TtSwapVars( pTruths + o*nWords, nIns, v, vRand );
        ABC_SWAP( int, pIPerm[vRand], pIPerm[v] );
    }
    for ( r = 0; r < 10*nIns; r++ )
    {
        nNodesPrev = nNodesMin;
        if ( fVeryVerbose )
            printf( "\nRound %d:\n", r );
        Pos = Gia_ManFindBestPosition( pTruths, nIns, nOuts, nWords, pStore, r&1, &nNodesMin, fVeryVerbose );
        Abc_TtCopy( pTruths, pStore+Pos*nOuts*nWords, nOuts*nWords, 0 );
        pTried[nIns-1]++;
        for ( v = nIns-2; v >= Pos; v-- )
        {
            ABC_SWAP( int, pTried[v+1], pTried[v] );
            ABC_SWAP( int, pIPerm[v+1], pIPerm[v] );
        }
        if ( fVeryVerbose )
            Gia_ManPermStats( nIns, pIPerm, pTried );
        nNoChange = nNodesPrev == nNodesMin ? nNoChange + 1 : 0;
        if ( nNoChange == 4 )
            break;
    }
    pSto = Gia_ManContructTree( pTruths, nIns, nOuts, nWords );
    nNodesEnd = Gia_ManTreeCountNodes(pSto);
    //Gia_ManDumpMuxes( pSto, "from_tt2.aig", pIPerm );
    if ( fVerbose )
        printf( "Nodes %5d -> %5d.    ", nNodesBeg, nNodesEnd );
    Gia_ManTreeFree( pSto );
    ABC_FREE( pStore );
    if ( pIPermOut )
        memcpy( pIPermOut, pIPerm, sizeof(int)*nIns );
    return nNodesEnd;
}
void Gia_ManPermuteTree( word * pTruths, int nIns, int nOuts, int nWords, int fRandom, int fVerbose )
{
    abctime clk = Abc_Clock();
    word * pTruthDup = Abc_TtDup( pTruths, nOuts*nWords, 0 );
    int r;
    //srand( time(NULL) );
    Gia_ManRandom(1);
    for ( r = 0; r < 100; r++ )
    {
        Gia_ManPermuteTreeOne( pTruthDup, nIns, nOuts, nWords, fRandom, NULL, 0, fVerbose );
        Abc_TtCopy( pTruthDup, pTruths, nOuts*nWords, 0 );
    }
    ABC_FREE( pTruthDup );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
#define TT_UNDEF ABC_CONST(0x1234567887654321)

static inline word Abc_Tt6Min_rec( word uF, word uR, int nVars, Vec_Wrd_t * vNodes )
{
    word uF0, uF1, uR0, uR1, uRes0, uRes1, uRes2; int i, Var;
    assert( nVars <= 6 );
    assert( (uF & uR) == 0 );
    if ( !uF && !uR )
        return TT_UNDEF;
    if ( !uF && !~uR )
        return 0;
    if ( !~uF && !uR )
        return ~(word)0;
    assert( nVars > 0 );
    for ( Var = nVars-1; Var >= 0; Var-- )
        if ( Abc_Tt6HasVar( uF, Var ) || Abc_Tt6HasVar( uR, Var ) )
             break;
    assert( Var >= 0 );
    if ( 1 && vNodes )
        Vec_WrdForEachEntry( vNodes, uRes2, i )
            if ( !(uF & ~uRes2) && !(uRes2 & uR) )
                return uRes2;
//            else if ( !(uF & uRes2) && !(~uRes2 & uR) )
//                return ~uRes2;
    uF0 = Abc_Tt6Cofactor0( uF, Var );
    uF1 = Abc_Tt6Cofactor1( uF, Var );
    uR0 = Abc_Tt6Cofactor0( uR, Var );
    uR1 = Abc_Tt6Cofactor1( uR, Var );
    uRes0 = Abc_Tt6Min_rec( uF0, uR0, Var, vNodes );
    uRes1 = Abc_Tt6Min_rec( uF1, uR1, Var, vNodes );
    if ( uRes0 == TT_UNDEF && uRes1 == TT_UNDEF )
        return TT_UNDEF;
    if ( uRes0 == TT_UNDEF )
        return uRes1;
    if ( uRes1 == TT_UNDEF )
        return uRes0;
    if ( uRes0 == uRes1 )
        return uRes0;
//    if ( (uRes0 & ~uRes1) == 0 )
//        printf( "0" );
//    else if ( (~uRes0 & uRes1) == 0 )
//        printf( "1" );
//    else
//        printf( "*" );
    uRes2 = (uRes0 & s_Truths6Neg[Var]) | (uRes1 & s_Truths6[Var]);
    assert( !(uF & ~uRes2) );
    assert( !(uRes2 & uR) );
    if ( vNodes )
        Vec_WrdPush( vNodes, uRes2 );
    return uRes2;
}
word * Abc_TtMin_rec( word * pF, word * pR, int nVars, Vec_Wrd_t * vMemory, Vec_Wrd_t * vNodes, Vec_Wec_t * vNodes2 )
{
    int i, Entry, nWords = Abc_TtWordNum(nVars);
    word * pRes0, * pRes1, * pRes2 = Vec_WrdFetch( vMemory, nWords );
    if ( nVars <= 6 )
    {
        pRes2[0] = Abc_Tt6Min_rec( pF[0], pR[0], nVars, vNodes );
        return pRes2;
    }
    assert( !Abc_TtIntersect(pF, pR, nWords, 0) );
    if ( Abc_TtIsConst0(pF, nWords) && Abc_TtIsConst0(pR, nWords) )
        return NULL;
    if ( Abc_TtIsConst0(pF, nWords) && Abc_TtIsConst1(pR, nWords) )
    {
        Abc_TtClear( pRes2, nWords );
        return pRes2;
    }
    if ( Abc_TtIsConst1(pF, nWords) && Abc_TtIsConst0(pR, nWords) )
    {
        Abc_TtFill( pRes2, nWords );
        return pRes2;
    }
    nWords >>= 1;
    if ( !Abc_TtHasVar( pF, nVars, nVars-1 ) && !Abc_TtHasVar( pR, nVars, nVars-1 ) )
    {
        pRes0 = Abc_TtMin_rec( pF, pR, nVars-1, vMemory, vNodes, vNodes2 );
        Abc_TtCopy( pRes2,          pRes0, nWords, 0 );
        Abc_TtCopy( pRes2 + nWords, pRes0, nWords, 0 );
        return pRes2;
    }
    if ( 1 && vNodes2 )
    {
        Vec_Int_t * vLayer = Vec_WecEntry( vNodes2, nVars );
        Vec_IntForEachEntry( vLayer, Entry, i )
        {
            word * pTemp = Vec_WrdEntryP( vMemory, Entry );
            if ( !Abc_TtIntersect(pTemp, pF, 2*nWords, 1) && !Abc_TtIntersect(pTemp, pR, 2*nWords, 0) )
                return pTemp;
/*
            if ( !Abc_TtIntersect(pTemp, pF, 2*nWords, 0) && !Abc_TtIntersect(pTemp, pR, 2*nWords, 1) )
            {
                Abc_TtCopy( pRes2, pTemp, 2*nWords, 1 );
                return pRes2;
            }
*/
        }
    }
    assert( nVars > 6 );
    pRes0 = Abc_TtMin_rec( pF,          pR,          nVars-1, vMemory, vNodes, vNodes2 );
    pRes1 = Abc_TtMin_rec( pF + nWords, pR + nWords, nVars-1, vMemory, vNodes, vNodes2 );
    if ( pRes0 == NULL && pRes1 == NULL )
        return NULL;
    if ( pRes0 == NULL || pRes1 == NULL || Abc_TtEqual(pRes0, pRes1, nWords) )
    {
        Abc_TtCopy( pRes2,          pRes0 ? pRes0 : pRes1, nWords, 0 );
        Abc_TtCopy( pRes2 + nWords, pRes0 ? pRes0 : pRes1, nWords, 0 );
        return pRes2;
    }
    Abc_TtCopy( pRes2,          pRes0, nWords, 0 );
    Abc_TtCopy( pRes2 + nWords, pRes1, nWords, 0 );
    assert( !Abc_TtIntersect(pRes2, pF, 2*nWords, 1) ); // assert( !(uF & ~uRes2) );
    assert( !Abc_TtIntersect(pRes2, pR, 2*nWords, 0) ); // assert( !(uRes2 & uR)  );
    if ( vNodes2 )
        Vec_WecPush( vNodes2, nVars, pRes2 - Vec_WrdArray(vMemory) );
    return pRes2;
}
word * Abc_TtMin( word * pF, word * pR, int nVars, Vec_Wrd_t * vMemory, Vec_Wrd_t * vNodes, Vec_Wec_t * vNodes2 )
{
    word * pResult; 
    int i, nWords = Abc_TtWordNum(nVars);
    assert( nVars >= 0 && nVars <= 16 );
    for ( i = nVars; i < 6; i++ )
        assert( !Abc_Tt6HasVar(pF[0], i) && !Abc_Tt6HasVar(pR[0], i) );
    Vec_WrdClear( vMemory );
    Vec_WrdGrow( vMemory, 1 << 20 );
    pResult = Abc_TtMin_rec( pF, pR, nVars, vMemory, vNodes, vNodes2 );
    if ( pResult == NULL )
    {
        Vec_WrdFill( vMemory, nWords, 0 );
        return Vec_WrdArray( vMemory );
    }
    //printf( "Memory %d (Truth table %d)\n", Vec_WrdSize(vMemory), nWords );
    Abc_TtCopy( Vec_WrdArray(vMemory), pResult, nWords, 0 );
    Vec_WrdShrink( vMemory, nWords );
    return Vec_WrdArray(vMemory);
}
word * Abc_TtMinArray( word * p, int nOuts, int nVars, int * pnNodes, int fVerbose )
{
    int o, i, nWords = Abc_TtWordNum(nVars);
    word * pRes, * pResult = ABC_ALLOC( word, nOuts*nWords/2 );
    Vec_Wrd_t * vMemory = Vec_WrdAlloc( 100 );
    Vec_Wrd_t * vNodes  = Vec_WrdAlloc( 100 );
    Vec_Wec_t * vNodes2 = Vec_WecStart( nVars+1 );
    Vec_WrdGrow( vMemory, 1 << 20 );
    for ( o = 0; o < nOuts/2; o++ )
    {
        word * pF = p + (2*o+0)*nWords;
        word * pR = p + (2*o+1)*nWords;
        for ( i = nVars; i < 6; i++ )
            assert( !Abc_Tt6HasVar(pF[0], i) && !Abc_Tt6HasVar(pR[0], i) );
        pRes = Abc_TtMin_rec( pF, pR, nVars, vMemory, vNodes, vNodes2 );
        if ( pResult == NULL )
            Abc_TtClear( pResult + o*nWords, nWords );
        else
            Abc_TtCopy( pResult + o*nWords, pRes, nWords, 0 );
    }
    if ( fVerbose )
    printf( "Nodes = %5d.  Nodes2 = %5d.  Total = %5d.    ", 
        Vec_WrdSize(vNodes), Vec_WecSizeSize(vNodes2), Vec_WrdSize(vNodes) + Vec_WecSizeSize(vNodes2) );
    //printf( "Memory %d (Truth table %d)\n", Vec_WrdSize(vMemory), nWords );
    if ( pnNodes )
        *pnNodes = Vec_WrdSize(vNodes) + Vec_WecSizeSize(vNodes2);
    Vec_WrdFree( vMemory );
    Vec_WrdFree( vNodes );
    Vec_WecFree( vNodes2 );
    return pResult;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word Abc_TtSimple6Min_rec( Gia_Man_t * p, word uF, word uC, int nVars, Vec_Wrd_t * vNodes, int * piLit, int * pPerm )
{
    word uF0, uF1, uC0, uC1, uRes0, uRes1, uRes2; int i, Var, iLit0, iLit1;
    word uFC =  uF & uC;
    word uRC = ~uF & uC;
    assert( nVars <= 6 );
    *piLit = 0;
    if ( !uFC )
    {
        *piLit = 0;
        return 0;
    }
    if ( !uRC )
    {
        *piLit = 1;
        return ~(word)0;
    }
    assert( nVars > 0 );
    if ( 1 && vNodes )
    {
        int iLit;
        Vec_WrdForEachEntryDouble( vNodes, uRes2, iLit, i )
            if ( !((uF ^ uRes2) & uC) )
            {
                *piLit = (unsigned)iLit;
                return uRes2;
            }
            else if ( !((uF ^ ~uRes2) & uC) )
            {
                *piLit = Abc_LitNot( (unsigned)iLit );
                return ~uRes2;
            }
    }
    for ( Var = nVars-1; Var >= 0; Var-- )
        if ( Abc_Tt6HasVar( uF, Var ) )
             break;
        else
            uC = Abc_Tt6Cofactor0(uC, Var) | Abc_Tt6Cofactor1(uC, Var);
    assert( Var >= 0 );
    uF0 = Abc_Tt6Cofactor0( uF, Var );
    uF1 = Abc_Tt6Cofactor1( uF, Var );
    uC0 = Abc_Tt6Cofactor0( uC, Var );
    uC1 = Abc_Tt6Cofactor1( uC, Var );
    uRes0 = Abc_TtSimple6Min_rec( p, uF0, uC0, Var, vNodes, &iLit0, pPerm );
    uRes1 = Abc_TtSimple6Min_rec( p, uF1, uC1, Var, vNodes, &iLit1, pPerm );
    if ( uRes0 == uRes1 )
    {
        *piLit = iLit0;
        return uRes0;
    }
    uRes2 = (uRes0 & s_Truths6Neg[Var]) | (uRes1 & s_Truths6[Var]);
    Var = pPerm ? pPerm[Var] : Var;
    //if ( !(uRes0 & ~uRes1 & uC1) )
    if ( !(uRes0 & ~uRes1) )
        *piLit = Gia_ManHashOr( p, Gia_ManHashAnd(p, Abc_Var2Lit(1+Var, 0), iLit1), iLit0 );
    //else if ( !(uRes1 & ~uRes0 & uC0) )
    else if ( !(uRes1 & ~uRes0) )
        *piLit = Gia_ManHashOr( p, Gia_ManHashAnd(p, Abc_Var2Lit(1+Var, 1), iLit0), iLit1 );
    else
        *piLit = Gia_ManHashMux( p, Abc_Var2Lit(1+Var, 0), iLit1, iLit0 );
    assert( !(uFC & ~uRes2) );
    assert( !(uRes2 & uRC) );
    if ( vNodes )
        Vec_WrdPushTwo( vNodes, uRes2, (word)*piLit );
    return uRes2;
}
word * Abc_TtSimpleMin_rec( Gia_Man_t * p, word * pF, word * pC, int nVars, Vec_Wrd_t * vMemory, Vec_Wrd_t * vNodes, Vec_Wec_t * vNodes2, int * piLit, int * pPerm )
{
    int i, Entry, Var, iLit0, iLit1, nWords = Abc_TtWordNum(nVars);
    word * pRes0, * pRes1, * pRes2 = Vec_WrdFetch( vMemory, nWords );
    *piLit = 0;
    if ( nVars <= 6 )
    {
        pRes2[0] = Abc_TtSimple6Min_rec( p, pF[0], pC[0], nVars, vNodes, piLit, pPerm );
        return pRes2;
    }
    if ( !Abc_TtIntersect(pF, pC, nWords, 0) )
    {
        *piLit = 0;
        Abc_TtClear( pRes2, nWords );
        return pRes2;
    }
    if ( !Abc_TtIntersect(pF, pC, nWords, 1) )
    {
        *piLit = 1;
        Abc_TtFill( pRes2, nWords );
        return pRes2;
    }
    nWords >>= 1;
    if ( !Abc_TtHasVar( pF, nVars, nVars-1 ) )
    {
        word * pCn = Vec_WrdFetch( vMemory, nWords );
        Abc_TtOr( pCn, pC, pC + nWords, nWords );
        pRes0 = Abc_TtSimpleMin_rec( p, pF, pCn, nVars-1, vMemory, vNodes, vNodes2, piLit, pPerm );
        Abc_TtCopy( pRes2,          pRes0, nWords, 0 );
        Abc_TtCopy( pRes2 + nWords, pRes0, nWords, 0 );
        return pRes2;
    }
    if ( 1 && vNodes2 )
    {
        Vec_Int_t * vLayer = Vec_WecEntry( vNodes2, nVars ); int iLit;
        Vec_IntForEachEntryDouble( vLayer, Entry, iLit, i )
        {
            word * pTemp = Vec_WrdEntryP( vMemory, Entry );
            if ( Abc_TtEqualCare(pTemp, pF, pC, 0, 2*nWords) )
            {
                *piLit = iLit;
                return pTemp;
            }
            else if ( Abc_TtEqualCare(pTemp, pF, pC, 1, 2*nWords) )
            {
                *piLit = Abc_LitNot(iLit);
                Abc_TtCopy( pRes2, pTemp, 2*nWords, 1 );
                return pRes2;
            }
        }
    }
    assert( nVars > 6 );
    pRes0 = Abc_TtSimpleMin_rec( p, pF,          pC,          nVars-1, vMemory, vNodes, vNodes2, &iLit0, pPerm );
    pRes1 = Abc_TtSimpleMin_rec( p, pF + nWords, pC + nWords, nVars-1, vMemory, vNodes, vNodes2, &iLit1, pPerm );
    if ( Abc_TtEqual(pRes0, pRes1, nWords) )
    {
        *piLit = iLit0;
        Abc_TtCopy( pRes2,          pRes0, nWords, 0 );
        Abc_TtCopy( pRes2 + nWords, pRes0, nWords, 0 );
        return pRes2;
    }
    Abc_TtCopy( pRes2,          pRes0, nWords, 0 );
    Abc_TtCopy( pRes2 + nWords, pRes1, nWords, 0 );
    Var = pPerm ? pPerm[nVars-1] : nVars-1;
    //if ( !Abc_TtIntersectCare(pRes1, pRes0, pC + nWords, nWords, 1) )
    if ( !Abc_TtIntersect(pRes1, pRes0, nWords, 1) )
        *piLit = Gia_ManHashOr( p, Gia_ManHashAnd(p, Abc_Var2Lit(1+Var, 0), iLit1), iLit0 );
    //else if ( !Abc_TtIntersectCare(pRes0, pRes1, pC, nWords, 1) )
    else if ( !Abc_TtIntersect(pRes0, pRes1, nWords, 1) )
        *piLit = Gia_ManHashOr( p, Gia_ManHashAnd(p, Abc_Var2Lit(1+Var, 1), iLit0), iLit1 );
    else
        *piLit = Gia_ManHashMux( p, Abc_Var2Lit(1+Var, 0), iLit1, iLit0 );
    assert( Abc_TtEqualCare(pRes2, pF, pC, 0, 2*nWords) );
    if ( vNodes2 )
    {
        Vec_Int_t * vLayer = Vec_WecEntry( vNodes2, nVars ); 
        Vec_IntPushTwo( vLayer, pRes2 - Vec_WrdArray(vMemory), *piLit );
    }
    return pRes2;
}
Gia_Man_t * Abc_TtSimpleMinArrayNew( word * p, int nVars, int nOuts, int * pnNodes, int fVerbose, int * pIPerm )
{
    Gia_Man_t * pNew, * pTemp;
    int o, i, iLit, nWords = Abc_TtWordNum(nVars);
    word * pF = ABC_ALLOC( word, nWords );
    word * pR = ABC_ALLOC( word, nWords );
    Vec_Wrd_t * vMemory = Vec_WrdAlloc( 100 );
    Vec_Wrd_t * vNodes  = Vec_WrdAlloc( 100 );
    Vec_Wec_t * vNodes2 = Vec_WecStart( nVars+1 );
    Vec_WrdGrow( vMemory, 1 << 20 );
    pNew = Gia_ManStart( 1000 );
    pNew->pName = Abc_UtilStrsav( "muxes" );
    for ( i = 0; i < nVars; i++ )
        Gia_ManAppendCi(pNew);
    Gia_ManHashAlloc( pNew );

    for ( o = 0; o < nOuts; o++ )
    {
        word * pCare = p + nOuts*nWords;
        word * pTruth = p + o*nWords;
        for ( i = nVars; i < 6; i++ )
            assert( !Abc_Tt6HasVar(pTruth[0], i) && !Abc_Tt6HasVar(pCare[0], i) );
        Abc_TtSimpleMin_rec( pNew, pTruth, pCare, nVars, vMemory, vNodes, vNodes2, &iLit, pIPerm );
        Gia_ManAppendCo( pNew, iLit );
    }
    if ( fVerbose )
    printf( "Nodes = %5d.  Nodes2 = %5d.  Total = %5d.    ", 
        Vec_WrdSize(vNodes), Vec_WecSizeSize(vNodes2), Vec_WrdSize(vNodes) + Vec_WecSizeSize(vNodes2) );
    //printf( "Memory %d (Truth table %d)\n", Vec_WrdSize(vMemory), nWords );
    if ( pnNodes )
        *pnNodes = Vec_WrdSize(vNodes) + Vec_WecSizeSize(vNodes2);
    Vec_WrdFree( vMemory );
    Vec_WrdFree( vNodes );
    Vec_WecFree( vNodes2 );
    ABC_FREE( pF );
    ABC_FREE( pR );

    Gia_ManHashStop( pNew );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}



/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word Abc_TtGia6Min_rec( Gia_Man_t * p, word uF, word uR, int nVars, Vec_Wrd_t * vNodes, int * piLit, int * pPerm )
{
    word uF0, uF1, uR0, uR1, uRes0, uRes1, uRes2; int i, Var, iLit0, iLit1;
    assert( nVars <= 6 );
    assert( (uF & uR) == 0 );
    *piLit = 0;
    if ( !uF && !uR )
        return TT_UNDEF;
    if ( !uF && !~uR )
    {
        *piLit = 0;
        return 0;
    }
    if ( !~uF && !uR )
    {
        *piLit = 1;
        return ~(word)0;
    }
    assert( nVars > 0 );
    for ( Var = nVars-1; Var >= 0; Var-- )
        if ( Abc_Tt6HasVar( uF, Var ) || Abc_Tt6HasVar( uR, Var ) )
             break;
    assert( Var >= 0 );
    if ( 1 && vNodes )
    {
        int iLit;
        Vec_WrdForEachEntryDouble( vNodes, uRes2, iLit, i )
            if ( !(uF & ~uRes2) && !(uRes2 & uR) )
            {
                *piLit = (unsigned)iLit;
                return uRes2;
            }
            else if ( !(uF & uRes2) && !(~uRes2 & uR) )
            {
                *piLit = Abc_LitNot( (unsigned)iLit );
                return ~uRes2;
            }
    }
    uF0 = Abc_Tt6Cofactor0( uF, Var );
    uF1 = Abc_Tt6Cofactor1( uF, Var );
    uR0 = Abc_Tt6Cofactor0( uR, Var );
    uR1 = Abc_Tt6Cofactor1( uR, Var );
    uRes0 = Abc_TtGia6Min_rec( p, uF0, uR0, Var, vNodes, &iLit0, pPerm );
    uRes1 = Abc_TtGia6Min_rec( p, uF1, uR1, Var, vNodes, &iLit1, pPerm );
    if ( uRes0 == TT_UNDEF && uRes1 == TT_UNDEF )
        return TT_UNDEF;
    if ( uRes0 == TT_UNDEF )
    {
        *piLit = iLit1;
        return uRes1;
    }
    if ( uRes1 == TT_UNDEF || uRes0 == uRes1 )
    {
        *piLit = iLit0;
        return uRes0;
    }
//    if ( (uRes0 & ~uRes1) == 0 )
//        printf( "0" );
//    else if ( (~uRes0 & uRes1) == 0 )
//        printf( "1" );
//    else
//        printf( "*" );
    uRes2 = (uRes0 & s_Truths6Neg[Var]) | (uRes1 & s_Truths6[Var]);
    Var = pPerm ? pPerm[Var] : Var;
    if ( !(uRes0 & ~uRes1) )
        *piLit = Gia_ManHashOr( p, Gia_ManHashAnd(p, Abc_Var2Lit(1+Var, 0), iLit1), iLit0 );
    else if ( !(uRes1 & ~uRes0) )
        *piLit = Gia_ManHashOr( p, Gia_ManHashAnd(p, Abc_Var2Lit(1+Var, 1), iLit0), iLit1 );
    else
        *piLit = Gia_ManHashMux( p, Abc_Var2Lit(1+Var, 0), iLit1, iLit0 );
    assert( !(uF & ~uRes2) );
    assert( !(uRes2 & uR) );
    if ( vNodes )
        Vec_WrdPushTwo( vNodes, uRes2, (word)*piLit );
    return uRes2;
}
word * Abc_TtGiaMin_rec( Gia_Man_t * p, word * pF, word * pR, int nVars, Vec_Wrd_t * vMemory, Vec_Wrd_t * vNodes, Vec_Wec_t * vNodes2, int * piLit, int * pPerm )
{
    int i, Entry, Var, iLit0, iLit1, nWords = Abc_TtWordNum(nVars);
    word * pRes0, * pRes1, * pRes2 = Vec_WrdFetch( vMemory, nWords );
    *piLit = 0;
    if ( nVars <= 6 )
    {
        pRes2[0] = Abc_TtGia6Min_rec( p, pF[0], pR[0], nVars, vNodes, piLit, pPerm );
        return pRes2;
    }
    assert( !Abc_TtIntersect(pF, pR, nWords, 0) );
    if ( Abc_TtIsConst0(pF, nWords) && Abc_TtIsConst0(pR, nWords) )
        return NULL;
    if ( Abc_TtIsConst0(pF, nWords) && Abc_TtIsConst1(pR, nWords) )
    {
        *piLit = 0;
        Abc_TtClear( pRes2, nWords );
        return pRes2;
    }
    if ( Abc_TtIsConst1(pF, nWords) && Abc_TtIsConst0(pR, nWords) )
    {
        *piLit = 1;
        Abc_TtFill( pRes2, nWords );
        return pRes2;
    }
    nWords >>= 1;
    if ( !Abc_TtHasVar( pF, nVars, nVars-1 ) && !Abc_TtHasVar( pR, nVars, nVars-1 ) )
    {
        pRes0 = Abc_TtGiaMin_rec( p, pF, pR, nVars-1, vMemory, vNodes, vNodes2, piLit, pPerm );
        Abc_TtCopy( pRes2,          pRes0, nWords, 0 );
        Abc_TtCopy( pRes2 + nWords, pRes0, nWords, 0 );
        return pRes2;
    }
    if ( 1 && vNodes2 )
    {
        Vec_Int_t * vLayer = Vec_WecEntry( vNodes2, nVars ); int iLit;
        Vec_IntForEachEntryDouble( vLayer, Entry, iLit, i )
        {
            word * pTemp = Vec_WrdEntryP( vMemory, Entry );
            if ( !Abc_TtIntersect(pTemp, pF, 2*nWords, 1) && !Abc_TtIntersect(pTemp, pR, 2*nWords, 0) )
            {
                *piLit = iLit;
                return pTemp;
            }
            else if ( !Abc_TtIntersect(pTemp, pF, 2*nWords, 0) && !Abc_TtIntersect(pTemp, pR, 2*nWords, 1) )
            {
                *piLit = Abc_LitNot(iLit);
                Abc_TtCopy( pRes2, pTemp, 2*nWords, 1 );
                return pRes2;
            }
        }
/*
        if ( nVars > 7 )
        {
            vLayer = Vec_WecEntry( vNodes2, nVars-1 );
            Vec_IntForEachEntryDouble( vLayer, Entry, iLit, i )
            {
                word * pTemp = Vec_WrdEntryP( vMemory, Entry );
                if ( !Abc_TtIntersect(pTemp, pF, 2*nWords, 1) && !Abc_TtIntersect(pTemp, pR, 2*nWords, 0) )
                {
                    *piLit = iLit;
                    return pTemp;
                }
                else if ( !Abc_TtIntersect(pTemp, pF, 2*nWords, 0) && !Abc_TtIntersect(pTemp, pR, 2*nWords, 1) )
                {
                    *piLit = Abc_LitNot(iLit);
                    Abc_TtCopy( pRes2, pTemp, 2*nWords, 1 );
                    return pRes2;
                }
            }
        }
*/
    }
    assert( nVars > 6 );
    pRes0 = Abc_TtGiaMin_rec( p, pF,          pR,          nVars-1, vMemory, vNodes, vNodes2, &iLit0, pPerm );
    pRes1 = Abc_TtGiaMin_rec( p, pF + nWords, pR + nWords, nVars-1, vMemory, vNodes, vNodes2, &iLit1, pPerm );
    if ( pRes0 == NULL && pRes1 == NULL )
        return NULL;
    if ( pRes0 == NULL || pRes1 == NULL || Abc_TtEqual(pRes0, pRes1, nWords) )
    {
        *piLit = pRes0 ? iLit0 : iLit1;
        Abc_TtCopy( pRes2,          pRes0 ? pRes0 : pRes1, nWords, 0 );
        Abc_TtCopy( pRes2 + nWords, pRes0 ? pRes0 : pRes1, nWords, 0 );
        return pRes2;
    }
    Abc_TtCopy( pRes2,          pRes0, nWords, 0 );
    Abc_TtCopy( pRes2 + nWords, pRes1, nWords, 0 );
    Var = pPerm ? pPerm[nVars-1] : nVars-1;
    if ( !Abc_TtIntersect(pRes1, pRes0, nWords, 1) )
        *piLit = Gia_ManHashOr( p, Gia_ManHashAnd(p, Abc_Var2Lit(1+Var, 0), iLit1), iLit0 );
    else if ( !Abc_TtIntersect(pRes0, pRes1, nWords, 1) )
        *piLit = Gia_ManHashOr( p, Gia_ManHashAnd(p, Abc_Var2Lit(1+Var, 1), iLit0), iLit1 );
    else
        *piLit = Gia_ManHashMux( p, Abc_Var2Lit(1+Var, 0), iLit1, iLit0 );
    assert( !Abc_TtIntersect(pRes2, pF, 2*nWords, 1) ); // assert( !(uF & ~uRes2) );
    assert( !Abc_TtIntersect(pRes2, pR, 2*nWords, 0) ); // assert( !(uRes2 & uR)  );
    if ( vNodes2 )
    {
        Vec_Int_t * vLayer = Vec_WecEntry( vNodes2, nVars ); 
        Vec_IntPushTwo( vLayer, pRes2 - Vec_WrdArray(vMemory), *piLit );
    }
    return pRes2;
}
Gia_Man_t * Abc_TtGiaMinArray( word * p, int nVars, int nOuts, int * pnNodes, int fVerbose, int * pIPerm )
{
    Gia_Man_t * pNew, * pTemp;
    int o, i, iLit, nWords = Abc_TtWordNum(nVars);
    word * pRes, * pResult = ABC_ALLOC( word, nOuts*nWords/2 );
    Vec_Wrd_t * vMemory = Vec_WrdAlloc( 100 );
    Vec_Wrd_t * vNodes  = Vec_WrdAlloc( 100 );
    Vec_Wec_t * vNodes2 = Vec_WecStart( nVars+1 );
    Vec_WrdGrow( vMemory, 1 << 20 );
    pNew = Gia_ManStart( 1000 );
    pNew->pName = Abc_UtilStrsav( "muxes" );
    for ( i = 0; i < nVars; i++ )
        Gia_ManAppendCi(pNew);
    Gia_ManHashAlloc( pNew );

    for ( o = 0; o < nOuts/2; o++ )
    {
        word * pF = p + (2*o+0)*nWords;
        word * pR = p + (2*o+1)*nWords;
        for ( i = nVars; i < 6; i++ )
            assert( !Abc_Tt6HasVar(pF[0], i) && !Abc_Tt6HasVar(pR[0], i) );
        pRes = Abc_TtGiaMin_rec( pNew, pF, pR, nVars, vMemory, vNodes, vNodes2, &iLit, pIPerm );
        if ( pResult == NULL )
        {
            Abc_TtClear( pResult + o*nWords, nWords );
            Gia_ManAppendCo( pNew, 0 );
        }
        else
        {
            Abc_TtCopy( pResult + o*nWords, pRes, nWords, 0 );
            Gia_ManAppendCo( pNew, iLit );
        }
    }
    if ( fVerbose )
    printf( "Nodes = %5d.  Nodes2 = %5d.  Total = %5d.    ", 
        Vec_WrdSize(vNodes), Vec_WecSizeSize(vNodes2), Vec_WrdSize(vNodes) + Vec_WecSizeSize(vNodes2) );
    //printf( "Memory %d (Truth table %d)\n", Vec_WrdSize(vMemory), nWords );
    if ( pnNodes )
        *pnNodes = Vec_WrdSize(vNodes) + Vec_WecSizeSize(vNodes2);
    Vec_WrdFree( vMemory );
    Vec_WrdFree( vNodes );
    Vec_WecFree( vNodes2 );
    ABC_FREE( pResult );

    Gia_ManHashStop( pNew );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}
Gia_Man_t * Abc_TtGiaMinArrayNew( word * p, int nVars, int nOuts, int * pnNodes, int fVerbose, int * pIPerm )
{
    Gia_Man_t * pNew, * pTemp;
    int o, i, iLit, nWords = Abc_TtWordNum(nVars);
    word * pF = ABC_ALLOC( word, nWords );
    word * pR = ABC_ALLOC( word, nWords );
    Vec_Wrd_t * vMemory = Vec_WrdAlloc( 100 );
    Vec_Wrd_t * vNodes  = Vec_WrdAlloc( 100 );
    Vec_Wec_t * vNodes2 = Vec_WecStart( nVars+1 );
    Vec_WrdGrow( vMemory, 1 << 20 );
    pNew = Gia_ManStart( 1000 );
    pNew->pName = Abc_UtilStrsav( "muxes" );
    for ( i = 0; i < nVars; i++ )
        Gia_ManAppendCi(pNew);
    Gia_ManHashAlloc( pNew );

    for ( o = 0; o < nOuts; o++ )
    {
        word * pCare = p + nOuts*nWords;
        word * pTruth = p + o*nWords;
        Abc_TtAnd( pF, pCare, pTruth, nWords, 0 );
        Abc_TtSharp( pR, pCare, pTruth, nWords );
        for ( i = nVars; i < 6; i++ )
            assert( !Abc_Tt6HasVar(pF[0], i) && !Abc_Tt6HasVar(pR[0], i) );
        Abc_TtGiaMin_rec( pNew, pF, pR, nVars, vMemory, vNodes, vNodes2, &iLit, pIPerm );
        Gia_ManAppendCo( pNew, iLit );
    }
    if ( fVerbose )
    printf( "Nodes = %5d.  Nodes2 = %5d.  Total = %5d.    ", 
        Vec_WrdSize(vNodes), Vec_WecSizeSize(vNodes2), Vec_WrdSize(vNodes) + Vec_WecSizeSize(vNodes2) );
    //printf( "Memory %d (Truth table %d)\n", Vec_WrdSize(vMemory), nWords );
    if ( pnNodes )
        *pnNodes = Vec_WrdSize(vNodes) + Vec_WecSizeSize(vNodes2);
    Vec_WrdFree( vMemory );
    Vec_WrdFree( vNodes );
    Vec_WecFree( vNodes2 );
    ABC_FREE( pF );
    ABC_FREE( pR );

    Gia_ManHashStop( pNew );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}


/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManBuildMuxes6_rec( Gia_Man_t * p, word t, int nVars, int * pPerm )
{
    int iLit0, iLit1, Var;
    assert( nVars <= 6 );
    if ( t == 0 )
        return 0;
    if ( ~t == 0 )
        return 1;
    assert( nVars > 0 );
    for ( Var = nVars-1; Var >= 0; Var-- )
        if ( Abc_Tt6HasVar( t, Var ) )
             break;
    assert( Var >= 0 );
    iLit0 = Gia_ManBuildMuxes6_rec( p, Abc_Tt6Cofactor0(t, Var), Var, pPerm );
    iLit1 = Gia_ManBuildMuxes6_rec( p, Abc_Tt6Cofactor1(t, Var), Var, pPerm );
    Var = pPerm ? pPerm[Var] : Var;
    return Gia_ManAppendMux( p, Abc_Var2Lit(1+Var, 0), iLit1, iLit0 );
}
int Gia_ManBuildMuxes_rec( Gia_Man_t * p, word * pTruth, int nVars, int * pPerm )
{
    int iLit0, iLit1, Var, nWords = Abc_TtWordNum(nVars);
    if ( nVars <= 6 )
        return Gia_ManBuildMuxes6_rec( p, pTruth[0], nVars, pPerm );
    if ( Abc_TtIsConst0(pTruth, nWords) )
        return 0;
    if ( Abc_TtIsConst1(pTruth, nWords) )
        return 1;
/*
    assert( nVars > 0 );
    if ( !Abc_TtHasVar( pTruth, nVars, nVars-1 ) )
        return Gia_ManBuildMuxes_rec( p, pTruth, nVars-1 );
    assert( nVars > 6 );
    iLit0 = Gia_ManBuildMuxes_rec( p, pTruth,          nVars-1 );
    iLit1 = Gia_ManBuildMuxes_rec( p, pTruth+nWords/2, nVars-1 );
*/
    assert( nVars > 0 );
    for ( Var = nVars-1; Var >= 0; Var-- )
        if ( Abc_TtHasVar( pTruth, nVars, Var ) )
             break;
    assert( Var >= 0 );
    if ( Var < 6 )
        return Gia_ManBuildMuxes6_rec( p, pTruth[0], Var+1, pPerm );
    iLit0 = Gia_ManBuildMuxes_rec( p, pTruth,                    Var, pPerm );
    iLit1 = Gia_ManBuildMuxes_rec( p, pTruth+Abc_TtWordNum(Var), Var, pPerm );
    Var = pPerm ? pPerm[Var] : Var;
    return Gia_ManAppendMux( p, Abc_Var2Lit(1+Var, 0), iLit1, iLit0 );
}
Gia_Man_t * Gia_ManBuildMuxesTest( word * pTruth, int nIns, int nOuts, int * pPerm )
{
    Gia_Man_t * pNew, * pTemp;
    int i, nWords = Abc_TtWordNum(nIns);
    pNew = Gia_ManStart( 1000 );
    pNew->pName = Abc_UtilStrsav( "muxes" );
    for ( i = 0; i < nIns; i++ )
        Gia_ManAppendCi(pNew);
    Gia_ManHashAlloc( pNew );
    for ( i = 0; i < nOuts; i++ )
        Gia_ManAppendCo( pNew, Gia_ManBuildMuxes_rec( pNew, pTruth+i*nWords, nIns, pPerm ) );
    Gia_ManHashStop( pNew );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}
Gia_Man_t * Gia_ManBuildMuxes( Tree_Sto_t * p, int * pIPerm )
{
    return Gia_ManBuildMuxesTest( p->pMem, p->nIns, p->nOuts, pIPerm ? pIPerm : p->pIPerm );
}
void Gia_ManDumpMuxes( Tree_Sto_t * p, char * pFileName, int * pIPerm )
{
    Gia_Man_t * pNew = Gia_ManBuildMuxes( p, pIPerm );
    Gia_AigerWrite( pNew, pFileName, 0, 0, 0 );
    Gia_ManStop( pNew );
    printf( "Finished dumping tree into AIG file \"%s\".\n", pFileName );
}
Gia_Man_t * Gia_ManCreateMuxGia( word * pTruths, int nIns, int nOuts, int nWords, int * pIPerm )
{
    Tree_Sto_t * pSto = Gia_ManContructTree( pTruths, nIns, nOuts, nWords );
    Gia_Man_t * pNew = Gia_ManBuildMuxes( pSto, pIPerm );
    //printf( "Internal nodes = %5d.\n", Gia_ManTreeCountNodes(pSto) );
    Gia_ManTreeFree( pSto );
    return pNew;
}
void Gia_ManDumpMuxGia( word * pTruths, int nIns, int nOuts, int nWords, int * pIPerm, char * pFileName )
{
    Tree_Sto_t * pSto = Gia_ManContructTree( pTruths, nIns, nOuts, nWords );
    Gia_ManDumpMuxes( pSto, pFileName, pIPerm );
    Gia_ManTreeFree( pSto );
}


/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_TryPermOptCare( word * pTruths, int nIns, int nOuts, int nWords, int nRounds, int fVerbose )
{
    abctime clk = Abc_Clock();
    Gia_Man_t * pNew;
    word * pTruthDup  = Abc_TtDup( pTruths, nOuts*nWords, 0 );
    word * pTruthBest = ABC_FALLOC( word, (nOuts+1)*nWords );
    int pIPermBest[TREE_MAX_VARS] = {0};
    int pIPerm[TREE_MAX_VARS] = {0};
    int r, rBest = -1, nNodes = -1, nNodesBest = ABC_INFINITY;
    //Gia_ManDumpMuxGia( pTruths, nIns, nOuts, nWords, NULL, "tt_beg.aig" );
    //srand( time(NULL) );
    Gia_ManRandom(1);
    for ( r = 0; r < nRounds; r++ )
    {
        nNodes = Gia_ManPermuteTreeOne( pTruthDup, nIns, nOuts, nWords, r>0, pIPerm, 0, fVerbose );
        if ( nNodesBest > nNodes )
        {
            nNodesBest = nNodes;
            memcpy( pIPermBest, pIPerm, sizeof(int)*nIns );
            Abc_TtCopy( pTruthBest, pTruthDup, nOuts*nWords, 0 );
            rBest = r;
        }
        Abc_TtCopy( pTruthDup, pTruths, nOuts*nWords, 0 );
        if ( fVerbose )
            printf( "\n" );
    }
    if ( fVerbose )
        printf( "Best round %3d. Best nodes %5d.  ", rBest, nNodesBest );
    ABC_FREE( pTruthDup );
    if ( fVerbose )
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    //pNew = Gia_ManCreateMuxGia( pTruthBest, nIns, nOuts, nWords, pIPermBest );
    pNew = Abc_TtSimpleMinArrayNew( pTruthBest, nIns, nOuts, NULL, 0, pIPermBest );
    //Gia_ManDumpMuxGia( pTruthBest, nIns, nOuts, nWords, pIPermBest, "tt_end.aig" );
    ABC_FREE( pTruthBest );
    return pNew;
}
Gia_Man_t * Gia_TryPermOpt2( word * pTruths, int nIns, int nOuts, int nWords, int nRounds, int fVerbose )
{
    abctime clk = Abc_Clock();
    Gia_Man_t * pNew;
    word * pRes, * pTruthDup = Abc_TtDup( pTruths, nOuts*nWords, 0 );
    word * pTruthBest = ABC_ALLOC( word, nOuts*nWords/2 );
    int pIPermBest[TREE_MAX_VARS] = {0};
    int pIPerm[TREE_MAX_VARS] = {0};
    int r, rBest = -1, nNodes = -1, nNodesBest = ABC_INFINITY;
    assert( nOuts % 2 == 0 );
    // collect onsets
    //for ( r = 0; r < nOuts/2; r++ )
    //    Abc_TtCopy( pTruthBest+r*nWords, pTruths+2*r*nWords, nWords, 0 );
    //Gia_ManDumpMuxGia( pTruthBest, nIns, nOuts/2, nWords, NULL, "tt_beg.aig" );
    //srand( time(NULL) );
    Gia_ManRandom(1);
    for ( r = 0; r < nRounds; r++ )
    {
        int nNodesAll = Gia_ManPermuteTreeOne( pTruthDup, nIns, nOuts, nWords, r>0, pIPerm, 0, fVerbose );
        pRes = Abc_TtMinArray( pTruthDup, nOuts, nIns, &nNodes, fVerbose );
        if ( nNodesBest > nNodes )
        {
            nNodesBest = nNodes;
            memcpy( pIPermBest, pIPerm, sizeof(int)*nIns );
            Abc_TtCopy( pTruthBest, pRes, nOuts*nWords/2, 0 );
            rBest = r;
        }
        ABC_FREE( pRes );
        Abc_TtCopy( pTruthDup, pTruths, nOuts*nWords, 0 );
        if ( fVerbose )
            printf( "\n" );
        nNodesAll = 0;
    }
    if ( fVerbose )
        printf( "Best round %3d. Best nodes %5d.  ", rBest, nNodesBest );
    ABC_FREE( pTruthDup );
    if ( fVerbose )
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    pNew = Gia_ManCreateMuxGia( pTruthBest, nIns, nOuts/2, nWords, pIPermBest );
    //Gia_ManDumpMuxGia( pTruthBest, nIns, nOuts/2, nWords, pIPermBest, "tt_end.aig" );
    ABC_FREE( pTruthBest );
    return pNew;
}
Gia_Man_t * Gia_TryPermOpt( word * pTruths, int nIns, int nOuts, int nWords, int nRounds, int fVerbose )
{
    abctime clk = Abc_Clock();
    Gia_Man_t * pBest = NULL;
    word * pTruthDup = Abc_TtDup( pTruths, nOuts*nWords, 0 );
    int pIPermBest[TREE_MAX_VARS] = {0};
    int pIPerm[TREE_MAX_VARS] = {0};
    int r, rBest = -1, nNodes2 = -1, nNodesBest = ABC_INFINITY;
    assert( nOuts % 2 == 0 );
    //srand( time(NULL) );
    Gia_ManRandom(1);
    for ( r = 0; r < nRounds; r++ )
    {
        int nNodesAll = Gia_ManPermuteTreeOne( pTruthDup, nIns, nOuts, nWords, r>0, pIPerm, 0, fVerbose );
        Gia_Man_t * pTemp = Abc_TtGiaMinArray( pTruthDup, nIns, nOuts, NULL, 0, pIPerm );
        nNodes2 = Gia_ManAndNum(pTemp);
        if ( nNodesBest > nNodes2 )
        {
            nNodesBest = nNodes2;
            memcpy( pIPermBest, pIPerm, sizeof(int)*nIns );
            rBest = r;

            Gia_ManStopP( &pBest );
            pBest = pTemp; 
            pTemp = NULL;
        }
        Gia_ManStopP( &pTemp );
        Abc_TtCopy( pTruthDup, pTruths, nOuts*nWords, 0 );
        if ( fVerbose )
            printf( "Permuted = %5d.  AIG = %5d.\n", nNodesAll, nNodes2 );
        nNodesAll = 0;
    }
    if ( fVerbose )
        printf( "Best round %3d. Best nodes %5d.  ", rBest, nNodesBest );
    ABC_FREE( pTruthDup );
    if ( fVerbose )
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    return pBest;
}
Gia_Man_t * Gia_TryPermOptNew( word * pTruths, int nIns, int nOuts, int nWords, int nRounds, int fVerbose )
{
    abctime clk = Abc_Clock();
    Gia_Man_t * pTemp, * pBest = NULL;
    word * pTruthDup = Abc_TtDup( pTruths, (nOuts+1)*nWords, 0 );
    int pIPermBest[TREE_MAX_VARS] = {0};
    int pIPerm[TREE_MAX_VARS] = {0};
    int r, rBest = -1, nNodes2 = -1, nNodesBest = ABC_INFINITY;
    //srand( time(NULL) );
    Gia_ManRandom(1);
    for ( r = 0; r < nRounds; r++ )
    {
        int nNodesAll = Gia_ManPermuteTreeOne( pTruthDup, nIns, nOuts, nWords, r>0, pIPerm, 0, fVerbose );
        Abc_TtPermute( pTruthDup + nOuts*nWords, pIPerm, nIns );
        //pTemp = Abc_TtGiaMinArrayNew( pTruthDup, nIns, nOuts, NULL, 0, pIPerm );
        pTemp = Abc_TtSimpleMinArrayNew( pTruthDup, nIns, nOuts, NULL, 0, pIPerm );
        nNodes2 = Gia_ManAndNum(pTemp);
        if ( nNodesBest > nNodes2 )
        {
            nNodesBest = nNodes2;
            memcpy( pIPermBest, pIPerm, sizeof(int)*nIns );
            rBest = r;

            Gia_ManStopP( &pBest );
            pBest = pTemp; 
            pTemp = NULL;
        }
        Gia_ManStopP( &pTemp );
/*
        for ( i = 0; i <= nOuts; i++ )
        {
            Abc_TtUnpermute( pTruthDup + i*nWords, pIPerm, nIns );
            if ( !Abc_TtEqual(pTruthDup + i*nWords, pTruths + i*nWords, nWords) )
                printf( "Verification failed for output %d (out of %d).\n", i, nOuts );
        }
*/
        Abc_TtCopy( pTruthDup, pTruths, (nOuts+1)*nWords, 0 );
        if ( fVerbose )
            printf( "Permuted = %5d.  AIG = %5d.\n", nNodesAll, nNodes2 );
        nNodesAll = 0;
    }
    if ( fVerbose )
        printf( "Best round %3d. Best nodes %5d.  ", rBest, nNodesBest );
    ABC_FREE( pTruthDup );
    if ( fVerbose )
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    return pBest;
}


/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_Tt6MinTest3( Gia_Man_t * p )
{
    word f = ABC_CONST(0x513B00000819050F);
    //word r = ABC_CONST(0xA000571507200000);
    word r = ~f;
    Vec_Wrd_t * vNodes = Vec_WrdAlloc( 100 );
    word Res = Abc_Tt6Min_rec( f, r, 6, vNodes );
    printf( "Nodes = %d.\n", Vec_WrdSize(vNodes) );
    if ( Res == f )
        printf( "Verification successful.\n" );
    else
        printf( "Verification FAILED.\n" );
    Vec_WrdFree( vNodes );
}
void Abc_Tt6MinTest2( Gia_Man_t * p )
{
    int fVerbose = 0;
    int i, nWords = Abc_TtWordNum(Gia_ManCiNum(p));
    word * pTruth = ABC_ALLOC( word, 3*nWords );
    word * pRes = NULL, * pTruths[3] = { pTruth, pTruth+nWords, pTruth+2*nWords };

    Vec_Int_t * vSupp   = Vec_IntAlloc( 100 );
    Vec_Wrd_t * vNodes  = Vec_WrdAlloc( 100 );
    Vec_Wec_t * vNodes2 = Vec_WecAlloc( 100 );
    Vec_Wrd_t * vMemory = Vec_WrdAlloc( 0 );

    Gia_Obj_t * pObj;
    Gia_ManForEachCi( p, pObj, i )
        Vec_IntPush( vSupp, Gia_ObjId(p, pObj) );

    Gia_ObjComputeTruthTableStart( p, Gia_ManCiNum(p) );
    assert( Gia_ManCoNum(p) == 3 );
    Gia_ManForEachCo( p, pObj, i )
    {
        word * pTruth = Gia_ObjComputeTruthTableCut( p, Gia_ObjFanin0(pObj), vSupp );
        Abc_TtCopy( pTruths[i], pTruth, nWords, Gia_ObjFaninC0(pObj) );
    }
    Gia_ObjComputeTruthTableStop( p );


    //Abc_TtSharp( pTruths[0], pTruths[0], pTruths[1], nWords );
    Abc_TtReverseVars( pTruths[0], Gia_ManCiNum(p) );
    Abc_TtCopy( pTruths[1], pTruths[0], nWords, 1 );

    pRes = Abc_TtMin( pTruths[0], pTruths[1], Gia_ManCiNum(p), vMemory, vNodes, vNodes2 );
    printf( "Nodes = %d.\n",  Vec_WrdSize(vNodes) );
    printf( "Nodes2 = %d.\n", Vec_WecSizeSize(vNodes2) );
    if ( Abc_TtEqual(pRes, pTruths[0], nWords) )
        printf( "Verification successful.\n" );
    else
        printf( "Verification FAILED.\n" );

    //printf( "Printing the tree:\n" );
//    Gia_ManPermuteTree( pTruths[0], Gia_ManCiNum(p), 1, nWords, fVerbose );
    Gia_ManPermuteTree( pTruth, Gia_ManCiNum(p), 3, nWords, 0, fVerbose );


/*
    Abc_TtReverseVars( pTruths[0], Gia_ManCiNum(p) );
    Abc_TtReverseVars( pTruths[1], Gia_ManCiNum(p) );
    Abc_TtReverseVars( pTruths[2], Gia_ManCiNum(p) );
    printf( "Printing the tree:\n" );
    Gia_ManContructTree( pTruth, Gia_ManCiNum(p), 3, nWords );
*/

/*
    pNew = Gia_ManBuildMuxesTest( pTruth, Gia_ManCiNum(p), Gia_ManCoNum(p), NULL );
    Gia_AigerWrite( pNew, "from_tt.aig", 0, 0, 0 );
    printf( "Dumping file \"%s\".\n", "from_tt.aig" );
    Gia_ManStop( pNew );
*/

    Vec_WrdFree( vMemory );
    Vec_WrdFree( vNodes );
    Vec_WecFree( vNodes2 );
    Vec_IntFree( vSupp );
    ABC_FREE( pTruth );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

