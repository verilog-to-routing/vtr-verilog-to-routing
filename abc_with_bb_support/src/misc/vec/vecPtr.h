/**CFile****************************************************************

  FileName    [vecPtr.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Resizable arrays.]

  Synopsis    [Resizable arrays of generic pointers.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: vecPtr.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef __VEC_PTR_H__
#define __VEC_PTR_H__

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

typedef struct Vec_Ptr_t_       Vec_Ptr_t;
struct Vec_Ptr_t_ 
{
    int              nCap;
    int              nSize;
    void **          pArray;
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

// iterators through entries
#define Vec_PtrForEachEntry( vVec, pEntry, i )                                               \
    for ( i = 0; (i < Vec_PtrSize(vVec)) && (((pEntry) = Vec_PtrEntry(vVec, i)), 1); i++ )
#define Vec_PtrForEachEntryStart( vVec, pEntry, i, Start )                                   \
    for ( i = Start; (i < Vec_PtrSize(vVec)) && (((pEntry) = Vec_PtrEntry(vVec, i)), 1); i++ )
#define Vec_PtrForEachEntryStop( vVec, pEntry, i, Stop )                                     \
    for ( i = 0; (i < Stop) && (((pEntry) = Vec_PtrEntry(vVec, i)), 1); i++ )
#define Vec_PtrForEachEntryStartStop( vVec, pEntry, i, Start, Stop )                         \
    for ( i = Start; (i < Stop) && (((pEntry) = Vec_PtrEntry(vVec, i)), 1); i++ )
#define Vec_PtrForEachEntryReverse( vVec, pEntry, i )                                        \
    for ( i = Vec_PtrSize(vVec) - 1; (i >= 0) && (((pEntry) = Vec_PtrEntry(vVec, i)), 1); i-- )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Ptr_t * Vec_PtrAlloc( int nCap )
{
    Vec_Ptr_t * p;
    p = ALLOC( Vec_Ptr_t, 1 );
    if ( nCap > 0 && nCap < 8 )
        nCap = 8;
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ALLOC( void *, p->nCap ) : NULL;
    return p;
}

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given size and cleans it.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Ptr_t * Vec_PtrStart( int nSize )
{
    Vec_Ptr_t * p;
    p = Vec_PtrAlloc( nSize );
    p->nSize = nSize;
    memset( p->pArray, 0, sizeof(void *) * nSize );
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates the vector from an integer array of the given size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Ptr_t * Vec_PtrAllocArray( void ** pArray, int nSize )
{
    Vec_Ptr_t * p;
    p = ALLOC( Vec_Ptr_t, 1 );
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
static inline Vec_Ptr_t * Vec_PtrAllocArrayCopy( void ** pArray, int nSize )
{
    Vec_Ptr_t * p;
    p = ALLOC( Vec_Ptr_t, 1 );
    p->nSize  = nSize;
    p->nCap   = nSize;
    p->pArray = ALLOC( void *, nSize );
    memcpy( p->pArray, pArray, sizeof(void *) * nSize );
    return p;
}

/**Function*************************************************************

  Synopsis    [Allocates the array of simulation info.]

  Description [Allocates the array containing given number of entries, 
  each of which contains given number of unsigned words of simulation data.
  The resulting array can be freed using regular procedure Vec_PtrFree().
  It is the responsibility of the user to ensure this array is never grown.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Ptr_t * Vec_PtrAllocSimInfo( int nEntries, int nWords )
{
    void ** pMemory;
    unsigned * pInfo;
    int i;
    pMemory = (void **)ALLOC( char, (sizeof(void *) + sizeof(unsigned) * nWords) * nEntries );
    pInfo = (unsigned *)(pMemory + nEntries);
    for ( i = 0; i < nEntries; i++ )
        pMemory[i] = pInfo + i * nWords;
    return Vec_PtrAllocArray( pMemory, nEntries );
}

/**Function*************************************************************

  Synopsis    [Allocates the array of truth tables for the given number of vars.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Ptr_t * Vec_PtrAllocTruthTables( int nVars )
{
    Vec_Ptr_t * p;
    unsigned Masks[5] = { 0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000 };
    unsigned * pTruth;
    int i, k, nWords;
    nWords = (nVars <= 5 ? 1 : (1 << (nVars - 5)));
    p = Vec_PtrAllocSimInfo( nVars, nWords );
    for ( i = 0; i < nVars; i++ )
    {
        pTruth = (unsigned *)p->pArray[i];
        if ( i < 5 )
        {
            for ( k = 0; k < nWords; k++ )
                pTruth[k] = Masks[i];
        }
        else
        {
            for ( k = 0; k < nWords; k++ )
                if ( k & (1 << (i-5)) )
                    pTruth[k] = ~(unsigned)0;
                else
                    pTruth[k] = 0;
        }
    }
    return p;
}

/**Function*************************************************************

  Synopsis    [Duplicates the integer array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Ptr_t * Vec_PtrDup( Vec_Ptr_t * pVec )
{
    Vec_Ptr_t * p;
    p = ALLOC( Vec_Ptr_t, 1 );
    p->nSize  = pVec->nSize;
    p->nCap   = pVec->nCap;
    p->pArray = p->nCap? ALLOC( void *, p->nCap ) : NULL;
    memcpy( p->pArray, pVec->pArray, sizeof(void *) * pVec->nSize );
    return p;
}

/**Function*************************************************************

  Synopsis    [Transfers the array into another vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Ptr_t * Vec_PtrDupArray( Vec_Ptr_t * pVec )
{
    Vec_Ptr_t * p;
    p = ALLOC( Vec_Ptr_t, 1 );
    p->nSize  = pVec->nSize;
    p->nCap   = pVec->nCap;
    p->pArray = pVec->pArray;
    pVec->nSize  = 0;
    pVec->nCap   = 0;
    pVec->pArray = NULL;
    return p;
}

/**Function*************************************************************

  Synopsis    [Frees the vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_PtrFree( Vec_Ptr_t * p )
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
static inline void ** Vec_PtrReleaseArray( Vec_Ptr_t * p )
{
    void ** pArray = p->pArray;
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
static inline void ** Vec_PtrArray( Vec_Ptr_t * p )
{
    return p->pArray;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_PtrSize( Vec_Ptr_t * p )
{
    return p->nSize;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void * Vec_PtrEntry( Vec_Ptr_t * p, int i )
{
    assert( i >= 0 && i < p->nSize );
    return p->pArray[i];
}

/**Function*************************************************************

  Synopsis    [Resizes the array of simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_PtrDoubleSimInfo( Vec_Ptr_t * vInfo )
{
    Vec_Ptr_t * vInfoNew;
    int nWords;
    assert( Vec_PtrSize(vInfo) > 2 );
    // get the new array
    nWords = (unsigned *)Vec_PtrEntry(vInfo,1) - (unsigned *)Vec_PtrEntry(vInfo,0);
    vInfoNew = Vec_PtrAllocSimInfo( 2*Vec_PtrSize(vInfo), nWords );
    // copy the simulation info
    memcpy( Vec_PtrEntry(vInfoNew,0), Vec_PtrEntry(vInfo,0), Vec_PtrSize(vInfo) * nWords * 4 );
    // replace the array
    free( vInfo->pArray );
    vInfo->pArray = vInfoNew->pArray;
    vInfo->nSize *= 2;
    vInfo->nCap *= 2;
    // free the old array
    vInfoNew->pArray = NULL;
    free( vInfoNew );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void ** Vec_PtrEntryP( Vec_Ptr_t * p, int i )
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
static inline void Vec_PtrWriteEntry( Vec_Ptr_t * p, int i, void * Entry )
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
static inline void * Vec_PtrEntryLast( Vec_Ptr_t * p )
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
static inline void Vec_PtrGrow( Vec_Ptr_t * p, int nCapMin )
{
    if ( p->nCap >= nCapMin )
        return;
    p->pArray = REALLOC( void *, p->pArray, nCapMin ); 
    p->nCap   = nCapMin;
}

/**Function*************************************************************

  Synopsis    [Fills the vector with given number of entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_PtrFill( Vec_Ptr_t * p, int nSize, void * Entry )
{
    int i;
    Vec_PtrGrow( p, nSize );
    for ( i = 0; i < nSize; i++ )
        p->pArray[i] = Entry;
    p->nSize = nSize;
}

/**Function*************************************************************

  Synopsis    [Fills the vector with given number of entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_PtrFillExtra( Vec_Ptr_t * p, int nSize, void * Entry )
{
    int i;
    if ( p->nSize >= nSize )
        return;
    assert( p->nSize < nSize );
    if ( 2 * p->nSize > nSize )
        Vec_PtrGrow( p, 2 * nSize );
    else
        Vec_PtrGrow( p, nSize );
    for ( i = p->nSize; i < nSize; i++ )
        p->pArray[i] = Entry;
    p->nSize = nSize;
}

/**Function*************************************************************

  Synopsis    [Returns the entry even if the place not exist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void * Vec_PtrGetEntry( Vec_Ptr_t * p, int i )
{
    Vec_PtrFillExtra( p, i + 1, NULL );
    return Vec_PtrEntry( p, i );
}

/**Function*************************************************************

  Synopsis    [Inserts the entry even if the place does not exist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_PtrSetEntry( Vec_Ptr_t * p, int i, void * Entry )
{
    Vec_PtrFillExtra( p, i + 1, NULL );
    Vec_PtrWriteEntry( p, i, Entry );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_PtrShrink( Vec_Ptr_t * p, int nSizeNew )
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
static inline void Vec_PtrClear( Vec_Ptr_t * p )
{
    p->nSize = 0;
}

/**Function*************************************************************

  Synopsis    [Copies the interger array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_PtrCopy( Vec_Ptr_t * pDest, Vec_Ptr_t * pSour )
{
    pDest->nSize = 0;
    Vec_PtrGrow( pDest, pSour->nSize );
    memcpy( pDest->pArray, pSour->pArray, sizeof(void *) * pSour->nSize );
    pDest->nSize = pSour->nSize;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_PtrPush( Vec_Ptr_t * p, void * Entry )
{
    if ( p->nSize == p->nCap )
    {
        if ( p->nCap < 16 )
            Vec_PtrGrow( p, 16 );
        else
            Vec_PtrGrow( p, 2 * p->nCap );
    }
    p->pArray[p->nSize++] = Entry;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_PtrPushUnique( Vec_Ptr_t * p, void * Entry )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        if ( p->pArray[i] == Entry )
            return 1;
    Vec_PtrPush( p, Entry );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns the last entry and removes it from the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void * Vec_PtrPop( Vec_Ptr_t * p )
{
    assert( p->nSize > 0 );
    return p->pArray[--p->nSize];
}

/**Function*************************************************************

  Synopsis    [Find entry.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_PtrFind( Vec_Ptr_t * p, void * Entry )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        if ( p->pArray[i] == Entry )
            return i;
    return -1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_PtrRemove( Vec_Ptr_t * p, void * Entry )
{
    int i;
    // delete assuming that it is closer to the end
    for ( i = p->nSize - 1; i >= 0; i-- )
        if ( p->pArray[i] == Entry )
            break;
    assert( i >= 0 );
/*
    // delete assuming that it is closer to the beginning
    for ( i = 0; i < p->nSize; i++ )
        if ( p->pArray[i] == Entry )
            break;
    assert( i < p->nSize );
*/
    for ( i++; i < p->nSize; i++ )
        p->pArray[i-1] = p->pArray[i];
    p->nSize--;
}

