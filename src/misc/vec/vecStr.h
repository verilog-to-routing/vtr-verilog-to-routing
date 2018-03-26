/**CFile****************************************************************

  FileName    [vecStr.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Resizable arrays.]

  Synopsis    [Resizable arrays of characters.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: vecStr.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__misc__vec__vecStr_h
#define ABC__misc__vec__vecStr_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>

ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Vec_Str_t_       Vec_Str_t;
struct Vec_Str_t_ 
{
    int              nCap;
    int              nSize;
    char *           pArray;
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

#define Vec_StrForEachEntry( vVec, Entry, i )                                               \
    for ( i = 0; (i < Vec_StrSize(vVec)) && (((Entry) = Vec_StrEntry(vVec, i)), 1); i++ )   

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Str_t * Vec_StrAlloc( int nCap )
{
    Vec_Str_t * p;
    p = ABC_ALLOC( Vec_Str_t, 1 );
    if ( nCap > 0 && nCap < 16 )
        nCap = 16;
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ABC_ALLOC( char, p->nCap ) : NULL;
    return p;
}
static inline Vec_Str_t * Vec_StrAllocExact( int nCap )
{
    Vec_Str_t * p;
    assert( nCap >= 0 );
    p = ABC_ALLOC( Vec_Str_t, 1 );
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ABC_ALLOC( char, p->nCap ) : NULL;
    return p;
}

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given size and cleans it.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Str_t * Vec_StrStart( int nSize )
{
    Vec_Str_t * p;
    p = Vec_StrAlloc( nSize );
    p->nSize = nSize;
    memset( p->pArray, 0, sizeof(char) * nSize );
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates the vector from an integer array of the given size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Str_t * Vec_StrAllocArray( char * pArray, int nSize )
{
    Vec_Str_t * p;
    p = ABC_ALLOC( Vec_Str_t, 1 );
    p->nSize  = nSize;
    p->nCap   = nSize;
    p->pArray = pArray;
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates the vector from an integer array of the given size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Str_t * Vec_StrAllocArrayCopy( char * pArray, int nSize )
{
    Vec_Str_t * p;
    p = ABC_ALLOC( Vec_Str_t, 1 );
    p->nSize  = nSize;
    p->nCap   = nSize;
    p->pArray = ABC_ALLOC( char, nSize );
    memcpy( p->pArray, pArray, sizeof(char) * nSize );
    return p;
}

/**Function*************************************************************

  Synopsis    [Duplicates the integer array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Str_t * Vec_StrDup( Vec_Str_t * pVec )
{
    Vec_Str_t * p;
    p = ABC_ALLOC( Vec_Str_t, 1 );
    p->nSize  = pVec->nSize;
    p->nCap   = pVec->nCap;
    p->pArray = p->nCap? ABC_ALLOC( char, p->nCap ) : NULL;
    memcpy( p->pArray, pVec->pArray, sizeof(char) * pVec->nSize );
    return p;
}

/**Function*************************************************************

  Synopsis    [Transfers the array into another vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Str_t * Vec_StrDupArray( Vec_Str_t * pVec )
{
    Vec_Str_t * p;
    p = ABC_ALLOC( Vec_Str_t, 1 );
    p->nSize  = pVec->nSize;
    p->nCap   = pVec->nCap;
    p->pArray = pVec->pArray;
    pVec->nSize  = 0;
    pVec->nCap   = 0;
    pVec->pArray = NULL;
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_StrZero( Vec_Str_t * p )
{
    p->pArray = NULL;
    p->nSize = 0;
    p->nCap = 0;
}
static inline void Vec_StrErase( Vec_Str_t * p )
{
    ABC_FREE( p->pArray );
    p->nSize = 0;
    p->nCap = 0;
}
static inline void Vec_StrFree( Vec_Str_t * p )
{
    ABC_FREE( p->pArray );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_StrFreeP( Vec_Str_t ** p )
{
    if ( *p == NULL )
        return;
    ABC_FREE( (*p)->pArray );
    ABC_FREE( (*p) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline char * Vec_StrReleaseArray( Vec_Str_t * p )
{
    char * pArray = p->pArray;
    p->nCap = 0;
    p->nSize = 0;
    p->pArray = NULL;
    return pArray;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline char * Vec_StrArray( Vec_Str_t * p )
{
    return p->pArray;
}
static inline char * Vec_StrLimit( Vec_Str_t * p )
{
    return p->pArray + p->nSize;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_StrSize( Vec_Str_t * p )
{
    return p->nSize;
}
static inline void Vec_StrSetSize( Vec_Str_t * p, int nSize )
{
    p->nSize = nSize;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_StrCap( Vec_Str_t * p )
{
    return p->nCap;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline double Vec_StrMemory( Vec_Str_t * p )
{
    return !p ? 0.0 : 1.0 * sizeof(char) * p->nCap + sizeof(Vec_Str_t);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline char Vec_StrEntry( Vec_Str_t * p, int i )
{
    assert( i >= 0 && i < p->nSize );
    return p->pArray[i];
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline char * Vec_StrEntryP( Vec_Str_t * p, int i )
{
    assert( i >= 0 && i < p->nSize );
    return p->pArray + i;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_StrWriteEntry( Vec_Str_t * p, int i, char Entry )
{
    assert( i >= 0 && i < p->nSize );
    p->pArray[i] = Entry;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline char Vec_StrEntryLast( Vec_Str_t * p )
{
    assert( p->nSize > 0 );
    return p->pArray[p->nSize-1];
}

/**Function*************************************************************

  Synopsis    [Resizes the vector to the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_StrGrow( Vec_Str_t * p, int nCapMin )
{
    if ( p->nCap >= nCapMin )
        return;
    p->pArray = ABC_REALLOC( char, p->pArray, nCapMin ); 
    p->nCap   = nCapMin;
}

/**Function*************************************************************

  Synopsis    [Fills the vector with given number of entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_StrFill( Vec_Str_t * p, int nSize, char Fill )
{
    int i;
    Vec_StrGrow( p, nSize );
    p->nSize = nSize;
    for ( i = 0; i < p->nSize; i++ )
        p->pArray[i] = Fill;
}

/**Function*************************************************************

  Synopsis    [Fills the vector with given number of entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_StrFillExtra( Vec_Str_t * p, int nSize, char Fill )
{
    int i;
    if ( nSize <= p->nSize )
        return;
    if ( nSize > 2 * p->nCap )
        Vec_StrGrow( p, nSize );
    else if ( nSize > p->nCap )
        Vec_StrGrow( p, 2 * p->nCap );
    for ( i = p->nSize; i < nSize; i++ )
        p->pArray[i] = Fill;
    p->nSize = nSize;
}

/**Function*************************************************************

  Synopsis    [Returns the entry even if the place not exist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline char Vec_StrGetEntry( Vec_Str_t * p, int i )
{
    Vec_StrFillExtra( p, i + 1, 0 );
    return Vec_StrEntry( p, i );
}

/**Function*************************************************************

  Synopsis    [Inserts the entry even if the place does not exist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_StrSetEntry( Vec_Str_t * p, int i, char Entry )
{
    Vec_StrFillExtra( p, i + 1, 0 );
    Vec_StrWriteEntry( p, i, Entry );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_StrShrink( Vec_Str_t * p, int nSizeNew )
{
    assert( p->nSize >= nSizeNew );
    p->nSize = nSizeNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_StrClear( Vec_Str_t * p )
{
    p->nSize = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_StrPush( Vec_Str_t * p, char Entry )
{
    if ( p->nSize == p->nCap )
    {
        if ( p->nCap < 16 )
            Vec_StrGrow( p, 16 );
        else
            Vec_StrGrow( p, 2 * p->nCap );
    }
    p->pArray[p->nSize++] = Entry;
}
static inline void Vec_StrPushBuffer( Vec_Str_t * p, char * pBuffer, int nSize )
{
    if ( p->nSize + nSize > p->nCap )
        Vec_StrGrow( p, 2 * (p->nSize + nSize) );
    memcpy( p->pArray + p->nSize, pBuffer, nSize );
    p->nSize += nSize;
}

/**Function*************************************************************

  Synopsis    [Returns the last entry and removes it from the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline char Vec_StrPop( Vec_Str_t * p )
{
    assert( p->nSize > 0 );
    return p->pArray[--p->nSize];
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_StrIntPrint( Vec_Str_t * p )
{
    int i;
    printf( "Vector has %d entries: {", Vec_StrSize(p) );
    for ( i = 0; i < Vec_StrSize(p); i++ )
        printf( " %d", (int)Vec_StrEntry(p, i) );
    printf( " }\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_StrPrintNum( Vec_Str_t * p, int Num )
{
    int i;
    char Digits[16];
    if ( Num == 0 )
    {
        Vec_StrPush( p, '0' );
        return;
    }
    if ( Num < 0 )
    {
        Vec_StrPush( p, '-' );
        Num = -Num;
    }
    for ( i = 0; Num; Num /= 10,  i++ )
        Digits[i] = Num % 10;
    for ( i--; i >= 0; i-- )
        Vec_StrPush( p, (char)('0' + Digits[i]) );
}
static inline void Vec_StrPrintNumStar( Vec_Str_t * p, int Num, int nDigits )
{
    int i;
    char Digits[16] = {0};
    if ( Num == 0 )
    {
        for ( i = 0; i < nDigits; i++ )
            Vec_StrPush( p, '0' );
        return;
    }
    if ( Num < 0 )
    {
        Vec_StrPush( p, '-' );
        Num = -Num;
        nDigits--;
    }
    for ( i = 0; Num; Num /= 10,  i++ )
        Digits[i] = Num % 10;
    for ( i = Abc_MaxInt(i, nDigits)-1; i >= 0; i-- )
        Vec_StrPush( p, (char)('0' + Digits[i]) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_StrPrintStr( Vec_Str_t * p, const char * pStr )
{
    int i, Length = (int)strlen(pStr);
    for ( i = 0; i < Length; i++ )
        Vec_StrPush( p, pStr[i] );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
#ifdef WIN32
#define vsnprintf _vsnprintf
#endif

static inline char * Vec_StrPrintF( Vec_Str_t * p, const char * format, ... )
{
    int nAdded, nSize = 1000; 
    va_list args;  va_start( args, format );
    Vec_StrGrow( p, Vec_StrSize(p) + nSize );
    nAdded = vsnprintf( Vec_StrLimit(p), nSize, format, args );
    if ( nAdded > nSize )
    {
        Vec_StrGrow( p, Vec_StrSize(p) + nAdded + nSize );
        nSize = vsnprintf( Vec_StrLimit(p), nAdded, format, args );
        assert( nSize == nAdded );
    }
    p->nSize += nAdded;
    va_end( args );
    return Vec_StrLimit(p) - nAdded;
}

/**Function*************************************************************

  Synopsis    [Appends the string to the char vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_StrAppend( Vec_Str_t * p, const char * pString )
{
    Vec_StrPrintStr( p, pString );
}
static inline void Vec_StrCopy( Vec_Str_t * p, const char * pString )
{
    Vec_StrClear( p );
    Vec_StrAppend( p, pString );
    Vec_StrPush( p, '\0' );
}

/**Function*************************************************************

  Synopsis    [Reverses the order of entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_StrReverseOrder( Vec_Str_t * p )
{
    int i, Temp;
    for ( i = 0; i < p->nSize/2; i++ )
    {
        Temp = p->pArray[i];
        p->pArray[i] = p->pArray[p->nSize-1-i];
        p->pArray[p->nSize-1-i] = Temp;
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_StrSum( Vec_Str_t * p ) 
{
    int i, Counter = 0;
    for ( i = 0; i < p->nSize; i++ )
        Counter += (int)p->pArray[i];
    return Counter;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_StrCountEntry( Vec_Str_t * p, char Entry ) 
{
    int i, Counter = 0;
    for ( i = 0; i < p->nSize; i++ )
        Counter += (p->pArray[i] == Entry);
    return Counter;
}
static inline int Vec_StrCountLarger( Vec_Str_t * p, char Entry ) 
{
    int i, Counter = 0;
    for ( i = 0; i < p->nSize; i++ )
        Counter += (p->pArray[i] > Entry);
    return Counter;
}
static inline int Vec_StrCountSmaller( Vec_Str_t * p, char Entry ) 
{
    int i, Counter = 0;
    for ( i = 0; i < p->nSize; i++ )
        Counter += (p->pArray[i] < Entry);
    return Counter;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_StrCountEntryLit( Vec_Str_t * p, char Entry ) 
{
    int i, Counter = 0;
    for ( i = 0; i < p->nSize; i++ )
        Counter += (Abc_Lit2Var((int)p->pArray[i]) == Entry);
    return Counter;
}
static inline int Vec_StrCountLargerLit( Vec_Str_t * p, char Entry ) 
{
    int i, Counter = 0;
    for ( i = 0; i < p->nSize; i++ )
        Counter += (Abc_Lit2Var((int)p->pArray[i]) > Entry);
    return Counter;
}
static inline int Vec_StrCountSmallerLit( Vec_Str_t * p, char Entry ) 
{
    int i, Counter = 0;
    for ( i = 0; i < p->nSize; i++ )
        Counter += (Abc_Lit2Var((int)p->pArray[i]) < Entry);
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Compares two strings.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_StrEqual( Vec_Str_t * p1, Vec_Str_t * p2 ) 
{
    int i;
    if ( p1->nSize != p2->nSize )
        return 0;
    for ( i = 0; i < p1->nSize; i++ )
        if ( p1->pArray[i] != p2->pArray[i] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Comparison procedure for two clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Vec_StrSortCompare1( char * pp1, char * pp2 )
{
    // for some reason commenting out lines (as shown) led to crashing of the release version
    if ( *pp1 < *pp2 )
        return -1;
    if ( *pp1 > *pp2 ) //
        return 1;
    return 0; //
}

/**Function*************************************************************

  Synopsis    [Comparison procedure for two clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Vec_StrSortCompare2( char * pp1, char * pp2 )
{
    // for some reason commenting out lines (as shown) led to crashing of the release version
    if ( *pp1 > *pp2 )
        return -1;
    if ( *pp1 < *pp2 ) //
        return 1;
    return 0; //
}

/**Function*************************************************************

  Synopsis    [Sorting the entries by their integer value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_StrSort( Vec_Str_t * p, int fReverse )
{
    if ( fReverse ) 
        qsort( (void *)p->pArray, p->nSize, sizeof(char), 
                (int (*)(const void *, const void *)) Vec_StrSortCompare2 );
    else
        qsort( (void *)p->pArray, p->nSize, sizeof(char), 
                (int (*)(const void *, const void *)) Vec_StrSortCompare1 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_StrCompareVec( Vec_Str_t * p1, Vec_Str_t * p2 )
{
    if ( p1 == NULL || p2 == NULL )
        return (p1 != NULL) - (p2 != NULL);
    if ( Vec_StrSize(p1) != Vec_StrSize(p2) )
        return Vec_StrSize(p1) - Vec_StrSize(p2);
    return memcmp( Vec_StrArray(p1), Vec_StrArray(p2), Vec_StrSize(p1) );
}


/**Function*************************************************************

  Synopsis    [Binary I/O for numbers (int/float/etc) and strings (char *).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_StrPutI_ne( Vec_Str_t * vOut, int Val )
{
    int i;
//    for ( i = 0; i < 4; i++ )
    for ( i = 3; i >= 0; i-- )
        Vec_StrPush( vOut, (char)(Val >> (8*i)) );
}
static inline int Vec_StrGetI_ne( Vec_Str_t * vOut, int * pPos )
{
    int i;
    int Val = 0;
//    for ( i = 0; i < 4; i++ )
    for ( i = 3; i >= 0; i-- )
        Val |= ((int)(unsigned char)Vec_StrEntry(vOut, (*pPos)++) << (8*i));
    return Val;
}

static inline void Vec_StrPutI( Vec_Str_t * vOut, int Val )
{
    for ( ; Val >= 0x80; Val >>= 7 )
        Vec_StrPush( vOut, (unsigned char)(Val | 0x80) );
    Vec_StrPush( vOut, (unsigned char)Val );
}
static inline int Vec_StrGetI( Vec_Str_t * vOut, int * pPos )
{
    unsigned char ch;
    int i = 0, Val = 0;
    while ( (ch = Vec_StrEntry(vOut, (*pPos)++)) & 0x80 )
        Val |= ((ch & 0x7f) << (7 * i++));
    return Val | (ch << (7 * i));
}

static inline void Vec_StrPutW( Vec_Str_t * vOut, word Val )
{
    int i;
    for ( i = 0; i < 8; i++ )
        Vec_StrPush( vOut, (char)(Val >> (8*i)) );
}
static inline word Vec_StrGetW( Vec_Str_t * vOut, int * pPos )
{
    int i;
    word Val = 0;
    for ( i = 0; i < 8; i++ )
        Val |= ((word)(unsigned char)Vec_StrEntry(vOut, (*pPos)++) << (8*i));
    return Val;
}

static inline void Vec_StrPutF( Vec_Str_t * vOut, float Val )
{
    union { float num; unsigned char data[4]; } tmp;
    tmp.num = Val;
    Vec_StrPush( vOut, tmp.data[0] );
    Vec_StrPush( vOut, tmp.data[1] );
    Vec_StrPush( vOut, tmp.data[2] );
    Vec_StrPush( vOut, tmp.data[3] );
}
static inline float Vec_StrGetF( Vec_Str_t * vOut, int * pPos )
{
    union { float num; unsigned char data[4]; } tmp;
    tmp.data[0] = Vec_StrEntry( vOut, (*pPos)++ );
    tmp.data[1] = Vec_StrEntry( vOut, (*pPos)++ );
    tmp.data[2] = Vec_StrEntry( vOut, (*pPos)++ );
    tmp.data[3] = Vec_StrEntry( vOut, (*pPos)++ );
    return tmp.num;
}

static inline void Vec_StrPutD( Vec_Str_t * vOut, double Val )
{
    union { double num; unsigned char data[8]; } tmp;
    int i, Lim = sizeof(double);
    tmp.num = Val;
    for ( i = 0; i < Lim; i++ )
        Vec_StrPush( vOut, tmp.data[i] );
}
static inline double Vec_StrGetD( Vec_Str_t * vOut, int * pPos )
{
    union { double num; unsigned char data[8]; } tmp;
    int i, Lim = sizeof(double);
    for ( i = 0; i < Lim; i++ )
        tmp.data[i] = Vec_StrEntry( vOut, (*pPos)++ );
    return tmp.num;
}

static inline void Vec_StrPutS( Vec_Str_t * vOut, char * pStr )
{
    while ( *pStr )
        Vec_StrPush( vOut, *pStr++ );
    Vec_StrPush( vOut, (char)0 );
}
static inline char * Vec_StrGetS( Vec_Str_t * vOut, int * pPos )
{
    char * pStr = Vec_StrEntryP( vOut, *pPos );
    while ( Vec_StrEntry(vOut, (*pPos)++) );
    return Abc_UtilStrsav(pStr);
}

static inline void Vec_StrPutC( Vec_Str_t * vOut, char c )
{
    Vec_StrPush( vOut, c );
}
static inline char Vec_StrGetC( Vec_Str_t * vOut, int * pPos )
{
    return Vec_StrEntry(vOut, (*pPos)++);
}



ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

