/**CFile****************************************************************

  FileName    [abc_global.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Global declarations.]

  Synopsis    [Global declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - Jan 30, 2009.]

  Revision    [$Id: abc_global.h,v 1.00 2009/01/30 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__misc__util__abc_global_h
#define ABC__misc__util__abc_global_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#ifndef ABC_USE_BRIDGE
#define ABC_USE_BRIDGE
#endif

#ifdef _WIN32
#ifndef __MINGW32__
#define inline __inline // compatible with MS VS 6.0
#pragma warning(disable : 4152) // warning C4152: nonstandard extension, function/data pointer conversion in expression
#pragma warning(disable : 4200) // warning C4200: nonstandard extension used : zero-sized array in struct/union
#pragma warning(disable : 4244) // warning C4244: '+=' : conversion from 'int ' to 'unsigned short ', possible loss of data
#pragma warning(disable : 4514) // warning C4514: 'Vec_StrPop' : unreferenced inline function has been removed
#pragma warning(disable : 4710) // warning C4710: function 'Vec_PtrGrow' not inlined
//#pragma warning( disable : 4273 )
#endif
#endif

#ifdef WIN32
  #ifdef WIN32_NO_DLL
    #define ABC_DLLEXPORT
    #define ABC_DLLIMPORT
  #else
    #define ABC_DLLEXPORT __declspec(dllexport)
    #define ABC_DLLIMPORT __declspec(dllimport)
  #endif
#else  /* defined(WIN32) */
#define ABC_DLLIMPORT
#endif /* defined(WIN32) */

#ifndef ABC_DLL
#define ABC_DLL ABC_DLLIMPORT
#endif

#if !defined(___unused)
#if defined(__GNUC__)
#define ___unused __attribute__ ((__unused__))
#else
#define ___unused
#endif
#endif

/*
#ifdef __cplusplus
#error "C++ code"
#else
#error "C code"
#endif
*/

#include <time.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

// catch memory leaks in Visual Studio
#ifdef WIN32
  #ifdef _DEBUG
    #define _CRTDBG_MAP_ALLOC
    #include <crtdbg.h>
  #endif
#endif

#include "abc_namespaces.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

#ifdef ABC_USE_STDINT_H
// If there is stdint.h, assume this is a reasonably-modern platform that
// would also have stddef.h and limits.h
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#if UINTPTR_MAX == 0xFFFFFFFFFFFFFFFF
#define SIZEOF_VOID_P 8
#ifdef _WIN32
#define NT64
#else
#define LIN64
#endif
#elif UINTPTR_MAX == 0xFFFFFFFF
#define SIZEOF_VOID_P 4
#ifdef _WIN32
#define NT
#else
#define LIN
#endif
#else
   #error unsupported platform
#endif

#if ULONG_MAX == 0xFFFFFFFFFFFFFFFF
#define SIZEOF_LONG 8
#elif ULONG_MAX == 0xFFFFFFFF
#define SIZEOF_LONG 4
#else
   #error unsupported platform
#endif

#if UINT_MAX == 0xFFFFFFFFFFFFFFFF
#define SIZEOF_INT 8
#elif UINT_MAX == 0xFFFFFFFF
#define SIZEOF_INT 4
#else
   #error unsupported platform
#endif

#endif

ABC_NAMESPACE_HEADER_START

/**
 * Pointer difference type; replacement for ptrdiff_t.
 * This is a signed integral type that is the same size as a pointer.
 * NOTE: This type may be different sizes on different platforms.
 */
#if       defined(__ccdoc__)
typedef platform_dependent_type ABC_PTRDIFF_T;
#elif     defined(ABC_USE_STDINT_H)
typedef ptrdiff_t ABC_PTRDIFF_T;
#elif     defined(LIN64)
typedef long ABC_PTRDIFF_T;
#elif     defined(NT64)
typedef long long ABC_PTRDIFF_T;
#elif     defined(NT) || defined(LIN) || defined(WIN32)
typedef int ABC_PTRDIFF_T;
#else
   #error unknown platform
