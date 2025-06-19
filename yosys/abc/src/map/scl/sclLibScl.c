/**CFile****************************************************************

  FileName    [sclLibScl.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Standard-cell library representation.]

  Synopsis    [Liberty abstraction for delay-oriented mapping.]

  Author      [Alan Mishchenko, Niklas Een]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 24, 2012.]

  Revision    [$Id: sclLibScl.c,v 1.0 2012/08/24 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sclLib.h"
#include "misc/st/st.h"
#include "map/mio/mio.h"
#include "bool/kit/kit.h"
#include "misc/extra/extra.h"
#include "misc/util/utilNam.h"
#include "map/scl/sclCon.h"

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
static void Abc_SclReadSurfaceGenlib( SC_Surface * p )
{
    Vec_Flt_t * vVec;
    Vec_Int_t * vVecI;
    int i, j;

    Vec_FltPush( &p->vIndex0, 0 );
    Vec_IntPush( &p->vIndex0I, Scl_Flt2Int(0) );

    Vec_FltPush( &p->vIndex1, 0 );
    Vec_IntPush( &p->vIndex1I, Scl_Flt2Int(0) );

    for ( i = 0; i < Vec_FltSize(&p->vIndex0); i++ )
    {
        vVec = Vec_FltAlloc( Vec_FltSize(&p->vIndex1) );
        Vec_PtrPush( &p->vData, vVec );
        vVecI = Vec_IntAlloc( Vec_FltSize(&p->vIndex1) );
        Vec_PtrPush( &p->vDataI, vVecI );
        for ( j = 0; j < Vec_FltSize(&p->vIndex1); j++ )
        {
            Vec_FltPush( vVec, 1 );
            Vec_IntPush( vVecI, Scl_Flt2Int(1) );
        }
    }
}
static int Abc_SclReadLibraryGenlib( SC_Lib * p, Mio_Library_t * pLib )
{
    Mio_Gate_t * pGate;
    Mio_Pin_t * pGatePin;
    int j, k;

    // Read non-composite fields:
    p->pName                 = Abc_UtilStrsav( Mio_LibraryReadName(pLib) );
    p->default_wire_load     = 0;
    p->default_wire_load_sel = 0;
    p->default_max_out_slew  = 0;

    p->unit_time             = 12;
    p->unit_cap_fst          = 1.0;
    p->unit_cap_snd          = 15;

    Mio_LibraryForEachGate( pLib, pGate )
    {
        SC_Cell * pCell = Abc_SclCellAlloc();
        pCell->Id = SC_LibCellNum(p);
        Vec_PtrPush( &p->vCells, pCell );

        pCell->pName          = Abc_UtilStrsav( Mio_GateReadName( pGate ) );     
        pCell->area           = Mio_GateReadArea( pGate );
        pCell->leakage        = 0;
        pCell->drive_strength = 0;

        pCell->n_inputs       = Mio_GateReadPinNum( pGate );    
        pCell->n_outputs      = 1;

        pCell->areaI          = Scl_Flt2Int(pCell->area);
        pCell->leakageI       = Scl_Flt2Int(pCell->leakage);

        Mio_GateForEachPin( pGate, pGatePin )
        {
            SC_Pin * pPin = Abc_SclPinAlloc();
            Vec_PtrPush( &pCell->vPins, pPin );

            pPin->dir      = sc_dir_Input;
            pPin->pName    = Abc_UtilStrsav( Mio_PinReadName( pGatePin ) ); 
            pPin->rise_cap = 0;
            pPin->fall_cap = 0;

            pPin->rise_capI = Scl_Flt2Int(pPin->rise_cap);
            pPin->fall_capI = Scl_Flt2Int(pPin->fall_cap);
        }

        for ( j = 0; j < pCell->n_outputs; j++ )
        {
            word * pTruth = Mio_GateReadTruthP( pGate );
            SC_Pin * pPin = Abc_SclPinAlloc();
            Vec_PtrPush( &pCell->vPins, pPin );

            pPin->dir          = sc_dir_Output;
            pPin->pName        = Abc_UtilStrsav( Mio_GateReadOutName( pGate ) );
            pPin->max_out_cap  = 0;
            pPin->max_out_slew = 0;

            // read function
            pPin->func_text = Abc_UtilStrsav( Mio_GateReadForm( pGate ) );
            Vec_WrdGrow( &pPin->vFunc, Abc_Truth6WordNum(pCell->n_inputs) );
            for ( k = 0; k < Vec_WrdCap(&pPin->vFunc); k++ )
                Vec_WrdPush( &pPin->vFunc, pTruth[k] );

            // read pins
            Mio_GateForEachPin( pGate, pGatePin )
            {
                Mio_PinPhase_t Value = Mio_PinReadPhase( pGatePin );
                SC_Timings * pRTime = Abc_SclTimingsAlloc();
                Vec_PtrPush( &pPin->vRTimings, pRTime );
                pRTime->pName = Abc_UtilStrsav( Mio_PinReadName( pGatePin ) ); 
                if ( 1 )
                {
                    SC_Timing * pTime = Abc_SclTimingAlloc();
                    Vec_PtrPush( &pRTime->vTimings, pTime );
                    if ( Value == MIO_PHASE_UNKNOWN )
                        pTime->tsense = (SC_TSense)sc_ts_Non;
                    else if ( Value == MIO_PHASE_INV )
                        pTime->tsense = (SC_TSense)sc_ts_Neg;
                    else if ( Value == MIO_PHASE_NONINV )
                        pTime->tsense = (SC_TSense)sc_ts_Pos;
                    else assert( 0 );
                    Abc_SclReadSurfaceGenlib( &pTime->pCellRise );
                    Abc_SclReadSurfaceGenlib( &pTime->pCellFall );
                    Abc_SclReadSurfaceGenlib( &pTime->pRiseTrans );
                    Abc_SclReadSurfaceGenlib( &pTime->pFallTrans );
                }
            }
        }
    }
    return 1;
}
SC_Lib * Abc_SclReadFromGenlib( void * pLib0 )
{
    Mio_Library_t * pLib = (Mio_Library_t *)pLib0;
    SC_Lib * p = Abc_SclLibAlloc();
    if ( !Abc_SclReadLibraryGenlib( p, pLib ) )
        return NULL;
    // hash gates by name
    Abc_SclHashCells( p );
    Abc_SclLinkCells( p );
    return p;
}


/**Function*************************************************************

  Synopsis    [Reading library from file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Abc_SclReadSurface( Vec_Str_t * vOut, int * pPos, SC_Surface * p )
{
    Vec_Flt_t * vVec;
    Vec_Int_t * vVecI;
    int i, j;

    for ( i = Vec_StrGetI(vOut, pPos); i != 0; i-- )
    {
        float Num = Vec_StrGetF(vOut, pPos);
        Vec_FltPush( &p->vIndex0, Num );
        Vec_IntPush( &p->vIndex0I, Scl_Flt2Int(Num) );
    }

    for ( i = Vec_StrGetI(vOut, pPos); i != 0; i-- )
    {
        float Num = Vec_StrGetF(vOut, pPos);
        Vec_FltPush( &p->vIndex1, Num );
        Vec_IntPush( &p->vIndex1I, Scl_Flt2Int(Num) );
    }

    for ( i = 0; i < Vec_FltSize(&p->vIndex0); i++ )
    {
        vVec = Vec_FltAlloc( Vec_FltSize(&p->vIndex1) );
        Vec_PtrPush( &p->vData, vVec );
        vVecI = Vec_IntAlloc( Vec_FltSize(&p->vIndex1) );
        Vec_PtrPush( &p->vDataI, vVecI );
        for ( j = 0; j < Vec_FltSize(&p->vIndex1); j++ )
        {
            float Num = Vec_StrGetF(vOut, pPos);
            Vec_FltPush( vVec, Num );
            Vec_IntPush( vVecI, Scl_Flt2Int(Num) );
        }
    }

    for ( i = 0; i < 3; i++ ) 
        p->approx[0][i] = Vec_StrGetF( vOut, pPos );
    for ( i = 0; i < 4; i++ ) 
        p->approx[1][i] = Vec_StrGetF( vOut, pPos );
    for ( i = 0; i < 6; i++ ) 
        p->approx[2][i] = Vec_StrGetF( vOut, pPos );
}
static int Abc_SclReadLibrary( Vec_Str_t * vOut, int * pPos, SC_Lib * p )
{
    int i, j, k, n;
    int version = Vec_StrGetI( vOut, pPos );
    if ( version != ABC_SCL_CUR_VERSION )
    { 
        Abc_Print( -1, "Wrong version of the SCL file.\n" ); 
        return 0; 
    }
    assert( version == ABC_SCL_CUR_VERSION ); // wrong version of the file

    // Read non-composite fields:
    p->pName                 = Vec_StrGetS(vOut, pPos);
    p->default_wire_load     = Vec_StrGetS(vOut, pPos);
    p->default_wire_load_sel = Vec_StrGetS(vOut, pPos);
    p->default_max_out_slew  = Vec_StrGetF(vOut, pPos);

    p->unit_time             = Vec_StrGetI(vOut, pPos);
    p->unit_cap_fst          = Vec_StrGetF(vOut, pPos);
    p->unit_cap_snd          = Vec_StrGetI(vOut, pPos);

    // Read 'wire_load' vector:
    for ( i = Vec_StrGetI(vOut, pPos); i != 0; i-- )
    {
        SC_WireLoad * pWL = Abc_SclWireLoadAlloc();
        Vec_PtrPush( &p->vWireLoads, pWL );

        pWL->pName = Vec_StrGetS(vOut, pPos);
        pWL->cap   = Vec_StrGetF(vOut, pPos);
        pWL->slope = Vec_StrGetF(vOut, pPos);

        for ( j = Vec_StrGetI(vOut, pPos); j != 0; j-- )
        {
            Vec_IntPush( &pWL->vFanout, Vec_StrGetI(vOut, pPos) );
            Vec_FltPush( &pWL->vLen,    Vec_StrGetF(vOut, pPos) );
        }
    }

    // Read 'wire_load_sel' vector:
    for ( i = Vec_StrGetI(vOut, pPos); i != 0; i-- )
    {
        SC_WireLoadSel * pWLS = Abc_SclWireLoadSelAlloc();
        Vec_PtrPush( &p->vWireLoadSels, pWLS );

        pWLS->pName = Vec_StrGetS(vOut, pPos);
        for ( j = Vec_StrGetI(vOut, pPos); j != 0; j-- )
        {
            Vec_FltPush( &pWLS->vAreaFrom,      Vec_StrGetF(vOut, pPos) );
            Vec_FltPush( &pWLS->vAreaTo,        Vec_StrGetF(vOut, pPos) );
            Vec_PtrPush( &pWLS->vWireLoadModel, Vec_StrGetS(vOut, pPos) );
        }
    }

    for ( i = Vec_StrGetI(vOut, pPos); i != 0; i-- )
    {
        SC_Cell * pCell = Abc_SclCellAlloc();
        pCell->Id = SC_LibCellNum(p);
        Vec_PtrPush( &p->vCells, pCell );

        pCell->pName          = Vec_StrGetS(vOut, pPos);     
        pCell->area           = Vec_StrGetF(vOut, pPos);
        pCell->leakage        = Vec_StrGetF(vOut, pPos);
        pCell->drive_strength = Vec_StrGetI(vOut, pPos);

        pCell->n_inputs       = Vec_StrGetI(vOut, pPos);
        pCell->n_outputs      = Vec_StrGetI(vOut, pPos);

        pCell->areaI          = Scl_Flt2Int(pCell->area);
        pCell->leakageI       = Scl_Flt2Int(pCell->leakage);
/*
        printf( "%s\n", pCell->pName );
        if ( !strcmp( "XOR3_X4M_A9TL", pCell->pName ) )
        {
            int s = 0;
        }
*/
        for ( j = 0; j < pCell->n_inputs; j++ )
        {
            SC_Pin * pPin = Abc_SclPinAlloc();
            Vec_PtrPush( &pCell->vPins, pPin );

            pPin->dir      = sc_dir_Input;
            pPin->pName    = Vec_StrGetS(vOut, pPos); 
            pPin->rise_cap = Vec_StrGetF(vOut, pPos);
            pPin->fall_cap = Vec_StrGetF(vOut, pPos);

            pPin->rise_capI = Scl_Flt2Int(pPin->rise_cap);
            pPin->fall_capI = Scl_Flt2Int(pPin->fall_cap);
        }

        for ( j = 0; j < pCell->n_outputs; j++ )
        {
            SC_Pin * pPin = Abc_SclPinAlloc();
            Vec_PtrPush( &pCell->vPins, pPin );

            pPin->dir          = sc_dir_Output;
            pPin->pName        = Vec_StrGetS(vOut, pPos); 
            pPin->max_out_cap  = Vec_StrGetF(vOut, pPos);
            pPin->max_out_slew = Vec_StrGetF(vOut, pPos);

            k = Vec_StrGetI(vOut, pPos);
            assert( k == pCell->n_inputs );

            // read function
            // (possibly empty) formula is always given
            assert( version == ABC_SCL_CUR_VERSION );
            assert( pPin->func_text == NULL );
            pPin->func_text = Vec_StrGetS(vOut, pPos); 
            if ( pPin->func_text[0] == 0 )
            {
                // formula is not given - read truth table
                ABC_FREE( pPin->func_text );
                assert( Vec_WrdSize(&pPin->vFunc) == 0 );
                Vec_WrdGrow( &pPin->vFunc, Abc_Truth6WordNum(pCell->n_inputs) );
                for ( k = 0; k < Vec_WrdCap(&pPin->vFunc); k++ )
                    Vec_WrdPush( &pPin->vFunc, Vec_StrGetW(vOut, pPos) );
            }
            else
            {
                // formula is given - derive truth table
                SC_Pin * pPin2;
                Vec_Ptr_t * vNames;
                Vec_Wrd_t * vFunc;
                // collect input names
                vNames = Vec_PtrAlloc( pCell->n_inputs );
                SC_CellForEachPinIn( pCell, pPin2, n )
                    Vec_PtrPush( vNames, pPin2->pName );
                // derive truth table
                assert( Vec_WrdSize(&pPin->vFunc) == 0 );
                Vec_WrdErase( &pPin->vFunc );
                vFunc = Mio_ParseFormulaTruth( pPin->func_text, (char **)Vec_PtrArray(vNames), pCell->n_inputs );
                pPin->vFunc = *vFunc;
                ABC_FREE( vFunc );
                Vec_PtrFree( vNames );
                // skip truth table
                assert( Vec_WrdSize(&pPin->vFunc) == Abc_Truth6WordNum(pCell->n_inputs) );
                for ( k = 0; k < Vec_WrdSize(&pPin->vFunc); k++ )
                {
                    word Value = Vec_StrGetW(vOut, pPos);
                    assert( Value == Vec_WrdEntry(&pPin->vFunc, k) );
                }
            }

            // Read 'rtiming': (pin-to-pin timing tables for this particular output)
            for ( k = 0; k < pCell->n_inputs; k++ )
            {
                SC_Timings * pRTime = Abc_SclTimingsAlloc();
                Vec_PtrPush( &pPin->vRTimings, pRTime );

                pRTime->pName = Vec_StrGetS(vOut, pPos);
                n = Vec_StrGetI(vOut, pPos); assert( n <= 1 );
                if ( n == 1 )
                {
                    SC_Timing * pTime = Abc_SclTimingAlloc();
                    Vec_PtrPush( &pRTime->vTimings, pTime );

                    pTime->tsense = (SC_TSense)Vec_StrGetI(vOut, pPos);
                    Abc_SclReadSurface( vOut, pPos, &pTime->pCellRise );
                    Abc_SclReadSurface( vOut, pPos, &pTime->pCellFall );
                    Abc_SclReadSurface( vOut, pPos, &pTime->pRiseTrans );
                    Abc_SclReadSurface( vOut, pPos, &pTime->pFallTrans );
                }
                else
                    assert( Vec_PtrSize(&pRTime->vTimings) == 0 );
            }
        }
    }
    return 1;
}
SC_Lib * Abc_SclReadFromStr( Vec_Str_t * vOut )
{
    SC_Lib * p;
    int Pos = 0;
    // read the library
    p = Abc_SclLibAlloc();
    if ( !Abc_SclReadLibrary( vOut, &Pos, p ) )
        return NULL;
    assert( Pos == Vec_StrSize(vOut) );
    // hash gates by name
    Abc_SclHashCells( p );
    Abc_SclLinkCells( p );
    return p;
}
SC_Lib * Abc_SclReadFromFile( char * pFileName )
{
    SC_Lib * p;
    FILE * pFile;
    Vec_Str_t * vOut;
    int nFileSize;
    pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" for reading.\n", pFileName );
        return NULL;
    }
    // get the file size, in bytes
    fseek( pFile, 0, SEEK_END );  
    nFileSize = ftell( pFile );  
    rewind( pFile ); 
    // load the contents
    vOut = Vec_StrAlloc( nFileSize );
    vOut->nSize = vOut->nCap;
    assert( nFileSize == Vec_StrSize(vOut) );
    nFileSize = fread( Vec_StrArray(vOut), 1, Vec_StrSize(vOut), pFile );
    assert( nFileSize == Vec_StrSize(vOut) );
    fclose( pFile );
    // read the library
    p = Abc_SclReadFromStr( vOut );
    if ( p != NULL )
        p->pFileName = Abc_UtilStrsav( pFileName );
    if ( p != NULL )
        Abc_SclLibNormalize( p );
    Vec_StrFree( vOut );
    return p;
}

