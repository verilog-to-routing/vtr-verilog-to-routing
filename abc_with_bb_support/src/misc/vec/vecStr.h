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
 
#ifndef __VEC_STR_H__
#define __VEC_STR_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>

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
    p = ALLOC( Vec_Str_t, 1 );
    if ( nCap > 0 && nCap < 16 )
        nCap = 16;
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ALLOC( char, p->nCap ) : NULL;
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
    p = ALLOC( Vec_Str_t, 1 );
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
    p = ALLOC( Vec_Str_t, 1 );
    p->nSize  = nSize;
    p->nCap   = nSize;
    p->pArray = ALLOC( char, nSize );
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
    p = ALLOC( Vec_Str_t, 1 );
    p->nSize  = pVec->nSize;
    p->nCap   = pVec->nCap;
    p->pArray = p->nCap? ALLOC( char, p->nCap ) : NULL;
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
    p = ALLOC( Vec_Str_t, 1 );
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
static inline void Vec_StrFree( Vec_Str_t * p )
{
    FREE( p->pArray );
    FREE( p );
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
    p->pArray = REALLOC( char, p->pArray, 2 * nCapMin ); 
    p->nCap   = 2 * nCapMin;
}

/**Function*************************************************************

  Synopsis    [Fills the vector with given number of entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_StrFill( Vec_Str_t * p, int nSize, char Entry )
{
    int i;
    Vec_StrGrow( p, nSize );
    p->nSize = nSize;
    for ( i = 0; i < p->nSize; i++ )
        p->pArray[i] = Entry;
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

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_StrPrintNum( Vec_Str_t * p, int Num )
{
    int i, nDigits;
    if ( Num < 0 )
    {
        Vec_StrPush( p, '-' );
        Num = -Num;
    }
    if ( Num < 10 )
    {
        Vec_StrPush( p, (char)('0' + Num) );
        return;
    }
    nDigits = Extra_Base10Log( Num );
    Vec_StrGrow( p, p->nSize + nDigits );
    for ( i = nDigits - 1; i >= 0; i-- )
    {
        Vec_StrWriteEntry( p, p->nSize + i, (char)('0' + Num % 10) );
        Num /= 10;
    }
    assert( Num == 0 );
    p->nSize += nDigits;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_StrPrintStr( Vec_Str_t * p, char * pStr )
{
    int i, Length = strlen(pStr);
    for ( i = 0; i < Length; i++ )
        Vec_StrPush( p, pStr[i] );
}

/**Function*************************************************************

  Synopsis    [Appends the string to the char vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_StrAppend( Vec_Str_t * p, char * pString )
{
    int i, nLength = strlen(pString);
    Vec_StrGrow( p, p->nSize + nLength );
    for ( i = 0; i < nLength; i++ )
        p->pArray[p->nSize + i] = pString[i];
    p->nSize += nLength;
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

  Synopsis    [Comparison procedure for two clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_StrSortCompare1( char * pp1, char * pp2 )
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
static inline int Vec_StrSortCompare2( char * pp1, char * pp2 )
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

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

