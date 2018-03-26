/**CFile****************************************************************

  FileName    [scl.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Standard-cell library representation.]

  Synopsis    [Relevant command handlers.]

  Author      [Alan Mishchenko, Niklas Een]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 24, 2012.]

  Revision    [$Id: scl.c,v 1.0 2012/08/24 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sclSize.h"
#include "base/main/mainInt.h"

#include "misc/util/utilNam.h"
#include "sclCon.h"

#include "map/mio/mio.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Scl_CommandReadLib    ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int Scl_CommandWriteLib   ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int Scl_CommandReadScl    ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int Scl_CommandWriteScl   ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int Scl_CommandPrintLib   ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int Scl_CommandLeak2Area  ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int Scl_CommandDumpGen    ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int Scl_CommandPrintGS    ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int Scl_CommandStime      ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int Scl_CommandTopo       ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int Scl_CommandUnBuffer   ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int Scl_CommandBuffer     ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int Scl_CommandBufferOld  ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int Scl_CommandMinsize    ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int Scl_CommandMaxsize    ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int Scl_CommandUpsize     ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int Scl_CommandDnsize     ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int Scl_CommandPrintBuf   ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int Scl_CommandReadConstr ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int Scl_CommandWriteConstr( Abc_Frame_t * pAbc, int argc, char ** argv );
static int Scl_CommandPrintConstr( Abc_Frame_t * pAbc, int argc, char ** argv );
static int Scl_CommandResetConstr( Abc_Frame_t * pAbc, int argc, char ** argv );

static inline Scl_Con_t * Scl_ConGetMan( Abc_Frame_t * pAbc )                    { return (Scl_Con_t *)pAbc->pAbcCon;                       }
static inline void        Scl_ConFreeMan( Abc_Frame_t * pAbc )                   { if ( pAbc->pAbcCon ) Scl_ConFree(Scl_ConGetMan(pAbc));   }
static inline void        Scl_ConUpdateMan( Abc_Frame_t * pAbc, Scl_Con_t * p )  { Scl_ConFreeMan(pAbc); pAbc->pAbcCon = p;                 }
              Scl_Con_t * Scl_ConReadMan()                                       { return Scl_ConGetMan( Abc_FrameGetGlobalFrame() );       }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Updating library in the frameframe.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_SclLoad( SC_Lib * pLib, SC_Lib ** ppScl )
{
    if ( *ppScl )
    {
        Abc_SclLibFree( *ppScl );
        *ppScl = NULL;
    }
    assert( *ppScl == NULL );
    if ( pLib )
        *(SC_Lib **)ppScl = pLib;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Scl_Init( Abc_Frame_t * pAbc )
{
    Cmd_CommandAdd( pAbc, "SCL mapping",  "read_lib",      Scl_CommandReadLib,     0 ); 
    Cmd_CommandAdd( pAbc, "SCL mapping",  "write_lib",     Scl_CommandWriteLib,    0 ); 
    Cmd_CommandAdd( pAbc, "SCL mapping",  "print_lib",     Scl_CommandPrintLib,    0 ); 
    Cmd_CommandAdd( pAbc, "SCL mapping",  "leak2area",     Scl_CommandLeak2Area,   0 ); 
    Cmd_CommandAdd( pAbc, "SCL mapping",  "read_scl",      Scl_CommandReadScl,     0 ); 
    Cmd_CommandAdd( pAbc, "SCL mapping",  "write_scl",     Scl_CommandWriteScl,    0 ); 
    Cmd_CommandAdd( pAbc, "SCL mapping",  "dump_genlib",   Scl_CommandDumpGen,     0 ); 
    Cmd_CommandAdd( pAbc, "SCL mapping",  "print_gs",      Scl_CommandPrintGS,     0 ); 
    Cmd_CommandAdd( pAbc, "SCL mapping",  "stime",         Scl_CommandStime,       0 ); 
    Cmd_CommandAdd( pAbc, "SCL mapping",  "topo",          Scl_CommandTopo,        1 ); 
    Cmd_CommandAdd( pAbc, "SCL mapping",  "unbuffer",      Scl_CommandUnBuffer,    1 ); 
    Cmd_CommandAdd( pAbc, "SCL mapping",  "buffer",        Scl_CommandBuffer,      1 ); 
//    Cmd_CommandAdd( pAbc, "SCL mapping",  "_buffer",       Scl_CommandBufferOld,   1 ); 
    Cmd_CommandAdd( pAbc, "SCL mapping",  "minsize",       Scl_CommandMinsize,     1 ); 
    Cmd_CommandAdd( pAbc, "SCL mapping",  "maxsize",       Scl_CommandMaxsize,     1 ); 
    Cmd_CommandAdd( pAbc, "SCL mapping",  "upsize",        Scl_CommandUpsize,      1 ); 
    Cmd_CommandAdd( pAbc, "SCL mapping",  "dnsize",        Scl_CommandDnsize,      1 ); 
    Cmd_CommandAdd( pAbc, "SCL mapping",  "print_buf",     Scl_CommandPrintBuf,    0 ); 
    Cmd_CommandAdd( pAbc, "SCL mapping",  "read_constr",   Scl_CommandReadConstr,  0 ); 
    Cmd_CommandAdd( pAbc, "SCL mapping",  "write_constr",  Scl_CommandWriteConstr, 0 ); 
    Cmd_CommandAdd( pAbc, "SCL mapping",  "print_constr",  Scl_CommandPrintConstr, 0 ); 
    Cmd_CommandAdd( pAbc, "SCL mapping",  "reset_constr",  Scl_CommandResetConstr, 0 ); 
}
void Scl_End( Abc_Frame_t * pAbc )
{
    Abc_SclLoad( NULL, (SC_Lib **)&pAbc->pLibScl );
    Scl_ConUpdateMan( pAbc, NULL );
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_CommandReadLib( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    char * pFileName;
    FILE * pFile;
    SC_Lib * pLib;
    int c, fDump = 0;
    float Slew = 0;
    float Gain = 0;
    int nGatesMin = 0;
    int fShortNames = 0;
    int fVerbose = 1;
    int fVeryVerbose = 0;

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "SGMdnvwh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'S':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-S\" should be followed by a floating point number.\n" );
                goto usage;
            }
            Slew = (float)atof(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( Slew <= 0.0 )
                goto usage;
            break;
        case 'G':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-G\" should be followed by a floating point number.\n" );
                goto usage;
            }
            Gain = (float)atof(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( Gain <= 0.0 )
                goto usage;
            break;
        case 'M':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-M\" should be followed by a positive integer.\n" );
                goto usage;
            }
            nGatesMin = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( nGatesMin < 0 ) 
                goto usage;
            break;
        case 'd':
            fDump ^= 1;
            break;
        case 'n':
            fShortNames ^= 1;
            break;
        case 'v':
            fVerbose ^= 1;
            break;
        case 'w':
            fVeryVerbose ^= 1;
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
    if ( (pFile = fopen( pFileName, "rb" )) == NULL )
    {
        fprintf( pAbc->Err, "Cannot open input file \"%s\". \n", pFileName );
        return 1;
    }
    fclose( pFile );
    // read new library
    pLib = Abc_SclReadLiberty( pFileName, fVerbose, fVeryVerbose );
    if ( pLib == NULL )
    {
        fprintf( pAbc->Err, "Reading SCL library from file \"%s\" has failed. \n", pFileName );
        return 1;
    }
    if ( Abc_SclLibClassNum(pLib) < 3 )
    {
        fprintf( pAbc->Err, "Library with only %d cell classes cannot be used.\n", Abc_SclLibClassNum(pLib) );
        Abc_SclLibFree(pLib);
        return 0;
    }
    Abc_SclLoad( pLib, (SC_Lib **)&pAbc->pLibScl );
    // convert the library if needed
    if ( fShortNames )
        Abc_SclShortNames( pLib );
    // dump the resulting library
    if ( fDump && pAbc->pLibScl )
        Abc_SclWriteLiberty( Extra_FileNameGenericAppend(pFileName, "_temp.lib"), (SC_Lib *)pAbc->pLibScl );
    // extract genlib library
    if ( pAbc->pLibScl )
    {
        Abc_SclInstallGenlib( pAbc->pLibScl, Slew, Gain, nGatesMin );
        Mio_LibraryTransferCellIds();
    }
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_lib [-SG float] [-M num] [-dnvwh] <file>\n" );
    fprintf( pAbc->Err, "\t           reads Liberty library from file\n" );
    fprintf( pAbc->Err, "\t-S float : the slew parameter used to generate the library [default = %.2f]\n", Slew );
    fprintf( pAbc->Err, "\t-G float : the gain parameter used to generate the library [default = %.2f]\n", Gain );
    fprintf( pAbc->Err, "\t-M num   : skip gate classes whose size is less than this [default = %d]\n", nGatesMin );
    fprintf( pAbc->Err, "\t-d       : toggle dumping the parsed library into file \"*_temp.lib\" [default = %s]\n", fDump? "yes": "no" );
    fprintf( pAbc->Err, "\t-n       : toggle replacing gate/pin names by short strings [default = %s]\n", fShortNames? "yes": "no" );
    fprintf( pAbc->Err, "\t-v       : toggle writing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    fprintf( pAbc->Err, "\t-w       : toggle writing information about skipped gates [default = %s]\n", fVeryVerbose? "yes": "no" );
    fprintf( pAbc->Err, "\t-h       : prints the command summary\n" );
    fprintf( pAbc->Err, "\t<file>   : the name of a file to read\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_CommandWriteLib( Abc_Frame_t * pAbc, int argc, char **argv )
{
    FILE * pFile;
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
    if ( pAbc->pLibScl == NULL )
    {
        fprintf( pAbc->Err, "There is no Liberty library available.\n" );
        return 1;
    }
    // get the input file name
    pFileName = argv[globalUtilOptind];
    if ( (pFile = fopen( pFileName, "wb" )) == NULL )
    {
        fprintf( pAbc->Err, "Cannot open output file \"%s\". \n", pFileName );
        return 1;
    }
    fclose( pFile );

    // save current library
    Abc_SclWriteLiberty( pFileName, (SC_Lib *)pAbc->pLibScl );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_lib [-h] <file>\n" );
    fprintf( pAbc->Err, "\t         write current Liberty library into file\n" );
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
int Scl_CommandPrintLib( Abc_Frame_t * pAbc, int argc, char **argv )
{
    float Slew = 0; // use library
    float Gain = 100;
    int fInvOnly = 0;
    int fShort = 0;
    int c;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "SGish" ) ) != EOF )
    {
        switch ( c )
        {
        case 'S':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-S\" should be followed by a floating point number.\n" );
                goto usage;
            }
            Slew = (float)atof(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( Slew <= 0.0 )
                goto usage;
            break;
        case 'G':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-G\" should be followed by a floating point number.\n" );
                goto usage;
            }
            Gain = (float)atof(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( Gain <= 0.0 )
                goto usage;
            break;
        case 'i':
            fInvOnly ^= 1;
            break;
        case 's':
            fShort ^= 1;
            break;
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }
    if ( pAbc->pLibScl == NULL )
    {
        fprintf( pAbc->Err, "There is no Liberty library available.\n" );
        return 1;
    }

    // save current library
    Abc_SclPrintCells( (SC_Lib *)pAbc->pLibScl, Slew, Gain, fInvOnly, fShort );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: print_lib [-SG float] [-ish]\n" );
    fprintf( pAbc->Err, "\t           prints statistics of Liberty library\n" );
    fprintf( pAbc->Err, "\t-S float : the slew parameter used to generate the library [default = %.2f]\n", Slew );
    fprintf( pAbc->Err, "\t-G float : the gain parameter used to generate the library [default = %.2f]\n", Gain );
    fprintf( pAbc->Err, "\t-i       : toggle printing invs/bufs only [default = %s]\n", fInvOnly? "yes": "no" );
    fprintf( pAbc->Err, "\t-s       : toggle printing in short format [default = %s]\n", fShort? "yes": "no" );
    fprintf( pAbc->Err, "\t-h       : print the help massage\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_CommandLeak2Area( Abc_Frame_t * pAbc, int argc, char **argv )
{
    float A = 1, B = 1;
    int c, fVerbose = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "ABvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'A':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-A\" should be followed by a floating point number.\n" );
                goto usage;
            }
            A = (float)atof(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( A <= 0.0 )
                goto usage;
            break;
        case 'B':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-B\" should be followed by a floating point number.\n" );
                goto usage;
            }
            B = (float)atof(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( B <= 0.0 )
                goto usage;
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
    if ( pAbc->pLibScl == NULL )
    {
        fprintf( pAbc->Err, "There is no Liberty library available.\n" );
        return 1;
    }
    // update the current library
    Abc_SclConvertLeakageIntoArea( (SC_Lib *)pAbc->pLibScl, A, B );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: leak2area [-AB float] [-v]\n" );
    fprintf( pAbc->Err, "\t           converts leakage into area: Area = A * Area + B * Leakage\n" );
    fprintf( pAbc->Err, "\t-A float : the multiplicative coefficient to transform area [default = %.2f]\n", A );
    fprintf( pAbc->Err, "\t-B float : the multiplicative coefficient to transform leakage [default = %.2f]\n", B );
    fprintf( pAbc->Err, "\t-v       : toggle printing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    fprintf( pAbc->Err, "\t-h       : print the help massage\n" );
    return 1;
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_CommandReadScl( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    FILE * pFile;
    SC_Lib * pLib;
    char * pFileName;
    int c, fDump = 0;

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "dh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'd':
            fDump ^= 1;
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
    if ( (pFile = fopen( pFileName, "rb" )) == NULL )
    {
        fprintf( pAbc->Err, "Cannot open input file \"%s\". \n", pFileName );
        return 1;
    }
    fclose( pFile );

    // read new library
    pLib = Abc_SclReadFromFile( pFileName );
    if ( pLib == NULL )
    {
        fprintf( pAbc->Err, "Reading SCL library from file \"%s\" has failed. \n", pFileName );
        return 1;
    }
    Abc_SclLoad( pLib, (SC_Lib **)&pAbc->pLibScl );
    if ( fDump )
        Abc_SclWriteLiberty( Extra_FileNameGenericAppend(pFileName, "_temp.lib"), (SC_Lib *)pAbc->pLibScl );
    // extract genlib library
    if ( pAbc->pLibScl )
    {
        Abc_SclInstallGenlib( pAbc->pLibScl, 0, 0, 0 );
        Mio_LibraryTransferCellIds();
    }
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_scl [-dh] <file>\n" );
    fprintf( pAbc->Err, "\t         reads extracted Liberty library from file\n" );
    fprintf( pAbc->Err, "\t-d     : toggle dumping the parsed library into file \"*_temp.lib\" [default = %s]\n", fDump? "yes": "no" );
    fprintf( pAbc->Err, "\t-h     : prints the command summary\n" );
    fprintf( pAbc->Err, "\t<file> : the name of a file to read\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_CommandWriteScl( Abc_Frame_t * pAbc, int argc, char **argv )
{
    FILE * pFile;
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
    if ( pAbc->pLibScl == NULL )
    {
        fprintf( pAbc->Err, "There is no Liberty library available.\n" );
        return 1;
    }
    // get the input file name
    pFileName = argv[globalUtilOptind];
    if ( (pFile = fopen( pFileName, "wb" )) == NULL )
    {
        fprintf( pAbc->Err, "Cannot open output file \"%s\". \n", pFileName );
        return 1;
    }
    fclose( pFile );

    // save current library
    Abc_SclWriteScl( pFileName, (SC_Lib *)pAbc->pLibScl );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_scl [-h] <file>\n" );
    fprintf( pAbc->Err, "\t         write extracted Liberty library into file\n" );
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
int Scl_CommandDumpGen( Abc_Frame_t * pAbc, int argc, char **argv )
{
    char * pFileName = NULL;
    float Slew = 0; // use the library
    float Gain = 200;
    int nGatesMin = 4;
    int c, fVerbose = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "SGMvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'S':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-S\" should be followed by a floating point number.\n" );
                goto usage;
            }
            Slew = (float)atof(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( Slew <= 0.0 )
                goto usage;
            break;
        case 'G':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-G\" should be followed by a floating point number.\n" );
                goto usage;
            }
            Gain = (float)atof(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( Gain <= 0.0 )
                goto usage;
            break;
        case 'M':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-M\" should be followed by a positive integer.\n" );
                goto usage;
            }
            nGatesMin = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( nGatesMin < 0 ) 
                goto usage;
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
    if ( pAbc->pLibScl == NULL )
    {
        fprintf( pAbc->Err, "There is no Liberty library available.\n" );
        goto usage;
    }
    if ( argc == globalUtilOptind + 1 )
        pFileName = argv[globalUtilOptind];
    Abc_SclDumpGenlib( pFileName, (SC_Lib *)pAbc->pLibScl, Slew, Gain, nGatesMin );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: dump_genlib [-SG float] [-M num] [-vh] <file>\n" );
    fprintf( pAbc->Err, "\t           writes GENLIB file for SCL library\n" );
    fprintf( pAbc->Err, "\t-S float : the slew parameter used to generate the library [default = %.2f]\n", Slew );
    fprintf( pAbc->Err, "\t-G float : the gain parameter used to generate the library [default = %.2f]\n", Gain );
    fprintf( pAbc->Err, "\t-M num   : skip gate classes whose size is less than this [default = %d]\n", nGatesMin );
    fprintf( pAbc->Err, "\t-v       : toggle printing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    fprintf( pAbc->Err, "\t-h       : print the command usage\n");
    fprintf( pAbc->Err, "\t<file>   : optional GENLIB file name\n");
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_CommandPrintGS( Abc_Frame_t * pAbc, int argc, char **argv )
{
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
    if ( Abc_FrameReadNtk(pAbc) == NULL )
    {
        fprintf( pAbc->Err, "There is no current network.\n" );
        return 1;
    }
    if ( !Abc_NtkHasMapping(Abc_FrameReadNtk(pAbc)) )
    {
        fprintf( pAbc->Err, "The current network is not mapped.\n" );
        return 1;
    }
    if ( pAbc->pLibScl == NULL )
    {
        fprintf( pAbc->Err, "There is no Liberty library available.\n" );
        return 1;
    }

    // save current library
    Abc_SclPrintGateSizes( (SC_Lib *)pAbc->pLibScl, Abc_FrameReadNtk(pAbc) );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: print_gs [-h]\n" );
    fprintf( pAbc->Err, "\t         prints gate sizes in the current mapping\n" );
    fprintf( pAbc->Err, "\t-h     : print the help massage\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_CommandStime( Abc_Frame_t * pAbc, int argc, char **argv )
{
    int c;
    int fShowAll      = 0;
    int fUseWireLoads = 0;
    int fPrintPath    = 0;
    int fDumpStats    = 0;
    int nTreeCRatio   = 0;

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "Xcapdh" ) ) != EOF )
    {
        switch ( c )
        {
            case 'X':
                if ( globalUtilOptind >= argc )
                {
                    Abc_Print( -1, "Command line switch \"-X\" should be followed by a positive integer.\n" );
                    goto usage;
                }
                nTreeCRatio = atoi(argv[globalUtilOptind]);
                globalUtilOptind++;
                if ( nTreeCRatio < 0 ) 
                    goto usage;
                break;
            case 'c':
                fUseWireLoads ^= 1;
                break;
            case 'a':
                fShowAll ^= 1;
                break;
            case 'p':
                fPrintPath ^= 1;
                break;
            case 'd':
                fDumpStats ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }

    if ( Abc_FrameReadNtk(pAbc) == NULL )
    {
        fprintf( pAbc->Err, "There is no current network.\n" );
        return 1;
    }
    if ( !Abc_NtkHasMapping(Abc_FrameReadNtk(pAbc)) )
    {
        fprintf( pAbc->Err, "The current network is not mapped.\n" );
        return 1;
    }
    if ( !Abc_SclCheckNtk(Abc_FrameReadNtk(pAbc), 0) )
    {
        fprintf( pAbc->Err, "The current network is not in a topo order (run \"topo\").\n" );
        return 1;
    }
    if ( pAbc->pLibScl == NULL )
    {
        fprintf( pAbc->Err, "There is no Liberty library available.\n" );
        return 1;
    }

    Abc_SclTimePerform( (SC_Lib *)pAbc->pLibScl, Abc_FrameReadNtk(pAbc), nTreeCRatio, fUseWireLoads, fShowAll, fPrintPath, fDumpStats );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: stime [-X num] [-capdth]\n" );
    fprintf( pAbc->Err, "\t         performs STA using Liberty library\n" );
    fprintf( pAbc->Err, "\t-X     : min Cout/Cave ratio for tree estimations [default = %d]\n", nTreeCRatio );
    fprintf( pAbc->Err, "\t-c     : toggle using wire-loads if specified [default = %s]\n", fUseWireLoads? "yes": "no" );
    fprintf( pAbc->Err, "\t-a     : display timing information for all nodes [default = %s]\n", fShowAll? "yes": "no" );
    fprintf( pAbc->Err, "\t-p     : display timing information for critical path [default = %s]\n", fPrintPath? "yes": "no" );
    fprintf( pAbc->Err, "\t-d     : toggle dumping statistics into a file [default = %s]\n", fDumpStats? "yes": "no" );
    fprintf( pAbc->Err, "\t-h     : print the help massage\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_CommandTopo( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
    Abc_Ntk_t * pNtkRes;
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

    if ( pNtk == NULL )
    {
        Abc_Print( -1, "Empty network.\n" );
        return 1;
    }
    if ( !Abc_NtkIsLogic(pNtk) )
    {
        Abc_Print( -1, "This command can only be applied to a logic network.\n" );
        return 1;
    }

    // modify the current network
    pNtkRes = Abc_NtkDupDfs( pNtk );
    if ( pNtkRes == NULL )
    {
        Abc_Print( -1, "The command has failed.\n" );
        return 1;
    }
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtkRes );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: topo [-vh]\n" );
    fprintf( pAbc->Err, "\t           rearranges nodes to be in a topological order\n" );
    fprintf( pAbc->Err, "\t-v       : toggle printing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    fprintf( pAbc->Err, "\t-h       : print the command usage\n");
    return 1;
} 

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_CommandUnBuffer( Abc_Frame_t * pAbc, int argc, char **argv )
{
    Abc_Ntk_t * pNtkRes, * pNtk = Abc_FrameReadNtk(pAbc);
    int c, fRemInv = 0, fVerbose = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "ivh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'i':
            fRemInv ^= 1;
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

    if ( pNtk == NULL )
    {
        fprintf( pAbc->Err, "There is no current network.\n" );
        return 1;
    }
    if ( !Abc_NtkIsLogic(pNtk) )
    {
        fprintf( pAbc->Err, "The current network is not a logic network.\n" );
        return 1;
    }
    if ( fRemInv )
        pNtkRes = Abc_SclUnBufferPhase( pNtk, fVerbose );
    else
        pNtkRes = Abc_SclUnBufferPerform( pNtk, fVerbose );
    if ( pNtkRes == NULL )
    {
        Abc_Print( -1, "The command has failed.\n" );
        return 1;
    }
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtkRes );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: unbuffer [-ivh]\n" );
    fprintf( pAbc->Err, "\t           collapses buffer/inverter trees\n" );
    fprintf( pAbc->Err, "\t-i       : toggle removing interters [default = %s]\n", fRemInv? "yes": "no" );
    fprintf( pAbc->Err, "\t-v       : toggle printing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    fprintf( pAbc->Err, "\t-h       : print the command usage\n");
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_CommandBuffer( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    SC_BusPars Pars, * pPars = &Pars;
    Abc_Ntk_t * pNtkRes, * pNtk = Abc_FrameReadNtk(pAbc);
    int c;
    memset( pPars, 0, sizeof(SC_BusPars) );
    pPars->GainRatio     =  300;
    pPars->Slew          = pAbc->pLibScl ? Abc_SclComputeAverageSlew((SC_Lib *)pAbc->pLibScl) : 100;
    pPars->nDegree       =   10;
    pPars->fSizeOnly     =    0;
    pPars->fAddBufs      =    1;
    pPars->fBufPis       =    0;
    pPars->fUseWireLoads =    0;
    pPars->fVerbose      =    0;
    pPars->fVeryVerbose  =    0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "GSNsbpcvwh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'G':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-G\" should be followed by a positive integer.\n" );
                goto usage;
            }
            pPars->GainRatio = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->GainRatio < 0 ) 
                goto usage;
            break;
        case 'S':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-S\" should be followed by a positive integer.\n" );
                goto usage;
            }
            pPars->Slew = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->Slew < 0 ) 
                goto usage;
            break;
        case 'N':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-N\" should be followed by a positive integer.\n" );
                goto usage;
            }
            pPars->nDegree = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->nDegree < 0 ) 
                goto usage;
            break;
        case 's':
            pPars->fSizeOnly ^= 1;
            break;
        case 'b':
            pPars->fAddBufs ^= 1;
            break;
        case 'p':
            pPars->fBufPis ^= 1;
            break;
        case 'c':
            pPars->fUseWireLoads ^= 1;
            break;
        case 'v':
            pPars->fVerbose ^= 1;
            break;
        case 'w':
            pPars->fVeryVerbose ^= 1;
            break;
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }

    if ( pNtk == NULL )
    {
        Abc_Print( -1, "Empty network.\n" );
        return 1;
    }
    if ( !Abc_NtkIsLogic(pNtk) )
    {
        Abc_Print( -1, "This command can only be applied to a logic network.\n" );
        return 1;
    }
    if ( !pPars->fSizeOnly && !pPars->fAddBufs && pNtk->vPhases == NULL )
    {
        Abc_Print( -1, "Fanin phase information is not avaiable.\n" );
        return 1;
    }
    if ( !pAbc->pLibScl || !Abc_SclHasDelayInfo(pAbc->pLibScl) )
    {
        Abc_Print( -1, "Library delay info is not available.\n" );
        return 1;
    }
    // modify the current network
    pNtkRes = Abc_SclBufferingPerform( pNtk, (SC_Lib *)pAbc->pLibScl, pPars );
    if ( pNtkRes == NULL )
    {
        Abc_Print( -1, "The command has failed.\n" );
        return 1;
    }
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtkRes );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: buffer [-GSN num] [-sbpcvwh]\n" );
    fprintf( pAbc->Err, "\t           performs buffering and sizing and mapped network\n" );
    fprintf( pAbc->Err, "\t-G <num> : target gain percentage [default = %d]\n", pPars->GainRatio );
    fprintf( pAbc->Err, "\t-S <num> : target slew in pisoseconds [default = %d]\n", pPars->Slew );
    fprintf( pAbc->Err, "\t-N <num> : the maximum fanout count [default = %d]\n", pPars->nDegree );
    fprintf( pAbc->Err, "\t-s       : toggle performing only sizing [default = %s]\n", pPars->fSizeOnly? "yes": "no" );
    fprintf( pAbc->Err, "\t-b       : toggle using buffers instead of inverters [default = %s]\n", pPars->fAddBufs? "yes": "no" );
    fprintf( pAbc->Err, "\t-p       : toggle buffering primary inputs [default = %s]\n", pPars->fBufPis? "yes": "no" );
    fprintf( pAbc->Err, "\t-c       : toggle using wire-loads if specified [default = %s]\n", pPars->fUseWireLoads? "yes": "no" );
    fprintf( pAbc->Err, "\t-v       : toggle printing verbose information [default = %s]\n", pPars->fVerbose? "yes": "no" );
    fprintf( pAbc->Err, "\t-w       : toggle printing more verbose information [default = %s]\n", pPars->fVeryVerbose? "yes": "no" );
    fprintf( pAbc->Err, "\t-h       : print the command usage\n");
    return 1;
} 

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_CommandBufferOld( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
    Abc_Ntk_t * pNtkRes;
    int FanMin, FanMax, FanMaxR, fAddInvs, fUseInvs, fBufPis, fSkipDup;
    int c, fVerbose;
    int fOldAlgo = 0;
    FanMin   =  6;
    FanMax   = 14;
    FanMaxR  =  0;
    fAddInvs =  0;
    fUseInvs =  0;
    fBufPis  =  0;
    fSkipDup =  0;
    fVerbose =  0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "NMRaixpdvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'N':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-N\" should be followed by a positive integer.\n" );
                goto usage;
            }
            FanMin = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( FanMin < 0 ) 
                goto usage;
            break;
        case 'M':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-M\" should be followed by a positive integer.\n" );
                goto usage;
            }
            FanMax = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( FanMax < 0 ) 
                goto usage;
            break;
        case 'R':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-R\" should be followed by a positive integer.\n" );
                goto usage;
            }
            FanMaxR = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( FanMaxR < 0 ) 
                goto usage;
            break;
        case 'a':
            fOldAlgo ^= 1;
            break;
        case 'i':
            fAddInvs ^= 1;
            break;
        case 'x':
            fUseInvs ^= 1;
            break;
        case 'p':
            fBufPis ^= 1;
            break;
        case 'd':
            fSkipDup ^= 1;
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

    if ( pNtk == NULL )
    {
        Abc_Print( -1, "Empty network.\n" );
        return 1;
    }
    if ( !Abc_NtkIsLogic(pNtk) )
    {
        Abc_Print( -1, "This command can only be applied to a logic network.\n" );
        return 1;
    }
    if ( fAddInvs && pNtk->vPhases == NULL )
    {
        Abc_Print( -1, "Fanin phase information is not avaiable.\n" );
        return 1;
    }
    if ( !pAbc->pLibScl || !Abc_SclHasDelayInfo(pAbc->pLibScl) )
    {
        Abc_Print( -1, "Library delay info is not available.\n" );
        return 1;
    }

    // modify the current network
    if ( fAddInvs )
        pNtkRes = Abc_SclBufferPhase( pNtk, fVerbose );
    else if ( fOldAlgo )
        pNtkRes = Abc_SclPerformBuffering( pNtk, FanMaxR, FanMax, fUseInvs, fVerbose );
    else
        pNtkRes = Abc_SclBufPerform( pNtk, FanMin, FanMax, fBufPis, fSkipDup, fVerbose );
    if ( pNtkRes == NULL )
    {
        Abc_Print( -1, "The command has failed.\n" );
        return 1;
    }
    // replace the current network
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtkRes );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: _buffer [-NMR num] [-aixpdvh]\n" );
    fprintf( pAbc->Err, "\t           performs buffering of the mapped network\n" );
    fprintf( pAbc->Err, "\t-N <num> : the min fanout considered by the algorithm [default = %d]\n", FanMin );
    fprintf( pAbc->Err, "\t-M <num> : the max allowed fanout count of node/buffer [default = %d]\n", FanMax );
    fprintf( pAbc->Err, "\t-R <num> : the max allowed fanout count of root node [default = %d]\n", FanMaxR );
    fprintf( pAbc->Err, "\t-a       : toggle using old algorithm [default = %s]\n", fOldAlgo? "yes": "no" );
    fprintf( pAbc->Err, "\t-i       : toggle adding interters instead of buffering [default = %s]\n", fAddInvs? "yes": "no" );
    fprintf( pAbc->Err, "\t-x       : toggle using interters instead of buffers [default = %s]\n", fUseInvs? "yes": "no" );
    fprintf( pAbc->Err, "\t-p       : toggle buffering primary inputs [default = %s]\n", fBufPis? "yes": "no" );
    fprintf( pAbc->Err, "\t-d       : toggle disabling gate duplication [default = %s]\n", fSkipDup? "yes": "no" );
    fprintf( pAbc->Err, "\t-v       : toggle printing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    fprintf( pAbc->Err, "\t-h       : print the command usage\n");
    return 1;
} 

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_CommandMinsize( Abc_Frame_t * pAbc, int argc, char **argv )
{
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
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

    if ( Abc_FrameReadNtk(pAbc) == NULL )
    {
        fprintf( pAbc->Err, "There is no current network.\n" );
        return 1;
    }
    if ( !Abc_NtkHasMapping(Abc_FrameReadNtk(pAbc)) )
    {
        fprintf( pAbc->Err, "The current network is not mapped.\n" );
        return 1;
    }
    if ( !Abc_SclCheckNtk(Abc_FrameReadNtk(pAbc), 0) )
    {
        fprintf( pAbc->Err, "The current network is not in a topo order (run \"topo\").\n" );
        return 1;
    }
    if ( pAbc->pLibScl == NULL )
    {
        fprintf( pAbc->Err, "There is no Liberty library available.\n" );
        return 1;
    }

    Abc_SclMinsizePerform( (SC_Lib *)pAbc->pLibScl, pNtk, 0, fVerbose );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: minsize [-vh]\n" );
    fprintf( pAbc->Err, "\t           downsizes all gates to their minimum size\n" );
    fprintf( pAbc->Err, "\t-v       : toggle printing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    fprintf( pAbc->Err, "\t-h       : print the command usage\n");
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_CommandMaxsize( Abc_Frame_t * pAbc, int argc, char **argv )
{
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
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

    if ( Abc_FrameReadNtk(pAbc) == NULL )
    {
        fprintf( pAbc->Err, "There is no current network.\n" );
        return 1;
    }
    if ( !Abc_NtkHasMapping(Abc_FrameReadNtk(pAbc)) )
    {
        fprintf( pAbc->Err, "The current network is not mapped.\n" );
        return 1;
    }
    if ( !Abc_SclCheckNtk(Abc_FrameReadNtk(pAbc), 0) )
    {
        fprintf( pAbc->Err, "The current network is not in a topo order (run \"topo\").\n" );
        return 1;
    }
    if ( pAbc->pLibScl == NULL )
    {
        fprintf( pAbc->Err, "There is no Liberty library available.\n" );
        return 1;
    }

    Abc_SclMinsizePerform( (SC_Lib *)pAbc->pLibScl, pNtk, 1, fVerbose );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: maxsize [-vh]\n" );
    fprintf( pAbc->Err, "\t           upsizes all gates to their maximum size\n" );
    fprintf( pAbc->Err, "\t-v       : toggle printing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    fprintf( pAbc->Err, "\t-h       : print the command usage\n");
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_CommandUpsize( Abc_Frame_t * pAbc, int argc, char **argv )
{
    SC_SizePars Pars, * pPars = &Pars;
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
    int c;
    memset( pPars, 0, sizeof(SC_SizePars) );
    pPars->nIters        = 1000;
    pPars->nIterNoChange =   50;
    pPars->Window        =    1;
    pPars->Ratio         =   10;
    pPars->Notches       = 1000;
    pPars->DelayUser     =    0;
    pPars->DelayGap      =    0;
    pPars->TimeOut       =    0;
    pPars->BuffTreeEst   =    0;
    pPars->BypassFreq    =    0;
    pPars->fUseDept      =    1;
    pPars->fUseWireLoads =    0;
    pPars->fDumpStats    =    0;
    pPars->fVerbose      =    0;
    pPars->fVeryVerbose  =    0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "IJWRNDGTXBcsdvwh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'I':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-I\" should be followed by a positive integer.\n" );
                goto usage;
            }
            pPars->nIters = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->nIters < 0 ) 
                goto usage;
            break;
        case 'J':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-J\" should be followed by a positive integer.\n" );
                goto usage;
            }
            pPars->nIterNoChange = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->nIterNoChange < 0 ) 
                goto usage;
            break;
        case 'W':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-W\" should be followed by a positive integer.\n" );
                goto usage;
            }
            pPars->Window = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->Window < 0 ) 
                goto usage;
            break;
        case 'R':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-R\" should be followed by a positive integer.\n" );
                goto usage;
            }
            pPars->Ratio = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->Ratio < 0 ) 
                goto usage;
            break;
        case 'N':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-N\" should be followed by a positive integer.\n" );
                goto usage;
            }
            pPars->Notches = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->Notches < 0 ) 
                goto usage;
            break;
        case 'D':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-D\" should be followed by a positive integer.\n" );
                goto usage;
            }
            pPars->DelayUser = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->DelayUser < 0 ) 
                goto usage;
            break;
        case 'G':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-G\" should be followed by a positive integer.\n" );
                goto usage;
            }
            pPars->DelayGap = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            break;
        case 'T':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-T\" should be followed by a positive integer.\n" );
                goto usage;
            }
            pPars->TimeOut = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->TimeOut < 0 ) 
                goto usage;
            break;
        case 'X':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-X\" should be followed by a positive integer.\n" );
                goto usage;
            }
            pPars->BuffTreeEst = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->BuffTreeEst < 0 ) 
                goto usage;
            break;
        case 'B':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-B\" should be followed by a positive integer.\n" );
                goto usage;
            }
            pPars->BypassFreq = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->BypassFreq < 0 ) 
                goto usage;
            break;
        case 'c':
            pPars->fUseWireLoads ^= 1;
            break;
        case 's':
            pPars->fUseDept ^= 1;
            break;
        case 'd':
            pPars->fDumpStats ^= 1;
            break;
        case 'v':
            pPars->fVerbose ^= 1;
            break;
        case 'w':
            pPars->fVeryVerbose ^= 1;
            break;
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }

    if ( Abc_FrameReadNtk(pAbc) == NULL )
    {
        fprintf( pAbc->Err, "There is no current network.\n" );
        return 1;
    }
    if ( !Abc_NtkHasMapping(Abc_FrameReadNtk(pAbc)) )
    {
        fprintf( pAbc->Err, "The current network is not mapped.\n" );
        return 1;
    }
    if ( !Abc_SclCheckNtk(Abc_FrameReadNtk(pAbc), 0) )
    {
        fprintf( pAbc->Err, "The current network is not in a topo order (run \"topo\").\n" );
        return 1;
    }
    if ( !pAbc->pLibScl || !Abc_SclHasDelayInfo(pAbc->pLibScl) )
    {
        Abc_Print( -1, "Library delay info is not available.\n" );
        return 1;
    }

    Abc_SclUpsizePerform( (SC_Lib *)pAbc->pLibScl, pNtk, pPars );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: upsize [-IJWRNDGTXB num] [-csdvwh]\n" );
    fprintf( pAbc->Err, "\t           selectively increases gate sizes on the critical path\n" );
    fprintf( pAbc->Err, "\t-I <num> : the number of upsizing iterations to perform [default = %d]\n", pPars->nIters );
    fprintf( pAbc->Err, "\t-J <num> : the number of iterations without improvement to stop [default = %d]\n", pPars->nIterNoChange );
    fprintf( pAbc->Err, "\t-W <num> : delay window (in percent) of near-critical COs [default = %d]\n", pPars->Window );
    fprintf( pAbc->Err, "\t-R <num> : ratio of critical nodes (in percent) to update [default = %d]\n", pPars->Ratio );
    fprintf( pAbc->Err, "\t-N <num> : limit on discrete upsizing steps at a node [default = %d]\n", pPars->Notches );
    fprintf( pAbc->Err, "\t-D <num> : delay target set by the user, in picoseconds [default = %d]\n", pPars->DelayUser );
    fprintf( pAbc->Err, "\t-G <num> : delay gap during updating, in picoseconds [default = %d]\n", pPars->DelayGap );
    fprintf( pAbc->Err, "\t-T <num> : approximate timeout in seconds [default = %d]\n", pPars->TimeOut );
    fprintf( pAbc->Err, "\t-X <num> : ratio for buffer tree estimation [default = %d]\n", pPars->BuffTreeEst );
    fprintf( pAbc->Err, "\t-B <num> : frequency of bypass transforms [default = %d]\n", pPars->BypassFreq );
    fprintf( pAbc->Err, "\t-c       : toggle using wire-loads if specified [default = %s]\n", pPars->fUseWireLoads? "yes": "no" );
    fprintf( pAbc->Err, "\t-s       : toggle using slack based on departure times [default = %s]\n", pPars->fUseDept? "yes": "no" );
    fprintf( pAbc->Err, "\t-d       : toggle dumping statistics into a file [default = %s]\n", pPars->fDumpStats? "yes": "no" );
    fprintf( pAbc->Err, "\t-v       : toggle printing verbose information [default = %s]\n", pPars->fVerbose? "yes": "no" );
    fprintf( pAbc->Err, "\t-w       : toggle printing more verbose information [default = %s]\n", pPars->fVeryVerbose? "yes": "no" );
    fprintf( pAbc->Err, "\t-h       : print the command usage\n");
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_CommandDnsize( Abc_Frame_t * pAbc, int argc, char **argv )
{
    SC_SizePars Pars, * pPars = &Pars;
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
    int c;
    memset( pPars, 0, sizeof(SC_SizePars) );
    pPars->nIters        =    5;
    pPars->nIterNoChange =   50;
    pPars->Notches       = 1000;
    pPars->DelayUser     =    0;
    pPars->DelayGap      = 1000;
    pPars->TimeOut       =    0;
    pPars->BuffTreeEst   =    0;
    pPars->fUseDept      =    1;
    pPars->fUseWireLoads =    0;
    pPars->fDumpStats    =    0;
    pPars->fVerbose      =    0;
    pPars->fVeryVerbose  =    0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "IJNDGTXcsdvwh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'I':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-I\" should be followed by a positive integer.\n" );
                goto usage;
            }
            pPars->nIters = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->nIters < 0 ) 
                goto usage;
            break;
        case 'J':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-J\" should be followed by a positive integer.\n" );
                goto usage;
            }
            pPars->nIterNoChange = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->nIterNoChange < 0 ) 
                goto usage;
            break;
        case 'N':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-N\" should be followed by a positive integer.\n" );
                goto usage;
            }
            pPars->Notches = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->Notches < 0 ) 
                goto usage;
            break;
        case 'D':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-D\" should be followed by a positive integer.\n" );
                goto usage;
            }
            pPars->DelayUser = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->DelayUser < 0 ) 
                goto usage;
            break;
        case 'G':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-G\" should be followed by a positive integer.\n" );
                goto usage;
            }
            pPars->DelayGap = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            break;
        case 'T':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-T\" should be followed by a positive integer.\n" );
                goto usage;
            }
            pPars->TimeOut = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->TimeOut < 0 ) 
                goto usage;
            break;
        case 'X':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-X\" should be followed by a positive integer.\n" );
                goto usage;
            }
            pPars->BuffTreeEst = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->BuffTreeEst < 0 ) 
                goto usage;
            break;
        case 'c':
            pPars->fUseWireLoads ^= 1;
            break;
        case 's':
            pPars->fUseDept ^= 1;
            break;
        case 'd':
            pPars->fDumpStats ^= 1;
            break;
        case 'v':
            pPars->fVerbose ^= 1;
            break;
        case 'w':
            pPars->fVeryVerbose ^= 1;
            break;
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }

    if ( Abc_FrameReadNtk(pAbc) == NULL )
    {
        fprintf( pAbc->Err, "There is no current network.\n" );
        return 1;
    }
    if ( !Abc_NtkHasMapping(Abc_FrameReadNtk(pAbc)) )
    {
        fprintf( pAbc->Err, "The current network is not mapped.\n" );
        return 1;
    }
    if ( !Abc_SclCheckNtk(Abc_FrameReadNtk(pAbc), 0) )
    {
        fprintf( pAbc->Err, "The current network is not in a topo order (run \"topo\").\n" );
        return 1;
    }
    if ( !pAbc->pLibScl || !Abc_SclHasDelayInfo(pAbc->pLibScl) )
    {
        Abc_Print( -1, "Library delay info is not available.\n" );
        return 1;
    }

    Abc_SclDnsizePerform( (SC_Lib *)pAbc->pLibScl, pNtk, pPars );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: dnsize [-IJNDGTX num] [-csdvwh]\n" );
    fprintf( pAbc->Err, "\t           selectively decreases gate sizes while maintaining delay\n" );
    fprintf( pAbc->Err, "\t-I <num> : the number of upsizing iterations to perform [default = %d]\n", pPars->nIters );
    fprintf( pAbc->Err, "\t-J <num> : the number of iterations without improvement to stop [default = %d]\n", pPars->nIterNoChange );
    fprintf( pAbc->Err, "\t-N <num> : limit on discrete upsizing steps at a node [default = %d]\n", pPars->Notches );
    fprintf( pAbc->Err, "\t-D <num> : delay target set by the user, in picoseconds [default = %d]\n", pPars->DelayUser );
    fprintf( pAbc->Err, "\t-G <num> : delay gap during updating, in picoseconds [default = %d]\n", pPars->DelayGap );
    fprintf( pAbc->Err, "\t-T <num> : approximate timeout in seconds [default = %d]\n", pPars->TimeOut );
    fprintf( pAbc->Err, "\t-X <num> : ratio for buffer tree estimation [default = %d]\n", pPars->BuffTreeEst );
    fprintf( pAbc->Err, "\t-c       : toggle using wire-loads if specified [default = %s]\n", pPars->fUseWireLoads? "yes": "no" );
    fprintf( pAbc->Err, "\t-s       : toggle using slack based on departure times [default = %s]\n", pPars->fUseDept? "yes": "no" );
    fprintf( pAbc->Err, "\t-d       : toggle dumping statistics into a file [default = %s]\n", pPars->fDumpStats? "yes": "no" );
    fprintf( pAbc->Err, "\t-v       : toggle printing verbose information [default = %s]\n", pPars->fVerbose? "yes": "no" );
    fprintf( pAbc->Err, "\t-w       : toggle printing more verbose information [default = %s]\n", pPars->fVeryVerbose? "yes": "no" );
    fprintf( pAbc->Err, "\t-h       : print the command usage\n");
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_CommandPrintBuf( Abc_Frame_t * pAbc, int argc, char **argv )
{
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
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

    if ( Abc_FrameReadNtk(pAbc) == NULL )
    {
        fprintf( pAbc->Err, "There is no current network.\n" );
        return 1;
    }
    if ( !Abc_NtkHasMapping(Abc_FrameReadNtk(pAbc)) )
    {
        fprintf( pAbc->Err, "The current network is not mapped.\n" );
        return 1;
    }
    if ( !Abc_SclCheckNtk(Abc_FrameReadNtk(pAbc), 0) )
    {
        fprintf( pAbc->Err, "The current network is not in a topo order (run \"topo\").\n" );
        return 1;
    }
    if ( !pAbc->pLibScl || !Abc_SclHasDelayInfo(pAbc->pLibScl) )
    {
        Abc_Print( -1, "Library delay info is not available.\n" );
        return 1;
    }

    Abc_SclPrintBuffers( (SC_Lib *)pAbc->pLibScl, pNtk, fVerbose );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: print_buf [-vh]\n" );
    fprintf( pAbc->Err, "\t           prints buffers trees of the current design\n" );
    fprintf( pAbc->Err, "\t-v       : toggle printing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    fprintf( pAbc->Err, "\t-h       : print the command usage\n");
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_CommandReadConstr( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    extern Abc_Nam_t * Abc_NtkNameMan( Abc_Ntk_t * p, int fOuts );
    extern void Abc_SclReadTimingConstr( Abc_Frame_t * pAbc, char * pFileName, int fVerbose );
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
    FILE * pFile;
    char * pFileName;
    int fUseNewFormat = 0;
    int c, fVerbose = 0;

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "nvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'n':
            fVerbose ^= 1;
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
    pFileName = argv[globalUtilOptind];
    if ( (pFile = fopen( pFileName, "rb" )) == NULL )
    {
        fprintf( pAbc->Err, "Cannot open input file \"%s\". \n", pFileName );
        return 1;
    }
    fclose( pFile );

    if ( !fUseNewFormat )
        Abc_SclReadTimingConstr( pAbc, pFileName, fVerbose );
    else
    {
        if ( pNtk == NULL )
        {
            fprintf( pAbc->Err, "There is no current network.\n" );
            return 1;
        }
        // input constraint manager
        if ( pNtk )
        {
            Scl_Con_t * pCon = Scl_ConRead( pFileName, Abc_NtkNameMan(pNtk, 0), Abc_NtkNameMan(pNtk, 1) );
            if ( pCon ) Scl_ConUpdateMan( pAbc, pCon );
        }
    }
    return 0;

usage:
    fprintf( pAbc->Err, "usage: read_constr [-nvh] <file>\n" );
    fprintf( pAbc->Err, "\t         read file with timing constraints for standard-cell designs\n" );
    fprintf( pAbc->Err, "\t-n     : toggle using new constraint file format [default = %s]\n", fUseNewFormat? "yes": "no" );
    fprintf( pAbc->Err, "\t-v     : toggle printing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    fprintf( pAbc->Err, "\t-h     : prints the command summary\n" );
    fprintf( pAbc->Err, "\t<file> : the name of a file to read\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_CommandWriteConstr( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Scl_Con_t * pCon = Scl_ConGetMan( pAbc );
    char * pFileName = NULL;
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
    if ( pCon == NULL )
    {
        Abc_Print( 1, "Scl_CommandWriteConstr(): There is no constraint manager.\n" );
        return 0;
    }
    if ( argc == globalUtilOptind + 1 )
        pFileName = argv[globalUtilOptind];
    else if ( argc == globalUtilOptind && pCon )
        pFileName = Extra_FileNameGenericAppend( pCon->pFileName, "_out.constr" );
    else 
    {
        printf( "Output file name should be given on the command line.\n" );
        return 0;
    }
    // perform writing
    if ( !strcmp( Extra_FileNameExtension(pFileName), "constr" )  )
        Scl_ConWrite( pCon, pFileName );
    else 
    {
        printf( "Scl_CommandWriteConstr(): Unrecognized output file extension.\n" );
        return 0;
    }
    return 0;

usage:
    fprintf( pAbc->Err, "usage: write_constr [-vh] <file>\n" );
    fprintf( pAbc->Err, "\t         writes current timing constraints into a file\n" );
    fprintf( pAbc->Err, "\t-v     : toggle printing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    fprintf( pAbc->Err, "\t-h     : prints the command summary\n" );
    fprintf( pAbc->Err, "\t<file> : the name of a file to read\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_CommandPrintConstr( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Scl_Con_t * pCon = Scl_ConGetMan( pAbc );
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
    //printf( "Primary input driving cell = %s\n", Abc_FrameReadDrivingCell() );
    //printf( "Primary output maximum load = %f\n", Abc_FrameReadMaxLoad() );

    if ( pCon ) Scl_ConWrite( pCon, NULL );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: print_constr [-vh] <file>\n" );
    fprintf( pAbc->Err, "\t         prints current timing constraints\n" );
    fprintf( pAbc->Err, "\t-v     : toggle printing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    fprintf( pAbc->Err, "\t-h     : prints the command summary\n" );
    fprintf( pAbc->Err, "\t<file> : the name of a file to read\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_CommandResetConstr( Abc_Frame_t * pAbc, int argc, char ** argv )
{
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
    Abc_FrameSetDrivingCell( NULL );
    Abc_FrameSetMaxLoad( 0 );

    Scl_ConUpdateMan( pAbc, NULL );
    return 0;

usage:
    fprintf( pAbc->Err, "usage: reset_constr [-vh] <file>\n" );
    fprintf( pAbc->Err, "\t         removes current timing constraints\n" );
    fprintf( pAbc->Err, "\t-v     : toggle printing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    fprintf( pAbc->Err, "\t-h     : prints the command summary\n" );
    fprintf( pAbc->Err, "\t<file> : the name of a file to read\n" );
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

