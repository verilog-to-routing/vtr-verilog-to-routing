/**CFile****************************************************************

  FileName    [fraInd.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [New FRAIG package.]

  Synopsis    [Inductive prover.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 30, 2007.]

  Revision    [$Id: fraInd.c,v 1.00 2007/06/30 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fra.h"
#include "sat/cnf/cnf.h"
#include "opt/dar/dar.h"
#include "aig/saig/saig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs AIG rewriting on the constraint manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_FraigInductionRewrite( Fra_Man_t * p )
{
    Aig_Man_t * pTemp;
    Aig_Obj_t * pObj, * pObjPo;
    int nTruePis, k, i;
    abctime clk = Abc_Clock();
    // perform AIG rewriting on the speculated frames
//    pTemp = Dar_ManRwsat( pTemp, 1, 0 );
    pTemp = Dar_ManRewriteDefault( p->pManFraig );
//    printf( "Before = %6d.  After = %6d.\n", Aig_ManNodeNum(p->pManFraig), Aig_ManNodeNum(pTemp) ); 
//Aig_ManDumpBlif( p->pManFraig, "1.blif", NULL, NULL );
//Aig_ManDumpBlif( pTemp, "2.blif", NULL, NULL );
//    Fra_FramesWriteCone( pTemp );
//    Aig_ManStop( pTemp );
    // transfer PI/register pointers
    assert( p->pManFraig->nRegs == pTemp->nRegs );
    assert( p->pManFraig->nAsserts == pTemp->nAsserts );
    nTruePis = Aig_ManCiNum(p->pManAig) - Aig_ManRegNum(p->pManAig);
    memset( p->pMemFraig, 0, sizeof(Aig_Obj_t *) * p->nSizeAlloc * p->nFramesAll );
    Fra_ObjSetFraig( Aig_ManConst1(p->pManAig), p->pPars->nFramesK, Aig_ManConst1(pTemp) );
    Aig_ManForEachPiSeq( p->pManAig, pObj, i )
        Fra_ObjSetFraig( pObj, p->pPars->nFramesK, Aig_ManCi(pTemp,nTruePis*p->pPars->nFramesK+i) );
    k = 0;
    assert( Aig_ManRegNum(p->pManAig) == Aig_ManCoNum(pTemp) - pTemp->nAsserts );
    Aig_ManForEachLoSeq( p->pManAig, pObj, i )
    {
        pObjPo = Aig_ManCo(pTemp, pTemp->nAsserts + k++);
        Fra_ObjSetFraig( pObj, p->pPars->nFramesK, Aig_ObjChild0(pObjPo) );
    }
    // exchange
    Aig_ManStop( p->pManFraig );
    p->pManFraig = pTemp;
p->timeRwr += Abc_Clock() - clk;
}

/**Function*************************************************************

  Synopsis    [Performs speculative reduction for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Fra_FramesConstrainNode( Aig_Man_t * pManFraig, Aig_Obj_t * pObj, int iFrame )
{
    Aig_Obj_t * pObjNew, * pObjNew2, * pObjRepr, * pObjReprNew, * pMiter;
    // skip nodes without representative
    if ( (pObjRepr = Fra_ClassObjRepr(pObj)) == NULL )
        return;
    assert( pObjRepr->Id < pObj->Id );
    // get the new node 
    pObjNew = Fra_ObjFraig( pObj, iFrame );
    // get the new node of the representative
    pObjReprNew = Fra_ObjFraig( pObjRepr, iFrame );
    // if this is the same node, no need to add constraints
    if ( Aig_Regular(pObjNew) == Aig_Regular(pObjReprNew) )
        return;
    // these are different nodes - perform speculative reduction
    pObjNew2 = Aig_NotCond( pObjReprNew, pObj->fPhase ^ pObjRepr->fPhase );
    // set the new node
    Fra_ObjSetFraig( pObj, iFrame, pObjNew2 );
    // add the constraint
    pMiter = Aig_Exor( pManFraig, pObjNew, pObjReprNew );
    pMiter = Aig_NotCond( pMiter, !Aig_ObjPhaseReal(pMiter) );
    assert( Aig_ObjPhaseReal(pMiter) == 1 );
    Aig_ObjCreateCo( pManFraig, pMiter );
}

/**Function*************************************************************

  Synopsis    [Prepares the inductive case with speculative reduction.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Fra_FramesWithClasses( Fra_Man_t * p )
{
    Aig_Man_t * pManFraig;
    Aig_Obj_t * pObj, * pObjLi, * pObjLo, * pObjNew;
    int i, k, f;
    assert( p->pManFraig == NULL );
    assert( Aig_ManRegNum(p->pManAig) > 0 );
    assert( Aig_ManRegNum(p->pManAig) < Aig_ManCiNum(p->pManAig) );

    // start the fraig package
    pManFraig = Aig_ManStart( Aig_ManObjNumMax(p->pManAig) * p->nFramesAll );
    pManFraig->pName = Abc_UtilStrsav( p->pManAig->pName );
    pManFraig->pSpec = Abc_UtilStrsav( p->pManAig->pSpec );
    pManFraig->nRegs = p->pManAig->nRegs;
    // create PI nodes for the frames
    for ( f = 0; f < p->nFramesAll; f++ )
        Fra_ObjSetFraig( Aig_ManConst1(p->pManAig), f, Aig_ManConst1(pManFraig) );
    for ( f = 0; f < p->nFramesAll; f++ )
        Aig_ManForEachPiSeq( p->pManAig, pObj, i )
            Fra_ObjSetFraig( pObj, f, Aig_ObjCreateCi(pManFraig) );
    // create latches for the first frame
    Aig_ManForEachLoSeq( p->pManAig, pObj, i )
        Fra_ObjSetFraig( pObj, 0, Aig_ObjCreateCi(pManFraig) );

    // add timeframes
//    pManFraig->fAddStrash = 1;
    for ( f = 0; f < p->nFramesAll - 1; f++ )
    {
        // set the constraints on the latch outputs
        Aig_ManForEachLoSeq( p->pManAig, pObj, i )
            Fra_FramesConstrainNode( pManFraig, pObj, f );
        // add internal nodes of this frame
        Aig_ManForEachNode( p->pManAig, pObj, i )
        {
            pObjNew = Aig_And( pManFraig, Fra_ObjChild0Fra(pObj,f), Fra_ObjChild1Fra(pObj,f) );
            Fra_ObjSetFraig( pObj, f, pObjNew );
            Fra_FramesConstrainNode( pManFraig, pObj, f );
        }
        // transfer latch input to the latch outputs 
        Aig_ManForEachLiLoSeq( p->pManAig, pObjLi, pObjLo, k )
            Fra_ObjSetFraig( pObjLo, f+1, Fra_ObjChild0Fra(pObjLi,f) );
    }
//    pManFraig->fAddStrash = 0;
    // mark the asserts
    pManFraig->nAsserts = Aig_ManCoNum(pManFraig);
    // add the POs for the latch outputs of the last frame
    Aig_ManForEachLoSeq( p->pManAig, pObj, i )
        Aig_ObjCreateCo( pManFraig, Fra_ObjFraig(pObj,p->nFramesAll-1) );

    // remove dangling nodes
    Aig_ManCleanup( pManFraig );
    // make sure the satisfying assignment is node assigned
    assert( pManFraig->pData == NULL );
    return pManFraig;
}

/**Function*************************************************************

  Synopsis    [Prepares the inductive case with speculative reduction.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_FramesAddMore( Aig_Man_t * p, int nFrames )
{
    Aig_Obj_t * pObj, ** pLatches;
    int i, k, f, nNodesOld;
    // set copy pointer of each object to point to itself
    Aig_ManForEachObj( p, pObj, i )
        pObj->pData = pObj;
    // iterate and add objects
    nNodesOld = Aig_ManObjNumMax(p);
    pLatches = ABC_ALLOC( Aig_Obj_t *, Aig_ManRegNum(p) );
    for ( f = 0; f < nFrames; f++ )
    {
        // clean latch inputs and outputs
        Aig_ManForEachLiSeq( p, pObj, i )
            pObj->pData = NULL;
        Aig_ManForEachLoSeq( p, pObj, i )
            pObj->pData = NULL;
        // save the latch input values
        k = 0;
        Aig_ManForEachLiSeq( p, pObj, i )
        {
            if ( Aig_ObjFanin0(pObj)->pData )
                pLatches[k++] = Aig_ObjChild0Copy(pObj);
            else
                pLatches[k++] = NULL;
        }
        // insert them as the latch output values
        k = 0;
        Aig_ManForEachLoSeq( p, pObj, i )
            pObj->pData = pLatches[k++];
        // create the next time frame of nodes
        Aig_ManForEachNode( p, pObj, i )
        {
            if ( i > nNodesOld )
                break;
            if ( Aig_ObjFanin0(pObj)->pData && Aig_ObjFanin1(pObj)->pData )
                pObj->pData = Aig_And( p, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
            else
                pObj->pData = NULL;
        }
    }
    ABC_FREE( pLatches );
}


/**Function*************************************************************

  Synopsis    [Performs partitioned sequential SAT sweepingG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Fra_FraigInductionPart( Aig_Man_t * pAig, Fra_Ssw_t * pPars )
{
    int fPrintParts = 0;
    char Buffer[100];
    Aig_Man_t * pTemp, * pNew;
    Vec_Ptr_t * vResult;
    Vec_Int_t * vPart;
    int * pMapBack;
    int i, nCountPis, nCountRegs;
    int nClasses, nPartSize, fVerbose;
    abctime clk = Abc_Clock();

    // save parameters
    nPartSize = pPars->nPartSize; pPars->nPartSize = 0;
    fVerbose  = pPars->fVerbose;  pPars->fVerbose = 0;
    // generate partitions
    if ( pAig->vClockDoms )
    {
        // divide large clock domains into separate partitions
        vResult = Vec_PtrAlloc( 100 );
        Vec_PtrForEachEntry( Vec_Int_t *, (Vec_Ptr_t *)pAig->vClockDoms, vPart, i )
        {
            if ( nPartSize && Vec_IntSize(vPart) > nPartSize )
                Aig_ManPartDivide( vResult, vPart, nPartSize, pPars->nOverSize );
            else
                Vec_PtrPush( vResult, Vec_IntDup(vPart) );
        }
    }
    else
        vResult = Aig_ManRegPartitionSimple( pAig, nPartSize, pPars->nOverSize );
//    vResult = Aig_ManPartitionSmartRegisters( pAig, nPartSize, 0 ); 
//    vResult = Aig_ManRegPartitionSmart( pAig, nPartSize );
    if ( fPrintParts )
    {
        // print partitions
        printf( "Simple partitioning. %d partitions are saved:\n", Vec_PtrSize(vResult) );
        Vec_PtrForEachEntry( Vec_Int_t *, vResult, vPart, i )
        {
            sprintf( Buffer, "part%03d.aig", i );
            pTemp = Aig_ManRegCreatePart( pAig, vPart, &nCountPis, &nCountRegs, NULL );
            Ioa_WriteAiger( pTemp, Buffer, 0, 0 );
            printf( "part%03d.aig : Reg = %4d. PI = %4d. (True = %4d. Regs = %4d.) And = %5d.\n", 
                i, Vec_IntSize(vPart), Aig_ManCiNum(pTemp)-Vec_IntSize(vPart), nCountPis, nCountRegs, Aig_ManNodeNum(pTemp) );
            Aig_ManStop( pTemp );
        }
    }

    // perform SSW with partitions
    Aig_ManReprStart( pAig, Aig_ManObjNumMax(pAig) );
    Vec_PtrForEachEntry( Vec_Int_t *, vResult, vPart, i )
    {
        pTemp = Aig_ManRegCreatePart( pAig, vPart, &nCountPis, &nCountRegs, &pMapBack );
        // create the projection of 1-hot registers
        if ( pAig->vOnehots )
            pTemp->vOnehots = Aig_ManRegProjectOnehots( pAig, pTemp, pAig->vOnehots, fVerbose );
        // run SSW
        pNew = Fra_FraigInduction( pTemp, pPars );
        nClasses = Aig_TransferMappedClasses( pAig, pTemp, pMapBack );
        if ( fVerbose )
            printf( "%3d : Reg = %4d. PI = %4d. (True = %4d. Regs = %4d.) And = %5d. It = %3d. Cl = %5d.\n", 
                i, Vec_IntSize(vPart), Aig_ManCiNum(pTemp)-Vec_IntSize(vPart), nCountPis, nCountRegs, Aig_ManNodeNum(pTemp), pPars->nIters, nClasses );
        Aig_ManStop( pNew );
        Aig_ManStop( pTemp );
        ABC_FREE( pMapBack );
    }
    // remap the AIG
    pNew = Aig_ManDupRepr( pAig, 0 );
    Aig_ManSeqCleanup( pNew );
//    Aig_ManPrintStats( pAig );
//    Aig_ManPrintStats( pNew );
    Vec_VecFree( (Vec_Vec_t *)vResult );
    pPars->nPartSize = nPartSize;
    pPars->fVerbose = fVerbose;
    if ( fVerbose )
    {
        ABC_PRT( "Total time", Abc_Clock() - clk );
    }
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Performs sequential SAT sweeping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Fra_FraigInduction( Aig_Man_t * pManAig, Fra_Ssw_t * pParams )
{
    int fUseSimpleCnf = 0;
    int fUseOldSimulation = 0;
    // other paramaters affecting performance
    // - presence of FRAIGing in Abc_NtkDarSeqSweep()
    // - using distance-1 patterns in Fra_SmlAssignDist1()
    // - the number of simulation patterns
    // - the number of BMC frames

    Fra_Man_t * p;
    Fra_Par_t Pars, * pPars = &Pars; 
    Aig_Obj_t * pObj;
    Cnf_Dat_t * pCnf;
    Aig_Man_t * pManAigNew = NULL;
    int nNodesBeg, nRegsBeg;
    int nIter = -1; // Suppress "might be used uninitialized"
    int i;
    abctime clk = Abc_Clock(), clk2;
    abctime TimeToStop = pParams->TimeLimit ? pParams->TimeLimit * CLOCKS_PER_SEC + Abc_Clock() : 0;

    if ( Aig_ManNodeNum(pManAig) == 0 )
    {
        pParams->nIters = 0;
        // Ntl_ManFinalize() needs the following to satisfy an assertion
        Aig_ManReprStart(pManAig,Aig_ManObjNumMax(pManAig));
        return Aig_ManDupOrdered(pManAig);
    }
    assert( Aig_ManRegNum(pManAig) > 0 );
    assert( pParams->nFramesK > 0 );
//Aig_ManShow( pManAig, 0, NULL );

    if ( pParams->fWriteImps && pParams->nPartSize > 0 )
    {
        pParams->nPartSize = 0;
        printf( "Partitioning was disabled to allow implication writing.\n" );
    }
    // perform partitioning
    if ( (pParams->nPartSize > 0 && pParams->nPartSize < Aig_ManRegNum(pManAig))
         || (pManAig->vClockDoms && Vec_VecSize(pManAig->vClockDoms) > 0)  )
        return Fra_FraigInductionPart( pManAig, pParams );
 
    nNodesBeg = Aig_ManNodeNum(pManAig);
    nRegsBeg  = Aig_ManRegNum(pManAig);

    // enhance the AIG by adding timeframes
//    Fra_FramesAddMore( pManAig, 3 );

    // get parameters
    Fra_ParamsDefaultSeq( pPars );
    pPars->nFramesP   = pParams->nFramesP;
    pPars->nFramesK   = pParams->nFramesK;
    pPars->nMaxImps   = pParams->nMaxImps;
    pPars->nMaxLevs   = pParams->nMaxLevs;
    pPars->fVerbose   = pParams->fVerbose;
    pPars->fRewrite   = pParams->fRewrite;
    pPars->fLatchCorr = pParams->fLatchCorr;
    pPars->fUseImps   = pParams->fUseImps;
    pPars->fWriteImps = pParams->fWriteImps;
    pPars->fUse1Hot   = pParams->fUse1Hot;

    assert( !(pPars->nFramesP > 0 && pPars->fUse1Hot) );
    assert( !(pPars->nFramesK > 1 && pPars->fUse1Hot) );
 
    // start the fraig manager for this run
    p = Fra_ManStart( pManAig, pPars );
    p->pPars->nBTLimitNode = 0;
    // derive and refine e-classes using K initialized frames
    if ( fUseOldSimulation )
    {
        if ( pPars->nFramesP > 0 )
        {
            pPars->nFramesP = 0;
            printf( "Fra_FraigInduction(): Prefix cannot be used.\n" );
        }
        p->pSml = Fra_SmlStart( pManAig, 0, pPars->nFramesK + 1, pPars->nSimWords );
        Fra_SmlSimulate( p, 1 );
    }
    else
    {
        // bug:  r iscas/blif/s5378.blif    ; st; ssw -v
        // bug:  r iscas/blif/s1238.blif    ; st; ssw -v
        // refine the classes with more simulation rounds
if ( pPars->fVerbose )
printf( "Simulating %d AIG nodes for %d cycles ... ", Aig_ManNodeNum(pManAig), pPars->nFramesP + 32 );
        p->pSml = Fra_SmlSimulateSeq( pManAig, pPars->nFramesP, 32, 1, 1  ); //pPars->nFramesK + 1, 1 );  
if ( pPars->fVerbose ) 
{
ABC_PRT( "Time", Abc_Clock() - clk );
}
        Fra_ClassesPrepare( p->pCla, p->pPars->fLatchCorr, p->pPars->nMaxLevs );
//        Fra_ClassesPostprocess( p->pCla );
        // compute one-hotness conditions
        if ( p->pPars->fUse1Hot )
            p->vOneHots = Fra_OneHotCompute( p, p->pSml );
        // allocate new simulation manager for simulating counter-examples
        Fra_SmlStop( p->pSml );
        p->pSml = Fra_SmlStart( pManAig, 0, pPars->nFramesK + 1, pPars->nSimWords );
    }

    // select the most expressive implications
    if ( pPars->fUseImps )
        p->pCla->vImps = Fra_ImpDerive( p, 5000000, pPars->nMaxImps, pPars->fLatchCorr );

    if ( pParams->TimeLimit != 0.0 && Abc_Clock() > TimeToStop )
    {
        if ( !pParams->fSilent )
            printf( "Fra_FraigInduction(): Runtime limit exceeded.\n" );
        goto finish;
    }

    // perform BMC (for the min number of frames)
    Fra_BmcPerform( p, pPars->nFramesP, pPars->nFramesK+1 ); // +1 is needed to prevent non-refinement
//Fra_ClassesPrint( p->pCla, 1 );
//    if ( p->vCex == NULL )
//        p->vCex = Vec_IntAlloc( 1000 );

    p->nLitsBeg  = Fra_ClassesCountLits( p->pCla );
    p->nNodesBeg = nNodesBeg; // Aig_ManNodeNum(pManAig);
    p->nRegsBeg  = nRegsBeg; // Aig_ManRegNum(pManAig);

    // dump AIG of the timeframes
//    pManAigNew = Fra_ClassesDeriveAig( p->pCla, pPars->nFramesK );
//    Aig_ManDumpBlif( pManAigNew, "frame_aig.blif", NULL, NULL );
//    Fra_ManPartitionTest2( pManAigNew );
//    Aig_ManStop( pManAigNew );
 
    // iterate the inductive case
    p->pCla->fRefinement = 1;
    for ( nIter = 0; p->pCla->fRefinement; nIter++ )
    {
        int nLitsOld = Fra_ClassesCountLits(p->pCla);
        int nImpsOld = p->pCla->vImps? Vec_IntSize(p->pCla->vImps) : 0;
        int nHotsOld = p->vOneHots? Fra_OneHotCount(p, p->vOneHots) : 0;
        abctime clk3 = Abc_Clock();

        if ( pParams->TimeLimit != 0.0 && Abc_Clock() > TimeToStop )
        {
            if ( !pParams->fSilent )
                printf( "Fra_FraigInduction(): Runtime limit exceeded.\n" );
            goto finish;
        }

        // mark the classes as non-refined
        p->pCla->fRefinement = 0;
        // derive non-init K-timeframes while implementing e-classes
clk2 = Abc_Clock();
        p->pManFraig = Fra_FramesWithClasses( p );
p->timeTrav += Abc_Clock() - clk2;
//Aig_ManDumpBlif( p->pManFraig, "testaig.blif", NULL, NULL );

        // perform AIG rewriting
        if ( p->pPars->fRewrite )
            Fra_FraigInductionRewrite( p );

        // convert the manager to SAT solver (the last nLatches outputs are inputs)
        if ( fUseSimpleCnf || pPars->fUseImps )
            pCnf = Cnf_DeriveSimple( p->pManFraig, Aig_ManRegNum(p->pManFraig) );
        else
            pCnf = Cnf_Derive( p->pManFraig, Aig_ManRegNum(p->pManFraig) );
//        Cnf_DataTranformPolarity( pCnf, 0 );
//Cnf_DataWriteIntoFile( pCnf, "temp.cnf", 1 );

        p->pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
        p->nSatVars = pCnf->nVars;
        assert( p->pSat != NULL );
        if ( p->pSat == NULL )
            printf( "Fra_FraigInduction(): Computed CNF is not valid.\n" );
        if ( pPars->fUseImps )
        {
            Fra_ImpAddToSolver( p, p->pCla->vImps, pCnf->pVarNums );
            if ( p->pSat == NULL )
                printf( "Fra_FraigInduction(): Adding implicationsn to CNF led to a conflict.\n" );
        }

        // set the pointers to the manager
        Aig_ManForEachObj( p->pManFraig, pObj, i )
            pObj->pData = p;

        // prepare solver for fraiging the last timeframe
        Fra_ManClean( p, Aig_ManObjNumMax(p->pManFraig) + Aig_ManNodeNum(p->pManAig) );

        // transfer PI/LO variable numbers
        Aig_ManForEachObj( p->pManFraig, pObj, i )
        {
            if ( pCnf->pVarNums[pObj->Id] == -1 )
                continue;
            Fra_ObjSetSatNum( pObj, pCnf->pVarNums[pObj->Id] );
            Fra_ObjSetFaninVec( pObj, (Vec_Ptr_t *)1 );
        }
        Cnf_DataFree( pCnf );

        // add one-hotness clauses
        if ( p->pPars->fUse1Hot )
            Fra_OneHotAssume( p, p->vOneHots );
//        if ( p->pManAig->vOnehots )
//            Fra_OneHotAddKnownConstraint( p, p->pManAig->vOnehots );
 
        // report the intermediate results
        if ( pPars->fVerbose )
        {
            printf( "%3d : C = %6d. Cl = %6d.  L = %6d. LR = %6d.  ", 
                nIter, Vec_PtrSize(p->pCla->vClasses1), Vec_PtrSize(p->pCla->vClasses), 
                Fra_ClassesCountLits(p->pCla), p->pManFraig->nAsserts );
            if ( p->pCla->vImps )
                printf( "I = %6d. ", Vec_IntSize(p->pCla->vImps) );
            if ( p->pPars->fUse1Hot )
                printf( "1h = %6d. ", Fra_OneHotCount(p, p->vOneHots) );
            printf( "NR = %6d. ", Aig_ManNodeNum(p->pManFraig) );
//            printf( "\n" );
        } 

        // perform sweeping
        p->nSatCallsRecent = 0;
        p->nSatCallsSkipped = 0;
clk2 = Abc_Clock();
        if ( p->pPars->fUse1Hot )
            Fra_OneHotCheck( p, p->vOneHots );
        Fra_FraigSweep( p );
        if ( pPars->fVerbose )
        {
            ABC_PRT( "T", Abc_Clock() - clk3 );
        } 

//        Sat_SolverPrintStats( stdout, p->pSat );
        // remove FRAIG and SAT solver
        Aig_ManStop( p->pManFraig );   p->pManFraig = NULL;
//        printf( "Vars = %d. Clauses = %d. Learnts = %d.\n", p->pSat->size, p->pSat->clauses.size, p->pSat->learnts.size );
        sat_solver_delete( p->pSat );  p->pSat = NULL; 
        memset( p->pMemFraig, 0, sizeof(Aig_Obj_t *) * p->nSizeAlloc * p->nFramesAll );
//        printf( "Recent SAT called = %d. Skipped = %d.\n", p->nSatCallsRecent, p->nSatCallsSkipped );
        assert( p->vTimeouts == NULL );
        if ( p->vTimeouts )
           printf( "Fra_FraigInduction(): SAT solver timed out!\n" );
        // check if refinement has happened
//        p->pCla->fRefinement = (int)(nLitsOld != Fra_ClassesCountLits(p->pCla));
        if ( p->pCla->fRefinement && 
            nLitsOld == Fra_ClassesCountLits(p->pCla) && 
            nImpsOld == (p->pCla->vImps? Vec_IntSize(p->pCla->vImps) : 0) && 
            nHotsOld == (p->vOneHots? Fra_OneHotCount(p, p->vOneHots) : 0) )
        {
            printf( "Fra_FraigInduction(): Internal error. The result may not verify.\n" );
            break;
        }
    }
/*
    // verify implications using simulation
    if ( p->pCla->vImps && Vec_IntSize(p->pCla->vImps) )
    {
        int Temp;
        abctime clk = Abc_Clock();
        if ( Temp = Fra_ImpVerifyUsingSimulation( p ) )
            printf( "Implications failing the simulation test = %d (out of %d).  ", Temp, Vec_IntSize(p->pCla->vImps) );
        else
            printf( "All %d implications have passed the simulation test.  ", Vec_IntSize(p->pCla->vImps) );
        ABC_PRT( "Time", Abc_Clock() - clk );
    }
*/

    // move the classes into representatives and reduce AIG
