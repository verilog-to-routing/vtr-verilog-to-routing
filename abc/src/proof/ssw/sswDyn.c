/**CFile****************************************************************

  FileName    [sswDyn.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Inductive prover with constraints.]

  Synopsis    [Dynamic loading of constraints.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 1, 2008.]

  Revision    [$Id: sswDyn.c,v 1.00 2008/09/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sswInt.h"
#include "misc/bar/bar.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Label PIs nodes of the frames corresponding to PIs of AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_ManLabelPiNodes( Ssw_Man_t * p )
{
    Aig_Obj_t * pObj, * pObjFrames;
    int f, i;
    Aig_ManConst1( p->pFrames )->fMarkA = 1;
    Aig_ManConst1( p->pFrames )->fMarkB = 1;
    for ( f = 0; f < p->nFrames; f++ )
    {
        Saig_ManForEachPi( p->pAig, pObj, i )
        {
            pObjFrames = Ssw_ObjFrame( p, pObj, f );
            assert( Aig_ObjIsCi(pObjFrames) );
            assert( pObjFrames->fMarkB == 0 );
            pObjFrames->fMarkA = 1;
            pObjFrames->fMarkB = 1;
        }
    }
}

/**Function*************************************************************

  Synopsis    [Collects new POs in p->vNewPos.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_ManCollectPis_rec( Aig_Obj_t * pObj, Vec_Ptr_t * vNewPis )
{
    assert( !Aig_IsComplement(pObj) );
    if ( pObj->fMarkA )
        return;
    pObj->fMarkA = 1;
    if ( Aig_ObjIsCi(pObj) )
    {
        Vec_PtrPush( vNewPis, pObj );
        return;
    }
    assert( Aig_ObjIsNode(pObj) );
    Ssw_ManCollectPis_rec( Aig_ObjFanin0(pObj), vNewPis );
    Ssw_ManCollectPis_rec( Aig_ObjFanin1(pObj), vNewPis );
}

/**Function*************************************************************

  Synopsis    [Collects new POs in p->vNewPos.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_ManCollectPos_rec( Ssw_Man_t * p, Aig_Obj_t * pObj, Vec_Int_t * vNewPos )
{
    Aig_Obj_t * pFanout;
    int iFanout = -1, i;
    assert( !Aig_IsComplement(pObj) );
    if ( pObj->fMarkB )
        return;
    pObj->fMarkB = 1;
    if ( pObj->Id > p->nSRMiterMaxId )
        return;
    if ( Aig_ObjIsCo(pObj) )
    {
        // skip if it is a register input PO
        if ( Aig_ObjCioId(pObj) >= Aig_ManCoNum(p->pFrames)-Aig_ManRegNum(p->pAig) )
            return;
        // add the number of this constraint
        Vec_IntPush( vNewPos, Aig_ObjCioId(pObj)/2 );
        return;
    }
    // visit the fanouts
    assert( p->pFrames->pFanData != NULL );
    Aig_ObjForEachFanout( p->pFrames, pObj, pFanout, iFanout, i )
        Ssw_ManCollectPos_rec( p, pFanout, vNewPos );
}

/**Function*************************************************************

  Synopsis    [Loads logic cones and relevant constraints.]

  Description [Both pRepr and pObj are objects of the AIG.
  The result is the current SAT solver loaded with the logic cones
  for pRepr and pObj corresponding to them in the frames,
  as well as all the relevant constraints.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_ManLoadSolver( Ssw_Man_t * p, Aig_Obj_t * pRepr, Aig_Obj_t * pObj )
{
    Aig_Obj_t * pObjFrames, * pReprFrames;
    Aig_Obj_t * pTemp, * pObj0, * pObj1;
    int i, iConstr, RetValue;

    assert( pRepr != pObj );
    // get the corresponding frames nodes
    pReprFrames = Aig_Regular( Ssw_ObjFrame( p, pRepr, p->pPars->nFramesK ) );
    pObjFrames  = Aig_Regular( Ssw_ObjFrame( p, pObj,  p->pPars->nFramesK ) );
    assert( pReprFrames != pObjFrames );
 /*
    // compute the AIG support
    Vec_PtrClear( p->vNewLos );
    Ssw_ManCollectPis_rec( pRepr, p->vNewLos );
    Ssw_ManCollectPis_rec( pObj,  p->vNewLos );
    // add logic cones for register outputs
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vNewLos, pTemp, i )
    {
        pObj0 = Aig_Regular( Ssw_ObjFrame( p, pTemp, p->pPars->nFramesK ) );
        Ssw_CnfNodeAddToSolver( p->pMSat, pObj0 );
    }
*/
    // add cones for the nodes
    Ssw_CnfNodeAddToSolver( p->pMSat, pReprFrames );
    Ssw_CnfNodeAddToSolver( p->pMSat, pObjFrames );

    // compute the frames support
    Vec_PtrClear( p->vNewLos );
    Ssw_ManCollectPis_rec( pReprFrames, p->vNewLos );
    Ssw_ManCollectPis_rec( pObjFrames,  p->vNewLos );
    // these nodes include both nodes corresponding to PIs and LOs
    // (the nodes corresponding to PIs should be labeled with fMarkB!)

    // collect the related constraint POs
    Vec_IntClear( p->vNewPos );
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vNewLos, pTemp, i )
        Ssw_ManCollectPos_rec( p, pTemp, p->vNewPos );
    // check if the corresponding pairs are added
    Vec_IntForEachEntry( p->vNewPos, iConstr, i )
    {
        pObj0 = Aig_ManCo( p->pFrames, 2*iConstr   );
        pObj1 = Aig_ManCo( p->pFrames, 2*iConstr+1 );
//        if ( pObj0->fMarkB && pObj1->fMarkB )
        if ( pObj0->fMarkB || pObj1->fMarkB )
        {
            pObj0->fMarkB = 1;
            pObj1->fMarkB = 1;
            Ssw_NodesAreConstrained( p, Aig_ObjChild0(pObj0), Aig_ObjChild0(pObj1) );
        }
    }
    if ( p->pMSat->pSat->qtail != p->pMSat->pSat->qhead )
    {
        RetValue = sat_solver_simplify(p->pMSat->pSat);
        assert( RetValue != 0 );
    }
}

