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

#include "io.h"
#include "main.h"
#include "mio.h"

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
static int Io_ReadBlifNetworkAsserts( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens );
static int Io_ReadBlifNetworkLatch( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens );
static int Io_ReadBlifNetworkNames( Io_ReadBlif_t * p, Vec_Ptr_t ** pvTokens );
static int Io_ReadBlifNetworkGate( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens );
static int Io_ReadBlifNetworkSubcircuit( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens );
static int Io_ReadBlifNetworkInputArrival( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens );
static int Io_ReadBlifNetworkDefaultInputArrival( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens );
static int Io_ReadBlifNetworkConnectBoxes( Io_ReadBlif_t * p, Abc_Ntk_t * pNtkMaster );

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
    Abc_NtkTimeInitialize( pNtk );
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
    if ( p->vTokens == NULL || strcmp( p->vTokens->pArray[0], ".model" ) )
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
        if ( p->vTokens && strcmp(p->vTokens->pArray[0], ".exdc") == 0 )
        {
            pNtk->pExdc = Io_ReadBlifNetworkOne( p );
            Abc_NtkFinalizeRead( pNtk->pExdc );
            if ( pNtk->pExdc == NULL )
                break;
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
            pNtkMaster->tName2Model = stmm_init_table(strcmp, stmm_strhash);
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
    ProgressBar * pProgress;
    Abc_Ntk_t * pNtk;
    char * pDirective;
    int iLine, fTokensReady, fStatus;

    // make sure the tokens are present
    assert( p->vTokens != NULL );

    // create the new network
    p->pNtkCur = pNtk = Abc_NtkAlloc( ABC_NTK_NETLIST, ABC_FUNC_SOP, 1 );
    // read the model name
    if ( strcmp( p->vTokens->pArray[0], ".model" ) == 0 )
        pNtk->pName = Extra_UtilStrsav( p->vTokens->pArray[1] );
    else if ( strcmp( p->vTokens->pArray[0], ".exdc" ) != 0 ) 
    {
        printf( "%s: File parsing skipped after line %d (\"%s\").\n", p->pFileName, 
            Extra_FileReaderGetLineNumber(p->pReader, 0), p->vTokens->pArray[0] );
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
        pDirective = p->vTokens->pArray[0];
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
        else if ( !strcmp( pDirective, ".asserts" ) )
            fStatus = Io_ReadBlifNetworkAsserts( p, p->vTokens );
        else if ( !strcmp( pDirective, ".input_arrival" ) )
            fStatus = Io_ReadBlifNetworkInputArrival( p, p->vTokens );
        else if ( !strcmp( pDirective, ".default_input_arrival" ) )
            fStatus = Io_ReadBlifNetworkDefaultInputArrival( p, p->vTokens );
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
            Extra_MmFlexStop( pNtk->pManFunc );
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
        Io_ReadCreatePi( p->pNtkCur, vTokens->pArray[i] );
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
        Io_ReadCreatePo( p->pNtkCur, vTokens->pArray[i] );
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadBlifNetworkAsserts( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens )
{
    int i;
    for ( i = 1; i < vTokens->nSize; i++ )
        Io_ReadCreateAssert( p->pNtkCur, vTokens->pArray[i] );
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
	char * pLatchType;
    Abc_LatchInfo_t* pLatchInfo;
    int ResetValue;
    if ( vTokens->nSize < 3 )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "The .latch line does not have enough tokens." );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    // create the latch
    pLatch = Io_ReadCreateLatch( pNtk, vTokens->pArray[1], vTokens->pArray[2] );
    // get the latch reset value
    if ( vTokens->nSize == 3 )
        Abc_LatchSetInitDc( pLatch );
    else
    {
		pLatchInfo = ((Abc_LatchInfo_t *)pLatch->pData);

		if (vTokens->nSize >= 5)
		{
			pLatchInfo->pClkName = strdup(vTokens->pArray[4]);
			pLatchType = vTokens->pArray[3];

			if (!strcmp(pLatchType, "re"))
				pLatchInfo->LatchType = ABC_RISING_EDGE;
			else if (!strcmp(pLatchType, "fe"))
				pLatchInfo->LatchType = ABC_FALLING_EDGE;
			else if (!strcmp(pLatchType, "ah"))
				pLatchInfo->LatchType = ABC_ACTIVE_HIGH;
			else if (!strcmp(pLatchType, "al"))
				pLatchInfo->LatchType = ABC_ACTIVE_LOW;
			else
				pLatchInfo->LatchType = ABC_ASYNC; // god knows when it is used...
		}
		else
		{
			pLatchInfo->pClkName = 0;
			pLatchInfo->LatchType = ABC_ASYNC;
		}

        ResetValue = atoi(vTokens->pArray[vTokens->nSize-1]);
        if ( ResetValue != 0 && ResetValue != 1 && ResetValue != 2 )
        {
            p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
            sprintf( p->sError, "The .latch line has an unknown reset value (%s).", vTokens->pArray[3] );
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
        while ( vTokens = Io_ReadBlifGetTokens(p) )
        {
            pToken = vTokens->pArray[0];
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
        while ( vTokens = Io_ReadBlifGetTokens(p) )
        {
            pToken = vTokens->pArray[0];
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
            Vec_StrAppend( p->vCubes, vTokens->pArray[0] );
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
    Abc_ObjSetData( pNode, Abc_SopRegister(pNtk->pManFunc, p->vCubes->pArray) );

    // check the size
    if ( Abc_ObjFaninNum(pNode) != Abc_SopGetVarNum(Abc_ObjData(pNode)) )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "The number of fanins (%d) of node %s is different from SOP size (%d).", 
            Abc_ObjFaninNum(pNode), Abc_ObjName(Abc_ObjFanout(pNode,0)), Abc_SopGetVarNum(Abc_ObjData(pNode)) );
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
int Io_ReadBlifNetworkGate( Io_ReadBlif_t * p, Vec_Ptr_t * vTokens )
{
    Mio_Library_t * pGenlib; 
    Mio_Gate_t * pGate;
    Abc_Obj_t * pNode;
    char ** ppNames;
    int i, nNames;

    // check that the library is available
    pGenlib = Abc_FrameReadLibGen();
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
    pGate = Mio_LibraryReadGateByName( pGenlib, vTokens->pArray[1] );
    if ( pGate == NULL )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Cannot find gate \"%s\" in the library.", vTokens->pArray[1] );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }

    // if this is the first line with gate, update the network type
    if ( Abc_NtkNodeNum(p->pNtkCur) == 0 )
    {
        assert( p->pNtkCur->ntkFunc == ABC_FUNC_SOP );
        p->pNtkCur->ntkFunc = ABC_FUNC_MAP;
        Extra_MmFlexStop( p->pNtkCur->pManFunc );
        p->pNtkCur->pManFunc = pGenlib;
    }

    // remove the formal parameter names
    for ( i = 2; i < vTokens->nSize; i++ )
    {
        vTokens->pArray[i] = Io_ReadBlifCleanName( vTokens->pArray[i] );
        if ( vTokens->pArray[i] == NULL )
        {
            p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
            sprintf( p->sError, "Invalid gate input assignment." );
            Io_ReadBlifPrintErrorMessage( p );
            return 1;
        }
    }

    // create the node
    ppNames = (char **)vTokens->pArray + 2;
    nNames  = vTokens->nSize - 3;
    pNode   = Io_ReadCreateNode( p->pNtkCur, ppNames[nNames], ppNames, nNames );

    // set the pointer to the functionality of the node
    Abc_ObjSetData( pNode, pGate );
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
    Vec_PtrForEachEntryStart( vTokens, pName, i, 1 )
//        Vec_PtrPush( vNames, Abc_NtkRegisterName(p->pNtkCur, pName) );
        Vec_PtrPush( vNames, Extra_UtilStrsav(pName) );  // memory leak!!!

    // create a new box and add it to the network
    pBox = Abc_NtkCreateBlackbox( p->pNtkCur );
    // set the pointer to the node names
    Abc_ObjSetData( pBox, vNames );
    // remember the line of the file
    pBox->pCopy = (void *)Extra_FileReaderGetLineNumber(p->pReader, 0);
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
    assert( strncmp( vTokens->pArray[0], ".input_arrival", 14 ) == 0 );
    if ( vTokens->nSize != 4 )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Wrong number of arguments on .input_arrival line." );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    pNet = Abc_NtkFindNet( p->pNtkCur, vTokens->pArray[1] );
    if ( pNet == NULL )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Cannot find object corresponding to %s on .input_arrival line.", vTokens->pArray[1] );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    TimeRise = strtod( vTokens->pArray[2], &pFoo1 );
    TimeFall = strtod( vTokens->pArray[3], &pFoo2 );
    if ( *pFoo1 != '\0' || *pFoo2 != '\0' )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Bad value (%s %s) for rise or fall time on .input_arrival line.", vTokens->pArray[2], vTokens->pArray[3] );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    // set the arrival time
    Abc_NtkTimeSetArrival( p->pNtkCur, Abc_ObjFanin0(pNet)->Id, (float)TimeRise, (float)TimeFall );
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
    assert( strncmp( vTokens->pArray[0], ".default_input_arrival", 23 ) == 0 );
    if ( vTokens->nSize != 3 )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Wrong number of arguments on .default_input_arrival line." );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    TimeRise = strtod( vTokens->pArray[1], &pFoo1 );
    TimeFall = strtod( vTokens->pArray[2], &pFoo2 );
    if ( *pFoo1 != '\0' || *pFoo2 != '\0' )
    {
        p->LineCur = Extra_FileReaderGetLineNumber(p->pReader, 0);
        sprintf( p->sError, "Bad value (%s %s) for rise or fall time on .default_input_arrival line.", vTokens->pArray[1], vTokens->pArray[2] );
        Io_ReadBlifPrintErrorMessage( p );
        return 1;
    }
    // set the arrival time
    Abc_NtkTimeSetDefaultArrival( p->pNtkCur, (float)TimeRise, (float)TimeFall );
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
            free( p->vNewTokens->pArray[i] );
        p->vNewTokens->nSize = 0;
    }

    // get the new tokens
    vTokens = Extra_FileReaderGetTokens(p->pReader);
    if ( vTokens == NULL )
        return vTokens;

    // check if there is a transfer to another line
    pLastToken = vTokens->pArray[vTokens->nSize - 1];
    if ( pLastToken[ strlen(pLastToken)-1 ] != '\\' )
        return vTokens;

    // remove the slash
    pLastToken[ strlen(pLastToken)-1 ] = 0;
    if ( pLastToken[0] == 0 )
        vTokens->nSize--;
    // load them into the new array
    for ( i = 0; i < vTokens->nSize; i++ )
        Vec_PtrPush( p->vNewTokens, Extra_UtilStrsav(vTokens->pArray[i]) );

    // load as long as there is the line break
    while ( 1 )
    {
        // get the new tokens
        vTokens = Extra_FileReaderGetTokens(p->pReader);
        if ( vTokens->nSize == 0 )
            return p->vNewTokens;
        // check if there is a transfer to another line
        pLastToken = vTokens->pArray[vTokens->nSize - 1];
        if ( pLastToken[ strlen(pLastToken)-1 ] == '\\' )
        {
            // remove the slash
            pLastToken[ strlen(pLastToken)-1 ] = 0;
            if ( pLastToken[0] == 0 )
                vTokens->nSize--;
            // load them into the new array
            for ( i = 0; i < vTokens->nSize; i++ )
                Vec_PtrPush( p->vNewTokens, Extra_UtilStrsav(vTokens->pArray[i]) );
            continue;
        }
        // otherwise, load them and break
        for ( i = 0; i < vTokens->nSize; i++ )
            Vec_PtrPush( p->vNewTokens, Extra_UtilStrsav(vTokens->pArray[i]) );
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
    p = ALLOC( Io_ReadBlif_t, 1 );
    memset( p, 0, sizeof(Io_ReadBlif_t) );
    p->pFileName  = pFileName;
    p->pReader    = pReader;
    p->Output     = stdout;
    p->vNewTokens = Vec_PtrAlloc( 100 );
    p->vCubes     = Vec_StrAlloc( 100 );
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
    free( p );
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
    char * pName, * pActual;
    int i, Length, Start;

    // get the model for this box
    pNames = pBox->pData;
    if ( !stmm_lookup( tName2Model, Vec_PtrEntry(pNames, 0), (char **)&pNtkModel ) )
    {
        p->LineCur = (int)pBox->pCopy;
        sprintf( p->sError, "Cannot find the model for subcircuit %s.", Vec_PtrEntry(pNames, 0) );
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
        Vec_PtrForEachEntryStart( pNames, pName, i, 1 )
        {
            pActual = Io_ReadBlifCleanName(pName);
            if ( pActual == NULL )
            {
                p->LineCur = (int)pBox->pCopy;
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
                p->LineCur = (int)pBox->pCopy;
                sprintf( p->sError, "Cannot find formal input \"%s\" as an PI of model \"%s\".", pName, Vec_PtrEntry(pNames, 0) );
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
                p->LineCur = (int)pBox->pCopy;
                sprintf( p->sError, "Formal input \"%s\" is used more than once.", pName );
                Io_ReadBlifPrintErrorMessage( p );
                return 1;
            }
            pObj->pCopy = (void *)pActual;
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
        pActual = (void *)pObj->pCopy;
        if ( pActual == NULL )
        {
            p->LineCur = (int)pBox->pCopy;
            sprintf( p->sError, "Formal input \"%s\" of model %s is not driven.", pName, Vec_PtrEntry(pNames, 0) );
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
    Vec_PtrForEachEntryStart( pNames, pName, i, Start )
    {
        pActual = Io_ReadBlifCleanName(pName);
        if ( pActual == NULL )
        {
            p->LineCur = (int)pBox->pCopy;
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
            p->LineCur = (int)pBox->pCopy;
            sprintf( p->sError, "Cannot find formal output \"%s\" as an PO of model \"%s\".", pName, Vec_PtrEntry(pNames, 0) );
            Io_ReadBlifPrintErrorMessage( p );
            return 1;
        }
        // get the PO
        pObj = Abc_ObjFanout0(pObj);
        if ( pObj->pCopy != NULL )
        {
            p->LineCur = (int)pBox->pCopy;
            sprintf( p->sError, "Formal output \"%s\" is used more than once.", pName );
            Io_ReadBlifPrintErrorMessage( p );
            return 1;
        }
        pObj->pCopy = (void *)pActual;
    }
    // create the fanouts of the box
    Abc_NtkForEachPo( pNtkModel, pObj, i )
    {
        pActual = (void *)pObj->pCopy;
        if ( pActual == NULL )
        {
            p->LineCur = (int)pBox->pCopy;
            sprintf( p->sError, "Formal output \"%s\" of model %s is not driven.", pName, Vec_PtrEntry(pNames, 0) );
            Io_ReadBlifPrintErrorMessage( p );
            return 1;
        }
        pNet = Abc_NtkFindOrCreateNet( pBox->pNtk, pActual );
        Abc_ObjAddFanin( pNet, pBox );
    }
    Abc_NtkForEachPo( pNtkModel, pObj, i )
        pObj->pCopy = NULL;

    // remove the array of names, assign the pointer to the model
    Vec_PtrForEachEntry( pBox->pData, pName, i )
        free( pName );
    Vec_PtrFree( pBox->pData );
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


