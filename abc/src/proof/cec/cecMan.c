/**CFile****************************************************************

  FileName    [cecMan.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Combinational equivalence checking.]

  Synopsis    [Manager procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cecMan.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cecInt.h"

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
Cec_ManSat_t * Cec_ManSatCreate( Gia_Man_t * pAig, Cec_ParSat_t * pPars )
{
    Cec_ManSat_t * p;
    // create interpolation manager
    p = ABC_ALLOC( Cec_ManSat_t, 1 );
    memset( p, 0, sizeof(Cec_ManSat_t) );
    p->pPars        = pPars;
    p->pAig         = pAig;
    // SAT solving
    p->nSatVars     = 1;
    p->pSatVars     = ABC_CALLOC( int, Gia_ManObjNum(pAig) );
    p->vUsedNodes   = Vec_PtrAlloc( 1000 );
    p->vFanins      = Vec_PtrAlloc( 100 );
    p->vCex         = Vec_IntAlloc( 100 );
    p->vVisits      = Vec_IntAlloc( 100 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Prints statistics of the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_ManSatPrintStats( Cec_ManSat_t * p )
{
    Abc_Print( 1, "CO = %8d  ", Gia_ManCoNum(p->pAig) );
    Abc_Print( 1, "AND = %8d  ", Gia_ManAndNum(p->pAig) );
    Abc_Print( 1, "Conf = %5d  ", p->pPars->nBTLimit );
    Abc_Print( 1, "MinVar = %5d  ", p->pPars->nSatVarMax );
    Abc_Print( 1, "MinCalls = %5d\n", p->pPars->nCallsRecycle );
    Abc_Print( 1, "Unsat calls %6d  (%6.2f %%)   Ave conf = %8.1f   ", 
        p->nSatUnsat, p->nSatTotal? 100.0*p->nSatUnsat/p->nSatTotal : 0.0, p->nSatUnsat? 1.0*p->nConfUnsat/p->nSatUnsat :0.0 );
    Abc_PrintTimeP( 1, "Time", p->timeSatUnsat, p->timeTotal );
    Abc_Print( 1, "Sat   calls %6d  (%6.2f %%)   Ave conf = %8.1f   ", 
        p->nSatSat,   p->nSatTotal? 100.0*p->nSatSat/p->nSatTotal : 0.0,   p->nSatSat? 1.0*p->nConfSat/p->nSatSat : 0.0 );
    Abc_PrintTimeP( 1, "Time", p->timeSatSat,   p->timeTotal );
    Abc_Print( 1, "Undef calls %6d  (%6.2f %%)   Ave conf = %8.1f   ", 
        p->nSatUndec, p->nSatTotal? 100.0*p->nSatUndec/p->nSatTotal : 0.0, p->nSatUndec? 1.0*p->nConfUndec/p->nSatUndec : 0.0 );
    Abc_PrintTimeP( 1, "Time", p->timeSatUndec, p->timeTotal );
    Abc_PrintTime( 1, "Total time", p->timeTotal );
}

/**Function*************************************************************

  Synopsis    [Frees the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_ManSatStop( Cec_ManSat_t * p )
{
    if ( p->pSat )
        sat_solver_delete( p->pSat );
    Vec_IntFree( p->vCex );
    Vec_IntFree( p->vVisits );
    Vec_PtrFree( p->vUsedNodes );
    Vec_PtrFree( p->vFanins );
    ABC_FREE( p->pSatVars );
    ABC_FREE( p );
}



/**Function*************************************************************

  Synopsis    [Creates AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cec_ManPat_t * Cec_ManPatStart()  
{ 
    Cec_ManPat_t * p;
    p = ABC_CALLOC( Cec_ManPat_t, 1 );
    p->vStorage  = Vec_StrAlloc( 1<<20 );
    p->vPattern1 = Vec_IntAlloc( 1000 );
    p->vPattern2 = Vec_IntAlloc( 1000 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_ManPatPrintStats( Cec_ManPat_t * p )  
{ 
    Abc_Print( 1, "Latest: P = %8d.  L = %10d.  Lm = %10d. Ave = %6.1f. MEM =%6.2f MB\n", 
        p->nPats, p->nPatLits, p->nPatLitsMin, 1.0 * p->nPatLitsMin/p->nPats, 
        1.0*(Vec_StrSize(p->vStorage)-p->iStart)/(1<<20) );
    Abc_Print( 1, "Total:  P = %8d.  L = %10d.  Lm = %10d. Ave = %6.1f. MEM =%6.2f MB\n", 
        p->nPatsAll, p->nPatLitsAll, p->nPatLitsMinAll, 1.0 * p->nPatLitsMinAll/p->nPatsAll, 
        1.0*Vec_StrSize(p->vStorage)/(1<<20) );
    Abc_PrintTimeP( 1, "Finding  ", p->timeFind,   p->timeTotal );
    Abc_PrintTimeP( 1, "Shrinking", p->timeShrink, p->timeTotal );
    Abc_PrintTimeP( 1, "Verifying", p->timeVerify, p->timeTotal );
    Abc_PrintTimeP( 1, "Sorting  ", p->timeSort,   p->timeTotal );
    Abc_PrintTimeP( 1, "Packing  ", p->timePack,   p->timeTotal );
    Abc_PrintTime( 1, "TOTAL    ",  p->timeTotal );
}

/**Function*************************************************************

  Synopsis    [Deletes AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_ManPatStop( Cec_ManPat_t * p )  
{
    Vec_StrFree( p->vStorage );
    Vec_IntFree( p->vPattern1 );
    Vec_IntFree( p->vPattern2 );
    ABC_FREE( p );
}



/**Function*************************************************************

  Synopsis    [Creates AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cec_ManSim_t * Cec_ManSimStart( Gia_Man_t * pAig, Cec_ParSim_t *  pPars )  
{ 
    Cec_ManSim_t * p;
    p = ABC_ALLOC( Cec_ManSim_t, 1 );
    memset( p, 0, sizeof(Cec_ManSim_t) );
    p->pAig  = pAig;
    p->pPars = pPars;
    p->nWords = pPars->nWords;
    p->pSimInfo = ABC_CALLOC( int, Gia_ManObjNum(pAig) );
    p->vClassOld  = Vec_IntAlloc( 1000 );
    p->vClassNew  = Vec_IntAlloc( 1000 );
    p->vClassTemp = Vec_IntAlloc( 1000 );
    p->vRefinedC  = Vec_IntAlloc( 10000 );
    p->vCiSimInfo = Vec_PtrAllocSimInfo( Gia_ManCiNum(p->pAig), pPars->nWords );
    if ( pPars->fCheckMiter || Gia_ManRegNum(p->pAig) )
    {
        p->vCoSimInfo = Vec_PtrAllocSimInfo( Gia_ManCoNum(p->pAig), pPars->nWords );
        Vec_PtrCleanSimInfo( p->vCoSimInfo, 0, pPars->nWords );
    }
    p->iOut = -1;
    return p;
}

/**Function*************************************************************

  Synopsis    [Deletes AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_ManSimStop( Cec_ManSim_t * p )  
{
    Vec_IntFree( p->vClassOld );
    Vec_IntFree( p->vClassNew );
    Vec_IntFree( p->vClassTemp );
    Vec_IntFree( p->vRefinedC );
    if ( p->vCiSimInfo ) 
        Vec_PtrFree( p->vCiSimInfo );
    if ( p->vCoSimInfo ) 
        Vec_PtrFree( p->vCoSimInfo );
    ABC_FREE( p->pScores );
    ABC_FREE( p->pCexComb );
    ABC_FREE( p->pCexes );
    ABC_FREE( p->pMems );
    ABC_FREE( p->pSimInfo );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    [Creates AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cec_ManFra_t * Cec_ManFraStart( Gia_Man_t * pAig, Cec_ParFra_t *  pPars )  
{ 
    Cec_ManFra_t * p;
    p = ABC_ALLOC( Cec_ManFra_t, 1 );
    memset( p, 0, sizeof(Cec_ManFra_t) );
    p->pAig  = pAig;
    p->pPars = pPars;
    p->vXorNodes  = Vec_IntAlloc( 1000 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Deletes AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_ManFraStop( Cec_ManFra_t * p )  
{
    Vec_IntFree( p->vXorNodes );
    ABC_FREE( p );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

