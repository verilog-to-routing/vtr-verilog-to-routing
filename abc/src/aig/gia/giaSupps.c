/**CFile****************************************************************

  FileName    [giaSupp.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Support computation manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaSupp.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig/gia/gia.h"
#include "base/main/mainInt.h"
#include "misc/util/utilTruth.h"
#include "misc/extra/extra.h"
#include "misc/vec/vecHsh.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Supp_Man_t_ Supp_Man_t;
struct Supp_Man_t_
{
    // user data
    int         nIters;     // optimization rounds
    int         nRounds;    // optimization rounds
    int         nWords;     // the number of simulation words
    int         nDivWords;  // the number of divisor words
    Vec_Wrd_t * vIsfs;      // functions to synthesize
    Vec_Int_t * vCands;     // candidate divisors
    Vec_Int_t * vWeights;   // divisor weights (optional)
    Vec_Wrd_t * vSims;      // simulation values
    Vec_Wrd_t * vSimsC;     // simulation values
    Gia_Man_t * pGia;       // used for object names
    // computed data
    Vec_Wrd_t * vDivs[2];   // simulation values
    Vec_Wrd_t * vDivsC[2];  // simulation values
    Vec_Wrd_t * vPats[2];   // simulation values
    Vec_Ptr_t * vMatrix;    // simulation values
    Vec_Wrd_t * vMask;      // simulation values
    Vec_Wrd_t * vRowTemp;   // simulation values
    Vec_Int_t * vCosts;     // candidate costs
    Hsh_VecMan_t * pHash;   // subsets considered
    Vec_Wrd_t * vSFuncs;    // ISF storage
    Vec_Int_t * vSStarts;   // subset function starts
    Vec_Int_t * vSCount;    // subset function count
    Vec_Int_t * vSPairs;    // subset pair count
    Vec_Int_t * vTemp;      // temporary
    Vec_Int_t * vTempSets;  // temporary
    Vec_Int_t * vTempPairs; // temporary
    Vec_Wec_t * vSolutions; // solutions for each node count
};

extern void Extra_BitMatrixTransposeP( Vec_Wrd_t * vSimsIn, int nWordsIn, Vec_Wrd_t * vSimsOut, int nWordsOut );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Supp_ManFuncInit( Vec_Wrd_t * vFuncs, int nWords )
{
    int i, k = 0, nFuncs = Vec_WrdSize(vFuncs) / nWords / 2;
    assert( 2 * nFuncs * nWords == Vec_WrdSize(vFuncs) );
    for ( i = 0; i < nFuncs; i++ )
    {
        word * pFunc0 = Vec_WrdEntryP(vFuncs, (2*i+0)*nWords);
        word * pFunc1 = Vec_WrdEntryP(vFuncs, (2*i+1)*nWords);
        if ( Abc_TtIsConst0(pFunc0, nWords) || Abc_TtIsConst0(pFunc1, nWords) )
            continue;
        if ( k < i ) Abc_TtCopy( Vec_WrdEntryP(vFuncs, (2*k+0)*nWords), pFunc0, nWords, 0 );
        if ( k < i ) Abc_TtCopy( Vec_WrdEntryP(vFuncs, (2*k+1)*nWords), pFunc1, nWords, 0 );
        k++;
    }
    Vec_WrdShrink( vFuncs, 2*k*nWords );
    return k;
}
int Supp_ManCostInit( Vec_Wrd_t * vFuncs, int nWords )
{
    int Res = 0, i, nFuncs = Vec_WrdSize(vFuncs) / nWords / 2;
    assert( 2 * nFuncs * nWords == Vec_WrdSize(vFuncs) );
    for ( i = 0; i < nFuncs; i++ )
    {
        word * pFunc0 = Vec_WrdEntryP(vFuncs, (2*i+0)*nWords);
        word * pFunc1 = Vec_WrdEntryP(vFuncs, (2*i+1)*nWords);
        Res += Abc_TtCountOnesVec(pFunc0, nWords) * Abc_TtCountOnesVec(pFunc1, nWords);
    }
    assert( nFuncs < 128 );
    assert( Res < (1 << 24) );
    return (nFuncs << 24) | Res;
}
void Supp_ManInit( Supp_Man_t * p )
{
    int Value, nFuncs, iSet = Hsh_VecManAdd( p->pHash, p->vTemp ); // empty set
    assert( iSet == 0 );
    Vec_IntPush( p->vSStarts, Vec_WrdSize(p->vSFuncs) );
    Vec_WrdAppend( p->vSFuncs, p->vIsfs );
    nFuncs = Supp_ManFuncInit( p->vSFuncs, p->nWords );
    assert( Vec_WrdSize(p->vSFuncs) == 2 * nFuncs * p->nWords );
    Value = Supp_ManCostInit(p->vSFuncs, p->nWords);
    Vec_IntPush( p->vSCount, Value >> 24 );
    Vec_IntPush( p->vSPairs, Value & 0xFFFFFF );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Supp_DeriveLines( Supp_Man_t * p )
{
    int n, i, iObj, nWords = p->nWords;
    int nDivWords = Abc_Bit6WordNum( Vec_IntSize(p->vCands) );
    //Vec_IntPrint( p->vCands );
    for ( n = 0; n < 2; n++ )
    {
        p->vDivs[n] = Vec_WrdStart( 64*nWords*nDivWords );
        p->vPats[n] = Vec_WrdStart( 64*nWords*nDivWords );
        //p->vDivsC[n] = Vec_WrdStart( 64*nWords*nDivWords );
        if ( p->vSimsC )
            Vec_IntForEachEntry( p->vCands, iObj, i )
                Abc_TtAndSharp( Vec_WrdEntryP(p->vDivsC[n], i*nWords), Vec_WrdEntryP(p->vSimsC, iObj*nWords), Vec_WrdEntryP(p->vSims, iObj*nWords), nWords, !n );
        else
            Vec_IntForEachEntry( p->vCands, iObj, i )
                Abc_TtCopy( Vec_WrdEntryP(p->vDivs[n], i*nWords), Vec_WrdEntryP(p->vSims, iObj*nWords), nWords, !n );
        Extra_BitMatrixTransposeP( p->vDivs[n], nWords, p->vPats[n], nDivWords );
    }
    return nDivWords;
}
Supp_Man_t * Supp_ManCreate( Vec_Wrd_t * vIsfs, Vec_Int_t * vCands, Vec_Int_t * vWeights, Vec_Wrd_t * vSims, Vec_Wrd_t * vSimsC, int nWords, Gia_Man_t * pGia, int nIters, int nRounds )
{
    Supp_Man_t * p = ABC_CALLOC( Supp_Man_t, 1 );
    p->nIters     = nIters;
    p->nRounds    = nRounds;
    p->nWords     = nWords;
    p->vIsfs      = vIsfs;
    p->vCands     = vCands;
    p->vWeights   = vWeights;
    p->vSims      = vSims;
    p->vSimsC     = vSimsC;
    p->pGia       = pGia;
    // computed data
    p->nDivWords  = Supp_DeriveLines( p );
    p->vMatrix    = Vec_PtrAlloc( 100 );
    p->vMask      = Vec_WrdAlloc( 100 );
    p->vRowTemp   = Vec_WrdStart( 64*p->nDivWords );
    p->vCosts     = Vec_IntStart( Vec_IntSize(p->vCands) );
    p->pHash      = Hsh_VecManStart( 1000 );
    p->vSFuncs    = Vec_WrdAlloc( 1000 );
    p->vSStarts   = Vec_IntAlloc( 1000 );
    p->vSCount    = Vec_IntAlloc( 1000 );
    p->vSPairs    = Vec_IntAlloc( 1000 );
    p->vSolutions = Vec_WecStart( 16 );
    p->vTemp      = Vec_IntAlloc( 10 );
    p->vTempSets  = Vec_IntAlloc( 10 );
    p->vTempPairs = Vec_IntAlloc( 10 );
    Supp_ManInit( p );
    return p;
}
void Supp_ManCleanMatrix( Supp_Man_t * p )
{
    Vec_Wrd_t * vTemp; int i;
    Vec_PtrForEachEntry( Vec_Wrd_t *, p->vMatrix, vTemp, i )
        Vec_WrdFreeP( &vTemp );
    Vec_PtrClear( p->vMatrix );
}
void Supp_ManDelete( Supp_Man_t * p )
{
    Supp_ManCleanMatrix( p );
    Vec_WrdFreeP( &p->vDivs[0] );
    Vec_WrdFreeP( &p->vDivs[1] );
    Vec_WrdFreeP( &p->vDivsC[0] );
    Vec_WrdFreeP( &p->vDivsC[1] );
    Vec_WrdFreeP( &p->vPats[0] );
    Vec_WrdFreeP( &p->vPats[1] );
    Vec_PtrFreeP( &p->vMatrix );
    Vec_WrdFreeP( &p->vMask );
    Vec_WrdFreeP( &p->vRowTemp );
    Vec_IntFreeP( &p->vCosts );
    Hsh_VecManStop( p->pHash );
    Vec_WrdFreeP( &p->vSFuncs );
    Vec_IntFreeP( &p->vSStarts );
    Vec_IntFreeP( &p->vSCount );
    Vec_IntFreeP( &p->vSPairs );
    Vec_WecFreeP( &p->vSolutions );
    Vec_IntFreeP( &p->vTemp );
    Vec_IntFreeP( &p->vTempSets );
    Vec_IntFreeP( &p->vTempPairs );
    ABC_FREE( p );
}
int Supp_ManMemory( Supp_Man_t * p )
{
    int nMem = sizeof(Supp_Man_t);
    nMem += 2*(int)Vec_WrdMemory( p->vDivs[0] );
    nMem += 2*(int)Vec_WrdMemory( p->vPats[0] );
    nMem += (Vec_PtrSize(p->vMatrix)+1)*(int)Vec_WrdMemory( p->vRowTemp );
    nMem += (int)Vec_WrdMemory( p->vMask );
    nMem += (int)Vec_IntMemory( p->vCosts );
    nMem += (int)Hsh_VecManMemory( p->pHash );
    nMem += (int)Vec_WrdMemory( p->vSFuncs );
    nMem += (int)Vec_IntMemory( p->vSStarts );
    nMem += (int)Vec_IntMemory( p->vSCount );
    nMem += (int)Vec_IntMemory( p->vSPairs );
    nMem += (int)Vec_WecMemory( p->vSolutions );
    nMem += (int)Vec_IntMemory( p->vTemp );
    nMem += (int)Vec_IntMemory( p->vTempSets );
    nMem += (int)Vec_IntMemory( p->vTempPairs );
    return nMem;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Supp_ArrayWeight( Vec_Int_t * vRes, Vec_Int_t * vWeights )
{
    int i, iObj, Cost = 0;
    if ( !vWeights )
        return Vec_IntSize(vRes);
    Vec_IntForEachEntry( vRes, iObj, i )
        Cost += Vec_IntEntry(vWeights, iObj);
    return Cost;
}
int Supp_SetWeight( Supp_Man_t * p, int iSet )
{
    return Supp_ArrayWeight( Hsh_VecReadEntry(p->pHash, iSet), p->vWeights );
}
int Supp_SetSize( Supp_Man_t * p, int iSet )
{
    return Vec_IntSize( Hsh_VecReadEntry(p->pHash, iSet) );
}
int Supp_SetFuncNum( Supp_Man_t * p, int iSet )
{
    return Vec_IntEntry(p->vSCount, iSet);
}
int Supp_SetPairNum( Supp_Man_t * p, int iSet )
{
    return Vec_IntEntry(p->vSPairs, iSet);
}
void Supp_SetConvert( Vec_Int_t * vSet, Vec_Int_t * vCands )
{
    int i, iObj;
    Vec_IntForEachEntry( vSet, iObj, i )
        Vec_IntWriteEntry( vSet, i, Vec_IntEntry(vCands, iObj) );
}
void Supp_PrintNodes( Gia_Man_t * pGia, Vec_Int_t * vObjs, int Skip, int Limit )
{
    int i, iObj;
    //printf( "Considering %d targets: ", Vec_IntSize(vObjs) );
    Vec_IntForEachEntryStart( vObjs, iObj, i, Skip )
    {
        if ( iObj < 0 )
            continue;
        printf( "(%d)", iObj );
        if ( pGia && pGia->vWeights && Vec_IntEntry(pGia->vWeights, iObj) > 0 )
            printf( "(%d)", Vec_IntEntry(pGia->vWeights, iObj) );
        if ( pGia )
            printf( " %s  ", Gia_ObjName(pGia, iObj)+2 );
        else
            printf( " n%d  ", iObj );
        if ( i >= Limit )
        {
            printf( "...  " );
            break;
        }
    }
    printf( "Cost = %d", Supp_ArrayWeight(vObjs, pGia ? pGia->vWeights : NULL) );
    printf( "\n" );
}
void Supp_PrintOne( Supp_Man_t * p, int iSet )
{
    Vec_Int_t * vSet = Hsh_VecReadEntry(p->pHash, iSet);
    printf( "Set %5d :  ", iSet );
    printf( "Funcs %2d  ", Supp_SetFuncNum(p, iSet) );
    printf( "Pairs %4d  ", Supp_SetPairNum(p, iSet) );
    printf( "Start %8d  ", Vec_IntEntry(p->vSStarts, iSet) );
    printf( "Weight %4d  ", Supp_ArrayWeight(vSet, p->vWeights) );
    Vec_IntClearAppend( p->vTemp, vSet );
    Supp_SetConvert( p->vTemp, p->vCands );
    Supp_PrintNodes( p->pGia, p->vTemp, 0, 6 );
}
int Supp_ManRefine1( Supp_Man_t * p, int iSet, int iObj )
{
    word * pSet = Vec_WrdEntryP( p->vSims, Vec_IntEntry(p->vCands, iObj)*p->nWords );
    word * pIsf; int nFuncs  = Vec_IntEntry(p->vSCount, iSet);
    int i, n, f, w, nFuncsNew = 0, Mark = Vec_WrdSize(p->vSFuncs), Res = 0;
    if ( Vec_WrdSize(p->vSFuncs) + 4*nFuncs*p->nWords > Vec_WrdCap(p->vSFuncs) )
        Vec_WrdGrow( p->vSFuncs, 2*Vec_WrdCap(p->vSFuncs) );
    pIsf = Vec_WrdEntryP( p->vSFuncs, Vec_IntEntry(p->vSStarts, iSet) );
    for ( i = 0; i < nFuncs; i++ )
    {
        word * pFunc[2] = { pIsf + (2*i+0)*p->nWords, pIsf + (2*i+1)*p->nWords };
        for ( f = 0; f < 2; f++ )
        {
            int nOnes[2], Start = Vec_WrdSize(p->vSFuncs);
            for ( n = 0; n < 2; n++ )
            {
                word * pLimit = Vec_WrdLimit(p->vSFuncs);
                if ( f )
                    for ( w = 0; w < p->nWords; w++ )
                        Vec_WrdPush( p->vSFuncs, pFunc[n][w] & pSet[w] );
                else
                    for ( w = 0; w < p->nWords; w++ )
                        Vec_WrdPush( p->vSFuncs, pFunc[n][w] & ~pSet[w] );
                nOnes[n] = Abc_TtCountOnesVec( pLimit, p->nWords );
            }
            if ( nOnes[0] && nOnes[1] )
                Res += nOnes[0] * nOnes[1];
            else
                Vec_WrdShrink( p->vSFuncs, Start );
        }
    }
    assert( Res < (1 << 24) );
    nFuncsNew = (Vec_WrdSize(p->vSFuncs) - Mark)/2/p->nWords;
    assert( nFuncsNew < 128 );
    assert( nFuncsNew >= 0 && nFuncsNew <= 2*nFuncs );
    return (nFuncsNew << 24) | Res;
}
void Supp_ManRefine( Supp_Man_t * p, int iSet, int iObj, int * pnFuncs, int * pnPairs )
{
    word * pDivs0 = Vec_WrdEntryP( p->vDivs[0], iObj*p->nWords );
    word * pDivs1 = Vec_WrdEntryP( p->vDivs[1], iObj*p->nWords );
    word * pIsf; int nFuncs = Supp_SetFuncNum(p, iSet); 
    int i, f, w, nFuncsNew = 0, Mark = Vec_WrdSize(p->vSFuncs), Res = 0;
    if ( Vec_WrdSize(p->vSFuncs) + 6*nFuncs*p->nWords > Vec_WrdCap(p->vSFuncs) )
        Vec_WrdGrow( p->vSFuncs, 2*Vec_WrdCap(p->vSFuncs) );
    if ( Vec_WrdSize(p->vSFuncs) == Vec_IntEntry(p->vSStarts, iSet) )
        pIsf = Vec_WrdLimit( p->vSFuncs );
    else
        pIsf = Vec_WrdEntryP( p->vSFuncs, Vec_IntEntry(p->vSStarts, iSet) );
    for ( i = 0; i < nFuncs; i++ )
    {
        word * pFunc[2] = { pIsf + (2*i+0)*p->nWords, pIsf + (2*i+1)*p->nWords };
        for ( f = 0; f < 3; f++ )
        {
            int nOnes[2], Start = Vec_WrdSize(p->vSFuncs);
            word * pLimit[2] = { Vec_WrdLimit(p->vSFuncs), Vec_WrdLimit(p->vSFuncs) + p->nWords };
            if ( f == 0 )
            {
                for ( w = 0; w < p->nWords; w++ )
                    Vec_WrdPush( p->vSFuncs, pFunc[0][w] &  pDivs0[w] );
                for ( w = 0; w < p->nWords; w++ )
                    Vec_WrdPush( p->vSFuncs, pFunc[1][w] & ~pDivs1[w] );
            }
            else if ( f == 1 )
            {
                for ( w = 0; w < p->nWords; w++ )
                    Vec_WrdPush( p->vSFuncs, pFunc[0][w] &  pDivs1[w] );
                for ( w = 0; w < p->nWords; w++ )
                    Vec_WrdPush( p->vSFuncs, pFunc[1][w] & ~pDivs0[w] );
            }
            else
            {
                for ( w = 0; w < p->nWords; w++ )
                    Vec_WrdPush( p->vSFuncs, pFunc[0][w] & ~pDivs0[w] & ~pDivs1[w] );
                for ( w = 0; w < p->nWords; w++ )
                    Vec_WrdPush( p->vSFuncs, pFunc[1][w] );
            }
            nOnes[0] = Abc_TtCountOnesVec( pLimit[0], p->nWords );
            nOnes[1] = Abc_TtCountOnesVec( pLimit[1], p->nWords );
            if ( nOnes[0] && nOnes[1] )
                Res += nOnes[0] * nOnes[1];
            else
                Vec_WrdShrink( p->vSFuncs, Start );
        }
    }
    assert( Res < (1 << 24) );
    nFuncsNew = (Vec_WrdSize(p->vSFuncs) - Mark)/2/p->nWords;
    *pnFuncs = nFuncsNew;
    *pnPairs = Res;
}
int Supp_ManSubsetAdd( Supp_Man_t * p, int iSet, int iObj, int fVerbose )
{
    int iSetNew, nEntries = Hsh_VecSize( p->pHash );
    Vec_Int_t * vSet = Hsh_VecReadEntry( p->pHash, iSet );
    Vec_IntClear( p->vTemp );
    Vec_IntAppend( p->vTemp, vSet );
    Vec_IntPushOrder( p->vTemp, iObj );
    assert( Vec_IntIsOrdered(p->vTemp, 0) );
    iSetNew = Hsh_VecManAdd( p->pHash, p->vTemp );
    if ( iSetNew == nEntries ) // new entry
    {
        int nFuncs, nPairs;
        Vec_IntPush( p->vSStarts, Vec_WrdSize(p->vSFuncs) );
        Supp_ManRefine( p, iSet, iObj, &nFuncs, &nPairs );
        Vec_IntPush( p->vSCount, nFuncs );
        Vec_IntPush( p->vSPairs, nPairs );
        assert( Hsh_VecSize(p->pHash) == Vec_IntSize(p->vSStarts) );
        assert( Hsh_VecSize(p->pHash) == Vec_IntSize(p->vSCount) );
        assert( Hsh_VecSize(p->pHash) == Vec_IntSize(p->vSPairs) );
        if ( Supp_SetFuncNum(p, iSetNew) == 0 && Supp_SetSize(p, iSetNew) < Vec_WecSize(p->vSolutions) )
            Vec_WecPush( p->vSolutions, Supp_SetSize(p, iSetNew), iSetNew );
        if ( fVerbose )
            Supp_PrintOne( p, iSetNew );
    }
    return iSetNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Supp_ComputePair1( Supp_Man_t * p, int iSet )
{
    int Random = 0xFFFFFF & Abc_Random(0);
    int nFuncs = Vec_IntEntry(p->vSCount, iSet);
    int iFunc = Random % nFuncs; 
    word * pIsf = Vec_WrdEntryP( p->vSFuncs, Vec_IntEntry(p->vSStarts, iSet) );
    word * pFunc[2] = { pIsf + (2*iFunc+0)*p->nWords, pIsf + (2*iFunc+1)*p->nWords };
    int iBit0 = ((Random >> 16) & 1) ? Abc_TtFindFirstBit2(pFunc[0], p->nWords) : Abc_TtFindLastBit2(pFunc[0], p->nWords);
    int iBit1 = ((Random >> 17) & 1) ? Abc_TtFindFirstBit2(pFunc[1], p->nWords) : Abc_TtFindLastBit2(pFunc[1], p->nWords);
    if ( 1 )
    {
        Vec_Int_t * vSet = Hsh_VecReadEntry( p->pHash, iSet ); int i, iObj;
        Vec_IntForEachEntry( vSet, iObj, i )
        {
            word * pSet = Vec_WrdEntryP( p->vSims, Vec_IntEntry(p->vCands, iObj)*p->nWords );
            assert( Abc_TtGetBit(pSet, iBit0) == Abc_TtGetBit(pSet, iBit1) );
        }
    }
    return (iBit0 << 16) | iBit1;
}
int Supp_ComputePair( Supp_Man_t * p, int iSet )
{
    int Random = 0xFFFFFF & Abc_Random(0);
    int nFuncs = Vec_IntEntry(p->vSCount, iSet);
    int iFunc = Random % nFuncs; 
    word * pIsf = Vec_WrdEntryP( p->vSFuncs, Vec_IntEntry(p->vSStarts, iSet) );
    word * pFunc[2] = { pIsf + (2*iFunc+0)*p->nWords, pIsf + (2*iFunc+1)*p->nWords };
    int iBit0 = ((Random >> 16) & 1) ? Abc_TtFindFirstBit2(pFunc[0], p->nWords) : Abc_TtFindLastBit2(pFunc[0], p->nWords);
    int iBit1 = ((Random >> 17) & 1) ? Abc_TtFindFirstBit2(pFunc[1], p->nWords) : Abc_TtFindLastBit2(pFunc[1], p->nWords);
    if ( 1 )
    {
        Vec_Int_t * vSet = Hsh_VecReadEntry( p->pHash, iSet ); int i, iObj;
        Vec_IntForEachEntry( vSet, iObj, i )
        {
            word * pSet0 = Vec_WrdEntryP( p->vDivs[0], iObj*p->nWords );
            word * pSet1 = Vec_WrdEntryP( p->vDivs[1], iObj*p->nWords );
            int Value00 = Abc_TtGetBit(pSet0, iBit0);
            int Value01 = Abc_TtGetBit(pSet0, iBit1);
            int Value10 = Abc_TtGetBit(pSet1, iBit0);
            int Value11 = Abc_TtGetBit(pSet1, iBit1);
            assert( !Value00 || !Value11 );
            assert( !Value01 || !Value10 );
        }
    }
    return (iBit0 << 16) | iBit1;
}
Vec_Int_t * Supp_Compute64Pairs( Supp_Man_t * p, Vec_Int_t * vSets )
{
    int i;
    Vec_IntClear( p->vTempPairs );
    for ( i = 0; i < 64; i++ )
    {
        int Rand = 0xFFFFFF & Abc_Random(0);
        int iSet = Vec_IntEntry( vSets, Rand % Vec_IntSize(vSets) );
        Vec_IntPush( p->vTempPairs, Supp_ComputePair(p, iSet) );
    }
    return p->vTempPairs;
}
void Supp_ManFillBlock( Supp_Man_t * p, Vec_Int_t * vPairs, Vec_Wrd_t * vRes )
{
    int i, Pair;
    assert( Vec_IntSize(vPairs) == 64 );
    Vec_IntForEachEntry( vPairs, Pair, i )
    {
        int iBit0 = Pair >> 16, iBit1 = Pair & 0xFFFF; 
        word * pPat00 = Vec_WrdEntryP(p->vPats[0], iBit0*p->nDivWords);
        word * pPat01 = Vec_WrdEntryP(p->vPats[0], iBit1*p->nDivWords);
        word * pPat10 = Vec_WrdEntryP(p->vPats[1], iBit0*p->nDivWords);
        word * pPat11 = Vec_WrdEntryP(p->vPats[1], iBit1*p->nDivWords);
        word * pPat   = Vec_WrdEntryP(p->vRowTemp, i*p->nDivWords);
        Abc_TtAnd( pPat, pPat00, pPat11, p->nDivWords, 0 );
        Abc_TtOrAnd( pPat, pPat01, pPat10, p->nDivWords );
    }
    Extra_BitMatrixTransposeP( p->vRowTemp, p->nDivWords, vRes, 1 );
}
void Supp_ManAddPatterns( Supp_Man_t * p, Vec_Int_t * vSets )
{
    Vec_Int_t * vPairs = Supp_Compute64Pairs( p, vSets );
    Vec_Wrd_t * vRow = Vec_WrdStart( 64*p->nDivWords );
    Supp_ManFillBlock( p, vPairs, vRow );
    Vec_PtrPush( p->vMatrix, vRow );
}
Vec_Int_t * Supp_ManCollectOnes( word * pSim, int nWords )
{
    Vec_Int_t * vRes = Vec_IntAlloc( 100 ); int i;
    for ( i = 0; i < 64*nWords; i++ )
        if ( Abc_TtGetBit(pSim, i) )
            Vec_IntPush( vRes, i );
    return vRes;
}
Vec_Int_t * Supp_Compute64PairsFunc( Supp_Man_t * p, Vec_Int_t * vBits0, Vec_Int_t * vBits1 )
{
    int i;
    Vec_IntClear( p->vTempPairs );
    for ( i = 0; i < 64; i++ )
    {
        int Random = 0xFFFFFF & Abc_Random(0);
        int iBit0 = Vec_IntEntry( vBits0, (Random & 0xFFF) % Vec_IntSize(vBits0) );
        int iBit1 = Vec_IntEntry( vBits1, (Random >>   12) % Vec_IntSize(vBits1) );
        Vec_IntPush( p->vTempPairs, (iBit0 << 16) | iBit1 );
    }
    return p->vTempPairs;
}
void Supp_ManAddPatternsFunc( Supp_Man_t * p, int nBatches )
{
    int b;
    Vec_Int_t * vBits0 = Supp_ManCollectOnes( Vec_WrdEntryP(p->vSFuncs, 0*p->nWords), p->nWords );
    Vec_Int_t * vBits1 = Supp_ManCollectOnes( Vec_WrdEntryP(p->vSFuncs, 1*p->nWords), p->nWords );
    for ( b = 0; b < nBatches; b++ )
    {
        Vec_Int_t * vPairs = Supp_Compute64PairsFunc( p, vBits0, vBits1 );
        Vec_Wrd_t * vRow = Vec_WrdStart( 64*p->nDivWords );
        Supp_ManFillBlock( p, vPairs, vRow );
        Vec_PtrPush( p->vMatrix, vRow );
    }
    Vec_IntFree( vBits0 );
    Vec_IntFree( vBits1 );
}
int Supp_FindNextDiv( Supp_Man_t * p, int Pair )
{
    int iDiv, iBit0 = Pair >> 16, iBit1 = Pair & 0xFFFF; 
    word * pPat00 = Vec_WrdEntryP(p->vPats[0], iBit0*p->nDivWords);
    word * pPat01 = Vec_WrdEntryP(p->vPats[0], iBit1*p->nDivWords);
    word * pPat10 = Vec_WrdEntryP(p->vPats[1], iBit0*p->nDivWords);
    word * pPat11 = Vec_WrdEntryP(p->vPats[1], iBit1*p->nDivWords);
    int iDiv1 = Abc_TtFindFirstAndBit2( pPat00, pPat11, p->nDivWords );
    int iDiv2 = Abc_TtFindFirstAndBit2( pPat01, pPat10, p->nDivWords );
    iDiv1 = iDiv1 == -1 ? ABC_INFINITY : iDiv1;
    iDiv2 = iDiv2 == -1 ? ABC_INFINITY : iDiv2;
    iDiv = Abc_MinInt( iDiv1, iDiv2 );
    assert( iDiv >= 0 && iDiv < Vec_IntSize(p->vCands) );
    return iDiv;
}
int Supp_ManRandomSolution( Supp_Man_t * p, int iSet, int fVerbose )
{
    Vec_IntClear( p->vTempSets );
    while ( Supp_SetFuncNum(p, iSet) > 0 )
    {
        int Pair = Supp_ComputePair( p, iSet );
        int iDiv = Supp_FindNextDiv( p, Pair );
        iSet = Supp_ManSubsetAdd( p, iSet, iDiv, fVerbose );
        if ( Supp_SetFuncNum(p, iSet) > 0 )
            Vec_IntPush( p->vTempSets, iSet );
    }
    if ( Vec_IntSize(p->vTempSets) < 2 )
        return iSet;
    Supp_ManAddPatterns( p, p->vTempSets );
    return iSet;
}
int Supp_ManSubsetRemove( Supp_Man_t * p, int iSet, int iPos )
{
    int i, iSetNew = 0, nSize = Supp_SetSize(p, iSet);
    for ( i = 0; i < nSize; i++ )
        if ( i != iPos && Supp_SetFuncNum(p, iSetNew) > 0 )
            iSetNew = Supp_ManSubsetAdd( p, iSetNew, Vec_IntEntry(Hsh_VecReadEntry(p->pHash, iSet), i), 0 );
    return iSetNew;
}
int Supp_ManMinimize( Supp_Man_t * p, int iSet0, int r, int fVerbose )
{
    int i, nSize = Supp_SetSize(p, iSet0);
    Vec_Int_t * vPerm = Vec_IntStartNatural( Supp_SetSize(p, iSet0) );
    Vec_IntRandomizeOrder( vPerm );
    Vec_IntClear( p->vTempSets );
    if ( fVerbose )
    printf( "Removing items from %d:\n", iSet0 );
    // make sure we first try to remove items with higher weight
    for ( i = 0; i < nSize; i++ )
    {
        int Index = Vec_IntEntry( vPerm, i );
        int iSet = Supp_ManSubsetRemove( p, iSet0, Index );
        if ( fVerbose )
        printf( "Item %2d : ", Index ); 
        if ( fVerbose )
        Supp_PrintOne( p, iSet );
        if ( Supp_SetFuncNum(p, iSet) == 0 )
        {
            Vec_IntFree( vPerm );
            return Supp_ManMinimize( p, iSet, r, fVerbose );
        }
        Vec_IntPush( p->vTempSets, iSet );
    }
    Supp_ManAddPatterns( p, p->vTempSets );
    Vec_IntFree( vPerm );
    return iSet0;
}
int Supp_ManFindNextObj( Supp_Man_t * p, int fVerbose )
{
    Vec_Wrd_t * vTemp; int r, k, iDiv;
    word Sim, * pMask = Vec_WrdArray(p->vMask);
    assert( Vec_WrdSize(p->vMask) == Vec_PtrSize(p->vMatrix) );
    Vec_IntFill( p->vCosts, Vec_IntSize(p->vCosts), 0 );
    Vec_PtrForEachEntry( Vec_Wrd_t *, p->vMatrix, vTemp, r )
        Vec_WrdForEachEntryStop( vTemp, Sim, k, Vec_IntSize(p->vCosts) )
            Vec_IntAddToEntry( p->vCosts, k, Abc_TtCountOnes(Sim & pMask[r]) );
    iDiv = Vec_IntArgMax( p->vCosts );
    if ( fVerbose )
    printf( "Choosing divisor %d with weight %d.\n", iDiv, Vec_IntEntry(p->vCosts, iDiv) );
    Vec_PtrForEachEntry( Vec_Wrd_t *, p->vMatrix, vTemp, r )
        pMask[r] &= ~Vec_WrdEntry( vTemp, iDiv );
    return iDiv;
}
int Supp_ManReconstruct( Supp_Man_t * p, int fVerbose )
{
    int iSet = 0;
    Vec_WrdFill( p->vMask, Vec_PtrSize(p->vMatrix), ~(word)0 );
    if ( fVerbose )
    printf( "\nBuilding a new set:\n" );
    while ( Supp_SetPairNum(p, iSet) )
    {
        int iDiv = Supp_ManFindNextObj( p, fVerbose );
        iSet = Supp_ManSubsetAdd( p, iSet, iDiv, fVerbose );
        if ( Abc_TtIsConst0(Vec_WrdArray(p->vMask), Vec_WrdSize(p->vMask)) )
            break;
    }
    if ( fVerbose )
    printf( "Adding random part:\n" );
    return Supp_ManRandomSolution( p, iSet, fVerbose );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int s_Counter = 0;

void Supp_DeriveDumpSims( FILE * pFile, Vec_Wrd_t * vDivs, int nWords )
{
    int i, k, nDivs = Vec_WrdSize(vDivs)/nWords;
    for ( i = 0; i < nDivs; i++ )
    {
        word * pSim = Vec_WrdEntryP(vDivs, i*nWords);
        for ( k = 0; k < 64*nWords; k++ )
            fprintf( pFile, "%c", '0'+Abc_TtGetBit(pSim, k) );
        fprintf( pFile, "\n" );
    }
}
void Supp_DeriveDumpSimsC( FILE * pFile, Vec_Wrd_t * vDivs[2], int nWords )
{
    int i, k, nDivs = Vec_WrdSize(vDivs[0])/nWords;
    for ( i = 0; i < nDivs; i++ )
    {
        word * pSim0 = Vec_WrdEntryP(vDivs[0], i*nWords);
        word * pSim1 = Vec_WrdEntryP(vDivs[1], i*nWords);
        for ( k = 0; k < 64*nWords; k++ )
            if ( Abc_TtGetBit(pSim0, k) )
                fprintf( pFile, "0" );
            else if ( Abc_TtGetBit(pSim1, k) )
                fprintf( pFile, "1" );
            else
                fprintf( pFile, "-" );
        fprintf( pFile, "\n" );
    }
}
void Supp_DeriveDumpProb( Vec_Wrd_t * vIsfs, Vec_Wrd_t * vDivs, int nWords )
{
    char Buffer[100]; int nDivs = Vec_WrdSize(vDivs)/nWords;
    int RetValue = sprintf( Buffer, "%02d.resub", s_Counter );
    FILE * pFile = fopen( Buffer, "wb" );
    if ( pFile == NULL )
        printf( "Cannot open output file.\n" );
    fprintf( pFile, "resyn %d %d %d %d\n", 0, nDivs, 1, 64*nWords );
    //fprintf( pFile, "%d %d %d %d\n", 0, nDivs, 1, 64*nWords );
    Supp_DeriveDumpSims( pFile, vDivs, nWords );
    Supp_DeriveDumpSims( pFile, vIsfs, nWords );
    fclose ( pFile );
    RetValue = 0;
}
void Supp_DeriveDumpProbC( Vec_Wrd_t * vIsfs, Vec_Wrd_t * vDivs[2], int nWords )
{
    char Buffer[100]; int nDivs = Vec_WrdSize(vDivs[0])/nWords;
    int RetValue = sprintf( Buffer, "%02d.resub", s_Counter );
    FILE * pFile = fopen( Buffer, "wb" );
    if ( pFile == NULL )
        printf( "Cannot open output file.\n" );
    fprintf( pFile, "resyn %d %d %d %d\n", 0, nDivs, 1, 64*nWords );
    //fprintf( pFile, "%d %d %d %d\n", 0, nDivs, 1, 64*nWords );
    Supp_DeriveDumpSimsC( pFile, vDivs, nWords );
    Supp_DeriveDumpSims( pFile, vIsfs, nWords );
    fclose ( pFile );
    RetValue = 0;
}
void Supp_DeriveDumpSol( Vec_Int_t * vSet, Vec_Int_t * vRes, int nDivs )
{
    char Buffer[100];
    int RetValue = sprintf( Buffer, "%02d.sol", s_Counter );
    int i, iLit, iLitRes = -1, nSupp = Vec_IntSize(vSet);
    FILE * pFile = fopen( Buffer, "wb" );
    if ( pFile == NULL )
        printf( "Cannot open output file.\n" );
    fprintf( pFile, "sol name aig %d\n", Vec_IntSize(vRes)/2 );
    //Vec_IntPrint( vSet );
    //Vec_IntPrint( vRes );
    Vec_IntForEachEntry( vRes, iLit, i )
    {
        assert( iLit != 2 && iLit != 3 );
        if ( iLit < 2 )
            iLitRes = iLit;
        else if ( iLit-4 < 2*nSupp )
        {
            int iDiv = Vec_IntEntry(vSet, Abc_Lit2Var(iLit-4));
            assert( iDiv >= 0 && iDiv < nDivs );
            iLitRes = Abc_Var2Lit(1+iDiv, Abc_LitIsCompl(iLit));
        }
        else
            iLitRes = iLit + 2*((nDivs+1)-(nSupp+2));
        fprintf( pFile, "%d ", iLitRes );
    }
    if ( Vec_IntSize(vRes) & 1 )
        fprintf( pFile, "%d ", iLitRes );
    fprintf( pFile, "\n" );
    fclose( pFile );
    RetValue = 0;
    printf( "Dumped solution info file \"%s\".\n", Buffer );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Supp_ManFindBestSolution( Supp_Man_t * p, Vec_Wec_t * vSols, int fVerbose, Vec_Int_t ** pvDivs )
{
    Vec_Int_t * vLevel, * vRes = NULL; 
    int i, k, iSet, nFuncs = 0, Count = 0;
    int iSolBest = -1, Cost, CostBest = ABC_INFINITY;
    Vec_WecForEachLevel( vSols, vLevel, i )
    {
        Count += Vec_IntSize(vLevel) > 0;
        Vec_IntForEachEntry( vLevel, iSet, k )
        {
            if ( fVerbose )
                printf( "%3d : ", nFuncs++ );
            Cost = Gia_ManEvalSolutionOne( p->pGia, p->vSims, p->vIsfs, p->vCands, Hsh_VecReadEntry(p->pHash, iSet), p->nWords, fVerbose );
            if ( Cost == -1 )
                continue;
            if ( CostBest > Cost )
            {
                CostBest = Cost;
                iSolBest = iSet;
            }
            if ( nFuncs > 5 )
                break;
        }
        if ( Count == 2 || k < Vec_IntSize(vLevel) )
            break;
    }
    if ( iSolBest > 0 && (CostBest >> 2) < 50 )
    {
        Vec_Int_t * vSet = Hsh_VecReadEntry( p->pHash, iSolBest ); int i, iObj;
        vRes = Gia_ManDeriveSolutionOne( p->pGia, p->vSims, p->vIsfs, p->vCands, vSet, p->nWords, CostBest & 3 );
        assert( !vRes || Vec_IntSize(vRes) == 2*(CostBest >> 2)+1 );
        if ( vRes && pvDivs )
        {
            Vec_IntClear( *pvDivs );
            Vec_IntPushTwo( *pvDivs, -1, -1 );
            Vec_IntForEachEntry( vSet, iObj, i )
                Vec_IntPush( *pvDivs, Vec_IntEntry(p->vCands, iObj) );
        }
        //Supp_DeriveDumpProbC( p->vIsfs, p->vDivsC, p->nWords );
        //Supp_DeriveDumpProb( p->vIsfs, p->vDivs[1], p->nWords );
        //Supp_DeriveDumpSol( vSet, vRes, Vec_WrdSize(p->vDivs[1])/p->nWords );
        //s_Counter++;
    }
    return vRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Supp_FindGivenOne( Supp_Man_t * p )
{
    int i, iObj, iSet = 0;
    Vec_Int_t * vSet = Vec_IntStart( 2 );
    //extern void Patch_GenerateSet11( Gia_Man_t * p, Vec_Int_t * vDivs, Vec_Int_t * vCands );
    //Patch_GenerateSet11( p->pGia, vSet, p->vCands );
    Vec_IntDrop( vSet, 0 );
    Vec_IntDrop( vSet, 0 );
    Vec_IntForEachEntry( vSet, iObj, i )
        iSet = Supp_ManSubsetAdd( p, iSet, iObj, 1 );
    Vec_IntFree( vSet );
    return iSet;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Supp_ManCompute( Vec_Wrd_t * vIsfs, Vec_Int_t * vCands, Vec_Int_t * vWeights, Vec_Wrd_t * vSims, Vec_Wrd_t * vSimsC, int nWords, Gia_Man_t * pGia, Vec_Int_t ** pvDivs, int nIters, int nRounds, int fVerbose )
{
    int fVeryVerbose = 0;
    int i, r, iSet, iBest = -1;
    abctime clk = Abc_Clock();
    Vec_Int_t * vRes = NULL; 
    Supp_Man_t * p = Supp_ManCreate( vIsfs, vCands, vWeights, vSims, vSimsC, nWords, pGia, nIters, nRounds );
    if ( Supp_SetFuncNum(p, 0) == 0 )
    {
        Supp_ManDelete( p );
        Vec_IntClear( *pvDivs );
        Vec_IntPushTwo( *pvDivs, -1, -1 );
        vRes = Vec_IntAlloc( 1 );
        Vec_IntPush( vRes, Abc_TtIsConst0(Vec_WrdArray(vIsfs), nWords) );
        return vRes;
    }
    if ( fVerbose )
    printf( "\nUsing %d divisors with %d words. Problem has %d functions and %d minterm pairs.\n", 
        Vec_IntSize(p->vCands), p->nWords, Supp_SetFuncNum(p, 0), Supp_SetPairNum(p, 0) );
    //iBest = Supp_FindGivenOne( p );
    if ( iBest == -1 )
    for ( i = 0; i < p->nIters; i++ )
    {
        Supp_ManAddPatternsFunc( p, i );
        iSet = Supp_ManRandomSolution( p, 0, fVeryVerbose );
        for ( r = 0; r < p->nRounds; r++ )
        {
            if ( fVeryVerbose )
            printf( "\n\nITER %d   ROUND %d\n", i, r );
            iSet = Supp_ManMinimize( p, iSet, r, fVeryVerbose );
            if ( iBest == -1 || Supp_SetWeight(p, iBest) > Supp_SetWeight(p, iSet) )
            //if ( Supp_SetSize(p, iBest) > Supp_SetSize(p, iSet) )
            {
                if ( fVeryVerbose )
                printf( "\nThe best solution found:\n" );
                if ( fVeryVerbose )
                Supp_PrintOne( p, iSet );
                iBest = iSet;
            }
            iSet = Supp_ManReconstruct( p, fVeryVerbose );
        }
        if ( fVeryVerbose )
        printf( "Matrix size %d.\n", Vec_PtrSize(p->vMatrix) );
        Supp_ManCleanMatrix( p );
    }
    if ( fVerbose )
    {
        printf( "Explored %d divisor sets. Found %d solutions. Memory usage %.2f MB.  ", 
            Hsh_VecSize(p->pHash), Vec_WecSizeSize(p->vSolutions), 1.0*Supp_ManMemory(p)/(1<<20) );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
        printf( "The best solution:  " );
        if ( iBest == -1 )
            printf( "No solution.\n" );
        else
            Supp_PrintOne( p, iBest );
    }
    vRes = Supp_ManFindBestSolution( p, p->vSolutions, fVerbose, pvDivs );
    //Vec_IntPrint( vRes );
    Supp_ManDelete( p );
//    if ( vRes && Vec_IntSize(vRes) == 0 )
//        Vec_IntFreeP( &vRes );
    return vRes;
}
void Supp_ManComputeTest( Gia_Man_t * p )
{
    Vec_Wrd_t * vSimsPi = Vec_WrdStartTruthTables( Gia_ManCiNum(p) );
    Vec_Wrd_t * vSims   = Gia_ManSimPatSimOut( p, vSimsPi, 0 );
    int i, iPoId, nWords = Vec_WrdSize(vSimsPi) / Gia_ManCiNum(p);
    Vec_Wrd_t * vIsfs = Vec_WrdStart( 2*nWords );
    Vec_Int_t * vCands = Vec_IntAlloc( 4 );
    Vec_Int_t * vRes;

//    for ( i = 0; i < Gia_ManCiNum(p)+5; i++ )
    for ( i = 0; i < Gia_ManCiNum(p); i++ )
        Vec_IntPush( vCands, 1+i );

    iPoId = Gia_ObjId(p, Gia_ManPo(p, 0));
    Abc_TtCopy( Vec_WrdEntryP(vIsfs, 0*nWords), Vec_WrdEntryP(vSims, iPoId*nWords), nWords, 1 );
    Abc_TtCopy( Vec_WrdEntryP(vIsfs, 1*nWords), Vec_WrdEntryP(vSims, iPoId*nWords), nWords, 0 );

    vRes = Supp_ManCompute( vIsfs, vCands, NULL, vSims, NULL, nWords, p, NULL, 1, 1, 0 );
    Vec_IntPrint( vRes );

    Vec_WrdFree( vSimsPi );
    Vec_WrdFree( vSims );
    Vec_WrdFree( vIsfs );
    Vec_IntFree( vCands );
    Vec_IntFree( vRes );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

