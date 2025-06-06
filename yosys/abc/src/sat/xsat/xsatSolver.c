/**CFile****************************************************************

  FileName    [xsatSolver.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [xSAT - A SAT solver written in C.
               Read the license file for more info.]

  Synopsis    [Solver internal functions implementation.]

  Author      [Bruno Schmitt <boschmitt@inf.ufrgs.br>]

  Affiliation [UC Berkeley / UFRGS]

  Date        [Ver. 1.0. Started - November 10, 2016.]

  Revision    []

***********************************************************************/
////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "xsatHeap.h"
#include "xsatSolver.h"
#include "xsatUtils.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DECLARATIONS                        ///
////////////////////////////////////////////////////////////////////////
/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int xSAT_SolverDecide( xSAT_Solver_t * s )
{
    int NextVar = VarUndef;

    while ( NextVar == VarUndef || Vec_StrEntry( s->vAssigns, NextVar ) != VarX )
    {
        if ( xSAT_HeapSize( s->hOrder ) == 0 )
        {
            NextVar = VarUndef;
            break;
        }
        else
            NextVar = xSAT_HeapRemoveMin( s->hOrder );
    }
    return NextVar;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void xSAT_SolverRebuildOrderHeap( xSAT_Solver_t * s )
{
    Vec_Int_t * vTemp = Vec_IntAlloc( Vec_StrSize( s->vAssigns ) );
    int Var;

    for ( Var = 0; Var < Vec_StrSize( s->vAssigns ); Var++ )
        if ( Vec_StrEntry( s->vAssigns, Var ) == VarX )
            Vec_IntPush( vTemp, Var );

    xSAT_HeapBuild( s->hOrder, vTemp );
    Vec_IntFree( vTemp );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void xSAT_SolverVarActRescale( xSAT_Solver_t * s )
{
    int i;
    unsigned * pActivity = ( unsigned * ) Vec_IntArray( s->vActivity );

    for ( i = 0; i < Vec_IntSize( s->vActivity ); i++ )
        pActivity[i] >>= 19;

    s->nVarActInc >>= 19;
    s->nVarActInc = Abc_MaxInt( s->nVarActInc, ( 1 << 5 ) );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void xSAT_SolverVarActBump( xSAT_Solver_t * s, int Var )
{
    unsigned * pActivity = ( unsigned * ) Vec_IntArray( s->vActivity );

    pActivity[Var] += s->nVarActInc;
    if ( pActivity[Var] & 0x80000000 )
        xSAT_SolverVarActRescale( s );

    if ( xSAT_HeapInHeap( s->hOrder, Var ) )
        xSAT_HeapDecrease( s->hOrder, Var );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void xSAT_SolverVarActDecay( xSAT_Solver_t * s )
{
    s->nVarActInc += ( s->nVarActInc >> 4 );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void xSAT_SolverClaActRescale( xSAT_Solver_t * s )
{
    xSAT_Clause_t * pC;
    int i, CRef;

    Vec_IntForEachEntry( s->vLearnts, CRef, i )
    {
        pC = xSAT_SolverReadClause( s, ( unsigned ) CRef );
        pC->pData[pC->nSize].Act >>= 14;
    }
    s->nClaActInc >>= 14;
    s->nClaActInc = Abc_MaxInt( s->nClaActInc, ( 1 << 10 ) );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void xSAT_SolverClaActBump( xSAT_Solver_t* s, xSAT_Clause_t * pCla )
{
    pCla->pData[pCla->nSize].Act += s->nClaActInc;
    if ( pCla->pData[pCla->nSize].Act & 0x80000000 )
        xSAT_SolverClaActRescale( s );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void xSAT_SolverClaActDecay( xSAT_Solver_t * s )
{
    s->nClaActInc += ( s->nClaActInc >> 10 );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int xSAT_SolverClaCalcLBD( xSAT_Solver_t * s, xSAT_Clause_t * pCla )
{
    int i;
    int nLBD = 0;

    s->nStamp++;
    for ( i = 0; i < pCla->nSize; i++ )
    {
        int Level = Vec_IntEntry( s->vLevels, xSAT_Lit2Var( pCla->pData[i].Lit ) );
        if ( ( unsigned ) Vec_IntEntry( s->vStamp, Level ) != s->nStamp )
        {
            Vec_IntWriteEntry( s->vStamp, Level, ( int ) s->nStamp );
            nLBD++;
        }
    }
    return nLBD;
}

static inline int xSAT_SolverClaCalcLBD2( xSAT_Solver_t * s, Vec_Int_t * vLits )
{
    int i;
    int nLBD = 0;

    s->nStamp++;
    for ( i = 0; i < Vec_IntSize( vLits ); i++ )
    {
        int Level = Vec_IntEntry( s->vLevels, xSAT_Lit2Var( Vec_IntEntry( vLits, i ) ) );
        if ( ( unsigned ) Vec_IntEntry( s->vStamp, Level ) != s->nStamp )
        {
            Vec_IntWriteEntry( s->vStamp, Level, ( int ) s->nStamp );
            nLBD++;
        }
    }
    return nLBD;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned xSAT_SolverClaNew( xSAT_Solver_t * s, Vec_Int_t * vLits , int fLearnt )
{
    unsigned CRef;
    xSAT_Clause_t * pCla;
    xSAT_Watcher_t w1;
    xSAT_Watcher_t w2;
    unsigned nWords;

    assert( Vec_IntSize( vLits ) > 1);
    assert( fLearnt == 0 || fLearnt == 1 );

    nWords = 3 + fLearnt + Vec_IntSize( vLits );
    CRef = xSAT_MemAppend( s->pMemory, nWords );
    pCla = xSAT_SolverReadClause( s, CRef );
    pCla->fLearnt = fLearnt;
    pCla->fMark = 0;
    pCla->fReallocd = 0;
    pCla->fCanBeDel = fLearnt;
    pCla->nSize = Vec_IntSize( vLits );
    memcpy( &( pCla->pData[0].Lit ), Vec_IntArray( vLits ), sizeof( int ) * Vec_IntSize( vLits ) );

    if ( fLearnt )
    {
        Vec_IntPush( s->vLearnts, CRef );
        pCla->nLBD = xSAT_SolverClaCalcLBD2( s, vLits );
        pCla->pData[pCla->nSize].Act = 0;
        s->Stats.nLearntLits += Vec_IntSize( vLits );
        xSAT_SolverClaActBump(s, pCla);
    }
    else
    {
        Vec_IntPush( s->vClauses, CRef );
        s->Stats.nClauseLits += Vec_IntSize( vLits );
    }

    w1.CRef = CRef;
    w2.CRef = CRef;
    w1.Blocker = pCla->pData[1].Lit;
    w2.Blocker = pCla->pData[0].Lit;

    if ( Vec_IntSize( vLits ) == 2 )
    {
        xSAT_WatchListPush( xSAT_VecWatchListEntry( s->vBinWatches, xSAT_NegLit( pCla->pData[0].Lit ) ), w1 );
        xSAT_WatchListPush( xSAT_VecWatchListEntry( s->vBinWatches, xSAT_NegLit( pCla->pData[1].Lit ) ), w2 );
    }
    else
    {
        xSAT_WatchListPush( xSAT_VecWatchListEntry( s->vWatches, xSAT_NegLit( pCla->pData[0].Lit ) ), w1 );
        xSAT_WatchListPush( xSAT_VecWatchListEntry( s->vWatches, xSAT_NegLit( pCla->pData[1].Lit ) ), w2 );
    }
    return CRef;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int xSAT_SolverEnqueue( xSAT_Solver_t * s, int Lit, unsigned Reason )
{
    int Var = xSAT_Lit2Var( Lit );

    Vec_StrWriteEntry( s->vAssigns, Var, (char)xSAT_LitSign( Lit ) );
    Vec_IntWriteEntry( s->vLevels, Var, xSAT_SolverDecisionLevel( s ) );
    Vec_IntWriteEntry( s->vReasons, Var, ( int ) Reason );
    Vec_IntPush( s->vTrail, Lit );

    return true;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void xSAT_SolverNewDecision( xSAT_Solver_t * s, int Lit )
{
    assert( Vec_StrEntry( s->vAssigns, xSAT_Lit2Var( Lit ) ) == VarX );
    s->Stats.nDecisions++;
    Vec_IntPush( s->vTrailLim, Vec_IntSize( s->vTrail ) );
    xSAT_SolverEnqueue( s, Lit, CRefUndef );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void xSAT_SolverCancelUntil( xSAT_Solver_t * s, int Level )
{
    int c;

    if ( xSAT_SolverDecisionLevel( s ) <= Level )
        return;

    for ( c = Vec_IntSize( s->vTrail ) - 1; c >= Vec_IntEntry( s->vTrailLim, Level ); c-- )
    {
        int Var = xSAT_Lit2Var( Vec_IntEntry( s->vTrail, c ) );

        Vec_StrWriteEntry( s->vAssigns, Var, VarX );
        Vec_IntWriteEntry( s->vReasons, Var, ( int ) CRefUndef );
        Vec_StrWriteEntry( s->vPolarity, Var, ( char )xSAT_LitSign( Vec_IntEntry( s->vTrail, c ) ) );

        if ( !xSAT_HeapInHeap( s->hOrder, Var ) )
            xSAT_HeapInsert( s->hOrder, Var );
    }

    s->iQhead = Vec_IntEntry( s->vTrailLim, Level );
    Vec_IntShrink( s->vTrail, Vec_IntEntry( s->vTrailLim, Level ) );
    Vec_IntShrink( s->vTrailLim, Level );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static int xSAT_SolverIsLitRemovable( xSAT_Solver_t* s, int Lit, int MinLevel )
{
    int top = Vec_IntSize( s->vTagged );

    assert( ( unsigned ) Vec_IntEntry( s->vReasons, xSAT_Lit2Var( Lit ) ) != CRefUndef );
    Vec_IntClear( s->vStack );
    Vec_IntPush( s->vStack, xSAT_Lit2Var( Lit ) );

    while ( Vec_IntSize( s->vStack ) )
    {
        int i;
        int v = Vec_IntPop(  s->vStack );
        xSAT_Clause_t* c = xSAT_SolverReadClause(s, ( unsigned ) Vec_IntEntry( s->vReasons, v ) );
        int * Lits = &( c->pData[0].Lit );

        assert( (unsigned) Vec_IntEntry( s->vReasons, v ) != CRefUndef);
        if( c->nSize == 2 && Vec_StrEntry( s->vAssigns, xSAT_Lit2Var( Lits[0] ) ) == xSAT_LitSign( xSAT_NegLit( Lits[0] ) ) )
        {
            assert( Vec_StrEntry( s->vAssigns, xSAT_Lit2Var( Lits[1] ) ) == xSAT_LitSign( ( Lits[1] ) ) );
            ABC_SWAP( int, Lits[0], Lits[1] );
        }

        for ( i = 1; i < c->nSize; i++ )
        {
            int v = xSAT_Lit2Var( Lits[i] );
            if ( !Vec_StrEntry( s->vSeen, v ) && Vec_IntEntry( s->vLevels, v ) )
            {
                if ( ( unsigned ) Vec_IntEntry( s->vReasons, v ) != CRefUndef && ( ( 1 << (Vec_IntEntry( s->vLevels, v ) & 31 ) ) & MinLevel ) )
                {
                    Vec_IntPush( s->vStack, v );
                    Vec_IntPush( s->vTagged, Lits[i] );
                    Vec_StrWriteEntry( s->vSeen, v, 1 );
                }
                else
                {
                    int Lit;
                    Vec_IntForEachEntryStart( s->vTagged, Lit, i, top )
                        Vec_StrWriteEntry( s->vSeen, xSAT_Lit2Var( Lit ), 0 );
                    Vec_IntShrink( s->vTagged, top );
                    return 0;
                }
            }
        }
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static void xSAT_SolverClaMinimisation( xSAT_Solver_t * s, Vec_Int_t * vLits )
{
    int * pLits = Vec_IntArray( vLits );
    int MinLevel = 0;
    int i, j;

    for ( i = 1; i < Vec_IntSize( vLits ); i++ )
    {
        int Level = Vec_IntEntry( s->vLevels, xSAT_Lit2Var( pLits[i] ) );
        MinLevel |= 1 << ( Level & 31 );
    }

    /* Remove reduntant literals */
    Vec_IntAppend( s->vTagged, vLits );
    for ( i = j = 1; i < Vec_IntSize( vLits ); i++ )
        if ( ( unsigned ) Vec_IntEntry( s->vReasons, xSAT_Lit2Var( pLits[i] ) ) == CRefUndef || !xSAT_SolverIsLitRemovable( s, pLits[i], MinLevel ) )
            pLits[j++] = pLits[i];
    Vec_IntShrink( vLits, j );

    /* Binary Resolution */
    if( Vec_IntSize( vLits ) <= 30 && xSAT_SolverClaCalcLBD2( s, vLits ) <= 6 )
    {
        int nb, l;
        int Lit;
        int FlaseLit = xSAT_NegLit( pLits[0] );
        xSAT_WatchList_t * ws = xSAT_VecWatchListEntry( s->vBinWatches, FlaseLit );
        xSAT_Watcher_t * begin = xSAT_WatchListArray( ws );
        xSAT_Watcher_t * end   = begin + xSAT_WatchListSize( ws );
        xSAT_Watcher_t * pWatcher;

        s->nStamp++;
        Vec_IntForEachEntry( vLits, Lit, i )
            Vec_IntWriteEntry( s->vStamp, xSAT_Lit2Var( Lit ), ( int ) s->nStamp );

        nb = 0;
        for ( pWatcher = begin; pWatcher < end; pWatcher++ )
        {
            int ImpLit = pWatcher->Blocker;

            if ( ( unsigned ) Vec_IntEntry( s->vStamp, xSAT_Lit2Var( ImpLit ) ) == s->nStamp && Vec_StrEntry( s->vAssigns, xSAT_Lit2Var( ImpLit ) ) == xSAT_LitSign( ImpLit ) )
            {
                nb++;
                Vec_IntWriteEntry( s->vStamp, xSAT_Lit2Var( ImpLit ), ( int )( s->nStamp - 1 ) );
            }
        }

        l = Vec_IntSize( vLits ) - 1;
        if ( nb > 0 )
        {
            for ( i = 1; i < Vec_IntSize( vLits ) - nb; i++ )
                if ( ( unsigned ) Vec_IntEntry( s->vStamp, xSAT_Lit2Var( pLits[i] ) ) != s->nStamp )
                {
                    int TempLit = pLits[l];
                    pLits[l] = pLits[i];
                    pLits[i] = TempLit;
                    i--; l--;
                }

            Vec_IntShrink( vLits, Vec_IntSize( vLits ) - nb );
        }
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static void xSAT_SolverAnalyze( xSAT_Solver_t* s, unsigned ConfCRef, Vec_Int_t * vLearnt, int * OutBtLevel, unsigned * nLBD )
{
    int * trail = Vec_IntArray( s->vTrail );
    int Count   = 0;
    int p       = LitUndef;
    int Idx     = Vec_IntSize( s->vTrail ) - 1;
    int * Lits;
    int Lit;
    int i, j;

    Vec_IntPush( vLearnt, LitUndef );
    do
    {
        xSAT_Clause_t * pCla;

        assert( ConfCRef != CRefUndef );
        pCla = xSAT_SolverReadClause(s, ConfCRef);
        Lits = &( pCla->pData[0].Lit );

        if( p != LitUndef && pCla->nSize == 2 && Vec_StrEntry( s->vAssigns, xSAT_Lit2Var( Lits[0] ) ) == xSAT_LitSign( xSAT_NegLit( Lits[0] ) ) )
        {
            assert( Vec_StrEntry( s->vAssigns, xSAT_Lit2Var( Lits[1] ) ) == xSAT_LitSign( ( Lits[1] ) ) );
            ABC_SWAP( int, Lits[0], Lits[1] );
        }

        if ( pCla->fLearnt )
            xSAT_SolverClaActBump( s, pCla );

        if ( pCla->fLearnt && pCla->nLBD > 2 )
        {
            unsigned int nLevels = xSAT_SolverClaCalcLBD( s, pCla );
            if ( nLevels + 1 < pCla->nLBD )
            {
                if ( pCla->nLBD <= s->Config.nLBDFrozenClause )
                    pCla->fCanBeDel = 0;
                pCla->nLBD = nLevels;
            }
        }

        for ( j = ( p == LitUndef ? 0 : 1 ); j < pCla->nSize; j++ )
        {
            int Var = xSAT_Lit2Var( Lits[j] );

            if ( Vec_StrEntry( s->vSeen, Var ) == 0 && Vec_IntEntry( s->vLevels, Var ) > 0 )
            {
                Vec_StrWriteEntry( s->vSeen, Var, 1 );
                xSAT_SolverVarActBump( s, Var );
                if ( Vec_IntEntry( s->vLevels, Var ) >= xSAT_SolverDecisionLevel( s ) )
                {
                    Count++;
                    if (  Vec_IntEntry( s->vReasons, Var ) != CRefUndef && xSAT_SolverReadClause( s, Vec_IntEntry( s->vReasons, Var ) )->fLearnt )
                        Vec_IntPush( s->vLastDLevel, Var );
                }
                else
                    Vec_IntPush( vLearnt, Lits[j] );
            }
        }

        while ( !Vec_StrEntry( s->vSeen, xSAT_Lit2Var( trail[Idx--] ) ) );

        // Next clause to look at
        p = trail[Idx+1];
        ConfCRef = ( unsigned ) Vec_IntEntry( s->vReasons, xSAT_Lit2Var( p ) );
        Vec_StrWriteEntry( s->vSeen, xSAT_Lit2Var( p ), 0 );
        Count--;

    } while ( Count > 0 );

    Vec_IntArray( vLearnt )[0] = xSAT_NegLit( p );
    xSAT_SolverClaMinimisation( s, vLearnt );

    // Find the backtrack level
    Lits = Vec_IntArray( vLearnt );
    if ( Vec_IntSize( vLearnt ) == 1 )
        *OutBtLevel = 0;
    else
    {
        int iMax = 1;
        int Max   = Vec_IntEntry( s->vLevels, xSAT_Lit2Var( Lits[1] ) );
        int Tmp;

        for (i = 2; i < Vec_IntSize( vLearnt ); i++)
            if ( Vec_IntEntry( s->vLevels, xSAT_Lit2Var( Lits[i]) ) > Max)
            {
                Max   = Vec_IntEntry( s->vLevels, xSAT_Lit2Var( Lits[i]) );
                iMax = i;
            }

        Tmp         = Lits[1];
        Lits[1]     = Lits[iMax];
        Lits[iMax]  = Tmp;
        *OutBtLevel = Vec_IntEntry( s->vLevels, xSAT_Lit2Var( Lits[1] ) );
    }

    *nLBD = xSAT_SolverClaCalcLBD2( s, vLearnt );
    if ( Vec_IntSize( s->vLastDLevel ) > 0 )
    {
        int Var;
        Vec_IntForEachEntry( s->vLastDLevel, Var, i )
        {
            if ( xSAT_SolverReadClause( s, Vec_IntEntry( s->vReasons, Var ) )->nLBD < *nLBD )
                xSAT_SolverVarActBump( s, Var );
        }

        Vec_IntClear( s->vLastDLevel );
    }

    Vec_IntForEachEntry( s->vTagged, Lit, i )
        Vec_StrWriteEntry( s->vSeen, xSAT_Lit2Var( Lit ), 0 );
    Vec_IntClear( s->vTagged );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned xSAT_SolverPropagate( xSAT_Solver_t* s )
{
    unsigned hConfl = CRefUndef;
    int * Lits;
    int NegLit;
    int nProp = 0;

    while ( s->iQhead < Vec_IntSize( s->vTrail ) )
    {
        int p = Vec_IntEntry( s->vTrail, s->iQhead++ );
        xSAT_WatchList_t* ws = xSAT_VecWatchListEntry( s->vBinWatches, p );
        xSAT_Watcher_t* begin = xSAT_WatchListArray( ws );
        xSAT_Watcher_t* end   = begin + xSAT_WatchListSize( ws );
        xSAT_Watcher_t *i, *j;

        nProp++;
        for ( i = begin; i < end; i++ )
        {
            if ( Vec_StrEntry( s->vAssigns, xSAT_Lit2Var( i->Blocker ) ) == VarX )
                xSAT_SolverEnqueue( s, i->Blocker, i->CRef );
            else if ( Vec_StrEntry( s->vAssigns, xSAT_Lit2Var( i->Blocker ) ) == xSAT_LitSign( xSAT_NegLit( i->Blocker ) ) )
                return i->CRef;
        }

        ws    = xSAT_VecWatchListEntry( s->vWatches, p );
        begin = xSAT_WatchListArray( ws );
        end   = begin + xSAT_WatchListSize( ws );

        for ( i = j = begin; i < end; )
        {
            xSAT_Clause_t * pCla;
            xSAT_Watcher_t w;
            if ( Vec_StrEntry( s->vAssigns, xSAT_Lit2Var( i->Blocker ) ) == xSAT_LitSign( i->Blocker ) )
            {
                *j++ = *i++;
                continue;
            }

            pCla = xSAT_SolverReadClause( s, i->CRef );
            Lits = &( pCla->pData[0].Lit );

            // Make sure the false literal is data[1]:
            NegLit = xSAT_NegLit( p );
            if ( Lits[0] == NegLit )
            {
                Lits[0] = Lits[1];
                Lits[1] = NegLit;
            }
            assert( Lits[1] == NegLit );

            w.CRef = i->CRef;
            w.Blocker = Lits[0];

            // If 0th watch is true, then clause is already satisfied.
            if ( Lits[0] != i->Blocker && Vec_StrEntry( s->vAssigns, xSAT_Lit2Var( Lits[0] ) ) == xSAT_LitSign( Lits[0] ) )
                *j++ = w;
            else
            {
                // Look for new watch:
                int * stop = Lits + pCla->nSize;
                int * k;
                for ( k = Lits + 2; k < stop; k++ )
                {
                    if (Vec_StrEntry( s->vAssigns, xSAT_Lit2Var( *k ) ) != !xSAT_LitSign( *k ) )
                    {
                        Lits[1] = *k;
                        *k = NegLit;
                        xSAT_WatchListPush( xSAT_VecWatchListEntry( s->vWatches, xSAT_NegLit( Lits[1] ) ), w );
                        goto next;
                    }
                }

                *j++ = w;

                // Clause is unit under assignment:
                if (Vec_StrEntry( s->vAssigns, xSAT_Lit2Var( Lits[0] ) ) == xSAT_LitSign( xSAT_NegLit( Lits[0] ) ) )
                {
                    hConfl = i->CRef;
                    i++;
                    s->iQhead = Vec_IntSize( s->vTrail );
                    // Copy the remaining watches:
                    while (i < end)
                        *j++ = *i++;
                }
                else
                    xSAT_SolverEnqueue( s, Lits[0], i->CRef );
            }
        next:
            i++;
        }

        s->Stats.nInspects += j - xSAT_WatchListArray( ws );
        xSAT_WatchListShrink( ws, j - xSAT_WatchListArray( ws ) );
    }

    s->Stats.nPropagations += nProp;
    s->nPropSimplify -= nProp;

    return hConfl;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void xSAT_SolverReduceDB( xSAT_Solver_t * s )
{
    static abctime TimeTotal = 0;
    abctime clk = Abc_Clock();
    int nLearnedOld = Vec_IntSize( s->vLearnts );
    int i, limit;
    unsigned CRef;
    xSAT_Clause_t * pCla;
    xSAT_Clause_t ** learnts_cls;

    learnts_cls = ABC_ALLOC( xSAT_Clause_t *, nLearnedOld );
    Vec_IntForEachEntry( s->vLearnts, CRef, i )
        learnts_cls[i] = xSAT_SolverReadClause(s, CRef);

    limit = nLearnedOld / 2;

    xSAT_UtilSort((void **) learnts_cls, nLearnedOld,
                  (int (*)( const void *, const void * )) xSAT_ClauseCompare);

    if ( learnts_cls[nLearnedOld / 2]->nLBD <= 3 )
        s->nRC2 += s->Config.nSpecialIncReduce;
    if ( learnts_cls[nLearnedOld - 1]->nLBD <= 5 )
        s->nRC2 += s->Config.nSpecialIncReduce;

    Vec_IntClear( s->vLearnts );
    for ( i = 0; i < nLearnedOld; i++ )
    {
        unsigned CRef;

        pCla = learnts_cls[i];
        CRef = xSAT_MemCRef( s->pMemory, ( unsigned * ) pCla );
        assert( pCla->fMark == 0 );
        if ( pCla->fCanBeDel && pCla->nLBD > 2 && pCla->nSize > 2 && (unsigned) Vec_IntEntry( s->vReasons, xSAT_Lit2Var( pCla->pData[0].Lit ) ) != CRef && ( i < limit ) )
        {
            pCla->fMark = 1;
            s->Stats.nLearntLits -= pCla->nSize;
            xSAT_WatchListRemove( xSAT_VecWatchListEntry( s->vWatches, xSAT_NegLit( pCla->pData[0].Lit ) ), CRef );
            xSAT_WatchListRemove( xSAT_VecWatchListEntry( s->vWatches, xSAT_NegLit( pCla->pData[1].Lit ) ), CRef );
        }
        else
        {
            if ( !pCla->fCanBeDel )
                limit++;
            pCla->fCanBeDel = 1;
            Vec_IntPush( s->vLearnts, CRef );
        }
    }
    ABC_FREE( learnts_cls );

    TimeTotal += Abc_Clock() - clk;
    if ( s->Config.fVerbose )
    {
        Abc_Print(1, "reduceDB: Keeping %7d out of %7d clauses (%5.2f %%)  ",
            Vec_IntSize( s->vLearnts ), nLearnedOld, 100.0 * Vec_IntSize( s->vLearnts ) / nLearnedOld );
        Abc_PrintTime( 1, "Time", TimeTotal );
    }
    xSAT_SolverGarbageCollect(s);
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
char xSAT_SolverSearch( xSAT_Solver_t * s )
{
    iword conflictC  = 0;

    s->Stats.nStarts++;
    for (;;)
    {
        unsigned hConfl = xSAT_SolverPropagate( s );

        if ( hConfl != CRefUndef )
        {
            /* Conflict */
            int BacktrackLevel;
            unsigned nLBD;
            unsigned CRef;

            s->Stats.nConflicts++;
            conflictC++;

            if ( xSAT_SolverDecisionLevel( s ) == 0 )
                return LBoolFalse;

            xSAT_BQueuePush( s->bqTrail, Vec_IntSize( s->vTrail ) );
            if ( s->Stats.nConflicts > s->Config.nFirstBlockRestart && xSAT_BQueueIsValid( s->bqLBD ) && ( Vec_IntSize( s->vTrail ) > ( s->Config.R * ( iword ) xSAT_BQueueAvg( s->bqTrail ) ) ) )
                xSAT_BQueueClean(s->bqLBD);

            Vec_IntClear( s->vLearntClause );
            xSAT_SolverAnalyze( s, hConfl, s->vLearntClause, &BacktrackLevel, &nLBD );

            s->nSumLBD += nLBD;
            xSAT_BQueuePush( s->bqLBD, nLBD );
            xSAT_SolverCancelUntil( s, BacktrackLevel );

            CRef = Vec_IntSize( s->vLearntClause ) == 1 ? CRefUndef : xSAT_SolverClaNew( s, s->vLearntClause , 1 );
            xSAT_SolverEnqueue( s, Vec_IntEntry( s->vLearntClause , 0 ), CRef );

            xSAT_SolverVarActDecay( s );
            xSAT_SolverClaActDecay( s );
        }
        else
        {
            /* No conflict */
            int NextVar;
            if ( xSAT_BQueueIsValid( s->bqLBD ) && ( ( ( iword )xSAT_BQueueAvg( s->bqLBD ) * s->Config.K ) > ( s->nSumLBD / s->Stats.nConflicts ) ) )
            {
                xSAT_BQueueClean( s->bqLBD );
                xSAT_SolverCancelUntil( s, 0 );
                return LBoolUndef;
            }

            // Simplify the set of problem clauses:
            if ( xSAT_SolverDecisionLevel( s ) == 0 )
                xSAT_SolverSimplify( s );

            // Reduce the set of learnt clauses:
            if ( s->Stats.nConflicts >= s->nConfBeforeReduce )
            {
                s->nRC1 = ( s->Stats.nConflicts / s->nRC2 ) + 1;
                xSAT_SolverReduceDB(s);
                s->nRC2 += s->Config.nIncReduce;
                s->nConfBeforeReduce = s->nRC1 * s->nRC2;
            }

            // New variable decision:
            NextVar = xSAT_SolverDecide( s );

            if ( NextVar == VarUndef )
                return LBoolTrue;

            xSAT_SolverNewDecision( s, xSAT_Var2Lit( NextVar, ( int ) Vec_StrEntry( s->vPolarity, NextVar ) ) );
        }
    }

    return LBoolUndef; // cannot happen
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void xSAT_SolverClaRealloc( xSAT_Mem_t * pDest, xSAT_Mem_t * pSrc, unsigned * pCRef )
{
    unsigned nNewCRef;
    xSAT_Clause_t * pNewCla;
    xSAT_Clause_t * pOldCla = xSAT_MemClauseHand( pSrc, *pCRef );

    if ( pOldCla->fReallocd )
    {
        *pCRef = ( unsigned ) pOldCla->nSize;
        return;
    }
    nNewCRef = xSAT_MemAppend( pDest, 3 + pOldCla->fLearnt + pOldCla->nSize );
    pNewCla = xSAT_MemClauseHand( pDest, nNewCRef );
    memcpy( pNewCla, pOldCla, (size_t)(( 3 + pOldCla->fLearnt + pOldCla->nSize ) * 4) );
    pOldCla->fReallocd = 1;
    pOldCla->nSize = ( unsigned ) nNewCRef;
    *pCRef = nNewCRef;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void xSAT_SolverGarbageCollect( xSAT_Solver_t * s )
{
    int i;
    unsigned * pArray;
    xSAT_Mem_t * pNewMemMngr = xSAT_MemAlloc(  xSAT_MemCap( s->pMemory ) - xSAT_MemWastedCap( s->pMemory ) );

    for ( i = 0; i < 2 * Vec_StrSize( s->vAssigns ); i++ )
    {
        xSAT_WatchList_t* ws = xSAT_VecWatchListEntry( s->vWatches, i);
        xSAT_Watcher_t* begin = xSAT_WatchListArray(ws);
        xSAT_Watcher_t* end   = begin + xSAT_WatchListSize(ws);
        xSAT_Watcher_t *w;

        for ( w = begin; w != end; w++ )
            xSAT_SolverClaRealloc( pNewMemMngr, s->pMemory, &(w->CRef) );

        ws = xSAT_VecWatchListEntry( s->vBinWatches, i);
        begin = xSAT_WatchListArray(ws);
        end   = begin + xSAT_WatchListSize(ws);
        for ( w = begin; w != end; w++ )
            xSAT_SolverClaRealloc( pNewMemMngr, s->pMemory, &(w->CRef) );
    }

    for ( i = 0; i < Vec_IntSize( s->vTrail ); i++ )
        if ( ( unsigned ) Vec_IntEntry( s->vReasons, xSAT_Lit2Var( Vec_IntEntry( s->vTrail, i ) ) ) != CRefUndef )
            xSAT_SolverClaRealloc( pNewMemMngr, s->pMemory, ( unsigned * ) &( Vec_IntArray( s->vReasons )[xSAT_Lit2Var( Vec_IntEntry( s->vTrail, i ) )] ) );

    pArray = ( unsigned * ) Vec_IntArray( s->vLearnts );
    for ( i = 0; i < Vec_IntSize( s->vLearnts ); i++ )
        xSAT_SolverClaRealloc( pNewMemMngr, s->pMemory, &(pArray[i]) );

    pArray = ( unsigned * ) Vec_IntArray( s->vClauses );
    for ( i = 0; i < Vec_IntSize( s->vClauses ); i++ )
        xSAT_SolverClaRealloc( pNewMemMngr, s->pMemory, &(pArray[i]) );

    xSAT_MemFree( s->pMemory );
    s->pMemory = pNewMemMngr;
}

ABC_NAMESPACE_IMPL_END
