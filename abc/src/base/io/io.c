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

#include "ioAbc.h"
#include "base/main/mainInt.h"
#include "aig/saig/saig.h"
#include "proof/abs/abs.h"
#include "sat/bmc/bmc.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int IoCommandRead        ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandReadAiger   ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandReadBaf     ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandReadBblif   ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandReadBlif    ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandReadBlifMv  ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandReadBench   ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandReadDsd     ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandReadEdif    ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandReadEqn     ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandReadFins    ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandReadInit    ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandReadPla     ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandReadPlaMo   ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandReadTruth   ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandReadVerilog ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandReadStatus  ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandReadGig     ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandReadJson    ( Abc_Frame_t * pAbc, int argc, char **argv );

static int IoCommandWrite       ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteHie    ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteAiger  ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteAigerCex( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteBaf    ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteBblif  ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteBlif   ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteBlifMv ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteBench  ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteBook   ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteCellNet( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteCnf    ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteCnf2   ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteCex    ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteDot    ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteEqn    ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteGml    ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteList   ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWritePla    ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteVerilog( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteSortCnf( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteTruth  ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteTruths ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteStatus ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteSmv    ( Abc_Frame_t * pAbc, int argc, char **argv );
static int IoCommandWriteJson   ( Abc_Frame_t * pAbc, int argc, char **argv );

extern void Abc_FrameCopyLTLDataBase( Abc_Frame_t *pAbc, Abc_Ntk_t * pNtk );

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
    Cmd_CommandAdd( pAbc, "I/O", "read_bblif",    IoCommandReadBblif,    1 );
    Cmd_CommandAdd( pAbc, "I/O", "read_blif",     IoCommandReadBlif,     1 );
    Cmd_CommandAdd( pAbc, "I/O", "read_blif_mv",  IoCommandReadBlifMv,   1 );
    Cmd_CommandAdd( pAbc, "I/O", "read_bench",    IoCommandReadBench,    1 );
    Cmd_CommandAdd( pAbc, "I/O", "read_dsd",      IoCommandReadDsd,      1 );
//    Cmd_CommandAdd( pAbc, "I/O", "read_edif",     IoCommandReadEdif,     1 );
    Cmd_CommandAdd( pAbc, "I/O", "read_eqn",      IoCommandReadEqn,      1 );
    Cmd_CommandAdd( pAbc, "I/O", "read_fins",     IoCommandReadFins,     0 );
    Cmd_CommandAdd( pAbc, "I/O", "read_init",     IoCommandReadInit,     1 );
    Cmd_CommandAdd( pAbc, "I/O", "read_pla",      IoCommandReadPla,      1 );
    Cmd_CommandAdd( pAbc, "I/O", "read_plamo",    IoCommandReadPlaMo,    1 );
    Cmd_CommandAdd( pAbc, "I/O", "read_truth",    IoCommandReadTruth,    1 );
    Cmd_CommandAdd( pAbc, "I/O", "read_verilog",  IoCommandReadVerilog,  1 );
    Cmd_CommandAdd( pAbc, "I/O", "read_status",   IoCommandReadStatus,   0 );
    Cmd_CommandAdd( pAbc, "I/O", "&read_gig",     IoCommandReadGig,      0 );
    Cmd_CommandAdd( pAbc, "I/O", "read_json",     IoCommandReadJson,     0 );

    Cmd_CommandAdd( pAbc, "I/O", "write",         IoCommandWrite,        0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_hie",     IoCommandWriteHie,     0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_aiger",   IoCommandWriteAiger,   0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_aiger_cex",   IoCommandWriteAigerCex,   0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_baf",     IoCommandWriteBaf,     0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_bblif",   IoCommandWriteBblif,   0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_blif",    IoCommandWriteBlif,    0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_blif_mv", IoCommandWriteBlifMv,  0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_bench",   IoCommandWriteBench,   0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_book",    IoCommandWriteBook,    0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_cellnet", IoCommandWriteCellNet, 0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_cex",     IoCommandWriteCex,     0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_cnf",     IoCommandWriteCnf,     0 );
    Cmd_CommandAdd( pAbc, "I/O", "&write_cnf",    IoCommandWriteCnf2,    0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_dot",     IoCommandWriteDot,     0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_eqn",     IoCommandWriteEqn,     0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_gml",     IoCommandWriteGml,     0 );
//    Cmd_CommandAdd( pAbc, "I/O", "write_list",    IoCommandWriteList,    0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_pla",     IoCommandWritePla,     0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_verilog", IoCommandWriteVerilog, 0 );
//    Cmd_CommandAdd( pAbc, "I/O", "write_verlib",  IoCommandWriteVerLib,  0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_sorter_cnf", IoCommandWriteSortCnf,  0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_truth",   IoCommandWriteTruth,   0 );
    Cmd_CommandAdd( pAbc, "I/O", "&write_truths", IoCommandWriteTruths,  0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_status",  IoCommandWriteStatus,  0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_smv",     IoCommandWriteSmv,     0 );
    Cmd_CommandAdd( pAbc, "I/O", "write_json",    IoCommandWriteJson,    0 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_End( Abc_Frame_t * pAbc )
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
    char Command[1000];
    Abc_Ntk_t * pNtk;
    char * pFileName, * pTemp;
    int c, fCheck, fBarBufs, fReadGia;

    fCheck = 1;
    fBarBufs = 0;
    fReadGia = 0;
    glo_fMapped = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "mcbgh" ) ) != EOF )
    {
        switch ( c )
        {
            case 'm':
                glo_fMapped ^= 1;
                break;
            case 'c':
                fCheck ^= 1;
                break;
            case 'b':
                fBarBufs ^= 1;
                break;
            case 'g':
                fReadGia ^= 1;
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
    // fix the wrong symbol
    for ( pTemp = pFileName; *pTemp; pTemp++ )
        if ( *pTemp == '>' || *pTemp == '\\' )
            *pTemp = '/';
    // read libraries
    Command[0] = 0;
    assert( strlen(pFileName) < 900 );
    if ( !strcmp( Extra_FileNameExtension(pFileName), "genlib" )  )
        sprintf( Command, "read_genlib %s", pFileName );
    else if ( !strcmp( Extra_FileNameExtension(pFileName), "lib" ) )
        sprintf( Command, "read_lib %s", pFileName );
    else if ( !strcmp( Extra_FileNameExtension(pFileName), "scl" ) )
        sprintf( Command, "read_scl %s", pFileName );
    else if ( !strcmp( Extra_FileNameExtension(pFileName), "super" ) )
        sprintf( Command, "read_super %s", pFileName );
    else if ( !strcmp( Extra_FileNameExtension(pFileName), "constr" ) )
        sprintf( Command, "read_constr %s", pFileName );
    else if ( !strcmp( Extra_FileNameExtension(pFileName), "c" ) )
        sprintf( Command, "so %s", pFileName );
    else if ( !strcmp( Extra_FileNameExtension(pFileName), "dsd" ) )
        sprintf( Command, "dsd_load %s", pFileName );
    if ( Command[0] )
    {
        Cmd_CommandExecute( pAbc, Command );
        return 0;
    }
    if ( fReadGia )
    {
        Abc_Ntk_t * pNtk = Io_ReadNetlist( pFileName, Io_ReadFileType(pFileName), fCheck );
        if ( pNtk )
        {
            Gia_Man_t * pGia = Abc_NtkFlattenHierarchyGia( pNtk, NULL, 0 );
            Abc_NtkDelete( pNtk );
            if ( pGia == NULL )
            {
                Abc_Print( 1, "Abc_CommandBlast(): Bit-blasting has failed.\n" );
                return 0;
            }
            Abc_FrameUpdateGia( pAbc, pGia );
        }
        return 0;
    }
    // check if the library is available
    if ( glo_fMapped && Abc_FrameReadLibGen() == NULL )
    {
        Abc_Print( 1, "Cannot read mapped design when the library is not given.\n" );
        return 0;
    }
    // read the file using the corresponding file reader
    pNtk = Io_Read( pFileName, Io_ReadFileType(pFileName), fCheck, fBarBufs );
    if ( pNtk == NULL )
        return 0;
    if ( Abc_NtkPiNum(pNtk) == 0 )
    {
        Abc_Print( 0, "The new network has no primary inputs. It is recommended\n" );
        Abc_Print( 1, "to add a dummy PI to make sure all commands work correctly.\n" );
    }
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtk );
    Abc_FrameCopyLTLDataBase( pAbc, pNtk );
    Abc_FrameClearVerifStatus( pAbc );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read [-mcbgh] <file>\n" );
    fprintf( pAbc->Err, "\t         replaces the current network by the network read from <file>\n" );
    fprintf( pAbc->Err, "\t         by calling the parser that matches the extension of <file>\n" );
    fprintf( pAbc->Err, "\t         (to read a hierarchical design, use \"read_hie\")\n" );
    fprintf( pAbc->Err, "\t-m     : toggle reading mapped Verilog [default = %s]\n", glo_fMapped? "yes":"no" );
    fprintf( pAbc->Err, "\t-c     : toggle network check after reading [default = %s]\n", fCheck? "yes":"no" );
    fprintf( pAbc->Err, "\t-b     : toggle reading barrier buffers [default = %s]\n", fBarBufs? "yes":"no" );
    fprintf( pAbc->Err, "\t-g     : toggle reading and flattening into &-space [default = %s]\n", fBarBufs? "yes":"no" );
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
    pNtk = Io_Read( pFileName, IO_FILE_AIGER, fCheck, 0 );
    if ( pNtk == NULL )
        return 1;
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtk );
    Abc_FrameClearVerifStatus( pAbc );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_aiger [-ch] <file>\n" );
    fprintf( pAbc->Err, "\t         reads the network in the AIGER format (http://fmv.jku.at/aiger)\n" );
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
    pNtk = Io_Read( pFileName, IO_FILE_BAF, fCheck, 0 );
    if ( pNtk == NULL )
        return 1;
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtk );
    Abc_FrameClearVerifStatus( pAbc );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_baf [-ch] <file>\n" );
    fprintf( pAbc->Err, "\t         reads the network in Binary Aig Format (BAF)\n" );
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
int IoCommandReadBblif( Abc_Frame_t * pAbc, int argc, char ** argv )
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
    pNtk = Io_Read( pFileName, IO_FILE_BBLIF, fCheck, 0 );
    if ( pNtk == NULL )
        return 1;
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtk );
    Abc_FrameClearVerifStatus( pAbc );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_bblif [-ch] <file>\n" );
    fprintf( pAbc->Err, "\t         reads the network in a binary BLIF format\n" );
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
    int fUseNewParser;
    int fSaveNames;
    int c;
    extern Abc_Ntk_t * Io_ReadBlifAsAig( char * pFileName, int fCheck );

    fCheck = 1;
    fReadAsAig = 0;
    fUseNewParser = 1;
    fSaveNames = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "nmach" ) ) != EOF )
    {
        switch ( c )
        {
            case 'n':
                fUseNewParser ^= 1;
                break;
            case 'm':
                fSaveNames ^= 1;
                break;
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
    else if ( fUseNewParser )
        pNtk = Io_Read( pFileName, IO_FILE_BLIF, fCheck, 0 );
    else
    {
        Abc_Ntk_t * pTemp;
        pNtk = Io_ReadBlif( pFileName, fCheck );
        if ( pNtk == NULL )
            return 1;
        if ( fSaveNames )
            Abc_NtkStartNameIds( pNtk );
        pNtk = Abc_NtkToLogic( pTemp = pNtk );
        if ( fSaveNames )
            Abc_NtkTransferNameIds( pTemp, pNtk );
        Abc_NtkDelete( pTemp );
    }

    if ( pNtk == NULL )
        return 1;
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtk );
    Abc_FrameClearVerifStatus( pAbc );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_blif [-nmach] <file>\n" );
    fprintf( pAbc->Err, "\t         reads the network in binary BLIF format\n" );
    fprintf( pAbc->Err, "\t         (if this command does not work, try \"read\")\n" );
    fprintf( pAbc->Err, "\t-n     : toggle using old BLIF parser without hierarchy support [default = %s]\n", !fUseNewParser? "yes":"no" );
    fprintf( pAbc->Err, "\t-m     : toggle saving original circuit names into a file [default = %s]\n", fSaveNames? "yes":"no" );
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
    pNtk = Io_Read( pFileName, IO_FILE_BLIFMV, fCheck, 0 );
    if ( pNtk == NULL )
        return 1;
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtk );
    Abc_FrameClearVerifStatus( pAbc );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_blif_mv [-ch] <file>\n" );
    fprintf( pAbc->Err, "\t         reads the network in BLIF-MV format\n" );
    fprintf( pAbc->Err, "\t         (if this command does not work, try \"read\")\n" );
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
    pNtk = Io_Read( pFileName, IO_FILE_BENCH, fCheck, 0 );
    if ( pNtk == NULL )
        return 1;
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtk );
    Abc_FrameClearVerifStatus( pAbc );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_bench [-ch] <file>\n" );
    fprintf( pAbc->Err, "\t         reads the network in BENCH format\n" );
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
    Abc_FrameClearVerifStatus( pAbc );
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
    fprintf( pAbc->Err, "\t          No spaces are allowed in formulas. In parentheses, LSB goes first.\n" );
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
    pNtk = Io_Read( pFileName, IO_FILE_EDIF, fCheck, 0 );
    if ( pNtk == NULL )
        return 1;
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtk );
    Abc_FrameClearVerifStatus( pAbc );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_edif [-ch] <file>\n" );
    fprintf( pAbc->Err, "\t         reads the network in EDIF (works only for ISCAS benchmarks)\n" );
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
    pNtk = Io_Read( pFileName, IO_FILE_EQN, fCheck, 0 );
    if ( pNtk == NULL )
        return 1;
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtk );
    Abc_FrameClearVerifStatus( pAbc );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_eqn [-ch] <file>\n" );
    fprintf( pAbc->Err, "\t         reads the network in equation format\n" );
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
int IoCommandReadFins( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    extern Vec_Int_t * Io_ReadFins( Abc_Ntk_t * pNtk, char * pFileName, int fVerbose );
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
    char * pFileName;
    int c, fVerbose = 0;

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "vh" ) ) != EOF )
    {
        switch ( c )
        {
            case 'v':
                fVerbose ^= 1;
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
    // check current network
    if ( pNtk == NULL )
    {
        Abc_Print( -1, "Empty network.\n" );
        return 1;
    }
    // compute information and save it in the network
    Vec_IntFreeP( &pNtk->vFins );
    pNtk->vFins = Io_ReadFins( pNtk, pFileName, fVerbose );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_fins [-vh] <file>\n" );
    fprintf( pAbc->Err, "\t         reads the network in equation format\n" );
    fprintf( pAbc->Err, "\t-v     : enable verbose output [default = %s].\n", fVerbose? "yes": "no" );
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
int IoCommandReadInit( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Abc_Ntk_t * pNtk;
    char * pFileName;
    int c;

    pNtk = Abc_FrameReadNtk(pAbc);
    pOut = Abc_FrameReadOut(pAbc);
    pErr = Abc_FrameReadErr(pAbc);

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
    if ( argc != globalUtilOptind && argc != globalUtilOptind + 1 )
        goto usage;

    if ( pNtk == NULL )
    {
        fprintf( pErr, "Empty network.\n" );
        return 1;
    }
    // get the input file name
    if ( argc == globalUtilOptind + 1 )
        pFileName = argv[globalUtilOptind];
    else if ( pNtk->pSpec )
        pFileName = Extra_FileNameGenericAppend( pNtk->pSpec, ".init" );
    else
    {
        printf( "File name should be given on the command line.\n" );
        return 1;
    }

    // read the file using the corresponding file reader
    pNtk = Abc_NtkDup( pNtk );
    Io_ReadBenchInit( pNtk, pFileName );
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtk );
    Abc_FrameClearVerifStatus( pAbc );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_init [-h] <file>\n" );
    fprintf( pAbc->Err, "\t         reads initial state of the network in BENCH format\n" );
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
    int c, fZeros = 0, fBoth = 0, fOnDc = 0, fSkipPrepro = 0, fCheck = 1;

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "zbdxch" ) ) != EOF )
    {
        switch ( c )
        {
            case 'z':
                fZeros ^= 1;
                break;
            case 'b':
                fBoth ^= 1;
                break;
            case 'd':
                fOnDc ^= 1;
                break;
            case 'x':
                fSkipPrepro ^= 1;
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
    if ( fZeros || fBoth || fOnDc || fSkipPrepro )
    {
        Abc_Ntk_t * pTemp;
        pNtk = Io_ReadPla( pFileName, fZeros, fBoth, fOnDc, fSkipPrepro, fCheck );
        if ( pNtk == NULL )
        {
            printf( "Reading PLA file has failed.\n" );
            return 1;
        }
        pNtk = Abc_NtkToLogic( pTemp = pNtk );
        Abc_NtkDelete( pTemp );
    }
    else
        pNtk = Io_Read( pFileName, IO_FILE_PLA, fCheck, 0 );
    if ( pNtk == NULL )
        return 1;
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtk );
    Abc_FrameClearVerifStatus( pAbc );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_pla [-zbdxch] <file>\n" );
    fprintf( pAbc->Err, "\t         reads the network in PLA\n" );
    fprintf( pAbc->Err, "\t-z     : toggle reading on-set and off-set [default = %s]\n", fZeros? "off-set":"on-set" );
    fprintf( pAbc->Err, "\t-b     : toggle reading both on-set and off-set as on-set [default = %s]\n", fBoth? "off-set":"on-set" );
    fprintf( pAbc->Err, "\t-d     : toggle reading both on-set and dc-set as on-set [default = %s]\n", fOnDc? "off-set":"on-set" );
    fprintf( pAbc->Err, "\t-x     : toggle reading Exclusive SOP rather than SOP [default = %s]\n", fSkipPrepro? "yes":"no" );
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
int IoCommandReadPlaMo( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    extern Abc_Ntk_t * Mop_ManTest( char * pFileName, int fMerge, int fVerbose );
    Abc_Ntk_t * pNtk;
    int c, fMerge = 0, fVerbose = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "mvh" ) ) != EOF )
    {
        switch ( c )
        {
            case 'm':
                fMerge ^= 1;
                break;
            case 'v':
                fVerbose ^= 1;
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
    pNtk = Mop_ManTest( argv[globalUtilOptind], fMerge, fVerbose );
    if ( pNtk == NULL )
        return 1;
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtk );
    Abc_FrameClearVerifStatus( pAbc );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_plamo [-mvh] <file>\n" );
    fprintf( pAbc->Err, "\t         reads the network in multi-output PLA\n" );
    fprintf( pAbc->Err, "\t-m     : toggle dist-1 merge for cubes with identical outputs [default = %s]\n", fMerge? "yes":"no" );
    fprintf( pAbc->Err, "\t-v     : toggle printing verbose information [default = %s]\n", fVerbose? "yes":"no" );
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
    if ( pSopCover == NULL || pSopCover[0] == 0 )
    {
        ABC_FREE( pSopCover );
        fprintf( pAbc->Err, "Reading truth table has failed.\n" );
        return 1;
    }

    pNtk = Abc_NtkCreateWithNode( pSopCover );
    ABC_FREE( pSopCover );
    if ( pNtk == NULL )
    {
        fprintf( pAbc->Err, "Deriving the network has failed.\n" );
        return 1;
    }
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtk );
    Abc_FrameClearVerifStatus( pAbc );
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
    int fCheck, fBarBufs;
    int c;

    fCheck = 1;
    fBarBufs = 0;
    glo_fMapped = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "mcbh" ) ) != EOF )
    {
        switch ( c )
        {
            case 'm':
                glo_fMapped ^= 1;
                break;
            case 'c':
                fCheck ^= 1;
                break;
            case 'b':
                fBarBufs ^= 1;
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
    pNtk = Io_Read( pFileName, IO_FILE_VERILOG, fCheck, fBarBufs );
    if ( pNtk == NULL )
        return 1;
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtk );
    Abc_FrameClearVerifStatus( pAbc );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_verilog [-mcbh] <file>\n" );
    fprintf( pAbc->Err, "\t         reads the network in Verilog (IWLS 2002/2005 subset)\n" );
    fprintf( pAbc->Err, "\t-m     : toggle reading mapped Verilog [default = %s]\n", glo_fMapped? "yes":"no" );
    fprintf( pAbc->Err, "\t-c     : toggle network check after reading [default = %s]\n", fCheck? "yes":"no" );
    fprintf( pAbc->Err, "\t-b     : toggle reading barrier buffers [default = %s]\n", fBarBufs? "yes":"no" );
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
int IoCommandReadStatus( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    char * pFileName;
    FILE * pFile;
    int c;
    extern int Abc_NtkReadLogFile( char * pFileName, Abc_Cex_t ** ppCex, int * pnFrames );

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
    {
        goto usage;
    }

    // get the input file name
    pFileName = argv[globalUtilOptind];
    if ( (pFile = fopen( pFileName, "r" )) == NULL )
    {
        fprintf( pAbc->Err, "Cannot open input file \"%s\". \n", pFileName );
        return 1;
    }
    fclose( pFile );

    // set the new network
    Abc_FrameClearVerifStatus( pAbc );
    pAbc->Status = Abc_NtkReadLogFile( pFileName, &pAbc->pCex, &pAbc->nFrames );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_status [-ch] <file>\n" );
    fprintf( pAbc->Err, "\t         reads verification log file\n" );
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
int IoCommandReadGig( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    extern Gia_Man_t * Gia_ManReadGig( char * pFileName );
    Gia_Man_t * pAig;
    char * pFileName;
    FILE * pFile;
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
    {
        goto usage;
    }

    // get the input file name
    pFileName = argv[globalUtilOptind];
    if ( (pFile = fopen( pFileName, "r" )) == NULL )
    {
        fprintf( pAbc->Err, "Cannot open input file \"%s\". \n", pFileName );
        return 1;
    }
    fclose( pFile );

    // set the new network
    pAig = Gia_ManReadGig( pFileName );
    Abc_FrameUpdateGia( pAbc, pAig );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: &read_gig [-h] <file>\n" );
    fprintf( pAbc->Err, "\t         reads design in GIG format\n" );
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
int IoCommandReadJson( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    extern Vec_Wec_t * Json_Read( char * pFileName, Abc_Nam_t ** ppStrs );
    Vec_Wec_t * vObjs;
    Abc_Nam_t * pStrs;
    char * pFileName;
    FILE * pFile;
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
    {
        goto usage;
    }

    // get the input file name
    pFileName = argv[globalUtilOptind];
    if ( (pFile = fopen( pFileName, "r" )) == NULL )
    {
        fprintf( pAbc->Err, "Cannot open input file \"%s\". \n", pFileName );
        return 1;
    }
    fclose( pFile );

    // set the new network
    vObjs = Json_Read( pFileName, &pStrs );
    if ( vObjs == NULL )
        return 0;
    Abc_FrameSetJsonStrs( pStrs );
    Abc_FrameSetJsonObjs( vObjs );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_json [-h] <file>\n" );
    fprintf( pAbc->Err, "\t         reads file in JSON format\n" );
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
    char Command[1000];
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
    // write libraries
    Command[0] = 0;
    assert( strlen(pFileName) < 900 );
    if ( !strcmp( Extra_FileNameExtension(pFileName), "genlib" )  )
        sprintf( Command, "write_genlib %s", pFileName );
    else if ( !strcmp( Extra_FileNameExtension(pFileName), "lib" ) )
        sprintf( Command, "write_liberty %s", pFileName );
    else if ( !strcmp( Extra_FileNameExtension(pFileName), "dsd" ) )
        sprintf( Command, "dsd_save %s", pFileName );
    if ( Command[0] )
    {
        Cmd_CommandExecute( pAbc, Command );
        return 0;
    }
    if ( pAbc->pNtkCur == NULL )
    {
        fprintf( pAbc->Out, "Empty network.\n" );
        return 0;
    }
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
    if ( pAbc->pNtkCur == NULL )
    {
        fprintf( pAbc->Out, "Empty network.\n" );
        return 0;
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
    int fCompact;
    int fUnique;
    int fVerbose;
    int c;

    fWriteSymbols = 0;
    fCompact      = 0;
    fUnique       = 0;
    fVerbose      = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "scuvh" ) ) != EOF )
    {
        switch ( c )
        {
            case 's':
                fWriteSymbols ^= 1;
                break;
            case 'c':
                fCompact ^= 1;
                break;
            case 'u':
                fUnique ^= 1;
                break;
            case 'v':
                fVerbose ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( pAbc->pNtkCur == NULL )
    {
        fprintf( pAbc->Out, "Empty network.\n" );
        return 0;
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the output file name
    pFileName = argv[globalUtilOptind];
    // call the corresponding file writer
    if ( !Abc_NtkIsStrash(pAbc->pNtkCur) )
    {
        fprintf( stdout, "Writing this format is only possible for structurally hashed AIGs.\n" );
        return 1;
    }
    if ( fUnique )
    {
        extern Aig_Man_t * Abc_NtkToDar( Abc_Ntk_t * pNtk, int fExors, int fRegisters );
        extern Abc_Ntk_t * Abc_NtkFromAigPhase( Aig_Man_t * pMan );
        Aig_Man_t * pAig = Abc_NtkToDar( pAbc->pNtkCur, 0, 1 );
        Aig_Man_t * pCan = Saig_ManDupIsoCanonical( pAig, fVerbose );
        Abc_Ntk_t * pTemp = Abc_NtkFromAigPhase( pCan );
        Aig_ManStop( pCan );
        Aig_ManStop( pAig );
        Io_WriteAiger( pTemp, pFileName, fWriteSymbols, fCompact, fUnique );
        Abc_NtkDelete( pTemp );
    }
    else
        Io_WriteAiger( pAbc->pNtkCur, pFileName, fWriteSymbols, fCompact, fUnique );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_aiger [-scuvh] <file>\n" );
    fprintf( pAbc->Err, "\t         writes the network in the AIGER format (http://fmv.jku.at/aiger)\n" );
    fprintf( pAbc->Err, "\t-s     : toggle saving I/O names [default = %s]\n", fWriteSymbols? "yes" : "no" );
    fprintf( pAbc->Err, "\t-c     : toggle writing more compactly [default = %s]\n", fCompact? "yes" : "no" );
    fprintf( pAbc->Err, "\t-u     : toggle writing canonical AIG structure [default = %s]\n", fUnique? "yes" : "no" );
    fprintf( pAbc->Err, "\t-v     : toggle printing verbose information [default = %s]\n", fVerbose? "yes" : "no" );
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
int IoCommandWriteAigerCex( Abc_Frame_t * pAbc, int argc, char **argv )
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
    if ( pAbc->pCex == NULL )
    {
        fprintf( pAbc->Out, "There is no current CEX.\n" );
        return 0;
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the output file name
    pFileName = argv[globalUtilOptind];
    Io_WriteAigerCex( pAbc->pCex, pAbc->pNtkCur, pAbc->pGia, pFileName );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_aiger_cex [-h] <file>\n" );
    fprintf( pAbc->Err, "\t         writes the current CEX in the AIGER format (http://fmv.jku.at/aiger)\n" );
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
    if ( pAbc->pNtkCur == NULL )
    {
        fprintf( pAbc->Out, "Empty network.\n" );
        return 0;
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
    fprintf( pAbc->Err, "\t         writes the network into a BLIF file\n" );
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
int IoCommandWriteBblif( Abc_Frame_t * pAbc, int argc, char **argv )
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
    if ( pAbc->pNtkCur == NULL )
    {
        fprintf( pAbc->Out, "Empty network.\n" );
        return 0;
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the output file name
    pFileName = argv[globalUtilOptind];
    // call the corresponding file writer
    Io_Write( pAbc->pNtkCur, pFileName, IO_FILE_BBLIF );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_bblif [-h] <file>\n" );
    fprintf( pAbc->Err, "\t         writes the network into a binary BLIF file\n" );
    fprintf( pAbc->Err, "\t-h     : print the help massage\n" );
    fprintf( pAbc->Err, "\tfile   : the name of the file to write (extension .bblif)\n" );
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
    char * pLutStruct = NULL;
    int c, fSpecial = 0;
    int fUseHie = 0;

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "Sjah" ) ) != EOF )
    {
        switch ( c )
        {
            case 'S':
                if ( globalUtilOptind >= argc )
                {
                    Abc_Print( -1, "Command line switch \"-S\" should be followed by string.\n" );
                    goto usage;
                }
                pLutStruct = argv[globalUtilOptind];
                globalUtilOptind++;
                if ( strlen(pLutStruct) != 2 && strlen(pLutStruct) != 3 ) 
                {
                    Abc_Print( -1, "Command line switch \"-S\" should be followed by a 2- or 3-char string (e.g. \"44\" or \"555\").\n" );
                    goto usage;
                }
                break;
            case 'j':
                fSpecial ^= 1;
                break;
            case 'a':
                fUseHie ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( pAbc->pNtkCur == NULL )
    {
        fprintf( pAbc->Out, "Empty network.\n" );
        return 0;
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the output file name
    pFileName = argv[globalUtilOptind];
    // call the corresponding file writer
    if ( fSpecial || pLutStruct )
        Io_WriteBlifSpecial( pAbc->pNtkCur, pFileName, pLutStruct, fUseHie );
    else
        Io_Write( pAbc->pNtkCur, pFileName, IO_FILE_BLIF );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_blif [-S str] [-jah] <file>\n" );
    fprintf( pAbc->Err, "\t         writes the network into a BLIF file\n" );
    fprintf( pAbc->Err, "\t-S str : string representing the LUT structure [default = %s]\n", pLutStruct ? pLutStruct : "not used" );  
    fprintf( pAbc->Err, "\t-j     : enables special BLIF writing [default = %s]\n", fSpecial? "yes" : "no" );;
    fprintf( pAbc->Err, "\t-a     : enables hierarchical BLIF writing for LUT structures [default = %s]\n", fUseHie? "yes" : "no" );;
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
    if ( pAbc->pNtkCur == NULL )
    {
        fprintf( pAbc->Out, "Empty network.\n" );
        return 0;
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
    fprintf( pAbc->Err, "\t         writes the network into a BLIF-MV file\n" );
    fprintf( pAbc->Err, "\t-h     : print the help massage\n" );
    fprintf( pAbc->Err, "\tfile   : the name of the file to write (extension .mv)\n" );
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
    if ( pAbc->pNtkCur == NULL )
    {
        fprintf( pAbc->Out, "Empty network.\n" );
        return 0;
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the output file name
    pFileName = argv[globalUtilOptind];
    // call the corresponding file writer
    if ( !fUseLuts )
        Io_Write( pAbc->pNtkCur, pFileName, IO_FILE_BENCH );
    else if ( pAbc->pNtkCur )
    {
        Abc_Ntk_t * pNtkTemp;
        pNtkTemp = Abc_NtkToNetlist( pAbc->pNtkCur );
        Abc_NtkToAig( pNtkTemp );
        Io_WriteBenchLut( pNtkTemp, pFileName );
        Abc_NtkDelete( pNtkTemp );
    }
    else
        printf( "There is no current network.\n" );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_bench [-lh] <file>\n" );
    fprintf( pAbc->Err, "\t         writes the network in BENCH format\n" );
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
int IoCommandWriteBook( Abc_Frame_t * pAbc, int argc, char **argv )
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
    Io_Write( pAbc->pNtkCur, pFileName, IO_FILE_BOOK );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_book [-h] <file> [-options]\n" );
    fprintf( pAbc->Err, "\t-h     : prints the help massage\n" );
    fprintf( pAbc->Err, "\tfile   : the name of the file to write (extension .aux, .nodes, .nets)\n" );
    fprintf( pAbc->Err, "\t\n" );
    fprintf( pAbc->Err, "\tThis command is developed by Myungchul Kim (University of Michigan).\n" );
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
    if ( pAbc->pNtkCur == NULL )
    {
        fprintf( pAbc->Out, "Empty network.\n" );
        return 0;
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    pNtk = pAbc->pNtkCur;
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
    fprintf( pAbc->Err, "\t         writes the network is the cellnet format\n" );
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
    int fNewAlgo;
    int fFastAlgo;
    int fAllPrimes;
    int fChangePol;
    int fVerbose;
    extern Abc_Ntk_t * Abc_NtkDarToCnf( Abc_Ntk_t * pNtk, char * pFileName, int fFastAlgo, int fChangePol, int fVerbose );

    fNewAlgo = 1;
    fFastAlgo = 0;
    fAllPrimes = 0;
    fChangePol = 1;
    fVerbose = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "nfpcvh" ) ) != EOF )
    {
        switch ( c )
        {
            case 'n':
                fNewAlgo ^= 1;
                break;
            case 'f':
                fFastAlgo ^= 1;
                break;
            case 'p':
                fAllPrimes ^= 1;
                break;
            case 'c':
                fChangePol ^= 1;
                break;
            case 'v':
                fVerbose ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( pAbc->pNtkCur == NULL )
    {
        fprintf( pAbc->Out, "Empty network.\n" );
        return 0;
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
    if ( fFastAlgo )
        Abc_NtkDarToCnf( pAbc->pNtkCur, pFileName, 1, fChangePol, fVerbose );
    else if ( fNewAlgo )
        Abc_NtkDarToCnf( pAbc->pNtkCur, pFileName, 0, fChangePol, fVerbose );
    else if ( fAllPrimes )
        Io_WriteCnf( pAbc->pNtkCur, pFileName, 1 );
    else
        Io_Write( pAbc->pNtkCur, pFileName, IO_FILE_CNF );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_cnf [-nfpcvh] <file>\n" );
    fprintf( pAbc->Err, "\t         generates CNF for the miter (see also \"&write_cnf\")\n" );
    fprintf( pAbc->Err, "\t-n     : toggle using new algorithm [default = %s]\n", fNewAlgo? "yes" : "no" );
    fprintf( pAbc->Err, "\t-f     : toggle using fast algorithm [default = %s]\n", fFastAlgo? "yes" : "no" );
    fprintf( pAbc->Err, "\t-p     : toggle using all primes to enhance implicativity [default = %s]\n", fAllPrimes? "yes" : "no" );
    fprintf( pAbc->Err, "\t-c     : toggle adjasting polarity of internal variables [default = %s]\n", fChangePol? "yes" : "no" );
    fprintf( pAbc->Err, "\t-v     : toggle printing verbose information [default = %s]\n", fVerbose? "yes" : "no" );
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
int IoCommandWriteCnf2( Abc_Frame_t * pAbc, int argc, char **argv )
{
    extern void Jf_ManDumpCnf( Gia_Man_t * p, char * pFileName, int fVerbose );
    extern void Mf_ManDumpCnf( Gia_Man_t * p, char * pFileName, int nLutSize, int fCnfObjIds, int fAddOrCla, int fVerbose );
    FILE * pFile;
    char * pFileName;
    int nLutSize    = 8;
    int fNewAlgo    = 1;
    int fCnfObjIds  = 0;
    int fAddOrCla   = 1;
    int c, fVerbose = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "Kaiovh" ) ) != EOF )
    {
        switch ( c )
        {
            case 'K':
                if ( globalUtilOptind >= argc )
                {
                    Abc_Print( -1, "Command line switch \"-K\" should be followed by an integer.\n" );
                    goto usage;
                }
                nLutSize = atoi(argv[globalUtilOptind]);
                globalUtilOptind++;
                break;
            case 'a':
                fNewAlgo ^= 1;
                break;
            case 'i':
                fCnfObjIds ^= 1;
                break;
            case 'o':
                fAddOrCla ^= 1;
                break;
            case 'v':
                fVerbose ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( pAbc->pGia == NULL )
    {
        Abc_Print( -1, "IoCommandWriteCnf2(): There is no AIG.\n" );
        return 1;
    } 
    if ( Gia_ManRegNum(pAbc->pGia) > 0 )
    {
        Abc_Print( -1, "IoCommandWriteCnf2(): Works only for combinational miters.\n" );
        return 0;
    }
    if ( nLutSize < 3 || nLutSize > 8 )
    {
        Abc_Print( -1, "IoCommandWriteCnf2(): Invalid LUT size (%d).\n", nLutSize );
        return 0;
    }
    if ( !fNewAlgo && !Sdm_ManCanRead() )
    {
        Abc_Print( -1, "IoCommandWriteCnf2(): Cannot input precomputed DSD information.\n" );
        return 0;
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the input file name
    pFileName = argv[globalUtilOptind];
    pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" for writing.\n", pFileName );
        return 0;
    }
    fclose( pFile );
    if ( fNewAlgo )
        Mf_ManDumpCnf( pAbc->pGia, pFileName, nLutSize, fCnfObjIds, fAddOrCla, fVerbose );
    else
        Jf_ManDumpCnf( pAbc->pGia, pFileName, fVerbose );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: &write_cnf [-Kaiovh] <file>\n" );
    fprintf( pAbc->Err, "\t           writes CNF produced by a new generator\n" );
    fprintf( pAbc->Err, "\t-K <num> : the LUT size (3 <= num <= 8) [default = %d]\n", nLutSize );
    fprintf( pAbc->Err, "\t-a       : toggle using new algorithm [default = %s]\n", fNewAlgo? "yes" : "no" );
    fprintf( pAbc->Err, "\t-i       : toggle using AIG object IDs as CNF variables [default = %s]\n", fCnfObjIds? "yes" : "no" );
    fprintf( pAbc->Err, "\t-o       : toggle adding OR clause for the outputs [default = %s]\n", fAddOrCla? "yes" : "no" );
    fprintf( pAbc->Err, "\t-v       : toggle printing verbose information [default = %s]\n", fVerbose? "yes" : "no" );
    fprintf( pAbc->Err, "\t-h       : print the help massage\n" );
    fprintf( pAbc->Err, "\tfile     : the name of the file to write\n" );
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
    if ( pAbc->pNtkCur == NULL )
    {
        fprintf( pAbc->Out, "Empty network.\n" );
        return 0;
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
    fprintf( pAbc->Err, "\t         writes the current network into a DOT file\n" );
    fprintf( pAbc->Err, "\t-h     : print the help massage\n" );
    fprintf( pAbc->Err, "\tfile   : the name of the file to write\n" );
    return 1;
}

ABC_NAMESPACE_IMPL_END

#include "proof/fra/fra.h"

ABC_NAMESPACE_IMPL_START


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandWriteCex( Abc_Frame_t * pAbc, int argc, char **argv )
{
    Abc_Ntk_t * pNtk;
    char * pFileName;
    int c, fNames  = 0;
    int fMinimize  = 0;
    int fUseSatBased = 0;
    int fHighEffort = 0;
    int fUseOldMin = 0;
    int fCheckCex  = 0;
    int forceSeq   = 0;
    int fAiger     = 0;
    int fPrintFull = 0;
    int fVerbose   = 0;

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "snmueocafvh" ) ) != EOF )
    {
        switch ( c )
        {
            case 's':
                forceSeq ^= 1;
                break;
            case 'n':
                fNames ^= 1;
                break;
            case 'm':
                fMinimize ^= 1;
                break;
            case 'u':
                fUseSatBased ^= 1;
                break;
            case 'e':
                fHighEffort ^= 1;
                break;
            case 'o':
                fUseOldMin ^= 1;
                break;
            case 'c':
                fCheckCex ^= 1;
                break;
            case 'a':
                fAiger ^= 1;
                break;
            case 'f':
                fPrintFull ^= 1;
                break;
            case 'v':
                fVerbose ^= 1;
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
    if ( pNtk->pModel == NULL && pAbc->pCex == NULL )
    {
        fprintf( pAbc->Out, "Counter-example is not available.\n" );
        return 0;
    }
    if ( argc != globalUtilOptind + 1 )
    {
        printf( "File name is missing on the command line.\n" );
        goto usage;
    }

    // get the input file name
    pFileName = argv[globalUtilOptind];
    // write the counter-example into the file
    if ( pAbc->pCex )
    { 
        Abc_Cex_t * pCex = pAbc->pCex;
        Abc_Obj_t * pObj;
        FILE * pFile;
        int i, f;
        /*
        Abc_NtkForEachLatch( pNtk, pObj, i )
            if ( !Abc_LatchIsInit0(pObj) )
            {
                fprintf( stdout, "IoCommandWriteCex(): The init-state should be all-0 for counter-example to work.\n" );
                fprintf( stdout, "Run commands \"undc\" and \"zero\" and then rerun the equivalence check.\n" );
                return 1;
            }
        */
        pFile = fopen( pFileName, "w" );
        if ( pFile == NULL )
        {
            fprintf( stdout, "IoCommandWriteCex(): Cannot open the output file \"%s\".\n", pFileName );
            return 1;
        }
        if ( fPrintFull )
        {
            extern Aig_Man_t * Abc_NtkToDar( Abc_Ntk_t * pNtk, int fExors, int fRegisters );
            Aig_Man_t * pAig = Abc_NtkToDar( pNtk, 0, 1 );
            Abc_Cex_t * pCexFull = Saig_ManExtendCex( pAig, pCex );
            Aig_ManStop( pAig );
            // output PI values (while skipping the minimized ones)
            assert( pCexFull->nBits == Abc_NtkCiNum(pNtk) * (pCex->iFrame + 1) );
            for ( f = 0; f <= pCex->iFrame; f++ )
                Abc_NtkForEachCi( pNtk, pObj, i )
                    fprintf( pFile, "%s@%d=%c ", Abc_ObjName(pObj), f, '0'+Abc_InfoHasBit(pCexFull->pData, Abc_NtkCiNum(pNtk)*f + i) );
            Abc_CexFreeP( &pCexFull );
        }
        else if ( fNames )
        {
            Abc_Cex_t * pCare = NULL;
            if ( fMinimize )
            {
                extern Aig_Man_t * Abc_NtkToDar( Abc_Ntk_t * pNtk, int fExors, int fRegisters );
                Aig_Man_t * pAig = Abc_NtkToDar( pNtk, 0, 1 );
                if ( fUseOldMin )
                {
                    pCare = Saig_ManCbaFindCexCareBits( pAig, pCex, 0, fVerbose );
                    if ( fCheckCex )
                        Bmc_CexCareVerify( pAig, pCex, pCare, fVerbose );
                }
                else if ( fUseSatBased )
                    pCare = Bmc_CexCareSatBasedMinimize( pAig, Saig_ManPiNum(pAig), pCex, fHighEffort, fCheckCex, fVerbose );
                else
                    pCare = Bmc_CexCareMinimize( pAig, Saig_ManPiNum(pAig), pCex, 4, fCheckCex, fVerbose );
                Aig_ManStop( pAig );
            }
            // output flop values (unaffected by the minimization)
            Abc_NtkForEachLatch( pNtk, pObj, i )
                fprintf( pFile, "%s@0=%c ", Abc_ObjName(Abc_ObjFanout0(pObj)), '0'+!Abc_LatchIsInit0(pObj) );
            // output PI values (while skipping the minimized ones)
            for ( f = 0; f <= pCex->iFrame; f++ )
                Abc_NtkForEachPi( pNtk, pObj, i )
                    if ( !pCare || Abc_InfoHasBit(pCare->pData, pCare->nRegs+pCare->nPis*f + i) )
                        fprintf( pFile, "%s@%d=%c ", Abc_ObjName(pObj), f, '0'+Abc_InfoHasBit(pCex->pData, pCex->nRegs+pCex->nPis*f + i) );
            Abc_CexFreeP( &pCare );
        }
        else
        {
            Abc_NtkForEachLatch( pNtk, pObj, i )
                fprintf( pFile, "%c", '0'+!Abc_LatchIsInit0(pObj) );

            for ( i = pCex->nRegs; i < pCex->nBits; i++ )
            {
                if ( fAiger && (i-pCex->nRegs)%pCex->nPis == 0)
                    fprintf( pFile, "\n");
                fprintf( pFile, "%c", '0'+Abc_InfoHasBit(pCex->pData, i) );
            }
        }
        fprintf( pFile, "\n" );
        fclose( pFile );
    }
    else
    {
        Abc_Obj_t * pObj;
        FILE * pFile = fopen( pFileName, "w" );
        int i;
        if ( pFile == NULL )
        {
            fprintf( stdout, "IoCommandWriteCex(): Cannot open the output file \"%s\".\n", pFileName );
            return 1;
        }
        if ( fNames )
        {
            const char *cycle_ctr = forceSeq?"@0":"";
            Abc_NtkForEachPi( pNtk, pObj, i )
//                fprintf( pFile, "%s=%c\n", Abc_ObjName(pObj), '0'+(pNtk->pModel[i]==1) );
                fprintf( pFile, "%s%s=%c\n", Abc_ObjName(pObj), cycle_ctr, '0'+(pNtk->pModel[i]==1) );
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
    fprintf( pAbc->Err, "usage: write_cex [-snmueocfvh] <file>\n" );
    fprintf( pAbc->Err, "\t         saves counter-example (CEX) derived by \"sat\", \"iprove\", \"dprove\", etc\n" );
    fprintf( pAbc->Err, "\t         the output file <file> contains values for each PI in natural order\n" );
    fprintf( pAbc->Err, "\t-s     : always report a sequential CEX (cycle 0 for comb) [default = %s]\n", forceSeq? "yes": "no" );
    fprintf( pAbc->Err, "\t-n     : write input names into the file [default = %s]\n", fNames? "yes": "no" );
    fprintf( pAbc->Err, "\t-m     : minimize CEX by dropping don't-care values [default = %s]\n", fMinimize? "yes": "no" );
    fprintf( pAbc->Err, "\t-u     : use fast SAT-based CEX minimization [default = %s]\n", fUseSatBased? "yes": "no" );
    fprintf( pAbc->Err, "\t-e     : use high-effort SAT-based CEX minimization [default = %s]\n", fHighEffort? "yes": "no" );
    fprintf( pAbc->Err, "\t-o     : use old CEX minimization algorithm [default = %s]\n", fUseOldMin? "yes": "no" );
    fprintf( pAbc->Err, "\t-c     : check generated CEX using ternary simulation [default = %s]\n", fCheckCex? "yes": "no" );
    fprintf( pAbc->Err, "\t-a     : print cex in AIGER 1.9 format [default = %s].\n", fAiger? "yes": "no" );
    fprintf( pAbc->Err, "\t-f     : enable printing flop values in each timeframe [default = %s].\n", fPrintFull? "yes": "no" );  
    fprintf( pAbc->Err, "\t-v     : enable verbose output [default = %s].\n", fVerbose? "yes": "no" );  
    fprintf( pAbc->Err, "\t-h     : print the help massage\n" );
    fprintf( pAbc->Err, "\t<file> : the name of the file to write\n" );
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
    if ( pAbc->pNtkCur == NULL )
    {
        fprintf( pAbc->Out, "Empty network.\n" );
        return 0;
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
    fprintf( pAbc->Err, "\t         writes the current network in the equation format\n" );
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
    if ( pAbc->pNtkCur == NULL )
    {
        fprintf( pAbc->Out, "Empty network.\n" );
        return 0;
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
    fprintf( pAbc->Err, "\t         writes network using graph representation formal GML\n" );
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
    if ( pAbc->pNtkCur == NULL )
    {
        fprintf( pAbc->Out, "Empty network.\n" );
        return 0;
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
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
    fprintf( pAbc->Err, "\t         writes network using graph representation formal GML\n" );
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
    int c, fUseMoPla = 0;

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "mh" ) ) != EOF )
    {
        switch ( c )
        {
            case 'm':
                fUseMoPla ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( pAbc->pNtkCur == NULL )
    {
        fprintf( pAbc->Out, "Empty network.\n" );
        return 0;
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the output file name
    pFileName = argv[globalUtilOptind];
    // call the corresponding file writer
    Io_Write( pAbc->pNtkCur, pFileName, fUseMoPla ? IO_FILE_MOPLA : IO_FILE_PLA );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_pla [-mh] <file>\n" );
    fprintf( pAbc->Err, "\t         writes the collapsed network into a PLA file\n" );
    fprintf( pAbc->Err, "\t-m     : toggle writing multi-output PLA [default = %s]\n", fUseMoPla? "yes":"no" );
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
    int c, fOnlyAnds = 0;

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "ah" ) ) != EOF )
    {
        switch ( c )
        {
            case 'a':
                fOnlyAnds ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( pAbc->pNtkCur == NULL )
    {
        fprintf( pAbc->Out, "Empty network.\n" );
        return 0;
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the output file name
    pFileName = argv[globalUtilOptind];
    // call the corresponding file writer
    if ( fOnlyAnds )
    {
        Abc_Ntk_t * pNtkTemp = Abc_NtkToNetlist( pAbc->pNtkCur );
        if ( !Abc_NtkHasAig(pNtkTemp) && !Abc_NtkHasMapping(pNtkTemp) )
            Abc_NtkToAig( pNtkTemp );
        Io_WriteVerilog( pNtkTemp, pFileName, 1 );
        Abc_NtkDelete( pNtkTemp );
    }
    else
    Io_Write( pAbc->pNtkCur, pFileName, IO_FILE_VERILOG );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_verilog [-ah] <file>\n" );
    fprintf( pAbc->Err, "\t         writes the current network in Verilog format\n" );
    fprintf( pAbc->Err, "\t-a     : toggle writing expressions with only ANDs (without XORs and MUXes) [default = %s]\n", fOnlyAnds? "yes":"no" );
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
int IoCommandWriteSortCnf( Abc_Frame_t * pAbc, int argc, char **argv )
{
    char * pFileName;
    int c;
    int nVars = 16;
    int nQueens = 4;
    extern void Abc_NtkWriteSorterCnf( char * pFileName, int nVars, int nQueens );

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "NQh" ) ) != EOF )
    {
        switch ( c )
        {
            case 'N':
                if ( globalUtilOptind >= argc )
                {
                    fprintf( stdout, "Command line switch \"-N\" should be followed by an integer.\n" );
                    goto usage;
                }
                nVars = atoi(argv[globalUtilOptind]);
                globalUtilOptind++;
                if ( nVars <= 0 ) 
                    goto usage;
                break;
            case 'Q':
                if ( globalUtilOptind >= argc )
                {
                    fprintf( stdout, "Command line switch \"-Q\" should be followed by an integer.\n" );
                    goto usage;
                }
                nQueens = atoi(argv[globalUtilOptind]);
                globalUtilOptind++;
                if ( nQueens <= 0 ) 
                    goto usage;
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
    Abc_NtkWriteSorterCnf( pFileName, nVars, nQueens );
    // call the corresponding file writer
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_sorter_cnf [-N <num>] [-Q <num>] <file>\n" );
    fprintf( pAbc->Err, "\t         writes CNF for the sorter\n" );
    fprintf( pAbc->Err, "\t-N num : the number of sorter bits [default = %d]\n", nVars );
    fprintf( pAbc->Err, "\t-Q num : the number of bits to be asserted to 1 [default = %d]\n", nQueens );
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
int IoCommandWriteTruth( Abc_Frame_t * pAbc, int argc, char **argv )
{
    Vec_Int_t * vTruth;
    Abc_Ntk_t * pNtk = pAbc->pNtkCur;
    Abc_Obj_t * pNode;
    char * pFileName;
    FILE * pFile;
    unsigned * pTruth;
    int fReverse = 0;
    int c;

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "rh" ) ) != EOF )
    {
        switch ( c )
        {
            case 'r':
                fReverse ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( pAbc->pNtkCur == NULL )
    {
        printf( "Current network is not available.\n" );
        return 0;
    }
    if ( !Abc_NtkIsLogic(pNtk) )
    {
        printf( "Current network should not an AIG. Run \"logic\".\n" );
        return 0;
    }
    if ( Abc_NtkPoNum(pNtk) != 1 )
    {
        printf( "Current network should have exactly one primary output.\n" );
        return 0;
    }
    if ( Abc_NtkNodeNum(pNtk) != 1 )
    {
        printf( "Current network should have exactly one node.\n" );
        return 0;
    }
    pNode = Abc_ObjFanin0( Abc_NtkPo(pNtk, 0) );
    if ( Abc_ObjFaninNum(pNode) == 0 )
    { 
        printf( "Can only write logic function with 0 inputs.\n" );
        return 0;
    }
    if ( Abc_ObjFaninNum(pNode) > 16 )
    { 
        printf( "Can only write logic function with no more than 16 inputs.\n" );
        return 0;
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the input file name
    pFileName = argv[globalUtilOptind];
    // convert to logic
    Abc_NtkToAig( pNtk );
    vTruth = Vec_IntAlloc( 0 );
    pTruth = Hop_ManConvertAigToTruth( (Hop_Man_t *)pNtk->pManFunc, (Hop_Obj_t *)pNode->pData, Abc_ObjFaninNum(pNode), vTruth, fReverse );
    pFile = fopen( pFileName, "w" );
    if ( pFile == NULL )
    {
        Vec_IntFree( vTruth );
        printf( "Cannot open file \"%s\" for writing.\n", pFileName );
        return 0;
    }
    Extra_PrintBinary( pFile, pTruth, 1<<Abc_ObjFaninNum(pNode) );
    fclose( pFile );
    Vec_IntFree( vTruth );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_truth [-rh] <file>\n" );
    fprintf( pAbc->Err, "\t         writes truth table into a file\n" );
    fprintf( pAbc->Err, "\t-r     : toggle reversing bits in the truth table [default = %s]\n", fReverse? "yes":"no" );
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
int IoCommandWriteTruths( Abc_Frame_t * pAbc, int argc, char **argv )
{
    Gia_Obj_t * pObj;
    char * pFileName;
    FILE * pFile;
    word * pTruth;
    int nBytes;
    int fReverse = 0;
    int fBinary = 0;
    int c, i;
 
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "rbh" ) ) != EOF )
    {
        switch ( c )
        {
            case 'r':
                fReverse ^= 1;
                break;
            case 'b':
                fBinary ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }
    if ( pAbc->pGia == NULL )
    {
        Abc_Print( -1, "IoCommandWriteTruths(): There is no AIG.\n" );
        return 1;
    } 
    if ( Gia_ManPiNum(pAbc->pGia) > 16 )
    {
        Abc_Print( -1, "IoCommandWriteTruths(): Can write truth tables up to 16 inputs.\n" );
        return 0;
    }
    if ( Gia_ManPiNum(pAbc->pGia) < 3 )
    {
        Abc_Print( -1, "IoCommandWriteTruths(): Can write truth tables for 3 inputs or more.\n" );
        return 0;
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the input file name
    pFileName = argv[globalUtilOptind];
    // convert to logic
    pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" for writing.\n", pFileName );
        return 0;
    }
    nBytes = 8 * Abc_Truth6WordNum( Gia_ManPiNum(pAbc->pGia) );
    Gia_ManForEachCo( pAbc->pGia, pObj, i )
    {
        pTruth = Gia_ObjComputeTruthTable( pAbc->pGia, pObj );
        if ( fBinary )
            fwrite( pTruth, nBytes, 1, pFile );
        else
            Extra_PrintHex( pFile, (unsigned *)pTruth, Gia_ManPiNum(pAbc->pGia) ), fprintf( pFile, "\n" );
    }
    fclose( pFile );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: &write_truths [-rbh] <file>\n" );
    fprintf( pAbc->Err, "\t         writes truth tables of each PO of GIA manager into a file\n" );
    fprintf( pAbc->Err, "\t-r     : toggle reversing bits in the truth table [default = %s]\n", fReverse? "yes":"no" );
    fprintf( pAbc->Err, "\t-b     : toggle using binary format [default = %s]\n", fBinary? "yes":"no" );
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
int IoCommandWriteStatus( Abc_Frame_t * pAbc, int argc, char **argv )
{
    char * pFileName;
    int c;
    extern void Abc_NtkWriteLogFile( char * pFileName, Abc_Cex_t * pCex, int Status, int nFrames, char * pCommand );

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
    Abc_NtkWriteLogFile( pFileName, pAbc->pCex, pAbc->Status, pAbc->nFrames, NULL );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_status [-h] <file>\n" );
    fprintf( pAbc->Err, "\t         writes verification log file\n" );
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
int IoCommandWriteSmv( Abc_Frame_t * pAbc, int argc, char **argv )
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
    if ( pAbc->pNtkCur == NULL )
    {
        fprintf( pAbc->Out, "Empty network.\n" );
        return 0;
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the output file name
    pFileName = argv[globalUtilOptind];
    // call the corresponding file writer
    Io_Write( pAbc->pNtkCur, pFileName, IO_FILE_SMV );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_smv [-h] <file>\n" );
    fprintf( pAbc->Err, "\t         write the network in SMV format\n" );
    fprintf( pAbc->Err, "\t-h     : print the help message\n" );
    fprintf( pAbc->Err, "\tfile   : the name of the file to write (extension .smv)\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandWriteJson( Abc_Frame_t * pAbc, int argc, char **argv )
{
    extern void Json_Write( char * pFileName, Abc_Nam_t * pStr, Vec_Wec_t * vObjs );
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
    if ( Abc_FrameReadJsonStrs(Abc_FrameReadGlobalFrame()) == NULL )
    {
        fprintf( pAbc->Out, "No JSON info is available.\n" );
        return 0;
    }
    if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the output file name
    pFileName = argv[globalUtilOptind];
    // call the corresponding file writer
    Json_Write( pFileName, Abc_FrameReadJsonStrs(Abc_FrameReadGlobalFrame()), Abc_FrameReadJsonObjs(Abc_FrameReadGlobalFrame()) );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_json [-h] <file>\n" );
    fprintf( pAbc->Err, "\t         write the network in JSON format\n" );
    fprintf( pAbc->Err, "\t-h     : print the help message\n" );
    fprintf( pAbc->Err, "\tfile   : the name of the file to write (extension .json)\n" );
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

