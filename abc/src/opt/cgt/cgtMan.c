/**CFile****************************************************************

  FileName    [cgtMan.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Clock gating package.]

  Synopsis    [Manipulation of clock gating manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cgtMan.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cgtInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cgt_Man_t * Cgt_ManCreate( Aig_Man_t * pAig, Aig_Man_t * pCare, Cgt_Par_t * pPars )
{
    Cgt_Man_t * p;
    // prepare the sequential AIG
    assert( Saig_ManRegNum(pAig) > 0 );
    Aig_ManFanoutStart( pAig );
    Aig_ManSetCioIds( pAig );
    // create interpolation manager
    p = ABC_ALLOC( Cgt_Man_t, 1 ); 
    memset( p, 0, sizeof(Cgt_Man_t) );
    p->pPars      = pPars;
    p->pAig       = pAig;
    p->vGatesAll  = Vec_VecStart( Saig_ManRegNum(pAig) );
    p->vFanout    = Vec_PtrAlloc( 1000 );
    p->vVisited   = Vec_PtrAlloc( 1000 );
    p->nPattWords = 16;
    if ( pCare == NULL )
        return p;
    // check out the constraints
    if ( Aig_ManCiNum(pCare) != Aig_ManCiNum(pAig) )
    {
        printf( "The PI count of care (%d) and AIG (%d) differ. Careset is not used.\n", 
            Aig_ManCiNum(pCare), Aig_ManCiNum(pAig) );
        return p;
    }
    p->pCare = pCare;
    p->vSuppsInv = (Vec_Vec_t *)Aig_ManSupportsInverse( p->pCare );
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cgt_ManClean( Cgt_Man_t * p )
{
    if ( p->pPart )
    {
        Aig_ManStop( p->pPart );
        p->pPart = NULL;
    }
    if ( p->pCnf )
    {
        Cnf_DataFree( p->pCnf );
        p->pCnf = NULL;
    }
    if ( p->pSat )
    {
        sat_solver_delete( p->pSat );
        p->pSat = NULL;
    }
    if ( p->vPatts )
    {
        Vec_PtrFree( p->vPatts );
        p->vPatts = NULL;
    }
}


/**Function*************************************************************

  Synopsis    [Prints stats of the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cgt_ManPrintStats( Cgt_Man_t * p )
{
    printf( "Params: LevMax = %d. CandMax = %d. OdcMax = %d. ConfMax = %d. VarMin = %d. FlopMin = %d.\n", 
        p->pPars->nLevelMax, p->pPars->nCandMax, p->pPars->nOdcMax, 
        p->pPars->nConfMax, p->pPars->nVarsMin, p->pPars->nFlopsMin );
    printf( "SAT   : Calls = %d. Unsat = %d. Sat = %d. Fails = %d.  Recycles = %d.  ", 
        p->nCalls, p->nCallsUnsat, p->nCallsSat, p->nCallsUndec, p->nRecycles );
    ABC_PRT( "Time", p->timeTotal );
/*
    p->timeOther = p->timeTotal-p->timeAig-p->timePrepare-p->timeSat-p->timeDecision;
    ABC_PRTP( "AIG        ", p->timeAig,       p->timeTotal );
    ABC_PRTP( "Prepare    ", p->timePrepare,   p->timeTotal );
    ABC_PRTP( "SAT solving", p->timeSat,       p->timeTotal );
    ABC_PRTP( "  unsat    ", p->timeSatUnsat,  p->timeTotal );
    ABC_PRTP( "  sat      ", p->timeSatSat,    p->timeTotal );
    ABC_PRTP( "  undecided", p->timeSatUndec,  p->timeTotal );
    ABC_PRTP( "Other      ", p->timeOther,     p->timeTotal );
    ABC_PRTP( "TOTAL      ", p->timeTotal,     p->timeTotal );
*/
}

/**Function*************************************************************

  Synopsis    [Frees the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cgt_ManStop( Cgt_Man_t * p )
{
    if ( p->pPars->fVerbose )
        Cgt_ManPrintStats( p );
    if ( p->pFrame )
        Aig_ManStop( p->pFrame );
    Cgt_ManClean( p );
    Vec_PtrFree( p->vFanout );
    Vec_PtrFree( p->vVisited );
    if ( p->vGates )
        Vec_PtrFree( p->vGates );
    if ( p->vGatesAll )
        Vec_VecFree( p->vGatesAll );
    if ( p->vSuppsInv )
        Vec_VecFree( p->vSuppsInv );
    ABC_FREE( p );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

