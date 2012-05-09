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
    pPars->nSimWords        =      32;  // the number of words in the simulation info
    pPars->dSimSatur        =   0.005;  // the ratio of refined classes when saturation is reached
    pPars->fPatScores       =       0;  // enables simulation pattern scoring
    pPars->MaxScore         =      25;  // max score after which resimulation is used
    pPars->fDoSparse        =       1;  // skips sparse functions
//    pPars->dActConeRatio    =    0.05;  // the ratio of cone to be bumped
//    pPars->dActConeBumpMax  =     5.0;  // the largest bump of activity
    pPars->dActConeRatio    =     0.3;  // the ratio of cone to be bumped
    pPars->dActConeBumpMax  =    10.0;  // the largest bump of activity
    pPars->nBTLimitNode     =     100;  // conflict limit at a node
    pPars->nBTLimitMiter    =  500000;  // conflict limit at an output
    pPars->nFramesK         =       0;  // the number of timeframes to unroll
    pPars->fConeBias        =       1;
    pPars->fRewrite         =       0;
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
    pPars->nSimWords        =       1;  // the number of words in the simulation info
    pPars->dSimSatur        =   0.005;  // the ratio of refined classes when saturation is reached
    pPars->fPatScores       =       0;  // enables simulation pattern scoring
    pPars->MaxScore         =      25;  // max score after which resimulation is used
    pPars->fDoSparse        =       1;  // skips sparse functions
//    pPars->dActConeRatio    =    0.05;  // the ratio of cone to be bumped
//    pPars->dActConeBumpMax  =     5.0;  // the largest bump of activity
    pPars->dActConeRatio    =     0.3;  // the ratio of cone to be bumped
    pPars->dActConeBumpMax  =    10.0;  // the largest bump of activity
    pPars->nBTLimitNode     =10000000;  // conflict limit at a node
    pPars->nBTLimitMiter    =  500000;  // conflict limit at an output
    pPars->nFramesK         =       1;  // the number of timeframes to unroll
    pPars->fConeBias        =       0;
    pPars->fRewrite         =       0;
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
    p = ALLOC( Fra_Man_t, 1 );
    memset( p, 0, sizeof(Fra_Man_t) );
    p->pPars      = pPars;
    p->pManAig    = pManAig;
    p->nSizeAlloc = Aig_ManObjIdMax( pManAig ) + 1;
    p->nFramesAll = pPars->nFramesK + 1;
    // allocate simulation info
    p->nSimWords  = pPars->nSimWords * p->nFramesAll;
    p->pSimWords  = ALLOC( unsigned, p->nSizeAlloc * p->nSimWords );
    // clean simulation info of the constant node
    memset( p->pSimWords, 0, sizeof(unsigned) * p->nSizeAlloc * p->nSimWords );
    // allocate storage for sim pattern
    p->nPatWords  = Aig_BitWordNum( Aig_ManPiNum(pManAig) * p->nFramesAll );
    p->pPatWords  = ALLOC( unsigned, p->nPatWords ); 
    p->pPatScores = ALLOC( int, 32 * p->nSimWords ); 
    p->vPiVars    = Vec_PtrAlloc( 100 );
    // equivalence classes
    p->pCla       = Fra_ClassesStart( pManAig );
    // allocate other members
    p->pMemFraig  = ALLOC( Aig_Obj_t *, p->nSizeAlloc * p->nFramesAll );
    memset( p->pMemFraig, 0, sizeof(Aig_Obj_t *) * p->nSizeAlloc * p->nFramesAll );
    p->pMemFanins = ALLOC( Vec_Ptr_t *, p->nSizeAlloc * p->nFramesAll + 10000 );
    memset( p->pMemFanins, 0, sizeof(Vec_Ptr_t *) * p->nSizeAlloc * p->nFramesAll + 10000 );
    p->pMemSatNums = ALLOC( int, p->nSizeAlloc * p->nFramesAll + 10000 );
    memset( p->pMemSatNums, 0, sizeof(int) * p->nSizeAlloc * p->nFramesAll + 10000 );
    // set random number generator
    srand( 0xABCABC );
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
void Fra_ManClean( Fra_Man_t * p )
{
    int i, Limit;

    Limit = Aig_ManObjIdMax(p->pManFraig) + 1;
    for ( i = 0; i < Limit; i++ )
        if ( p->pMemFanins[i] && p->pMemFanins[i] != (void *)1 )
            Vec_PtrFree( p->pMemFanins[i] );

    memset( p->pMemFraig, 0, sizeof(Aig_Obj_t *) * p->nSizeAlloc * p->nFramesAll );
    memset( p->pMemFanins, 0, sizeof(Vec_Ptr_t *) * Limit );
    memset( p->pMemSatNums, 0, sizeof(int) * Limit );

    Aig_ManStop( p->pManFraig );   
    p->pManFraig = NULL;

    sat_solver_delete( p->pSat );  
    p->pSat = NULL; 
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
    pManFraig = Aig_ManStart( Aig_ManObjIdMax(p->pManAig) + 1 );
    // set the pointers to the available fraig nodes
    Fra_ObjSetFraig( Aig_ManConst1(p->pManAig), 0, Aig_ManConst1(pManFraig) );
    Aig_ManForEachPi( p->pManAig, pObj, i )
        Fra_ObjSetFraig( pObj, 0, Aig_ObjCreatePi(pManFraig) );
    // set the pointers to the manager
    Aig_ManForEachObj( pManFraig, pObj, i )
        pObj->pData = p;
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
    Aig_ManForEachPo( p->pManAig, pObj, i )
        Aig_ObjCreatePo( p->pManFraig, Fra_ObjChild0Fra(pObj,0) );
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
    int i;
    for ( i = 0; i < p->nSizeAlloc; i++ )
        if ( p->pMemFanins[i] && p->pMemFanins[i] != (void *)1 )
            Vec_PtrFree( p->pMemFanins[i] );
    if ( p->pPars->fVerbose )
        Fra_ManPrint( p );
    if ( p->vTimeouts ) Vec_PtrFree( p->vTimeouts );
    if ( p->vPiVars )   Vec_PtrFree( p->vPiVars );
    if ( p->pSat )      sat_solver_delete( p->pSat );
    if ( p->pCla  )     Fra_ClassesStop( p->pCla );
    FREE( p->pMemFraig );
    FREE( p->pMemFanins );
    FREE( p->pMemSatNums );
    FREE( p->pPatScores );
    FREE( p->pPatWords );
    FREE( p->pSimWords );
    free( p );
}

