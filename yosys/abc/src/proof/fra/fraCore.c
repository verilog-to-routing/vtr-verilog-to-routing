/**CFile****************************************************************

  FileName    [fraCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [New FRAIG package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 30, 2007.]

  Revision    [$Id: fraCore.c,v 1.00 2007/06/30 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fra.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

/* 
    Speculating reduction in the sequential case leads to an interesting 
    situation when a counter-ex may not refine any classes. This happens
    for non-constant equivalence classes. In such cases the representative
    of the class (proved by simulation to be non-constant) may be reduced 
    to a constant during the speculative reduction. The fraig-representative 
    of this representative node is a constant node, even though this is a 
    non-constant class. Experiments have shown that this situation happens 
    very often at the beginning of the refinement iteration when there are 
    many spurious candidate equivalence classes (especially if heavy-duty 
    simulatation of BMC was node used at the beginning). As a result, the 
    SAT solver run may return a counter-ex that  distinguishes the given 
    representative node from the constant-1 node but this counter-ex
    does not distinguish the nodes in the non-costant class... This is why 
    there is no check of refinement after a counter-ex in the sequential case.
*/

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reports the status of the miter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_FraigMiterStatus( Aig_Man_t * p )
{
    Aig_Obj_t * pObj, * pChild;
    int i, CountConst0 = 0, CountNonConst0 = 0, CountUndecided = 0;
    if ( p->pData )
        return 0;
    Aig_ManForEachPoSeq( p, pObj, i )
    {
        pChild = Aig_ObjChild0(pObj);
        // check if the output is constant 0
        if ( pChild == Aig_ManConst0(p) )
        {
            CountConst0++;
            continue;
        }
        // check if the output is constant 1
        if ( pChild == Aig_ManConst1(p) )
        {
            CountNonConst0++;
            continue;
        }
        // check if the output is a primary input
        if ( Aig_ObjIsCi(Aig_Regular(pChild)) && Aig_ObjCioId(Aig_Regular(pChild)) < p->nTruePis )
        {
            CountNonConst0++;
            continue;
        }
        // check if the output can be not constant 0
        if ( Aig_Regular(pChild)->fPhase != (unsigned)Aig_IsComplement(pChild) )
        {
            CountNonConst0++;
            continue;
        }
        CountUndecided++;
    }
/*
    if ( p->pParams->fVerbose )
    {
        printf( "Miter has %d outputs. ", Aig_ManCoNum(p->pManAig) );
        printf( "Const0 = %d.  ", CountConst0 );
        printf( "NonConst0 = %d.  ", CountNonConst0 );
        printf( "Undecided = %d.  ", CountUndecided );
        printf( "\n" );
    }
*/
    if ( CountNonConst0 )
        return 0;
    if ( CountUndecided )
        return -1;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Reports the status of the miter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_FraigMiterAssertedOutput( Aig_Man_t * p )
{
    Aig_Obj_t * pObj, * pChild;
    int i;
    Aig_ManForEachPoSeq( p, pObj, i )
    {
        pChild = Aig_ObjChild0(pObj);
        // check if the output is constant 0
        if ( pChild == Aig_ManConst0(p) )
            continue;
        // check if the output is constant 1
        if ( pChild == Aig_ManConst1(p) )
            return i;
        // check if the output can be not constant 0
        if ( Aig_Regular(pChild)->fPhase != (unsigned)Aig_IsComplement(pChild) )
            return i;
    }
    return -1;
}

/**Function*************************************************************

  Synopsis    [Write speculative miter for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Fra_FraigNodeSpeculate( Fra_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pObjFraig, Aig_Obj_t * pObjReprFraig )
{ 
    static int Counter = 0;
    char FileName[20];
    Aig_Man_t * pTemp;
    Aig_Obj_t * pNode;
    int i;
    // create manager with the logic for these two nodes
    pTemp = Aig_ManExtractMiter( p->pManFraig, pObjFraig, pObjReprFraig );
    // dump the logic into a file
    sprintf( FileName, "aig\\%03d.blif", ++Counter );
    Aig_ManDumpBlif( pTemp, FileName, NULL, NULL );
    printf( "Speculation cone with %d nodes was written into file \"%s\".\n", Aig_ManNodeNum(pTemp), FileName );
    // clean up
    Aig_ManStop( pTemp );
    Aig_ManForEachObj( p->pManFraig, pNode, i )
        pNode->pData = p;
}

/**Function*************************************************************

  Synopsis    [Verifies the generated counter-ex.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_FraigVerifyCounterEx( Fra_Man_t * p, Vec_Int_t * vCex )
{
    Aig_Obj_t * pObj, ** ppClass;
    int i, c;
    assert( Aig_ManCiNum(p->pManAig) == Vec_IntSize(vCex) );
    // make sure the input pattern is not used
    Aig_ManForEachObj( p->pManAig, pObj, i )
        assert( !pObj->fMarkB );
    // simulate the cex through the AIG
    Aig_ManConst1(p->pManAig)->fMarkB = 1;
    Aig_ManForEachCi( p->pManAig, pObj, i )
        pObj->fMarkB = Vec_IntEntry(vCex, i);
    Aig_ManForEachNode( p->pManAig, pObj, i )
        pObj->fMarkB = (Aig_ObjFanin0(pObj)->fMarkB ^ Aig_ObjFaninC0(pObj)) & 
                       (Aig_ObjFanin1(pObj)->fMarkB ^ Aig_ObjFaninC1(pObj));
    Aig_ManForEachCo( p->pManAig, pObj, i )
        pObj->fMarkB = Aig_ObjFanin0(pObj)->fMarkB ^ Aig_ObjFaninC0(pObj);
    // check if the classes hold
    Vec_PtrForEachEntry( Aig_Obj_t *, p->pCla->vClasses1, pObj, i )
    {
        if ( pObj->fPhase != pObj->fMarkB )
            printf( "The node %d is not constant under cex!\n", pObj->Id );
    }
    Vec_PtrForEachEntry( Aig_Obj_t **, p->pCla->vClasses, ppClass, i )
    {
        for ( c = 1; ppClass[c]; c++ )
            if ( (ppClass[0]->fPhase ^ ppClass[c]->fPhase) != (ppClass[0]->fMarkB ^ ppClass[c]->fMarkB) )
                printf( "The nodes %d and %d are not equal under cex!\n", ppClass[0]->Id, ppClass[c]->Id );
//        for ( c = 0; ppClass[c]; c++ )
//            if ( Fra_ObjFraig(ppClass[c],p->pPars->nFramesK) == Aig_ManConst1(p->pManFraig) )
//                printf( "A member of non-constant class has a constant repr!\n" );
    }
    // clean the simulation pattern
    Aig_ManForEachObj( p->pManAig, pObj, i )
        pObj->fMarkB = 0;
}

/**Function*************************************************************

  Synopsis    [Performs fraiging for one node.]

  Description [Returns the fraiged node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Fra_FraigNode( Fra_Man_t * p, Aig_Obj_t * pObj )
{ 
    Aig_Obj_t * pObjRepr, * pObjFraig, * pObjFraig2, * pObjReprFraig;
    int RetValue;
    assert( !Aig_IsComplement(pObj) );
    // get representative of this class
    pObjRepr = Fra_ClassObjRepr( pObj );
    if ( pObjRepr == NULL || // this is a unique node
       (!p->pPars->fDoSparse && pObjRepr == Aig_ManConst1(p->pManAig)) ) // this is a sparse node
        return;
    // get the fraiged node
    pObjFraig = Fra_ObjFraig( pObj, p->pPars->nFramesK );
    // get the fraiged representative
    pObjReprFraig = Fra_ObjFraig( pObjRepr, p->pPars->nFramesK );
    // if the fraiged nodes are the same, return
    if ( Aig_Regular(pObjFraig) == Aig_Regular(pObjReprFraig) )
    {
        p->nSatCallsSkipped++;
        return;
    }
    assert( p->pPars->nFramesK || Aig_Regular(pObjFraig) != Aig_ManConst1(p->pManFraig) );
    // if they are proved different, the c-ex will be in p->pPatWords
    RetValue = Fra_NodesAreEquiv( p, Aig_Regular(pObjReprFraig), Aig_Regular(pObjFraig) );
    if ( RetValue == 1 )  // proved equivalent
    {
//        if ( p->pPars->fChoicing )
//            Aig_ObjCreateRepr( p->pManFraig, Aig_Regular(pObjReprFraig), Aig_Regular(pObjFraig) );
        // the nodes proved equal
        pObjFraig2 = Aig_NotCond( pObjReprFraig, pObj->fPhase ^ pObjRepr->fPhase );
        Fra_ObjSetFraig( pObj, p->pPars->nFramesK, pObjFraig2 );
        return;
    }
    if ( RetValue == -1 ) // failed
    {
        if ( p->vTimeouts == NULL )
            p->vTimeouts = Vec_PtrAlloc( 100 );
        Vec_PtrPush( p->vTimeouts, pObj );
        if ( !p->pPars->fSpeculate )
            return;
        assert( 0 );
        // speculate
        p->nSpeculs++;
        pObjFraig2 = Aig_NotCond( pObjReprFraig, pObj->fPhase ^ pObjRepr->fPhase );
        Fra_ObjSetFraig( pObj, p->pPars->nFramesK, pObjFraig2 );
        Fra_FraigNodeSpeculate( p, pObj, Aig_Regular(pObjFraig), Aig_Regular(pObjReprFraig) );
        return;
    }
    // disprove the nodes
    p->pCla->fRefinement = 1;
    // if we do not include the node into those disproved, we may end up 
    // merging this node with another representative, for which proof has timed out
    if ( p->vTimeouts )
        Vec_PtrPush( p->vTimeouts, pObj );
    // verify that the counter-example satisfies all the constraints
//    if ( p->vCex )
//        Fra_FraigVerifyCounterEx( p, p->vCex );
    // simulate the counter-example and return the Fraig node
    Fra_SmlResimulate( p );
    if ( p->pManFraig->pData )
        return;
    if ( !p->pPars->nFramesK && Fra_ClassObjRepr(pObj) == pObjRepr )
        printf( "Fra_FraigNode(): Error in class refinement!\n" );
    assert( p->pPars->nFramesK || Fra_ClassObjRepr(pObj) != pObjRepr );
}

/**Function*************************************************************

  Synopsis    [Performs fraiging for the internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_FraigSweep( Fra_Man_t * p )
{
//    Bar_Progress_t * pProgress = NULL;
    Aig_Obj_t * pObj, * pObjNew;
    int i, Pos = 0;
    int nBTracksOld;
    // fraig latch outputs
    Aig_ManForEachLoSeq( p->pManAig, pObj, i )
    {
        Fra_FraigNode( p, pObj );
        if ( p->pPars->fUseImps )
            Pos = Fra_ImpCheckForNode( p, p->pCla->vImps, pObj, Pos );
    }
    if ( p->pPars->fLatchCorr )
        return;
    // fraig internal nodes
//    if ( !p->pPars->fDontShowBar )
//        pProgress = Bar_ProgressStart( stdout, Aig_ManObjNumMax(p->pManAig) );
    nBTracksOld = p->pPars->nBTLimitNode;
    Aig_ManForEachNode( p->pManAig, pObj, i )
    {
//        if ( pProgress )
//            Bar_ProgressUpdate( pProgress, i, NULL );
        // derive and remember the new fraig node
        pObjNew = Aig_And( p->pManFraig, Fra_ObjChild0Fra(pObj,p->pPars->nFramesK), Fra_ObjChild1Fra(pObj,p->pPars->nFramesK) );
        Fra_ObjSetFraig( pObj, p->pPars->nFramesK, pObjNew );
        Aig_Regular(pObjNew)->pData = p;
        // quit if simulation detected a counter-example for a PO
        if ( p->pManFraig->pData )
            continue;
//        if ( Aig_SupportSize(p->pManAig,pObj) > 16 )
//            continue;
        // perform fraiging
        if ( p->pPars->nLevelMax && (int)pObj->Level > p->pPars->nLevelMax )
            p->pPars->nBTLimitNode = 5;
        Fra_FraigNode( p, pObj );
        if ( p->pPars->nLevelMax && (int)pObj->Level > p->pPars->nLevelMax )
            p->pPars->nBTLimitNode = nBTracksOld;
        // check implications
        if ( p->pPars->fUseImps )
            Pos = Fra_ImpCheckForNode( p, p->pCla->vImps, pObj, Pos );
    }
//    if ( pProgress )
//        Bar_ProgressStop( pProgress );
    // try to prove the outputs of the miter
    p->nNodesMiter = Aig_ManNodeNum(p->pManFraig);
//    Fra_MiterStatus( p->pManFraig );
//    if ( p->pPars->fProve && p->pManFraig->pData == NULL )
//        Fra_MiterProve( p );
    // compress implications after processing all of them
    if ( p->pPars->fUseImps )
        Fra_ImpCompactArray( p->pCla->vImps );
}

/**Function*************************************************************

  Synopsis    [Performs fraiging of the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Fra_FraigPerform( Aig_Man_t * pManAig, Fra_Par_t * pPars )
{
    Fra_Man_t * p;
    Aig_Man_t * pManAigNew;
    abctime clk;
    if ( Aig_ManNodeNum(pManAig) == 0 )
        return Aig_ManDupOrdered(pManAig);
clk = Abc_Clock();
    p = Fra_ManStart( pManAig, pPars );
    p->pManFraig = Fra_ManPrepareComb( p );
    p->pSml = Fra_SmlStart( pManAig, 0, 1, pPars->nSimWords );
    Fra_SmlSimulate( p, 0 );
//    if ( p->pPars->fChoicing )
//        Aig_ManReprStart( p->pManFraig, Aig_ManObjNumMax(p->pManAig) );
    // collect initial states
    p->nLitsBeg  = Fra_ClassesCountLits( p->pCla );
    p->nNodesBeg = Aig_ManNodeNum(pManAig);
    p->nRegsBeg  = Aig_ManRegNum(pManAig);
    // perform fraig sweep
if ( p->pPars->fVerbose )
Fra_ClassesPrint( p->pCla, 1 );
    Fra_FraigSweep( p );
    // call back the procedure to check implications
    if ( pManAig->pImpFunc )
        pManAig->pImpFunc( p, pManAig->pImpData );
    // no need to filter one-hot clauses because they satisfy base case by construction
    // finalize the fraiged manager
    Fra_ManFinalizeComb( p );
    if ( p->pPars->fChoicing )
    { 
abctime clk2 = Abc_Clock();
        Fra_ClassesCopyReprs( p->pCla, p->vTimeouts );
        pManAigNew = Aig_ManDupRepr( p->pManAig, 1 );
        Aig_ManReprStart( pManAigNew, Aig_ManObjNumMax(pManAigNew) );
        Aig_ManTransferRepr( pManAigNew, p->pManAig );
        Aig_ManMarkValidChoices( pManAigNew );
        Aig_ManStop( p->pManFraig );
        p->pManFraig = NULL;
p->timeTrav += Abc_Clock() - clk2;
    }
    else
    {
        Fra_ClassesCopyReprs( p->pCla, p->vTimeouts );
        Aig_ManCleanup( p->pManFraig );
        pManAigNew = p->pManFraig;
        p->pManFraig = NULL;
    }
p->timeTotal = Abc_Clock() - clk;
    // collect final stats
    p->nLitsEnd  = Fra_ClassesCountLits( p->pCla );
    p->nNodesEnd = Aig_ManNodeNum(pManAigNew);
    p->nRegsEnd  = Aig_ManRegNum(pManAigNew);
    Fra_ManStop( p );
    return pManAigNew;
}

/**Function*************************************************************

  Synopsis    [Performs choicing of the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Fra_FraigChoice( Aig_Man_t * pManAig, int nConfMax, int nLevelMax )
{
    Fra_Par_t Pars, * pPars = &Pars; 
    Fra_ParamsDefault( pPars );
    pPars->nBTLimitNode = nConfMax;
    pPars->fChoicing    = 1;
    pPars->fDoSparse    = 1;
    pPars->fSpeculate   = 0;
    pPars->fProve       = 0;
    pPars->fVerbose     = 0;
    pPars->fDontShowBar = 1;
    pPars->nLevelMax    = nLevelMax;
    return Fra_FraigPerform( pManAig, pPars );
}
 
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Fra_FraigEquivence( Aig_Man_t * pManAig, int nConfMax, int fProve )
{
    Aig_Man_t * pFraig;
    Fra_Par_t Pars, * pPars = &Pars; 
    Fra_ParamsDefault( pPars );
    pPars->nBTLimitNode = nConfMax;
    pPars->fChoicing    = 0;
    pPars->fDoSparse    = 1;
    pPars->fSpeculate   = 0;
    pPars->fProve       = fProve;
    pPars->fVerbose     = 0;
    pPars->fDontShowBar = 1;
    pFraig = Fra_FraigPerform( pManAig, pPars );
    return pFraig;
} 

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

