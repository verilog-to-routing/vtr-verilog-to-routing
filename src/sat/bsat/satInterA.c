/**CFile****************************************************************

  FileName    [satInter.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT sat_solver.]

  Synopsis    [Interpolation package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: satInter.c,v 1.4 2005/09/16 22:55:03 casem Exp $]

***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "satStore.h"
#include "aig/aig/aig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// variable assignments 
static const lit    LIT_UNDEF = 0xffffffff;

// interpolation manager
struct Inta_Man_t_
{
    // clauses of the problems
    Sto_Man_t *     pCnf;         // the set of CNF clauses for A and B
    Vec_Int_t *     vVarsAB;      // the array of global variables
    // various parameters
    int             fVerbose;     // verbosiness flag
    int             fProofVerif;  // verifies the proof
    int             fProofWrite;  // writes the proof file
    int             nVarsAlloc;   // the allocated size of var arrays
    int             nClosAlloc;   // the allocated size of clause arrays
    // internal BCP
    int             nRootSize;    // the number of root level assignments
    int             nTrailSize;   // the number of assignments made 
    lit *           pTrail;       // chronological order of assignments (size nVars)
    lit *           pAssigns;     // assignments by variable (size nVars) 
    char *          pSeens;       // temporary mark (size nVars)
    Sto_Cls_t **    pReasons;     // reasons for each assignment (size nVars)          
    Sto_Cls_t **    pWatches;     // watched clauses for each literal (size 2*nVars)          
    // interpolation data
    Aig_Man_t *     pAig;         // the AIG manager for recording the interpolant
    int *           pVarTypes;    // variable type (size nVars) [1=A, 0=B, <0=AB]
    Aig_Obj_t **    pInters;      // storage for interpolants as truth tables (size nClauses)
    int             nIntersAlloc; // the allocated size of truth table array
    // proof recording
    int             Counter;      // counter of resolved clauses
    int *           pProofNums;   // the proof numbers for each clause (size nClauses)
    FILE *          pFile;        // the file for proof recording
    // internal verification
    Vec_Int_t *     vResLits;
    // runtime stats
    abctime         timeBcp;      // the runtime for BCP
    abctime         timeTrace;    // the runtime of trace construction
    abctime         timeTotal;    // the total runtime of interpolation
};

// procedure to get hold of the clauses' truth table 
static inline Aig_Obj_t ** Inta_ManAigRead( Inta_Man_t * pMan, Sto_Cls_t * pCls )                   { return pMan->pInters + pCls->Id;          }
static inline void         Inta_ManAigClear( Inta_Man_t * pMan, Aig_Obj_t ** p )                    { *p = Aig_ManConst0(pMan->pAig);           }
static inline void         Inta_ManAigFill( Inta_Man_t * pMan, Aig_Obj_t ** p )                     { *p = Aig_ManConst1(pMan->pAig);           }
static inline void         Inta_ManAigCopy( Inta_Man_t * pMan, Aig_Obj_t ** p, Aig_Obj_t ** q )     { *p = *q;                                  }
static inline void         Inta_ManAigAnd( Inta_Man_t * pMan, Aig_Obj_t ** p, Aig_Obj_t ** q )      { *p = Aig_And(pMan->pAig, *p, *q);         }
static inline void         Inta_ManAigOr( Inta_Man_t * pMan, Aig_Obj_t ** p, Aig_Obj_t ** q )       { *p = Aig_Or(pMan->pAig, *p, *q);          }
static inline void         Inta_ManAigOrNot( Inta_Man_t * pMan, Aig_Obj_t ** p, Aig_Obj_t ** q )    { *p = Aig_Or(pMan->pAig, *p, Aig_Not(*q)); }
static inline void         Inta_ManAigOrVar( Inta_Man_t * pMan, Aig_Obj_t ** p, int v )             { *p = Aig_Or(pMan->pAig, *p, Aig_IthVar(pMan->pAig, v));          }
static inline void         Inta_ManAigOrNotVar( Inta_Man_t * pMan, Aig_Obj_t ** p, int v )          { *p = Aig_Or(pMan->pAig, *p, Aig_Not(Aig_IthVar(pMan->pAig, v))); }

// reading/writing the proof for a clause
static inline int          Inta_ManProofGet( Inta_Man_t * p, Sto_Cls_t * pCls )                  { return p->pProofNums[pCls->Id];           }
static inline void         Inta_ManProofSet( Inta_Man_t * p, Sto_Cls_t * pCls, int n )           { p->pProofNums[pCls->Id] = n;              }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocate proof manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Inta_Man_t * Inta_ManAlloc()
{
    Inta_Man_t * p;
    // allocate the manager
    p = (Inta_Man_t *)ABC_ALLOC( char, sizeof(Inta_Man_t) );
    memset( p, 0, sizeof(Inta_Man_t) );
    // verification
    p->vResLits = Vec_IntAlloc( 1000 );
    // parameters
    p->fProofWrite = 0;
    p->fProofVerif = 1;
    return p;    
}

/**Function*************************************************************

  Synopsis    [Count common variables in the clauses of A and B.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Inta_ManGlobalVars( Inta_Man_t * p )
{
    Sto_Cls_t * pClause;
    int LargeNum = -100000000;
    int Var, nVarsAB, v;

    // mark the variable encountered in the clauses of A
    Sto_ManForEachClauseRoot( p->pCnf, pClause )
    {
        if ( !pClause->fA )
            break;
        for ( v = 0; v < (int)pClause->nLits; v++ )
            p->pVarTypes[lit_var(pClause->pLits[v])] = 1;
    }

    // check variables that appear in clauses of B
    nVarsAB = 0;
    Sto_ManForEachClauseRoot( p->pCnf, pClause )
    {
        if ( pClause->fA )
            continue;
        for ( v = 0; v < (int)pClause->nLits; v++ )
        {
            Var = lit_var(pClause->pLits[v]);
            if ( p->pVarTypes[Var] == 1 ) // var of A
            {
                // change it into a global variable
                nVarsAB++;
                p->pVarTypes[Var] = LargeNum;
            }
        }
    }
    assert( nVarsAB <= Vec_IntSize(p->vVarsAB) );

    // order global variables
    nVarsAB = 0;
    Vec_IntForEachEntry( p->vVarsAB, Var, v )
        p->pVarTypes[Var] = -(1+nVarsAB++);

    // check that there is no extra global variables
    for ( v = 0; v < p->pCnf->nVars; v++ )
        assert( p->pVarTypes[v] != LargeNum );
    return nVarsAB;
}

/**Function*************************************************************

  Synopsis    [Resize proof manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Inta_ManResize( Inta_Man_t * p )
{
    p->Counter = 0;
    // check if resizing is needed
    if ( p->nVarsAlloc < p->pCnf->nVars )
    {
        // find the new size
        if ( p->nVarsAlloc == 0 )
            p->nVarsAlloc = 1;
        while ( p->nVarsAlloc < p->pCnf->nVars ) 
            p->nVarsAlloc *= 2;
        // resize the arrays
        p->pTrail    = ABC_REALLOC(lit,         p->pTrail,    p->nVarsAlloc );
        p->pAssigns  = ABC_REALLOC(lit,         p->pAssigns,  p->nVarsAlloc );
        p->pSeens    = ABC_REALLOC(char,        p->pSeens,    p->nVarsAlloc );
        p->pVarTypes = ABC_REALLOC(int,         p->pVarTypes, p->nVarsAlloc );
        p->pReasons  = ABC_REALLOC(Sto_Cls_t *, p->pReasons,  p->nVarsAlloc );
        p->pWatches  = ABC_REALLOC(Sto_Cls_t *, p->pWatches,  p->nVarsAlloc*2 );
    }

    // clean the free space
    memset( p->pAssigns , 0xff, sizeof(lit) * p->pCnf->nVars );
    memset( p->pSeens   , 0,    sizeof(char) * p->pCnf->nVars );
    memset( p->pVarTypes, 0,    sizeof(int) * p->pCnf->nVars );
    memset( p->pReasons , 0,    sizeof(Sto_Cls_t *) * p->pCnf->nVars );
    memset( p->pWatches , 0,    sizeof(Sto_Cls_t *) * p->pCnf->nVars*2 );

    // compute the number of common variables
    Inta_ManGlobalVars( p );

    // check if resizing of clauses is needed
    if ( p->nClosAlloc < p->pCnf->nClauses )
    {
        // find the new size
        if ( p->nClosAlloc == 0 )
            p->nClosAlloc = 1;
        while ( p->nClosAlloc < p->pCnf->nClauses ) 
            p->nClosAlloc *= 2;
        // resize the arrays
        p->pProofNums = ABC_REALLOC( int, p->pProofNums,  p->nClosAlloc );
    }
    memset( p->pProofNums, 0, sizeof(int) * p->pCnf->nClauses );

    // check if resizing of truth tables is needed
    if ( p->nIntersAlloc < p->pCnf->nClauses )
    {
        p->nIntersAlloc = p->pCnf->nClauses;
        p->pInters = ABC_REALLOC( Aig_Obj_t *, p->pInters, p->nIntersAlloc );
    }
    memset( p->pInters, 0, sizeof(Aig_Obj_t *) * p->pCnf->nClauses );
}

/**Function*************************************************************

  Synopsis    [Deallocate proof manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Inta_ManFree( Inta_Man_t * p )
{
/*
    printf( "Runtime stats:\n" );
ABC_PRT( "BCP     ", p->timeBcp   );
ABC_PRT( "Trace   ", p->timeTrace );
ABC_PRT( "TOTAL   ", p->timeTotal );
*/
    ABC_FREE( p->pInters );
    ABC_FREE( p->pProofNums );
    ABC_FREE( p->pTrail );
    ABC_FREE( p->pAssigns );
    ABC_FREE( p->pSeens );
    ABC_FREE( p->pVarTypes );
    ABC_FREE( p->pReasons );
    ABC_FREE( p->pWatches );
    Vec_IntFree( p->vResLits );
    ABC_FREE( p );
}




