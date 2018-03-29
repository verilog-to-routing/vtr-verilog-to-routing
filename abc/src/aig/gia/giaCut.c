/**CFile****************************************************************

  FileName    [giaCut.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Stand-alone cut computation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaCut.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "misc/util/utilTruth.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define GIA_MAX_CUTSIZE    8
#define GIA_MAX_CUTNUM     51
#define GIA_MAX_TT_WORDS   ((GIA_MAX_CUTSIZE > 6) ? 1 << (GIA_MAX_CUTSIZE-6) : 1)

#define GIA_CUT_NO_LEAF   0xF

typedef struct Gia_Cut_t_ Gia_Cut_t; 
struct Gia_Cut_t_
{
    word            Sign;                      // signature
    int             iFunc;                     // functionality
    int             Cost;                      // cut cost
    int             CostLev;                   // cut cost
    unsigned        nTreeLeaves  : 28;         // tree leaves
    unsigned        nLeaves      :  4;         // leaf count
    int             pLeaves[GIA_MAX_CUTSIZE];  // leaves
};

typedef struct Gia_Sto_t_ Gia_Sto_t; 
struct Gia_Sto_t_
{
    int             nCutSize;
    int             nCutNum;
    int             fCutMin;
    int             fTruthMin;
    int             fVerbose;
    Gia_Man_t *     pGia;                      // user's AIG manager (will be modified by adding nodes)
    Vec_Int_t *     vRefs;                     // refs for each node
    Vec_Wec_t *     vCuts;                     // cuts for each node
    Vec_Mem_t *     vTtMem;                    // truth tables
    Gia_Cut_t       pCuts[3][GIA_MAX_CUTNUM];  // temporary cuts
    Gia_Cut_t *     ppCuts[GIA_MAX_CUTNUM];    // temporary cut pointers
    int             nCutsR;                    // the number of cuts
    int             Pivot;                     // current object
    int             iCutBest;                  // best-delay cut
    int             nCutsOver;                 // overflow cuts
    double          CutCount[4];               // cut counters
    abctime         clkStart;                  // starting time
};

static inline word * Gia_CutTruth( Gia_Sto_t * p, Gia_Cut_t * pCut ) { return Vec_MemReadEntry(p->vTtMem, Abc_Lit2Var(pCut->iFunc));           }

#define Sdb_ForEachCut( pList, pCut, i ) for ( i = 0, pCut = pList + 1; i < pList[0]; i++, pCut += pCut[0] + 2 )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Check correctness of cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word Gia_CutGetSign( Gia_Cut_t * pCut )
{
    word Sign = 0; int i; 
    for ( i = 0; i < (int)pCut->nLeaves; i++ )
        Sign |= ((word)1) << (pCut->pLeaves[i] & 0x3F);
    return Sign;
}
static inline int Gia_CutCheck( Gia_Cut_t * pBase, Gia_Cut_t * pCut ) // check if pCut is contained in pBase
{
    int nSizeB = pBase->nLeaves;
    int nSizeC = pCut->nLeaves;
    int i, * pB = pBase->pLeaves;
    int k, * pC = pCut->pLeaves;
    for ( i = 0; i < nSizeC; i++ )
    {
        for ( k = 0; k < nSizeB; k++ )
            if ( pC[i] == pB[k] )
                break;
        if ( k == nSizeB )
            return 0;
    }
    return 1;
}
static inline int Gia_CutSetCheckArray( Gia_Cut_t ** ppCuts, int nCuts )
{
    Gia_Cut_t * pCut0, * pCut1; 
    int i, k, m, n, Value;
    assert( nCuts > 0 );
    for ( i = 0; i < nCuts; i++ )
    {
        pCut0 = ppCuts[i];
        assert( pCut0->nLeaves <= GIA_MAX_CUTSIZE );
        assert( pCut0->Sign == Gia_CutGetSign(pCut0) );
        // check duplicates
        for ( m = 0; m < (int)pCut0->nLeaves; m++ )
        for ( n = m + 1; n < (int)pCut0->nLeaves; n++ )
            assert( pCut0->pLeaves[m] < pCut0->pLeaves[n] );
        // check pairs
        for ( k = 0; k < nCuts; k++ )
        {
            pCut1 = ppCuts[k];
            if ( pCut0 == pCut1 )
                continue;
            // check containments
            Value = Gia_CutCheck( pCut0, pCut1 );
            assert( Value == 0 );
        }
    }
    return 1;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_CutMergeOrder( Gia_Cut_t * pCut0, Gia_Cut_t * pCut1, Gia_Cut_t * pCut, int nCutSize )
{ 
    int nSize0   = pCut0->nLeaves;
    int nSize1   = pCut1->nLeaves;
    int i, * pC0 = pCut0->pLeaves;
    int k, * pC1 = pCut1->pLeaves;
    int c, * pC  = pCut->pLeaves;
    // the case of the largest cut sizes
    if ( nSize0 == nCutSize && nSize1 == nCutSize )
    {
        for ( i = 0; i < nSize0; i++ )
        {
            if ( pC0[i] != pC1[i] )  return 0;
            pC[i] = pC0[i];
        }
        pCut->nLeaves = nCutSize;
        pCut->iFunc = -1;
        pCut->Sign = pCut0->Sign | pCut1->Sign;
        return 1;
    }
    // compare two cuts with different numbers
    i = k = c = 0;
    if ( nSize0 == 0 ) goto FlushCut1;
    if ( nSize1 == 0 ) goto FlushCut0;
    while ( 1 )
    {
        if ( c == nCutSize ) return 0;
        if ( pC0[i] < pC1[k] )
        {
            pC[c++] = pC0[i++];
            if ( i >= nSize0 ) goto FlushCut1;
        }
        else if ( pC0[i] > pC1[k] )
        {
            pC[c++] = pC1[k++];
            if ( k >= nSize1 ) goto FlushCut0;
        }
        else
        {
            pC[c++] = pC0[i++]; k++;
            if ( i >= nSize0 ) goto FlushCut1;
            if ( k >= nSize1 ) goto FlushCut0;
        }
    }

FlushCut0:
    if ( c + nSize0 > nCutSize + i ) return 0;
    while ( i < nSize0 )
        pC[c++] = pC0[i++];
    pCut->nLeaves = c;
    pCut->iFunc = -1;
    pCut->Sign = pCut0->Sign | pCut1->Sign;
    return 1;

FlushCut1:
    if ( c + nSize1 > nCutSize + k ) return 0;
    while ( k < nSize1 )
        pC[c++] = pC1[k++];
    pCut->nLeaves = c;
    pCut->iFunc = -1;
    pCut->Sign = pCut0->Sign | pCut1->Sign;
    return 1;
}
static inline int Gia_CutMergeOrder2( Gia_Cut_t * pCut0, Gia_Cut_t * pCut1, Gia_Cut_t * pCut, int nCutSize )
{ 
    int x0, i0 = 0, nSize0 = pCut0->nLeaves, * pC0 = pCut0->pLeaves;
    int x1, i1 = 0, nSize1 = pCut1->nLeaves, * pC1 = pCut1->pLeaves;
    int xMin, c = 0, * pC  = pCut->pLeaves;
    while ( 1 )
    {
        x0 = (i0 == nSize0) ? ABC_INFINITY : pC0[i0];
        x1 = (i1 == nSize1) ? ABC_INFINITY : pC1[i1];
        xMin = Abc_MinInt(x0, x1);
        if ( xMin == ABC_INFINITY ) break;
        if ( c == nCutSize ) return 0;
        pC[c++] = xMin;
        if (x0 == xMin) i0++;
        if (x1 == xMin) i1++;
    }
    pCut->nLeaves = c;
    pCut->iFunc = -1;
    pCut->Sign = pCut0->Sign | pCut1->Sign;
    return 1;
}
static inline int Gia_CutSetCutIsContainedOrder( Gia_Cut_t * pBase, Gia_Cut_t * pCut ) // check if pCut is contained in pBase
{
    int i, nSizeB = pBase->nLeaves;
    int k, nSizeC = pCut->nLeaves;
    if ( nSizeB == nSizeC )
    {
        for ( i = 0; i < nSizeB; i++ )
            if ( pBase->pLeaves[i] != pCut->pLeaves[i] )
                return 0;
        return 1;
    }
    assert( nSizeB > nSizeC ); 
    if ( nSizeC == 0 )
        return 1;
    for ( i = k = 0; i < nSizeB; i++ )
    {
        if ( pBase->pLeaves[i] > pCut->pLeaves[k] )
            return 0;
        if ( pBase->pLeaves[i] == pCut->pLeaves[k] )
        {
            if ( ++k == nSizeC )
                return 1;
        }
    }
    return 0;
}
static inline int Gia_CutSetLastCutIsContained( Gia_Cut_t ** pCuts, int nCuts )
{
    int i;
    for ( i = 0; i < nCuts; i++ )
        if ( pCuts[i]->nLeaves <= pCuts[nCuts]->nLeaves && (pCuts[i]->Sign & pCuts[nCuts]->Sign) == pCuts[i]->Sign && Gia_CutSetCutIsContainedOrder(pCuts[nCuts], pCuts[i]) )
            return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_CutCompare( Gia_Cut_t * pCut0, Gia_Cut_t * pCut1 )
{
    if ( pCut0->nTreeLeaves < pCut1->nTreeLeaves )  return -1;
    if ( pCut0->nTreeLeaves > pCut1->nTreeLeaves )  return  1;
    if ( pCut0->nLeaves     < pCut1->nLeaves )      return -1;
    if ( pCut0->nLeaves     > pCut1->nLeaves )      return  1;
    return 0;
}
static inline int Gia_CutSetLastCutContains( Gia_Cut_t ** pCuts, int nCuts )
{
    int i, k, fChanges = 0;
    for ( i = 0; i < nCuts; i++ )
        if ( pCuts[nCuts]->nLeaves < pCuts[i]->nLeaves && (pCuts[nCuts]->Sign & pCuts[i]->Sign) == pCuts[nCuts]->Sign && Gia_CutSetCutIsContainedOrder(pCuts[i], pCuts[nCuts]) )
            pCuts[i]->nLeaves = GIA_CUT_NO_LEAF, fChanges = 1;
    if ( !fChanges )
        return nCuts;
    for ( i = k = 0; i <= nCuts; i++ )
    {
        if ( pCuts[i]->nLeaves == GIA_CUT_NO_LEAF )
            continue;
        if ( k < i )
            ABC_SWAP( Gia_Cut_t *, pCuts[k], pCuts[i] );
        k++;
    }
    return k - 1;
}
static inline void Gia_CutSetSortByCost( Gia_Cut_t ** pCuts, int nCuts )
{
    int i;
    for ( i = nCuts; i > 0; i-- )
    {
        if ( Gia_CutCompare(pCuts[i - 1], pCuts[i]) < 0 )//!= 1 )
            return;
        ABC_SWAP( Gia_Cut_t *, pCuts[i - 1], pCuts[i] );
    }
}
static inline int Gia_CutSetAddCut( Gia_Cut_t ** pCuts, int nCuts, int nCutNum )
{
    if ( nCuts == 0 )
        return 1;
    nCuts = Gia_CutSetLastCutContains(pCuts, nCuts);
    assert( nCuts >= 0 );
    Gia_CutSetSortByCost( pCuts, nCuts );
    // add new cut if there is room
    return Abc_MinInt( nCuts + 1, nCutNum - 1 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_CutComputeTruth6( Gia_Sto_t * p, Gia_Cut_t * pCut0, Gia_Cut_t * pCut1, int fCompl0, int fCompl1, Gia_Cut_t * pCutR, int fIsXor )
{
    int nOldSupp = pCutR->nLeaves, truthId, fCompl; word t;
    word t0 = *Gia_CutTruth(p, pCut0);
    word t1 = *Gia_CutTruth(p, pCut1);
    if ( Abc_LitIsCompl(pCut0->iFunc) ^ fCompl0 ) t0 = ~t0;
    if ( Abc_LitIsCompl(pCut1->iFunc) ^ fCompl1 ) t1 = ~t1;
    t0 = Abc_Tt6Expand( t0, pCut0->pLeaves, pCut0->nLeaves, pCutR->pLeaves, pCutR->nLeaves );
    t1 = Abc_Tt6Expand( t1, pCut1->pLeaves, pCut1->nLeaves, pCutR->pLeaves, pCutR->nLeaves );
    t =  fIsXor ? t0 ^ t1 : t0 & t1;
    if ( (fCompl = (int)(t & 1)) ) t = ~t;
    if ( p->fTruthMin )
        pCutR->nLeaves = Abc_Tt6MinBase( &t, pCutR->pLeaves, pCutR->nLeaves );
    assert( (int)(t & 1) == 0 );
    truthId        = Vec_MemHashInsert(p->vTtMem, &t);
    pCutR->iFunc   = Abc_Var2Lit( truthId, fCompl );
    assert( (int)pCutR->nLeaves <= nOldSupp );
    return (int)pCutR->nLeaves < nOldSupp;
}
static inline int Gia_CutComputeTruth( Gia_Sto_t * p, Gia_Cut_t * pCut0, Gia_Cut_t * pCut1, int fCompl0, int fCompl1, Gia_Cut_t * pCutR, int fIsXor )
{
    if ( p->nCutSize <= 6 )
        return Gia_CutComputeTruth6( p, pCut0, pCut1, fCompl0, fCompl1, pCutR, fIsXor );
    {
    word uTruth[GIA_MAX_TT_WORDS], uTruth0[GIA_MAX_TT_WORDS], uTruth1[GIA_MAX_TT_WORDS];
    int nOldSupp   = pCutR->nLeaves, truthId;
    int nCutSize   = p->nCutSize, fCompl;
    int nWords     = Abc_Truth6WordNum(nCutSize);
    word * pTruth0 = Gia_CutTruth(p, pCut0);
    word * pTruth1 = Gia_CutTruth(p, pCut1);
    Abc_TtCopy( uTruth0, pTruth0, nWords, Abc_LitIsCompl(pCut0->iFunc) ^ fCompl0 );
    Abc_TtCopy( uTruth1, pTruth1, nWords, Abc_LitIsCompl(pCut1->iFunc) ^ fCompl1 );
    Abc_TtExpand( uTruth0, nCutSize, pCut0->pLeaves, pCut0->nLeaves, pCutR->pLeaves, pCutR->nLeaves );
    Abc_TtExpand( uTruth1, nCutSize, pCut1->pLeaves, pCut1->nLeaves, pCutR->pLeaves, pCutR->nLeaves );
    if ( fIsXor )
        Abc_TtXor( uTruth, uTruth0, uTruth1, nWords, (fCompl = (int)((uTruth0[0] ^ uTruth1[0]) & 1)) );
    else
        Abc_TtAnd( uTruth, uTruth0, uTruth1, nWords, (fCompl = (int)((uTruth0[0] & uTruth1[0]) & 1)) );
    if ( p->fTruthMin )
        pCutR->nLeaves = Abc_TtMinBase( uTruth, pCutR->pLeaves, pCutR->nLeaves, nCutSize );
    assert( (uTruth[0] & 1) == 0 );
//Kit_DsdPrintFromTruth( uTruth, pCutR->nLeaves ), printf("\n" ), printf("\n" );
    truthId        = Vec_MemHashInsert(p->vTtMem, uTruth);
    pCutR->iFunc   = Abc_Var2Lit( truthId, fCompl );
    assert( (int)pCutR->nLeaves <= nOldSupp );
    return (int)pCutR->nLeaves < nOldSupp;
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_CutCountBits( word i )
{
    i = i - ((i >> 1) & 0x5555555555555555);
    i = (i & 0x3333333333333333) + ((i >> 2) & 0x3333333333333333);
    i = ((i + (i >> 4)) & 0x0F0F0F0F0F0F0F0F);
    return (i*(0x0101010101010101))>>56;
}
static inline void Gia_CutAddUnit( Gia_Sto_t * p, int iObj )
{
    Vec_Int_t * vThis = Vec_WecEntry( p->vCuts, iObj );
    if ( Vec_IntSize(vThis) == 0 )
        Vec_IntPush( vThis, 1 );
    else 
        Vec_IntAddToEntry( vThis, 0, 1 );
    Vec_IntPush( vThis, 1 );
    Vec_IntPush( vThis, iObj );
    Vec_IntPush( vThis, 2 );
}
static inline void Gia_CutAddZero( Gia_Sto_t * p, int iObj )
{
    Vec_Int_t * vThis = Vec_WecEntry( p->vCuts, iObj );
    assert( Vec_IntSize(vThis) == 0 );
    Vec_IntPush( vThis, 1 );
    Vec_IntPush( vThis, 0 );
    Vec_IntPush( vThis, 0 );
}
static inline int Gia_CutTreeLeaves( Gia_Sto_t * p, Gia_Cut_t * pCut )
{
    int i, Cost = 0;
    for ( i = 0; i < (int)pCut->nLeaves; i++ )
        Cost += Vec_IntEntry( p->vRefs, pCut->pLeaves[i] ) == 1;
    return Cost;
}
static inline int Gia_StoPrepareSet( Gia_Sto_t * p, int iObj, int Index )
{
    Vec_Int_t * vThis = Vec_WecEntry( p->vCuts, iObj );
    int i, v, * pCut, * pList = Vec_IntArray( vThis );
    Sdb_ForEachCut( pList, pCut, i )
    {
        Gia_Cut_t * pCutTemp = &p->pCuts[Index][i];
        pCutTemp->nLeaves = pCut[0];
        for ( v = 1; v <= pCut[0]; v++ )
            pCutTemp->pLeaves[v-1] = pCut[v];
        pCutTemp->iFunc = pCut[pCut[0]+1];
        pCutTemp->Sign = Gia_CutGetSign( pCutTemp );
        pCutTemp->nTreeLeaves = Gia_CutTreeLeaves( p, pCutTemp );
    }
    return pList[0];
}
static inline void Gia_StoInitResult( Gia_Sto_t * p )
{
    int i; 
    for ( i = 0; i < GIA_MAX_CUTNUM; i++ )
        p->ppCuts[i] = &p->pCuts[2][i];
}
static inline void Gia_StoStoreResult( Gia_Sto_t * p, int iObj, Gia_Cut_t ** pCuts, int nCuts )
{
    int i, v;
    Vec_Int_t * vList = Vec_WecEntry( p->vCuts, iObj );
    Vec_IntPush( vList, nCuts );
    for ( i = 0; i < nCuts; i++ )
    {
        Vec_IntPush( vList, pCuts[i]->nLeaves );
        for ( v = 0; v < (int)pCuts[i]->nLeaves; v++ )
            Vec_IntPush( vList, pCuts[i]->pLeaves[v] );
        Vec_IntPush( vList, pCuts[i]->iFunc );
    }
}
static inline void Gia_CutPrint( Gia_Sto_t * p, int iObj, Gia_Cut_t * pCut )
{
    int i, nDigits = Abc_Base10Log(Gia_ManObjNum(p->pGia)); 
    if ( pCut == NULL ) { printf( "No cut.\n" ); return; }
    printf( "%d  {", pCut->nLeaves );
    for ( i = 0; i < (int)pCut->nLeaves; i++ )
        printf( " %*d", nDigits, pCut->pLeaves[i] );
    for ( ; i < (int)p->nCutSize; i++ )
        printf( " %*s", nDigits, " " );
    printf( "  }  Cost = %3d  CostL = %3d  Tree = %d  ", 
        pCut->Cost, pCut->CostLev, pCut->nTreeLeaves );
    printf( "\n" );
}
void Gia_StoMergeCuts( Gia_Sto_t * p, int iObj )
{
    Gia_Obj_t * pObj = Gia_ManObj(p->pGia, iObj);
    int fIsXor       = Gia_ObjIsXor(pObj);
    int nCutSize     = p->nCutSize;
    int nCutNum      = p->nCutNum;
    int fComp0       = Gia_ObjFaninC0(pObj);
    int fComp1       = Gia_ObjFaninC1(pObj);
    int Fan0         = Gia_ObjFaninId0(pObj, iObj);
    int Fan1         = Gia_ObjFaninId1(pObj, iObj);
    int nCuts0       = Gia_StoPrepareSet( p, Fan0, 0 );
    int nCuts1       = Gia_StoPrepareSet( p, Fan1, 1 );
    int i, k, nCutsR = 0;
    Gia_Cut_t * pCut0, * pCut1, ** pCutsR = p->ppCuts;
    assert( !Gia_ObjIsBuf(pObj) );
    assert( !Gia_ObjIsMux(p->pGia, pObj) );
    Gia_StoInitResult( p );
    p->CutCount[0] += nCuts0 * nCuts1;
    for ( i = 0, pCut0 = p->pCuts[0]; i < nCuts0; i++, pCut0++ )
    for ( k = 0, pCut1 = p->pCuts[1]; k < nCuts1; k++, pCut1++ )
    {
        if ( (int)(pCut0->nLeaves + pCut1->nLeaves) > nCutSize && Gia_CutCountBits(pCut0->Sign | pCut1->Sign) > nCutSize )
            continue;
        p->CutCount[1]++; 
        if ( !Gia_CutMergeOrder(pCut0, pCut1, pCutsR[nCutsR], nCutSize) )
            continue;
        if ( Gia_CutSetLastCutIsContained(pCutsR, nCutsR) )
            continue;
        p->CutCount[2]++;
        if ( p->fCutMin && Gia_CutComputeTruth(p, pCut0, pCut1, fComp0, fComp1, pCutsR[nCutsR], fIsXor) )
            pCutsR[nCutsR]->Sign = Gia_CutGetSign(pCutsR[nCutsR]);
        pCutsR[nCutsR]->nTreeLeaves = Gia_CutTreeLeaves( p, pCutsR[nCutsR] );
        nCutsR = Gia_CutSetAddCut( pCutsR, nCutsR, nCutNum );
    }
    p->CutCount[3] += nCutsR;
    p->nCutsOver += nCutsR == nCutNum-1;
    p->nCutsR = nCutsR;
    p->Pivot = iObj;
    // debug printout
    if ( 0 )
    {
        printf( "*** Obj = %4d  NumCuts = %4d\n", iObj, nCutsR );
        for ( i = 0; i < nCutsR; i++ )
            Gia_CutPrint( p, iObj, pCutsR[i] );
        printf( "\n" );
    }
    // verify
    assert( nCutsR > 0 && nCutsR < nCutNum );
    assert( Gia_CutSetCheckArray(pCutsR, nCutsR) );
    // store the cutset
    Gia_StoStoreResult( p, iObj, pCutsR, nCutsR );
    if ( nCutsR > 1 || pCutsR[0]->nLeaves > 1 )
        Gia_CutAddUnit( p, iObj );
}


/**Function*************************************************************

  Synopsis    [Incremental cut computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Sto_t * Gia_StoAlloc( Gia_Man_t * pGia, int nCutSize, int nCutNum, int fCutMin, int fTruthMin, int fVerbose )
{
    Gia_Sto_t * p;
    assert( nCutSize < GIA_CUT_NO_LEAF );
    assert( nCutSize > 1 && nCutSize <= GIA_MAX_CUTSIZE );
    assert( nCutNum > 1 && nCutNum < GIA_MAX_CUTNUM );
    p = ABC_CALLOC( Gia_Sto_t, 1 );
    p->clkStart  = Abc_Clock();
    p->nCutSize  = nCutSize;
    p->nCutNum   = nCutNum;
    p->fCutMin   = fCutMin;
    p->fTruthMin = fTruthMin;
    p->fVerbose  = fVerbose;
    p->pGia      = pGia;
    p->vRefs     = Vec_IntAlloc( Gia_ManObjNum(pGia) );
    p->vCuts     = Vec_WecStart( Gia_ManObjNum(pGia) );
    p->vTtMem    = fCutMin ? Vec_MemAllocForTT( nCutSize, 0 ) : NULL;
    return p;
}
void Gia_StoFree( Gia_Sto_t * p )
{
    Vec_IntFree( p->vRefs );
    Vec_WecFree( p->vCuts );
    if ( p->fCutMin )
        Vec_MemHashFree( p->vTtMem );
    if ( p->fCutMin )
        Vec_MemFree( p->vTtMem );
    ABC_FREE( p );
}
void Gia_StoComputeCutsConst0( Gia_Sto_t * p, int iObj )
{
    Gia_CutAddZero( p, iObj );
}
void Gia_StoComputeCutsCi( Gia_Sto_t * p, int iObj )
{
    Gia_CutAddUnit( p, iObj );
}
void Gia_StoComputeCutsNode( Gia_Sto_t * p, int iObj )
{
    Gia_StoMergeCuts( p, iObj );
}
void Gia_StoRefObj( Gia_Sto_t * p, int iObj )
{
    Gia_Obj_t * pObj = Gia_ManObj(p->pGia, iObj);
    assert( iObj == Vec_IntSize(p->vRefs) );
    Vec_IntPush( p->vRefs, 0 );
    if ( Gia_ObjIsAnd(pObj) )
    {
        Vec_IntAddToEntry( p->vRefs, Gia_ObjFaninId0(pObj, iObj), 1 );
        Vec_IntAddToEntry( p->vRefs, Gia_ObjFaninId1(pObj, iObj), 1 );
    }
    else if ( Gia_ObjIsCo(pObj) )
        Vec_IntAddToEntry( p->vRefs, Gia_ObjFaninId0(pObj, iObj), 1 );
}
void Gia_StoComputeCuts( Gia_Man_t * pGia )
{
    int nCutSize  =  6;
    int nCutNum   = 25;
    int fCutMin   =  1;
    int fTruthMin =  1;
    int fVerbose  =  1;
    Gia_Sto_t * p = Gia_StoAlloc( pGia, nCutSize, nCutNum, fCutMin, fTruthMin, fVerbose );
    Gia_Obj_t * pObj;  int i, iObj;
    assert( nCutSize <= GIA_MAX_CUTSIZE );
    assert( nCutNum  <  GIA_MAX_CUTNUM  );
    // prepare references
    Gia_ManForEachObj( p->pGia, pObj, iObj )
        Gia_StoRefObj( p, iObj );
    // compute cuts
    Gia_StoComputeCutsConst0( p, 0 );
    Gia_ManForEachCiId( p->pGia, iObj, i )
        Gia_StoComputeCutsCi( p, iObj );
    Gia_ManForEachAnd( p->pGia, pObj, iObj )
        Gia_StoComputeCutsNode( p, iObj );
    if ( p->fVerbose )
    {
        printf( "Running cut computation with CutSize = %d  CutNum = %d  CutMin = %s  TruthMin = %s\n", 
            p->nCutSize, p->nCutNum, p->fCutMin ? "yes":"no", p->fTruthMin ? "yes":"no" );
        printf( "CutPair = %.0f  ",         p->CutCount[0] );
        printf( "Merge = %.0f (%.2f %%)  ", p->CutCount[1], 100.0*p->CutCount[1]/p->CutCount[0] );
        printf( "Eval = %.0f (%.2f %%)  ",  p->CutCount[2], 100.0*p->CutCount[2]/p->CutCount[0] );
        printf( "Cut = %.0f (%.2f %%)  ",   p->CutCount[3], 100.0*p->CutCount[3]/p->CutCount[0] );
        printf( "Cut/Node = %.2f  ",        p->CutCount[3] / Gia_ManAndNum(p->pGia) );
        printf( "\n" );
        printf( "The number of nodes with cut count over the limit (%d cuts) = %d nodes (out of %d).  ", 
            p->nCutNum, p->nCutsOver, Gia_ManAndNum(pGia) );
        Abc_PrintTime( 0, "Time", Abc_Clock() - p->clkStart );
    }
    Gia_StoFree( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

