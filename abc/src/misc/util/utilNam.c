/**CFile****************************************************************

  FileName    [utilNam.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Manager for character strings.]

  Synopsis    [Manager for character strings.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: utilNam.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "abc_global.h"
#include "misc/vec/vec.h"
#include "utilNam.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// this package maps non-empty character strings into natural numbers and back

// name manager
struct Abc_Nam_t_
{
    // info storage for names
    int              nStore;       // the size of allocated storage
    int              iHandle;      // the current free handle
    char *           pStore;       // storage for name objects
    // internal number mappings
    Vec_Int_t        vInt2Handle;  // mapping integers into handles
    Vec_Int_t        vInt2Next;    // mapping integers into nexts
    // hash table for names
    int *            pBins;        // the hash table bins 
    int              nBins;        // the number of bins 
    // manager recycling
    int              nRefs;        // reference counter for the manager
    // internal buffer
    Vec_Str_t        vBuffer;      
};

static inline char * Abc_NamHandleToStr( Abc_Nam_t * p, int h )        { return (char *)(p->pStore + h);                         }
static inline int    Abc_NamIntToHandle( Abc_Nam_t * p, int i )        { return Vec_IntEntry(&p->vInt2Handle, i);                }
static inline char * Abc_NamIntToStr( Abc_Nam_t * p, int i )           { return Abc_NamHandleToStr(p, Abc_NamIntToHandle(p,i));  }
static inline int    Abc_NamIntToNext( Abc_Nam_t * p, int i )          { return Vec_IntEntry(&p->vInt2Next, i);                  }
static inline int *  Abc_NamIntToNextP( Abc_Nam_t * p, int i )         { return Vec_IntEntryP(&p->vInt2Next, i);                 }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Nam_t * Abc_NamStart( int nObjs, int nAveSize )
{
    Abc_Nam_t * p;
    if ( nObjs == 0 )
        nObjs = 16;
    p = ABC_CALLOC( Abc_Nam_t, 1 );
    p->nStore      = ((nObjs * (nAveSize + 1) + 16) / 4) * 4;
    p->pStore      = ABC_ALLOC( char, p->nStore );
    p->nBins       = Abc_PrimeCudd( nObjs );
    p->pBins       = ABC_CALLOC( int, p->nBins );
    // 0th object is unused
    Vec_IntGrow( &p->vInt2Handle, nObjs );  Vec_IntPush( &p->vInt2Handle, -1 );
    Vec_IntGrow( &p->vInt2Next,   nObjs );  Vec_IntPush( &p->vInt2Next,   -1 );
    p->iHandle     = 4;
    memset( p->pStore, 0, 4 );
//Abc_Print( 1, "Starting nam with %d bins.\n", p->nBins );
    // start reference counting
    p->nRefs       = 1;
    return p;
}

/**Function*************************************************************

  Synopsis    [Deletes manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NamStop( Abc_Nam_t * p )
{
//Abc_Print( 1, "Starting nam with %d bins.\n", p->nBins );
    Vec_StrErase( &p->vBuffer );
    Vec_IntErase( &p->vInt2Handle );
    Vec_IntErase( &p->vInt2Next );
    ABC_FREE( p->pStore );
    ABC_FREE( p->pBins );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Prints manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NamPrint( Abc_Nam_t * p, char * pFileName )
{
    FILE * pFile = pFileName ? fopen( pFileName, "wb" ) : stdout;
    int h, i;
    if ( pFile == NULL ) { printf( "Count node open file %s\n", pFileName ); return; }
    Vec_IntForEachEntryStart( &p->vInt2Handle, h, i, 1 )
        fprintf( pFile, "%8d = %s\n", i, Abc_NamHandleToStr(p, h) );
    if ( pFile != stdout )
        fclose(pFile);
}

/**Function*************************************************************

  Synopsis    [References the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Nam_t * Abc_NamRef( Abc_Nam_t * p )
{
    p->nRefs++;
    return p;
}

/**Function*************************************************************

  Synopsis    [Dereferences the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NamDeref( Abc_Nam_t * p )
{
    if ( p == NULL )
        return;
    if ( --p->nRefs == 0 )
        Abc_NamStop( p );
}

/**Function*************************************************************

  Synopsis    [Returns the number of used entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NamObjNumMax( Abc_Nam_t * p )
{
    return Vec_IntSize(&p->vInt2Handle);
}

/**Function*************************************************************

  Synopsis    [Reports memory usage of the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NamMemUsed( Abc_Nam_t * p )
{
    if ( p == NULL )
        return 0;
    return sizeof(Abc_Nam_t) + p->iHandle + sizeof(int) * p->nBins + 
        sizeof(int) * (p->vInt2Handle.nSize + p->vInt2Next.nSize);
}

/**Function*************************************************************

  Synopsis    [Reports memory usage of the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NamMemAlloc( Abc_Nam_t * p )
{
    if ( p == NULL )
        return 0;
    return sizeof(Abc_Nam_t) + p->nStore + sizeof(int) * p->nBins + 
        sizeof(int) * (p->vInt2Handle.nCap + p->vInt2Next.nCap);
}

/**Function*************************************************************

  Synopsis    [Computes hash value of the C-string.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NamStrHash( const char * pStr, const char * pLim, int nTableSize )
{
    static int s_FPrimes[128] = { 
        1009, 1049, 1093, 1151, 1201, 1249, 1297, 1361, 1427, 1459, 
        1499, 1559, 1607, 1657, 1709, 1759, 1823, 1877, 1933, 1997, 
        2039, 2089, 2141, 2213, 2269, 2311, 2371, 2411, 2467, 2543, 
        2609, 2663, 2699, 2741, 2797, 2851, 2909, 2969, 3037, 3089, 
        3169, 3221, 3299, 3331, 3389, 3461, 3517, 3557, 3613, 3671, 
        3719, 3779, 3847, 3907, 3943, 4013, 4073, 4129, 4201, 4243, 
        4289, 4363, 4441, 4493, 4549, 4621, 4663, 4729, 4793, 4871, 
        4933, 4973, 5021, 5087, 5153, 5227, 5281, 5351, 5417, 5471, 
        5519, 5573, 5651, 5693, 5749, 5821, 5861, 5923, 6011, 6073, 
        6131, 6199, 6257, 6301, 6353, 6397, 6481, 6563, 6619, 6689, 
        6737, 6803, 6863, 6917, 6977, 7027, 7109, 7187, 7237, 7309, 
        7393, 7477, 7523, 7561, 7607, 7681, 7727, 7817, 7877, 7933, 
        8011, 8039, 8059, 8081, 8093, 8111, 8123, 8147
    };
    unsigned i, uHash;
    if ( pLim )
    {
        for ( uHash = 0, i = 0; pStr+i < pLim; i++ )
            if ( i & 1 ) 
                uHash *= pStr[i] * s_FPrimes[i & 0x7F];
            else
                uHash ^= pStr[i] * s_FPrimes[i & 0x7F];
    }
    else
    {
        for ( uHash = 0, i = 0; pStr[i]; i++ )
            if ( i & 1 ) 
                uHash *= pStr[i] * s_FPrimes[i & 0x7F];
            else
                uHash ^= pStr[i] * s_FPrimes[i & 0x7F];
    }
    return uHash % nTableSize;
}
// https://en.wikipedia.org/wiki/Jenkins_hash_function
int Abc_NamStrHash2( const char * pStr, const char * pLim, int nTableSize ) 
{
    int nSize = pLim ? pLim - pStr : -1;
    int i = 0; unsigned hash = 0;
    while ( i != nSize && pStr[i] ) 
    {
        hash += pStr[i++];
        hash += hash << 10;
        hash ^= hash >> 6;
    }
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;
    return (int)(hash % nTableSize);
}

/**Function*************************************************************

  Synopsis    [Returns place where this string is, or should be.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_NamStrcmp( char * pStr, char * pLim, char * pThis )
{
    if ( pLim )
    {
        while ( pStr < pLim )
            if ( *pStr++ != *pThis++ )
                return 1;
        return *pThis != '\0';
    }
    else
    {
        while ( *pStr )
            if ( *pStr++ != *pThis++ )
                return 1;
        return *pThis != '\0';
    }
}
static inline int * Abc_NamStrHashFind( Abc_Nam_t * p, const char * pStr, const char * pLim )
{
    char * pThis;
    int * pPlace = (int *)(p->pBins + Abc_NamStrHash( pStr, pLim, p->nBins ));
    assert( *pStr );
    for ( pThis = (*pPlace)? Abc_NamIntToStr(p, *pPlace) : NULL; 
          pThis;    pPlace = Abc_NamIntToNextP(p, *pPlace), 
          pThis = (*pPlace)? Abc_NamIntToStr(p, *pPlace) : NULL )
              if ( !Abc_NamStrcmp( (char *)pStr, (char *)pLim, pThis ) )
                  break;
    return pPlace;
}

/**Function*************************************************************

  Synopsis    [Resizes the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NamStrHashResize( Abc_Nam_t * p )
{
    Vec_Int_t vInt2HandleOld;  char * pThis; 
    int * piPlace, * pBinsOld, iHandleOld, i;//, clk = Abc_Clock();
    assert( p->pBins != NULL );
//    Abc_Print( 1, "Resizing names manager hash table from %6d to %6d. ", p->nBins, Abc_PrimeCudd( 3 * p->nBins ) );
    // replace the table
    pBinsOld = p->pBins;
    p->nBins = Abc_PrimeCudd( 3 * p->nBins ); 
    p->pBins = ABC_CALLOC( int, p->nBins );
    // replace the handles array
    vInt2HandleOld = p->vInt2Handle;
    Vec_IntZero( &p->vInt2Handle );
    Vec_IntGrow( &p->vInt2Handle, 2 * Vec_IntSize(&vInt2HandleOld) ); 
    Vec_IntPush( &p->vInt2Handle, -1 );
    Vec_IntClear( &p->vInt2Next );    Vec_IntPush( &p->vInt2Next, -1 );
    // rehash the entries from the old table
    Vec_IntForEachEntryStart( &vInt2HandleOld, iHandleOld, i, 1 )
    {
        pThis   = Abc_NamHandleToStr( p, iHandleOld );
        piPlace = Abc_NamStrHashFind( p, pThis, NULL );
        assert( *piPlace == 0 );
        *piPlace = Vec_IntSize( &p->vInt2Handle );
        assert( Vec_IntSize( &p->vInt2Handle ) == i );
        Vec_IntPush( &p->vInt2Handle, iHandleOld );
        Vec_IntPush( &p->vInt2Next,   0 );
    }
    Vec_IntErase( &vInt2HandleOld );
    ABC_FREE( pBinsOld );
//    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
}

/**Function*************************************************************

  Synopsis    [Returns the index of the string in the table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NamStrFind( Abc_Nam_t * p, char * pStr )
{
    return *Abc_NamStrHashFind( p, pStr, NULL );
}
int Abc_NamStrFindLim( Abc_Nam_t * p, char * pStr, char * pLim )
{
    return *Abc_NamStrHashFind( p, pStr, pLim );
}

/**Function*************************************************************

  Synopsis    [Finds or adds the given name to storage.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NamStrFindOrAdd( Abc_Nam_t * p, char * pStr, int * pfFound )
{
    int i, iHandleNew;
    int *piPlace;
    if ( !(pStr[0] != '\\' || pStr[strlen(pStr)-1] == ' ') )
    {
        for ( i = strlen(pStr) - 1; i >= 0; i-- )
            if ( *pStr == ' ' )
                break;
        assert( i < (int)strlen(pStr) );
    }
    piPlace = Abc_NamStrHashFind( p, pStr, NULL );
    if ( *piPlace )
    {
        if ( pfFound )
            *pfFound = 1;
        return *piPlace;
    }
    if ( pfFound )
        *pfFound = 0;
    iHandleNew = p->iHandle + strlen(pStr) + 1;
    while ( p->nStore < iHandleNew )
    {
        p->nStore *= 3;
        p->nStore /= 2;
        p->pStore  = ABC_REALLOC( char, p->pStore, p->nStore );
    }
    assert( p->nStore >= iHandleNew );
    // create new handle
    *piPlace = Vec_IntSize( &p->vInt2Handle );
    strcpy( Abc_NamHandleToStr( p, p->iHandle ), pStr );
    Vec_IntPush( &p->vInt2Handle, p->iHandle );
    Vec_IntPush( &p->vInt2Next, 0 );
    p->iHandle = iHandleNew;
    // extend the hash table
    if ( Vec_IntSize(&p->vInt2Handle) > 2 * p->nBins )
        Abc_NamStrHashResize( p );
    return Vec_IntSize(&p->vInt2Handle) - 1;
}
int Abc_NamStrFindOrAddLim( Abc_Nam_t * p, char * pStr, char * pLim, int * pfFound )
{
    int iHandleNew;
    int *piPlace;
    char * pStore;
    assert( pStr < pLim );
    piPlace = Abc_NamStrHashFind( p, pStr, pLim );
    if ( *piPlace )
    {
        if ( pfFound )
            *pfFound = 1;
        return *piPlace;
    }
    if ( pfFound )
        *pfFound = 0;
    iHandleNew = p->iHandle + (pLim - pStr) + 1;
    while ( p->nStore < iHandleNew )
    {
        p->nStore *= 3;
        p->nStore /= 2;
        p->pStore  = ABC_REALLOC( char, p->pStore, p->nStore );
    }
    assert( p->nStore >= iHandleNew );
    // create new handle
    *piPlace = Vec_IntSize( &p->vInt2Handle );
    pStore = Abc_NamHandleToStr( p, p->iHandle );
    strncpy( pStore, pStr, pLim - pStr );
    pStore[pLim - pStr] = 0;
    Vec_IntPush( &p->vInt2Handle, p->iHandle );
    Vec_IntPush( &p->vInt2Next, 0 );
    p->iHandle = iHandleNew;
    // extend the hash table
    if ( Vec_IntSize(&p->vInt2Handle) > 2 * p->nBins )
        Abc_NamStrHashResize( p );
    return Vec_IntSize(&p->vInt2Handle) - 1;
}
int Abc_NamStrFindOrAddF( Abc_Nam_t * p, const char * format, ...  )
{
    int nAdded, nSize = 1000; 
    va_list args;  va_start( args, format );
    Vec_StrGrow( &p->vBuffer, Vec_StrSize(&p->vBuffer) + nSize );
    nAdded = vsnprintf( Vec_StrLimit(&p->vBuffer), nSize, format, args );
    if ( nAdded > nSize )
    {
        Vec_StrGrow( &p->vBuffer, Vec_StrSize(&p->vBuffer) + nAdded + nSize );
        nSize = vsnprintf( Vec_StrLimit(&p->vBuffer), nAdded, format, args );
        assert( nSize == nAdded );
    }
    va_end( args );
    return Abc_NamStrFindOrAddLim( p, Vec_StrLimit(&p->vBuffer), Vec_StrLimit(&p->vBuffer) + nAdded, NULL );
}

/**Function*************************************************************

  Synopsis    [Returns name from name ID.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_NamStr( Abc_Nam_t * p, int NameId )
{
    return NameId > 0 ? Abc_NamIntToStr(p, NameId) : NULL;
}

/**Function*************************************************************

  Synopsis    [Returns internal buffer.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Str_t * Abc_NamBuffer( Abc_Nam_t * p )
{
    Vec_StrClear(&p->vBuffer);
    return &p->vBuffer;
}

/**Function*************************************************************

  Synopsis    [For each ID of the first manager, gives ID of the second one.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Abc_NamComputeIdMap( Abc_Nam_t * p1, Abc_Nam_t * p2 )
{
    Vec_Int_t * vMap;
    char * pThis;
    int * piPlace, iHandle1, i;
    if ( p1 == p2 )
        return Vec_IntStartNatural( Abc_NamObjNumMax(p1) );
    vMap = Vec_IntStart( Abc_NamObjNumMax(p1) );
    Vec_IntForEachEntryStart( &p1->vInt2Handle, iHandle1, i, 1 )
    {
        pThis = Abc_NamHandleToStr( p1, iHandle1 );
        piPlace = Abc_NamStrHashFind( p2, pThis, NULL );
        Vec_IntWriteEntry( vMap, i, *piPlace );
//        Abc_Print( 1, "%d->%d  ", i, *piPlace );
    }
    return vMap;
}

/**Function*************************************************************

  Synopsis    [Returns the number of common names in the array.]

  Description [The array contains name IDs in the first manager.
  The procedure returns the number of entries that correspond to names
  in the first manager that appear in the second manager.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NamReportCommon( Vec_Int_t * vNameIds1, Abc_Nam_t * p1, Abc_Nam_t * p2 )
{
    int i, Entry, Counter = 0;
    Vec_IntForEachEntry( vNameIds1, Entry, i )
    {
        assert( Entry > 0 && Entry < Abc_NamObjNumMax(p1) );
        Counter += (Abc_NamStrFind(p2, Abc_NamStr(p1, Entry)) > 0);
//        if ( Abc_NamStrFind(p2, Abc_NamStr(p1, Entry)) == 0 )
//            Abc_Print( 1, "unique name <%s>\n", Abc_NamStr(p1, Entry) );
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Returns the name that appears in p1 does not appear in p2.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_NamReportUnique( Vec_Int_t * vNameIds1, Abc_Nam_t * p1, Abc_Nam_t * p2 )
{
    int i, Entry;
    Vec_IntForEachEntry( vNameIds1, Entry, i )
    {
        assert( Entry > 0 && Entry < Abc_NamObjNumMax(p1) );
        if ( Abc_NamStrFind(p2, Abc_NamStr(p1, Entry)) == 0 )
            return Abc_NamStr(p1, Entry);
    }
    return NULL;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

