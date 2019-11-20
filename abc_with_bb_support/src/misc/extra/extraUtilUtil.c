/**CFile****************************************************************

  FileName    [extraUtilUtil.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [extra]

  Synopsis    [Old SIS utilities.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: extraUtilUtil.c,v 1.0 2003/02/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include <stdio.h>
#include "extra.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define EXTRA_RLIMIT_DATA_DEFAULT 67108864  // assume 64MB by default 

/*  File   : getopt.c
 *  Author : Henry Spencer, University of Toronto
 *  Updated: 28 April 1984
 *
 *  Changes: (R Rudell)
 *  changed index() to strchr();
 *  added getopt_reset() to reset the getopt argument parsing
 *
 *  Purpose: get option letter from argv.
 */

char * globalUtilOptarg;        // Global argument pointer (util_optarg)
int    globalUtilOptind = 0;    // Global argv index (util_optind)

static char *pScanStr;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [util_cpu_time()]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
long Extra_CpuTime()
{
    return clock();
}

/**Function*************************************************************

  Synopsis    [getSoftDataLimit()]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_GetSoftDataLimit()
{
    return EXTRA_RLIMIT_DATA_DEFAULT;
}

/**Function*************************************************************

  Synopsis    [util_getopt_reset()]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_UtilGetoptReset()
{
    globalUtilOptarg = 0;
    globalUtilOptind = 0;
    pScanStr = 0;
}

/**Function*************************************************************

  Synopsis    [util_getopt()]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_UtilGetopt( int argc, char *argv[], char *optstring )
{
    register int c;
    register char *place;

    globalUtilOptarg = NULL;

    if (pScanStr == NULL || *pScanStr == '\0') {
    if (globalUtilOptind == 0) globalUtilOptind++;
    if (globalUtilOptind >= argc) return EOF;
    place = argv[globalUtilOptind];
    if (place[0] != '-' || place[1] == '\0') return EOF;
    globalUtilOptind++;
    if (place[1] == '-' && place[2] == '\0') return EOF;
    pScanStr = place+1;
    }

    c = *pScanStr++;
    place = strchr(optstring, c);
    if (place == NULL || c == ':') {
    (void) fprintf(stderr, "%s: unknown option %c\n", argv[0], c);
    return '?';
    }
    if (*++place == ':') {
    if (*pScanStr != '\0') {
        globalUtilOptarg = pScanStr;
        pScanStr = NULL;
    } else {
        if (globalUtilOptind >= argc) {
        (void) fprintf(stderr, "%s: %c requires an argument\n", 
            argv[0], c);
        return '?';
        }
        globalUtilOptarg = argv[globalUtilOptind];
        globalUtilOptind++;
    }
    }
    return c;
}

/**Function*************************************************************

  Synopsis    [util_print_time()]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Extra_UtilPrintTime( long t )
{
    static char s[40];

    (void) sprintf(s, "%ld.%02ld sec", t/1000, (t%1000)/10);
    return s;
}


/**Function*************************************************************

  Synopsis    [Extra_UtilStrsav()]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Extra_UtilStrsav( char *s )
{
    if(s == NULL) {  /* added 7/95, for robustness */
       return s;
    }
    else {
       return strcpy(ALLOC(char, strlen(s)+1), s);
    }
}

/**Function*************************************************************

  Synopsis    [util_tilde_expand()]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Extra_UtilTildeExpand( char *fname )
{
    return Extra_UtilStrsav( fname );
}

/**Function*************************************************************

  Synopsis    [check_file()]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_UtilCheckFile(char *filename, char *mode)
{
    FILE *fp;
    int got_file;

    if (strcmp(mode, "x") == 0) {
	mode = "r";
    }
    fp = fopen(filename, mode);
    got_file = (fp != 0);
    if (fp != 0) {
	(void) fclose(fp);
    }
    return got_file;
}

/**Function*************************************************************

  Synopsis    [util_file_search()]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Extra_UtilFileSearch(char *file, char *path, char *mode)
//char *file;			// file we're looking for 
//char *path;			// search path, colon separated 
//char *mode;			// "r", "w", or "x" 
{
    int quit;
    char *buffer, *filename, *save_path, *cp;

    if (path == 0 || strcmp(path, "") == 0) {
	path = ".";		/* just look in the current directory */
    }

    save_path = path = Extra_UtilStrsav(path);
    quit = 0;
    do {
	cp = strchr(path, ':');
	if (cp != 0) {
	    *cp = '\0';
	} else {
	    quit = 1;
	}

	/* cons up the filename out of the path and file name */
	if (strcmp(path, ".") == 0) {
	    buffer = Extra_UtilStrsav(file);
	} else {
	    buffer = ALLOC(char, strlen(path) + strlen(file) + 4);
	    (void) sprintf(buffer, "%s/%s", path, file);
	}
	filename = Extra_UtilTildeExpand(buffer);
	FREE(buffer);

	/* see if we can access it */
	if (Extra_UtilCheckFile(filename, mode)) {
	    FREE(save_path);
	    return filename;
	}
	FREE(filename);
	path = ++cp;
    } while (! quit); 

    FREE(save_path);
    return 0;
}

/**Function*************************************************************

  Synopsis    [MMout_of_memory()]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
/* MMout_of_memory -- out of memory for lazy people, flush and exit */
void Extra_UtilMMout_Of_Memory( long size )
{
    (void) fflush(stdout);
    (void) fprintf(stderr, "\nout of memory allocating %u bytes\n",
		   (unsigned) size);
	assert( 0 );
    exit(1);
}

/**Function*************************************************************

  Synopsis    [MMoutOfMemory()]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void (*Extra_UtilMMoutOfMemory)() = Extra_UtilMMout_Of_Memory;


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


