/**CFile****************************************************************

  FileName    [xsatBQueue.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [xSAT - A SAT solver written in C.
               Read the license file for more info.]

  Synopsis    [Bounded queue implementation.]

  Author      [Bruno Schmitt <boschmitt@inf.ufrgs.br>]

  Affiliation [UC Berkeley / UFRGS]

  Date        [Ver. 1.0. Started - November 10, 2016.]

  Revision    []

***********************************************************************/
#ifndef ABC__sat__xSAT__xsatBQueue_h
#define ABC__sat__xSAT__xsatBQueue_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////
#include "misc/util/abc_global.h"

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
typedef struct xSAT_BQueue_t_ xSAT_BQueue_t;
struct xSAT_BQueue_t_
{
    int nSize;
    int nCap;
    int iFirst;
    int iEmpty;
    word nSum;
    unsigned * pData;
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DECLARATIONS                        ///
////////////////////////////////////////////////////////////////////////
/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline xSAT_BQueue_t * xSAT_BQueueNew( int nCap )
{
    xSAT_BQueue_t * p = ABC_CALLOC( xSAT_BQueue_t, 1 );
    p->nCap = nCap;
    p->pData = ABC_CALLOC( unsigned, nCap );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void xSAT_BQueueFree( xSAT_BQueue_t * p )
{
    ABC_FREE( p->pData );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void xSAT_BQueuePush( xSAT_BQueue_t * p, unsigned Value )
{
    if ( p->nSize == p->nCap )
    {
        assert(p->iFirst == p->iEmpty);
        p->nSum -= p->pData[p->iFirst];
        p->iFirst = ( p->iFirst + 1 ) % p->nCap;
    }
    else
        p->nSize++;

    p->nSum += Value;
    p->pData[p->iEmpty] = Value;
    if ( ( ++p->iEmpty ) == p->nCap )
    {
        p->iEmpty = 0;
        p->iFirst = 0;
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int xSAT_BQueuePop( xSAT_BQueue_t * p )
{
    int RetValue;
    assert( p->nSize >= 1 );
    RetValue = p->pData[p->iFirst];
    p->nSum -= RetValue;
    p->iFirst = ( p->iFirst + 1 ) % p->nCap;
    p->nSize--;
    return RetValue;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned xSAT_BQueueAvg( xSAT_BQueue_t * p )
{
    return ( unsigned )( p->nSum / ( ( word ) p->nSize ) );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int xSAT_BQueueIsValid( xSAT_BQueue_t * p )
{
    return ( p->nCap == p->nSize );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void xSAT_BQueueClean( xSAT_BQueue_t * p )
{
    p->iFirst = 0;
    p->iEmpty = 0;
    p->nSize = 0;
    p->nSum = 0;
}

ABC_NAMESPACE_HEADER_END

#endif
