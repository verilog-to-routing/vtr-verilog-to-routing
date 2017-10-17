/**CFile****************************************************************

  FileName    [ioReadBlif.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedures to read BLIF files.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: ioReadBlif.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ioAbc.h"
#include "base/main/main.h"
#include "map/mio/mio.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Io_ReadBlif_t_        Io_ReadBlif_t;   // all reading info
struct Io_ReadBlif_t_
{
    // general info about file
    char *               pFileName;    // the name of the file
    Extra_FileReader_t * pReader;      // the input file reader
    // current processing info
    Abc_Ntk_t *          pNtkMaster;   // the primary network
    Abc_Ntk_t *          pNtkCur;      // the primary network
    int                  LineCur;      // the line currently parsed
    // temporary storage for tokens
    Vec_Ptr_t *          vTokens;      // the current tokens
    Vec_Ptr_t *          vNewTokens;   // the temporary storage for the tokens
    Vec_Str_t *          vCubes;       // the temporary storage for the tokens
    // timing information
    Vec_Int_t *          vInArrs;      // input arrival
    Vec_Int_t *          vOutReqs;     // output required
    Vec_Int_t *          vInDrives;    // input drive
    Vec_Int_t *          vOutLoads;    // output load
    float                DefInArrRise; // input arrival default
    float                DefInArrFall; // input arrival default
    float                DefOutReqRise;// output required default
    float                DefOutReqFall;// output required default
    float                DefInDriRise; // input drive default
    float                DefInDriFall; // input drive default
    float                DefOutLoadRise;// output load default
    float                DefOutLoadFall;// output load default
    int                  fHaveDefInArr;  // provided in the file
    int                  fHaveDefOutReq; // provided in the file
    int                  fHaveDefInDri;  // provided in the file
    int                  fHaveDefOutLoad;// provided in the file
    // the error message
    FILE *               Output;       // the output stream
    char                 sError[1000]; // the error string generated during parsing
    int                  fError;       // set to 1 when error occurs
};

static Io_ReadBlif_t * Io_ReadBlifFile( char * pFileName );
static void Io_ReadBlifFree( Io_ReadBlif_t * p );
static void Io_ReadBlifPrintErrorMessage( Io_ReadBlif_t * p );
static Vec_Ptr_t * Io_ReadBlifGetTokens( Io_ReadBlif_t * p );
static char * Io_ReadBlifCleanName( char * pName );

static Abc_Ntk_t * Io_ReadBlifNetwork( Io_ReadBlif_t * p );
static Abc_Ntk_t * Io_ReadBlifNetworkOne( Io_ReadBlif_t * p );
static int Io_ReadBlifNetworkInputs( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens );
static int Io_ReadBlifNetworkOutputs( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens );
static int Io_ReadBlifNetworkLatch( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens );
static int Io_ReadBlifNetworkNames( Io_ReadBlif_t * p, Vec_Ptr_t ** pvTokens );
static int Io_ReadBlifNetworkGate( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens );
static int Io_ReadBlifNetworkSubcircuit( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens );
static int Io_ReadBlifNetworkInputArrival( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens );
static int Io_ReadBlifNetworkOutputRequired( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens );
static int Io_ReadBlifNetworkDefaultInputArrival( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens );
static int Io_ReadBlifNetworkDefaultOutputRequired( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens );
static int Io_ReadBlifNetworkInputDrive( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens );
static int Io_ReadBlifNetworkOutputLoad( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens );
static int Io_ReadBlifNetworkDefaultInputDrive( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens );
static int Io_ReadBlifNetworkDefaultOutputLoad( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens );
static int Io_ReadBlifNetworkAndGateDelay( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens );
static int Io_ReadBlifNetworkConnectBoxes( Io_ReadBlif_t * p, Abc_Ntk_t * pNtkMaster );
static int Io_ReadBlifCreateTiming( Io_ReadBlif_t * p, Abc_Ntk_t * pNtkMaster );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reads the (hierarchical) network from the BLIF file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Io_ReadBlif( char * pFileName, int fCheck )
{
    Io_ReadBlif_t * p;
    Abc_Ntk_t * pNtk;

    // start the file
    p = Io_ReadBlifFile( pFileName );
    if ( p == NULL )
        return NULL;

    // read the hierarchical network
    pNtk = Io_ReadBlifNetwork( p );
    if ( pNtk == NULL )
    {
        Io_ReadBlifFree( p );
        return NULL;
    }
    pNtk->pSpec = Extra_UtilStrsav( pFileName );
    Io_ReadBlifCreateTiming( p, pNtk );
    Io_ReadBlifFree( p );

    // make sure that everything is okay with the network structure
    if ( fCheck && !Abc_NtkCheckRead( pNtk ) )
    {
        printf( "Io_ReadBlif: The network check has failed.\n" );
        Abc_NtkDelete( pNtk );
        return NULL;
    }
    return pNtk;
}

/**Function*************************************************************

  Synopsis    [Iteratively reads several networks in the hierarchical design.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Io_ReadBlifNetwork( Io_ReadBlif_t * p )
{
    Abc_Ntk_t * pNtk, * pNtkMaster;

    // read the name of the master network
    p->vTokens = Io_ReadBlifGetTokens(p);
    if ( p->vTokens == NULL || strcmp( (char *)p->vTokens->pArray[0], ".model" ) )
    {
        p->LineCur = 0;
        sprintf( p->sError, "Wrong input file format." );
        Io_ReadBlifPrintErrorMessage( p );
        return NULL;
    }

    // read networks (with EXDC) 
    pNtkMaster = NULL;
    while ( p->vTokens )
    {
        // read the network and its EXDC if present
        pNtk = Io_ReadBlifNetworkOne( p );
        if ( pNtk == NULL )
            break;
        if ( p->vTokens && strcmp((char *)p->vTokens->pArray[0], ".exdc") == 0 )
        {
            pNtk->pExdc = Io_ReadBlifNetworkOne( p );
            if ( pNtk->pExdc == NULL )
                break;
            Abc_NtkFinalizeRead( pNtk->pExdc );
        }
        // add this network as part of the hierarchy
        if ( pNtkMaster == NULL ) // no master network so far
        {
            p->pNtkMaster = pNtkMaster = pNtk;
            continue;
        }
/*
        // make sure hierarchy does not have the network with this name
        if ( pNtkMaster->tName2Model && stmm_is_member( pNtkMaster->tName2Model, pNtk->pName ) )
        {
            p->LineCur = 0;
            sprintf( p->sError, "Model %s is multiply defined in the file.", pNtk->pName );
            Io_ReadBlifPrintErrorMessage( p );
            Abc_NtkDelete( pNtk );
            Abc_NtkDelete( pNtkMaster );
            pNtkMaster = NULL;
            return NULL;
        }
        // add the network to the hierarchy
        if ( pNtkMaster->tName2Model == NULL )
            pNtkMaster->tName2Model = stmm_init_table((int (*)(void))strcmp, (int (*)(void))stmm_strhash);
        stmm_insert( pNtkMaster->tName2Model, pNtk->pName, (char *)pNtk );
*/
    }
