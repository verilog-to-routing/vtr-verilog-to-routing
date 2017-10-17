/**CFile****************************************************************

  FileName    [main.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [The main package.]

  Synopsis    [Here everything starts.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: main.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "mainInt.h"

// this line should be included in the library project
//#define _LIB

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int TypeCheck( Abc_Frame_t * pAbc, char * s);

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

#ifndef _LIB

/**Function*************************************************************

  Synopsis    [The main() procedure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int main( int argc, char * argv[] )
{
    Abc_Frame_t * pAbc;
    char sCommandUsr[500], sCommandTmp[100], sReadCmd[20], sWriteCmd[20], c;
    char * sCommand, * sOutFile, * sInFile;
    int  fStatus = 0;
    bool fBatch, fInitSource, fInitRead, fFinalWrite;

    // added to detect memory leaks:
#ifdef _DEBUG
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
    
//    Npn_Experiment();
//    Npn_Generate();

	// get global frame (singleton pattern)
	// will be initialized on first call
	pAbc = Abc_FrameGetGlobalFrame();

    // default options
    fBatch = 0;
    fInitSource = 1;
    fInitRead   = 0;
    fFinalWrite = 0;
    sInFile = sOutFile = NULL;
    sprintf( sReadCmd,  "read"  );
    sprintf( sWriteCmd, "write" );
    
    Extra_UtilGetoptReset();
    while ((c = Extra_UtilGetopt(argc, argv, "c:hf:F:o:st:T:x")) != EOF) {
        switch(c) {
            case 'c':
                strcpy( sCommandUsr, globalUtilOptarg );
                fBatch = 1;
                break;
                
            case 'f':
                sprintf(sCommandUsr, "source %s", globalUtilOptarg);
                fBatch = 1;
                break;

            case 'F':
                sprintf(sCommandUsr, "source -x %s", globalUtilOptarg);
                fBatch = 1;
                break;
                
            case 'h':
                goto usage;
                break;
                
            case 'o':
                sOutFile = globalUtilOptarg;
                fFinalWrite = 1;
                break;
                
            case 's':
                fInitSource = 0;
                break;
                
            case 't':
                if ( TypeCheck( pAbc, globalUtilOptarg ) )
                {
                    if ( !strcmp(globalUtilOptarg, "none") == 0 )
                    {
                        fInitRead = 1;
                        sprintf( sReadCmd, "read_%s", globalUtilOptarg );
                    }
                }
                else {
                    goto usage;
                }
                fBatch = 1;
                break;
                
            case 'T':
                if ( TypeCheck( pAbc, globalUtilOptarg ) )
                {
                    if (!strcmp(globalUtilOptarg, "none") == 0)
                    {
                        fFinalWrite = 1;
                        sprintf( sWriteCmd, "write_%s", globalUtilOptarg);
                    }
                }
                else {
                    goto usage;
                }
                fBatch = 1;
                break;
                
            case 'x':
                fFinalWrite = 0;
                fInitRead   = 0;
                fBatch = 1;
                break;
                
            default:
                goto usage;
        }
    }
    
    if ( fBatch )
    {
		pAbc->fBatchMode = 1;

        if (argc - globalUtilOptind == 0)
        {
            sInFile = NULL;
        }
        else if (argc - globalUtilOptind == 1)
        {
            fInitRead = 1;
            sInFile = argv[globalUtilOptind];
        }
        else
        {
            Abc_UtilsPrintUsage( pAbc, argv[0] );
        }
        
        // source the resource file
        if ( fInitSource )
        {
            Abc_UtilsSource( pAbc );
        }
        
        fStatus = 0;
        if ( fInitRead && sInFile )
        {
            sprintf( sCommandTmp, "%s %s", sReadCmd, sInFile );
            fStatus = Cmd_CommandExecute( pAbc, sCommandTmp );
        }
        
        if ( fStatus == 0 )
        {
            /* cmd line contains `source <file>' */
            fStatus = Cmd_CommandExecute( pAbc, sCommandUsr );
            if ( (fStatus == 0 || fStatus == -1) && fFinalWrite && sOutFile )
            {
                sprintf( sCommandTmp, "%s %s", sWriteCmd, sOutFile );
                fStatus = Cmd_CommandExecute( pAbc, sCommandTmp );
            }
        }
        
    }
    else
    {
        // start interactive mode
        // print the hello line
        Abc_UtilsPrintHello( pAbc );
        
        // source the resource file
        if ( fInitSource )
        {
            Abc_UtilsSource( pAbc );
        }
		        
        // execute commands given by the user
        while ( !feof(stdin) )
        {
            // print command line prompt and
            // get the command from the user
            sCommand = Abc_UtilsGetUsersInput( pAbc );
            
            // execute the user's command
            fStatus = Cmd_CommandExecute( pAbc, sCommand );
            
            // stop if the user quitted or an error occurred
            if ( fStatus == -1 || fStatus == -2 )
                break;
        }
    }
      
    // if the memory should be freed, quit packages
    if ( fStatus < 0 ) 
    {
        Abc_Stop();
    }
    return 0;

usage:
    Abc_UtilsPrintHello( pAbc );
    Abc_UtilsPrintUsage( pAbc, argv[0] );
    return 1;
}

#endif

/**Function*************************************************************

  Synopsis    [Initialization procedure for the library project.]

  Description [Note that when Abc_Start() is run in a static library
  project, it does not load the resource file by default. As a result, 
  ABC is not set up the same way, as when it is run on a command line. 
  For example, some error messages while parsing files will not be 
  produced, and intermediate networks will not be checked for consistancy. 
  One possibility is to load the resource file after Abc_Start() as follows:
  Abc_UtilsSource(  Abc_FrameGetGlobalFrame() );]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_Start()
{
    Abc_Frame_t * pAbc;
    // added to detect memory leaks:
#ifdef _DEBUG
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
    // start the glocal frame
    pAbc = Abc_FrameGetGlobalFrame();
    // source the resource file
//    Abc_UtilsSource( pAbc );
}

/**Function*************************************************************

  Synopsis    [Deallocation procedure for the library project.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_Stop()
{
    Abc_Frame_t * pAbc;
    pAbc = Abc_FrameGetGlobalFrame();
    // perform uninitializations
    Abc_FrameEnd( pAbc );
    // stop the framework
    Abc_FrameDeallocate( pAbc );
}

/**Function********************************************************************

  Synopsis    [Returns 1 if s is a file type recognized, else returns 0.]

  Description [Returns 1 if s is a file type recognized by ABC, else returns 0. 
  Recognized types are "blif", "bench", "pla", and "none".]

  SideEffects []

******************************************************************************/
static int TypeCheck( Abc_Frame_t * pAbc, char * s )
{
    if (strcmp(s, "blif") == 0)
        return 1;
    else if (strcmp(s, "bench") == 0)
        return 1;
    else if (strcmp(s, "pla") == 0)
        return 1;
    else if (strcmp(s, "none") == 0)
        return 1;
    else {
        fprintf( pAbc->Err, "unknown type %s\n", s );
        return 0;
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


