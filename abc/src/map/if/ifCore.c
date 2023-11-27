/**CFile****************************************************************

  FileName    [ifCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [FPGA mapping based on priority cuts.]

  Synopsis    [The central part of the mapper.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 21, 2006.]

  Revision    [$Id: ifCore.c,v 1.00 2006/11/21 00:00:00 alanmi Exp $]

***********************************************************************/

#include "if.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern abctime s_MappingTime;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManSetDefaultPars( If_Par_t * pPars )
{
    memset( pPars, 0, sizeof(If_Par_t) );
    pPars->nLutSize    = -1;
    pPars->nCutsMax    =  8;
    pPars->nFlowIters  =  1;
    pPars->nAreaIters  =  2;
    pPars->DelayTarget = -1;
    pPars->Epsilon     =  (float)0.005;
    pPars->fPreprocess =  1;
    pPars->fArea       =  0;
    pPars->fFancy      =  0;
    pPars->fExpRed     =  1;
    pPars->fLatchPaths =  0;
    pPars->fEdge       =  1;
    pPars->fPower      =  0;
    pPars->fCutMin     =  0;
    pPars->fBidec      =  0;
    pPars->fVerbose    =  0;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_ManPerformMapping( If_Man_t * p )
{
    p->pPars->fAreaOnly = p->pPars->fArea; // temporary
    // create the CI cutsets
    If_ManSetupCiCutSets( p );
    // allocate memory for other cutsets
    If_ManSetupSetAll( p, If_ManCrossCut(p) );
    // derive reverse top order
    p->vObjsRev = If_ManReverseOrder( p );
    return If_ManPerformMappingComb( p );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_ManPerformMappingComb( If_Man_t * p )
{
    If_Obj_t * pObj;
    abctime clkTotal = Abc_Clock();
    int i;
    //p->vVisited2 = Vec_IntAlloc( 100 );
    //p->vMarks = Vec_StrStart( If_ManObjNum(p) );

    // set arrival times and fanout estimates
    If_ManForEachCi( p, pObj, i )
    {
        If_ObjSetArrTime( pObj, p->pPars->pTimesArr ? p->pPars->pTimesArr[i] : (float)0.0 );
        pObj->EstRefs = (float)1.0;
    }

    // delay oriented mapping
    if ( p->pPars->fPreprocess && !p->pPars->fArea )
    {
        // map for delay
        If_ManPerformMappingRound( p, p->pPars->nCutsMax, 0, 1, 1, "Delay" );
        // map for delay second option
        p->pPars->fFancy = 1;
        If_ManResetOriginalRefs( p );
        If_ManPerformMappingRound( p, p->pPars->nCutsMax, 0, 1, 0, "Delay-2" );
        p->pPars->fFancy = 0;
        // map for area
        p->pPars->fArea = 1;
        If_ManResetOriginalRefs( p );
        If_ManPerformMappingRound( p, p->pPars->nCutsMax, 0, 1, 0, "Area" );
        p->pPars->fArea = 0;
    }
    else
        If_ManPerformMappingRound( p, p->pPars->nCutsMax, 0, 0, 1, "Delay" );

    // try to improve area by expanding and reducing the cuts
    if ( p->pPars->fExpRed )
        If_ManImproveMapping( p );

    // area flow oriented mapping
    for ( i = 0; i < p->pPars->nFlowIters; i++ )
    {
        If_ManPerformMappingRound( p, p->pPars->nCutsMax, 1, 0, 0, "Flow" );
        if ( p->pPars->fExpRed )
            If_ManImproveMapping( p );
    }

    // area oriented mapping
    for ( i = 0; i < p->pPars->nAreaIters; i++ )
    {
        If_ManPerformMappingRound( p, p->pPars->nCutsMax, 2, 0, 0, "Area" );
        if ( p->pPars->fExpRed )
            If_ManImproveMapping( p );
    }

    if ( p->pPars->fVerbose )
    {
//        Abc_Print( 1, "Total memory = %7.2f MB. Peak cut memory = %7.2f MB.  ", 
//            1.0 * (p->nObjBytes + 2*sizeof(void *)) * If_ManObjNum(p) / (1<<20), 
//            1.0 * p->nSetBytes * Mem_FixedReadMaxEntriesUsed(p->pMemSet) / (1<<20) );
        Abc_PrintTime( 1, "Total time", Abc_Clock() - clkTotal );
    }
//    Abc_Print( 1, "Cross cut memory = %d.\n", Mem_FixedReadMaxEntriesUsed(p->pMemSet) );
    s_MappingTime = Abc_Clock() - clkTotal;
//    Abc_Print( 1, "Special POs = %d.\n", If_ManCountSpecialPos(p) );

/*
    {
        static char * pLastName = NULL;
        FILE * pTable = fopen( "fpga/ucsb/stats.txt", "a+" );
        if ( pLastName == NULL || strcmp(pLastName, p->pName) )
        {
            fprintf( pTable, "\n" );
            fprintf( pTable, "%s ", p->pName );

            fprintf( pTable, "%d ", If_ManCiNum(p) );
            fprintf( pTable, "%d ", If_ManCoNum(p) );
            fprintf( pTable, "%d ", If_ManAndNum(p) );

            ABC_FREE( pLastName );
            pLastName = Abc_UtilStrsav( p->pName );
        }

        fprintf( pTable, "%d ", (int)p->AreaGlo );
        fprintf( pTable, "%d ", (int)p->RequiredGlo );
        fclose( pTable );
    }
*/
    p->pPars->FinalDelay = p->RequiredGlo;
    p->pPars->FinalArea  = p->AreaGlo;
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

