/**CFile****************************************************************

  FileName    [giaResub.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Resubstitution.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaResub.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "misc/vec/vecWec.h"
#include "misc/vec/vecQue.h"
#include "misc/vec/vecHsh.h"
#include "misc/util/utilTruth.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes MFFCs of all qualifying nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ObjCheckMffc_rec( Gia_Man_t * p,Gia_Obj_t * pObj, int Limit, Vec_Int_t * vNodes )
{
    int iFanin;
    if ( Gia_ObjIsCi(pObj) )
        return 1;
    assert( Gia_ObjIsAnd(pObj) );
    iFanin = Gia_ObjFaninId0p(p, pObj);
    Vec_IntPush( vNodes, iFanin );
    if ( !Gia_ObjRefDecId(p, iFanin) && (Vec_IntSize(vNodes) > Limit || !Gia_ObjCheckMffc_rec(p, Gia_ObjFanin0(pObj), Limit, vNodes)) )
        return 0;
    iFanin = Gia_ObjFaninId1p(p, pObj);
    Vec_IntPush( vNodes, iFanin );
    if ( !Gia_ObjRefDecId(p, iFanin) && (Vec_IntSize(vNodes) > Limit || !Gia_ObjCheckMffc_rec(p, Gia_ObjFanin1(pObj), Limit, vNodes)) )
        return 0;
    if ( !Gia_ObjIsMux(p, pObj) )
        return 1;
    iFanin = Gia_ObjFaninId2p(p, pObj);
    Vec_IntPush( vNodes, iFanin );
    if ( !Gia_ObjRefDecId(p, iFanin) && (Vec_IntSize(vNodes) > Limit || !Gia_ObjCheckMffc_rec(p, Gia_ObjFanin2(p, pObj), Limit, vNodes)) )
        return 0;
    return 1;
}
static inline int Gia_ObjCheckMffc( Gia_Man_t * p, Gia_Obj_t * pRoot, int Limit, Vec_Int_t * vNodes, Vec_Int_t * vLeaves, Vec_Int_t * vInners )
{
    int RetValue, iObj, i;
    Vec_IntClear( vNodes );
    RetValue = Gia_ObjCheckMffc_rec( p, pRoot, Limit, vNodes );
    if ( RetValue )
    {
        Vec_IntClear( vLeaves );
        Vec_IntClear( vInners );
        Vec_IntSort( vNodes, 0 );
        Vec_IntForEachEntry( vNodes, iObj, i )
            if ( Gia_ObjRefNumId(p, iObj) > 0 || Gia_ObjIsCi(Gia_ManObj(p, iObj)) )
            {
                if ( !Vec_IntSize(vLeaves) || Vec_IntEntryLast(vLeaves) != iObj )
                    Vec_IntPush( vLeaves, iObj );
            }
            else
            {
                if ( !Vec_IntSize(vInners) || Vec_IntEntryLast(vInners) != iObj )
                    Vec_IntPush( vInners, iObj );
            }
        Vec_IntPush( vInners, Gia_ObjId(p, pRoot) );
    }
    Vec_IntForEachEntry( vNodes, iObj, i )
        Gia_ObjRefIncId( p, iObj );
    return RetValue;
}
Vec_Wec_t * Gia_ManComputeMffcs( Gia_Man_t * p, int LimitMin, int LimitMax, int SuppMax, int RatioBest )
{
    Gia_Obj_t * pObj;
    Vec_Wec_t * vMffcs;
    Vec_Int_t * vNodes, * vLeaves, * vInners, * vMffc;
    int i, iPivot;
    assert( p->pMuxes );
    vNodes  = Vec_IntAlloc( 2 * LimitMax );
    vLeaves = Vec_IntAlloc( 2 * LimitMax );
    vInners = Vec_IntAlloc( 2 * LimitMax );
    vMffcs  = Vec_WecAlloc( 1000 );
    Gia_ManCreateRefs( p );
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( !Gia_ObjRefNum(p, pObj) )
            continue;
        if ( !Gia_ObjCheckMffc(p, pObj, LimitMax, vNodes, vLeaves, vInners) )
            continue;
        if ( Vec_IntSize(vInners) < LimitMin )
            continue;
        if ( Vec_IntSize(vLeaves) > SuppMax )
            continue;
        // improve cut
        // collect cut
        vMffc = Vec_WecPushLevel( vMffcs );
        Vec_IntGrow( vMffc, Vec_IntSize(vLeaves) + Vec_IntSize(vInners) + 20 );
        Vec_IntPush( vMffc, i );
        Vec_IntPush( vMffc, Vec_IntSize(vLeaves) );
        Vec_IntPush( vMffc, Vec_IntSize(vInners) );
        Vec_IntAppend( vMffc, vLeaves );
//        Vec_IntAppend( vMffc, vInners );
        // add last entry equal to the ratio
        Vec_IntPush( vMffc, 1000 * Vec_IntSize(vInners) / Vec_IntSize(vLeaves) );
    }
    Vec_IntFree( vNodes );
    Vec_IntFree( vLeaves );
    Vec_IntFree( vInners );
    // sort MFFCs by their inner/leaf ratio
    Vec_WecSortByLastInt( vMffcs, 1 );
    Vec_WecForEachLevel( vMffcs, vMffc, i )
        Vec_IntPop( vMffc );
    // remove those whose ratio is not good
    iPivot = RatioBest * Vec_WecSize(vMffcs) / 100;
    Vec_WecForEachLevelStart( vMffcs, vMffc, i, iPivot )
        Vec_IntErase( vMffc );
    assert( iPivot <= Vec_WecSize(vMffcs) );
    Vec_WecShrink( vMffcs, iPivot );
    return vMffcs;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManPrintDivStats( Gia_Man_t * p, Vec_Wec_t * vMffcs, Vec_Wec_t * vPivots ) 
{
    int fVerbose = 0;
    Vec_Int_t * vMffc;
    int i, nDivs, nDivsAll = 0, nDivs0 = 0;
    Vec_WecForEachLevel( vMffcs, vMffc, i )
    {
        nDivs = Vec_IntSize(vMffc) - 3 - Vec_IntEntry(vMffc, 1) - Vec_IntEntry(vMffc, 2);
        nDivs0 += (nDivs == 0);
        nDivsAll += nDivs;
        if ( !fVerbose )
            continue;
        printf( "%6d : ",      Vec_IntEntry(vMffc, 0) );
        printf( "Leaf =%3d  ", Vec_IntEntry(vMffc, 1) );
        printf( "Mffc =%4d  ", Vec_IntEntry(vMffc, 2) );
        printf( "Divs =%4d  ", nDivs );
        printf( "\n" );
    }
    printf( "Collected %d (%.1f %%) MFFCs and %d (%.1f %%) have no divisors (div ave for others is %.2f).\n", 
        Vec_WecSize(vMffcs), 100.0 * Vec_WecSize(vMffcs) / Gia_ManAndNum(p), 
        nDivs0, 100.0 * nDivs0 / Gia_ManAndNum(p), 
        1.0*nDivsAll/Abc_MaxInt(1, Vec_WecSize(vMffcs) - nDivs0) );
    printf( "Using %.2f MB for MFFCs and %.2f MB for pivots.   ", 
        Vec_WecMemory(vMffcs)/(1<<20), Vec_WecMemory(vPivots)/(1<<20) );
}

/**Function*************************************************************

  Synopsis    [Compute divisors and Boolean functions for the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManAddDivisors( Gia_Man_t * p, Vec_Wec_t * vMffcs )
{
    Vec_Wec_t * vPivots;
    Vec_Int_t * vMffc, * vPivot, * vPivot0, * vPivot1;
    Vec_Int_t * vCommon, * vCommon2, * vMap;
    Gia_Obj_t * pObj;
    int i, k, iObj, iPivot, iMffc;
//abctime clkStart = Abc_Clock();
    // initialize pivots (mapping of nodes into MFFCs whose leaves they are)
    vMap = Vec_IntStartFull( Gia_ManObjNum(p) );
    vPivots = Vec_WecStart( Gia_ManObjNum(p) );
    Vec_WecForEachLevel( vMffcs, vMffc, i )
    {
        assert( Vec_IntSize(vMffc) == 3 + Vec_IntEntry(vMffc, 1) );
        iPivot = Vec_IntEntry( vMffc, 0 );
        Vec_IntWriteEntry( vMap, iPivot, i );
        // iterate through the MFFC leaves
        Vec_IntForEachEntryStart( vMffc, iObj, k, 3 )
        {
            vPivot = Vec_WecEntry( vPivots, iObj );
            if ( Vec_IntSize(vPivot) == 0 )
                Vec_IntGrow(vPivot, 4);
            Vec_IntPush( vPivot, iPivot );            
        }
    }
    Vec_WecForEachLevel( vPivots, vPivot, i )
        Vec_IntSort( vPivot, 0 );
    // create pivots for internal nodes while growing MFFCs
    vCommon = Vec_IntAlloc( 100 );
    vCommon2 = Vec_IntAlloc( 100 );
    Gia_ManForEachAnd( p, pObj, i )
    {
        // find commont pivots
        // the slow down happens because some PIs have very large sets of pivots
        vPivot0 = Vec_WecEntry( vPivots, Gia_ObjFaninId0(pObj, i) );
        vPivot1 = Vec_WecEntry( vPivots, Gia_ObjFaninId1(pObj, i) );
        Vec_IntTwoFindCommon( vPivot0, vPivot1, vCommon );
        if ( Gia_ObjIsMuxId(p, i) )
        {
            vPivot = Vec_WecEntry( vPivots, Gia_ObjFaninId2(p, i) );
            Vec_IntTwoFindCommon( vPivot, vCommon, vCommon2 );
            ABC_SWAP( Vec_Int_t *, vCommon, vCommon2 );
        }
        if ( Vec_IntSize(vCommon) == 0 )
            continue;
        // add new pivots (this trick increased memory used in vPivots)
        vPivot = Vec_WecEntry( vPivots, i );
        Vec_IntTwoMerge2( vPivot, vCommon, vCommon2 );
        ABC_SWAP( Vec_Int_t, *vPivot, *vCommon2 );
        // grow MFFCs
        Vec_IntForEachEntry( vCommon, iObj, k )
        {
            iMffc = Vec_IntEntry( vMap, iObj );
            assert( iMffc != -1 );
            vMffc = Vec_WecEntry( vMffcs, iMffc );
            Vec_IntPush( vMffc, i );
        }
    }
//Abc_PrintTime( 1, "Time", Abc_Clock() - clkStart );
    Vec_IntFree( vCommon );
    Vec_IntFree( vCommon2 );
    Vec_IntFree( vMap );
    Gia_ManPrintDivStats( p, vMffcs, vPivots );
    Vec_WecFree( vPivots );
    // returns the modified array of MFFCs
}
void Gia_ManResubTest( Gia_Man_t * p )
{
    Vec_Wec_t * vMffcs;
    Gia_Man_t * pNew = Gia_ManDupMuxes( p, 2 );
abctime clkStart = Abc_Clock();
    vMffcs = Gia_ManComputeMffcs( pNew, 4, 100, 8, 100 );
    Gia_ManAddDivisors( pNew, vMffcs );
    Vec_WecFree( vMffcs );
Abc_PrintTime( 1, "Time", Abc_Clock() - clkStart );
    Gia_ManStop( pNew );
}





/**Function*************************************************************

  Synopsis    [Resubstitution data-structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
typedef struct Gia_ResbMan_t_ Gia_ResbMan_t;
struct Gia_ResbMan_t_
{
    int         nWords;
    int         nLimit;
    int         nDivsMax;
    int         iChoice;
    int         fUseXor;
    int         fDebug;
    int         fVerbose;
    int         fVeryVerbose;
    Vec_Ptr_t * vDivs;
    Vec_Int_t * vGates;
    Vec_Int_t * vUnateLits[2];
    Vec_Int_t * vNotUnateVars[2];
    Vec_Int_t * vUnatePairs[2];
    Vec_Int_t * vBinateVars;
    Vec_Int_t * vUnateLitsW[2];
    Vec_Int_t * vUnatePairsW[2];
    Vec_Wec_t * vSorter;
    word *      pSets[2];
    word *      pDivA;
    word *      pDivB;
    Vec_Wrd_t * vSims;
};
Gia_ResbMan_t * Gia_ResbAlloc( int nWords )
{
    Gia_ResbMan_t * p   = ABC_CALLOC( Gia_ResbMan_t, 1 );
    p->nWords           = nWords;
    p->vUnateLits[0]    = Vec_IntAlloc( 100 );
    p->vUnateLits[1]    = Vec_IntAlloc( 100 );
    p->vNotUnateVars[0] = Vec_IntAlloc( 100 );
    p->vNotUnateVars[1] = Vec_IntAlloc( 100 );
    p->vUnatePairs[0]   = Vec_IntAlloc( 100 );
    p->vUnatePairs[1]   = Vec_IntAlloc( 100 );
    p->vUnateLitsW[0]   = Vec_IntAlloc( 100 );
    p->vUnateLitsW[1]   = Vec_IntAlloc( 100 );
    p->vUnatePairsW[0]  = Vec_IntAlloc( 100 );
    p->vUnatePairsW[1]  = Vec_IntAlloc( 100 );
    p->vSorter          = Vec_WecAlloc( nWords*64 );
    p->vBinateVars      = Vec_IntAlloc( 100 );
    p->vGates           = Vec_IntAlloc( 100 );
    p->vDivs            = Vec_PtrAlloc( 100 );
    p->pSets[0]         = ABC_CALLOC( word, nWords );
    p->pSets[1]         = ABC_CALLOC( word, nWords );
    p->pDivA            = ABC_CALLOC( word, nWords );
    p->pDivB            = ABC_CALLOC( word, nWords );
    p->vSims            = Vec_WrdAlloc( 100 );
    return p;
}
void Gia_ResbInit( Gia_ResbMan_t * p, Vec_Ptr_t * vDivs, int nWords, int nLimit, int nDivsMax, int iChoice, int fUseXor, int fDebug, int fVerbose, int fVeryVerbose )
{
    assert( p->nWords == nWords );
    p->nLimit       = nLimit;
    p->nDivsMax     = nDivsMax;
    p->iChoice      = iChoice;
    p->fUseXor      = fUseXor;
    p->fDebug       = fDebug;
    p->fVerbose     = fVerbose;
    p->fVeryVerbose = fVeryVerbose;
    Abc_TtCopy( p->pSets[0], (word *)Vec_PtrEntry(vDivs, 0), nWords, 0 );
    Abc_TtCopy( p->pSets[1], (word *)Vec_PtrEntry(vDivs, 1), nWords, 0 );
    Vec_PtrClear( p->vDivs );
    Vec_PtrAppend( p->vDivs, vDivs );
    Vec_IntClear( p->vGates );
    Vec_IntClear( p->vUnateLits[0]    );
    Vec_IntClear( p->vUnateLits[1]    );
    Vec_IntClear( p->vNotUnateVars[0] );
    Vec_IntClear( p->vNotUnateVars[1] );
    Vec_IntClear( p->vUnatePairs[0]   );
    Vec_IntClear( p->vUnatePairs[1]   );
    Vec_IntClear( p->vUnateLitsW[0]   );
    Vec_IntClear( p->vUnateLitsW[1]   );
    Vec_IntClear( p->vUnatePairsW[0]  );
    Vec_IntClear( p->vUnatePairsW[1]  );
    Vec_IntClear( p->vBinateVars      );
}
void Gia_ResbFree( Gia_ResbMan_t * p )
{
    Vec_IntFree( p->vUnateLits[0]    );
    Vec_IntFree( p->vUnateLits[1]    );
    Vec_IntFree( p->vNotUnateVars[0] );
    Vec_IntFree( p->vNotUnateVars[1] );
    Vec_IntFree( p->vUnatePairs[0]   );
    Vec_IntFree( p->vUnatePairs[1]   );
    Vec_IntFree( p->vUnateLitsW[0]   );
    Vec_IntFree( p->vUnateLitsW[1]   );
    Vec_IntFree( p->vUnatePairsW[0]  );
    Vec_IntFree( p->vUnatePairsW[1]  );
    Vec_IntFree( p->vBinateVars      );
    Vec_IntFree( p->vGates           );
    Vec_WrdFree( p->vSims            );
    Vec_PtrFree( p->vDivs            );
    Vec_WecFree( p->vSorter          );
    ABC_FREE( p->pSets[0] );
    ABC_FREE( p->pSets[1] );
    ABC_FREE( p->pDivA );
    ABC_FREE( p->pDivB );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Print resubstitution.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManResubPrintNode( Vec_Int_t * vRes, int nVars, int Node, int fCompl )
{
    extern void Gia_ManResubPrintLit( Vec_Int_t * vRes, int nVars, int iLit );
    int iLit0 = Vec_IntEntry( vRes, 2*Node + 0 );
    int iLit1 = Vec_IntEntry( vRes, 2*Node + 1 );
    assert( iLit0 != iLit1 );
    if ( iLit0 > iLit1 && Abc_LitIsCompl(fCompl) ) // xor
    {
        printf( "~" );
        fCompl = 0;
    }
    printf( "(" );
    Gia_ManResubPrintLit( vRes, nVars, Abc_LitNotCond(iLit0, fCompl) );
    printf( " %c ", iLit0 > iLit1 ? '^' : (fCompl ? '|' : '&') );
    Gia_ManResubPrintLit( vRes, nVars, Abc_LitNotCond(iLit1, fCompl) );
    printf( ")" );
}
void Gia_ManResubPrintLit( Vec_Int_t * vRes, int nVars, int iLit )
{
    if ( Abc_Lit2Var(iLit) < nVars )
    {
        if ( nVars < 26 )
            printf( "%s%c", Abc_LitIsCompl(iLit) ? "~":"", 'a' + Abc_Lit2Var(iLit)-2 );
        else
            printf( "%si%d", Abc_LitIsCompl(iLit) ? "~":"", Abc_Lit2Var(iLit)-2 );
    }
    else
        Gia_ManResubPrintNode( vRes, nVars, Abc_Lit2Var(iLit) - nVars, Abc_LitIsCompl(iLit) );
}
int Gia_ManResubPrint( Vec_Int_t * vRes, int nVars )
{
    int iTopLit;
    if ( Vec_IntSize(vRes) == 0 )
        return printf( "none" );
    assert( Vec_IntSize(vRes) % 2 == 1 );
    iTopLit = Vec_IntEntryLast(vRes);
    if ( iTopLit == 0 )
        return printf( "const0" );
    if ( iTopLit == 1 )
        return printf( "const1" );
    Gia_ManResubPrintLit( vRes, nVars, iTopLit );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Verify resubstitution.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManResubVerify( Gia_ResbMan_t * p, word * pFunc )
{
    int nVars = Vec_PtrSize(p->vDivs);
    int iTopLit, RetValue;
    word * pDivRes; 
    if ( Vec_IntSize(p->vGates) == 0 )
        return -1;
    iTopLit = Vec_IntEntryLast(p->vGates);
    assert( iTopLit >= 0 );
    if ( iTopLit == 0 )
    {
        if ( pFunc ) Abc_TtClear( pFunc, p->nWords );
        return Abc_TtIsConst0( p->pSets[1], p->nWords );
    }
    if ( iTopLit == 1 )
    {
        if ( pFunc ) Abc_TtFill( pFunc, p->nWords );
        return Abc_TtIsConst0( p->pSets[0], p->nWords );
    }
    if ( Abc_Lit2Var(iTopLit) < nVars )
    {
        assert( Vec_IntSize(p->vGates) == 1 );
        pDivRes = (word *)Vec_PtrEntry( p->vDivs, Abc_Lit2Var(iTopLit) );
    }
    else
    {
        int i, iLit0, iLit1;
        assert( Vec_IntSize(p->vGates) > 1 );
        assert( Vec_IntSize(p->vGates) % 2 == 1 );
        assert( Abc_Lit2Var(iTopLit)-nVars == Vec_IntSize(p->vGates)/2-1 );
        Vec_WrdFill( p->vSims, p->nWords * Vec_IntSize(p->vGates)/2, 0 );
        Vec_IntForEachEntryDouble( p->vGates, iLit0, iLit1, i )
        {
            int iVar0 = Abc_Lit2Var(iLit0);
            int iVar1 = Abc_Lit2Var(iLit1);
            word * pDiv0 = iVar0 < nVars ? (word *)Vec_PtrEntry(p->vDivs, iVar0) : Vec_WrdEntryP(p->vSims, p->nWords*(iVar0 - nVars));
            word * pDiv1 = iVar1 < nVars ? (word *)Vec_PtrEntry(p->vDivs, iVar1) : Vec_WrdEntryP(p->vSims, p->nWords*(iVar1 - nVars));
            word * pDiv  = Vec_WrdEntryP(p->vSims, p->nWords*i/2);
            if ( iVar0 < iVar1 )
                Abc_TtAndCompl( pDiv, pDiv0, Abc_LitIsCompl(iLit0), pDiv1, Abc_LitIsCompl(iLit1), p->nWords );
            else if ( iVar0 > iVar1 )
            {
                assert( !Abc_LitIsCompl(iLit0) );
                assert( !Abc_LitIsCompl(iLit1) );
                Abc_TtXor( pDiv, pDiv0, pDiv1, p->nWords, 0 );
            }
            else assert( 0 );
        }
        pDivRes = Vec_WrdEntryP( p->vSims, p->nWords*(Vec_IntSize(p->vGates)/2-1) );
    }
    if ( Abc_LitIsCompl(iTopLit) )
        RetValue = !Abc_TtIntersectOne(p->pSets[1], 0, pDivRes, 0, p->nWords) && !Abc_TtIntersectOne(p->pSets[0], 0, pDivRes, 1, p->nWords);
    else
        RetValue = !Abc_TtIntersectOne(p->pSets[0], 0, pDivRes, 0, p->nWords) && !Abc_TtIntersectOne(p->pSets[1], 0, pDivRes, 1, p->nWords);
    if ( pFunc ) Abc_TtCopy( pFunc, pDivRes, p->nWords, Abc_LitIsCompl(iTopLit) );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Construct AIG manager from gates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManConstructFromMap( Gia_Man_t * pNew, Vec_Int_t * vGates, int nVars, Vec_Int_t * vUsed, Vec_Int_t * vCopy, int fHash )
{
    int i, iLit0, iLit1, iLitRes, iTopLit = Vec_IntEntryLast( vGates );
    assert( Vec_IntSize(vUsed) == nVars );
    assert( Vec_IntSize(vGates) > 1 );
    assert( Vec_IntSize(vGates) % 2 == 1 );
    assert( Abc_Lit2Var(iTopLit)-nVars == Vec_IntSize(vGates)/2-1 );
    Vec_IntClear( vCopy );
    Vec_IntForEachEntryDouble( vGates, iLit0, iLit1, i )
    {
        int iVar0 = Abc_Lit2Var(iLit0);
        int iVar1 = Abc_Lit2Var(iLit1);
        int iRes0 = iVar0 < nVars ? Vec_IntEntry(vUsed, iVar0) : Vec_IntEntry(vCopy, iVar0 - nVars);
        int iRes1 = iVar1 < nVars ? Vec_IntEntry(vUsed, iVar1) : Vec_IntEntry(vCopy, iVar1 - nVars);
        if ( iVar0 < iVar1 )
        {
            if ( fHash )
                iLitRes = Gia_ManHashAnd( pNew, Abc_LitNotCond(iRes0, Abc_LitIsCompl(iLit0)), Abc_LitNotCond(iRes1, Abc_LitIsCompl(iLit1)) );
            else
                iLitRes = Gia_ManAppendAnd( pNew, Abc_LitNotCond(iRes0, Abc_LitIsCompl(iLit0)), Abc_LitNotCond(iRes1, Abc_LitIsCompl(iLit1)) );
        }
        else if ( iVar0 > iVar1 )
        {
            assert( !Abc_LitIsCompl(iLit0) );
            assert( !Abc_LitIsCompl(iLit1) );
            if ( fHash )
                iLitRes = Gia_ManHashXor( pNew, Abc_LitNotCond(iRes0, Abc_LitIsCompl(iLit0)), Abc_LitNotCond(iRes1, Abc_LitIsCompl(iLit1)) );
            else
                iLitRes = Gia_ManAppendXor( pNew, Abc_LitNotCond(iRes0, Abc_LitIsCompl(iLit0)), Abc_LitNotCond(iRes1, Abc_LitIsCompl(iLit1)) );
        }
        else assert( 0 );
        Vec_IntPush( vCopy, iLitRes );
    }
    assert( Vec_IntSize(vCopy) == Vec_IntSize(vGates)/2 );
    iLitRes = Vec_IntEntry( vCopy, Vec_IntSize(vGates)/2-1 );
    return iLitRes;
}
Gia_Man_t * Gia_ManConstructFromGates( Vec_Wec_t * vFuncs, int nDivs )
{
    Vec_Int_t * vGates; int i, k, iLit;
    Vec_Int_t * vCopy = Vec_IntAlloc( 100 );
    Vec_Int_t * vUsed = Vec_IntStartFull( nDivs );
    Gia_Man_t * pNew = Gia_ManStart( 100 );
    pNew->pName = Abc_UtilStrsav( "resub" );
    Vec_WecForEachLevel( vFuncs, vGates, i )
    {
        assert( Vec_IntSize(vGates) % 2 == 1 );
        Vec_IntForEachEntry( vGates, iLit, k )
        {
            int iVar = Abc_Lit2Var(iLit);
            if ( iVar > 0 && iVar < nDivs && Vec_IntEntry(vUsed, iVar) == -1 )
                Vec_IntWriteEntry( vUsed, iVar, Gia_ManAppendCi(pNew) );
        }
    }
    Vec_WecForEachLevel( vFuncs, vGates, i )
    {
        int iLitRes, iTopLit = Vec_IntEntryLast( vGates );
        if ( Abc_Lit2Var(iTopLit) == 0 )
            iLitRes = 0;
        else if ( Abc_Lit2Var(iTopLit) < nDivs )
            iLitRes = Vec_IntEntry( vUsed, Abc_Lit2Var(iTopLit) );
        else
            iLitRes = Gia_ManConstructFromMap( pNew, vGates, nDivs, vUsed, vCopy, 0 );
        Gia_ManAppendCo( pNew, Abc_LitNotCond( iLitRes, Abc_LitIsCompl(iTopLit) ) );
    }
    Vec_IntFree( vCopy );
    Vec_IntFree( vUsed );
    return pNew;
}
Gia_Man_t * Gia_ManConstructFromGates2( Vec_Wec_t * vFuncs, Vec_Wec_t * vDivs, int nObjs, Vec_Int_t ** pvSupp )
{
    Vec_Int_t * vGates; int i, k, iVar, iLit;
    Vec_Int_t * vSupp  = Vec_IntAlloc( 100 );
    Vec_Int_t * vCopy  = Vec_IntAlloc( 100 );
    Vec_Wec_t * vUseds = Vec_WecStart( Vec_WecSize(vDivs) );
    Vec_Int_t * vMap   = Vec_IntStartFull( nObjs );
    Gia_Man_t * pNew   = Gia_ManStart( 100 );
    pNew->pName = Abc_UtilStrsav( "resub" );
    assert( Vec_WecSize(vFuncs) == Vec_WecSize(vDivs) );
    Vec_WecForEachLevel( vFuncs, vGates, i )
    {
        Vec_Int_t * vDiv = Vec_WecEntry( vDivs, i );
        assert( Vec_IntSize(vGates) % 2 == 1 );
        Vec_IntForEachEntry( vGates, iLit, k )
        {
            int iVar = Abc_Lit2Var(iLit);
            if ( iVar > 0 && iVar < Vec_IntSize(vDiv) && Vec_IntEntry(vMap, Vec_IntEntry(vDiv, iVar)) == -1 )
                Vec_IntWriteEntry( vMap, Vec_IntPushReturn(vSupp, Vec_IntEntry(vDiv, iVar)), 0 );
        }
    }
    Vec_IntSort( vSupp, 0 );
    Vec_IntForEachEntry( vSupp, iVar, k )
        Vec_IntWriteEntry( vMap, iVar, Gia_ManAppendCi(pNew) );
    Vec_WecForEachLevel( vFuncs, vGates, i )
    {
        Vec_Int_t * vDiv  = Vec_WecEntry( vDivs, i );
        Vec_Int_t * vUsed = Vec_WecEntry( vUseds, i );
        Vec_IntFill( vUsed, Vec_IntSize(vDiv), -1 );
        Vec_IntForEachEntry( vGates, iLit, k )
        {
            int iVar = Abc_Lit2Var(iLit);
            if ( iVar > 0 && iVar < Vec_IntSize(vDiv) )
            {
                assert( Vec_IntEntry(vMap, Vec_IntEntry(vDiv, iVar)) > 0 );
                Vec_IntWriteEntry( vUsed, iVar, Vec_IntEntry(vMap, Vec_IntEntry(vDiv, iVar)) );
            }
        }
    }
    Vec_WecForEachLevel( vFuncs, vGates, i )
    {
        Vec_Int_t * vDiv  = Vec_WecEntry( vDivs, i );
        Vec_Int_t * vUsed = Vec_WecEntry( vUseds, i );
        int iLitRes, iTopLit = Vec_IntEntryLast( vGates );
        if ( Abc_Lit2Var(iTopLit) == 0 )
            iLitRes = 0;
        else if ( Abc_Lit2Var(iTopLit) < Vec_IntSize(vDiv) )
            iLitRes = Vec_IntEntry( vUsed, Abc_Lit2Var(iTopLit) );
        else
            iLitRes = Gia_ManConstructFromMap( pNew, vGates, Vec_IntSize(vDiv), vUsed, vCopy, 0 );
        Gia_ManAppendCo( pNew, Abc_LitNotCond( iLitRes, Abc_LitIsCompl(iTopLit) ) );
    }
    Vec_IntFree( vMap );
    Vec_IntFree( vCopy );
    Vec_WecFree( vUseds );
    if ( pvSupp )
        *pvSupp = vSupp;
    else
        Vec_IntFree( vSupp );
    return pNew;
}
Vec_Int_t * Gia_ManToGates( Gia_Man_t * p )
{
    Vec_Int_t * vRes = Vec_IntAlloc( 2*Gia_ManAndNum(p) + 1 );
    Gia_Obj_t * pRoot = Gia_ManCo( p, 0 );
    int iRoot = Gia_ObjFaninId0p(p, pRoot) - 1;
    int nVars = Gia_ManCiNum(p);
    assert( Gia_ManCoNum(p) == 1 );
    if ( iRoot == -1 )
        Vec_IntPush( vRes, Gia_ObjFaninC0(pRoot) );
    else if ( iRoot < nVars )
        Vec_IntPush( vRes, 4 + Abc_Var2Lit(iRoot, Gia_ObjFaninC0(pRoot)) );
    else
    {
        Gia_Obj_t * pObj, * pLast = NULL; int i;
        Gia_ManForEachCi( p, pObj, i )
            assert( Gia_ObjId(p, pObj) == i+1 );
        Gia_ManForEachAnd( p, pObj, i )
        {
            int iLit0 = Abc_Var2Lit( Gia_ObjFaninId0(pObj, i) - 1, Gia_ObjFaninC0(pObj) );
            int iLit1 = Abc_Var2Lit( Gia_ObjFaninId1(pObj, i) - 1, Gia_ObjFaninC1(pObj) );
            if ( iLit0 > iLit1 )
                iLit0 ^= iLit1, iLit1 ^= iLit0, iLit0 ^= iLit1;
            Vec_IntPushTwo( vRes, 4 + iLit0, 4 + iLit1 );
            pLast = pObj;
        }
        assert( pLast == Gia_ObjFanin0(pRoot) );
        Vec_IntPush( vRes, 4 + Abc_Var2Lit(iRoot, Gia_ObjFaninC0(pRoot)) );
    }
    assert( Vec_IntSize(vRes) == 2*Gia_ManAndNum(p) + 1 );
    return vRes;
}

/**Function*************************************************************

  Synopsis    [Construct AIG manager from gates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManInsertOrder_rec( Gia_Man_t * p, int iObj, Vec_Int_t * vObjs, Vec_Wec_t * vFuncs, Vec_Int_t * vNodes )
{
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    if ( iObj == 0 )
        return;
    if ( pObj->fPhase )
    {
        int nVars = Gia_ManObjNum(p);
        int k, iLit, Index = Vec_IntFind( vObjs, iObj );
        Vec_Int_t * vGates = Vec_WecEntry( vFuncs, Index );
        assert( Gia_ObjIsCo(pObj) || Gia_ObjIsAnd(pObj) );
        Vec_IntForEachEntry( vGates, iLit, k )
            if ( Abc_Lit2Var(iLit) < nVars )
                Gia_ManInsertOrder_rec( p, Abc_Lit2Var(iLit), vObjs, vFuncs, vNodes );
    }
    else if ( Gia_ObjIsCo(pObj) )
        Gia_ManInsertOrder_rec( p, Gia_ObjFaninId0p(p, pObj), vObjs, vFuncs, vNodes );
    else if ( Gia_ObjIsAnd(pObj) )
    {
        Gia_ManInsertOrder_rec( p, Gia_ObjFaninId0p(p, pObj), vObjs, vFuncs, vNodes );
        Gia_ManInsertOrder_rec( p, Gia_ObjFaninId1p(p, pObj), vObjs, vFuncs, vNodes );
    }
    else assert( Gia_ObjIsCi(pObj) );
    if ( !Gia_ObjIsCi(pObj) )
        Vec_IntPush( vNodes, iObj );
}
Vec_Int_t * Gia_ManInsertOrder( Gia_Man_t * p, Vec_Int_t * vObjs, Vec_Wec_t * vFuncs )
{
    int i, Id;
    Vec_Int_t * vNodes = Vec_IntAlloc( Gia_ManObjNum(p) );
    Gia_ManForEachCoId( p, Id, i )
        Gia_ManInsertOrder_rec( p, Id, vObjs, vFuncs, vNodes );
    return vNodes;
}
Gia_Man_t * Gia_ManInsertFromGates( Gia_Man_t * p, Vec_Int_t * vObjs, Vec_Wec_t * vFuncs )
{
    Gia_Man_t * pNew, * pTemp; 
    Gia_Obj_t * pObj; 
    int i, nVars = Gia_ManObjNum(p);
    Vec_Int_t * vUsed = Vec_IntStartFull( nVars );
    Vec_Int_t * vNodes, * vCopy = Vec_IntAlloc(100);
    Gia_ManForEachObjVec( vObjs, p, pObj, i )
        pObj->fPhase = 1;
    vNodes = Gia_ManInsertOrder( p, vObjs, vFuncs );
    pNew = Gia_ManStart( Gia_ManObjNum(p) + 1000 );
    Gia_ManHashStart( pNew );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachObjVec( vNodes, p, pObj, i )
        if ( !pObj->fPhase )
        {
            if ( Gia_ObjIsCo(pObj) )
                pObj->Value = Gia_ObjFanin0Copy(pObj);
            else if ( Gia_ObjIsAnd(pObj) )            
                pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
            else assert( 0 );
        }
        else
        {
            int k, iLit, Index = Vec_IntFind( vObjs, Gia_ObjId(p, pObj) );
            Vec_Int_t * vGates = Vec_WecEntry( vFuncs, Index );
            int iLitRes, iTopLit = Vec_IntEntryLast( vGates );
            if ( Abc_Lit2Var(iTopLit) == 0 )
                iLitRes = 0;
            else if ( Abc_Lit2Var(iTopLit) < nVars )
                iLitRes = Gia_ManObj(p, Abc_Lit2Var(iTopLit))->Value;
            else
            {
                Vec_IntForEachEntry( vGates, iLit, k )
                    Vec_IntWriteEntry( vUsed, Abc_Lit2Var(iLit), Gia_ManObj(p, Abc_Lit2Var(iLit))->Value );
                iLitRes = Gia_ManConstructFromMap( pNew, vGates, nVars, vUsed, vCopy, 1 );
                Vec_IntForEachEntry( vGates, iLit, k )
                    Vec_IntWriteEntry( vUsed, Abc_Lit2Var(iLit), -1 );
            }
            pObj->Value = Abc_LitNotCond( iLitRes, Abc_LitIsCompl(iTopLit) );
        }
    Gia_ManForEachCo( p, pObj, i )
        Gia_ManAppendCo( pNew, pObj->Value );
    Gia_ManForEachObjVec( vObjs, p, pObj, i )
        pObj->fPhase = 0;
    Gia_ManHashStop( pNew );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    Vec_IntFree( vNodes );
    Vec_IntFree( vUsed );
    Vec_IntFree( vCopy );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Perform resubstitution.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_ManFindFirstCommonLit( Vec_Int_t * vArr1, Vec_Int_t * vArr2, int fVerbose )
{
    int * pBeg1 = vArr1->pArray;
    int * pBeg2 = vArr2->pArray;
    int * pEnd1 = vArr1->pArray + vArr1->nSize;
    int * pEnd2 = vArr2->pArray + vArr2->nSize;
    int * pStart1 = vArr1->pArray;
    int * pStart2 = vArr2->pArray;
    int nRemoved = 0;
    while ( pBeg1 < pEnd1 && pBeg2 < pEnd2 )
    {
        if ( Abc_Lit2Var(*pBeg1) == Abc_Lit2Var(*pBeg2) )
        { 
            if ( *pBeg1 != *pBeg2 ) 
                return *pBeg1; 
            else
                pBeg1++, pBeg2++;
            nRemoved++;
        }
        else if ( *pBeg1 < *pBeg2 )
            *pStart1++ = *pBeg1++;
        else 
            *pStart2++ = *pBeg2++;
    }
    while ( pBeg1 < pEnd1 )
        *pStart1++ = *pBeg1++;
    while ( pBeg2 < pEnd2 )
        *pStart2++ = *pBeg2++;
    Vec_IntShrink( vArr1, pStart1 - vArr1->pArray );
    Vec_IntShrink( vArr2, pStart2 - vArr2->pArray );
    //if ( fVerbose ) printf( "Removed %d duplicated entries.  Array1 = %d.  Array2 = %d.\n", nRemoved, Vec_IntSize(vArr1), Vec_IntSize(vArr2) );
    return -1;
}

void Gia_ManFindOneUnateInt( word * pOff, word * pOn, Vec_Ptr_t * vDivs, int nWords, Vec_Int_t * vUnateLits, Vec_Int_t * vNotUnateVars )
{
    word * pDiv; int i;
    Vec_IntClear( vUnateLits );
    Vec_IntClear( vNotUnateVars );
    Vec_PtrForEachEntryStart( word *, vDivs, pDiv, i, 2 )
        if ( !Abc_TtIntersectOne( pOff, 0, pDiv, 0, nWords ) )
            Vec_IntPush( vUnateLits, Abc_Var2Lit(i, 0) );
        else if ( !Abc_TtIntersectOne( pOff, 0, pDiv, 1, nWords ) )
            Vec_IntPush( vUnateLits, Abc_Var2Lit(i, 1) );
        else
            Vec_IntPush( vNotUnateVars, i );
}
int Gia_ManFindOneUnate( word * pSets[2], Vec_Ptr_t * vDivs, int nWords, Vec_Int_t * vUnateLits[2], Vec_Int_t * vNotUnateVars[2], int fVerbose )
{
    int n;
    if ( fVerbose ) printf( "  " );
    for ( n = 0; n < 2; n++ )
    {
        Gia_ManFindOneUnateInt( pSets[n], pSets[!n], vDivs, nWords, vUnateLits[n], vNotUnateVars[n] );
        if ( fVerbose ) printf( "U%d =%4d ", n, Vec_IntSize(vUnateLits[n]) );
    }
    return Gia_ManFindFirstCommonLit( vUnateLits[0], vUnateLits[1], fVerbose );
}

static inline int Gia_ManDivCover( word * pOff, word * pOn, word * pDivA, int ComplA, word * pDivB, int ComplB, int nWords )
{
    //assert( !Abc_TtIntersectOne(pOff, 0, pDivA, ComplA, nWords) );
    //assert( !Abc_TtIntersectOne(pOff, 0, pDivB, ComplB, nWords) );
    return !Abc_TtIntersectTwo( pOn, 0, pDivA, !ComplA, pDivB, !ComplB, nWords );
}
int Gia_ManFindTwoUnateInt( word * pOff, word * pOn, Vec_Ptr_t * vDivs, int nWords, Vec_Int_t * vUnateLits, Vec_Int_t * vUnateLitsW, int * pnPairs )
{
    int i, k, iDiv0_, iDiv1_, Cover0, Cover1;
    int nTotal = Abc_TtCountOnesVec( pOn, nWords );
    (*pnPairs) = 0;
    Vec_IntForEachEntryTwo( vUnateLits, vUnateLitsW, iDiv0_, Cover0, i )
    {
        if ( 2*Cover0 < nTotal )
            break;
        Vec_IntForEachEntryTwoStart( vUnateLits, vUnateLitsW, iDiv1_, Cover1, k, i+1 )
        {
            int iDiv0 = Abc_MinInt( iDiv0_, iDiv1_ );
            int iDiv1 = Abc_MaxInt( iDiv0_, iDiv1_ );
            word * pDiv0 = (word *)Vec_PtrEntry(vDivs, Abc_Lit2Var(iDiv0));
            word * pDiv1 = (word *)Vec_PtrEntry(vDivs, Abc_Lit2Var(iDiv1));
            if ( Cover0 + Cover1 < nTotal )
                break;
            (*pnPairs)++;
            if ( Gia_ManDivCover(pOff, pOn, pDiv1, Abc_LitIsCompl(iDiv1), pDiv0, Abc_LitIsCompl(iDiv0), nWords) )
                return Abc_Var2Lit((Abc_LitNot(iDiv1) << 15) | Abc_LitNot(iDiv0), 1);
        }
    }
    return -1;
}
int Gia_ManFindTwoUnate( word * pSets[2], Vec_Ptr_t * vDivs, int nWords, Vec_Int_t * vUnateLits[2], Vec_Int_t * vUnateLitsW[2], int fVerbose )
{
    int n, iLit, nPairs;
    if ( fVerbose ) printf( "  " );
    for ( n = 0; n < 2; n++ )
    {
        //int nPairsAll = Vec_IntSize(vUnateLits[n])*(Vec_IntSize(vUnateLits[n])-1)/2;
        iLit = Gia_ManFindTwoUnateInt( pSets[n], pSets[!n], vDivs, nWords, vUnateLits[n], vUnateLitsW[n], &nPairs );
        if ( fVerbose ) printf( "UU%d =%5d ", n, nPairs );
        if ( iLit >= 0 )
            return Abc_LitNotCond(iLit, n);
    }
    return -1;
}

void Gia_ManFindXorInt( word * pOff, word * pOn, Vec_Int_t * vBinate, Vec_Ptr_t * vDivs, int nWords, Vec_Int_t * vUnatePairs )
{
    int i, k, iDiv0_, iDiv1_;
    int Limit2 = Vec_IntSize(vBinate);//Abc_MinInt( Vec_IntSize(vBinate), 100 );
    Vec_IntForEachEntryStop( vBinate, iDiv1_, i, Limit2 )
    Vec_IntForEachEntryStop( vBinate, iDiv0_, k, i )
    {
        int iDiv0 = Abc_MinInt( iDiv0_, iDiv1_ );
        int iDiv1 = Abc_MaxInt( iDiv0_, iDiv1_ );
        word * pDiv0 = (word *)Vec_PtrEntry(vDivs, iDiv0);
        word * pDiv1 = (word *)Vec_PtrEntry(vDivs, iDiv1);
        if ( !Abc_TtIntersectXor( pOff, 0, pDiv0, pDiv1, 0, nWords ) )
            Vec_IntPush( vUnatePairs, Abc_Var2Lit((Abc_Var2Lit(iDiv0, 0) << 15) | Abc_Var2Lit(iDiv1, 0), 0) );
        else if ( !Abc_TtIntersectXor( pOff, 0, pDiv0, pDiv1, 1, nWords ) )
            Vec_IntPush( vUnatePairs, Abc_Var2Lit((Abc_Var2Lit(iDiv0, 0) << 15) | Abc_Var2Lit(iDiv1, 0), 1) );
    }
}
int Gia_ManFindXor( word * pSets[2], Vec_Ptr_t * vDivs, int nWords, Vec_Int_t * vBinateVars, Vec_Int_t * vUnatePairs[2], int fVerbose )
{
    int n;
    if ( fVerbose ) printf( "  " );
    for ( n = 0; n < 2; n++ )
    {
        Vec_IntClear( vUnatePairs[n] );
        Gia_ManFindXorInt( pSets[n], pSets[!n], vBinateVars, vDivs, nWords, vUnatePairs[n] );
        if ( fVerbose ) printf( "UX%d =%5d ", n, Vec_IntSize(vUnatePairs[n]) );
    }
    return Gia_ManFindFirstCommonLit( vUnatePairs[0], vUnatePairs[1], fVerbose );
}

void Gia_ManFindUnatePairsInt( word * pOff, word * pOn, Vec_Int_t * vBinate, Vec_Ptr_t * vDivs, int nWords, Vec_Int_t * vUnatePairs )
{
    int n, i, k, iDiv0_, iDiv1_;
    int Limit2 = Vec_IntSize(vBinate);//Abc_MinInt( Vec_IntSize(vBinate), 100 );
    Vec_IntForEachEntryStop( vBinate, iDiv1_, i, Limit2 )
    Vec_IntForEachEntryStop( vBinate, iDiv0_, k, i )
    {
        int iDiv0 = Abc_MinInt( iDiv0_, iDiv1_ );
        int iDiv1 = Abc_MaxInt( iDiv0_, iDiv1_ );
        word * pDiv0 = (word *)Vec_PtrEntry(vDivs, iDiv0);
        word * pDiv1 = (word *)Vec_PtrEntry(vDivs, iDiv1);
        for ( n = 0; n < 4; n++ )
        {
            int iLit0 = Abc_Var2Lit( iDiv0, n&1 );
            int iLit1 = Abc_Var2Lit( iDiv1, n>>1 );
            //if ( !Abc_TtIntersectTwo( pOff, 0, pDiv1, n>>1, pDiv0, n&1, nWords ) )
            if ( !Abc_TtIntersectTwo( pOff, 0, pDiv1, n>>1, pDiv0, n&1, nWords ) && Abc_TtIntersectTwo( pOn, 0, pDiv1, n>>1, pDiv0, n&1, nWords ) )
                Vec_IntPush( vUnatePairs, Abc_Var2Lit((iLit1 << 15) | iLit0, 0) );
        }
    }
}
void Gia_ManFindUnatePairs( word * pSets[2], Vec_Ptr_t * vDivs, int nWords, Vec_Int_t * vBinateVars, Vec_Int_t * vUnatePairs[2], int fVerbose )
{
    int n, RetValue;
    if ( fVerbose ) printf( "  " );
    for ( n = 0; n < 2; n++ )
    {
        int nBefore = Vec_IntSize(vUnatePairs[n]);
        Gia_ManFindUnatePairsInt( pSets[n], pSets[!n], vBinateVars, vDivs, nWords, vUnatePairs[n] );
        if ( fVerbose ) printf( "UP%d =%5d ", n, Vec_IntSize(vUnatePairs[n])-nBefore );
    }
    RetValue = Gia_ManFindFirstCommonLit( vUnatePairs[0], vUnatePairs[1], fVerbose );
    assert( RetValue == -1 );
}

void Gia_ManDeriveDivPair( int iDiv, Vec_Ptr_t * vDivs, int nWords, word * pRes )
{
    int fComp = Abc_LitIsCompl(iDiv);
    int iDiv0 = Abc_Lit2Var(iDiv) & 0x7FFF;
    int iDiv1 = Abc_Lit2Var(iDiv) >> 15;
    word * pDiv0 = (word *)Vec_PtrEntry(vDivs, Abc_Lit2Var(iDiv0));
    word * pDiv1 = (word *)Vec_PtrEntry(vDivs, Abc_Lit2Var(iDiv1));
    if ( iDiv0 < iDiv1 )
    {
        assert( !fComp );
        Abc_TtAndCompl( pRes, pDiv0, Abc_LitIsCompl(iDiv0), pDiv1, Abc_LitIsCompl(iDiv1), nWords );
    }
    else 
    {
        assert( !Abc_LitIsCompl(iDiv0) );
        assert( !Abc_LitIsCompl(iDiv1) );
        Abc_TtXor( pRes, pDiv0, pDiv1, nWords, 0 );
    }
}
int Gia_ManFindDivGateInt( word * pOff, word * pOn, Vec_Ptr_t * vDivs, int nWords, Vec_Int_t * vUnateLits, Vec_Int_t * vUnatePairs, Vec_Int_t * vUnateLitsW, Vec_Int_t * vUnatePairsW, word * pDivTemp )
{
    int i, k, iDiv0, iDiv1, Cover0, Cover1;
    int nTotal = Abc_TtCountOnesVec( pOn, nWords );
    Vec_IntForEachEntryTwo( vUnateLits, vUnateLitsW, iDiv0, Cover0, i )
    {
        word * pDiv0 = (word *)Vec_PtrEntry(vDivs, Abc_Lit2Var(iDiv0));
        if ( 2*Cover0 < nTotal )
            break;
        Vec_IntForEachEntryTwo( vUnatePairs, vUnatePairsW, iDiv1, Cover1, k )
        {
            int fComp1 = Abc_LitIsCompl(iDiv1);
            if ( Cover0 + Cover1 < nTotal )
                break;
            Gia_ManDeriveDivPair( iDiv1, vDivs, nWords, pDivTemp );
            if ( Gia_ManDivCover(pOff, pOn, pDiv0, Abc_LitIsCompl(iDiv0), pDivTemp, fComp1, nWords) )
                return Abc_Var2Lit((Abc_Var2Lit(k, 1) << 15) | Abc_LitNot(iDiv0), 1);
        }
    }
    return -1;
}
int Gia_ManFindDivGate( word * pSets[2], Vec_Ptr_t * vDivs, int nWords, Vec_Int_t * vUnateLits[2], Vec_Int_t * vUnatePairs[2], Vec_Int_t * vUnateLitsW[2], Vec_Int_t * vUnatePairsW[2], word * pDivTemp )
{
    int n, iLit;
    for ( n = 0; n < 2; n++ )
    {
        iLit = Gia_ManFindDivGateInt( pSets[n], pSets[!n], vDivs, nWords, vUnateLits[n], vUnatePairs[n], vUnateLitsW[n], vUnatePairsW[n], pDivTemp );
        if ( iLit >= 0 )
            return Abc_LitNotCond( iLit, n );
    }
    return -1;
}

int Gia_ManFindGateGateInt( word * pOff, word * pOn, Vec_Ptr_t * vDivs, int nWords, Vec_Int_t * vUnatePairs, Vec_Int_t * vUnatePairsW, word * pDivTempA, word * pDivTempB )
{
    int i, k, iDiv0, iDiv1, Cover0, Cover1;
    int nTotal = Abc_TtCountOnesVec( pOn, nWords );
    Vec_IntForEachEntryTwo( vUnatePairs, vUnatePairsW, iDiv0, Cover0, k )
    {
        int fCompA = Abc_LitIsCompl(iDiv0);
        if ( 2*Cover0 < nTotal )
            break;
        Gia_ManDeriveDivPair( iDiv0, vDivs, nWords, pDivTempA );
        Vec_IntForEachEntryTwoStart( vUnatePairs, vUnatePairsW, iDiv1, Cover1, i, k+1 )
        {
            int fCompB = Abc_LitIsCompl(iDiv1);
            if ( Cover0 + Cover1 < nTotal )
                break;
            Gia_ManDeriveDivPair( iDiv1, vDivs, nWords, pDivTempB );
            if ( Gia_ManDivCover(pOff, pOn, pDivTempA, fCompA, pDivTempB, fCompB, nWords) )
                return Abc_Var2Lit((Abc_Var2Lit(i, 1) << 15) | Abc_Var2Lit(k, 1), 1);
        }
    }
    return -1;
}
int Gia_ManFindGateGate( word * pSets[2], Vec_Ptr_t * vDivs, int nWords, Vec_Int_t * vUnatePairs[2], Vec_Int_t * vUnatePairsW[2], word * pDivTempA, word * pDivTempB )
{
    int n, iLit;
    for ( n = 0; n < 2; n++ )
    {
        iLit = Gia_ManFindGateGateInt( pSets[n], pSets[!n], vDivs, nWords, vUnatePairs[n], vUnatePairsW[n], pDivTempA, pDivTempB );
        if ( iLit >= 0 )
            return Abc_LitNotCond( iLit, n );
    }
    return -1;
}

void Gia_ManSortUnatesInt( word * pOff, word * pOn, Vec_Ptr_t * vDivs, int nWords, Vec_Int_t * vUnateLits, Vec_Int_t * vUnateLitsW, Vec_Wec_t * vSorter )
{
    int i, k, iLit;
    Vec_Int_t * vLevel;
    Vec_WecInit( vSorter, nWords*64 );
    Vec_IntForEachEntry( vUnateLits, iLit, i )
    {
        word * pDiv = (word *)Vec_PtrEntry(vDivs, Abc_Lit2Var(iLit));
        //assert( !Abc_TtIntersectOne( pOff, 0, pDiv, Abc_LitIsCompl(iLit), nWords ) );
        Vec_WecPush( vSorter, Abc_TtCountOnesVecMask(pDiv, pOn, nWords, Abc_LitIsCompl(iLit)), iLit );
    }
    Vec_IntClear( vUnateLits );
    Vec_IntClear( vUnateLitsW );
    Vec_WecForEachLevelReverse( vSorter, vLevel, k )
        Vec_IntForEachEntry( vLevel, iLit, i )
        {
            Vec_IntPush( vUnateLits, iLit );
            Vec_IntPush( vUnateLitsW, k );
        }
    //Vec_IntPrint( Vec_WecEntry(vSorter, 0) );
    Vec_WecClear( vSorter );
}
void Gia_ManSortUnates( word * pSets[2], Vec_Ptr_t * vDivs, int nWords, Vec_Int_t * vUnateLits[2], Vec_Int_t * vUnateLitsW[2], Vec_Wec_t * vSorter )
{
    int n;
    for ( n = 0; n < 2; n++ )
        Gia_ManSortUnatesInt( pSets[n], pSets[!n], vDivs, nWords, vUnateLits[n], vUnateLitsW[n], vSorter );
}

void Gia_ManSortPairsInt( word * pOff, word * pOn, Vec_Ptr_t * vDivs, int nWords, Vec_Int_t * vUnatePairs, Vec_Int_t * vUnatePairsW, Vec_Wec_t * vSorter )
{
    int i, k, iPair;
    Vec_Int_t * vLevel;
    Vec_WecInit( vSorter, nWords*64 );
    Vec_IntForEachEntry( vUnatePairs, iPair, i )
    {
        int fComp = Abc_LitIsCompl(iPair);
        int iLit0 = Abc_Lit2Var(iPair) & 0x7FFF;
        int iLit1 = Abc_Lit2Var(iPair) >> 15;
        word * pDiv0 = (word *)Vec_PtrEntry( vDivs, Abc_Lit2Var(iLit0) );
        word * pDiv1 = (word *)Vec_PtrEntry( vDivs, Abc_Lit2Var(iLit1) );
        if ( iLit0 < iLit1 )
        {
            assert( !fComp );
            //assert( !Abc_TtIntersectTwo( pOff, 0, pDiv0, Abc_LitIsCompl(iLit0), pDiv1, Abc_LitIsCompl(iLit1), nWords ) );
            Vec_WecPush( vSorter, Abc_TtCountOnesVecMask2(pDiv0, pDiv1, Abc_LitIsCompl(iLit0), Abc_LitIsCompl(iLit1), pOn, nWords), iPair );
        }
        else
        {
            assert( !Abc_LitIsCompl(iLit0) );
            assert( !Abc_LitIsCompl(iLit1) );
            //assert( !Abc_TtIntersectXor( pOff, 0, pDiv0, pDiv1, fComp, nWords ) );
            Vec_WecPush( vSorter, Abc_TtCountOnesVecXorMask(pDiv0, pDiv1, fComp, pOn, nWords), iPair );
        }
    }
    Vec_IntClear( vUnatePairs );
    Vec_IntClear( vUnatePairsW );
    Vec_WecForEachLevelReverse( vSorter, vLevel, k )
        Vec_IntForEachEntry( vLevel, iPair, i )
        {
            Vec_IntPush( vUnatePairs, iPair );
            Vec_IntPush( vUnatePairsW, k );
        }
    //Vec_IntPrint( Vec_WecEntry(vSorter, 0) );
    Vec_WecClear( vSorter );

}
void Gia_ManSortPairs( word * pSets[2], Vec_Ptr_t * vDivs, int nWords, Vec_Int_t * vUnateLits[2], Vec_Int_t * vUnateLitsW[2], Vec_Wec_t * vSorter )
{
    int n;
    for ( n = 0; n < 2; n++ )
        Gia_ManSortPairsInt( pSets[n], pSets[!n], vDivs, nWords, vUnateLits[n], vUnateLitsW[n], vSorter );
}

void Gia_ManSortBinate( word * pSets[2], Vec_Ptr_t * vDivs, int nWords, Vec_Int_t * vBinateVars, Vec_Wec_t * vSorter )
{
    Vec_Int_t * vLevel;
    int nMints[2] = { Abc_TtCountOnesVec(pSets[0], nWords), Abc_TtCountOnesVec(pSets[1], nWords) };
    word * pBig = nMints[0] > nMints[1] ? pSets[0] : pSets[1];
    word * pSmo = nMints[0] > nMints[1] ? pSets[1] : pSets[0];
    int Big = Abc_MaxInt( nMints[0], nMints[1] );
    int Smo = Abc_MinInt( nMints[0], nMints[1] );
    int i, k, iDiv, Gain;

    Vec_WecInit( vSorter, nWords*64 );
    Vec_IntForEachEntry( vBinateVars, iDiv, i )
    {
        word * pDiv = (word *)Vec_PtrEntry( vDivs, iDiv );
        int nInter[2] = { Abc_TtCountOnesVecMask(pBig, pDiv, nWords, 0), Abc_TtCountOnesVecMask(pSmo, pDiv, nWords, 0) };
        if ( nInter[0] < Big/2 ) // complement the divisor
        {
            nInter[0] = Big - nInter[0];
            nInter[1] = Smo - nInter[1];
        }
        assert( nInter[0] >= Big/2 );
        Gain = Abc_MaxInt( 0, nInter[0] - Big/2 + Smo/2 - nInter[1] );
        Vec_WecPush( vSorter, Gain, iDiv );
    }

    Vec_IntClear( vBinateVars );
    Vec_WecForEachLevelReverse( vSorter, vLevel, k )
        Vec_IntForEachEntry( vLevel, iDiv, i )
            Vec_IntPush( vBinateVars, iDiv );
    Vec_WecClear( vSorter );

    if ( Vec_IntSize(vBinateVars) > 2000 )
        Vec_IntShrink( vBinateVars, 2000 );
}

/**Function*************************************************************

  Synopsis    [Perform resubstitution.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManResubFindBestBinate( Gia_ResbMan_t * p )
{
    int nMintsAll = Abc_TtCountOnesVec(p->pSets[0], p->nWords) + Abc_TtCountOnesVec(p->pSets[1], p->nWords);
    int i, iDiv, iLitBest = -1, CostBest = -1;
//Vec_IntPrint( p->vBinateVars );
//Dau_DsdPrintFromTruth( p->pSets[0], 6 );
//Dau_DsdPrintFromTruth( p->pSets[1], 6 );
    Vec_IntForEachEntry( p->vBinateVars, iDiv, i )
    {
        word * pDiv = (word *)Vec_PtrEntry(p->vDivs, iDiv);
        int nMints0 = Abc_TtCountOnesVecMask( pDiv, p->pSets[0], p->nWords, 0 );
        int nMints1 = Abc_TtCountOnesVecMask( pDiv, p->pSets[1], p->nWords, 0 );
        if ( CostBest < nMints0 + nMints1 ) 
        {
            CostBest = nMints0 + nMints1;
            iLitBest = Abc_Var2Lit( iDiv, 0 );
        }
        if ( CostBest < nMintsAll - nMints0 - nMints1 ) 
        {
            CostBest = nMintsAll - nMints0 - nMints1;
            iLitBest = Abc_Var2Lit( iDiv, 1 );
        }
    }
    return iLitBest;
}
int Gia_ManResubAddNode( Gia_ResbMan_t * p, int iLit0, int iLit1, int Type )
{
    int iNode = Vec_PtrSize(p->vDivs) + Vec_IntSize(p->vGates)/2;
    int fFlip = (Type == 2) ^ (iLit0 > iLit1);
    int iFan0 = fFlip ? iLit1 : iLit0;
    int iFan1 = fFlip ? iLit0 : iLit1;
    assert( iLit0 != iLit1 );
    if ( Type == 2 )
        assert( iFan0 > iFan1 );
    else
        assert( iFan0 < iFan1 );
    Vec_IntPushTwo( p->vGates, Abc_LitNotCond(iFan0, Type==1), Abc_LitNotCond(iFan1, Type==1) );
    return Abc_Var2Lit( iNode, Type==1 );
}
int Gia_ManResubPerformMux_rec( Gia_ResbMan_t * p, int nLimit, int Depth )
{
    extern int Gia_ManResubPerform_rec( Gia_ResbMan_t * p, int nLimit, int Depth );
    int iDivBest, iResLit0, iResLit1, nNodes;
    word * pDiv, * pCopy[2];
    if ( Depth == 0 )
        return -1;
    if ( nLimit < 3 )
        return -1;
    iDivBest = Gia_ManResubFindBestBinate( p );
    if ( iDivBest == -1 )
        return -1;
    pCopy[0] = ABC_CALLOC( word, p->nWords );
    pCopy[1] = ABC_CALLOC( word, p->nWords );
    Abc_TtCopy( pCopy[0], p->pSets[0], p->nWords, 0 );
    Abc_TtCopy( pCopy[1], p->pSets[1], p->nWords, 0 );
    pDiv = (word *)Vec_PtrEntry( p->vDivs, Abc_Lit2Var(iDivBest) );
    Abc_TtAndSharp( p->pSets[0], pCopy[0], pDiv, p->nWords, !Abc_LitIsCompl(iDivBest) );
    Abc_TtAndSharp( p->pSets[1], pCopy[1], pDiv, p->nWords, !Abc_LitIsCompl(iDivBest) );
    nNodes = Vec_IntSize(p->vGates)/2;
    //iResLit0 = Gia_ManResubPerform_rec( p, nLimit-3 );
    iResLit0 = Gia_ManResubPerform_rec( p, nLimit, 0 );
    if ( iResLit0 == -1 )
        iResLit0 = Gia_ManResubPerformMux_rec( p, nLimit, Depth-1 );
    if ( iResLit0 == -1 )
    {
        ABC_FREE( pCopy[0] );
        ABC_FREE( pCopy[1] );
        return -1;
    }
    Abc_TtAndSharp( p->pSets[0], pCopy[0], pDiv, p->nWords,  Abc_LitIsCompl(iDivBest) );
    Abc_TtAndSharp( p->pSets[1], pCopy[1], pDiv, p->nWords,  Abc_LitIsCompl(iDivBest) );
    ABC_FREE( pCopy[0] );
    ABC_FREE( pCopy[1] );
    nNodes = Vec_IntSize(p->vGates)/2 - nNodes;
    if ( nLimit-nNodes < 3 )
        return -1;
    //iResLit1 = Gia_ManResubPerform_rec( p, nLimit-3-nNodes );
    iResLit1 = Gia_ManResubPerform_rec( p, nLimit, 0 );
    if ( iResLit1 == -1 )
        iResLit1 = Gia_ManResubPerformMux_rec( p, nLimit, Depth-1 );
    if ( iResLit1 == -1 )
        return -1;
    else
    {
        int iLit0 = Gia_ManResubAddNode( p, Abc_LitNot(iDivBest), iResLit0, 0 );
        int iLit1 = Gia_ManResubAddNode( p,            iDivBest,  iResLit1, 0 );
        return Gia_ManResubAddNode( p, iLit0, iLit1, 1 );
    }
}

/**Function*************************************************************

  Synopsis    [Perform resubstitution.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManResubPerform_rec( Gia_ResbMan_t * p, int nLimit, int Depth )
{
    int TopOneW[2] = {0}, TopTwoW[2] = {0}, Max1, Max2, iResLit, nVars = Vec_PtrSize(p->vDivs);
    if ( p->fVerbose )
    {
        int nMints[2] = { Abc_TtCountOnesVec(p->pSets[0], p->nWords), Abc_TtCountOnesVec(p->pSets[1], p->nWords) };
        printf( "      " ); 
        printf( "ISF: " ); 
        printf( "0=%5d (%5.2f %%) ",  nMints[0], 100.0*nMints[0]/(64*p->nWords) );
        printf( "1=%5d (%5.2f %%)  ", nMints[1], 100.0*nMints[1]/(64*p->nWords) );
    }
    if ( Abc_TtIsConst0( p->pSets[1], p->nWords ) )
        return 0;
    if ( Abc_TtIsConst0( p->pSets[0], p->nWords ) )
        return 1;
    iResLit = Gia_ManFindOneUnate( p->pSets, p->vDivs, p->nWords, p->vUnateLits, p->vNotUnateVars, p->fVerbose );
    if ( iResLit >= 0 ) // buffer
        return iResLit;
    if ( nLimit == 0 )
        return -1;
    Gia_ManSortUnates( p->pSets, p->vDivs, p->nWords, p->vUnateLits, p->vUnateLitsW, p->vSorter );
    iResLit = Gia_ManFindTwoUnate( p->pSets, p->vDivs, p->nWords, p->vUnateLits, p->vUnateLitsW, p->fVerbose );
    if ( iResLit >= 0 ) // and
    {
        int iNode = nVars + Vec_IntSize(p->vGates)/2;
        int fComp = Abc_LitIsCompl(iResLit);
        int iDiv0 = Abc_Lit2Var(iResLit) & 0x7FFF;
        int iDiv1 = Abc_Lit2Var(iResLit) >> 15;
        assert( iDiv0 < iDiv1 );
        Vec_IntPushTwo( p->vGates, iDiv0, iDiv1 );
        return Abc_Var2Lit( iNode, fComp );
    }
    Vec_IntTwoFindCommon( p->vNotUnateVars[0], p->vNotUnateVars[1], p->vBinateVars );
    if ( Depth )
        return Gia_ManResubPerformMux_rec( p, nLimit, Depth );
    if ( Vec_IntSize(p->vBinateVars) > p->nDivsMax )
        Vec_IntShrink( p->vBinateVars, p->nDivsMax );
    if ( p->fVerbose ) printf( "  B = %3d", Vec_IntSize(p->vBinateVars) );
    //Gia_ManSortBinate( p->pSets, p->vDivs, p->nWords, p->vBinateVars, p->vSorter );
    if ( p->fUseXor )
    {
        iResLit = Gia_ManFindXor( p->pSets, p->vDivs, p->nWords, p->vBinateVars, p->vUnatePairs, p->fVerbose );
        if ( iResLit >= 0 ) // xor
        {
            int iNode = nVars + Vec_IntSize(p->vGates)/2;
            int fComp = Abc_LitIsCompl(iResLit);
            int iDiv0 = Abc_Lit2Var(iResLit) & 0x7FFF;
            int iDiv1 = Abc_Lit2Var(iResLit) >> 15;
            assert( !Abc_LitIsCompl(iDiv0) );
            assert( !Abc_LitIsCompl(iDiv1) );
            assert( iDiv0 > iDiv1 );
            Vec_IntPushTwo( p->vGates, iDiv0, iDiv1 );
            return Abc_Var2Lit( iNode, fComp );
        }
    }
    if ( nLimit == 1 )
        return -1;
    Gia_ManFindUnatePairs( p->pSets, p->vDivs, p->nWords, p->vBinateVars, p->vUnatePairs, p->fVerbose );
    Gia_ManSortPairs( p->pSets, p->vDivs, p->nWords, p->vUnatePairs, p->vUnatePairsW, p->vSorter );
    iResLit = Gia_ManFindDivGate( p->pSets, p->vDivs, p->nWords, p->vUnateLits, p->vUnatePairs, p->vUnateLitsW, p->vUnatePairsW, p->pDivA );
    if ( iResLit >= 0 ) // and(div,pair)
    {
        int iNode  = nVars + Vec_IntSize(p->vGates)/2;

        int fComp  = Abc_LitIsCompl(iResLit);
        int iDiv0  = Abc_Lit2Var(iResLit) & 0x7FFF; // div
        int iDiv1  = Abc_Lit2Var(iResLit) >> 15;    // pair

        int Div1   = Vec_IntEntry( p->vUnatePairs[!fComp], Abc_Lit2Var(iDiv1) );
        int fComp1 = Abc_LitIsCompl(Div1) ^ Abc_LitIsCompl(iDiv1);
        int iDiv10 = Abc_Lit2Var(Div1) & 0x7FFF;
        int iDiv11 = Abc_Lit2Var(Div1) >> 15;   

        Vec_IntPushTwo( p->vGates, iDiv10, iDiv11 );
        Vec_IntPushTwo( p->vGates, iDiv0, Abc_Var2Lit(iNode, fComp1) );
        return Abc_Var2Lit( iNode+1, fComp );
    }
//    if ( nLimit == 2 )
//        return -1;
    if ( nLimit >= 3 )
    {
        iResLit = Gia_ManFindGateGate( p->pSets, p->vDivs, p->nWords, p->vUnatePairs, p->vUnatePairsW, p->pDivA, p->pDivB );
        if ( iResLit >= 0 ) // and(pair,pair)
        {
            int iNode  = nVars + Vec_IntSize(p->vGates)/2;

            int fComp  = Abc_LitIsCompl(iResLit);
            int iDiv0  = Abc_Lit2Var(iResLit) & 0x7FFF; // pair
            int iDiv1  = Abc_Lit2Var(iResLit) >> 15;    // pair

            int Div0   = Vec_IntEntry( p->vUnatePairs[!fComp], Abc_Lit2Var(iDiv0) );
            int fComp0 = Abc_LitIsCompl(Div0) ^ Abc_LitIsCompl(iDiv0);
            int iDiv00 = Abc_Lit2Var(Div0) & 0x7FFF;
            int iDiv01 = Abc_Lit2Var(Div0) >> 15;   
        
            int Div1   = Vec_IntEntry( p->vUnatePairs[!fComp], Abc_Lit2Var(iDiv1) );
            int fComp1 = Abc_LitIsCompl(Div1) ^ Abc_LitIsCompl(iDiv1);
            int iDiv10 = Abc_Lit2Var(Div1) & 0x7FFF;
            int iDiv11 = Abc_Lit2Var(Div1) >> 15;   
        
            Vec_IntPushTwo( p->vGates, iDiv00, iDiv01 );
            Vec_IntPushTwo( p->vGates, iDiv10, iDiv11 );
            Vec_IntPushTwo( p->vGates, Abc_Var2Lit(iNode, fComp0), Abc_Var2Lit(iNode+1, fComp1) );
            return Abc_Var2Lit( iNode+2, fComp );
        }
    }
//    if ( nLimit == 3 )
//        return -1;
    if ( Vec_IntSize(p->vUnateLits[0]) + Vec_IntSize(p->vUnateLits[1]) + Vec_IntSize(p->vUnatePairs[0]) + Vec_IntSize(p->vUnatePairs[1]) == 0 )
        return -1;

    TopOneW[0] = Vec_IntSize(p->vUnateLitsW[0]) ? Vec_IntEntry(p->vUnateLitsW[0], 0) : 0;
    TopOneW[1] = Vec_IntSize(p->vUnateLitsW[1]) ? Vec_IntEntry(p->vUnateLitsW[1], 0) : 0;

    TopTwoW[0] = Vec_IntSize(p->vUnatePairsW[0]) ? Vec_IntEntry(p->vUnatePairsW[0], 0) : 0;
    TopTwoW[1] = Vec_IntSize(p->vUnatePairsW[1]) ? Vec_IntEntry(p->vUnatePairsW[1], 0) : 0;

    Max1 = Abc_MaxInt(TopOneW[0], TopOneW[1]);
    Max2 = Abc_MaxInt(TopTwoW[0], TopTwoW[1]);
    if ( Abc_MaxInt(Max1, Max2) == 0 )
        return -1;

    if ( Max1 > Max2/2 )
    {
        if ( nLimit >= 2 && (Max1 == TopOneW[0] || Max1 == TopOneW[1]) )
        {
            int fUseOr  = Max1 == TopOneW[0];
            int iDiv    = Vec_IntEntry( p->vUnateLits[!fUseOr], 0 );
            int fComp   = Abc_LitIsCompl(iDiv);
            word * pDiv = (word *)Vec_PtrEntry( p->vDivs, Abc_Lit2Var(iDiv) );
            Abc_TtAndSharp( p->pSets[fUseOr], p->pSets[fUseOr], pDiv, p->nWords, !fComp );
            if ( p->fVerbose )
                printf( "\n" ); 
            iResLit = Gia_ManResubPerform_rec( p, nLimit-1, Depth );
            if ( iResLit >= 0 ) 
            {
                int iNode = nVars + Vec_IntSize(p->vGates)/2;
                if ( iDiv < iResLit )
                    Vec_IntPushTwo( p->vGates, Abc_LitNot(iDiv), Abc_LitNotCond(iResLit, fUseOr) );
                else
                    Vec_IntPushTwo( p->vGates, Abc_LitNotCond(iResLit, fUseOr), Abc_LitNot(iDiv) );
                return Abc_Var2Lit( iNode, fUseOr );
            }
        }
        if ( Max2 == 0 )
            return -1;
/*
        if ( Max2 == TopTwoW[0] || Max2 == TopTwoW[1] )
        {
            int fUseOr  = Max2 == TopTwoW[0];
            int iDiv    = Vec_IntEntry( p->vUnatePairs[!fUseOr], 0 );
            int fComp   = Abc_LitIsCompl(iDiv);
            Gia_ManDeriveDivPair( iDiv, p->vDivs, p->nWords, p->pDivA );
            Abc_TtAndSharp( p->pSets[fUseOr], p->pSets[fUseOr], p->pDivA, p->nWords, !fComp );
            if ( p->fVerbose )
                printf( "\n      " ); 
            iResLit = Gia_ManResubPerform_rec( p, nLimit-2 );
            if ( iResLit >= 0 ) 
            {
                int iNode = nVars + Vec_IntSize(p->vGates)/2;
                int iDiv0 = Abc_Lit2Var(iDiv) & 0x7FFF;   
                int iDiv1 = Abc_Lit2Var(iDiv) >> 15;      
                Vec_IntPushTwo( p->vGates, iDiv0, iDiv1 );
                Vec_IntPushTwo( p->vGates, Abc_LitNotCond(iResLit, fUseOr), Abc_Var2Lit(iNode, !fComp) );
                return Abc_Var2Lit( iNode+1, fUseOr );
            }
        }
*/
    }
    else
    {
        if ( nLimit >= 3 && (Max2 == TopTwoW[0] || Max2 == TopTwoW[1]) )
        {
            int fUseOr  = Max2 == TopTwoW[0];
            int iDiv    = Vec_IntEntry( p->vUnatePairs[!fUseOr], 0 );
            int fComp   = Abc_LitIsCompl(iDiv);
            Gia_ManDeriveDivPair( iDiv, p->vDivs, p->nWords, p->pDivA );
            Abc_TtAndSharp( p->pSets[fUseOr], p->pSets[fUseOr], p->pDivA, p->nWords, !fComp );
            if ( p->fVerbose )
                printf( "\n" ); 
            iResLit = Gia_ManResubPerform_rec( p, nLimit-2, Depth );
            if ( iResLit >= 0 ) 
            {
                int iNode = nVars + Vec_IntSize(p->vGates)/2;
                int iDiv0 = Abc_Lit2Var(iDiv) & 0x7FFF;   
                int iDiv1 = Abc_Lit2Var(iDiv) >> 15;      
                Vec_IntPushTwo( p->vGates, iDiv0, iDiv1 );
                Vec_IntPushTwo( p->vGates, Abc_LitNotCond(iResLit, fUseOr), Abc_Var2Lit(iNode, !fComp) );
                return Abc_Var2Lit( iNode+1, fUseOr );
            }
        }
        if ( Max1 == 0 )
            return -1;
/*
        if ( Max1 == TopOneW[0] || Max1 == TopOneW[1] )
        {
            int fUseOr  = Max1 == TopOneW[0];
            int iDiv    = Vec_IntEntry( p->vUnateLits[!fUseOr], 0 );
            int fComp   = Abc_LitIsCompl(iDiv);
            word * pDiv = (word *)Vec_PtrEntry( p->vDivs, Abc_Lit2Var(iDiv) );
            Abc_TtAndSharp( p->pSets[fUseOr], p->pSets[fUseOr], pDiv, p->nWords, !fComp );
            if ( p->fVerbose )
                printf( "\n      " ); 
            iResLit = Gia_ManResubPerform_rec( p, nLimit-1 );
            if ( iResLit >= 0 ) 
            {
                int iNode = nVars + Vec_IntSize(p->vGates)/2;
                Vec_IntPushTwo( p->vGates, Abc_LitNot(iDiv), Abc_LitNotCond(iResLit, fUseOr) );
                return Abc_Var2Lit( iNode, fUseOr );
            }
        }
*/
    }
    return -1;
}
void Gia_ManResubPerform( Gia_ResbMan_t * p, Vec_Ptr_t * vDivs, int nWords, int nLimit, int nDivsMax, int iChoice, int fUseXor, int fDebug, int fVerbose, int Depth )
{
    int Res;
    Gia_ResbInit( p, vDivs, nWords, nLimit, nDivsMax, iChoice, fUseXor, fDebug, fVerbose, fVerbose );
    Res = Gia_ManResubPerform_rec( p, nLimit, Depth );
    if ( Res >= 0 ) 
        Vec_IntPush( p->vGates, Res );
    else
        Vec_IntClear( p->vGates );
    if ( fVerbose )
        printf( "\n" );
}
Vec_Int_t * Gia_ManResubOne( Vec_Ptr_t * vDivs, int nWords, int nLimit, int nDivsMax, int iChoice, int fUseXor, int fDebug, int fVerbose, word * pFunc, int Depth )
{
    Vec_Int_t * vRes;
    Gia_ResbMan_t * p = Gia_ResbAlloc( nWords );
    Gia_ManResubPerform( p, vDivs, nWords, nLimit, nDivsMax, iChoice, fUseXor, fDebug, fVerbose, Depth );
    if ( fVerbose )
        Gia_ManResubPrint( p->vGates, Vec_PtrSize(vDivs) );
    //if ( fVerbose )
    //    printf( "\n" );
    if ( !Gia_ManResubVerify(p, pFunc) )
    {
        Gia_ManResubPrint( p->vGates, Vec_PtrSize(vDivs) );
        printf( "Verification FAILED.\n" );
    }
    else if ( fDebug && fVerbose )
        printf( "Verification succeeded." );
    if ( fVerbose )
        printf( "\n" );
    vRes = Vec_IntDup( p->vGates );
    Gia_ResbFree( p );
    return vRes;
}

