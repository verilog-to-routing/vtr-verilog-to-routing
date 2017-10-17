/**CFile****************************************************************

  FileName    [io.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Command file.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: io.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "io.h"
#include "mainInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int IoCommandRead        ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandReadAiger   ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandReadBaf     ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandReadBlif    ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandReadBlifMv  ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandReadBench   ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandReadDsd     ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandReadEdif    ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandReadEqn     ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandReadPla     ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandReadTruth   ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandReadVerilog ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandReadVer     ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandReadVerLib  ( Abc_Frame_t * pAbc, int argc, char **argv );

static int IoCommandWrite       ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteHie    ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteAiger  ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteBaf    ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteBlif   ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteBlifMv ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteBench  ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteCellNet( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteCnf    ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteCounter( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteDot    ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteEqn    ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteGml    ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteList   ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWritePla    ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteVerilog( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteVerLib ( Abc_Frame_t * pAbc, int argc, char **argv );

extern int glo_fMapped;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_Init( Abc_Frame_t * pAbc )
{
    Cmd_CommandAdd( pAbc, "I/O", "read",          IoCommandRead,         1 );
    Cmd_CommandAdd( pAbc, "I/O", "read_aiger",    IoCommandReadAiger,    1 );
    Cmd_CommandAdd( pAbc, "I/O", "read_baf",      IoCommandReadBaf,      1 );
    Cmd_CommandAdd( pAbc, "I/O", "read_blif",     IoCommandReadBlif,     1 );
    Cmd_CommandAdd( pAbc, "I/O", "read_blif_mv",  IoCommandReadBlif,     1 );
    Cmd_CommandAdd( pAbc, "I/O", "read_bench",    IoCommandReadBench,    1 );
    Cmd_CommandAdd( pAbc, "I/O", "read_dsd",      IoCommandReadDsd,      1 );
//    Cmd_CommandAdd( pAbc, "I/O", "read_edif",     IoCommandReadEdif,     1 );
    Cmd_CommandAdd( pAbc, "I/O", "read_eqn",      IoCommandReadEqn,      1 );
    Cmd_CommandAdd( pAbc, "I/O", "read_pla",      IoCommandReadPla,      1 );
    Cmd_CommandAdd( pAbc, "I/O", "read_truth",    IoCommandReadTruth,    1 );
    Cmd_CommandAdd( pAbc, "I/O", "read_verilog",  IoCommandReadVerilog,  1 );
//    Cmd_CommandAdd( pAbc, "I/O", "read_ver",      IoCommandReadVer,      1 );
//    Cmd_CommandAdd( pAbc, "I/O", "read_verlib",   IoCommandReadVerLib,   0 );

    Cmd_CommandAdd( pAbc, "I/O", "write",         IoCommandWrite,        0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_hie",     IoCommandWriteHie,     0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_aiger",   IoCommandWriteAiger,   0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_baf",     IoCommandWriteBaf,     0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_blif",    IoCommandWriteBlif,    0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_blif_mv", IoCommandWriteBlifMv,  0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_bench",   IoCommandWriteBench,   0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_cellnet", IoCommandWriteCellNet, 0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_counter", IoCommandWriteCounter, 0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_cnf",     IoCommandWriteCnf,     0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_dot",     IoCommandWriteDot,     0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_eqn",     IoCommandWriteEqn,     0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_gml",     IoCommandWriteGml,     0 );
//    Cmd_CommandAdd( pAbc, "I/O", "write_list",    IoCommandWriteList,    0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_pla",     IoCommandWritePla,     0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_verilog", IoCommandWriteVerilog, 0 );
//    Cmd_CommandAdd( pAbc, "I/O", "write_verlib",  IoCommandWriteVerLib,  0 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_End()
{
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandRead( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Abc_Ntk_t * pNtk;
    char * pFileName;
    int fCheck;
    int c;

    fCheck = 1;
    glo_fMapped = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "mch" ) ) != EOF )
    {
        switch ( c )
        {
            case 'm':
                glo_fMapped ^= 1;
                break;
            case 'c':
                fCheck ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the input file name
    pFileName = argv[globalUtilOptind];
    // read the file using the corresponding file reader
    pNtk = Io_Read( pFileName, Io_ReadFileType(pFileName), fCheck );
    if ( pNtk == NULL )
        return 1;
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtk );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read [-mch] <file>\n" );
    fprintf( pAbc->Err, "\t         replaces the current network by the network read from <file>\n" );
    fprintf( pAbc->Err, "\t         by calling the parser that matches the extension of <file>\n" );
    fprintf( pAbc->Err, "\t         (to read a hierarchical design, use \"read_hie\")\n" );
    fprintf( pAbc->Err, "\t-m     : toggle reading mapped Verilog [default = %s]\n", glo_fMapped? "yes":"no" );
    fprintf( pAbc->Err, "\t-c     : toggle network check after reading [default = %s]\n", fCheck? "yes":"no" );
    fprintf( pAbc->Err, "\t-h     : prints the command summary\n" );
    fprintf( pAbc->Err, "\tfile   : the name of a file to read\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandReadAiger( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Abc_Ntk_t * pNtk;
    char * pFileName;
    int fCheck;
    int c;

    fCheck = 1;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "ch" ) ) != EOF )
    {
        switch ( c )
        {
            case 'c':
                fCheck ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the input file name
    pFileName = argv[globalUtilOptind];
    // read the file using the corresponding file reader
    pNtk = Io_Read( pFileName, IO_FILE_AIGER, fCheck );
    if ( pNtk == NULL )
        return 1;
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtk );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_aiger [-ch] <file>\n" );
    fprintf( pAbc->Err, "\t         read the network in the AIGER format (http://fmv.jku.at/aiger)\n" );
    fprintf( pAbc->Err, "\t-c     : toggle network check after reading [default = %s]\n", fCheck? "yes":"no" );
    fprintf( pAbc->Err, "\t-h     : prints the command summary\n" );
    fprintf( pAbc->Err, "\tfile   : the name of a file to read\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandReadBaf( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Abc_Ntk_t * pNtk;
    char * pFileName;
    int fCheck;
    int c;

    fCheck = 1;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "ch" ) ) != EOF )
    {
        switch ( c )
        {
            case 'c':
                fCheck ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the input file name
    pFileName = argv[globalUtilOptind];
    // read the file using the corresponding file reader
    pNtk = Io_Read( pFileName, IO_FILE_BAF, fCheck );
    if ( pNtk == NULL )
        return 1;
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtk );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_baf [-ch] <file>\n" );
    fprintf( pAbc->Err, "\t         read the network in Binary Aig Format (BAF)\n" );
    fprintf( pAbc->Err, "\t-c     : toggle network check after reading [default = %s]\n", fCheck? "yes":"no" );
    fprintf( pAbc->Err, "\t-h     : prints the command summary\n" );
    fprintf( pAbc->Err, "\tfile   : the name of a file to read\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandReadBlif( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Abc_Ntk_t * pNtk;
    char * pFileName;
    int fReadAsAig;
    int fCheck;
    int c;
    extern Abc_Ntk_t * Io_ReadBlifAsAig( char * pFileName, int fCheck );

    fCheck = 1;
    fReadAsAig = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "ach" ) ) != EOF )
    {
        switch ( c )
        {
            case 'a':
                fReadAsAig ^= 1;
                break;
            case 'c':
                fCheck ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the input file name
    pFileName = argv[globalUtilOptind];
    // read the file using the corresponding file reader
    if ( fReadAsAig )
        pNtk = Io_ReadBlifAsAig( pFileName, fCheck );
    else
//        pNtk = Io_Read( pFileName, IO_FILE_BLIF, fCheck );
    {
        Abc_Ntk_t * pTemp;
        pNtk = Io_ReadBlif( pFileName, fCheck );
        pNtk = Abc_NtkToLogic( pTemp = pNtk );
        Abc_NtkDelete( pTemp );
    }

    if ( pNtk == NULL )
        return 1;
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtk );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_blif [-ach] <file>\n" );
    fprintf( pAbc->Err, "\t         read the network in binary BLIF format\n" );
    fprintf( pAbc->Err, "\t-a     : toggle creating AIG while reading the file [default = %s]\n", fReadAsAig? "yes":"no" );
    fprintf( pAbc->Err, "\t-c     : toggle network check after reading [default = %s]\n", fCheck? "yes":"no" );
    fprintf( pAbc->Err, "\t-h     : prints the command summary\n" );
    fprintf( pAbc->Err, "\tfile   : the name of a file to read\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandReadBlifMv( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Abc_Ntk_t * pNtk;
    char * pFileName;
    int fCheck;
    int c;

    fCheck = 1;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "ch" ) ) != EOF )
    {
        switch ( c )
        {
            case 'c':
                fCheck ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the input file name
    pFileName = argv[globalUtilOptind];
    // read the file using the corresponding file reader
    pNtk = Io_Read( pFileName, IO_FILE_BLIFMV, fCheck );
    if ( pNtk == NULL )
        return 1;
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtk );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_blif_mv [-ch] <file>\n" );
    fprintf( pAbc->Err, "\t         read the network in BLIF-MV format\n" );
    fprintf( pAbc->Err, "\t-c     : toggle network check after reading [default = %s]\n", fCheck? "yes":"no" );
    fprintf( pAbc->Err, "\t-h     : prints the command summary\n" );
    fprintf( pAbc->Err, "\tfile   : the name of a file to read\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandReadBench( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Abc_Ntk_t * pNtk;
    char * pFileName;
    int fCheck;
    int c;

    fCheck = 1;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "ch" ) ) != EOF )
    {
        switch ( c )
        {
            case 'c':
                fCheck ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the input file name
    pFileName = argv[globalUtilOptind];
    // read the file using the corresponding file reader
    pNtk = Io_Read( pFileName, IO_FILE_BENCH, fCheck );
    if ( pNtk == NULL )
        return 1;
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtk );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_bench [-ch] <file>\n" );
    fprintf( pAbc->Err, "\t         read the network in BENCH format\n" );
    fprintf( pAbc->Err, "\t-c     : toggle network check after reading [default = %s]\n", fCheck? "yes":"no" );
    fprintf( pAbc->Err, "\t-h     : prints the command summary\n" );
    fprintf( pAbc->Err, "\tfile   : the name of a file to read\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandReadDsd( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Abc_Ntk_t * pNtk;
    char * pString;
    int fCheck;
    int c;
    extern Abc_Ntk_t * Io_ReadDsd( char * pFormula );

    fCheck = 1;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "ch" ) ) != EOF )
    {
        switch ( c )
        {
            case 'c':
                fCheck ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the input file name
    pString = argv[globalUtilOptind];
    // read the file using the corresponding file reader
    pNtk = Io_ReadDsd( pString );
    if ( pNtk == NULL )
        return 1;
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtk );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_dsd [-h] <formula>\n" );
    fprintf( pAbc->Err, "\t          parses a formula representing DSD of a function\n" );
    fprintf( pAbc->Err, "\t-h      : prints the command summary\n" );
    fprintf( pAbc->Err, "\tformula : the formula representing disjoint-support decomposition (DSD)\n" );
    fprintf( pAbc->Err, "\t          Example of a formula: !(a*(b+CA(!d,e*f,c))*79B3(g,h,i,k))\n" );
    fprintf( pAbc->Err, "\t          where \'!\' is an INV, \'*\' is an AND, \'+\' is an XOR, \n" );
    fprintf( pAbc->Err, "\t          CA and 79B3 are hexadecimal representations of truth tables\n" );
    fprintf( pAbc->Err, "\t          (in this case CA=11001010 is truth table of MUX(Data0,Data1,Ctrl))\n" );
    fprintf( pAbc->Err, "\t          The lower chars (a,b,c,etc) are reserved for elementary variables.\n" );
    fprintf( pAbc->Err, "\t          The upper chars (A,B,C,etc) are reserved for hexadecimal digits.\n" );
    fprintf( pAbc->Err, "\t          No spaces are allowed in formulas. In parantheses, LSB goes first.\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandReadEdif( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Abc_Ntk_t * pNtk;
    char * pFileName;
    int fCheck;
    int c;

    fCheck = 1;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "ch" ) ) != EOF )
    {
        switch ( c )
        {
            case 'c':
                fCheck ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the input file name
    pFileName = argv[globalUtilOptind];
    // read the file using the corresponding file reader
    pNtk = Io_Read( pFileName, IO_FILE_EDIF, fCheck );
    if ( pNtk == NULL )
        return 1;
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtk );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_edif [-ch] <file>\n" );
    fprintf( pAbc->Err, "\t         read the network in EDIF (works only for ISCAS benchmarks)\n" );
    fprintf( pAbc->Err, "\t-c     : toggle network check after reading [default = %s]\n", fCheck? "yes":"no" );
    fprintf( pAbc->Err, "\t-h     : prints the command summary\n" );
    fprintf( pAbc->Err, "\tfile   : the name of a file to read\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandReadEqn( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Abc_Ntk_t * pNtk;
    char * pFileName;
    int fCheck;
    int c;

    fCheck = 1;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "ch" ) ) != EOF )
    {
        switch ( c )
        {
            case 'c':
                fCheck ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the input file name
    pFileName = argv[globalUtilOptind];
    // read the file using the corresponding file reader
    pNtk = Io_Read( pFileName, IO_FILE_EQN, fCheck );
    if ( pNtk == NULL )
        return 1;
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtk );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_eqn [-ch] <file>\n" );
    fprintf( pAbc->Err, "\t         read the network in equation format\n" );
    fprintf( pAbc->Err, "\t-c     : toggle network check after reading [default = %s]\n", fCheck? "yes":"no" );
    fprintf( pAbc->Err, "\t-h     : prints the command summary\n" );
    fprintf( pAbc->Err, "\tfile   : the name of a file to read\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandReadPla( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Abc_Ntk_t * pNtk;
    char * pFileName;
    int fCheck;
    int c;

    fCheck = 1;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "ch" ) ) != EOF )
    {
        switch ( c )
        {
            case 'c':
                fCheck ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the input file name
    pFileName = argv[globalUtilOptind];
    // read the file using the corresponding file reader
    pNtk = Io_Read( pFileName, IO_FILE_PLA, fCheck );
    if ( pNtk == NULL )
        return 1;
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtk );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_pla [-ch] <file>\n" );
    fprintf( pAbc->Err, "\t         read the network in PLA\n" );
    fprintf( pAbc->Err, "\t-c     : toggle network check after reading [default = %s]\n", fCheck? "yes":"no" );
    fprintf( pAbc->Err, "\t-h     : prints the command summary\n" );
    fprintf( pAbc->Err, "\tfile   : the name of a file to read\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandReadTruth( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Abc_Ntk_t * pNtk;
    char * pSopCover;
    int fHex;
    int c;

    fHex = 1;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "xh" ) ) != EOF )
    {
        switch ( c )
        {
            case 'x':
                fHex ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }

    if ( argc != globalUtilOptind + 1 )
    {
        goto usage;
    }

    // convert truth table to SOP
    if ( fHex )
        pSopCover = Abc_SopFromTruthHex(argv[globalUtilOptind]);
    else
        pSopCover = Abc_SopFromTruthBin(argv[globalUtilOptind]);
    if ( pSopCover == NULL )
    {
        fprintf( pAbc->Err, "Reading truth table has failed.\n" );
        return 1;
    }

    pNtk = Abc_NtkCreateWithNode( pSopCover );
    free( pSopCover );
    if ( pNtk == NULL )
    {
        fprintf( pAbc->Err, "Deriving the network has failed.\n" );
        return 1;
    }
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtk );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_truth [-xh] <truth>\n" );
    fprintf( pAbc->Err, "\t         creates network with node having given truth table\n" );
    fprintf( pAbc->Err, "\t-x     : toggles between bin and hex representation [default = %s]\n", fHex? "hex":"bin" );
    fprintf( pAbc->Err, "\t-h     : prints the command summary\n" );
    fprintf( pAbc->Err, "\ttruth  : truth table with most signficant bit first (e.g. 1000 for AND(a,b))\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandReadVerilog( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Abc_Ntk_t * pNtk;
    char * pFileName;
    int fCheck;
    int c;

    fCheck = 1;
    glo_fMapped = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "mch" ) ) != EOF )
    {
        switch ( c )
        {
            case 'm':
                glo_fMapped ^= 1;
                break;
            case 'c':
                fCheck ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the input file name
    pFileName = argv[globalUtilOptind];
    // read the file using the corresponding file reader
    pNtk = Io_Read( pFileName, IO_FILE_VERILOG, fCheck );
    if ( pNtk == NULL )
        return 1;
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtk );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_verilog [-mch] <file>\n" );
    fprintf( pAbc->Err, "\t         read the network in Verilog (IWLS 2002/2005 subset)\n" );
    fprintf( pAbc->Err, "\t-m     : toggle reading mapped Verilog [default = %s]\n", glo_fMapped? "yes":"no" );
    fprintf( pAbc->Err, "\t-c     : toggle network check after reading [default = %s]\n", fCheck? "yes":"no" );
    fprintf( pAbc->Err, "\t-h     : prints the command summary\n" );
    fprintf( pAbc->Err, "\tfile   : the name of a file to read\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandReadVer( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Abc_Ntk_t * pNtk, * pNtkNew;
    Abc_Lib_t * pDesign;
    char * pFileName;
    FILE * pFile;
    int fCheck;
    int c;
    extern Abc_Ntk_t * Abc_LibDeriveAig( Abc_Ntk_t * pNtk, Abc_Lib_t * pLib );
    extern Abc_Lib_t * Ver_ParseFile( char * pFileName, Abc_Lib_t * pGateLib, int fCheck, int fUseMemMan );

    printf( "Stand-alone structural Verilog reader is available as command \"read_verilog\".\n" );
    return 0;

    fCheck = 1;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "ch" ) ) != EOF )
    {
        switch ( c )
        {
            case 'c':
                fCheck ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }

    if ( argc != globalUtilOptind + 1 )
    {
        goto usage;
    }

    // get the input file name
    pFileName = argv[globalUtilOptind];
    if ( (pFile = fopen( pFileName, "r" )) == NULL )
    {
        fprintf( pAbc->Err, "Cannot open input file \"%s\". ", pFileName );
        if ( pFileName = Extra_FileGetSimilarName( pFileName, ".blif", ".bench", ".pla", ".baf", ".aig" ) )
            fprintf( pAbc->Err, "Did you mean \"%s\"?", pFileName );
        fprintf( pAbc->Err, "\n" );
        return 1;
    }
    fclose( pFile );

    // set the new network
    pDesign = Ver_ParseFile( pFileName, Abc_FrameReadLibVer(), fCheck, 1 );
    if ( pDesign == NULL )
    {
        fprintf( pAbc->Err, "Reading network from the verilog file has failed.\n" );
        return 1;
    }

    // derive root design
    pNtk = Abc_LibDeriveRoot( pDesign );
    Abc_LibFree( pDesign, NULL );
    if ( pNtk == NULL )
    {
        fprintf( pAbc->Err, "Deriving root module has failed.\n" );
        return 1;
    }

    // derive the AIG network from this design
    pNtkNew = Abc_LibDeriveAig( pNtk, Abc_FrameReadLibVer() );
    Abc_NtkDelete( pNtk );
    if ( pNtkNew == NULL )
    {
        fprintf( pAbc->Err, "Converting root module to AIG has failed.\n" );
        return 1;
    }
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtkNew );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_ver [-ch] <file>\n" );
    fprintf( pAbc->Err, "\t         read a network in structural verilog (using current library)\n" );
    fprintf( pAbc->Err, "\t-c     : toggle network check after reading [default = %s]\n", fCheck? "yes":"no" );
    fprintf( pAbc->Err, "\t-h     : prints the command summary\n" );
    fprintf( pAbc->Err, "\tfile   : the name of a file to read\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandReadVerLib( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Abc_Lib_t * pLibrary;
    char * pFileName;
    FILE * pFile;
    int fCheck;
    int c;
    extern Abc_Lib_t * Ver_ParseFile( char * pFileName, Abc_Lib_t * pGateLib, int fCheck, int fUseMemMan );

    fCheck = 1;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "ch" ) ) != EOF )
    {
        switch ( c )
        {
            case 'c':
                fCheck ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }

    if ( argc != globalUtilOptind + 1 )
    {
        goto usage;
    }

    // get the input file name
    pFileName = argv[globalUtilOptind];
    if ( (pFile = fopen( pFileName, "r" )) == NULL )
    {
        fprintf( pAbc->Err, "Cannot open input file \"%s\". ", pFileName );
        if ( pFileName = Extra_FileGetSimilarName( pFileName, ".blif", ".bench", ".pla", ".baf", ".aig" ) )
            fprintf( pAbc->Err, "Did you mean \"%s\"?", pFileName );
        fprintf( pAbc->Err, "\n" );
        return 1;
    }
    fclose( pFile );

    // set the new network
    pLibrary = Ver_ParseFile( pFileName, NULL, fCheck, 0 );
    if ( pLibrary == NULL )
    {
        fprintf( pAbc->Err, "Reading library from the verilog file has failed.\n" );
        return 1;
    }
    printf( "The library contains %d gates.\n", st_count(pLibrary->tModules) );
    // free old library
    if ( Abc_FrameReadLibVer() )
        Abc_LibFree( Abc_FrameReadLibVer(), NULL );
    // read new library
    Abc_FrameSetLibVer( pLibrary );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_verlib [-ch] <file>\n" );
    fprintf( pAbc->Err, "\t         read a gate library in structural verilog\n" );
    fprintf( pAbc->Err, "\t-c     : toggle network check after reading [default = %s]\n", fCheck? "yes":"no" );
    fprintf( pAbc->Err, "\t-h     : prints the command summary\n" );
    fprintf( pAbc->Err, "\tfile   : the name of a file to read\n" );
    return 1;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandWrite( Abc_Frame_t * pAbc, int argc, char **argv )
{
    char * pFileName;
    int c;

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "h" ) ) != EOF )
    {
        switch ( c )
        {
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the output file name
    pFileName = argv[globalUtilOptind];
    // call the corresponding file writer
    Io_Write( pAbc->pNtkCur, pFileName, Io_ReadFileType(pFileName) );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write [-h] <file>\n" );
    fprintf( pAbc->Err, "\t         writes the current network into <file> by calling\n" );
    fprintf( pAbc->Err, "\t         the writer that matches the extension of <file>\n" );
    fprintf( pAbc->Err, "\t-h     : print the help massage\n" );
    fprintf( pAbc->Err, "\tfile   : the name of the file to write\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandWriteHie( Abc_Frame_t * pAbc, int argc, char **argv )
{
    char * pBaseName, * pFileName;
    int c;

    glo_fMapped = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "mh" ) ) != EOF )
    {
        switch ( c )
        {
            case 'm':
                glo_fMapped ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( argc != globalUtilOptind + 2 )
        goto usage;
    // get the output file name
    pBaseName = argv[globalUtilOptind];
    pFileName = argv[globalUtilOptind+1];
    // call the corresponding file writer
//    Io_Write( pAbc->pNtkCur, pFileName, Io_ReadFileType(pFileName) );
    Io_WriteHie( pAbc->pNtkCur, pBaseName, pFileName );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_hie [-h] <orig> <file>\n" );
    fprintf( pAbc->Err, "\t         writes the current network into <file> by calling\n" );
    fprintf( pAbc->Err, "\t         the hierarchical writer that matches the extension of <file>\n" );
    fprintf( pAbc->Err, "\t-m     : toggle reading mapped Verilog for <orig> [default = %s]\n", glo_fMapped? "yes":"no" );
    fprintf( pAbc->Err, "\t-h     : print the help massage\n" );
    fprintf( pAbc->Err, "\torig   : the name of the original file with the hierarchical design\n" );
    fprintf( pAbc->Err, "\tfile   : the name of the file to write\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandWriteAiger( Abc_Frame_t * pAbc, int argc, char **argv )
{
    char * pFileName;
    int fWriteSymbols;
    int c;

    fWriteSymbols = 1;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "sh" ) ) != EOF )
    {
        switch ( c )
        {
            case 's':
                fWriteSymbols ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the output file name
    pFileName = argv[globalUtilOptind];
    // call the corresponding file writer
    if ( fWriteSymbols )
        Io_Write( pAbc->pNtkCur, pFileName, IO_FILE_AIGER );
    else
    {
        if ( !Abc_NtkIsStrash(pAbc->pNtkCur) )
        {
            fprintf( stdout, "Writing this format is only possible for structurally hashed AIGs.\n" );
            return 1;
        }
        Io_WriteAiger( pAbc->pNtkCur, pFileName, 0 );
    }
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_aiger [-sh] <file>\n" );
    fprintf( pAbc->Err, "\t         write the network in the AIGER format (http://fmv.jku.at/aiger)\n" );
    fprintf( pAbc->Err, "\t-s     : toggle saving I/O names [default = %s]\n", fWriteSymbols? "yes" : "no" );
    fprintf( pAbc->Err, "\t-h     : print the help massage\n" );
    fprintf( pAbc->Err, "\tfile   : the name of the file to write (extension .aig)\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandWriteBaf( Abc_Frame_t * pAbc, int argc, char **argv )
{
    char * pFileName;
    int c;

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "h" ) ) != EOF )
    {
        switch ( c )
        {
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the output file name
    pFileName = argv[globalUtilOptind];
    // call the corresponding file writer
    Io_Write( pAbc->pNtkCur, pFileName, IO_FILE_BAF );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_baf [-h] <file>\n" );
    fprintf( pAbc->Err, "\t         write the network into a BLIF file\n" );
    fprintf( pAbc->Err, "\t-h     : print the help massage\n" );
    fprintf( pAbc->Err, "\tfile   : the name of the file to write (extension .baf)\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandWriteBlif( Abc_Frame_t * pAbc, int argc, char **argv )
{
    char * pFileName;
    int c;

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "h" ) ) != EOF )
    {
        switch ( c )
        {
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the output file name
    pFileName = argv[globalUtilOptind];
    // call the corresponding file writer
    Io_Write( pAbc->pNtkCur, pFileName, IO_FILE_BLIF );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_blif [-h] <file>\n" );
    fprintf( pAbc->Err, "\t         write the network into a BLIF file\n" );
    fprintf( pAbc->Err, "\t-h     : print the help massage\n" );
    fprintf( pAbc->Err, "\tfile   : the name of the file to write (extension .blif)\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandWriteBlifMv( Abc_Frame_t * pAbc, int argc, char **argv )
{
    char * pFileName;
    int c;

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "h" ) ) != EOF )
    {
        switch ( c )
        {
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the output file name
    pFileName = argv[globalUtilOptind];
    // call the corresponding file writer
    Io_Write( pAbc->pNtkCur, pFileName, IO_FILE_BLIFMV );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_blif_mv [-h] <file>\n" );
    fprintf( pAbc->Err, "\t         write the network into a BLIF-MV file\n" );
    fprintf( pAbc->Err, "\t-h     : print the help massage\n" );
    fprintf( pAbc->Err, "\tfile   : the name of the file to write (extension .blif)\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandWriteBench( Abc_Frame_t * pAbc, int argc, char **argv )
{
    char * pFileName;
    int fUseLuts;
    int c;

    fUseLuts = 1;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "lh" ) ) != EOF )
    {
        switch ( c )
        {
            case 'l':
                fUseLuts ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the output file name
    pFileName = argv[globalUtilOptind];
    // call the corresponding file writer
    if ( !fUseLuts )
        Io_Write( pAbc->pNtkCur, pFileName, IO_FILE_BENCH );
    else
    {
        Abc_Ntk_t * pNtkTemp;
        pNtkTemp = Abc_NtkToNetlist( pAbc->pNtkCur );
        Abc_NtkToAig( pNtkTemp );
        Io_WriteBenchLut( pNtkTemp, pFileName );
        Abc_NtkDelete( pNtkTemp );
    }
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_bench [-lh] <file>\n" );
    fprintf( pAbc->Err, "\t         write the network in BENCH format\n" );
    fprintf( pAbc->Err, "\t-l     : toggle using LUTs in the output [default = %s]\n", fUseLuts? "yes" : "no" );
    fprintf( pAbc->Err, "\t-h     : print the help massage\n" );
    fprintf( pAbc->Err, "\tfile   : the name of the file to write (extension .bench)\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandWriteCellNet( Abc_Frame_t * pAbc, int argc, char **argv )
{
    Abc_Ntk_t * pNtk;
    char * pFileName;
    int c;
    extern void Io_WriteCellNet( Abc_Ntk_t * pNtk, char * pFileName );

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "h" ) ) != EOF )
    {
        switch ( c )
        {
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    pNtk = pAbc->pNtkCur;
    if ( pNtk == NULL )
    {
        fprintf( pAbc->Out, "Empty network.\n" );
        return 0;
    }
    // get the output file name
    pFileName = argv[globalUtilOptind];
    // call the corresponding file writer
    if ( !Abc_NtkIsLogic(pNtk) )
    {
        fprintf( pAbc->Out, "The network should be a logic network (if it an AIG, use command \"logic\")\n" );
        return 0;
    }
    Io_WriteCellNet( pNtk, pFileName );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_cellnet [-h] <file>\n" );
    fprintf( pAbc->Err, "\t         write the network is the cellnet format\n" );
    fprintf( pAbc->Err, "\t-h     : print the help massage\n" );
    fprintf( pAbc->Err, "\tfile   : the name of the file to write\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandWriteCnf( Abc_Frame_t * pAbc, int argc, char **argv )
{
    char * pFileName;
    int c;
    int fAllPrimes;
    int fNewAlgo;
    extern Abc_Ntk_t * Abc_NtkDarToCnf( Abc_Ntk_t * pNtk, char * pFileName );

    fNewAlgo = 1;
    fAllPrimes = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "nph" ) ) != EOF )
    {
        switch ( c )
        {
            case 'n':
                fNewAlgo ^= 1;
                break;
            case 'p':
                fAllPrimes ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the output file name
    pFileName = argv[globalUtilOptind];
    // check if the feature will be used
    if ( Abc_NtkIsStrash(pAbc->pNtkCur) && fAllPrimes )
    {
        fAllPrimes = 0;
        printf( "Warning: Selected option to write all primes has no effect when deriving CNF from AIG.\n" );
    }
    // call the corresponding file writer
    if ( fNewAlgo )
        Abc_NtkDarToCnf( pAbc->pNtkCur, pFileName );
    else if ( fAllPrimes )
        Io_WriteCnf( pAbc->pNtkCur, pFileName, 1 );
    else
        Io_Write( pAbc->pNtkCur, pFileName, IO_FILE_CNF );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_cnf [-nph] <file>\n" );
    fprintf( pAbc->Err, "\t         write the miter cone into a CNF file\n" );
    fprintf( pAbc->Err, "\t-n     : toggle using new algorithm [default = %s]\n", fNewAlgo? "yes" : "no" );
    fprintf( pAbc->Err, "\t-p     : toggle using all primes to enhance implicativity [default = %s]\n", fAllPrimes? "yes" : "no" );
    fprintf( pAbc->Err, "\t-h     : print the help massage\n" );
    fprintf( pAbc->Err, "\tfile   : the name of the file to write\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandWriteDot( Abc_Frame_t * pAbc, int argc, char **argv )
{
    char * pFileName;
    int c;

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "h" ) ) != EOF )
    {
        switch ( c )
        {
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the output file name
    pFileName = argv[globalUtilOptind];
    // call the corresponding file writer
    Io_Write( pAbc->pNtkCur, pFileName, IO_FILE_DOT );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_dot [-h] <file>\n" );
    fprintf( pAbc->Err, "\t         write the current network into a DOT file\n" );
    fprintf( pAbc->Err, "\t-h     : print the help massage\n" );
    fprintf( pAbc->Err, "\tfile   : the name of the file to write\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandWriteCounter( Abc_Frame_t * pAbc, int argc, char **argv )
{
    Abc_Ntk_t * pNtk;
    char * pFileName;
    int c;
    int fNames;

    fNames = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "nh" ) ) != EOF )
    {
        switch ( c )
        {
            case 'n':
                fNames ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }

    pNtk = pAbc->pNtkCur;
    if ( pNtk == NULL )
    {
        fprintf( pAbc->Out, "Empty network.\n" );
        return 0;
    }

    if ( argc != globalUtilOptind + 1 )
    {
        goto usage;
    }
    // get the input file name
    pFileName = argv[globalUtilOptind];

    if ( pNtk->pModel == NULL )
    {
        fprintf( pAbc->Out, "Counter-example is not available.\n" );
        return 0;
    }

    // write the counter-example into the file
    {
        Abc_Obj_t * pObj;
        FILE * pFile = fopen( pFileName, "w" );
        int i;
        if ( pFile == NULL )
        {
            fprintf( stdout, "IoCommandWriteCounter(): Cannot open the output file \"%s\".\n", pFileName );
            return 1;
        }
        if ( fNames )
        {
            Abc_NtkForEachPi( pNtk, pObj, i )
                fprintf( pFile, "%s=%c ", Abc_ObjName(pObj), '0'+(pNtk->pModel[i]==1) );
        }
        else
        {
            Abc_NtkForEachPi( pNtk, pObj, i )
                fprintf( pFile, "%c", '0'+(pNtk->pModel[i]==1) );
        }
        fprintf( pFile, "\n" );
        fclose( pFile );
    }

    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_counter [-nh] <file>\n" );
    fprintf( pAbc->Err, "\t         writes the counter-example derived by \"prove\" or \"sat\"\n" );
    fprintf( pAbc->Err, "\t         the file contains values for each PI in the natural order\n" );
    fprintf( pAbc->Err, "\t-n     : write input names into the file [default = %s]\n", fNames? "yes": "no" );
    fprintf( pAbc->Err, "\t-h     : print the help massage\n" );
    fprintf( pAbc->Err, "\tfile   : the name of the file to write\n" );
    return 1;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandWriteEqn( Abc_Frame_t * pAbc, int argc, char **argv )
{
    char * pFileName;
    int c;

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "h" ) ) != EOF )
    {
        switch ( c )
        {
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the output file name
    pFileName = argv[globalUtilOptind];
    // call the corresponding file writer
    Io_Write( pAbc->pNtkCur, pFileName, IO_FILE_EQN );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_eqn [-h] <file>\n" );
    fprintf( pAbc->Err, "\t         write the current network in the equation format\n" );
    fprintf( pAbc->Err, "\t-h     : print the help massage\n" );
    fprintf( pAbc->Err, "\tfile   : the name of the file to write\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandWriteGml( Abc_Frame_t * pAbc, int argc, char **argv )
{
    char * pFileName;
    int c;

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "h" ) ) != EOF )
    {
        switch ( c )
        {
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the output file name
    pFileName = argv[globalUtilOptind];
    // call the corresponding file writer
    Io_Write( pAbc->pNtkCur, pFileName, IO_FILE_GML );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_gml [-h] <file>\n" );
    fprintf( pAbc->Err, "\t         write network using graph representation formal GML\n" );
    fprintf( pAbc->Err, "\t-h     : print the help massage\n" );
    fprintf( pAbc->Err, "\tfile   : the name of the file to write\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandWriteList( Abc_Frame_t * pAbc, int argc, char **argv )
{
    char * pFileName;
    int fUseHost;
    int c;

    printf( "This command currently does not work.\n" );
    return 0;

    fUseHost = 1;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "nh" ) ) != EOF )
    {
        switch ( c )
        {
            case 'n':
                fUseHost ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    if ( pAbc->pNtkCur == NULL )
    {
        fprintf( pAbc->Out, "Empty network.\n" );
        return 0;
    }
/*
    if ( !Abc_NtkIsSeq(pAbc->pNtkCur) )
    {
        fprintf( stdout, "IoCommandWriteList(): Can write adjacency list for sequential AIGs only.\n" );
        return 0;
    }
*/
    // get the input file name
    pFileName = argv[globalUtilOptind];
    // write the file
    Io_WriteList( pAbc->pNtkCur, pFileName, fUseHost );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_list [-nh] <file>\n" );
    fprintf( pAbc->Err, "\t         write network using graph representation formal GML\n" );
    fprintf( pAbc->Err, "\t-n     : toggle writing host node [default = %s]\n", fUseHost? "yes":"no" );
    fprintf( pAbc->Err, "\t-h     : print the help massage\n" );
    fprintf( pAbc->Err, "\tfile   : the name of the file to write\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandWritePla( Abc_Frame_t * pAbc, int argc, char **argv )
{
    char * pFileName;
    int c;

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "h" ) ) != EOF )
    {
        switch ( c )
        {
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the output file name
    pFileName = argv[globalUtilOptind];
    // call the corresponding file writer
    Io_Write( pAbc->pNtkCur, pFileName, IO_FILE_PLA );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_pla [-h] <file>\n" );
    fprintf( pAbc->Err, "\t         write the collapsed network into a PLA file\n" );
    fprintf( pAbc->Err, "\t-h     : print the help massage\n" );
    fprintf( pAbc->Err, "\tfile   : the name of the file to write\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandWriteVerilog( Abc_Frame_t * pAbc, int argc, char **argv )
{
    char * pFileName;
    int c;

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "h" ) ) != EOF )
    {
        switch ( c )
        {
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the output file name
    pFileName = argv[globalUtilOptind];
    // call the corresponding file writer
    Io_Write( pAbc->pNtkCur, pFileName, IO_FILE_VERILOG );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_verilog [-h] <file>\n" );
    fprintf( pAbc->Err, "\t         write the current network in Verilog format\n" );
    fprintf( pAbc->Err, "\t-h     : print the help massage\n" );
    fprintf( pAbc->Err, "\tfile   : the name of the file to write\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandWriteVerLib( Abc_Frame_t * pAbc, int argc, char **argv )
{
    Abc_Lib_t * pLibrary;
    char * pFileName;
    int c;
    extern void Io_WriteVerilogLibrary( Abc_Lib_t * pLibrary, char * pFileName );

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "h" ) ) != EOF )
    {
        switch ( c )
        {
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the input file name
    pFileName = argv[globalUtilOptind];
    // derive the netlist
    pLibrary = Abc_FrameReadLibVer();
    if ( pLibrary == NULL )
    {
        fprintf( pAbc->Out, "Verilog library is not specified.\n" );
        return 0;
    }
//    Io_WriteVerilogLibrary( pLibrary, pFileName );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_verlib [-h] <file>\n" );
    fprintf( pAbc->Err, "\t         write the current verilog library\n" );
    fprintf( pAbc->Err, "\t-h     : print the help massage\n" );
    fprintf( pAbc->Err, "\tfile   : the name of the file to write\n" );
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


