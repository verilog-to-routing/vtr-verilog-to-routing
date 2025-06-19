/**CFile****************************************************************

  FileName    [pr.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Proof recording.]

  Synopsis    [Core procedures of the package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: pr.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "misc/util/abc_global.h"
#include "pr.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef unsigned lit;

typedef struct Pr_Cls_t_ Pr_Cls_t;
struct Pr_Cls_t_
{
    unsigned        uTruth;       // interpolant
    void *          pProof;       // the proof node 
//    void *          pAntis;       // the anticedents
    Pr_Cls_t *      pNext;        // the next clause
    Pr_Cls_t *      pNext0;       // the next 0-watch
    Pr_Cls_t *      pNext1;       // the next 0-watch
    int             Id;           // the clause ID
    unsigned        fA     :  1;  // belongs to A
    unsigned        fRoot  :  1;  // original clause
    unsigned        fVisit :  1;  // visited clause
    unsigned        nLits  : 24;  // the number of literals
    lit             pLits[0];     // literals of this clause
};

struct Pr_Man_t_
{
    // general data
    int             fProofWrite;  // writes the proof file
    int             fProofVerif;  // verifies the proof
    int             nVars;        // the number of variables
    int             nVarsAB;      // the number of global variables
    int             nRoots;       // the number of root clauses
    int             nClauses;     // the number of all clauses
    int             nClausesA;    // the number of clauses of A 
    Pr_Cls_t *      pHead;        // the head clause
    Pr_Cls_t *      pTail;        // the tail clause
    Pr_Cls_t *      pLearnt;      // the tail clause
    Pr_Cls_t *      pEmpty;       // the empty clause
    // internal BCP
    int             nRootSize;    // the number of root level assignments
    int             nTrailSize;   // the number of assignments made 
    lit *           pTrail;       // chronological order of assignments (size nVars)
    lit *           pAssigns;     // assignments by variable (size nVars) 
    char *          pSeens;       // temporary mark (size nVars)
    char *          pVarTypes;    // variable type (size nVars) [1=A, 0=B, <0=AB]
    Pr_Cls_t **     pReasons;     // reasons for each assignment (size nVars)          
    Pr_Cls_t **     pWatches;     // watched clauses for each literal (size 2*nVars)          
    int             nVarsAlloc;   // the allocated size of arrays
    // proof recording
    void *          pManProof;    // proof manager
    int             Counter;      // counter of resolved clauses
    // memory management
    int             nChunkSize;   // the number of bytes in a chunk
    int             nChunkUsed;   // the number of bytes used in the last chunk
    char *          pChunkLast;   // the last memory chunk
    // internal verification
    lit *           pResLits;     // the literals of the resolvent   
    int             nResLits;     // the number of literals of the resolvent
    int             nResLitsAlloc;// the number of literals of the resolvent
    // runtime stats
    abctime         timeBcp;
    abctime         timeTrace;
    abctime         timeRead;
    abctime         timeTotal;
};

// variable assignments 
static const lit  LIT_UNDEF = 0xffffffff;

// variable/literal conversions (taken from MiniSat)
static inline lit   toLit    (int v)        { return v + v;  }
static inline lit   toLitCond(int v, int c) { return v + v + (c != 0); }
static inline lit   lit_neg  (lit l)        { return l ^ 1;  }
static inline int   lit_var  (lit l)        { return l >> 1; }
static inline int   lit_sign (lit l)        { return l & 1;  }
static inline int   lit_print(lit l)        { return lit_sign(l)? -lit_var(l)-1 : lit_var(l)+1; }
static inline lit   lit_read (int s)        { return s > 0 ? toLit(s-1) : lit_neg(toLit(-s-1)); }
static inline int   lit_check(lit l, int n) { return l >= 0 && lit_var(l) < n;                  }

// iterators through the clauses
#define Pr_ManForEachClause( p, pCls )        for( pCls = p->pHead; pCls; pCls = pCls->pNext )
#define Pr_ManForEachClauseRoot( p, pCls )    for( pCls = p->pHead; pCls != p->pLearnt; pCls = pCls->pNext )
#define Pr_ManForEachClauseLearnt( p, pCls )  for( pCls = p->pLearnt; pCls; pCls = pCls->pNext )

// static procedures
static char *     Pr_ManMemoryFetch( Pr_Man_t * p, int nBytes );
static void       Pr_ManMemoryStop( Pr_Man_t * p );
static void       Pr_ManResize( Pr_Man_t * p, int nVarsNew );

// exported procedures
extern Pr_Man_t * Pr_ManAlloc( int nVarsAlloc );
extern void       Pr_ManFree( Pr_Man_t * p );
extern int        Pr_ManAddClause( Pr_Man_t * p, lit * pBeg, lit * pEnd, int fRoot, int fClauseA );
extern int        Pr_ManProofWrite( Pr_Man_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocate proof manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Pr_Man_t * Pr_ManAlloc( int nVarsAlloc )
{
    Pr_Man_t * p;
    // allocate the manager
    p = (Pr_Man_t *)ABC_ALLOC( char, sizeof(Pr_Man_t) );
    memset( p, 0, sizeof(Pr_Man_t) );
    // allocate internal arrays
    Pr_ManResize( p, nVarsAlloc? nVarsAlloc : 256 );
    // set the starting number of variables
    p->nVars = 0;
    // memory management
    p->nChunkSize = (1<<16); // use 64K chunks
    // verification
    p->nResLitsAlloc = (1<<16);
    p->pResLits = ABC_ALLOC( lit, p->nResLitsAlloc );
    // parameters
    p->fProofWrite = 0;
    p->fProofVerif = 0;
    return p;    
}

/**Function*************************************************************

  Synopsis    [Resize proof manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Pr_ManResize( Pr_Man_t * p, int nVarsNew )
{
    // check if resizing is needed
    if ( p->nVarsAlloc < nVarsNew )
    {
        int nVarsAllocOld = p->nVarsAlloc;
        // find the new size
        if ( p->nVarsAlloc == 0 )
            p->nVarsAlloc = 1;
        while ( p->nVarsAlloc < nVarsNew ) 
            p->nVarsAlloc *= 2;
        // resize the arrays
        p->pTrail    = ABC_REALLOC(lit,        p->pTrail,    p->nVarsAlloc );
        p->pAssigns  = ABC_REALLOC(lit,        p->pAssigns,  p->nVarsAlloc );
        p->pSeens    = ABC_REALLOC(char,       p->pSeens,    p->nVarsAlloc );
        p->pVarTypes = ABC_REALLOC(char,       p->pVarTypes, p->nVarsAlloc );
        p->pReasons  = ABC_REALLOC(Pr_Cls_t *, p->pReasons,  p->nVarsAlloc );
        p->pWatches  = ABC_REALLOC(Pr_Cls_t *, p->pWatches,  p->nVarsAlloc*2 );
        // clean the free space
        memset( p->pAssigns  + nVarsAllocOld, 0xff, sizeof(lit) * (p->nVarsAlloc - nVarsAllocOld) );
        memset( p->pSeens    + nVarsAllocOld, 0, sizeof(char) * (p->nVarsAlloc - nVarsAllocOld) );
        memset( p->pVarTypes + nVarsAllocOld, 0, sizeof(char) * (p->nVarsAlloc - nVarsAllocOld) );
        memset( p->pReasons  + nVarsAllocOld, 0, sizeof(Pr_Cls_t *) * (p->nVarsAlloc - nVarsAllocOld) );
        memset( p->pWatches  + nVarsAllocOld, 0, sizeof(Pr_Cls_t *) * (p->nVarsAlloc - nVarsAllocOld)*2 );
    }
    // adjust the number of variables
    if ( p->nVars < nVarsNew )
        p->nVars = nVarsNew;
}

/**Function*************************************************************

  Synopsis    [Deallocate proof manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Pr_ManFree( Pr_Man_t * p )
{
    printf( "Runtime stats:\n" );
ABC_PRT( "Reading ", p->timeRead  );
ABC_PRT( "BCP     ", p->timeBcp   );
ABC_PRT( "Trace   ", p->timeTrace );
ABC_PRT( "TOTAL   ", p->timeTotal );

    Pr_ManMemoryStop( p );
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

  Synopsis    [Adds one clause to the watcher list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Pr_ManWatchClause( Pr_Man_t * p, Pr_Cls_t * pClause, lit Lit )
{
    assert( lit_check(Lit, p->nVars) );
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

  Synopsis    [Adds one clause to the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Pr_ManAddClause( Pr_Man_t * p, lit * pBeg, lit * pEnd, int fRoot, int fClauseA )
{
    Pr_Cls_t * pClause;
    lit Lit, * i, * j;
    int nSize, VarMax;

    // process the literals
    if ( pBeg < pEnd )
    {
        // insertion sort
        VarMax = lit_var( *pBeg );
        for ( i = pBeg + 1; i < pEnd; i++ )
        {
            Lit = *i;
            VarMax = lit_var(Lit) > VarMax ? lit_var(Lit) : VarMax;
            for ( j = i; j > pBeg && *(j-1) > Lit; j-- )
                *j = *(j-1);
            *j = Lit;
        }
        // make sure there is no duplicated variables
        for ( i = pBeg + 1; i < pEnd; i++ )
            assert( lit_var(*(i-1)) != lit_var(*i) );
        // resize the manager
        Pr_ManResize( p, VarMax+1 );
    }

    // get memory for the clause
    nSize = sizeof(Pr_Cls_t) + sizeof(lit) * (pEnd - pBeg);
    pClause = (Pr_Cls_t *)Pr_ManMemoryFetch( p, nSize );
    memset( pClause, 0, sizeof(Pr_Cls_t) );

    // assign the clause
    assert( !fClauseA || fRoot ); // clause of A is always a root clause
    p->nRoots += fRoot;
    p->nClausesA += fClauseA;
    pClause->Id = p->nClauses++;
    pClause->fA = fClauseA;
    pClause->fRoot = fRoot;
    pClause->nLits = pEnd - pBeg;
    memcpy( pClause->pLits, pBeg, sizeof(lit) * (pEnd - pBeg) );

    // add the clause to the list
    if ( p->pHead == NULL )
        p->pHead = pClause;
    if ( p->pTail == NULL )
        p->pTail = pClause;
    else
    {
        p->pTail->pNext = pClause;
        p->pTail = pClause;
    }

    // mark the first learnt clause
    if ( p->pLearnt == NULL && !pClause->fRoot )
        p->pLearnt = pClause;

    // add the empty clause
    if ( pClause->nLits == 0 )
    {
        if ( p->pEmpty )
            printf( "More than one empty clause!\n" );
        p->pEmpty = pClause;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Fetches memory.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Pr_ManMemoryFetch( Pr_Man_t * p, int nBytes )
{
    char * pMem;
    if ( p->pChunkLast == NULL || nBytes > p->nChunkSize - p->nChunkUsed )
    {
        pMem = (char *)ABC_ALLOC( char, p->nChunkSize );
        *(char **)pMem = p->pChunkLast;
        p->pChunkLast = pMem;
        p->nChunkUsed = sizeof(char *);
    }
    pMem = p->pChunkLast + p->nChunkUsed;
    p->nChunkUsed += nBytes;
    return pMem;
}

/**Function*************************************************************

  Synopsis    [Frees memory manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Pr_ManMemoryStop( Pr_Man_t * p )
{
    char * pMem, * pNext;
    if ( p->pChunkLast == NULL )
        return;
    for ( pMem = p->pChunkLast; pNext = *(char **)pMem; pMem = pNext )
        ABC_FREE( pMem );
    ABC_FREE( pMem );
}

/**Function*************************************************************

  Synopsis    [Reports memory usage in bytes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Pr_ManMemoryReport( Pr_Man_t * p )
{
    int Total;
    char * pMem, * pNext;
    if ( p->pChunkLast == NULL )
        return 0;
    Total = p->nChunkUsed; 
    for ( pMem = p->pChunkLast; pNext = *(char **)pMem; pMem = pNext )
        Total += p->nChunkSize;
    return Total;
}

/**Function*************************************************************

  Synopsis    [Records the proof.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_PrintBinary_( FILE * pFile, unsigned Sign[], int nBits )
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
void Pr_ManPrintInterOne( Pr_Man_t * p, Pr_Cls_t * pClause )
{
    printf( "Clause %2d :  ", pClause->Id );
    Extra_PrintBinary_( stdout, &pClause->uTruth, (1 << p->nVarsAB) );
    printf( "\n" );
}



/**Function*************************************************************

  Synopsis    [Records implication.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Pr_ManEnqueue( Pr_Man_t * p, lit Lit, Pr_Cls_t * pReason )
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
static inline void Pr_ManCancelUntil( Pr_Man_t * p, int Level )
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
static inline Pr_Cls_t * Pr_ManPropagateOne( Pr_Man_t * p, lit Lit )
{
    Pr_Cls_t ** ppPrev, * pCur, * pTemp;
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
            Pr_ManWatchClause( p, pCur, pCur->pLits[1] );
            break;
        }
        if ( i < (int)pCur->nLits ) // found new watch
            continue;

        // clause is unit - enqueue new implication
        if ( Pr_ManEnqueue(p, pCur->pLits[0], pCur) )
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
Pr_Cls_t * Pr_ManPropagate( Pr_Man_t * p, int Start )
{
    Pr_Cls_t * pClause;
    int i;
    abctime clk = Abc_Clock();
    for ( i = Start; i < p->nTrailSize; i++ )
    {
        pClause = Pr_ManPropagateOne( p, p->pTrail[i] );
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

  Synopsis    [Prints the clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Pr_ManPrintClause( Pr_Cls_t * pClause )
{
    int i;
    printf( "Clause ID = %d. Proof = %d. {", pClause->Id, (int)pClause->pProof );
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
void Pr_ManPrintResolvent( lit * pResLits, int nResLits )
{
    int i;
    printf( "Resolvent: {" );
    for ( i = 0; i < nResLits; i++ )
        printf( " %d", pResLits[i] );
    printf( " }\n" );
}

/**Function*************************************************************

  Synopsis    [Writes one root clause into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Pr_ManProofWriteOne( Pr_Man_t * p, Pr_Cls_t * pClause )
{
    pClause->pProof = (void *)++p->Counter;

    if ( p->fProofWrite )
    {
        int v;
        fprintf( (FILE *)p->pManProof, "%d", (int)pClause->pProof );
        for ( v = 0; v < (int)pClause->nLits; v++ )
            fprintf( (FILE *)p->pManProof, " %d", lit_print(pClause->pLits[v]) );
        fprintf( (FILE *)p->pManProof, " 0 0\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Traces the proof for one clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Pr_ManProofTraceOne( Pr_Man_t * p, Pr_Cls_t * pConflict, Pr_Cls_t * pFinal )
{
    Pr_Cls_t * pReason;
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

    if ( p->nClausesA )
        pFinal->uTruth = pConflict->uTruth;
   
    // follow the trail backwards
    PrevId = (int)pConflict->pProof;
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
        assert( pReason->pProof > 0 );
        p->Counter++;
        if ( p->fProofWrite )
            fprintf( (FILE *)p->pManProof, "%d * %d %d 0\n", p->Counter, PrevId, (int)pReason->pProof );
        PrevId = p->Counter;

        if ( p->nClausesA )
        {
            if ( p->pVarTypes[Var] == 1 ) // var of A
                pFinal->uTruth |= pReason->uTruth;
            else
                pFinal->uTruth &= pReason->uTruth;
        }
 
        // resolve the temporary resolvent with the reason clause
        if ( p->fProofVerif )
        {
            int v1, v2; 
            if ( fPrint )
                Pr_ManPrintResolvent( p->pResLits, p->nResLits );
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
                        printf( "Recording clause %d: Ran out of space for intermediate resolvent.\n, pFinal->Id" );
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
            Pr_ManPrintResolvent( p->pResLits, p->nResLits );
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
            Pr_ManPrintClause( pConflict );
            Pr_ManPrintResolvent( p->pResLits, p->nResLits );
            Pr_ManPrintClause( pFinal );
        }
    }
p->timeTrace += Abc_Clock() - clk;

    // return the proof pointer 
    if ( p->nClausesA )
    {
        Pr_ManPrintInterOne( p, pFinal );
    }
    return p->Counter;
}

/**Function*************************************************************

  Synopsis    [Records the proof for one clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Pr_ManProofRecordOne( Pr_Man_t * p, Pr_Cls_t * pClause )
{
    Pr_Cls_t * pConflict;
    int i;

    // empty clause never ends up there
    assert( pClause->nLits > 0 );
    if ( pClause->nLits == 0 )
        printf( "Error: Empty clause is attempted.\n" );

    // add assumptions to the trail
    assert( !pClause->fRoot );
    assert( p->nTrailSize == p->nRootSize );
    for ( i = 0; i < (int)pClause->nLits; i++ )
        if ( !Pr_ManEnqueue( p, lit_neg(pClause->pLits[i]), NULL ) )
        {
            assert( 0 ); // impossible
            return 0;
        }

    // propagate the assumptions
    pConflict = Pr_ManPropagate( p, p->nRootSize );
    if ( pConflict == NULL )
    {
        assert( 0 ); // cannot prove
        return 0;
    }

    // construct the proof
    pClause->pProof = (void *)Pr_ManProofTraceOne( p, pConflict, pClause );

    // undo to the root level
    Pr_ManCancelUntil( p, p->nRootSize );

    // add large clauses to the watched lists
    if ( pClause->nLits > 1 )
    {
        Pr_ManWatchClause( p, pClause, pClause->pLits[0] );
        Pr_ManWatchClause( p, pClause, pClause->pLits[1] );
        return 1;
    }
    assert( pClause->nLits == 1 );

    // if the clause proved is unit, add it and propagate
    if ( !Pr_ManEnqueue( p, pClause->pLits[0], pClause ) )
    {
        assert( 0 ); // impossible
        return 0;
    }

    // propagate the assumption
    pConflict = Pr_ManPropagate( p, p->nRootSize );
    if ( pConflict )
    {
        // construct the proof
        p->pEmpty->pProof = (void *)Pr_ManProofTraceOne( p, pConflict, p->pEmpty );
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
int Pr_ManProcessRoots( Pr_Man_t * p )
{
    Pr_Cls_t * pClause;
    int Counter;

    // make sure the root clauses are preceeding the learnt clauses
    Counter = 0;
    Pr_ManForEachClause( p, pClause )
    {
        assert( (int)pClause->fA    == (Counter < (int)p->nClausesA) );
        assert( (int)pClause->fRoot == (Counter < (int)p->nRoots)    );
        Counter++;
    }
    assert( p->nClauses == Counter );

    // make sure the last clause if empty
    assert( p->pTail->nLits == 0 );

    // go through the root unit clauses
    p->nTrailSize = 0;
    Pr_ManForEachClauseRoot( p, pClause )
    {
        // create watcher lists for the root clauses
        if ( pClause->nLits > 1 )
        {
            Pr_ManWatchClause( p, pClause, pClause->pLits[0] );
            Pr_ManWatchClause( p, pClause, pClause->pLits[1] );
        }
        // empty clause and large clauses
        if ( pClause->nLits != 1 )
            continue;
        // unit clause
        assert( lit_check(pClause->pLits[0], p->nVars) );
        if ( !Pr_ManEnqueue( p, pClause->pLits[0], pClause ) )
        {
            // detected root level conflict
            printf( "Pr_ManProcessRoots(): Detected a root-level conflict\n" );
            assert( 0 );
            return 0;
        }
    }

    // propagate the root unit clauses
    pClause = Pr_ManPropagate( p, 0 );
    if ( pClause )
    {
        // detected root level conflict
        printf( "Pr_ManProcessRoots(): Detected a root-level conflict\n" );
        assert( 0 );
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
void Pr_ManPrepareInter( Pr_Man_t * p )
{
    unsigned uTruths[5] = { 0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000 };
    Pr_Cls_t * pClause;
    int Var, v;

    // mark the variable encountered in the clauses of A
    Pr_ManForEachClauseRoot( p, pClause )
    {
        if ( !pClause->fA )
            break;
        for ( v = 0; v < (int)pClause->nLits; v++ )
            p->pVarTypes[lit_var(pClause->pLits[v])] = 1;
    }

    // check variables that appear in clauses of B
    p->nVarsAB = 0;
    Pr_ManForEachClauseRoot( p, pClause )
    {
        if ( pClause->fA )
            continue;
        for ( v = 0; v < (int)pClause->nLits; v++ )
        {
            Var = lit_var(pClause->pLits[v]);
            if ( p->pVarTypes[Var] == 1 ) // var of A
            {
                // change it into a global variable
                p->nVarsAB++;
                p->pVarTypes[Var] = -1;
            }
        }
    }

    // order global variables
    p->nVarsAB = 0;
    for ( v = 0; v < p->nVars; v++ )
        if ( p->pVarTypes[v] == -1 )
            p->pVarTypes[v] -= p->nVarsAB++;
printf( "There are %d global variables.\n", p->nVarsAB );

    // set interpolants for root clauses
    Pr_ManForEachClauseRoot( p, pClause )
    {
        if ( !pClause->fA ) // clause of B
        {
            pClause->uTruth = ~0;
            Pr_ManPrintInterOne( p, pClause );
            continue;
        }
        // clause of A
        pClause->uTruth = 0;
        for ( v = 0; v < (int)pClause->nLits; v++ )
        {
            Var = lit_var(pClause->pLits[v]);
            if ( p->pVarTypes[Var] < 0 ) // global var
            {
                if ( lit_sign(pClause->pLits[v]) ) // negative var
                    pClause->uTruth |= ~uTruths[ -p->pVarTypes[Var]-1 ];
                else
                    pClause->uTruth |= uTruths[ -p->pVarTypes[Var]-1 ];
            }
        }
        Pr_ManPrintInterOne( p, pClause );
    }
}

/**Function*************************************************************

  Synopsis    [Records the proof.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Pr_ManProofWrite( Pr_Man_t * p )
{
    Pr_Cls_t * pClause;
    int RetValue = 1;

    // propagate root level assignments
    Pr_ManProcessRoots( p );

    // prepare the interpolant computation
    if ( p->nClausesA )
        Pr_ManPrepareInter( p );

    // construct proof for each clause
    // start the proof
    if ( p->fProofWrite )
        p->pManProof = fopen( "proof.cnf_", "w" );
    p->Counter = 0;

    // write the root clauses
    Pr_ManForEachClauseRoot( p, pClause )
        Pr_ManProofWriteOne( p, pClause );

    // consider each learned clause
    Pr_ManForEachClauseLearnt( p, pClause )
    {
        if ( !Pr_ManProofRecordOne( p, pClause ) )
        {
            RetValue = 0;
            break;
        }
    }

    if ( p->nClausesA )
    {
        printf( "Interpolant:  " );
    }


    // stop the proof
    if ( p->fProofWrite )
    {
        fclose( (FILE *)p->pManProof );
        p->pManProof = NULL;    
    }
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Reads clauses from file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Pr_Man_t * Pr_ManProofRead( char * pFileName )
{
    Pr_Man_t * p = NULL;
    char * pCur, * pBuffer = NULL;
    int * pArray = NULL;
    FILE * pFile;
    int RetValue, Counter, nNumbers, Temp;
    int nClauses, nClausesA, nRoots, nVars;

    // open the file
    pFile = fopen( pFileName, "r" );
    if ( pFile == NULL )
    {
        printf( "Count not open input file \"%s\".\n", pFileName );
        return NULL;
    }

    // read the file
    pBuffer = (char *)ABC_ALLOC( char, (1<<16) );
    for ( Counter = 0; fgets( pBuffer, (1<<16), pFile ); )
    {
        if ( pBuffer[0] == 'c' )
            continue;
        if ( pBuffer[0] == 'p' )
        {
            assert( p == NULL );
            nClausesA = 0;
            RetValue = sscanf( pBuffer + 1, "%d %d %d %d", &nVars, &nClauses, &nRoots, &nClausesA );
            if ( RetValue != 3 && RetValue != 4 )
            {
                printf( "Wrong input file format.\n" );
            }
            p = Pr_ManAlloc( nVars );
            pArray = (int *)ABC_ALLOC( char, sizeof(int) * (nVars + 10) );
            continue;
        }
        // skip empty lines
        for ( pCur = pBuffer; *pCur; pCur++ )
            if ( !(*pCur == ' ' || *pCur == '\t' || *pCur == '\r' || *pCur == '\n') )
                break;
        if ( *pCur == 0 )
            continue;
        // scan the numbers from file
        nNumbers = 0;
        pCur = pBuffer;
        while ( *pCur )
        {
            // skip spaces
            for ( ; *pCur && *pCur == ' '; pCur++ );
            // read next number
            Temp = 0;
            sscanf( pCur, "%d", &Temp );
            if ( Temp == 0 )
                break;
            pArray[ nNumbers++ ] = lit_read( Temp );
            // skip non-spaces
            for ( ; *pCur && *pCur != ' '; pCur++ );
        }
        // add the clause
        if ( !Pr_ManAddClause( p, (unsigned *)pArray, (unsigned *)pArray + nNumbers, Counter < nRoots, Counter < nClausesA ) )
        {
            printf( "Bad clause number %d.\n", Counter );
            return NULL;
        }
        // count the clauses
        Counter++;
    }
    // check the number of clauses
    if ( Counter != nClauses )
        printf( "Expected %d clauses but read %d.\n", nClauses, Counter );

    // finish
    if ( pArray ) ABC_FREE( pArray );
    if ( pBuffer ) ABC_FREE( pBuffer );
    fclose( pFile );
    return p;
}

/**Function*************************************************************

  Synopsis    [Records the proof.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
/*
int Pr_ManProofCount_rec( Pr_Cls_t * pClause )
{
    Pr_Cls_t * pNext;
    int i, Counter;    
    if ( pClause->fRoot )
        return 0;
    if ( pClause->fVisit )
        return 0;
    pClause->fVisit = 1;
    // count the number of visited clauses
    Counter = 1;
    Vec_PtrForEachEntry( Pr_Cls_t *, pClause->pAntis, pNext, i )
        Counter += Pr_ManProofCount_rec( pNext );
    return Counter;
}
*/

