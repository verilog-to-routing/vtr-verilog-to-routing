/**CFile****************************************************************

  FileName    [verCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Verilog parser.]

  Synopsis    [Parses several flavors of structural Verilog.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 19, 2006.]

  Revision    [$Id: verCore.c,v 1.00 2006/08/19 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ver.h"
#include "map/mio/mio.h"
#include "base/main/main.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// types of verilog signals
typedef enum { 
    VER_SIG_NONE = 0,
    VER_SIG_INPUT,
    VER_SIG_OUTPUT,
    VER_SIG_INOUT,
    VER_SIG_REG,
    VER_SIG_WIRE
} Ver_SignalType_t;

// types of verilog gates
typedef enum { 
    VER_GATE_AND = 0,
    VER_GATE_OR,
    VER_GATE_XOR,
    VER_GATE_BUF,
    VER_GATE_NAND,
    VER_GATE_NOR,
    VER_GATE_XNOR,
    VER_GATE_NOT
} Ver_GateType_t;

static Ver_Man_t * Ver_ParseStart( char * pFileName, Abc_Des_t * pGateLib );
static void Ver_ParseStop( Ver_Man_t * p );
static void Ver_ParseFreeData( Ver_Man_t * p );
static void Ver_ParseInternal( Ver_Man_t * p );
static int  Ver_ParseModule( Ver_Man_t * p );
static int  Ver_ParseSignal( Ver_Man_t * p, Abc_Ntk_t * pNtk, Ver_SignalType_t SigType );
static int  Ver_ParseAlways( Ver_Man_t * p, Abc_Ntk_t * pNtk );
static int  Ver_ParseInitial( Ver_Man_t * p, Abc_Ntk_t * pNtk );
static int  Ver_ParseAssign( Ver_Man_t * p, Abc_Ntk_t * pNtk );
static int  Ver_ParseGateStandard( Ver_Man_t * pMan, Abc_Ntk_t * pNtk, Ver_GateType_t GateType );
static int  Ver_ParseFlopStandard( Ver_Man_t * pMan, Abc_Ntk_t * pNtk );
static int  Ver_ParseGate( Ver_Man_t * p, Abc_Ntk_t * pNtk, Mio_Gate_t * pGate );
static int  Ver_ParseBox( Ver_Man_t * pMan, Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkBox );
static int  Ver_ParseConnectBox( Ver_Man_t * pMan, Abc_Obj_t * pBox );
static int  Ver_ParseAttachBoxes( Ver_Man_t * pMan );

static Abc_Obj_t * Ver_ParseCreatePi( Abc_Ntk_t * pNtk, char * pName );
static Abc_Obj_t * Ver_ParseCreatePo( Abc_Ntk_t * pNtk, char * pName );
static Abc_Obj_t * Ver_ParseCreateLatch( Abc_Ntk_t * pNtk, Abc_Obj_t * pNetLI, Abc_Obj_t * pNetLO );
static Abc_Obj_t * Ver_ParseCreateInv( Abc_Ntk_t * pNtk, Abc_Obj_t * pNet );

static void Ver_ParseRemoveSuffixTable( Ver_Man_t * pMan );

static inline int Ver_NtkIsDefined( Abc_Ntk_t * pNtkBox )  { assert( pNtkBox->pName );     return Abc_NtkPiNum(pNtkBox) || Abc_NtkPoNum(pNtkBox);  }
static inline int Ver_ObjIsConnected( Abc_Obj_t * pObj )   { assert( Abc_ObjIsBox(pObj) ); return Abc_ObjFaninNum(pObj) || Abc_ObjFanoutNum(pObj); }

int glo_fMapped = 0; // this is bad!

typedef struct Ver_Bundle_t_    Ver_Bundle_t;
struct Ver_Bundle_t_
{
    char *          pNameFormal;   // the name of the formal net
    Vec_Ptr_t *     vNetsActual;   // the vector of actual nets (MSB to LSB)
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Start parser.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ver_Man_t * Ver_ParseStart( char * pFileName, Abc_Des_t * pGateLib )
{
    Ver_Man_t * p;
    p = ABC_ALLOC( Ver_Man_t, 1 );
    memset( p, 0, sizeof(Ver_Man_t) );
    p->pFileName = pFileName;
    p->pReader   = Ver_StreamAlloc( pFileName );
    if ( p->pReader == NULL )
    {
        ABC_FREE( p );
        return NULL;
    }
    p->Output    = stdout;
    p->vNames    = Vec_PtrAlloc( 100 );
    p->vStackFn  = Vec_PtrAlloc( 100 );
    p->vStackOp  = Vec_IntAlloc( 100 );
    p->vPerm     = Vec_IntAlloc( 100 );
    // create the design library and assign the technology library
    p->pDesign   = Abc_DesCreate( pFileName );
    p->pDesign->pLibrary = pGateLib;
    // derive library from SCL
//    if ( Abc_FrameReadLibScl() )
//        Abc_SclInstallGenlib( Abc_FrameReadLibScl(), 0, 0, 0 );
    p->pDesign->pGenlib = Abc_FrameReadLibGen();
    return p;
}

/**Function*************************************************************

  Synopsis    [Stop parser.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ver_ParseStop( Ver_Man_t * p )
{
    if ( p->pProgress )
        Extra_ProgressBarStop( p->pProgress );
    Ver_StreamFree( p->pReader );
    Vec_PtrFree( p->vNames   );
    Vec_PtrFree( p->vStackFn );
    Vec_IntFree( p->vStackOp );
    Vec_IntFree( p->vPerm );
    ABC_FREE( p );
}
 
/**Function*************************************************************

  Synopsis    [File parser.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Des_t * Ver_ParseFile( char * pFileName, Abc_Des_t * pGateLib, int fCheck, int fUseMemMan )
{
    Ver_Man_t * p;
    Abc_Des_t * pDesign;
    // start the parser
    p = Ver_ParseStart( pFileName, pGateLib );
    p->fMapped    = glo_fMapped;
    p->fCheck     = fCheck;
    p->fUseMemMan = fUseMemMan;
    if ( glo_fMapped )
    {
        Hop_ManStop((Hop_Man_t *)p->pDesign->pManFunc);
        p->pDesign->pManFunc = NULL;
    }
    // parse the file
    Ver_ParseInternal( p );
    // save the result
    pDesign = p->pDesign;
    p->pDesign = NULL;
    // stop the parser
    Ver_ParseStop( p );
    return pDesign;
}

/**Function*************************************************************

  Synopsis    [File parser.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ver_ParseInternal( Ver_Man_t * pMan )
{
    Abc_Ntk_t * pNtk;
    char * pToken;
    int i;

    // preparse the modeles
    pMan->pProgress = Extra_ProgressBarStart( stdout, Ver_StreamGetFileSize(pMan->pReader) );
    while ( 1 )
    {
        // get the next token
        pToken = Ver_ParseGetName( pMan );
        if ( pToken == NULL )
            break;
        if ( strcmp( pToken, "module" ) )
        {
            sprintf( pMan->sError, "Cannot read \"module\" directive." );
            Ver_ParsePrintErrorMessage( pMan );
            return;
        }
        // parse the module
        if ( !Ver_ParseModule(pMan) )
            return;
    }
    Extra_ProgressBarStop( pMan->pProgress );
    pMan->pProgress = NULL;

    // process defined and undefined boxes
    if ( !Ver_ParseAttachBoxes( pMan ) )
        return;

    // connect the boxes and check
    Vec_PtrForEachEntry( Abc_Ntk_t *, pMan->pDesign->vModules, pNtk, i )
    {
        // fix the dangling nets
        Abc_NtkFinalizeRead( pNtk );
        // check the network for correctness
        if ( pMan->fCheck && !Abc_NtkCheckRead( pNtk ) )
        {
            pMan->fTopLevel = 1;
            sprintf( pMan->sError, "The network check has failed for network %s.", pNtk->pName );
            Ver_ParsePrintErrorMessage( pMan );
            return;
        }
    }
}

/**Function*************************************************************

  Synopsis    [File parser.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ver_ParseFreeData( Ver_Man_t * p )
{
    if ( p->pDesign )
    {
        Abc_DesFree( p->pDesign, NULL );
        p->pDesign = NULL;
    }
}

/**Function*************************************************************

  Synopsis    [Prints the error message including the file name and line number.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ver_ParsePrintErrorMessage( Ver_Man_t * p )
{
    p->fError = 1;
    if ( p->fTopLevel ) // the line number is not given
        fprintf( p->Output, "%s: %s\n", p->pFileName, p->sError );
    else // print the error message with the line number
        fprintf( p->Output, "%s (line %d): %s\n", 
            p->pFileName, Ver_StreamGetLineNumber(p->pReader), p->sError );
    // free the data
    Ver_ParseFreeData( p );
}

/**Function*************************************************************

  Synopsis    [Finds the network by name or create a new blackbox network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Ver_ParseFindOrCreateNetwork( Ver_Man_t * pMan, char * pName )
{
    Abc_Ntk_t * pNtkNew;
    // check if the network exists
    if ( (pNtkNew = Abc_DesFindModelByName( pMan->pDesign, pName )) )
        return pNtkNew;
//printf( "Creating network %s.\n", pName );
    // create new network
    pNtkNew = Abc_NtkAlloc( ABC_NTK_NETLIST, ABC_FUNC_BLACKBOX, pMan->fUseMemMan );
    pNtkNew->pName = Extra_UtilStrsav( pName );
    pNtkNew->pSpec = NULL;
    // add module to the design
    Abc_DesAddModel( pMan->pDesign, pNtkNew );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Finds the network by name or create a new blackbox network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Ver_ParseFindNet( Abc_Ntk_t * pNtk, char * pName )
{
    Abc_Obj_t * pObj;
    if ( (pObj = Abc_NtkFindNet(pNtk, pName)) )
        return pObj;
    if ( !strcmp( pName, "1\'b0" ) || !strcmp( pName, "1\'bx" ) )
        return Abc_NtkFindOrCreateNet( pNtk, "1\'b0" );
    if ( !strcmp( pName, "1\'b1" ) )
        return Abc_NtkFindOrCreateNet( pNtk, "1\'b1" );
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Converts the network from the blackbox type into a different one.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_ParseConvertNetwork( Ver_Man_t * pMan, Abc_Ntk_t * pNtk, int fMapped )
{
    if ( fMapped )
    {
        // convert from the blackbox into the network with local functions representated by AIGs
        if ( pNtk->ntkFunc == ABC_FUNC_BLACKBOX )
        {
            // change network type
            assert( pNtk->pManFunc == NULL );
            pNtk->ntkFunc = ABC_FUNC_MAP;
            pNtk->pManFunc = pMan->pDesign->pGenlib;
        }
        else if ( pNtk->ntkFunc != ABC_FUNC_MAP )
        {
            sprintf( pMan->sError, "The network %s appears to have both gates and assign statements. Currently such network are not allowed. One way to fix this problem might be to replace assigns by buffers from the library.", pNtk->pName );
            Ver_ParsePrintErrorMessage( pMan );
            return 0;
        }
    }
    else
    {
        // convert from the blackbox into the network with local functions representated by AIGs
        if ( pNtk->ntkFunc == ABC_FUNC_BLACKBOX )
        {
            // change network type
            assert( pNtk->pManFunc == NULL );
            pNtk->ntkFunc = ABC_FUNC_AIG;
            pNtk->pManFunc = pMan->pDesign->pManFunc;
        }
        else if ( pNtk->ntkFunc != ABC_FUNC_AIG )
        {
            sprintf( pMan->sError, "The network %s appears to have both gates and assign statements. Currently such network are not allowed. One way to fix this problem might be to replace assigns by buffers from the library.", pNtk->pName );
            Ver_ParsePrintErrorMessage( pMan );
            return 0;
        }
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Parses one Verilog module.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_ParseModule( Ver_Man_t * pMan )
{
    Mio_Gate_t * pGate;
    Ver_Stream_t * p = pMan->pReader;
    Abc_Ntk_t * pNtk, * pNtkTemp;
    char * pWord, Symbol;
    int RetValue;

    // get the network name
    pWord = Ver_ParseGetName( pMan );

    // get the network with this name
    pNtk = Ver_ParseFindOrCreateNetwork( pMan, pWord );

    // make sure we stopped at the opening parenthesis
    if ( Ver_StreamPopChar(p) != '(' )
    {
        sprintf( pMan->sError, "Cannot find \"(\" after \"module\" in network %s.", pNtk->pName );
        Ver_ParsePrintErrorMessage( pMan );
        return 0;
    }

    // skip to the end of parentheses
    do {
        if ( Ver_ParseGetName( pMan ) == NULL )
            return 0;
        Symbol = Ver_StreamPopChar(p);
    } while ( Symbol == ',' );
    assert( Symbol == ')' );
    if ( !Ver_ParseSkipComments( pMan ) )
        return 0;
    Symbol = Ver_StreamPopChar(p);
    if ( Symbol != ';' )
    {
        sprintf( pMan->sError, "Expected closing parenthesis after \"module\"." );
        Ver_ParsePrintErrorMessage( pMan );
        return 0;
    }

    // parse the inputs/outputs/registers/wires/inouts
    while ( 1 )
    {
        Extra_ProgressBarUpdate( pMan->pProgress, Ver_StreamGetCurPosition(p), NULL );
        pWord = Ver_ParseGetName( pMan );
        if ( pWord == NULL )
            return 0;
        if ( !strcmp( pWord, "input" ) )
            RetValue = Ver_ParseSignal( pMan, pNtk, VER_SIG_INPUT );
        else if ( !strcmp( pWord, "output" ) )
            RetValue = Ver_ParseSignal( pMan, pNtk, VER_SIG_OUTPUT );
        else if ( !strcmp( pWord, "reg" ) )
            RetValue = Ver_ParseSignal( pMan, pNtk, VER_SIG_REG );
        else if ( !strcmp( pWord, "wire" ) )
            RetValue = Ver_ParseSignal( pMan, pNtk, VER_SIG_WIRE );
        else if ( !strcmp( pWord, "inout" ) )
            RetValue = Ver_ParseSignal( pMan, pNtk, VER_SIG_INOUT );
        else 
            break;
        if ( RetValue == 0 )
            return 0;
    }

    // parse the remaining statements
    while ( 1 )
    {
        Extra_ProgressBarUpdate( pMan->pProgress, Ver_StreamGetCurPosition(p), NULL );

        if ( !strcmp( pWord, "and" ) )
            RetValue = Ver_ParseGateStandard( pMan, pNtk, VER_GATE_AND );
        else if ( !strcmp( pWord, "or" ) )
            RetValue = Ver_ParseGateStandard( pMan, pNtk, VER_GATE_OR );
        else if ( !strcmp( pWord, "xor" ) )
            RetValue = Ver_ParseGateStandard( pMan, pNtk, VER_GATE_XOR );
        else if ( !strcmp( pWord, "buf" ) )
            RetValue = Ver_ParseGateStandard( pMan, pNtk, VER_GATE_BUF );
        else if ( !strcmp( pWord, "nand" ) )
            RetValue = Ver_ParseGateStandard( pMan, pNtk, VER_GATE_NAND );
        else if ( !strcmp( pWord, "nor" ) )
            RetValue = Ver_ParseGateStandard( pMan, pNtk, VER_GATE_NOR );
        else if ( !strcmp( pWord, "xnor" ) )
            RetValue = Ver_ParseGateStandard( pMan, pNtk, VER_GATE_XNOR );
        else if ( !strcmp( pWord, "not" ) )
            RetValue = Ver_ParseGateStandard( pMan, pNtk, VER_GATE_NOT );

        else if ( !strcmp( pWord, "dff" ) )
            RetValue = Ver_ParseFlopStandard( pMan, pNtk );

        else if ( !strcmp( pWord, "assign" ) )
            RetValue = Ver_ParseAssign( pMan, pNtk );
        else if ( !strcmp( pWord, "always" ) )
            RetValue = Ver_ParseAlways( pMan, pNtk );
        else if ( !strcmp( pWord, "initial" ) )
            RetValue = Ver_ParseInitial( pMan, pNtk );
        else if ( !strcmp( pWord, "endmodule" ) )
            break;
        else if ( pMan->pDesign->pGenlib && (pGate = Mio_LibraryReadGateByName((Mio_Library_t *)pMan->pDesign->pGenlib, pWord, NULL)) ) // current design
            RetValue = Ver_ParseGate( pMan, pNtk, pGate );
//        else if ( pMan->pDesign->pLibrary && st__lookup(pMan->pDesign->pLibrary->tModules, pWord, (char**)&pNtkTemp) ) // gate library
//            RetValue = Ver_ParseGate( pMan, pNtkTemp );
        else if ( !strcmp( pWord, "wire" ) )
            RetValue = Ver_ParseSignal( pMan, pNtk, VER_SIG_WIRE );
        else // assume this is the box used in the current design
        {
            pNtkTemp = Ver_ParseFindOrCreateNetwork( pMan, pWord );
            RetValue = Ver_ParseBox( pMan, pNtk, pNtkTemp );
        }
        if ( RetValue == 0 )
            return 0;
        // skip the comments
        if ( !Ver_ParseSkipComments( pMan ) )
            return 0;
        // get new word
        pWord = Ver_ParseGetName( pMan );
        if ( pWord == NULL )
            return 0;
    }

    // convert from the blackbox into the network with local functions representated by AIGs
    if ( pNtk->ntkFunc == ABC_FUNC_BLACKBOX )
    {
        if ( Abc_NtkNodeNum(pNtk) > 0 || Abc_NtkBoxNum(pNtk) > 0 )
        {
            if ( !Ver_ParseConvertNetwork( pMan, pNtk, pMan->fMapped ) )
                return 0;
        }
        else
        {
            Abc_Obj_t * pObj, * pBox, * pTerm;
            int i; 
            pBox = Abc_NtkCreateBlackbox(pNtk);
            Abc_NtkForEachPi( pNtk, pObj, i )
            {
                pTerm = Abc_NtkCreateBi(pNtk);
                Abc_ObjAddFanin( pTerm, Abc_ObjFanout0(pObj) );
                Abc_ObjAddFanin( pBox, pTerm );
            }
            Abc_NtkForEachPo( pNtk, pObj, i )
            {
                pTerm = Abc_NtkCreateBo(pNtk);
                Abc_ObjAddFanin( pTerm, pBox );
                Abc_ObjAddFanin( Abc_ObjFanin0(pObj), pTerm );
            }
        }
    }

    // remove the table if needed
    Ver_ParseRemoveSuffixTable( pMan );
    return 1;
}


/**Function*************************************************************

  Synopsis    [Lookups the suffix of the signal of the form [m:n].]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_ParseLookupSuffix( Ver_Man_t * pMan, char * pWord, int * pnMsb, int * pnLsb )
{
    unsigned Value;
    *pnMsb = *pnLsb = -1;
    if ( pMan->tName2Suffix == NULL )
        return 1;
    if ( ! st__lookup( pMan->tName2Suffix, (char *)pWord, (char **)&Value ) )
        return 1;
    *pnMsb = (Value >> 8) & 0xff;
    *pnLsb = Value & 0xff;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Lookups the suffix of the signal of the form [m:n].]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_ParseInsertsSuffix( Ver_Man_t * pMan, char * pWord, int nMsb, int nLsb )
{
    unsigned Value;
    if ( pMan->tName2Suffix == NULL )
        pMan->tName2Suffix = st__init_table( strcmp, st__strhash );
    if ( st__is_member( pMan->tName2Suffix, pWord ) )
        return 1;
    assert( nMsb >= 0 && nMsb < 128 );
    assert( nLsb >= 0 && nLsb < 128 );
    Value = (nMsb << 8) | nLsb;
    st__insert( pMan->tName2Suffix, Extra_UtilStrsav(pWord), (char *)(ABC_PTRUINT_T)Value );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Lookups the suffic of the signal of the form [m:n].]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ver_ParseRemoveSuffixTable( Ver_Man_t * pMan )
{
    st__generator * gen;
    char * pKey, * pValue;
    if ( pMan->tName2Suffix == NULL )
        return;
    st__foreach_item( pMan->tName2Suffix, gen, (const char **)&pKey, (char **)&pValue )
        ABC_FREE( pKey );
    st__free_table( pMan->tName2Suffix );
    pMan->tName2Suffix = NULL;
}

/**Function*************************************************************

  Synopsis    [Determine signal prefix of the form [Beg:End].]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_ParseSignalPrefix( Ver_Man_t * pMan, char ** ppWord, int * pnMsb, int * pnLsb )
{
    char * pWord = *ppWord, * pTemp;
    int nMsb, nLsb;
    assert( pWord[0] == '[' );
    // get the beginning
    nMsb = atoi( pWord + 1 );
    // find the splitter
    while ( *pWord && *pWord != ':' && *pWord != ']' )
        pWord++;
    if ( *pWord == 0 )
    {
        sprintf( pMan->sError, "Cannot find closing bracket in this line." );
        Ver_ParsePrintErrorMessage( pMan );
        return 0;
    }
    if ( *pWord == ']' )
        nLsb = nMsb;
    else
    {
        assert( *pWord == ':' );
        nLsb = atoi( pWord + 1 );
        // find the closing parenthesis
        while ( *pWord && *pWord != ']' )
            pWord++;
        if ( *pWord == 0 )
        {
            sprintf( pMan->sError, "Cannot find closing bracket in this line." );
            Ver_ParsePrintErrorMessage( pMan );
            return 0;
        }
        assert( *pWord == ']' );
        pWord++;

        // fix the case when \<name> follows after [] without space
        if ( *pWord == '\\' )
        {
            pWord++;
            pTemp = pWord;
            while ( *pTemp && *pTemp != ' ' )
                pTemp++;
            if ( *pTemp == ' ' )
                *pTemp = 0;
        }
    }
    assert( nMsb >= 0 && nLsb >= 0 );
    // return
    *ppWord = pWord;
    *pnMsb = nMsb;
    *pnLsb = nLsb;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Determine signal suffix of the form [m:n].]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_ParseSignalSuffix( Ver_Man_t * pMan, char * pWord, int * pnMsb, int * pnLsb )
{
    char * pCur;
    int Length;
    Length = strlen(pWord);
    assert( pWord[Length-1] == ']' );
    // walk backward
    for ( pCur = pWord + Length - 2; pCur != pWord; pCur-- )
        if ( *pCur == ':' || *pCur == '[' )
            break;
    if ( pCur == pWord )
    {
        sprintf( pMan->sError, "Cannot find opening bracket in signal name %s.", pWord );
        Ver_ParsePrintErrorMessage( pMan );
        return 0;
    }
    if ( *pCur == '[' )
    {
        *pnMsb = *pnLsb = atoi(pCur+1);
        *pCur = 0;
        return 1;
    }
    assert( *pCur == ':' );
    // get the end of the interval
    *pnLsb = atoi(pCur+1);
    // find the beginning
    for ( pCur = pWord + Length - 2; pCur != pWord; pCur-- )
        if ( *pCur == '[' )
            break;
    if ( pCur == pWord )
    {
        sprintf( pMan->sError, "Cannot find opening bracket in signal name %s.", pWord );
        Ver_ParsePrintErrorMessage( pMan );
        return 0;
    }
    assert( *pCur == '[' );
    // get the beginning of the interval
    *pnMsb = atoi(pCur+1);
    // cut the word
    *pCur = 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns the values of constant bits.]

  Description [The resulting bits are in MSB to LSB order.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_ParseConstant( Ver_Man_t * pMan, char * pWord )
{
    int nBits, i;
    assert( pWord[0] >= '1' && pWord[1] <= '9' );
    nBits = atoi(pWord);
    // find the next symbol \'
    while ( *pWord && *pWord != '\'' )
        pWord++;
    if ( *pWord == 0 )
    {
        sprintf( pMan->sError, "Cannot find symbol \' in the constant." );
        Ver_ParsePrintErrorMessage( pMan );
        return 0;
    }
    assert( *pWord == '\'' );
    pWord++;
    if ( *pWord != 'b' )
    {
        sprintf( pMan->sError, "Currently can only handle binary constants." );
        Ver_ParsePrintErrorMessage( pMan );
        return 0;
    }
    pWord++;
    // scan the bits
    Vec_PtrClear( pMan->vNames );
    for ( i = 0; i < nBits; i++ )
    {
      if ( pWord[i] != '0' && pWord[i] != '1' && pWord[i] != 'x' )
      {
         sprintf( pMan->sError, "Having problem parsing the binary constant." );
         Ver_ParsePrintErrorMessage( pMan );
         return 0;
      }
      if ( pWord[i] == 'x' ) 
          Vec_PtrPush( pMan->vNames, (void *)0 );
      else 
          Vec_PtrPush( pMan->vNames, (void *)(ABC_PTRUINT_T)(pWord[i]-'0') );
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Parses one directive.]

  Description [The signals are added in the order from LSB to MSB.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_ParseSignal( Ver_Man_t * pMan, Abc_Ntk_t * pNtk, Ver_SignalType_t SigType )
{
    Ver_Stream_t * p = pMan->pReader;
    char Buffer[1000], Symbol, * pWord;
    int nMsb, nLsb, Bit, Limit, i;
    nMsb = nLsb = -1;
    while ( 1 )
    {
        // get the next word
        pWord = Ver_ParseGetName( pMan );
        if ( pWord == NULL )
            return 0;

        if ( !strcmp(pWord, "wire") )
            continue;

        // check if the range is specified
        if ( pWord[0] == '[' && !pMan->fNameLast )
        {
            assert( nMsb == -1 && nLsb == -1 );
            Ver_ParseSignalPrefix( pMan, &pWord, &nMsb, &nLsb );
            // check the case when there is space between bracket and the next word
            if ( *pWord == 0 )
            {
                // get the signal name
                pWord = Ver_ParseGetName( pMan );
                if ( pWord == NULL )
                    return 0;
            }
        }

        // create signals
        if ( nMsb == -1 && nLsb == -1 )
        {
            if ( SigType == VER_SIG_INPUT || SigType == VER_SIG_INOUT )
                Ver_ParseCreatePi( pNtk, pWord );
            if ( SigType == VER_SIG_OUTPUT || SigType == VER_SIG_INOUT )
                Ver_ParseCreatePo( pNtk, pWord );
            if ( SigType == VER_SIG_WIRE || SigType == VER_SIG_REG )
                Abc_NtkFindOrCreateNet( pNtk, pWord );
        }
        else
        {
            assert( nMsb >= 0 && nLsb >= 0 );
            // add to the hash table
            Ver_ParseInsertsSuffix( pMan, pWord, nMsb, nLsb );
            // add signals from Lsb to Msb
            Limit = nMsb > nLsb? nMsb - nLsb + 1: nLsb - nMsb + 1;
            for ( i = 0, Bit = nLsb; i < Limit; i++, Bit = nMsb > nLsb ? Bit + 1: Bit - 1  )
            {
//                sprintf( Buffer, "%s[%d]", pWord, Bit );
                if ( Limit > 1 )
                    sprintf( Buffer, "%s[%d]", pWord, Bit );
                else
                    sprintf( Buffer, "%s", pWord );
                if ( SigType == VER_SIG_INPUT || SigType == VER_SIG_INOUT )
                    Ver_ParseCreatePi( pNtk, Buffer );
                if ( SigType == VER_SIG_OUTPUT || SigType == VER_SIG_INOUT )
                    Ver_ParseCreatePo( pNtk, Buffer );
                if ( SigType == VER_SIG_WIRE || SigType == VER_SIG_REG )
                    Abc_NtkFindOrCreateNet( pNtk, Buffer );
            }
        }

        Symbol = Ver_StreamPopChar(p);
        if ( Symbol == ',' )
            continue;
        if ( Symbol == ';' )
            return 1;
        break;
    }
    sprintf( pMan->sError, "Cannot parse signal line (expected , or ;)." );
    Ver_ParsePrintErrorMessage( pMan );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Parses one directive.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_ParseAlways( Ver_Man_t * pMan, Abc_Ntk_t * pNtk )
{
    Ver_Stream_t * p = pMan->pReader;
    Abc_Obj_t * pNet, * pNet2;
    int fStopAfterOne;
    char * pWord, * pWord2;
    char Symbol;
    // parse the directive 
    pWord = Ver_ParseGetName( pMan );
    if ( pWord == NULL )
        return 0;
    if ( pWord[0] == '@' )
    {
        Ver_StreamSkipToChars( p, ")" );
        Ver_StreamPopChar(p);
        // parse the directive 
        pWord = Ver_ParseGetName( pMan );
        if ( pWord == NULL )
            return 0;
    }
    // decide how many statements to parse
    fStopAfterOne = 0;
    if ( strcmp( pWord, "begin" ) )
        fStopAfterOne = 1;
    // iterate over the initial states
    while ( 1 )
    {
        if ( !fStopAfterOne )
        {
            // get the name of the output signal
            pWord = Ver_ParseGetName( pMan );
            if ( pWord == NULL )
                return 0;
            // look for the end of directive
            if ( !strcmp( pWord, "end" ) )
                break;
        }
        // get the fanout net
        pNet = Ver_ParseFindNet( pNtk, pWord );
        if ( pNet == NULL )
        {
            sprintf( pMan->sError, "Cannot read the always statement for %s (output wire is not defined).", pWord );
            Ver_ParsePrintErrorMessage( pMan );
            return 0;
        }
        // get the equality sign
        Symbol = Ver_StreamPopChar(p);
        if ( Symbol != '<' && Symbol != '=' )
        {
            sprintf( pMan->sError, "Cannot read the assign statement for %s (expected <= or =).", pWord );
            Ver_ParsePrintErrorMessage( pMan );
            return 0;
        }
        if ( Symbol == '<' )
            Ver_StreamPopChar(p);
        // skip the comments
        if ( !Ver_ParseSkipComments( pMan ) )
            return 0;
        // get the second name
        pWord2 = Ver_ParseGetName( pMan );
        if ( pWord2 == NULL )
            return 0;
        // check if the name is complemented
        if ( pWord2[0] == '~' )
        {
            pNet2 = Ver_ParseFindNet( pNtk, pWord2+1 );
            pNet2 = Ver_ParseCreateInv( pNtk, pNet2 );
        }
        else
            pNet2 = Ver_ParseFindNet( pNtk, pWord2 );
        if ( pNet2 == NULL )
        {
            sprintf( pMan->sError, "Cannot read the always statement for %s (input wire is not defined).", pWord2 );
            Ver_ParsePrintErrorMessage( pMan );
            return 0;
        }
        // create the latch
        Ver_ParseCreateLatch( pNtk, pNet2, pNet );
        // remove the last symbol
        Symbol = Ver_StreamPopChar(p);
        assert( Symbol == ';' );
        // quit if only one directive
        if ( fStopAfterOne )
            break;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Parses one directive.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_ParseInitial( Ver_Man_t * pMan, Abc_Ntk_t * pNtk )
{
    Ver_Stream_t * p = pMan->pReader;
    Abc_Obj_t * pNode, * pNet;
    int fStopAfterOne;
    char * pWord, * pEquation;
    char Symbol;
    // parse the directive 
    pWord = Ver_ParseGetName( pMan );
    if ( pWord == NULL )
        return 0;
    // decide how many statements to parse
    fStopAfterOne = 0;
    if ( strcmp( pWord, "begin" ) )
        fStopAfterOne = 1;
    // iterate over the initial states
    while ( 1 )
    {
        if ( !fStopAfterOne )
        {
            // get the name of the output signal
            pWord = Ver_ParseGetName( pMan );
            if ( pWord == NULL )
                return 0;
            // look for the end of directive
            if ( !strcmp( pWord, "end" ) )
                break;
        }
        // get the fanout net
        pNet = Ver_ParseFindNet( pNtk, pWord );
        if ( pNet == NULL )
        {
            sprintf( pMan->sError, "Cannot read the initial statement for %s (output wire is not defined).", pWord );
            Ver_ParsePrintErrorMessage( pMan );
            return 0;
        }
        // get the equality sign
        Symbol = Ver_StreamPopChar(p);
        if ( Symbol != '<' && Symbol != '=' )
        {
            sprintf( pMan->sError, "Cannot read the assign statement for %s (expected <= or =).", pWord );
            Ver_ParsePrintErrorMessage( pMan );
            return 0;
        }
        if ( Symbol == '<' )
            Ver_StreamPopChar(p);
        // skip the comments
        if ( !Ver_ParseSkipComments( pMan ) )
            return 0;
        // get the second name
        pEquation = Ver_StreamGetWord( p, ";" );
        if ( pEquation == NULL )
            return 0;
        // find the corresponding latch
        if ( Abc_ObjFaninNum(pNet) == 0 )
        {
            sprintf( pMan->sError, "Cannot find the latch to assign the initial value." );
            Ver_ParsePrintErrorMessage( pMan );
            return 0;
        }
        pNode = Abc_ObjFanin0(Abc_ObjFanin0(pNet));
        assert( Abc_ObjIsLatch(pNode) );
        // set the initial state
        if ( !strcmp(pEquation, "0") || !strcmp(pEquation, "1\'b0") )
            Abc_LatchSetInit0( pNode );
        else if ( !strcmp(pEquation, "1") || !strcmp(pEquation, "1\'b1") )
            Abc_LatchSetInit1( pNode );
//        else if ( !strcmp(pEquation, "2") )
//            Abc_LatchSetInitDc( pNode );
        else 
        {
            sprintf( pMan->sError, "Incorrect initial value of the latch %s.", Abc_ObjName(pNet) );
            Ver_ParsePrintErrorMessage( pMan );
            return 0;
        }
        // remove the last symbol
        Symbol = Ver_StreamPopChar(p);
        assert( Symbol == ';' );
        // quit if only one directive
        if ( fStopAfterOne )
            break;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Parses one directive.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_ParseAssign( Ver_Man_t * pMan, Abc_Ntk_t * pNtk )
{
    char Buffer[1000], Buffer2[1000];
    Ver_Stream_t * p = pMan->pReader;
    Abc_Obj_t * pNode, * pNet;
    char * pWord, * pName, * pEquation;
    Hop_Obj_t * pFunc;
    char Symbol;
    int i, Bit, Limit, Length, fReduction;
    int nMsb, nLsb;

//    if ( Ver_StreamGetLineNumber(p) == 2756 )
//    {
//        int x = 0;
//    }

    // convert from the blackbox into the network with local functions representated by AIGs
    if ( !Ver_ParseConvertNetwork( pMan, pNtk, pMan->fMapped ) )
        return 0;

    while ( 1 )
    {
        // get the name of the output signal
        pWord = Ver_ParseGetName( pMan );
        if ( pWord == NULL )
            return 0;
        if ( strcmp(pWord, "#1") == 0 )
            continue;
        // check for vector-inputs
        if ( !Ver_ParseLookupSuffix( pMan, pWord, &nMsb, &nLsb ) )
            return 0;
        // handle special case of constant assignment
        Limit = nMsb > nLsb? nMsb - nLsb + 1: nLsb - nMsb + 1;
        if ( nMsb >= 0 && nLsb >= 0 && Limit > 1 )
        {
            // save the fanout name
            if ( !strcmp(pWord, "1\'h0") )
                strcpy( Buffer, "1\'b0" );
            else if ( !strcmp(pWord, "1\'h1") )
                strcpy( Buffer, "1\'b1" );
            else
                strcpy( Buffer, pWord );
            // get the equality sign
            if ( Ver_StreamPopChar(p) != '=' )
            {
                sprintf( pMan->sError, "Cannot read the assign statement for %s (expected equality sign).", pWord );
                Ver_ParsePrintErrorMessage( pMan );
                return 0;
            }
            // get the constant
            pWord = Ver_ParseGetName( pMan );
            if ( pWord == NULL )
                return 0;
            // check if it is indeed a constant
            if ( !(pWord[0] >= '0' && pWord[0] <= '9') )
            {
                sprintf( pMan->sError, "Currently can only assign vector-signal \"%s\" to be a constant.", Buffer );
                Ver_ParsePrintErrorMessage( pMan );
                return 0;
            }

            // get individual bits of the constant
            if ( !Ver_ParseConstant( pMan, pWord ) )
                return 0;
            // check that the constant has the same size
            Limit = nMsb > nLsb? nMsb - nLsb + 1: nLsb - nMsb + 1;
            if ( Limit != Vec_PtrSize(pMan->vNames) )
            {
                sprintf( pMan->sError, "The constant size (%d) is different from the signal\"%s\" size (%d).", 
                    Vec_PtrSize(pMan->vNames), Buffer, Limit );
                Ver_ParsePrintErrorMessage( pMan );
                return 0;
            }
            // iterate through the bits
            for ( i = 0, Bit = nLsb; i < Limit; i++, Bit = nMsb > nLsb ? Bit + 1: Bit - 1  )
            {
                // get the fanin net
                if ( Vec_PtrEntry( pMan->vNames, Limit-1-i ) )
                    pNet = Ver_ParseFindNet( pNtk, "1\'b1" );
                else
                    pNet = Ver_ParseFindNet( pNtk, "1\'b0" );
                assert( pNet != NULL );

                // create the buffer
                pNode = Abc_NtkCreateNodeBuf( pNtk, pNet );

                // get the fanout net
                sprintf( Buffer2, "%s[%d]", Buffer, Bit );
                pNet = Ver_ParseFindNet( pNtk, Buffer2 );
                if ( pNet == NULL )
                {
                    sprintf( pMan->sError, "Cannot read the assign statement for %s (output wire is not defined).", pWord );
                    Ver_ParsePrintErrorMessage( pMan );
                    return 0;
                }
                Abc_ObjAddFanin( pNet, pNode );
            }
            // go to the end of the line
            Ver_ParseSkipComments( pMan );
        }
        else
        {
            // consider the case of reduction operations
            fReduction = 0;
            if ( pWord[0] == '{' && !pMan->fNameLast )
                fReduction = 1;
            if ( fReduction )
            {
                pWord++;
                pWord[strlen(pWord)-1] = 0;
                assert( pWord[0] != '\\' );
            }
            // get the fanout net
            pNet = Ver_ParseFindNet( pNtk, pWord );
            if ( pNet == NULL )
            {
                sprintf( pMan->sError, "Cannot read the assign statement for %s (output wire is not defined).", pWord );
                Ver_ParsePrintErrorMessage( pMan );
                return 0;
            }
            // get the equality sign
            if ( Ver_StreamPopChar(p) != '=' )
            {
                sprintf( pMan->sError, "Cannot read the assign statement for %s (expected equality sign).", pWord );
                Ver_ParsePrintErrorMessage( pMan );
                return 0;
            }
            // skip the comments
            if ( !Ver_ParseSkipComments( pMan ) )
                return 0;
            // get the second name
            if ( fReduction )
                pEquation = Ver_StreamGetWord( p, ";" );
            else
                pEquation = Ver_StreamGetWord( p, ",;" );
            if ( pEquation == NULL )
            {
                sprintf( pMan->sError, "Cannot read the equation for %s.", Abc_ObjName(pNet) );
                Ver_ParsePrintErrorMessage( pMan );
                return 0;
            }

            // consider the case of mapped network
            Vec_PtrClear( pMan->vNames );
            if ( pMan->fMapped )
            {
                if ( !strcmp( pEquation, "1\'b0" ) )
                    pFunc = (Hop_Obj_t *)Mio_LibraryReadConst0((Mio_Library_t *)Abc_FrameReadLibGen());
                else if ( !strcmp( pEquation, "1\'b1" ) )
                    pFunc = (Hop_Obj_t *)Mio_LibraryReadConst1((Mio_Library_t *)Abc_FrameReadLibGen());
                else
                {
                    // "assign foo = \bar ;"
                    if ( *pEquation == '\\' )
                    {
                        pEquation++;
                        pEquation[strlen(pEquation) - 1] = 0;
                    }
                    if ( Ver_ParseFindNet(pNtk, pEquation) == NULL )
                    {
                        sprintf( pMan->sError, "Cannot read Verilog with non-trivial assignments in the mapped netlist." );
                        Ver_ParsePrintErrorMessage( pMan );
                        return 0;
                    }
                    Vec_PtrPush( pMan->vNames, (void *)(ABC_PTRUINT_T)strlen(pEquation) );
                    Vec_PtrPush( pMan->vNames, pEquation );
                    // get the buffer
                    pFunc = (Hop_Obj_t *)Mio_LibraryReadBuf((Mio_Library_t *)Abc_FrameReadLibGen());
                    if ( pFunc == NULL )
                    {
                        sprintf( pMan->sError, "Reading assign statement for node %s has failed because the genlib library has no buffer.", Abc_ObjName(pNet) );
                        Ver_ParsePrintErrorMessage( pMan );
                        return 0;
                    }
                }
            }
            else
            {
                if ( !strcmp(pEquation, "0") || !strcmp(pEquation, "1\'b0") || !strcmp(pEquation, "1\'bx") )
                    pFunc = Hop_ManConst0((Hop_Man_t *)pNtk->pManFunc);
                else if ( !strcmp(pEquation, "1") || !strcmp(pEquation, "1\'b1") )
                    pFunc = Hop_ManConst1((Hop_Man_t *)pNtk->pManFunc);
                else if ( fReduction )
                    pFunc = (Hop_Obj_t *)Ver_FormulaReduction( pEquation, pNtk->pManFunc, pMan->vNames, pMan->sError );  
                else
                    pFunc = (Hop_Obj_t *)Ver_FormulaParser( pEquation, pNtk->pManFunc, pMan->vNames, pMan->vStackFn, pMan->vStackOp, pMan->sError );  
                if ( pFunc == NULL )
                {
                    Ver_ParsePrintErrorMessage( pMan );
                    return 0;
                }
            }

            // create the node with the given inputs
            pNode = Abc_NtkCreateNode( pNtk );
            pNode->pData = pFunc;
            Abc_ObjAddFanin( pNet, pNode );
            // connect to fanin nets
            for ( i = 0; i < Vec_PtrSize(pMan->vNames)/2; i++ )
            {
                // get the name of this signal
                Length = (int)(ABC_PTRUINT_T)Vec_PtrEntry( pMan->vNames, 2*i );
                pName  = (char *)Vec_PtrEntry( pMan->vNames, 2*i + 1 );
                pName[Length] = 0;
                // try name
//                pNet = Ver_ParseFindNet( pNtk, pName );
                if ( !strcmp(pName, "1\'h0") )
                    pNet = Ver_ParseFindNet( pNtk, "1\'b0" );
                else if ( !strcmp(pName, "1\'h1") )
                    pNet = Ver_ParseFindNet( pNtk, "1\'b1" );
                else
                    pNet = Ver_ParseFindNet( pNtk, pName );
                // find the corresponding net
                if ( pNet == NULL )
                {
                    sprintf( pMan->sError, "Cannot read the assign statement for %s (input wire %s is not defined).", pWord, pName );
                    Ver_ParsePrintErrorMessage( pMan );
                    return 0;
                }
                Abc_ObjAddFanin( pNode, pNet );
            }
        }

        Symbol = Ver_StreamPopChar(p);
        if ( Symbol == ',' )
            continue;
        if ( Symbol == ';' )
            return 1;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Parses one directive.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_ParseGateStandard( Ver_Man_t * pMan, Abc_Ntk_t * pNtk, Ver_GateType_t GateType )
{
    Ver_Stream_t * p = pMan->pReader;
    Abc_Obj_t * pNet, * pNode;
    char * pWord, Symbol;

    // convert from the blackbox into the network with local functions representated by AIGs
    if ( !Ver_ParseConvertNetwork( pMan, pNtk, pMan->fMapped ) )
        return 0;

    // this is gate name - throw it away
    if ( Ver_StreamPopChar(p) != '(' )
    {
        sprintf( pMan->sError, "Cannot parse a standard gate (expected opening parenthesis)." );
        Ver_ParsePrintErrorMessage( pMan );
        return 0;
    }
    Ver_ParseSkipComments( pMan );

    // create the node
    pNode = Abc_NtkCreateNode( pNtk );

    // parse pairs of formal/actural inputs
    while ( 1 )
    {
        // parse the output name
        pWord = Ver_ParseGetName( pMan );
        if ( pWord == NULL )
            return 0;
        // get the net corresponding to this output
        pNet = Ver_ParseFindNet( pNtk, pWord );
        if ( pNet == NULL )
        {
            sprintf( pMan->sError, "Net is missing in gate %s.", pWord );
            Ver_ParsePrintErrorMessage( pMan );
            return 0;
        }
        // if this is the first net, add it as an output
        if ( Abc_ObjFanoutNum(pNode) == 0 )
            Abc_ObjAddFanin( pNet, pNode );
        else
            Abc_ObjAddFanin( pNode, pNet );
        // check if it is the end of gate
        Ver_ParseSkipComments( pMan );
        Symbol = Ver_StreamPopChar(p);
        if ( Symbol == ')' )
            break;
        // skip comma
        if ( Symbol != ',' )
        {
            sprintf( pMan->sError, "Cannot parse a standard gate %s (expected closing parenthesis).", Abc_ObjName(Abc_ObjFanout0(pNode)) );
            Ver_ParsePrintErrorMessage( pMan );
            return 0;
        }
        Ver_ParseSkipComments( pMan );
    }
    if ( (GateType == VER_GATE_BUF || GateType == VER_GATE_NOT) && Abc_ObjFaninNum(pNode) != 1 )
    {
        sprintf( pMan->sError, "Buffer or interver with multiple fanouts %s (currently not supported).", Abc_ObjName(Abc_ObjFanout0(pNode)) );
        Ver_ParsePrintErrorMessage( pMan );
        return 0;
    }

    // check if it is the end of gate
    Ver_ParseSkipComments( pMan );
    if ( Ver_StreamPopChar(p) != ';' )
    {
        sprintf( pMan->sError, "Cannot read standard gate %s (expected closing semicolumn).", Abc_ObjName(Abc_ObjFanout0(pNode)) );
        Ver_ParsePrintErrorMessage( pMan );
        return 0;
    }
    // add logic function
    if ( GateType == VER_GATE_AND || GateType == VER_GATE_NAND )
        pNode->pData = Hop_CreateAnd( (Hop_Man_t *)pNtk->pManFunc, Abc_ObjFaninNum(pNode) );
    else if ( GateType == VER_GATE_OR || GateType == VER_GATE_NOR )
        pNode->pData = Hop_CreateOr( (Hop_Man_t *)pNtk->pManFunc, Abc_ObjFaninNum(pNode) );
    else if ( GateType == VER_GATE_XOR || GateType == VER_GATE_XNOR )
        pNode->pData = Hop_CreateExor( (Hop_Man_t *)pNtk->pManFunc, Abc_ObjFaninNum(pNode) );
    else if ( GateType == VER_GATE_BUF || GateType == VER_GATE_NOT )
        pNode->pData = Hop_CreateAnd( (Hop_Man_t *)pNtk->pManFunc, Abc_ObjFaninNum(pNode) );
    if ( GateType == VER_GATE_NAND || GateType == VER_GATE_NOR || GateType == VER_GATE_XNOR || GateType == VER_GATE_NOT )
        pNode->pData = Hop_Not( (Hop_Obj_t *)pNode->pData );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Parses one directive.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_ParseFlopStandard( Ver_Man_t * pMan, Abc_Ntk_t * pNtk )
{
    Ver_Stream_t * p = pMan->pReader;
    Abc_Obj_t * pNetLi, * pNetLo, * pLatch;
    char * pWord, Symbol;

    // convert from the blackbox into the network with local functions representated by AIGs
    if ( !Ver_ParseConvertNetwork( pMan, pNtk, pMan->fMapped ) )
        return 0;

    // this is gate name - throw it away
    if ( Ver_StreamPopChar(p) != '(' )
    {
        sprintf( pMan->sError, "Cannot parse a standard gate (expected opening parenthesis)." );
        Ver_ParsePrintErrorMessage( pMan );
        return 0;
    }
    Ver_ParseSkipComments( pMan );

    // parse the output name
    pWord = Ver_ParseGetName( pMan );
    if ( pWord == NULL )
        return 0;
    // get the net corresponding to this output
    pNetLo = Ver_ParseFindNet( pNtk, pWord );
    if ( pNetLo == NULL )
    {
        sprintf( pMan->sError, "Net is missing in gate %s.", pWord );
        Ver_ParsePrintErrorMessage( pMan );
        return 0;
    }

    // check if it is the end of gate
    Ver_ParseSkipComments( pMan );
    Symbol = Ver_StreamPopChar(p);
    if ( Symbol == ')' )
    {
        sprintf( pMan->sError, "Cannot parse the flop." );
        Ver_ParsePrintErrorMessage( pMan );
        return 0;
    }
    // skip comma
    if ( Symbol != ',' )
    {
        sprintf( pMan->sError, "Cannot parse the flop." );
        Ver_ParsePrintErrorMessage( pMan );
        return 0;
    }
    Ver_ParseSkipComments( pMan );

    // parse the output name
    pWord = Ver_ParseGetName( pMan );
    if ( pWord == NULL )
        return 0;
    // get the net corresponding to this output
    pNetLi = Ver_ParseFindNet( pNtk, pWord );
    if ( pNetLi == NULL )
    {
        sprintf( pMan->sError, "Net is missing in gate %s.", pWord );
        Ver_ParsePrintErrorMessage( pMan );
        return 0;
    }

    // check if it is the end of gate
    Ver_ParseSkipComments( pMan );
    Symbol = Ver_StreamPopChar(p);
    if ( Symbol != ')' )
    {
        sprintf( pMan->sError, "Cannot parse the flop." );
        Ver_ParsePrintErrorMessage( pMan );
        return 0;
    }

    // check if it is the end of gate
    Ver_ParseSkipComments( pMan );
    if ( Ver_StreamPopChar(p) != ';' )
    {
        sprintf( pMan->sError, "Cannot parse the flop." );
        Ver_ParsePrintErrorMessage( pMan );
        return 0;
    }

    // create the latch
    pLatch = Ver_ParseCreateLatch( pNtk, pNetLi, pNetLo );
    Abc_LatchSetInit0( pLatch );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns the index of the given pin the gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_FindGateInput( Mio_Gate_t * pGate, char * pName )
{
    Mio_Pin_t * pGatePin;
    int i;
    for ( i = 0, pGatePin = Mio_GateReadPins(pGate); pGatePin != NULL; pGatePin = Mio_PinReadNext(pGatePin), i++ )
        if ( strcmp(pName, Mio_PinReadName(pGatePin)) == 0 )
            return i;
    if ( strcmp(pName, Mio_GateReadOutName(pGate)) == 0 )
        return i;
    if ( Mio_GateReadTwin(pGate) && strcmp(pName, Mio_GateReadOutName(Mio_GateReadTwin(pGate))) == 0 )
        return i+1;
    return -1;
}

/**Function*************************************************************

  Synopsis    [Parses one directive.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_ParseGate( Ver_Man_t * pMan, Abc_Ntk_t * pNtk, Mio_Gate_t * pGate )
{
    Ver_Stream_t * p = pMan->pReader;
    Abc_Obj_t * pNetActual, * pNode, * pNode2 = NULL;
    char * pWord, Symbol;
    int Input, i, nFanins = Mio_GateReadPinNum(pGate);

    // convert from the blackbox into the network with local functions representated by gates
    if ( 1 != pMan->fMapped )
    {
        sprintf( pMan->sError, "The network appears to be mapped. Use \"r -m\" to read mapped Verilog." );
        Ver_ParsePrintErrorMessage( pMan );
        return 0;
    }

    // update the network type if needed
    if ( !Ver_ParseConvertNetwork( pMan, pNtk, 1 ) )
        return 0;

    // parse the directive and set the pointers to the PIs/POs of the gate
    pWord = Ver_ParseGetName( pMan );
    if ( pWord == NULL )
        return 0;
    // this is gate name - throw it away
    if ( Ver_StreamPopChar(p) != '(' )
    {
        sprintf( pMan->sError, "Cannot parse gate %s (expected opening parenthesis).", Mio_GateReadName(pGate) );
        Ver_ParsePrintErrorMessage( pMan );
        return 0;
    }
    Ver_ParseSkipComments( pMan );

    // start the node
    pNode = Abc_NtkCreateNode( pNtk );
    pNode->pData = pGate;
    if ( Mio_GateReadTwin(pGate) )
    {
        pNode2 = Abc_NtkCreateNode( pNtk );
        pNode2->pData = Mio_GateReadTwin(pGate);
    }
    // parse pairs of formal/actural inputs
    Vec_IntClear( pMan->vPerm );
    while ( 1 )
    {
        // process one pair of formal/actual parameters
        if ( Ver_StreamPopChar(p) != '.' )
        {
            sprintf( pMan->sError, "Cannot parse gate %s (expected .).", Mio_GateReadName(pGate) );
            Ver_ParsePrintErrorMessage( pMan );
            return 0;
        }

        // parse the formal name
        pWord = Ver_ParseGetName( pMan );
        if ( pWord == NULL )
            return 0;

        // find the corresponding pin of the gate
        Input = Ver_FindGateInput( pGate, pWord );
        if ( Input == -1 )
        {
            sprintf( pMan->sError, "Formal input name %s cannot be found in the gate %s.", pWord, Mio_GateReadOutName(pGate) );
            Ver_ParsePrintErrorMessage( pMan );
            return 0;
        }

        // open the parenthesis
        if ( Ver_StreamPopChar(p) != '(' )
        {
            sprintf( pMan->sError, "Cannot formal parameter %s of gate %s (expected opening parenthesis).", pWord, Mio_GateReadName(pGate) );
            Ver_ParsePrintErrorMessage( pMan );
            return 0;
        }

        // parse the actual name
        pWord = Ver_ParseGetName( pMan );
        if ( pWord == NULL )
            return 0;
        // check if the name is complemented
        assert( pWord[0] != '~' );
/*
        fCompl = (pWord[0] == '~');
        if ( fCompl )
        {
            fComplUsed = 1;
            pWord++;
            if ( pNtk->pData == NULL )
                pNtk->pData = Extra_MmFlexStart();
        }
*/
        // get the actual net
        pNetActual = Ver_ParseFindNet( pNtk, pWord );
        if ( pNetActual == NULL )
        {
            sprintf( pMan->sError, "Actual net %s is missing.", pWord );
            Ver_ParsePrintErrorMessage( pMan );
            return 0;
        }

        // close the parenthesis
        if ( Ver_StreamPopChar(p) != ')' )
        {
            sprintf( pMan->sError, "Cannot formal parameter %s of gate %s (expected closing parenthesis).", pWord, Mio_GateReadName(pGate) );
            Ver_ParsePrintErrorMessage( pMan );
            return 0;
        }

        // add the fanin
        if ( Input < nFanins )
        {
            Vec_IntPush( pMan->vPerm, Input );
            Abc_ObjAddFanin( pNode, pNetActual ); // fanin
            if ( pNode2 )
                Abc_ObjAddFanin( pNode2, pNetActual ); // fanin
        }
        else if ( Input == nFanins )
            Abc_ObjAddFanin( pNetActual, pNode ); // fanout
        else if ( Input == nFanins + 1 )
            Abc_ObjAddFanin( pNetActual, pNode2 ); // fanout
        else
            assert( 0 );

        // check if it is the end of gate
        Ver_ParseSkipComments( pMan );
        Symbol = Ver_StreamPopChar(p);
        if ( Symbol == ')' )
            break;

        // skip comma
        if ( Symbol != ',' )
        {
            sprintf( pMan->sError, "Cannot formal parameter %s of gate %s (expected closing parenthesis).", pWord, Mio_GateReadName(pGate) );
            Ver_ParsePrintErrorMessage( pMan );
            return 0;
        }
        Ver_ParseSkipComments( pMan );
    }

    // check that the gate as the same number of input
    if ( !(Abc_ObjFaninNum(pNode) == nFanins && Abc_ObjFanoutNum(pNode) == 1) )
    {
        sprintf( pMan->sError, "Parsing of gate %s has failed.", Mio_GateReadName(pGate) );
        Ver_ParsePrintErrorMessage( pMan );
        return 0;
    }

    // check if it is the end of gate
    Ver_ParseSkipComments( pMan );
    if ( Ver_StreamPopChar(p) != ';' )
    {
        sprintf( pMan->sError, "Cannot read gate %s (expected closing semicolumn).", Mio_GateReadName(pGate) );
        Ver_ParsePrintErrorMessage( pMan );
        return 0;
    }
    
    // check if we need to permute the inputs
    Vec_IntForEachEntry( pMan->vPerm, Input, i )
        if ( Input != i )
            break;
    if ( i < Vec_IntSize(pMan->vPerm) )
    {
        // add the fanin numnbers to the end of the permuation array
        for ( i = 0; i < nFanins; i++ )
            Vec_IntPush( pMan->vPerm, Abc_ObjFaninId(pNode, i) );
        // write the fanin numbers into their corresponding places (according to the gate) 
        for ( i = 0; i < nFanins; i++ )
            Vec_IntWriteEntry( &pNode->vFanins, Vec_IntEntry(pMan->vPerm, i), Vec_IntEntry(pMan->vPerm, i+nFanins) );
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Parses one directive.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_ParseBox( Ver_Man_t * pMan, Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkBox )
{
    char Buffer[1000];
    Ver_Stream_t * p = pMan->pReader;
    Ver_Bundle_t * pBundle;
    Vec_Ptr_t * vBundles;
    Abc_Obj_t * pNetActual; 
    Abc_Obj_t * pNode;
    char * pWord, Symbol;
    int fCompl, fFormalIsGiven;
    int i, k, Bit, Limit, nMsb, nLsb, fQuit, flag;

    // gate the name of the box
    pWord = Ver_ParseGetName( pMan );
    if ( pWord == NULL )
        return 0;

    // create a box with this name
    pNode = Abc_NtkCreateBlackbox( pNtk );
    pNode->pData = pNtkBox;
    Abc_ObjAssignName( pNode, pWord, NULL );

    // continue parsing the box
    if ( Ver_StreamPopChar(p) != '(' )
    {
        sprintf( pMan->sError, "Cannot parse box %s (expected opening parenthesis).", Abc_ObjName(pNode) );
        Ver_ParsePrintErrorMessage( pMan );
        return 0;
    }
    Ver_ParseSkipComments( pMan );
 
    // parse pairs of formal/actual inputs
    vBundles = Vec_PtrAlloc( 16 );
    pNode->pCopy = (Abc_Obj_t *)vBundles;
    while ( 1 )
    {
        // allocate the bundle (formal name + array of actual nets)
        pBundle = ABC_ALLOC( Ver_Bundle_t, 1 );
        pBundle->pNameFormal = NULL;
        pBundle->vNetsActual = Vec_PtrAlloc( 4 );
        Vec_PtrPush( vBundles, pBundle );

        // process one pair of formal/actual parameters
        fFormalIsGiven = 0;
        if ( Ver_StreamScanChar(p) == '.' )
        {
            fFormalIsGiven = 1;
            if ( Ver_StreamPopChar(p) != '.' )
            {
                sprintf( pMan->sError, "Cannot parse box %s (expected .).", Abc_ObjName(pNode) );
                Ver_ParsePrintErrorMessage( pMan );
                return 0;
            }

            // parse the formal name
            pWord = Ver_ParseGetName( pMan );
            if ( pWord == NULL )
                return 0;

            // save the name
            pBundle->pNameFormal = Extra_UtilStrsav( pWord );

            // open the parenthesis
            if ( Ver_StreamPopChar(p) != '(' )
            {
                sprintf( pMan->sError, "Cannot formal parameter %s of box %s (expected opening parenthesis).", pWord, Abc_ObjName(pNode));
                Ver_ParsePrintErrorMessage( pMan );
                return 0;
            }
            Ver_ParseSkipComments( pMan );
        }

        // check if this is the beginning of {} expression
        Symbol = Ver_StreamScanChar(p);

        // consider the case of vector-inputs
        if ( Symbol == '{' )
        {
            // skip this char
            Ver_StreamPopChar(p);

            // read actual names
            i = 0;
            fQuit = 0;
            while ( 1 )
            {
                // parse the formal name
                Ver_ParseSkipComments( pMan );
                pWord = Ver_ParseGetName( pMan );
                if ( pWord == NULL )
                    return 0;

                // check if the last char is a closing brace
                if ( pWord[strlen(pWord)-1] == '}' )
                {
                    pWord[strlen(pWord)-1] = 0;
                    fQuit = 1;
                }
                if ( pWord[0] == 0 )
                    break;

                // check for constant
                if ( pWord[0] >= '1' && pWord[0] <= '9' )
                {
                    if ( !Ver_ParseConstant( pMan, pWord ) )
                        return 0;
                    // add constant MSB to LSB
                    for ( k = 0; k < Vec_PtrSize(pMan->vNames); k++, i++ )
                    {
                        // get the actual net
                        sprintf( Buffer, "1\'b%d", (int)(Vec_PtrEntry(pMan->vNames,k) != NULL) );
                        pNetActual = Ver_ParseFindNet( pNtk, Buffer );
                        if ( pNetActual == NULL )
                        {
                            sprintf( pMan->sError, "Actual net \"%s\" is missing in gate \"%s\".", Buffer, Abc_ObjName(pNode) );
                            Ver_ParsePrintErrorMessage( pMan );
                            return 0;
                        }
                        Vec_PtrPush( pBundle->vNetsActual, pNetActual );
                    }
                }
                else
                {
                    // get the suffix of the form [m:n]
                    if ( pWord[strlen(pWord)-1] == ']' && !pMan->fNameLast )
                        Ver_ParseSignalSuffix( pMan, pWord, &nMsb, &nLsb );
                    else
                        Ver_ParseLookupSuffix( pMan, pWord, &nMsb, &nLsb );

                    // generate signals
                    if ( nMsb == -1 && nLsb == -1 )
                    {
                        // get the actual net
                        pNetActual = Ver_ParseFindNet( pNtk, pWord );
                        if ( pNetActual == NULL )
                        {
                            if ( !strncmp(pWord, "Open_", 5) ||
                                !strncmp(pWord, "dct_unconnected", 15) ) 
                                pNetActual = Abc_NtkCreateNet( pNtk );
                            else
                            {
                                sprintf( pMan->sError, "Actual net \"%s\" is missing in box \"%s\".", pWord, Abc_ObjName(pNode) );
                                Ver_ParsePrintErrorMessage( pMan );
                                return 0;
                            }
                        }
                        Vec_PtrPush( pBundle->vNetsActual, pNetActual );
                        i++;
                    }
                    else
                    {
                        // go from MSB to LSB
                        assert( nMsb >= 0 && nLsb >= 0 );
                        Limit = (nMsb > nLsb) ? nMsb - nLsb + 1: nLsb - nMsb + 1;  
                        for ( Bit = nMsb, k = Limit - 1; k >= 0; Bit = (nMsb > nLsb ? Bit - 1: Bit + 1), k--, i++ )
                        {
                            // get the actual net
                            sprintf( Buffer, "%s[%d]", pWord, Bit );
                            pNetActual = Ver_ParseFindNet( pNtk, Buffer );
                            if ( pNetActual == NULL )
                            {
                                if ( !strncmp(pWord, "Open_", 5) ||
                                    !strncmp(pWord, "dct_unconnected", 15) ) 
                                    pNetActual = Abc_NtkCreateNet( pNtk );
                                else
                                {
                                    sprintf( pMan->sError, "Actual net \"%s\" is missing in box \"%s\".", pWord, Abc_ObjName(pNode) );
                                    Ver_ParsePrintErrorMessage( pMan );
                                    return 0;
                                }
                            }
                            Vec_PtrPush( pBundle->vNetsActual, pNetActual );
                        }
                    }
                }

                if ( fQuit )
                    break;

                // skip comma
                Ver_ParseSkipComments( pMan );
                Symbol = Ver_StreamPopChar(p);
                if ( Symbol == '}' )
                    break;
                if ( Symbol != ',' )
                {
                    sprintf( pMan->sError, "Cannot parse formal parameter %s of gate %s (expected comma).", pWord, Abc_ObjName(pNode) );
                    Ver_ParsePrintErrorMessage( pMan );
                    return 0;
                }
            }
        }
        else
        {
            // get the next word
            pWord = Ver_ParseGetName( pMan );
            if ( pWord == NULL )
                return 0;
            // consider the case of empty name
            fCompl = 0;
            if ( pWord[0] == 0 )
            {
                pNetActual = Abc_NtkCreateNet( pNtk );
                Vec_PtrPush( pBundle->vNetsActual, Abc_ObjNotCond( pNetActual, fCompl ) );
            }
            else
            {
                // get the actual net
                flag=0;
                pNetActual = Ver_ParseFindNet( pNtk, pWord );
                if ( pNetActual == NULL ) 
                {
                    Ver_ParseLookupSuffix( pMan, pWord, &nMsb, &nLsb );
                    if ( nMsb == -1 && nLsb == -1 ) 
                    {
                        Ver_ParseSignalSuffix( pMan, pWord, &nMsb, &nLsb );
                        if ( nMsb == -1 && nLsb == -1 ) 
                        {
                            if ( !strncmp(pWord, "Open_", 5) ||
                                !strncmp(pWord, "dct_unconnected", 15) ) 
                            {
                                pNetActual = Abc_NtkCreateNet( pNtk );
                                Vec_PtrPush( pBundle->vNetsActual, pNetActual );
                            } 
                            else 
                            {
                                sprintf( pMan->sError, "Actual net \"%s\" is missing in box \"%s\".", pWord, Abc_ObjName(pNode) );
                                Ver_ParsePrintErrorMessage( pMan );
                                return 0;
                            }
                        } 
                        else 
                        {
                            flag=1;
                        }
                    } 
                    else 
                    {
                        flag=1;
                    }
                    if (flag) 
                    {
                        Limit = (nMsb > nLsb) ? nMsb - nLsb + 1: nLsb - nMsb + 1;  
                        for ( Bit = nMsb, k = Limit - 1; k >= 0; Bit = (nMsb > nLsb ? Bit - 1: Bit + 1), k--)
                        {
                            // get the actual net
                            sprintf( Buffer, "%s[%d]", pWord, Bit );
                            pNetActual = Ver_ParseFindNet( pNtk, Buffer );
                            if ( pNetActual == NULL )
                            {
                                if ( !strncmp(pWord, "Open_", 5) ||
                                    !strncmp(pWord, "dct_unconnected", 15)) 
                                    pNetActual = Abc_NtkCreateNet( pNtk );
                                else
                                {
                                    sprintf( pMan->sError, "Actual net \"%s\" is missing in box \"%s\".", pWord, Abc_ObjName(pNode) );
                                    Ver_ParsePrintErrorMessage( pMan );
                                    return 0;
                                }
                            }
                            Vec_PtrPush( pBundle->vNetsActual, pNetActual );
                        }
                    }
                } 
                else 
                {
                    Vec_PtrPush( pBundle->vNetsActual, Abc_ObjNotCond( pNetActual, fCompl ) );
                }
            }
        }

        if ( fFormalIsGiven )
        {
            // close the parenthesis
            Ver_ParseSkipComments( pMan );
            if ( Ver_StreamPopChar(p) != ')' )
            {
                sprintf( pMan->sError, "Cannot parse formal parameter %s of box %s (expected closing parenthesis).", pWord, Abc_ObjName(pNode) );
                Ver_ParsePrintErrorMessage( pMan );
                return 0;
            }
            Ver_ParseSkipComments( pMan );
        }

        // check if it is the end of gate
        Symbol = Ver_StreamPopChar(p);
        if ( Symbol == ')' )
            break;
        // skip comma
        if ( Symbol != ',' )
        {
            sprintf( pMan->sError, "Cannot parse formal parameter %s of box %s (expected comma).", pWord, Abc_ObjName(pNode) );
            Ver_ParsePrintErrorMessage( pMan );
            return 0;
        }
        Ver_ParseSkipComments( pMan );
    }

    // check if it is the end of gate
    Ver_ParseSkipComments( pMan );
    if ( Ver_StreamPopChar(p) != ';' )
    {
        sprintf( pMan->sError, "Cannot read box %s (expected closing semicolumn).", Abc_ObjName(pNode) );
        Ver_ParsePrintErrorMessage( pMan );
        return 0;
    }

    return 1;
}

/**Function*************************************************************

  Synopsis    [Connects one box to the network]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ver_ParseFreeBundle( Ver_Bundle_t * pBundle )
{
    ABC_FREE( pBundle->pNameFormal );
    Vec_PtrFree( pBundle->vNetsActual );
    ABC_FREE( pBundle );
}

/**Function*************************************************************

  Synopsis    [Connects one box to the network]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_ParseConnectBox( Ver_Man_t * pMan, Abc_Obj_t * pBox )
{
    Vec_Ptr_t * vBundles = (Vec_Ptr_t *)pBox->pCopy;
    Abc_Ntk_t * pNtk = pBox->pNtk;
    Abc_Ntk_t * pNtkBox = (Abc_Ntk_t *)pBox->pData;
    Abc_Obj_t * pTerm, * pTermNew, * pNetAct;
    Ver_Bundle_t * pBundle;
    char * pNameFormal;
    int i, k, j, iBundle, Length;

    assert( !Ver_ObjIsConnected(pBox) );
    assert( Ver_NtkIsDefined(pNtkBox) );
    assert( !Abc_NtkHasBlackbox(pNtkBox) || Abc_NtkBoxNum(pNtkBox) == 1 );

/*
    // clean the PI/PO nets
    Abc_NtkForEachPi( pNtkBox, pTerm, i )
        Abc_ObjFanout0(pTerm)->pCopy = NULL;
    Abc_NtkForEachPo( pNtkBox, pTerm, i )
        Abc_ObjFanin0(pTerm)->pCopy = NULL;
*/

    // check the number of actual nets is the same as the number of formal nets
    if ( Vec_PtrSize(vBundles) > Abc_NtkPiNum(pNtkBox) + Abc_NtkPoNum(pNtkBox) )
    {
        sprintf( pMan->sError, "The number of actual IOs (%d) is bigger than the number of formal IOs (%d) when instantiating network %s in box %s.", 
            Vec_PtrSize(vBundles), Abc_NtkPiNum(pNtkBox) + Abc_NtkPoNum(pNtkBox), pNtkBox->pName, Abc_ObjName(pBox) );
        // free the bundling
        Vec_PtrForEachEntry( Ver_Bundle_t *, vBundles, pBundle, k )
            Ver_ParseFreeBundle( pBundle );
        Vec_PtrFree( vBundles );
        pBox->pCopy = NULL;
        Ver_ParsePrintErrorMessage( pMan );
        return 0;
    }

    // check if some of them do not have formal names
    Vec_PtrForEachEntry( Ver_Bundle_t *, vBundles, pBundle, k )
        if ( pBundle->pNameFormal == NULL )
            break;
    if ( k < Vec_PtrSize(vBundles) )
    {
        printf( "Warning: The instance %s of network %s will be connected without using formal names.\n", pNtkBox->pName, Abc_ObjName(pBox) );
        // add all actual nets in the bundles
        iBundle = 0;
        Vec_PtrForEachEntry( Ver_Bundle_t *, vBundles, pBundle, j )
            iBundle += Vec_PtrSize(pBundle->vNetsActual);

        // check the number of actual nets is the same as the number of formal nets
        if ( iBundle != Abc_NtkPiNum(pNtkBox) + Abc_NtkPoNum(pNtkBox) )
        {
            sprintf( pMan->sError, "The number of actual IOs (%d) is different from the number of formal IOs (%d) when instantiating network %s in box %s.", 
                Vec_PtrSize(vBundles), Abc_NtkPiNum(pNtkBox) + Abc_NtkPoNum(pNtkBox), pNtkBox->pName, Abc_ObjName(pBox) );
            // free the bundling
            Vec_PtrForEachEntry( Ver_Bundle_t *, vBundles, pBundle, k )
                Ver_ParseFreeBundle( pBundle );
            Vec_PtrFree( vBundles );
            pBox->pCopy = NULL;
            Ver_ParsePrintErrorMessage( pMan );
            return 0;
        }
        // connect bundles in the natural order
        iBundle = 0;
        Abc_NtkForEachPi( pNtkBox, pTerm, i )
        {
            pBundle = (Ver_Bundle_t *)Vec_PtrEntry( vBundles, iBundle++ );
            // the bundle is found - add the connections - using order LSB to MSB
            Vec_PtrForEachEntryReverse( Abc_Obj_t *, pBundle->vNetsActual, pNetAct, k )
            {
                pTermNew = Abc_NtkCreateBi( pNtk );
                Abc_ObjAddFanin( pBox, pTermNew );
                Abc_ObjAddFanin( pTermNew, pNetAct );
                i++;
            }
            i--;
        }
        // create fanins of the box
        Abc_NtkForEachPo( pNtkBox, pTerm, i )
        {
            pBundle = (Ver_Bundle_t *)Vec_PtrEntry( vBundles, iBundle++ );
            // the bundle is found - add the connections - using order LSB to MSB
            Vec_PtrForEachEntryReverse( Abc_Obj_t *, pBundle->vNetsActual, pNetAct, k )
            {
                pTermNew = Abc_NtkCreateBo( pNtk );
                Abc_ObjAddFanin( pTermNew, pBox );
                Abc_ObjAddFanin( pNetAct, pTermNew );
                i++;
            }
            i--;
        }

        // free the bundling
        Vec_PtrForEachEntry( Ver_Bundle_t *, vBundles, pBundle, k )
            Ver_ParseFreeBundle( pBundle );
        Vec_PtrFree( vBundles );
        pBox->pCopy = NULL;
        return 1;
    }

    // bundles arrive in any order - but inside each bundle the order is MSB to LSB
    // make sure every formal PI has a corresponding net
    Abc_NtkForEachPi( pNtkBox, pTerm, i )
    {
        // get the name of this formal net
        pNameFormal = Abc_ObjName( Abc_ObjFanout0(pTerm) );
        // try to find the bundle with this formal net
        pBundle = NULL;
        Vec_PtrForEachEntry( Ver_Bundle_t *, vBundles, pBundle, k )
            if ( !strcmp(pBundle->pNameFormal, pNameFormal) )
                break;
        assert( pBundle != NULL );
        // if the bundle is not found, try without parentheses
        if ( k == Vec_PtrSize(vBundles) )
        {
            pBundle = NULL;
            Length = strlen(pNameFormal);
            if ( pNameFormal[Length-1] == ']' )
            {
                // find the opening brace
                for ( Length--; Length >= 0; Length-- )
                    if ( pNameFormal[Length] == '[' )
                        break;
                // compare names before brace
                if ( Length > 0 )
                {
                    Vec_PtrForEachEntry( Ver_Bundle_t *, vBundles, pBundle, j )
                        if ( !strncmp(pBundle->pNameFormal, pNameFormal, Length) && (int)strlen(pBundle->pNameFormal) == Length )
                            break;
                    if ( j == Vec_PtrSize(vBundles) )
                        pBundle = NULL;
                }
            }
            if ( pBundle == NULL )
            {
                sprintf( pMan->sError, "Cannot find an actual net for the formal net %s when instantiating network %s in box %s.", 
                    pNameFormal, pNtkBox->pName, Abc_ObjName(pBox) );
                Ver_ParsePrintErrorMessage( pMan );
                return 0;
            }
        }
        // the bundle is found - add the connections - using order LSB to MSB
        Vec_PtrForEachEntryReverse( Abc_Obj_t *, pBundle->vNetsActual, pNetAct, k )
        {
            pTermNew = Abc_NtkCreateBi( pNtk );
            Abc_ObjAddFanin( pBox, pTermNew );
            Abc_ObjAddFanin( pTermNew, pNetAct );
            i++;
        }
        i--;
    }

    // connect those formal POs that do have nets
    Abc_NtkForEachPo( pNtkBox, pTerm, i )
    {
        // get the name of this PI
        pNameFormal = Abc_ObjName( Abc_ObjFanin0(pTerm) );
        // try to find this formal net in the bundle
        pBundle = NULL;
        Vec_PtrForEachEntry( Ver_Bundle_t *, vBundles, pBundle, k )
            if ( !strcmp(pBundle->pNameFormal, pNameFormal) )
                break;
        assert( pBundle != NULL );
        // if the name is not found, try without parentheses
        if ( k == Vec_PtrSize(vBundles) )
        {
            pBundle = NULL;
            Length = strlen(pNameFormal);
            if ( pNameFormal[Length-1] == ']' )
            {
                // find the opening brace
                for ( Length--; Length >= 0; Length-- )
                    if ( pNameFormal[Length] == '[' )
                        break;
                // compare names before brace
                if ( Length > 0 )
                {
                    Vec_PtrForEachEntry( Ver_Bundle_t *, vBundles, pBundle, j )
                        if ( !strncmp(pBundle->pNameFormal, pNameFormal, Length) && (int)strlen(pBundle->pNameFormal) == Length ) 
                            break;
                    if ( j == Vec_PtrSize(vBundles) )
                        pBundle = NULL;
                }
            }
            if ( pBundle == NULL )
            {
                char Buffer[1000];
//                printf( "Warning: The formal output %s is not driven when instantiating network %s in box %s.", 
//                    pNameFormal, pNtkBox->pName, Abc_ObjName(pBox) );
                pTermNew = Abc_NtkCreateBo( pNtk );
                sprintf( Buffer, "_temp_net%d", Abc_ObjId(pTermNew) );
                pNetAct = Abc_NtkFindOrCreateNet( pNtk, Buffer );
                Abc_ObjAddFanin( pTermNew, pBox );
                Abc_ObjAddFanin( pNetAct, pTermNew );
                continue;
            }
        }
        // the bundle is found - add the connections
        Vec_PtrForEachEntryReverse( Abc_Obj_t *, pBundle->vNetsActual, pNetAct, k )
        {
            if ( !strcmp(Abc_ObjName(pNetAct), "1\'b0") || !strcmp(Abc_ObjName(pNetAct), "1\'b1") )
            {
                sprintf( pMan->sError, "It looks like formal output %s is driving a constant net (%s) when instantiating network %s in box %s.", 
                    pBundle->pNameFormal, Abc_ObjName(pNetAct), pNtkBox->pName, Abc_ObjName(pBox) );
                // free the bundling
                Vec_PtrForEachEntry( Ver_Bundle_t *, vBundles, pBundle, k )
                    Ver_ParseFreeBundle( pBundle );
                Vec_PtrFree( vBundles );
                pBox->pCopy = NULL;
                Ver_ParsePrintErrorMessage( pMan );
                return 0;
            }
            pTermNew = Abc_NtkCreateBo( pNtk );
            Abc_ObjAddFanin( pTermNew, pBox );
            Abc_ObjAddFanin( pNetAct, pTermNew );
            i++;
        }
        i--;
    }

    // free the bundling
    Vec_PtrForEachEntry( Ver_Bundle_t *, vBundles, pBundle, k )
        Ver_ParseFreeBundle( pBundle );
    Vec_PtrFree( vBundles );
    pBox->pCopy = NULL;
    return 1;
}


