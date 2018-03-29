/**CFile****************************************************************

  FileName    [fraigMan.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [Implementation of the FRAIG manager.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - October 1, 2004]

  Revision    [$Id: fraigMan.c,v 1.11 2005/07/08 01:01:31 alanmi Exp $]

***********************************************************************/

#include "fraigInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

abctime timeSelect;
abctime timeAssign;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Sets the default parameters of the package.]

  Description [This set of parameters is tuned for equivalence checking.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Prove_ParamsSetDefault( Prove_Params_t * pParams )
{
    // clean the parameter structure 
    memset( pParams, 0, sizeof(Prove_Params_t) );
    // general parameters
    pParams->fUseFraiging         = 1;       // enables fraiging
    pParams->fUseRewriting        = 1;       // enables rewriting
    pParams->fUseBdds             = 0;       // enables BDD construction when other methods fail
    pParams->fVerbose             = 0;       // prints verbose stats
    // iterations
    pParams->nItersMax            = 6;       // the number of iterations
    // mitering 
    pParams->nMiteringLimitStart  = 5000;    // starting mitering limit
    pParams->nMiteringLimitMulti  = 2.0;     // multiplicative coefficient to increase the limit in each iteration
    // rewriting (currently not used)
    pParams->nRewritingLimitStart = 3;       // the number of rewriting iterations
    pParams->nRewritingLimitMulti = 1.0;     // multiplicative coefficient to increase the limit in each iteration
    // fraiging 
    pParams->nFraigingLimitStart  = 2;       // starting backtrack(conflict) limit
    pParams->nFraigingLimitMulti  = 8.0;     // multiplicative coefficient to increase the limit in each iteration
    // last-gasp BDD construction
    pParams->nBddSizeLimit        = 1000000; // the number of BDD nodes when construction is aborted
    pParams->fBddReorder          = 1;       // enables dynamic BDD variable reordering
    // last-gasp mitering
//    pParams->nMiteringLimitLast   = 1000000; // final mitering limit
    pParams->nMiteringLimitLast   = 0;       // final mitering limit
    // global SAT solver limits
    pParams->nTotalBacktrackLimit = 0;       // global limit on the number of backtracks
    pParams->nTotalInspectLimit   = 0;       // global limit on the number of clause inspects
//    pParams->nTotalInspectLimit   = 100000000;  // global limit on the number of clause inspects
}

/**Function*************************************************************

  Synopsis    [Prints out the current values of CEC engine parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Prove_ParamsPrint( Prove_Params_t * pParams )
{
    printf( "CEC enging parameters:\n" );
    printf( "Fraiging enabled: %s\n", pParams->fUseFraiging? "yes":"no" );
    printf( "Rewriting enabled: %s\n", pParams->fUseRewriting? "yes":"no" );
    printf( "BDD construction enabled: %s\n", pParams->fUseBdds? "yes":"no" );
    printf( "Verbose output enabled: %s\n", pParams->fVerbose? "yes":"no" );
    printf( "Solver iterations: %d\n", pParams->nItersMax );
    printf( "Starting mitering limit: %d\n", pParams->nMiteringLimitStart );
    printf( "Multiplicative coeficient for mitering: %.2f\n", pParams->nMiteringLimitMulti );
    printf( "Starting number of rewriting iterations: %d\n", pParams->nRewritingLimitStart );
    printf( "Multiplicative coeficient for rewriting: %.2f\n", pParams->nRewritingLimitMulti );
    printf( "Starting number of conflicts in fraiging: %.2f\n", pParams->nFraigingLimitMulti );
    printf( "Multiplicative coeficient for fraiging: %.2f\n", pParams->nRewritingLimitMulti );
    printf( "BDD size limit for bailing out: %d\n", pParams->nBddSizeLimit );
    printf( "BDD reordering enabled: %s\n", pParams->fBddReorder? "yes":"no" );
    printf( "Last-gasp mitering limit: %d\n", pParams->nMiteringLimitLast );
    printf( "Total conflict limit: %d\n", (int)pParams->nTotalBacktrackLimit );
    printf( "Total inspection limit: %d\n", (int)pParams->nTotalInspectLimit );
    printf( "Parameter dump complete.\n" );
}

/**Function*************************************************************

  Synopsis    [Sets the default parameters of the package.]

  Description [This set of parameters is tuned for equivalence checking.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_ParamsSetDefault( Fraig_Params_t * pParams )
{
    memset( pParams, 0, sizeof(Fraig_Params_t) );
    pParams->nPatsRand  = FRAIG_PATTERNS_RANDOM;  // the number of words of random simulation info
    pParams->nPatsDyna  = FRAIG_PATTERNS_DYNAMIC; // the number of words of dynamic simulation info
    pParams->nBTLimit   = 99;                     // the max number of backtracks to perform
    pParams->nSeconds   = 20;                     // the max number of seconds to solve the miter
    pParams->fFuncRed   =  1;                     // performs only one level hashing
    pParams->fFeedBack  =  1;                     // enables solver feedback
    pParams->fDist1Pats =  1;                     // enables distance-1 patterns
    pParams->fDoSparse  =  0;                     // performs equiv tests for sparse functions 
    pParams->fChoicing  =  0;                     // enables recording structural choices
    pParams->fTryProve  =  1;                     // tries to solve the final miter
    pParams->fVerbose   =  0;                     // the verbosiness flag
    pParams->fVerboseP  =  0;                     // the verbose flag for reporting the proof
    pParams->fInternal  =  0;                     // the flag indicates the internal run 
    pParams->nConfLimit =  0;                     // the limit on the number of conflicts
    pParams->nInspLimit =  0;                     // the limit on the number of inspections
}

/**Function*************************************************************

  Synopsis    [Sets the default parameters of the package.]

  Description [This set of parameters is tuned for complete FRAIGing.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_ParamsSetDefaultFull( Fraig_Params_t * pParams )
{
    memset( pParams, 0, sizeof(Fraig_Params_t) );
    pParams->nPatsRand  = FRAIG_PATTERNS_RANDOM;  // the number of words of random simulation info
    pParams->nPatsDyna  = FRAIG_PATTERNS_DYNAMIC; // the number of words of dynamic simulation info
    pParams->nBTLimit   = -1;                     // the max number of backtracks to perform
    pParams->nSeconds   = 20;                     // the max number of seconds to solve the miter
    pParams->fFuncRed   =  1;                     // performs only one level hashing
    pParams->fFeedBack  =  1;                     // enables solver feedback
    pParams->fDist1Pats =  1;                     // enables distance-1 patterns
    pParams->fDoSparse  =  1;                     // performs equiv tests for sparse functions 
    pParams->fChoicing  =  0;                     // enables recording structural choices
    pParams->fTryProve  =  0;                     // tries to solve the final miter
    pParams->fVerbose   =  0;                     // the verbosiness flag
    pParams->fVerboseP  =  0;                     // the verbose flag for reporting the proof
    pParams->fInternal  =  0;                     // the flag indicates the internal run 
    pParams->nConfLimit =  0;                     // the limit on the number of conflicts
    pParams->nInspLimit =  0;                     // the limit on the number of inspections
}

/**Function*************************************************************

  Synopsis    [Creates the new FRAIG manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Man_t * Fraig_ManCreate( Fraig_Params_t * pParams )
{
    Fraig_Params_t Params;
    Fraig_Man_t * p;

    // set the random seed for simulation
//    srand( 0xFEEDDEAF );
//    srand( 0xDEADCAFE );
    Aig_ManRandom( 1 );

    // set parameters for equivalence checking
    if ( pParams == NULL )
        Fraig_ParamsSetDefault( pParams = &Params );
    // adjust the amount of simulation info
    if ( pParams->nPatsRand < 128 )
        pParams->nPatsRand = 128;
    if ( pParams->nPatsRand > 32768 )
        pParams->nPatsRand = 32768;
    if ( pParams->nPatsDyna < 128 )
        pParams->nPatsDyna = 128;
    if ( pParams->nPatsDyna > 32768 )
        pParams->nPatsDyna = 32768;
    // if reduction is not performed, allocate minimum simulation info
    if ( !pParams->fFuncRed )
        pParams->nPatsRand = pParams->nPatsDyna = 128;

    // start the manager
    p = ABC_ALLOC( Fraig_Man_t, 1 );
    memset( p, 0, sizeof(Fraig_Man_t) );

    // set the default parameters
    p->nWordsRand = FRAIG_NUM_WORDS( pParams->nPatsRand );  // the number of words of random simulation info
    p->nWordsDyna = FRAIG_NUM_WORDS( pParams->nPatsDyna );  // the number of patterns for dynamic simulation info
    p->nBTLimit   = pParams->nBTLimit;    // -1 means infinite backtrack limit
    p->nSeconds   = pParams->nSeconds;    // the timeout for the final miter
    p->fFuncRed   = pParams->fFuncRed;    // enables functional reduction (otherwise, only one-level hashing is performed)
    p->fFeedBack  = pParams->fFeedBack;   // enables solver feedback (the use of counter-examples in simulation)
    p->fDist1Pats = pParams->fDist1Pats;  // enables solver feedback (the use of counter-examples in simulation)
    p->fDoSparse  = pParams->fDoSparse;   // performs equivalence checking for sparse functions (whose sim-info is 0)
    p->fChoicing  = pParams->fChoicing;   // disable accumulation of structural choices (keeps only the first choice)
    p->fTryProve  = pParams->fTryProve;   // disable accumulation of structural choices (keeps only the first choice)
    p->fVerbose   = pParams->fVerbose;    // disable verbose output
    p->fVerboseP  = pParams->fVerboseP;   // disable verbose output
    p->nInspLimit = pParams->nInspLimit;  // the limit on the number of inspections

    // start memory managers
    p->mmNodes    = Fraig_MemFixedStart( sizeof(Fraig_Node_t) );
    p->mmSims     = Fraig_MemFixedStart( sizeof(unsigned) * (p->nWordsRand + p->nWordsDyna) );
    // allocate node arrays
    p->vInputs    = Fraig_NodeVecAlloc( 1000 );    // the array of primary inputs
    p->vOutputs   = Fraig_NodeVecAlloc( 1000 );    // the array of primary outputs
    p->vNodes     = Fraig_NodeVecAlloc( 1000 );    // the array of internal nodes
    // start the tables
    p->pTableS    = Fraig_HashTableCreate( 1000 ); // hashing by structure
    p->pTableF    = Fraig_HashTableCreate( 1000 ); // hashing by function
    p->pTableF0   = Fraig_HashTableCreate( 1000 ); // hashing by function (for sparse functions)
    // create the constant node
    p->pConst1    = Fraig_NodeCreateConst( p );
    // initialize SAT solver feedback data structures
    Fraig_FeedBackInit( p );
    // initialize other variables
    p->vProj      = Msat_IntVecAlloc( 10 ); 
    p->nTravIds   = 1;
    p->nTravIds2  = 1;
    return p;
}

/**Function*************************************************************

  Synopsis    [Deallocates the mapping manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_ManFree( Fraig_Man_t * p )
{
    int i;
    if ( p->fVerbose )   
    {
        if ( p->fChoicing ) Fraig_ManReportChoices( p );
        Fraig_ManPrintStats( p );
//        Fraig_TablePrintStatsS( p );
//        Fraig_TablePrintStatsF( p );
//        Fraig_TablePrintStatsF0( p );
    }
 
    for ( i = 0; i < p->vNodes->nSize; i++ )
        if ( p->vNodes->pArray[i]->vFanins )
        {
            Fraig_NodeVecFree( p->vNodes->pArray[i]->vFanins );
            p->vNodes->pArray[i]->vFanins = NULL;
        }

    if ( p->vInputs )    Fraig_NodeVecFree( p->vInputs );
    if ( p->vNodes )     Fraig_NodeVecFree( p->vNodes );
    if ( p->vOutputs )   Fraig_NodeVecFree( p->vOutputs );

    if ( p->pTableS )    Fraig_HashTableFree( p->pTableS );
    if ( p->pTableF )    Fraig_HashTableFree( p->pTableF );
    if ( p->pTableF0 )   Fraig_HashTableFree( p->pTableF0 );

    if ( p->pSat )       Msat_SolverFree( p->pSat );
    if ( p->vProj )      Msat_IntVecFree( p->vProj );
    if ( p->vCones )     Fraig_NodeVecFree( p->vCones );
    if ( p->vPatsReal )  Msat_IntVecFree( p->vPatsReal );
    if ( p->pModel )     ABC_FREE( p->pModel );

    Fraig_MemFixedStop( p->mmNodes, 0 );
    Fraig_MemFixedStop( p->mmSims, 0 );

    if ( p->pSuppS )
    {
        ABC_FREE( p->pSuppS[0] );
        ABC_FREE( p->pSuppS );
    }
    if ( p->pSuppF )
    {
        ABC_FREE( p->pSuppF[0] );
        ABC_FREE( p->pSuppF );
    }

    ABC_FREE( p->ppOutputNames );
    ABC_FREE( p->ppInputNames );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Prepares the SAT solver to run on the two nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_ManCreateSolver( Fraig_Man_t * p )
{
    extern abctime timeSelect;
    extern abctime timeAssign;
    assert( p->pSat == NULL );
    // allocate data for SAT solving
    p->pSat       = Msat_SolverAlloc( 500, 1, 1, 1, 1, 0 );
    p->vVarsInt   = Msat_SolverReadConeVars( p->pSat );   
    p->vAdjacents = Msat_SolverReadAdjacents( p->pSat );
    p->vVarsUsed  = Msat_SolverReadVarsUsed( p->pSat );
    timeSelect = 0;
    timeAssign = 0;
}

 
/**Function*************************************************************

  Synopsis    [Deallocates the mapping manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_ManPrintStats( Fraig_Man_t * p )
{
    double nMemory;
    nMemory = ((double)(p->vInputs->nSize + p->vNodes->nSize) * 
        (sizeof(Fraig_Node_t) + sizeof(unsigned)*(p->nWordsRand + p->nWordsDyna) /*+ p->nSuppWords*sizeof(unsigned)*/))/(1<<20);
    printf( "Words: Random = %d. Dynamic = %d. Used = %d. Memory = %0.2f MB.\n", 
        p->nWordsRand, p->nWordsDyna, p->iWordPerm, nMemory );
    printf( "Proof = %d. Counter-example = %d. Fail = %d. FailReal = %d. Zero = %d.\n", 
        p->nSatProof, p->nSatCounter, p->nSatFails, p->nSatFailsReal, p->nSatZeros );
    printf( "Nodes: Final = %d. Total = %d. Mux = %d. (Exor = %d.) ClaVars = %d.\n", 
        Fraig_CountNodes(p,0), p->vNodes->nSize, Fraig_ManCountMuxes(p), Fraig_ManCountExors(p), p->nVarsClauses );
    if ( p->pSat ) Msat_SolverPrintStats( p->pSat );
    Fraig_PrintTime( "AIG simulation  ", p->timeSims  );
    Fraig_PrintTime( "AIG traversal   ", p->timeTrav  );
    Fraig_PrintTime( "Solver feedback ", p->timeFeed  );
    Fraig_PrintTime( "SAT solving     ", p->timeSat   );
    Fraig_PrintTime( "Network update  ", p->timeToNet );
    Fraig_PrintTime( "TOTAL RUNTIME   ", p->timeTotal );
    if ( p->time1 > 0 ) { Fraig_PrintTime( "time1", p->time1 ); }
    if ( p->time2 > 0 ) { Fraig_PrintTime( "time2", p->time2 ); }
    if ( p->time3 > 0 ) { Fraig_PrintTime( "time3", p->time3 ); }
    if ( p->time4 > 0 ) { Fraig_PrintTime( "time4", p->time4 ); }
