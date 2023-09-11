/**CFile****************************************************************

  FileName    [giaBalance.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [AIG balancing.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaBalance.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "misc/vec/vecHash.h"
#include "misc/vec/vecQue.h"
#include "opt/dau/dau.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define BAL_LEAF_MAX    6
#define BAL_CUT_MAX     8
#define BAL_SUPER      50
#define BAL_NO_LEAF    31
#define BAL_NO_FUNC    134217727     // (1<<27)-1

typedef struct Bal_Cut_t_ Bal_Cut_t; 
struct Bal_Cut_t_
{
    word             Sign;           // signature
    int              Delay;          // delay
    unsigned         iFunc   : 27;   // function (BAL_NO_FUNC)
    unsigned         nLeaves :  5;   // leaf number (Bal_NO_LEAF)
    int              pLeaves[BAL_LEAF_MAX]; // leaves
};

// operation manager
typedef struct Bal_Man_t_ Bal_Man_t;
struct Bal_Man_t_
{
    Gia_Man_t *      pGia;           // user AIG
    int              nLutSize;       // LUT size
    int              nCutNum;        // cut number
    int              fCutMin;        // cut minimization
    int              fVerbose;       // verbose
    Gia_Man_t *      pNew;           // derived AIG
    Vec_Int_t *      vCosts;         // cost of supergate nodes
    Vec_Ptr_t *      vCutSets;       // object cutsets
    abctime          clkStart;       // starting the clock
};

static inline Bal_Man_t * Bal_GiaMan( Gia_Man_t * p )                   { return (Bal_Man_t *)p->pData;           }

static inline int         Bal_ObjCost( Bal_Man_t * p, int i )           { return Vec_IntEntry(p->vCosts, i);      }
static inline int         Bal_LitCost( Bal_Man_t * p, int i )           { return Bal_ObjCost(p, Abc_Lit2Var(i));  }
static inline int         Bal_ObjDelay( Bal_Man_t * p, int i )          { return Bal_ObjCost(p, i) >> 4;          }
static inline int         Bal_LitDelay( Bal_Man_t * p, int i )          { return Bal_ObjDelay(p, Abc_Lit2Var(i)); }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Bal_Man_t * Bal_ManAlloc( Gia_Man_t * pGia, Gia_Man_t * pNew, int nLutSize, int nCutNum, int fVerbose )
{
    Bal_Man_t * p;
    p = ABC_CALLOC( Bal_Man_t, 1 );
    p->clkStart = Abc_Clock();
    p->pGia     = pGia;
    p->pNew     = pNew;
    p->nLutSize = nLutSize;
    p->nCutNum  = nCutNum;
    p->fVerbose = fVerbose;
    p->vCosts   = Vec_IntAlloc( 3 * Gia_ManObjNum(pGia) / 2 );
    p->vCutSets = Vec_PtrAlloc( 3 * Gia_ManObjNum(pGia) / 2 );
    Vec_IntFill( p->vCosts, Gia_ManObjNum(pNew), 0 );
    Vec_PtrFill( p->vCutSets, Gia_ManObjNum(pNew), NULL );
    pNew->pData = p;
    return p;
}
void Bal_ManFree( Bal_Man_t * p )
{
    Vec_PtrFreeFree( p->vCutSets );
    Vec_IntFree( p->vCosts );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Bal_CutCountBits( word i )
{
    i = i - ((i >> 1) & 0x5555555555555555);
    i = (i & 0x3333333333333333) + ((i >> 2) & 0x3333333333333333);
    i = ((i + (i >> 4)) & 0x0F0F0F0F0F0F0F0F);
    return (i*(0x0101010101010101))>>56;
}
static inline word Bal_CutGetSign( int * pLeaves, int nLeaves )
{
    word Sign = 0; int i; 
    for ( i = 0; i < nLeaves; i++ )
        Sign |= ((word)1) << (pLeaves[i] & 0x3F);
    return Sign;
}
static inline int Bal_CutCreateUnit( Bal_Cut_t * p, int i, int Delay )
{
    p->iFunc = 2;
    p->Delay = Delay;
    p->nLeaves = 1;
    p->pLeaves[0] = i;
    p->Sign = ((word)1) << (i & 0x3F);
    return 1;
}
static inline int Bal_ManPrepareSet( Bal_Man_t * p, int iObj, int Index, int fUnit, Bal_Cut_t ** ppCutSet )
{
    static Bal_Cut_t CutTemp[3]; int i;
    if ( Vec_PtrEntry(p->vCutSets, iObj) == NULL || fUnit )
        return Bal_CutCreateUnit( (*ppCutSet = CutTemp + Index), iObj, Bal_ObjDelay(p, iObj)+1 );
    *ppCutSet = (Bal_Cut_t *)Vec_PtrEntry(p->vCutSets, iObj);
    for ( i = 0; i < p->nCutNum; i++ ) 
        if ( (*ppCutSet)[i].nLeaves == BAL_NO_LEAF )
            return i;
    return i;
}


/**Function*************************************************************

  Synopsis    [Check correctness of cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Bal_CutCheck( Bal_Cut_t * pBase, Bal_Cut_t * pCut ) // check if pCut is contained in pBase
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
static inline int Bal_SetCheckArray( Bal_Cut_t ** ppCuts, int nCuts )
{
    Bal_Cut_t * pCut0, * pCut1; 
    int i, k, m, n, Value;
    assert( nCuts > 0 );
    for ( i = 0; i < nCuts; i++ )
    {
        pCut0 = ppCuts[i];
        assert( pCut0->nLeaves <= BAL_LEAF_MAX );
        assert( pCut0->Sign == Bal_CutGetSign(pCut0->pLeaves, pCut0->nLeaves) );
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
            Value = Bal_CutCheck( pCut0, pCut1 );
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
static inline int Bal_CutMergeOrder( Bal_Cut_t * pCut0, Bal_Cut_t * pCut1, Bal_Cut_t * pCut, int nLutSize )
{ 
    int nSize0   = pCut0->nLeaves;
    int nSize1   = pCut1->nLeaves;
    int i, * pC0 = pCut0->pLeaves;
    int k, * pC1 = pCut1->pLeaves;
    int c, * pC  = pCut->pLeaves;
    // the case of the largest cut sizes
    if ( nSize0 == nLutSize && nSize1 == nLutSize )
    {
        for ( i = 0; i < nSize0; i++ )
        {
            if ( pC0[i] != pC1[i] )  return 0;
            pC[i] = pC0[i];
        }
        pCut->nLeaves = nLutSize;
        pCut->iFunc = BAL_NO_FUNC;
        pCut->Sign = pCut0->Sign | pCut1->Sign;
        pCut->Delay = Abc_MaxInt( pCut0->Delay, pCut1->Delay );
        return 1;
    }
    // compare two cuts with different numbers
    i = k = c = 0;
    while ( 1 )
    {
        if ( c == nLutSize ) return 0;
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
    if ( c + nSize0 > nLutSize + i ) return 0;
    while ( i < nSize0 )
        pC[c++] = pC0[i++];
    pCut->nLeaves = c;
    pCut->iFunc = BAL_NO_FUNC;
    pCut->Sign = pCut0->Sign | pCut1->Sign;
    pCut->Delay = Abc_MaxInt( pCut0->Delay, pCut1->Delay );
    return 1;

FlushCut1:
    if ( c + nSize1 > nLutSize + k ) return 0;
    while ( k < nSize1 )
        pC[c++] = pC1[k++];
    pCut->nLeaves = c;
    pCut->iFunc = BAL_NO_FUNC;
    pCut->Sign = pCut0->Sign | pCut1->Sign;
    pCut->Delay = Abc_MaxInt( pCut0->Delay, pCut1->Delay );
    return 1;
}
static inline int Bal_CutMergeOrderMux( Bal_Cut_t * pCut0, Bal_Cut_t * pCut1, Bal_Cut_t * pCut2, Bal_Cut_t * pCut, int nLutSize )
{ 
    int x0, i0 = 0, nSize0 = pCut0->nLeaves, * pC0 = pCut0->pLeaves;
    int x1, i1 = 0, nSize1 = pCut1->nLeaves, * pC1 = pCut1->pLeaves;
    int x2, i2 = 0, nSize2 = pCut2->nLeaves, * pC2 = pCut2->pLeaves;
    int xMin, c = 0, * pC  = pCut->pLeaves;
    while ( 1 )
    {
        x0 = (i0 == nSize0) ? ABC_INFINITY : pC0[i0];
        x1 = (i1 == nSize1) ? ABC_INFINITY : pC1[i1];
        x2 = (i2 == nSize2) ? ABC_INFINITY : pC2[i2];
        xMin = Abc_MinInt( Abc_MinInt(x0, x1), x2 );
        if ( xMin == ABC_INFINITY ) break;
        if ( c == nLutSize ) return 0;
        pC[c++] = xMin;
        if (x0 == xMin) i0++;
        if (x1 == xMin) i1++;
        if (x2 == xMin) i2++;
    }
    pCut->nLeaves = c;
    pCut->iFunc = BAL_NO_FUNC;
    pCut->Sign = pCut0->Sign | pCut1->Sign | pCut2->Sign;
    pCut->Delay = Abc_MaxInt( pCut0->Delay, Abc_MaxInt(pCut1->Delay, pCut2->Delay) );
    return 1;
}
static inline int Bal_SetCutIsContainedOrder( Bal_Cut_t * pBase, Bal_Cut_t * pCut ) // check if pCut is contained in pBase
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
static inline int Bal_SetLastCutIsContained( Bal_Cut_t ** pCuts, int nCuts )
{
    int i;
    for ( i = 0; i < nCuts; i++ )
        if ( pCuts[i]->nLeaves <= pCuts[nCuts]->nLeaves && (pCuts[i]->Sign & pCuts[nCuts]->Sign) == pCuts[i]->Sign && Bal_SetCutIsContainedOrder(pCuts[nCuts], pCuts[i]) )
            return 1;
    return 0;
}
static inline int Bal_SetLastCutContains( Bal_Cut_t ** pCuts, int nCuts )
{
    int i, k, fChanges = 0;
    for ( i = 0; i < nCuts; i++ )
        if ( pCuts[nCuts]->nLeaves < pCuts[i]->nLeaves && (pCuts[nCuts]->Sign & pCuts[i]->Sign) == pCuts[nCuts]->Sign && Bal_SetCutIsContainedOrder(pCuts[i], pCuts[nCuts]) )
            pCuts[i]->nLeaves = BAL_NO_LEAF, fChanges = 1;
    if ( !fChanges )
        return nCuts;
    for ( i = k = 0; i <= nCuts; i++ )
    {
        if ( pCuts[i]->nLeaves == BAL_NO_LEAF )
            continue;
        if ( k < i )
            ABC_SWAP( Bal_Cut_t *, pCuts[k], pCuts[i] );
        k++;
    }
    return k - 1;
}
static inline int Bal_CutCompareArea( Bal_Cut_t * pCut0, Bal_Cut_t * pCut1 )
{
    if ( pCut0->Delay   < pCut1->Delay   )  return -1;
    if ( pCut0->Delay   > pCut1->Delay   )  return  1;
    if ( pCut0->nLeaves < pCut1->nLeaves )  return -1;
    if ( pCut0->nLeaves > pCut1->nLeaves )  return  1;
    return 0;
}
static inline void Bal_SetSortByDelay( Bal_Cut_t ** pCuts, int nCuts )
{
    int i;
    for ( i = nCuts; i > 0; i-- )
    {
        if ( Bal_CutCompareArea(pCuts[i - 1], pCuts[i]) < 0 )//!= 1 )
            return;
        ABC_SWAP( Bal_Cut_t *, pCuts[i - 1], pCuts[i] );
    }
}
static inline int Bal_SetAddCut( Bal_Cut_t ** pCuts, int nCuts, int nCutNum )
{
    if ( nCuts == 0 )
        return 1;
    nCuts = Bal_SetLastCutContains(pCuts, nCuts);
    Bal_SetSortByDelay( pCuts, nCuts );
    return Abc_MinInt( nCuts + 1, nCutNum - 1 );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bal_ManDeriveCuts( Bal_Man_t * p, int iFan0, int iFan1, int iFan2, int fCompl0, int fCompl1, int fCompl2, int fUnit0, int fUnit1, int fUnit2, int fIsXor, int Target, int fSave )
{
    Bal_Cut_t pCutSet[BAL_CUT_MAX], * pCutsR[BAL_CUT_MAX];
    Bal_Cut_t * pCutSet0, * pCutSet1, * pCutSet2;
    int nCuts0 = Bal_ManPrepareSet( p, iFan0, 0, fUnit0, &pCutSet0 );
    int nCuts1 = Bal_ManPrepareSet( p, iFan1, 1, fUnit1, &pCutSet1 );
    Bal_Cut_t * pCut0, * pCut0Lim = pCutSet0 + nCuts0;
    Bal_Cut_t * pCut1, * pCut1Lim = pCutSet1 + nCuts1;
    int i, Cost, nCutsR = 0;
    memset( pCutSet, 0, sizeof(Bal_Cut_t) * p->nCutNum );
    for ( i = 0; i < p->nCutNum; i++ )
        pCutsR[i] = pCutSet + i;
    // enumerate cuts
    if ( iFan2 > 0 )
    {
        int nCuts2 = Bal_ManPrepareSet( p, iFan2, 2, fUnit2, &pCutSet2 );
        Bal_Cut_t * pCut2, * pCut2Lim = pCutSet2 + nCuts2;
        for ( pCut0 = pCutSet0; pCut0 < pCut0Lim; pCut0++ )
        for ( pCut1 = pCutSet1; pCut1 < pCut1Lim; pCut1++ )
        for ( pCut2 = pCutSet2; pCut2 < pCut2Lim; pCut2++ )
        {
            if ( Bal_CutCountBits(pCut0->Sign | pCut1->Sign | pCut2->Sign) > p->nLutSize )
                continue;
            if ( !Bal_CutMergeOrderMux(pCut0, pCut1, pCut2, pCutsR[nCutsR], p->nLutSize) )
                continue;
            assert( pCutsR[nCutsR]->Delay == Target );
            if ( Bal_SetLastCutIsContained(pCutsR, nCutsR) )
                continue;
//            if ( p->fCutMin && Bal_CutComputeTruthMux(p, pCut0, pCut1, pCut2, fCompl0, fCompl1, fCompl2, pCutsR[nCutsR]) )
//                pCutsR[nCutsR]->Sign = Bal_CutGetSign(pCutsR[nCutsR]->pLeaves, pCutsR[nCutsR]->nLeaves);
            nCutsR = Bal_SetAddCut( pCutsR, nCutsR, p->nCutNum );
        }
    }
    else
    {
        for ( pCut0 = pCutSet0; pCut0 < pCut0Lim; pCut0++ )
        for ( pCut1 = pCutSet1; pCut1 < pCut1Lim; pCut1++ )
        {
            if ( Bal_CutCountBits(pCut0->Sign | pCut1->Sign) > p->nLutSize )
                continue;
            if ( !Bal_CutMergeOrder(pCut0, pCut1, pCutsR[nCutsR], p->nLutSize) )
                continue;
            assert( pCutsR[nCutsR]->Delay == Target );
            if ( Bal_SetLastCutIsContained(pCutsR, nCutsR) )
                continue;
//            if ( p->fCutMin && Bal_CutComputeTruth(p, pCut0, pCut1, fCompl0, fCompl1, pCutsR[nCutsR], fIsXor) )
//                pCutsR[nCutsR]->Sign = Bal_CutGetSign(pCutsR[nCutsR]->pLeaves, pCutsR[nCutsR]->nLeaves);
            nCutsR = Bal_SetAddCut( pCutsR, nCutsR, p->nCutNum );
        }
    }
    if ( nCutsR == 0 )
        return -1;
//printf( "%d ", nCutsR );
    Cost = ((pCutsR[0]->Delay << 4) | pCutsR[0]->nLeaves);
    // verify
    assert( nCutsR > 0 && nCutsR < p->nCutNum );
    assert( Bal_SetCheckArray(pCutsR, nCutsR) );
    // save cuts
    if ( fSave && Cost >= 0 )
    {
        pCutSet0 = ABC_CALLOC( Bal_Cut_t, p->nCutNum );
        Vec_PtrPush( p->vCutSets, pCutSet0 );
        assert( Vec_PtrSize(p->vCutSets) == Gia_ManObjNum(p->pNew) );
        for ( i = 0; i < nCutsR; i++ )
            pCutSet0[i] = *pCutsR[i];        
        for ( ; i < p->nCutNum; i++ )
            pCutSet0[i].nLeaves = BAL_NO_LEAF;
        Vec_IntPush( p->vCosts, Cost );
        assert( Vec_IntSize(p->vCosts) == Gia_ManObjNum(p->pNew) );
    }
    return Cost;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bal_ManSetGateLevel( Bal_Man_t * p, Gia_Obj_t * pObjOld, int iLitNew )
{
    int iFan0, iFan1, iFan2, Cost;
    int fCompl0, fCompl1, fCompl2;
    int fUnit0, fUnit1, fUnit2;
    int Delay0, Delay1, Delay2, DelayMax;
    int iObjNew = Abc_Lit2Var(iLitNew);
    Gia_Obj_t * pObjNew = Gia_ManObj( p->pNew, iObjNew );
    int fMux = Gia_ObjIsMux(p->pNew, pObjNew);
    if ( iObjNew < Vec_PtrSize(p->vCutSets) )
        return -1;
    iFan0    = Gia_ObjFaninId0( pObjNew, iObjNew );
    iFan1    = Gia_ObjFaninId1( pObjNew, iObjNew );
    iFan2    = fMux ? Gia_ObjFaninId2(p->pNew, iObjNew) : 0;
    fCompl0  = Gia_ObjFaninC0( pObjNew );
    fCompl1  = Gia_ObjFaninC1( pObjNew );
    fCompl2  = fMux ? Gia_ObjFaninC2(p->pNew, pObjNew) : 0;
    Delay0   = Bal_ObjDelay( p, iFan0 );
    Delay1   = Bal_ObjDelay( p, iFan1 );
    Delay2   = Bal_ObjDelay( p, iFan2 );
    DelayMax = Abc_MaxInt( Delay0, Abc_MaxInt(Delay1, Delay2) );
    fUnit0   = (int)(Delay0 != DelayMax); 
    fUnit1   = (int)(Delay1 != DelayMax); 
    fUnit2   = (int)(Delay2 != DelayMax); 
    if ( DelayMax > 0 )
    {
//printf( "A" );
        Cost = Bal_ManDeriveCuts(p, iFan0, iFan1, iFan2, fCompl0, fCompl1, fCompl2, fUnit0, fUnit1, fUnit2, Gia_ObjIsXor(pObjNew), DelayMax, 1 );
//printf( "B" );
        if ( Cost >= 0 )
            return Cost;
    }
    DelayMax++;
    fUnit0 = fUnit1 = fUnit2 = 1;
//printf( "A" );
    Cost = Bal_ManDeriveCuts(p, iFan0, iFan1, iFan2, fCompl0, fCompl1, fCompl2, fUnit0, fUnit1, fUnit2, Gia_ObjIsXor(pObjNew), DelayMax, 1 );
//printf( "B" );
    assert( Cost >= 0 );
    return Cost;
}
int Bal_ManEvalTwo( Bal_Man_t * p, int iLitNew0, int iLitNew1, int iLitNew2, int fIsXor )
{
    int iFan0    = Abc_Lit2Var( iLitNew0 );
    int iFan1    = Abc_Lit2Var( iLitNew1 );
    int iFan2    = Abc_Lit2Var( iLitNew2 );
    int fCompl0  = Abc_LitIsCompl( iLitNew0 );
    int fCompl1  = Abc_LitIsCompl( iLitNew1 );
    int fCompl2  = Abc_LitIsCompl( iLitNew2 );
    int Delay0   = Bal_ObjDelay( p, iFan0 );
    int Delay1   = Bal_ObjDelay( p, iFan1 );
    int Delay2   = Bal_ObjDelay( p, iFan2 );
    int DelayMax = Abc_MaxInt( Delay0, Abc_MaxInt(Delay1, Delay2) );
    int fUnit0   = (int)(Delay0 != DelayMax); 
    int fUnit1   = (int)(Delay1 != DelayMax); 
    int fUnit2   = (int)(Delay2 != DelayMax); 
    if ( DelayMax == 0 )
        return -1;
    return Bal_ManDeriveCuts(p, iFan0, iFan1, iFan2, fCompl0, fCompl1, fCompl2, fUnit0, fUnit1, fUnit2, fIsXor, DelayMax, 0 );
}

/**Function*************************************************************

  Synopsis    [Sort literals by their cost.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntSelectSortCostLit( Vec_Int_t * vSuper, Vec_Int_t * vCosts )
{
    int * pArray = Vec_IntArray(vSuper);
    int nSize = Vec_IntSize(vSuper);
    int i, j, best_i;
    for ( i = 0; i < nSize-1; i++ )
    {
        best_i = i;
        for ( j = i+1; j < nSize; j++ )
            if ( Vec_IntEntry(vCosts, Abc_Lit2Var(pArray[j])) > Vec_IntEntry(vCosts, Abc_Lit2Var(pArray[best_i])) )
                best_i = j;
        ABC_SWAP( int, pArray[i], pArray[best_i] );
    }
}
static inline void Vec_IntPushOrderCost2( Vec_Int_t * vSuper, Vec_Int_t * vCosts, int iLit )
{
    int i, nSize, * pArray;
    Vec_IntPush( vSuper, iLit );
    pArray = Vec_IntArray(vSuper);
    nSize = Vec_IntSize(vSuper);
    for ( i = nSize-1; i > 0; i-- )
    {
        if ( Vec_IntEntry(vCosts, Abc_Lit2Var(pArray[i])) <= Vec_IntEntry(vCosts, Abc_Lit2Var(pArray[i - 1])) )
            return;
        ABC_SWAP( int, pArray[i], pArray[i - 1] );
    }
}
static inline int Vec_IntFindFirstSameDelayAsLast( Bal_Man_t * p, Vec_Int_t * vSuper )
{
    int i, DelayCur, Delay = Bal_LitDelay( p, Vec_IntEntryLast(vSuper) );
    assert( Vec_IntSize(vSuper) > 1 );
    for ( i = Vec_IntSize(vSuper)-1; i > 0; i-- )
    {
        DelayCur = Bal_LitDelay( p, Vec_IntEntry(vSuper, i-1) );
        assert( DelayCur >= Delay );
        if ( DelayCur > Delay )
            return i;
    }
    return i;
}


/**Function*************************************************************

  Synopsis    [Select the best pair to merge.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Bal_ManFindBestPair( Bal_Man_t * p, Vec_Int_t * vSuper, Gia_Obj_t * pObj )
{
    int * pSuper = Vec_IntArray(vSuper);
    int iBeg = Vec_IntFindFirstSameDelayAsLast( p, vSuper );
    int iEnd = Vec_IntSize(vSuper)-1;
    int i, k, iBest = -1, kBest = -1, BestCost = ABC_INFINITY, Cost;
    assert( iBeg <= iEnd );
    // check if we can add to the higher levels without increasing cost
    for ( k = iBeg-1; k >= 0; k-- )
    for ( i = iEnd; i >= iBeg; i-- )
    {
        Cost = Bal_ManEvalTwo( p, pSuper[i], pSuper[k], 0, Gia_ObjIsXor(pObj) );
        if ( Cost == -1 )
            continue;
        if ( Cost == Bal_LitCost(p, pSuper[k]) )
        {
//            printf( "A" );
            return (k << 16)|i;
        }
        if ( BestCost > Cost )
            BestCost = Cost, iBest = i, kBest = k;
    }
    if ( BestCost != ABC_INFINITY && (BestCost >> 4) == Bal_LitDelay(p, pSuper[kBest]) )
    {
//        printf( "B" );
        return (kBest << 16)|iBest;
    }
    // check if some can be added to lowest level without increasing cost
    BestCost = ABC_INFINITY;
    for ( i = iBeg; i <= iEnd; i++ )
    for ( k = i+1;  k <= iEnd; k++ )
    {
        Cost = Bal_ManEvalTwo( p, pSuper[i], pSuper[k], 0, Gia_ObjIsXor(pObj) );
        if ( Cost == -1 )
            continue;
        if ( Cost == Abc_MaxInt(Bal_LitCost(p, pSuper[i]), Bal_LitCost(p, pSuper[k])) )
        {
//            printf( "C" );
            return (k << 16)|i;
        }
        if ( BestCost > Cost )
            BestCost = Cost, iBest = i, kBest = k;
    }
    if ( BestCost != ABC_INFINITY )
    {
//        printf( "D" );
        return (kBest << 16)|iBest;
    }
//    printf( "E" );
    // group pairs from lowest level based on proximity
    return (iEnd << 16)|(iEnd-1);
}

/**Function*************************************************************

  Synopsis    [Simplify multi-input AND/XOR.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManSimplifyXor( Vec_Int_t * vSuper )
{
    int i, k = 0, Prev = -1, This, fCompl = 0;
    Vec_IntForEachEntry( vSuper, This, i )
    {
        if ( This == 0 )
            continue;
        if ( This == 1 )
            fCompl ^= 1; 
        else if ( Prev != This )
            Vec_IntWriteEntry( vSuper, k++, This ), Prev = This;
        else
            Prev = -1, k--;
    }
    Vec_IntShrink( vSuper, k );
    if ( Vec_IntSize( vSuper ) == 0 )
        Vec_IntPush( vSuper, fCompl );
    else if ( fCompl )
        Vec_IntWriteEntry( vSuper, 0, Abc_LitNot(Vec_IntEntry(vSuper, 0)) );
}
static inline void Gia_ManSimplifyAnd( Vec_Int_t * vSuper )
{
    int i, k = 0, Prev = -1, This;
    Vec_IntForEachEntry( vSuper, This, i )
    {
        if ( This == 0 )
            { Vec_IntFill(vSuper, 1, 0); return; }
        if ( This == 1 )
            continue;
        if ( Prev == -1 || Abc_Lit2Var(Prev) != Abc_Lit2Var(This) )
            Vec_IntWriteEntry( vSuper, k++, This ), Prev = This;
        else if ( Prev != This )
            { Vec_IntFill(vSuper, 1, 0); return; }
    }
    Vec_IntShrink( vSuper, k );
    if ( Vec_IntSize( vSuper ) == 0 )
        Vec_IntPush( vSuper, 1 );
}

/**Function*************************************************************

  Synopsis    [Collect multi-input AND/XOR.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManSuperCollectXor_rec( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    assert( !Gia_IsComplement(pObj) );
    if ( !Gia_ObjIsXor(pObj) || 
//        Gia_ObjRefNum(p, pObj) > 1 || 
        Gia_ObjRefNum(p, pObj) > 3 || 
//        (Gia_ObjRefNum(p, pObj) == 2 && (Gia_ObjRefNum(p, Gia_ObjFanin0(pObj)) == 1 || Gia_ObjRefNum(p, Gia_ObjFanin1(pObj)) == 1)) || 
        Vec_IntSize(p->vSuper) > BAL_SUPER )
    {
        Vec_IntPush( p->vSuper, Gia_ObjToLit(p, pObj) );
        return;
    }
    assert( !Gia_ObjFaninC0(pObj) && !Gia_ObjFaninC1(pObj) );
    Gia_ManSuperCollectXor_rec( p, Gia_ObjFanin0(pObj) );
    Gia_ManSuperCollectXor_rec( p, Gia_ObjFanin1(pObj) );
}
static inline void Gia_ManSuperCollectAnd_rec( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( Gia_IsComplement(pObj) || 
        !Gia_ObjIsAndReal(p, pObj) || 
//        Gia_ObjRefNum(p, pObj) > 1 || 
        Gia_ObjRefNum(p, pObj) > 3 || 
//        (Gia_ObjRefNum(p, pObj) == 2 && (Gia_ObjRefNum(p, Gia_ObjFanin0(pObj)) == 1 || Gia_ObjRefNum(p, Gia_ObjFanin1(pObj)) == 1)) || 
        Vec_IntSize(p->vSuper) > BAL_SUPER )
    {
        Vec_IntPush( p->vSuper, Gia_ObjToLit(p, pObj) );
        return;
    }
    Gia_ManSuperCollectAnd_rec( p, Gia_ObjChild0(pObj) );
    Gia_ManSuperCollectAnd_rec( p, Gia_ObjChild1(pObj) );
}
static inline void Gia_ManSuperCollect( Gia_Man_t * p, Gia_Obj_t * pObj )
{
//    int nSize;
    if ( p->vSuper == NULL )
        p->vSuper = Vec_IntAlloc( 1000 );
    else
        Vec_IntClear( p->vSuper );
    if ( Gia_ObjIsXor(pObj) )
    {
        assert( !Gia_ObjFaninC0(pObj) && !Gia_ObjFaninC1(pObj) );
        Gia_ManSuperCollectXor_rec( p, Gia_ObjFanin0(pObj) );
        Gia_ManSuperCollectXor_rec( p, Gia_ObjFanin1(pObj) );
//        nSize = Vec_IntSize(vSuper);
        Vec_IntSort( p->vSuper, 0 );
        Gia_ManSimplifyXor( p->vSuper );
//        if ( nSize != Vec_IntSize(vSuper) )
//            printf( "X %d->%d  ", nSize, Vec_IntSize(vSuper) );
    }
    else if ( Gia_ObjIsAndReal(p, pObj) )
    {
        Gia_ManSuperCollectAnd_rec( p, Gia_ObjChild0(pObj) );
        Gia_ManSuperCollectAnd_rec( p, Gia_ObjChild1(pObj) );
//        nSize = Vec_IntSize(vSuper);
        Vec_IntSort( p->vSuper, 0 );
        Gia_ManSimplifyAnd( p->vSuper );
//        if ( nSize != Vec_IntSize(vSuper) )
//            printf( "A %d->%d  ", nSize, Vec_IntSize(vSuper) );
    }
    else assert( 0 );
//    if ( nSize > 10 )
//        printf( "%d ", nSize );
    assert( Vec_IntSize(p->vSuper) > 0 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManCreateGate( Gia_Man_t * pNew, Gia_Obj_t * pObj, Vec_Int_t * vSuper )
{
    int iLit0 = Vec_IntPop(vSuper);
    int iLit1 = Vec_IntPop(vSuper);
    int iLit, i;
    if ( !Gia_ObjIsXor(pObj) )
        iLit = Gia_ManHashAnd( pNew, iLit0, iLit1 );
    else if ( pNew->pMuxes )
        iLit = Gia_ManHashXorReal( pNew, iLit0, iLit1 );
    else 
        iLit = Gia_ManHashXor( pNew, iLit0, iLit1 );
    Vec_IntPush( vSuper, iLit );
    Bal_ManSetGateLevel( Bal_GiaMan(pNew), pObj, iLit );
//    Gia_ObjSetGateLevel( pNew, Gia_ManObj(pNew, Abc_Lit2Var(iLit)) );
    // shift to the corrent location
    for ( i = Vec_IntSize(vSuper)-1; i > 0; i-- )
    {
        int iLit1 = Vec_IntEntry(vSuper, i);
        int iLit2 = Vec_IntEntry(vSuper, i-1);
        if ( Gia_ObjLevelId(pNew, Abc_Lit2Var(iLit1)) <= Gia_ObjLevelId(pNew, Abc_Lit2Var(iLit2)) )
            break;
        Vec_IntWriteEntry( vSuper, i,   iLit2 );
        Vec_IntWriteEntry( vSuper, i-1, iLit1 );
    }
}
static inline int Gia_ManBalanceGate( Gia_Man_t * pNew, Gia_Obj_t * pObj, Vec_Int_t * vSuper, int * pLits, int nLits )
{
    Vec_IntClear( vSuper );
    if ( nLits == 1 )
        Vec_IntPush( vSuper, pLits[0] );
    else if ( nLits == 2 )
    {
        Vec_IntPush( vSuper, pLits[0] );
        Vec_IntPush( vSuper, pLits[1] );
        Gia_ManCreateGate( pNew, pObj, vSuper );
    }
    else if ( nLits > 2 )
    {
        Bal_Man_t * p = Bal_GiaMan(pNew); int i;
        for ( i = 0; i < nLits; i++ )
            Vec_IntPush( vSuper, pLits[i] );
        // sort by level/cut-size
        Vec_IntSelectSortCostLit( vSuper, p->vCosts );
        // iterate till everything is grouped
        while ( Vec_IntSize(vSuper) > 1 )
        {
            int iLit, Res = Bal_ManFindBestPair( p, vSuper, pObj );
            int iBest = Vec_IntEntry( vSuper, Res >> 16 );
            int kBest = Vec_IntEntry( vSuper, Res & 0xFFFF );
            Vec_IntRemove( vSuper, iBest );
            Vec_IntRemove( vSuper, kBest );
            if ( Gia_ObjIsXor(pObj) )
                iLit = Gia_ManHashXorReal( pNew, iBest, kBest );
            else 
                iLit = Gia_ManHashAnd( pNew, iBest, kBest );
            Bal_ManSetGateLevel( p, pObj, iLit );
            Vec_IntPushOrderCost2( vSuper, p->vCosts, iLit );
        }
    }
    // consider trivial case
    assert( Vec_IntSize(vSuper) == 1 );
    return Vec_IntEntry(vSuper, 0);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManBalance_rec( Gia_Man_t * pNew, Gia_Man_t * p, Gia_Obj_t * pObj )
{
    int i, iLit, iBeg, iEnd;
    if ( ~pObj->Value )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    // handle MUX
    if ( Gia_ObjIsMux(p, pObj) )
    {
        Gia_ManBalance_rec( pNew, p, Gia_ObjFanin0(pObj) );
        Gia_ManBalance_rec( pNew, p, Gia_ObjFanin1(pObj) );
        Gia_ManBalance_rec( pNew, p, Gia_ObjFanin2(p, pObj) );
        pObj->Value = Gia_ManHashMuxReal( pNew, Gia_ObjFanin2Copy(p, pObj), Gia_ObjFanin1Copy(pObj), Gia_ObjFanin0Copy(pObj) );
        Bal_ManSetGateLevel( Bal_GiaMan(pNew), pObj, pObj->Value );
//        Gia_ObjSetGateLevel( pNew, Gia_ManObj(pNew, Abc_Lit2Var(pObj->Value)) );
        return;
    }
    // find supergate
    Gia_ManSuperCollect( p, pObj );
    // save entries
    if ( p->vStore == NULL )
        p->vStore = Vec_IntAlloc( 1000 );
    iBeg = Vec_IntSize( p->vStore );
    Vec_IntAppend( p->vStore, p->vSuper );
    iEnd = Vec_IntSize( p->vStore );
    // call recursively
    Vec_IntForEachEntryStartStop( p->vStore, iLit, i, iBeg, iEnd )
    {
        Gia_Obj_t * pTemp = Gia_ManObj( p, Abc_Lit2Var(iLit) );
        Gia_ManBalance_rec( pNew, p, pTemp );
        Vec_IntWriteEntry( p->vStore, i, Abc_LitNotCond(pTemp->Value, Abc_LitIsCompl(iLit)) );
    }
    assert( Vec_IntSize(p->vStore) == iEnd );
    // consider general case
    pObj->Value = Gia_ManBalanceGate( pNew, pObj, p->vSuper, Vec_IntEntryP(p->vStore, iBeg), iEnd-iBeg );
    Vec_IntShrink( p->vStore, iBeg );
}
static inline Gia_Man_t * Gia_ManBalanceInt( Gia_Man_t * p, int nLutSize, int nCutNum, int fVerbose )
{
    Bal_Man_t * pMan;
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;
    int i;
    Gia_ManFillValue( p );
    Gia_ManCreateRefs( p ); 
    // start the new manager
    pNew = Gia_ManStart( 3*Gia_ManObjNum(p)/2 );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    pNew->pMuxes = ABC_CALLOC( unsigned, pNew->nObjsAlloc );
    pNew->vLevels = Vec_IntStart( pNew->nObjsAlloc );
    // create constant and inputs
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    // create balancing manager
    pMan = Bal_ManAlloc( p, pNew, nLutSize, nCutNum, fVerbose );
    // create internal nodes
    Gia_ManHashStart( pNew );
    Gia_ManForEachCo( p, pObj, i )
        Gia_ManBalance_rec( pNew, p, Gia_ObjFanin0(pObj) );
    Gia_ManForEachCo( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
//    if ( fVerbose )
    {
        int nLevelMax = 0;
        Gia_ManForEachCo( pNew, pObj, i )
        {
            nLevelMax = Abc_MaxInt( nLevelMax, Bal_ObjDelay(pMan, Gia_ObjFaninId0p(pNew, pObj)) );
//            printf( "%d=%d ", i, Bal_ObjDelay(pMan, Gia_ObjFaninId0p(pNew, pObj)) );
        }
        printf( "Best delay = %d\n", nLevelMax );
    }

//    assert( Gia_ManObjNum(pNew) <= Gia_ManObjNum(p) );
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    // delete manager
    Bal_ManFree( pMan );
    // perform cleanup
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}
Gia_Man_t * Gia_ManBalanceLut( Gia_Man_t * p, int nLutSize, int nCutNum, int fVerbose )
{
    Gia_Man_t * pNew, * pNew1, * pNew2;
    if ( fVerbose )      Gia_ManPrintStats( p, NULL );
    pNew = Gia_ManDupMuxes( p, 2 );
    if ( fVerbose )      Gia_ManPrintStats( pNew, NULL );
    pNew1 = Gia_ManBalanceInt( pNew, nLutSize, nCutNum, fVerbose );
    if ( fVerbose )      Gia_ManPrintStats( pNew1, NULL );
    Gia_ManStop( pNew );
    pNew2 = Gia_ManDupNoMuxes( pNew1, 0 );
    if ( fVerbose )      Gia_ManPrintStats( pNew2, NULL );
    Gia_ManStop( pNew1 );
    return pNew2;
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