/**Function*************************************************************

  Synopsis    [Prints stats for the fraiging manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ManPrint( Fra_Man_t * p )
{
    double nMemory = 1.0*Aig_ManObjIdMax(p->pManAig)*((p->nSimWords+2)*sizeof(unsigned)+6*sizeof(void*))/(1<<20);
    printf( "SimWord = %d. Round = %d.  Mem = %0.2f Mb.  LitBeg = %d.  LitEnd = %d. (%6.2f %%).\n", 
        p->nSimWords, p->nSimRounds, nMemory, p->nLitsBeg, p->nLitsEnd, 100.0*p->nLitsEnd/p->nLitsBeg );
    printf( "Proof = %d. Cex = %d. Fail = %d. FailReal = %d. Zero = %d. C-lim = %d. Vars = %d.\n", 
        p->nSatProof, p->nSatCallsSat, p->nSatFails, p->nSatFailsReal, p->nLitsZero, p->pPars->nBTLimitNode, p->nSatVars );
    printf( "NBeg = %d. NEnd = %d. (Gain = %6.2f %%).  RBeg = %d. REnd = %d. (Gain = %6.2f %%).\n", 
        p->nNodesBeg, p->nNodesEnd, 100.0*(p->nNodesBeg-p->nNodesEnd)/p->nNodesBeg, 
        p->nRegsBeg, p->nRegsEnd, 100.0*(p->nRegsBeg-p->nRegsEnd)/p->nRegsBeg );
    if ( p->pSat ) Sat_SolverPrintStats( stdout, p->pSat );
    PRT( "AIG simulation  ", p->timeSim  );
    PRT( "AIG traversal   ", p->timeTrav );
    if ( p->timeRwr )
    {
    PRT( "AIG rewriting   ", p->timeRwr  );
    }
    PRT( "SAT solving     ", p->timeSat  );
    PRT( "    Unsat       ", p->timeSatUnsat );
    PRT( "    Sat         ", p->timeSatSat   );
    PRT( "    Fail        ", p->timeSatFail  );
    PRT( "Class refining  ", p->timeRef   );
    PRT( "TOTAL RUNTIME   ", p->timeTotal );
    if ( p->time1 ) { PRT( "time1           ", p->time1 ); }
    if ( p->nSpeculs )
    printf( "Speculations = %d.\n", p->nSpeculs );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


