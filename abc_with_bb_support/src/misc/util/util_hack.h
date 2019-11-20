/**CFile****************************************************************

  FileName    [util_hack.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [This file is used to simulate the presence of "util.h".]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: util_hack.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef __UTIL_HACK_H__
#define __UTIL_HACK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <math.h>

#define EXTERN                 extern
#define NIL(type)              ((type *) 0)
#define random                 rand
#define srandom                srand

#define util_cpu_time          Extra_CpuTime            
#define getSoftDataLimit       Extra_GetSoftDataLimit   
#define util_getopt_reset      Extra_UtilGetoptReset    
#define util_getopt            Extra_UtilGetopt         
#define util_print_time        Extra_UtilPrintTime      
#define util_strsav            Extra_UtilStrsav         
#define util_tilde_expand      Extra_UtilTildeExpand    
#define util_file_search       Extra_UtilFileSearch
#define MMoutOfMemory          Extra_UtilMMoutOfMemory      

#define util_optarg            globalUtilOptarg
#define util_optind            globalUtilOptind

#ifndef ARGS
#  ifdef __STDC__
#     define ARGS(args)	args
#  else
#     define ARGS(args) ()
# endif
#endif

#ifndef ABS
#  define ABS(a)			((a) < 0 ? -(a) : (a))
#endif

#ifndef MAX
#  define MAX(a,b)		((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#  define MIN(a,b)		((a) < (b) ? (a) : (b))
#endif

#define ALLOC(type, num)       ((type *) malloc(sizeof(type) * (num)))
#define FREE(obj)              ((obj) ? (free((char *) (obj)), (obj) = 0) : 0)
#define REALLOC(type, obj, num) \
        ((obj) ? ((type *) realloc((char *)(obj), sizeof(type) * (num))) : \
        ((type *) malloc(sizeof(type) * (num))))

extern long        Extra_CpuTime();
extern int         Extra_GetSoftDataLimit();
extern void        Extra_UtilGetoptReset();
extern int         Extra_UtilGetopt( int argc, char *argv[], char *optstring );
extern char *      Extra_UtilPrintTime( long t );
extern char *      Extra_UtilStrsav( char *s );
extern char *      Extra_UtilTildeExpand( char *fname );
extern char *      Extra_UtilFileSearch( char *file, char *path, char *mode );

extern char *      globalUtilOptarg;
extern int         globalUtilOptind;

#ifdef __cplusplus
}
#endif

#endif
