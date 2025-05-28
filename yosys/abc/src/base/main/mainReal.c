/*////////////////////////////////////////////////////////////////////////////

ABC: System for Sequential Synthesis and Verification

http://www.eecs.berkeley.edu/~alanmi/abc/

Copyright (c) The Regents of the University of California. All rights reserved.

Permission is hereby granted, without written agreement and without license or
royalty fees, to use, copy, modify, and distribute this software and its
documentation for any purpose, provided that the above copyright notice and
the following two paragraphs appear in all copies of this software.

IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT OF
THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE. THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS" BASIS,
AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO PROVIDE MAINTENANCE,
SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

////////////////////////////////////////////////////////////////////////////*/

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

#ifndef WIN32
#include <sys/time.h>    
#include <sys/times.h>   
#include <sys/resource.h>
#include <unistd.h>      
#if !defined(__wasm)
#include <signal.h>      
#endif
#include <stdlib.h>
#endif

#include "base/abc/abc.h"
#include "mainInt.h"
#include "base/wlc/wlc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int TypeCheck( Abc_Frame_t * pAbc, const char * s);

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

unsigned enable_dbg_outs = 1;  

/**Function*************************************************************

  Synopsis    [The main() procedure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_RealMain( int argc, char * argv[] )
{
    Abc_Frame_t * pAbc;
    Vec_Str_t* sCommandUsr = Vec_StrAlloc(1000);
    char sCommandTmp[ABC_MAX_STR], sReadCmd[1000], sWriteCmd[1000];
    const char * sOutFile, * sInFile;
    char * sCommand;
    int  fStatus = 0;
    int c, fInitSource, fInitRead, fFinalWrite;

    enum {
        INTERACTIVE, // interactive mode
        BATCH, // batch mode, run a command and quit
        BATCH_THEN_INTERACTIVE, // run a command, then back to interactive mode
        BATCH_QUIET, // as in batch mode, but don't echo the command
        BATCH_QUIET_THEN_INTERACTIVE, // as in batch then interactive mode, but don't echo the command
        BATCH_SMT // special batch mode, which expends SMTLIB problem via stdin
    } fBatch;

    // added to detect memory leaks
    // watch for {,,msvcrtd.dll}*__p__crtBreakAlloc()
    // (http://support.microsoft.com/kb/151585)
#if defined(_DEBUG) && defined(_MSC_VER)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // get global frame (singleton pattern)
    // will be initialized on first call
    pAbc = Abc_FrameGetGlobalFrame();
    pAbc->sBinary = argv[0];

    // default options
    fBatch      = INTERACTIVE;
    fInitSource = 1;
    fInitRead   = 0;
    fFinalWrite = 0;
    sInFile = sOutFile = NULL;
    sprintf( sReadCmd,  "read"  );
    sprintf( sWriteCmd, "write" );

    Extra_UtilGetoptReset();
    while ((c = Extra_UtilGetopt(argc, argv, "dm:l:c:q:C:Q:S:hf:F:o:st:T:xb")) != EOF) {
        switch(c) {

            case 'd':                                          
                enable_dbg_outs ^= 1;                            
                break;                                          

            case 'm': {
#if !defined(WIN32) && !defined(ABC_NO_RLIMIT)
                int maxMb = atoi(globalUtilOptarg);             
                printf("Limiting memory use to %d MB\n", maxMb);
                struct rlimit limit = {                         
                    maxMb * (1llu << 20), /* soft limit */      
                    maxMb * (1llu << 20)  /* hard limit */      
                };                                              
                setrlimit(RLIMIT_AS, &limit);                   