/**Function*************************************************************

  Synopsis    [Top level.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static Gia_ResbMan_t * s_pResbMan = NULL;

void Abc_ResubPrepareManager( int nWords )
{
    if ( s_pResbMan != NULL )
        Gia_ResbFree( s_pResbMan );
    s_pResbMan = NULL;
    if ( nWords > 0 )
        s_pResbMan = Gia_ResbAlloc( nWords );
}

int Abc_ResubComputeFunction( void ** ppDivs, int nDivs, int nWords, int nLimit, int nDivsMax, int iChoice, int fUseXor, int fDebug, int fVerbose, int ** ppArray )
{
    Vec_Ptr_t Divs = { nDivs, nDivs, ppDivs };
    assert( s_pResbMan != NULL ); // first call Abc_ResubPrepareManager()
    Gia_ManResubPerform( s_pResbMan, &Divs, nWords, nLimit, nDivsMax, iChoice, fUseXor, fDebug, fVerbose==2, 0 );
    if ( fVerbose )
    {
        int nGates = Vec_IntSize(s_pResbMan->vGates)/2;
        if ( nGates )
        {
            printf( "      Gain = %2d  Gates = %2d  __________  F = ", nLimit+1-nGates, nGates );
            Gia_ManResubPrint( s_pResbMan->vGates, nDivs );
            printf( "\n" );
        }
    }
    if ( fDebug )
    {
        if ( !Gia_ManResubVerify(s_pResbMan, NULL) )
        {
            Gia_ManResubPrint( s_pResbMan->vGates, nDivs );
            printf( "Verification FAILED.\n" );
        }
        //else
        //    printf( "Verification succeeded.\n" );
    }
    *ppArray = Vec_IntArray(s_pResbMan->vGates);
    assert( Vec_IntSize(s_pResbMan->vGates)/2 <= nLimit );
    return Vec_IntSize(s_pResbMan->vGates);
}

void Abc_ResubDumpProblem( char * pFileName, void ** ppDivs, int nDivs, int nWords )
{
    Vec_Wrd_t * vSims = Vec_WrdAlloc( nDivs * nWords );
    word ** pDivs = (word **)ppDivs;
    int d, w;
    for ( d = 0; d < nDivs;  d++ )
    for ( w = 0; w < nWords; w++ )
        Vec_WrdPush( vSims, pDivs[d][w] );
    Vec_WrdDumpHex( pFileName, vSims, nWords, 1 );
    Vec_WrdFree( vSims );
}

/**Function*************************************************************

  Synopsis    [Top level.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

extern void Extra_PrintHex( FILE * pFile, unsigned * pTruth, int nVars );
extern void Dau_DsdPrintFromTruth2( word * pTruth, int nVarsInit );

void Gia_ManResubTest3()
{
    int nVars = 4;
    int fVerbose = 1;
    word Divs[6] = { 0, 0, 
        ABC_CONST(0xAAAAAAAAAAAAAAAA),
        ABC_CONST(0xCCCCCCCCCCCCCCCC),
        ABC_CONST(0xF0F0F0F0F0F0F0F0),
        ABC_CONST(0xFF00FF00FF00FF00) 
    };
    Vec_Ptr_t * vDivs = Vec_PtrAlloc( 6 );
    Vec_Int_t * vRes = Vec_IntAlloc( 100 );
    int i, k, ArraySize, * pArray; 
    for ( i = 0; i < 6; i++ )
        Vec_PtrPush( vDivs, Divs+i );
    Abc_ResubPrepareManager( 1 );
    for ( i = 0; i < (1<<(1<<nVars)); i++ ) //if ( i == 0xCA ) 
    {
        word Truth = Abc_Tt6Stretch( i, nVars );
        Divs[0] = ~Truth;
        Divs[1] =  Truth;
        printf( "%3d : ", i );
        Extra_PrintHex( stdout, (unsigned*)&Truth, nVars );
        printf( " " );
        Dau_DsdPrintFromTruth2( &Truth, nVars );
        printf( "           " );

        //Abc_ResubDumpProblem( "temp.resub", (void **)Vec_PtrArray(vDivs), Vec_PtrSize(vDivs), 1 );
        ArraySize = Abc_ResubComputeFunction( (void **)Vec_PtrArray(vDivs), Vec_PtrSize(vDivs), 1, 16, 50, 0, 0, 1, fVerbose, &pArray );
        printf( "\n" );

        Vec_IntClear( vRes );
        for ( k = 0; k < ArraySize; k++ )
            Vec_IntPush( vRes, pArray[k] );

        if ( i == 1000 )
            break;
    }
    Abc_ResubPrepareManager( 0 );
    Vec_IntFree( vRes );
    Vec_PtrFree( vDivs );
}
void Gia_ManResubTest3_()
{
    Gia_ResbMan_t * p = Gia_ResbAlloc( 1 );
    word Divs[6] = { 0, 0, 
        ABC_CONST(0xAAAAAAAAAAAAAAAA),
        ABC_CONST(0xCCCCCCCCCCCCCCCC),
        ABC_CONST(0xF0F0F0F0F0F0F0F0),
        ABC_CONST(0xFF00FF00FF00FF00) 
    };
    Vec_Ptr_t * vDivs = Vec_PtrAlloc( 6 );
    Vec_Int_t * vRes = Vec_IntAlloc( 100 );
    int i; 
    for ( i = 0; i < 6; i++ )
        Vec_PtrPush( vDivs, Divs+i );

    {
        word Truth = (Divs[2] | Divs[3]) & (Divs[4] & Divs[5]);
//        word Truth = (~Divs[2] | Divs[3]) | ~Divs[4];
        Divs[0] = ~Truth;
        Divs[1] =  Truth;
        Extra_PrintHex( stdout, (unsigned*)&Truth, 6 );
        printf( " " );
        Dau_DsdPrintFromTruth2( &Truth, 6 );
        printf( "       " );
        Gia_ManResubPerform( p, vDivs, 1, 100, 0, 50, 1, 1, 0, 0 );
    }
    Gia_ResbFree( p );
    Vec_IntFree( vRes );
    Vec_PtrFree( vDivs );
}

/**Function*************************************************************

  Synopsis    [Top level.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManResubPair( Vec_Wrd_t * vOn, Vec_Wrd_t * vOff, int nWords, int nIns )
{
    Gia_ResbMan_t * p = Gia_ResbAlloc( nWords*2 );
    Vec_Ptr_t * vDivs = Vec_PtrAllocSimInfo( nIns+2, nWords*2 );
    word * pSim; int i;
    Vec_PtrForEachEntry( word *, vDivs, pSim, i )
    {
        if ( i == 0 )
        {
            memset( pSim,        0x00, sizeof(word)*nWords );
            memset( pSim+nWords, 0xFF, sizeof(word)*nWords );
        }
        else if ( i == 1 )
        {
            memset( pSim,        0xFF, sizeof(word)*nWords );
            memset( pSim+nWords, 0x00, sizeof(word)*nWords );
        }
        else
        {
            memmove( pSim,        Vec_WrdEntryP(vOn,  (i-2)*nWords), sizeof(word)*nWords );
            memmove( pSim+nWords, Vec_WrdEntryP(vOff, (i-2)*nWords), sizeof(word)*nWords );
        }
    }
    Gia_ManResubPerform( p, vDivs, nWords*2, 100, 0, 50, 1, 1, 0, 0 );
    Gia_ManResubPrint( p->vGates, Vec_PtrSize(vDivs) );
    printf( "\n" );
    //Vec_PtrFree( vDivs );
    Gia_ResbFree( p );
}

/**Function*************************************************************

  Synopsis    [Top level.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCheckResub( Vec_Ptr_t * vDivs, int nWords )
{
    //int i, nVars = 6, pVarSet[10] = { 2, 189, 2127, 2125, 177, 178 };
    int i, nVars = 3, pVarSet[10] = { 2, 3, 4 };
    word * pOff = (word *)Vec_PtrEntry( vDivs, 0 );
    word * pOn  = (word *)Vec_PtrEntry( vDivs, 1 );
    Vec_Int_t * vValue = Vec_IntStartFull( 1 << 6 );
    printf( "Verifying resub:\n" );
    for ( i = 0; i < 64*nWords; i++ )
    {
        int v, Mint = 0, Value = Abc_TtGetBit(pOn, i);
        if ( !Abc_TtGetBit(pOff, i) && !Value )
            continue;
        for ( v = 0; v < nVars; v++ )
            if ( Abc_TtGetBit((word *)Vec_PtrEntry(vDivs, pVarSet[v]), i) )
                Mint |= 1 << v;
        if ( Vec_IntEntry(vValue, Mint) == -1 )
            Vec_IntWriteEntry(vValue, Mint, Value);
        else if ( Vec_IntEntry(vValue, Mint) != Value )
            printf( "Mismatch in pattern %d\n", i );
    }
    printf( "Finished verifying resub.\n" );
    Vec_IntFree( vValue );
}
Vec_Ptr_t * Gia_ManDeriveDivs( Vec_Wrd_t * vSims, int nWords )
{
    int i, nDivs = Vec_WrdSize(vSims)/nWords;
    Vec_Ptr_t * vDivs = Vec_PtrAlloc( nDivs );
    for ( i = 0; i < nDivs; i++ )
        Vec_PtrPush( vDivs, Vec_WrdEntryP(vSims, nWords*i) );
    return vDivs;
}
Gia_Man_t * Gia_ManResub2( Gia_Man_t * pGia, int nNodes, int nSupp, int nDivs, int iChoice, int fUseXor, int fVerbose, int fVeryVerbose )
{
    return NULL;
}
Gia_Man_t * Gia_ManResub1( char * pFileName, int nNodes, int nSupp, int nDivs, int iChoice, int fUseXor, int fVerbose, int fVeryVerbose )
{
    int nWords = 0;
    Gia_Man_t * pMan   = NULL;
    Vec_Wrd_t * vSims  = Vec_WrdReadHex( pFileName, &nWords, 1 );
    Vec_Ptr_t * vDivs  = vSims ? Gia_ManDeriveDivs( vSims, nWords ) : NULL;
    Gia_ResbMan_t * p = Gia_ResbAlloc( nWords );
    //Gia_ManCheckResub( vDivs, nWords );
    if ( Vec_PtrSize(vDivs) >= (1<<14) )
    {
        printf( "Reducing all divs from %d to %d.\n", Vec_PtrSize(vDivs), (1<<14)-1 );
        Vec_PtrShrink( vDivs, (1<<14)-1 );
    }
    assert( Vec_PtrSize(vDivs) < (1<<14) );
    Gia_ManResubPerform( p, vDivs, nWords, 100, 50, iChoice, fUseXor, 1, 1, 0 );
    if ( Vec_IntSize(p->vGates) )
    {
        Vec_Wec_t * vGates = Vec_WecStart(1);
        Vec_IntAppend( Vec_WecEntry(vGates, 0), p->vGates );
        pMan = Gia_ManConstructFromGates( vGates, Vec_PtrSize(vDivs) );
        Vec_WecFree( vGates );
    }
    else
        printf( "Decomposition did not succeed.\n" );
    Gia_ResbFree( p );
    Vec_PtrFree( vDivs );
    Vec_WrdFree( vSims );
    return pMan;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManUnivTfo_rec( Gia_Man_t * p, int iObj, Vec_Int_t * vNodes, Vec_Int_t * vPos )
{
    int i, iFan, Count = 1;
    if ( Gia_ObjIsTravIdCurrentId(p, iObj) )
        return 0;
    Gia_ObjSetTravIdCurrentId(p, iObj);
    if ( vNodes && Gia_ObjIsCo(Gia_ManObj(p, iObj)) )
        Vec_IntPush( vNodes, iObj );    
    if ( vPos && Gia_ObjIsCo(Gia_ManObj(p, iObj)) )
        Vec_IntPush( vPos, iObj );
    Gia_ObjForEachFanoutStaticId( p, iObj, iFan, i )
        Count += Gia_ManUnivTfo_rec( p, iFan, vNodes, vPos );
    return Count;
}
int Gia_ManUnivTfo( Gia_Man_t * p, int * pObjs, int nObjs, Vec_Int_t ** pvNodes, Vec_Int_t ** pvPos )
{
    int i, Count = 0;
    if ( pvNodes )
    {
        if ( *pvNodes )
            Vec_IntClear( *pvNodes );
        else
            *pvNodes = Vec_IntAlloc( 100 );
    }
    if ( pvPos )
    {
        if ( *pvPos )
            Vec_IntClear( *pvPos );
        else
            *pvPos = Vec_IntAlloc( 100 );
    }
    Gia_ManIncrementTravId( p );
    for ( i = 0; i < nObjs; i++ )
        Count += Gia_ManUnivTfo_rec( p, pObjs[i], pvNodes ? *pvNodes : NULL, pvPos ? *pvPos : NULL );
    if ( pvNodes )
        Vec_IntSort( *pvNodes, 0 );
    if ( pvPos )
        Vec_IntSort( *pvPos, 0 );
    return Count;
}

/**Function*************************************************************

  Synopsis    [Tuning resub.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManTryResub( Gia_Man_t * p )
{
    int nLimit   =  20;
    int nDivsMax = 200;
    int iChoice  =   0;
    int fUseXor  =   1;
    int fDebug   =   1;
    int fVerbose =   0;
    abctime clk, clkResub = 0, clkStart = Abc_Clock();
    Vec_Ptr_t * vvSims = Vec_PtrAlloc( 100 );
    Vec_Wrd_t * vSims;
    word * pSets[2], * pFunc;
    Gia_Obj_t * pObj, * pObj2; 
    int i, i2, nWords, nNonDec = 0, nTotal = 0;
    assert( Gia_ManCiNum(p) < 16 );
    Vec_WrdFreeP( &p->vSimsPi );
    p->vSimsPi = Vec_WrdStartTruthTables( Gia_ManCiNum(p) );
    nWords = Vec_WrdSize(p->vSimsPi) / Gia_ManCiNum(p);
    //Vec_WrdPrintHex( p->vSimsPi, nWords );
    pSets[0] = ABC_CALLOC( word, nWords );
    pSets[1] = ABC_CALLOC( word, nWords );
    vSims = Gia_ManSimPatSim( p );
    Gia_ManLevelNum(p);
    Gia_ManCreateRefs(p);
    Abc_ResubPrepareManager( nWords );
    Gia_ManStaticFanoutStart( p );
    Gia_ManForEachAnd( p, pObj, i )
    {
        Vec_Int_t vGates;
        int * pArray, nArray, nTfo, iObj = Gia_ObjId(p, pObj);
        int Level = Gia_ObjLevel(p, pObj);
        int nMffc = Gia_NodeMffcSizeMark(p, pObj);
        pFunc = Vec_WrdEntryP( vSims, nWords*iObj );
        Abc_TtCopy( pSets[0], pFunc, nWords, 1 );
        Abc_TtCopy( pSets[1], pFunc, nWords, 0 );
        Vec_PtrClear( vvSims );
        Vec_PtrPushTwo( vvSims, pSets[0], pSets[1] );
        nTfo = Gia_ManUnivTfo( p, &iObj, 1, NULL, NULL );
        Gia_ManForEachCi( p, pObj2, i2 )
            Vec_PtrPush( vvSims, Vec_WrdEntryP(vSims, nWords*Gia_ObjId(p, pObj2)) );
        Gia_ManForEachAnd( p, pObj2, i2 )
            if ( !Gia_ObjIsTravIdCurrent(p, pObj2) && !Gia_ObjIsTravIdPrevious(p, pObj2) && Gia_ObjLevel(p, pObj2) <= Level )
                Vec_PtrPush( vvSims, Vec_WrdEntryP(vSims, nWords*Gia_ObjId(p, pObj2)) );
        if ( fVerbose )
        printf( "%3d : Lev = %2d  Mffc = %2d  Divs = %3d  Tfo = %3d\n", iObj, Level, nMffc, Vec_PtrSize(vvSims)-2, nTfo );
        clk = Abc_Clock();
        nArray = Abc_ResubComputeFunction( (void **)Vec_PtrArray(vvSims), Vec_PtrSize(vvSims), nWords, Abc_MinInt(nMffc-1, nLimit), nDivsMax, iChoice, fUseXor, fDebug, fVerbose, &pArray );
        clkResub += Abc_Clock() - clk;
        vGates.nSize = vGates.nCap = nArray;
        vGates.pArray = pArray;
        assert( nMffc > Vec_IntSize(&vGates)/2 );
        if ( Vec_IntSize(&vGates) > 0 )
            nTotal += nMffc - Vec_IntSize(&vGates)/2;
        nNonDec += Vec_IntSize(&vGates) == 0;
    }
    printf( "Total nodes = %5d.  Non-realizable = %5d.  Gain = %6d.  ", Gia_ManAndNum(p), nNonDec, nTotal );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clkStart );
    Abc_PrintTime( 1, "Pure resub time", clkResub );
    Abc_ResubPrepareManager( 0 );
    Gia_ManStaticFanoutStop( p );
    Vec_PtrFree( vvSims );
    Vec_WrdFree( vSims );
    ABC_FREE( pSets[0] );
    ABC_FREE( pSets[1] );
}


/**Function*************************************************************

  Synopsis    [Deriving a subset.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManDeriveShrink( Vec_Wrd_t * vFuncs, int nWords )
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
void Gia_ManDeriveCounts( Vec_Wrd_t * vFuncs, int nWords, Vec_Int_t * vCounts )
{
    int i, nFuncs = Vec_WrdSize(vFuncs) / nWords / 2;
    assert( 2 * nFuncs * nWords == Vec_WrdSize(vFuncs) );
    Vec_IntClear( vCounts );
    for ( i = 0; i < 2*nFuncs; i++ )
        Vec_IntPush( vCounts, Abc_TtCountOnesVec(Vec_WrdEntryP(vFuncs, i*nWords), nWords) );
}
int Gia_ManDeriveCost( Vec_Wrd_t * vFuncs, int nWords, word * pMask, Vec_Int_t * vCounts )
{
    int i, Res = 0, nFuncs = Vec_WrdSize(vFuncs) / nWords / 2;
    assert( 2 * nFuncs * nWords == Vec_WrdSize(vFuncs) );
    assert( Vec_IntSize(vCounts) * nWords == Vec_WrdSize(vFuncs) );
    for ( i = 0; i < nFuncs; i++ )
    {
        int Total[2] = { Vec_IntEntry(vCounts, 2*i+0), Vec_IntEntry(vCounts, 2*i+1) };
        int This[2]  = { Abc_TtCountOnesVecMask(Vec_WrdEntryP(vFuncs, (2*i+0)*nWords), pMask, nWords, 0),
                         Abc_TtCountOnesVecMask(Vec_WrdEntryP(vFuncs, (2*i+1)*nWords), pMask, nWords, 0) };
        assert( Total[0] >= This[0] && Total[1] >= This[1] );
        Res += This[0] * This[1] + (Total[0] - This[0]) * (Total[1] - This[1]);
    }
    return Res;
}
int Gia_ManDeriveSimpleCost( Vec_Int_t * vCounts )
{
    int i, Ent1, Ent2, Res = 0;
    Vec_IntForEachEntryDouble( vCounts, Ent1, Ent2, i )
        Res += Ent1*Ent2;
    return Res;
}
void Gia_ManDeriveNext( Vec_Wrd_t * vFuncs, int nWords, word * pMask )
{
    int i, iStop = Vec_WrdSize(vFuncs); word Data;
    int nFuncs = Vec_WrdSize(vFuncs) / nWords / 2;
    assert( 2 * nFuncs * nWords == Vec_WrdSize(vFuncs) );
    Vec_WrdForEachEntryStop( vFuncs, Data, i, iStop )
        Vec_WrdPush( vFuncs, Data );
    for ( i = 0; i < nFuncs; i++ )
    {
        word * pFunc0n = Vec_WrdEntryP(vFuncs, (2*i+0)*nWords);
        word * pFunc1n = Vec_WrdEntryP(vFuncs, (2*i+1)*nWords);
        word * pFunc0p = Vec_WrdEntryP(vFuncs, (2*i+0)*nWords + iStop);
        word * pFunc1p = Vec_WrdEntryP(vFuncs, (2*i+1)*nWords + iStop);
        Abc_TtAnd( pFunc0p, pFunc0n, pMask, nWords, 0 );
        Abc_TtAnd( pFunc1p, pFunc1n, pMask, nWords, 0 );
        Abc_TtSharp( pFunc0n, pFunc0n, pMask, nWords );
        Abc_TtSharp( pFunc1n, pFunc1n, pMask, nWords );
    }
}
Vec_Int_t * Gia_ManDeriveSubset( Gia_Man_t * p, Vec_Wrd_t * vFuncs, Vec_Int_t * vObjs, Vec_Wrd_t * vSims, int nWords, int fVerbose )
{
    int i, k, iObj, CostBestPrev, nFuncs = Vec_WrdSize(vFuncs) / nWords;
    Vec_Int_t * vRes    = Vec_IntAlloc( 100 );
    Vec_Int_t * vCounts = Vec_IntAlloc( nFuncs * 2 );
    Vec_Wrd_t * vFSims  = Vec_WrdDup( vFuncs );
    assert( nFuncs * nWords == Vec_WrdSize(vFuncs) );
    assert( Gia_ManObjNum(p) * nWords == Vec_WrdSize(vSims) );
    assert( Vec_IntSize(vObjs) <= Gia_ManCandNum(p) );
    nFuncs = Gia_ManDeriveShrink( vFSims, nWords );
    Gia_ManDeriveCounts( vFSims, nWords, vCounts );
    assert( Vec_IntSize(vCounts) * nWords == Vec_WrdSize(vFSims) );
    CostBestPrev = Gia_ManDeriveSimpleCost( vCounts );
    if ( fVerbose )
    printf( "Processing %d functions and %d objects with cost %d\n", nFuncs, Vec_IntSize(vObjs), CostBestPrev );
    for ( i = 0; nFuncs > 0; i++ )
    {
        int iObjBest = -1, CountThis, Count0 = ABC_INFINITY, CountBest = ABC_INFINITY;
        Vec_IntForEachEntry( vObjs, iObj, k )
        {
            if ( Vec_IntFind(vRes, iObj) >= 0 )
                continue;
            CountThis = Gia_ManDeriveCost( vFSims, nWords, Vec_WrdEntryP(vSims, iObj*nWords), vCounts );
            if ( CountBest > CountThis )
            {
                CountBest = CountThis;
                iObjBest = iObj;
            }
            if ( !k ) Count0 = CountThis;
        }
        if ( Count0 < CostBestPrev )
        {
            CountBest = Count0;
            iObjBest = Vec_IntEntry(vObjs, 0);
        }
        Gia_ManDeriveNext( vFSims, nWords, Vec_WrdEntryP(vSims, iObjBest*nWords) );
        nFuncs = Gia_ManDeriveShrink( vFSims, nWords );
        Gia_ManDeriveCounts( vFSims, nWords, vCounts );
        assert( CountBest == Gia_ManDeriveSimpleCost(vCounts) );
        Vec_IntPush( vRes, iObjBest );
        CostBestPrev = CountBest;
        if ( fVerbose )
        printf( "Iter %2d :  Funcs = %6d.  Object %6d.  Cost %6d.\n", i, nFuncs, iObjBest, CountBest );
    }
    Vec_IntFree( vCounts );
    Vec_WrdFree( vFSims );
    return vRes;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

