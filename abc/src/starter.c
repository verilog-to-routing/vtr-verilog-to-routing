/**CFile****************************************************************

  FileName    [starter.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Wrapper for calling ABC.]

  Synopsis    [A demo program illustrating parallel execution of ABC.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - October 22, 2009.]

  Revision    [$Id: starter.c,v 1.00 2009/10/22 00:00:00 alanmi Exp $]

***********************************************************************/

// To compile on Linux run:  gcc -pthread -o starter starter.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#ifdef WIN32
#include "pthread.h"
#else
#include <pthread.h>
#include <unistd.h>
#endif

// the max number of commands to execute from the input file
#define MAX_COMM_NUM 1000

// time printing
#define ABC_PRT(a,t)    (printf("%s = ", (a)), printf("%7.2f sec\n", (float)(t)/(float)(CLOCKS_PER_SEC)))

// the number of currently running threads
static int nThreadsRunning = 0;

// mutext to control access to the number of threads
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// procedure for duplicating strings
char * Abc_UtilStrsav( char * s )   { return s ? strcpy(malloc(strlen(s)+1), s) : NULL;  }

/**Function*************************************************************

  Synopsis    [This procedures executes one call to system().]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Abc_RunThread( void * Command )
{
    // perform the call
    if ( system( (char *)Command ) )
    {
        assert(pthread_mutex_lock(&mutex) == 0);
        fprintf( stderr, "The following command has returned non-zero exit status:\n" );
        fprintf( stderr, "\"%s\"\n", (char *)Command );
        fprintf( stderr, "Sorry for the inconvenience.\n" );
        fflush( stdout );
        assert(pthread_mutex_unlock(&mutex) == 0);
    }

    // decrement the number of threads runining 
    assert(pthread_mutex_lock(&mutex) == 0);
    nThreadsRunning--;
    assert(pthread_mutex_unlock(&mutex) == 0);

    // quit this thread
	//printf("...Finishing %s\n", (char *)Command);
    free( Command );
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
int main( int argc, char * argv[] )
{
    FILE * pFile, * pOutput = stdout;
	pthread_t ThreadIds[MAX_COMM_NUM];
    char * pBufferCopy, Buffer[MAX_COMM_NUM];
	int i, nCPUs = 0, nLines = 0, Counter;
    clock_t clk = clock();
    
    // check command line arguments
    if ( argc != 3 )
        { fprintf( stderr, "Wrong number of command line arguments.\n" ); goto usage; }

    // get the number of CPUs
    nCPUs = atoi( argv[1] );
    if ( nCPUs <= 0 )
        { fprintf( pOutput, "Cannot read an integer represting the number of CPUs.\n" ); goto usage; }

    // open the file and make sure it is available
    pFile = fopen( argv[2], "r" );
    if ( pFile == NULL )
        { fprintf( pOutput, "Input file \"%s\" cannot be opened.\n", argv[2] ); goto usage; }

    // read commands and execute at most <num> of them at a time
//    assert(mutex == PTHREAD_MUTEX_INITIALIZER);
    while ( fgets( Buffer, MAX_COMM_NUM, pFile ) != NULL )
    {
        // get the command from the file
        if ( Buffer[0] == '\n' || Buffer[0] == '\r' || Buffer[0] == '\t' || 
			 Buffer[0] == ' ' || Buffer[0] == '#')
		{
            continue;
		}

        if ( Buffer[strlen(Buffer)-1] == '\n' )
            Buffer[strlen(Buffer)-1] = 0;
        if ( Buffer[strlen(Buffer)-1] == '\r' )
            Buffer[strlen(Buffer)-1] = 0;

        // wait till there is an empty thread
        while ( 1 )
        {
            assert(pthread_mutex_lock(&mutex) == 0);
            Counter = nThreadsRunning;
            assert(pthread_mutex_unlock(&mutex) == 0);
            if ( Counter < nCPUs - 1 )
                break;
//            Sleep( 100 );
        }

        // increament the number of threads running
        assert(pthread_mutex_lock(&mutex) == 0);
        nThreadsRunning++;
        printf( "Calling:  %s\n", (char *)Buffer );  
        fflush( stdout );
        assert(pthread_mutex_unlock(&mutex) == 0);

        // create thread to execute this command
        pBufferCopy = Abc_UtilStrsav( Buffer );
        assert(pthread_create( &ThreadIds[nLines], NULL, Abc_RunThread, (void *)pBufferCopy ) == 0);
        if ( ++nLines == MAX_COMM_NUM )
            { fprintf( pOutput, "Cannot execute more than %d commands from file \"%s\".\n", nLines, argv[2] ); break; }
    }

    // wait for all the threads to finish
    while ( 1 )
    {
        assert(pthread_mutex_lock(&mutex) == 0);
        Counter = nThreadsRunning;
        assert(pthread_mutex_unlock(&mutex) == 0);
        if ( Counter == 0 )
            break;
    }

    // cleanup
    assert(pthread_mutex_destroy(&mutex) == 0);
//    assert(mutex == NULL);
    fclose( pFile );
    printf( "Finished processing commands in file \"%s\".  ", argv[2] );
    ABC_PRT( "Total time", clock() - clk );
	return 0;

usage: 
    // skip the path name till the binary name
    for ( i = strlen(argv[0]) - 1; i > 0; i-- )
        if ( argv[0][i-1] == '\\' || argv[0][i-1] == '/' )
            break;
    // print usage message
    fprintf( pOutput, "usage: %s <num> <file>\n", argv[0]+i );
    fprintf( pOutput, "       executes command listed in <file> in parallel on <num> CPUs\n" );
    fprintf( pOutput, "\n" );
    return 1;

}