#endif /* defined(PLATFORM) */

/**
 * Unsigned integral type that can contain a pointer.
 * This is an unsigned integral type that is the same size as a pointer.
 * NOTE: This type may be different sizes on different platforms.
 */
#if       defined(__ccdoc__)
typedef platform_dependent_type ABC_PTRUINT_T;
#elif     defined(ABC_USE_STDINT_H)
typedef uintptr_t ABC_PTRUINT_T;
#elif     defined(LIN64)
typedef unsigned long ABC_PTRUINT_T;
#elif     defined(NT64)
typedef unsigned long long ABC_PTRUINT_T;
#elif     defined(NT) || defined(LIN) || defined(WIN32)
typedef unsigned int ABC_PTRUINT_T;
#else
   #error unknown platform
#endif /* defined(PLATFORM) */

/**
 * Signed integral type that can contain a pointer.
 * This is a signed integral type that is the same size as a pointer.
 * NOTE: This type may be different sizes on different platforms.
 */
#if       defined(__ccdoc__)
typedef platform_dependent_type ABC_PTRINT_T;
#elif     defined(ABC_USE_STDINT_H)
typedef intptr_t ABC_PTRINT_T;
#elif     defined(LIN64)
typedef long ABC_PTRINT_T;
#elif     defined(NT64)
typedef long long ABC_PTRINT_T;
#elif     defined(NT) || defined(LIN) || defined(WIN32)
typedef int ABC_PTRINT_T;
#else
   #error unknown platform
#endif /* defined(PLATFORM) */

/**
 * 64-bit signed integral type.
 */
#if       defined(__ccdoc__)
typedef platform_dependent_type ABC_INT64_T;
#elif     defined(ABC_USE_STDINT_H)
typedef int64_t ABC_INT64_T;
#elif     defined(LIN64)
typedef long ABC_INT64_T;
#elif     defined(NT64) || defined(LIN)
typedef long long ABC_INT64_T;
#elif     defined(WIN32) || defined(NT)
typedef signed __int64 ABC_INT64_T;
#else
   #error unknown platform
#endif /* defined(PLATFORM) */

/**
 * 64-bit unsigned integral type.
 */
#if       defined(__ccdoc__)
typedef platform_dependent_type ABC_UINT64_T;
#elif     defined(ABC_USE_STDINT_H)
typedef uint64_t ABC_UINT64_T;
#elif     defined(LIN64)
typedef unsigned long ABC_UINT64_T;
#elif     defined(NT64) || defined(LIN)
typedef unsigned long long ABC_UINT64_T;
#elif     defined(WIN32) || defined(NT)
typedef unsigned __int64 ABC_UINT64_T;
#else
   #error unknown platform
#endif /* defined(PLATFORM) */

#ifdef LIN
  #define ABC_CONST(number) number ## ULL 
#else // LIN64 and windows
  #define ABC_CONST(number) number
#endif

typedef ABC_UINT64_T word;
typedef ABC_INT64_T iword;

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

#define ABC_INFINITY    (1000000000)

#define ABC_SWAP(Type, a, b)  { Type t = a; a = b; b = t; }

#define ABC_PRT(a,t)    (Abc_Print(1, "%s =", (a)), Abc_Print(1, "%9.2f sec\n", 1.0*((double)(t))/((double)CLOCKS_PER_SEC)))
#define ABC_PRTr(a,t)   (Abc_Print(1, "%s =", (a)), Abc_Print(1, "%9.2f sec\r", 1.0*((double)(t))/((double)CLOCKS_PER_SEC)))
#define ABC_PRTn(a,t)   (Abc_Print(1, "%s =", (a)), Abc_Print(1, "%9.2f sec  ", 1.0*((double)(t))/((double)CLOCKS_PER_SEC)))
#define ABC_PRTP(a,t,T) (Abc_Print(1, "%s =", (a)), Abc_Print(1, "%9.2f sec (%6.2f %%)\n", 1.0*((double)(t))/((double)CLOCKS_PER_SEC), ((double)(T))? 100.0*((double)(t))/((double)(T)) : 0.0))
#define ABC_PRM(a,f)    (Abc_Print(1, "%s =", (a)), Abc_Print(1, "%10.3f MB\n",    1.0*((double)(f))/(1<<20)))
#define ABC_PRMr(a,f)   (Abc_Print(1, "%s =", (a)), Abc_Print(1, "%10.3f MB\r",    1.0*((double)(f))/(1<<20)))
#define ABC_PRMn(a,f)   (Abc_Print(1, "%s =", (a)), Abc_Print(1, "%10.3f MB  ",    1.0*((double)(f))/(1<<20)))
#define ABC_PRMP(a,f,F) (Abc_Print(1, "%s =", (a)), Abc_Print(1, "%10.3f MB (%6.2f %%)\n",  (1.0*((double)(f))/(1<<20)), (((double)(F))? 100.0*((double)(f))/((double)(F)) : 0.0) ) )

