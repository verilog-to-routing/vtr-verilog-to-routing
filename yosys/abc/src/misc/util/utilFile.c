/**CFile****************************************************************

  FileName    [utilFile.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Temporary file utilities.]

  Synopsis    [Temporary file utilities.]

  Author      [Niklas Een]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 29, 2010.]

  Revision    [$Id: utilFile.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

// Handle legacy macros
#if !defined(S_IREAD)
#if defined(S_IRUSR)
#define S_IREAD S_IRUSR
#else
#error S_IREAD is undefined
#endif
#endif

#if !defined(S_IWRITE)
#if defined(S_IWUSR)
#define S_IWRITE S_IWUSR
#else
#error S_IWRITE is undefined
#endif
#endif

#if defined(_MSC_VER) || defined(__MINGW32__)
#include <windows.h>
#include <process.h>
#include <io.h>
#else
#include <unistd.h>
#endif

#include "abc_global.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static ABC_UINT64_T realTimeAbs()  // -- absolute time in nano-seconds
{
#if defined(_MSC_VER)
    LARGE_INTEGER f, t;
    double realTime_freq;
    int ok;

    ok = QueryPerformanceFrequency(&f); assert(ok);
    realTime_freq = 1.0 / (__int64)(((ABC_UINT64_T)f.LowPart) | ((ABC_UINT64_T)f.HighPart << 32));

    ok = QueryPerformanceCounter(&t); assert(ok);
    return (ABC_UINT64_T)(__int64)(((__int64)(((ABC_UINT64_T)t.LowPart | ((ABC_UINT64_T)t.HighPart << 32))) * realTime_freq * 1000000000));
#else
    return 0;
#endif
}

/**Function*************************************************************

  Synopsis    [Opens a temporary file.]

  Description [Opens a temporary file with given prefix and returns file 
  descriptor (-1 on failure) and a string containing the name of the file 
  (to be freed by caller).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int tmpFile(const char* prefix, const char* suffix, char** out_name)
{
#if defined(_MSC_VER) || defined(__MINGW32__)
    int i, fd;
    *out_name = (char*)malloc(strlen(prefix) + strlen(suffix) + 27);
    for (i = 0; i < 10; i++){
        sprintf(*out_name, "%s%I64X%d%s", prefix, realTimeAbs(), _getpid(), suffix);
        fd = _open(*out_name, O_CREAT | O_EXCL | O_BINARY | O_RDWR, _S_IREAD | _S_IWRITE);
        if (fd == -1){
            free(*out_name);
            *out_name = NULL;
        }
        return fd;
    }
    assert(0);  // -- could not open temporary file
    return 0;
#elif defined(__wasm)
    static int seq = 0; // no risk of collision since we're in a sandbox
    int fd;
    *out_name = (char*)malloc(strlen(prefix) + strlen(suffix) + 9);
    sprintf(*out_name, "%s%08d%s", prefix, seq++, suffix);
    fd = open(*out_name, O_CREAT | O_EXCL | O_RDWR, S_IREAD | S_IWRITE);
    if (fd == -1){
        free(*out_name);
        *out_name = NULL;
    }
    return fd;
#else
    int fd;
    *out_name = (char*)malloc(strlen(prefix) + strlen(suffix) + 7);
    assert(*out_name != NULL);
    sprintf(*out_name, "%sXXXXXX", prefix);
    fd = mkstemp(*out_name);
    if (fd == -1){
        free(*out_name);
        *out_name = NULL;
    }else{
        // Kludge:
        close(fd);
        unlink(*out_name);
        strcat(*out_name, suffix);
        fd = open(*out_name, O_CREAT | O_EXCL | O_RDWR, S_IREAD | S_IWRITE);
        if (fd == -1){
            free(*out_name);
            *out_name = NULL;
        }
    }
    return fd;
#endif
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
/*
int main(int argc, char** argv)
{
    char* tmp_filename;
    int   fd = tmpFile("__abctmp_", &tmp_filename);
    FILE* out = fdopen(fd, "wb");

    fprintf(out, "This file contains important information.\n");
    fclose(out);

    printf("Created: %s\n", tmp_filename);
    free(tmp_filename);

    return 0;
}
*/

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char* vnsprintf(const char* format, va_list args)
{
    unsigned n;
    char*    ret;
    va_list  args_copy;

    static FILE* dummy_file = NULL;
    if (!dummy_file)
    {
#if !defined(_MSC_VER) && !defined(__MINGW32)
        dummy_file = fopen("/dev/null", "wb");
#else
        dummy_file = fopen("NUL", "wb");
#endif
    }

#if defined(__va_copy)
    __va_copy(args_copy, args);
#else
  #if defined(va_copy)
    va_copy(args_copy, args);
  #else
    args_copy = args;
  #endif
#endif
    n = vfprintf(dummy_file, format, args);
    ret = ABC_ALLOC( char, n + 1 );
    ret[n] = (char)255;
    args = args_copy;
    vsprintf(ret, format, args);
#if !defined(__va_copy) && defined(va_copy)
    va_end(args_copy);
#endif
    assert(ret[n] == 0);
    return ret;
}

char* nsprintf(const char* format, ...)
{
    char* ret;
    va_list args;
    va_start(args, format);
    ret = vnsprintf(format, args);
    va_end(args);
    return ret;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