clk2 = Abc_Clock();
    if ( p->pPars->fWriteImps && p->vOneHots && Fra_OneHotCount(p, p->vOneHots) )
    {
        extern void Ioa_WriteAiger( Aig_Man_t * pMan, char * pFileName, int fWriteSymbols, int fCompact );
        Aig_Man_t * pNew;
        char * pFileName = Ioa_FileNameGenericAppend( p->pManAig->pName, "_care.aig" );
        printf( "Care one-hotness clauses will be written into file \"%s\".\n", pFileName );
        pManAigNew = Aig_ManDupOrdered( pManAig );
        pNew = Fra_OneHotCreateExdc( p, p->vOneHots );
        Ioa_WriteAiger( pNew, pFileName, 0, 1 );
        Aig_ManStop( pNew );
    }
    else 
    {
    //    Fra_ClassesPrint( p->pCla, 1 );
        Fra_ClassesSelectRepr( p->pCla );
        Fra_ClassesCopyReprs( p->pCla, p->vTimeouts );
        pManAigNew = Aig_ManDupRepr( pManAig, 0 );
    }
    // add implications to the manager
//    if ( fWriteImps && p->pCla->vImps && Vec_IntSize(p->pCla->vImps) )
//        Fra_ImpRecordInManager( p, pManAigNew );
    // cleanup the new manager
    Aig_ManSeqCleanup( pManAigNew );
    // remove pointers to the dead nodes