/*
    // if there is a hierarchy, connect the boxes
    if ( pNtkMaster && pNtkMaster->tName2Model )
    {
        if ( Io_ReadBlifNetworkConnectBoxes( p, pNtkMaster ) )
        {
            Abc_NtkDelete( pNtkMaster );
            return NULL;
        }
    }
    else 
*/
    if ( !p->fError )
        Abc_NtkFinalizeRead( pNtkMaster );
    // return the master network
    return pNtkMaster;
}

/**Function*************************************************************

  Synopsis    [Reads one (main or exdc) network from the BLIF file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Io_ReadBlifNetworkOne( Io_ReadBlif_t * p )
{
    ProgressBar * pProgress = NULL;
    Abc_Ntk_t * pNtk;
    char * pDirective;
    int iLine, fTokensReady, fStatus;

    // make sure the tokens are present
    assert( p->vTokens != NULL );

    // create the new network
    p->pNtkCur = pNtk = Abc_NtkAlloc( ABC_NTK_NETLIST, ABC_FUNC_SOP, 1 );
    // read the model name
    if ( strcmp( (char *)p->vTokens->pArray[0], ".model" ) == 0 )
    {
        char * pToken, * pPivot;
        if ( Vec_PtrSize(p->vTokens) != 2 )
        {
            p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
            sprintf( p->sError, "The .model line does not have exactly two entries." );
            Io_ReadBlifPrintErrorMessage( p );
            return NULL;
        }
        for ( pPivot = pToken = (char *)Vec_PtrEntry(p->vTokens, 1); *pToken; pToken++ )
            if ( *pToken == '/' || *pToken == '\\' )
                pPivot = pToken+1;
        pNtk->pName = Extra_UtilStrsav( pPivot );
    }
    else if ( strcmp( (char *)p->vTokens->pArray[0], ".exdc" ) != 0 ) 
    {
        printf( "%s: File parsing skipped after line %d (\"%s\").\n", p->pFileName, 
            Extra_FileReaderGetLineNumber(p->pReader, 0), (char*)p->vTokens->pArray[0] );
        Abc_NtkDelete(pNtk);
        p->pNtkCur = NULL;
        return NULL;
    }

    // read the inputs/outputs
    if ( p->pNtkMaster == NULL )
        pProgress = Extra_ProgressBarStart( stdout, Extra_FileReaderGetFileSize(p->pReader) );
    fTokensReady = fStatus = 0;
    for ( iLine = 0; fTokensReady || (p->vTokens = Io_ReadBlifGetTokens(p)); iLine++ )
    {
        if ( p->pNtkMaster == NULL && iLine % 1000 == 0 )
            Extra_ProgressBarUpdate( pProgress, Extra_FileReaderGetCurPosition(p->pReader), NULL );

        // consider different line types
        fTokensReady = 0;
        pDirective = (char *)p->vTokens->pArray[0];
        if ( !strcmp( pDirective, ".names" ) )
            { fStatus = Io_ReadBlifNetworkNames( p, &p->vTokens ); fTokensReady = 1; }
        else if ( !strcmp( pDirective, ".gate" ) )
            fStatus = Io_ReadBlifNetworkGate( p, p->vTokens );
        else if ( !strcmp( pDirective, ".latch" ) )
            fStatus = Io_ReadBlifNetworkLatch( p, p->vTokens );
        else if ( !strcmp( pDirective, ".inputs" ) )
            fStatus = Io_ReadBlifNetworkInputs( p, p->vTokens );
        else if ( !strcmp( pDirective, ".outputs" ) )
            fStatus = Io_ReadBlifNetworkOutputs( p, p->vTokens );
        else if ( !strcmp( pDirective, ".input_arrival" ) )
            fStatus = Io_ReadBlifNetworkInputArrival( p, p->vTokens );
        else if ( !strcmp( pDirective, ".output_required" ) )
            fStatus = Io_ReadBlifNetworkOutputRequired( p, p->vTokens );
        else if ( !strcmp( pDirective, ".default_input_arrival" ) )
            fStatus = Io_ReadBlifNetworkDefaultInputArrival( p, p->vTokens );
        else if ( !strcmp( pDirective, ".default_output_required" ) )
            fStatus = Io_ReadBlifNetworkDefaultOutputRequired( p, p->vTokens );
        else if ( !strcmp( pDirective, ".input_drive" ) )
            fStatus = Io_ReadBlifNetworkInputDrive( p, p->vTokens );
        else if ( !strcmp( pDirective, ".output_load" ) )
            fStatus = Io_ReadBlifNetworkOutputLoad( p, p->vTokens );
        else if ( !strcmp( pDirective, ".default_input_drive" ) )
            fStatus = Io_ReadBlifNetworkDefaultInputDrive( p, p->vTokens );
        else if ( !strcmp( pDirective, ".default_output_load" ) )
            fStatus = Io_ReadBlifNetworkDefaultOutputLoad( p, p->vTokens );
        else if ( !strcmp( pDirective, ".and_gate_delay" ) )
            fStatus = Io_ReadBlifNetworkAndGateDelay( p, p->vTokens );
//        else if ( !strcmp( pDirective, ".subckt" ) )
//            fStatus = Io_ReadBlifNetworkSubcircuit( p, p->vTokens );
        else if ( !strcmp( pDirective, ".exdc" ) )
            break;
        else if ( !strcmp( pDirective, ".end" ) )
        {
            p->vTokens = Io_ReadBlifGetTokens(p);
            break;
        }
        else if ( !strcmp( pDirective, ".blackbox" ) )
        {
            pNtk->ntkType = ABC_NTK_NETLIST;
            pNtk->ntkFunc = ABC_FUNC_BLACKBOX;
            Mem_FlexStop( (Mem_Flex_t *)pNtk->pManFunc, 0 );
            pNtk->pManFunc = NULL;
        }
        else
            printf( "%s (line %d): Skipping directive \"%s\".\n", p->pFileName, 
                Extra_FileReaderGetLineNumber(p->pReader, 0), pDirective );
        if ( p->vTokens == NULL ) // some files do not have ".end" in the end
            break;
        if ( fStatus == 1 )
        {
            Extra_ProgressBarStop( pProgress );
            Abc_NtkDelete( pNtk );
            return NULL;
        }
    }
    if ( p->pNtkMaster == NULL )
        Extra_ProgressBarStop( pProgress );
    return pNtk;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadBlifNetworkInputs( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens )
{
    int i;
    for ( i = 1; i < vTokens->nSize; i++ )
        Io_ReadCreatePi( p->pNtkCur, (char *)vTokens->pArray[i] );
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadBlifNetworkOutputs( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens )
{
    int i;
    for ( i = 1; i < vTokens->nSize; i++ )
        Io_ReadCreatePo( p->pNtkCur, (char *)vTokens->pArray[i] );
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadBlifNetworkLatch( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens )
{ 
    Abc_Ntk_t * pNtk = p->pNtkCur;
    Abc_Obj_t * pLatch;
    int ResetValue;
    if ( vTokens->nSize < 3 )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "The .latch line does not have enough tokens." );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    // create the latch
    pLatch = Io_ReadCreateLatch( pNtk, (char *)vTokens->pArray[1], (char *)vTokens->pArray[2] );
    // get the latch reset value
    if ( vTokens->nSize == 3 )
        Abc_LatchSetInitDc( pLatch );
    else
    {
        ResetValue = atoi((char *)vTokens->pArray[vTokens->nSize-1]);
        if ( ResetValue != 0 && ResetValue != 1 && ResetValue != 2 )
        {
            p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
            sprintf( p->sError, "The .latch line has an unknown reset value (%s).", (char*)vTokens->pArray[3] );
            Io_ReadBlifPrintErrorMessage( p );
            return 1;
        }
        if ( ResetValue == 0 )
            Abc_LatchSetInit0( pLatch );
        else if ( ResetValue == 1 )
            Abc_LatchSetInit1( pLatch );
        else if ( ResetValue == 2 )
            Abc_LatchSetInitDc( pLatch );
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadBlifNetworkNames( Io_ReadBlif_t * p, Vec_Ptr_t ** pvTokens )
{
    Vec_Ptr_t * vTokens = *pvTokens;
    Abc_Ntk_t * pNtk = p->pNtkCur;
    Abc_Obj_t * pNode;
    char * pToken, Char, ** ppNames;
    int nFanins, nNames;

    // create a new node and add it to the network
    if ( vTokens->nSize < 2 )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "The .names line has less than two tokens." );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }

    // create the node
    ppNames = (char **)vTokens->pArray + 1;
    nNames  = vTokens->nSize - 2;
    pNode   = Io_ReadCreateNode( pNtk, ppNames[nNames], ppNames, nNames );

    // derive the functionality of the node
    p->vCubes->nSize = 0;
    nFanins = vTokens->nSize - 2;
    if ( nFanins == 0 )
    {
        while ( (vTokens = Io_ReadBlifGetTokens(p)) )
        {
            pToken = (char *)vTokens->pArray[0];
            if ( pToken[0] == '.' )
                break;
            // read the cube
            if ( vTokens->nSize != 1 )
            {
                p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
                sprintf( p->sError, "The number of tokens in the constant cube is wrong." );
                Io_ReadBlifPrintErrorMessage( p );
                return 1;
            }
            // create the cube
            Char = ((char *)vTokens->pArray[0])[0];
            Vec_StrPush( p->vCubes, ' ' );
            Vec_StrPush( p->vCubes, Char );
            Vec_StrPush( p->vCubes, '\n' );
        }
    }
    else
    {
        while ( (vTokens = Io_ReadBlifGetTokens(p)) )
        {
            pToken = (char *)vTokens->pArray[0];
            if ( pToken[0] == '.' )
                break;
            // read the cube
            if ( vTokens->nSize != 2 )
            {
                p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
                sprintf( p->sError, "The number of tokens in the cube is wrong." );
                Io_ReadBlifPrintErrorMessage( p );
                return 1;
            }
            // create the cube
            Vec_StrPrintStr( p->vCubes, (char *)vTokens->pArray[0] );
            // check the char 
            Char = ((char *)vTokens->pArray[1])[0];
            if ( Char != '0' && Char != '1' && Char != 'x' && Char != 'n' )
            {
                p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
                sprintf( p->sError, "The output character in the constant cube is wrong." );
                Io_ReadBlifPrintErrorMessage( p );
                return 1;
            }
            Vec_StrPush( p->vCubes, ' ' );
            Vec_StrPush( p->vCubes, Char );
            Vec_StrPush( p->vCubes, '\n' );
        }
    }
    // if there is nothing there
    if ( p->vCubes->nSize == 0 )
    {
        // create an empty cube
        Vec_StrPush( p->vCubes, ' ' );
        Vec_StrPush( p->vCubes, '0' );
        Vec_StrPush( p->vCubes, '\n' );
    }
    Vec_StrPush( p->vCubes, 0 );

    // set the pointer to the functionality of the node
    Abc_ObjSetData( pNode, Abc_SopRegister((Mem_Flex_t *)pNtk->pManFunc, p->vCubes->pArray) );

    // check the size
    if ( Abc_ObjFaninNum(pNode) != Abc_SopGetVarNum((char *)Abc_ObjData(pNode)) )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "The number of fanins (%d) of node %s is different from SOP size (%d).", 
            Abc_ObjFaninNum(pNode), Abc_ObjName(Abc_ObjFanout(pNode,0)), Abc_SopGetVarNum((char *)Abc_ObjData(pNode)) );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }

    // return the last array of tokens
    *pvTokens = vTokens;
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadBlifReorderFormalNames( Vec_Ptr_t * vTokens, Mio_Gate_t * pGate, Mio_Gate_t * pTwin )
{
    Mio_Pin_t * pGatePin;
    char * pName, * pNamePin;
    int i, k, nSize, Length;
    nSize = Vec_PtrSize(vTokens);
    if ( pTwin == NULL )
    {
        if ( nSize - 3 != Mio_GateReadPinNum(pGate) )
            return 0;
    }
    else
    {
        if ( nSize - 3 != Mio_GateReadPinNum(pGate) && nSize - 4 != Mio_GateReadPinNum(pGate) )
            return 0;
    }
    // check if the names are in order
    for ( pGatePin = Mio_GateReadPins(pGate), i = 0; pGatePin; pGatePin = Mio_PinReadNext(pGatePin), i++ )
    {
        pNamePin = Mio_PinReadName(pGatePin);
        Length = strlen(pNamePin);
        pName = (char *)Vec_PtrEntry(vTokens, i+2);
        if ( !strncmp( pNamePin, pName, Length ) && pName[Length] == '=' )
            continue;
        break;
    }
    if ( pTwin == NULL )
    {
        if ( i == Mio_GateReadPinNum(pGate) )
            return 1;
        // reorder the pins
        for ( pGatePin = Mio_GateReadPins(pGate), i = 0; pGatePin; pGatePin = Mio_PinReadNext(pGatePin), i++ )
        {
            pNamePin = Mio_PinReadName(pGatePin);
            Length = strlen(pNamePin);
            for ( k = 2; k < nSize; k++ )
            {
                pName = (char *)Vec_PtrEntry(vTokens, k);
                if ( !strncmp( pNamePin, pName, Length ) && pName[Length] == '=' )
                {
                    Vec_PtrPush( vTokens, pName );
                    break;
                }
            }
        }
        pNamePin = Mio_GateReadOutName(pGate);
        Length = strlen(pNamePin);
        for ( k = 2; k < nSize; k++ )
        {
            pName = (char *)Vec_PtrEntry(vTokens, k);
            if ( !strncmp( pNamePin, pName, Length ) && pName[Length] == '=' )
            {
                Vec_PtrPush( vTokens, pName );
                break;
            }
        }
        if ( Vec_PtrSize(vTokens) - nSize != nSize - 2 )
            return 0;
        Vec_PtrForEachEntryStart( char *, vTokens, pName, k, nSize )
            Vec_PtrWriteEntry( vTokens, k - nSize + 2, pName );
        Vec_PtrShrink( vTokens, nSize );
    }
    else
    {
        if ( i != Mio_GateReadPinNum(pGate) ) // expect the correct order of input pins in the network with twin gates
            return 0;
        // check the last two entries
        if ( nSize - 3 == Mio_GateReadPinNum(pGate) ) // only one output is available
        {
            pNamePin = Mio_GateReadOutName(pGate);
            Length = strlen(pNamePin);
            pName = (char *)Vec_PtrEntry(vTokens, nSize - 1);
            if ( !strncmp( pNamePin, pName, Length ) && pName[Length] == '=' ) // the last entry is pGate
            {
                Vec_PtrPush( vTokens, NULL );
                return 1;
            }
            pNamePin = Mio_GateReadOutName(pTwin);
            Length = strlen(pNamePin);
            pName = (char *)Vec_PtrEntry(vTokens, nSize - 1);
            if ( !strncmp( pNamePin, pName, Length ) && pName[Length] == '=' ) // the last entry is pTwin
            {
                pName = (char *)Vec_PtrPop( vTokens );
                Vec_PtrPush( vTokens, NULL );
                Vec_PtrPush( vTokens, pName );
                return 1;
            }
            return 0;
        }
        if ( nSize - 4 == Mio_GateReadPinNum(pGate) ) // two outputs are available
        {
            pNamePin = Mio_GateReadOutName(pGate);
            Length = strlen(pNamePin);
            pName = (char *)Vec_PtrEntry(vTokens, nSize - 2);
            if ( !(!strncmp( pNamePin, pName, Length ) && pName[Length] == '=') )
                return 0;
            pNamePin = Mio_GateReadOutName(pTwin);
            Length = strlen(pNamePin);
            pName = (char *)Vec_PtrEntry(vTokens, nSize - 1);
            if ( !(!strncmp( pNamePin, pName, Length ) && pName[Length] == '=') )
                return 0;
            return 1;
        }
        assert( 0 );
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadBlifNetworkGate( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens )
{
    Mio_Library_t * pGenlib; 
    Mio_Gate_t * pGate;
    Abc_Obj_t * pNode;
    char ** ppNames;
    int i, nNames;

    // check that the library is available
    pGenlib = (Mio_Library_t *)Abc_FrameReadLibGen();
    if ( pGenlib == NULL )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "The current library is not available." );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }

    // create a new node and add it to the network
    if ( vTokens->nSize < 2 )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "The .gate line has less than two tokens." );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }

    // get the gate
    pGate = Mio_LibraryReadGateByName( pGenlib, (char *)vTokens->pArray[1], NULL );
    if ( pGate == NULL )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Cannot find gate \"%s\" in the library.", (char*)vTokens->pArray[1] );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }

    // if this is the first line with gate, update the network type
    if ( Abc_NtkNodeNum(p->pNtkCur) == 0 )
    {
        assert( p->pNtkCur->ntkFunc == ABC_FUNC_SOP );
        p->pNtkCur->ntkFunc = ABC_FUNC_MAP;
        Mem_FlexStop( (Mem_Flex_t *)p->pNtkCur->pManFunc, 0 );
        p->pNtkCur->pManFunc = pGenlib;
    }

    // reorder the formal inputs to be in the same order as in the gate
    if ( !Io_ReadBlifReorderFormalNames( vTokens, pGate, Mio_GateReadTwin(pGate) ) )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Mismatch in the fanins of gate \"%s\".", (char*)vTokens->pArray[1] );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }


    // remove the formal parameter names
    for ( i = 2; i < vTokens->nSize; i++ )
    {
        vTokens->pArray[i] = Io_ReadBlifCleanName( (char *)vTokens->pArray[i] );
        if ( vTokens->pArray[i] == NULL )
        {
            p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
            sprintf( p->sError, "Invalid gate input assignment." );
            Io_ReadBlifPrintErrorMessage( p );
            return 1;
        }
    }

    // create the node
    if ( Mio_GateReadTwin(pGate) == NULL )
    {
        nNames  = vTokens->nSize - 3;
        ppNames = (char **)vTokens->pArray + 2;
        pNode   = Io_ReadCreateNode( p->pNtkCur, ppNames[nNames], ppNames, nNames );
        Abc_ObjSetData( pNode, pGate );
    }
    else
    {
        nNames  = vTokens->nSize - 4;
        ppNames = (char **)vTokens->pArray + 2;
        assert( ppNames[nNames] != NULL || ppNames[nNames+1] != NULL );
        if ( ppNames[nNames] )
        {
            pNode   = Io_ReadCreateNode( p->pNtkCur, ppNames[nNames], ppNames, nNames );
            Abc_ObjSetData( pNode, pGate );
        }
        if ( ppNames[nNames+1] )
        {
            pNode   = Io_ReadCreateNode( p->pNtkCur, ppNames[nNames+1], ppNames, nNames );
            Abc_ObjSetData( pNode, Mio_GateReadTwin(pGate) );
        }
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Creates a multi-input multi-output box in the hierarchical design.]

  Description []
               
  SideEffects [] 

  SeeAlso     []

***********************************************************************/
int Io_ReadBlifNetworkSubcircuit( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens )
{
    Abc_Obj_t * pBox;
    Vec_Ptr_t * vNames;
    char * pName;
    int i;

    // create a new node and add it to the network
    if ( vTokens->nSize < 3 )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "The .subcircuit line has less than three tokens." );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }

    // store the names of formal/actual inputs/outputs of the box
    vNames = Vec_PtrAlloc( 10 );
    Vec_PtrForEachEntryStart( char *, vTokens, pName, i, 1 )