/**Function*************************************************************

  Synopsis    [Tranfers simulation information from FRAIG to AIG.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_ManSweepTransferDyn( Ssw_Man_t * p )
{
    Aig_Obj_t * pObj, * pObjFraig;
    unsigned * pInfo;
    int i, f, nFrames;

    // transfer simulation information
    Aig_ManForEachCi( p->pAig, pObj, i )
    {
        pObjFraig = Ssw_ObjFrame( p, pObj, 0 );
        if ( pObjFraig == Aig_ManConst0(p->pFrames) )
        {
            Ssw_SmlObjAssignConst( p->pSml, pObj, 0, 0 );
            continue;
        }
        assert( !Aig_IsComplement(pObjFraig) );
        assert( Aig_ObjIsCi(pObjFraig) );
        pInfo = (unsigned *)Vec_PtrEntry( p->vSimInfo, Aig_ObjCioId(pObjFraig) );
        Ssw_SmlObjSetWord( p->pSml, pObj, pInfo[0], 0, 0 );
    }
    // set random simulation info for the second frame
    for ( f = 1; f < p->nFrames; f++ )
    {
        Saig_ManForEachPi( p->pAig, pObj, i )
        {
            pObjFraig = Ssw_ObjFrame( p, pObj, f );
            assert( !Aig_IsComplement(pObjFraig) );
            assert( Aig_ObjIsCi(pObjFraig) );
            pInfo = (unsigned *)Vec_PtrEntry( p->vSimInfo, Aig_ObjCioId(pObjFraig) );
            Ssw_SmlObjSetWord( p->pSml, pObj, pInfo[0], 0, f );
        }
    }
    // create random info
    nFrames = Ssw_SmlNumFrames( p->pSml );
    for ( ; f < nFrames; f++ )
    {
        Saig_ManForEachPi( p->pAig, pObj, i )
            Ssw_SmlAssignRandomFrame( p->pSml, pObj, f );
    }
}

/**Function*************************************************************

  Synopsis    [Performs one round of simulation with counter-examples.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_ManSweepResimulateDyn( Ssw_Man_t * p, int f )
{
    int RetValue1, RetValue2;
    abctime clk = Abc_Clock();
    // transfer PI simulation information from storage
//    Ssw_SmlAssignDist1Plus( p->pSml, p->pPatWords );
    Ssw_ManSweepTransferDyn( p );
    // simulate internal nodes
//    Ssw_SmlSimulateOneFrame( p->pSml );
    Ssw_SmlSimulateOne( p->pSml );
    // check equivalence classes
    RetValue1 = Ssw_ClassesRefineConst1( p->ppClasses, 1 );
    RetValue2 = Ssw_ClassesRefine( p->ppClasses, 1 );
    // prepare simulation info for the next round
    Vec_PtrCleanSimInfo( p->vSimInfo, 0, 1 );
    p->nPatterns = 0;
    p->nSimRounds++;
p->timeSimSat += Abc_Clock() - clk;
    return RetValue1 > 0 || RetValue2 > 0;
}

/**Function*************************************************************

  Synopsis    [Performs one round of simulation with counter-examples.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_ManSweepResimulateDynLocal( Ssw_Man_t * p, int f )
{
    Aig_Obj_t * pObj, * pRepr, ** ppClass;
    int i, k, nSize, RetValue1, RetValue2;
    abctime clk = Abc_Clock();
    p->nSimRounds++;
    // transfer PI simulation information from storage
//    Ssw_SmlAssignDist1Plus( p->pSml, p->pPatWords );
    Ssw_ManSweepTransferDyn( p );
    // determine const1 cands and classes to be simulated
    Vec_PtrClear( p->vResimConsts );
    Vec_PtrClear( p->vResimClasses );
    Aig_ManIncrementTravId( p->pAig );
    for ( i = p->iNodeStart; i < p->iNodeLast + p->pPars->nResimDelta; i++ )
    {
        if ( i >= Aig_ManObjNumMax( p->pAig ) )
            break;
        pObj = Aig_ManObj( p->pAig, i );
        if ( pObj == NULL )
            continue;
        if ( Ssw_ObjIsConst1Cand(p->pAig, pObj) )
        {
            Vec_PtrPush( p->vResimConsts, pObj );
            continue;
        }
        pRepr = Aig_ObjRepr(p->pAig, pObj);
        if ( pRepr == NULL )
            continue;
        if ( Aig_ObjIsTravIdCurrent(p->pAig, pRepr) )
            continue;
        Aig_ObjSetTravIdCurrent(p->pAig, pRepr);
        Vec_PtrPush( p->vResimClasses, pRepr );
    }
    // simulate internal nodes
//    Ssw_SmlSimulateOneFrame( p->pSml );
//    Ssw_SmlSimulateOne( p->pSml );
    // resimulate dynamically
//    Aig_ManIncrementTravId( p->pAig );
//    Aig_ObjIsTravIdCurrent( p->pAig, Aig_ManConst1(p->pAig) );
    p->nVisCounter++;
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vResimConsts, pObj, i )
        Ssw_SmlSimulateOneDyn_rec( p->pSml, pObj, p->nFrames-1, p->pVisited, p->nVisCounter );
    // resimulate the cone of influence of the cand classes
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vResimClasses, pRepr, i )
    {
        ppClass = Ssw_ClassesReadClass( p->ppClasses, pRepr, &nSize );
        for ( k = 0; k < nSize; k++ )
            Ssw_SmlSimulateOneDyn_rec( p->pSml, ppClass[k], p->nFrames-1, p->pVisited, p->nVisCounter );
    }

    // check equivalence classes
//    RetValue1 = Ssw_ClassesRefineConst1( p->ppClasses, 1 );
//    RetValue2 = Ssw_ClassesRefine( p->ppClasses, 1 );
    // refine these nodes
    RetValue1 = Ssw_ClassesRefineConst1Group( p->ppClasses, p->vResimConsts, 1 );
    RetValue2 = 0;
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vResimClasses, pRepr, i )
        RetValue2 += Ssw_ClassesRefineOneClass( p->ppClasses, pRepr, 1 );

    // prepare simulation info for the next round
    Vec_PtrCleanSimInfo( p->vSimInfo, 0, 1 );
    p->nPatterns = 0;
    p->nSimRounds++;
p->timeSimSat += Abc_Clock() - clk;
    return RetValue1 > 0 || RetValue2 > 0;
}

/**Function*************************************************************

  Synopsis    [Performs fraiging for the internal nodes.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_ManSweepDyn( Ssw_Man_t * p )
{
    Bar_Progress_t * pProgress = NULL;
    Aig_Obj_t * pObj, * pObjNew;
    int i, f;
    abctime clk;

    // perform speculative reduction
clk = Abc_Clock();
    // create timeframes
    p->pFrames = Ssw_FramesWithClasses( p );
    Aig_ManFanoutStart( p->pFrames );
    p->nSRMiterMaxId = Aig_ManObjNumMax( p->pFrames );

    // map constants and PIs of the last frame
    f = p->pPars->nFramesK;
    Ssw_ObjSetFrame( p, Aig_ManConst1(p->pAig), f, Aig_ManConst1(p->pFrames) );
    Saig_ManForEachPi( p->pAig, pObj, i )
        Ssw_ObjSetFrame( p, pObj, f, Aig_ObjCreateCi(p->pFrames) );
    Aig_ManSetCioIds( p->pFrames );
    // label nodes corresponding to primary inputs
    Ssw_ManLabelPiNodes( p );
p->timeReduce += Abc_Clock() - clk;

    // prepare simulation info
    assert( p->vSimInfo == NULL );
    p->vSimInfo = Vec_PtrAllocSimInfo( Aig_ManCiNum(p->pFrames), 1 );
    Vec_PtrCleanSimInfo( p->vSimInfo, 0, 1 );

    // sweep internal nodes
    p->fRefined = 0;
    Ssw_ClassesClearRefined( p->ppClasses );
    if ( p->pPars->fVerbose )
        pProgress = Bar_ProgressStart( stdout, Aig_ManObjNumMax(p->pAig) );
    p->iNodeStart = 0;
    Aig_ManForEachObj( p->pAig, pObj, i )
    {
        if ( p->iNodeStart == 0 )
            p->iNodeStart = i;
        if ( p->pPars->fVerbose )
            Bar_ProgressUpdate( pProgress, i, NULL );
        if ( Saig_ObjIsLo(p->pAig, pObj) )
            p->fRefined |= Ssw_ManSweepNode( p, pObj, f, 0, NULL );
        else if ( Aig_ObjIsNode(pObj) )
        {
            pObjNew = Aig_And( p->pFrames, Ssw_ObjChild0Fra(p, pObj, f), Ssw_ObjChild1Fra(p, pObj, f) );
            Ssw_ObjSetFrame( p, pObj, f, pObjNew );
            p->fRefined |= Ssw_ManSweepNode( p, pObj, f, 0, NULL );
        }
        // check if it is time to recycle the solver
        if ( p->pMSat->pSat == NULL ||
            (p->pPars->nSatVarMax2 &&
             p->pMSat->nSatVars > p->pPars->nSatVarMax2 &&
             p->nRecycleCalls > p->pPars->nRecycleCalls2) )
        {
            // resimulate
            if ( p->nPatterns > 0 )
            {
                p->iNodeLast = i;
                if ( p->pPars->fLocalSim )
                    Ssw_ManSweepResimulateDynLocal( p, f );
                else
                    Ssw_ManSweepResimulateDyn( p, f );
                p->iNodeStart = i+1;
            }
//                Abc_Print( 1, "Recycling SAT solver with %d vars and %d calls.\n", 
//                    p->pMSat->nSatVars, p->nRecycleCalls );
//            Aig_ManCleanMarkAB( p->pAig );
            Aig_ManCleanMarkAB( p->pFrames );
            // label nodes corresponding to primary inputs
            Ssw_ManLabelPiNodes( p );
            // replace the solver
            if ( p->pMSat )
            {
                p->nVarsMax  = Abc_MaxInt( p->nVarsMax,  p->pMSat->nSatVars );
                p->nCallsMax = Abc_MaxInt( p->nCallsMax, p->pMSat->nSolverCalls );
                Ssw_SatStop( p->pMSat );
                p->nRecycles++;
                p->nRecyclesTotal++;
                p->nRecycleCalls = 0;
            }
            p->pMSat = Ssw_SatStart( 0 );
            assert( p->nPatterns == 0 );
        }
        // resimulate
        if ( p->nPatterns == 32 )
        {
            p->iNodeLast = i;
            if ( p->pPars->fLocalSim )
                Ssw_ManSweepResimulateDynLocal( p, f );
            else
                Ssw_ManSweepResimulateDyn( p, f );
            p->iNodeStart = i+1;
        }
    }
    // resimulate
    if ( p->nPatterns > 0 )
    {
        p->iNodeLast = i;
        if ( p->pPars->fLocalSim )
            Ssw_ManSweepResimulateDynLocal( p, f );
        else
            Ssw_ManSweepResimulateDyn( p, f );
    }
    // collect stats
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