/**Function*************************************************************

  Synopsis    [Records the proof.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Pr_ManProofTest( char * pFileName )
{
    Pr_Man_t * p;
    abctime clk, clkTotal = Abc_Clock();

clk = Abc_Clock();
    p = Pr_ManProofRead( pFileName );
p->timeRead = Abc_Clock() - clk;
    if ( p == NULL )
        return 0;

    Pr_ManProofWrite( p );

    // print stats
/*
    nUsed = Pr_ManProofCount_rec( p->pEmpty );
    printf( "Roots = %d. Learned = %d. Total = %d. Steps = %d.  Ave = %.2f.  Used = %d. Ratio = %.2f. \n", 
        p->nRoots, p->nClauses-p->nRoots, p->nClauses, p->Counter,  
        1.0*(p->Counter-p->nRoots)/(p->nClauses-p->nRoots),
        nUsed, 1.0*nUsed/(p->nClauses-p->nRoots)  );
*/
    printf( "Vars = %d. Roots = %d. Learned = %d. Resol steps = %d.  Ave = %.2f.  Mem = %.2f MB\n", 
        p->nVars, p->nRoots, p->nClauses-p->nRoots, p->Counter,  
        1.0*(p->Counter-p->nRoots)/(p->nClauses-p->nRoots), 
        1.0*Pr_ManMemoryReport(p)/(1<<20) );

p->timeTotal = Abc_Clock() - clkTotal;
    Pr_ManFree( p );
    return 1;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

