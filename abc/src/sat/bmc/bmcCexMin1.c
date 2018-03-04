/**CFile****************************************************************

  FileName    [bmcCexMin1.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based bounded model checking.]

  Synopsis    [CEX minimization.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: bmcCexMin1.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig/ioa/ioa.h"
#include "bmc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Find the roots to begin traversal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManCexMinGetCos( Aig_Man_t * pAig, Abc_Cex_t * pCex, Vec_Int_t * vLeaves, Vec_Int_t * vRoots )
{
    Aig_Obj_t * pObj;
    int i;
    Vec_IntClear( vRoots );
    if ( vLeaves == NULL )
    {
        pObj = Aig_ManCo( pAig, pCex->iPo );
        Vec_IntPush( vRoots, Aig_ObjId(pObj) );
        return;
    }
    Aig_ManForEachObjVec( vLeaves, pAig, pObj, i )
        if ( Saig_ObjIsLo(pAig, pObj) )
            Vec_IntPush( vRoots, Aig_ObjId( Saig_ObjLoToLi(pAig, pObj) ) );
}

/**Function*************************************************************

  Synopsis    [Collects CI of the given timeframe.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManCexMinCollectFrameTerms_rec( Aig_Man_t * pAig, Aig_Obj_t * pObj, Vec_Int_t * vFrameCisOne )
{
    if ( Aig_ObjIsTravIdCurrent(pAig, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(pAig, pObj);
    if ( Aig_ObjIsCo(pObj) )
        Saig_ManCexMinCollectFrameTerms_rec( pAig, Aig_ObjFanin0(pObj), vFrameCisOne );
    else if ( Aig_ObjIsNode(pObj) )
    {
        Saig_ManCexMinCollectFrameTerms_rec( pAig, Aig_ObjFanin0(pObj), vFrameCisOne );
        Saig_ManCexMinCollectFrameTerms_rec( pAig, Aig_ObjFanin1(pObj), vFrameCisOne );
    }
    else if ( Aig_ObjIsCi(pObj) )
        Vec_IntPush( vFrameCisOne, Aig_ObjId(pObj) );
}

/**Function*************************************************************

  Synopsis    [Collects CIs of all timeframes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Vec_t * Saig_ManCexMinCollectFrameTerms( Aig_Man_t * pAig, Abc_Cex_t * pCex )
{
    Vec_Vec_t * vFrameCis;
    Vec_Int_t * vRoots, * vLeaves;
    Aig_Obj_t * pObj;
    int i, f;
    // create terminals
    vRoots = Vec_IntAlloc( 1000 );
    vFrameCis = Vec_VecStart( pCex->iFrame+1 );
    for ( f = pCex->iFrame; f >= 0; f-- )
    {
        // create roots
        vLeaves = (f == pCex->iFrame) ? NULL : Vec_VecEntryInt(vFrameCis, f+1);
        Saig_ManCexMinGetCos( pAig, pCex, vLeaves, vRoots );
        // collect nodes starting from the roots
        Aig_ManIncrementTravId( pAig );
        Aig_ManForEachObjVec( vRoots, pAig, pObj, i )
            Saig_ManCexMinCollectFrameTerms_rec( pAig, pObj, Vec_VecEntryInt(vFrameCis, f) );
    }
    Vec_IntFree( vRoots );
    return vFrameCis;
}



/**Function*************************************************************

  Synopsis    [Recursively sets phase and priority.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManCexMinDerivePhasePriority_rec( Aig_Man_t * pAig, Aig_Obj_t * pObj )
{
    if ( Aig_ObjIsTravIdCurrent(pAig, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(pAig, pObj);
    if ( Aig_ObjIsCo(pObj) )
    {
        Saig_ManCexMinDerivePhasePriority_rec( pAig, Aig_ObjFanin0(pObj) );
        assert( Aig_ObjFanin0(pObj)->iData >= 0 );
        pObj->iData = Aig_ObjFanin0(pObj)->iData ^ Aig_ObjFaninC0(pObj);
    }
    else if ( Aig_ObjIsNode(pObj) )
    {
        int fPhase0, fPhase1, iPrio0, iPrio1;
        Saig_ManCexMinDerivePhasePriority_rec( pAig, Aig_ObjFanin0(pObj) );
        Saig_ManCexMinDerivePhasePriority_rec( pAig, Aig_ObjFanin1(pObj) );
        assert( Aig_ObjFanin0(pObj)->iData >= 0 );
        assert( Aig_ObjFanin1(pObj)->iData >= 0 );
        fPhase0 = Abc_LitIsCompl( Aig_ObjFanin0(pObj)->iData ) ^ Aig_ObjFaninC0(pObj);
        fPhase1 = Abc_LitIsCompl( Aig_ObjFanin1(pObj)->iData ) ^ Aig_ObjFaninC1(pObj);
        iPrio0  = Abc_Lit2Var( Aig_ObjFanin0(pObj)->iData );
        iPrio1  = Abc_Lit2Var( Aig_ObjFanin1(pObj)->iData );
        if ( fPhase0 && fPhase1 ) // both are one
            pObj->iData = Abc_Var2Lit( Abc_MinInt(iPrio0, iPrio1), 1 );
        else if ( !fPhase0 && fPhase1 ) 
            pObj->iData = Abc_Var2Lit( iPrio0, 0 );
        else if ( fPhase0 && !fPhase1 )
            pObj->iData = Abc_Var2Lit( iPrio1, 0 );
        else // both are zero
            pObj->iData = Abc_Var2Lit( Abc_MaxInt(iPrio0, iPrio1), 0 );
    }
}

/**Function*************************************************************

  Synopsis    [Verify phase.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManCexMinVerifyPhase( Aig_Man_t * pAig, Abc_Cex_t * pCex, int f )
{
    Aig_Obj_t * pObj;
    int i;
    Aig_ManConst1(pAig)->fPhase = 1;
    Saig_ManForEachPi( pAig, pObj, i )
        pObj->fPhase = Abc_InfoHasBit(pCex->pData, pCex->nRegs + f * pCex->nPis + i);
    if ( f == 0 )
    {
        Saig_ManForEachLo( pAig, pObj, i )
            pObj->fPhase = 0;
    }
    else
    {
        Saig_ManForEachLo( pAig, pObj, i )
            pObj->fPhase = Saig_ObjLoToLi(pAig, pObj)->fPhase;
    }
    Aig_ManForEachNode( pAig, pObj, i )
        pObj->fPhase = (Aig_ObjFaninC0(pObj) ^ Aig_ObjFanin0(pObj)->fPhase) & 
                       (Aig_ObjFaninC1(pObj) ^ Aig_ObjFanin1(pObj)->fPhase);
    Aig_ManForEachCo( pAig, pObj, i )
        pObj->fPhase = (Aig_ObjFaninC0(pObj) ^ Aig_ObjFanin0(pObj)->fPhase);
}

/**Function*************************************************************

  Synopsis    [Collects phase and priority of all timeframes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManCexMinDerivePhasePriority( Aig_Man_t * pAig, Abc_Cex_t * pCex, Vec_Vec_t * vFrameCis, Vec_Vec_t * vFramePPs, int f, Vec_Int_t * vRoots )
{
    Vec_Int_t * vFramePPsOne, * vFrameCisOne, * vLeaves;
    Aig_Obj_t * pObj;
    int i;
    // set PP for the CIs
    vFrameCisOne = Vec_VecEntryInt( vFrameCis, f );
    vFramePPsOne = Vec_VecEntryInt( vFramePPs, f );
    Aig_ManForEachObjVec( vFrameCisOne, pAig, pObj, i )
    {
        pObj->iData = Vec_IntEntry( vFramePPsOne, i );
        assert( pObj->iData >= 0 );
    }
//    if ( f == 0 )
//        Saig_ManCexMinVerifyPhase( pAig, pCex, f );

    // create roots
    vLeaves = (f == pCex->iFrame) ? NULL : Vec_VecEntryInt(vFrameCis, f+1);
    Saig_ManCexMinGetCos( pAig, pCex, vLeaves, vRoots );
    // derive for the nodes starting from the roots
    Aig_ManIncrementTravId( pAig );
    Aig_ManForEachObjVec( vRoots, pAig, pObj, i )
    {
        Saig_ManCexMinDerivePhasePriority_rec( pAig, pObj );
//        if ( f == 0 )
//            assert( (pObj->iData & 1) == pObj->fPhase );
    }
}

/**Function*************************************************************

  Synopsis    [Collects phase and priority of all timeframes.]

  Description []
               
  SideEffects []

  SeeAlso     []
 
***********************************************************************/
Vec_Vec_t * Saig_ManCexMinCollectPhasePriority_( Aig_Man_t * pAig, Abc_Cex_t * pCex, Vec_Vec_t * vFrameCis )
{
    Vec_Vec_t * vFramePPs;
    Vec_Int_t * vRoots, * vFramePPsOne, * vFrameCisOne;
    Aig_Obj_t * pObj;
    int i, f, nPrioOffset;

    // initialize phase and priority
    Aig_ManForEachObj( pAig, pObj, i )
        pObj->iData = -1;

    // set the constant node to higher priority than the flops
    vFramePPs = Vec_VecStart( pCex->iFrame+1 );
    nPrioOffset = (pCex->iFrame + 1) * pCex->nPis;
    Aig_ManConst1(pAig)->iData = Abc_Var2Lit( nPrioOffset + pCex->nRegs, 1 );
    vRoots = Vec_IntAlloc( 1000 );
    for ( f = 0; f <= pCex->iFrame; f++ )
    {
        int nPiCount = 0;
        // fill in PP for the CIs
        vFrameCisOne = Vec_VecEntryInt( vFrameCis, f );
        vFramePPsOne = Vec_VecEntryInt( vFramePPs, f );
        assert( Vec_IntSize(vFramePPsOne) == 0 );
        Aig_ManForEachObjVec( vFrameCisOne, pAig, pObj, i )
        {
            assert( Aig_ObjIsCi(pObj) );
            if ( Saig_ObjIsPi(pAig, pObj) )
                Vec_IntPush( vFramePPsOne, Abc_Var2Lit( (f+1) * pCex->nPis - nPiCount++, Abc_InfoHasBit(pCex->pData, pCex->nRegs + f * pCex->nPis + Aig_ObjCioId(pObj)) ) );
            else if ( f == 0 )
                Vec_IntPush( vFramePPsOne, Abc_Var2Lit( nPrioOffset + Saig_ObjRegId(pAig, pObj), 0 ) );
            else
                Vec_IntPush( vFramePPsOne, Saig_ObjLoToLi(pAig, pObj)->iData );
        }
        // compute the PP info
        Saig_ManCexMinDerivePhasePriority( pAig, pCex, vFrameCis, vFramePPs, f, vRoots );
    }
    Vec_IntFree( vRoots );
    // check the output
    pObj = Aig_ManCo( pAig, pCex->iPo );
    assert( Abc_LitIsCompl(pObj->iData) );
    return vFramePPs;
}

