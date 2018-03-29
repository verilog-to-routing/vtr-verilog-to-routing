/**CFile****************************************************************

  FileName    [satProof2.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT solver.]

  Synopsis    [Proof logging.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: satProof2.h,v 1.0 2004/01/01 1:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__sat__bsat__satProof2_h
#define ABC__sat__bsat__satProof2_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "misc/vec/vec.h"

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct Prf_Man_t_ Prf_Man_t;
struct Prf_Man_t_ 
{
    int             iFirst;    // first learned clause with proof
    int             iFirst2;   // first learned clause with proof
    int             nWords;    // the number of proof words
    word *          pInfo;     // pointer to the current proof
    Vec_Wrd_t *     vInfo;     // proof information
    Vec_Int_t *     vSaved;    // IDs of saved clauses
    Vec_Int_t *     vId2Pr;    // mapping proof IDs of problem clauses into bitshifts (user's array)
};

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////

static inline int    Prf_BitWordNum( int nWidth )                { return (nWidth >> 6) + ((nWidth & 63) > 0);                           }
static inline int    Prf_ManSize( Prf_Man_t * p )                { return Vec_WrdSize( p->vInfo ) / p->nWords;                           }
static inline void   Prf_ManClearNewInfo( Prf_Man_t * p )        { int w; for ( w = 0; w < p->nWords; w++ ) Vec_WrdPush( p->vInfo, 0 );  }
static inline word * Prf_ManClauseInfo( Prf_Man_t * p, int Id )  { return Vec_WrdEntryP( p->vInfo, Id * p->nWords );                     }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DECLARATIONS                        ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Prf_Man_t * Prf_ManAlloc()
{
    Prf_Man_t * p;
    p = ABC_CALLOC( Prf_Man_t, 1 );
    p->iFirst  = -1;
    p->iFirst2 = -1;
    p->vInfo   = Vec_WrdAlloc( 1000 );
    p->vSaved  = Vec_IntAlloc( 1000 );
    return p;
}
static inline void Prf_ManStop( Prf_Man_t * p )
{
    if ( p == NULL )
        return;
    Vec_IntFree( p->vSaved );
    Vec_WrdFree( p->vInfo );
    ABC_FREE( p );
}
static inline void Prf_ManStopP( Prf_Man_t ** p )
{
    Prf_ManStop( *p );
    *p = NULL;
}
static inline double Prf_ManMemory( Prf_Man_t * p )
{
    return Vec_WrdMemory(p->vInfo) + Vec_IntMemory(p->vSaved) + sizeof(Prf_Man_t);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Prf_ManRestart( Prf_Man_t * p, Vec_Int_t * vId2Pr, int iFirst, int nWidth )
{
    assert( p->iFirst == -1 );
    p->iFirst = iFirst;
    p->nWords = Prf_BitWordNum( nWidth );
    p->vId2Pr = vId2Pr;
    p->pInfo  = NULL;
    Vec_WrdClear( p->vInfo );
}
static inline void Prf_ManGrow( Prf_Man_t * p, int nWidth )
{
    Vec_Wrd_t * vInfoNew;
    int i, w, nSize, nWordsNew;
    assert( p->iFirst >= 0 );
    assert( p->pInfo == NULL );
    if ( nWidth < 64 * p->nWords )
        return;
    // get word count after resizing
    nWordsNew = Abc_MaxInt( Prf_BitWordNum(nWidth), 2 * p->nWords );
    // remap the entries
    nSize = Prf_ManSize( p );
    vInfoNew = Vec_WrdAlloc( (nSize + 1000) * nWordsNew );
    for ( i = 0; i < nSize; i++ )
    {
        p->pInfo = Prf_ManClauseInfo( p, i );
        for ( w = 0; w < p->nWords; w++ )
            Vec_WrdPush( vInfoNew, p->pInfo[w] );
        for ( ; w < nWordsNew; w++ )
            Vec_WrdPush( vInfoNew, 0 );
    }
    Vec_WrdFree( p->vInfo );
    p->vInfo = vInfoNew;
    p->nWords = nWordsNew;
    p->pInfo = NULL;

}
static inline void Prf_ManShrink( Prf_Man_t * p, int iClause )
{
    assert( p->iFirst >= 0 );
    assert( iClause - p->iFirst >= 0 );
    assert( iClause - p->iFirst < Prf_ManSize(p) ); 
    Vec_WrdShrink( p->vInfo, (iClause - p->iFirst) * p->nWords );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Prf_ManAddSaved( Prf_Man_t * p, int i, int iNew )
{
    assert( p->iFirst >= 0 );
    if ( i < p->iFirst )
        return;
    if ( Vec_IntSize(p->vSaved) == 0 )
    {
        assert( p->iFirst2 == -1 );
        p->iFirst2 = iNew;
    }
    Vec_IntPush( p->vSaved, i );
}
static inline void Prf_ManCompact( Prf_Man_t * p, int iNew )
{
    int i, w, k = 0, Entry, nSize;
    assert( p->iFirst >= 0 );
    assert( p->pInfo == NULL );
    nSize = Prf_ManSize( p );
    Vec_IntForEachEntry( p->vSaved, Entry, i )
    {
        assert( Entry - p->iFirst >= 0 && Entry - p->iFirst < nSize );
        p->pInfo = Prf_ManClauseInfo( p, Entry - p->iFirst );
        for ( w = 0; w < p->nWords; w++ )
            Vec_WrdWriteEntry( p->vInfo, k++, p->pInfo[w] );
    }
    Vec_WrdShrink( p->vInfo, k );
    Vec_IntClear( p->vSaved );
    p->pInfo = NULL;
    // update first
    if ( p->iFirst2 == -1 )
        p->iFirst = iNew;
    else
        p->iFirst = p->iFirst2;
    p->iFirst2 = -1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Prf_ManChainResolve( Prf_Man_t * p, clause * c )
{
    assert( p->iFirst >= 0 );
    assert( p->pInfo != NULL );
    // add to proof info
    if ( c->lrn ) // learned clause
    {
        if ( clause_id(c) >= p->iFirst )
        {
            word * pProofStart;
            int w;
            assert( clause_id(c) - p->iFirst >= 0 );
            assert( clause_id(c) - p->iFirst < Prf_ManSize(p) ); 
            pProofStart = Prf_ManClauseInfo( p, clause_id(c) - p->iFirst );
            for ( w = 0; w < p->nWords; w++ )
                p->pInfo[w] |= pProofStart[w];
        }
    }
    else // problem clause
    {  
        if ( clause_id(c) >= 0 ) // has proof ID
        {
            int Entry;
            if ( p->vId2Pr == NULL )
                Entry = clause_id(c);
            else
                Entry = Vec_IntEntry( p->vId2Pr, clause_id(c) );
            if ( Entry >= 0 )
            {
                assert( Entry < 64 * p->nWords );
                Abc_InfoSetBit( (unsigned *)p->pInfo, Entry );
            }
        }
    }
}
static inline void Prf_ManChainStart( Prf_Man_t * p, clause * c )
{
    assert( p->iFirst >= 0 );
    // prepare info for new clause
    Prf_ManClearNewInfo( p );
    // get pointer to the proof
    assert( p->pInfo == NULL );
    p->pInfo = Prf_ManClauseInfo( p, Prf_ManSize(p)-1 );
    // add to proof info
    Prf_ManChainResolve( p, c );
}
static inline int Prf_ManChainStop( Prf_Man_t * p )
{
    assert( p->pInfo != NULL );
    p->pInfo = NULL;
    return 0;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Int_t * Prf_ManUnsatCore( Prf_Man_t * p )
{
    Vec_Int_t * vCore;
    int i, Entry;
    assert( p->iFirst >= 0 );
    assert( p->pInfo == NULL );
    assert( Prf_ManSize(p) > 0 );
    vCore = Vec_IntAlloc( 64 * p->nWords );
    p->pInfo = Prf_ManClauseInfo( p, Prf_ManSize(p)-1 );
    if ( p->vId2Pr == NULL )
    {
        for ( i = 0; i < 64 * p->nWords; i++ )
            if ( Abc_InfoHasBit( (unsigned *)p->pInfo, i ) )
                Vec_IntPush( vCore, i );
    }
    else
    {
        Vec_IntForEachEntry( p->vId2Pr, Entry, i )
        {
            if ( Entry < 0 )
                continue;
            assert( Entry < 64 * p->nWords );
            if ( Abc_InfoHasBit( (unsigned *)p->pInfo, Entry ) )
                Vec_IntPush( vCore, i );
        }
    }
    p->pInfo = NULL;
    Vec_IntSort( vCore, 1 );
    return vCore;
}



ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