/**Function*************************************************************

  Synopsis    [Moves the first nItems to the end.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_PtrReorder( Vec_Ptr_t * p, int nItems )
{
    assert( nItems < p->nSize );
    Vec_PtrGrow( p, nItems + p->nSize );
    memmove( (char **)p->pArray + p->nSize, p->pArray, nItems * sizeof(void*) );
    memmove( p->pArray, (char **)p->pArray + nItems, p->nSize * sizeof(void*) );
}

/**Function*************************************************************

  Synopsis    [Sorting the entries by their integer value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_PtrSort( Vec_Ptr_t * p, int (*Vec_PtrSortCompare)() )
{
    if ( p->nSize < 2 )
        return;
    qsort( (void *)p->pArray, p->nSize, sizeof(void *), 
            (int (*)(const void *, const void *)) Vec_PtrSortCompare );
}

/**Function*************************************************************

  Synopsis    [Sorting the entries by their integer value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_PtrUniqify( Vec_Ptr_t * p, int (*Vec_PtrSortCompare)() )
{
    int i, k;
    if ( p->nSize < 2 )
        return;
    qsort( (void *)p->pArray, p->nSize, sizeof(void *), 
            (int (*)(const void *, const void *)) Vec_PtrSortCompare );
    for ( i = k = 1; i < p->nSize; i++ )
        if ( p->pArray[i] != p->pArray[i-1] )
            p->pArray[k++] = p->pArray[i];
    p->nSize = k;
}

#endif


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