/**Function*************************************************************

  Synopsis    [Prints the clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Inta_ManPrintClause( Inta_Man_t * p, Sto_Cls_t * pClause )
{
    int i;
    printf( "Clause ID = %d. Proof = %d. {", pClause->Id, Inta_ManProofGet(p, pClause) );
    for ( i = 0; i < (int)pClause->nLits; i++ )
        printf( " %d", pClause->pLits[i] );
    printf( " }\n" );
}

/**Function*************************************************************

  Synopsis    [Prints the resolvent.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Inta_ManPrintResolvent( Vec_Int_t * vResLits )
{
    int i, Entry;
    printf( "Resolvent: {" );
    Vec_IntForEachEntry( vResLits, Entry, i )
        printf( " %d", Entry );
    printf( " }\n" );
}

/**Function*************************************************************

  Synopsis    [Prints the interpolant for one clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Inta_ManPrintInterOne( Inta_Man_t * p, Sto_Cls_t * pClause )
{
    printf( "Clause %2d :  ", pClause->Id );
//    Extra_PrintBinary___( stdout, Inta_ManAigRead(p, pClause), (1 << p->nVarsAB) );
    printf( "\n" );
}



/**Function*************************************************************

  Synopsis    [Adds one clause to the watcher list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Inta_ManWatchClause( Inta_Man_t * p, Sto_Cls_t * pClause, lit Lit )
{
    assert( lit_check(Lit, p->pCnf->nVars) );
    if ( pClause->pLits[0] == Lit )
        pClause->pNext0 = p->pWatches[lit_neg(Lit)];  
    else
    {
        assert( pClause->pLits[1] == Lit );
        pClause->pNext1 = p->pWatches[lit_neg(Lit)];  
    }
    p->pWatches[lit_neg(Lit)] = pClause;
}


/**Function*************************************************************

  Synopsis    [Records implication.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Inta_ManEnqueue( Inta_Man_t * p, lit Lit, Sto_Cls_t * pReason )
{
    int Var = lit_var(Lit);
    if ( p->pAssigns[Var] != LIT_UNDEF )
        return p->pAssigns[Var] == Lit;
    p->pAssigns[Var] = Lit;
    p->pReasons[Var] = pReason;
    p->pTrail[p->nTrailSize++] = Lit;
//printf( "assigning var %d value %d\n", Var, !lit_sign(Lit) );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Records implication.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Inta_ManCancelUntil( Inta_Man_t * p, int Level )
{
    lit Lit;
    int i, Var;
    for ( i = p->nTrailSize - 1; i >= Level; i-- )
    {
        Lit = p->pTrail[i];
        Var = lit_var( Lit );
        p->pReasons[Var] = NULL;
        p->pAssigns[Var] = LIT_UNDEF;
//printf( "cancelling var %d\n", Var );
    }
    p->nTrailSize = Level;
}

/**Function*************************************************************

  Synopsis    [Propagate one assignment.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Sto_Cls_t * Inta_ManPropagateOne( Inta_Man_t * p, lit Lit )
{
    Sto_Cls_t ** ppPrev, * pCur, * pTemp;
    lit LitF = lit_neg(Lit);
    int i;
    // iterate through the literals
    ppPrev = p->pWatches + Lit;
    for ( pCur = p->pWatches[Lit]; pCur; pCur = *ppPrev )
    {
        // make sure the false literal is in the second literal of the clause
        if ( pCur->pLits[0] == LitF )
        {
            pCur->pLits[0] = pCur->pLits[1];
            pCur->pLits[1] = LitF;
            pTemp = pCur->pNext0;
            pCur->pNext0 = pCur->pNext1;
            pCur->pNext1 = pTemp;
        }
        assert( pCur->pLits[1] == LitF );

        // if the first literal is true, the clause is satisfied
        if ( pCur->pLits[0] == p->pAssigns[lit_var(pCur->pLits[0])] )
        {
            ppPrev = &pCur->pNext1;
            continue;
        }

        // look for a new literal to watch
        for ( i = 2; i < (int)pCur->nLits; i++ )
        {
            // skip the case when the literal is false
            if ( lit_neg(pCur->pLits[i]) == p->pAssigns[lit_var(pCur->pLits[i])] )
                continue;
            // the literal is either true or unassigned - watch it
            pCur->pLits[1] = pCur->pLits[i];
            pCur->pLits[i] = LitF;
            // remove this clause from the watch list of Lit
            *ppPrev = pCur->pNext1;
            // add this clause to the watch list of pCur->pLits[i] (now it is pCur->pLits[1])
            Inta_ManWatchClause( p, pCur, pCur->pLits[1] );
            break;
        }
        if ( i < (int)pCur->nLits ) // found new watch
            continue;

        // clause is unit - enqueue new implication
        if ( Inta_ManEnqueue(p, pCur->pLits[0], pCur) )
        {
            ppPrev = &pCur->pNext1;
            continue;
        }

        // conflict detected - return the conflict clause
        return pCur;
    }
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Propagate the current assignments.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sto_Cls_t * Inta_ManPropagate( Inta_Man_t * p, int Start )
{
    Sto_Cls_t * pClause;
    int i;
    abctime clk = Abc_Clock();
    for ( i = Start; i < p->nTrailSize; i++ )
    {
        pClause = Inta_ManPropagateOne( p, p->pTrail[i] );
        if ( pClause )
        {
p->timeBcp += Abc_Clock() - clk;
            return pClause;
        }
    }
p->timeBcp += Abc_Clock() - clk;
    return NULL;
}


/**Function*************************************************************

  Synopsis    [Writes one root clause into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Inta_ManProofWriteOne( Inta_Man_t * p, Sto_Cls_t * pClause )
{
    Inta_ManProofSet( p, pClause, ++p->Counter );

    if ( p->fProofWrite )
    {
        int v;
        fprintf( p->pFile, "%d", Inta_ManProofGet(p, pClause) );
        for ( v = 0; v < (int)pClause->nLits; v++ )
            fprintf( p->pFile, " %d", lit_print(pClause->pLits[v]) );
        fprintf( p->pFile, " 0 0\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Traces the proof for one clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Inta_ManProofTraceOne( Inta_Man_t * p, Sto_Cls_t * pConflict, Sto_Cls_t * pFinal )
{
    Sto_Cls_t * pReason;
    int i, v, Var, PrevId;
    int fPrint = 0;
    abctime clk = Abc_Clock();

    // collect resolvent literals
    if ( p->fProofVerif )
    {
        Vec_IntClear( p->vResLits );
        for ( i = 0; i < (int)pConflict->nLits; i++ )
            Vec_IntPush( p->vResLits, pConflict->pLits[i] );
    }

    // mark all the variables in the conflict as seen
    for ( v = 0; v < (int)pConflict->nLits; v++ )
        p->pSeens[lit_var(pConflict->pLits[v])] = 1;

    // start the anticedents
//    pFinal->pAntis = Vec_PtrAlloc( 32 );
//    Vec_PtrPush( pFinal->pAntis, pConflict );

    if ( p->pCnf->nClausesA )
        Inta_ManAigCopy( p, Inta_ManAigRead(p, pFinal), Inta_ManAigRead(p, pConflict) );

    // follow the trail backwards
    PrevId = Inta_ManProofGet(p, pConflict);
    for ( i = p->nTrailSize - 1; i >= 0; i-- )
    {
        // skip literals that are not involved
        Var = lit_var(p->pTrail[i]);
        if ( !p->pSeens[Var] )
            continue;
        p->pSeens[Var] = 0;

        // skip literals of the resulting clause
        pReason = p->pReasons[Var];
        if ( pReason == NULL )
            continue;
        assert( p->pTrail[i] == pReason->pLits[0] );

        // add the variables to seen
        for ( v = 1; v < (int)pReason->nLits; v++ )
            p->pSeens[lit_var(pReason->pLits[v])] = 1;

        // record the reason clause
        assert( Inta_ManProofGet(p, pReason) > 0 );
        p->Counter++;
        if ( p->fProofWrite )
            fprintf( p->pFile, "%d * %d %d 0\n", p->Counter, PrevId, Inta_ManProofGet(p, pReason) );
        PrevId = p->Counter;

        if ( p->pCnf->nClausesA )
        {
            if ( p->pVarTypes[Var] == 1 ) // var of A
                Inta_ManAigOr( p, Inta_ManAigRead(p, pFinal), Inta_ManAigRead(p, pReason) );
            else
                Inta_ManAigAnd( p, Inta_ManAigRead(p, pFinal), Inta_ManAigRead(p, pReason) );
        }
 
        // resolve the temporary resolvent with the reason clause
        if ( p->fProofVerif )
        {
            int v1, v2, Entry = -1; 
            if ( fPrint )
                Inta_ManPrintResolvent( p->vResLits );
            // check that the var is present in the resolvent
            Vec_IntForEachEntry( p->vResLits, Entry, v1 )
                if ( lit_var(Entry) == Var )
                    break;
            if ( v1 == Vec_IntSize(p->vResLits) )
                printf( "Recording clause %d: Cannot find variable %d in the temporary resolvent.\n", pFinal->Id, Var );
            if ( Entry != lit_neg(pReason->pLits[0]) )
                printf( "Recording clause %d: The resolved variable %d is in the wrong polarity.\n", pFinal->Id, Var );
            // remove variable v1 from the resolvent
            assert( lit_var(Entry) == Var );
            Vec_IntRemove( p->vResLits, Entry );
            // add variables of the reason clause
            for ( v2 = 1; v2 < (int)pReason->nLits; v2++ )
            {
                Vec_IntForEachEntry( p->vResLits, Entry, v1 )
                    if ( lit_var(Entry) == lit_var(pReason->pLits[v2]) )
                        break;
                // if it is a new variable, add it to the resolvent
                if ( v1 == Vec_IntSize(p->vResLits) ) 
                {
                    Vec_IntPush( p->vResLits, pReason->pLits[v2] );
                    continue;
                }
                // if the variable is the same, the literal should be the same too
                if ( Entry == pReason->pLits[v2] )
                    continue;
                // the literal is different
                printf( "Recording clause %d: Trying to resolve the clause with more than one opposite literal.\n", pFinal->Id );
            }
        }
//        Vec_PtrPush( pFinal->pAntis, pReason );
    }

    // unmark all seen variables
//    for ( i = p->nTrailSize - 1; i >= 0; i-- )
//        p->pSeens[lit_var(p->pTrail[i])] = 0;
    // check that the literals are unmarked
//    for ( i = p->nTrailSize - 1; i >= 0; i-- )
//        assert( p->pSeens[lit_var(p->pTrail[i])] == 0 );

    // use the resulting clause to check the correctness of resolution
    if ( p->fProofVerif )
    {
        int v1, v2, Entry = -1; 
        if ( fPrint )
            Inta_ManPrintResolvent( p->vResLits );
        Vec_IntForEachEntry( p->vResLits, Entry, v1 )
        {
            for ( v2 = 0; v2 < (int)pFinal->nLits; v2++ )
                if ( pFinal->pLits[v2] == Entry )
                    break;
            if ( v2 < (int)pFinal->nLits )
                continue;
            break;
        }
        if ( v1 < Vec_IntSize(p->vResLits) )
        {
            printf( "Recording clause %d: The final resolvent is wrong.\n", pFinal->Id );
            Inta_ManPrintClause( p, pConflict );
            Inta_ManPrintResolvent( p->vResLits );
            Inta_ManPrintClause( p, pFinal );
        }

        // if there are literals in the clause that are not in the resolvent
        // it means that the derived resolvent is stronger than the clause
        // we can replace the clause with the resolvent by removing these literals
        if ( Vec_IntSize(p->vResLits) != (int)pFinal->nLits )
        {
            for ( v1 = 0; v1 < (int)pFinal->nLits; v1++ )
            {
                Vec_IntForEachEntry( p->vResLits, Entry, v2 )
                    if ( pFinal->pLits[v1] == Entry )
                        break;
                if ( v2 < Vec_IntSize(p->vResLits) )
                    continue;
                // remove literal v1 from the final clause
                pFinal->nLits--;
                for ( v2 = v1; v2 < (int)pFinal->nLits; v2++ )
                    pFinal->pLits[v2] = pFinal->pLits[v2+1];
                v1--;
            }
            assert( Vec_IntSize(p->vResLits) == (int)pFinal->nLits );
        }
    }
p->timeTrace += Abc_Clock() - clk;

    // return the proof pointer 
    if ( p->pCnf->nClausesA )
    {
//        Inta_ManPrintInterOne( p, pFinal );
    }
    Inta_ManProofSet( p, pFinal, p->Counter );
    // make sure the same proof ID is not asssigned to two consecutive clauses
    assert( p->pProofNums[pFinal->Id-1] != p->Counter );
//    if ( p->pProofNums[pFinal->Id] == p->pProofNums[pFinal->Id-1] )
//        p->pProofNums[pFinal->Id] = p->pProofNums[pConflict->Id];
    return p->Counter;
}

/**Function*************************************************************

  Synopsis    [Records the proof for one clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Inta_ManProofRecordOne( Inta_Man_t * p, Sto_Cls_t * pClause )
{
    Sto_Cls_t * pConflict;
    int i;

    // empty clause never ends up there
    assert( pClause->nLits > 0 );
    if ( pClause->nLits == 0 )
        printf( "Error: Empty clause is attempted.\n" );

    assert( !pClause->fRoot );
    assert( p->nTrailSize == p->nRootSize );

    // if any of the clause literals are already assumed
    // it means that the clause is redundant and can be skipped
    for ( i = 0; i < (int)pClause->nLits; i++ )
        if ( p->pAssigns[lit_var(pClause->pLits[i])] == pClause->pLits[i] )
            return 1;

    // add assumptions to the trail
    for ( i = 0; i < (int)pClause->nLits; i++ )
        if ( !Inta_ManEnqueue( p, lit_neg(pClause->pLits[i]), NULL ) )
        {
            assert( 0 ); // impossible
            return 0;
        }

    // propagate the assumptions
    pConflict = Inta_ManPropagate( p, p->nRootSize );
    if ( pConflict == NULL )
    {
        assert( 0 ); // cannot prove
        return 0;
    } 

    // skip the clause if it is weaker or the same as the conflict clause
    if ( pClause->nLits >= pConflict->nLits )
    {
        // check if every literal of conflict clause can be found in the given clause
        int j;
        for ( i = 0; i < (int)pConflict->nLits; i++ )
        {
            for ( j = 0; j < (int)pClause->nLits; j++ )
                if ( pConflict->pLits[i] == pClause->pLits[j] )
                    break;
            if ( j == (int)pClause->nLits ) // literal pConflict->pLits[i] is not found
                break;
        }
        if ( i == (int)pConflict->nLits ) // all lits are found
        {
            // undo to the root level
            Inta_ManCancelUntil( p, p->nRootSize );
            return 1;
        }
    }

    // construct the proof
    Inta_ManProofTraceOne( p, pConflict, pClause );

    // undo to the root level
    Inta_ManCancelUntil( p, p->nRootSize );

    // add large clauses to the watched lists
    if ( pClause->nLits > 1 )
    {
        Inta_ManWatchClause( p, pClause, pClause->pLits[0] );
        Inta_ManWatchClause( p, pClause, pClause->pLits[1] );
        return 1;
    }
    assert( pClause->nLits == 1 );

    // if the clause proved is unit, add it and propagate
    if ( !Inta_ManEnqueue( p, pClause->pLits[0], pClause ) )
    {
        assert( 0 ); // impossible
        return 0;
    }

    // propagate the assumption
    pConflict = Inta_ManPropagate( p, p->nRootSize );
    if ( pConflict )
    {
        // construct the proof
        Inta_ManProofTraceOne( p, pConflict, p->pCnf->pEmpty );
//        if ( p->fVerbose )
//            printf( "Found last conflict after adding unit clause number %d!\n", pClause->Id );
        return 0;
    }

    // update the root level
    p->nRootSize = p->nTrailSize;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Propagate the root clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Inta_ManProcessRoots( Inta_Man_t * p )
{
    Sto_Cls_t * pClause;
    int Counter;

    // make sure the root clauses are preceeding the learnt clauses
    Counter = 0;
    Sto_ManForEachClause( p->pCnf, pClause )
    {
        assert( (int)pClause->fA    == (Counter < (int)p->pCnf->nClausesA) );
        assert( (int)pClause->fRoot == (Counter < (int)p->pCnf->nRoots)    );
        Counter++;
    }
    assert( p->pCnf->nClauses == Counter );

    // make sure the last clause if empty
    assert( p->pCnf->pTail->nLits == 0 );

    // go through the root unit clauses
    p->nTrailSize = 0;
    Sto_ManForEachClauseRoot( p->pCnf, pClause )
    {
        // create watcher lists for the root clauses
        if ( pClause->nLits > 1 )
        {
            Inta_ManWatchClause( p, pClause, pClause->pLits[0] );
            Inta_ManWatchClause( p, pClause, pClause->pLits[1] );
        }
        // empty clause and large clauses
        if ( pClause->nLits != 1 )
            continue;
        // unit clause
        assert( lit_check(pClause->pLits[0], p->pCnf->nVars) );
        if ( !Inta_ManEnqueue( p, pClause->pLits[0], pClause ) )
        {
            // detected root level conflict
//            printf( "Error in Inta_ManProcessRoots(): Detected a root-level conflict too early!\n" );
//            assert( 0 );
            // detected root level conflict
            Inta_ManProofTraceOne( p, pClause, p->pCnf->pEmpty );
            if ( p->fVerbose )
                printf( "Found root level conflict!\n" );
            return 0;
        }
    }

    // propagate the root unit clauses
    pClause = Inta_ManPropagate( p, 0 );
    if ( pClause )
    {
        // detected root level conflict
        Inta_ManProofTraceOne( p, pClause, p->pCnf->pEmpty );
        if ( p->fVerbose )
            printf( "Found root level conflict!\n" );
        return 0;
    }

    // set the root level
    p->nRootSize = p->nTrailSize;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Records the proof.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Inta_ManPrepareInter( Inta_Man_t * p )
{
    Sto_Cls_t * pClause;
    int Var, VarAB, v;

    // set interpolants for root clauses
    Sto_ManForEachClauseRoot( p->pCnf, pClause )
    {
        if ( !pClause->fA ) // clause of B
        {
            Inta_ManAigFill( p, Inta_ManAigRead(p, pClause) );
//            Inta_ManPrintInterOne( p, pClause );
            continue;
        }
        // clause of A
        Inta_ManAigClear( p, Inta_ManAigRead(p, pClause) );
        for ( v = 0; v < (int)pClause->nLits; v++ )
        {
            Var = lit_var(pClause->pLits[v]);
            if ( p->pVarTypes[Var] < 0 ) // global var
            {
                VarAB = -p->pVarTypes[Var]-1;
                assert( VarAB >= 0 && VarAB < Vec_IntSize(p->vVarsAB) );
                if ( lit_sign(pClause->pLits[v]) ) // negative var
                    Inta_ManAigOrNotVar( p, Inta_ManAigRead(p, pClause), VarAB );
                else
                    Inta_ManAigOrVar( p, Inta_ManAigRead(p, pClause), VarAB );
            }
        }
//        Inta_ManPrintInterOne( p, pClause );
    }
}

/**Function*************************************************************

  Synopsis    [Computes interpolant for the given CNF.]

  Description [Takes the interpolation manager, the CNF deriving by the SAT
  solver, which includes ClausesA, ClausesB, and learned clauses. Additional
  arguments are the vector of variables common to AB and the verbosiness flag.
  Returns the AIG manager with a single output, containing the interpolant.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Inta_ManInterpolate( Inta_Man_t * p, Sto_Man_t * pCnf, abctime TimeToStop, void * vVarsAB, int fVerbose )
{
    Aig_Man_t * pRes;
    Aig_Obj_t * pObj;
    Sto_Cls_t * pClause;
    int RetValue = 1;
    abctime clkTotal = Abc_Clock();

    if ( TimeToStop && Abc_Clock() > TimeToStop )
        return NULL;

    // check that the CNF makes sense
    assert( pCnf->nVars > 0 && pCnf->nClauses > 0 );
    p->pCnf = pCnf;
    p->fVerbose = fVerbose;
    p->vVarsAB = (Vec_Int_t *)vVarsAB;
    p->pAig = pRes = Aig_ManStart( 10000 );
    Aig_IthVar( p->pAig, Vec_IntSize(p->vVarsAB) - 1 );

    // adjust the manager
    Inta_ManResize( p );

    // prepare the interpolant computation
    Inta_ManPrepareInter( p );

    // construct proof for each clause
    // start the proof
    if ( p->fProofWrite )
    {
        p->pFile = fopen( "proof.cnf_", "w" );
        p->Counter = 0;
    }

    // write the root clauses
    Sto_ManForEachClauseRoot( p->pCnf, pClause )
    {
        Inta_ManProofWriteOne( p, pClause );
        if ( TimeToStop && Abc_Clock() > TimeToStop )
        {
            Aig_ManStop( pRes );
            p->pAig = NULL;
            return NULL;
        }
    }

    // propagate root level assignments
    if ( Inta_ManProcessRoots( p ) )
    {
        // if there is no conflict, consider learned clauses
        Sto_ManForEachClause( p->pCnf, pClause )
        {
            if ( pClause->fRoot )
                continue;
            if ( !Inta_ManProofRecordOne( p, pClause ) )
            {
                RetValue = 0;
                break;
            }
            if ( TimeToStop && Abc_Clock() > TimeToStop )
            {
                Aig_ManStop( pRes );
                p->pAig = NULL;
                return NULL;
            }
        }
    }

    // stop the proof
    if ( p->fProofWrite )
    { 
        fclose( p->pFile );
//        Sat_ProofChecker( "proof.cnf_" );
        p->pFile = NULL;    
    }

    if ( fVerbose )
    {
//        ABC_PRT( "Interpo", Abc_Clock() - clkTotal );
    printf( "Vars = %d. Roots = %d. Learned = %d. Resol steps = %d.  Ave = %.2f.  Mem = %.2f MB  ", 
        p->pCnf->nVars, p->pCnf->nRoots, p->pCnf->nClauses-p->pCnf->nRoots, p->Counter,  
        1.0*(p->Counter-p->pCnf->nRoots)/(p->pCnf->nClauses-p->pCnf->nRoots), 
        1.0*Sto_ManMemoryReport(p->pCnf)/(1<<20) );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clkTotal );
p->timeTotal += Abc_Clock() - clkTotal;
    }

    pObj = *Inta_ManAigRead( p, p->pCnf->pTail );
    Aig_ObjCreateCo( pRes, pObj );
    Aig_ManCleanup( pRes );

    p->pAig = NULL;
    return pRes;
    
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Inta_ManDeriveClauses( Inta_Man_t * pMan, Sto_Man_t * pCnf, int fClausesA )
{
    Aig_Man_t * p;
    Aig_Obj_t * pMiter, * pSum, * pLit;
    Sto_Cls_t * pClause;
    int Var, VarAB, v;
    p = Aig_ManStart( 10000 );
    pMiter = Aig_ManConst1(p);
    Sto_ManForEachClauseRoot( pCnf, pClause )
    {
        if ( fClausesA ^ pClause->fA ) // clause of B
            continue;
        // clause of A
        pSum = Aig_ManConst0(p);
        for ( v = 0; v < (int)pClause->nLits; v++ )
        {
            Var = lit_var(pClause->pLits[v]);
            if ( pMan->pVarTypes[Var] < 0 ) // global var
            {
                VarAB = -pMan->pVarTypes[Var]-1;
                assert( VarAB >= 0 && VarAB < Vec_IntSize(pMan->vVarsAB) );
                pLit = Aig_NotCond( Aig_IthVar(p, VarAB), lit_sign(pClause->pLits[v]) );
            }
            else
                pLit = Aig_NotCond( Aig_IthVar(p, Vec_IntSize(pMan->vVarsAB)+1+Var), lit_sign(pClause->pLits[v]) );
            pSum = Aig_Or( p, pSum, pLit );
        }
        pMiter = Aig_And( p, pMiter, pSum );
    }
    Aig_ObjCreateCo( p, pMiter );
    return p;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

