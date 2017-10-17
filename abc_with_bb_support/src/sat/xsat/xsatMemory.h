/**CFile****************************************************************

  FileName    [xsatMemory.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [xSAT - A SAT solver written in C.
               Read the license file for more info.]

  Synopsis    [Memory management implementation.]

  Author      [Bruno Schmitt <boschmitt@inf.ufrgs.br>]

  Affiliation [UC Berkeley / UFRGS]

  Date        [Ver. 1.0. Started - November 10, 2016.]

  Revision    []

***********************************************************************/
#ifndef ABC__sat__xSAT__xsatMemory_h
#define ABC__sat__xSAT__xsatMemory_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////
#include "misc/util/abc_global.h"

#include "xsatClause.h"

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
typedef struct xSAT_Mem_t_ xSAT_Mem_t;
struct xSAT_Mem_t_
{
    unsigned   nSize;
    unsigned   nCap;
    unsigned   nWasted;
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
static inline xSAT_Clause_t *  xSAT_MemClauseHand( xSAT_Mem_t * p, int h )
{
    return h != 0xFFFFFFFF ? ( xSAT_Clause_t * )( p->pData + h ) : NULL;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void xSAT_MemGrow( xSAT_Mem_t * p, unsigned nCap )
{
    unsigned nPrevCap = p->nCap;
    if ( p->nCap >= nCap )
        return;
    while (p->nCap < nCap)
    {
        unsigned delta = ( ( p->nCap >> 1 ) + ( p->nCap >> 3 ) + 2 ) & ~1;
        p->nCap += delta;
        assert(p->nCap >= nPrevCap);
    }
    assert(p->nCap > 0);
    p->pData = ABC_REALLOC( unsigned, p->pData, p->nCap );
}

/**Function*************************************************************

  Synopsis    [Allocating vector.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline xSAT_Mem_t * xSAT_MemAlloc( int nCap )
{
    xSAT_Mem_t * p;
    p = ABC_CALLOC( xSAT_Mem_t, 1 );
    if (nCap <= 0)
        nCap = 1024*1024;

    xSAT_MemGrow(p, nCap);
    return p;
}

/**Function*************************************************************

  Synopsis    [Resetting vector.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void xSAT_MemRestart( xSAT_Mem_t * p )
{
    p->nSize = 0;
    p->nWasted = 0;
}

/**Function*************************************************************

  Synopsis    [Freeing vector.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void xSAT_MemFree( xSAT_Mem_t * p )
{
    ABC_FREE( p->pData );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Creates new clause.]

  Description [The resulting clause is fully initialized.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned xSAT_MemAppend( xSAT_Mem_t * p, int nSize )
{
    unsigned nPrevSize;
    assert(nSize > 0);
    xSAT_MemGrow( p, p->nSize + nSize );
    nPrevSize = p->nSize;
    p->nSize += nSize;
    assert(p->nSize > nPrevSize);
    return nPrevSize;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned xSAT_MemCRef( xSAT_Mem_t * p, unsigned * pC )
{
    return ( unsigned )( pC - &(p->pData[0]) );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned xSAT_MemCap( xSAT_Mem_t * p )
{
    return p->nCap;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned xSAT_MemWastedCap( xSAT_Mem_t * p )
{
    return p->nWasted;
}

ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