#define ABC_ALLOC(type, num)     ((type *) malloc(sizeof(type) * (size_t)(num)))
#define ABC_CALLOC(type, num)    ((type *) calloc((size_t)(num), sizeof(type)))
#define ABC_FALLOC(type, num)    ((type *) memset(malloc(sizeof(type) * (size_t)(num)), 0xff, sizeof(type) * (size_t)(num)))
#define ABC_FREE(obj)            ((obj) ? (free((char *) (obj)), (obj) = 0) : 0)
#define ABC_REALLOC(type, obj, num) \
        ((obj) ? ((type *) realloc((char *)(obj), sizeof(type) * (size_t)(num))) : \
         ((type *) malloc(sizeof(type) * (size_t)(num))))

static inline int      Abc_AbsInt( int a        )             { return a < 0 ? -a : a; }
static inline int      Abc_MaxInt( int a, int b )             { return a > b ?  a : b; }
static inline int      Abc_MinInt( int a, int b )             { return a < b ?  a : b; }
static inline word     Abc_MaxWord( word a, word b )          { return a > b ?  a : b; }
static inline word     Abc_MinWord( word a, word b )          { return a < b ?  a : b; }
static inline float    Abc_AbsFloat( float a          )       { return a < 0 ? -a : a; }
static inline float    Abc_MaxFloat( float a, float b )       { return a > b ?  a : b; }
static inline float    Abc_MinFloat( float a, float b )       { return a < b ?  a : b; }
static inline double   Abc_AbsDouble( double a           )    { return a < 0 ? -a : a; }
static inline double   Abc_MaxDouble( double a, double b )    { return a > b ?  a : b; }
static inline double   Abc_MinDouble( double a, double b )    { return a < b ?  a : b; }

