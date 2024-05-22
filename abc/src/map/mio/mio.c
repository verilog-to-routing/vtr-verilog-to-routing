/**CFile****************************************************************

  pFileName    [mio.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [File reading/writing for technology mapping.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 18, 2003.]

  Revision    [$Id: mio.c,v 1.4 2004/08/05 18:34:51 satrajit Exp $]

***********************************************************************/

//#define _BSD_SOURCE

#ifndef WIN32
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include <unistd.h>
#endif

#include "base/main/main.h"
#include "mio.h"
#include "map/mapper/mapper.h"
#include "map/amap/amap.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Mio_CommandReadLiberty( Abc_Frame_t * pAbc, int argc, char **argv );
static int Mio_CommandReadGenlib( Abc_Frame_t * pAbc, int argc, char **argv );
static int Mio_CommandWriteGenlib( Abc_Frame_t * pAbc, int argc, char **argv );
static int Mio_CommandPrintGenlib( Abc_Frame_t * pAbc, int argc, char **argv );
static int Mio_CommandReadProfile( Abc_Frame_t * pAbc, int argc, char **argv );
static int Mio_CommandWriteProfile( Abc_Frame_t * pAbc, int argc, char **argv );
static int Mio_CommandPrintProfile( Abc_Frame_t * pAbc, int argc, char **argv );

