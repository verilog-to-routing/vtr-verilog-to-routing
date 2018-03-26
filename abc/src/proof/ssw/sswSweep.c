/**CFile****************************************************************

  FileName    [sswSweep.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Inductive prover with constraints.]

  Synopsis    [One round of SAT sweeping.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 1, 2008.]

  Revision    [$Id: sswSweep.c,v 1.00 2008/09/01 00:00:00 alanmi Exp $]

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

  Synopsis    [Retrives value of the PI in the original AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_ManGetSatVarValue( Ssw_Man_t * p, Aig_Obj_t * pObj, int f )
{
    int fUseNoBoundary = 0;
    Aig_Obj_t * pObjFraig;
    int Value;
//    assert( Aig_ObjIsCi(pObj) );
    pObjFraig = Ssw_ObjFrame( p, pObj, f );
    if ( fUseNoBoundary )
    {
        Value = Ssw_CnfGetNodeValue( p->pMSat, Aig_Regular(pObjFraig) );
        Value ^= Aig_IsComplement(pObjFraig);
    }
    else
    {
        int nVarNum = Ssw_ObjSatNum( p->pMSat, Aig_Regular(pObjFraig) );
        Value = (!nVarNum)? 0 : (Aig_IsComplement(pObjFraig) ^ sat_solver_var_value( p->pMSat->pSat, nVarNum ));
    }

//    Value = (Aig_IsComplement(pObjFraig) ^ ((!nVarNum)? 0 : sat_solver_var_value( p->pSat, nVarNum )));
//    Value = (!nVarNum)? Aig_ManRandom(0) & 1 : (Aig_IsComplement(pObjFraig) ^ sat_solver_var_value( p->pSat, nVarNum ));
    if ( p->pPars->fPolarFlip )
    {
        if ( Aig_Regular(pObjFraig)->fPhase )  Value ^= 1;
    }
    return Value;
}

/**Function*************************************************************

  Synopsis    [Performs fraiging for the internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_CheckConstraints( Ssw_Man_t * p )
{
    Aig_Obj_t * pObj, * pObj2;
    int nConstrPairs, i;
    int Counter = 0;
    nConstrPairs = Aig_ManCoNum(p->pFrames)-Aig_ManRegNum(p->pAig);
    assert( (nConstrPairs & 1) == 0 );
    for ( i = 0; i < nConstrPairs; i += 2 )
    {
        pObj  = Aig_ManCo( p->pFrames, i   );
        pObj2 = Aig_ManCo( p->pFrames, i+1 );
        if ( Ssw_NodesAreEquiv( p, Aig_ObjFanin0(pObj), Aig_ObjFanin0(pObj2) ) != 1 )
        {
            Ssw_NodesAreConstrained( p, Aig_ObjChild0(pObj), Aig_ObjChild0(pObj2) );
            Counter++;
        }
    }
    Abc_Print( 1, "Total constraints = %d. Added constraints = %d.\n", nConstrPairs/2, Counter );
}

/**Function*************************************************************

  Synopsis    [Copy pattern from the solver into the internal storage.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_SmlSavePatternAigPhase( Ssw_Man_t * p, int f )
{
    Aig_Obj_t * pObj;
    int i;
    memset( p->pPatWords, 0, sizeof(unsigned) * p->nPatWords );
    Aig_ManForEachCi( p->pAig, pObj, i )
        if ( Aig_ObjPhaseReal( Ssw_ObjFrame(p, pObj, f) ) )
            Abc_InfoSetBit( p->pPatWords, i );
}

/**Function*************************************************************

  Synopsis    [Copy pattern from the solver into the internal storage.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_SmlSavePatternAig( Ssw_Man_t * p, int f )
{
    Aig_Obj_t * pObj;
    int i;
    memset( p->pPatWords, 0, sizeof(unsigned) * p->nPatWords );
    Aig_ManForEachCi( p->pAig, pObj, i )
        if ( Ssw_ManGetSatVarValue( p, pObj, f ) )
            Abc_InfoSetBit( p->pPatWords, i );
}

/**Function*************************************************************

  Synopsis    [Saves one counter-example into internal storage.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_SmlAddPatternDyn( Ssw_Man_t * p )
{
    Aig_Obj_t * pObj;
    unsigned * pInfo;
    int i, nVarNum;
    // iterate through the PIs of the frames
    Vec_PtrForEachEntry( Aig_Obj_t *, p->pMSat->vUsedPis, pObj, i )
    {
        assert( Aig_ObjIsCi(pObj) );
        nVarNum = Ssw_ObjSatNum( p->pMSat, pObj );
        assert( nVarNum > 0 );
        if ( sat_solver_var_value( p->pMSat->pSat, nVarNum ) )
        {
            pInfo = (unsigned *)Vec_PtrEntry( p->vSimInfo, Aig_ObjCioId(pObj) );
            Abc_InfoSetBit( pInfo, p->nPatterns );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Performs fraiging for one node.]

  Description [Returns the fraiged node.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_ManSweepNode( Ssw_Man_t * p, Aig_Obj_t * pObj, int f, int fBmc, Vec_Int_t * vPairs )
{
    Aig_Obj_t * pObjRepr, * pObjFraig, * pObjFraig2, * pObjReprFraig;
    int RetValue;
    abctime clk;
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
    // add constraints on demand
    if ( !fBmc && p->pPars->fDynamic )
    {
clk = Abc_Clock();
        Ssw_ManLoadSolver( p, pObjRepr, pObj );
        p->nRecycleCalls++;
p->timeMarkCones += Abc_Clock() - clk;
    }
    // call equivalence checking
    if ( Aig_Regular(pObjFraig) != Aig_ManConst1(p->pFrames) )
        RetValue = Ssw_NodesAreEquiv( p, Aig_Regular(pObjReprFraig), Aig_Regular(pObjFraig) );
    else
        RetValue = Ssw_NodesAreEquiv( p, Aig_Regular(pObjFraig), Aig_Regular(pObjReprFraig) );
    if ( RetValue == 1 )  // proved equivalent
    {
        pObjFraig2 = Aig_NotCond( pObjReprFraig, pObj->fPhase ^ pObjRepr->fPhase );
        Ssw_ObjSetFrame( p, pObj, f, pObjFraig2 );
        if ( p->pPars->fEquivDump2 && vPairs )
        {
            Vec_IntPush( vPairs, pObjRepr->Id );
            Vec_IntPush( vPairs, pObj->Id );
        }
        return 0;
    }
    if ( p->pPars->fEquivDump && vPairs )
    {
        Vec_IntPush( vPairs, pObjRepr->Id );
        Vec_IntPush( vPairs, pObj->Id );
    }
    if ( RetValue == -1 ) // timed out
    {
        Ssw_ClassesRemoveNode( p->ppClasses, pObj );
        return 1;
    }
    // disproved the equivalence
    if ( !fBmc && p->pPars->fDynamic )
    {
        Ssw_SmlAddPatternDyn( p );
        p->nPatterns++;
        return 1;
    }
    else
        Ssw_SmlSavePatternAig( p, f );
    if ( !p->pPars->fConstrs )
        Ssw_ManResimulateWord( p, pObj, pObjRepr, f );
    else
        Ssw_ManResimulateBit( p, pObj, pObjRepr );
    assert( Aig_ObjRepr( p->pAig, pObj ) != pObjRepr );
    if ( Aig_ObjRepr( p->pAig, pObj ) == pObjRepr )
    {
        Abc_Print( 1, "Ssw_ManSweepNode(): Failed to refine representative.\n" );
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Performs fraiging for the internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_ManSweepBmc( Ssw_Man_t * p )
{
    Bar_Progress_t * pProgress = NULL;
    Aig_Obj_t * pObj, * pObjNew, * pObjLi, * pObjLo;
    int i, f;
    abctime clk;
clk = Abc_Clock();

    // start initialized timeframes
    p->pFrames = Aig_ManStart( Aig_ManObjNumMax(p->pAig) * p->pPars->nFramesK );
    Saig_ManForEachLo( p->pAig, pObj, i )
        Ssw_ObjSetFrame( p, pObj, 0, Aig_ManConst0(p->pFrames) );

    // sweep internal nodes
    p->fRefined = 0;
    if ( p->pPars->fVerbose )
        pProgress = Bar_ProgressStart( stdout, Aig_ManObjNumMax(p->pAig) * p->pPars->nFramesK );
    for ( f = 0; f < p->pPars->nFramesK; f++ )
    {
        // map constants and PIs
        Ssw_ObjSetFrame( p, Aig_ManConst1(p->pAig), f, Aig_ManConst1(p->pFrames) );
        Saig_ManForEachPi( p->pAig, pObj, i )
            Ssw_ObjSetFrame( p, pObj, f, Aig_ObjCreateCi(p->pFrames) );
        // sweep flops
        Saig_ManForEachLo( p->pAig, pObj, i )
            p->fRefined |= Ssw_ManSweepNode( p, pObj, f, 1, NULL );
        // sweep internal nodes
        Aig_ManForEachNode( p->pAig, pObj, i )
        {
            if ( p->pPars->fVerbose )
                Bar_ProgressUpdate( pProgress, Aig_ManObjNumMax(p->pAig) * f + i, NULL );
            pObjNew = Aig_And( p->pFrames, Ssw_ObjChild0Fra(p, pObj, f), Ssw_ObjChild1Fra(p, pObj, f) );
            Ssw_ObjSetFrame( p, pObj, f, pObjNew );
            p->fRefined |= Ssw_ManSweepNode( p, pObj, f, 1, NULL );
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

  Synopsis    [Generates AIG with the following nodes put into seq miters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssw_ManDumpEquivMiter( Aig_Man_t * p, Vec_Int_t * vPairs, int Num, int fAddOuts )
{
    FILE * pFile;
    char pBuffer[16];
    Aig_Man_t * pNew;
    sprintf( pBuffer, "equiv%03d.aig", Num );
    pFile = fopen( pBuffer, "w" );
    if ( pFile == NULL )
    {
        Abc_Print( 1, "Cannot open file %s for writing.\n", pBuffer );
        return;
    }
    fclose( pFile );
    pNew = Saig_ManCreateEquivMiter( p, vPairs, fAddOuts );
    Ioa_WriteAiger( pNew, pBuffer, 0, 0 );
    Aig_ManStop( pNew );
    Abc_Print( 1, "AIG with %4d disproved equivs is dumped into file \"%s\".\n", Vec_IntSize(vPairs)/2, pBuffer );
}


/**Function*************************************************************

  Synopsis    [Performs fraiging for the internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_ManSweep( Ssw_Man_t * p )
{
    static int Counter;
    Bar_Progress_t * pProgress = NULL;
    Aig_Obj_t * pObj, * pObj2, * pObjNew;
    int nConstrPairs, i, f;
    abctime clk;
    Vec_Int_t * vObjPairs;

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
    sat_solver_simplify( p->pMSat->pSat );

    // map constants and PIs of the last frame
    f = p->pPars->nFramesK;
    Ssw_ObjSetFrame( p, Aig_ManConst1(p->pAig), f, Aig_ManConst1(p->pFrames) );
    Saig_ManForEachPi( p->pAig, pObj, i )
        Ssw_ObjSetFrame( p, pObj, f, Aig_ObjCreateCi(p->pFrames) );
p->timeReduce += Abc_Clock() - clk;

    // sweep internal nodes
    p->fRefined = 0;
    Ssw_ClassesClearRefined( p->ppClasses );
    if ( p->pPars->fVerbose )
        pProgress = Bar_ProgressStart( stdout, Aig_ManObjNumMax(p->pAig) );
    vObjPairs = (p->pPars->fEquivDump || p->pPars->fEquivDump2)? Vec_IntAlloc(1000) : NULL;
    Aig_ManForEachObj( p->pAig, pObj, i )
    {
        if ( p->pPars->fVerbose )
            Bar_ProgressUpdate( pProgress, i, NULL );
        if ( Saig_ObjIsLo(p->pAig, pObj) )
            p->fRefined |= Ssw_ManSweepNode( p, pObj, f, 0, vObjPairs );
        else if ( Aig_ObjIsNode(pObj) )
        {
            pObjNew = Aig_And( p->pFrames, Ssw_ObjChild0Fra(p, pObj, f), Ssw_ObjChild1Fra(p, pObj, f) );
            Ssw_ObjSetFrame( p, pObj, f, pObjNew );
            p->fRefined |= Ssw_ManSweepNode( p, pObj, f, 0, vObjPairs );
        }
    }
    if ( p->pPars->fVerbose )
        Bar_ProgressStop( pProgress );

    // cleanup
//    Ssw_ClassesCheck( p->ppClasses );
    if ( p->pPars->fEquivDump )
        Ssw_ManDumpEquivMiter( p->pAig, vObjPairs, Counter++, 1 );
    if ( p->pPars->fEquivDump2 && !p->fRefined )
        Ssw_ManDumpEquivMiter( p->pAig, vObjPairs, 0, 0 );
    Vec_IntFreeP( &vObjPairs );
    return p->fRefined;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END
