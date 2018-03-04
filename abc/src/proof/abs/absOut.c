/**CFile****************************************************************

  FileName    [absOut.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Abstraction package.]

  Synopsis    [Abstraction refinement outside of abstraction engines.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: absOut.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abs.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Derive a new counter-example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Gia_ManCexRemap( Gia_Man_t * p, Abc_Cex_t * pCexAbs, Vec_Int_t * vPis )
{
    Abc_Cex_t * pCex;
    int i, f, iPiNum;
    assert( pCexAbs->iPo == 0 );
    // start the counter-example
    pCex = Abc_CexAlloc( Gia_ManRegNum(p), Gia_ManPiNum(p), pCexAbs->iFrame+1 );
    pCex->iFrame = pCexAbs->iFrame;
    pCex->iPo    = pCexAbs->iPo;
    // copy the bit data
    for ( f = 0; f <= pCexAbs->iFrame; f++ )
        for ( i = 0; i < Vec_IntSize(vPis); i++ )
        {
            if ( Abc_InfoHasBit( pCexAbs->pData, pCexAbs->nRegs + pCexAbs->nPis * f + i ) )
            {
                iPiNum = Gia_ObjCioId( Gia_ManObj(p, Vec_IntEntry(vPis, i)) );
                Abc_InfoSetBit( pCex->pData, pCex->nRegs + pCex->nPis * f + iPiNum );
            }
        }
    // verify the counter example
    if ( !Gia_ManVerifyCex( p, pCex, 0 ) )
    {
        Abc_Print( 1, "Gia_ManCexRemap(): Counter-example is invalid.\n" );
        Abc_CexFree( pCex );
        pCex = NULL;
    }
    else
    {
        Abc_Print( 1, "Counter-example verification is successful.\n" );
        Abc_Print( 1, "Output %d of miter \"%s\" was asserted in frame %d. \n", pCex->iPo, p->pName, pCex->iFrame );
    }
    return pCex;
}

/**Function*************************************************************

  Synopsis    [Refines gate-level abstraction using the counter-example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManGlaRefine( Gia_Man_t * p, Abc_Cex_t * pCex, int fMinCut, int fVerbose )
{
    extern void Nwk_ManDeriveMinCut( Gia_Man_t * p, int fVerbose );
    int fAddOneLayer = 1;
    Abc_Cex_t * pCexNew = NULL;
    Gia_Man_t * pAbs;
    Aig_Man_t * pAig;
    Abc_Cex_t * pCare;
    Vec_Int_t * vPis, * vPPis;
    int f, i, iObjId;
    abctime clk = Abc_Clock();
    int nOnes = 0, Counter = 0;
    if ( p->vGateClasses == NULL )
    {
        Abc_Print( 1, "Gia_ManGlaRefine(): Abstraction gate map is missing.\n" );
        return -1;
    }
    // derive abstraction
    pAbs = Gia_ManDupAbsGates( p, p->vGateClasses );
    Gia_ManStop( pAbs );
    pAbs = Gia_ManDupAbsGates( p, p->vGateClasses );
    if ( Gia_ManPiNum(pAbs) != pCex->nPis )
    {
        Abc_Print( 1, "Gia_ManGlaRefine(): The PI counts in GLA and in CEX do not match.\n" );
        Gia_ManStop( pAbs );
        return -1;
    }
    if ( !Gia_ManVerifyCex( pAbs, pCex, 0 ) )
    {
        Abc_Print( 1, "Gia_ManGlaRefine(): The initial counter-example is invalid.\n" );
//        Gia_ManStop( pAbs );
//        return -1;
    }
//    else
//        Abc_Print( 1, "Gia_ManGlaRefine(): The initial counter-example is correct.\n" );
    // get inputs
    Gia_ManGlaCollect( p, p->vGateClasses, &vPis, &vPPis, NULL, NULL );
    assert( Vec_IntSize(vPis) + Vec_IntSize(vPPis) == Gia_ManPiNum(pAbs) );
    // add missing logic
    if ( fAddOneLayer )
    {
        Gia_Obj_t * pObj;
        // check if this is a real counter-example
        Gia_ObjTerSimSet0( Gia_ManConst0(pAbs) );
        for ( f = 0; f <= pCex->iFrame; f++ )
        {
            Gia_ManForEachPi( pAbs, pObj, i )
            {
                if ( i >= Vec_IntSize(vPis) ) // PPIs
                    Gia_ObjTerSimSetX( pObj );
                else if ( Abc_InfoHasBit(pCex->pData, pCex->nRegs + pCex->nPis * f + i) )
                    Gia_ObjTerSimSet1( pObj );
                else
                    Gia_ObjTerSimSet0( pObj );
            }
            Gia_ManForEachRo( pAbs, pObj, i )
            {
                if ( f == 0 )
                    Gia_ObjTerSimSet0( pObj );
                else
                    Gia_ObjTerSimRo( pAbs, pObj );
            }
            Gia_ManForEachAnd( pAbs, pObj, i )
                Gia_ObjTerSimAnd( pObj );
            Gia_ManForEachCo( pAbs, pObj, i )
                Gia_ObjTerSimCo( pObj );
        }
        pObj = Gia_ManPo( pAbs, 0 );
        if ( Gia_ObjTerSimGet1(pObj) )
        {
            pCexNew = Gia_ManCexRemap( p, pCex, vPis );
            Abc_Print( 1, "Procedure &gla_refine found a real counter-example in frame %d.\n", pCexNew->iFrame );
        }
//        else
//            Abc_Print( 1, "CEX is not real.\n" );
        Gia_ManForEachObj( pAbs, pObj, i )
            Gia_ObjTerSimSetC( pObj );
        if ( pCexNew == NULL )
        {
            // grow one layer
            Vec_IntForEachEntry( vPPis, iObjId, i )
            {
                assert( Vec_IntEntry( p->vGateClasses, iObjId ) == 0 );
                Vec_IntWriteEntry( p->vGateClasses, iObjId, 1 );
            }
            if ( fVerbose )
            {
                Abc_Print( 1, "Additional objects = %d.  ", Vec_IntSize(vPPis) );
                Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
            }
        }
    }
    else
    {
        // minimize the CEX
        pAig = Gia_ManToAigSimple( pAbs );
        pCare = Saig_ManCbaFindCexCareBits( pAig, pCex, Vec_IntSize(vPis), fVerbose );
        Aig_ManStop( pAig );
        if ( pCare == NULL )
            Abc_Print( 1, "Counter-example minimization has failed.\n" );
        // add new objects to the map
        iObjId = -1;
        for ( f = 0; f <= pCare->iFrame; f++ )
            for ( i = 0; i < pCare->nPis; i++ )
                if ( Abc_InfoHasBit( pCare->pData, pCare->nRegs + f * pCare->nPis + i ) )
                {
                    nOnes++;
                    assert( i >= Vec_IntSize(vPis) );
                    iObjId = Vec_IntEntry( vPPis, i - Vec_IntSize(vPis) );
                    assert( iObjId > 0 && iObjId < Gia_ManObjNum(p) );
                    if ( Vec_IntEntry( p->vGateClasses, iObjId ) > 0 )
                        continue;
                    assert( Vec_IntEntry( p->vGateClasses, iObjId ) == 0 );
                    Vec_IntWriteEntry( p->vGateClasses, iObjId, 1 );
    //                Abc_Print( 1, "Adding object %d.\n", iObjId );
    //                Gia_ObjPrint( p, Gia_ManObj(p, iObjId) );
                    Counter++;
                }
        Abc_CexFree( pCare );
        if ( fVerbose )
        {
            Abc_Print( 1, "Essential bits = %d.  Additional objects = %d.  ", nOnes, Counter );
            Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
        }
        // consider the case of SAT
        if ( iObjId == -1 )
        {
            pCexNew = Gia_ManCexRemap( p, pCex, vPis );
            Abc_Print( 1, "Procedure &gla_refine found a real counter-example in frame %d.\n", pCexNew->iFrame );
        }
    }
    Vec_IntFree( vPis );
    Vec_IntFree( vPPis );
    Gia_ManStop( pAbs );
    if ( pCexNew )
    {
        ABC_FREE( p->pCexSeq );
        p->pCexSeq = pCexNew;
        return 0;
    }
    // extract abstraction to include min-cut
    if ( fMinCut )
        Nwk_ManDeriveMinCut( p, fVerbose );
    return -1;
}





/**Function*************************************************************

  Synopsis    [Resimulates the counter-example and returns flop values.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManGetStateAndCheckCex( Gia_Man_t * pAig, Abc_Cex_t * p, int iFrame )
{
    Vec_Int_t * vInit = Vec_IntAlloc( Gia_ManRegNum(pAig) );
    Gia_Obj_t * pObj, * pObjRi, * pObjRo;
    int RetValue, i, k, iBit = 0;
    assert( iFrame >= 0 && iFrame <= p->iFrame );
    Gia_ManCleanMark0(pAig);
    Gia_ManForEachRo( pAig, pObj, i )
        pObj->fMark0 = 0;//Abc_InfoHasBit(p->pData, iBit++);
    for ( i = 0, iBit = p->nRegs; i <= p->iFrame; i++ )
    {
        if ( i == iFrame )
        {
            Gia_ManForEachRo( pAig, pObjRo, k )
                Vec_IntPush( vInit, pObjRo->fMark0 );
        }
        Gia_ManForEachPi( pAig, pObj, k )
            pObj->fMark0 = Abc_InfoHasBit(p->pData, iBit++);
        Gia_ManForEachAnd( pAig, pObj, k )
            pObj->fMark0 = (Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj)) & 
                           (Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj));
        Gia_ManForEachCo( pAig, pObj, k )
            pObj->fMark0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
        if ( i == p->iFrame )
            break;
        Gia_ManForEachRiRo( pAig, pObjRi, pObjRo, k )
            pObjRo->fMark0 = pObjRi->fMark0;
    }
    assert( iBit == p->nBits );
    RetValue = Gia_ManPo(pAig, p->iPo)->fMark0;
    if ( RetValue != 1 )
        Vec_IntFreeP( &vInit );
    Gia_ManCleanMark0(pAig);
    return vInit;
}

/**Function*************************************************************

  Synopsis    [Verify counter-example starting in the given timeframe.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCheckCex( Gia_Man_t * pAig, Abc_Cex_t * p, int iFrame )
{
    Gia_Obj_t * pObj, * pObjRi, * pObjRo;
    int RetValue, i, k, iBit = 0;
    assert( iFrame >= 0 && iFrame <= p->iFrame );
    Gia_ManCleanMark0(pAig);
    Gia_ManForEachRo( pAig, pObj, i )
        pObj->fMark0 = 0;//Abc_InfoHasBit(p->pData, iBit++);
    for ( i = iFrame, iBit += p->nRegs + Gia_ManPiNum(pAig) * iFrame; i <= p->iFrame; i++ )
    {
        Gia_ManForEachPi( pAig, pObj, k )
            pObj->fMark0 = Abc_InfoHasBit(p->pData, iBit++);
        Gia_ManForEachAnd( pAig, pObj, k )
            pObj->fMark0 = (Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj)) & 
                           (Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj));
        Gia_ManForEachCo( pAig, pObj, k )
            pObj->fMark0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
        if ( i == p->iFrame )
            break;
        Gia_ManForEachRiRo( pAig, pObjRi, pObjRo, k )
            pObjRo->fMark0 = pObjRi->fMark0;
    }
    assert( iBit == p->nBits );
    RetValue = Gia_ManPo(pAig, p->iPo)->fMark0;
    Gia_ManCleanMark0(pAig);
    if ( RetValue == 1 )
        printf( "Shortened CEX holds for the abstraction of the fast-forwarded model.\n" );
    else
        printf( "Shortened CEX does not hold for the abstraction of the fast-forwarded model.\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManTransformFlops( Gia_Man_t * p, Vec_Int_t * vFlops, Vec_Int_t * vInit )
{
    Vec_Bit_t * vInitNew;
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i, iFlopId;
    assert( Vec_IntSize(vInit) == Vec_IntSize(vFlops) );
    vInitNew = Vec_BitStart( Gia_ManRegNum(p) );
    Gia_ManForEachObjVec( vFlops, p, pObj, i )
    {
        assert( Gia_ObjIsRo(p, pObj) );
        if ( Vec_IntEntry(vInit, i) == 0 )
            continue;
        iFlopId = Gia_ObjCioId(pObj) - Gia_ManPiNum(p);
        assert( iFlopId >= 0 && iFlopId < Gia_ManRegNum(p) );
        Vec_BitWriteEntry( vInitNew, iFlopId, 1 );
    }
    pNew = Gia_ManDupFlip( p, Vec_BitArray(vInitNew) );
    Vec_BitFree( vInitNew );
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManNewRefine( Gia_Man_t * p, Abc_Cex_t * pCex, int iFrameStart, int iFrameExtra, int fVerbose )
{
    Gia_Man_t * pAbs, * pNew;
    Vec_Int_t * vFlops, * vInit;
    Vec_Int_t * vCopy;
//    abctime clk = Abc_Clock();
    int RetValue;
    ABC_FREE( p->pCexSeq );
    if ( p->vGateClasses == NULL )
    {
        Abc_Print( 1, "Gia_ManNewRefine(): Abstraction gate map is missing.\n" );
        return -1;
    }
    vCopy = Vec_IntDup( p->vGateClasses );
    Abc_Print( 1, "Refining with %d-frame CEX, starting in frame %d, with %d extra frames.\n", pCex->iFrame, iFrameStart, iFrameExtra );
    // derive abstraction
    pAbs = Gia_ManDupAbsGates( p, p->vGateClasses );
    Gia_ManStop( pAbs );
    pAbs = Gia_ManDupAbsGates( p, p->vGateClasses );
    if ( Gia_ManPiNum(pAbs) != pCex->nPis )
    {
        Abc_Print( 1, "Gia_ManNewRefine(): The PI counts in GLA and in CEX do not match.\n" );
        Gia_ManStop( pAbs );
        Vec_IntFree( vCopy );
        return -1;
    }
    // get the state in frame iFrameStart
    vInit = Gia_ManGetStateAndCheckCex( pAbs, pCex, iFrameStart );
    if ( vInit == NULL )
    {
        Abc_Print( 1, "Gia_ManNewRefine(): The initial counter-example is invalid.\n" );
        Gia_ManStop( pAbs );
        Vec_IntFree( vCopy );
        return -1;
    }
    if ( fVerbose )
        Abc_Print( 1, "Gia_ManNewRefine(): The initial counter-example is correct.\n" );
    // get inputs
    Gia_ManGlaCollect( p, p->vGateClasses, NULL, NULL, &vFlops, NULL );
//    assert( Vec_IntSize(vPis) + Vec_IntSize(vPPis) == Gia_ManPiNum(pAbs) );
    Gia_ManStop( pAbs );
//Vec_IntPrint( vFlops );
//Vec_IntPrint( vInit );
    // transform the manager to have new init state
    pNew = Gia_ManTransformFlops( p, vFlops, vInit );
    Vec_IntFree( vFlops );
    Vec_IntFree( vInit );
    // verify abstraction
    {
        Gia_Man_t * pAbs = Gia_ManDupAbsGates( pNew, p->vGateClasses );
        Gia_ManCheckCex( pAbs, pCex, iFrameStart );
        Gia_ManStop( pAbs );
    }
    // transfer abstraction
    assert( pNew->vGateClasses == NULL );
    pNew->vGateClasses = Vec_IntDup( p->vGateClasses );
    // perform abstraction for the new AIG
    {
        Abs_Par_t Pars, * pPars = &Pars;
        Abs_ParSetDefaults( pPars );
        pPars->nFramesMax = pCex->iFrame - iFrameStart + 1 + iFrameExtra;
        pPars->fVerbose = fVerbose;
        RetValue = Gia_ManPerformGla( pNew, pPars );
        if ( RetValue == 0 ) // spurious SAT
        {
            Vec_IntFreeP( &pNew->vGateClasses );
            pNew->vGateClasses = Vec_IntDup( vCopy );
        }
    }
    // move the abstraction map
    Vec_IntFreeP( &p->vGateClasses );
    p->vGateClasses = pNew->vGateClasses;
    pNew->vGateClasses = NULL;
    // cleanup
    Gia_ManStop( pNew );
    Vec_IntFree( vCopy );
    return -1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