/**Function*************************************************************

  Synopsis    [Collects phase and priority of all timeframes.]

  Description []
               
  SideEffects []

  SeeAlso     []
 
***********************************************************************/
Vec_Vec_t * Saig_ManCexMinCollectPhasePriority( Aig_Man_t * pAig, Abc_Cex_t * pCex, Vec_Vec_t * vFrameCis )
{
    Vec_Vec_t * vFramePPs;
    Vec_Int_t * vRoots, * vFramePPsOne, * vFrameCisOne;
    Aig_Obj_t * pObj;
    int i, f, nPrioOffset;

    // initialize phase and priority
    Aig_ManForEachObj( pAig, pObj, i )
        pObj->iData = -1;

    // set the constant node to higher priority than the flops
    vFramePPs = Vec_VecStart( pCex->iFrame+1 );
    nPrioOffset = pCex->nRegs;
    Aig_ManConst1(pAig)->iData = Abc_Var2Lit( nPrioOffset + (pCex->iFrame + 1) * pCex->nPis, 1 );
    vRoots = Vec_IntAlloc( 1000 );
    for ( f = 0; f <= pCex->iFrame; f++ )
    {
        int nPiCount = 0;
        // fill in PP for the CIs
        vFrameCisOne = Vec_VecEntryInt( vFrameCis, f );
        vFramePPsOne = Vec_VecEntryInt( vFramePPs, f );
        assert( Vec_IntSize(vFramePPsOne) == 0 );
        Aig_ManForEachObjVec( vFrameCisOne, pAig, pObj, i )
        {
            assert( Aig_ObjIsCi(pObj) );
            if ( Saig_ObjIsPi(pAig, pObj) )
                Vec_IntPush( vFramePPsOne, Abc_Var2Lit( nPrioOffset + (f+1) * pCex->nPis - 1 - nPiCount++, Abc_InfoHasBit(pCex->pData, pCex->nRegs + f * pCex->nPis + Aig_ObjCioId(pObj)) ) );
            else if ( f == 0 )
                Vec_IntPush( vFramePPsOne, Abc_Var2Lit( Saig_ObjRegId(pAig, pObj), 0 ) );
            else
                Vec_IntPush( vFramePPsOne, Saig_ObjLoToLi(pAig, pObj)->iData );
        }
        // compute the PP info
        Saig_ManCexMinDerivePhasePriority( pAig, pCex, vFrameCis, vFramePPs, f, vRoots );
    }
    Vec_IntFree( vRoots );
    // check the output
    pObj = Aig_ManCo( pAig, pCex->iPo );
    assert( Abc_LitIsCompl(pObj->iData) );
    return vFramePPs;
}