/**Function*************************************************************

  Synopsis    [Writing library into file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Abc_SclWriteSurface( Vec_Str_t * vOut, SC_Surface * p )
{
    Vec_Flt_t * vVec;
    float Entry;
    int i, k;

    Vec_StrPutI( vOut, Vec_FltSize(&p->vIndex0) );
    Vec_FltForEachEntry( &p->vIndex0, Entry, i )
        Vec_StrPutF( vOut, Entry );

    Vec_StrPutI( vOut, Vec_FltSize(&p->vIndex1) );
    Vec_FltForEachEntry( &p->vIndex1, Entry, i )
        Vec_StrPutF( vOut, Entry );

    Vec_PtrForEachEntry( Vec_Flt_t *, &p->vData, vVec, i )
        Vec_FltForEachEntry( vVec, Entry, k )
            Vec_StrPutF( vOut, Entry );

    for ( i = 0; i < 3; i++ ) 
        Vec_StrPutF( vOut, p->approx[0][i] );
    for ( i = 0; i < 4; i++ ) 
        Vec_StrPutF( vOut, p->approx[1][i] );
    for ( i = 0; i < 6; i++ ) 
        Vec_StrPutF( vOut, p->approx[2][i] );
}
static void Abc_SclWriteLibraryCellsOnly( Vec_Str_t * vOut, SC_Lib * p, int fAddOn )
{
    SC_Cell * pCell;
    SC_Pin * pPin;
    int i, j, k;    
    SC_LibForEachCell( p, pCell, i )
    {
        if ( pCell->seq || pCell->unsupp )
            continue;

        if ( fAddOn ) {
            Vec_StrPush( vOut, 'L' );
            Vec_StrPush( vOut, '0'+(char)fAddOn );
            Vec_StrPush( vOut, '_' );
        }

        Vec_StrPutS( vOut, pCell->pName );
        Vec_StrPutF( vOut, pCell->area );
        Vec_StrPutF( vOut, pCell->leakage );
        Vec_StrPutI( vOut, pCell->drive_strength );

        // Write 'pins': (sorted at this point; first inputs, then outputs)
        Vec_StrPutI( vOut, pCell->n_inputs);
        Vec_StrPutI( vOut, pCell->n_outputs);

        SC_CellForEachPinIn( pCell, pPin, j )
        {
            assert(pPin->dir == sc_dir_Input);
            Vec_StrPutS( vOut, pPin->pName );
            Vec_StrPutF( vOut, pPin->rise_cap );
            Vec_StrPutF( vOut, pPin->fall_cap );
        }

        SC_CellForEachPinOut( pCell, pPin, j )
        {
            SC_Timings * pRTime;
            word uWord;

            assert(pPin->dir == sc_dir_Output);
            Vec_StrPutS( vOut, pPin->pName );
            Vec_StrPutF( vOut, pPin->max_out_cap );
            Vec_StrPutF( vOut, pPin->max_out_slew );
            Vec_StrPutI( vOut, pCell->n_inputs );

            // write function
            Vec_StrPutS( vOut, pPin->func_text ? pPin->func_text : (char *)"" );

            // write truth table
            assert( Vec_WrdSize(&pPin->vFunc) == Abc_Truth6WordNum(pCell->n_inputs) );
            Vec_WrdForEachEntry( &pPin->vFunc, uWord, k ) // -- 'size = 1u << (n_vars - 6)'
                Vec_StrPutW( vOut, uWord );  // -- 64-bit number, written uncompressed (low-byte first)

            // Write 'rtiming': (pin-to-pin timing tables for this particular output)
            assert( Vec_PtrSize(&pPin->vRTimings) == pCell->n_inputs );
            SC_PinForEachRTiming( pPin, pRTime, k )
            {
                Vec_StrPutS( vOut, pRTime->pName );
                Vec_StrPutI( vOut, Vec_PtrSize(&pRTime->vTimings) );
                    // -- NOTE! After post-processing, the size of the 'rtiming[k]' vector is either
                    // 0 or 1 (in static timing, we have merged all tables to get the worst case).
                    // The case with size 0 should only occur for multi-output gates.
                if ( Vec_PtrSize(&pRTime->vTimings) == 1 )
                {
                    SC_Timing * pTime = (SC_Timing *)Vec_PtrEntry( &pRTime->vTimings, 0 );
                        // -- NOTE! We don't need to save 'related_pin' string because we have sorted 
                        // the elements on input pins.
                    Vec_StrPutI( vOut, (int)pTime->tsense);
                    Abc_SclWriteSurface( vOut, &pTime->pCellRise );
                    Abc_SclWriteSurface( vOut, &pTime->pCellFall );
                    Abc_SclWriteSurface( vOut, &pTime->pRiseTrans );
                    Abc_SclWriteSurface( vOut, &pTime->pFallTrans );
                }
                else
                    assert( Vec_PtrSize(&pRTime->vTimings) == 0 );
            }
        }
    }    
}
int Abc_SclCountValidCells( SC_Lib * p )
{
    SC_Cell * pCell;    
    int i, n_valid_cells = 0;
    SC_LibForEachCell( p, pCell, i )
        if ( !(pCell->seq || pCell->unsupp) )
            n_valid_cells++;
    return n_valid_cells;
}
static void Abc_SclWriteLibrary( Vec_Str_t * vOut, SC_Lib * p, int nExtra, int fUsePrefix )
{
    SC_WireLoad * pWL;
    SC_WireLoadSel * pWLS;
    int n_valid_cells;
    int i, j;

    Vec_StrPutI( vOut, ABC_SCL_CUR_VERSION );

    // Write non-composite fields:
    Vec_StrPutS( vOut, p->pName );
    Vec_StrPutS( vOut, p->default_wire_load );
    Vec_StrPutS( vOut, p->default_wire_load_sel );
    Vec_StrPutF( vOut, p->default_max_out_slew );

    assert( p->unit_time >= 0 );
    assert( p->unit_cap_snd >= 0 );
    Vec_StrPutI( vOut, p->unit_time );
    Vec_StrPutF( vOut, p->unit_cap_fst );
    Vec_StrPutI( vOut, p->unit_cap_snd );

    // Write 'wire_load' vector:
    Vec_StrPutI( vOut, Vec_PtrSize(&p->vWireLoads) );
    SC_LibForEachWireLoad( p, pWL, i )
    {
        Vec_StrPutS( vOut, pWL->pName );
        Vec_StrPutF( vOut, pWL->cap );
        Vec_StrPutF( vOut, pWL->slope );

        Vec_StrPutI( vOut, Vec_IntSize(&pWL->vFanout) );
        for ( j = 0; j < Vec_IntSize(&pWL->vFanout); j++ )
        {
            Vec_StrPutI( vOut, Vec_IntEntry(&pWL->vFanout, j) );
            Vec_StrPutF( vOut, Vec_FltEntry(&pWL->vLen, j) );
        }
    }

    // Write 'wire_load_sel' vector:
    Vec_StrPutI( vOut, Vec_PtrSize(&p->vWireLoadSels) );
    SC_LibForEachWireLoadSel( p, pWLS, i )
    {
        Vec_StrPutS( vOut, pWLS->pName );
        Vec_StrPutI( vOut, Vec_FltSize(&pWLS->vAreaFrom) );
        for ( j = 0; j < Vec_FltSize(&pWLS->vAreaFrom); j++)
        {
            Vec_StrPutF( vOut, Vec_FltEntry(&pWLS->vAreaFrom, j) );
            Vec_StrPutF( vOut, Vec_FltEntry(&pWLS->vAreaTo, j) );
            Vec_StrPutS( vOut, (char *)Vec_PtrEntry(&pWLS->vWireLoadModel, j) );
        }
    }

    // Write 'cells' vector:
    n_valid_cells = Abc_SclCountValidCells( p );
    Vec_StrPutI( vOut, n_valid_cells + nExtra );
    Abc_SclWriteLibraryCellsOnly( vOut, p, fUsePrefix ? 1 : 0 );
}
void Abc_SclWriteScl( char * pFileName, SC_Lib * p )
{
    Vec_Str_t * vOut;
    vOut = Vec_StrAlloc( 10000 );
    Abc_SclWriteLibrary( vOut, p, 0, 0 );
    if ( Vec_StrSize(vOut) > 0 )
    {
        FILE * pFile = fopen( pFileName, "wb" );
        if ( pFile == NULL )
            printf( "Cannot open file \"%s\" for writing.\n", pFileName );
        else
        {
            fwrite( Vec_StrArray(vOut), 1, Vec_StrSize(vOut), pFile );
            fclose( pFile );
        }
    }
    Vec_StrFree( vOut );    
}


/**Function*************************************************************

  Synopsis    [Writing library into text file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Abc_SclWriteSurfaceText( FILE * s, SC_Surface * p )
{
    Vec_Flt_t * vVec;
    float Entry;
    int i, k;

    fprintf( s, "          index_1(\"" );  
    Vec_FltForEachEntry( &p->vIndex0, Entry, i )
        fprintf( s, "%f%s", Entry, i == Vec_FltSize(&p->vIndex0)-1 ? "":", " );
    fprintf( s, "\");\n" );

    fprintf( s, "          index_2(\"" );  
    Vec_FltForEachEntry( &p->vIndex1, Entry, i )
        fprintf( s, "%f%s", Entry, i == Vec_FltSize(&p->vIndex1)-1 ? "":", " );
    fprintf( s, "\");\n" );

    fprintf( s, "          values (\"" );  
    Vec_PtrForEachEntry( Vec_Flt_t *, &p->vData, vVec, i )
    {
        Vec_FltForEachEntry( vVec, Entry, k )
            fprintf( s, "%f%s", Entry, i == Vec_PtrSize(&p->vData)-1 && k == Vec_FltSize(vVec)-1 ? "\");":", " );
        if ( i == Vec_PtrSize(&p->vData)-1 )
            fprintf( s, "\n" );
        else
        {
            fprintf( s, "\\\n" );
            fprintf( s, "                   " );  
        }
    }
/*
    fprintf( s, "       approximations: \n" );
    fprintf( s, "       " );
    for ( i = 0; i < 3; i++ ) 
        fprintf( s, "%f ", p->approx[0][i] );
    fprintf( s, "\n" );
    fprintf( s, "       " );
    for ( i = 0; i < 4; i++ ) 
        fprintf( s, "%f ", p->approx[1][i] );
    fprintf( s, "\n" );
    fprintf( s, "       " );
    for ( i = 0; i < 6; i++ ) 
        fprintf( s, "%f ", p->approx[2][i] );
    fprintf( s, "\n" );
    fprintf( s, "       \n" );
*/
}
static void Abc_SclWriteLibraryText( FILE * s, SC_Lib * p )
{
    SC_WireLoad * pWL;
    SC_WireLoadSel * pWLS;
    SC_Cell * pCell;
    SC_Pin * pPin;
    int n_valid_cells;
    int i, j, k;
    fprintf( s, "/* This Liberty file was generated by ABC on %s */\n", Extra_TimeStamp() );
    fprintf( s, "/* The original unabridged library came from file \"%s\".*/\n\n", p->pFileName );

//    fprintf( s, "%d", ABC_SCL_CUR_VERSION );
    fprintf( s, "library(%s) {\n\n",                         p->pName );
    if ( p->default_wire_load && strlen(p->default_wire_load) )
    fprintf( s, "  default_wire_load : \"%s\";\n",           p->default_wire_load );
    if ( p->default_wire_load_sel && strlen(p->default_wire_load_sel) )
    fprintf( s, "  default_wire_load_selection : \"%s\";\n", p->default_wire_load_sel );
    if ( p->default_max_out_slew != -1 )
    fprintf( s, "  default_max_transition : %f;\n",          p->default_max_out_slew );
    if ( p->unit_time == 9 )
    fprintf( s, "  time_unit : \"1ns\";\n" );
    else if ( p->unit_time == 10 )
    fprintf( s, "  time_unit : \"100ps\";\n" );
    else if ( p->unit_time == 11 )
    fprintf( s, "  time_unit : \"10ps\";\n" );
    else if ( p->unit_time == 12 )
    fprintf( s, "  time_unit : \"1ps\";\n" );
    else assert( 0 );
    fprintf( s, "  capacitive_load_unit(%.1f,%s);\n",        p->unit_cap_fst, p->unit_cap_snd == 12 ? "pf" : "ff" );
    fprintf( s, "\n" );

    // Write 'wire_load' vector:
    SC_LibForEachWireLoad( p, pWL, i )
    {
        fprintf( s, "  wire_load(\"%s\") {\n", pWL->pName );
        fprintf( s, "    capacitance : %f;\n", pWL->cap );
        fprintf( s, "    slope : %f;\n", pWL->slope );
        for ( j = 0; j < Vec_IntSize(&pWL->vFanout); j++ )
            fprintf( s, "    fanout_length( %d, %f );\n", Vec_IntEntry(&pWL->vFanout, j), Vec_FltEntry(&pWL->vLen, j) );
        fprintf( s, "  }\n\n" );
    }

    // Write 'wire_load_sel' vector:
    SC_LibForEachWireLoadSel( p, pWLS, i )
    {
        fprintf( s, "  wire_load_selection(\"%s\") {\n", pWLS->pName );
        for ( j = 0; j < Vec_FltSize(&pWLS->vAreaFrom); j++)
            fprintf( s, "    wire_load_from_area( %f, %f, %s );\n", 
                Vec_FltEntry(&pWLS->vAreaFrom, j), 
                Vec_FltEntry(&pWLS->vAreaTo, j), 
                (char *)Vec_PtrEntry(&pWLS->vWireLoadModel, j) );
        fprintf( s, "  }\n\n" );
    }

    // Write 'cells' vector:
    n_valid_cells = 0;
    SC_LibForEachCell( p, pCell, i )
        if ( !(pCell->seq || pCell->unsupp) )
            n_valid_cells++;

    SC_LibForEachCell( p, pCell, i )
    {
        if ( pCell->seq || pCell->unsupp )
            continue;

        fprintf( s, "\n" );
        fprintf( s, "  cell(%s) {\n",                             pCell->pName );
        fprintf( s, "    /*  n_inputs = %d  n_outputs = %d */\n", pCell->n_inputs, pCell->n_outputs );
        fprintf( s, "    area : %f;\n",                           pCell->area );
        fprintf( s, "    cell_leakage_power : %f;\n",             pCell->leakage );
        fprintf( s, "    drive_strength : %d;\n",                 pCell->drive_strength );

        SC_CellForEachPinIn( pCell, pPin, j )
        {
            assert(pPin->dir == sc_dir_Input);
            fprintf( s, "    pin(%s) {\n", pPin->pName );
            fprintf( s, "      direction : %s;\n", "input" );
            fprintf( s, "      fall_capacitance : %f;\n", pPin->fall_cap );
            fprintf( s, "      rise_capacitance : %f;\n", pPin->rise_cap );
            fprintf( s, "    }\n" );
        }

        SC_CellForEachPinOut( pCell, pPin, j )
        {
            SC_Timings * pRTime;
//            word uWord;
            assert(pPin->dir == sc_dir_Output);
            fprintf( s, "    pin(%s) {\n", pPin->pName );
            fprintf( s, "      direction : %s;\n", "output" );
            fprintf( s, "      max_capacitance : %f;\n", pPin->max_out_cap );
            fprintf( s, "      max_transition : %f;\n", pPin->max_out_slew );
            fprintf( s, "      function : \"%s\";\n", pPin->func_text ? pPin->func_text : "?" );
            fprintf( s, "      /*  truth table = " );
            Extra_PrintHex( s, (unsigned *)Vec_WrdArray(&pPin->vFunc), pCell->n_inputs );
            fprintf( s, "  */\n" );

            // Write 'rtiming': (pin-to-pin timing tables for this particular output)
            assert( Vec_PtrSize(&pPin->vRTimings) == pCell->n_inputs );
            SC_PinForEachRTiming( pPin, pRTime, k )
            {
                if ( Vec_PtrSize(&pRTime->vTimings) == 1 )
                {
                    SC_Timing * pTime = (SC_Timing *)Vec_PtrEntry( &pRTime->vTimings, 0 );
                    fprintf( s, "      timing() {\n" );
                    fprintf( s, "        related_pin : \"%s\"\n", pRTime->pName );
                    if ( pTime->tsense == sc_ts_Pos )
                        fprintf( s, "        timing_sense : positive_unate;\n" );
                    else if ( pTime->tsense == sc_ts_Neg )
                        fprintf( s, "        timing_sense : negative_unate;\n" );
                    else if ( pTime->tsense == sc_ts_Non )
                        fprintf( s, "        timing_sense : non_unate;\n" );
                    else assert( 0 );

                    fprintf( s, "        cell_rise() {\n" );
                    Abc_SclWriteSurfaceText( s, &pTime->pCellRise );
                    fprintf( s, "        }\n" );

                    fprintf( s, "        cell_fall() {\n" );
                    Abc_SclWriteSurfaceText( s, &pTime->pCellFall );
                    fprintf( s, "        }\n" );

                    fprintf( s, "        rise_transition() {\n" );
                    Abc_SclWriteSurfaceText( s, &pTime->pRiseTrans );
                    fprintf( s, "        }\n" );

                    fprintf( s, "        fall_transition() {\n" );
                    Abc_SclWriteSurfaceText( s, &pTime->pFallTrans );
                    fprintf( s, "        }\n" );
                    fprintf( s, "      }\n" );
                }
                else
                    assert( Vec_PtrSize(&pRTime->vTimings) == 0 );
            }
            fprintf( s, "    }\n" );
        }
        fprintf( s, "  }\n" );
    }
    fprintf( s, "}\n\n" );
}
void Abc_SclWriteLiberty( char * pFileName, SC_Lib * p )
{
    FILE * pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
        printf( "Cannot open text file \"%s\" for writing.\n", pFileName );
    else
    {
        Abc_SclWriteLibraryText( pFile, p );
        fclose( pFile );
        printf( "Dumped internal library with %d cells into Liberty file \"%s\".\n", SC_LibCellNum(p), pFileName );
    }
}

/**Function*************************************************************

  Synopsis    [Appends cells of pLib2 to those of pLib1.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
SC_Lib * Abc_SclMergeLibraries( SC_Lib * pLib1, SC_Lib * pLib2, int fUsePrefix )
{
    Vec_Str_t * vOut = Vec_StrAlloc( 10000 );
    int n_valid_cells2 = Abc_SclCountValidCells( pLib2 );
    Abc_SclWriteLibrary( vOut, pLib1, n_valid_cells2, fUsePrefix );
    Abc_SclWriteLibraryCellsOnly( vOut, pLib2, fUsePrefix ? 2 : 0 );
    SC_Lib * p = Abc_SclReadFromStr( vOut );
    p->pFileName = Abc_UtilStrsav( pLib1->pFileName );
    p->pName = ABC_ALLOC( char, strlen(pLib1->pName) + strlen(pLib2->pName) + 10 );
    sprintf( p->pName, "%s__and__%s", pLib1->pName, pLib2->pName );
    Vec_StrFree( vOut );
    printf( "Updated library \"%s\" with additional %d cells from library \"%s\".\n", pLib1->pName, n_valid_cells2, pLib2->pName );
    return p;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