//        Vec_PtrPush( vNames, Abc_NtkRegisterName(p->pNtkCur, pName) );
        Vec_PtrPush( vNames, Extra_UtilStrsav(pName) );  // memory leak!!!

    // create a new box and add it to the network
    pBox = Abc_NtkCreateBlackbox( p->pNtkCur );
    // set the pointer to the node names
    Abc_ObjSetData( pBox, vNames );
    // remember the line of the file
    pBox->pCopy = (Abc_Obj_t *)(ABC_PTRINT_T)Extra_FileReaderGetLineNumber(p->pReader, 0);
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Io_ReadBlifCleanName( char * pName )
{
    int i, Length;
    Length = strlen(pName);
    for ( i = 0; i < Length; i++ )
        if ( pName[i] == '=' )
            return pName + i + 1;
    return NULL;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadBlifNetworkInputArrival( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens )
{
    Abc_Obj_t * pNet;
    char * pFoo1, * pFoo2;
    double TimeRise, TimeFall;
 
    // make sure this is indeed the .inputs line
    assert( strncmp( (char *)vTokens->pArray[0], ".input_arrival", 14 ) == 0 );
    if ( vTokens->nSize != 4 )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Wrong number of arguments on .input_arrival line." );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    pNet = Abc_NtkFindNet( p->pNtkCur, (char *)vTokens->pArray[1] );
    if ( pNet == NULL )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Cannot find object corresponding to %s on .input_arrival line.", (char*)vTokens->pArray[1] );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    TimeRise = strtod( (char *)vTokens->pArray[2], &pFoo1 );
    TimeFall = strtod( (char *)vTokens->pArray[3], &pFoo2 );
    if ( *pFoo1 != '\0' || *pFoo2 != '\0' )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Bad value (%s %s) for rise or fall time on .input_arrival line.", (char*)vTokens->pArray[2], (char*)vTokens->pArray[3] );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    // set timing info
    //Abc_NtkTimeSetArrival( p->pNtkCur, Abc_ObjFanin0(pNet)->Id, (float)TimeRise, (float)TimeFall );
    Vec_IntPush( p->vInArrs, Abc_ObjFanin0(pNet)->Id );
    Vec_IntPush( p->vInArrs, Abc_Float2Int((float)TimeRise) );
    Vec_IntPush( p->vInArrs, Abc_Float2Int((float)TimeFall) );
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadBlifNetworkOutputRequired( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens )
{
    Abc_Obj_t * pNet;
    char * pFoo1, * pFoo2;
    double TimeRise, TimeFall;
 
    // make sure this is indeed the .inputs line
    assert( strncmp( (char *)vTokens->pArray[0], ".output_required", 16 ) == 0 );
    if ( vTokens->nSize != 4 )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Wrong number of arguments on .output_required line." );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    pNet = Abc_NtkFindNet( p->pNtkCur, (char *)vTokens->pArray[1] );
    if ( pNet == NULL )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Cannot find object corresponding to %s on .output_required line.", (char*)vTokens->pArray[1] );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    TimeRise = strtod( (char *)vTokens->pArray[2], &pFoo1 );
    TimeFall = strtod( (char *)vTokens->pArray[3], &pFoo2 );
    if ( *pFoo1 != '\0' || *pFoo2 != '\0' )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Bad value (%s %s) for rise or fall time on .output_required line.", (char*)vTokens->pArray[2], (char*)vTokens->pArray[3] );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    // set timing info
//    Abc_NtkTimeSetRequired( p->pNtkCur, Abc_ObjFanout0(pNet)->Id, (float)TimeRise, (float)TimeFall );
    Vec_IntPush( p->vOutReqs, Abc_ObjFanout0(pNet)->Id );
    Vec_IntPush( p->vOutReqs, Abc_Float2Int((float)TimeRise) );
    Vec_IntPush( p->vOutReqs, Abc_Float2Int((float)TimeFall) );
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadBlifNetworkDefaultInputArrival( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens )
{
    char * pFoo1, * pFoo2;
    double TimeRise, TimeFall;

    // make sure this is indeed the .inputs line
    assert( strncmp( (char *)vTokens->pArray[0], ".default_input_arrival", 23 ) == 0 );
    if ( vTokens->nSize != 3 )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Wrong number of arguments on .default_input_arrival line." );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    TimeRise = strtod( (char *)vTokens->pArray[1], &pFoo1 );
    TimeFall = strtod( (char *)vTokens->pArray[2], &pFoo2 );
    if ( *pFoo1 != '\0' || *pFoo2 != '\0' )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Bad value (%s %s) for rise or fall time on .default_input_arrival line.", (char*)vTokens->pArray[1], (char*)vTokens->pArray[2] );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    // set timing info
    //Abc_NtkTimeSetDefaultArrival( p->pNtkCur, (float)TimeRise, (float)TimeFall );
    p->DefInArrRise = (float)TimeRise;
    p->DefInArrFall = (float)TimeFall;
    p->fHaveDefInArr = 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadBlifNetworkDefaultOutputRequired( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens )
{
    char * pFoo1, * pFoo2;
    double TimeRise, TimeFall;

    // make sure this is indeed the .inputs line
    assert( strncmp( (char *)vTokens->pArray[0], ".default_output_required", 25 ) == 0 );
    if ( vTokens->nSize != 3 )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Wrong number of arguments on .default_output_required line." );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    TimeRise = strtod( (char *)vTokens->pArray[1], &pFoo1 );
    TimeFall = strtod( (char *)vTokens->pArray[2], &pFoo2 );
    if ( *pFoo1 != '\0' || *pFoo2 != '\0' )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Bad value (%s %s) for rise or fall time on .default_output_required line.", (char*)vTokens->pArray[1], (char*)vTokens->pArray[2] );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    // set timing info
//    Abc_NtkTimeSetDefaultRequired( p->pNtkCur, (float)TimeRise, (float)TimeFall );
    p->DefOutReqRise = (float)TimeRise;
    p->DefOutReqFall = (float)TimeFall;
    p->fHaveDefOutReq = 1;
    return 0;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadFindCiId( Abc_Ntk_t * pNtk, Abc_Obj_t * pObj )
{
    Abc_Obj_t * pTemp;
    int i;
    Abc_NtkForEachCi( pNtk, pTemp, i )
        if ( pTemp == pObj )
            return i;
    return -1;
}
int Io_ReadBlifNetworkInputDrive( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens )
{
    Abc_Obj_t * pNet;
    char * pFoo1, * pFoo2;
    double TimeRise, TimeFall;
 
    // make sure this is indeed the .inputs line
    assert( strncmp( (char *)vTokens->pArray[0], ".input_drive", 12 ) == 0 );
    if ( vTokens->nSize != 4 )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Wrong number of arguments on .input_drive line." );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    pNet = Abc_NtkFindNet( p->pNtkCur, (char *)vTokens->pArray[1] );
    if ( pNet == NULL )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Cannot find object corresponding to %s on .input_drive line.", (char*)vTokens->pArray[1] );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    TimeRise = strtod( (char *)vTokens->pArray[2], &pFoo1 );
    TimeFall = strtod( (char *)vTokens->pArray[3], &pFoo2 );
    if ( *pFoo1 != '\0' || *pFoo2 != '\0' )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Bad value (%s %s) for rise or fall time on .input_drive line.", (char*)vTokens->pArray[2], (char*)vTokens->pArray[3] );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    // set timing info
    //Abc_NtkTimeSetInputDrive( p->pNtkCur, Io_ReadFindCiId(p->pNtkCur, Abc_NtkObj(p->pNtkCur, Abc_ObjFanin0(pNet)->Id)), (float)TimeRise, (float)TimeFall );
    Vec_IntPush( p->vInDrives, Io_ReadFindCiId(p->pNtkCur, Abc_NtkObj(p->pNtkCur, Abc_ObjFanin0(pNet)->Id)) );
    Vec_IntPush( p->vInDrives, Abc_Float2Int((float)TimeRise) );
    Vec_IntPush( p->vInDrives, Abc_Float2Int((float)TimeFall) );
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadFindCoId( Abc_Ntk_t * pNtk, Abc_Obj_t * pObj )
{
    Abc_Obj_t * pTemp;
    int i;
    Abc_NtkForEachPo( pNtk, pTemp, i )
        if ( pTemp == pObj )
            return i;
    return -1;
}
int Io_ReadBlifNetworkOutputLoad( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens )
{
    Abc_Obj_t * pNet;
    char * pFoo1, * pFoo2;
    double TimeRise, TimeFall;
 
    // make sure this is indeed the .inputs line
    assert( strncmp( (char *)vTokens->pArray[0], ".output_load", 12 ) == 0 );
    if ( vTokens->nSize != 4 )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Wrong number of arguments on .output_load line." );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    pNet = Abc_NtkFindNet( p->pNtkCur, (char *)vTokens->pArray[1] );
    if ( pNet == NULL )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Cannot find object corresponding to %s on .output_load line.", (char*)vTokens->pArray[1] );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    TimeRise = strtod( (char *)vTokens->pArray[2], &pFoo1 );
    TimeFall = strtod( (char *)vTokens->pArray[3], &pFoo2 );
    if ( *pFoo1 != '\0' || *pFoo2 != '\0' )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Bad value (%s %s) for rise or fall time on .output_load line.", (char*)vTokens->pArray[2], (char*)vTokens->pArray[3] );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    // set timing info
//    Abc_NtkTimeSetOutputLoad( p->pNtkCur, Io_ReadFindCoId(p->pNtkCur, Abc_NtkObj(p->pNtkCur, Abc_ObjFanout0(pNet)->Id)), (float)TimeRise, (float)TimeFall );
    Vec_IntPush( p->vOutLoads, Io_ReadFindCoId(p->pNtkCur, Abc_NtkObj(p->pNtkCur, Abc_ObjFanout0(pNet)->Id)) );
    Vec_IntPush( p->vOutLoads, Abc_Float2Int((float)TimeRise) );
    Vec_IntPush( p->vOutLoads, Abc_Float2Int((float)TimeFall) );
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadBlifNetworkDefaultInputDrive( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens )
{
    char * pFoo1, * pFoo2;
    double TimeRise, TimeFall;

    // make sure this is indeed the .inputs line
    assert( strncmp( (char *)vTokens->pArray[0], ".default_input_drive", 21 ) == 0 );
    if ( vTokens->nSize != 3 )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Wrong number of arguments on .default_input_drive line." );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    TimeRise = strtod( (char *)vTokens->pArray[1], &pFoo1 );
    TimeFall = strtod( (char *)vTokens->pArray[2], &pFoo2 );
    if ( *pFoo1 != '\0' || *pFoo2 != '\0' )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Bad value (%s %s) for rise or fall time on .default_input_drive line.", (char*)vTokens->pArray[1], (char*)vTokens->pArray[2] );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    // set timing info
//    Abc_NtkTimeSetDefaultInputDrive( p->pNtkCur, (float)TimeRise, (float)TimeFall );
    p->DefInDriRise = (float)TimeRise;
    p->DefInDriFall = (float)TimeFall;
    p->fHaveDefInDri = 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadBlifNetworkDefaultOutputLoad( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens )
{
    char * pFoo1, * pFoo2;
    double TimeRise, TimeFall;

    // make sure this is indeed the .inputs line
    assert( strncmp( (char *)vTokens->pArray[0], ".default_output_load", 21 ) == 0 );
    if ( vTokens->nSize != 3 )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Wrong number of arguments on .default_output_load line." );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    TimeRise = strtod( (char *)vTokens->pArray[1], &pFoo1 );
    TimeFall = strtod( (char *)vTokens->pArray[2], &pFoo2 );
    if ( *pFoo1 != '\0' || *pFoo2 != '\0' )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Bad value (%s %s) for rise or fall time on .default_output_load line.", (char*)vTokens->pArray[1], (char*)vTokens->pArray[2] );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    // set timing info
//    Abc_NtkTimeSetDefaultOutputLoad( p->pNtkCur, (float)TimeRise, (float)TimeFall );
    p->DefOutLoadRise = (float)TimeRise;
    p->DefOutLoadFall = (float)TimeFall;
    p->fHaveDefOutLoad = 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadBlifNetworkAndGateDelay( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens )
{
    char * pFoo1;
    double AndGateDelay;

    // make sure this is indeed the .inputs line
    assert( strncmp( (char *)vTokens->pArray[0], ".and_gate_delay", 25 ) == 0 );
    if ( vTokens->nSize != 2 )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Wrong number of arguments (%d) on .and_gate_delay line (should be 1).", vTokens->nSize-1 );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    AndGateDelay = strtod( (char *)vTokens->pArray[1], &pFoo1 );
    if ( *pFoo1 != '\0' )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Bad value (%s) for AND gate delay in on .and_gate_delay line line.", (char*)vTokens->pArray[1] );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    // set timing info
    p->pNtkCur->AndGateDelay = (float)AndGateDelay;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Prints the error message including the file name and line number.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_ReadBlifPrintErrorMessage( Io_ReadBlif_t * p )
{
    p->fError = 1;
    if ( p->LineCur == 0 ) // the line number is not given
        fprintf( p->Output, "%s: %s\n", p->pFileName, p->sError );
    else // print the error message with the line number
        fprintf( p->Output, "%s (line %d): %s\n", p->pFileName, p->LineCur, p->sError );
}

/**Function*************************************************************

  Synopsis    [Gets the tokens taking into account the line breaks.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Io_ReadBlifGetTokens( Io_ReadBlif_t * p )
{
    Vec_Ptr_t * vTokens;
    char * pLastToken;
    int i;

    // get rid of the old tokens
    if ( p->vNewTokens->nSize > 0 )
    {
        for ( i = 0; i < p->vNewTokens->nSize; i++ )
            ABC_FREE( p->vNewTokens->pArray[i] );
        p->vNewTokens->nSize = 0;
    }

    // get the new tokens
    vTokens = (Vec_Ptr_t *)Extra_FileReaderGetTokens(p->pReader);
    if ( vTokens == NULL )
        return vTokens;

    // check if there is a transfer to another line
    pLastToken = (char *)vTokens->pArray[vTokens->nSize - 1];
    if ( pLastToken[ strlen(pLastToken)-1 ] != '\\' )
        return vTokens;

    // remove the slash
    pLastToken[ strlen(pLastToken)-1 ] = 0;
    if ( pLastToken[0] == 0 )
        vTokens->nSize--;
    // load them into the new array
    for ( i = 0; i < vTokens->nSize; i++ )
        Vec_PtrPush( p->vNewTokens, Extra_UtilStrsav((char *)vTokens->pArray[i]) );

    // load as long as there is the line break
    while ( 1 )
    {
        // get the new tokens
        vTokens = (Vec_Ptr_t *)Extra_FileReaderGetTokens(p->pReader);
        if ( vTokens->nSize == 0 )
            return p->vNewTokens;
        // check if there is a transfer to another line
        pLastToken = (char *)vTokens->pArray[vTokens->nSize - 1];
        if ( pLastToken[ strlen(pLastToken)-1 ] == '\\' )
        {
            // remove the slash
            pLastToken[ strlen(pLastToken)-1 ] = 0;
            if ( pLastToken[0] == 0 )
                vTokens->nSize--;
            // load them into the new array
            for ( i = 0; i < vTokens->nSize; i++ )
                Vec_PtrPush( p->vNewTokens, Extra_UtilStrsav((char *)vTokens->pArray[i]) );
            continue;
        }
        // otherwise, load them and break
        for ( i = 0; i < vTokens->nSize; i++ )
            Vec_PtrPush( p->vNewTokens, Extra_UtilStrsav((char *)vTokens->pArray[i]) );
        break;
    }
    return p->vNewTokens;
}

/**Function*************************************************************

  Synopsis    [Starts the reading data structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Io_ReadBlif_t * Io_ReadBlifFile( char * pFileName )
{
    Extra_FileReader_t * pReader;
    Io_ReadBlif_t * p;

    // start the reader
    pReader = Extra_FileReaderAlloc( pFileName, "#", "\n\r", " \t" );

    if ( pReader == NULL )
        return NULL;

    // start the reading data structure
    p = ABC_ALLOC( Io_ReadBlif_t, 1 );
    memset( p, 0, sizeof(Io_ReadBlif_t) );
    p->pFileName  = pFileName;
    p->pReader    = pReader;
    p->Output     = stdout;
    p->vNewTokens = Vec_PtrAlloc( 100 );
    p->vCubes     = Vec_StrAlloc( 100 );
    p->vInArrs    = Vec_IntAlloc( 100 );
    p->vOutReqs   = Vec_IntAlloc( 100 );
    p->vInDrives  = Vec_IntAlloc( 100 );
    p->vOutLoads  = Vec_IntAlloc( 100 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Frees the data structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_ReadBlifFree( Io_ReadBlif_t * p )
{
    Extra_FileReaderFree( p->pReader );
    Vec_PtrFree( p->vNewTokens );
    Vec_StrFree( p->vCubes );
    Vec_IntFree( p->vInArrs );
    Vec_IntFree( p->vOutReqs );
    Vec_IntFree( p->vInDrives );
    Vec_IntFree( p->vOutLoads );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    [Connect one box.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadBlifNetworkConnectBoxesOneBox( Io_ReadBlif_t * p, Abc_Obj_t * pBox, stmm_table * tName2Model )
{
    Vec_Ptr_t * pNames;
    Abc_Ntk_t * pNtkModel;
    Abc_Obj_t * pObj, * pNet;
    char * pName = NULL, * pActual;
    int i, Length, Start = -1;

    // get the model for this box
    pNames = (Vec_Ptr_t *)pBox->pData;
    if ( !stmm_lookup( tName2Model, (char *)Vec_PtrEntry(pNames, 0), (char **)&pNtkModel ) )
    {
        p->LineCur = (int)(ABC_PTRINT_T)pBox->pCopy;
        sprintf( p->sError, "Cannot find the model for subcircuit %s.", (char*)Vec_PtrEntry(pNames, 0) );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }

    // create the fanins of the box
    Abc_NtkForEachPi( pNtkModel, pObj, i )
        pObj->pCopy = NULL;
    if ( Abc_NtkPiNum(pNtkModel) == 0 )
        Start = 1;
    else
    {
        Vec_PtrForEachEntryStart( char *, pNames, pName, i, 1 )
        {
            pActual = Io_ReadBlifCleanName(pName);
            if ( pActual == NULL )
            {
                p->LineCur = (int)(ABC_PTRINT_T)pBox->pCopy;
                sprintf( p->sError, "Cannot parse formal/actual name pair \"%s\".", pName );
                Io_ReadBlifPrintErrorMessage( p );
                return 1;
            }
            Length = pActual - pName - 1;
            pName[Length] = 0;
            // find the PI net with this name
            pObj = Abc_NtkFindNet( pNtkModel, pName );
            if ( pObj == NULL )
            {
                p->LineCur = (int)(ABC_PTRINT_T)pBox->pCopy;
                sprintf( p->sError, "Cannot find formal input \"%s\" as an PI of model \"%s\".", pName, (char*)Vec_PtrEntry(pNames, 0) );
                Io_ReadBlifPrintErrorMessage( p );
                return 1;
            }
            // get the PI
            pObj = Abc_ObjFanin0(pObj);
            // quit if this is not a PI net
            if ( !Abc_ObjIsPi(pObj) )
            {
                pName[Length] = '=';
                Start = i;
                break;
            }
            // remember the actual name in the net
            if ( pObj->pCopy != NULL )
            {
                p->LineCur = (int)(ABC_PTRINT_T)pBox->pCopy;
                sprintf( p->sError, "Formal input \"%s\" is used more than once.", pName );
                Io_ReadBlifPrintErrorMessage( p );
                return 1;
            }
            pObj->pCopy = (Abc_Obj_t *)pActual;
            // quit if we processed all PIs
            if ( i == Abc_NtkPiNum(pNtkModel) )
            {
                Start = i+1;
                break;
            }
        }
    }
    // create the fanins of the box
    Abc_NtkForEachPi( pNtkModel, pObj, i )
    {
        pActual = (char *)pObj->pCopy;
        if ( pActual == NULL )
        {
            p->LineCur = (int)(ABC_PTRINT_T)pBox->pCopy;
            sprintf( p->sError, "Formal input \"%s\" of model %s is not driven.", pName, (char*)Vec_PtrEntry(pNames, 0) );
            Io_ReadBlifPrintErrorMessage( p );
            return 1;
        }
        pNet = Abc_NtkFindOrCreateNet( pBox->pNtk, pActual );
        Abc_ObjAddFanin( pBox, pNet );
    }
    Abc_NtkForEachPi( pNtkModel, pObj, i )
        pObj->pCopy = NULL;

    // create the fanouts of the box
    Abc_NtkForEachPo( pNtkModel, pObj, i )
        pObj->pCopy = NULL;
    Vec_PtrForEachEntryStart( char *, pNames, pName, i, Start )
    {
        pActual = Io_ReadBlifCleanName(pName);
        if ( pActual == NULL )
        {
            p->LineCur = (int)(ABC_PTRINT_T)pBox->pCopy;
            sprintf( p->sError, "Cannot parse formal/actual name pair \"%s\".", pName );
            Io_ReadBlifPrintErrorMessage( p );
            return 1;
        }
        Length = pActual - pName - 1;
        pName[Length] = 0;
        // find the PO net with this name
        pObj = Abc_NtkFindNet( pNtkModel, pName );
        if ( pObj == NULL )
        {
            p->LineCur = (int)(ABC_PTRINT_T)pBox->pCopy;
            sprintf( p->sError, "Cannot find formal output \"%s\" as an PO of model \"%s\".", pName, (char*)Vec_PtrEntry(pNames, 0) );
            Io_ReadBlifPrintErrorMessage( p );
            return 1;
        }
        // get the PO
        pObj = Abc_ObjFanout0(pObj);
        if ( pObj->pCopy != NULL )
        {
            p->LineCur = (int)(ABC_PTRINT_T)pBox->pCopy;
            sprintf( p->sError, "Formal output \"%s\" is used more than once.", pName );
            Io_ReadBlifPrintErrorMessage( p );
            return 1;
        }
        pObj->pCopy = (Abc_Obj_t *)pActual;
    }
    // create the fanouts of the box
    Abc_NtkForEachPo( pNtkModel, pObj, i )
    {
        pActual = (char *)pObj->pCopy;
        if ( pActual == NULL )
        {
            p->LineCur = (int)(ABC_PTRINT_T)pBox->pCopy;
            sprintf( p->sError, "Formal output \"%s\" of model %s is not driven.", pName, (char*)Vec_PtrEntry(pNames, 0) );
            Io_ReadBlifPrintErrorMessage( p );
            return 1;
        }
        pNet = Abc_NtkFindOrCreateNet( pBox->pNtk, pActual );
        Abc_ObjAddFanin( pNet, pBox );
    }
    Abc_NtkForEachPo( pNtkModel, pObj, i )
        pObj->pCopy = NULL;

    // remove the array of names, assign the pointer to the model
    Vec_PtrForEachEntry( char *, (Vec_Ptr_t *)pBox->pData, pName, i )
        ABC_FREE( pName );
    Vec_PtrFree( (Vec_Ptr_t *)pBox->pData );
    pBox->pData = pNtkModel;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Connect the boxes in the hierarchy of networks.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadBlifNetworkConnectBoxesOne( Io_ReadBlif_t * p, Abc_Ntk_t * pNtk, stmm_table * tName2Model )
{
    Abc_Obj_t * pBox;
    int i;
    // go through the boxes
    Abc_NtkForEachBlackbox( pNtk, pBox, i )
        if ( Io_ReadBlifNetworkConnectBoxesOneBox( p, pBox, tName2Model ) )
            return 1;
    Abc_NtkFinalizeRead( pNtk );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Creates timing manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadBlifCreateTiming( Io_ReadBlif_t * p, Abc_Ntk_t * pNtk )
{
    int Id, Rise, Fall, i;

    // set timing info
    //Abc_NtkTimeSetDefaultArrival( p->pNtkCur, (float)TimeRise, (float)TimeFall );
//    p->DefInArrRise = (float)TimeRise;
//    p->DefInArrFall = (float)TimeFall;
    if ( p->fHaveDefInArr )
    Abc_NtkTimeSetDefaultArrival( pNtk, p->DefInArrRise, p->DefInArrFall );
    // set timing info
    //Abc_NtkTimeSetDefaultRequired( p->pNtkCur, (float)TimeRise, (float)TimeFall );
//    p->DefOutReqRise = (float)TimeRise;
//    p->DefOutReqFall = (float)TimeFall;
    if ( p->fHaveDefOutReq )
    Abc_NtkTimeSetDefaultRequired( pNtk, p->DefOutReqRise, p->DefOutReqFall );
    // set timing info
    //Abc_NtkTimeSetDefaultInputDrive( p->pNtkCur, (float)TimeRise, (float)TimeFall );
//    p->DefInDriRise = (float)TimeRise;
//    p->DefInDriFall = (float)TimeFall;
    if ( p->fHaveDefInDri )
    Abc_NtkTimeSetDefaultInputDrive( pNtk, p->DefInDriRise, p->DefInDriFall );
    // set timing info
    //Abc_NtkTimeSetDefaultOutputLoad( p->pNtkCur, (float)TimeRise, (float)TimeFall );
//    p->DefOutLoadRise = (float)TimeRise;
//    p->DefOutLoadFall = (float)TimeFall;
    if ( p->fHaveDefOutLoad )
    Abc_NtkTimeSetDefaultOutputLoad( pNtk, p->DefOutLoadRise, p->DefOutLoadFall );

    // set timing info
    //Abc_NtkTimeSetArrival( p->pNtkCur, Abc_ObjFanin0(pNet)->Id, (float)TimeRise, (float)TimeFall );
//    Vec_IntPush( p->vInArrs, Abc_ObjFanin0(pNet)->Id );
//    Vec_IntPush( p->vInArrs, Abc_Float2Int((float)TimeRise) );
//    Vec_IntPush( p->vInArrs, Abc_Float2Int((float)TimeFall) );
    Vec_IntForEachEntryTriple( p->vInArrs, Id, Rise, Fall, i )
        Abc_NtkTimeSetArrival( pNtk, Id, Abc_Int2Float(Rise), Abc_Int2Float(Fall) );
    // set timing info
    //Abc_NtkTimeSetRequired( p->pNtkCur, Abc_ObjFanout0(pNet)->Id, (float)TimeRise, (float)TimeFall );
//    Vec_IntPush( p->vOutReqs, Abc_ObjFanout0(pNet)->Id );
//    Vec_IntPush( p->vOutReqs, Abc_Float2Int((float)TimeRise) );
//    Vec_IntPush( p->vOutReqs, Abc_Float2Int((float)TimeFall) );
    Vec_IntForEachEntryTriple( p->vOutReqs, Id, Rise, Fall, i )
        Abc_NtkTimeSetRequired( pNtk, Id, Abc_Int2Float(Rise), Abc_Int2Float(Fall) );
    // set timing info
    //Abc_NtkTimeSetInputDrive( p->pNtkCur, Io_ReadFindCiId(p->pNtkCur, Abc_NtkObj(p->pNtkCur, Abc_ObjFanin0(pNet)->Id)), (float)TimeRise, (float)TimeFall );
//    Vec_IntPush( p->vInDrives, Io_ReadFindCiId(p->pNtkCur, Abc_NtkObj(p->pNtkCur, Abc_ObjFanin0(pNet)->Id)) );
//    Vec_IntPush( p->vInDrives, Abc_Float2Int((float)TimeRise) );
//    Vec_IntPush( p->vInDrives, Abc_Float2Int((float)TimeFall) );
    Vec_IntForEachEntryTriple( p->vInDrives, Id, Rise, Fall, i )
        Abc_NtkTimeSetInputDrive( pNtk, Id, Abc_Int2Float(Rise), Abc_Int2Float(Fall) );
    // set timing info
    //Abc_NtkTimeSetOutputLoad( p->pNtkCur, Io_ReadFindCoId(p->pNtkCur, Abc_NtkObj(p->pNtkCur, Abc_ObjFanout0(pNet)->Id)), (float)TimeRise, (float)TimeFall );
//    Vec_IntPush( p->vOutLoads, Io_ReadFindCoId(p->pNtkCur, Abc_NtkObj(p->pNtkCur, Abc_ObjFanout0(pNet)->Id)) );
//    Vec_IntPush( p->vOutLoads, Abc_Float2Int((float)TimeRise) );
//    Vec_IntPush( p->vOutLoads, Abc_Float2Int((float)TimeFall) );
    Vec_IntForEachEntryTriple( p->vOutLoads, Id, Rise, Fall, i )
        Abc_NtkTimeSetOutputLoad( pNtk, Id, Abc_Int2Float(Rise), Abc_Int2Float(Fall) );
    return 1;
}

#if 0

/**Function*************************************************************

  Synopsis    [Connect the boxes in the hierarchy of networks.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadBlifNetworkConnectBoxes( Io_ReadBlif_t * p, Abc_Ntk_t * pNtkMaster )
{
    stmm_generator * gen;
    Abc_Ntk_t * pNtk;
    char * pName;
    // connect the master network
    if ( Io_ReadBlifNetworkConnectBoxesOne( p, pNtkMaster, pNtkMaster->tName2Model ) )
        return 1;
    // connect other networks
    stmm_foreach_item( pNtkMaster->tName2Model, gen, &pName, (char **)&pNtk )
        if ( Io_ReadBlifNetworkConnectBoxesOne( p, pNtk, pNtkMaster->tName2Model ) )
            return 1;
    return 0;
}

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

