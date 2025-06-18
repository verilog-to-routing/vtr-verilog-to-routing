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

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// variable assignments 
static const lit    LIT_UNDEF = 0xffffffff;

// interpolation manager
struct Int_Man_t_
{
    // clauses of the problems
    Sto_Man_t *     pCnf;         // the set of CNF clauses for A and B
    int             pGloVars[16]; // global variables
    int             nGloVars;     // the number of global variables
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
    int             nVarsAB;      // the number of global variables
    int *           pVarTypes;    // variable type (size nVars) [1=A, 0=B, <0=AB]
    unsigned *      pInters;      // storage for interpolants as truth tables (size nClauses)
    int             nIntersAlloc; // the allocated size of truth table array
    int             nWords;       // the number of words in the truth table
    // proof recording
    int             Counter;      // counter of resolved clauses
    int *           pProofNums;   // the proof numbers for each clause (size nClauses)
    FILE *          pFile;        // the file for proof recording
    // internal verification
    lit *           pResLits;     // the literals of the resolvent   
    int             nResLits;     // the number of literals of the resolvent
    int             nResLitsAlloc;// the number of literals of the resolvent
    // runtime stats
    abctime         timeBcp;      // the runtime for BCP
    abctime         timeTrace;    // the runtime of trace construction
    abctime         timeTotal;    // the total runtime of interpolation
};

// procedure to get hold of the clauses' truth table 
static inline unsigned * Int_ManTruthRead( Int_Man_t * p, Sto_Cls_t * pCls )          { return p->pInters + pCls->Id * p->nWords;                 }
static inline void       Int_ManTruthClear( unsigned * p, int nWords )                { int i; for ( i = nWords - 1; i >= 0; i-- ) p[i]  =  0;    }
static inline void       Int_ManTruthFill( unsigned * p, int nWords )                 { int i; for ( i = nWords - 1; i >= 0; i-- ) p[i]  = ~0;    }
static inline void       Int_ManTruthCopy( unsigned * p, unsigned * q, int nWords )   { int i; for ( i = nWords - 1; i >= 0; i-- ) p[i]  =  q[i]; }
static inline void       Int_ManTruthAnd( unsigned * p, unsigned * q, int nWords )    { int i; for ( i = nWords - 1; i >= 0; i-- ) p[i] &=  q[i]; }
static inline void       Int_ManTruthOr( unsigned * p, unsigned * q, int nWords )     { int i; for ( i = nWords - 1; i >= 0; i-- ) p[i] |=  q[i]; }
static inline void       Int_ManTruthOrNot( unsigned * p, unsigned * q, int nWords )  { int i; for ( i = nWords - 1; i >= 0; i-- ) p[i] |= ~q[i]; }

// reading/writing the proof for a clause
static inline int        Int_ManProofGet( Int_Man_t * p, Sto_Cls_t * pCls )           { return p->pProofNums[pCls->Id];  }
static inline void       Int_ManProofSet( Int_Man_t * p, Sto_Cls_t * pCls, int n )    { p->pProofNums[pCls->Id] = n;     }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocate proof manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Int_Man_t * Int_ManAlloc()
{
    Int_Man_t * p;
    // allocate the manager
    p = (Int_Man_t *)ABC_ALLOC( char, sizeof(Int_Man_t) );
    memset( p, 0, sizeof(Int_Man_t) );
    // verification
    p->nResLitsAlloc = (1<<16);
    p->pResLits = ABC_ALLOC( lit, p->nResLitsAlloc );
    // parameters
    p->fProofWrite = 0;
    p->fProofVerif = 1;
    return p;    
}

