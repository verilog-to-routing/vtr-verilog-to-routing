/**CFile****************************************************************

  FileName    [mioUtils.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [File reading/writing for technology mapping.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: mioUtils.c,v 1.6 2004/09/03 18:02:20 satrajit Exp $]

***********************************************************************/

#include <math.h>
#include "mioInt.h"
#include "base/main/main.h"
#include "exp.h"
#include "misc/util/utilTruth.h"
#include "opt/dau/dau.h"
#include "misc/util/utilNam.h"
#include "map/scl/sclLib.h"
#include "map/scl/sclCon.h"

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
void Mio_LibraryDelete( Mio_Library_t * pLib )
{
    Mio_Gate_t * pGate, * pGate2;
    if ( pLib == NULL )
        return;
    Mio_LibraryMatchesStop( pLib );
    Mio_LibraryMatches2Stop( pLib );
    // free the bindings of nodes to gates from this library for all networks
    Abc_FrameUnmapAllNetworks( Abc_FrameGetGlobalFrame() );
    // free the library
    ABC_FREE( pLib->pName );
    Mio_LibraryForEachGateSafe( pLib, pGate, pGate2 )
        Mio_GateDelete( pGate );
    Mem_FlexStop( pLib->pMmFlex, 0 );
    Vec_StrFree( pLib->vCube );
    if ( pLib->tName2Gate )
        st__free_table( pLib->tName2Gate );
//    if ( pLib->dd )
//        Cudd_Quit( pLib->dd );
    ABC_FREE( pLib->ppGates0 );
    ABC_FREE( pLib->ppGatesName );
    ABC_FREE( pLib );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mio_GateDelete( Mio_Gate_t * pGate )
{
    Mio_Pin_t * pPin, * pPin2;
    if ( pGate->nInputs > 6 )
        ABC_FREE( pGate->pTruth );
    Vec_IntFreeP( &pGate->vExpr );
    ABC_FREE( pGate->pOutName );
    ABC_FREE( pGate->pName );
    ABC_FREE( pGate->pForm );
//    if ( pGate->bFunc )
//        Cudd_RecursiveDeref( pGate->pLib->dd, pGate->bFunc );
    Mio_GateForEachPinSafe( pGate, pPin, pPin2 )
        Mio_PinDelete( pPin );   
    ABC_FREE( pGate );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mio_PinDelete( Mio_Pin_t * pPin )
{
    ABC_FREE( pPin->pName );
    ABC_FREE( pPin );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mio_Pin_t * Mio_PinDup( Mio_Pin_t * pPin )
{
    Mio_Pin_t * pPinNew;

    pPinNew = ABC_ALLOC( Mio_Pin_t, 1 );
    *pPinNew = *pPin;
    pPinNew->pName = (pPinNew->pName ? Abc_UtilStrsav(pPinNew->pName) : NULL);
    pPinNew->pNext = NULL;

    return pPinNew;
}




/**Function*************************************************************

  Synopsis    [Check if pin characteristics are the same.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mio_CheckPins( Mio_Pin_t * pPin1, Mio_Pin_t * pPin2 )
{
    if ( pPin1 == NULL || pPin2 == NULL )
        return 1;
    if ( pPin1->dLoadInput != pPin2->dLoadInput )
        return 0;
    if ( pPin1->dLoadMax != pPin2->dLoadMax )
        return 0;
    if ( pPin1->dDelayBlockRise != pPin2->dDelayBlockRise )
        return 0;
    if ( pPin1->dDelayFanoutRise != pPin2->dDelayFanoutRise )
        return 0;
    if ( pPin1->dDelayBlockFall != pPin2->dDelayBlockFall )
        return 0;
    if ( pPin1->dDelayFanoutFall != pPin2->dDelayFanoutFall )
        return 0;
    return 1;
}
int Mio_CheckGates( Mio_Library_t * pLib )
{
    Mio_Gate_t * pGate;
    Mio_Pin_t * pPin0 = NULL, * pPin = NULL;
    Mio_LibraryForEachGate( pLib, pGate )
        Mio_GateForEachPin( pGate, pPin )
            if ( Mio_CheckPins( pPin0, pPin ) )
                pPin0 = pPin;
            else
                return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mio_WritePin( FILE * pFile, Mio_Pin_t * pPin, int NameLen, int fAllPins )
{
    char * pPhaseNames[10] = { "UNKNOWN", "INV", "NONINV" };
    if ( fAllPins )
        fprintf( pFile, "PIN *  " );
    else
        fprintf( pFile, "\n    PIN %*s  ", NameLen, pPin->pName );
    fprintf( pFile, "%7s ",   pPhaseNames[pPin->Phase] );
    fprintf( pFile, "%3d ",   (int)pPin->dLoadInput );
    fprintf( pFile, "%3d ",   (int)pPin->dLoadMax );
    fprintf( pFile, "%8.2f ", pPin->dDelayBlockRise );
    fprintf( pFile, "%8.2f ", pPin->dDelayFanoutRise );
    fprintf( pFile, "%8.2f ", pPin->dDelayBlockFall );
    fprintf( pFile, "%8.2f",  pPin->dDelayFanoutFall );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mio_WriteGate( FILE * pFile, Mio_Gate_t * pGate, int GateLen, int NameLen, int FormLen, int fPrintSops, int fAllPins )
{
    char Buffer[5000];
    Mio_Pin_t * pPin;
    assert( NameLen+FormLen+2 < 5000 );
    sprintf( Buffer, "%s=%s;",    pGate->pOutName, pGate->pForm );
    fprintf( pFile, "GATE %-*s ", GateLen, pGate->pName );
    fprintf( pFile, "%8.2f  ",    pGate->dArea );
    fprintf( pFile, "%-*s ",      Abc_MinInt(NameLen+FormLen+2, 60), Buffer );
    // print the pins
    if ( fPrintSops )
        fprintf( pFile, "%s",       pGate->pSop? pGate->pSop : "unspecified\n" );
    if ( fAllPins && pGate->pPins ) // equal pins
        Mio_WritePin( pFile, pGate->pPins, NameLen, 1 );
    else // different pins
        Mio_GateForEachPin( pGate, pPin )
            Mio_WritePin( pFile, pPin, NameLen, 0 );
    fprintf( pFile, "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mio_WriteLibrary( FILE * pFile, Mio_Library_t * pLib, int fPrintSops, int fShort, int fSelected )
{
    Mio_Gate_t * pGate;
    Mio_Pin_t * pPin;
    Vec_Ptr_t * vGates = Vec_PtrAlloc( 1000 );
    int i, nCells, GateLen = 0, NameLen = 0, FormLen = 0;
    int fAllPins = fShort || Mio_CheckGates( pLib );
    if ( fSelected )
    {
        Mio_Cell2_t * pCells = Mio_CollectRootsNewDefault2( 6, &nCells, 0 );
        for ( i = 0; i < nCells; i++ )
            Vec_PtrPush( vGates, pCells[i].pMioGate );
        ABC_FREE( pCells );
    }
    else
    {
        for ( i = 0; i < pLib->nGates; i++ )
            Vec_PtrPush( vGates, pLib->ppGates0[i] );
    }
    Vec_PtrForEachEntry( Mio_Gate_t *, vGates, pGate, i )
    {
        GateLen = Abc_MaxInt( GateLen, strlen(pGate->pName) );
        NameLen = Abc_MaxInt( NameLen, strlen(pGate->pOutName) );
        FormLen = Abc_MaxInt( FormLen, strlen(pGate->pForm) );
        Mio_GateForEachPin( pGate, pPin )
            NameLen = Abc_MaxInt( NameLen, strlen(pPin->pName) );
    }
    fprintf( pFile, "# The genlib library \"%s\" with %d gates written by ABC on %s\n", pLib->pName, Vec_PtrSize(vGates), Extra_TimeStamp() );
    Vec_PtrForEachEntry( Mio_Gate_t *, vGates, pGate, i )
        Mio_WriteGate( pFile, pGate, GateLen, NameLen, FormLen, fPrintSops, fAllPins );
    Vec_PtrFree( vGates );
}

/**Function*************************************************************

  Synopsis    [Compares the max delay of two gates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mio_DelayCompare( Mio_Gate_t ** ppG1, Mio_Gate_t ** ppG2 )
{
    int Comp;
    float Eps = (float)0.0094636;
    if ( (*ppG1)->dDelayMax < (*ppG2)->dDelayMax - Eps )
        return -1;
    if ( (*ppG1)->dDelayMax > (*ppG2)->dDelayMax + Eps )
        return 1;
    // compare names
    Comp = strcmp( (*ppG1)->pName, (*ppG2)->pName );
    if ( Comp < 0 )
        return -1;
    if ( Comp > 0 )
        return 1;
    assert( 0 );
    return 0;
}
int Mio_AreaCompare( Mio_Cell_t * pG1, Mio_Cell_t * pG2 )
{
    int Comp;
    float Eps = (float)0.0094636;
    if ( pG1->nFanins < pG2->nFanins )
        return -1;
    if ( pG1->nFanins > pG2->nFanins )
        return 1;
    if ( pG1->Area < pG2->Area - Eps )
        return -1;
    if ( pG1->Area > pG2->Area + Eps )
        return 1;
    // compare names
    Comp = strcmp( pG1->pName, pG2->pName );
    if ( Comp < 0 )
        return -1;
    if ( Comp > 0 )
        return 1;
    assert( 0 );
    return 0;
}
int Mio_AreaCompare2( Mio_Cell2_t * pG1, Mio_Cell2_t * pG2 )
{
    int Comp;
    if ( pG1->nFanins < pG2->nFanins )
        return -1;
    if ( pG1->nFanins > pG2->nFanins )
        return 1;
    if ( pG1->AreaW < pG2->AreaW )
        return -1;
    if ( pG1->AreaW > pG2->AreaW )
        return 1;
    // compare names
    Comp = strcmp( pG1->pName, pG2->pName );
    if ( Comp < 0 )
        return -1;
    if ( Comp > 0 )
        return 1;
    assert( 0 );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Collects the set of root gates.]

  Description [Only collects the gates with unique functionality, 
  which have fewer inputs and shorter delay than the given limits.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline float Mio_CellDelayAve( Mio_Cell_t * pCell )
{
    float CellDelay = 0; int k;
    for ( k = 0; k < (int)pCell->nFanins; k++ )
        CellDelay += pCell->Delays[k];
    if ( pCell->nFanins )
        CellDelay /= pCell->nFanins;
    return CellDelay;
}
static inline float Mio_GateDelayAve( Mio_Gate_t * pGate )
{
    float GateDelay = 0;
    Mio_Pin_t * pPin; 
    Mio_GateForEachPin( pGate, pPin )
        GateDelay += (float)(0.5 * pPin->dDelayBlockRise + 0.5 * pPin->dDelayBlockFall);
    if ( pGate->nInputs )
        GateDelay /= pGate->nInputs;
    return GateDelay;
}
static inline int Mio_CompareTwoGates( Mio_Gate_t * pCell, Mio_Gate_t * pGate )
{
    int Comp;
    float Eps = (float)0.0094636;
    float CellDelay, GateDelay;
    // compare areas
    if ( pCell->dArea > (float)pGate->dArea + Eps )
        return 1;
    if ( pCell->dArea < (float)pGate->dArea - Eps )
        return 0;
    // compare delays
    CellDelay = Mio_GateDelayAve( pCell );
    GateDelay = Mio_GateDelayAve( pGate );
    if ( CellDelay > GateDelay + Eps )
        return 1;
    if ( CellDelay < GateDelay - Eps )
        return 0;
    // compare names
    Comp = strcmp( pCell->pName, pGate->pName );
    if ( Comp > 0 )
        return 1;
    if ( Comp < 0 )
        return 0;
    assert( 0 );
    return 0;
}
Mio_Gate_t ** Mio_CollectRoots( Mio_Library_t * pLib, int nInputs, float tDelay, int fSkipInv, int * pnGates, int fVerbose )
{
    Mio_Gate_t * pGate;
    Mio_Gate_t ** ppGates;
    int i, nGates, iGate, fProfile;
    nGates = Mio_LibraryReadGateNum( pLib );
    ppGates = ABC_ALLOC( Mio_Gate_t *, nGates );
    iGate = 0;
    // check if profile is entered
    fProfile = Mio_LibraryHasProfile( pLib );
    if ( fProfile )
        printf( "Mio_CollectRoots(): Using gate profile to select gates for mapping.\n" );
    // for each functionality, select gate with the smallest area
    // if equal areas, select gate with lexicographically smaller name
    Mio_LibraryForEachGate( pLib, pGate )
    {
        if ( pGate->nInputs > nInputs )
            continue;
        if ( fProfile && Mio_GateReadProfile(pGate) == 0 && pGate->nInputs > 1 )
            continue;
        if ( tDelay > 0.0 && pGate->dDelayMax > (double)tDelay )
            continue;
        if ( pGate->uTruth == 0 || pGate->uTruth == ~(word)0 )
            continue;
        if ( pGate->uTruth == ABC_CONST(0xAAAAAAAAAAAAAAAA) )
            continue;
        if ( pGate->uTruth == ~ABC_CONST(0xAAAAAAAAAAAAAAAA) && fSkipInv )
            continue;
        if ( pGate->pTwin ) // skip multi-output gates for now
            continue;
        // check if the gate with this functionality already exists
        for ( i = 0; i < iGate; i++ )
            if ( ppGates[i]->uTruth == pGate->uTruth )
            {
                if ( Mio_CompareTwoGates(ppGates[i], pGate) )
                    ppGates[i] = pGate;
                break;
            }
        if ( i < iGate )
            continue;
        assert( iGate < nGates );
        ppGates[ iGate++ ] = pGate;
        if ( fVerbose )
            printf( "Selected gate %3d:   %-20s  A = %7.2f  D = %7.2f  %3s = %-s\n", 
                iGate+1, pGate->pName, pGate->dArea, pGate->dDelayMax, pGate->pOutName, pGate->pForm );
    }
    // sort by delay
    if ( iGate > 0 ) 
    {
        qsort( (void *)ppGates, iGate, sizeof(Mio_Gate_t *), 
                (int (*)(const void *, const void *)) Mio_DelayCompare );
        assert( Mio_DelayCompare( ppGates, ppGates + iGate - 1 ) <= 0 );
    }
    if ( pnGates )
        *pnGates = iGate;
    return ppGates;
}


/**Function*************************************************************

  Synopsis    [Collects the set of root gates.]

  Description [Only collects the gates with unique functionality, 
  which have fewer inputs and shorter delay than the given limits.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Mio_CompareTwo( Mio_Cell_t * pCell, Mio_Gate_t * pGate )
{
    int Comp;
    float Eps = (float)0.0094636;
    float CellDelay, GateDelay;
    // compare areas
    if ( pCell->Area > (float)pGate->dArea + Eps )
        return 1;
    if ( pCell->Area < (float)pGate->dArea - Eps )
        return 0;
    // compare delays
    CellDelay = Mio_CellDelayAve( pCell );
    GateDelay = Mio_GateDelayAve( pGate );
    if ( CellDelay > GateDelay + Eps )
        return 1;
    if ( CellDelay < GateDelay - Eps )
        return 0;
    // compare names
    Comp = strcmp( pCell->pName, pGate->pName );
    if ( Comp > 0 )
        return 1;
    if ( Comp < 0 )
        return 0;
    assert( 0 );
    return 0;
}
static inline void Mio_CollectCopy( Mio_Cell_t * pCell, Mio_Gate_t * pGate )
{
    Mio_Pin_t * pPin;  int k;
    pCell->pName   = pGate->pName;
    pCell->uTruth  = pGate->uTruth;
    pCell->Area    = (float)pGate->dArea;
    pCell->nFanins = pGate->nInputs;
    for ( k = 0, pPin = pGate->pPins; pPin; pPin = pPin->pNext, k++ )
        pCell->Delays[k] = (float)(0.5 * pPin->dDelayBlockRise + 0.5 * pPin->dDelayBlockFall);
}

Mio_Cell_t * Mio_CollectRootsNew( Mio_Library_t * pLib, int nInputs, int * pnGates, int fVerbose )
{
    Mio_Gate_t * pGate;
    Mio_Cell_t * ppCells;
    int i, nGates, iCell = 4;
    nGates = Mio_LibraryReadGateNum( pLib );
    ppCells = ABC_CALLOC( Mio_Cell_t, nGates + 4 );
    // for each functionality, select gate with the smallest area
    // if equal areas, select gate with smaller average pin delay
    // if these are also equal, select lexicographically smaller name
    Mio_LibraryForEachGate( pLib, pGate )
    {
        if ( pGate->nInputs > nInputs || pGate->pTwin ) // skip large and multi-output
            continue;
        // check if the gate with this functionality already exists
        for ( i = 0; i < iCell; i++ )
            if ( ppCells[i].pName && ppCells[i].uTruth == pGate->uTruth )
            {
                if ( Mio_CompareTwo( ppCells + i, pGate ) )
                    Mio_CollectCopy( ppCells + i, pGate );
                break;
            }
        if ( i < iCell )
            continue;
        if ( pGate->uTruth == 0 || pGate->uTruth == ~(word)0 )
        {
            int Idx = (int)(pGate->uTruth == ~(word)0);
            assert( pGate->nInputs == 0 );
            Mio_CollectCopy( ppCells + Idx, pGate );
            continue;
        }
        if ( pGate->uTruth == ABC_CONST(0xAAAAAAAAAAAAAAAA) || pGate->uTruth == ~ABC_CONST(0xAAAAAAAAAAAAAAAA) )
        {
            int Idx = 2 + (int)(pGate->uTruth == ~ABC_CONST(0xAAAAAAAAAAAAAAAA));
            assert( pGate->nInputs == 1 );
            Mio_CollectCopy( ppCells + Idx, pGate );
            continue;
        }
        Mio_CollectCopy( ppCells + iCell++, pGate );
    }
    if ( ppCells[0].pName == NULL )
        { printf( "Error: Cannot find constant 0 gate in the library.\n" ); return NULL; }
    if ( ppCells[1].pName == NULL )
        { printf( "Error: Cannot find constant 1 gate in the library.\n" ); return NULL; }
    if ( ppCells[2].pName == NULL )
        { printf( "Error: Cannot find buffer gate in the library.\n" );     return NULL; }
    if ( ppCells[3].pName == NULL )
        { printf( "Error: Cannot find inverter gate in the library.\n" );   return NULL; }
    // sort by delay
    if ( iCell > 5 ) 
    {
        qsort( (void *)(ppCells + 4), iCell - 4, sizeof(Mio_Cell_t), 
                (int (*)(const void *, const void *)) Mio_AreaCompare );
        assert( Mio_AreaCompare( ppCells + 4, ppCells + iCell - 1 ) <= 0 );
    }
    // assign IDs
    for ( i = 0; i < iCell; i++ )
        ppCells[i].Id = ppCells[i].pName ? i : -1;

    // report
    if ( fVerbose )
    {
        // count gates
        int * pCounts = ABC_CALLOC( int, nGates + 4 );
        Mio_LibraryForEachGate( pLib, pGate )
        {
            if ( pGate->nInputs > nInputs || pGate->pTwin ) // skip large and multi-output
                continue;
            for ( i = 0; i < iCell; i++ )
                if ( ppCells[i].pName && ppCells[i].uTruth == pGate->uTruth )
                {
                    pCounts[i]++;
                    break;
                }
            assert( i < iCell );
        }
        for ( i = 0; i < iCell; i++ )
        {
            Mio_Cell_t * pCell = ppCells + i;
            printf( "%4d : ", i );
            if ( pCell->pName == NULL )
                printf( "None\n" );
            else
                printf( "%-20s   In = %d   N = %3d   A = %12.6f   D = %12.6f\n", 
                    pCell->pName, pCell->nFanins, pCounts[i], pCell->Area, Mio_CellDelayAve(pCell) );
        }
        ABC_FREE( pCounts );
    }
    if ( pnGates )
        *pnGates = iCell;
    return ppCells;
}
Mio_Cell_t * Mio_CollectRootsNewDefault( int nInputs, int * pnGates, int fVerbose )
{
    return Mio_CollectRootsNew( (Mio_Library_t *)Abc_FrameReadLibGen(), nInputs, pnGates, fVerbose );
}

/**Function*************************************************************

  Synopsis    [Collects the set of root gates.]

  Description [Only collects the gates with unique functionality, 
  which have fewer inputs and shorter delay than the given limits.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Mio_CompareTwo2( Mio_Cell2_t * pCell1, Mio_Cell2_t * pCell2 )
{
    int Comp;
    // compare areas
    if ( pCell1->AreaW > pCell2->AreaW )
        return 1;
    if ( pCell1->AreaW < pCell2->AreaW )
        return 0;
    // compare delays
    if ( pCell1->iDelayAve > pCell2->iDelayAve )
        return 1;
    if ( pCell1->iDelayAve < pCell2->iDelayAve )
        return 0;
    // compare names
    Comp = strcmp( pCell1->pName, pCell2->pName );
    if ( Comp > 0 )
        return 1;
    if ( Comp < 0 )
        return 0;
    assert( 0 );
    return 0;
}
static inline void Mio_CollectCopy2( Mio_Cell2_t * pCell, Mio_Gate_t * pGate )
{
    Mio_Pin_t * pPin;  int k;
    pCell->pName     = pGate->pName;
    pCell->vExpr     = pGate->vExpr;
    pCell->uTruth    = pGate->uTruth;
    pCell->AreaF     = pGate->dArea;
    pCell->AreaW     = (word)(SCL_NUM * pGate->dArea);
    pCell->nFanins   = pGate->nInputs;
    pCell->pMioGate  = pGate;
    pCell->iDelayAve = 0;
    for ( k = 0, pPin = pGate->pPins; pPin; pPin = pPin->pNext, k++ )
    {
        pCell->iDelays[k] = (int)(SCL_NUM/2 * pPin->dDelayBlockRise + SCL_NUM/2 * pPin->dDelayBlockFall);
        pCell->iDelayAve += pCell->iDelays[k];
    }
    if ( pCell->nFanins )
        pCell->iDelayAve /= pCell->nFanins;
}

Mio_Cell2_t * Mio_CollectRootsNew2( Mio_Library_t * pLib, int nInputs, int * pnGates, int fVerbose )
{
    Mio_Gate_t * pGate0;
    Mio_Cell2_t * ppCells0, * ppCells, * pCell;
    int i, nGates, iCell0 = 0, iCell = 4;
    // create space for new cells
    nGates = Mio_LibraryReadGateNum( pLib );
    ppCells  = ABC_CALLOC( Mio_Cell2_t, nGates + 4 );
    // copy all gates first
    ppCells0 = ABC_CALLOC( Mio_Cell2_t, nGates );
    Mio_LibraryForEachGate( pLib, pGate0 )
        if ( !(pGate0->nInputs > nInputs || pGate0->pTwin) ) // skip large and multi-output
            Mio_CollectCopy2( ppCells0 + iCell0++, pGate0 );
    assert( iCell0 <= nGates );
    // for each functionality, select gate with the smallest area
    // if equal areas, select gate with smaller average pin delay
    // if these are also equal, select lexicographically smaller name
    for ( pCell = ppCells0; pCell < ppCells0 + iCell0; pCell++ )
    {
        // check if the gate with this functionality already exists
        for ( i = 0; i < iCell; i++ )
            if ( ppCells[i].pName && ppCells[i].uTruth == pCell->uTruth )
            {
                if ( Mio_CompareTwo2( ppCells + i, pCell ) )
                    ppCells[i] = *pCell;
                break;
            }
        if ( i < iCell )
            continue;
        if ( pCell->uTruth == 0 || pCell->uTruth == ~(word)0 )
        {
            int Idx = (int)(pCell->uTruth == ~(word)0);
            assert( pCell->nFanins == 0 );
            ppCells[Idx] = *pCell;
            continue;
        }
        if ( pCell->uTruth == ABC_CONST(0xAAAAAAAAAAAAAAAA) || pCell->uTruth == ~ABC_CONST(0xAAAAAAAAAAAAAAAA) )
        {
            int Idx = 2 + (int)(pCell->uTruth == ~ABC_CONST(0xAAAAAAAAAAAAAAAA));
            assert( pCell->nFanins == 1 );
            ppCells[Idx] = *pCell;
            continue;
        }
        ppCells[iCell++] = *pCell;
    }
    ABC_FREE( ppCells0 );
    if ( ppCells[0].pName == NULL )
        { printf( "Error: Cannot find constant 0 gate in the library.\n" ); return NULL; }
    if ( ppCells[1].pName == NULL )
        { printf( "Error: Cannot find constant 1 gate in the library.\n" ); return NULL; }
    if ( ppCells[2].pName == NULL )
        { printf( "Error: Cannot find buffer gate in the library.\n" );     return NULL; }
    if ( ppCells[3].pName == NULL )
        { printf( "Error: Cannot find inverter gate in the library.\n" );   return NULL; }
    // sort by delay
    if ( iCell > 5 ) 
    {
        qsort( (void *)(ppCells + 4), iCell - 4, sizeof(Mio_Cell2_t), 
                (int (*)(const void *, const void *)) Mio_AreaCompare2 );
        assert( Mio_AreaCompare2( ppCells + 4, ppCells + iCell - 1 ) <= 0 );
    }
    // assign IDs
    Mio_LibraryForEachGate( pLib, pGate0 )
        Mio_GateSetCell( pGate0, -1 );
    for ( i = 0; i < iCell; i++ )
    {
        ppCells[i].Id = ppCells[i].pName ? i : -1;
        Mio_GateSetCell( (Mio_Gate_t *)ppCells[i].pMioGate, i );
    }

    // report
    if ( fVerbose )
    {
        // count gates
        int * pCounts = ABC_CALLOC( int, nGates + 4 );
        Mio_LibraryForEachGate( pLib, pGate0 )
        {
            if ( pGate0->nInputs > nInputs || pGate0->pTwin ) // skip large and multi-output
                continue;
            for ( i = 0; i < iCell; i++ )
                if ( ppCells[i].pName && ppCells[i].uTruth == pGate0->uTruth )
                {
                    pCounts[i]++;
                    break;
                }
            assert( i < iCell );
        }
        for ( i = 0; i < iCell; i++ )
        {
            Mio_Cell2_t * pCell = ppCells + i;
            printf( "%4d : ", i );
            if ( pCell->pName == NULL )
                printf( "None\n" );
            else
                printf( "%-20s   In = %d   N = %3d   A = %12.6f   D = %12.6f\n", 
                    pCell->pName, pCell->nFanins, pCounts[i], pCell->AreaF, Scl_Int2Flt(pCell->iDelayAve) );
        }
        ABC_FREE( pCounts );
    }
    if ( pnGates )
        *pnGates = iCell;
    return ppCells;
}
Mio_Cell2_t * Mio_CollectRootsNewDefault2( int nInputs, int * pnGates, int fVerbose )
{
    return Mio_CollectRootsNew2( (Mio_Library_t *)Abc_FrameReadLibGen(), nInputs, pnGates, fVerbose );
}

/**Function*************************************************************

  Synopsis    [Derives the truth table of the gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mio_CollectRootsNewDefault3( int nInputs, Vec_Ptr_t ** pvNames, Vec_Wrd_t ** pvTruths )
{
    Mio_Library_t * pLib = (Mio_Library_t *)Abc_FrameReadLibGen();
    int i, iGate = 0, nGates = pLib ? Mio_LibraryReadGateNum( pLib ) : 0;
    Mio_Gate_t * pGate0, ** ppGates; word * pTruth;
    if ( pLib == NULL )
        return 0;
    ppGates = ABC_CALLOC( Mio_Gate_t *, nGates );
    Mio_LibraryForEachGate( pLib, pGate0 )
        ppGates[pGate0->Cell] = pGate0;
    *pvNames  = Vec_PtrAlloc( nGates );
    *pvTruths = Vec_WrdStart( nGates * 4 );
    for ( i = 0; i < nGates; i++ )
    {
        pGate0 = ppGates[i];
        if ( pGate0->nInputs > nInputs || pGate0->pTwin ) // skip large and multi-output
            continue;
        Vec_PtrPush( *pvNames, pGate0->pName );
        pTruth = Vec_WrdEntryP( *pvTruths, iGate++*4 );
        if ( pGate0->nInputs <= 6 )
            pTruth[0] = pTruth[1] = pTruth[2] = pTruth[3] = pGate0->uTruth;
        else if ( pGate0->nInputs == 7 )
        {
            pTruth[0] = pTruth[2] = pGate0->pTruth[0];
            pTruth[1] = pTruth[3] = pGate0->pTruth[1];
        }
        else if ( pGate0->nInputs == 8 )
            memcpy( pTruth, pGate0->pTruth, 4*sizeof(word) );
    }
    assert( iGate == nGates );
    assert( Vec_WrdEntry(*pvTruths,  0) ==        0 );
    assert( Vec_WrdEntry(*pvTruths,  4) == ~(word)0 );
    assert( Vec_WrdEntry(*pvTruths,  8) ==  s_Truths6[0] );
    assert( Vec_WrdEntry(*pvTruths, 12) == ~s_Truths6[0] );
    ABC_FREE( ppGates );
    return nGates;
}

/**Function*************************************************************

  Synopsis    [Derives the truth table of the gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
word Mio_DeriveTruthTable6( Mio_Gate_t * pGate )
{
    static unsigned uTruths6[6][2] = {
        { 0xAAAAAAAA, 0xAAAAAAAA },
        { 0xCCCCCCCC, 0xCCCCCCCC },
        { 0xF0F0F0F0, 0xF0F0F0F0 },
        { 0xFF00FF00, 0xFF00FF00 },
        { 0xFFFF0000, 0xFFFF0000 },
        { 0x00000000, 0xFFFFFFFF }
    };
    union {
      unsigned u[2];
      word w;
    } uTruthRes;
    assert( pGate->nInputs <= 6 );
    Mio_DeriveTruthTable( pGate, uTruths6, pGate->nInputs, 6, uTruthRes.u );
    return uTruthRes.w;
}

#if 0

/**Function*************************************************************

  Synopsis    [Recursively derives the truth table of the gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mio_DeriveTruthTable_rec( DdNode * bFunc, unsigned uTruthsIn[][2], unsigned uTruthRes[] )
{
    unsigned uTruthsCof0[2];
    unsigned uTruthsCof1[2];

    // complement the resulting truth table, if the function is complemented
    if ( Cudd_IsComplement(bFunc) )
    {
        Mio_DeriveTruthTable_rec( Cudd_Not(bFunc), uTruthsIn, uTruthRes );
        uTruthRes[0] = ~uTruthRes[0];
        uTruthRes[1] = ~uTruthRes[1];
        return;
    }

    // if the function is constant 1, return the constant 1 truth table
    if ( bFunc->index == CUDD_CONST_INDEX )
    {
        uTruthRes[0] = MIO_FULL;
        uTruthRes[1] = MIO_FULL;
        return;
    }

    // solve the problem for both cofactors
    Mio_DeriveTruthTable_rec( cuddE(bFunc), uTruthsIn, uTruthsCof0 );
    Mio_DeriveTruthTable_rec( cuddT(bFunc), uTruthsIn, uTruthsCof1 );

    // derive the resulting truth table using the input truth tables
    uTruthRes[0] = (uTruthsCof0[0] & ~uTruthsIn[bFunc->index][0]) |
                   (uTruthsCof1[0] &  uTruthsIn[bFunc->index][0]);
    uTruthRes[1] = (uTruthsCof0[1] & ~uTruthsIn[bFunc->index][1]) |
                   (uTruthsCof1[1] &  uTruthsIn[bFunc->index][1]);
}

/**Function*************************************************************

  Synopsis    [Derives the truth table of the gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mio_DeriveTruthTable( Mio_Gate_t * pGate, unsigned uTruthsIn[][2], int nSigns, int nInputs, unsigned uTruthRes[] )
{
    Mio_DeriveTruthTable_rec( pGate->bFunc, uTruthsIn, uTruthRes );
}

#endif

/**Function*************************************************************

  Synopsis    [Derives the truth table of the gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mio_DeriveTruthTable( Mio_Gate_t * pGate, unsigned uTruthsIn[][2], int nSigns, int nInputs, unsigned uTruthRes[] )
{
    word uRes, uFanins[6];
    int i;
    assert( pGate->nInputs == nSigns );
    for ( i = 0; i < nSigns; i++ )
        uFanins[i] = (((word)uTruthsIn[i][1]) << 32) | (word)uTruthsIn[i][0];
    uRes = Exp_Truth6( nSigns, pGate->vExpr, (word *)uFanins );
    uTruthRes[0] = uRes & 0xFFFFFFFF;
    uTruthRes[1] = uRes >> 32;
}

/**Function*************************************************************

  Synopsis    [Reads the number of variables in the cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mio_SopGetVarNum( char * pSop )
{
    char * pCur;
    for ( pCur = pSop; *pCur != '\n'; pCur++ )
        if ( *pCur == 0 )
            return -1;
    return pCur - pSop - 2;
}

/**Function*************************************************************

  Synopsis    [Derives the truth table of the root of the gate.]

  Description [Given the truth tables of the leaves of the gate,
  this procedure derives the truth table of the root.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mio_DeriveTruthTable2( Mio_Gate_t * pGate, unsigned uTruthsIn[][2], int nTruths, int nInputs, unsigned uTruthRes[] )
{
    unsigned uSignCube[2];
    int i, nFanins;
    char * pCube;

    // make sure that the number of input truth tables in equal to the number of gate inputs
    assert( pGate->nInputs == nTruths );
    assert( nInputs < 7 );

    nFanins = Mio_SopGetVarNum( pGate->pSop );
    assert( nFanins == nInputs );

    // clean the resulting truth table
    uTruthRes[0] = 0;
    uTruthRes[1] = 0;
    if ( nInputs < 6 )
    {
//        Abc_SopForEachCube( pGate->pSop, nFanins, pCube )
        for ( pCube = pGate->pSop; *pCube; pCube += (nFanins) + 3 )
        {
            // add the clause
            uSignCube[0] = MIO_FULL;
            for ( i = 0; i < nFanins; i++ )
            {
                if ( pCube[i] == '0' )
                    uSignCube[0] &= ~uTruthsIn[i][0];
                else if ( pCube[i] == '1' )
                    uSignCube[0] &=  uTruthsIn[i][0];
            }
        }
        if ( nInputs < 5 )
            uTruthRes[0] &= MIO_MASK(1<<nInputs);
    }
    else
    {
        // consider the case when two unsigneds should be used
//        Abc_SopForEachCube( pGate->pSop, nFanins, pCube )
        for ( pCube = pGate->pSop; *pCube; pCube += (nFanins) + 3 )
        {
            uSignCube[0] = MIO_FULL;
            uSignCube[1] = MIO_FULL;
            for ( i = 0; i < nFanins; i++ )
            {
                if ( pCube[i] == '0' )
                {
                    uSignCube[0] &= ~uTruthsIn[i][0];
                    uSignCube[1] &= ~uTruthsIn[i][1];
                }
                else if ( pCube[i] == '1' )
                {
                    uSignCube[0] &=  uTruthsIn[i][0];
                    uSignCube[1] &=  uTruthsIn[i][1];
                }
            }
            uTruthRes[0] |= uSignCube[0];
            uTruthRes[1] |= uSignCube[1];
        }
    }
}

/**Function*************************************************************

  Synopsis    [Derives the area and delay of the root of the gate.]

  Description [Array of the resulting delays should be initialized 
  to the (negative) SUPER_NO_VAR value.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mio_DeriveGateDelays( Mio_Gate_t * pGate, 
    float ** ptPinDelays, int nPins, int nInputs, float tDelayZero, 
    float * ptDelaysRes, float * ptPinDelayMax )
{
    Mio_Pin_t * pPin;
    float Delay, DelayMax;
    int i, k;
    assert( pGate->nInputs == nPins );
    // set all the delays to the unused delay
    for ( i = 0; i < nInputs; i++ )
        ptDelaysRes[i] = tDelayZero;
    // compute the delays for each input and the max delay at the same time
    DelayMax = 0;
    for ( i = 0; i < nInputs; i++ )
    {
        for ( k = 0, pPin = pGate->pPins; pPin; pPin = pPin->pNext, k++ )
        {
            if ( ptPinDelays[k][i] < 0 )
                continue;
            Delay = ptPinDelays[k][i] + (float)pPin->dDelayBlockMax;
            if ( ptDelaysRes[i] < Delay )
                ptDelaysRes[i] = Delay;
        }
        if ( k != nPins )
        {
            printf ("DEBUG: problem gate is %s\n", Mio_GateReadName( pGate ));
        }
        assert( k == nPins );
        if ( DelayMax < ptDelaysRes[i] )
            DelayMax = ptDelaysRes[i];
    }
    *ptPinDelayMax = DelayMax;
}


/**Function*************************************************************

  Synopsis    [Creates a pseudo-gate.]

  Description [The pseudo-gate is a N-input gate with all info set to 0.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mio_Gate_t * Mio_GateCreatePseudo( int nInputs )
{
    Mio_Gate_t * pGate;
    Mio_Pin_t * pPin;
    int i;
    // allocate the gate structure
    pGate = ABC_ALLOC( Mio_Gate_t, 1 );
    memset( pGate, 0, sizeof(Mio_Gate_t) );
    pGate->nInputs = nInputs;
    // create pins
    for ( i = 0; i < nInputs; i++ )
    {
        pPin = ABC_ALLOC( Mio_Pin_t, 1 );
        memset( pPin, 0, sizeof(Mio_Pin_t) );
        pPin->pNext = pGate->pPins;
        pGate->pPins = pPin;
    }
    return pGate;
}

/**Function*************************************************************

  Synopsis    [Adds constant value to all delay values.]

  Description [The pseudo-gate is a N-input gate with all info set to 0.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mio_LibraryShiftDelay( Mio_Library_t * pLib, double Shift )
{
    Mio_Gate_t * pGate;
    Mio_Pin_t * pPin;
    Mio_LibraryForEachGate( pLib, pGate )
    {
        pGate->dDelayMax += Shift;
        Mio_GateForEachPin( pGate, pPin )
        {
            pPin->dDelayBlockRise += Shift;
            pPin->dDelayBlockFall += Shift;
            pPin->dDelayBlockMax  += Shift;
        }
    }
}

/**Function*************************************************************

  Synopsis    [Multiply areas/delays by values proportional to fanin count.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mio_LibraryMultiArea( Mio_Library_t * pLib, double Multi )
{
    Mio_Gate_t * pGate;
    Mio_LibraryForEachGate( pLib, pGate )
    {
        if ( pGate->nInputs < 2 )
            continue;
//        printf( "Before %8.3f  ", pGate->dArea );
        pGate->dArea *= pow( pGate->nInputs, Multi );
//        printf( "After %8.3f  Inputs = %d. Factor = %8.3f\n", pGate->dArea, pGate->nInputs, pow( pGate->nInputs, Multi ) );
    }
}
void Mio_LibraryMultiDelay( Mio_Library_t * pLib, double Multi )
{
    Mio_Gate_t * pGate;
    Mio_Pin_t * pPin;
    Mio_LibraryForEachGate( pLib, pGate )
    {
        if ( pGate->nInputs < 2 )
            continue;
//        printf( "Before %8.3f  ", pGate->dDelayMax );
        pGate->dDelayMax *= pow( pGate->nInputs, Multi );
//        printf( "After %8.3f  Inputs = %d. Factor = %8.3f\n", pGate->dDelayMax, pGate->nInputs, pow( pGate->nInputs, Multi ) );
        Mio_GateForEachPin( pGate, pPin )
        {
            pPin->dDelayBlockRise *= pow( pGate->nInputs, Multi );
            pPin->dDelayBlockFall *= pow( pGate->nInputs, Multi );
            pPin->dDelayBlockMax  *= pow( pGate->nInputs, Multi );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Transfers delays from the second to the first.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mio_LibraryTransferDelays( Mio_Library_t * pLibD, Mio_Library_t * pLibS )
{
    Mio_Gate_t * pGateD, * pGateS;
    Mio_Pin_t * pPinD, * pPinS;
    Mio_LibraryForEachGate( pLibS, pGateS )
    {
        Mio_LibraryForEachGate( pLibD, pGateD )
        {
            if ( pGateD->uTruth != pGateS->uTruth )
                continue;
            pPinS = Mio_GateReadPins( pGateS );
            Mio_GateForEachPin( pGateD, pPinD )
            {
                if (pPinS)
                {
                    pPinD->dDelayBlockRise = pPinS->dDelayBlockRise;
                    pPinD->dDelayBlockFall = pPinS->dDelayBlockFall;
                    pPinD->dDelayBlockMax  = pPinS->dDelayBlockMax;
                    pPinS = Mio_PinReadNext(pPinS);
                }
                else
                {
                    pPinD->dDelayBlockRise = 0;
                    pPinD->dDelayBlockFall = 0;
                    pPinD->dDelayBlockMax  = 0;
                }
            }
        }
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nf_ManPrepareGate( int nVars, word uTruth, int * pComp, int * pPerm, Vec_Wrd_t * vResult )
{
    int nPerms = Extra_Factorial( nVars );
    int nMints = (1 << nVars);
    word tCur, tTemp1, tTemp2;
    int i, p, c;
    Vec_WrdClear( vResult );
    for ( i = 0; i < 2; i++ )
    {
        tCur = i ? ~uTruth : uTruth;
        tTemp1 = tCur;
        for ( p = 0; p < nPerms; p++ )
        {
            tTemp2 = tCur;
            for ( c = 0; c < nMints; c++ )
            {
                Vec_WrdPush( vResult, tCur );
                tCur = Abc_Tt6Flip( tCur, pComp[c] );
            }
            assert( tTemp2 == tCur );
            tCur = Abc_Tt6SwapAdjacent( tCur, pPerm[p] );
        }
        assert( tTemp1 == tCur );
    }
}
void Nf_ManPreparePrint( int nVars, int * pComp, int * pPerm, char Line[2*720*64][8] )
{
    int nPerms = Extra_Factorial( nVars );
    int nMints = (1 << nVars);
    char * pChar, * pChar2;
    int i, p, c, n = 0;
    for ( i = 0; i < nVars; i++ )
        Line[0][i] = 'A' + nVars - 1 - i;
    Line[0][nVars] = '+';
    Line[0][nVars+1] = 0;
    for ( i = 0; i < 2; i++ )
    {
        Line[n][nVars] = i ? '-' : '+';
        for ( p = 0; p < nPerms; p++ )
        {
            for ( c = 0; c < nMints; c++ )
            {
                strcpy( Line[n+1], Line[n] ); n++;
                pChar = &Line[n][pComp[c]];
                if ( *pChar >= 'A' && *pChar <= 'Z' )
                    *pChar += 'a' - 'A';
                else if ( *pChar >= 'a' && *pChar <= 'z' )
                    *pChar -= 'a' - 'A';
            }
            pChar  = &Line[n][pPerm[p]];
            pChar2 = pChar + 1;
            ABC_SWAP( char, *pChar, *pChar2 );
        }
    }
    assert( n == 2*nPerms*nMints );
    n = 0;
    for ( i = 0; i < 2; i++ )
        for ( p = 0; p < nPerms; p++ )
            for ( c = 0; c < nMints; c++ )
                printf("%8d : %d %3d %2d : %s\n", n, i, p, c, Line[n]), n++;
}

void Nf_ManPrepareLibrary( Mio_Library_t * pLib )
{
//    char Lines[2*720*64][8];
//    Nf_ManPreparePrint( 6, pComp, pPerm, Lines );
    int * pComp[7];
    int * pPerm[7];
    Mio_Gate_t ** ppGates;
    Vec_Wrd_t * vResult;
    word * pTruths;
    int * pSizes;
    int nGates, i, nClasses = 0, nTotal;
    abctime clk = Abc_Clock();

    for ( i = 2; i <= 6; i++ )
        pComp[i] = Extra_GreyCodeSchedule( i );
    for ( i = 2; i <= 6; i++ )
        pPerm[i] = Extra_PermSchedule( i );

    // collect truth tables
    ppGates = Mio_CollectRoots( pLib, 6, (float)1.0e+20, 1, &nGates, 0 );
    pSizes  = ABC_CALLOC( int, nGates );
    pTruths = ABC_CALLOC( word, nGates );
    vResult = Vec_WrdAlloc( 2 * 720 * 64 );
    for ( i = 0; i < nGates; i++ )
    {
        pSizes[i] = Mio_GateReadPinNum( ppGates[i] );
        assert( pSizes[i] > 1 && pSizes[i] <= 6 );
        pTruths[i] = Mio_GateReadTruth( ppGates[i] );

        Nf_ManPrepareGate( pSizes[i], pTruths[i], pComp[pSizes[i]], pPerm[pSizes[i]], vResult );
        Vec_WrdUniqify(vResult);
        nClasses += Vec_WrdSize(vResult);
        nTotal = (1 << (pSizes[i]+1)) * Extra_Factorial(pSizes[i]);

        printf( "%6d : ", i );
        printf( "%16s : ", Mio_GateReadName( ppGates[i] ) );
        printf( "%48s : ", Mio_GateReadForm( ppGates[i] ) );
        printf( "Inputs = %2d   ", pSizes[i] );
        printf( "Total = %6d  ", nTotal );
        printf( "Classes = %6d ", Vec_WrdSize(vResult) );
        printf( "Configs = %8.2f ", 1.0*nTotal/Vec_WrdSize(vResult) );
        printf( "%6.2f %%  ", 100.0*Vec_WrdSize(vResult)/nTotal );
        Dau_DsdPrintFromTruth( &pTruths[i], pSizes[i] );
//        printf( "\n" );
    }
    Vec_WrdFree( vResult );
    ABC_FREE( ppGates );
    ABC_FREE( pSizes );
    ABC_FREE( pTruths );

    for ( i = 2; i <= 6; i++ )
        ABC_FREE( pComp[i] );
    for ( i = 2; i <= 6; i++ )
        ABC_FREE( pPerm[i] );

    printf( "Classes = %d.  ", nClasses );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
}

void Nf_ManPrepareLibraryTest2()
{
    Mio_Library_t * pLib = (Mio_Library_t *)Abc_FrameReadLibGen();
    if ( pLib != NULL )
        Nf_ManPrepareLibrary( pLib );
    else
        printf( "Standard cell library is not available.\n" );

}

/**Function*************************************************************

  Synopsis    [Install library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mio_LibraryTransferCellIds()
{
    Mio_Gate_t * pGate;
    Mio_Library_t * pLib = (Mio_Library_t *)Abc_FrameReadLibGen();
    SC_Lib * pScl = (SC_Lib *)Abc_FrameReadLibScl();
    int CellId;
    if ( pScl == NULL )
    {
        printf( "SC library cannot be found.\n" );
        return;
    }
    if ( pLib == NULL )
    {
        printf( "Genlib library cannot be found.\n" );
        return;
    }
    Mio_LibraryForEachGate( pLib, pGate )
    {
        if ( Mio_GateReadPinNum(pGate) == 0 )
            continue;
        CellId = Abc_SclCellFind( pScl, Mio_GateReadName(pGate) );
        if ( CellId < 0 )
            printf( "Cannot find cell ID of gate %s.\n", Mio_GateReadName(pGate) );
        else
            Mio_GateSetCell( pGate, CellId );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mio_LibraryReadProfile( FILE * pFile, Mio_Library_t * pLib )
{
    Mio_Gate_t * pGate;    
    char pBuffer[1000];
    while ( fgets( pBuffer, 1000, pFile ) != NULL )
    {
        char * pToken = strtok( pBuffer, " \t\n" );
        if ( pToken == NULL )
            continue;
        if ( pToken[0] == '#' )
            continue;
        // read gate
        pGate = Mio_LibraryReadGateByName( pLib, pToken, NULL );
        if ( pGate == NULL )
        {
            printf( "Cannot find gate \"%s\" in library \"%s\".\n", pToken, Mio_LibraryReadName(pLib) );
            continue;
        }
        // read profile
        pToken = strtok( NULL, " \t\n" );
        Mio_GateSetProfile( pGate, atoi(pToken) );
    }
}

void Mio_LibraryWriteProfile( FILE * pFile, Mio_Library_t * pLib )
{
    Mio_Gate_t * pGate;    
    Mio_LibraryForEachGate( pLib, pGate )
        if ( Mio_GateReadProfile(pGate) > 0 )
            fprintf( pFile, "%-24s  %6d\n", Mio_GateReadName(pGate), Mio_GateReadProfile(pGate) );
}

int Mio_LibraryHasProfile( Mio_Library_t * pLib )
{
    Mio_Gate_t * pGate;    
    Mio_LibraryForEachGate( pLib, pGate )
        if ( Mio_GateReadProfile(pGate) > 0 )
            return 1;
    return 0;
}


void Mio_LibraryTransferProfile( Mio_Library_t * pLibDst, Mio_Library_t * pLibSrc )
{
    Mio_Gate_t * pGateSrc, * pGateDst;    
    Mio_LibraryForEachGate( pLibDst, pGateDst )
        Mio_GateSetProfile( pGateDst, 0 );
    Mio_LibraryForEachGate( pLibSrc, pGateSrc )
        if ( Mio_GateReadProfile(pGateSrc) > 0 )
        {
            // find gate by name
            pGateDst = Mio_LibraryReadGateByName( pLibDst, Mio_GateReadName(pGateSrc), NULL );
            if ( pGateDst == NULL )
            {
                // find gate by function
                Mio_LibraryForEachGate( pLibDst, pGateDst )
                    if ( pGateDst->uTruth == pGateSrc->uTruth )
                        break;
                if ( pGateDst == NULL )
                {
                    printf( "Cannot find gate \"%s\" in library \"%s\".\n", Mio_GateReadName(pGateSrc), Mio_LibraryReadName(pLibDst) );
                    continue;
                }
            }
            Mio_GateAddToProfile( pGateDst, Mio_GateReadProfile(pGateSrc) );
        }
}
void Mio_LibraryTransferProfile2( Mio_Library_t * pLibDst, Mio_Library_t * pLibSrc )
{
    Mio_Gate_t * pGateSrc, * pGateDst;    
    Mio_LibraryForEachGate( pLibDst, pGateDst )
        Mio_GateSetProfile2( pGateDst, 0 );
    Mio_LibraryForEachGate( pLibSrc, pGateSrc )
        if ( Mio_GateReadProfile2(pGateSrc) > 0 )
        {
            // find gate by name
            pGateDst = Mio_LibraryReadGateByName( pLibDst, Mio_GateReadName(pGateSrc), NULL );
            if ( pGateDst == NULL )
            {
                // find gate by function
                Mio_LibraryForEachGate( pLibDst, pGateDst )
                    if ( pGateDst->uTruth == pGateSrc->uTruth )
                        break;
                if ( pGateDst == NULL )
                {
                    printf( "Cannot find gate \"%s\" in library \"%s\".\n", Mio_GateReadName(pGateSrc), Mio_LibraryReadName(pLibDst) );
                    continue;
                }
            }
            Mio_GateAddToProfile2( pGateDst, Mio_GateReadProfile2(pGateSrc) );
        }
}

void Mio_LibraryCleanProfile2( Mio_Library_t * pLib )
{
    Mio_Gate_t * pGate;    
    Mio_LibraryForEachGate( pLib, pGate )
        Mio_GateSetProfile2( pGate, 0 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mio_LibraryHashGates( Mio_Library_t * pLib )
{
    Mio_Gate_t * pGate;    
    Mio_LibraryForEachGate( pLib, pGate )
        if ( pGate->pTwin )
        {
            printf( "Gates with multiple outputs are not supported.\n" );
            return;
        }
    if ( pLib->tName2Gate )
        st__free_table( pLib->tName2Gate );
    pLib->tName2Gate = st__init_table(strcmp, st__strhash);
    Mio_LibraryForEachGate( pLib, pGate )
        st__insert( pLib->tName2Gate, pGate->pName, (char *)pGate );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_SclIsChar( char c )
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}
static inline int Abc_SclIsName( char c )
{
    return Abc_SclIsChar(c) || (c >= '0' && c <= '9');
}
static inline char * Abc_SclFindLimit( char * pName )
{
    assert( Abc_SclIsChar(*pName) );
    while ( Abc_SclIsName(*pName) )
        pName++;
    return pName;
}
static inline int Abc_SclAreEqual( char * pBase, char * pName, char * pLimit )
{
    return !strncmp( pBase, pName, pLimit - pName );
}
void Mio_LibraryShortFormula( Mio_Gate_t * pCell, char * pForm, char * pBuffer )
{
    Mio_Pin_t * pPin; 
    char * pTemp, * pLimit; int i;
    if ( !strncmp(pForm, "CONST", 5) )
    {
        sprintf( pBuffer, "%s", pForm );
        return;
    }
    for ( pTemp = pForm; *pTemp; )
    {
        if ( !Abc_SclIsChar(*pTemp) )
        {
            *pBuffer++ = *pTemp++;
            continue;
        }
        pLimit = Abc_SclFindLimit( pTemp );
        i = 0;
        Mio_GateForEachPin( pCell, pPin )
        {
            if ( Abc_SclAreEqual( pPin->pName, pTemp, pLimit ) )
            {
                *pBuffer++ = 'a' + i;
                break;
            }
            i++;
        }
        pTemp = pLimit;
    }
    *pBuffer++ = 0;
}
void Mio_LibraryShortNames( Mio_Library_t * pLib )
{
    char Buffer[10000];
    Mio_Gate_t * pGate; Mio_Pin_t * pPin;
    int c = 0, i, nDigits = Abc_Base10Log( Mio_LibraryReadGateNum(pLib) );
    // itereate through classes
    Mio_LibraryForEachGate( pLib, pGate )
    {
        ABC_FREE( pGate->pName );
        sprintf( Buffer, "g%0*d", nDigits, ++c );
        pGate->pName = Abc_UtilStrsav( Buffer );
        // update formula
        Mio_LibraryShortFormula( pGate, pGate->pForm, Buffer );
        ABC_FREE( pGate->pForm );
        pGate->pForm = Abc_UtilStrsav( Buffer );
        // pin names
        i = 0;
        Mio_GateForEachPin( pGate, pPin )
        {
            ABC_FREE( pPin->pName );
            sprintf( Buffer, "%c", 'a'+i );
            pPin->pName = Abc_UtilStrsav( Buffer );
            i++;
        }
        // output pin
        ABC_FREE( pGate->pOutName );
        sprintf( Buffer, "z" );
        pGate->pOutName = Abc_UtilStrsav( Buffer );
    }
    Mio_LibraryHashGates( pLib );
    // update library name
    printf( "Renaming library \"%s\" into \"%s%d\".\n", pLib->pName, "lib", Mio_LibraryReadGateNum(pLib) );
    ABC_FREE( pLib->pName );
    sprintf( Buffer, "lib%d", Mio_LibraryReadGateNum(pLib) );
    pLib->pName = Abc_UtilStrsav( Buffer );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mio_LibraryMatchesStop( Mio_Library_t * pLib )
{
    if ( !pLib->vTtMem )
        return;
    Vec_WecFree( pLib->vTt2Match );
    Vec_MemHashFree( pLib->vTtMem );
    Vec_MemFree( pLib->vTtMem );
    ABC_FREE( pLib->pCells );
}
void Mio_LibraryMatchesStart( Mio_Library_t * pLib, int fPinFilter, int fPinPerm, int fPinQuick )
{
    extern Mio_Cell2_t * Nf_StoDeriveMatches( Vec_Mem_t * vTtMem, Vec_Wec_t * vTt2Match, int * pnCells, int fPinFilter, int fPinPerm, int fPinQuick );
    if ( pLib->vTtMem && pLib->fPinFilter == fPinFilter && pLib->fPinPerm == fPinPerm && pLib->fPinQuick == fPinQuick )
        return;
    if ( pLib->vTtMem )
        Mio_LibraryMatchesStop( pLib );
    pLib->fPinFilter = fPinFilter;  // pin filtering
    pLib->fPinPerm   = fPinPerm;    // pin permutation
    pLib->fPinQuick  = fPinQuick;   // pin permutation
    pLib->vTtMem     = Vec_MemAllocForTT( 6, 0 );          
    pLib->vTt2Match  = Vec_WecAlloc( 1000 ); 
    Vec_WecPushLevel( pLib->vTt2Match );
    Vec_WecPushLevel( pLib->vTt2Match );
    assert( Vec_WecSize(pLib->vTt2Match) == Vec_MemEntryNum(pLib->vTtMem) );
    pLib->pCells = Nf_StoDeriveMatches( pLib->vTtMem, pLib->vTt2Match, &pLib->nCells, fPinFilter, fPinPerm, fPinQuick );
}
void Mio_LibraryMatchesFetch( Mio_Library_t * pLib, Vec_Mem_t ** pvTtMem, Vec_Wec_t ** pvTt2Match, Mio_Cell2_t ** ppCells, int * pnCells, int fPinFilter, int fPinPerm, int fPinQuick )
{
    Mio_LibraryMatchesStart( pLib, fPinFilter, fPinPerm, fPinQuick );
    *pvTtMem    = pLib->vTtMem;     // truth tables
    *pvTt2Match = pLib->vTt2Match;  // matches for truth tables
    *ppCells    = pLib->pCells;     // library gates
    *pnCells    = pLib->nCells;     // library gate count
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mio_LibraryMatches2Stop( Mio_Library_t * pLib )
{
    int i;
    if ( !pLib->vNames )
        return;
    Vec_PtrFree( pLib->vNames );
    Vec_WrdFree( pLib->vTruths );
    Vec_IntFree( pLib->vTt2Match4 );
    Vec_IntFree( pLib->vConfigs );
    for ( i = 0; i < 3; i++ )
    {
        Vec_MemHashFree( pLib->vTtMem2[i] );
        Vec_MemFree( pLib->vTtMem2[i] );
        Vec_IntFree( pLib->vTt2Match2[i] );
    }
}
void Mio_LibraryMatches2Start( Mio_Library_t * pLib )
{
    extern int Gia_ManDeriveMatches( Vec_Ptr_t ** pvNames, Vec_Wrd_t ** pvTruths, Vec_Int_t ** pvTt2Match4, Vec_Int_t ** pvConfigs, Vec_Mem_t * pvTtMem2[3], Vec_Int_t * pvTt2Match2[3] );
    if ( pLib->vNames )
        return;
    if ( pLib->vTtMem )
        Mio_LibraryMatches2Stop( pLib );
    Gia_ManDeriveMatches( &pLib->vNames, &pLib->vTruths, &pLib->vTt2Match4, &pLib->vConfigs, pLib->vTtMem2, pLib->vTt2Match2 );
}
void Mio_LibraryMatches2Fetch( Mio_Library_t * pLib, Vec_Ptr_t ** pvNames, Vec_Wrd_t ** pvTruths, Vec_Int_t ** pvTt2Match4, Vec_Int_t ** pvConfigs, Vec_Mem_t * pvTtMem2[3], Vec_Int_t * pvTt2Match2[3] )
{
    int i;
    Mio_LibraryMatches2Start( pLib );
    *pvNames     = pLib->vNames;    
    *pvTruths    = pLib->vTruths;    
    *pvTt2Match4 = pLib->vTt2Match4;    
    *pvConfigs   = pLib->vConfigs;    
    for ( i = 0; i < 3; i++ )
    {
        pvTtMem2[i]  = pLib->vTtMem2[i];    
        pvTt2Match2[i] = pLib->vTt2Match2[i];   
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

