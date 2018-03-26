/**CFile****************************************************************

  FileName    [xsatSolverAPI.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [xSAT - A SAT solver written in C.
               Read the license file for more info.]

  Synopsis    [Solver external API functions implementation.]

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

#include "xsatSolver.h"

ABC_NAMESPACE_IMPL_START

xSAT_SolverOptions_t DefaultConfig =
{
    1,     //.fVerbose = 1,

    0,     //.nConfLimit = 0,
    0,     //.nInsLimit = 0,
    0,     //.nRuntimeLimit = 0,

    0.8,   //.K = 0.8,
    1.4,   //.R = 1.4,
    10000, //.nFirstBlockRestart = 10000,
    50,    //.nSizeLBDQueue = 50,
    5000,  //.nSizeTrailQueue = 5000,

    2000,  //.nConfFirstReduce = 2000,
    300,   //.nIncReduce = 300,
    1000,  //.nSpecialIncReduce = 1000,

    30     //.nLBDFrozenClause = 30
};


/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
xSAT_Solver_t* xSAT_SolverCreate()
{
    xSAT_Solver_t * s = (xSAT_Solver_t *) ABC_CALLOC( char, sizeof( xSAT_Solver_t ) );
    s->Config = DefaultConfig;

    s->pMemory = xSAT_MemAlloc(0);
    s->vClauses = Vec_IntAlloc(0);
    s->vLearnts = Vec_IntAlloc(0);
    s->vWatches = xSAT_VecWatchListAlloc( 0 );
    s->vBinWatches = xSAT_VecWatchListAlloc( 0 );

    s->vTrailLim = Vec_IntAlloc(0);
    s->vTrail = Vec_IntAlloc( 0 );

    s->vActivity = Vec_IntAlloc( 0 );
    s->hOrder = xSAT_HeapAlloc( s->vActivity );

    s->vPolarity = Vec_StrAlloc( 0 );
    s->vTags = Vec_StrAlloc( 0 );
    s->vAssigns = Vec_StrAlloc( 0 );
    s->vLevels = Vec_IntAlloc( 0 );
    s->vReasons = Vec_IntAlloc( 0 );
    s->vStamp = Vec_IntAlloc( 0 );

    s->vTagged = Vec_IntAlloc(0);
    s->vStack  = Vec_IntAlloc(0);

    s->vSeen = Vec_StrAlloc( 0 );
    s->vLearntClause = Vec_IntAlloc(0);
    s->vLastDLevel = Vec_IntAlloc(0);


    s->bqTrail = xSAT_BQueueNew( s->Config.nSizeTrailQueue );
    s->bqLBD = xSAT_BQueueNew( s->Config.nSizeLBDQueue );

    s->nVarActInc = (1 <<  5);
    s->nClaActInc = (1 << 11);

    s->nConfBeforeReduce = s->Config.nConfFirstReduce;
    s->nRC1 = 1;
    s->nRC2 = s->Config.nConfFirstReduce;
    return s;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void xSAT_SolverDestroy( xSAT_Solver_t * s )
{
    xSAT_MemFree( s->pMemory );
    Vec_IntFree( s->vClauses );
    Vec_IntFree( s->vLearnts );
    xSAT_VecWatchListFree( s->vWatches );
    xSAT_VecWatchListFree( s->vBinWatches );

    xSAT_HeapFree(s->hOrder);
    Vec_IntFree( s->vTrailLim );
    Vec_IntFree( s->vTrail );
    Vec_IntFree( s->vTagged );
    Vec_IntFree( s->vStack );

    Vec_StrFree( s->vSeen );
    Vec_IntFree( s->vLearntClause );
    Vec_IntFree( s->vLastDLevel );

    Vec_IntFree( s->vActivity );
    Vec_StrFree( s->vPolarity );
    Vec_StrFree( s->vTags );
    Vec_StrFree( s->vAssigns );
    Vec_IntFree( s->vLevels );
    Vec_IntFree( s->vReasons );
    Vec_IntFree( s->vStamp );

    xSAT_BQueueFree(s->bqLBD);
    xSAT_BQueueFree(s->bqTrail);

    ABC_FREE(s);
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int xSAT_SolverSimplify( xSAT_Solver_t * s )
{
    int i, j;
    unsigned CRef;
    assert( xSAT_SolverDecisionLevel(s) == 0 );

    if ( xSAT_SolverPropagate(s) != CRefUndef )
        return false;

    if ( s->nAssignSimplify == Vec_IntSize( s->vTrail ) || s->nPropSimplify > 0 )
        return true;

    j = 0;
    Vec_IntForEachEntry( s->vClauses, CRef, i )
    {
        xSAT_Clause_t * pCla = xSAT_SolverReadClause( s, CRef );
        if ( xSAT_SolverIsClauseSatisfied( s, pCla ) )
        {
            pCla->fMark = 1;
            s->Stats.nClauseLits -= pCla->nSize;

            if ( pCla->nSize == 2 )
            {
                xSAT_WatchListRemove( xSAT_VecWatchListEntry( s->vBinWatches, xSAT_NegLit(pCla->pData[0].Lit) ), CRef );
                xSAT_WatchListRemove( xSAT_VecWatchListEntry( s->vBinWatches, xSAT_NegLit(pCla->pData[1].Lit) ), CRef );
            }
            else
            {
                xSAT_WatchListRemove( xSAT_VecWatchListEntry( s->vWatches, xSAT_NegLit(pCla->pData[0].Lit) ), CRef );
                xSAT_WatchListRemove( xSAT_VecWatchListEntry( s->vWatches, xSAT_NegLit(pCla->pData[1].Lit) ), CRef );
            }
        }
        else
            Vec_IntWriteEntry( s->vClauses, j++, CRef );
    }
    Vec_IntShrink( s->vClauses, j );
    xSAT_SolverRebuildOrderHeap( s );

    s->nAssignSimplify = Vec_IntSize( s->vTrail );
    s->nPropSimplify = s->Stats.nClauseLits + s->Stats.nLearntLits;

    return true;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void xSAT_SolverAddVariable( xSAT_Solver_t* s, int Sign )
{
    int Var = Vec_IntSize( s->vActivity );

    xSAT_VecWatchListPush( s->vWatches );
    xSAT_VecWatchListPush( s->vWatches );
    xSAT_VecWatchListPush( s->vBinWatches );
    xSAT_VecWatchListPush( s->vBinWatches );

    Vec_IntPush( s->vActivity, 0 );
    Vec_IntPush( s->vLevels, 0 );
    Vec_StrPush( s->vAssigns, VarX );
    Vec_StrPush( s->vPolarity, 1 );
    Vec_StrPush( s->vTags, 0 );
    Vec_IntPush( s->vReasons, ( int ) CRefUndef );
    Vec_IntPush( s->vStamp, 0 );
    Vec_StrPush( s->vSeen, 0 );

    xSAT_HeapInsert( s->hOrder, Var );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int xSAT_SolverAddClause( xSAT_Solver_t * s, Vec_Int_t * vLits )
{
    int i, j;
    int Lit, PrevLit;
    int MaxVar;

    Vec_IntSort( vLits, 0 );
    MaxVar = xSAT_Lit2Var( Vec_IntEntryLast( vLits ) );
    while ( MaxVar >= Vec_IntSize( s->vActivity ) )
        xSAT_SolverAddVariable( s, 1 );

    j = 0;
    PrevLit = LitUndef;
    Vec_IntForEachEntry( vLits, Lit, i )
    {
        if ( Lit == xSAT_NegLit( PrevLit ) || Vec_StrEntry( s->vAssigns, xSAT_Lit2Var( Lit ) ) == xSAT_LitSign( Lit ) )
            return true;
        else if ( Lit != PrevLit && Vec_StrEntry( s->vAssigns, xSAT_Lit2Var( Lit ) ) == VarX )
        {
            PrevLit = Lit;
            Vec_IntWriteEntry( vLits, j++, Lit );
        }
    }
    Vec_IntShrink( vLits, j );

    if ( Vec_IntSize( vLits ) == 0 )
        return false;
    if ( Vec_IntSize( vLits ) == 1 )
    {
        xSAT_SolverEnqueue( s, Vec_IntEntry( vLits, 0 ), CRefUndef );
        return ( xSAT_SolverPropagate( s ) == CRefUndef );
    }

    xSAT_SolverClaNew( s, vLits, 0 );
    return true;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int xSAT_SolverSolve( xSAT_Solver_t* s )
{
    char status = LBoolUndef;

    assert(s);
    if ( s->Config.fVerbose )
    {
        printf( "==========================================[ BLACK MAGIC ]================================================\n" );
        printf( "|                                |                                |                                     |\n" );
        printf( "| - Restarts:                    | - Reduce Clause DB:            | - Minimize Asserting:               |\n" );
        printf( "|   * LBD Queue    : %6d      |   * First     : %6d         |    * size < %3d                     |\n",       s->Config.nSizeLBDQueue, s->Config.nConfFirstReduce, 0 );
        printf( "|   * Trail Queue  : %6d      |   * Inc       : %6d         |    * lbd  < %3d                     |\n",       s->Config.nSizeTrailQueue, s->Config.nIncReduce, 0 );
        printf( "|   * K            : %6.2f      |   * Special   : %6d         |                                     |\n",     s->Config.K, s->Config.nSpecialIncReduce );
        printf( "|   * R            : %6.2f      |   * Protected :  (lbd)< %2d     |                                     |\n", s->Config.R, s->Config.nLBDFrozenClause );
        printf( "|                                |                                |                                     |\n" );
        printf( "=========================================================================================================\n" );
    }

    while ( status == LBoolUndef )
        status = xSAT_SolverSearch( s );

    if ( s->Config.fVerbose )
        printf( "=========================================================================================================\n" );

    xSAT_SolverCancelUntil( s, 0 );
    return status;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void xSAT_SolverPrintStats( xSAT_Solver_t * s )
{
    printf( "starts        : %10d\n", s->Stats.nStarts );
    printf( "conflicts     : %10ld\n", s->Stats.nConflicts );
    printf( "decisions     : %10ld\n", s->Stats.nDecisions );
    printf( "propagations  : %10ld\n", s->Stats.nPropagations );
}

ABC_NAMESPACE_IMPL_END
