/**CFile****************************************************************

  FileName    [mio.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [File reading/writing for technology mapping.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 18, 2003.]

  Revision    [$Id: mio.c,v 1.4 2004/08/05 18:34:51 satrajit Exp $]

***********************************************************************/

#include "abc.h"
#include "mvc.h"
#include "mainInt.h"
#include "mioInt.h"
#include "mapper.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Mio_CommandReadLibrary( Abc_Frame_t * pAbc, int argc, char **argv );
static int Mio_CommandPrintLibrary( Abc_Frame_t * pAbc, int argc, char **argv );

// internal version of GENLIB library
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
    char * pFileTemp = "mcnc_temp.genlib";
    Mio_Library_t * pLibGen;
    FILE * pFile;
    int i;

    // write genlib into file
    pFile = fopen( pFileTemp, "w" );
    for ( i = 0; pMcncGenlib[i]; i++ )
        fputs( pMcncGenlib[i], pFile );
    fclose( pFile );
    // read genlib from file
    pLibGen = Mio_LibraryRead( pAbc, pFileTemp, NULL, 0 );
    Abc_FrameSetLibGen( pLibGen );
#ifdef WIN32
        _unlink( pFileTemp );
#else
        unlink( pFileTemp );
#endif

    Cmd_CommandAdd( pAbc, "SC mapping", "read_library",   Mio_CommandReadLibrary,  0 ); 
    Cmd_CommandAdd( pAbc, "SC mapping", "print_library",  Mio_CommandPrintLibrary, 0 ); 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mio_End()
{
//    Mio_LibraryDelete( s_pLib );
    Mio_LibraryDelete( Abc_FrameReadLibGen() );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mio_CommandReadLibrary( Abc_Frame_t * pAbc, int argc, char **argv )
{
    FILE * pFile;
    FILE * pOut, * pErr;
    Mio_Library_t * pLib;
    Abc_Ntk_t * pNet;
    char * FileName;
    int fVerbose;
    int c;

    pNet = Abc_FrameReadNtk(pAbc);
    pOut = Abc_FrameReadOut(pAbc);
    pErr = Abc_FrameReadErr(pAbc);

    // set the defaults
    fVerbose = 1;
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
    {
        goto usage;
    }

    // get the input file name
    FileName = argv[globalUtilOptind];
    if ( (pFile = Io_FileOpen( FileName, "open_path", "r", 0 )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\". ", FileName );
        if ( (FileName = Extra_FileGetSimilarName( FileName, ".genlib", ".lib", ".gen", ".g", NULL )) )
            fprintf( pErr, "Did you mean \"%s\"?", FileName );
        fprintf( pErr, "\n" );
        return 1;
    }
    fclose( pFile );

    // set the new network
    pLib = Mio_LibraryRead( pAbc, FileName, 0, fVerbose );  
    if ( pLib == NULL )
    {
        fprintf( pErr, "Reading GENLIB library has failed.\n" );
        return 1;
    }
    // free the current superlib because it depends on the old Mio library
    if ( Abc_FrameReadLibSuper() )
    {
        extern void Map_SuperLibFree( Map_SuperLib_t * p );
//        Map_SuperLibFree( s_pSuperLib );
//        s_pSuperLib = NULL;
        Map_SuperLibFree( Abc_FrameReadLibSuper() );
        Abc_FrameSetLibSuper( NULL );
    }

    // replace the current library
//    Mio_LibraryDelete( s_pLib );
//    s_pLib = pLib;
    Mio_LibraryDelete( Abc_FrameReadLibGen() );
    Abc_FrameSetLibGen( pLib );
    return 0;

usage:
    fprintf( pErr, "usage: read_library [-vh]\n");
    fprintf( pErr, "\t         read the library from a genlib file\n" );  
    fprintf( pErr, "\t-h     : enable verbose output\n");
    return 1;       /* error exit */
}


/**Function*************************************************************

  Synopsis    [Command procedure to read LUT libraries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mio_CommandPrintLibrary( Abc_Frame_t * pAbc, int argc, char **argv )
{
    FILE * pOut, * pErr;
    Abc_Ntk_t * pNet;
    int fVerbose;
    int c;

    pNet = Abc_FrameReadNtk(pAbc);
    pOut = Abc_FrameReadOut(pAbc);
    pErr = Abc_FrameReadErr(pAbc);

    // set the defaults
    fVerbose = 1;
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


    if ( argc != globalUtilOptind )
    {
        goto usage;
    }

    // set the new network
    Mio_WriteLibrary( stdout, Abc_FrameReadLibGen(), 0 );
    return 0;

usage:
    fprintf( pErr, "\nusage: print_library [-vh]\n");
    fprintf( pErr, "\t          print the current genlib library\n" );  
    fprintf( pErr, "\t-v      : toggles enabling of verbose output [default = %s]\n", (fVerbose? "yes" : "no") );
    fprintf( pErr, "\t-h      : print the command usage\n");
    return 1;       /* error exit */
}
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


