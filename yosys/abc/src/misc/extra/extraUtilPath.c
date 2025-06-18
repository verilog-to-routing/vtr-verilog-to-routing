/**CFile****************************************************************

  FileName    [extraUtilPath.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [extra]

  Synopsis    [Path enumeration.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: extraUtilPath.c,v 1.0 2003/02/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "aig/gia/gia.h"
#include "misc/vec/vecHsh.h"

#include <math.h>
#include "sat/bmc/bmc.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satStore.h"
#include "misc/extra/extra.h"


ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes AIG representing the set of all paths in the graph.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeVarX( int nSize, int y, int x )
{
    return Abc_Var2Lit( nSize * y + x, 0 );
}
int Abc_NodeVarY( int nSize, int y, int x )
{
    return Abc_Var2Lit( nSize * (nSize + 1) + nSize * x + y, 0 );
}

Gia_Man_t * Abc_EnumeratePaths( int nSize )
{
    Gia_Man_t * pTemp, * pGia = Gia_ManStart( 10000 );
    int * pNodes = ABC_CALLOC( int, nSize+1 );
    int x, y, nVars = 2*nSize*(nSize+1);
    for ( x = 0; x < nVars; x++ )
        Gia_ManAppendCi( pGia );
    Gia_ManHashAlloc( pGia );
    // y = 0; x = 0;
    pNodes[0] = 1;
    // y = 0; x > 0 
    for ( x = 1; x <= nSize; x++ )
        pNodes[x] = Gia_ManHashAnd( pGia, pNodes[x-1], Abc_NodeVarX(nSize, 0, x) );
    // y > 0; x >= 0
    for ( y = 1; y <= nSize; y++ )
    {
        // y > 0; x = 0
        pNodes[0] = Gia_ManHashAnd( pGia, pNodes[0], Abc_NodeVarY(nSize, y, 0) );
        // y > 0; x > 0
        for ( x = 1; x <= nSize; x++ )
        {
            int iHor  = Gia_ManHashAnd( pGia, pNodes[x-1], Abc_NodeVarX(nSize, y, x) );
            int iVer  = Gia_ManHashAnd( pGia, pNodes[x],   Abc_NodeVarY(nSize, y, x) );
            pNodes[x] = Gia_ManHashOr( pGia, iHor, iVer );
        }
    }
    Gia_ManAppendCo( pGia, pNodes[nSize] );
    pGia = Gia_ManCleanup( pTemp = pGia );
    Gia_ManStop( pTemp );
    ABC_FREE( pNodes );
    return pGia;
}
void Abc_EnumeratePathsTest()
{
    int nSize = 2;
    Gia_Man_t * pGia = Abc_EnumeratePaths( nSize );
    Gia_AigerWrite( pGia, "testpath.aig", 0, 0, 0 );
    Gia_ManStop( pGia );
}


/**Function*************************************************************

  Synopsis    [Generate NxN grid.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Abc_GraphGrid( int n )
{
    Vec_Int_t * vEdges = Vec_IntAlloc( 4*n*(n-1) ); // two nodes per edge
    int i, k;
    for ( i = 0; i < n; i++ )
    {
        for ( k = 0; k < n-1; k++ )
            Vec_IntPushTwo( vEdges, i*n+k, i*n+k+1 );
        if ( i == n-1 ) break;
        for ( k = 0; k < n; k++ )
            Vec_IntPushTwo( vEdges, i*n+k, i*n+k+n );
    }
    //Vec_IntPrint( vEdges );
    return vEdges;
}
Vec_Int_t * Abc_GraphNodeLife( Vec_Int_t * vEdges, int n )
{
    Vec_Int_t * vLife = Vec_IntStartFull( 2*n*n ); // start/stop per node
    int One, Two, i;
    Vec_IntForEachEntryDouble( vEdges, One, Two, i )
    {
        if ( Vec_IntEntry(vLife, 2*One) == -1 )
            Vec_IntWriteEntry(vLife, 2*One, i/2);
        if ( Vec_IntEntry(vLife, 2*Two) == -1 )
            Vec_IntWriteEntry(vLife, 2*Two, i/2);
        Vec_IntWriteEntry(vLife, 2*One+1, i/2);
        Vec_IntWriteEntry(vLife, 2*Two+1, i/2);
    }
    //Vec_IntPrint( vLife );
    return vLife;
}
Vec_Wec_t * Abc_GraphFrontiers( Vec_Int_t * vEdges, Vec_Int_t * vLife )
{
    Vec_Wec_t * vFronts = Vec_WecAlloc( Vec_IntSize(vEdges)/2 ); // front for each edge
    Vec_Int_t * vTemp = Vec_IntAlloc( Vec_IntSize(vLife)/2 );
    int e, n;    
    Vec_WecPushLevel(vFronts);
    for ( e = 0; e < Vec_IntSize(vEdges)/2; e++ )
    {
        int * pNodes = Vec_IntEntryP(vEdges, 2*e);
        for ( n = 0; n < 2; n++ )
            if ( Vec_IntEntry(vLife, 2*pNodes[n]) == e ) // first time
                Vec_IntPush( vTemp, pNodes[n] );
            else if ( Vec_IntEntry(vLife, 2*pNodes[n]+1) == e ) // last time
                Vec_IntRemove( vTemp, pNodes[n] );
        Vec_IntAppend( Vec_WecPushLevel(vFronts), vTemp );
    }
    //Vec_WecPrint( vFronts, 0 );
    Vec_IntFree( vTemp );
    return vFronts;
}

/**Function*************************************************************

  Synopsis    [Print grid.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_GraphPathPrint4( int * pBuffer, Vec_Int_t * vEdges )
{
    char Box[13][13];
    int x, y;
    int e, nEdges = Vec_IntSize(vEdges)/2;
    for ( y = 0; y < 13; y++ )
    for ( x = 0; x < 13; x++ )
        if ( y % 4 == 0 && x % 4 == 0 )
            Box[y][x] = '*';
        else
            Box[y][x] = ' ';
    for ( e = 0; e < nEdges; e++ )
    {
        int * pNodes = Vec_IntEntryP(vEdges, 2*e);
        int y0 = 4*(pNodes[0]/4);
        int x0 = 4*(pNodes[0]%4);
        int y1 = 4*(pNodes[1]/4);
        int x1 = 4*(pNodes[1]%4);
        if ( !pBuffer[e] )
            continue;
        if ( y0 == y1 )
        {
            for ( x = x0+1; x < x1; x++ )
                Box[y0][x] = '-';
        }
        else if ( x0 == x1 )
        {
            for ( y = y0+1; y < y1; y++ )
                Box[y][x0] = '|';
        }
        else assert( 0 );
    }
    for ( y = 0; y < 13; y++, printf("\n") )
    for ( x = 0; x < 13; x++ )
        printf( "%c", Box[y][x] );
    printf( "\n\n=================================\n\n" );
}
void Abc_GraphPathPrint5( int * pBuffer, Vec_Int_t * vEdges )
{
    char Box[17][17];
    int x, y;
    int e, nEdges = Vec_IntSize(vEdges)/2;
    for ( y = 0; y < 17; y++ )
    for ( x = 0; x < 17; x++ )
        if ( y % 4 == 0 && x % 4 == 0 )
            Box[y][x] = '*';
        else
            Box[y][x] = ' ';
    for ( e = 0; e < nEdges; e++ )
    {
        int * pNodes = Vec_IntEntryP(vEdges, 2*e);
        int y0 = 4*(pNodes[0]/5);
        int x0 = 4*(pNodes[0]%5);
        int y1 = 4*(pNodes[1]/5);
        int x1 = 4*(pNodes[1]%5);
        if ( !pBuffer[e] )
            continue;
        if ( y0 == y1 )
        {
            for ( x = x0+1; x < x1; x++ )
                Box[y0][x] = '-';
        }
        else if ( x0 == x1 )
        {
            for ( y = y0+1; y < y1; y++ )
                Box[y][x0] = '|';
        }
        else assert( 0 );
    }
    for ( y = 0; y < 17; y++, printf("\n") )
    for ( x = 0; x < 17; x++ )
        printf( "%c", Box[y][x] );
    printf( "\n\n=================================\n\n" );
}

/**Function*************************************************************

  Synopsis    [Count paths.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
double Abc_GraphCountPaths_rec( int Lev, int Node, Vec_Wec_t * vNodes, double ** pCache, int * pBuffer, Vec_Int_t * vEdges )
{
    double Res0, Res1;
//    if ( Node == -2 ) Abc_GraphPathPrint4( pBuffer, vEdges );
    if ( Node == -2 )
        return 1;
    if ( Node == -1 )
        return 0;
    if ( pCache[Lev][Node] != -1.0 )
        return pCache[Lev][Node];
    pBuffer[Lev] = 0;
    Res0 = Abc_GraphCountPaths_rec( Lev+1, Vec_IntEntry( Vec_WecEntry(vNodes, Lev), 2*Node   ), vNodes, pCache, pBuffer, vEdges );
    pBuffer[Lev] = 1;
    Res1 = Abc_GraphCountPaths_rec( Lev+1, Vec_IntEntry( Vec_WecEntry(vNodes, Lev), 2*Node+1 ), vNodes, pCache, pBuffer, vEdges );
    return (pCache[Lev][Node] = Res0 + Res1);
}
double Abc_GraphCountPaths( Vec_Wec_t * vNodes, Vec_Int_t * vEdges )
{
    int i, k, pBuffer[1000] = {0};
    double ** pCache = ABC_ALLOC( double *, Vec_WecSize(vNodes) );
    Vec_Int_t * vLevel;   double Value;
    Vec_WecForEachLevel( vNodes, vLevel, i )
    {
        pCache[i] = ABC_ALLOC( double, Vec_IntSize(vLevel) );
        for ( k = 0; k < Vec_IntSize(vLevel); k++ )
            pCache[i][k] = -1.0;
    }
    Value = Abc_GraphCountPaths_rec( 0, 0, vNodes, pCache, pBuffer, vEdges );
    for ( i = 0; i < Vec_WecSize(vNodes); i++ )
        ABC_FREE( pCache[i] );
    ABC_FREE( pCache );
    return Value;
}

/**Function*************************************************************

  Synopsis    [Build AIG for paths.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_GraphDeriveGia_rec( Gia_Man_t * p, int Lev, int Node, Vec_Wec_t * vNodes, int ** pCache, int * pBuffer, Vec_Int_t * vEdges )
{
    int Res0, Res1;
    if ( Node == -2 )
        return 1;
    if ( Node == -1 )
        return 0;
    if ( pCache[Lev][Node] != -1 )
        return pCache[Lev][Node];
    pBuffer[Lev] = 0;
    Res0 = Abc_GraphDeriveGia_rec( p, Lev+1, Vec_IntEntry( Vec_WecEntry(vNodes, Lev), 2*Node   ), vNodes, pCache, pBuffer, vEdges );
    pBuffer[Lev] = 1;
    Res1 = Abc_GraphDeriveGia_rec( p, Lev+1, Vec_IntEntry( Vec_WecEntry(vNodes, Lev), 2*Node+1 ), vNodes, pCache, pBuffer, vEdges );
    return ( pCache[Lev][Node] = Gia_ManHashMux(p, Gia_Obj2Lit(p, Gia_ManCi(p, Lev)), Res1, Res0) );
}
Gia_Man_t * Abc_GraphDeriveGia( Vec_Wec_t * vNodes, Vec_Int_t * vEdges )
{
    int ** pCache;
    int i, Value, pBuffer[1000] = {0};  
    Vec_Int_t * vLevel;
    // start AIG
    Gia_Man_t * pTemp, * p = Gia_ManStart( 1000 );
    p->pName = Abc_UtilStrsav("paths");
    for ( i = 0; i < Vec_IntSize(vEdges)/2; i++ )
        Gia_ManAppendCi(p);
    Gia_ManHashAlloc(p);
    // alloc cache
    pCache = ABC_ALLOC( int *, Vec_WecSize(vNodes) );
    Vec_WecForEachLevel( vNodes, vLevel, i )
        pCache[i] = ABC_FALLOC( int, Vec_IntSize(vLevel) );
    Value = Abc_GraphDeriveGia_rec( p, 0, 0, vNodes, pCache, pBuffer, vEdges );
    for ( i = 0; i < Vec_WecSize(vNodes); i++ )
        ABC_FREE( pCache[i] );
    ABC_FREE( pCache );
    // cleanup
    Gia_ManAppendCo( p, Value );
    p = Gia_ManCleanup( pTemp = p );
    Gia_ManStop( pTemp );
    return p;
}
void Abc_GraphDeriveGiaDump( Vec_Wec_t * vNodes, Vec_Int_t * vEdges, int Size )
{
    char pFileName[100];
    Gia_Man_t * pGia = Abc_GraphDeriveGia( vNodes, vEdges );
    sprintf( pFileName, "grid_%dx%d_e%03d.aig", Size, Size, Vec_IntSize(vEdges)/2 );
    Gia_AigerWrite( pGia, pFileName, 0, 0, 0 );
    Gia_ManStop( pGia );
    printf( "Finished dumping AIG into file \"%s\".\n", pFileName );
}

/**Function*************************************************************

  Synopsis    [Build frontier.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_GraphBuildState( Vec_Int_t * vState, int e, int x, Vec_Int_t * vEdges, Vec_Int_t * vLife, Vec_Wec_t * vFronts, int * pFront, Vec_Int_t * vStateNew, int fVerbose )
{
    Vec_Int_t * vFront = Vec_WecEntry( vFronts, e );
    Vec_Int_t * vFront2 = Vec_WecEntry( vFronts, e+1 );
    int * pNodes = Vec_IntEntryP(vEdges, 2*e);
    int nNodes = Vec_IntSize(vLife)/2;
    int i, n, Node, First, pEquivs[2];
    assert( pNodes[0] < pNodes[1] );
    if ( fVerbose ) printf( "Edge = %d. Arc = %d.\nCurrent state: ", e, x );
    Vec_IntForEachEntry( vFront, Node, i )
    {
        pFront[Node] = Vec_IntEntry(vState, i);
        if ( fVerbose ) printf( "%d(%d) ", pFront[Node] & 0xFFFF, pFront[Node] >> 16 );
    }
    if ( fVerbose ) printf( "\n" );
    for ( n = 0; n < 2; n++ )
        if ( Vec_IntEntry(vLife, 2*pNodes[n]) == e ) // first time
            pFront[pNodes[n]] = pNodes[n]; // degree = 0; comp = singleton
    if ( x )
    {
        if ( (pFront[pNodes[0]] & 0xFFFF) == (pFront[pNodes[1]] & 0xFFFF) ) // the same comp
            return -1; // const 0
        for ( n = 0; n < 2; n++ )
        {
            int Degree = pFront[pNodes[n]] >> 16;
            if ( (pNodes[n] == 0 || pNodes[n] == nNodes-1) ? Degree >= 1 : Degree >= 2 )
                return -1; // const 0
            pFront[pNodes[n]] += (1 << 16); // degree++
        }
    }
    // remember equivalence classes
    pEquivs[0] = pFront[pNodes[0]] & 0xFFFF;
    pEquivs[1] = pFront[pNodes[1]] & 0xFFFF;
    // remove some nodes from the frontier
    for ( n = 0; n < 2; n++ )
        if ( Vec_IntEntry(vLife, 2*pNodes[n]+1) == e ) // last time
        {
            int Degree = pFront[pNodes[n]] >> 16;
            if ( (pNodes[n] == 0 || pNodes[n] == nNodes-1) ? Degree != 1 : Degree != 0 && Degree != 2 )
                return -1; // const 0
            // if it is part of the comp, update
            First = -1;
            Vec_IntForEachEntry( vFront2, Node, i )
            {
                assert( Node != pNodes[n] );
                if ( (pFront[Node] & 0xFFFF) == pEquivs[n] )
                {
                    if ( First == -1 )
                        First = Node;
                    pFront[Node] = (pFront[Node] & 0xFFFF0000) | First;
                }
            }
            if ( First != -1 )
                pEquivs[n] = First;
        }
    if ( x )
    {
        // union comp
        int First = -1;
        Vec_IntForEachEntry( vFront2, Node, i )
            if ( (pFront[Node] & 0xFFFF) == pEquivs[0] || (pFront[Node] & 0xFFFF) == pEquivs[1] )
            {
                if ( First == -1 )
                    First = Node;
                pFront[Node] = (pFront[Node] & 0xFFFF0000) | First;
            }
    }
    // create next state
    Vec_IntClear( vStateNew );
    if ( fVerbose ) printf( "Next state: " );
    Vec_IntForEachEntry( vFront2, Node, i )
    {
        Vec_IntPush( vStateNew, pFront[Node] );
        if ( fVerbose ) printf( "%d(%d) ", pFront[Node] & 0xFFFF, pFront[Node] >> 16 );
    }
    if ( fVerbose ) printf( "\n\n" );
    return 1;
}
void Abc_GraphBuildFrontier( int nSize, Vec_Int_t * vEdges, Vec_Int_t * vLife, Vec_Wec_t * vFronts, int fVerbose )
{
    abctime clk = Abc_Clock();
    double nPaths;
    int nEdges = Vec_IntSize(vEdges)/2;
    int nNodes = Vec_IntSize(vLife)/2;
    Vec_Wec_t * vNodes = Vec_WecAlloc( nEdges );
    Vec_Int_t * vStateNew = Vec_IntAlloc( nNodes );
    Vec_Int_t * vStateCount = Vec_IntAlloc( nEdges );
    int e, s, x, Next, * pFront = ABC_CALLOC( int, nNodes );
    Hsh_VecMan_t * pThis = Hsh_VecManStart( 1000 );
    Hsh_VecMan_t * pNext = Hsh_VecManStart( 1000 );
    Hsh_VecManAdd( pThis, vStateNew );
    for ( e = 0; e < nEdges; e++ )
    {
        Vec_Int_t * vNode = Vec_WecPushLevel(vNodes);
        int nStates = Hsh_VecSize( pThis );
        Vec_IntPush( vStateCount, nStates );
        if ( fVerbose )
        {
            printf( "\n" );
            printf( "Processing edge %d = {%d %d}\n", e, Vec_IntEntry(vEdges, 2*e), Vec_IntEntry(vEdges, 2*e+1) );
            printf( "Frontier: " );  Vec_IntPrint( Vec_WecEntry(vFronts, e) );
            printf( "\n" );
        }
        for ( s = 0; s < nStates; s++ )
        {
            Vec_Int_t * vState = Hsh_VecReadEntry(pThis, s);
            for ( x = 0; x < 2; x++ )
            {
                Next = Abc_GraphBuildState(vState, e, x, vEdges, vLife, vFronts, pFront, vStateNew, fVerbose);
                if ( Next == 1 )
                {
                    if ( e == nEdges - 1 ) // last edge
                        Next = -2; // const1
                    else
                        Next = Hsh_VecManAdd( pNext, vStateNew );
                }
                if ( fVerbose ) printf( "Return value = %d\n", Next );
                Vec_IntPush( vNode, Next );
            }
        }
        Hsh_VecManStop( pThis );
        pThis = pNext;
        pNext = Hsh_VecManStart( 1000 );
    }
    nPaths = Abc_GraphCountPaths(vNodes, vEdges);
    printf( "States = %8d   Paths = %24.0f  ", Vec_IntSum(vStateCount), nPaths ); 
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    if ( fVerbose )
        Vec_IntPrint( vStateCount );
    Abc_GraphDeriveGiaDump( vNodes, vEdges, nSize );
    ABC_FREE( pFront );
    Vec_WecFree( vNodes );
    Vec_IntFree( vStateNew );
    Vec_IntFree( vStateCount );
    Hsh_VecManStop( pThis );
    Hsh_VecManStop( pNext );
}
void Abc_EnumerateFrontierTest( int nSize )
{
    //int nSize    = 3;
    int fVerbose = 0;
    Vec_Int_t * vEdges = Abc_GraphGrid( nSize );
    Vec_Int_t * vLife = Abc_GraphNodeLife( vEdges, nSize );
    Vec_Wec_t * vFronts = Abc_GraphFrontiers( vEdges, vLife );

    Abc_GraphBuildFrontier( nSize, vEdges, vLife, vFronts, fVerbose );

    Vec_WecFree( vFronts );
    Vec_IntFree( vLife );
    Vec_IntFree( vEdges );
}


/**Function*************************************************************

  Synopsis    [Performs SAT-based path enumeration.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
double Abc_Word2Double( word w )
{
    double Res = 0;  int i;
    for ( i = 0; i < 64; i++ )
        if ( (w >> i) & 1 )
            Res += pow(2,i);
    return Res;
}
void Abc_GraphSolve( Gia_Man_t * pGia )
{
    int nIters = 1000;
    Cnf_Dat_t * pCnf = (Cnf_Dat_t *)Mf_ManGenerateCnf( pGia, 8, 0, 1, 0, 0 );
    sat_solver * pSat; Vec_Int_t * vLits = Vec_IntAlloc( 100 );
    int i, k, iLit, nVars = Gia_ManCiNum(pGia);
    int iCiVarBeg = pCnf->nVars - nVars;
    word Total = 0;
    word Mint1 = 0;
    word Mint2 = 0;

    // restart the SAT solver
    pSat = sat_solver_new();
    sat_solver_setnvars( pSat, pCnf->nVars );
    // add timeframe clauses
    for ( i = 0; i < pCnf->nClauses; i++ )
        if ( !sat_solver_addclause( pSat, pCnf->pClauses[i], pCnf->pClauses[i+1] ) )
            assert( 0 );
    // create trivial assignment
    Vec_IntClear( vLits );
    for ( k = 0; k < nVars; k++ )
        Vec_IntPush( vLits, Abc_Var2Lit(iCiVarBeg+k, 1) );
    // generate random assignment
    for ( i = 0; i < nIters; i++ )
    {
        int Status = sat_solver_solve_lexsat( pSat, Vec_IntArray(vLits), Vec_IntSize(vLits) );
        if ( Status != l_True )
            break;
        assert( Status == l_True );
        // block this assignment
        Vec_IntForEachEntry( vLits, iLit, k )
            Vec_IntWriteEntry( vLits, k, Abc_LitNot(iLit) );
        if ( !sat_solver_addclause( pSat, Vec_IntArray(vLits), Vec_IntLimit(vLits) ) )
            break;
        Vec_IntForEachEntry( vLits, iLit, k )
            Vec_IntWriteEntry( vLits, k, Abc_LitNot(iLit) );
        // collect new minterm
        Mint2 = 0;
        Vec_IntForEachEntry( vLits, iLit, k )
            if ( !Abc_LitIsCompl(iLit) )
                Mint2 |= ((word)1) << (nVars-1-k);
        if ( Mint1 == 0 )
            Mint1 = Mint2;
        // report
        //printf( "Iter %3d : ", i );
        //Extra_PrintBinary( stdout, (unsigned *)&Mint2, Abc_MinInt(64, nVars) ); printf( "\n" );
    }
    //Mint1 = 0;
    Total = (Mint2-Mint1)/nIters;
    printf( "Vars = %d   Iters = %d   Ave = %.0f   Total = %.0f  ", nVars, nIters, Abc_Word2Double(Mint2-Mint1), Abc_Word2Double(Total) );
    printf( "Estimate = %.0f\n", (pow(2,nVars)-Abc_Word2Double(Mint1))/Abc_Word2Double((Mint2-Mint1)/nIters) );

    sat_solver_delete( pSat );
    Cnf_DataFree( pCnf );
    Vec_IntFree( vLits );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