/**Function*************************************************************

  Synopsis    [Connects the defined boxes.]

  Description [Returns 2 if there are any undef boxes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_ParseConnectDefBoxes( Ver_Man_t * pMan )
{
    Abc_Ntk_t * pNtk;
    Abc_Obj_t * pBox;
    int i, k, RetValue = 1;
    // go through all the modules
    Vec_PtrForEachEntry( Abc_Ntk_t *, pMan->pDesign->vModules, pNtk, i )
    {
        // go through all the boxes of this module
        Abc_NtkForEachBox( pNtk, pBox, k )
        {
            if ( Abc_ObjIsLatch(pBox) )
                continue;
            // skip internal boxes of the blackboxes
            if ( pBox->pData == NULL )
                continue;
            // if the network is undefined, it will be connected later
            if ( !Ver_NtkIsDefined((Abc_Ntk_t *)pBox->pData) )
            {
                RetValue = 2;
                continue;
            }
            // connect the box
            if ( !Ver_ParseConnectBox( pMan, pBox ) )
                return 0;
            // if the network is a true blackbox, skip
            if ( Abc_NtkHasBlackbox((Abc_Ntk_t *)pBox->pData) )
                continue;
            // convert the box to the whitebox
            Abc_ObjBlackboxToWhitebox( pBox );
        }
    }
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Collects the undef boxes and maps them into their instances.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Ver_ParseCollectUndefBoxes( Ver_Man_t * pMan )
{
    Vec_Ptr_t * vUndefs;
    Abc_Ntk_t * pNtk, * pNtkBox;
    Abc_Obj_t * pBox;
    int i, k;
    // clear the module structures
    Vec_PtrForEachEntry( Abc_Ntk_t *, pMan->pDesign->vModules, pNtk, i )
        pNtk->pData = NULL;
    // go through all the blackboxes
    vUndefs = Vec_PtrAlloc( 16 );
    Vec_PtrForEachEntry( Abc_Ntk_t *, pMan->pDesign->vModules, pNtk, i )
    {
        Abc_NtkForEachBlackbox( pNtk, pBox, k )
        {
            pNtkBox = (Abc_Ntk_t *)pBox->pData;
            if ( pNtkBox == NULL )
                continue;
            if ( Ver_NtkIsDefined(pNtkBox) )
                continue;
            if ( pNtkBox->pData == NULL )
            {
                // save the box
                Vec_PtrPush( vUndefs, pNtkBox );
                pNtkBox->pData = Vec_PtrAlloc( 16 );
            }
            // save the instance
            Vec_PtrPush( (Vec_Ptr_t *)pNtkBox->pData, pBox );
        }
    }
    return vUndefs;
}

/**Function*************************************************************

  Synopsis    [Reports how many times each type of undefined box occurs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ver_ParseReportUndefBoxes( Ver_Man_t * pMan )
{
    Abc_Ntk_t * pNtk;
    Abc_Obj_t * pBox;
    int i, k, nBoxes;
    // clean 
    nBoxes = 0;
    Vec_PtrForEachEntry( Abc_Ntk_t *, pMan->pDesign->vModules, pNtk, i )
    {
        pNtk->fHiePath = 0;
        if ( !Ver_NtkIsDefined(pNtk) )
            nBoxes++;
    }
    // count
    Vec_PtrForEachEntry( Abc_Ntk_t *, pMan->pDesign->vModules, pNtk, i )
        Abc_NtkForEachBlackbox( pNtk, pBox, k )
            if ( pBox->pData && !Ver_NtkIsDefined((Abc_Ntk_t *)pBox->pData) )
                ((Abc_Ntk_t *)pBox->pData)->fHiePath++;
    // print the stats
    printf( "Warning: The design contains %d undefined object types interpreted as blackboxes:\n", nBoxes );
    Vec_PtrForEachEntry( Abc_Ntk_t *, pMan->pDesign->vModules, pNtk, i )
        if ( !Ver_NtkIsDefined(pNtk) )
            printf( "%s (%d)  ", Abc_NtkName(pNtk), pNtk->fHiePath );
    printf( "\n" );
    // clean 
    Vec_PtrForEachEntry( Abc_Ntk_t *, pMan->pDesign->vModules, pNtk, i )
        pNtk->fHiePath = 0;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if there are non-driven nets.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_ParseCheckNondrivenNets( Vec_Ptr_t * vUndefs )
{
    Abc_Ntk_t * pNtk;
    Ver_Bundle_t * pBundle;
    Abc_Obj_t * pBox, * pNet;
    int i, k, j, m;
    // go through undef box types
    Vec_PtrForEachEntry( Abc_Ntk_t *, vUndefs, pNtk, i )
        // go through instances of this type
        Vec_PtrForEachEntry( Abc_Obj_t *, (Vec_Ptr_t *)pNtk->pData, pBox, k )
            // go through the bundles of this instance
            Vec_PtrForEachEntryReverse( Ver_Bundle_t *, (Vec_Ptr_t *)pBox->pCopy, pBundle, j )
                // go through the actual nets of this bundle
                if ( pBundle )
                Vec_PtrForEachEntry( Abc_Obj_t *, pBundle->vNetsActual, pNet, m )
                {
                    if ( Abc_ObjFaninNum(pNet) == 0 ) // non-driven
                        if ( strcmp(Abc_ObjName(pNet), "1\'b0") && strcmp(Abc_ObjName(pNet), "1\'b1") ) // diff from a const
                            return 1;
                }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Checks if formal nets with the given name are driven in any of the instances of undef boxes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_ParseFormalNetsAreDriven( Abc_Ntk_t * pNtk, char * pNameFormal )
{
    Ver_Bundle_t * pBundle = NULL;
    Abc_Obj_t * pBox, * pNet;
    int k, j, m;
    // go through instances of this type
    Vec_PtrForEachEntry( Abc_Obj_t *, (Vec_Ptr_t *)pNtk->pData, pBox, k )
    {
        // find a bundle with the given name in this instance
        Vec_PtrForEachEntryReverse( Ver_Bundle_t *, (Vec_Ptr_t *)pBox->pCopy, pBundle, j )
            if ( pBundle && !strcmp( pBundle->pNameFormal, pNameFormal ) )
                break;
        // skip non-driven bundles
        if ( j == Vec_PtrSize((Vec_Ptr_t *)pBox->pCopy) )
            continue;
        // check if all nets are driven in this bundle
        assert(pBundle); // Verify that pBundle was assigned to.
        Vec_PtrForEachEntry( Abc_Obj_t *, pBundle->vNetsActual, pNet, m )
            if ( Abc_ObjFaninNum(pNet) > 0 )
                return 1;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns the non-driven bundle that is given distance from the end.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ver_Bundle_t * Ver_ParseGetNondrivenBundle( Abc_Ntk_t * pNtk, int Counter )
{
    Ver_Bundle_t * pBundle;
    Abc_Obj_t * pBox, * pNet;
    int k, m;
    // go through instances of this type
    Vec_PtrForEachEntry( Abc_Obj_t *, (Vec_Ptr_t *)pNtk->pData, pBox, k )
    {
        if ( Counter >= Vec_PtrSize((Vec_Ptr_t *)pBox->pCopy) )
            continue;
        // get the bundle given distance away
        pBundle = (Ver_Bundle_t *)Vec_PtrEntry( (Vec_Ptr_t *)pBox->pCopy, Vec_PtrSize((Vec_Ptr_t *)pBox->pCopy) - 1 - Counter );
        if ( pBundle == NULL )
            continue;
        // go through the actual nets of this bundle
        Vec_PtrForEachEntry( Abc_Obj_t *, pBundle->vNetsActual, pNet, m )
            if ( !Abc_ObjFaninNum(pNet) && !Ver_ParseFormalNetsAreDriven(pNtk, pBundle->pNameFormal) ) // non-driven
                return pBundle;
    }
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Drives the bundle in the given undef box.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_ParseDriveFormal( Ver_Man_t * pMan, Abc_Ntk_t * pNtk, Ver_Bundle_t * pBundle0 )
{
    char Buffer[200];
    char * pName;
    Ver_Bundle_t * pBundle = NULL;
    Abc_Obj_t * pBox, * pTerm, * pTermNew, * pNetAct, * pNetFormal;
    int k, j, m;

    // drive this net in the undef box
    Vec_PtrForEachEntry( Abc_Obj_t *, pBundle0->vNetsActual, pNetAct, m )
    {
        // create the formal net
        if ( Vec_PtrSize(pBundle0->vNetsActual) == 1 )
            sprintf( Buffer, "%s", pBundle0->pNameFormal );
        else
            sprintf( Buffer, "%s[%d]", pBundle0->pNameFormal, m );
        assert( Abc_NtkFindNet( pNtk, Buffer ) == NULL );
        pNetFormal = Abc_NtkFindOrCreateNet( pNtk, Buffer );
        // connect it to the box
        pTerm = Abc_NtkCreateBo( pNtk );
        assert( Abc_NtkBoxNum(pNtk) <= 1 );
        pBox = Abc_NtkBoxNum(pNtk)? Abc_NtkBox(pNtk,0) : Abc_NtkCreateBlackbox(pNtk);
        Abc_ObjAddFanin( Abc_NtkCreatePo(pNtk), pNetFormal );
        Abc_ObjAddFanin( pNetFormal, pTerm );
        Abc_ObjAddFanin( pTerm, pBox );
    }

    // go through instances of this type
    pName = Extra_UtilStrsav(pBundle0->pNameFormal);
    Vec_PtrForEachEntry( Abc_Obj_t *, (Vec_Ptr_t *)pNtk->pData, pBox, k )
    {
        // find a bundle with the given name in this instance
        Vec_PtrForEachEntryReverse( Ver_Bundle_t *, (Vec_Ptr_t *)pBox->pCopy, pBundle, j )
            if ( pBundle && !strcmp( pBundle->pNameFormal, pName ) )
                break;
        // skip non-driven bundles
        if ( j == Vec_PtrSize((Vec_Ptr_t *)pBox->pCopy) )
            continue;
        // check if any nets are driven in this bundle
        assert(pBundle); // Verify pBundle was assigned to.
        Vec_PtrForEachEntry( Abc_Obj_t *, pBundle->vNetsActual, pNetAct, m )
            if ( Abc_ObjFaninNum(pNetAct) > 0 )
            {
                sprintf( pMan->sError, "Missing specification of the I/Os of undefined box \"%s\".", Abc_NtkName(pNtk) );
                Ver_ParsePrintErrorMessage( pMan );
                return 0;
            }
        // drive the nets by the undef box
        Vec_PtrForEachEntryReverse( Abc_Obj_t *, pBundle->vNetsActual, pNetAct, m )
        {
            pTermNew = Abc_NtkCreateBo( pNetAct->pNtk );
            Abc_ObjAddFanin( pTermNew, pBox );
            Abc_ObjAddFanin( pNetAct, pTermNew );
        }
        // remove the bundle
        Ver_ParseFreeBundle( pBundle ); pBundle = NULL;
        Vec_PtrWriteEntry( (Vec_Ptr_t *)pBox->pCopy, j, NULL );
    }
    ABC_FREE( pName );
    return 1;
}


/**Function*************************************************************

  Synopsis    [Drives the bundle in the given undef box.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_ParseDriveInputs( Ver_Man_t * pMan, Vec_Ptr_t * vUndefs )
{
    char Buffer[200];
    Ver_Bundle_t * pBundle;
    Abc_Ntk_t * pNtk;
    Abc_Obj_t * pBox, * pBox2, * pTerm, * pTermNew, * pNetFormal, * pNetAct;
    int i, k, j, m, CountCur, CountTotal = -1;
    // iterate through the undef boxes
    Vec_PtrForEachEntry( Abc_Ntk_t *, vUndefs, pNtk, i )
    {
        // count the number of unconnected bundles for instances of this type of box
        CountTotal = -1;
        Vec_PtrForEachEntry( Abc_Obj_t *, (Vec_Ptr_t *)pNtk->pData, pBox, k )
        {
            CountCur = 0;
            Vec_PtrForEachEntry( Ver_Bundle_t *, (Vec_Ptr_t *)pBox->pCopy, pBundle, j )
                CountCur += (pBundle != NULL);
            if ( CountTotal == -1 )
                CountTotal = CountCur;
            else if ( CountTotal != CountCur )
            {
                sprintf( pMan->sError, "The number of formal inputs (%d) is different from the expected one (%d) when instantiating network %s in box %s.", 
                    CountCur, CountTotal, pNtk->pName, Abc_ObjName(pBox) );
                Ver_ParsePrintErrorMessage( pMan );
                return 0;
            }
        }

        // create formals
        pBox = (Abc_Obj_t *)Vec_PtrEntry( (Vec_Ptr_t *)pNtk->pData, 0 );
        Vec_PtrForEachEntry( Ver_Bundle_t *, (Vec_Ptr_t *)pBox->pCopy, pBundle, j )
        {
            if ( pBundle == NULL )
                continue;
            Vec_PtrForEachEntry( Abc_Obj_t *, pBundle->vNetsActual, pNetAct, m )
            {
                // find create the formal net
                if ( Vec_PtrSize(pBundle->vNetsActual) == 1 )
                    sprintf( Buffer, "%s", pBundle->pNameFormal );
                else
                    sprintf( Buffer, "%s[%d]", pBundle->pNameFormal, m );
                assert( Abc_NtkFindNet( pNtk, Buffer ) == NULL );
                pNetFormal = Abc_NtkFindOrCreateNet( pNtk, Buffer );
                // connect
                pTerm = Abc_NtkCreateBi( pNtk );
                assert( Abc_NtkBoxNum(pNtk) <= 1 );
                pBox2 = Abc_NtkBoxNum(pNtk)? Abc_NtkBox(pNtk,0) : Abc_NtkCreateBlackbox(pNtk);
                Abc_ObjAddFanin( pNetFormal, Abc_NtkCreatePi(pNtk) );
                Abc_ObjAddFanin( pTerm, pNetFormal );
                Abc_ObjAddFanin( pBox2, pTerm );
            }
        }

        // go through all the boxes
        Vec_PtrForEachEntry( Abc_Obj_t *, (Vec_Ptr_t *)pNtk->pData, pBox, k )
        {
            // go through all the bundles
            Vec_PtrForEachEntry( Ver_Bundle_t *, (Vec_Ptr_t *)pBox->pCopy, pBundle, j )
            {
                if ( pBundle == NULL )
                    continue;
                // drive the nets by the undef box
                Vec_PtrForEachEntryReverse( Abc_Obj_t *, pBundle->vNetsActual, pNetAct, m )
                {
                    pTermNew = Abc_NtkCreateBi( pNetAct->pNtk );
                    Abc_ObjAddFanin( pBox, pTermNew );
                    Abc_ObjAddFanin( pTermNew, pNetAct );
                }
                // remove the bundle
                Ver_ParseFreeBundle( pBundle );
                Vec_PtrWriteEntry( (Vec_Ptr_t *)pBox->pCopy, j, NULL );
            }

            // free the bundles
            Vec_PtrFree( (Vec_Ptr_t *)pBox->pCopy );
            pBox->pCopy = NULL;
        }
    }
    return 1;
}


/**Function*************************************************************

  Synopsis    [Returns the max size of any undef box.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_ParseMaxBoxSize( Vec_Ptr_t * vUndefs )
{
    Abc_Ntk_t * pNtk;
    Abc_Obj_t * pBox;
    int i, k, nMaxSize = 0;
    // go through undef box types
    Vec_PtrForEachEntry( Abc_Ntk_t *, vUndefs, pNtk, i )
        // go through instances of this type
        Vec_PtrForEachEntry( Abc_Obj_t *, (Vec_Ptr_t *)pNtk->pData, pBox, k )
            // check the number of bundles of this instance
            if ( nMaxSize < Vec_PtrSize((Vec_Ptr_t *)pBox->pCopy) )
                nMaxSize = Vec_PtrSize((Vec_Ptr_t *)pBox->pCopy);
    return nMaxSize;
}

/**Function*************************************************************

  Synopsis    [Prints the comprehensive report into a log file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ver_ParsePrintLog( Ver_Man_t * pMan )
{
    Abc_Ntk_t * pNtk, * pNtkBox;
    Abc_Obj_t * pBox;
    FILE * pFile;
    char * pNameGeneric;
    char Buffer[1000];
    int i, k, Count1 = 0;

    // open the log file
    pNameGeneric = Extra_FileNameGeneric( pMan->pFileName );
    sprintf( Buffer, "%s.log", pNameGeneric );
    ABC_FREE( pNameGeneric );
    pFile = fopen( Buffer, "w" );

    // count the total number of instances and how many times they occur
    Vec_PtrForEachEntry( Abc_Ntk_t *, pMan->pDesign->vModules, pNtk, i )
        pNtk->fHieVisited = 0;
    Vec_PtrForEachEntry( Abc_Ntk_t *, pMan->pDesign->vModules, pNtk, i )
        Abc_NtkForEachBox( pNtk, pBox, k )
        {
            if ( Abc_ObjIsLatch(pBox) )
                continue;
            pNtkBox = (Abc_Ntk_t *)pBox->pData;
            if ( pNtkBox == NULL )
                continue;
            pNtkBox->fHieVisited++;
        }
    // print each box and its stats
    fprintf( pFile, "The hierarhical design %s contains %d modules:\n", pMan->pFileName, Vec_PtrSize(pMan->pDesign->vModules) );
    Vec_PtrForEachEntry( Abc_Ntk_t *, pMan->pDesign->vModules, pNtk, i )
    {
        fprintf( pFile, "%-50s : ", Abc_NtkName(pNtk) );
        if ( !Ver_NtkIsDefined(pNtk) )
            fprintf( pFile, "undefbox" );
        else if ( Abc_NtkHasBlackbox(pNtk) )
            fprintf( pFile, "blackbox" );
        else
            fprintf( pFile, "logicbox" );
        fprintf( pFile, " instantiated %6d times ", pNtk->fHieVisited );
//        fprintf( pFile, "\n   " );
        fprintf( pFile, " pi = %4d", Abc_NtkPiNum(pNtk) );
        fprintf( pFile, " po = %4d", Abc_NtkPoNum(pNtk) );
        fprintf( pFile, " nd = %8d",  Abc_NtkNodeNum(pNtk) );
        fprintf( pFile, " lat = %6d",  Abc_NtkLatchNum(pNtk) );
        fprintf( pFile, " box = %6d", Abc_NtkBoxNum(pNtk)-Abc_NtkLatchNum(pNtk) );
        fprintf( pFile, "\n" );
        Count1 += (Abc_NtkPoNum(pNtk) == 1);
    }
    Vec_PtrForEachEntry( Abc_Ntk_t *, pMan->pDesign->vModules, pNtk, i )
        pNtk->fHieVisited = 0;
    fprintf( pFile, "The number of modules with one output = %d (%.2f %%).\n", Count1, 100.0 * Count1/Vec_PtrSize(pMan->pDesign->vModules) ); 

    // report instances with dangling outputs
    if ( Vec_PtrSize(pMan->pDesign->vModules) > 1 )
    {
        Vec_Ptr_t * vBundles;
        Ver_Bundle_t * pBundle;
        int j, nActNets, Counter = 0;
        // count the number of instances with dangling outputs
        Vec_PtrForEachEntry( Abc_Ntk_t *, pMan->pDesign->vModules, pNtk, i )
        {
            Abc_NtkForEachBox( pNtk, pBox, k )
            {
                if ( Abc_ObjIsLatch(pBox) )
                    continue;
                vBundles = (Vec_Ptr_t *)pBox->pCopy;
                pNtkBox = (Abc_Ntk_t *)pBox->pData;
                if ( pNtkBox == NULL )
                    continue;
                if ( !Ver_NtkIsDefined(pNtkBox) )
                    continue;
                // count the number of actual nets
                nActNets = 0;
                Vec_PtrForEachEntry( Ver_Bundle_t *, vBundles, pBundle, j )
                    nActNets += Vec_PtrSize(pBundle->vNetsActual);
                // the box is defined and will be connected
                if ( nActNets != Abc_NtkPiNum(pNtkBox) + Abc_NtkPoNum(pNtkBox) )
                    Counter++;
            }
        }
        if ( Counter == 0 )
            fprintf( pFile, "The outputs of all box instances are connected.\n" );
        else
        {
            fprintf( pFile, "\n" );
            fprintf( pFile, "The outputs of %d box instances are not connected:\n", Counter );
            // enumerate through the boxes
            Vec_PtrForEachEntry( Abc_Ntk_t *, pMan->pDesign->vModules, pNtk, i )
            {
                Abc_NtkForEachBox( pNtk, pBox, k )
                {
                    if ( Abc_ObjIsLatch(pBox) )
                        continue;
                    vBundles = (Vec_Ptr_t *)pBox->pCopy;
                    pNtkBox = (Abc_Ntk_t *)pBox->pData;
                    if ( pNtkBox == NULL )
                        continue;
                    if ( !Ver_NtkIsDefined(pNtkBox) )
                        continue;
                    // count the number of actual nets
                    nActNets = 0;
                    Vec_PtrForEachEntry( Ver_Bundle_t *, vBundles, pBundle, j )
                        nActNets += Vec_PtrSize(pBundle->vNetsActual);
                    // the box is defined and will be connected
                    if ( nActNets != Abc_NtkPiNum(pNtkBox) + Abc_NtkPoNum(pNtkBox) )
                        fprintf( pFile, "In module \"%s\" instance \"%s\" of box \"%s\" has different numbers of actual/formal nets (%d/%d).\n",
                            Abc_NtkName(pNtk), Abc_ObjName(pBox), Abc_NtkName(pNtkBox), nActNets, Abc_NtkPiNum(pNtkBox) + Abc_NtkPoNum(pNtkBox) );
                }
            }
        }
    }
    fclose( pFile );
    printf( "Hierarchy statistics can be found in log file \"%s\".\n", Buffer );
}


/**Function*************************************************************

  Synopsis    [Attaches the boxes to the network.]

  Description [This procedure is called after the design is parsed. 
  At that point, all the defined models have their PIs present. 
  They are connected first. Next undef boxes are processed (if present).
  Iteratively, one bundle is selected to be driven by the undef boxes in such 
  a way that there is no conflict (if it is driven by an instance of the box,
  no other net will be driven twice by the same formal net of some other instance 
  of the same box). In the end, all the remaining nets that cannot be driven 
  by the undef boxes are connected to the undef boxes as inputs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_ParseAttachBoxes( Ver_Man_t * pMan )
{
    int fPrintLog = 0;
    Abc_Ntk_t * pNtk = NULL;
    Ver_Bundle_t * pBundle;
    Vec_Ptr_t * vUndefs;
    int i, RetValue, Counter, nMaxBoxSize;

    // print the log file
    if ( fPrintLog && pMan->pDesign->vModules && Vec_PtrSize(pMan->pDesign->vModules) > 1 )
        Ver_ParsePrintLog( pMan );

    // connect defined boxes
    RetValue = Ver_ParseConnectDefBoxes( pMan );
    if ( RetValue < 2 )
        return RetValue;

    // report the boxes
    Ver_ParseReportUndefBoxes( pMan );

    // collect undef box types and their actual instances
    vUndefs = Ver_ParseCollectUndefBoxes( pMan );
    assert( Vec_PtrSize( vUndefs ) > 0 );

    // go through all undef box types
    Counter = 0;
    nMaxBoxSize = Ver_ParseMaxBoxSize( vUndefs );
    while ( Ver_ParseCheckNondrivenNets(vUndefs) && Counter < nMaxBoxSize )
    {
        // go through undef box types
        pBundle = NULL;
        Vec_PtrForEachEntry( Abc_Ntk_t *, vUndefs, pNtk, i )
            if ( (pBundle = Ver_ParseGetNondrivenBundle( pNtk, Counter )) )
                break;
        if ( pBundle == NULL )
        {
            Counter++;
            continue;
        }
        // drive this bundle by this box
        if ( !Ver_ParseDriveFormal( pMan, pNtk, pBundle ) )
            return 0;
    }

    // make all the remaining bundles the drivers of undefs
    if ( !Ver_ParseDriveInputs( pMan, vUndefs ) )
        return 0;

    // cleanup
    Vec_PtrForEachEntry( Abc_Ntk_t *, vUndefs, pNtk, i )
    {
        Vec_PtrFree( (Vec_Ptr_t *)pNtk->pData );
        pNtk->pData = NULL;
    }
    Vec_PtrFree( vUndefs ); 
    return 1;
}


/**Function*************************************************************

  Synopsis    [Creates PI terminal and net.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Ver_ParseCreatePi( Abc_Ntk_t * pNtk, char * pName )
{
    Abc_Obj_t * pNet, * pTerm;
    // get the PI net
//    pNet  = Ver_ParseFindNet( pNtk, pName );
//    if ( pNet )
//        printf( "Warning: PI \"%s\" appears twice in the list.\n", pName );
    pNet  = Abc_NtkFindOrCreateNet( pNtk, pName );
    // add the PI node
    pTerm = Abc_NtkCreatePi( pNtk );
    Abc_ObjAddFanin( pNet, pTerm );
    return pTerm;
}

/**Function*************************************************************

  Synopsis    [Creates PO terminal and net.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Ver_ParseCreatePo( Abc_Ntk_t * pNtk, char * pName )
{
    Abc_Obj_t * pNet, * pTerm;
    // get the PO net
//    pNet  = Ver_ParseFindNet( pNtk, pName );
//    if ( pNet && Abc_ObjFaninNum(pNet) == 0 )
//        printf( "Warning: PO \"%s\" appears twice in the list.\n", pName );
    pNet  = Abc_NtkFindOrCreateNet( pNtk, pName );
    // add the PO node
    pTerm = Abc_NtkCreatePo( pNtk );
    Abc_ObjAddFanin( pTerm, pNet );
    return pTerm;
}

/**Function*************************************************************

  Synopsis    [Create a latch with the given input/output.]

  Description [By default, the latch value is a don't-care.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Ver_ParseCreateLatch( Abc_Ntk_t * pNtk, Abc_Obj_t * pNetLI, Abc_Obj_t * pNetLO )
{
    Abc_Obj_t * pLatch, * pTerm;
    // add the BO terminal
    pTerm = Abc_NtkCreateBi( pNtk );
    Abc_ObjAddFanin( pTerm, pNetLI );
    // add the latch box
    pLatch = Abc_NtkCreateLatch( pNtk );
    Abc_ObjAddFanin( pLatch, pTerm  );
    // add the BI terminal
    pTerm = Abc_NtkCreateBo( pNtk );
    Abc_ObjAddFanin( pTerm, pLatch );
    // get the LO net
    Abc_ObjAddFanin( pNetLO, pTerm );
    // set latch name
    Abc_ObjAssignName( pLatch, Abc_ObjName(pNetLO), "L" );
    Abc_LatchSetInitDc( pLatch );
    return pLatch;
}

/**Function*************************************************************

  Synopsis    [Creates inverter and returns its net.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Ver_ParseCreateInv( Abc_Ntk_t * pNtk, Abc_Obj_t * pNet )
{
    Abc_Obj_t * pObj;
    pObj = Abc_NtkCreateNodeInv( pNtk, pNet );
    pNet = Abc_NtkCreateNet( pNtk );
    Abc_ObjAddFanin( pNet, pObj );
    return pNet;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

