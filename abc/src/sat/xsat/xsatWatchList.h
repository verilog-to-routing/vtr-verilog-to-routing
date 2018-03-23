/**CFile****************************************************************

  FileName    [xsatWatchList.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [xSAT - A SAT solver written in C.
               Read the license file for more info.]

  Synopsis    [Watch list and its related structures implementation]

  Author      [Bruno Schmitt <boschmitt@inf.ufrgs.br>]

  Affiliation [UC Berkeley / UFRGS]

  Date        [Ver. 1.0. Started - November 10, 2016.]

  Revision    []

***********************************************************************/
#ifndef ABC__sat__xSAT__xsatWatchList_h
#define ABC__sat__xSAT__xsatWatchList_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////
#include "misc/util/abc_global.h"

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
typedef struct xSAT_Watcher_t_ xSAT_Watcher_t;
struct xSAT_Watcher_t_
{
    unsigned CRef;
    int Blocker;
};

typedef struct xSAT_WatchList_t_ xSAT_WatchList_t;
struct xSAT_WatchList_t_
{
    int nCap;
    int nSize;
    xSAT_Watcher_t * pArray;
};

typedef struct xSAT_VecWatchList_t_ xSAT_VecWatchList_t;
struct xSAT_VecWatchList_t_
{
    int nCap;
    int nSize;
    xSAT_WatchList_t * pArray;
};

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void xSAT_WatchListFree( xSAT_WatchList_t * v )
{
    if ( v->pArray )
        ABC_FREE( v->pArray );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int  xSAT_WatchListSize( xSAT_WatchList_t * v )
{
    return v->nSize;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void xSAT_WatchListShrink( xSAT_WatchList_t * v, int k )
{
    assert(k <= v->nSize);
    v->nSize = k;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void xSAT_WatchListPush( xSAT_WatchList_t * v, xSAT_Watcher_t e )
{
    assert( v );
    if ( v->nSize == v->nCap )
    {
        int newsize = ( v->nCap < 4 ) ? 4 : ( v->nCap / 2 ) * 3;

        v->pArray = ABC_REALLOC( xSAT_Watcher_t, v->pArray, newsize );
        if ( v->pArray == NULL )
        {
            printf( "Failed to realloc memory from %.1f MB to %.1f MB.\n",
                1.0 * v->nCap / (1<<20), 1.0 * newsize / (1<<20) );
            fflush( stdout );
        }
        v->nCap = newsize;
    }

    v->pArray[v->nSize++] = e;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline xSAT_Watcher_t* xSAT_WatchListArray( xSAT_WatchList_t * v )
{
    return v->pArray;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void xSAT_WatchListRemove( xSAT_WatchList_t * v, unsigned CRef )
{
    xSAT_Watcher_t* ws = xSAT_WatchListArray(v);
    int j = 0;

    for ( ; ws[j].CRef != CRef; j++ );
    assert( j < xSAT_WatchListSize( v ) );
    memmove( v->pArray + j, v->pArray + j + 1, ( v->nSize - j - 1 ) * sizeof( xSAT_Watcher_t ) );
    v->nSize -= 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline xSAT_VecWatchList_t * xSAT_VecWatchListAlloc( int nCap )
{
    xSAT_VecWatchList_t * v = ABC_ALLOC( xSAT_VecWatchList_t, 1 );

    v->nCap   = 4;
    v->nSize  = 0;
    v->pArray = ( xSAT_WatchList_t * ) ABC_CALLOC(xSAT_WatchList_t, sizeof( xSAT_WatchList_t ) * v->nCap);
    return v;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void xSAT_VecWatchListFree( xSAT_VecWatchList_t* v )
{
    int i;
    for( i = 0; i < v->nSize; i++ )
        xSAT_WatchListFree( v->pArray + i );

    ABC_FREE( v->pArray );
    ABC_FREE( v );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void xSAT_VecWatchListPush( xSAT_VecWatchList_t* v )
{
    if ( v->nSize == v->nCap )
    {
        int newsize = (v->nCap < 4) ? v->nCap * 2 : (v->nCap / 2) * 3;

        v->pArray = ABC_REALLOC( xSAT_WatchList_t, v->pArray, newsize );
        memset( v->pArray + v->nCap, 0, sizeof( xSAT_WatchList_t ) * ( newsize - v->nCap ) );
        if ( v->pArray == NULL )
        {
            printf( "Failed to realloc memory from %.1f MB to %.1f MB.\n",
                1.0 * v->nCap / (1<<20), 1.0 * newsize / (1<<20) );
            fflush( stdout );
        }
        v->nCap = newsize;
    }

    v->nSize++;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline xSAT_WatchList_t * xSAT_VecWatchListEntry( xSAT_VecWatchList_t* v, int iEntry )
{
    assert( iEntry < v->nCap );
    assert( iEntry < v->nSize );
    return v->pArray + iEntry;
}

ABC_NAMESPACE_HEADER_END

#endif