static inline int      Abc_Float2Int( float Val )             { union { int x; float y; } v; v.y = Val; return v.x;         }
static inline float    Abc_Int2Float( int Num )               { union { int x; float y; } v; v.x = Num; return v.y;         }
static inline word     Abc_Dbl2Word( double Dbl )             { union { word x; double y; } v; v.y = Dbl; return v.x;       }
static inline double   Abc_Word2Dbl( word Num )               { union { word x; double y; } v; v.x = Num; return v.y;       }
static inline int      Abc_Base2Log( unsigned n )             { int r; if ( n < 2 ) return (int)n; for ( r = 0, n--; n; n >>= 1, r++ ) {}; return r; }
static inline int      Abc_Base10Log( unsigned n )            { int r; if ( n < 2 ) return (int)n; for ( r = 0, n--; n; n /= 10, r++ ) {}; return r; }
static inline int      Abc_Base16Log( unsigned n )            { int r; if ( n < 2 ) return (int)n; for ( r = 0, n--; n; n /= 16, r++ ) {}; return r; }
static inline char *   Abc_UtilStrsav( char * s )             { return s ? strcpy(ABC_ALLOC(char, strlen(s)+1), s) : NULL;  }
static inline char *   Abc_UtilStrsavTwo( char * s, char * a ){ char * r; if (!a) return Abc_UtilStrsav(s); r = ABC_ALLOC(char, strlen(s)+strlen(a)+1); sprintf(r, "%s%s", s, a ); return r; }
static inline char *   Abc_UtilStrsavNum( char * s, int n )   { char * r; if (!s) return NULL;              r = ABC_ALLOC(char, strlen(s)+12+1);        sprintf(r, "%s%d", s, n ); return r; }
static inline int      Abc_BitByteNum( int nBits )            { return (nBits>>3) + ((nBits&7)  > 0);                       }
static inline int      Abc_BitWordNum( int nBits )            { return (nBits>>5) + ((nBits&31) > 0);                       }
static inline int      Abc_Bit6WordNum( int nBits )           { return (nBits>>6) + ((nBits&63) > 0);                       }
static inline int      Abc_TruthByteNum( int nVars )          { return nVars <= 3 ? 1 : (1 << (nVars - 3));                 }
static inline int      Abc_TruthWordNum( int nVars )          { return nVars <= 5 ? 1 : (1 << (nVars - 5));                 }
static inline int      Abc_Truth6WordNum( int nVars )         { return nVars <= 6 ? 1 : (1 << (nVars - 6));                 }
static inline int      Abc_InfoHasBit( unsigned * p, int i )  { return (p[(i)>>5] & (unsigned)(1<<((i) & 31))) > 0;         }
static inline void     Abc_InfoSetBit( unsigned * p, int i )  { p[(i)>>5] |= (unsigned)(1<<((i) & 31));                     }
static inline void     Abc_InfoXorBit( unsigned * p, int i )  { p[(i)>>5] ^= (unsigned)(1<<((i) & 31));                     }
static inline unsigned Abc_InfoMask( int nVar )               { return (~(unsigned)0) >> (32-nVar);                         }

static inline int      Abc_Var2Lit( int Var, int c )          { assert(Var >= 0 && !(c >> 1)); return Var + Var + c;        }
static inline int      Abc_Lit2Var( int Lit )                 { assert(Lit >= 0); return Lit >> 1;                          }
static inline int      Abc_LitIsCompl( int Lit )              { assert(Lit >= 0); return Lit & 1;                           }
static inline int      Abc_LitNot( int Lit )                  { assert(Lit >= 0); return Lit ^ 1;                           }
static inline int      Abc_LitNotCond( int Lit, int c )       { assert(Lit >= 0); return Lit ^ (int)(c > 0);                }
static inline int      Abc_LitRegular( int Lit )              { assert(Lit >= 0); return Lit & ~01;                         }
static inline int      Abc_Lit2LitV( int * pMap, int Lit )    { assert(Lit >= 0); return Abc_Var2Lit( pMap[Abc_Lit2Var(Lit)], Abc_LitIsCompl(Lit) );      }
static inline int      Abc_Lit2LitL( int * pMap, int Lit )    { assert(Lit >= 0); return Abc_LitNotCond( pMap[Abc_Lit2Var(Lit)], Abc_LitIsCompl(Lit) );   }

static inline int      Abc_Ptr2Int( void * p )                { return (int)(ABC_PTRINT_T)p;      }
static inline void *   Abc_Int2Ptr( int i )                   { return (void *)(ABC_PTRINT_T)i;   }
static inline word     Abc_Ptr2Wrd( void * p )                { return (word)(ABC_PTRUINT_T)p;    }
static inline void *   Abc_Wrd2Ptr( word i )                  { return (void *)(ABC_PTRUINT_T)i;  }

static inline int      Abc_Var2Lit2( int Var, int Att )       { assert(!(Att >> 2)); return (Var << 2) + Att; }
static inline int      Abc_Lit2Var2( int Lit )                { assert(Lit >= 0);    return Lit >> 2;         }
static inline int      Abc_Lit2Att2( int Lit )                { assert(Lit >= 0);    return Lit & 3;          }
static inline int      Abc_Var2Lit3( int Var, int Att )       { assert(!(Att >> 3)); return (Var << 3) + Att; }
static inline int      Abc_Lit2Var3( int Lit )                { assert(Lit >= 0);    return Lit >> 3;         }
static inline int      Abc_Lit2Att3( int Lit )                { assert(Lit >= 0);    return Lit & 7;          }
static inline int      Abc_Var2Lit4( int Var, int Att )       { assert(!(Att >> 4)); return (Var << 4) + Att; }
static inline int      Abc_Lit2Var4( int Lit )                { assert(Lit >= 0);    return Lit >> 4;         }
static inline int      Abc_Lit2Att4( int Lit )                { assert(Lit >= 0);    return Lit & 15;         }