/**Function*************************************************************

  Synopsis    [Returns reasons for the property to fail.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManCexMinCollectReason_rec( Aig_Man_t * p, Aig_Obj_t * pObj, Vec_Int_t * vReason, int fPiReason )
{
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(p, pObj);
    if ( Aig_ObjIsCi(pObj) )
    {
        if ( fPiReason && Saig_ObjIsPi(p, pObj) )
            Vec_IntPush( vReason, Abc_Var2Lit( Aig_ObjCioId(pObj), !Abc_LitIsCompl(pObj->iData) ) );
        else if ( !fPiReason && Saig_ObjIsLo(p, pObj) )
            Vec_IntPush( vReason, Abc_Var2Lit( Saig_ObjRegId(p, pObj), !Abc_LitIsCompl(pObj->iData) ) );
        return;
    }
    if ( Aig_ObjIsCo(pObj) )
    {
        Saig_ManCexMinCollectReason_rec( p, Aig_ObjFanin0(pObj), vReason, fPiReason );
        return;
    }
    if ( Aig_ObjIsConst1(pObj) )
        return;
    assert( Aig_ObjIsNode(pObj) );
    if ( Abc_LitIsCompl(pObj->iData) ) // value 1
    {
        int fPhase0 = Abc_LitIsCompl( Aig_ObjFanin0(pObj)->iData ) ^ Aig_ObjFaninC0(pObj);
        int fPhase1 = Abc_LitIsCompl( Aig_ObjFanin1(pObj)->iData ) ^ Aig_ObjFaninC1(pObj);
        assert( fPhase0 && fPhase1 );
        Saig_ManCexMinCollectReason_rec( p, Aig_ObjFanin0(pObj), vReason, fPiReason );
        Saig_ManCexMinCollectReason_rec( p, Aig_ObjFanin1(pObj), vReason, fPiReason );
    }
    else
    {
        int fPhase0 = Abc_LitIsCompl( Aig_ObjFanin0(pObj)->iData ) ^ Aig_ObjFaninC0(pObj);
        int fPhase1 = Abc_LitIsCompl( Aig_ObjFanin1(pObj)->iData ) ^ Aig_ObjFaninC1(pObj);
        assert( !fPhase0 || !fPhase1 );
        if ( !fPhase0 && fPhase1 )
            Saig_ManCexMinCollectReason_rec( p, Aig_ObjFanin0(pObj), vReason, fPiReason );
        else if ( fPhase0 && !fPhase1 )
            Saig_ManCexMinCollectReason_rec( p, Aig_ObjFanin1(pObj), vReason, fPiReason );
        else 
        {
            int iPrio0 = Abc_Lit2Var( Aig_ObjFanin0(pObj)->iData );
            int iPrio1 = Abc_Lit2Var( Aig_ObjFanin1(pObj)->iData );
            if ( iPrio0 >= iPrio1 )
                Saig_ManCexMinCollectReason_rec( p, Aig_ObjFanin0(pObj), vReason, fPiReason );
            else
                Saig_ManCexMinCollectReason_rec( p, Aig_ObjFanin1(pObj), vReason, fPiReason );
        }
    }
}


/**Function*************************************************************

  Synopsis    [Collects phase and priority of all timeframes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Vec_t * Saig_ManCexMinCollectReason( Aig_Man_t * pAig, Abc_Cex_t * pCex, Vec_Vec_t * vFrameCis, Vec_Vec_t * vFramePPs, int fPiReason )
{
    Vec_Vec_t * vFrameReas;
    Vec_Int_t * vRoots, * vLeaves;
    Aig_Obj_t * pObj;
    int i, f;
    // select reason for the property to fail
    vFrameReas = Vec_VecStart( pCex->iFrame+1 );
    vRoots = Vec_IntAlloc( 1000 );
    for ( f = pCex->iFrame; f >= 0; f-- )
    {
        // set phase and polarity
        Saig_ManCexMinDerivePhasePriority( pAig, pCex, vFrameCis, vFramePPs, f, vRoots );
        // create roots
        vLeaves = (f == pCex->iFrame) ? NULL : Vec_VecEntryInt(vFrameCis, f+1);
        Saig_ManCexMinGetCos( pAig, pCex, vLeaves, vRoots );
        // collect nodes starting from the roots
        Aig_ManIncrementTravId( pAig );
        Aig_ManForEachObjVec( vRoots, pAig, pObj, i )
            Saig_ManCexMinCollectReason_rec( pAig, pObj, Vec_VecEntryInt(vFrameReas, f), fPiReason );
//printf( "%d(%d) ", Vec_VecLevelSize(vFrameCis, f), Vec_VecLevelSize(vFrameReas, f) ); 
    }
//printf( "\n" );
    Vec_IntFree( vRoots );
    return vFrameReas;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Vec_t * Saig_ManCexMinComputeReason( Aig_Man_t * pAig, Abc_Cex_t * pCex, int fPiReason )
{
    Vec_Vec_t * vFrameCis, * vFramePPs, * vFrameReas;
    // sanity checks
    assert( pCex->nPis == Saig_ManPiNum(pAig) );
    assert( pCex->nRegs == Saig_ManRegNum(pAig) );
    assert( pCex->iPo >= 0 && pCex->iPo < Saig_ManPoNum(pAig) );
    // derive frame terms
    vFrameCis = Saig_ManCexMinCollectFrameTerms( pAig, pCex );
    // derive phase and priority
    vFramePPs = Saig_ManCexMinCollectPhasePriority( pAig, pCex, vFrameCis );
    // derive reasons for property failure
    vFrameReas = Saig_ManCexMinCollectReason( pAig, pCex, vFrameCis, vFramePPs, fPiReason );
    Vec_VecFree( vFramePPs );
    Vec_VecFree( vFrameCis );
    return vFrameReas;
}


/**Function*************************************************************

  Synopsis    [Duplicate with literals.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManCexMinDupWithCubes( Aig_Man_t * pAig, Vec_Vec_t * vReg2Value )
{
    Vec_Int_t * vLevel;
    Aig_Man_t * pAigNew;
    Aig_Obj_t * pObj, * pMiter;
    int i, k, Lit;
    assert( pAig->nConstrs == 0 );
    // start the new manager
    pAigNew = Aig_ManStart( Aig_ManNodeNum(pAig) + Vec_VecSizeSize(vReg2Value) + Vec_VecSize(vReg2Value) );
    pAigNew->pName = Abc_UtilStrsav( pAig->pName );
    // map the constant node
    Aig_ManConst1(pAig)->pData = Aig_ManConst1( pAigNew );
    // create variables for PIs
    Aig_ManForEachCi( pAig, pObj, i )
        pObj->pData = Aig_ObjCreateCi( pAigNew );
    // add internal nodes of this frame
    Aig_ManForEachNode( pAig, pObj, i )
        pObj->pData = Aig_And( pAigNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    // create POs for cubes
    Vec_VecForEachLevelInt( vReg2Value, vLevel, i )
    {
        if ( i == 0 )
            continue;
        pMiter = Aig_ManConst1( pAigNew );
        Vec_IntForEachEntry( vLevel, Lit, k )
        {
            assert( Lit >= 0 && Lit < 2 * Aig_ManRegNum(pAig) );
            pObj = Saig_ManLi( pAig, Abc_Lit2Var(Lit) );
            pMiter = Aig_And( pAigNew, pMiter, Aig_NotCond(Aig_ObjChild0Copy(pObj), Abc_LitIsCompl(Lit)) );
        }
        Aig_ObjCreateCo( pAigNew, pMiter );
    }
    // transfer to register outputs
    Saig_ManForEachLi( pAig, pObj, i )
        Aig_ObjCreateCo( pAigNew, Aig_ObjChild0Copy(pObj) );
    // finalize
    Aig_ManCleanup( pAigNew );
    Aig_ManSetRegNum( pAigNew, Aig_ManRegNum(pAig) );
    return pAigNew;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Saig_ManCexMinPerform( Aig_Man_t * pAig, Abc_Cex_t * pCex )
{
    int fReasonPi = 0;

    Abc_Cex_t * pCexMin = NULL;
    Aig_Man_t * pManNew = NULL;
    Vec_Vec_t * vFrameReas;
    vFrameReas = Saig_ManCexMinComputeReason( pAig, pCex, fReasonPi );
    printf( "Reason size = %d.  Ave = %d.\n", Vec_VecSizeSize(vFrameReas), Vec_VecSizeSize(vFrameReas)/(pCex->iFrame + 1) );

    // try reducing the frames
    if ( !fReasonPi )
    {
        char * pFileName = "aigcube.aig";
        pManNew = Saig_ManCexMinDupWithCubes( pAig, vFrameReas );
        Ioa_WriteAiger( pManNew, pFileName, 0, 0 );
        Aig_ManStop( pManNew );
        printf( "Intermediate AIG is written into file \"%s\".\n", pFileName );
    }

    // find reduced counter-example
    Vec_VecFree( vFrameReas );
    return pCexMin;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

