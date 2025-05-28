/**CFile****************************************************************

  FileName    [simMan.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Simulation manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: simMan.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "sim.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts the simulation manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sym_Man_t * Sym_ManStart( Abc_Ntk_t * pNtk, int fVerbose )
{
    Sym_Man_t * p;
    int i, v; 
    // start the manager
    p = ABC_ALLOC( Sym_Man_t, 1 );
    memset( p, 0, sizeof(Sym_Man_t) );
    p->pNtk = pNtk;
    p->vNodes     = Abc_NtkDfs( pNtk, 0 );
    p->nInputs    = Abc_NtkCiNum(p->pNtk);
    p->nOutputs   = Abc_NtkCoNum(p->pNtk);
    // internal simulation information
    p->nSimWords  = SIM_NUM_WORDS(p->nInputs);
    p->vSim       = Sim_UtilInfoAlloc( Abc_NtkObjNumMax(pNtk), p->nSimWords, 0 );
    // symmetry info for each output
    p->vMatrSymms    = Vec_PtrStart( p->nOutputs );
    p->vMatrNonSymms = Vec_PtrStart( p->nOutputs );
    p->vPairsTotal   = Vec_IntStart( p->nOutputs );
    p->vPairsSym     = Vec_IntStart( p->nOutputs );
    p->vPairsNonSym  = Vec_IntStart( p->nOutputs );
    for ( i = 0; i < p->nOutputs; i++ )
    {
        p->vMatrSymms->pArray[i]    = Extra_BitMatrixStart( p->nInputs );
        p->vMatrNonSymms->pArray[i] = Extra_BitMatrixStart( p->nInputs );
    }
    // temporary patterns
    p->uPatRand = ABC_ALLOC( unsigned, p->nSimWords );
    p->uPatCol  = ABC_ALLOC( unsigned, p->nSimWords );
    p->uPatRow  = ABC_ALLOC( unsigned, p->nSimWords );
    p->vVarsU   = Vec_IntStart( 100 );
    p->vVarsV   = Vec_IntStart( 100 );
    // compute supports
    p->vSuppFun  = Sim_ComputeFunSupp( pNtk, fVerbose );
    p->vSupports = Vec_VecStart( p->nOutputs );
    for ( i = 0; i < p->nOutputs; i++ )
        for ( v = 0; v < p->nInputs; v++ )
            if ( Sim_SuppFunHasVar( p->vSuppFun, i, v ) )
                Vec_VecPushInt( p->vSupports, i, v );
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops the simulation manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sym_ManStop( Sym_Man_t * p )
{
    int i;
    Sym_ManPrintStats( p );
    if ( p->vSuppFun )     Sim_UtilInfoFree( p->vSuppFun );   
    if ( p->vSim )         Sim_UtilInfoFree( p->vSim );   
    if ( p->vNodes )       Vec_PtrFree( p->vNodes );
    if ( p->vSupports )    Vec_VecFree( p->vSupports );
    for ( i = 0; i < p->nOutputs; i++ )
    {
        Extra_BitMatrixStop( (Extra_BitMat_t *)p->vMatrSymms->pArray[i] );
        Extra_BitMatrixStop( (Extra_BitMat_t *)p->vMatrNonSymms->pArray[i] );
    }
    Vec_IntFree( p->vVarsU );
    Vec_IntFree( p->vVarsV );
    Vec_PtrFree( p->vMatrSymms );
    Vec_PtrFree( p->vMatrNonSymms );
    Vec_IntFree( p->vPairsTotal );
    Vec_IntFree( p->vPairsSym );
    Vec_IntFree( p->vPairsNonSym );
    ABC_FREE( p->uPatRand );
    ABC_FREE( p->uPatCol );
    ABC_FREE( p->uPatRow );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Prints the manager statisticis.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sym_ManPrintStats( Sym_Man_t * p )
{
//    printf( "Inputs = %5d. Outputs = %5d. Sim words = %5d.\n", 
//        Abc_NtkCiNum(p->pNtk), Abc_NtkCoNum(p->pNtk), p->nSimWords );
    printf( "Total symm         = %8d.\n", p->nPairsSymm );
    printf( "Structural symm    = %8d.\n", p->nPairsSymmStr );
    printf( "Total non-sym      = %8d.\n", p->nPairsNonSymm );
    printf( "Total var pairs    = %8d.\n", p->nPairsTotal );
    printf( "Sat runs SAT       = %8d.\n", p->nSatRunsSat );
    printf( "Sat runs UNSAT     = %8d.\n", p->nSatRunsUnsat );
    ABC_PRT( "Structural  ", p->timeStruct );
    ABC_PRT( "Simulation  ", p->timeSim );
    ABC_PRT( "Matrix      ", p->timeMatr );
    ABC_PRT( "Counting    ", p->timeCount );
    ABC_PRT( "Fraiging    ", p->timeFraig );
    ABC_PRT( "SAT         ", p->timeSat );
    ABC_PRT( "TOTAL       ", p->timeTotal );
}


/**Function*************************************************************

  Synopsis    [Starts the simulation manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sim_Man_t * Sim_ManStart( Abc_Ntk_t * pNtk, int fLightweight )
{
    Sim_Man_t * p;
    // start the manager
    p = ABC_ALLOC( Sim_Man_t, 1 );
    memset( p, 0, sizeof(Sim_Man_t) );
    p->pNtk = pNtk;
    p->nInputs    = Abc_NtkCiNum(p->pNtk);
    p->nOutputs   = Abc_NtkCoNum(p->pNtk);
    // internal simulation information
    p->nSimBits   = 2048;
    p->nSimWords  = SIM_NUM_WORDS(p->nSimBits);
    p->vSim0      = Sim_UtilInfoAlloc( Abc_NtkObjNumMax(pNtk), p->nSimWords, 0 );
    p->fLightweight = fLightweight;
    if (!p->fLightweight) {
        p->vSim1      = Sim_UtilInfoAlloc( Abc_NtkObjNumMax(pNtk), p->nSimWords, 0 );
        // support information
        p->nSuppBits  = Abc_NtkCiNum(pNtk);
        p->nSuppWords = SIM_NUM_WORDS(p->nSuppBits);
        p->vSuppStr   = Sim_ComputeStrSupp( pNtk );
        p->vSuppFun   = Sim_UtilInfoAlloc( Abc_NtkCoNum(p->pNtk),  p->nSuppWords, 1 );
        // other data
        p->pMmPat     = Extra_MmFixedStart( sizeof(Sim_Pat_t) + p->nSuppWords * sizeof(unsigned) ); 
        p->vFifo      = Vec_PtrAlloc( 100 );
        p->vDiffs     = Vec_IntAlloc( 100 );
        // allocate support targets (array of unresolved outputs for each input)
        p->vSuppTargs = Vec_VecStart( p->nInputs );
    }
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops the simulation manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_ManStop( Sim_Man_t * p )
{
    Sim_ManPrintStats( p );
    if ( p->vSim0 )        Sim_UtilInfoFree( p->vSim0 );       
    if ( p->vSim1 )        Sim_UtilInfoFree( p->vSim1 );       
    if ( p->vSuppStr )     Sim_UtilInfoFree( p->vSuppStr );    
//    if ( p->vSuppFun )     Sim_UtilInfoFree( p->vSuppFun );    
    if ( p->vSuppTargs )   Vec_VecFree( p->vSuppTargs );
    if ( p->pMmPat )       Extra_MmFixedStop( p->pMmPat );
    if ( p->vFifo )        Vec_PtrFree( p->vFifo );
    if ( p->vDiffs )       Vec_IntFree( p->vDiffs );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Prints the manager statisticis.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_ManPrintStats( Sim_Man_t * p )
{
//    printf( "Inputs = %5d. Outputs = %5d. Sim words = %5d.\n", 
//        Abc_NtkCiNum(p->pNtk), Abc_NtkCoNum(p->pNtk), p->nSimWords );
    printf( "Total func supps   = %8d.\n", Sim_UtilCountSuppSizes(p, 0) );
    printf( "Total struct supps = %8d.\n", Sim_UtilCountSuppSizes(p, 1) );
    printf( "Sat runs SAT       = %8d.\n", p->nSatRunsSat );
    printf( "Sat runs UNSAT     = %8d.\n", p->nSatRunsUnsat );
    ABC_PRT( "Simulation  ", p->timeSim );
    ABC_PRT( "Traversal   ", p->timeTrav );
    ABC_PRT( "Fraiging    ", p->timeFraig );
    ABC_PRT( "SAT         ", p->timeSat );
    ABC_PRT( "TOTAL       ", p->timeTotal );
}



/**Function*************************************************************

  Synopsis    [Returns one simulation pattern.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sim_Pat_t * Sim_ManPatAlloc( Sim_Man_t * p )
{
    Sim_Pat_t * pPat;
    pPat = (Sim_Pat_t *)Extra_MmFixedEntryFetch( p->pMmPat );
    pPat->Output = -1;
    pPat->pData  = (unsigned *)((char *)pPat + sizeof(Sim_Pat_t));
    memset( pPat->pData, 0, p->nSuppWords * sizeof(unsigned) );
    return pPat;
}

/**Function*************************************************************

  Synopsis    [Returns one simulation pattern.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_ManPatFree( Sim_Man_t * p, Sim_Pat_t * pPat )
{
    Extra_MmFixedEntryRecycle( p->pMmPat, (char *)pPat );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

