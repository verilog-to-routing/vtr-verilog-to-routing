/**CFile****************************************************************

  FileName    [mpm.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Configurable technology mapper.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 1, 2013.]

  Revision    [$Id: mpm.c,v 1.00 2013/06/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "mpmInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mpm_Man_t * Mpm_ManStart( Mig_Man_t * pMig, Mpm_Par_t * pPars )
{
    Mpm_Man_t * p;
    int i;
    assert( sizeof(Mpm_Uni_t) % sizeof(word) == 0 );      // aligned info to word boundary
    assert( pPars->nNumCuts <= MPM_CUT_MAX );
    assert( !pPars->fUseTruth || pPars->pLib->LutMax <= 16 );
    assert( !pPars->fUseDsd || pPars->pLib->LutMax <= 6 );
    Mig_ManSetRefs( pMig );
    // alloc
    p = ABC_CALLOC( Mpm_Man_t, 1 );
    p->pMig      = pMig;
    p->pPars     = pPars;
    p->pLibLut   = pPars->pLib;
    p->nLutSize  = pPars->pLib->LutMax;
    p->nTruWords = pPars->fUseTruth ? Abc_Truth6WordNum(p->nLutSize) : 0;
    p->nNumCuts  = pPars->nNumCuts;
    // cuts
    assert( Mpm_CutWordNum(32) < 32 ); // using 5 bits for word count
    p->pManCuts  = Mmr_StepStart( 13, Abc_Base2Log(Mpm_CutWordNum(p->nLutSize) + 1) );
    Vec_PtrGrow( &p->vFreeUnits, p->nNumCuts + 1 );
    for ( i = p->nNumCuts; i >= 0; i-- )
        Vec_PtrPush( &p->vFreeUnits, p->pCutUnits + i );
    p->vTemp     = Vec_PtrAlloc( 1000 );
    // mapping attributes
    Vec_IntFill( &p->vCutBests, Mig_ManObjNum(pMig), 0 );
    Vec_IntFill( &p->vCutLists, Mig_ManObjNum(pMig), 0 );
    Vec_IntFill( &p->vMigRefs, Mig_ManObjNum(pMig), 0 );
    Vec_IntFill( &p->vMapRefs, Mig_ManObjNum(pMig), 0 );
    Vec_IntFill( &p->vEstRefs, Mig_ManObjNum(pMig), 0 );
    Vec_IntFill( &p->vRequireds, Mig_ManObjNum(pMig), ABC_INFINITY );
    Vec_IntFill( &p->vTimes, Mig_ManObjNum(pMig), 0 );
    Vec_IntFill( &p->vAreas, Mig_ManObjNum(pMig), 0 );
    Vec_IntFill( &p->vEdges, Mig_ManObjNum(pMig), 0 );
    // start DSD manager
    assert( !p->pPars->fUseTruth || !p->pPars->fUseDsd );
    if ( p->pPars->fUseTruth )
    { 
        p->vTtMem = Vec_MemAlloc( p->nTruWords, 12 ); // 32 KB/page for 6-var functions
        Vec_MemHashAlloc( p->vTtMem, 10000 );
        p->funcCst0 = Vec_MemHashInsert( p->vTtMem, p->Truth );
        Abc_TtUnit( p->Truth, p->nTruWords, 0 );
        p->funcVar0 = Vec_MemHashInsert( p->vTtMem, p->Truth );
    }
    else if ( p->pPars->fUseDsd )
    {
        Mpm_ManPrecomputePerms( p );
        p->funcVar0 = 1;
    }
    // finish
    p->timeTotal = Abc_Clock();
    pMig->pMan = p;
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mpm_ManStop( Mpm_Man_t * p )
{
    if ( p->pPars->fUseTruth && p->pPars->fVeryVerbose )
    {
        char * pFileName = "truths.txt";
        FILE * pFile = fopen( pFileName, "wb" );
        Vec_MemDump( pFile, p->vTtMem );
        fclose( pFile );
        printf( "Dumped %d %d-var truth tables into file \"%s\" (%.2f MB).\n", 
            Vec_MemEntryNum(p->vTtMem), p->nLutSize, pFileName,
            (16.0 * p->nTruWords + 1.0) * Vec_MemEntryNum(p->vTtMem) / (1 << 20) );
    }
    if ( p->pPars->fUseDsd && p->pPars->fVerbose )
        Mpm_ManPrintDsdStats( p );
    if ( p->vTtMem ) 
    {
        Vec_MemHashFree( p->vTtMem );
        Vec_MemFree( p->vTtMem );
    }
    if ( p->pHash )
    {
        Vec_WrdFree( p->vPerm6 );
        Vec_IntFree( p->vMap2Perm );
        Vec_IntFree( p->vConfgRes );
        Vec_IntFree( p->pHash->vData );
        Hsh_IntManStop( p->pHash );
    }
    Vec_WecFreeP( &p->vNpnConfigs );
    Vec_PtrFree( p->vTemp );
    Mmr_StepStop( p->pManCuts );
    ABC_FREE( p->vFreeUnits.pArray );
    // mapping attributes
    ABC_FREE( p->vCutBests.pArray );
    ABC_FREE( p->vCutLists.pArray );
    ABC_FREE( p->vMigRefs.pArray );
    ABC_FREE( p->vMapRefs.pArray );
    ABC_FREE( p->vEstRefs.pArray );
    ABC_FREE( p->vRequireds.pArray );
    ABC_FREE( p->vTimes.pArray );
    ABC_FREE( p->vAreas.pArray );
    ABC_FREE( p->vEdges.pArray );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mpm_ManPrintStatsInit( Mpm_Man_t * p )
{
    printf( "K = %d.  C = %d.  Cand = %d. XOR = %d. MUX = %d. Choice = %d.  CutMin = %d. Truth = %d. DSD = %d.\n", 
        p->nLutSize, p->nNumCuts, Mig_ManCandNum(p->pMig), 
        Mig_ManXorNum(p->pMig), Mig_ManMuxNum(p->pMig), p->pMig->nChoices, 
        p->pPars->fCutMin, p->pPars->fUseTruth, p->pPars->fUseDsd );
}
void Mpm_ManPrintStats( Mpm_Man_t * p )
{
    printf( "Memory usage:  Mig = %.2f MB  Map = %.2f MB  Cut = %.2f MB    Total = %.2f MB.  ", 
        1.0 * Mig_ManObjNum(p->pMig) * sizeof(Mig_Obj_t) / (1 << 20), 
        1.0 * Mig_ManObjNum(p->pMig) * 48 / (1 << 20), 
        1.0 * Mmr_StepMemory(p->pManCuts) / (1 << 17), 
        1.0 * Mig_ManObjNum(p->pMig) * sizeof(Mig_Obj_t) / (1 << 20) + 
        1.0 * Mig_ManObjNum(p->pMig) * 48 / (1 << 20) +
        1.0 * Mmr_StepMemory(p->pManCuts) / (1 << 17) );
    if ( p->timeDerive )
    {
        printf( "\n" );
        p->timeTotal = Abc_Clock() - p->timeTotal;
        p->timeOther = p->timeTotal - p->timeDerive;

        Abc_Print( 1, "Runtime breakdown:\n" );
        ABC_PRTP( "Complete cut computation   ", p->timeDerive , p->timeTotal );
        ABC_PRTP( "- Merging cuts             ", p->timeMerge  , p->timeTotal );
        ABC_PRTP( "- Evaluting cut parameters ", p->timeEval   , p->timeTotal );
        ABC_PRTP( "- Checking cut containment ", p->timeCompare, p->timeTotal );
        ABC_PRTP( "- Adding cuts to storage   ", p->timeStore  , p->timeTotal );
        ABC_PRTP( "Other                      ", p->timeOther  , p->timeTotal );
        ABC_PRTP( "TOTAL                      ", p->timeTotal  , p->timeTotal );
    }
    else
        Abc_PrintTime( 1, "Time", Abc_Clock() - p->timeTotal );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

