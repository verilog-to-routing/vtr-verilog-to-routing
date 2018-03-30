/**CFile****************************************************************

  FileName    [mapper.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Command file for the mapper package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - June 1, 2004.]

  Revision    [$Id: mapper.c,v 1.7 2005/01/23 06:59:42 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "base/main/mainInt.h"
#include "map/mio/mio.h"
#include "mapperInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Map_CommandReadLibrary ( Abc_Frame_t * pAbc, int argc, char **argv );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_Init( Abc_Frame_t * pAbc )
{
    Cmd_CommandAdd( pAbc, "SC mapping", "read_super",  Map_CommandReadLibrary, 0 ); 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_End( Abc_Frame_t * pAbc )
{
//    Map_SuperLibFree( s_pSuperLib );
     Map_SuperLibFree( (Map_SuperLib_t *)Abc_FrameReadLibSuper() );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_CommandReadLibrary( Abc_Frame_t * pAbc, int argc, char **argv )
{
    FILE * pFile;
    FILE * pOut, * pErr;
    Map_SuperLib_t * pLib;
    Abc_Ntk_t * pNet;
    char * FileName, * ExcludeFile;
    int fVerbose;
    int fAlgorithm;
    int c;

    pNet = Abc_FrameReadNtk(pAbc);
    pOut = Abc_FrameReadOut(pAbc);
    pErr = Abc_FrameReadErr(pAbc);

    // set the defaults
    fVerbose = 1;
    fAlgorithm = 1;
    ExcludeFile = 0;
    Extra_UtilGetoptReset();
    while ( (c = Extra_UtilGetopt(argc, argv, "eovh")) != EOF ) 
    {
        switch (c) 
        {
            case 'e':
                ExcludeFile = argv[globalUtilOptind];
                if ( ExcludeFile == 0 )
                    goto usage;
                globalUtilOptind++;
                break;
            case 'o':
                fAlgorithm ^= 1;
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
    FileName = argv[globalUtilOptind];
    if ( (pFile = Io_FileOpen( FileName, "open_path", "r", 0 )) == NULL )
//    if ( (pFile = fopen( FileName, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\". ", FileName );
        if (( FileName = Extra_FileGetSimilarName( FileName, ".genlib", ".lib", ".gen", ".g", NULL )) )
            fprintf( pErr, "Did you mean \"%s\"?", FileName );
        fprintf( pErr, "\n" );
        return 1;
    }
    fclose( pFile );

    if ( Abc_FrameReadLibGen() == NULL )
    {
        fprintf( pErr, "Genlib library should be read in first..\n" );
        return 1;
    }

    // set the new network
    pLib = Map_SuperLibCreate( (Mio_Library_t *)Abc_FrameReadLibGen(), NULL, FileName, ExcludeFile, fAlgorithm, fVerbose );
    if ( pLib == NULL )
    {
        fprintf( pErr, "Reading supergate library has failed.\n" );
        return 1;
    }
    // replace the current library
//    Map_SuperLibFree( s_pSuperLib );
//    s_pSuperLib = pLib;
    Map_SuperLibFree( (Map_SuperLib_t *)Abc_FrameReadLibSuper() );
    Abc_FrameSetLibSuper( pLib );
    // replace the current genlib library
//    Mio_LibraryDelete( (Mio_Library_t *)Abc_FrameReadLibGen() );
//    Abc_FrameSetLibGen( (Mio_Library_t *)pLib->pGenlib );
    return 0;

usage:
    fprintf( pErr, "\nusage: read_super [-ovh]\n");
    fprintf( pErr, "\t         read the supergate library from the file\n" );  
    fprintf( pErr, "\t-e file : file contains list of genlib gates to exclude\n" );
    fprintf( pErr, "\t-o      : toggles the use of old file format [default = %s]\n", (fAlgorithm? "new" : "old") );
    fprintf( pErr, "\t-v      : toggles enabling of verbose output [default = %s]\n", (fVerbose? "yes" : "no") );
    fprintf( pErr, "\t-h      : print the command usage\n");
    return 1;       /* error exit */
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

