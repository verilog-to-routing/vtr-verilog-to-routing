/**CFile****************************************************************

  FileName    [sclLibUtil.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Standard-cell library representation.]

  Synopsis    [Various library utilities.]

  Author      [Alan Mishchenko, Niklas Een]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 24, 2012.]

  Revision    [$Id: sclLibUtil.c,v 1.0 2012/08/24 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sclLib.h"
#include "misc/st/st.h"
#include "map/mio/mio.h"
#include "bool/kit/kit.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reading library from file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static unsigned Abc_SclHashString( char * pName, int TableSize ) 
{
    static int s_Primes[10] = { 1291, 1699, 2357, 4177, 5147, 5647, 6343, 7103, 7873, 8147 };
    unsigned i, Key = 0;
    for ( i = 0; pName[i] != '\0'; i++ )
        Key += s_Primes[i%10]*pName[i]*pName[i];
    return Key % TableSize;
}
int * Abc_SclHashLookup( SC_Lib * p, char * pName )
{
    int i;
    for ( i = Abc_SclHashString(pName, p->nBins); i < p->nBins; i = (i + 1) % p->nBins )
        if ( p->pBins[i] == -1 || !strcmp(pName, SC_LibCell(p, p->pBins[i])->pName) )
            return p->pBins + i;
    assert( 0 );
    return NULL;
}
void Abc_SclHashCells( SC_Lib * p )
{
    SC_Cell * pCell;
    int i, * pPlace;
    assert( p->nBins == 0 );
    p->nBins = Abc_PrimeCudd( 5 * SC_LibCellNum(p) );
    p->pBins = ABC_FALLOC( int, p->nBins );
    SC_LibForEachCell( p, pCell, i )
    {
        pPlace = Abc_SclHashLookup( p, pCell->pName );
        if ( *pPlace != -1 && pCell->pName )
            printf( "There are two standard cells with the same name (%s).\n", pCell->pName );
        assert( *pPlace == -1 );
        *pPlace = i;
    }
}
int Abc_SclCellFind( SC_Lib * p, char * pName )
{
    int *pPlace = Abc_SclHashLookup( p, pName );
    return pPlace ? *pPlace : -1;
}
int Abc_SclClassCellNum( SC_Cell * pClass )
{
    SC_Cell * pCell;
    int i, Count = 0;
    SC_RingForEachCell( pClass, pCell, i )
        if ( !pCell->fSkip )
            Count++;
    return Count;
}
int Abc_SclLibClassNum( SC_Lib * pLib )
{
    SC_Cell * pRepr;
    int i, Count = 0;
    SC_LibForEachCellClass( pLib, pRepr, i )
        Count++;
    return Count;
}

/**Function*************************************************************

  Synopsis    [Change cell names and pin names.]

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
void Abc_SclShortFormula( SC_Cell * pCell, char * pForm, char * pBuffer )
{
    SC_Pin * pPin; int i;
    char * pTemp, * pLimit;
    for ( pTemp = pForm; *pTemp; )
    {
        if ( !Abc_SclIsChar(*pTemp) )
        {
            *pBuffer++ = *pTemp++;
            continue;
        }
        pLimit = Abc_SclFindLimit( pTemp );
        SC_CellForEachPinIn( pCell, pPin, i )
            if ( Abc_SclAreEqual( pPin->pName, pTemp, pLimit ) )
            {
                *pBuffer++ = 'a' + i;
                break;
            }
        assert( i < pCell->n_inputs );
        pTemp = pLimit;
    }
    *pBuffer++ = 0;
}
static inline void Abc_SclTimingUpdate( SC_Cell * pCell, SC_Timing * p, char * Buffer )
{
    SC_Pin * pPin; int i;
    SC_CellForEachPinIn( pCell, pPin, i )
        if ( p->related_pin && !strcmp(p->related_pin, pPin->pName) )
        {
            ABC_FREE( p->related_pin );
            sprintf( Buffer, "%c", 'a'+i );
            p->related_pin = Abc_UtilStrsav( Buffer );
        }
}
static inline void Abc_SclTimingsUpdate( SC_Cell * pCell, SC_Timings * p, char * Buffer )
{
    SC_Timing * pTemp; int i;
    Vec_PtrForEachEntry( SC_Timing *, &p->vTimings, pTemp, i )
        Abc_SclTimingUpdate( pCell, pTemp, Buffer );
}
static inline void Abc_SclPinUpdate( SC_Cell * pCell, SC_Pin * p, char * Buffer )
{
    // update pin names in the timing
    SC_Timings * pTemp; int i;
    SC_PinForEachRTiming( p, pTemp, i )
    {
        SC_Pin * pPin; int k;
        Abc_SclTimingsUpdate( pCell, pTemp, Buffer );
        SC_CellForEachPinIn( pCell, pPin, k )
            if ( pTemp->pName && !strcmp(pTemp->pName, pPin->pName) )
            {
                ABC_FREE( pTemp->pName );
                sprintf( Buffer, "%c", 'a'+k );
                pTemp->pName = Abc_UtilStrsav( Buffer );
            }
    }
    // update formula
    Abc_SclShortFormula( pCell, p->func_text, Buffer );
    ABC_FREE( p->func_text );
    p->func_text = Abc_UtilStrsav( Buffer );
}
void Abc_SclShortNames( SC_Lib * p )
{
    char Buffer[10000];
    SC_Cell * pClass, * pCell; SC_Pin * pPin;
    int i, k, n, nClasses = Abc_SclLibClassNum(p);
    unsigned char nDigits = (unsigned char)Abc_Base10Log( nClasses );
    // itereate through classes
    SC_LibForEachCellClass( p, pClass, i )
    {
        unsigned char nDigits2 = (unsigned char)Abc_Base10Log( Abc_SclClassCellNum(pClass) );
        SC_RingForEachCell( pClass, pCell, k )
        {
            ABC_FREE( pCell->pName );
            sprintf( Buffer, "g%0*d_%0*d", nDigits, i, nDigits2, k );
            pCell->pName = Abc_UtilStrsav( Buffer );
            // formula
            SC_CellForEachPinOut( pCell, pPin, n )
                Abc_SclPinUpdate( pCell, pPin, Buffer );
            // pin names
            SC_CellForEachPinIn( pCell, pPin, n )
            {
                ABC_FREE( pPin->pName );
                sprintf( Buffer, "%c", (char)('a'+n) );
                pPin->pName = Abc_UtilStrsav( Buffer );
            }
            SC_CellForEachPinOut( pCell, pPin, n )
            {
                ABC_FREE( pPin->pName );
                sprintf( Buffer, "%c", (char)('z'-n+pCell->n_inputs) );
                pPin->pName = Abc_UtilStrsav( Buffer );
            }
        }
    }
    p->nBins = 0;
    ABC_FREE( p->pBins );
    Abc_SclHashCells( p );
    // update library name
    printf( "Renaming library \"%s\" into \"%s%d\".\n", p->pName, "lib", SC_LibCellNum(p) );
    ABC_FREE( p->pName );
    sprintf( Buffer, "lib%d", SC_LibCellNum(p) );
    p->pName = Abc_UtilStrsav( Buffer );
}
/**Function*************************************************************

  Synopsis    [Links equal gates into rings while sorting them by area.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Abc_SclCompareCells( SC_Cell ** pp1, SC_Cell ** pp2 )
{
    if ( (*pp1)->n_inputs < (*pp2)->n_inputs )
        return -1;
    if ( (*pp1)->n_inputs > (*pp2)->n_inputs )
        return 1;
//    if ( (*pp1)->area < (*pp2)->area )
//        return -1;
//    if ( (*pp1)->area > (*pp2)->area )
//        return 1;
    if ( SC_CellPinCapAve(*pp1) < SC_CellPinCapAve(*pp2) )
        return -1;
    if ( SC_CellPinCapAve(*pp1) > SC_CellPinCapAve(*pp2) )
        return 1;
    return strcmp( (*pp1)->pName, (*pp2)->pName );
}
void Abc_SclLinkCells( SC_Lib * p )
{
    Vec_Ptr_t * vList;
    SC_Cell * pCell, * pRepr = NULL;
    int i, k;
    assert( Vec_PtrSize(&p->vCellClasses) == 0 );
    SC_LibForEachCell( p, pCell, i )
    {
        // find gate with the same function
        SC_LibForEachCellClass( p, pRepr, k )
            if ( pCell->n_inputs  == pRepr->n_inputs && 
                 pCell->n_outputs == pRepr->n_outputs && 
                 Vec_WrdEqual(SC_CellFunc(pCell), SC_CellFunc(pRepr)) )
                break;
        if ( k == Vec_PtrSize(&p->vCellClasses) )
        {
            Vec_PtrPush( &p->vCellClasses, pCell );
            pCell->pNext = pCell->pPrev = pCell;
            continue;
        }
        // add it to the list before the cell
        pRepr->pPrev->pNext = pCell; pCell->pNext = pRepr;
        pCell->pPrev = pRepr->pPrev; pRepr->pPrev = pCell;
    }
    // sort cells by size then by name
    qsort( (void *)Vec_PtrArray(&p->vCellClasses), Vec_PtrSize(&p->vCellClasses), sizeof(void *), (int(*)(const void *,const void *))Abc_SclCompareCells );
    // sort cell lists
    vList = Vec_PtrAlloc( 100 );
    SC_LibForEachCellClass( p, pRepr, k )
    {
        Vec_PtrClear( vList );
        SC_RingForEachCell( pRepr, pCell, i )
            Vec_PtrPush( vList, pCell );
        qsort( (void *)Vec_PtrArray(vList), (size_t)Vec_PtrSize(vList), sizeof(void *), (int(*)(const void *,const void *))Abc_SclCompareCells );
        // create new representative
        pRepr = (SC_Cell *)Vec_PtrEntry( vList, 0 );
        pRepr->pNext = pRepr->pPrev = pRepr;
        pRepr->pRepr = pRepr;
        pRepr->pAve  = (SC_Cell *)Vec_PtrEntry( vList, Vec_PtrSize(vList)/2 );
        pRepr->Order = 0;
        pRepr->nGates = Vec_PtrSize(vList);
        // relink cells
        Vec_PtrForEachEntryStart( SC_Cell *, vList, pCell, i, 1 )
        {
            pRepr->pPrev->pNext = pCell; pCell->pNext = pRepr;
            pCell->pPrev = pRepr->pPrev; pRepr->pPrev = pCell;
            pCell->pRepr = pRepr;
            pCell->pAve  = (SC_Cell *)Vec_PtrEntry( vList, Vec_PtrSize(vList)/2 );
            pCell->Order = i;
            pCell->nGates = Vec_PtrSize(vList);
        }
        // update list
        Vec_PtrWriteEntry( &p->vCellClasses, k, pRepr );
    }
    Vec_PtrFree( vList );
}

/**Function*************************************************************

  Synopsis    [Returns the largest inverter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
SC_Cell * Abc_SclFindInvertor( SC_Lib * p, int fFindBuff )
{
    SC_Cell * pCell = NULL;
    word Truth = fFindBuff ? ABC_CONST(0xAAAAAAAAAAAAAAAA) : ABC_CONST(0x5555555555555555);
    int k;
    SC_LibForEachCellClass( p, pCell, k )
        if ( pCell->n_inputs == 1 && Vec_WrdEntry(&SC_CellPin(pCell, 1)->vFunc, 0) == Truth )
            break;
    // take representative
    return pCell ? pCell->pRepr : NULL;
}
SC_Cell * Abc_SclFindSmallestGate( SC_Cell * p, float CinMin )
{
    SC_Cell * pRes = NULL;
    int i;
    SC_RingForEachCell( p->pRepr, pRes, i )
        if ( SC_CellPinCapAve(pRes) > CinMin )
            return pRes;
    // take the largest gate
    return p->pRepr->pPrev;
}

/**Function*************************************************************

  Synopsis    [Returns the wireload model for the given area.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
SC_WireLoad * Abc_SclFetchWireLoadModel( SC_Lib * p, char * pWLoadUsed )
{
    SC_WireLoad * pWL = NULL;
    int i;
    // Get the actual table and reformat it for 'wire_cap' output:
    assert( pWLoadUsed != NULL );
    SC_LibForEachWireLoad( p, pWL, i )
        if ( !strcmp(pWL->pName, pWLoadUsed) )
            break;
    if ( i == Vec_PtrSize(&p->vWireLoads) )
    {
        Abc_Print( -1, "Cannot find wire load model \"%s\".\n", pWLoadUsed );
        exit(1);
    }
//    printf( "Using wireload model \"%s\".\n", pWL->pName );
    return pWL;
}
SC_WireLoad * Abc_SclFindWireLoadModel( SC_Lib * p, float Area )
{
    char * pWLoadUsed = NULL;
    int i;
    if ( p->default_wire_load_sel && strlen(p->default_wire_load_sel) )
    {
        SC_WireLoadSel * pWLS = NULL;
        SC_LibForEachWireLoadSel( p, pWLS, i )
            if ( !strcmp(pWLS->pName, p->default_wire_load_sel) )
                break;
        if ( i == Vec_PtrSize(&p->vWireLoadSels) )
        {
            Abc_Print( -1, "Cannot find wire load selection model \"%s\".\n", p->default_wire_load_sel );
            exit(1);
        }
        for ( i = 0; i < Vec_FltSize(&pWLS->vAreaFrom); i++)
            if ( Area >= Vec_FltEntry(&pWLS->vAreaFrom, i) && Area <  Vec_FltEntry(&pWLS->vAreaTo, i) )
            {
                pWLoadUsed = (char *)Vec_PtrEntry(&pWLS->vWireLoadModel, i);
                break;
            }
        if ( i == Vec_FltSize(&pWLS->vAreaFrom) )
            pWLoadUsed = (char *)Vec_PtrEntryLast(&pWLS->vWireLoadModel);
    }
    else if ( p->default_wire_load && strlen(p->default_wire_load) )
        pWLoadUsed = p->default_wire_load;
    else
    {
//        Abc_Print( 0, "No wire model given.\n" );
        return NULL;
    }
    return Abc_SclFetchWireLoadModel( p, pWLoadUsed );
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the library has delay info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_SclHasDelayInfo( void * pScl )
{
    SC_Lib * p = (SC_Lib *)pScl;
    SC_Cell * pCell;
    SC_Timing * pTime;
    pCell = Abc_SclFindInvertor(p, 0);
    if ( pCell == NULL )
        return 0;
    pTime = Scl_CellPinTime( pCell, 0 );
    if ( pTime == NULL )
        return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns "average" slew.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Abc_SclComputeAverageSlew( SC_Lib * p )
{
    SC_Cell * pCell;
    SC_Timing * pTime;
    Vec_Flt_t * vIndex;
    pCell = Abc_SclFindInvertor(p, 0);
    if ( pCell == NULL )
        return 0;
    pTime = Scl_CellPinTime( pCell, 0 );
    if ( pTime == NULL )
        return 0;
    vIndex = &pTime->pCellRise.vIndex0; // slew
    return Vec_FltEntry( vIndex, Vec_FltSize(vIndex)/3 );
}

/**Function*************************************************************

  Synopsis    [Compute delay parameters of pin/cell/class.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_SclComputeParametersPin( SC_Lib * p, SC_Cell * pCell, int iPin, float Slew, float * pLD, float * pPD )
{
    SC_Pair Load0, Load1, Load2;
    SC_Pair ArrIn  = { 0.0, 0.0 };
    SC_Pair SlewIn = { Slew, Slew };
    SC_Pair ArrOut0 = { 0.0, 0.0 };
    SC_Pair ArrOut1 = { 0.0, 0.0 };
    SC_Pair ArrOut2 = { 0.0, 0.0 };
    SC_Pair SlewOut = { 0.0, 0.0 };
    SC_Timing * pTime = Scl_CellPinTime( pCell, iPin );
    Vec_Flt_t * vIndex = pTime ? &pTime->pCellRise.vIndex1 : NULL; // capacitance
    if ( vIndex == NULL )
        return 0;
    // handle constant table
    if ( Vec_FltSize(vIndex) == 1 )
    {
        *pLD = 0;
        *pPD = Vec_FltEntry( (Vec_Flt_t *)Vec_PtrEntry(&pTime->pCellRise.vData, 0), 0 );
        return 1;
    }
    // get load points
    Load0.rise = Load0.fall = 0.0;
    Load1.rise = Load1.fall = Vec_FltEntry( vIndex, 0 );
    Load2.rise = Load2.fall = Vec_FltEntry( vIndex, Vec_FltSize(vIndex) - 2 );
    // compute delay
    Scl_LibPinArrival( pTime, &ArrIn, &SlewIn, &Load0, &ArrOut0, &SlewOut );
    Scl_LibPinArrival( pTime, &ArrIn, &SlewIn, &Load1, &ArrOut1, &SlewOut );
    Scl_LibPinArrival( pTime, &ArrIn, &SlewIn, &Load2, &ArrOut2, &SlewOut );
    ArrOut0.rise = 0.5 * ArrOut0.rise + 0.5 * ArrOut0.fall;
    ArrOut1.rise = 0.5 * ArrOut1.rise + 0.5 * ArrOut1.fall;
    ArrOut2.rise = 0.5 * ArrOut2.rise + 0.5 * ArrOut2.fall;
    // get tangent
    *pLD = (ArrOut2.rise - ArrOut1.rise) / ((Load2.rise - Load1.rise) / SC_CellPinCap(pCell, iPin));
    // get constant
    *pPD = ArrOut0.rise;
    return 1;
}
int Abc_SclComputeParametersCell( SC_Lib * p, SC_Cell * pCell, float Slew, float * pLD, float * pPD )
{
    SC_Pin * pPin;
    float LD, PD, ld, pd;
    int i;
    LD = PD = ld = pd = 0;
    SC_CellForEachPinIn( pCell, pPin, i )
    {
        if ( !Abc_SclComputeParametersPin( p, pCell, i, Slew, &ld, &pd ) )
            return 0;
        LD += ld; PD += pd;
    }
    *pLD = LD / Abc_MaxInt(1, pCell->n_inputs);
    *pPD = PD / Abc_MaxInt(1, pCell->n_inputs);
    return 1;
}
void Abc_SclComputeParametersClass( SC_Lib * p, SC_Cell * pRepr, float Slew, float * pLD, float * pPD )
{
    SC_Cell * pCell;
    float LD, PD, ld, pd;
    int i, Count = 0;
    LD = PD = ld = pd = 0;
    SC_RingForEachCell( pRepr, pCell, i )
    {
        Abc_SclComputeParametersCell( p, pCell, Slew, &ld, &pd );
        LD += ld; PD += pd;
        Count++;
    }
    *pLD = LD / Abc_MaxInt(1, Count);
    *pPD = PD / Abc_MaxInt(1, Count);
}
void Abc_SclComputeParametersClassPin( SC_Lib * p, SC_Cell * pRepr, int iPin, float Slew, float * pLD, float * pPD )
{
    SC_Cell * pCell;
    float LD, PD, ld, pd;
    int i, Count = 0;
    LD = PD = ld = pd = 0;
    SC_RingForEachCell( pRepr, pCell, i )
    {
        Abc_SclComputeParametersPin( p, pCell, iPin, Slew, &ld, &pd );
        LD += ld; PD += pd;
        Count++;
    }
    *pLD = LD / Abc_MaxInt(1, Count);
    *pPD = PD / Abc_MaxInt(1, Count);
}
float Abc_SclComputeDelayCellPin( SC_Lib * p, SC_Cell * pCell, int iPin, float Slew, float Gain )
{
    float LD = 0, PD = 0;
    Abc_SclComputeParametersPin( p, pCell, iPin, Slew, &LD, &PD );
    return 0.01 * LD * Gain + PD;
}
float Abc_SclComputeDelayClassPin( SC_Lib * p, SC_Cell * pRepr, int iPin, float Slew, float Gain )
{
    SC_Cell * pCell;
    float Delay = 0;
    int i, Count = 0;
    SC_RingForEachCell( pRepr, pCell, i )
    {
        if ( pCell->fSkip ) 
            continue;
//        if ( pRepr == pCell ) // skip the first gate
//            continue;
        Delay += Abc_SclComputeDelayCellPin( p, pCell, iPin, Slew, Gain );
        Count++;
    }
    return Delay / Abc_MaxInt(1, Count);
}
float Abc_SclComputeAreaClass( SC_Cell * pRepr )
{
    SC_Cell * pCell;
    float Area = 0;
    int i, Count = 0;
    SC_RingForEachCell( pRepr, pCell, i )
    {
        if ( pCell->fSkip ) 
            continue;
        Area += pCell->area;
        Count++;
    }
    return Area / Abc_MaxInt(1, Count);
}

/**Function*************************************************************

  Synopsis    [Print cells]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_SclMarkSkippedCells( SC_Lib * p )
{
    char FileName[1000];
    char Buffer[1000], * pName;
    SC_Cell * pCell;
    FILE * pFile;
    int CellId, nSkipped = 0;
    sprintf( FileName, "%s.skip", p->pName );
    pFile = fopen( FileName, "rb" );
    if ( pFile == NULL )
        return;
    while ( fgets( Buffer, 999, pFile ) != NULL )
    {
        pName = strtok( Buffer, "\r\n\t " );
        if ( pName == NULL )
            continue;
        CellId = Abc_SclCellFind( p, pName );
        if ( CellId == -1 )
        {
            printf( "Cannot find cell \"%s\" in the library \"%s\".\n", pName, p->pName );
            continue;
        }
        pCell = SC_LibCell( p, CellId );
        pCell->fSkip = 1;
        nSkipped++;
    }
    fclose( pFile );
    printf( "Marked %d cells for skipping in the library \"%s\".\n", nSkipped, p->pName );
}
void Abc_SclPrintCells( SC_Lib * p, float SlewInit, float Gain, int fInvOnly, int fShort )
{
    SC_Cell * pCell, * pRepr;
    SC_Pin * pPin;
    int i, j, k, nLength = 0;
    float Slew = (SlewInit == 0) ? Abc_SclComputeAverageSlew(p) : SlewInit;
    float LD = 0, PD = 0;
    assert( Vec_PtrSize(&p->vCellClasses) > 0 );
    printf( "Library \"%s\" ", p->pName );
    printf( "has %d cells in %d classes.  ", 
        Vec_PtrSize(&p->vCells), Vec_PtrSize(&p->vCellClasses) );
    if ( !fShort )
        printf( "Delay estimate is based on slew %.2f ps and gain %.2f.", Slew, Gain );
    printf( "\n" );
    Abc_SclMarkSkippedCells( p );
    // find the longest name
    SC_LibForEachCellClass( p, pRepr, k )
        SC_RingForEachCell( pRepr, pCell, i )
            nLength = Abc_MaxInt( nLength, strlen(pRepr->pName) );
    // print cells
    SC_LibForEachCellClass( p, pRepr, k )
    {
        if ( fInvOnly && pRepr->n_inputs != 1 )
            continue;
        SC_CellForEachPinOut( pRepr, pPin, i )
        {
            if ( i == pRepr->n_inputs )
            {
                printf( "Class%4d : ",   k );
                printf( "Cells =%3d   ", Abc_SclClassCellNum(pRepr) );
                printf( "Ins =%2d  ",    pRepr->n_inputs );
                printf( "Outs =%2d  ",   pRepr->n_outputs );
            }
            else
                printf( "                                            " );
            if ( pPin->func_text )
                printf( "%-30s", pPin->func_text  );
            printf( "    "  );
            Kit_DsdPrintFromTruth( (unsigned *)Vec_WrdArray(&pPin->vFunc), pRepr->n_inputs );
            printf( "\n" );
            if ( fShort )
                continue;
            SC_RingForEachCell( pRepr, pCell, j )
            {
                printf( "  %3d ",           j+1 );
                printf( "%s",               pCell->fSkip ? "s" : " " );
                printf( " : " );
                printf( "%-*s  ",           nLength, pCell->pName );
                printf( "%2d   ",           pCell->drive_strength );
                printf( "A =%8.2f  ",       pCell->area );
                printf( "L =%8.2f  ",       pCell->leakage );
                if ( pCell->n_outputs == 1 )
                {
                    if ( Abc_SclComputeParametersCell( p, pCell, Slew, &LD, &PD ) )
                    {
                        printf( "D =%6.1f ps  ",    0.01 * Gain * LD + PD );
                        printf( "LD =%6.1f ps  ",   LD );
                        printf( "PD =%6.1f ps    ", PD );
                        printf( "C =%5.1f ff  ",    SC_CellPinCapAve(pCell) );
                        printf( "Cm =%5.0f ff    ", SC_CellPin(pCell, pCell->n_inputs)->max_out_cap );
                        printf( "Sm =%5.1f ps ",    SC_CellPin(pCell, pCell->n_inputs)->max_out_slew );
                    }
                }
                printf( "\n" );
            }
            break;
        }
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_SclConvertLeakageIntoArea( SC_Lib * p, float A, float B )
{
    SC_Cell * pCell; int i;
    SC_LibForEachCell( p, pCell, i )
        pCell->area = A * pCell->area + B * pCell->leakage;
}


/**Function*************************************************************

  Synopsis    [Print cells]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_SclLibNormalizeSurface( SC_Surface * p, float Time, float Load )
{
    Vec_Flt_t * vArray;
    int i, k; float Entry;
    Vec_FltForEachEntry( &p->vIndex0, Entry, i ) // slew
        Vec_FltWriteEntry( &p->vIndex0, i, Time * Entry );
    Vec_FltForEachEntry( &p->vIndex1, Entry, i ) // load
        Vec_FltWriteEntry( &p->vIndex1, i, Load * Entry );
    Vec_PtrForEachEntry( Vec_Flt_t *, &p->vData, vArray, k )
        Vec_FltForEachEntry( vArray, Entry, i ) // delay/slew
            Vec_FltWriteEntry( vArray, i, Time * Entry );
}
void Abc_SclLibNormalize( SC_Lib * p )
{
    SC_WireLoad * pWL;
    SC_Cell * pCell;
    SC_Pin * pPin;
    SC_Timings * pTimings;
    SC_Timing * pTiming;
    int i, k, m, n;
    float Time = 1.0 * pow(10.0, 12 - p->unit_time);
    float Load = p->unit_cap_fst * pow(10.0, 15 - p->unit_cap_snd);
    if ( Time == 1 && Load == 1 )
        return;
    p->unit_time = 12;
    p->unit_cap_fst = 1;
    p->unit_cap_snd = 15;
    p->default_max_out_slew *= Time;
    SC_LibForEachWireLoad( p, pWL, i )
        pWL->cap *= Load;
    SC_LibForEachCell( p, pCell, i )
    SC_CellForEachPin( pCell, pPin, k )
    {
        pPin->cap *= Load;
        pPin->rise_cap *= Load;
        pPin->fall_cap *= Load;
        pPin->max_out_cap *= Load;
        pPin->max_out_slew *= Time;
        SC_PinForEachRTiming( pPin, pTimings, m )
        Vec_PtrForEachEntry( SC_Timing *, &pTimings->vTimings, pTiming, n )
        {
            Abc_SclLibNormalizeSurface( &pTiming->pCellRise, Time, Load );
            Abc_SclLibNormalizeSurface( &pTiming->pCellFall, Time, Load );
            Abc_SclLibNormalizeSurface( &pTiming->pRiseTrans, Time, Load );
            Abc_SclLibNormalizeSurface( &pTiming->pFallTrans, Time, Load );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Derives simple GENLIB library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Str_t * Abc_SclProduceGenlibStrSimple( SC_Lib * p )
{
    char Buffer[200];
    Vec_Str_t * vStr;
    SC_Cell * pCell;
    SC_Pin * pPin, * pPinOut;
    int i, j, k, Count = 2;
    // mark skipped cells
//    Abc_SclMarkSkippedCells( p );
    vStr = Vec_StrAlloc( 1000 );
    Vec_StrPrintStr( vStr, "GATE _const0_            0.00 z=CONST0;\n" );
    Vec_StrPrintStr( vStr, "GATE _const1_            0.00 z=CONST1;\n" );
    SC_LibForEachCell( p, pCell, i )
    {
        if ( pCell->n_inputs == 0 )
            continue;
        assert( strlen(pCell->pName) < 200 );
        SC_CellForEachPinOut( pCell, pPinOut, j )
        {
            Vec_StrPrintStr( vStr, "GATE " );
            sprintf( Buffer, "%-16s", pCell->pName );
            Vec_StrPrintStr( vStr, Buffer );
            Vec_StrPrintStr( vStr, " " );
            sprintf( Buffer, "%7.2f", pCell->area );
            Vec_StrPrintStr( vStr, Buffer );
            Vec_StrPrintStr( vStr, " " );
            Vec_StrPrintStr( vStr, pPinOut->pName );
            Vec_StrPrintStr( vStr, "=" );
            Vec_StrPrintStr( vStr, pPinOut->func_text ? pPinOut->func_text : "?" );
            Vec_StrPrintStr( vStr, ";\n" );
            SC_CellForEachPinIn( pCell, pPin, k )
            {
                Vec_StrPrintStr( vStr, "         PIN " );
                sprintf( Buffer, "%-4s", pPin->pName );
                Vec_StrPrintStr( vStr, Buffer );
                sprintf( Buffer, " UNKNOWN  1  999  1.00  0.00  1.00  0.00\n" );
                Vec_StrPrintStr( vStr, Buffer );
            }
            Count++;
        }
    }
    Vec_StrPrintStr( vStr, "\n.end\n" );
    Vec_StrPush( vStr, '\0' );
//    printf( "GENLIB library with %d gates is produced:\n", Count );
//    printf( "%s", Vec_StrArray(vStr) );
    return vStr;
}
Mio_Library_t * Abc_SclDeriveGenlibSimple( void * pScl )
{
    SC_Lib * p = (SC_Lib *)pScl;
    Vec_Str_t * vStr = Abc_SclProduceGenlibStrSimple( p );
    Mio_Library_t * pLib = Mio_LibraryRead( p->pFileName, Vec_StrArray(vStr), NULL, 0 );  
    Vec_StrFree( vStr );
    if ( pLib )
        printf( "Derived GENLIB library \"%s\" with %d gates.\n", p->pName, SC_LibCellNum(p) );
    else
        printf( "Reading library has filed.\n" );
    return pLib;
}


/**Function*************************************************************

  Synopsis    [Derive GENLIB library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Str_t * Abc_SclProduceGenlibStr( SC_Lib * p, float Slew, float Gain, int nGatesMin, int * pnCellCount )
{
    char Buffer[200];
    Vec_Str_t * vStr;
    SC_Cell * pRepr;
    SC_Pin * pPin;
    int i, k, Count = 2, nClassMax = 0;
    // find the largest number of cells in a class
    SC_LibForEachCellClass( p, pRepr, i )
        if ( pRepr->n_outputs == 1 )
            nClassMax = Abc_MaxInt( nClassMax, Abc_SclClassCellNum(pRepr) );
    // update the number
    if ( nGatesMin && nGatesMin >= nClassMax )
        nGatesMin = 0;
    // mark skipped cells
    Abc_SclMarkSkippedCells( p );
    vStr = Vec_StrAlloc( 1000 );
    Vec_StrPrintStr( vStr, "GATE _const0_            0.00 z=CONST0;\n" );
    Vec_StrPrintStr( vStr, "GATE _const1_            0.00 z=CONST1;\n" );
    SC_LibForEachCellClass( p, pRepr, i )
    {
        if ( pRepr->n_inputs == 0 )
            continue;
        if ( pRepr->n_outputs > 1 )
            continue;
        if ( nGatesMin && pRepr->n_inputs > 2 && Abc_SclClassCellNum(pRepr) < nGatesMin )
            continue;
        assert( strlen(pRepr->pName) < 200 );
        Vec_StrPrintStr( vStr, "GATE " );
        sprintf( Buffer, "%-16s", pRepr->pName );
        Vec_StrPrintStr( vStr, Buffer );
        Vec_StrPrintStr( vStr, " " );
//        sprintf( Buffer, "%7.2f", Abc_SclComputeAreaClass(pRepr) );
        sprintf( Buffer, "%7.2f", pRepr->area );
        Vec_StrPrintStr( vStr, Buffer );
        Vec_StrPrintStr( vStr, " " );
        Vec_StrPrintStr( vStr, SC_CellPinName(pRepr, pRepr->n_inputs) );
        Vec_StrPrintStr( vStr, "=" );
        Vec_StrPrintStr( vStr, SC_CellPinOutFunc(pRepr, 0) ? SC_CellPinOutFunc(pRepr, 0) : "?" );
        Vec_StrPrintStr( vStr, ";\n" );
        SC_CellForEachPinIn( pRepr, pPin, k )
        {
            float Delay = Abc_SclComputeDelayClassPin( p, pRepr, k, Slew, Gain );
            assert( Delay > 0 );
            Vec_StrPrintStr( vStr, "         PIN " );
            sprintf( Buffer, "%-4s", pPin->pName );
            Vec_StrPrintStr( vStr, Buffer );
            sprintf( Buffer, " UNKNOWN  1  999  %7.2f  0.00  %7.2f  0.00\n", Delay, Delay );
            Vec_StrPrintStr( vStr, Buffer );
        }
        Count++;
    }
    Vec_StrPrintStr( vStr, "\n.end\n" );
    Vec_StrPush( vStr, '\0' );
//    printf( "GENLIB library with %d gates is produced:\n", Count );
//    printf( "%s", Vec_StrArray(vStr) );
    if ( pnCellCount )
        *pnCellCount = Count;
    return vStr;
}
Vec_Str_t * Abc_SclProduceGenlibStrProfile( SC_Lib * p, Mio_Library_t * pLib, float Slew, float Gain, int nGatesMin, int * pnCellCount )
{
    char Buffer[200];
    Vec_Str_t * vStr;
    SC_Cell * pRepr;
    SC_Pin * pPin;
    int i, k, Count = 2, nClassMax = 0;
    // find the largest number of cells in a class
    SC_LibForEachCellClass( p, pRepr, i )
        if ( pRepr->n_outputs == 1 )
            nClassMax = Abc_MaxInt( nClassMax, Abc_SclClassCellNum(pRepr) );
    // update the number
    if ( nGatesMin && nGatesMin >= nClassMax )
        nGatesMin = 0;
    // mark skipped cells
    Abc_SclMarkSkippedCells( p );
    vStr = Vec_StrAlloc( 1000 );
    Vec_StrPrintStr( vStr, "GATE _const0_            0.00 z=CONST0;\n" );
    Vec_StrPrintStr( vStr, "GATE _const1_            0.00 z=CONST1;\n" );
    SC_LibForEachCell( p, pRepr, i )
    {
        if ( pRepr->n_inputs == 0 )
            continue;
        if ( pRepr->n_outputs > 1 )
            continue;
        if ( nGatesMin && pRepr->n_inputs > 2 && Abc_SclClassCellNum(pRepr) < nGatesMin )
            continue;
        // check if the gate is in the profile
        if ( pRepr->n_inputs > 1 )
        {
            Mio_Gate_t * pGate = Mio_LibraryReadGateByName( pLib, pRepr->pName, NULL );
            if ( pGate == NULL || Mio_GateReadProfile(pGate) == 0 )
                continue;
        }
        // process gate
        assert( strlen(pRepr->pName) < 200 );
        Vec_StrPrintStr( vStr, "GATE " );
        sprintf( Buffer, "%-16s", pRepr->pName );
        Vec_StrPrintStr( vStr, Buffer );
        Vec_StrPrintStr( vStr, " " );
//        sprintf( Buffer, "%7.2f", Abc_SclComputeAreaClass(pRepr) );
        sprintf( Buffer, "%7.2f", pRepr->area );
        Vec_StrPrintStr( vStr, Buffer );
        Vec_StrPrintStr( vStr, " " );
        Vec_StrPrintStr( vStr, SC_CellPinName(pRepr, pRepr->n_inputs) );
        Vec_StrPrintStr( vStr, "=" );
        Vec_StrPrintStr( vStr, SC_CellPinOutFunc(pRepr, 0) ? SC_CellPinOutFunc(pRepr, 0) : "?" );
        Vec_StrPrintStr( vStr, ";\n" );
        SC_CellForEachPinIn( pRepr, pPin, k )
        {
            float Delay = Abc_SclComputeDelayClassPin( p, pRepr, k, Slew, Gain );
            assert( Delay > 0 );
            Vec_StrPrintStr( vStr, "         PIN " );
            sprintf( Buffer, "%-4s", pPin->pName );
            Vec_StrPrintStr( vStr, Buffer );
            sprintf( Buffer, " UNKNOWN  1  999  %7.2f  0.00  %7.2f  0.00\n", Delay, Delay );
            Vec_StrPrintStr( vStr, Buffer );
        }
        Count++;
    }
    Vec_StrPrintStr( vStr, "\n.end\n" );
    Vec_StrPush( vStr, '\0' );
//    printf( "GENLIB library with %d gates is produced:\n", Count );
//    printf( "%s", Vec_StrArray(vStr) );
    if ( pnCellCount )
        *pnCellCount = Count;
    return vStr;
}
void Abc_SclDumpGenlib( char * pFileName, SC_Lib * p, float SlewInit, float Gain, int nGatesMin )
{
    int nCellCount = 0;
    char FileName[1000];
    float Slew = (SlewInit == 0) ? Abc_SclComputeAverageSlew(p) : SlewInit;
    Vec_Str_t * vStr;
    FILE * pFile;
    if ( pFileName == NULL )
        sprintf( FileName, "%s_s%03d_g%03d_m%d.genlib", p->pName, (int)Slew, (int)Gain, nGatesMin );
    else
        sprintf( FileName, "%s", pFileName );
    pFile = fopen( FileName, "wb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" for writing.\n", FileName );
        return;
    }
    vStr = Abc_SclProduceGenlibStr( p, Slew, Gain, nGatesMin, &nCellCount );
    fprintf( pFile, "%s", Vec_StrArray(vStr) );
    Vec_StrFree( vStr );
    fclose( pFile );
    printf( "Written GENLIB library with %d gates into file \"%s\".\n", nCellCount, FileName );
}
Mio_Library_t * Abc_SclDeriveGenlib( void * pScl, void * pMio, float SlewInit, float Gain, int nGatesMin, int fVerbose )
{
    int nCellCount = 0;
    SC_Lib * p = (SC_Lib *)pScl;
    float Slew = (SlewInit == 0) ? Abc_SclComputeAverageSlew(p) : SlewInit;
    Vec_Str_t * vStr;
    Mio_Library_t * pLib;
    if ( pMio == NULL )
        vStr = Abc_SclProduceGenlibStr( p, Slew, Gain, nGatesMin, &nCellCount );
    else
        vStr = Abc_SclProduceGenlibStrProfile( p, (Mio_Library_t *)pMio, Slew, Gain, nGatesMin, &nCellCount );
    pLib = Mio_LibraryRead( p->pFileName, Vec_StrArray(vStr), NULL, 0 );  
    Vec_StrFree( vStr );
    if ( !pLib )
        printf( "Reading library has filed.\n" );
    else if ( fVerbose )
        printf( "Derived GENLIB library \"%s\" with %d gates using slew %.2f ps and gain %.2f.\n", p->pName, nCellCount, Slew, Gain );
    return pLib;
}

/**Function*************************************************************

  Synopsis    [Install library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_SclInstallGenlib( void * pScl, float SlewInit, float Gain, int nGatesMin )
{
    SC_Lib * p = (SC_Lib *)pScl;
    Vec_Str_t * vStr, * vStr2;
    float Slew = (SlewInit == 0) ? Abc_SclComputeAverageSlew(p) : SlewInit;
    int RetValue, nGateCount = SC_LibCellNum(p);
    if ( Gain == 0 )
        vStr = Abc_SclProduceGenlibStrSimple(p);
    else
        vStr = Abc_SclProduceGenlibStr( p, Slew, Gain, nGatesMin, &nGateCount );
    vStr2 = Vec_StrDup( vStr );
    RetValue = Mio_UpdateGenlib2( vStr, vStr2, p->pName, 0 );
    Vec_StrFree( vStr );
    Vec_StrFree( vStr2 );
    if ( !RetValue )
        printf( "Reading library has filed.\n" );
    else if ( Gain != 0 )
        printf( "Derived GENLIB library \"%s\" with %d gates using slew %.2f ps and gain %.2f.\n", p->pName, nGateCount, Slew, Gain );
//    else
//        printf( "Derived unit-delay GENLIB library \"%s\" with %d gates.\n", p->pName, nGateCount );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

