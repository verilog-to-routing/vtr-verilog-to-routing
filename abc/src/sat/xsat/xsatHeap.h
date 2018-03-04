/**CFile****************************************************************

  FileName    [xsatHeap.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [xSAT - A SAT solver written in C.
               Read the license file for more info.]

  Synopsis    [Heap implementation.]

  Author      [Bruno Schmitt <boschmitt@inf.ufrgs.br>]

  Affiliation [UC Berkeley / UFRGS]

  Date        [Ver. 1.0. Started - November 10, 2016.]

  Revision    []

***********************************************************************/
#ifndef ABC__sat__xSAT__xsatHeap_h
#define ABC__sat__xSAT__xsatHeap_h

#include "misc/util/abc_global.h"
#include "misc/vec/vecInt.h"

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
typedef struct xSAT_Heap_t_ xSAT_Heap_t;
struct xSAT_Heap_t_
{
    Vec_Int_t * vActivity;
    Vec_Int_t * vIndices;
    Vec_Int_t * vHeap;
};

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int xSAT_HeapSize( xSAT_Heap_t * h )
{
    return Vec_IntSize( h->vHeap );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int xSAT_HeapInHeap( xSAT_Heap_t * h, int Var )
{
    return ( Var < Vec_IntSize( h->vIndices ) ) && ( Vec_IntEntry( h->vIndices, Var ) >= 0 );
}

static inline int Left  ( int i ) { return 2 * i + 1; }
static inline int Right ( int i ) { return ( i + 1 ) * 2; }
static inline int Parent( int i ) { return ( i - 1 ) >> 1; }
static inline int Compare( xSAT_Heap_t * p, int x, int y )
{
    return ( unsigned )Vec_IntEntry( p->vActivity, x ) > ( unsigned )Vec_IntEntry( p->vActivity, y );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void xSAT_HeapPercolateUp( xSAT_Heap_t * h, int i )
{
    int x = Vec_IntEntry( h->vHeap, i );
    int p = Parent( i );

    while ( i != 0 && Compare( h, x, Vec_IntEntry( h->vHeap, p ) ) )
    {
        Vec_IntWriteEntry( h->vHeap, i, Vec_IntEntry( h->vHeap, p ) );
        Vec_IntWriteEntry( h->vIndices, Vec_IntEntry( h->vHeap, p ), i );
        i = p;
        p = Parent(p);
    }
    Vec_IntWriteEntry( h->vHeap, i, x );
    Vec_IntWriteEntry( h->vIndices, x, i );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void xSAT_HeapPercolateDown( xSAT_Heap_t * h, int i )
{
    int x = Vec_IntEntry( h->vHeap, i );

    while ( Left( i ) < Vec_IntSize( h->vHeap ) )
    {
        int child = Right( i ) < Vec_IntSize( h->vHeap ) &&
                    Compare( h, Vec_IntEntry( h->vHeap, Right( i ) ), Vec_IntEntry( h->vHeap, Left( i ) ) ) ?
                    Right( i ) : Left( i );

        if ( !Compare( h, Vec_IntEntry( h->vHeap, child ), x ) )
            break;

        Vec_IntWriteEntry( h->vHeap, i, Vec_IntEntry( h->vHeap, child ) );
        Vec_IntWriteEntry( h->vIndices, Vec_IntEntry( h->vHeap, i ), i );
        i = child;
    }
    Vec_IntWriteEntry( h->vHeap, i, x );
    Vec_IntWriteEntry( h->vIndices, x, i );
}

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline xSAT_Heap_t * xSAT_HeapAlloc( Vec_Int_t * vActivity )
{
    xSAT_Heap_t * p = ABC_ALLOC( xSAT_Heap_t, 1 );
    p->vActivity = vActivity;
    p->vIndices = Vec_IntAlloc( 0 );
    p->vHeap = Vec_IntAlloc( 0 );

    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void xSAT_HeapFree( xSAT_Heap_t * p )
{
    Vec_IntFree( p->vIndices );
    Vec_IntFree( p->vHeap );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void xSAT_HeapIncrease( xSAT_Heap_t * h, int e )
{
    assert( xSAT_HeapInHeap( h, e ) );
    xSAT_HeapPercolateDown( h, Vec_IntEntry( h->vIndices, e ) );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void xSAT_HeapDecrease( xSAT_Heap_t * p, int e )
{
    assert( xSAT_HeapInHeap( p, e ) );
    xSAT_HeapPercolateUp( p , Vec_IntEntry( p->vIndices, e ) );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void xSAT_HeapInsert( xSAT_Heap_t * p, int n )
{
    Vec_IntFillExtra( p->vIndices, n + 1, -1);
    assert( !xSAT_HeapInHeap( p, n ) );

    Vec_IntWriteEntry( p->vIndices, n, Vec_IntSize( p->vHeap ) );
    Vec_IntPush( p->vHeap, n );
    xSAT_HeapPercolateUp( p, Vec_IntEntry( p->vIndices, n ) );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void xSAT_HeapUpdate( xSAT_Heap_t * p, int i )
{
    if ( !xSAT_HeapInHeap( p, i ) )
        xSAT_HeapInsert( p, i );
    else
    {
        xSAT_HeapPercolateUp( p, Vec_IntEntry( p->vIndices, i ) );
        xSAT_HeapPercolateDown( p, Vec_IntEntry( p->vIndices, i ) );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void xSAT_HeapBuild( xSAT_Heap_t * p, Vec_Int_t * Vars )
{
    int i, Var;

    Vec_IntForEachEntry( p->vHeap, Var, i )
        Vec_IntWriteEntry( p->vIndices, Var, -1 );
    Vec_IntClear( p->vHeap );

    Vec_IntForEachEntry( Vars, Var, i )
    {
        Vec_IntWriteEntry( p->vIndices, Var, i );
        Vec_IntPush( p->vHeap, Var );
    }

    for ( ( i = Vec_IntSize( p->vHeap ) / 2 - 1 ); i >= 0; i-- )
        xSAT_HeapPercolateDown( p, i );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void xSAT_HeapClear( xSAT_Heap_t * p )
{
    Vec_IntFill( p->vIndices, Vec_IntSize( p->vIndices ), -1 );
    Vec_IntClear( p->vHeap );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int xSAT_HeapRemoveMin( xSAT_Heap_t * p )
{
    int x = Vec_IntEntry( p->vHeap, 0 );
    Vec_IntWriteEntry( p->vHeap, 0, Vec_IntEntryLast( p->vHeap ) );
    Vec_IntWriteEntry( p->vIndices, Vec_IntEntry( p->vHeap, 0), 0 );
    Vec_IntWriteEntry( p->vIndices, x, -1 );
    Vec_IntPop( p->vHeap );
    if ( Vec_IntSize( p->vHeap ) > 1 )
        xSAT_HeapPercolateDown( p, 0 );
    return x;
}

ABC_NAMESPACE_HEADER_END

#endif
