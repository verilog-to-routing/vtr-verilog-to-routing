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

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Cnf_Man_t * s_pManCnf = NULL;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Converts AIG into the SAT solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cnf_Dat_t * Cnf_Derive( Aig_Man_t * pAig, int nOutputs )
{
    Cnf_Man_t * p;
    Cnf_Dat_t * pCnf;
    Vec_Ptr_t * vMapped;
    Aig_MmFixed_t * pMemCuts;
    int clk;
    // allocate the CNF manager
    if ( s_pManCnf == NULL )
        s_pManCnf = Cnf_ManStart();
    // connect the managers
    p = s_pManCnf;
    p->pManAig = pAig;

    // generate cuts for all nodes, assign cost, and find best cuts
clk = clock();
    pMemCuts = Dar_ManComputeCuts( pAig, 10 );
p->timeCuts = clock() - clk;

    // find the mapping
clk = clock();
    Cnf_DeriveMapping( p );
p->timeMap = clock() - clk;
//    Aig_ManScanMapping( p, 1 );

    // convert it into CNF
clk = clock();
    Cnf_ManTransferCuts( p );
    vMapped = Cnf_ManScanMapping( p, 1, 1 );
    pCnf = Cnf_ManWriteCnf( p, vMapped, nOutputs );
    Vec_PtrFree( vMapped );
    Aig_MmFixedStop( pMemCuts, 0 );
p->timeSave = clock() - clk;

   // reset reference counters
    Aig_ManResetRefs( pAig );
//PRT( "Cuts   ", p->timeCuts );
//PRT( "Map    ", p->timeMap  );
//PRT( "Saving ", p->timeSave );
    return pCnf;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cnf_Man_t * Cnf_ManRead()
{
    return s_pManCnf;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cnf_ClearMemory()
{
    if ( s_pManCnf == NULL )
        return;
    Cnf_ManStop( s_pManCnf );
    s_pManCnf = NULL;
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
clk = clock();
        Cnf_ManScanMapping( p, 0 );
        Cnf_ManMapForCnf( p );
PRT( "iter ", clock() - clk );
    }
*/
    // write the file
    vMapped = Aig_ManScanMapping( p, 1 );
    Vec_PtrFree( vMapped );

clk = clock();
    Cnf_ManTransferCuts( p );

    Cnf_ManPostprocess( p );
    Cnf_ManScanMapping( p, 0 );
/*
    Cnf_ManPostprocess( p );
    Cnf_ManScanMapping( p, 0 );
    Cnf_ManPostprocess( p );
    Cnf_ManScanMapping( p, 0 );
*/
PRT( "Ext ", clock() - clk );

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


