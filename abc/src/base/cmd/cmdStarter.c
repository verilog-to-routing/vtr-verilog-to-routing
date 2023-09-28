/**CFile****************************************************************

  FileName    [cmdStarter.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Command to start many instances of ABC in parallel.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cmdStarter.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "misc/util/abc_global.h"
#include "misc/extra/extra.h"

#ifdef ABC_USE_PTHREADS

#ifdef _WIN32
#include "../lib/pthread.h"
#else
#include <pthread.h>
#include <unistd.h>
#endif

#endif

ABC_NAMESPACE_IMPL_START
 
////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#ifndef ABC_USE_PTHREADS

void Cmd_RunStarter( char * pFileName, char * pBinary, char * pCommand, int nCores ) {}

#else // pthreads are used

// the number of concurrently running threads
static volatile int nThreadsRunning = 0;

// mutex to control access to the number of threads
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [This procedures executes one call to system().]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Abc_RunThread( void * pCommand )
{
    int status;
    // perform the call
    if ( system( (char *)pCommand ) )
    {
        fprintf( stderr, "The following command has returned non-zero exit status:\n" );
        fprintf( stderr, "\"%s\"\n\n", (char *)pCommand );
        fflush( stdout );
    }
    free( pCommand );

    // decrement the number of threads runining 
    status = pthread_mutex_lock(&mutex);   assert(status == 0);
    nThreadsRunning--;
    status = pthread_mutex_unlock(&mutex); assert(status == 0);

    // quit this thread
    //printf("...Finishing %s\n", (char *)Command);
    pthread_exit( NULL );
    assert(0);
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Takes file with commands to be executed and the number of CPUs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cmd_RunStarter( char * pFileName, char * pBinary, char * pCommand, int nCores )
{
    FILE * pFile, * pFileTemp;
    pthread_t * pThreadIds;
    char * BufferCopy, * Buffer;
    int nLines, LineMax, Line, Len;
    int i, c, status, Counter;
    abctime clk = Abc_Clock();

    // check the number of cores
    if ( nCores < 2 )
    {
        fprintf( stdout, "The number of cores (%d) should be more than 1.\n", nCores ); 
        return; 
    }

    // open the file and make sure it is available
    pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    { 
        fprintf( stdout, "Input file \"%s\" cannot be opened.\n", pFileName ); 
        return; 
    }

    // count the number of lines and the longest line
    nLines = LineMax = Line = 0;
    while ( (c = fgetc(pFile)) != EOF )
    {
        Line++;
        if ( c != '\n' )
            continue;
        nLines++;
        LineMax = Abc_MaxInt( LineMax, Line );
        Line = 0;       
    }
    nLines += 10;
    LineMax += LineMax + 100;
    LineMax += pBinary  ? strlen(pBinary) : 0;
    LineMax += pCommand ? strlen(pCommand) : 0;

    // allocate storage
    Buffer = ABC_ALLOC( char, LineMax );
    pThreadIds = ABC_ALLOC( pthread_t, nLines );

    // check if all files can be opened
    if ( pCommand != NULL )
    {
        // read file names
        rewind( pFile );
        for ( i = 0; fgets( Buffer, LineMax, pFile ) != NULL; i++ )
        {
            // remove trailing spaces
            for ( Len = strlen(Buffer) - 1; Len >= 0; Len-- )
                if ( Buffer[Len] == '\n' || Buffer[Len] == '\r' || Buffer[Len] == '\t' || Buffer[Len] == ' ' )
                    Buffer[Len] = 0;
                else
                    break;

            // get command from file
            if ( Buffer[0] == 0 || Buffer[0] == '\n' || Buffer[0] == '\r' || Buffer[0] == '\t' || Buffer[0] == ' ' || Buffer[0] == '#' )
                continue;

            // try to open the file
            pFileTemp = fopen( Buffer, "rb" );
            if ( pFileTemp == NULL )
            {
                fprintf( stdout, "Starter cannot open file \"%s\".\n", Buffer );
                fflush( stdout );
                ABC_FREE( pThreadIds );
                ABC_FREE( Buffer );
                fclose( pFile );
                return;
            }
            fclose( pFileTemp );
        }
    } 
 
    // read commands and execute at most <num> of them at a time
    rewind( pFile );
    for ( i = 0; fgets( Buffer, LineMax, pFile ) != NULL; i++ )
    {
        // remove trailing spaces
        for ( Len = strlen(Buffer) - 1; Len >= 0; Len-- )
            if ( Buffer[Len] == '\n' || Buffer[Len] == '\r' || Buffer[Len] == '\t' || Buffer[Len] == ' ' )
                Buffer[Len] = 0;
            else
                break;

        // get command from file
        if ( Buffer[0] == 0 || Buffer[0] == '\n' || Buffer[0] == '\r' || Buffer[0] == '\t' || Buffer[0] == ' ' || Buffer[0] == '#' )
            continue;

        // create command
        if ( pCommand != NULL )
        {
            BufferCopy = ABC_ALLOC( char, LineMax );
            sprintf( BufferCopy, "%s -c \"%s; %s\" > %s", pBinary, Buffer, pCommand, Extra_FileNameGenericAppend(Buffer, ".txt") );
        }
        else
            BufferCopy = Abc_UtilStrsav( Buffer );
        fprintf( stdout, "Calling:  %s\n", (char *)BufferCopy );  
        fflush( stdout );

        // wait till there is an empty thread
        while ( 1 )
        {
            status = pthread_mutex_lock(&mutex);   assert(status == 0);
            Counter = nThreadsRunning;
            status = pthread_mutex_unlock(&mutex); assert(status == 0);
            if ( Counter < nCores - 1 )
                break;
//            Sleep( 100 );
        }

        // increament the number of threads running
        status = pthread_mutex_lock(&mutex);   assert(status == 0);
        nThreadsRunning++;
        status = pthread_mutex_unlock(&mutex); assert(status == 0);

        // create thread to execute this command
        status = pthread_create( &pThreadIds[i], NULL, Abc_RunThread, (void *)BufferCopy );  assert(status == 0);
        assert( i < nLines );
    }
    ABC_FREE( pThreadIds );
    ABC_FREE( Buffer );
    fclose( pFile );

    // wait for all the threads to finish
    while ( 1 )
    {
        status = pthread_mutex_lock(&mutex);   assert(status == 0);
        Counter = nThreadsRunning;
        status = pthread_mutex_unlock(&mutex); assert(status == 0);
        if ( Counter == 0 )
            break;
    }

    // cleanup
//    status = pthread_mutex_destroy(&mutex);   assert(status == 0);
//    mutex = PTHREAD_MUTEX_INITIALIZER;
    fprintf( stdout, "Finished processing commands in file \"%s\".  ", pFileName );
    Abc_PrintTime( 1, "Total wall time", Abc_Clock() - clk );
    fflush( stdout );
}

#endif // pthreads are used

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

