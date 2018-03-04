/**CFile****************************************************************

  FileName    [amapLib.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Technology mapper for standard cells.]

  Synopsis    [Standard-cell library.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: amapLib.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "amapInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocs a library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Amap_Lib_t * Amap_LibAlloc()
{
    Amap_Lib_t * p;
    p = (Amap_Lib_t *)ABC_ALLOC( Amap_Lib_t, 1 );
    memset( p, 0, sizeof(Amap_Lib_t) );
    p->vGates = Vec_PtrAlloc( 100 );
    p->pMemGates = Aig_MmFlexStart();
    p->pMemSet = Aig_MmFlexStart();
    return p;
}

/**Function*************************************************************

  Synopsis    [Deallocs a library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Amap_LibFree( Amap_Lib_t * p )
{
    if ( p == NULL )
        return;
    if ( p->vSelect )
        Vec_PtrFree( p->vSelect );
    if ( p->vSorted )
        Vec_PtrFree( p->vSorted );
    if ( p->vGates )
        Vec_PtrFree( p->vGates );
    if ( p->vRules )
        Vec_VecFree( (Vec_Vec_t *)p->vRules );
    if ( p->vRulesX )
        Vec_VecFree( (Vec_Vec_t *)p->vRulesX );
    if ( p->vRules3 )
        Vec_IntFree( p->vRules3 );
    Aig_MmFlexStop( p->pMemGates, 0 );
    Aig_MmFlexStop( p->pMemSet, 0 );
    ABC_FREE( p->pRules );
    ABC_FREE( p->pRulesX );
    ABC_FREE( p->pNodes );
    ABC_FREE( p->pName );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Returns the largest gate size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Amap_LibNumPinsMax( Amap_Lib_t * p )
{
    Amap_Gat_t * pGate;
    int i, Counter = 0;
    Amap_LibForEachGate( p, pGate, i )
        if ( Counter < (int)pGate->nPins )
            Counter = pGate->nPins;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Writes one pin.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Amap_LibWritePin( FILE * pFile, Amap_Pin_t * pPin )
{
    char * pPhaseNames[10] = { "UNKNOWN", "INV", "NONINV" };
    fprintf( pFile, "    PIN " );
    fprintf( pFile, "%9s ",     pPin->pName );
    fprintf( pFile, "%10s ",    pPhaseNames[pPin->Phase] );
    fprintf( pFile, "%6d ",     (int)pPin->dLoadInput );
    fprintf( pFile, "%6d ",     (int)pPin->dLoadMax );
    fprintf( pFile, "%6.2f ",   pPin->dDelayBlockRise );
    fprintf( pFile, "%6.2f ",   pPin->dDelayFanoutRise );
    fprintf( pFile, "%6.2f ",   pPin->dDelayBlockFall );
    fprintf( pFile, "%6.2f",    pPin->dDelayFanoutFall );
    fprintf( pFile, "\n" );
}

/**Function*************************************************************

  Synopsis    [Writes one gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Amap_LibWriteGate( FILE * pFile, Amap_Gat_t * pGate, int fPrintDsd )
{
    Amap_Pin_t * pPin;
    fprintf( pFile, "GATE " );
    fprintf( pFile, "%12s ",      pGate->pName );
    fprintf( pFile, "%10.2f   ",  pGate->dArea );
    fprintf( pFile, "%s=%s;\n",   pGate->pOutName,    pGate->pForm );
    if ( fPrintDsd )
    {
        if ( pGate->pFunc == NULL )
            printf( "Truth table is not available.\n" );
        else
            Kit_DsdPrintFromTruth( pGate->pFunc, pGate->nPins );
    }
    Amap_GateForEachPin( pGate, pPin )
        Amap_LibWritePin( pFile, pPin );
}

/**Function*************************************************************

  Synopsis    [Writes library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Amap_LibWrite( FILE * pFile, Amap_Lib_t * pLib, int fPrintDsd )
{
    Amap_Gat_t * pGate;
    int i;
    fprintf( pFile, "# The genlib library \"%s\".\n", pLib->pName );
    Amap_LibForEachGate( pLib, pGate, i )
        Amap_LibWriteGate( pFile, pGate, fPrintDsd );
}
        
/**Function*************************************************************

  Synopsis    [Compares two gates by area.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Amap_LibCompareGatesByArea( Amap_Gat_t ** pp1, Amap_Gat_t ** pp2 )
{
    double Diff = (*pp1)->dArea - (*pp2)->dArea;
    if ( Diff < 0.0 )
        return -1;
    if ( Diff > 0.0 ) 
        return 1;
    return 0; 
}
        
/**Function*************************************************************

  Synopsis    [Compares gates by area.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Amap_LibSortGatesByArea( Amap_Lib_t * pLib )
{
    Vec_Ptr_t * vSorted;
    vSorted = Vec_PtrDup( pLib->vGates );
    qsort( (void *)Vec_PtrArray(vSorted), Vec_PtrSize(vSorted), sizeof(void *), 
            (int (*)(const void *, const void *)) Amap_LibCompareGatesByArea );
    return vSorted;
}

/**Function*************************************************************

  Synopsis    [Finds min-area gate with the given function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Amap_Gat_t * Amap_LibFindGate( Amap_Lib_t * p, unsigned uTruth )
{
    Amap_Gat_t * pGate;
    int i;
    Vec_PtrForEachEntry( Amap_Gat_t *, p->vSorted, pGate, i )
        if ( pGate->nPins <= 5 && pGate->pFunc[0] == uTruth )
            return pGate;
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Selects gates useful for area-only mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Amap_LibSelectGates( Amap_Lib_t * p, int fVerbose )
{
    Vec_Ptr_t * vSelect;
    Amap_Gat_t * pGate, * pGate2;
    int i, k;//, clk = Abc_Clock();
    p->pGate0   = Amap_LibFindGate( p, 0 );
    p->pGate1   = Amap_LibFindGate( p, ~0 );
    p->pGateBuf = Amap_LibFindGate( p, 0xAAAAAAAA );
    p->pGateInv = Amap_LibFindGate( p, ~0xAAAAAAAA );
    vSelect = Vec_PtrAlloc( 100 );
    Vec_PtrForEachEntry( Amap_Gat_t *, p->vSorted, pGate, i )
    {
        if ( pGate->pFunc == NULL || pGate->pTwin != NULL )
            continue;
        Vec_PtrForEachEntryStop( Amap_Gat_t *, p->vSorted, pGate2, k, i )
        {
            if ( pGate2->pFunc == NULL || pGate2->pTwin != NULL )
                continue;
            if ( pGate2->nPins != pGate->nPins )
                continue;
            if ( !memcmp( pGate2->pFunc, pGate->pFunc, sizeof(unsigned) * Abc_TruthWordNum(pGate->nPins) ) )
                break;
        }
        if ( k < i )
            continue;
        Vec_PtrPush( vSelect, pGate );            
    }
    return vSelect;
}

/**Function*************************************************************

  Synopsis    [Selects gates useful for area-only mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Amap_LibPrintSelectedGates( Amap_Lib_t * p, int fAllGates )
{
    Vec_Ptr_t * vArray;
    Amap_Gat_t * pGate;
    int i;
    vArray = fAllGates? p->vGates : p->vSelect;
    Vec_PtrForEachEntry( Amap_Gat_t *, vArray, pGate, i )
    {
        printf( "%3d :%12s %d %9.2f  ", i, pGate->pName, pGate->nPins, pGate->dArea );
        printf( "%4s=%40s  ", pGate->pOutName, pGate->pForm );
        printf( "DSD: " );
        Kit_DsdPrintFromTruth( pGate->pFunc, pGate->nPins );
        printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Parses equations for the gates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Amap_Lib_t * Amap_LibReadAndPrepare( char * pFileName, char * pBuffer, int fVerbose, int fVeryVerbose )
{
    Amap_Lib_t * p;
    abctime clk = Abc_Clock();
    if ( pBuffer == NULL )
        p = Amap_LibReadFile( pFileName, fVerbose );
    else
    {
        p = Amap_LibReadBuffer( pBuffer, fVerbose );
        if ( p )
            p->pName = Abc_UtilStrsav( pFileName );
    }
    if ( fVerbose )
        printf( "Read %d gates from file \"%s\".\n", Vec_PtrSize(p->vGates), pFileName );
    if ( p == NULL )
        return NULL;
    if ( !Amap_LibParseEquations( p, fVerbose ) )
    {
        Amap_LibFree( p );
        return NULL;
    }
    p->vSorted = Amap_LibSortGatesByArea( p );
    p->vSelect = Amap_LibSelectGates( p, fVerbose );
    if ( fVerbose )
    {
        printf( "Selected %d functionally unique gates. ", Vec_PtrSize(p->vSelect) );
        ABC_PRT( "Time", Abc_Clock() - clk );
//       Amap_LibPrintSelectedGates( p, 0 );
    }
    clk = Abc_Clock();
    Amap_LibCreateRules( p, fVeryVerbose );
    if ( fVerbose )
    {
        printf( "Created %d rules and %d matches. ", p->nNodes, p->nSets );
        ABC_PRT( "Time", Abc_Clock() - clk );
    }
    return p;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

