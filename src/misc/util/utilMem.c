/**CFile****************************************************************

  FileName    [utilMem.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Memory recycling utilities.]

  Synopsis    [Memory recycling utilities.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: utilMem.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "abc_global.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
 

typedef struct Vec_Mem_t_       Vec_Mem_t;
struct Vec_Mem_t_ 
{
    int              nCap;
    int              nSize;
    void **          pArray;
};

void * s_vAllocs = NULL;
void * s_vFrees  = NULL;
int    s_fInterrupt = 0;

#define ABC_MEM_ALLOC(type, num)     ((type *) malloc(sizeof(type) * (num)))
#define ABC_MEM_CALLOC(type, num)     ((type *) calloc((num), sizeof(type)))
#define ABC_MEM_FALLOC(type, num)     ((type *) memset(malloc(sizeof(type) * (num)), 0xff, sizeof(type) * (num)))
#define ABC_MEM_FREE(obj)             ((obj) ? (free((char *) (obj)), (obj) = 0) : 0)
#define ABC_MEM_REALLOC(type, obj, num)    \
        ((obj) ? ((type *) realloc((char *)(obj), sizeof(type) * (num))) : \
         ((type *) malloc(sizeof(type) * (num))))

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Mem_t * Vec_MemAlloc( int nCap )
{
    Vec_Mem_t * p;
    p = ABC_MEM_ALLOC( Vec_Mem_t, 1 );
    if ( nCap > 0 && nCap < 8 )
        nCap = 8;
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ABC_MEM_ALLOC( void *, p->nCap ) : NULL;
    return p;
}

/**Function*************************************************************

  Synopsis    [Frees the vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_MemFree( Vec_Mem_t * p )
{
    ABC_MEM_FREE( p->pArray );
    ABC_MEM_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Resizes the vector to the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_MemGrow( Vec_Mem_t * p, int nCapMin )
{
    if ( p->nCap >= nCapMin )
        return;
    p->pArray = ABC_MEM_REALLOC( void *, p->pArray, nCapMin ); 
    p->nCap   = nCapMin;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_MemPush( Vec_Mem_t * p, void * Entry )
{
    if ( p->nSize == p->nCap )
    {
        if ( p->nCap < 16 )
            Vec_MemGrow( p, 16 );
        else
            Vec_MemGrow( p, 2 * p->nCap );
    }
    p->pArray[p->nSize++] = Entry;
}

/**Function*************************************************************

  Synopsis    [Sorting the entries by their integer value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Vec_MemSort( Vec_Mem_t * p, int (*Vec_MemSortCompare)() )
{
    if ( p->nSize < 2 )
        return;
    qsort( (void *)p->pArray, p->nSize, sizeof(void *), 
            (int (*)(const void *, const void *)) Vec_MemSortCompare );
}


/**Function*************************************************************

  Synopsis    [Remembers an allocated pointer.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Util_MemRecAlloc( void * pMem )
{
    if ( s_vAllocs )
        Vec_MemPush( (Vec_Mem_t *)s_vAllocs, pMem );
    return pMem;
}

/**Function*************************************************************

  Synopsis    [Remembers a deallocated pointer.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Util_MemRecFree( void * pMem )
{
    if ( s_vFrees )
        Vec_MemPush( (Vec_Mem_t *)s_vFrees, pMem );
    return pMem;
}

/**Function*************************************************************

  Synopsis    [Procedure used for sorting the nodes in decreasing order of levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Util_ComparePointers( void ** pp1, void ** pp2 )
{
    if ( *pp1 < *pp2 )
        return -1;
    if ( *pp1 > *pp2 ) 
        return 1;
    return 0; 
}

/**Function*************************************************************

  Synopsis    [Finds entries that do not appear in both lists.]

  Description [Assumes that the vectors are sorted in the increasing order.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Mem_t * Vec_MemTwoMerge( Vec_Mem_t * vArr1, Vec_Mem_t * vArr2 )
{
    Vec_Mem_t * vArr = Vec_MemAlloc( vArr1->nSize + vArr2->nSize ); 
    void ** pBeg  = vArr->pArray;
    void ** pBeg1 = vArr1->pArray;
    void ** pBeg2 = vArr2->pArray;
    void ** pEnd1 = vArr1->pArray + vArr1->nSize;
    void ** pEnd2 = vArr2->pArray + vArr2->nSize;
    while ( pBeg1 < pEnd1 && pBeg2 < pEnd2 )
    {
        if ( *pBeg1 == *pBeg2 )
            pBeg1++, pBeg2++;
        else if ( *pBeg1 < *pBeg2 )
        {
            free( *pBeg1 );
            *pBeg++ = *pBeg1++;
        }
        else 
            assert( 0 );
//            *pBeg++ = *pBeg2++;
    }
    while ( pBeg1 < pEnd1 )
        *pBeg++ = *pBeg1++;
//    while ( pBeg2 < pEnd2 )
//        *pBeg++ = *pBeg2++;
    assert( pBeg2 >= pEnd2 );
    vArr->nSize = pBeg - vArr->pArray;
    assert( vArr->nSize <= vArr->nCap );
    assert( vArr->nSize >= vArr1->nSize );
    assert( vArr->nSize >= vArr2->nSize );
    return vArr;
}

/**Function*************************************************************

  Synopsis    [Recycles the accumulated memory.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Util_MemRecRecycle()
{
    Vec_Mem_t * vMerge;
    assert( s_vAllocs == NULL );
    assert( s_vFrees == NULL );
    Vec_MemSort( (Vec_Mem_t *)s_vAllocs, (int (*)())Util_ComparePointers );
    Vec_MemSort( (Vec_Mem_t *)s_vFrees, (int (*)())Util_ComparePointers );
    vMerge = (Vec_Mem_t *)Vec_MemTwoMerge( (Vec_Mem_t *)s_vAllocs, (Vec_Mem_t *)s_vFrees );
    Vec_MemFree( vMerge );
}

/**Function*************************************************************

  Synopsis    [Starts memory structures.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Util_MemRecStart()
{
    assert( s_vAllocs == NULL && s_vFrees == NULL );
    s_vAllocs = Vec_MemAlloc( 1000 );
    s_vFrees = Vec_MemAlloc( 1000 );
}

/**Function*************************************************************

  Synopsis    [Quits memory structures.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Util_MemRecQuit()
{
    assert( s_vAllocs != NULL && s_vFrees != NULL );
    Vec_MemFree( (Vec_Mem_t *)s_vAllocs );
    Vec_MemFree( (Vec_Mem_t *)s_vFrees );
}

/**Function*************************************************************

  Synopsis    [Starts memory structures.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Util_MemRecIsSet()
{
    return s_vAllocs != NULL && s_vFrees != NULL;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