/**Function*************************************************************

  Synopsis    [Allocate proof manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Int_ManSetGlobalVars( Int_Man_t * p, int nGloVars )
{
    p->nGloVars = nGloVars;
    return p->pGloVars;
}

/**Function*************************************************************

  Synopsis    [Count common variables in the clauses of A and B.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Int_ManGlobalVars( Int_Man_t * p )
{
    Sto_Cls_t * pClause;
    int Var, nVarsAB, v;

    // mark the variable encountered in the clauses of A
    Sto_ManForEachClauseRoot( p->pCnf, pClause )
    {
        if ( !pClause->fA )
            break;
        for ( v = 0; v < (int)pClause->nLits; v++ )
            p->pVarTypes[lit_var(pClause->pLits[v])] = 1;
    }

    if ( p->nGloVars )
    {
        for ( v = 0; v < p->nGloVars; v++ )
            p->pVarTypes[ p->pGloVars[v] ] = - v - 1;
        return p->nGloVars;
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
                p->pVarTypes[Var] = -1;
            }
        }
    }

    // order global variables
    nVarsAB = 0;
    for ( v = 0; v < p->pCnf->nVars; v++ )
        if ( p->pVarTypes[v] == -1 )
            p->pVarTypes[v] -= nVarsAB++;
//printf( "There are %d global variables.\n", nVarsAB );
    return nVarsAB;
}

/**Function*************************************************************

  Synopsis    [Resize proof manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Int_ManResize( Int_Man_t * p )
{
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
    p->nVarsAB = Int_ManGlobalVars( p );
    // compute the number of words in the truth table
    p->nWords = (p->nVarsAB <= 5 ? 1 : (1 << (p->nVarsAB - 5)));

    // check if resizing of clauses is needed
    if ( p->nClosAlloc < p->pCnf->nClauses )
    {
        // find the new size
        if ( p->nClosAlloc == 0 )
            p->nClosAlloc = 1;
        while ( p->nClosAlloc < p->pCnf->nClauses ) 
            p->nClosAlloc *= 2;
        // resize the arrays
        p->pProofNums = ABC_REALLOC(int, p->pProofNums,  p->nClosAlloc );
    }
    memset( p->pProofNums, 0, sizeof(int) * p->pCnf->nClauses );

    // check if resizing of truth tables is needed
    if ( p->nIntersAlloc < p->nWords * p->pCnf->nClauses )
    {
        p->nIntersAlloc = p->nWords * p->pCnf->nClauses;
        p->pInters = ABC_REALLOC(unsigned, p->pInters, p->nIntersAlloc );
    }
//    memset( p->pInters, 0, sizeof(unsigned) * p->nWords * p->pCnf->nClauses );
}

/**Function*************************************************************

  Synopsis    [Deallocate proof manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Int_ManFree( Int_Man_t * p )
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
    ABC_FREE( p->pResLits );
    ABC_FREE( p );
}




/**Function*************************************************************

  Synopsis    [Prints the clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Int_ManPrintClause( Int_Man_t * p, Sto_Cls_t * pClause )
{
    int i;
    printf( "Clause ID = %d. Proof = %d. {", pClause->Id, Int_ManProofGet(p, pClause) );
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
void Int_ManPrintResolvent( lit * pResLits, int nResLits )
{
    int i;
    printf( "Resolvent: {" );
    for ( i = 0; i < nResLits; i++ )
        printf( " %d", pResLits[i] );
    printf( " }\n" );
}

/**Function*************************************************************

  Synopsis    [Records the proof.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_PrintBinary__( FILE * pFile, unsigned Sign[], int nBits )
{
    int Remainder, nWords;
    int w, i;

    Remainder = (nBits%(sizeof(unsigned)*8));
    nWords    = (nBits/(sizeof(unsigned)*8)) + (Remainder>0);

    for ( w = nWords-1; w >= 0; w-- )
        for ( i = ((w == nWords-1 && Remainder)? Remainder-1: 31); i >= 0; i-- )
            fprintf( pFile, "%c", '0' + (int)((Sign[w] & (1<<i)) > 0) );
}

/**Function*************************************************************

  Synopsis    [Prints the interpolant for one clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Int_ManPrintInterOne( Int_Man_t * p, Sto_Cls_t * pClause )
{
    printf( "Clause %2d :  ", pClause->Id );
    Extra_PrintBinary__( stdout, Int_ManTruthRead(p, pClause), (1 << p->nVarsAB) );
    printf( "\n" );
}



/**Function*************************************************************

  Synopsis    [Adds one clause to the watcher list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Int_ManWatchClause( Int_Man_t * p, Sto_Cls_t * pClause, lit Lit )
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
static inline int Int_ManEnqueue( Int_Man_t * p, lit Lit, Sto_Cls_t * pReason )
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
static inline void Int_ManCancelUntil( Int_Man_t * p, int Level )
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
static inline Sto_Cls_t * Int_ManPropagateOne( Int_Man_t * p, lit Lit )
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
            Int_ManWatchClause( p, pCur, pCur->pLits[1] );
            break;
        }
        if ( i < (int)pCur->nLits ) // found new watch
            continue;

        // clause is unit - enqueue new implication
        if ( Int_ManEnqueue(p, pCur->pLits[0], pCur) )
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
Sto_Cls_t * Int_ManPropagate( Int_Man_t * p, int Start )
{
    Sto_Cls_t * pClause;
    int i;
    abctime clk = Abc_Clock();
    for ( i = Start; i < p->nTrailSize; i++ )
    {
        pClause = Int_ManPropagateOne( p, p->pTrail[i] );
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
void Int_ManProofWriteOne( Int_Man_t * p, Sto_Cls_t * pClause )
{
    Int_ManProofSet( p, pClause, ++p->Counter );

    if ( p->fProofWrite )
    {
        int v;
        fprintf( p->pFile, "%d", Int_ManProofGet(p, pClause) );
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
int Int_ManProofTraceOne( Int_Man_t * p, Sto_Cls_t * pConflict, Sto_Cls_t * pFinal )
{
    Sto_Cls_t * pReason;
    int i, v, Var, PrevId;
    int fPrint = 0;
    abctime clk = Abc_Clock();

    // collect resolvent literals
    if ( p->fProofVerif )
    {
        assert( (int)pConflict->nLits <= p->nResLitsAlloc );
        memcpy( p->pResLits, pConflict->pLits, sizeof(lit) * pConflict->nLits );
        p->nResLits = pConflict->nLits;
    }

    // mark all the variables in the conflict as seen
    for ( v = 0; v < (int)pConflict->nLits; v++ )
        p->pSeens[lit_var(pConflict->pLits[v])] = 1;

    // start the anticedents
//    pFinal->pAntis = Vec_PtrAlloc( 32 );
//    Vec_PtrPush( pFinal->pAntis, pConflict );

    if ( p->pCnf->nClausesA )
        Int_ManTruthCopy( Int_ManTruthRead(p, pFinal), Int_ManTruthRead(p, pConflict), p->nWords );

    // follow the trail backwards
    PrevId = Int_ManProofGet(p, pConflict);
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
        assert( Int_ManProofGet(p, pReason) > 0 );
        p->Counter++;
        if ( p->fProofWrite )
            fprintf( p->pFile, "%d * %d %d 0\n", p->Counter, PrevId, Int_ManProofGet(p, pReason) );
        PrevId = p->Counter;

        if ( p->pCnf->nClausesA )
        {
            if ( p->pVarTypes[Var] == 1 ) // var of A
                Int_ManTruthOr( Int_ManTruthRead(p, pFinal), Int_ManTruthRead(p, pReason), p->nWords );
            else
                Int_ManTruthAnd( Int_ManTruthRead(p, pFinal), Int_ManTruthRead(p, pReason), p->nWords );
        }
 
        // resolve the temporary resolvent with the reason clause
        if ( p->fProofVerif )
        {
            int v1, v2; 
            if ( fPrint )
                Int_ManPrintResolvent( p->pResLits, p->nResLits );
            // check that the var is present in the resolvent
            for ( v1 = 0; v1 < p->nResLits; v1++ )
                if ( lit_var(p->pResLits[v1]) == Var )
                    break;
            if ( v1 == p->nResLits )
                printf( "Recording clause %d: Cannot find variable %d in the temporary resolvent.\n", pFinal->Id, Var );
            if ( p->pResLits[v1] != lit_neg(pReason->pLits[0]) )
                printf( "Recording clause %d: The resolved variable %d is in the wrong polarity.\n", pFinal->Id, Var );
            // remove this variable from the resolvent
            assert( lit_var(p->pResLits[v1]) == Var );
            p->nResLits--;
            for ( ; v1 < p->nResLits; v1++ )
                p->pResLits[v1] = p->pResLits[v1+1];
            // add variables of the reason clause
            for ( v2 = 1; v2 < (int)pReason->nLits; v2++ )
            {
                for ( v1 = 0; v1 < p->nResLits; v1++ )
                    if ( lit_var(p->pResLits[v1]) == lit_var(pReason->pLits[v2]) )
                        break;
                // if it is a new variable, add it to the resolvent
                if ( v1 == p->nResLits ) 
                {
                    if ( p->nResLits == p->nResLitsAlloc )
                        printf( "Recording clause %d: Ran out of space for intermediate resolvent.\n", pFinal->Id );
                    p->pResLits[ p->nResLits++ ] = pReason->pLits[v2];
                    continue;
                }
                // if the variable is the same, the literal should be the same too
                if ( p->pResLits[v1] == pReason->pLits[v2] )
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
        int v1, v2; 
        if ( fPrint )
            Int_ManPrintResolvent( p->pResLits, p->nResLits );
        for ( v1 = 0; v1 < p->nResLits; v1++ )
        {
            for ( v2 = 0; v2 < (int)pFinal->nLits; v2++ )
                if ( pFinal->pLits[v2] == p->pResLits[v1] )
                    break;
            if ( v2 < (int)pFinal->nLits )
                continue;
            break;
        }
        if ( v1 < p->nResLits )
        {
            printf( "Recording clause %d: The final resolvent is wrong.\n", pFinal->Id );
            Int_ManPrintClause( p, pConflict );
            Int_ManPrintResolvent( p->pResLits, p->nResLits );
            Int_ManPrintClause( p, pFinal );
        }

        // if there are literals in the clause that are not in the resolvent
        // it means that the derived resolvent is stronger than the clause
        // we can replace the clause with the resolvent by removing these literals
        if ( p->nResLits != (int)pFinal->nLits )
        {
            for ( v1 = 0; v1 < (int)pFinal->nLits; v1++ )
            {
                for ( v2 = 0; v2 < p->nResLits; v2++ )
                    if ( pFinal->pLits[v1] == p->pResLits[v2] )
                        break;
                if ( v2 < p->nResLits )
                    continue;
                // remove literal v1 from the final clause
                pFinal->nLits--;
                for ( v2 = v1; v2 < (int)pFinal->nLits; v2++ )
                    pFinal->pLits[v2] = pFinal->pLits[v2+1];
                v1--;
            }
            assert( p->nResLits == (int)pFinal->nLits );
        }
    }
p->timeTrace += Abc_Clock() - clk;

    // return the proof pointer 
    if ( p->pCnf->nClausesA )
    {
//        Int_ManPrintInterOne( p, pFinal );
    }
    Int_ManProofSet( p, pFinal, p->Counter );
    // make sure the same proof ID is not asssigned to two consecutive clauses
    assert( p->pProofNums[pFinal->Id-1] != p->Counter );
    return p->Counter;
}

/**Function*************************************************************

  Synopsis    [Records the proof for one clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Int_ManProofRecordOne( Int_Man_t * p, Sto_Cls_t * pClause )
{
    Sto_Cls_t * pConflict;
    int i;

    // empty clause never ends up there
    assert( pClause->nLits > 0 );
    if ( pClause->nLits == 0 )
        printf( "Error: Empty clause is attempted.\n" );

    // add assumptions to the trail
    assert( !pClause->fRoot );
    assert( p->nTrailSize == p->nRootSize );

    // if any of the clause literals are already assumed
    // it means that the clause is redundant and can be skipped
    for ( i = 0; i < (int)pClause->nLits; i++ )
        if ( p->pAssigns[lit_var(pClause->pLits[i])] == pClause->pLits[i] )
            return 1;

    for ( i = 0; i < (int)pClause->nLits; i++ )
        if ( !Int_ManEnqueue( p, lit_neg(pClause->pLits[i]), NULL ) )
        {
            assert( 0 ); // impossible
            return 0;
        }

    // propagate the assumptions
    pConflict = Int_ManPropagate( p, p->nRootSize );
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
            Int_ManCancelUntil( p, p->nRootSize );
            return 1;
        }
    }

    // construct the proof
    Int_ManProofTraceOne( p, pConflict, pClause );

    // undo to the root level
    Int_ManCancelUntil( p, p->nRootSize );

    // add large clauses to the watched lists
    if ( pClause->nLits > 1 )
    {
        Int_ManWatchClause( p, pClause, pClause->pLits[0] );
        Int_ManWatchClause( p, pClause, pClause->pLits[1] );
        return 1;
    }
    assert( pClause->nLits == 1 );

    // if the clause proved is unit, add it and propagate
    if ( !Int_ManEnqueue( p, pClause->pLits[0], pClause ) )
    {
        assert( 0 ); // impossible
        return 0;
    }

    // propagate the assumption
    pConflict = Int_ManPropagate( p, p->nRootSize );
    if ( pConflict )
    {
        // construct the proof
        Int_ManProofTraceOne( p, pConflict, p->pCnf->pEmpty );
        if ( p->fVerbose )
            printf( "Found last conflict after adding unit clause number %d!\n", pClause->Id );
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
int Int_ManProcessRoots( Int_Man_t * p )
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
            Int_ManWatchClause( p, pClause, pClause->pLits[0] );
            Int_ManWatchClause( p, pClause, pClause->pLits[1] );
        }
        // empty clause and large clauses
        if ( pClause->nLits != 1 )
            continue;
        // unit clause
        assert( lit_check(pClause->pLits[0], p->pCnf->nVars) );
        if ( !Int_ManEnqueue( p, pClause->pLits[0], pClause ) )
        {
            // detected root level conflict
//            printf( "Error in Int_ManProcessRoots(): Detected a root-level conflict too early!\n" );
//            assert( 0 );
            // detected root level conflict
            Int_ManProofTraceOne( p, pClause, p->pCnf->pEmpty );
            if ( p->fVerbose )
                printf( "Found root level conflict!\n" );
            return 0;
        }
    }

    // propagate the root unit clauses
    pClause = Int_ManPropagate( p, 0 );
    if ( pClause )
    {
        // detected root level conflict
        Int_ManProofTraceOne( p, pClause, p->pCnf->pEmpty );
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
void Int_ManPrepareInter( Int_Man_t * p )
{
    // elementary truth tables
    unsigned uTruths[8][8] = {
        { 0xAAAAAAAA,0xAAAAAAAA,0xAAAAAAAA,0xAAAAAAAA,0xAAAAAAAA,0xAAAAAAAA,0xAAAAAAAA,0xAAAAAAAA },
        { 0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC },
        { 0xF0F0F0F0,0xF0F0F0F0,0xF0F0F0F0,0xF0F0F0F0,0xF0F0F0F0,0xF0F0F0F0,0xF0F0F0F0,0xF0F0F0F0 },
        { 0xFF00FF00,0xFF00FF00,0xFF00FF00,0xFF00FF00,0xFF00FF00,0xFF00FF00,0xFF00FF00,0xFF00FF00 },
        { 0xFFFF0000,0xFFFF0000,0xFFFF0000,0xFFFF0000,0xFFFF0000,0xFFFF0000,0xFFFF0000,0xFFFF0000 }, 
        { 0x00000000,0xFFFFFFFF,0x00000000,0xFFFFFFFF,0x00000000,0xFFFFFFFF,0x00000000,0xFFFFFFFF }, 
        { 0x00000000,0x00000000,0xFFFFFFFF,0xFFFFFFFF,0x00000000,0x00000000,0xFFFFFFFF,0xFFFFFFFF }, 
        { 0x00000000,0x00000000,0x00000000,0x00000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF } 
    };
    Sto_Cls_t * pClause;
    int Var, VarAB, v;
    assert( p->nVarsAB <= 8 );

    // set interpolants for root clauses
    Sto_ManForEachClauseRoot( p->pCnf, pClause )
    {
        if ( !pClause->fA ) // clause of B
        {
            Int_ManTruthFill( Int_ManTruthRead(p, pClause), p->nWords );
//            Int_ManPrintInterOne( p, pClause );
            continue;
        }
        // clause of A
        Int_ManTruthClear( Int_ManTruthRead(p, pClause), p->nWords );
        for ( v = 0; v < (int)pClause->nLits; v++ )
        {
            Var = lit_var(pClause->pLits[v]);
            if ( p->pVarTypes[Var] < 0 ) // global var
            {
                VarAB = -p->pVarTypes[Var]-1;
                assert( VarAB >= 0 && VarAB < p->nVarsAB );
                if ( lit_sign(pClause->pLits[v]) ) // negative var
                    Int_ManTruthOrNot( Int_ManTruthRead(p, pClause), uTruths[VarAB], p->nWords );
                else
                    Int_ManTruthOr( Int_ManTruthRead(p, pClause), uTruths[VarAB], p->nWords );
            }
        }
//        Int_ManPrintInterOne( p, pClause );
    }
}

/**Function*************************************************************

  Synopsis    [Computes interpolant for the given CNF.]

  Description [Returns the number of common variable found and interpolant.
  Returns 0, if something did not work.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Int_ManInterpolate( Int_Man_t * p, Sto_Man_t * pCnf, int fVerbose, unsigned ** ppResult )
{
    Sto_Cls_t * pClause;
    int RetValue = 1;
    abctime clkTotal = Abc_Clock();

    // check that the CNF makes sense
    assert( pCnf->nVars > 0 && pCnf->nClauses > 0 );
    p->pCnf = pCnf;
    p->fVerbose = fVerbose;
    *ppResult = NULL;

    // adjust the manager
    Int_ManResize( p );

    // prepare the interpolant computation
    Int_ManPrepareInter( p );

    // construct proof for each clause
    // start the proof
    if ( p->fProofWrite )
    {
        p->pFile = fopen( "proof.cnf_", "w" );
        p->Counter = 0;
    }

    // write the root clauses
    Sto_ManForEachClauseRoot( p->pCnf, pClause )
        Int_ManProofWriteOne( p, pClause );

    // propagate root level assignments
    if ( Int_ManProcessRoots( p ) )
    {
        // if there is no conflict, consider learned clauses
        Sto_ManForEachClause( p->pCnf, pClause )
        {
            if ( pClause->fRoot )
                continue;
            if ( !Int_ManProofRecordOne( p, pClause ) )
            {
                RetValue = 0;
                break;
            }
        }
    }

    // stop the proof
    if ( p->fProofWrite )
    {
        fclose( p->pFile );
        p->pFile = NULL;    
    }

    if ( fVerbose )
    {
    printf( "Vars = %d. Roots = %d. Learned = %d. Resol steps = %d.  Ave = %.2f.  Mem = %.2f MB\n", 
        p->pCnf->nVars, p->pCnf->nRoots, p->pCnf->nClauses-p->pCnf->nRoots, p->Counter,  
        1.0*(p->Counter-p->pCnf->nRoots)/(p->pCnf->nClauses-p->pCnf->nRoots), 
        1.0*Sto_ManMemoryReport(p->pCnf)/(1<<20) );
p->timeTotal += Abc_Clock() - clkTotal;
    }

    *ppResult = Int_ManTruthRead( p, p->pCnf->pTail );
    return p->nVarsAB;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