//    Aig_ManForEachObj( pManAig, pObj, i )
//        if ( pObj->pData && Aig_ObjIsNone(pObj->pData) )
//            pObj->pData = NULL;
//    Aig_ManCountMergeRegs( pManAigNew );
p->timeTrav += Abc_Clock() - clk2;
p->timeTotal = Abc_Clock() - clk;
    // get the final stats
    p->nLitsEnd  = Fra_ClassesCountLits( p->pCla );
    p->nNodesEnd = Aig_ManNodeNum(pManAigNew);
    p->nRegsEnd  = Aig_ManRegNum(pManAigNew);
    // free the manager
finish:
    Fra_ManStop( p );
    // check the output
//    if ( Aig_ManCoNum(pManAigNew) - Aig_ManRegNum(pManAigNew) == 1 )
//        if ( Aig_ObjChild0( Aig_ManCo(pManAigNew,0) ) == Aig_ManConst0(pManAigNew) )
//            printf( "Proved output constant 0.\n" );
    pParams->nIters = nIter;
    return pManAigNew;
}

/**Function*************************************************************

  Synopsis    [Outputs a set of pairs of equivalent nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_FraigInductionTest( char * pFileName, Fra_Ssw_t * pParams )
{
    FILE * pFile;
    char * pFilePairs;
    Aig_Man_t * pMan, * pNew;
    Aig_Obj_t * pObj, * pRepr;
    int * pNum2Id;
    int i, Counter = 0;
    pMan = Saig_ManReadBlif( pFileName );
    if ( pMan == NULL )
        return 0;
    // perform seq SAT sweeping
    pNew = Fra_FraigInduction( pMan, pParams );
    if ( pNew == NULL )
    {
        Aig_ManStop( pMan );
        return 0;
    }
    if ( pParams->fVerbose )
    {
        printf( "Original AIG: " );
        Aig_ManPrintStats( pMan );
        printf( "Reduced  AIG: " );
        Aig_ManPrintStats( pNew );
    }
    Aig_ManStop( pNew );
    pNum2Id = (int *)pMan->pData;
    // write the output file
    pFilePairs = Aig_FileNameGenericAppend( pFileName, ".pairs" );
    pFile = fopen( pFilePairs, "w" );
    Aig_ManForEachObj( pMan, pObj, i )
        if ( (pRepr = pMan->pReprs[pObj->Id]) )
        {
            fprintf( pFile, "%d %d %c\n", pNum2Id[pObj->Id], pNum2Id[pRepr->Id], (Aig_ObjPhase(pObj) ^ Aig_ObjPhase(pRepr))? '-' : '+' );
            Counter++;
        }
    fclose( pFile );
    if ( pParams->fVerbose )
    {
        printf( "Result: %d pairs of seq equiv nodes are written into file \"%s\".\n", Counter, pFilePairs );
    }
    Aig_ManStop( pMan );
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

