/**CFile****************************************************************

  FileName    [darMan.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware AIG rewriting.]

  Synopsis    [AIG manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: darMan.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "darInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts the rewriting manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dar_Man_t * Dar_ManStart( Aig_Man_t * pAig, Dar_RwrPar_t * pPars )
{
    Dar_Man_t * p;
    // start the manager
    p = ALLOC( Dar_Man_t, 1 );
    memset( p, 0, sizeof(Dar_Man_t) );
    p->pPars = pPars;
    p->pAig  = pAig;
    // prepare the internal memory manager
    p->pMemCuts = Aig_MmFixedStart( p->pPars->nCutsMax * sizeof(Dar_Cut_t), 1024 );
    // other data
    p->vLeavesBest = Vec_PtrAlloc( 4 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops the rewriting manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dar_ManStop( Dar_Man_t * p )
{
    if ( p->pPars->fVerbose )
        Dar_ManPrintStats( p );
    if ( p->pMemCuts )
        Aig_MmFixedStop( p->pMemCuts, 0 );
    if ( p->vLeavesBest ) 
        Vec_PtrFree( p->vLeavesBest );
    free( p );
}

/**Function*************************************************************

  Synopsis    [Stops the AIG manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dar_ManPrintStats( Dar_Man_t * p )
{
    unsigned pCanons[222];
    int Gain, i;
    extern void Kit_DsdPrintFromTruth( unsigned * pTruth, int nVars );

    Gain = p->nNodesInit - Aig_ManNodeNum(p->pAig);
    printf( "Tried = %8d. Beg = %8d. End = %8d. Gain = %6d. (%6.2f %%).  Cut mem = %d Mb\n", 
        p->nNodesTried, p->nNodesInit, Aig_ManNodeNum(p->pAig), Gain, 100.0*Gain/p->nNodesInit, p->nCutMemUsed );
    printf( "Cuts = %8d. Tried = %8d. Used = %8d. Bad = %5d. Skipped = %5d. Ave = %.2f.\n", 
        p->nCutsAll, p->nCutsTried, p->nCutsUsed, p->nCutsBad, p->nCutsSkipped,
        (float)p->nCutsUsed/Aig_ManNodeNum(p->pAig) );

    printf( "Bufs = %5d. BufMax = %5d. BufReplace = %6d. BufFix = %6d.  Levels = %4d.\n", 
        Aig_ManBufNum(p->pAig), p->pAig->nBufMax, p->pAig->nBufReplaces, p->pAig->nBufFixes, Aig_ManLevels(p->pAig) );
    PRT( "Cuts  ", p->timeCuts );
    PRT( "Eval  ", p->timeEval );
    PRT( "Other ", p->timeOther );
    PRT( "TOTAL ", p->timeTotal );

    if ( !p->pPars->fVeryVerbose )
        return;
    Dar_LibReturnCanonicals( pCanons );
    for ( i = 0; i < 222; i++ )
    {
        if ( p->ClassGains[i] == 0 && p->ClassTimes[i] == 0 )
            continue;
        printf( "%3d : ", i );
        printf( "G = %6d (%5.2f %%)  ", p->ClassGains[i], Gain? 100.0*p->ClassGains[i]/Gain : 0.0 );
        printf( "S = %8d (%5.2f %%)  ", p->ClassSubgs[i], p->nTotalSubgs? 100.0*p->ClassSubgs[i]/p->nTotalSubgs : 0.0 );
        printf( "R = %7d   ", p->ClassGains[i]? p->ClassSubgs[i]/p->ClassGains[i] : 9999999 );
        Kit_DsdPrintFromTruth( pCanons + i, 4 );
//        PRTP( "T", p->ClassTimes[i], p->timeEval );
    }
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