//    ABC_PRT( "Selection ", timeSelect );
//    ABC_PRT( "Assignment", timeAssign );
    fflush( stdout );
}

/**Function*************************************************************

  Synopsis    [Allocates simulation information for all nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_NodeVec_t * Fraig_UtilInfoAlloc( int nSize, int nWords, int fClean )
{
    Fraig_NodeVec_t * vInfo;
    unsigned * pUnsigned;
    int i;
    assert( nSize > 0 && nWords > 0 );
    vInfo = Fraig_NodeVecAlloc( nSize );
    pUnsigned = ABC_ALLOC( unsigned, nSize * nWords );
    vInfo->pArray[0] = (Fraig_Node_t *)pUnsigned;
    if ( fClean )
        memset( pUnsigned, 0, sizeof(unsigned) * nSize * nWords );
    for ( i = 1; i < nSize; i++ )
        vInfo->pArray[i] = (Fraig_Node_t *)(((unsigned *)vInfo->pArray[i-1]) + nWords);
    vInfo->nSize = nSize;
    return vInfo;
}

/**Function*************************************************************

  Synopsis    [Returns simulation info of all nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_NodeVec_t * Fraig_ManGetSimInfo( Fraig_Man_t * p )
{
    Fraig_NodeVec_t * vInfo;
    Fraig_Node_t * pNode;
    unsigned * pUnsigned;
    int nRandom, nDynamic;
    int i, k, nWords;

    nRandom  = Fraig_ManReadPatternNumRandom( p );
    nDynamic = Fraig_ManReadPatternNumDynamic( p );
    nWords = nRandom / 32 + nDynamic / 32;

    vInfo = Fraig_UtilInfoAlloc( p->vNodes->nSize, nWords, 0 );
    for ( i = 0; i < p->vNodes->nSize; i++ )
    {
        pNode = p->vNodes->pArray[i];
        assert( i == pNode->Num );
        pUnsigned = (unsigned *)vInfo->pArray[i];
        for ( k = 0; k < nRandom / 32; k++ )
            pUnsigned[k] = pNode->puSimR[k];
        for ( k = 0; k < nDynamic / 32; k++ )
            pUnsigned[nRandom / 32 + k] = pNode->puSimD[k];
    }
    return vInfo;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if A v B is always true based on the siminfo.]

  Description [A v B is always true iff A' * B' is always false.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_ManCheckClauseUsingSimInfo( Fraig_Man_t * p, Fraig_Node_t * pNode1, Fraig_Node_t * pNode2 )
{
    int fCompl1, fCompl2, i;

    fCompl1 = 1 ^ Fraig_IsComplement(pNode1) ^ Fraig_Regular(pNode1)->fInv;
    fCompl2 = 1 ^ Fraig_IsComplement(pNode2) ^ Fraig_Regular(pNode2)->fInv;

    pNode1 = Fraig_Regular(pNode1);
    pNode2 = Fraig_Regular(pNode2);
    assert( pNode1 != pNode2 );
    
    // check the simulation info
    if ( fCompl1 && fCompl2 )
    {
        for ( i = 0; i < p->nWordsRand; i++ )
            if ( ~pNode1->puSimR[i] & ~pNode2->puSimR[i] )
                return 0;
        for ( i = 0; i < p->iWordStart; i++ )
            if ( ~pNode1->puSimD[i] & ~pNode2->puSimD[i] )
                return 0;
        return 1;
    }
    if ( !fCompl1 && fCompl2 )
    {
        for ( i = 0; i < p->nWordsRand; i++ )
            if ( pNode1->puSimR[i] & ~pNode2->puSimR[i] )
                return 0;
        for ( i = 0; i < p->iWordStart; i++ )
            if ( pNode1->puSimD[i] & ~pNode2->puSimD[i] )
                return 0;
        return 1;
    }
    if ( fCompl1 && !fCompl2 )
    {
        for ( i = 0; i < p->nWordsRand; i++ )
            if ( ~pNode1->puSimR[i] & pNode2->puSimR[i] )
                return 0;
        for ( i = 0; i < p->iWordStart; i++ )
            if ( ~pNode1->puSimD[i] & pNode2->puSimD[i] )
                return 0;
        return 1;
    }
//    if ( fCompl1 && fCompl2 )
    {
        for ( i = 0; i < p->nWordsRand; i++ )
            if ( pNode1->puSimR[i] & pNode2->puSimR[i] )
                return 0;
        for ( i = 0; i < p->iWordStart; i++ )
            if ( pNode1->puSimD[i] & pNode2->puSimD[i] )
                return 0;
        return 1;
    }
}

/**Function*************************************************************

  Synopsis    [Adds clauses to the solver.]

  Description [This procedure is used to add external clauses to the solver.
  The clauses are given by sets of nodes. Each node stands for one literal.
  If the node is complemented, the literal is negated.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_ManAddClause( Fraig_Man_t * p, Fraig_Node_t ** ppNodes, int nNodes )
{
    Fraig_Node_t * pNode;
    int i, fComp, RetValue;
    if ( p->pSat == NULL )
        Fraig_ManCreateSolver( p );
    // create four clauses
    Msat_IntVecClear( p->vProj );
    for ( i = 0; i < nNodes; i++ )
    {
        pNode = Fraig_Regular(ppNodes[i]);
        fComp = Fraig_IsComplement(ppNodes[i]);
        Msat_IntVecPush( p->vProj, MSAT_VAR2LIT(pNode->Num, fComp) );
//        printf( "%d(%d) ", pNode->Num, fComp );
    }
//    printf( "\n" );
    RetValue = Msat_SolverAddClause( p->pSat, p->vProj );
    assert( RetValue );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END

