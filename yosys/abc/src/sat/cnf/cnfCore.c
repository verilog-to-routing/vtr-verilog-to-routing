/**CFile****************************************************************

  FileName    [cnfCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG-to-CNF conversion.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: cnfCore.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cnf.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Cnf_Man_t * s_pManCnf = NULL;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cnf_ManPrepare()
{
    if ( s_pManCnf == NULL )
    {
//        printf( "\n\nCreating CNF manager!!!!!\n\n" );
        s_pManCnf = Cnf_ManStart();
    }
}
Cnf_Man_t * Cnf_ManRead()
{
    return s_pManCnf;
}
void Cnf_ManFree()
{
    if ( s_pManCnf == NULL )
        return;
    Cnf_ManStop( s_pManCnf );
    s_pManCnf = NULL;
}


/**Function*************************************************************

  Synopsis    [Converts AIG into the SAT solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Cnf_DeriveMappingArray( Aig_Man_t * pAig )
{
    Vec_Int_t * vResult;
    Cnf_Man_t * p;
    Vec_Ptr_t * vMapped;
    Aig_MmFixed_t * pMemCuts;
    abctime clk;
    // allocate the CNF manager
    p = Cnf_ManStart();
    p->pManAig = pAig;

    // generate cuts for all nodes, assign cost, and find best cuts
clk = Abc_Clock();
    pMemCuts = Dar_ManComputeCuts( pAig, 10, 0, 0 );
p->timeCuts = Abc_Clock() - clk;

    // find the mapping
clk = Abc_Clock();
    Cnf_DeriveMapping( p );
p->timeMap = Abc_Clock() - clk;
//    Aig_ManScanMapping( p, 1 );

    // convert it into CNF
clk = Abc_Clock();
    Cnf_ManTransferCuts( p );
    vMapped = Cnf_ManScanMapping( p, 1, 0 );
    vResult = Cnf_ManWriteCnfMapping( p, vMapped );
    Vec_PtrFree( vMapped );
    Aig_MmFixedStop( pMemCuts, 0 );
p->timeSave = Abc_Clock() - clk;

   // reset reference counters
    Aig_ManResetRefs( pAig );
//ABC_PRT( "Cuts   ", p->timeCuts );
//ABC_PRT( "Map    ", p->timeMap  );
//ABC_PRT( "Saving ", p->timeSave );
    Cnf_ManStop( p );
    return vResult;
}
 
/**Function*************************************************************

  Synopsis    [Converts AIG into the SAT solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cnf_Dat_t * Cnf_DeriveWithMan( Cnf_Man_t * p, Aig_Man_t * pAig, int nOutputs )
{
    Cnf_Dat_t * pCnf;
    Vec_Ptr_t * vMapped;
    Aig_MmFixed_t * pMemCuts;
    abctime clk;
    // connect the managers
    p->pManAig = pAig;

    // generate cuts for all nodes, assign cost, and find best cuts
clk = Abc_Clock();
    pMemCuts = Dar_ManComputeCuts( pAig, 10, 0, 0 );
p->timeCuts = Abc_Clock() - clk;

    // find the mapping
clk = Abc_Clock();
    Cnf_DeriveMapping( p );
p->timeMap = Abc_Clock() - clk;
//    Aig_ManScanMapping( p, 1 );

    // convert it into CNF
clk = Abc_Clock();
    Cnf_ManTransferCuts( p );
    vMapped = Cnf_ManScanMapping( p, 1, 1 );
    pCnf = Cnf_ManWriteCnf( p, vMapped, nOutputs );
    Vec_PtrFree( vMapped );
    Aig_MmFixedStop( pMemCuts, 0 );
p->timeSave = Abc_Clock() - clk;

   // reset reference counters
    Aig_ManResetRefs( pAig );
//ABC_PRT( "Cuts   ", p->timeCuts );
//ABC_PRT( "Map    ", p->timeMap  );
//ABC_PRT( "Saving ", p->timeSave );
    return pCnf;
}
Cnf_Dat_t * Cnf_Derive( Aig_Man_t * pAig, int nOutputs )
{
    Cnf_ManPrepare();
    return Cnf_DeriveWithMan( s_pManCnf, pAig, nOutputs );
}
 
/**Function*************************************************************

  Synopsis    [Converts AIG into the SAT solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cnf_Dat_t * Cnf_DeriveOtherWithMan( Cnf_Man_t * p, Aig_Man_t * pAig, int fSkipTtMin )
{
    Cnf_Dat_t * pCnf;
    Vec_Ptr_t * vMapped;
    Aig_MmFixed_t * pMemCuts;
    abctime clk;
    // connect the managers
    p->pManAig = pAig;

    // generate cuts for all nodes, assign cost, and find best cuts
clk = Abc_Clock();
    pMemCuts = Dar_ManComputeCuts( pAig, 10, fSkipTtMin, 0 );
p->timeCuts = Abc_Clock() - clk;

    // find the mapping
clk = Abc_Clock();
    Cnf_DeriveMapping( p );
p->timeMap = Abc_Clock() - clk;
//    Aig_ManScanMapping( p, 1 );

    // convert it into CNF
clk = Abc_Clock();
    Cnf_ManTransferCuts( p );
    vMapped = Cnf_ManScanMapping( p, 1, 1 );
    pCnf = Cnf_ManWriteCnfOther( p, vMapped );
    pCnf->vMapping = Cnf_ManWriteCnfMapping( p, vMapped );
    Vec_PtrFree( vMapped );
    Aig_MmFixedStop( pMemCuts, 0 );
p->timeSave = Abc_Clock() - clk;

   // reset reference counters
    Aig_ManResetRefs( pAig );
//ABC_PRT( "Cuts   ", p->timeCuts );
//ABC_PRT( "Map    ", p->timeMap  );
//ABC_PRT( "Saving ", p->timeSave );
    return pCnf;
}
Cnf_Dat_t * Cnf_DeriveOther( Aig_Man_t * pAig, int fSkipTtMin )
{
    Cnf_ManPrepare();
    return Cnf_DeriveOtherWithMan( s_pManCnf, pAig, fSkipTtMin );
}

#if 0

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cnf_Dat_t * Cnf_Derive_old( Aig_Man_t * pAig )
{
/*
    // iteratively improve area flow
    for ( i = 0; i < nIters; i++ )
    {
clk = Abc_Clock();
        Cnf_ManScanMapping( p, 0 );
        Cnf_ManMapForCnf( p );
ABC_PRT( "iter ", Abc_Clock() - clk );
    }
*/
    // write the file
    vMapped = Aig_ManScanMapping( p, 1 );
    Vec_PtrFree( vMapped );

clk = Abc_Clock();
    Cnf_ManTransferCuts( p );

    Cnf_ManPostprocess( p );
    Cnf_ManScanMapping( p, 0 );
/*
    Cnf_ManPostprocess( p );
    Cnf_ManScanMapping( p, 0 );
    Cnf_ManPostprocess( p );
    Cnf_ManScanMapping( p, 0 );
*/
ABC_PRT( "Ext ", Abc_Clock() - clk );

/*
    vMapped = Cnf_ManScanMapping( p, 1 );
    pCnf = Cnf_ManWriteCnf( p, vMapped );
    Vec_PtrFree( vMapped );

    // clean up
    Cnf_ManFreeCuts( p );
    Dar_ManCutsFree( pAig );
    return pCnf;
*/
    Aig_MmFixedStop( pMemCuts, 0 );
    return NULL;
}

#endif


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

