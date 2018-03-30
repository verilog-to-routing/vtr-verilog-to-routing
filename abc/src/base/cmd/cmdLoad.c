/**CFile****************************************************************

  FileName    [cmdApi.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [External procedures of the command package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cmdApi.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "base/main/mainInt.h"
#include "cmd.h"
#include "cmdInt.h"
#include "misc/util/utilSignal.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int CmdCommandLoad( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Vec_Str_t * vCommand;
    FILE * pFile;
    int i;
    vCommand = Vec_StrAlloc( 100 );
    Vec_StrAppend( vCommand, "abccmd_" );
    Vec_StrAppend( vCommand, argv[0] );
    Vec_StrAppend( vCommand, ".exe" );
    Vec_StrPush( vCommand, 0 );
    // check if there is the binary
    if ( (pFile = fopen( Vec_StrArray(vCommand), "r" )) == NULL )
    {
        Vec_StrFree( vCommand );
        Abc_Print( -1, "Cannot run the binary \"%s\".\n\n", Vec_StrArray(vCommand) );
        return 1;
    }
    fclose( pFile );
    Vec_StrPop( vCommand );
    // add other arguments
    for ( i = 1; i < argc; i++ )
    {
        Vec_StrAppend( vCommand, " " );
        Vec_StrAppend( vCommand, argv[i] );
    }
    Vec_StrPush( vCommand, 0 );
    // run the command line
    if ( Util_SignalSystem( Vec_StrArray(vCommand) ) )
    {
        Vec_StrFree( vCommand );
        Abc_Print( -1, "The following command has returned non-zero exit status:\n" );
        Abc_Print( -1, "\"%s\"\n", Vec_StrArray(vCommand) );
        return 1;
    }
    Vec_StrFree( vCommand );
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
#if defined(WIN32) && !defined(__cplusplus)

#include <direct.h>
#include <io.h>

/**Function*************************************************************

  Synopsis    [Collect file names ending with .exe]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * CmdCollectFileNames()
{
    Vec_Ptr_t * vFileNames;
    struct _finddata_t c_file;
    long   hFile;
    if( (hFile = _findfirst( "*.exe", &c_file )) == -1L )
    {
//        Abc_Print( 0, "No files with extention \"%s\" in the current directory.\n", "exe" );
        return NULL;
    }
    vFileNames = Vec_PtrAlloc( 100 );
    do {
        Vec_PtrPush( vFileNames, Extra_UtilStrsav( c_file.name ) );
    } while( _findnext( hFile, &c_file ) == 0 );
    _findclose( hFile );
    return vFileNames;
}

#else 

/**Function*************************************************************

  Synopsis    [Collect file names ending with .exe]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * CmdCollectFileNames()
{
    return NULL;
}

#endif


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Load_Init( Abc_Frame_t * pAbc )
{
    Vec_Ptr_t * vFileNames;
    char * pName, * pStop;
    int i;
    vFileNames = CmdCollectFileNames();
    if ( vFileNames == NULL )
        return;
    Vec_PtrForEachEntry( char *, vFileNames, pName, i )
    {
        if ( strncmp( pName, "abccmd_", 7 ) )
            continue;
        // get the command name
//        pName[6] = '!';
        pStop = strstr( pName + 7, "." );
        if ( pStop )
            *pStop = 0;
        // add the command
        Cmd_CommandAdd( pAbc, "ZZ", pName+7, CmdCommandLoad, 0 );
//        printf( "Loaded command \"%s\"\n", pName+7 );
    }
    Vec_PtrFreeFree( vFileNames );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Load_End( Abc_Frame_t * pAbc )
{
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

