/*
 * Revision Control Information
 *
 * $Source: /vol/opua/opua2/sis/sis-1.2/common/src/sis/util/RCS/util.h,v $
 * $Author: sis $
 * $Revision: 1.9 $
 * $Date: 1993/06/07 21:04:07 $
 *
 */
#ifndef ABC__misc__espresso__util_old_h
#define ABC__misc__espresso__util_old_h

#if defined(_IBMR2)
#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE           /* Argh!  IBM strikes again */
#endif
#ifndef _ALL_SOURCE
#define _ALL_SOURCE             /* Argh!  IBM strikes again */
#endif
#ifndef _ANSI_C_SOURCE
#define _ANSI_C_SOURCE          /* Argh!  IBM strikes again */
#endif
#endif

#if defined(__STDC__) || defined(sprite) || defined(_IBMR2) || defined(__osf__)
#include <unistd.h>
#endif

#if defined(_IBMR2) && !defined(__STDC__)
#define _BSD
#endif

#include "ansi.h"	/* since some files don't include sis.h */

/* This was taken out and defined at compile time in the SIS Makefile
   that uses the OctTools.  When the OctTools are used, USE_MM is defined,
   because the OctTools contain libmm.a.  Otherwise, USE_MM is not defined,
   since the mm package is not distributed with SIS, only with Oct. */

/* #define USE_MM */		/* choose libmm.a as the memory allocator */

#define NIL(type)		((type *) 0)

#ifdef USE_MM
/*
 *  assumes the memory manager is libmm.a
 *	- allows malloc(0) or realloc(obj, 0)
 *	- catches out of memory (and calls MMout_of_memory())
 *	- catch free(0) and realloc(0, size) in the macros
 */
#define ALLOC(type, num)	\
    ((type *) malloc(sizeof(type) * (num)))
#define REALLOC(type, obj, num)	\
    (obj) ? ((type *) realloc((char *) obj, sizeof(type) * (num))) : \
	    ((type *) malloc(sizeof(type) * (num)))
#define FREE(obj)		\
    ((obj) ? (free((char *) (obj)), (obj) = 0) : 0)
#else
/*
 *  enforce strict semantics on the memory allocator
 *	- when in doubt, delete the '#define USE_MM' above
 */
#define ALLOC(type, num)	\
    ((type *) MMalloc((long) sizeof(type) * (long) (num)))
#define REALLOC(type, obj, num)	\
    ((type *) MMrealloc((char *) (obj), (long) sizeof(type) * (long) (num)))
#define FREE(obj)		\
    ((obj) ? (free((void *) (obj)), (obj) = 0) : 0)
#endif


/* Ultrix (and SABER) have 'fixed' certain functions which used to be int */
#if defined(ultrix) || defined(SABER) || defined(aiws) || defined(__hpux) || defined(__STDC__) || defined(apollo)
#define VOID_HACK void
#else
#define VOID_HACK int
#endif


/* No machines seem to have much of a problem with these */
#include <stdio.h>
#include <ctype.h>


/* Some machines fail to define some functions in stdio.h */
#if !defined(__STDC__) && !defined(sprite) && !defined(_IBMR2) && !defined(__osf__)
extern FILE *popen(), *tmpfile();
extern int pclose();
#ifndef clearerr		/* is a macro on many machines, but not all */
extern VOID_HACK clearerr();
#endif
#ifndef rewind
extern VOID_HACK rewind();
#endif
#endif

#ifndef PORT_H
#include <sys/types.h>
#include <signal.h>
#if defined(ultrix)
#if defined(_SIZE_T_)
#define ultrix4
#else
#if defined(SIGLOST)
#define ultrix3
#else
#define ultrix2
#endif
#endif
#endif
#endif

/* most machines don't give us a header file for these */
#if defined(__STDC__) || defined(sprite) || defined(_IBMR2) || defined(__osf__) || defined(sunos4) || defined(__hpux)
#include <stdlib.h>
#if defined(__hpux)
#include <errno.h>    /* For perror() defininition */
#endif /* __hpux */
#else
extern VOID_HACK abort(), free(), exit(), perror();
extern char *getenv();
#ifdef ultrix4
extern void *malloc(), *realloc(), *calloc();
#else
extern char *malloc(), *realloc(), *calloc();
#endif
#if defined(aiws) 
extern int sprintf();
#else
#ifndef _IBMR2
extern char *sprintf();
#endif
#endif
extern int system();
extern double atof();
#endif

#ifndef PORT_H
#if defined(ultrix3) || defined(sunos4) || defined(_IBMR2) || defined(__STDC__)
#define SIGNAL_FN       void
#else
/* sequent, ultrix2, 4.3BSD (vax, hp), sunos3 */
#define SIGNAL_FN       int
#endif
#endif