#endif
                break; 
            }                                         
            case 'l': {
#if !defined(WIN32) && !defined(ABC_NO_RLIMIT)
                rlim_t maxTime = atoi(globalUtilOptarg);           
                printf("Limiting time to %d seconds\n", (int)maxTime);
                struct rlimit limit = {                         
                    maxTime,             /* soft limit */       
                    maxTime              /* hard limit */       
                };                                              
                setrlimit(RLIMIT_CPU, &limit);                  
#endif
                break; 
            }                                         
            case 'c':
                if( Vec_StrSize(sCommandUsr) > 0 )
                {
                    Vec_StrAppend(sCommandUsr, " ; ");
                }
                Vec_StrAppend(sCommandUsr, globalUtilOptarg );
                fBatch = BATCH;
                break;

            case 'q':
                if( Vec_StrSize(sCommandUsr) > 0 )
                {
                    Vec_StrAppend(sCommandUsr, " ; ");
                }
                Vec_StrAppend(sCommandUsr, globalUtilOptarg );
                fBatch = BATCH_QUIET;
                break;

            case 'Q':
                if( Vec_StrSize(sCommandUsr) > 0 )
                {
                    Vec_StrAppend(sCommandUsr, " ; ");
                }
                Vec_StrAppend(sCommandUsr, globalUtilOptarg );
                fBatch = BATCH_QUIET_THEN_INTERACTIVE;
                break;

            case 'C':
                if( Vec_StrSize(sCommandUsr) > 0 )
                {
                    Vec_StrAppend(sCommandUsr, " ; ");
                }
                Vec_StrAppend(sCommandUsr, globalUtilOptarg );
                fBatch = BATCH_THEN_INTERACTIVE;
                break;

            case 'S':
                if( Vec_StrSize(sCommandUsr) > 0 )
                {
                    Vec_StrAppend(sCommandUsr, " ; ");
                }
                Vec_StrAppend(sCommandUsr, globalUtilOptarg );
                fBatch = BATCH_SMT;
                break;

            case 'f':
                if( Vec_StrSize(sCommandUsr) > 0 )
                {
                    Vec_StrAppend(sCommandUsr, " ; ");
                }
                Vec_StrPrintF(sCommandUsr, "source %s", globalUtilOptarg );
                fBatch = BATCH;
                break;

            case 'F':
                if( Vec_StrSize(sCommandUsr) > 0 )
                {
                    Vec_StrAppend(sCommandUsr, " ; ");
                }
                Vec_StrPrintF(sCommandUsr, "source -x %s", globalUtilOptarg );
                fBatch = BATCH;
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
                    if ( (!strcmp(globalUtilOptarg, "none")) == 0 )
                    {
                        fInitRead = 1;
                        sprintf( sReadCmd, "read_%s", globalUtilOptarg );
                    }
                }
                else {
                    goto usage;
                }
                fBatch = BATCH;
                break;

            case 'T':
                if ( TypeCheck( pAbc, globalUtilOptarg ) )
                {
                    if ( (!strcmp(globalUtilOptarg, "none")) == 0)
                    {
                        fFinalWrite = 1;
                        sprintf( sWriteCmd, "write_%s", globalUtilOptarg);
                    }
                }
                else {
                    goto usage;
                }
                fBatch = BATCH;
                break;

            case 'x':
                fFinalWrite = 0;
                fInitRead   = 0;
                fBatch = BATCH;
                break;

            case 'b':
                Abc_FrameSetBridgeMode();
                break;

            default:
                goto usage;
        }
    }

    Vec_StrPush(sCommandUsr, '\0');

    if ( fBatch == BATCH_SMT )
    {
        Wlc_StdinProcessSmt( pAbc, Vec_StrArray(sCommandUsr) );
        Abc_Stop();
        return 0;
    }

    if ( Abc_FrameIsBridgeMode() )
    {
        extern Gia_Man_t * Gia_ManFromBridge( FILE * pFile, Vec_Int_t ** pvInit );
        pAbc->pGia = Gia_ManFromBridge( stdin, NULL );
    }
    else if ( fBatch!=INTERACTIVE && fBatch!=BATCH_QUIET && fBatch!=BATCH_QUIET_THEN_INTERACTIVE && Vec_StrSize(sCommandUsr)>0 )
        Abc_Print( 1, "ABC command line: \"%s\".\n\n", Vec_StrArray(sCommandUsr) );

    if ( fBatch!=INTERACTIVE )
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
            fStatus = Cmd_CommandExecute( pAbc, Vec_StrArray(sCommandUsr) );
            if ( (fStatus == 0 || fStatus == -1) && fFinalWrite && sOutFile )
            {
                sprintf( sCommandTmp, "%s %s", sWriteCmd, sOutFile );
                fStatus = Cmd_CommandExecute( pAbc, sCommandTmp );
            }
        }

        if (fBatch == BATCH_THEN_INTERACTIVE || fBatch == BATCH_QUIET_THEN_INTERACTIVE){
            fBatch = INTERACTIVE;
            pAbc->fBatchMode = 0;
        }
    }

    Vec_StrFreeP(&sCommandUsr);

    if ( fBatch==INTERACTIVE )
    {
        // start interactive mode

        // print the hello line
        Abc_UtilsPrintHello( pAbc );
        // print history of the recent commands
        Cmd_HistoryPrint( pAbc, 10 );

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
//    if ( fStatus < 0 ) 
    {
        Abc_Stop();
    }
    return 0;

usage:
    Abc_UtilsPrintHello( pAbc );
    Abc_UtilsPrintUsage( pAbc, argv[0] );
    return 1;
}

/**Function********************************************************************

  Synopsis    [Returns 1 if s is a file type recognized, else returns 0.]

  Description [Returns 1 if s is a file type recognized by ABC, else returns 0. 
  Recognized types are "blif", "bench", "pla", and "none".]

  SideEffects []

******************************************************************************/
static int TypeCheck( Abc_Frame_t * pAbc, const char * s )
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


ABC_NAMESPACE_IMPL_END