typedef ABC_INT64_T abctime;
// counting wall time
static inline abctime Abc_Clock()
{
#if defined(__APPLE__) && defined(__MACH__)
  #define APPLE_MACH (__APPLE__ & __MACH__)
#else
  #define APPLE_MACH 0
#endif
#if (defined(LIN) || defined(LIN64)) && !APPLE_MACH && !defined(__MINGW32__)
    struct timespec ts;
    if ( clock_gettime(CLOCK_MONOTONIC, &ts) < 0 ) 
        return (abctime)-1;
    abctime res = ((abctime) ts.tv_sec) * CLOCKS_PER_SEC;
    res += (((abctime) ts.tv_nsec) * CLOCKS_PER_SEC) / 1000000000;
    return res;
#else
    return (abctime) clock();
#endif
}
// counting thread time
static inline abctime Abc_ThreadClock()
{
#if defined(__APPLE__) && defined(__MACH__)
  #define APPLE_MACH (__APPLE__ & __MACH__)
#else
  #define APPLE_MACH 0
#endif
#if (defined(LIN) || defined(LIN64)) && !APPLE_MACH && !defined(__MINGW32__)
    struct timespec ts;
    if ( clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) < 0 ) 
        return (abctime)-1;
    abctime res = ((abctime) ts.tv_sec) * CLOCKS_PER_SEC;
    res += (((abctime) ts.tv_nsec) * CLOCKS_PER_SEC) / 1000000000;
    return res;
#else
    return (abctime) clock();
#endif
}

// misc printing procedures
enum Abc_VerbLevel
{
    ABC_PROMPT   = -2,
    ABC_ERROR    = -1,
    ABC_WARNING  =  0,
    ABC_STANDARD =  1,
    ABC_VERBOSE  =  2
};


#ifdef ABC_USE_BRIDGE

// bridge communication
#define BRIDGE_NETLIST           106
#define BRIDGE_ABS_NETLIST       107
extern int Gia_ManToBridgeText( FILE * pFile, int Size, unsigned char * pBuffer );
extern int Gia_ManToBridgeAbsNetlist( FILE * pFile, void * p, int pkg_type );

// string printing
extern char * vnsprintf(const char* format, va_list args);
extern char * nsprintf(const char* format, ...);

static inline void Abc_Print( int level, const char * format, ... )
{
    extern ABC_DLL int Abc_FrameIsBridgeMode();
    va_list args;
    extern unsigned enable_dbg_outs;  
    if ( !enable_dbg_outs )
        return;

    if ( ! Abc_FrameIsBridgeMode() ){
        if ( level == ABC_ERROR )
            printf( "Error: " );
        else if ( level == ABC_WARNING )
            printf( "Warning: " );
    }else{
        if ( level == ABC_ERROR )
            Gia_ManToBridgeText( stdout, (int)strlen("Error: "), (unsigned char*)"Error: " );
        else if ( level == ABC_WARNING )
            Gia_ManToBridgeText( stdout, (int)strlen("Warning: "), (unsigned char*)"Warning: " );
    }

    va_start( args, format );
    if ( Abc_FrameIsBridgeMode() )
    {
        char * tmp = vnsprintf( format, args );
        Gia_ManToBridgeText( stdout, (int)strlen(tmp), (unsigned char*)tmp );
        free( tmp );
    }
    else
        vprintf( format, args );
    va_end( args );
}

#else

static inline void Abc_Print( int level, const char * format, ... )
{
    va_list args;
    va_start( args, format );
    if ( level == ABC_ERROR )
        printf( "Error: " );
    else if ( level == ABC_WARNING )
        printf( "Warning: " );
    vprintf( format, args );
    va_end( args );
}

