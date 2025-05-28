/**CFile****************************************************************

  FileName    [fraMan.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [New FRAIG package.]

  Synopsis    [Starts the FRAIG manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 30, 2007.]

  Revision    [$Id: fraMan.c,v 1.00 2007/06/30 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fra.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Sets the default solving parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ParamsDefault( Fra_Par_t * pPars )
{
    memset( pPars, 0, sizeof(Fra_Par_t) );
    pPars->nSimWords        =       32;  // the number of words in the simulation info
    pPars->dSimSatur        =    0.005;  // the ratio of refined classes when saturation is reached
    pPars->fPatScores       =        0;  // enables simulation pattern scoring
    pPars->MaxScore         =       25;  // max score after which resimulation is used
    pPars->fDoSparse        =        1;  // skips sparse functions
//    pPars->dActConeRatio    =     0.05;  // the ratio of cone to be bumped
//    pPars->dActConeBumpMax  =      5.0;  // the largest bump of activity
    pPars->dActConeRatio    =      0.3;  // the ratio of cone to be bumped
    pPars->dActConeBumpMax  =     10.0;  // the largest bump of activity
    pPars->nBTLimitNode     =      100;  // conflict limit at a node
    pPars->nBTLimitMiter    =   500000;  // conflict limit at an output
    pPars->nFramesK         =        0;  // the number of timeframes to unroll
    pPars->fConeBias        =        1;
    pPars->fRewrite         =        0;
}

/**Function*************************************************************

  Synopsis    [Sets the default solving parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ParamsDefaultSeq( Fra_Par_t * pPars )
{
    memset( pPars, 0, sizeof(Fra_Par_t) );
    pPars->nSimWords        =        1;  // the number of words in the simulation info
    pPars->dSimSatur        =    0.005;  // the ratio of refined classes when saturation is reached
    pPars->fPatScores       =        0;  // enables simulation pattern scoring
    pPars->MaxScore         =       25;  // max score after which resimulation is used
    pPars->fDoSparse        =        1;  // skips sparse functions
    pPars->dActConeRatio    =      0.3;  // the ratio of cone to be bumped
    pPars->dActConeBumpMax  =     10.0;  // the largest bump of activity
    pPars->nBTLimitNode     = 10000000;  // conflict limit at a node
    pPars->nBTLimitMiter    =   500000;  // conflict limit at an output
    pPars->nFramesK         =        1;  // the number of timeframes to unroll
    pPars->fConeBias        =        0;
    pPars->fRewrite         =        0;
    pPars->fLatchCorr       =        0;
} 

/**Function*************************************************************

  Synopsis    [Starts the fraiging manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fra_Man_t * Fra_ManStart( Aig_Man_t * pManAig, Fra_Par_t * pPars )
{
    Fra_Man_t * p;
    Aig_Obj_t * pObj;
    int i;
    // allocate the fraiging manager
    p = ABC_ALLOC( Fra_Man_t, 1 );
    memset( p, 0, sizeof(Fra_Man_t) );
    p->pPars      = pPars;
    p->pManAig    = pManAig;
    p->nSizeAlloc = Aig_ManObjNumMax( pManAig );
    p->nFramesAll = pPars->nFramesK + 1;
    // allocate storage for sim pattern
    p->nPatWords  = Abc_BitWordNum( (Aig_ManCiNum(pManAig) - Aig_ManRegNum(pManAig)) * p->nFramesAll + Aig_ManRegNum(pManAig) );
    p->pPatWords  = ABC_ALLOC( unsigned, p->nPatWords ); 
    p->vPiVars    = Vec_PtrAlloc( 100 );
    // equivalence classes
    p->pCla       = Fra_ClassesStart( pManAig );
    // allocate other members
    p->pMemFraig  = ABC_ALLOC( Aig_Obj_t *, p->nSizeAlloc * p->nFramesAll );
    memset( p->pMemFraig, 0, sizeof(Aig_Obj_t *) * p->nSizeAlloc * p->nFramesAll );
    // set random number generator
//    srand( 0xABCABC );
    Aig_ManRandom(1);
    // set the pointer to the manager
    Aig_ManForEachObj( p->pManAig, pObj, i )
        pObj->pData = p;
    return p;
}

/**Function*************************************************************

  Synopsis    [Starts the fraiging manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ManClean( Fra_Man_t * p, int nNodesMax )
{
    int i;
    // remove old arrays
    for ( i = 0; i < p->nMemAlloc; i++ )
        if ( p->pMemFanins[i] && p->pMemFanins[i] != (void *)1 )
            Vec_PtrFree( p->pMemFanins[i] );
    // realloc for the new size
    if ( p->nMemAlloc < nNodesMax )
    {
        int nMemAllocNew = nNodesMax + 5000;
        p->pMemFanins = ABC_REALLOC( Vec_Ptr_t *, p->pMemFanins, nMemAllocNew );
        p->pMemSatNums = ABC_REALLOC( int, p->pMemSatNums, nMemAllocNew );
        p->nMemAlloc = nMemAllocNew;
    }
    // prepare for the new run
    memset( p->pMemFanins, 0, sizeof(Vec_Ptr_t *) * p->nMemAlloc );
    memset( p->pMemSatNums, 0, sizeof(int) * p->nMemAlloc );
}

/**Function*************************************************************

  Synopsis    [Prepares the new manager to begin fraiging.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Fra_ManPrepareComb( Fra_Man_t * p )
{
    Aig_Man_t * pManFraig;
    Aig_Obj_t * pObj;
    int i;
    assert( p->pManFraig == NULL );
    // start the fraig package
    pManFraig = Aig_ManStart( Aig_ManObjNumMax(p->pManAig) );
    pManFraig->pName = Abc_UtilStrsav( p->pManAig->pName );
    pManFraig->pSpec = Abc_UtilStrsav( p->pManAig->pSpec );
    pManFraig->nRegs    = p->pManAig->nRegs;
    pManFraig->nAsserts = p->pManAig->nAsserts;
    // set the pointers to the available fraig nodes
    Fra_ObjSetFraig( Aig_ManConst1(p->pManAig), 0, Aig_ManConst1(pManFraig) );
    Aig_ManForEachCi( p->pManAig, pObj, i )
        Fra_ObjSetFraig( pObj, 0, Aig_ObjCreateCi(pManFraig) );
    // set the pointers to the manager
    Aig_ManForEachObj( pManFraig, pObj, i )
        pObj->pData = p;
    // allocate memory for mapping FRAIG nodes into SAT numbers and fanins
    p->nMemAlloc = p->nSizeAlloc;
    p->pMemFanins = ABC_ALLOC( Vec_Ptr_t *, p->nMemAlloc );
    memset( p->pMemFanins, 0, sizeof(Vec_Ptr_t *) * p->nMemAlloc );
    p->pMemSatNums = ABC_ALLOC( int, p->nMemAlloc );
    memset( p->pMemSatNums, 0, sizeof(int) * p->nMemAlloc );
    // make sure the satisfying assignment is node assigned
    assert( pManFraig->pData == NULL );
    return pManFraig;
}

/**Function*************************************************************

  Synopsis    [Finalizes the combinational miter after fraiging.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ManFinalizeComb( Fra_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i;
    // add the POs
    Aig_ManForEachCo( p->pManAig, pObj, i )
        Aig_ObjCreateCo( p->pManFraig, Fra_ObjChild0Fra(pObj,0) );
    // postprocess
    Aig_ManCleanMarkB( p->pManFraig );
}


/**Function*************************************************************

  Synopsis    [Stops the fraiging manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ManStop( Fra_Man_t * p )
{
    if ( p->pPars->fVerbose )
        Fra_ManPrint( p );
    // save mapping from original nodes into FRAIG nodes
    if ( p->pManAig )
    {
        if ( p->pManAig->pObjCopies )
            ABC_FREE( p->pManAig->pObjCopies );
        p->pManAig->pObjCopies = p->pMemFraig;
        p->pMemFraig = NULL;
    }
    Fra_ManClean( p, 0 );
    if ( p->vTimeouts ) Vec_PtrFree( p->vTimeouts );
    if ( p->vPiVars )   Vec_PtrFree( p->vPiVars );
    if ( p->pSat )      sat_solver_delete( p->pSat );
    if ( p->pCla  )     Fra_ClassesStop( p->pCla );
    if ( p->pSml )      Fra_SmlStop( p->pSml );
    if ( p->vCex )      Vec_IntFree( p->vCex );
    if ( p->vOneHots )  Vec_IntFree( p->vOneHots );
    ABC_FREE( p->pMemFraig );
    ABC_FREE( p->pMemFanins );
    ABC_FREE( p->pMemSatNums );
    ABC_FREE( p->pPatWords );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Prints stats for the fraiging manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ManPrint( Fra_Man_t * p )
{
    double nMemory = 1.0*Aig_ManObjNumMax(p->pManAig)*(p->pSml->nWordsTotal*sizeof(unsigned)+6*sizeof(void*))/(1<<20);
    printf( "SimWord = %d. Round = %d.  Mem = %0.2f MB.  LitBeg = %d.  LitEnd = %d. (%6.2f %%).\n", 
        p->pPars->nSimWords, p->pSml->nSimRounds, nMemory, p->nLitsBeg, p->nLitsEnd, 100.0*p->nLitsEnd/(p->nLitsBeg?p->nLitsBeg:1) );
    printf( "Proof = %d. Cex = %d. Fail = %d. FailReal = %d. C-lim = %d. ImpRatio = %6.2f %%\n", 
        p->nSatProof, p->nSatCallsSat, p->nSatFails, p->nSatFailsReal, p->pPars->nBTLimitNode, Fra_ImpComputeStateSpaceRatio(p) );
    printf( "NBeg = %d. NEnd = %d. (Gain = %6.2f %%).  RBeg = %d. REnd = %d. (Gain = %6.2f %%).\n", 
        p->nNodesBeg, p->nNodesEnd, 100.0*(p->nNodesBeg-p->nNodesEnd)/(p->nNodesBeg?p->nNodesBeg:1), 
        p->nRegsBeg, p->nRegsEnd, 100.0*(p->nRegsBeg-p->nRegsEnd)/(p->nRegsBeg?p->nRegsBeg:1) );
    if ( p->pSat )             Sat_SolverPrintStats( stdout, p->pSat );
    if ( p->pPars->fUse1Hot )  Fra_OneHotEstimateCoverage( p, p->vOneHots );
    ABC_PRT( "AIG simulation  ", p->pSml->timeSim  );
    ABC_PRT( "AIG traversal   ", p->timeTrav );
    if ( p->timeRwr )
    {
    ABC_PRT( "AIG rewriting   ", p->timeRwr  );
    }
    ABC_PRT( "SAT solving     ", p->timeSat  );
    ABC_PRT( "    Unsat       ", p->timeSatUnsat );
    ABC_PRT( "    Sat         ", p->timeSatSat   );
    ABC_PRT( "    Fail        ", p->timeSatFail  );
    ABC_PRT( "Class refining  ", p->timeRef   );
    ABC_PRT( "TOTAL RUNTIME   ", p->timeTotal );
    if ( p->time1 ) { ABC_PRT( "time1           ", p->time1 ); }
    if ( p->nSpeculs )
    printf( "Speculations = %d.\n", p->nSpeculs );
    fflush( stdout );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