/* some call it strings.h, some call it string.h; others, also have memory.h */
#if defined(__STDC__) || defined(sprite)
#include <string.h>
#else
#if defined(ultrix4) || defined(__hpux)
#include <strings.h>
#else
#if defined(_IBMR2) || defined(__osf__)
#include<string.h>
#include<strings.h>
#else
/* ANSI C string.h -- 1/11/88 Draft Standard */
/* ugly, awful hack */
#ifndef PORT_H
extern char *strcpy(), *strncpy(), *strcat(), *strncat(), *strerror();
extern char *strpbrk(), *strtok(), *strchr(), *strrchr(), *strstr();
extern int strcoll(), strxfrm(), strncmp(), strlen(), strspn(), strcspn();
extern char *memmove(), *memccpy(), *memchr(), *memcpy(), *memset();
extern int memcmp(), strcmp();
#endif
#endif
#endif
#endif

/* a few extras */
#if defined(__hpux)
#define random() lrand48()
#define srandom(a) srand48(a)
#define bzero(a,b) memset(a, 0, b)
#else
#if !defined(__osf__) && !defined(linux)
/* these are defined as macros in stdlib.h */
extern VOID_HACK srandom();
extern long random();
#endif
#endif

/* code from sis-1.3 commented out below
#if defined(__STDC__) || defined(sprite)
#include <assert.h>
#else
#ifndef NDEBUG
#define assert(ex) {\
    if (! (ex)) {\
	(void) fprintf(stderr,\
	    "Assertion failed: file %s, line %d\n\"%s\"\n",\
	    __FILE__, __LINE__, "ex");\
	(void) fflush(stdout);\
	abort();\
    }\
}
#else
#define assert(ex) {ex;}
#endif
#endif
*/

   /* Sunil 5/3/97:
   sis-1.4: dont let the assert call go to the OS, since
   much of the code in SIS has actual computation done in
   the assert function. %$#$@@#! The OS version of assert
   will do nothing if NDEBUG is set. We cant let that happen...
   */
#  ifdef NDEBUG
#    define assert(ex) {ex;}
#  else
#    define assert(ex) {\
    if (! (ex)) {\
	(void) fprintf(stderr,\
	    "Assertion failed: file %s, line %d\n\"%s\"\n",\
	    __FILE__, __LINE__, "ex");\
	(void) fflush(stdout);\
	abort();\
    }\
}
#  endif


#define fail(why) {\
    (void) fprintf(stderr, "Fatal error: file %s, line %d\n%s\n",\
	__FILE__, __LINE__, why);\
    (void) fflush(stdout);\
    abort();\
}


#ifdef lint
#undef putc			/* correct lint '_flsbuf' bug */
#undef ALLOC			/* allow for lint -h flag */
#undef REALLOC
#define ALLOC(type, num)	(((type *) 0) + (num))
#define REALLOC(type, obj, num)	((obj) + (num))
#endif

/*
#if !defined(__osf__)
#define MAXPATHLEN
 1024
#endif
*/

/* These arguably do NOT belong in util.h */
#ifndef ABS
#define ABS(a)			((a) < 0 ? -(a) : (a))
#endif
#ifndef MAX
#define MAX(a,b)		((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b)		((a) < (b) ? (a) : (b))
#endif


#ifndef USE_MM
EXTERN void MMout_of_memory ARGS((long));
EXTERN char *MMalloc ARGS((long));
EXTERN char *MMrealloc ARGS((char *, long));
EXTERN void MMfree ARGS((char *));
#endif

EXTERN void util_print_cpu_stats ARGS((FILE *));
EXTERN long util_cpu_time ARGS((void));
EXTERN void util_getopt_reset ARGS((void));
EXTERN int util_getopt ARGS((int, char **, char *));
EXTERN char *util_path_search ARGS((char *));
EXTERN char *util_file_search ARGS((char *, char *, char *));
EXTERN int util_pipefork ARGS((char **, FILE **, FILE **, int *));
EXTERN char *util_print_time ARGS((long));
EXTERN int util_save_image ARGS((char *, char *));
EXTERN char *util_strsav ARGS((char *));
EXTERN int util_do_nothing ARGS((void));
EXTERN char *util_tilde_expand ARGS((char *));
EXTERN char *util_tempnam ARGS((char *, char *));
EXTERN FILE *util_tmpfile ARGS((void));
EXTERN long getSoftDataLimit();

#define ptime()         util_cpu_time()
#define print_time(t)   util_print_time(t)

/* util_getopt() global variables (ack !) */
extern int util_optind;
extern char *util_optarg;

#include <math.h>
#ifndef HUGE_VAL
#ifndef HUGE
#define HUGE  8.9884656743115790e+307
#endif
#define HUGE_VAL HUGE
#endif
#ifndef MAXINT
#define MAXINT (1 << 30)
#endif

#include <varargs.h>

ABC_NAMESPACE_HEADER_START



ABC_NAMESPACE_HEADER_END

#endif