#endif


static inline void Abc_PrintInt( int i )
{
    double v3 = (double)i/1000;
    double v6 = (double)i/1000000;

    Abc_Print( 1,  "  " );

    if ( i > -1000 && i < 1000 )
        Abc_Print( 1, " %4d", i );

    else if ( v3 > -9.995 && v3 < 9.995 )
        Abc_Print( 1, "%4.2fk", v3 );
    else if ( v3 > -99.95 && v3 < 99.95 )
        Abc_Print( 1, "%4.1fk", v3 );
    else if ( v3 > -999.5 && v3 < 999.5 )
        Abc_Print( 1, "%4.0fk", v3 );

    else if ( v6 > -9.995 && v6 < 9.995 )
        Abc_Print( 1, "%4.2fm", v6 );
    else if ( v6 > -99.95 && v6 < 99.95 )
        Abc_Print( 1, "%4.1fm", v6 );
    else if ( v6 > -999.5 && v6 < 999.5 )
        Abc_Print( 1, "%4.0fm", v6 );
}


static inline void Abc_PrintTime( int level, const char * pStr, abctime time )
{
    ABC_PRT( pStr, time );
}

static inline void Abc_PrintTimeP( int level, const char * pStr, abctime time, abctime Time )
{
    ABC_PRTP( pStr, time, Time );
}

static inline void Abc_PrintMemoryP( int level, const char * pStr, int mem, int Mem )
{
    ABC_PRMP( pStr, mem, Mem );
}

// Returns the next prime >= p
static inline int Abc_PrimeCudd( unsigned int p )
{
    int i,pn;
    p--;
    do {
        p++;
        if (p&1)
        {
            pn = 1;
            i = 3;
            while ((unsigned) (i * i) <= p)
            {
                if (p % (unsigned)i == 0) {
                    pn = 0;
                    break;
                }
                i += 2;
            }
        }
        else
            pn = 0;
    } while (!pn);
    return (int)(p);

} // end of Cudd_Prime 

// the returned buffer has 32 unused bytes at the end, filled with zeros
static inline void * Abc_FileReadContents( char * pFileName, int * pnFileSize )
{
    int RetValue, nFileSize;
    char * pBuffer;
    FILE * pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
        return NULL;
    // get the file size, in bytes
    fseek( pFile, 0, SEEK_END );
    nFileSize = ftell( pFile );
    if ( pnFileSize )
        *pnFileSize = nFileSize;
    rewind( pFile );
    // load the contents of the file into memory
    pBuffer = ABC_ALLOC( char, nFileSize + 32 );
    RetValue = fread( pBuffer, 1, nFileSize, pFile );
    memset( pBuffer + nFileSize, 0, 32 );
    assert( RetValue == nFileSize );
    fclose( pFile );
    return (void *)pBuffer;
}
static inline void Abc_ReverseOrder( int * pA, int nA )
{
    int i;
    for ( i = 0; i < nA/2; i++ )
        ABC_SWAP( int, pA[i], pA[nA-1-i] );
}


// sorting
extern void   Abc_MergeSort( int * pInput, int nSize );
extern int *  Abc_MergeSortCost( int * pCosts, int nSize );
extern void   Abc_MergeSortCost2( int * pInput, int nSize, int * pCost );
extern void   Abc_MergeSortCost2Reverse( int * pInput, int nSize, int * pCost );
extern void   Abc_QuickSort1( word * pData, int nSize, int fDecrease );
extern void   Abc_QuickSort2( word * pData, int nSize, int fDecrease );
extern void   Abc_QuickSort3( word * pData, int nSize, int fDecrease );
extern void   Abc_QuickSortCostData( int * pCosts, int nSize, int fDecrease, word * pData, int * pResult );
extern int *  Abc_QuickSortCost( int * pCosts, int nSize, int fDecrease );

extern unsigned Abc_Random( int fReset );
extern word     Abc_RandomW( int fReset );

ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