/*
// internal version of genlib library
static char * pMcncGenlib[25] = {
    "GATE inv1    1   O=!a;             PIN * INV     1 999 0.9 0.0 0.9 0.0\n",
    "GATE inv2    2   O=!a;             PIN * INV     2 999 1.0 0.0 1.0 0.0\n",
    "GATE inv3    3   O=!a;             PIN * INV     3 999 1.1 0.0 1.1 0.0\n",
    "GATE inv4    4   O=!a;             PIN * INV     4 999 1.2 0.0 1.2 0.0\n",
    "GATE nand2   2   O=!(a*b);         PIN * INV     1 999 1.0 0.0 1.0 0.0\n",
    "GATE nand3   3   O=!(a*b*c);       PIN * INV     1 999 1.1 0.0 1.1 0.0\n",
    "GATE nand4   4   O=!(a*b*c*d);     PIN * INV     1 999 1.4 0.0 1.4 0.0\n",
    "GATE nor2    2   O=!(a+b);         PIN * INV     1 999 1.4 0.0 1.4 0.0\n",
    "GATE nor3    3   O=!(a+b+c);       PIN * INV     1 999 2.4 0.0 2.4 0.0\n",
    "GATE nor4    4   O=!(a+b+c+d);     PIN * INV     1 999 3.8 0.0 3.8 0.0\n",
    "GATE xora    5   O=a*!b+!a*b;      PIN * UNKNOWN 2 999 1.9 0.0 1.9 0.0\n",
    "GATE xorb    5   O=!(a*b+!a*!b);   PIN * UNKNOWN 2 999 1.9 0.0 1.9 0.0\n",
    "GATE xnora   5   O=a*b+!a*!b;      PIN * UNKNOWN 2 999 2.1 0.0 2.1 0.0\n",
    "GATE xnorb   5   O=!(!a*b+a*!b);   PIN * UNKNOWN 2 999 2.1 0.0 2.1 0.0\n",
    "GATE aoi21   3   O=!(a*b+c);       PIN * INV     1 999 1.6 0.0 1.6 0.0\n",
    "GATE aoi22   4   O=!(a*b+c*d);     PIN * INV     1 999 2.0 0.0 2.0 0.0\n",
    "GATE oai21   3   O=!((a+b)*c);     PIN * INV     1 999 1.6 0.0 1.6 0.0\n",
    "GATE oai22   4   O=!((a+b)*(c+d)); PIN * INV     1 999 2.0 0.0 2.0 0.0\n",
    "GATE buf     1   O=a;              PIN * NONINV  1 999 1.0 0.0 1.0 0.0\n",
    "GATE zero    0   O=CONST0;\n",
    "GATE one     0   O=CONST1;\n"
};
*/
////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mio_Init( Abc_Frame_t * pAbc )
{
//    Cmd_CommandAdd( pAbc, "SC mapping", "read_liberty",   Mio_CommandReadLiberty,  0 ); 

    Cmd_CommandAdd( pAbc, "SC mapping", "read_genlib",    Mio_CommandReadGenlib,  0 ); 
    Cmd_CommandAdd( pAbc, "SC mapping", "write_genlib",   Mio_CommandWriteGenlib, 0 ); 
    Cmd_CommandAdd( pAbc, "SC mapping", "print_genlib",   Mio_CommandPrintGenlib, 0 ); 

    Cmd_CommandAdd( pAbc, "SC mapping", "read_profile",   Mio_CommandReadProfile,  0 ); 
    Cmd_CommandAdd( pAbc, "SC mapping", "write_profile",  Mio_CommandWriteProfile, 0 ); 
    Cmd_CommandAdd( pAbc, "SC mapping", "print_profile",  Mio_CommandPrintProfile, 0 ); 

    Cmd_CommandAdd( pAbc, "SC mapping", "read_library",   Mio_CommandReadGenlib,  0 ); 
    Cmd_CommandAdd( pAbc, "SC mapping", "write_library",  Mio_CommandWriteGenlib, 0 ); 
    Cmd_CommandAdd( pAbc, "SC mapping", "print_library",  Mio_CommandPrintGenlib, 0 ); 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mio_End( Abc_Frame_t * pAbc )
{
    Mio_LibraryDelete( (Mio_Library_t *)Abc_FrameReadLibGen() );
    Amap_LibFree( (Amap_Lib_t *)Abc_FrameReadLibGen2() );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mio_UpdateGenlib( Mio_Library_t * pLib )
{
    // free the current superlib because it depends on the old Mio library
    if ( Abc_FrameReadLibSuper() )
    {
        Map_SuperLibFree( (Map_SuperLib_t *)Abc_FrameReadLibSuper() );
        Abc_FrameSetLibSuper( NULL );
    }

    // replace the current library
    Mio_LibraryDelete( (Mio_Library_t *)Abc_FrameReadLibGen() );
    Abc_FrameSetLibGen( pLib );

    // replace the current library
    Amap_LibFree( (Amap_Lib_t *)Abc_FrameReadLibGen2() );
    Abc_FrameSetLibGen2( NULL );
}
int Mio_UpdateGenlib2( Vec_Str_t * vStr, Vec_Str_t * vStr2, char * pFileName, int fVerbose )
{
    Mio_Library_t * pLib;
    // set the new network
    pLib = Mio_LibraryRead( pFileName, Vec_StrArray(vStr), NULL, fVerbose );  
    if ( pLib == NULL )
        return 0;

    // free the current superlib because it depends on the old Mio library
    if ( Abc_FrameReadLibSuper() )
    {
        Map_SuperLibFree( (Map_SuperLib_t *)Abc_FrameReadLibSuper() );
        Abc_FrameSetLibSuper( NULL );
    }

    // replace the current library
    Mio_LibraryDelete( (Mio_Library_t *)Abc_FrameReadLibGen() );
    Abc_FrameSetLibGen( pLib );

    // set the new network
    pLib = (Mio_Library_t *)Amap_LibReadAndPrepare( pFileName, Vec_StrArray(vStr2), 0, 0 );  
    if ( pLib == NULL )
        return 0;

    // replace the current library
    Amap_LibFree( (Amap_Lib_t *)Abc_FrameReadLibGen2() );
    Abc_FrameSetLibGen2( pLib );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mio_CommandReadLiberty( Abc_Frame_t * pAbc, int argc, char **argv )
{
    int fUseFileInterface = 0;
    char Command[1000];
    FILE * pFile;
    FILE * pOut, * pErr;
    char * pFileName;
    int fVerbose;
    int c;

    pOut = Abc_FrameReadOut(pAbc);
    pErr = Abc_FrameReadErr(pAbc);

    // set the defaults
    fVerbose = 0;
    Extra_UtilGetoptReset();
    while ( (c = Extra_UtilGetopt(argc, argv, "vh")) != EOF ) 
    {
        switch (c) 
        {
            case 'v':
                fVerbose ^= 1;
                break;
            case 'h':
                goto usage;
                break;
            default:
                goto usage;
        }
    }

    if ( argc != globalUtilOptind + 1 )
        goto usage;

    // get the input file name
    pFileName = argv[globalUtilOptind];
    if ( (pFile = Io_FileOpen( pFileName, "open_path", "r", 0 )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\". ", pFileName );
        if ( (pFileName = Extra_FileGetSimilarName( pFileName, ".genlib", ".lib", ".scl", ".g", NULL )) )
            fprintf( pErr, "Did you mean \"%s\"?", pFileName );
        fprintf( pErr, "\n" );
        return 1;
    }
    fclose( pFile );

    if ( fUseFileInterface )
    {
        if ( !Amap_LibertyParse( pFileName, fVerbose ) )
            return 0;
        assert( strlen(pFileName) < 900 );
        sprintf( Command, "read_genlib %s", Extra_FileNameGenericAppend(pFileName, ".genlib") );
        Cmd_CommandExecute( pAbc, Command );
    }
    else
    {
        Vec_Str_t * vStr, * vStr2;
        int RetValue;
        vStr = Amap_LibertyParseStr( pFileName, fVerbose );
        if ( vStr == NULL )
            return 0;
        vStr2 = Vec_StrDup(vStr);
        RetValue = Mio_UpdateGenlib2( vStr, vStr2, pFileName, fVerbose );
        Vec_StrFree( vStr );
        Vec_StrFree( vStr2 );
        if ( !RetValue )
            printf( "Reading library has filed.\n" );
    }
    return 0;

usage:
    fprintf( pErr, "usage: read_liberty [-vh]\n");
    fprintf( pErr, "\t         read standard cell library in Liberty format\n" );  
    fprintf( pErr, "\t         (if the library contains more than one gate\n" );  
    fprintf( pErr, "\t         with the same Boolean function, only the gate\n" );  
    fprintf( pErr, "\t         with the smallest area will be used)\n" );  
    fprintf( pErr, "\t-v     : toggle verbose printout [default = %s]\n", fVerbose? "yes": "no" );
    fprintf( pErr, "\t-h     : enable verbose output\n");
    return 1;       
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mio_CommandReadGenlib( Abc_Frame_t * pAbc, int argc, char **argv )
{
    FILE * pFile;
    FILE * pOut, * pErr;
    Mio_Library_t * pLib;
    Amap_Lib_t * pLib2;
    char * pFileName;
    char * pExcludeFile = NULL;
    double WireDelay = 0.0;
    int fShortNames = 0;
    int c, fVerbose = 1;

    pOut = Abc_FrameReadOut(pAbc);
    pErr = Abc_FrameReadErr(pAbc);
    Extra_UtilGetoptReset();
    while ( (c = Extra_UtilGetopt(argc, argv, "WEnvh")) != EOF ) 
    {
        switch (c) 
        {
            case 'W':
                if ( globalUtilOptind >= argc )
                {
                    Abc_Print( -1, "Command line switch \"-W\" should be followed by a floating point number.\n" );
                    goto usage;
                }
                WireDelay = (float)atof(argv[globalUtilOptind]);
                globalUtilOptind++;
                if ( WireDelay <= 0.0 ) 
                    goto usage;
                break;
            case 'E':
                if ( globalUtilOptind >= argc )
                {
                    Abc_Print( -1, "Command line switch \"-E\" should be followed by a file name.\n" );
                    goto usage;
                }
                pExcludeFile = argv[globalUtilOptind];
                globalUtilOptind++;
                break;
            case 'n':
                fShortNames ^= 1;
                break;
            case 'v':
                fVerbose ^= 1;
                break;
            case 'h':
                goto usage;
                break;
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
    if ( (pFile = Io_FileOpen( pFileName, "open_path", "r", 0 )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\". ", pFileName );
        if ( (pFileName = Extra_FileGetSimilarName( pFileName, ".genlib", ".lib", ".scl", ".g", NULL )) )
            fprintf( pErr, "Did you mean \"%s\"?", pFileName );
        fprintf( pErr, "\n" );
        return 1;
    }
    fclose( pFile );

    // set the new network
    pLib = Mio_LibraryRead( pFileName, NULL, pExcludeFile, fVerbose );  
    if ( pLib == NULL )
    {
        fprintf( pErr, "Reading genlib library has failed.\n" );
        return 1;
    }
    if ( fVerbose )
        printf( "Entered genlib library with %d gates from file \"%s\".\n", Mio_LibraryReadGateNum(pLib), pFileName );

    // convert the library if needed
    if ( fShortNames )
        Mio_LibraryShortNames( pLib );

    // add the fixed number (wire delay) to all delays in the library
    if ( WireDelay != 0.0 )
        Mio_LibraryShiftDelay( pLib, WireDelay );

    // prepare libraries
    Mio_UpdateGenlib( pLib );

    // replace the current library
    pLib2 = Amap_LibReadAndPrepare( pFileName, NULL, 0, 0 );  
    if ( pLib2 == NULL )
    {
        fprintf( pErr, "Reading second genlib library has failed.\n" );
        return 1;
    }
    Abc_FrameSetLibGen2( pLib2 );
    return 0;

usage:
    fprintf( pErr, "usage: read_genlib [-W float] [-E filename] [-nvh]\n");
    fprintf( pErr, "\t           read the library from a genlib file\n" );  
    fprintf( pErr, "\t           (if the library contains more than one gate\n" );  
    fprintf( pErr, "\t           with the same Boolean function, only the gate\n" );  
    fprintf( pErr, "\t           with the smallest area will be used)\n" );  
    fprintf( pErr, "\t-W float : wire delay (added to pin-to-pin gate delays) [default = %g]\n", WireDelay );  
    fprintf( pErr, "\t-E file :  the file name with gates to be excluded [default = none]\n" );
    fprintf( pErr, "\t-n       : toggle replacing gate/pin names by short strings [default = %s]\n", fShortNames? "yes": "no" );
    fprintf( pErr, "\t-v       : toggle verbose printout [default = %s]\n", fVerbose? "yes": "no" );
    fprintf( pErr, "\t-h       : enable verbose output\n");
    return 1;       
}

/**Function*************************************************************

  Synopsis    [Command procedure to read LUT libraries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mio_CommandWriteGenlib( Abc_Frame_t * pAbc, int argc, char **argv )
{
    FILE * pOut, * pErr, * pFile;
    char * pFileName;
    int fSelected = 0;
    int fVerilog = 0;
    int fVerbose = 0;
    int c;

    pOut = Abc_FrameReadOut(pAbc);
    pErr = Abc_FrameReadErr(pAbc);

    Extra_UtilGetoptReset();
    while ( (c = Extra_UtilGetopt(argc, argv, "agvh")) != EOF ) 
    {
        switch (c) 
        {
            case 'a':
                fSelected ^= 1;
                break;
            case 'g':
                fVerilog ^= 1;
                break;
            case 'v':
                fVerbose ^= 1;
                break;
            case 'h':
                goto usage;
                break;
            default:
                goto usage;
        }
    }
    if ( Abc_FrameReadLibGen() == NULL )
    {
        printf( "Library is not available.\n" );
        return 1;
    }
    if ( argc != globalUtilOptind + 1 )
    {
        printf( "The file name is not given.\n" );
        return 1;
    }

    pFileName = argv[globalUtilOptind];
    pFile = fopen( pFileName, "w" );
    if ( pFile == NULL )
    {
        printf( "Error! Cannot open file \"%s\" for writing the library.\n", pFileName );
        return 1;
    }
    if ( fVerilog )
        Mio_WriteLibraryVerilog( pFile, (Mio_Library_t *)Abc_FrameReadLibGen(), 0, 0, fSelected );
    else
        Mio_WriteLibrary( pFile, (Mio_Library_t *)Abc_FrameReadLibGen(), 0, 0, fSelected );
    fclose( pFile );
    printf( "The current genlib library is written into file \"%s\".\n", pFileName );
    return 0;

usage:
    fprintf( pErr, "\nusage: write_genlib [-agvh] <file>\n");
    fprintf( pErr, "\t          writes the current genlib library into a file\n" );  
    fprintf( pErr, "\t-a      : toggles writing min-area gates [default = %s]\n", fSelected? "yes" : "no" );
    fprintf( pErr, "\t-g      : toggles writing the library in Verilog [default = %s]\n", fVerilog? "yes" : "no" );
    fprintf( pErr, "\t-v      : toggles enabling of verbose output [default = %s]\n", fVerbose? "yes" : "no" );
    fprintf( pErr, "\t-h      : print the command usage\n");
    fprintf( pErr, "\t<file>  : optional file name to write the library\n");
    return 1;       
}

/**Function*************************************************************

  Synopsis    [Command procedure to read LUT libraries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mio_CommandPrintGenlib( Abc_Frame_t * pAbc, int argc, char **argv )
{
    FILE * pOut, * pErr;
    int fShort = 0;
    int fSelected = 0;
    int fVerbose = 0;
    int c;

    pOut = Abc_FrameReadOut(pAbc);
    pErr = Abc_FrameReadErr(pAbc);

    // set the defaults
    Extra_UtilGetoptReset();
    while ( (c = Extra_UtilGetopt(argc, argv, "savh")) != EOF ) 
    {
        switch (c) 
        {
            case 's':
                fShort ^= 1;
                break;
            case 'a':
                fSelected ^= 1;
                break;
            case 'v':
                fVerbose ^= 1;
                break;
            case 'h':
                goto usage;
                break;
            default:
                goto usage;
        }
    }
    if ( Abc_FrameReadLibGen() == NULL )
    {
        printf( "Library is not available.\n" );
        return 1;
    }
    Mio_WriteLibrary( stdout, (Mio_Library_t *)Abc_FrameReadLibGen(), 0, fShort, fSelected );
    return 0;

usage:
    fprintf( pErr, "\nusage: print_genlib [-savh]\n");
    fprintf( pErr, "\t          print the current genlib library\n" );  
    fprintf( pErr, "\t-s      : toggles writing short form [default = %s]\n", fShort? "yes" : "no" );
    fprintf( pErr, "\t-a      : toggles writing min-area gates [default = %s]\n", fSelected? "yes" : "no" );
    fprintf( pErr, "\t-v      : toggles enabling of verbose output [default = %s]\n", fVerbose? "yes" : "no" );
    fprintf( pErr, "\t-h      : print the command usage\n");
    return 1;       
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mio_CommandReadProfile( Abc_Frame_t * pAbc, int argc, char **argv )
{
    FILE * pFile, * pOut, * pErr;
    Mio_Library_t * pLib;
    char * pFileName;
    int c;

    pOut = Abc_FrameReadOut(pAbc);
    pErr = Abc_FrameReadErr(pAbc);

    // set the defaults
    Extra_UtilGetoptReset();
    while ( (c = Extra_UtilGetopt(argc, argv, "h")) != EOF ) 
    {
        switch (c) 
        {
            case 'h':
                goto usage;
                break;
            default:
                goto usage;
        }
    }
    if ( argc != globalUtilOptind + 1 )
    {
        goto usage;
    }

    pLib = (Mio_Library_t *)Abc_FrameReadLibGen();
    if ( pLib == NULL )
    {
        fprintf( pErr, "There is no Genlib library entered.\n" );
        return 1;
    }

    // get the input file name
    pFileName = argv[globalUtilOptind];
    if ( (pFile = Io_FileOpen( pFileName, "open_path", "r", 0 )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\". ", pFileName );
        if ( (pFileName = Extra_FileGetSimilarName( pFileName, ".profile", NULL, NULL, NULL, NULL )) )
            fprintf( pErr, "Did you mean \"%s\"?", pFileName );
        fprintf( pErr, "\n" );
        return 1;
    }

    // set the new network
    Mio_LibraryReadProfile( pFile, pLib );
    fclose( pFile );
    return 0;

usage:
    fprintf( pErr, "usage: read_profile [-h] <file>\n");
    fprintf( pErr, "\t          read a gate profile from a profile file\n" );  
    fprintf( pErr, "\t-h      : enable verbose output\n");
    fprintf( pErr, "\t<file>  : file name to read the profile\n");
    return 1;       
}

/**Function*************************************************************

  Synopsis    [Command procedure to read LUT libraries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mio_CommandWriteProfile( Abc_Frame_t * pAbc, int argc, char **argv )
{
    FILE * pOut, * pErr, * pFile;
    char * pFileName;
    int c;

    pOut = Abc_FrameReadOut(pAbc);
    pErr = Abc_FrameReadErr(pAbc);

    Extra_UtilGetoptReset();
    while ( (c = Extra_UtilGetopt(argc, argv, "h")) != EOF ) 
    {
        switch (c) 
        {
            case 'h':
                goto usage;
                break;
            default:
                goto usage;
        }
    }
    if ( Abc_FrameReadLibGen() == NULL )
    {
        printf( "Library is not available.\n" );
        return 1;
    }
    if ( argc != globalUtilOptind + 1 )
    {
        printf( "The file name is not given.\n" );
        return 1;
    }

    pFileName = argv[globalUtilOptind];
    pFile = fopen( pFileName, "w" );
    if ( pFile == NULL )
    {
        printf( "Error! Cannot open file \"%s\" for writing the library.\n", pFileName );
        return 1;
    }
    Mio_LibraryWriteProfile( pFile, (Mio_Library_t *)Abc_FrameReadLibGen() );
    fclose( pFile );
    printf( "The current profile is written into file \"%s\".\n", pFileName );
    return 0;

usage:
    fprintf( pErr, "\nusage: write_profile [-h] <file>\n");
    fprintf( pErr, "\t          writes the current profile into a file\n" );  
    fprintf( pErr, "\t-h      : print the command usage\n");
    fprintf( pErr, "\t<file>  : file name to write the profile\n");
    return 1;       
}

/**Function*************************************************************

  Synopsis    [Command procedure to read LUT libraries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mio_CommandPrintProfile( Abc_Frame_t * pAbc, int argc, char **argv )
{
    FILE * pOut, * pErr;
    int fShort = 0;
    int fSelected = 0;
    int fVerbose = 0;
    int c;

    pOut = Abc_FrameReadOut(pAbc);
    pErr = Abc_FrameReadErr(pAbc);

    // set the defaults
    Extra_UtilGetoptReset();
    while ( (c = Extra_UtilGetopt(argc, argv, "savh")) != EOF ) 
    {
        switch (c) 
        {
            case 's':
                fShort ^= 1;
                break;
            case 'a':
                fSelected ^= 1;
                break;
            case 'v':
                fVerbose ^= 1;
                break;
            case 'h':
                goto usage;
                break;
            default:
                goto usage;
        }
    }
    if ( Abc_FrameReadLibGen() == NULL )
    {
        printf( "Library is not available.\n" );
        return 1;
    }
    Mio_LibraryWriteProfile( stdout, (Mio_Library_t *)Abc_FrameReadLibGen() );
    return 0;

usage:
    fprintf( pErr, "\nusage: print_profile [-h]\n");
    fprintf( pErr, "\t          print the current gate profile\n" );  
    fprintf( pErr, "\t-h      : print the command usage\n");
    return 1;       
}
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

