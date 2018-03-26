/**CFile****************************************************************

  FileName    [sswConstr.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Inductive prover with constraints.]

  Synopsis    [One round of SAT sweeping.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 1, 2008.]

  Revision    [$Id: sswConstr.c,v 1.00 2008/09/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sswInt.h"
#include "sat/cnf/cnf.h"
#include "misc/bar/bar.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Constructs initialized timeframes with constraints as POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Ssw_FramesWithConstraints( Aig_Man_t * p, int nFrames )
{
    Aig_Man_t * pFrames;
    Aig_Obj_t * pObj, * pObjLi, * pObjLo;
    int i, f;
//    assert( Saig_ManConstrNum(p) > 0 );
    assert( Aig_ManRegNum(p) > 0 );
    assert( Aig_ManRegNum(p) < Aig_ManCiNum(p) );
    // start the fraig package
    pFrames = Aig_ManStart( Aig_ManObjNumMax(p) * nFrames );
    // create latches for the first frame
    Saig_ManForEachLo( p, pObj, i )
        Aig_ObjSetCopy( pObj, Aig_ManConst0(pFrames) );
    // add timeframes
    for ( f = 0; f < nFrames; f++ )
    {
        // map constants and PIs
        Aig_ObjSetCopy( Aig_ManConst1(p), Aig_ManConst1(pFrames) );
        Saig_ManForEachPi( p, pObj, i )
            Aig_ObjSetCopy( pObj, Aig_ObjCreateCi(pFrames) );
        // add internal nodes of this frame
        Aig_ManForEachNode( p, pObj, i )
            Aig_ObjSetCopy( pObj, Aig_And( pFrames, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) ) );
        // transfer to the primary output
        Aig_ManForEachCo( p, pObj, i )
            Aig_ObjSetCopy( pObj, Aig_ObjChild0Copy(pObj) );
        // create constraint outputs
        Saig_ManForEachPo( p, pObj, i )
        {
            if ( i < Saig_ManPoNum(p) - Saig_ManConstrNum(p) )
                continue;
            Aig_ObjCreateCo( pFrames, Aig_Not( Aig_ObjCopy(pObj) ) );
        }
        // transfer latch inputs to the latch outputs
        Saig_ManForEachLiLo( p, pObjLi, pObjLo, i )
            Aig_ObjSetCopy( pObjLo, Aig_ObjCopy(pObjLi) );
    }
    // remove dangling nodes
    Aig_ManCleanup( pFrames );
    return pFrames;
}

/**Function*************************************************************

  Synopsis    [Finds one satisfiable assignment of the timeframes.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_ManSetConstrPhases( Aig_Man_t * p, int nFrames, Vec_Int_t ** pvInits )
{
    Aig_Man_t * pFrames;
    sat_solver * pSat;
    Cnf_Dat_t * pCnf;
    Aig_Obj_t * pObj;
    int i, RetValue;
    if ( pvInits )
        *pvInits = NULL;
//    assert( p->nConstrs > 0 );
    // derive the timeframes
    pFrames = Ssw_FramesWithConstraints( p, nFrames );
    // create CNF
    pCnf = Cnf_Derive( pFrames, 0 );
    // create SAT solver
    pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
    if ( pSat == NULL )
    {
        Cnf_DataFree( pCnf );
        Aig_ManStop( pFrames );
        return 1;
    }
    // solve
    RetValue = sat_solver_solve( pSat, NULL, NULL,
        (ABC_INT64_T)1000000, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
    if ( RetValue == l_True && pvInits )
    {
        *pvInits = Vec_IntAlloc( 1000 );
        Aig_ManForEachCi( pFrames, pObj, i )
            Vec_IntPush( *pvInits, sat_solver_var_value(pSat, pCnf->pVarNums[Aig_ObjId(pObj)]) );

//        Aig_ManForEachCi( pFrames, pObj, i )
//            Abc_Print( 1, "%d", Vec_IntEntry(*pvInits, i) );
//        Abc_Print( 1, "\n" );
    }
    sat_solver_delete( pSat );
    Cnf_DataFree( pCnf );
    Aig_ManStop( pFrames );
    if ( RetValue == l_False )
        return 1;
    if ( RetValue == l_True )
        return 0;
    return -1;
}

/**Function*************************************************************

  Synopsis    [Performs fraiging for one node.]

  Description [Returns the fraiged node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_ManSetConstrPhases_( Aig_Man_t * p, int nFrames, Vec_Int_t ** pvInits )
{
    Vec_Int_t * vLits;
    sat_solver * pSat;
    Cnf_Dat_t * pCnf;
    Aig_Obj_t * pObj;
    int i, f, iVar, RetValue, nRegs;
    if ( pvInits )
        *pvInits = NULL;
    assert( p->nConstrs > 0 );
    // create CNF
    nRegs = p->nRegs; p->nRegs = 0;
    pCnf = Cnf_Derive( p, Aig_ManCoNum(p) );
    p->nRegs = nRegs;
    // create SAT solver
    pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, nFrames, 0 );
    assert( pSat->size == nFrames * pCnf->nVars );
    // collect constraint literals
    vLits = Vec_IntAlloc( 100 );
    Saig_ManForEachLo( p, pObj, i )
    {
        assert( pCnf->pVarNums[Aig_ObjId(pObj)] >= 0 );
        Vec_IntPush( vLits, toLitCond(pCnf->pVarNums[Aig_ObjId(pObj)], 1) );
    }
    for ( f = 0; f < nFrames; f++ )
    {
        Saig_ManForEachPo( p, pObj, i )
        {
            if ( i < Saig_ManPoNum(p) - Saig_ManConstrNum(p) )
                continue;
            assert( pCnf->pVarNums[Aig_ObjId(pObj)] >= 0 );
            iVar = pCnf->pVarNums[Aig_ObjId(pObj)] + pCnf->nVars*f;
            Vec_IntPush( vLits, toLitCond(iVar, 1) );
        }
    }
    RetValue = sat_solver_solve( pSat, (int *)Vec_IntArray(vLits),
        (int *)Vec_IntArray(vLits) + Vec_IntSize(vLits),
        (ABC_INT64_T)1000000, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
    if ( RetValue == l_True && pvInits )
    {
        *pvInits = Vec_IntAlloc( 1000 );
        for ( f = 0; f < nFrames; f++ )
        {
            Saig_ManForEachPi( p, pObj, i )
            {
                iVar = pCnf->pVarNums[Aig_ObjId(pObj)] + pCnf->nVars*f;
                Vec_IntPush( *pvInits, sat_solver_var_value(pSat, iVar) );
            }
        }
    }
    sat_solver_delete( pSat );
    Vec_IntFree( vLits );
    Cnf_DataFree( pCnf );
    if ( RetValue == l_False )
        return 1;
    if ( RetValue == l_True )
        return 0;
    return -1;
}

/**Function*************************************************************

  Synopsis    [Performs fraiging for one node.]

  Description [Returns the fraiged node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_ManPrintPolarity( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i;
    Aig_ManForEachObj( p, pObj, i )
        Abc_Print( 1, "%d", pObj->fPhase );
    Abc_Print( 1, "\n" );
}

/**Function*************************************************************

  Synopsis    [Performs fraiging for one node.]

  Description [Returns the fraiged node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_ManRefineByConstrSim( Ssw_Man_t * p )
{
    Aig_Obj_t * pObj, * pObjLi;
    int f, i, iLits, RetValue1, RetValue2;
    int nFrames = Vec_IntSize(p->vInits) / Saig_ManPiNum(p->pAig);
    assert( Vec_IntSize(p->vInits) % Saig_ManPiNum(p->pAig) == 0 );
    // assign register outputs
    Saig_ManForEachLi( p->pAig, pObj, i )
        pObj->fMarkB = 0;
    // simulate the timeframes
    iLits = 0;
    for ( f = 0; f < nFrames; f++ )
    {
        // set the PI simulation information
        Aig_ManConst1(p->pAig)->fMarkB = 1;
        Saig_ManForEachPi( p->pAig, pObj, i )
            pObj->fMarkB = Vec_IntEntry( p->vInits, iLits++ );
        Saig_ManForEachLiLo( p->pAig, pObjLi, pObj, i )
            pObj->fMarkB = pObjLi->fMarkB;
        // simulate internal nodes
        Aig_ManForEachNode( p->pAig, pObj, i )
            pObj->fMarkB = ( Aig_ObjFanin0(pObj)->fMarkB ^ Aig_ObjFaninC0(pObj) )
                         & ( Aig_ObjFanin1(pObj)->fMarkB ^ Aig_ObjFaninC1(pObj) );
        // assign the COs
        Aig_ManForEachCo( p->pAig, pObj, i )
            pObj->fMarkB = ( Aig_ObjFanin0(pObj)->fMarkB ^ Aig_ObjFaninC0(pObj) );
        // check the outputs
        Saig_ManForEachPo( p->pAig, pObj, i )
        {
            if ( i < Saig_ManPoNum(p->pAig) - Saig_ManConstrNum(p->pAig) )
            {
                if ( pObj->fMarkB && Saig_ManConstrNum(p->pAig) )
                    Abc_Print( 1, "output %d failed in frame %d.\n", i, f );
            }
            else
            {
                if ( pObj->fMarkB && Saig_ManConstrNum(p->pAig) )
                    Abc_Print( 1, "constraint %d failed in frame %d.\n", i, f );
            }
        }
        // transfer
        if ( f == 0 )
        { // copy markB into phase
            Aig_ManForEachObj( p->pAig, pObj, i )
                pObj->fPhase = pObj->fMarkB;
        }
        else
        { // refine classes
            RetValue1 = Ssw_ClassesRefineConst1( p->ppClasses, 0 );
            RetValue2 = Ssw_ClassesRefine( p->ppClasses, 0 );
        }
    }
    assert( iLits == Vec_IntSize(p->vInits) );
}


/**Function*************************************************************

  Synopsis    [Performs fraiging for one node.]

  Description [Returns the fraiged node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_ManSweepNodeConstr( Ssw_Man_t * p, Aig_Obj_t * pObj, int f, int fBmc )
{
    Aig_Obj_t * pObjRepr, * pObjFraig, * pObjFraig2, * pObjReprFraig;
    int RetValue;
    // get representative of this class
    pObjRepr = Aig_ObjRepr( p->pAig, pObj );
    if ( pObjRepr == NULL )
        return 0;
    // get the fraiged node
    pObjFraig = Ssw_ObjFrame( p, pObj, f );
    // get the fraiged representative
    pObjReprFraig = Ssw_ObjFrame( p, pObjRepr, f );
    // check if constant 0 pattern distinquishes these nodes
    assert( pObjFraig != NULL && pObjReprFraig != NULL );
    assert( (pObj->fPhase == pObjRepr->fPhase) == (Aig_ObjPhaseReal(pObjFraig) == Aig_ObjPhaseReal(pObjReprFraig)) );
    // if the fraiged nodes are the same, return
    if ( Aig_Regular(pObjFraig) == Aig_Regular(pObjReprFraig) )
        return 0;
    // call equivalence checking
    if ( Aig_Regular(pObjFraig) != Aig_ManConst1(p->pFrames) )
        RetValue = Ssw_NodesAreEquiv( p, Aig_Regular(pObjReprFraig), Aig_Regular(pObjFraig) );
    else
        RetValue = Ssw_NodesAreEquiv( p, Aig_Regular(pObjFraig), Aig_Regular(pObjReprFraig) );
    if ( RetValue == 1 )  // proved equivalent
    {
        pObjFraig2 = Aig_NotCond( pObjReprFraig, pObj->fPhase ^ pObjRepr->fPhase );
        Ssw_ObjSetFrame( p, pObj, f, pObjFraig2 );
        return 0;
    }
    if ( RetValue == -1 ) // timed out
    {
        Ssw_ClassesRemoveNode( p->ppClasses, pObj );
        return 1;
    }
    // disproved equivalence
    Ssw_SmlSavePatternAig( p, f );
    Ssw_ManResimulateBit( p, pObj, pObjRepr );
    assert( Aig_ObjRepr( p->pAig, pObj ) != pObjRepr );
    if ( Aig_ObjRepr( p->pAig, pObj ) == pObjRepr )
    {
        Abc_Print( 1, "Ssw_ManSweepNodeConstr(): Failed to refine representative.\n" );
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Performs fraiging for the internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Ssw_ManSweepBmcConstr_rec( Ssw_Man_t * p, Aig_Obj_t * pObj, int f )
{
    Aig_Obj_t * pObjNew, * pObjLi;
    pObjNew = Ssw_ObjFrame( p, pObj, f );
    if ( pObjNew )
        return pObjNew;
    assert( !Saig_ObjIsPi(p->pAig, pObj) );
    if ( Saig_ObjIsLo(p->pAig, pObj) )
    {
        assert( f > 0 );
        pObjLi  = Saig_ObjLoToLi( p->pAig, pObj );
        pObjNew = Ssw_ManSweepBmcConstr_rec( p, Aig_ObjFanin0(pObjLi), f-1 );
        pObjNew = Aig_NotCond( pObjNew, Aig_ObjFaninC0(pObjLi) );
    }
    else
    {
        assert( Aig_ObjIsNode(pObj) );
        Ssw_ManSweepBmcConstr_rec( p, Aig_ObjFanin0(pObj), f );
        Ssw_ManSweepBmcConstr_rec( p, Aig_ObjFanin1(pObj), f );
        pObjNew = Aig_And( p->pFrames, Ssw_ObjChild0Fra(p, pObj, f), Ssw_ObjChild1Fra(p, pObj, f) );
    }
    Ssw_ObjSetFrame( p, pObj, f, pObjNew );
    assert( pObjNew != NULL );
    return pObjNew;
}

/**Function*************************************************************

  Synopsis    [Performs fraiging for the internal nodes.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_ManSweepBmcConstr_old( Ssw_Man_t * p )
{
    Bar_Progress_t * pProgress = NULL;
    Aig_Obj_t * pObj, * pObjNew, * pObjLi, * pObjLo;
    int i, f, iLits;
    abctime clk;
clk = Abc_Clock();

    // start initialized timeframes
    p->pFrames = Aig_ManStart( Aig_ManObjNumMax(p->pAig) * p->pPars->nFramesK );
    Saig_ManForEachLo( p->pAig, pObj, i )
        Ssw_ObjSetFrame( p, pObj, 0, Aig_ManConst0(p->pFrames) );

    // build the constraint outputs
    iLits = 0;
    for ( f = 0; f < p->pPars->nFramesK; f++ )
    {
        // map constants and PIs
        Ssw_ObjSetFrame( p, Aig_ManConst1(p->pAig), f, Aig_ManConst1(p->pFrames) );
        Saig_ManForEachPi( p->pAig, pObj, i )
        {
            pObjNew = Aig_ObjCreateCi(p->pFrames);
            pObjNew->fPhase = Vec_IntEntry( p->vInits, iLits++ );
            Ssw_ObjSetFrame( p, pObj, f, pObjNew );
        }
        // build the constraint cones
        Saig_ManForEachPo( p->pAig, pObj, i )
        {
            if ( i < Saig_ManPoNum(p->pAig) - Saig_ManConstrNum(p->pAig) )
                continue;
            pObjNew = Ssw_ManSweepBmcConstr_rec( p, Aig_ObjFanin0(pObj), f );
            pObjNew = Aig_NotCond( pObjNew, Aig_ObjFaninC0(pObj) );
            if ( Aig_Regular(pObjNew) == Aig_ManConst1(p->pFrames) )
            {
                assert( Aig_IsComplement(pObjNew) );
                continue;
            }
            Ssw_NodesAreConstrained( p, pObjNew, Aig_ManConst0(p->pFrames) );
        }
    }
    assert( Vec_IntSize(p->vInits) == iLits + Saig_ManPiNum(p->pAig) );

    // sweep internal nodes
    p->fRefined = 0;
    if ( p->pPars->fVerbose )
        pProgress = Bar_ProgressStart( stdout, Aig_ManObjNumMax(p->pAig) * p->pPars->nFramesK );
    for ( f = 0; f < p->pPars->nFramesK; f++ )
    {
        // sweep internal nodes
        Aig_ManForEachNode( p->pAig, pObj, i )
        {
            if ( p->pPars->fVerbose )
                Bar_ProgressUpdate( pProgress, Aig_ManObjNumMax(p->pAig) * f + i, NULL );
            pObjNew = Aig_And( p->pFrames, Ssw_ObjChild0Fra(p, pObj, f), Ssw_ObjChild1Fra(p, pObj, f) );
            Ssw_ObjSetFrame( p, pObj, f, pObjNew );
            p->fRefined |= Ssw_ManSweepNodeConstr( p, pObj, f, 1 );
        }
        // quit if this is the last timeframe
        if ( f == p->pPars->nFramesK - 1 )
            break;
        // transfer latch input to the latch outputs
        Aig_ManForEachCo( p->pAig, pObj, i )
            Ssw_ObjSetFrame( p, pObj, f, Ssw_ObjChild0Fra(p, pObj, f) );
        // build logic cones for register outputs
        Saig_ManForEachLiLo( p->pAig, pObjLi, pObjLo, i )
        {
            pObjNew = Ssw_ObjFrame( p, pObjLi, f );
            Ssw_ObjSetFrame( p, pObjLo, f+1, pObjNew );
            Ssw_CnfNodeAddToSolver( p->pMSat, Aig_Regular(pObjNew) );//
        }
    }
    if ( p->pPars->fVerbose )
        Bar_ProgressStop( pProgress );

    // cleanup
//    Ssw_ClassesCheck( p->ppClasses );
p->timeBmc += Abc_Clock() - clk;
    return p->fRefined;
}

/**Function*************************************************************

  Synopsis    [Performs fraiging for the internal nodes.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_ManSweepBmcConstr( Ssw_Man_t * p )
{
    Aig_Obj_t * pObj, * pObjNew, * pObjLi, * pObjLo;
    int i, f, iLits;
    abctime clk;
clk = Abc_Clock();

    // start initialized timeframes
    p->pFrames = Aig_ManStart( Aig_ManObjNumMax(p->pAig) * p->pPars->nFramesK );
    Saig_ManForEachLo( p->pAig, pObj, i )
        Ssw_ObjSetFrame( p, pObj, 0, Aig_ManConst0(p->pFrames) );

    // build the constraint outputs
    iLits = 0;
    p->fRefined = 0;
    for ( f = 0; f < p->pPars->nFramesK; f++ )
    {
        // map constants and PIs
        Ssw_ObjSetFrame( p, Aig_ManConst1(p->pAig), f, Aig_ManConst1(p->pFrames) );
        Saig_ManForEachPi( p->pAig, pObj, i )
        {
            pObjNew = Aig_ObjCreateCi(p->pFrames);
            pObjNew->fPhase = Vec_IntEntry( p->vInits, iLits++ );
            Ssw_ObjSetFrame( p, pObj, f, pObjNew );
        }
        // build the constraint cones
        Saig_ManForEachPo( p->pAig, pObj, i )
        {
            if ( i < Saig_ManPoNum(p->pAig) - Saig_ManConstrNum(p->pAig) )
                continue;
            pObjNew = Ssw_ManSweepBmcConstr_rec( p, Aig_ObjFanin0(pObj), f );
            pObjNew = Aig_NotCond( pObjNew, Aig_ObjFaninC0(pObj) );
            if ( Aig_Regular(pObjNew) == Aig_ManConst1(p->pFrames) )
            {
                assert( Aig_IsComplement(pObjNew) );
                continue;
            }
            Ssw_NodesAreConstrained( p, pObjNew, Aig_ManConst0(p->pFrames) );
        }        
        // sweep flops
        Saig_ManForEachLo( p->pAig, pObj, i )
            p->fRefined |= Ssw_ManSweepNodeConstr( p, pObj, f, 1 );
        // sweep internal nodes
        Aig_ManForEachNode( p->pAig, pObj, i )
        {
            pObjNew = Aig_And( p->pFrames, Ssw_ObjChild0Fra(p, pObj, f), Ssw_ObjChild1Fra(p, pObj, f) );
            Ssw_ObjSetFrame( p, pObj, f, pObjNew );
            p->fRefined |= Ssw_ManSweepNodeConstr( p, pObj, f, 1 );
        }
        // quit if this is the last timeframe
        if ( f == p->pPars->nFramesK - 1 )
            break;
        // transfer latch input to the latch outputs
        Aig_ManForEachCo( p->pAig, pObj, i )
            Ssw_ObjSetFrame( p, pObj, f, Ssw_ObjChild0Fra(p, pObj, f) );
        // build logic cones for register outputs
        Saig_ManForEachLiLo( p->pAig, pObjLi, pObjLo, i )
        {
            pObjNew = Ssw_ObjFrame( p, pObjLi, f );
            Ssw_ObjSetFrame( p, pObjLo, f+1, pObjNew );
            Ssw_CnfNodeAddToSolver( p->pMSat, Aig_Regular(pObjNew) );//
        }
    }
    assert( Vec_IntSize(p->vInits) == iLits + Saig_ManPiNum(p->pAig) );

    // cleanup
//    Ssw_ClassesCheck( p->ppClasses );
p->timeBmc += Abc_Clock() - clk;
    return p->fRefined;
}




/**Function*************************************************************

  Synopsis    [Performs fraiging for the internal nodes.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Ssw_FramesWithClasses_rec( Ssw_Man_t * p, Aig_Obj_t * pObj, int f )
{
    Aig_Obj_t * pObjNew, * pObjLi;
    pObjNew = Ssw_ObjFrame( p, pObj, f );
    if ( pObjNew )
        return pObjNew;
    assert( !Saig_ObjIsPi(p->pAig, pObj) );
    if ( Saig_ObjIsLo(p->pAig, pObj) )
    {
        assert( f > 0 );
        pObjLi  = Saig_ObjLoToLi( p->pAig, pObj );
        pObjNew = Ssw_FramesWithClasses_rec( p, Aig_ObjFanin0(pObjLi), f-1 );
        pObjNew = Aig_NotCond( pObjNew, Aig_ObjFaninC0(pObjLi) );
    }
    else
    {
        assert( Aig_ObjIsNode(pObj) );
        Ssw_FramesWithClasses_rec( p, Aig_ObjFanin0(pObj), f );
        Ssw_FramesWithClasses_rec( p, Aig_ObjFanin1(pObj), f );
        pObjNew = Aig_And( p->pFrames, Ssw_ObjChild0Fra(p, pObj, f), Ssw_ObjChild1Fra(p, pObj, f) );
    }
    Ssw_ObjSetFrame( p, pObj, f, pObjNew );
    assert( pObjNew != NULL );
    return pObjNew;
}


/**Function*************************************************************

  Synopsis    [Performs fraiging for the internal nodes.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_ManSweepConstr( Ssw_Man_t * p )
{
    Bar_Progress_t * pProgress = NULL;
    Aig_Obj_t * pObj, * pObj2, * pObjNew;
    int nConstrPairs, i, f, iLits;
    abctime clk;
//Ssw_ManPrintPolarity( p->pAig );

    // perform speculative reduction
clk = Abc_Clock();
    // create timeframes
    p->pFrames = Ssw_FramesWithClasses( p );
    // add constants
    nConstrPairs = Aig_ManCoNum(p->pFrames)-Aig_ManRegNum(p->pAig);
    assert( (nConstrPairs & 1) == 0 );
    for ( i = 0; i < nConstrPairs; i += 2 )
    {
        pObj  = Aig_ManCo( p->pFrames, i   );
        pObj2 = Aig_ManCo( p->pFrames, i+1 );
        Ssw_NodesAreConstrained( p, Aig_ObjChild0(pObj), Aig_ObjChild0(pObj2) );
    }
    // build logic cones for register inputs
    for ( i = 0; i < Aig_ManRegNum(p->pAig); i++ )
    {
        pObj  = Aig_ManCo( p->pFrames, nConstrPairs + i );
        Ssw_CnfNodeAddToSolver( p->pMSat, Aig_ObjFanin0(pObj) );//
    }

    // map constants and PIs of the last frame
    f = p->pPars->nFramesK;
//    iLits = 0;
    iLits = f * Saig_ManPiNum(p->pAig);
    Ssw_ObjSetFrame( p, Aig_ManConst1(p->pAig), f, Aig_ManConst1(p->pFrames) );
    Saig_ManForEachPi( p->pAig, pObj, i )
    {
        pObjNew = Aig_ObjCreateCi(p->pFrames);
        pObjNew->fPhase = (p->vInits != NULL) && Vec_IntEntry(p->vInits, iLits++);
        Ssw_ObjSetFrame( p, pObj, f, pObjNew );
    }
    assert( Vec_IntSize(p->vInits) == iLits );
p->timeReduce += Abc_Clock() - clk;

    // add constraints to all timeframes
    for ( f = 0; f <= p->pPars->nFramesK; f++ )
    {
        Saig_ManForEachPo( p->pAig, pObj, i )
        {
            if ( i < Saig_ManPoNum(p->pAig) - Saig_ManConstrNum(p->pAig) )
                continue;
            Ssw_FramesWithClasses_rec( p, Aig_ObjFanin0(pObj), f );
//            if ( Aig_Regular(Ssw_ObjChild0Fra(p,pObj,f)) == Aig_ManConst1(p->pFrames) )
            if ( Ssw_ObjChild0Fra(p,pObj,f) == Aig_ManConst0(p->pFrames) )
                continue;
            assert( Ssw_ObjChild0Fra(p,pObj,f) != Aig_ManConst1(p->pFrames) );
            if ( Ssw_ObjChild0Fra(p,pObj,f) == Aig_ManConst1(p->pFrames) )
            {
                Abc_Print( 1, "Polarity violation.\n" );
                continue;
            }
            Ssw_NodesAreConstrained( p, Ssw_ObjChild0Fra(p,pObj,f), Aig_ManConst0(p->pFrames) );
        }
    }
    f = p->pPars->nFramesK;
    // clean the solver
    sat_solver_simplify( p->pMSat->pSat );


    // sweep internal nodes
    p->fRefined = 0;
    Ssw_ClassesClearRefined( p->ppClasses );
    if ( p->pPars->fVerbose )
        pProgress = Bar_ProgressStart( stdout, Aig_ManObjNumMax(p->pAig) );
    Aig_ManForEachObj( p->pAig, pObj, i )
    {
        if ( p->pPars->fVerbose )
            Bar_ProgressUpdate( pProgress, i, NULL );
        if ( Saig_ObjIsLo(p->pAig, pObj) )
            p->fRefined |= Ssw_ManSweepNodeConstr( p, pObj, f, 0 );
        else if ( Aig_ObjIsNode(pObj) )
        {
            pObjNew = Aig_And( p->pFrames, Ssw_ObjChild0Fra(p, pObj, f), Ssw_ObjChild1Fra(p, pObj, f) );
            Ssw_ObjSetFrame( p, pObj, f, pObjNew );
            p->fRefined |= Ssw_ManSweepNodeConstr( p, pObj, f, 0 );
        }
    }
    if ( p->pPars->fVerbose )
        Bar_ProgressStop( pProgress );
    // cleanup
//    Ssw_ClassesCheck( p->ppClasses );
    return p->fRefined;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END
