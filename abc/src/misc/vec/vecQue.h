/**CFile****************************************************************

  FileName    [vecQue.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Resizable arrays.]

  Synopsis    [Priority queue.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: vecQue.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__misc__vec__Queue_h
#define ABC__misc__vec__Queue_h

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

typedef struct Vec_Que_t_  Vec_Que_t;
struct Vec_Que_t_ 
{
    int             nCap;
    int             nSize;
    int *           pHeap;
    int *           pOrder;
    float **        pCostsFlt;  // owned by the caller
};

static inline float Vec_QuePrio( Vec_Que_t * p, int v ) { return *p->pCostsFlt ? (*p->pCostsFlt)[v] : v; }

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
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
static inline Vec_Que_t * Vec_QueAlloc( int nCap )
{
    Vec_Que_t * p;
    p = ABC_CALLOC( Vec_Que_t, 1 );
    if ( nCap < 16 )
        nCap = 16;
    p->nSize  = 1;
    p->nCap   = nCap + 1;
    p->pHeap  = ABC_FALLOC( int, p->nCap );
    p->pOrder = ABC_FALLOC( int, p->nCap );
    return p;
}
static inline void Vec_QueFree( Vec_Que_t * p )
{
    ABC_FREE( p->pOrder );
    ABC_FREE( p->pHeap );
    ABC_FREE( p );
}
static inline void Vec_QueFreeP( Vec_Que_t ** p )
{
    if ( *p )
        Vec_QueFree( *p );
    *p = NULL;
}
static inline void Vec_QueSetPriority( Vec_Que_t * p, float ** pCosts )
{
    assert( p->pCostsFlt == NULL );
    p->pCostsFlt = pCosts;
}
static inline void Vec_QueGrow( Vec_Que_t * p, int nCapMin )
{
    if ( p->nCap >= nCapMin )
        return;
    p->pHeap  = ABC_REALLOC( int, p->pHeap,  nCapMin );
    p->pOrder = ABC_REALLOC( int, p->pOrder, nCapMin ); 
    memset( p->pHeap  + p->nCap, 0xff, (size_t)(nCapMin - p->nCap) * sizeof(int) );
    memset( p->pOrder + p->nCap, 0xff, (size_t)(nCapMin - p->nCap) * sizeof(int) );
    p->nCap   = nCapMin;
}
static inline void Vec_QueClear( Vec_Que_t * p )
{
    int i;
    assert( p->pHeap[0] == -1 );
    for ( i = 1; i < p->nSize; i++ )
    {
        assert( p->pHeap[i] >= 0 && p->pOrder[p->pHeap[i]] == i );
        p->pOrder[p->pHeap[i]] = -1;
        p->pHeap[i] = -1;
    }
    p->nSize = 1;
}
static inline double Vec_QueMemory( Vec_Que_t * p )
{
    return !p ? 0.0 : 2.0 * sizeof(int) * (size_t)p->nCap + sizeof(Vec_Que_t) ;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_QueSize( Vec_Que_t * p )
{
    assert( p->nSize > 0 );
    return p->nSize - 1;
}
static inline int Vec_QueTop( Vec_Que_t * p )
{
    return Vec_QueSize(p) > 0 ? p->pHeap[1] : -1;
}
static inline float Vec_QueTopPriority( Vec_Que_t * p )
{
    return Vec_QueSize(p) > 0 ? Vec_QuePrio(p, p->pHeap[1]) : -ABC_INFINITY;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_QueMoveUp( Vec_Que_t * p, int v )
{
    float Cost = Vec_QuePrio(p, v);
    int i      = p->pOrder[v];
    int parent = i >> 1;
    int fMoved = 0;
    assert( p->pOrder[v] != -1 );
    assert( p->pHeap[i] == v );
    while ( i > 1 && Cost > Vec_QuePrio(p, p->pHeap[parent]) )
    {
        p->pHeap[i]            = p->pHeap[parent];
        p->pOrder[p->pHeap[i]] = i;
        i                      = parent;
        parent                 = i >> 1;
        fMoved                 = 1;
    }
    p->pHeap[i]  = v;
    p->pOrder[v] = i;
    return fMoved;
}
static inline void Vec_QueMoveDown( Vec_Que_t * p, int v )
{
    float Cost = Vec_QuePrio(p, v);
    int i      = p->pOrder[v];
    int child  = i << 1;
    while ( child < p->nSize )
    {
        if ( child + 1 < p->nSize && Vec_QuePrio(p, p->pHeap[child]) < Vec_QuePrio(p, p->pHeap[child+1]) )
            child++;
        assert( child < p->nSize );
        if ( Cost >= Vec_QuePrio(p, p->pHeap[child]))
            break;
        p->pHeap[i]            = p->pHeap[child];
        p->pOrder[p->pHeap[i]] = i;
        i                      = child;
        child                  = child << 1;
    }
    p->pHeap[i]  = v;
    p->pOrder[v] = i;
}
static inline void Vec_QueUpdate( Vec_Que_t * p, int v )
{
    if ( !Vec_QueMoveUp( p, v ) )
        Vec_QueMoveDown( p, v );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_QueIsMember( Vec_Que_t * p, int v )
{
    assert( v >= 0 );
    return (int)( v < p->nCap && p->pOrder[v] >= 0 );
}
static inline void Vec_QuePush( Vec_Que_t * p, int v )
{
    if ( p->nSize >= p->nCap )
        Vec_QueGrow( p, Abc_MaxInt(p->nSize+1, 2*p->nCap) );
    if ( v >= p->nCap )
        Vec_QueGrow( p, Abc_MaxInt(v+1, 2*p->nCap) );
    assert( p->nSize < p->nCap );
    assert( p->pOrder[v] == -1 );
    assert( p->pHeap[p->nSize] == -1 );
    p->pOrder[v] = p->nSize;
    p->pHeap[p->nSize++] = v;
    Vec_QueMoveUp( p, v );
}
static inline int Vec_QuePop( Vec_Que_t * p )
{
    int v, Res;
    assert( p->nSize > 1 );
    Res = p->pHeap[1];      p->pOrder[Res] = -1;
    if ( --p->nSize == 1 )
    {
        p->pHeap[1] = -1;
        return Res;
    }
    v = p->pHeap[p->nSize]; p->pHeap[p->nSize] = -1;
    p->pHeap[1] = v;        p->pOrder[v] = 1;
    Vec_QueMoveDown( p, v );
    return Res;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_QuePrint( Vec_Que_t * p )
{
    int i, k, m;
    for ( i = k = 1; i < p->nSize; i += k, k *= 2 )
    {
        for ( m = 0; m < k; m++ )
            if ( i+m < p->nSize )
                printf( "%-5d", p->pHeap[i+m] );
        printf( "\n" );
        for ( m = 0; m < k; m++ )
            if ( i+m < p->nSize )
                printf( "%-5.0f", Vec_QuePrio(p, p->pHeap[i+m]) );
        printf( "\n" );
        printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_QueCheck( Vec_Que_t * p )
{
    int i, child;
    assert( p->nSize > 0 );
    assert( p->nSize <= p->nCap );
    // check mapping
    for ( i = 0; i < p->nSize-1; i++ )
        assert( p->pOrder[i] > 0 );
    for ( ; i < p->nCap; i++ )
        assert( p->pOrder[i] == -1 );
    // check heap
    assert( p->pHeap[0] == -1 );
    for ( i = 0; i < p->nSize-1; i++ )
        assert( p->pHeap[p->pOrder[i]] == i );
    for ( i++; i < p->nCap; i++ )
        assert( p->pHeap[i] == -1 );
    // check heap property
    for ( i = 1; i < p->nSize; i++ )
    {
        child = i << 1;
        if ( child < p->nSize )
            assert( Vec_QuePrio(p, p->pHeap[i]) >= Vec_QuePrio(p, p->pHeap[child]) );
        child++;
        if ( child < p->nSize )
            assert( Vec_QuePrio(p, p->pHeap[i]) >= Vec_QuePrio(p, p->pHeap[child]) );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_QueTest( Vec_Flt_t * vCosts )
{
    Vec_Int_t * vTop;
    Vec_Que_t * p;
    int i, Entry;

    // start the queue
    p = Vec_QueAlloc( Vec_FltSize(vCosts) );
    Vec_QueSetPriority( p, Vec_FltArrayP(vCosts) );
    for ( i = 0; i < Vec_FltSize(vCosts); i++ )
        Vec_QuePush( p, i );
//    Vec_QuePrint( p );
    Vec_QueCheck( p );

    // find the topmost 10%
    vTop = Vec_IntAlloc( Vec_FltSize(vCosts) / 10 );
    while ( Vec_IntSize(vTop) < Vec_FltSize(vCosts) / 10 )
        Vec_IntPush( vTop, Vec_QuePop(p) );
//    Vec_IntPrint( vTop );
//    Vec_QueCheck( p ); // queque is not ready at this point!!!

    // put them back
    Vec_IntForEachEntry( vTop, Entry, i )
        Vec_QuePush( p, Entry );
    Vec_IntFree( vTop );
    Vec_QueCheck( p );

    Vec_QueFree( p );
}

/*
    {
        extern void Vec_QueTest( Vec_Flt_t * p );
        Vec_QueTest( p->vTimesOut );
    }
*/

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_END

#endif

