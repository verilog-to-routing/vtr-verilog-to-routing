/**CFile****************************************************************

  FileName    [ifDelay.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [FPGA mapping based on priority cuts.]

  Synopsis    [Delay balancing for cut functions.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 21, 2006.]

  Revision    [$Id: ifDelay.c,v 1.00 2006/11/21 00:00:00 alanmi Exp $]

***********************************************************************/

#include "if.h"
#include "ifCount.h"
#include "bool/kit/kit.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define IF_MAX_CUBES 70

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
 
/**Function*************************************************************

  Synopsis    [Computes the SOP delay using balanced AND decomposition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int If_CutMaxCubeSize( Vec_Int_t * vCover, int nVars )
{
    int i, k, Entry, Literal, Count, CountMax = 0;
    Vec_IntForEachEntry( vCover, Entry, i )
    {
        Count = 0;
        for ( k = 0; k < nVars; k++ )
        {
            Literal = (3 & (Entry >> (k << 1)));
            if ( Literal == 1 || Literal == 2 )
                Count++;
        }
        CountMax = Abc_MaxInt( CountMax, Count );
    }
    return CountMax;
}
int If_CutDelaySop( If_Man_t * p, If_Cut_t * pCut )
{
    char * pPerm = If_CutPerm( pCut );
    // delay is calculated using 1+log2(NumFanins)
    static double GateDelays[20] = { 1.00, 1.00, 2.00, 2.58, 3.00, 3.32, 3.58, 3.81, 4.00, 4.17, 4.32, 4.46, 4.58, 4.70, 4.81, 4.91, 5.00, 5.09, 5.17, 5.25 };
    Vec_Int_t * vCover;
    If_Obj_t * pLeaf;
    int i, nLitMax, Delay, DelayMax;
    // mark cut as a user cut
    pCut->fUser = 1;
    if ( pCut->nLeaves == 0 )
        return 0;
    if ( pCut->nLeaves == 1 )
        return (int)If_ObjCutBest(If_CutLeaf(p, pCut, 0))->Delay;
    vCover = Vec_WecEntry( p->vTtIsops[pCut->nLeaves], Abc_Lit2Var(If_CutTruthLit(pCut)) );
    if ( Vec_IntSize(vCover) == 0 )
        return -1;
    // mark the output as complemented
//    vAnds = If_CutDelaySopAnds( p, pCut, vCover, RetValue ^ pCut->fCompl );
    if ( Vec_IntSize(vCover) > p->pPars->nGateSize )
        return -1;
    // set the area cost
    assert( If_CutLeaveNum(pCut) >= 0 && If_CutLeaveNum(pCut) <= 16 );
    // compute the gate delay
    nLitMax = If_CutMaxCubeSize( vCover, If_CutLeaveNum(pCut) );
    if ( Vec_IntSize(vCover) < 2 )
    {
        pCut->Cost = Vec_IntSize(vCover);
        Delay = (int)(GateDelays[If_CutLeaveNum(pCut)] + 0.5);
        DelayMax = 0;
        If_CutForEachLeaf( p, pCut, pLeaf, i )
            DelayMax = Abc_MaxInt( DelayMax, If_ObjCutBest(pLeaf)->Delay + (pPerm[i] = (char)Delay) );
    }
    else
    {
        pCut->Cost = Vec_IntSize(vCover) + 1;
        Delay = (int)(GateDelays[If_CutLeaveNum(pCut)] + GateDelays[nLitMax] + 0.5);
        DelayMax = 0;
        If_CutForEachLeaf( p, pCut, pLeaf, i )
            DelayMax = Abc_MaxInt( DelayMax, If_ObjCutBest(pLeaf)->Delay + (pPerm[i] = (char)Delay) );
    }
    return DelayMax;
}


/**Function*************************************************************

  Synopsis    [Compute pin delays.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_CutSopBalancePinDelaysInt( Vec_Int_t * vCover, int * pTimes, word * pFaninRes, int nSuppAll, word * pRes )
{
    word pPinDelsAnd[IF_MAX_FUNC_LUTSIZE], pPinDelsOr[IF_MAX_CUBES];
    int nCounterAnd, pCounterAnd[IF_MAX_FUNC_LUTSIZE];
    int nCounterOr,  pCounterOr[IF_MAX_CUBES];
    int i, k, Entry, Literal, Delay = 0;
    word ResAnd;
    if ( Vec_IntSize(vCover) > IF_MAX_CUBES )
        return -1;
    nCounterOr = 0;
    Vec_IntForEachEntry( vCover, Entry, i )
    { 
        nCounterAnd = 0;
        for ( k = 0; k < nSuppAll; k++ )
        {
            Literal = 3 & (Entry >> (k << 1));
            if ( Literal == 1 || Literal == 2 ) // neg or pos literal
                Delay = If_LogCounterPinDelays( pCounterAnd, &nCounterAnd, pPinDelsAnd, pTimes[k], pFaninRes[k], nSuppAll, 0 );
            else if ( Literal != 0 ) 
                assert( 0 );
        }
        assert( nCounterAnd > 0 );
        ResAnd = If_LogPinDelaysMulti( pPinDelsAnd, nCounterAnd, nSuppAll, 0 );
        Delay = If_LogCounterPinDelays( pCounterOr, &nCounterOr, pPinDelsOr, Delay, ResAnd, nSuppAll, 0 );
    }
    assert( nCounterOr > 0 );
    *pRes = If_LogPinDelaysMulti( pPinDelsOr, nCounterOr, nSuppAll, 0 );
    return Delay;
}
int If_CutSopBalancePinDelaysIntInt( Vec_Int_t * vCover, int * pTimes, int nSuppAll, char * pPerm )
{
    int i, Delay;
    word Res, FaninRes[IF_MAX_FUNC_LUTSIZE];
    for ( i = 0; i < nSuppAll; i++ )
        FaninRes[i] = If_CutPinDelayInit(i);
    Delay = If_CutSopBalancePinDelaysInt( vCover, pTimes, FaninRes, nSuppAll, &Res );
    If_CutPinDelayTranslate( Res, nSuppAll, pPerm );
    return Delay;
}
int If_CutSopBalancePinDelays( If_Man_t * p, If_Cut_t * pCut, char * pPerm )
{
    if ( pCut->nLeaves == 0 ) // const
        return 0;
    if ( pCut->nLeaves == 1 ) // variable
    {
        pPerm[0] = 0;
        return (int)If_ObjCutBest(If_CutLeaf(p, pCut, 0))->Delay;
    }
    else
    {
        Vec_Int_t * vCover;
        int i, pTimes[IF_MAX_FUNC_LUTSIZE];
        vCover = Vec_WecEntry( p->vTtIsops[pCut->nLeaves], Abc_Lit2Var(If_CutTruthLit(pCut)) );
        if ( Vec_IntSize(vCover) == 0 )
            return -1;
        for ( i = 0; i < If_CutLeaveNum(pCut); i++ )
            pTimes[i] = (int)If_ObjCutBest(If_CutLeaf(p, pCut, i))->Delay; 
        return If_CutSopBalancePinDelaysIntInt( vCover, pTimes, If_CutLeaveNum(pCut), pPerm );
    }
}

/**Function*************************************************************

  Synopsis    [Evaluate delay using SOP balancing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_CutSopBalanceEvalInt( Vec_Int_t * vCover, int * pTimes, int * pFaninLits, Vec_Int_t * vAig, int * piRes, int nSuppAll, int * pArea )
{
    int nCounterAnd, pCounterAnd[IF_MAX_FUNC_LUTSIZE], pFaninLitsAnd[IF_MAX_FUNC_LUTSIZE];
    int nCounterOr,  pCounterOr[IF_MAX_CUBES],  pFaninLitsOr[IF_MAX_CUBES];
    int i, k, Entry, Literal, nLits, Delay = 0, iRes = 0;
    if ( Vec_IntSize(vCover) > IF_MAX_CUBES )
        return -1;
    nCounterOr = 0;
    Vec_IntForEachEntry( vCover, Entry, i )
    { 
        nCounterAnd = nLits = 0;
        for ( k = 0; k < nSuppAll; k++ )
        {
            Literal = 3 & (Entry >> (k << 1));
            if ( Literal == 1 ) // neg literal
                nLits++, Delay = If_LogCounterAddAig( pCounterAnd, &nCounterAnd, pFaninLitsAnd, pTimes[k], vAig ? Abc_LitNot(pFaninLits[k]) : -1, vAig, nSuppAll, 0, 0 );
            else if ( Literal == 2 ) // pos literal
                nLits++, Delay = If_LogCounterAddAig( pCounterAnd, &nCounterAnd, pFaninLitsAnd, pTimes[k], vAig ? pFaninLits[k] : -1, vAig, nSuppAll, 0, 0 );
            else if ( Literal != 0 ) 
                assert( 0 );
        }
        assert( nCounterAnd > 0 );
        assert( nLits > 0 );
        if ( vAig )
            iRes = If_LogCreateAndXorMulti( vAig, pFaninLitsAnd, nCounterAnd, nSuppAll, 0 );
        else
            *pArea += nLits == 1 ? 0 : nLits - 1;
        Delay = If_LogCounterAddAig( pCounterOr, &nCounterOr, pFaninLitsOr, Delay, vAig ? Abc_LitNot(iRes) : -1, vAig, nSuppAll, 0, 0 );
    }
    assert( nCounterOr > 0 );
    if ( vAig )
    {
        *piRes = Abc_LitNot( If_LogCreateAndXorMulti( vAig, pFaninLitsOr, nCounterOr, nSuppAll, 0 ) );
        if ( ((vCover->nCap >> 16) & 1) )  // hack to remember complemented attribute
            *piRes = Abc_LitNot( *piRes );
    }
    else       
        *pArea += Vec_IntSize(vCover) == 1 ? 0 : Vec_IntSize(vCover) - 1;
    return Delay;
}
int If_CutSopBalanceEvalIntInt( Vec_Int_t * vCover, int nLeaves, int * pTimes, Vec_Int_t * vAig, int fCompl, int * pArea ) 
{
    int pFaninLits[IF_MAX_FUNC_LUTSIZE];
    int iRes = 0, Res, k;
    if ( vAig )
        for ( k = 0; k < nLeaves; k++ )
            pFaninLits[k] = Abc_Var2Lit(k, 0);
    Res = If_CutSopBalanceEvalInt( vCover, pTimes, pFaninLits, vAig, &iRes, nLeaves, pArea );
    if ( Res == -1 )
        return -1;
    assert( vAig == NULL || Abc_Lit2Var(iRes) == nLeaves + Abc_Lit2Var(Vec_IntSize(vAig)) - 1 );
    if ( vAig )
        Vec_IntPush( vAig, Abc_LitIsCompl(iRes) ^ fCompl );
    assert( vAig == NULL || (Vec_IntSize(vAig) & 1) );
    return Res;
}
int If_CutSopBalanceEval( If_Man_t * p, If_Cut_t * pCut, Vec_Int_t * vAig )
{
    pCut->fUser = 1;
    if ( vAig )
        Vec_IntClear( vAig );
    if ( pCut->nLeaves == 0 ) // const
    {
        assert( Abc_Lit2Var(If_CutTruthLit(pCut)) == 0 );
        if ( vAig )
            Vec_IntPush( vAig, Abc_LitIsCompl(If_CutTruthLit(pCut)) );
        pCut->Cost = 0;
        return 0;
    }
    if ( pCut->nLeaves == 1 ) // variable
    {
        assert( Abc_Lit2Var(If_CutTruthLit(pCut)) == 1 );
        if ( vAig )
            Vec_IntPush( vAig, 0 );
        if ( vAig )
            Vec_IntPush( vAig, Abc_LitIsCompl(If_CutTruthLit(pCut)) );
        pCut->Cost = 0;
        return (int)If_ObjCutBest(If_CutLeaf(p, pCut, 0))->Delay;
    }
    else
    {
        int fVerbose = 0;
        Vec_Int_t * vCover = Vec_WecEntry( p->vTtIsops[pCut->nLeaves], Abc_Lit2Var(If_CutTruthLit(pCut)) );
        int Delay, Area = 0;
        int i, pTimes[IF_MAX_FUNC_LUTSIZE];
        if ( vCover == NULL )
            return -1;
        assert( Vec_IntSize(vCover) > 0 );
        for ( i = 0; i < If_CutLeaveNum(pCut); i++ )
            pTimes[i] = (int)If_ObjCutBest(If_CutLeaf(p, pCut, i))->Delay; 
        Delay = If_CutSopBalanceEvalIntInt( vCover, If_CutLeaveNum(pCut), pTimes, vAig, Abc_LitIsCompl(If_CutTruthLit(pCut)) ^ pCut->fCompl, &Area );
        pCut->Cost = Area;
        if ( fVerbose )
        {
            int Max = 0, Two = 0;
            for ( i = 0; i < If_CutLeaveNum(pCut); i++ )
                Max = Abc_MaxInt( Max, pTimes[i] );
            for ( i = 0; i < If_CutLeaveNum(pCut); i++ )
                if ( pTimes[i] != Max )
                    Two = Abc_MaxInt( Two, pTimes[i] );
            if ( Two + 2 < Max && Max + 3 < Delay )
            {
                for ( i = 0; i < If_CutLeaveNum(pCut); i++ )
                    printf( "%3d ", pTimes[i] );
                for ( ; i < p->pPars->nLutSize; i++ )
                    printf( "    " );
                printf( "-> %3d   ", Delay );
                Dau_DsdPrintFromTruth( If_CutTruthW(p, pCut), If_CutLeaveNum(pCut) );
                Kit_TruthIsopPrintCover( vCover, If_CutLeaveNum(pCut), Abc_LitIsCompl(If_CutTruthLit(pCut)) ^ pCut->fCompl );
                {
                    Vec_Int_t vIsop;
                    int pIsop[64];
                    vIsop.nCap = vIsop.nSize = Abc_Tt6Esop( *If_CutTruthW(p, pCut), pCut->nLeaves, pIsop );
                    vIsop.pArray = pIsop;
                    printf( "ESOP (%d -> %d)\n", Vec_IntSize(vCover), vIsop.nSize );
                    Kit_TruthIsopPrintCover( &vIsop, If_CutLeaveNum(pCut), 0 );
                }
                printf( "\n" );
            }
        }
        return Delay;
    }
}

/**Function*************************************************************

  Synopsis    [Evaluate delay using SOP balancing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_CutLutBalancePinDelays( If_Man_t * p, If_Cut_t * pCut, char * pPerm )
{
    if ( pCut->nLeaves == 0 ) // const
        return 0;
    if ( pCut->nLeaves == 1 ) // variable
    {
        pPerm[0] = 0;
        return (int)If_ObjCutBest(If_CutLeaf(p, pCut, 0))->Delay;
    }
    else
    {
        char * pCutPerm = If_CutDsdPerm( p, pCut );
        int LutSize = p->pPars->pLutStruct[0] - '0';
        int i, Delay, DelayMax = -1;
        assert( (If_CutLeaveNum(pCut) > LutSize) == (pCut->uMaskFunc > 0) );
        for ( i = 0; i < If_CutLeaveNum(pCut); i++ )
        {
            if ( If_CutLeaveNum(pCut) > LutSize && ((pCut->uMaskFunc >> (i << 1)) & 1) )
                pPerm[Abc_Lit2Var((int)pCutPerm[i])] = 2;
            else
                pPerm[Abc_Lit2Var((int)pCutPerm[i])] = 1;
        }
        for ( i = 0; i < If_CutLeaveNum(pCut); i++ )
        {
            Delay = (int)If_ObjCutBest(If_CutLeaf(p, pCut, i))->Delay;
            DelayMax = Abc_MaxInt( DelayMax, Delay + (int)pPerm[i] );
        }
        return DelayMax;
    }
}

/**Function*************************************************************

  Synopsis    [Evaluate delay using SOP balancing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_CutLutBalanceEval( If_Man_t * p, If_Cut_t * pCut )
{
    pCut->fUser = 1;
    pCut->Cost = pCut->nLeaves > 1 ? 1 : 0;
    pCut->uMaskFunc = 0;
    if ( pCut->nLeaves == 0 ) // const
    {
        assert( Abc_Lit2Var(If_CutTruthLit(pCut)) == 0 );
        return 0;
    }
    if ( pCut->nLeaves == 1 ) // variable
    {
        assert( Abc_Lit2Var(If_CutTruthLit(pCut)) == 1 );
        return (int)If_ObjCutBest(If_CutLeaf(p, pCut, 0))->Delay;
    }
    else
    {
        char * pCutPerm = If_CutDsdPerm( p, pCut );
        int LutSize = p->pPars->pLutStruct[0] - '0';
        int i, pTimes[IF_MAX_FUNC_LUTSIZE];
        int DelayMax = -1, nLeafMax = 0;
        unsigned uLeafMask = 0;
        for ( i = 0; i < If_CutLeaveNum(pCut); i++ )
        {
            pTimes[i] = (int)If_ObjCutBest(If_CutLeaf(p, pCut, Abc_Lit2Var((int)pCutPerm[i])))->Delay; 
            if ( DelayMax < pTimes[i] )
                DelayMax = pTimes[i], nLeafMax = 1, uLeafMask = (1 << (i << 1));
            else if ( DelayMax == pTimes[i] )
                nLeafMax++, uLeafMask |= (1 << (i << 1));
        }
        if ( If_CutLeaveNum(pCut) <= LutSize )
            return DelayMax + 1;
        pCut->Cost = 2;
        if ( nLeafMax <= LutSize - 1 )
        {
            pCut->uMaskFunc = If_DsdManCheckXY( p->pIfDsdMan, If_CutDsdLit(p, pCut), LutSize, 1, uLeafMask, 0, 0 );
            if ( pCut->uMaskFunc > 0 )
                return DelayMax + 1;
        }
        pCut->uMaskFunc = If_DsdManCheckXY( p->pIfDsdMan, If_CutDsdLit(p, pCut), LutSize, 1, 0, 0, 0 );
        if ( pCut->uMaskFunc == 0 )
            return -1;
        return DelayMax + 2;
    }
}
/*
int If_CutLutBalanceEval( If_Man_t * p, If_Cut_t * pCut )
{
    char pPerm[16];
    int Delay2, Delay = If_CutLutBalanceEvalInt( p, pCut );
    if ( Delay == -1 )
        return -1;
    Delay2 = If_CutLutBalancePinDelays( p, pCut, pPerm );
    if ( Delay2 != Delay )
    {
        int s = 0;
        char * pCutPerm = If_CutDsdPerm( p, pCut );
        If_DsdManPrintNode( p->pIfDsdMan, If_CutDsdLit(p, pCut) );        Dau_DecPrintSet( pCut->uMaskFunc, pCut->nLeaves, 1 );
        Kit_DsdPrintFromTruth( If_CutTruthUR(p, pCut), pCut->nLeaves ); printf( "\n" );
        for ( s = 0; s < pCut->nLeaves; s++ )
//            printf( "%d ", (int)If_ObjCutBest(If_CutLeaf(p, pCut, Abc_Lit2Var((int)pCutPerm[s])))->Delay );
            printf( "%d ", (int)If_ObjCutBest(If_CutLeaf(p, pCut, s))->Delay );
        printf( "\n" );

        Delay  = If_CutLutBalanceEvalInt( p, pCut );
        Delay2 = If_CutLutBalancePinDelays( p, pCut, pPerm );
    }

    return Delay;
}
*/

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

