/**CFile****************************************************************

  FileName    [sscCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT sweeping under constraints.]

  Synopsis    [The core procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 29, 2008.]

  Revision    [$Id: sscCore.c,v 1.00 2008/07/29 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sscInt.h"
#include "proof/cec/cec.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [This procedure sets default parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssc_ManSetDefaultParams( Ssc_Pars_t * p )
{
    memset( p, 0, sizeof(Ssc_Pars_t) );
    p->nWords         =     1;  // the number of simulation words
    p->nBTLimit       =  1000;  // conflict limit at a node
    p->nSatVarMax     =  5000;  // the max number of SAT variables
    p->nCallsRecycle  =   100;  // calls to perform before recycling SAT solver
    p->fVerbose       =     0;  // verbose stats
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssc_ManStop( Ssc_Man_t * p )
{
    Vec_IntFreeP( &p->vFront );
    Vec_IntFreeP( &p->vFanins );
    Vec_IntFreeP( &p->vPattern );
    Vec_IntFreeP( &p->vDisPairs );
    Vec_IntFreeP( &p->vPivot );
    Vec_IntFreeP( &p->vId2Var );
    Vec_IntFreeP( &p->vVar2Id );
    if ( p->pSat ) sat_solver_delete( p->pSat );
    Gia_ManStopP( &p->pFraig );
    ABC_FREE( p );
}
Ssc_Man_t * Ssc_ManStart( Gia_Man_t * pAig, Gia_Man_t * pCare, Ssc_Pars_t * pPars )
{
    Ssc_Man_t * p;
    p = ABC_CALLOC( Ssc_Man_t, 1 );
    p->pPars  = pPars;
    p->pAig   = pAig;
    p->pCare  = pCare;
    p->pFraig = Gia_ManDupDfs( p->pCare );
    assert( p->pFraig->pHTable == NULL );
    assert( !Gia_ManHasDangling(p->pFraig) );
    Gia_ManInvertPos( p->pFraig );
    Ssc_ManStartSolver( p );
    if ( p->pSat == NULL )
    {
        printf( "Constraints are UNSAT after propagation.\n" );
        Ssc_ManStop( p );
        return (Ssc_Man_t *)(ABC_PTRINT_T)1;
    }
//    p->vPivot = Ssc_GiaFindPivotSim( p->pFraig );
//    Vec_IntFreeP( &p->vPivot  );
    p->vPivot = Ssc_ManFindPivotSat( p );
    if ( p->vPivot == (Vec_Int_t *)(ABC_PTRINT_T)1 )
    {
        printf( "Constraints are UNSAT.\n" );
        Ssc_ManStop( p );
        return (Ssc_Man_t *)(ABC_PTRINT_T)1;
    }
    if ( p->vPivot == NULL )
    {
        printf( "Conflict limit is reached while trying to find one SAT assignment.\n" );
        Ssc_ManStop( p );
        return NULL;
    }
    sat_solver_bookmark( p->pSat );
//    Vec_IntPrint( p->vPivot );
    Gia_ManSetPhasePattern( p->pAig, p->vPivot );
    Gia_ManSetPhasePattern( p->pCare, p->vPivot );
    if ( Gia_ManCheckCoPhase(p->pCare) )
    {
        printf( "Computed reference pattern violates %d constraints (this is a bug!).\n", Gia_ManCheckCoPhase(p->pCare) );
        Ssc_ManStop( p );
        return NULL;
    }
    // other things
    p->vDisPairs = Vec_IntAlloc( 100 );
    p->vPattern = Vec_IntAlloc( 100 );
    p->vFanins = Vec_IntAlloc( 100 );
    p->vFront = Vec_IntAlloc( 100 );
    Ssc_GiaClassesInit( pAig );
    // now it is ready for refinement using simulation 
    return p;
}
void Ssc_ManPrintStats( Ssc_Man_t * p )
{
    Abc_Print( 1, "Parameters: SimWords = %d. SatConfs = %d. SatVarMax = %d. CallsRec = %d. Verbose = %d.\n",
        p->pPars->nWords, p->pPars->nBTLimit, p->pPars->nSatVarMax, p->pPars->nCallsRecycle, p->pPars->fVerbose );
    Abc_Print( 1, "SAT calls : Total = %d. Proof = %d. Cex = %d. Undec = %d.\n",
        p->nSatCalls, p->nSatCallsUnsat, p->nSatCallsSat, p->nSatCallsUndec );
    Abc_Print( 1, "SAT solver: Vars = %d. Clauses = %d. Recycles = %d. Sim rounds = %d.\n",
        sat_solver_nvars(p->pSat), sat_solver_nclauses(p->pSat), p->nRecycles, p->nSimRounds );

    p->timeOther = p->timeTotal - p->timeSimInit - p->timeSimSat - p->timeCnfGen - p->timeSatSat - p->timeSatUnsat - p->timeSatUndec;
    ABC_PRTP( "Initialization ", p->timeSimInit,            p->timeTotal );
    ABC_PRTP( "SAT simulation ", p->timeSimSat,             p->timeTotal );
    ABC_PRTP( "CNF generation ", p->timeSimSat,             p->timeTotal );
    ABC_PRTP( "SAT solving    ", p->timeSat-p->timeCnfGen,  p->timeTotal );
    ABC_PRTP( "  unsat        ", p->timeSatUnsat,           p->timeTotal );
    ABC_PRTP( "  sat          ", p->timeSatSat,             p->timeTotal );
    ABC_PRTP( "  undecided    ", p->timeSatUndec,           p->timeTotal );
    ABC_PRTP( "Other          ", p->timeOther,              p->timeTotal );
    ABC_PRTP( "TOTAL          ", p->timeTotal,              p->timeTotal );
}

/**Function*************************************************************

  Synopsis    [Refine one class by resimulating one pattern.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssc_GiaSimulatePatternFraig_rec( Ssc_Man_t * p, int iFraigObj )
{
    Gia_Obj_t * pObj;
    int Res0, Res1;
    if ( Ssc_ObjSatVar(p, iFraigObj) )
        return sat_solver_var_value( p->pSat, Ssc_ObjSatVar(p, iFraigObj) );
    pObj = Gia_ManObj( p->pFraig, iFraigObj );
    assert( Gia_ObjIsAnd(pObj) );
    Res0 = Ssc_GiaSimulatePatternFraig_rec( p, Gia_ObjFaninId0(pObj, iFraigObj) );
    Res1 = Ssc_GiaSimulatePatternFraig_rec( p, Gia_ObjFaninId1(pObj, iFraigObj) );
    pObj->fMark0 = (Res0 ^ Gia_ObjFaninC0(pObj)) & (Res1 ^ Gia_ObjFaninC1(pObj));
    return pObj->fMark0;
}
int Ssc_GiaSimulatePattern_rec( Ssc_Man_t * p, Gia_Obj_t * pObj )
{
    int Res0, Res1;
    if ( Gia_ObjIsTravIdCurrent(p->pAig, pObj) )
        return pObj->fMark0;
    Gia_ObjSetTravIdCurrent(p->pAig, pObj);
    if ( ~pObj->Value )  // mapping into FRAIG exists - simulate FRAIG
    { 
        Res0 = Ssc_GiaSimulatePatternFraig_rec( p, Abc_Lit2Var(pObj->Value) );
        pObj->fMark0 = Res0 ^ Abc_LitIsCompl(pObj->Value);
    }
    else // mapping into FRAIG does not exist - simulate AIG
    {
        Res0 = Ssc_GiaSimulatePattern_rec( p, Gia_ObjFanin0(pObj) );
        Res1 = Ssc_GiaSimulatePattern_rec( p, Gia_ObjFanin1(pObj) );
        pObj->fMark0 = (Res0 ^ Gia_ObjFaninC0(pObj)) & (Res1 ^ Gia_ObjFaninC1(pObj));
    }
    return pObj->fMark0;
}
int Ssc_GiaResimulateOneClass( Ssc_Man_t * p, int iRepr, int iObj )
{
    int Ent, RetValue;
    assert( iRepr == Gia_ObjRepr(p->pAig, iObj) );
    assert( Gia_ObjIsHead( p->pAig, iRepr ) );
    // set bit-values at the nodes according to the counter-example
    Gia_ManIncrementTravId( p->pAig );
    Gia_ClassForEachObj( p->pAig, iRepr, Ent )
        Ssc_GiaSimulatePattern_rec( p, Gia_ManObj(p->pAig, Ent) );
    // refine one class using these bit-values
    RetValue = Ssc_GiaSimClassRefineOneBit( p->pAig, iRepr );
    // check that the candidate equivalence is indeed refined
    assert( iRepr != Gia_ObjRepr(p->pAig, iObj) );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Perform verification of conditional sweeping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssc_PerformVerification( Gia_Man_t * p0, Gia_Man_t * p1, Gia_Man_t * pC )
{
    int Status;
    Cec_ParCec_t ParsCec, * pPars = &ParsCec;
    // derive the OR of constraint outputs
    Gia_Man_t * pCond = Gia_ManDupAndOr( pC, Gia_ManPoNum(p0), 1, 0 );
    // derive F = F & !OR(c0, c1, c2, ...)
    Gia_Man_t * p0c = Gia_ManMiter( p0, pCond, 0, 0, 0, 1, 0 );
    // derive F = F & !OR(c0, c1, c2, ...)
    Gia_Man_t * p1c = Gia_ManMiter( p1, pCond, 0, 0, 0, 1, 0 );
    // derive dual-output miter
    Gia_Man_t * pMiter = Gia_ManMiter( p0c, p1c, 0, 1, 0, 0, 0 );
    Gia_ManStop( p0c );
    Gia_ManStop( p1c );
    Gia_ManStop( pCond );
    // run equivalence checking
    Cec_ManCecSetDefaultParams( pPars );
    Status = Cec_ManVerify( pMiter, pPars );
    Gia_ManStop( pMiter );
    // report the results
    if ( Status == 1 )
        printf( "Verification succeeded.\n" );
    else if ( Status == 0 )
        printf( "Verification failed.\n" );
    else if ( Status == -1 )
        printf( "Verification undecided.\n" );
    else assert( 0 );
    return Status;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Ssc_PerformSweepingInt( Gia_Man_t * pAig, Gia_Man_t * pCare, Ssc_Pars_t * pPars )
{
    Ssc_Man_t * p;
    Gia_Man_t * pResult, * pTemp;
    Gia_Obj_t * pObj, * pRepr;
    abctime clk, clkTotal = Abc_Clock();
    int i, fCompl, nRefined, status;
clk = Abc_Clock();
    assert( Gia_ManRegNum(pCare) == 0 );
    assert( Gia_ManCiNum(pAig) == Gia_ManCiNum(pCare) );
    assert( Gia_ManIsNormalized(pAig) );
    assert( Gia_ManIsNormalized(pCare) );
    // reset random numbers
    Gia_ManRandom( 1 );
    // sweeping manager
    p = Ssc_ManStart( pAig, pCare, pPars );
    if ( p == (Ssc_Man_t *)(ABC_PTRINT_T)1 ) // UNSAT
        return Gia_ManDupZero( pAig );
    if ( p == NULL ) // timeout
        return Gia_ManDup( pAig );
    if ( p->pPars->fVerbose )
        printf( "Care set produced %d hits out of %d.\n", Ssc_GiaEstimateCare(p->pFraig, 5), 640 );
    // perform simulation 
    while ( 1 ) 
    {
        // simulate care set
        Ssc_GiaRandomPiPattern( p->pFraig, 5, NULL );
        Ssc_GiaSimRound( p->pFraig );
        // transfer care patterns to user's AIG
        if ( !Ssc_GiaTransferPiPattern( pAig, p->pFraig, p->vPivot ) )
            break;
        // simulate the main AIG
        Ssc_GiaSimRound( pAig );
        nRefined = Ssc_GiaClassesRefine( pAig );
        if ( pPars->fVerbose ) 
            Gia_ManEquivPrintClasses( pAig, 0, 0 );
        if ( nRefined <= Gia_ManCandNum(pAig) / 100 )
            break;
    }
p->timeSimInit += Abc_Clock() - clk;

    // prepare user's AIG
    Gia_ManFillValue(pAig);
    Gia_ManConst0(pAig)->Value = 0;
    Gia_ManForEachCi( pAig, pObj, i )
        pObj->Value = Gia_Obj2Lit( p->pFraig, Gia_ManCi(p->pFraig, i) );
//    Gia_ManLevelNum(pAig);
    // prepare swept AIG (should be done after starting SAT solver in Ssc_ManStart)
    Gia_ManHashStart( p->pFraig );
    // perform sweeping
    Ssc_GiaResetPiPattern( pAig, pPars->nWords );
    Ssc_GiaSavePiPattern( pAig, p->vPivot );
    Gia_ManForEachCand( pAig, pObj, i )
    {
        if ( pAig->iPatsPi == 64 * pPars->nWords )
        {
clk = Abc_Clock();
            Ssc_GiaSimRound( pAig );
            Ssc_GiaClassesRefine( pAig );
            if ( pPars->fVerbose ) 
                Gia_ManEquivPrintClasses( pAig, 0, 0 );
            Ssc_GiaClassesCheckPairs( pAig, p->vDisPairs );
            Vec_IntClear( p->vDisPairs );
            // prepare next patterns
            Ssc_GiaResetPiPattern( pAig, pPars->nWords );
            Ssc_GiaSavePiPattern( pAig, p->vPivot );
p->timeSimSat += Abc_Clock() - clk;
//printf( "\n" );
        }
        if ( Gia_ObjIsAnd(pObj) )
            pObj->Value = Gia_ManHashAnd( p->pFraig, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        if ( !Gia_ObjHasRepr(pAig, i) )
            continue;
        pRepr = Gia_ObjReprObj(pAig, i);
        if ( (int)pObj->Value == Abc_LitNotCond( pRepr->Value, pRepr->fPhase ^ pObj->fPhase ) )
        {
            Gia_ObjSetProved( pAig, i );
            continue;
        }
        assert( Abc_Lit2Var(pRepr->Value) != Abc_Lit2Var(pObj->Value) );
        fCompl = pRepr->fPhase ^ pObj->fPhase ^ Abc_LitIsCompl(pRepr->Value) ^ Abc_LitIsCompl(pObj->Value);

        // perform SAT call
clk = Abc_Clock();
        p->nSatCalls++;
        status = Ssc_ManCheckEquivalence( p, Abc_Lit2Var(pRepr->Value), Abc_Lit2Var(pObj->Value), fCompl );
        if ( status == l_False )
        {
            p->nSatCallsUnsat++;
            pObj->Value = Abc_LitNotCond( pRepr->Value, pRepr->fPhase ^ pObj->fPhase );
            Gia_ObjSetProved( pAig, i );
        }
        else if ( status == l_True )
        {
            p->nSatCallsSat++;
            Ssc_GiaSavePiPattern( pAig, p->vPattern );
            Vec_IntPush( p->vDisPairs, Gia_ObjRepr(p->pAig, i) );
            Vec_IntPush( p->vDisPairs, i );
//            printf( "Try %2d and %2d: ", Gia_ObjRepr(p->pAig, i), i );
//            Vec_IntPrint( p->vPattern );
            if ( Gia_ObjRepr(p->pAig, i) > 0 )
                Ssc_GiaResimulateOneClass( p, Gia_ObjRepr(p->pAig, i), i );
        }
        else if ( status == l_Undef )
            p->nSatCallsUndec++;
        else assert( 0 );
p->timeSat += Abc_Clock() - clk;
    }
    if ( pAig->iPatsPi > 1 )
    {
clk = Abc_Clock();
        while ( pAig->iPatsPi < 64 * pPars->nWords )
            Ssc_GiaSavePiPattern( pAig, p->vPivot );
        Ssc_GiaSimRound( pAig );
        Ssc_GiaClassesRefine( pAig );
        if ( pPars->fVerbose ) 
            Gia_ManEquivPrintClasses( pAig, 0, 0 );
        Ssc_GiaClassesCheckPairs( pAig, p->vDisPairs );
        Vec_IntClear( p->vDisPairs );
p->timeSimSat += Abc_Clock() - clk;
    }
//    Gia_ManEquivPrintClasses( pAig, 1, 0 );
//    Gia_ManPrint( pAig );

    // generate the resulting AIG
    pResult = Gia_ManEquivReduce( pAig, 0, 0, 1, 0 );
    if ( pResult == NULL )
    {
        printf( "There is no equivalences.\n" );
        ABC_FREE( pAig->pReprs );
        ABC_FREE( pAig->pNexts );
        pResult = Gia_ManDup( pAig );
    }
    pResult = Gia_ManCleanup( pTemp = pResult );
    Gia_ManStop( pTemp );
    p->timeTotal = Abc_Clock() - clkTotal;
    if ( pPars->fVerbose )
        Ssc_ManPrintStats( p );
    Ssc_ManStop( p );
    // count the number of representatives
    if ( pPars->fVerbose ) 
    {
        Abc_Print( 1, "Reduction in AIG nodes:%8d  ->%8d (%6.2f %%).  ", 
            Gia_ManAndNum(pAig), Gia_ManAndNum(pResult), 
            100.0 - 100.0 * Gia_ManAndNum(pResult) / Gia_ManAndNum(pAig) );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clkTotal );
    }
    return pResult;
}
Gia_Man_t * Ssc_PerformSweeping( Gia_Man_t * pAig, Gia_Man_t * pCare, Ssc_Pars_t * pPars )
{
    Gia_Man_t * pResult = Ssc_PerformSweepingInt( pAig, pCare, pPars );
    if ( pPars->fVerify )
        Ssc_PerformVerification( pAig, pResult, pCare );
    return pResult;
}
Gia_Man_t * Ssc_PerformSweepingConstr( Gia_Man_t * p, Ssc_Pars_t * pPars )
{
    Gia_Man_t * pAig, * pCare, * pResult;
    int i;
    if ( pPars->fVerbose )
        Abc_Print( 0, "SAT sweeping AIG with %d constraints.\n", p->nConstrs );
    if ( p->nConstrs == 0 )
    {
        pAig = Gia_ManDup( p );
        pCare = Gia_ManStart( Gia_ManCiNum(p) + 2 );
        pCare->pName = Abc_UtilStrsav( "care" );
        for ( i = 0; i < Gia_ManCiNum(p); i++ )
            Gia_ManAppendCi( pCare );
        Gia_ManAppendCo( pCare, 0 );
    }
    else
    {
        Vec_Int_t * vOuts = Vec_IntStartNatural( Gia_ManPoNum(p) );
        pAig = Gia_ManDupCones( p, Vec_IntArray(vOuts), Gia_ManPoNum(p) - p->nConstrs, 0 );
        pCare = Gia_ManDupCones( p, Vec_IntArray(vOuts) + Gia_ManPoNum(p) - p->nConstrs, p->nConstrs, 0 );
        Vec_IntFree( vOuts );
    }
    if ( pPars->fVerbose )
    {
        printf( "User AIG: " );
        Gia_ManPrintStats( pAig, NULL );
        printf( "Care AIG: " );
        Gia_ManPrintStats( pCare, NULL );
    }

    pAig = Gia_ManDupLevelized( pResult = pAig );
    Gia_ManStop( pResult );
    pResult = Ssc_PerformSweeping( pAig, pCare, pPars );
    if ( pPars->fAppend )
    {
        Gia_ManDupAppendShare( pResult, pCare );
        pResult->nConstrs = Gia_ManPoNum(pCare);
    }
    Gia_ManStop( pAig );
    Gia_ManStop( pCare );
    return pResult;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

